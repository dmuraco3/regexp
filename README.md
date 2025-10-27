# Regex Engine

A lightweight regular expression engine written in C that compiles regex patterns into NFAs (Non-deterministic Finite Automata) and performs pattern matching.

## Features

- **Parser**: Converts regex patterns into an Abstract Syntax Tree (AST)
- **Compiler**: Transforms AST into NFA using Thompson's construction
- **Matcher**: Executes NFA-based pattern matching with epsilon-closure

### Supported Regex Syntax

| Operator | Description | Example |
|----------|-------------|---------|
| `.` | Wildcard - matches any character | `a.c` matches "abc", "a1c", "aZc" |
| `*` | Zero or more | `ab*c` matches "ac", "abc", "abbc" |
| `+` | One or more | `ab+c` matches "abc", "abbc" (not "ac") |
| `?` | Zero or one | `ab?c` matches "ac", "abc" |
| `\|` | Alternation (or) | `a\|b` matches "a" or "b" |
| `()` | Grouping | `(ab)+` matches "ab", "abab" |
| `^` | Start anchor | `^abc` matches "abc" at start |
| `$` | End anchor | `abc$` matches "abc" at end |

## Building

```bash
# Configure and build
cmake -B build
cmake --build build

# Run tests
cd build/tests && ./run_tests

# Install (optional - installs to /usr/local by default)
sudo cmake --install build
```

After installing, you can use the library system-wide:
```bash
gcc your_program.c -lregex -o your_program
```

Or using the traditional approach:

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

## Project Structure

```
regex/
├── include/
│   ├── regexp.h        # Unified public header (use this!)
│   ├── parser.h        # Regex pattern parser API
│   ├── compiler.h      # AST → NFA compiler API
│   └── matcher.h       # NFA-based pattern matching API
├── src/
│   ├── parser.c        # Parser implementation
│   ├── compiler.c      # Compiler implementation
│   └── matcher.c       # Matcher implementation
├── tests/
│   ├── parser_test.cpp
│   ├── compiler_test.cpp
│   └── matcher_test.cpp
└── CMakeLists.txt
```

## Usage

```c
#include <regexp.h>  // Single unified header

// Parse regex pattern
AstNode* tree = parse("^a(b|c)*d+$");

// Compile to NFA
NfaFragment nfa = compile_ast(tree);

// Match against input strings
bool matches = match(nfa, "abd");      // true
bool no_match = match(nfa, "ac");      // false

// Clean up
free_nfa(nfa.start);
free_ast(tree);
```

### Linking

When compiling your program:
```bash
gcc your_program.c -I/path/to/regex/include -L/path/to/regex/build/src -lregex -o your_program
```

## Implementation Details

### Parser
- Recursive descent parser
- Operator precedence: `*+?` > concatenation > `|`
- Creates AST with nodes: `LITERAL`, `CONCAT`, `ALTERNATION`, `QUANTIFIER`, `WILDCARD`

### Compiler
- Uses Thompson's construction algorithm
- Each AST node → NFA fragment with start/accept states
- Epsilon transitions (`EPSILON = 0`) for structural connections
- Wildcard matches via special `ANY_CHAR` symbol

### Matcher
- Simulates NFA execution on input string
- Maintains sets of active states
- Computes epsilon-closure for non-determinism
- Returns `true` if any accepting state is reached at end of input

## Testing

The project uses Google Test for unit testing:

```bash
# Run all tests
cd build && ctest

# Run with verbose output
cd build/tests && ./run_tests

# Run specific test suite
./run_tests --gtest_filter=Matcher.*
```

## Requirements

- CMake 3.29+
- C compiler (C standard)
- C++17 compiler (for tests)
- Google Test (automatically fetched by CMake)

## License

This project is licensed under a Use-Only License. You may use and run the software freely, but copying, modifying, or distributing the source code requires prior written permission from the author. See [LICENSE](LICENSE) file for full details.

To request permission, please open an issue in the repository or contact dylanjmuraco@gmail.com.

## Author

Dylan Muraco  
Email: dylanjmuraco@gmail.com
