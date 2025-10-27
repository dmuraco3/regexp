//
// Created by Dylan Muraco on 10/24/25.
//

#include <stdlib.h>
#include "compiler.h"

#include <stdio.h>

static NfaState* create_state(bool is_accepting, unsigned long *next_state_id) {
    NfaState *state = malloc(sizeof(NfaState));
    if(state == NULL) {
        fprintf(stderr, "create_state  Error: failed to allocate NfaState\n");
        exit(1);
    }
    state->id = (*next_state_id)++;
    state->is_accepting = is_accepting;
    state->out1 = NULL;
    state->out2 = NULL;

    return state;
}

static Transition* create_transition(char symbol, NfaState *to) {
    Transition *trans = malloc(sizeof(Transition));
    if(trans == NULL) {
        fprintf(stderr, "create_transition  Error: failed to allocate Transition\n");
        exit(1);
    }
    trans->symbol = symbol;
    trans->to = to;
    return trans;
}

NfaFragment create_literal_fragment(char symbol, unsigned long *next_state_id) {
    NfaState *accept_state = create_state(true, next_state_id);
    NfaState *start_state = create_state(false, next_state_id);

    start_state->out1 = create_transition(symbol, accept_state);

    NfaFragment fragment;
    fragment.start = start_state;
    fragment.accept = accept_state;

    return fragment;
}

NfaFragment create_wildcard_fragment(unsigned long *next_state_id) {
    NfaState *accept_state = create_state(true, next_state_id);
    NfaState *start_state = create_state(false, next_state_id);

    start_state->out1 = create_transition(ANY_CHAR, accept_state);

    NfaFragment fragment;
    fragment.start = start_state;
    fragment.accept = accept_state;

    return fragment;
}

NfaFragment create_concat_fragment(NfaFragment frag1, NfaFragment frag2) {
    frag1.accept->is_accepting = false;
    frag1.accept->out1 = create_transition(EPSILON, frag2.start);

    NfaFragment fragment;
    fragment.start = frag1.start;
    fragment.accept = frag2.accept;

    return fragment;
}

NfaFragment create_alternation_fragment(NfaFragment frag1, NfaFragment frag2, unsigned long *next_state_id) {
    NfaState *start_state = create_state(false, next_state_id);
    NfaState *accept_state = create_state(true, next_state_id);

    start_state->out1 = create_transition(EPSILON, frag1.start);
    start_state->out2 = create_transition(EPSILON, frag2.start);

    frag1.accept->is_accepting = false;
    frag2.accept->is_accepting = false;

    frag1.accept->out1 = create_transition(EPSILON, accept_state);
    frag2.accept->out1 = create_transition(EPSILON, accept_state);

    NfaFragment fragment;
    fragment.start = start_state;
    fragment.accept = accept_state;

    return fragment;
}

NfaFragment create_star_fragment(NfaFragment frag, unsigned long *next_state_id) {
    NfaState *start_state = create_state(false, next_state_id);
    NfaState *accept_state = create_state(true, next_state_id);

    frag.accept->is_accepting = false;

    start_state->out1 = create_transition(EPSILON, accept_state);
    start_state->out2 = create_transition(EPSILON, frag.start);

    frag.accept->out1 = create_transition(EPSILON, frag.start);
    frag.accept->out2 = create_transition(EPSILON, accept_state);

    NfaFragment fragment;
    fragment.start = start_state;
    fragment.accept = accept_state;

    return fragment;
}

NfaFragment create_plus_fragment(NfaFragment frag, unsigned long *next_state_id) {
    NfaState *start_state = create_state(false, next_state_id);
    NfaState *accept_state = create_state(true, next_state_id);

    frag.accept->is_accepting = false;

    start_state->out1 = create_transition(EPSILON, frag.start);

    frag.accept->out1 = create_transition(EPSILON, frag.start);
    frag.accept->out2 = create_transition(EPSILON, accept_state);

    NfaFragment fragment;
    fragment.start = start_state;
    fragment.accept = accept_state;

    return fragment;
}

