//
// Created by Dylan Muraco on 10/24/25.
//

#ifndef MATCHER_H
#define MATCHER_H


#include <stddef.h> // For size_t

#include "compiler.h" // Your NfaState definition


typedef struct {
    NfaState **states; // Dynamically allocated array of state pointers
    size_t count;      // Number of states currently in the set
    size_t capacity;   // Allocated size of the states array
} NfaStateSet;

// Function prototypes for set operations
void init_set(NfaStateSet *set);
void add_state(NfaStateSet *set, NfaState *state);
void free_set(NfaStateSet *set);
void clear_set(NfaStateSet *set); // Resets count to 0 but keeps allocation

void epsilon_closure(NfaStateSet *initial_set, NfaStateSet *closure_set);

bool match(NfaFragment fragment, const char *input);

// Capture group support
typedef struct {
    char *name;          // Group name (NULL for numbered groups in future)
    char *value;         // Matched text
    int start;           // Start position in input
    int end;             // End position in input
} CaptureGroup;

typedef struct {
    bool matched;        // Whether the pattern matched
    int num_groups;      // Number of capture groups
    CaptureGroup *groups; // Array of captured groups
} MatchResult;

MatchResult match_with_captures(NfaFragment fragment, const char *input);
void free_match_result(MatchResult *result);

#endif //MATCHER_H
