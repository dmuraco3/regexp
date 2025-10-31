# Regex Engine

A lightweight regular expression engine written in C that compiles regex patterns into NFAs (Non-deterministic Finite Automata) and performs pattern matching.

## Features

- **Parser**: Converts regex patterns into an Abstract Syntax Tree (AST)
- **Compiler**: Transforms AST into NFA using Thompson's construction
- **Matcher**: Executes NFA-based pattern matching with epsilon-closure

### Supported Regex Syntax

#### Basic Operators
| Operator | Description                      | Example                                 |
|----------|----------------------------------|-----------------------------------------|
| `.`      | Wildcard - matches any character | `a.c` matches "abc", "a1c", "aZc"       |
| `*`      | Zero or more                     | `ab*c` matches "ac", "abc", "abbc"      |
| `+`      | One or more                      | `ab+c` matches "abc", "abbc" (not "ac") |
| `?`      | Zero or one                      | `ab?c` matches "ac", "abc"              |
| `\|`     | Alternation (or)                 | `a\|b` matches "a" or "b"               |
| `()`     | Grouping                         | `(ab)+` matches "ab", "abab"            |
| `^`      | Start anchor                     | `^abc` matches "abc" at start           |
| `$`      | End anchor                       | `abc$` matches "abc" at end             |

#### Escaping
| Syntax    | Description           | Example                       |
|-----------|-----------------------|-------------------------------|
| `\*`      | Literal asterisk      | `a\*b` matches "a*b"          |
| `\+`      | Literal plus          | `a\+b` matches "a+b"          |
| `\.`      | Literal dot           | `a\.b` matches "a.b"          |
| `\|`      | Literal pipe          | `a\|b` matches "a\|b"         |
| `\(` `\)` | Literal parentheses   | `\(hello\)` matches "(hello)" |
| `\?`      | Literal question mark | `a\?` matches "a?"            |

#### Character Classes
| Syntax        | Description                                            | Example                              |
|---------------|--------------------------------------------------------|--------------------------------------|
| `[abc]`       | Character set - matches any single character in set    | `[abc]` matches "a", "b", or "c"     |
| `[a-z]`       | Character range                                        | `[a-z]` matches any lowercase letter |
| `[a-zA-Z0-9]` | Multiple ranges                                        | Matches any alphanumeric character   |
| `[^abc]`      | Negated set - matches any character NOT in the set     | `[^0-9]` matches any non-digit       |
| `[\]\-\[]`    | Escaped characters in sets                             | Matches `]`, `-`, or `[`             |

#### Shorthand Character Classes
| Syntax | Equivalent To      | Description                                            |
|--------|--------------------|--------------------------------------------------------|
| `\d`   | `[0-9]`            | Matches any digit                                      |
| `\D`   | `[^0-9]`           | Matches any non-digit                                  |
| `\w`   | `[a-zA-Z0-9_]`     | Matches any word character (alphanumeric + underscore) |
| `\W`   | `[^a-zA-Z0-9_]`    | Matches any non-word character                         |
| `\s`   | `[ \t\n\r\f\v]`    | Matches any whitespace character                       |
| `\S`   | `[^ \t\n\r\f\v]`   | Matches any non-whitespace character                   |

#### Named Capture Groups
| Syntax         | Description         | Example                                                    |
|----------------|---------------------|------------------------------------------------------------|
| `(?<name>...)` | Named capture group | `(?<word>\w+)` captures word characters into group "word"  |

**Example with captures:**
```c
AstNode* tree = parse("^(?<user>\\w+)@(?<domain>\\w+)\\.(?<tld>\\w+)$");
NfaFragment nfa = compile_ast(tree);

MatchResult result = match_with_captures(nfa, "test@example.com");
if (result.matched) {
    for (size_t i = 0; i < result.num_groups; i++) {
        printf("%s = '%s' [%zu:%zu]\n", 
               result.groups[i].name,
               result.groups[i].value,
               result.groups[i].start,
               result.groups[i].end);
    }
    // Output:
    // user = 'test' [0:4]
    // domain = 'example' [5:12]
    // tld = 'com' [13:16]
}
free_match_result(&result);
free_nfa(nfa.start);
free_ast(tree);
```

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

### Basic Pattern Matching

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

### Character Classes and Escaping

