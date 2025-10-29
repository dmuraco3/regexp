#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


LiteralNode* create_literal_node(char value) {
    LiteralNode* node = malloc(sizeof(LiteralNode));
    node->base.type = NODE_LITERAL;
    node->value = value;
    return node;
}

AlternationNode* create_alternation_node(AstNode *left, AstNode *right) {
    AlternationNode* node = malloc(sizeof(AlternationNode));
    node->base.type = NODE_ALTERNATION;
    node->left = left;
    node->right = right;
    return node;
}

ConcatNode* create_concat_node(AstNode *left, AstNode *right) {
    ConcatNode* node = malloc(sizeof(ConcatNode));
    node->base.type = NODE_CONCAT;
    node->left = left;
    node->right = right;
    return node;
}

QuantifierNode* create_quantifier_node(AstNode *child, char quantifier) {
    QuantifierNode* node = malloc(sizeof(QuantifierNode));
    node->base.type = NODE_QUANTIFIER;
    node->quantifier = quantifier;
    node->child = child;
    return node;
}

WildcardNode* create_wildcard_node() {
    WildcardNode* node = malloc(sizeof(WildcardNode));
    node->base.type = NODE_WILDCARD;
    return node;
}

CharClassNode* create_char_class_node(bool negated) {
    CharClassNode* node = malloc(sizeof(CharClassNode));
    node->base.type = NODE_CHAR_CLASS;
    node->negated = negated;
    // Initialize all characters to false (not in set)
    for (int i = 0; i < 256; i++) {
        node->char_set[i] = false;
    }
    return node;
}

AstNode* parse_alternation(ParserState *state) {
    AstNode *left = parse_concatenation(state);

    if (state->input[state->index] == '|') {
        state->index++; // consume '|'

        AstNode *right = parse_alternation(state);

        AlternationNode *node = create_alternation_node(left, right);
        return (AstNode*)node;
    }

    return left;
}

AstNode* parse_concatenation(ParserState *state) {
    AstNode *left = parse_quantifier(state);

    while(state->input[state->index] != '\0' && state->input[state->index] != '|' && state->input[state->index] != ')') {
        AstNode *right = parse_quantifier(state);

        left = (AstNode*) create_concat_node(left, right);
    }

    return left;
}

AstNode* parse_quantifier(ParserState *state) {
    AstNode *child = parse_atom(state);

    char q = state->input[state->index];
    if (q == '*' || q == '+' || q == '?') {
        state->index++; // consume quantifier

        QuantifierNode *node = create_quantifier_node(child, q);
        return (AstNode*)node;
    }

    return child;
}

AstNode* parse_char_class(ParserState *state) {
    // We're at the opening '['
    state->index++; // consume '['
    
    bool negated = false;
    if (state->input[state->index] == '^') {
        negated = true;
        state->index++; // consume '^'
    }
    
    CharClassNode* node = create_char_class_node(negated);
    
    // Parse characters until we hit ']'
    while (state->input[state->index] != '\0' && state->input[state->index] != ']') {
        char current = state->input[state->index];
        
        // Handle escape sequences within character class
        if (current == '\\') {
            state->index++; // consume backslash
            if (state->input[state->index] == '\0') {
                fprintf(stderr, "parse_char_class  Error: unexpected end of input after backslash\n");
                free(node);
                exit(1);
            }
            current = state->input[state->index];
            node->char_set[(unsigned char)current] = true;
            state->index++;
        }
        // Check for range (e.g., a-z)
        else if (state->input[state->index + 1] == '-' && 
                 state->input[state->index + 2] != ']' && 
                 state->input[state->index + 2] != '\0') {
            char start = current;
            state->index += 2; // skip current char and '-'
            char end = state->input[state->index];
            
            if (end == '\\') {
                state->index++; // consume backslash
                end = state->input[state->index];
            }
            
            if (start > end) {
                fprintf(stderr, "parse_char_class  Error: invalid range %c-%c\n", start, end);
                free(node);
                exit(1);
            }
            
            // Add all characters in the range
            for (int c = start; c <= end; c++) {
                node->char_set[c] = true;
            }
            state->index++;
        }
        // Regular character
        else {
            node->char_set[(unsigned char)current] = true;
            state->index++;
        }
    }
    
    if (state->input[state->index] != ']') {
        fprintf(stderr, "parse_char_class  Error: unmatched '['\n");
        free(node);
        exit(1);
    }
    
    state->index++; // consume ']'
    
    return (AstNode*)node;
}

