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

TEST(Matcher, MatchesEscapedCharacters) {
    AstNode* tree = parse("^a\\*b\\+c$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "a*b+c" };
    const char *invalid_strings[] = { "abc", "aac", "a**b++c", "aabbc", "ab+c", "a*bc", "a*b+", "*b+c" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesEscapedDotAndPipe) {
    AstNode* tree = parse("^hello\\.world\\|test$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "hello.world|test" };
    const char *invalid_strings[] = { "helloXworld|test", "hello.worldXtest", "hello world|test", "helloworld|test", "hello.world", "|test" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesEscapedParenthesesAndQuestionMark) {
    AstNode* tree = parse("^\\(optional\\)\\?$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "(optional)?" };
    const char *invalid_strings[] = { "optional?", "(optional)", "?", "()", "(optional)??", "optional" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesMixedEscapedAndUnescapedMetacharacters) {
    AstNode* tree = parse("^a\\*+b\\.?c$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    // Pattern: one or more literal '*', followed by zero or one literal '.', followed by 'c'
    // So: a*c, a**c, a***c, a*.c, a**.c, etc.
    const char *valid_strings[] = { "a*bc", "a**bc", "a***bc", "a*b.c", "a**b.c" };
    const char *invalid_strings[] = { "abc", "a.c", "a*c", "a*b..c", "bc", "*bc" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesBasicCharacterClass) {
    AstNode* tree = parse("^[abc]$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "a", "b", "c" };
    const char *invalid_strings[] = { "d", "ab", "x", "abc", "" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesCharacterClassRange) {
    AstNode* tree = parse("^[0-9]$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "0", "5", "9", "3", "7" };
    const char *invalid_strings[] = { "a", "A", "!", "10", "" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesNegatedCharacterClass) {
    AstNode* tree = parse("^[^0-9]$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "a", "Z", "!", "@", " " };
    const char *invalid_strings[] = { "0", "5", "9", "10", "" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesCharacterClassInPattern) {
    AstNode* tree = parse("^[a-z]+[0-9]+$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "a1", "abc123", "xyz999", "hello42" };
    const char *invalid_strings[] = { "123", "ABC123", "a", "123abc", "" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesComplexCharacterClassPattern) {
    AstNode* tree = parse("^[a-zA-Z_][a-zA-Z0-9_]*$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "a", "Z", "_", "hello", "World123", "_private", "var_name_1" };
    const char *invalid_strings[] = { "123", "1abc", "", "hello-world", "test!" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesMixedCharClassesAndLiterals) {
    AstNode* tree = parse("^test[0-9][a-z]?end$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "test0end", "test5aend", "test9zend", "test3mend" };
    const char *invalid_strings[] = { "testend", "test0Aend", "test99end", "test0aaend" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesShorthandDigitClass) {
    AstNode* tree = parse("^\\d+$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "0", "123", "999", "42" };
    const char *invalid_strings[] = { "a", "1a2", "abc", "", " " };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesShorthandWordClass) {
    AstNode* tree = parse("^\\w+$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "hello", "World123", "_test", "a", "ABC_123" };
    const char *invalid_strings[] = { "hello world", "test-case", "foo!", "", "@test" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesShorthandWhitespaceClass) {
    AstNode* tree = parse("^\\s+$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { " ", "  ", "\t", "\n", " \t\n", "\r\n" };
    const char *invalid_strings[] = { "a", "hello", "1", " a ", "" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesNegatedShorthandClasses) {
    AstNode* tree = parse("^\\D+$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "abc", "XYZ", "hello", "!!!" };
    const char *invalid_strings[] = { "123", "a1b", "test9", "" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesMixedShorthandClasses) {
    AstNode* tree = parse("^\\w+\\s+\\d+$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "hello 123", "test\t42", "foo  999", "a 0" };
    const char *invalid_strings[] = { "hello123", "123 test", "test  ", "  123", "" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}

TEST(Matcher, MatchesEmailLikePattern) {
    AstNode* tree = parse("^\\w+@\\w+\\.\\w+$");
    ASSERT_NE(tree, nullptr);
    NfaFragment nfa = compile_ast(tree);
    ASSERT_NE(nfa.start, nullptr);
    ASSERT_NE(nfa.accept, nullptr);

    const char *valid_strings[] = { "user@example.com", "test@domain.org", "a@b.c" };
    const char *invalid_strings[] = { "user@example", "test.domain.com", "@example.com", "user@.com", "" };

    for (const char *str : valid_strings) {
        EXPECT_TRUE(match(nfa, str)) << "Expected to match: " << str;
    }

    for (const char *str : invalid_strings) {
        EXPECT_FALSE(match(nfa, str)) << "Expected not to match: " << str;
    }

    free_nfa(nfa.start);
    free_ast(tree);
}


