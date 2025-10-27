#include <gtest/gtest.h>
#include <fstream>

extern "C" {
    #include <regexp.h>
}

TEST(Matcher, MatchesSimpleLiteral) {
    AstNode* tree = parse("^a(b|c)*d+$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "ad", "abd", "acd", "abbd", "acccdd", "abcbcd" };
    const char *invalid_strings[] = { "a", "ab", "ac", "abdde", "abcc", "abcdc" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesComplexPattern) {
    AstNode* tree = parse("^(a(b|c.)*d|e+f?.)$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "ad", "abd", "abbd", "ac1d", "ac1bd", "abc1d", "e1", "eeA", "ef", "efZ", "eeeef#", "ef1" };
    const char *invalid_strings[] = { "a", "d", "ab", "ac1", "a d", "add", "abd1", "ac1c2b3d", "e",  "1ad", "ade", "e1e", "a(b|c.)*d" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }


    free_nfa(nfa.start);
    free_ast(tree);
}
