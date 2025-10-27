#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

typedef enum {
    NODE_LITERAL,
    NODE_CONCAT,
    NODE_ALTERNATION,
    NODE_QUANTIFIER,
    NODE_WILDCARD
} NodeType;

typedef struct AstNode {
    NodeType type;
} AstNode;

typedef struct {
    AstNode base;
    char value;
} LiteralNode;

typedef struct {
    AstNode base;
    AstNode *left;
    AstNode *right;
} AlternationNode;

typedef struct {
    AstNode base;
    AstNode *left;
    AstNode *right;
} ConcatNode;

typedef struct {
    AstNode base;
    char quantifier;
    AstNode *child;
} QuantifierNode;

typedef struct {
    AstNode base;
} WildcardNode;

LiteralNode* create_literal_node(char value);
AlternationNode* create_alternation_node(AstNode *left, AstNode *right);
ConcatNode* create_concat_node(AstNode *left, AstNode *right);
QuantifierNode* create_quantifier_node(AstNode *child, char quantifier);
WildcardNode* create_wildcard_node();


typedef struct {
    const char *input;
    int index;
} ParserState;

AstNode* parse_atom(ParserState *state);
AstNode* parse_alternation(ParserState *state);
AstNode* parse_concatenation(ParserState *state);
AstNode* parse_quantifier(ParserState *state);
AstNode* parse(const char *input);

void free_ast(AstNode *node);

void print_ast(AstNode *node);

#endif //REGEX_PARSER_H