AstNode* parse_atom(ParserState *state) {
    char c = state->input[state->index];

    // Handle escape sequences
    if (c == '\\') {
        state->index++; // consume backslash
        char escaped_char = state->input[state->index];
        
        if (escaped_char == '\0') {
            fprintf(stderr, "parse_atom  Error: unexpected end of input after backslash at position %d\n", state->index - 1);
            exit(1);
        }
        
        // Check for shorthand character classes
        if (escaped_char == 'd' || escaped_char == 'D') {
            // \d matches [0-9], \D matches [^0-9]
            state->index++; // consume 'd' or 'D'
            bool negated = (escaped_char == 'D');
            CharClassNode* node = create_char_class_node(negated);
            for (int i = '0'; i <= '9'; i++) {
                node->char_set[i] = true;
            }
            return (AstNode*)node;
        }
        else if (escaped_char == 'w' || escaped_char == 'W') {
            // \w matches [a-zA-Z0-9_], \W matches [^a-zA-Z0-9_]
            state->index++; // consume 'w' or 'W'
            bool negated = (escaped_char == 'W');
            CharClassNode* node = create_char_class_node(negated);
            for (int i = 'a'; i <= 'z'; i++) {
                node->char_set[i] = true;
            }
            for (int i = 'A'; i <= 'Z'; i++) {
                node->char_set[i] = true;
            }
            for (int i = '0'; i <= '9'; i++) {
                node->char_set[i] = true;
            }
            node->char_set['_'] = true;
            return (AstNode*)node;
        }
        else if (escaped_char == 's' || escaped_char == 'S') {
            // \s matches whitespace [ \t\n\r\f\v], \S matches [^ \t\n\r\f\v]
            state->index++; // consume 's' or 'S'
            bool negated = (escaped_char == 'S');
            CharClassNode* node = create_char_class_node(negated);
            node->char_set[' '] = true;   // space
            node->char_set['\t'] = true;  // tab
            node->char_set['\n'] = true;  // newline
            node->char_set['\r'] = true;  // carriage return
            node->char_set['\f'] = true;  // form feed
            node->char_set['\v'] = true;  // vertical tab
            return (AstNode*)node;
        }
        
        // The escaped character is treated as a literal
        state->index++; // consume the escaped character
        return (AstNode*)create_literal_node(escaped_char);
    }

    // Handle character classes
    if (c == '[') {
        return parse_char_class(state);
    }

    if (c == '(') {
        state->index++;

        AstNode *node = parse_alternation(state);

        if(state->input[state->index] != ')') {
            fprintf(stderr, "parse_atom  Error: unmatched parenthesis\n");
            exit(1);
        }
        state->index++;
        return node;
    }
    if(c == '.') {
        state->index++;
        return (AstNode*)create_wildcard_node();
    }
    if(c == '*' || c == '+' || c == '?' || c == '|' || c == ')' || c == ']' || c == '\0') {
        fprintf(stderr, "parse_atom  Error: unexpected character '%c' at position %d\n", c, state->index);
        exit(1);
    }

    state->index++;

    return (AstNode*)create_literal_node(c);
}

AstNode* parse(const char *input) {
    if (input == NULL) {
        return NULL;
    }

    char *input_buf = input;

    size_t last_idx = strlen(input) - 1;
    if(input[0] != '^' && input[last_idx] != '$') {
        size_t new_size = strlen(input) + 2;
        if(input[0] != '^') {
            new_size += 2;
        }
        if(input[last_idx] != '$') {
            new_size += 2;
        }
        new_size += 1;
        input_buf = calloc(new_size, sizeof(char));
        if(input_buf == NULL) {
            fprintf(stderr, "parse  Error: failed to allocate input buffer\n");
            return NULL;
        }
        if(input[0] != '^') {
            strncpy(input_buf, ".*(", 3);
            strcat(input_buf, input);
        } else {
            strcpy(input_buf, input);
        }
        if(input[last_idx] != '$') {
            strcat(input_buf, ").*");
        }
    }
    else {
        size_t new_size = strlen(input) + 1;
        if(input[0] == '^') {
            new_size -= 1;
        }
        if(input[last_idx] == '$') {
            new_size -= 1;
        }
        input_buf = calloc(new_size, sizeof(char));
        if(input_buf == NULL) {
            fprintf(stderr, "parse  Error: failed to allocate input buffer\n");
            return NULL;
        }
        size_t start_idx = (input[0] == '^') ? 1 : 0;
        size_t end_idx = (input[last_idx] == '$') ? last_idx : strlen(input);
        strncpy(input_buf, input + start_idx, end_idx - start_idx);
        input_buf[end_idx - start_idx] = '\0';
    }
    printf("new input_buf: %s\n", input_buf);

    ParserState state;
    state.input = input_buf;
    state.index = 0;

    AstNode *root = parse_alternation(&state);

    if (root == NULL) {
        return NULL;
    }

    if(state.input[state.index] != '\0') {
        fprintf(stderr, "parse  Error: unexpected character '%c' at position %d\n", state.input[state.index], state.index);
        free(input_buf);
        free_ast(root);
        return NULL;
    }

    free(input_buf);

    return root;
}

