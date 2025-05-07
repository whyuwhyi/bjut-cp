/**
 * @file automaton.h
 * @brief LR automaton representation (internal)
 */

#ifndef LR_AUTOMATON_H
#define LR_AUTOMATON_H

#include "../grammar.h"
#include "item.h"
#include <stdbool.h>

/**
 * @brief LR automaton state
 */
typedef struct LRState {
  int id; /* State ID */

  /* Items in this state */
  LRItem **items; /* Items in this state */
  int item_count; /* Number of items */

  /* Transitions from this state */
  struct {
    int symbol_id;         /* Symbol ID */
    struct LRState *state; /* Target state */
  } *transitions;
  int transition_count; /* Number of transitions */

  /* Core items (for state comparison) */
  int *core_items;     /* Core item indices */
  int core_item_count; /* Number of core items */
} LRState;

/**
 * @brief LR automaton
 */
typedef struct {
  LRState *start_state; /* Start state */
  LRState **states;     /* All states */
  int state_count;      /* Number of states */
  Grammar *grammar;     /* Grammar for the automaton */
} LRAutomaton;

/**
 * @brief Create an LR automaton
 *
 * @param grammar Grammar
 * @return LRAutomaton* Created automaton
 */
LRAutomaton *lr_automaton_create(Grammar *grammar);

/**
 * @brief Destroy an LR automaton
 *
 * @param automaton Automaton to destroy
 */
void lr_automaton_destroy(LRAutomaton *automaton);

/**
 * @brief Create a new LR state
 *
 * @param id State ID
 * @return LRState* Created state
 */
LRState *lr_state_create(int id);

/**
 * @brief Destroy an LR state
 *
 * @param state State to destroy
 */
void lr_state_destroy(LRState *state);

/**
 * @brief Add an item to a state
 *
 * @param state State to add to
 * @param item Item to add
 * @return bool Success status
 */
bool lr_state_add_item(LRState *state, LRItem *item);

/**
 * @brief Find an item in a state
 *
 * @param state State to search
 * @param production_id Production ID
 * @param dot_position Dot position
 * @return int Item index, or -1 if not found
 */
int lr_state_find_item(LRState *state, int production_id, int dot_position);

/**
 * @brief Add a transition to a state
 *
 * @param state State to add to
 * @param symbol_id Symbol ID
 * @param target_state Target state
 * @return bool Success status
 */
bool lr_state_add_transition(LRState *state, int symbol_id,
                             LRState *target_state);

/**
 * @brief Compare two states (core items only)
 *
 * @param state1 First state
 * @param state2 Second state
 * @return bool True if states have same core items
 */
bool lr_state_equals(LRState *state1, LRState *state2);

/**
 * @brief Compare two states including lookaheads
 *
 * @param state1 First state
 * @param state2 Second state
 * @return bool True if states are identical
 */
bool lr_state_equals_with_lookaheads(LRState *state1, LRState *state2);

/**
 * @brief Print a state's items
 *
 * @param state State to print
 * @param grammar Grammar
 */
void lr_state_print(LRState *state, Grammar *grammar);

/**
 * @brief Print the entire automaton
 *
 * @param automaton Automaton to print
 */
void lr_automaton_print(LRAutomaton *automaton);

#endif /* LR_AUTOMATON_H */
