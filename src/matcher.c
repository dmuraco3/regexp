//
// Created by Dylan Muraco on 10/24/25.
//

#include "matcher.h"

#include <stdlib.h>
#include <stdio.h>

void init_set(NfaStateSet *set) {
    set->states = NULL;
    set->count = 0;
    set->capacity = 0;
}

// Adds a state if not already present (simple linear scan for duplicates)
void add_state(NfaStateSet *set, NfaState *state) {
    if (state == NULL) return;

    // Check for duplicates (simple, could be faster with sorting/hashing)
    for (size_t i = 0; i < set->count; ++i) {
        if (set->states[i] == state) {
            return; // Already in the set
        }
    }

    // Resize if necessary
    if (set->count >= set->capacity) {
        size_t new_capacity = (set->capacity == 0) ? 16 : set->capacity * 2;
        NfaState **new_states = (NfaState**)realloc(set->states, new_capacity * sizeof(NfaState*));
        if (!new_states) {
            perror("Failed to realloc NfaStateSet");
            // In a real library, handle this more gracefully (e.g., return error)
            exit(EXIT_FAILURE);
        }
        set->states = new_states;
        set->capacity = new_capacity;
    }

    // Add the new state
    set->states[set->count++] = state;
}

void free_set(NfaStateSet *set) {
    free(set->states);
    set->states = NULL;
    set->count = 0;
    set->capacity = 0;
}

void clear_set(NfaStateSet *set) {
    set->count = 0; // Keep allocated memory for reuse
}

static bool contains_state(const NfaStateSet *set, const NfaState *state) {
    if (!state) return false;
    for (size_t i = 0; i < set->count; ++i) {
        if (set->states[i] == state) {
            return true;
        }
    }
    return false;
}

static void process_epsilon_neighbor(NfaState *next, NfaStateSet *closure_set, NfaStateSet *stack) {
    // Use the file-scope helper function
    if (next && !contains_state(closure_set, next)) {
        add_state(closure_set, next); // Add to final closure
        add_state(stack, next); // Add to stack to explore its neighbors
    }
}

void epsilon_closure(NfaStateSet *initial_set, NfaStateSet *closure_set) {
    NfaStateSet stack; // Stack for DFS
    init_set(&stack);

    // Initialize stack and closure with initial states
    clear_set(closure_set); // Ensure closure set starts empty
    for (size_t i = 0; i < initial_set->count; ++i) {
        NfaState *s = initial_set->states[i];
        if (s) {
            // Use the file-scope helper function
            if (!contains_state(closure_set, s)) {
                add_state(closure_set, s);
                add_state(&stack, s); // Add to stack for processing
            }
        }
    }

    // Perform DFS
    while (stack.count > 0) {
        NfaState *current_state = stack.states[--stack.count]; // Pop

        // Helper lambda/function to process neighbors via epsilon

        // Explore epsilon transitions
        if (current_state->out1 && current_state->out1->symbol == EPSILON) {
            process_epsilon_neighbor(current_state->out1->to, closure_set, &stack);
        }
        if (current_state->out2 && current_state->out2->symbol == EPSILON) {
            process_epsilon_neighbor(current_state->out2->to, closure_set, &stack);
        }
    }

    free_set(&stack); // Free the stack's internal array
}

bool match(NfaFragment fragment, const char *input) {
    NfaState *start_state = fragment.start;
    if (!start_state || !input) {
        return false;
    }

    NfaStateSet current_states, next_states, temp_reachable;
    init_set(&current_states);
    init_set(&next_states);
    init_set(&temp_reachable);

    // 1. Initial state: epsilon closure of the start state
    NfaStateSet initial_single;
    init_set(&initial_single);
    add_state(&initial_single, start_state);
    epsilon_closure(&initial_single, &current_states); // Call new version
    free_set(&initial_single);

    // 2. Process each character in the input string
    for (size_t i = 0; input[i] != '\0'; ++i) {
        char current_char = input[i];
        clear_set(&temp_reachable);

        // Find states directly reachable on the current character
        for (size_t j = 0; j < current_states.count; ++j) {
            NfaState *s = current_states.states[j];
            
            // Check out1 transition
            if (s->out1) {
                bool matches = false;
                if (s->out1->symbol == current_char) {
                    matches = true;
                } else if (s->out1->symbol == ANY_CHAR) {
                    matches = true;
                } else if (s->out1->symbol == CHAR_CLASS && s->out1->char_class_set != NULL) {
                    // Check if current_char is in the character class
                    bool in_set = s->out1->char_class_set[(unsigned char)current_char];
                    // If negated, invert the match
                    matches = s->out1->char_class_negated ? !in_set : in_set;
                }
                
                if (matches) {
                    add_state(&temp_reachable, s->out1->to);
                }
            }
            
            // Check out2 transition
            if (s->out2) {
                bool matches = false;
                if (s->out2->symbol == current_char) {
                    matches = true;
                } else if (s->out2->symbol == ANY_CHAR) {
                    matches = true;
                } else if (s->out2->symbol == CHAR_CLASS && s->out2->char_class_set != NULL) {
                    // Check if current_char is in the character class
                    bool in_set = s->out2->char_class_set[(unsigned char)current_char];
                    // If negated, invert the match
                    matches = s->out2->char_class_negated ? !in_set : in_set;
                }
                
                if (matches) {
                    add_state(&temp_reachable, s->out2->to);
                }
            }
        }

        // Compute the epsilon closure of the reachable states
        clear_set(&next_states);
        epsilon_closure(&temp_reachable, &next_states); // Call new version

        // Update current states for the next iteration (swap pointers)
        NfaStateSet temp_swap = current_states;
        current_states = next_states;
        next_states = temp_swap;

        // If no states are reachable after this character, fail early
        if (current_states.count == 0) {
            break; // No further match possible
        }
    }

    // 3. Final check: Is any state in the final set an accepting state?
    bool is_match = false;
    for (size_t i = 0; i < current_states.count; ++i) {
        if (current_states.states[i]->is_accepting) {
            is_match = true;
            break;
        }
    }

    // Cleanup
    free_set(&current_states);
    free_set(&next_states);
    free_set(&temp_reachable);

    return is_match;
}
