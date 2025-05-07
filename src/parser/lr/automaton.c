/**
 * @file automaton.c
 * @brief LR automaton representation implementation
 */

#include "automaton.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create an LR automaton
 */
LRAutomaton *lr_automaton_create(Grammar *grammar) {
  if (!grammar) {
    return NULL;
  }

  LRAutomaton *automaton = (LRAutomaton *)safe_malloc(sizeof(LRAutomaton));
  if (!automaton) {
    return NULL;
  }

  automaton->start_state = NULL;
  automaton->states = NULL;
  automaton->state_count = 0;
  automaton->grammar = grammar;

  DEBUG_PRINT("Created LR automaton");
  return automaton;
}

/**
 * @brief Destroy an LR automaton
 */
void lr_automaton_destroy(LRAutomaton *automaton) {
  if (!automaton) {
    return;
  }

  if (automaton->states) {
    for (int i = 0; i < automaton->state_count; i++) {
      if (automaton->states[i]) {
        lr_state_destroy(automaton->states[i]);
      }
    }
    free(automaton->states);
  }

  free(automaton);
  DEBUG_PRINT("Destroyed LR automaton");
}

/**
 * @brief Create a new LR state
 */
LRState *lr_state_create(int id) {
  LRState *state = (LRState *)safe_malloc(sizeof(LRState));
  if (!state) {
    return NULL;
  }

  state->id = id;
  state->items = NULL;
  state->item_count = 0;
  state->transitions = NULL;
  state->transition_count = 0;
  state->core_items = NULL;
  state->core_item_count = 0;

  DEBUG_PRINT("Created LR state %d", id);
  return state;
}

/**
 * @brief Destroy an LR state
 */
void lr_state_destroy(LRState *state) {
  if (!state) {
    return;
  }

  if (state->items) {
    for (int i = 0; i < state->item_count; i++) {
      lr_item_destroy(state->items[i]);
    }
    free(state->items);
  }

  if (state->transitions) {
    free(state->transitions);
  }

  if (state->core_items) {
    free(state->core_items);
  }

  free(state);
  DEBUG_PRINT("Destroyed LR state");
}

/**
 * @brief Add an item to a state
 */
bool lr_state_add_item(LRState *state, LRItem *item) {
  if (!state || !item) {
    return false;
  }

  /* Check if item already exists */
  for (int i = 0; i < state->item_count; i++) {
    if (lr_item_equals(state->items[i], item)) {
      /* Add any new lookaheads */
      if (item->lookahead_count > 0 &&
          lr_item_add_lookaheads(state->items[i], item->lookaheads,
                                 item->lookahead_count)) {
        return true;
      }

      return false;
    }
  }

  /* Add new item */
  LRItem **new_items = (LRItem **)safe_realloc(
      state->items, (state->item_count + 1) * sizeof(LRItem *));
  if (!new_items) {
    return false;
  }

  state->items = new_items;
  state->items[state->item_count] = item;
  state->item_count++;

  /* Update core items if this is a core item */
  if (lr_item_is_core(item)) {
    int *new_core_items = (int *)safe_realloc(
        state->core_items, (state->core_item_count + 1) * sizeof(int));
    if (!new_core_items) {
      return false;
    }

    state->core_items = new_core_items;
    state->core_items[state->core_item_count] = state->item_count - 1;
    state->core_item_count++;
  }

  DEBUG_PRINT("Added item [%d, %d] to state %d", item->production_id,
              item->dot_position, state->id);
  return true;
}

/**
 * @brief Find an item in a state
 */
int lr_state_find_item(LRState *state, int production_id, int dot_position) {
  if (!state) {
    return -1;
  }

  for (int i = 0; i < state->item_count; i++) {
    if (state->items[i]->production_id == production_id &&
        state->items[i]->dot_position == dot_position) {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Add a transition to a state
 */
bool lr_state_add_transition(LRState *state, int symbol_id,
                             LRState *target_state) {
  if (!state || !target_state) {
    return false;
  }

  /* Check if transition already exists */
  for (int i = 0; i < state->transition_count; i++) {
    if (state->transitions[i].symbol_id == symbol_id) {
      state->transitions[i].state = target_state;
      DEBUG_PRINT("Updated transition from state %d on symbol %d to state %d",
                  state->id, symbol_id, target_state->id);
      return true;
    }
  }

  /* Add new transition */
  typedef struct {
    int symbol_id;
    LRState *state;
  } Transition;

  Transition *new_transitions = (Transition *)safe_realloc(
      state->transitions, (state->transition_count + 1) * sizeof(Transition));
  if (!new_transitions) {
    return false;
  }

  state->transitions = new_transitions;
  state->transitions[state->transition_count].symbol_id = symbol_id;
  state->transitions[state->transition_count].state = target_state;
  state->transition_count++;

  DEBUG_PRINT("Added transition from state %d on symbol %d to state %d",
              state->id, symbol_id, target_state->id);
  return true;
}

/**
 * @brief Compare two states (core items only)
 */
bool lr_state_equals(LRState *state1, LRState *state2) {
  if (!state1 || !state2) {
    return false;
  }

  if (state1->core_item_count != state2->core_item_count) {
    return false;
  }

  for (int i = 0; i < state1->core_item_count; i++) {
    int item1_idx = state1->core_items[i];

    bool found = false;
    for (int j = 0; j < state2->core_item_count; j++) {
      int item2_idx = state2->core_items[j];

      if (lr_item_equals(state1->items[item1_idx], state2->items[item2_idx])) {
        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Compare two states including lookaheads
 */
bool lr_state_equals_with_lookaheads(LRState *state1, LRState *state2) {
  if (!state1 || !state2) {
    return false;
  }

  if (state1->core_item_count != state2->core_item_count) {
    return false;
  }

  for (int i = 0; i < state1->core_item_count; i++) {
    int item1_idx = state1->core_items[i];

    bool found = false;
    for (int j = 0; j < state2->core_item_count; j++) {
      int item2_idx = state2->core_items[j];

      if (lr_item_equals_with_lookaheads(state1->items[item1_idx],
                                         state2->items[item2_idx])) {
        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Print a state's items
 */
void lr_state_print(LRState *state, Grammar *grammar) {
  if (!state || !grammar) {
    return;
  }

  printf("State %d:\n", state->id);
  for (int i = 0; i < state->item_count; i++) {
    printf("  ");
    lr_item_print(state->items[i], grammar);
    printf("\n");
  }

  printf("  Transitions:\n");
  for (int i = 0; i < state->transition_count; i++) {
    int symbol_id = state->transitions[i].symbol_id;
    const char *symbol_name = "";

    /* Find symbol name */
    for (int j = 0; j < grammar->terminals_count; j++) {
      if (grammar->terminal_indices[j] == symbol_id) {
        symbol_name = grammar->symbols[symbol_id].name;
        break;
      }
    }

    if (symbol_name[0] == '\0') {
      for (int j = 0; j < grammar->nonterminals_count; j++) {
        if (grammar->nonterminal_indices[j] == symbol_id) {
          symbol_name = grammar->symbols[symbol_id].name;
          break;
        }
      }
    }

    printf("    %s -> State %d\n", symbol_name,
           state->transitions[i].state->id);
  }
}

/**
 * @brief Print the entire automaton
 */
void lr_automaton_print(LRAutomaton *automaton) {
  if (!automaton) {
    return;
  }

  printf("LR Automaton:\n");
  printf("-------------\n");

  for (int i = 0; i < automaton->state_count; i++) {
    lr_state_print(automaton->states[i], automaton->grammar);
    printf("\n");
  }
}
