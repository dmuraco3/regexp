//
// Created by Dylan Muraco on 10/24/25.
//

#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include "parser.h"


#define ANY_CHAR (-1)
#define EPSILON 0


typedef struct Transition {
    char symbol;
    struct NfaState *to;
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

NfaFragment create_concat_fragment(NfaFragment frag1, NfaFragment frag2);

NfaFragment create_alternation_fragment(NfaFragment frag1, NfaFragment frag2, unsigned long *next_state_id);

NfaFragment create_start_fragment(NfaFragment frag, unsigned long *next_state_id);

NfaFragment create_plus_fragment(NfaFragment frag, unsigned long *next_state_id);

NfaFragment create_option_fragment(NfaFragment frag, unsigned long *next_state_id);

NfaFragment compile_ast(AstNode* node);

void free_nfa(NfaState *start);

void print_nfa(NfaState *start_state);

#endif //COMPILER_H