NfaFragment create_option_fragment(NfaFragment frag, unsigned long *next_state_id) {
    NfaState *start_state = create_state(false, next_state_id);
    NfaState *accept_state = create_state(true, next_state_id);

    frag.accept->is_accepting = false;

    start_state->out1 = create_transition(EPSILON, accept_state);
    start_state->out2 = create_transition(EPSILON, frag.start);

    frag.accept->out1 = create_transition(EPSILON, accept_state);

    NfaFragment fragment;
    fragment.start = start_state;
    fragment.accept = accept_state;

    return fragment;
}

static NfaFragment recursive_compile_ast(AstNode *node, unsigned long *next_state_id) {
    if(node == NULL) {
        fprintf(stderr, "compile_ast  Error: NULL AST node\n");
        exit(1);
    }

    NfaFragment frag;

    switch(node->type) {
        case NODE_LITERAL: {
            LiteralNode *literal_node = (LiteralNode *)node;
            frag = create_literal_fragment(literal_node->value, next_state_id);
            break;
        }
        case NODE_CONCAT: {
            ConcatNode *concat_node = (ConcatNode *)node;
            NfaFragment left_frag = recursive_compile_ast(concat_node->left, next_state_id);
            NfaFragment right_frag = recursive_compile_ast(concat_node->right, next_state_id);
            frag = create_concat_fragment(left_frag, right_frag);
            break;
        }
        case NODE_ALTERNATION: {
            AlternationNode *alt_node = (AlternationNode *)node;
            NfaFragment left_frag = recursive_compile_ast(alt_node->left, next_state_id);
            NfaFragment right_frag = recursive_compile_ast(alt_node->right, next_state_id);
            frag = create_alternation_fragment(left_frag, right_frag, next_state_id);
            break;
        }
        case NODE_QUANTIFIER: {
            QuantifierNode *quant_node = (QuantifierNode *)node;
            NfaFragment child_frag = recursive_compile_ast(quant_node->child, next_state_id);
            switch(quant_node->quantifier) {
                case '*':
                    frag = create_star_fragment(child_frag, next_state_id);
                    break;
                case '+':
                    frag = create_plus_fragment(child_frag, next_state_id);
                    break;
                case '?':
                    frag = create_option_fragment(child_frag, next_state_id);
                    break;
                default:
                    fprintf(stderr, "compile_ast  Error: unknown quantifier '%c'\n", quant_node->quantifier);
                    frag = child_frag;
                    break;
            }
            break;
        }
        case NODE_WILDCARD: {
            frag = create_wildcard_fragment(next_state_id);
            break;
        }
        default:
            frag = create_literal_fragment('\0', next_state_id);
            break;
    }

    return frag;
}

NfaFragment compile_ast(AstNode *node) {
    unsigned long next_state_id = 0;
    return recursive_compile_ast(node, &next_state_id);
}

void free_nfa(NfaState *start) {
    if (start == NULL) {
        return;
    }

    // Use a simple stack for DFS traversal
    NfaState **stack = malloc(1024 * sizeof(NfaState*));
    if (stack == NULL) {
        return;
    }
    int stack_size = 0;

    // Use a dynamically allocated visited array to track visited states
    size_t visited_capacity = 1024;
    bool *visited = calloc(visited_capacity, sizeof(bool));
    if (visited == NULL) {
        free(stack);
        return;
    }

    stack[stack_size++] = start;

    while (stack_size > 0) {
        NfaState *state = stack[--stack_size];

        if (state == NULL) {
            continue;
        }

        // Expand visited array if needed
        if (state->id >= visited_capacity) {
            size_t new_capacity = visited_capacity * 2;
            while (state->id >= new_capacity) {
                new_capacity *= 2;
            }
            bool *new_visited = realloc(visited, new_capacity * sizeof(bool));
            if (new_visited == NULL) {
                free(visited);
                free(stack);
                return;
            }
            // Initialize new portion to false
            for (size_t i = visited_capacity; i < new_capacity; i++) {
                new_visited[i] = false;
            }
            visited = new_visited;
            visited_capacity = new_capacity;
        }

        if (visited[state->id]) {
            continue;
        }
        visited[state->id] = true;

        // Push successor states before freeing transitions
        if (state->out1 != NULL) {
            if (state->out1->to != NULL) {
                stack[stack_size++] = state->out1->to;
            }
            free(state->out1);
        }
        if (state->out2 != NULL) {
            if (state->out2->to != NULL) {
                stack[stack_size++] = state->out2->to;
            }
            free(state->out2);
        }

        free(state);
    }

    free(visited);
    free(stack);
}

