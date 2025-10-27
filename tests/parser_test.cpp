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