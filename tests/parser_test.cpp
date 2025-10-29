#include <gtest/gtest.h>

extern "C" {
    #include <regexp.h>
}

TEST(ParserAST, HandlesNull) {
    AstNode* tree = parse(nullptr);

    ASSERT_EQ(tree, nullptr);
}

TEST(ParserAST, ParsesSingleLiteral) {
    AstNode* tree = parse("^a$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_LITERAL);

    LiteralNode* literal = reinterpret_cast<LiteralNode *>(tree);
    ASSERT_EQ(literal->value, 'a');

    free_ast(tree);
}

TEST(ParserAST, Prints) {
    AstNode* tree = parse("a(b|c)*");

    ASSERT_NE(tree, nullptr);
    print_ast(tree);
}

TEST(ParserAST, ParsesEscapedStar) {
    AstNode* tree = parse("^a\\*b$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CONCAT);

    // Tree structure is left-associative: ((a *) b)
    ConcatNode* concat1 = reinterpret_cast<ConcatNode*>(tree);
    ASSERT_EQ(concat1->left->type, NODE_CONCAT);
    
    ConcatNode* concat2 = reinterpret_cast<ConcatNode*>(concat1->left);
    ASSERT_EQ(concat2->left->type, NODE_LITERAL);
    LiteralNode* literal_a = reinterpret_cast<LiteralNode*>(concat2->left);
    ASSERT_EQ(literal_a->value, 'a');
    
    ASSERT_EQ(concat2->right->type, NODE_LITERAL);
    LiteralNode* literal_star = reinterpret_cast<LiteralNode*>(concat2->right);
    ASSERT_EQ(literal_star->value, '*');

    ASSERT_EQ(concat1->right->type, NODE_LITERAL);
    LiteralNode* literal_b = reinterpret_cast<LiteralNode*>(concat1->right);
    ASSERT_EQ(literal_b->value, 'b');

    free_ast(tree);
}

TEST(ParserAST, ParsesEscapedPlus) {
    AstNode* tree = parse("^\\+$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_LITERAL);

    LiteralNode* literal = reinterpret_cast<LiteralNode*>(tree);
    ASSERT_EQ(literal->value, '+');

    free_ast(tree);
}

TEST(ParserAST, ParsesEscapedDot) {
    AstNode* tree = parse("^a\\.b$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CONCAT);

    // Tree structure is left-associative: ((a .) b)
    ConcatNode* concat1 = reinterpret_cast<ConcatNode*>(tree);
    ASSERT_EQ(concat1->left->type, NODE_CONCAT);
    
    ConcatNode* concat2 = reinterpret_cast<ConcatNode*>(concat1->left);
    ASSERT_EQ(concat2->left->type, NODE_LITERAL);
    LiteralNode* literal_a = reinterpret_cast<LiteralNode*>(concat2->left);
    ASSERT_EQ(literal_a->value, 'a');
    
    ASSERT_EQ(concat2->right->type, NODE_LITERAL);
    LiteralNode* literal_dot = reinterpret_cast<LiteralNode*>(concat2->right);
    ASSERT_EQ(literal_dot->value, '.');

    ASSERT_EQ(concat1->right->type, NODE_LITERAL);
    LiteralNode* literal_b = reinterpret_cast<LiteralNode*>(concat1->right);
    ASSERT_EQ(literal_b->value, 'b');

    free_ast(tree);
}

TEST(ParserAST, ParsesEscapedPipe) {
    AstNode* tree = parse("^a\\|b$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CONCAT);

    // Tree structure is left-associative: ((a |) b)
    ConcatNode* concat1 = reinterpret_cast<ConcatNode*>(tree);
    ASSERT_EQ(concat1->left->type, NODE_CONCAT);
    
    ConcatNode* concat2 = reinterpret_cast<ConcatNode*>(concat1->left);
    ASSERT_EQ(concat2->left->type, NODE_LITERAL);
    LiteralNode* literal_a = reinterpret_cast<LiteralNode*>(concat2->left);
    ASSERT_EQ(literal_a->value, 'a');
    
    ASSERT_EQ(concat2->right->type, NODE_LITERAL);
    LiteralNode* literal_pipe = reinterpret_cast<LiteralNode*>(concat2->right);
    ASSERT_EQ(literal_pipe->value, '|');

    ASSERT_EQ(concat1->right->type, NODE_LITERAL);
    LiteralNode* literal_b = reinterpret_cast<LiteralNode*>(concat1->right);
    ASSERT_EQ(literal_b->value, 'b');

    free_ast(tree);
}

TEST(ParserAST, ParsesEscapedParentheses) {
    AstNode* tree = parse("^\\(abc\\)$");

    ASSERT_NE(tree, nullptr);
    
    // Should create a concatenation of literals: '(', 'a', 'b', 'c', ')'
    ASSERT_EQ(tree->type, NODE_CONCAT);
    
    free_ast(tree);
}

TEST(ParserAST, ParsesMultipleEscapedCharacters) {
    AstNode* tree = parse("^\\*\\+\\?$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CONCAT);
    
    // Tree structure is left-associative: ((* +) ?)
    ConcatNode* concat1 = reinterpret_cast<ConcatNode*>(tree);
    ASSERT_EQ(concat1->left->type, NODE_CONCAT);
    
    ConcatNode* concat2 = reinterpret_cast<ConcatNode*>(concat1->left);
    ASSERT_EQ(concat2->left->type, NODE_LITERAL);
    LiteralNode* literal_star = reinterpret_cast<LiteralNode*>(concat2->left);
    ASSERT_EQ(literal_star->value, '*');
    
    ASSERT_EQ(concat2->right->type, NODE_LITERAL);
    LiteralNode* literal_plus = reinterpret_cast<LiteralNode*>(concat2->right);
    ASSERT_EQ(literal_plus->value, '+');

    ASSERT_EQ(concat1->right->type, NODE_LITERAL);
    LiteralNode* literal_question = reinterpret_cast<LiteralNode*>(concat1->right);
    ASSERT_EQ(literal_question->value, '?');

    free_ast(tree);
}

TEST(ParserAST, ParsesBasicCharacterClass) {
    AstNode* tree = parse("^[abc]$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_FALSE(cc->negated);
    ASSERT_TRUE(cc->char_set['a']);
    ASSERT_TRUE(cc->char_set['b']);
    ASSERT_TRUE(cc->char_set['c']);
    ASSERT_FALSE(cc->char_set['d']);
    ASSERT_FALSE(cc->char_set['x']);

    free_ast(tree);
}

TEST(ParserAST, ParsesCharacterClassWithRange) {
    AstNode* tree = parse("^[a-z]$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_FALSE(cc->negated);
    
    // Check some characters in the range
    ASSERT_TRUE(cc->char_set['a']);
    ASSERT_TRUE(cc->char_set['m']);
    ASSERT_TRUE(cc->char_set['z']);
    
    // Check characters outside the range
    ASSERT_FALSE(cc->char_set['A']);
    ASSERT_FALSE(cc->char_set['0']);

    free_ast(tree);
}

TEST(ParserAST, ParsesCharacterClassWithMultipleRanges) {
    AstNode* tree = parse("^[a-z0-9]$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_FALSE(cc->negated);
    
    // Check lowercase letters
    ASSERT_TRUE(cc->char_set['a']);
    ASSERT_TRUE(cc->char_set['z']);
    
    // Check digits
    ASSERT_TRUE(cc->char_set['0']);
    ASSERT_TRUE(cc->char_set['5']);
    ASSERT_TRUE(cc->char_set['9']);
    
    // Check characters outside ranges
    ASSERT_FALSE(cc->char_set['A']);
    ASSERT_FALSE(cc->char_set['-']);

    free_ast(tree);
}

TEST(ParserAST, ParsesNegatedCharacterClass) {
    AstNode* tree = parse("^[^abc]$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_TRUE(cc->negated);
    ASSERT_TRUE(cc->char_set['a']);
    ASSERT_TRUE(cc->char_set['b']);
    ASSERT_TRUE(cc->char_set['c']);
    ASSERT_FALSE(cc->char_set['d']);
    ASSERT_FALSE(cc->char_set['x']);

    free_ast(tree);
}

TEST(ParserAST, ParsesCharacterClassWithEscapedChars) {
    AstNode* tree = parse("^[\\]\\-\\[]$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_FALSE(cc->negated);
    ASSERT_TRUE(cc->char_set[']']);
    ASSERT_TRUE(cc->char_set['-']);
    ASSERT_TRUE(cc->char_set['[']);

    free_ast(tree);
}

TEST(ParserAST, ParsesCharacterClassInPattern) {
    AstNode* tree = parse("^a[0-9]+b$");

    ASSERT_NE(tree, nullptr);
    print_ast(tree);
    
    // Tree structure: ((a [0-9]+) b)
    ASSERT_EQ(tree->type, NODE_CONCAT);
    
    ConcatNode* concat1 = reinterpret_cast<ConcatNode*>(tree);
    ASSERT_EQ(concat1->left->type, NODE_CONCAT);
    
    ConcatNode* concat2 = reinterpret_cast<ConcatNode*>(concat1->left);
    ASSERT_EQ(concat2->left->type, NODE_LITERAL);
    LiteralNode* lit_a = reinterpret_cast<LiteralNode*>(concat2->left);
    ASSERT_EQ(lit_a->value, 'a');
    
    ASSERT_EQ(concat2->right->type, NODE_QUANTIFIER);
    QuantifierNode* quant = reinterpret_cast<QuantifierNode*>(concat2->right);
    ASSERT_EQ(quant->quantifier, '+');
    ASSERT_EQ(quant->child->type, NODE_CHAR_CLASS);
    
    CharClassNode* cc = reinterpret_cast<CharClassNode*>(quant->child);
    ASSERT_FALSE(cc->negated);
    ASSERT_TRUE(cc->char_set['0']);
    ASSERT_TRUE(cc->char_set['9']);
    
    ASSERT_EQ(concat1->right->type, NODE_LITERAL);
    LiteralNode* lit_b = reinterpret_cast<LiteralNode*>(concat1->right);
    ASSERT_EQ(lit_b->value, 'b');

    free_ast(tree);
}

TEST(ParserAST, ParsesMixedCharacterClassAndLiterals) {
    AstNode* tree = parse("^[aB3]xy[^def]$");

    ASSERT_NE(tree, nullptr);
    print_ast(tree);

    free_ast(tree);
}

TEST(ParserAST, ParsesShorthandDigitClass) {
    AstNode* tree = parse("^\\d$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_FALSE(cc->negated);
    ASSERT_TRUE(cc->char_set['0']);
    ASSERT_TRUE(cc->char_set['5']);
    ASSERT_TRUE(cc->char_set['9']);
    ASSERT_FALSE(cc->char_set['a']);
    ASSERT_FALSE(cc->char_set['A']);

    free_ast(tree);
}

TEST(ParserAST, ParsesShorthandWordClass) {
    AstNode* tree = parse("^\\w$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_FALSE(cc->negated);
    ASSERT_TRUE(cc->char_set['a']);
    ASSERT_TRUE(cc->char_set['z']);
    ASSERT_TRUE(cc->char_set['A']);
    ASSERT_TRUE(cc->char_set['Z']);
    ASSERT_TRUE(cc->char_set['0']);
    ASSERT_TRUE(cc->char_set['9']);
    ASSERT_TRUE(cc->char_set['_']);
    ASSERT_FALSE(cc->char_set['-']);
    ASSERT_FALSE(cc->char_set[' ']);

    free_ast(tree);
}

TEST(ParserAST, ParsesShorthandWhitespaceClass) {
    AstNode* tree = parse("^\\s$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_FALSE(cc->negated);
    ASSERT_TRUE(cc->char_set[' ']);
    ASSERT_TRUE(cc->char_set['\t']);
    ASSERT_TRUE(cc->char_set['\n']);
    ASSERT_TRUE(cc->char_set['\r']);
    ASSERT_TRUE(cc->char_set['\f']);
    ASSERT_TRUE(cc->char_set['\v']);
    ASSERT_FALSE(cc->char_set['a']);

    free_ast(tree);
}

TEST(ParserAST, ParsesNegatedShorthandDigitClass) {
    AstNode* tree = parse("^\\D$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_TRUE(cc->negated);
    ASSERT_TRUE(cc->char_set['0']);
    ASSERT_TRUE(cc->char_set['9']);

    free_ast(tree);
}

TEST(ParserAST, ParsesNegatedShorthandWordClass) {
    AstNode* tree = parse("^\\W$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_TRUE(cc->negated);
    ASSERT_TRUE(cc->char_set['a']);
    ASSERT_TRUE(cc->char_set['_']);

    free_ast(tree);
}

TEST(ParserAST, ParsesNegatedShorthandWhitespaceClass) {
    AstNode* tree = parse("^\\S$");

    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(tree->type, NODE_CHAR_CLASS);

    CharClassNode* cc = reinterpret_cast<CharClassNode*>(tree);
    ASSERT_TRUE(cc->negated);
    ASSERT_TRUE(cc->char_set[' ']);
    ASSERT_TRUE(cc->char_set['\t']);

    free_ast(tree);
}

TEST(ParserAST, ParsesShorthandClassesInPattern) {
    AstNode* tree = parse("^\\w+@\\w+\\.\\w+$");

    ASSERT_NE(tree, nullptr);
    print_ast(tree);

    free_ast(tree);
}