void free_ast(AstNode *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case NODE_LITERAL:
            break;
        case NODE_CONCAT:
        case NODE_ALTERNATION: {
            AlternationNode *bin_node = (AlternationNode*)node;
            free_ast(bin_node->left);
            free_ast(bin_node->right);
            break;
        }
        case NODE_QUANTIFIER: {
            QuantifierNode *q_node = (QuantifierNode*)node;
            free_ast(q_node->child);
            break;
        }
        case NODE_WILDCARD:
            break;
        case NODE_CHAR_CLASS:
            break;
    }

    free(node);
}

static void print_ast_recursive(AstNode *node, char *prefix, bool is_last) {
    if (node == NULL) {
        return; // Safety check
    }

    // 1. Print the current node's line
    printf("%s", prefix); // Print the accumulated prefix (lines and spaces)
    printf(is_last ? "└── " : "├── "); // Use the correct branch connector

    // Print the node information
    switch (node->type) {
        case NODE_CONCAT:
            printf("CONCAT\n");
            break;
        case NODE_LITERAL:
            printf("LITERAL('%c')\n", ((LiteralNode*)node)->value);
            break;
        case NODE_QUANTIFIER:
            printf("QUANTIFIER('%c')\n", ((QuantifierNode*)node)->quantifier);
            break;
        case NODE_ALTERNATION:
            printf("ALTERNATION\n");
            break;
        case NODE_WILDCARD:
            printf("WILDCARD(.)\n");
            break;
        case NODE_CHAR_CLASS: {
            CharClassNode* cc_node = (CharClassNode*)node;
            printf("CHAR_CLASS%s[", cc_node->negated ? "(negated)" : "");
            bool first = true;
            for (int i = 0; i < 256; i++) {
                if (cc_node->char_set[i]) {
                    if (!first) printf(",");
                    if (i >= 32 && i < 127) {
                        printf("%c", i);
                    } else {
                        printf("\\x%02x", i);
                    }
                    first = false;
                }
            }
            printf("]\n");
            break;
        }
    }

    // 2. Prepare the prefix for the children
    char child_prefix[256]; // Buffer for the next level's prefix
    strcpy(child_prefix, prefix);
    strcat(child_prefix, is_last ? "    " : "│   "); // Add space or line based on if current node is last

    // 3. Recurse for children, correctly identifying the last child
    switch (node->type) {
        case NODE_WILDCARD:
        case NODE_LITERAL:
        case NODE_CHAR_CLASS:
            // No children
            break;
        case NODE_CONCAT:
        case NODE_ALTERNATION: {
            // These nodes have two children (left and right)
            AlternationNode *bin_node = (AlternationNode*)node;
            // The left child is never the last one in a binary node display
            print_ast_recursive(bin_node->left, child_prefix, false);
            // The right child is always the last one
            print_ast_recursive(bin_node->right, child_prefix, true);
            break;
        }
        case NODE_QUANTIFIER: {
            // Quantifier nodes have only one child
            QuantifierNode *q_node = (QuantifierNode*)node;
            // The single child is always the last (and only) child
            print_ast_recursive(q_node->child, child_prefix, true);
            break;
        }
    }
}

void print_ast(AstNode *root) {
    if (root != NULL) {
        // Start recursion with an empty prefix, root is considered "last" initially
        print_ast_recursive(root, "", true);
    } else {
        printf("(Empty AST)\n");
    }
}