```c
#include <regexp.h>

// Match identifiers: letter or underscore, followed by alphanumeric/underscore
AstNode* tree = parse("^[a-zA-Z_][a-zA-Z0-9_]*$");
NfaFragment nfa = compile_ast(tree);

bool valid = match(nfa, "my_var123");    // true
bool invalid = match(nfa, "123abc");     // false

free_nfa(nfa.start);
free_ast(tree);

// Match literal dots in version numbers
tree = parse("^\\d+\\.\\d+\\.\\d+$");
nfa = compile_ast(tree);

bool version = match(nfa, "1.2.3");      // true
bool not_version = match(nfa, "1x2x3");  // false

free_nfa(nfa.start);
free_ast(tree);
```

### Shorthand Classes

```c
#include <regexp.h>

// Match email-like patterns using shorthand classes
AstNode* tree = parse("^\\w+@\\w+\\.\\w+$");
NfaFragment nfa = compile_ast(tree);

bool email1 = match(nfa, "user@example.com");  // true
bool email2 = match(nfa, "test@test.org");      // true
bool not_email = match(nfa, "invalid@");        // false

free_nfa(nfa.start);
free_ast(tree);
```

### Named Capture Groups

```c
#include <regexp.h>
#include <stdio.h>

// Parse pattern with named captures
AstNode* tree = parse("^(?<year>\\d+)-(?<month>\\d+)-(?<day>\\d+)$");
NfaFragment nfa = compile_ast(tree);

// Match and extract captures
MatchResult result = match_with_captures(nfa, "2025-10-31");

if (result.matched) {
    printf("Match found with %zu capture groups:\n", result.num_groups);
    for (size_t i = 0; i < result.num_groups; i++) {
        printf("  %s = '%s' (position %zu-%zu)\n",
               result.groups[i].name,
               result.groups[i].value,
               result.groups[i].start,
               result.groups[i].end);
    }
    // Output:
    // Match found with 3 capture groups:
    //   year = '2025' (position 0-4)
    //   month = '10' (position 5-7)
    //   day = '31' (position 8-10)
}

// Clean up
free_match_result(&result);
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
- Creates AST with nodes: `LITERAL`, `CONCAT`, `ALTERNATION`, `QUANTIFIER`, `WILDCARD`, `CHAR_CLASS`, `CAPTURE_GROUP`
- Handles escape sequences with backslash (`\`)
- Parses character classes with ranges and negation
- Expands shorthand classes (`\d`, `\w`, `\s`) into full character sets
- Extracts named capture group syntax `(?<name>...)`

### Compiler
- Uses Thompson's construction algorithm
- Each AST node → NFA fragment with start/accept states
- Special transition symbols:
  - `EPSILON (0)` - Structural connections (don't consume input)
  - `ANY_CHAR (-1)` - Wildcard matching
  - `CHAR_CLASS (-2)` - Character class matching with 256-bit bitmap
  - `CAPTURE_START (-3)` - Mark beginning of capture group
  - `CAPTURE_END (-4)` - Mark end of capture group
- Character classes use bitmap for O(1) lookup with negation support
- Capture groups add epsilon-like markers with unique IDs

### Matcher
- Simulates NFA execution on input string
- Maintains sets of active states
- Computes epsilon-closure for non-determinism (including capture markers)
- **Basic matching**: `match()` returns `true` if pattern matches
- **Capture extraction**: `match_with_captures()` returns `MatchResult` with:
  - Array of `CaptureGroup` structs (name, value, start, end positions)
  - Tracks capture markers during NFA traversal
  - Extracts substring values for each named group

## Testing

The project uses Google Test for unit testing with 45 comprehensive tests:

```bash
# Run all tests (recommended - runs each test in isolation)
cd build && ctest

# Run with verbose output
ctest --output-on-failure

# Run specific test suite
ctest -R Parser  # Parser tests
ctest -R Compiler  # Compiler tests
ctest -R Matcher  # Matcher tests
ctest -R Captures  # Capture group tests
```

**Note:** There is a known issue where running `./tests/run_tests` directly causes a segmentation fault when all tests run in the same process. This appears to be a memory corruption issue between tests. Use `ctest` instead, which runs each test in isolation and works perfectly.

### Test Coverage
- **Parser tests (23)**: Literals, wildcards, quantifiers, alternation, escaping, character classes, shorthand classes
- **Compiler tests (2)**: AST to NFA conversion validation
- **Matcher tests (20)**: Pattern matching, character classes, shorthand classes, capture groups

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
