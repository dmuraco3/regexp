//
// Created by Dylan Muraco on 10/24/25.
//

#include "matcher.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
        
        // Skip NULL states (shouldn't happen, but be defensive)
        if (!current_state) {
            continue;
        }

        // Helper lambda/function to process neighbors via epsilon

        // Explore epsilon transitions (including CAPTURE_START and CAPTURE_END which don't consume input)
        if (current_state->out1 && (current_state->out1->symbol == EPSILON || 
                                     current_state->out1->symbol == CAPTURE_START ||
                                     current_state->out1->symbol == CAPTURE_END)) {
            process_epsilon_neighbor(current_state->out1->to, closure_set, &stack);
        }
        if (current_state->out2 && (current_state->out2->symbol == EPSILON ||
                                     current_state->out2->symbol == CAPTURE_START ||
                                     current_state->out2->symbol == CAPTURE_END)) {
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
                
                if (matches && s->out1->to) {
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
                
                if (matches && s->out2->to) {
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

// Structure to track active capture groups during matching
typedef struct {
    int capture_id;
    char *name;
    size_t start_pos;
} ActiveCapture;

typedef struct {
    ActiveCapture *captures;
    size_t count;
    size_t capacity;
} ActiveCaptureStack;

static void init_capture_stack(ActiveCaptureStack *stack) {
    stack->captures = NULL;
    stack->count = 0;
    stack->capacity = 0;
}

static void push_capture(ActiveCaptureStack *stack, int capture_id, const char *name, size_t start_pos) {
    if (stack->count >= stack->capacity) {
        size_t new_capacity = (stack->capacity == 0) ? 8 : stack->capacity * 2;
        ActiveCapture *new_captures = (ActiveCapture*)realloc(stack->captures, new_capacity * sizeof(ActiveCapture));
        if (!new_captures) {
            perror("Failed to realloc ActiveCaptureStack");
            exit(EXIT_FAILURE);
        }
        stack->captures = new_captures;
        stack->capacity = new_capacity;
    }
    
    ActiveCapture *cap = &stack->captures[stack->count++];
    cap->capture_id = capture_id;
    cap->name = name ? strdup(name) : NULL;
    cap->start_pos = start_pos;
}

static ActiveCapture* find_active_capture(ActiveCaptureStack *stack, int capture_id) {
    // Search from end (most recent) to beginning
    for (int i = (int)stack->count - 1; i >= 0; i--) {
        if (stack->captures[i].capture_id == capture_id) {
            return &stack->captures[i];
        }
    }
    return NULL;
}

static void free_capture_stack(ActiveCaptureStack *stack) {
    for (size_t i = 0; i < stack->count; i++) {
        free(stack->captures[i].name);
    }
    free(stack->captures);
    stack->captures = NULL;
    stack->count = 0;
    stack->capacity = 0;
}

MatchResult match_with_captures(NfaFragment fragment, const char *input) {
    MatchResult result;
    result.matched = false;
    result.num_groups = 0;
    result.groups = NULL;
    
    NfaState *start_state = fragment.start;
    if (!start_state || !input) {
        return result;
    }

    NfaStateSet current_states, next_states, temp_reachable;
    init_set(&current_states);
    init_set(&next_states);
    init_set(&temp_reachable);
    
    ActiveCaptureStack capture_stack;
    init_capture_stack(&capture_stack);
    
    // Track completed captures
    CaptureGroup *completed_captures = NULL;
    size_t completed_count = 0;
    size_t completed_capacity = 0;

    // 1. Initial state: epsilon closure of the start state
    NfaStateSet initial_single;
    init_set(&initial_single);
    add_state(&initial_single, start_state);
    
    // Process initial epsilon closure and capture any CAPTURE_START markers
    clear_set(&current_states);
    NfaStateSet stack;
    init_set(&stack);
    
    for (size_t i = 0; i < initial_single.count; ++i) {
        NfaState *s = initial_single.states[i];
        if (s && !contains_state(&current_states, s)) {
            add_state(&current_states, s);
            add_state(&stack, s);
        }
    }
    
    while (stack.count > 0) {
        NfaState *s = stack.states[--stack.count];
        
        if (s->out1) {
            if (s->out1->symbol == CAPTURE_START) {
                push_capture(&capture_stack, s->out1->capture_id, s->out1->capture_name, 0);
            }
            
            if (s->out1->symbol == EPSILON || s->out1->symbol == CAPTURE_START || s->out1->symbol == CAPTURE_END) {
                process_epsilon_neighbor(s->out1->to, &current_states, &stack);
            }
        }
        
        if (s->out2) {
            if (s->out2->symbol == CAPTURE_START) {
                push_capture(&capture_stack, s->out2->capture_id, s->out2->capture_name, 0);
            }
            
            if (s->out2->symbol == EPSILON || s->out2->symbol == CAPTURE_START || s->out2->symbol == CAPTURE_END) {
                process_epsilon_neighbor(s->out2->to, &current_states, &stack);
            }
        }
    }
    
    free_set(&stack);
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
                    bool in_set = s->out1->char_class_set[(unsigned char)current_char];
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
                    bool in_set = s->out2->char_class_set[(unsigned char)current_char];
                    matches = s->out2->char_class_negated ? !in_set : in_set;
                }
                
                if (matches) {
                    add_state(&temp_reachable, s->out2->to);
                }
            }
        }

        // Compute the epsilon closure and track capture markers
        clear_set(&next_states);
        init_set(&stack);
        
        for (size_t j = 0; j < temp_reachable.count; ++j) {
            NfaState *s = temp_reachable.states[j];
            if (s && !contains_state(&next_states, s)) {
                add_state(&next_states, s);
                add_state(&stack, s);
            }
        }
        
        while (stack.count > 0) {
            NfaState *s = stack.states[--stack.count];
            
            if (s->out1) {
                if (s->out1->symbol == CAPTURE_START) {
                    push_capture(&capture_stack, s->out1->capture_id, s->out1->capture_name, i + 1);
                } else if (s->out1->symbol == CAPTURE_END) {
                    ActiveCapture *active = find_active_capture(&capture_stack, s->out1->capture_id);
                    if (active) {
                        // Check if we already have a capture for this ID (update it)
                        CaptureGroup *existing = NULL;
                        for (size_t k = 0; k < completed_count; k++) {
                            if (completed_captures[k].name && active->name && 
                                strcmp(completed_captures[k].name, active->name) == 0) {
                                existing = &completed_captures[k];
                                break;
                            }
                        }
                        
                        if (existing) {
                            // Update existing capture
                            free(existing->value);
                            existing->start = active->start_pos;
                            existing->end = i + 1;
                            size_t len = existing->end - existing->start;
                            existing->value = (char*)malloc(len + 1);
                            if (existing->value) {
                                strncpy(existing->value, input + existing->start, len);
                                existing->value[len] = '\0';
                            }
                        } else {
                            // Create new completed capture
                            if (completed_count >= completed_capacity) {
                                size_t new_capacity = (completed_capacity == 0) ? 4 : completed_capacity * 2;
                                CaptureGroup *new_groups = (CaptureGroup*)realloc(completed_captures, new_capacity * sizeof(CaptureGroup));
                                if (!new_groups) {
                                    perror("Failed to realloc completed captures");
                                    exit(EXIT_FAILURE);
                                }
                                completed_captures = new_groups;
                                completed_capacity = new_capacity;
                            }
                            
                            CaptureGroup *group = &completed_captures[completed_count++];
                            group->name = active->name ? strdup(active->name) : NULL;
                            group->start = active->start_pos;
                            group->end = i + 1;
                            
                            // Extract the captured substring
                            size_t len = group->end - group->start;
                            group->value = (char*)malloc(len + 1);
                            if (group->value) {
                                strncpy(group->value, input + group->start, len);
                                group->value[len] = '\0';
                            }
                        }
                    }
                }
                
                if (s->out1->symbol == EPSILON || s->out1->symbol == CAPTURE_START || s->out1->symbol == CAPTURE_END) {
                    process_epsilon_neighbor(s->out1->to, &next_states, &stack);
                }
            }
            
            if (s->out2) {
                if (s->out2->symbol == CAPTURE_START) {
                    push_capture(&capture_stack, s->out2->capture_id, s->out2->capture_name, i + 1);
                } else if (s->out2->symbol == CAPTURE_END) {
                    ActiveCapture *active = find_active_capture(&capture_stack, s->out2->capture_id);
                    if (active) {
                        // Check if we already have a capture for this ID (update it)
                        CaptureGroup *existing = NULL;
                        for (size_t k = 0; k < completed_count; k++) {
                            if (completed_captures[k].name && active->name && 
                                strcmp(completed_captures[k].name, active->name) == 0) {
                                existing = &completed_captures[k];
                                break;
                            }
                        }
                        
                        if (existing) {
                            // Update existing capture
                            free(existing->value);
                            existing->start = active->start_pos;
                            existing->end = i + 1;
                            size_t len = existing->end - existing->start;
                            existing->value = (char*)malloc(len + 1);
                            if (existing->value) {
                                strncpy(existing->value, input + existing->start, len);
                                existing->value[len] = '\0';
                            }
                        } else {
                            // Create new completed capture
                            if (completed_count >= completed_capacity) {
                                size_t new_capacity = (completed_capacity == 0) ? 4 : completed_capacity * 2;
                                CaptureGroup *new_groups = (CaptureGroup*)realloc(completed_captures, new_capacity * sizeof(CaptureGroup));
                                if (!new_groups) {
                                    perror("Failed to realloc completed captures");
                                    exit(EXIT_FAILURE);
                                }
                                completed_captures = new_groups;
                                completed_capacity = new_capacity;
                            }
                            
                            CaptureGroup *group = &completed_captures[completed_count++];
                            group->name = active->name ? strdup(active->name) : NULL;
                            group->start = active->start_pos;
                            group->end = i + 1;
                            
                            // Extract the captured substring
                            size_t len = group->end - group->start;
                            group->value = (char*)malloc(len + 1);
                            if (group->value) {
                                strncpy(group->value, input + group->start, len);
                                group->value[len] = '\0';
                            }
                        }
                    }
                }
                
                if (s->out2->symbol == EPSILON || s->out2->symbol == CAPTURE_START || s->out2->symbol == CAPTURE_END) {
                    process_epsilon_neighbor(s->out2->to, &next_states, &stack);
                }
            }
        }
        
        free_set(&stack);

        // Update current states for the next iteration
        NfaStateSet temp_swap = current_states;
        current_states = next_states;
        next_states = temp_swap;

        // If no states are reachable after this character, fail early
        if (current_states.count == 0) {
            break;
        }
    }

    // 3. Final check: Is any state in the final set an accepting state?
    for (size_t i = 0; i < current_states.count; ++i) {
        if (current_states.states[i]->is_accepting) {
            result.matched = true;
            break;
        }
    }
    
    // Set the results
    if (result.matched) {
        result.num_groups = completed_count;
        result.groups = completed_captures;
    } else {
        // Free completed captures if match failed
        for (size_t i = 0; i < completed_count; i++) {
            free(completed_captures[i].name);
            free(completed_captures[i].value);
        }
        free(completed_captures);
    }

    // Cleanup
    free_set(&current_states);
    free_set(&next_states);
    free_set(&temp_reachable);
    free_capture_stack(&capture_stack);

    return result;
}

void free_match_result(MatchResult *result) {
    if (result && result->groups) {
        for (size_t i = 0; i < result->num_groups; i++) {
            free(result->groups[i].name);
            free(result->groups[i].value);
        }
        free(result->groups);
        result->groups = NULL;
        result->num_groups = 0;
    }
}