static void print_nfa_recursive(NfaState *state, bool **visited_ptr, size_t *allocated_size) {
    // Base case: NULL state
    if (state == NULL) {
        return;
    }

    // --- Dynamic Resizing Logic ---
    // Check if the current state ID fits in the allocated array
    if (state->id >= *allocated_size) {
        size_t new_size = *allocated_size;
        // Double the size until it's large enough
        while (state->id >= new_size) {
            new_size = (new_size == 0) ? 16 : new_size * 2; // Start small or double
        }

        // Attempt to reallocate (remembering the original pointer)
        bool *new_visited = (bool*)realloc(*visited_ptr, new_size * sizeof(bool));
        if (new_visited == NULL) {
            perror("Failed to realloc visited array for printing");
            // Can't continue printing reliably without tracking
            return;
        }

        // Realloc succeeded, update the pointer and size
        *visited_ptr = new_visited;

        // IMPORTANT: Initialize the *newly* allocated portion to false
        // Use calloc-like behavior: initialize from old size up to new size
        for (size_t i = *allocated_size; i < new_size; ++i) {
             (*visited_ptr)[i] = false;
        }

        *allocated_size = new_size;
    }
    // --- End Resizing Logic ---


    // Base case: Already visited
    // Use the potentially updated visited_ptr
    if ((*visited_ptr)[state->id]) {
        return;
    }

    // Mark as visited
    (*visited_ptr)[state->id] = true;

    // 1. Print current state info (same as before)
    printf("State %lu:", state->id);
    if (state->is_accepting) {
        printf(" (ACCEPTING)");
    }
    printf("\n");

    // 2. Print outgoing transitions and recurse
    // Pass the pointers for visited and allocated_size
    if (state->out1 != NULL) {
        printf("  -> ");
        if (state->out1->symbol == EPSILON) {
            printf("ε");
        } else {
            printf("'%c'", state->out1->symbol);
        }
        printf(" to State %lu\n", state->out1->to ? state->out1->to->id : -1);
    }

    if (state->out2 != NULL) {
        printf("  -> ");
        if (state->out2->symbol == EPSILON) {
            printf("ε");
        } else {
            printf("'%c'", state->out2->symbol);
        }
        printf(" to State %lu\n", state->out2->to ? state->out2->to->id : -1);
    }
    if(state->out1 != NULL) {
        print_nfa_recursive(state->out1->to, visited_ptr, allocated_size); // Pass pointers
    }
    if(state->out2 != NULL) {
        print_nfa_recursive(state->out2->to, visited_ptr, allocated_size); // Pass pointers
    }
}

void print_nfa(NfaState *start_state) {
    if (start_state == NULL) {
        printf("NFA is empty.\n");
        return;
    }

    // Start with a small initial allocation (or NULL)
    size_t allocated_size = 1;
    bool *visited = NULL; // Start with NULL, let the first access trigger allocation

    printf("--- NFA Structure ---\n");
    // Pass pointers so the recursive function can update them
    print_nfa_recursive(start_state, &visited, &allocated_size);
    printf("---------------------\n");

    // Free the final allocated array
    free(visited);
}
