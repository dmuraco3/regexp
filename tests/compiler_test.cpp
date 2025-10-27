#include <gtest/gtest.h>

extern "C" {
    #include <regexp.h>
}

TEST(CompilerNFA, CompilesSingleLiteral) {
    AstNode* tree = parse("^a$");
    ASSERT_NE(tree, nullptr);

    auto [start, accept] = compile_ast(tree);
    ASSERT_NE(start, nullptr);
    ASSERT_NE(accept, nullptr);
    ASSERT_TRUE(accept->is_accepting);

    ASSERT_EQ(start->out1->symbol, 'a');
    ASSERT_EQ(start->out1->to, accept);

    free_nfa(start);
    free_ast(tree);
}

TEST(CompilerNFA, CompilesComplicated) {
    AstNode* tree = parse("ab*");
    ASSERT_NE(tree, nullptr);

    auto [start, accept] = compile_ast(tree);
    ASSERT_NE(start, nullptr);
    ASSERT_NE(accept, nullptr);
    ASSERT_TRUE(accept->is_accepting);

    free_nfa(start);
    free_ast(tree);
}