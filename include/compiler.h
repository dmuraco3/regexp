//
// Created by Dylan Muraco on 10/24/25.
//

#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include "parser.h"


#define ANY_CHAR (-1)
#define EPSILON 0
#define CHAR_CLASS (-2)
#define CAPTURE_START (-3)
#define CAPTURE_END (-4)


typedef struct Transition {
    char symbol;
    struct NfaState *to;
    // For character classes: if symbol == CHAR_CLASS, use these fields
    bool *char_class_set;     // Pointer to the character set bitmap
    bool char_class_negated;  // Whether the class is negated
    // For capture groups: if symbol == CAPTURE_START or CAPTURE_END
    char *capture_name;       // Name of the capture group
    int capture_id;           // Unique ID for the capture group
} Transition;

typedef struct NfaState {
    unsigned long id;
    bool is_accepting;

    Transition *out1;
    Transition *out2;
} NfaState;

typedef struct NfaFragment {
    NfaState *start;
    NfaState *accept;
} NfaFragment;

NfaFragment create_literal_fragment(char c, unsigned long *next_state_id);

NfaFragment create_wildcard_fragment(unsigned long *next_state_id);

NfaFragment create_char_class_fragment(bool negated, bool char_set[256], unsigned long *next_state_id);

NfaFragment create_capture_group_fragment(const char *name, int capture_id, NfaFragment child_frag, unsigned long *next_state_id);

NfaFragment create_concat_fragment(NfaFragment frag1, NfaFragment frag2);

NfaFragment create_alternation_fragment(NfaFragment frag1, NfaFragment frag2, unsigned long *next_state_id);

NfaFragment create_start_fragment(NfaFragment frag, unsigned long *next_state_id);

NfaFragment create_plus_fragment(NfaFragment frag, unsigned long *next_state_id);

NfaFragment create_option_fragment(NfaFragment frag, unsigned long *next_state_id);

NfaFragment compile_ast(AstNode* node);

void free_nfa(NfaState *start);

void print_nfa(NfaState *start_state);

#endif //COMPILER_H
