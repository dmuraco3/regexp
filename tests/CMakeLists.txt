add_executable(run_tests
    parser_test.cpp
    compiler_test.cpp
        matcher_test.cpp
)

target_link_libraries(run_tests
    PRIVATE
    regexp
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(run_tests)