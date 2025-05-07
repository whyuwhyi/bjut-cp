/**
 * @file lr_parser.c
 * @brief Common LR parser implementation
 */

#include "lr_parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define missing constants */
#define MAX_TERMINALS 100
#define MAX_SYMBOLS 200

/**
 * @brief Get symbol ID from a symbol
 */
int get_symbol_id(Grammar *grammar, Symbol *symbol) {
  if (!grammar || !symbol) {
    return -1;
  }

  if (symbol->type == SYMBOL_TERMINAL) {
    /* Find terminal index */
    for (int i = 0; i < grammar->terminals_count; i++) {
      if (grammar->symbols[grammar->terminal_indices[i]].token ==
          symbol->token) {
        return grammar->terminal_indices[i];
      }
    }
  } else if (symbol->type == SYMBOL_NONTERMINAL) {
    return grammar->nonterminal_indices[symbol->nonterminal];
  }

  return -1;
}

/**
 * @brief Check if a symbol is a terminal
 */
bool is_terminal_symbol(Grammar *grammar, Symbol *symbol) {
  if (!grammar || !symbol) {
    return false;
  }

  return symbol->type == SYMBOL_TERMINAL;
}

/**
 * @brief Check if a symbol is a non-terminal
 */
bool is_nonterminal_symbol(Grammar *grammar, Symbol *symbol) {
  if (!grammar || !symbol) {
    return false;
  }

  return symbol->type == SYMBOL_NONTERMINAL;
}

/**
 * @brief Perform closure operation on a set of LR items
 */
bool lr_closure(LRAutomaton *automaton, LRState *state, bool use_lookaheads) {
  if (!automaton || !state) {
    return false;
  }

  Grammar *grammar = automaton->grammar;
  if (!grammar) {
    return false;
  }

  bool added;
  do {
    added = false;

    /* Check each item in the state */
    for (int i = 0; i < state->item_count; i++) {
      LRItem *curr_item = state->items[i];

      /* Skip if dot is at the end */
      if (curr_item->dot_position >=
          grammar->productions[curr_item->production_id].rhs_length) {
        continue;
      }

      /* Get symbol after dot */
      Symbol *symbol_after_dot = &grammar->productions[curr_item->production_id]
                                      .rhs[curr_item->dot_position];

      /* Skip if symbol is not a non-terminal */
      if (symbol_after_dot->type != SYMBOL_NONTERMINAL) {
        continue;
      }

      int nonterminal_id = symbol_after_dot->nonterminal;

      /* Find all productions with this non-terminal on the LHS */
      for (int p = 0; p < grammar->productions_count; p++) {
        if (grammar->productions[p].lhs == nonterminal_id) {
          /* Create a new item */
          LRItem *new_item;

          if (use_lookaheads) {
            /* For LR(1) items, calculate lookaheads */
            /* This is simplified for now */
            int lookaheads[1] = {0}; // Simplified
            new_item = lr_item_create(p, 0, lookaheads, 1);
          } else {
            /* For LR(0) items */
            new_item = lr_item_create(p, 0, NULL, 0);
          }

          if (!new_item) {
            return false;
          }

          /* Try to add the item to the state */
          if (lr_state_add_item(state, new_item)) {
            /* Item was added, keep track for loop condition */
            added = true;
          } else {
            /* Item already exists, we need to free it */
            lr_item_destroy(new_item);
          }
        }
      }
    }
  } while (added);

  return true;
}

/**
 * @brief Compute the GOTO function for an LR state
 */
LRState *lr_goto(LRAutomaton *automaton, LRState *state, int symbol_id) {
  if (!automaton || !state) {
    return NULL;
  }

  /* Create a new state */
  LRState *new_state = lr_state_create(automaton->state_count);
  if (!new_state) {
    return NULL;
  }

  /* Find all items where the dot is before the symbol */
  for (int i = 0; i < state->item_count; i++) {
    LRItem *curr_item = state->items[i];
    Grammar *grammar = automaton->grammar;

    /* Check if dot is before the symbol */
    if (curr_item->dot_position <
        grammar->productions[curr_item->production_id].rhs_length) {
      /* Get the symbol after the dot */
      Symbol *symbol = &grammar->productions[curr_item->production_id]
                            .rhs[curr_item->dot_position];

      /* Check if this is the symbol we're looking for */
      int curr_symbol_id = get_symbol_id(grammar, symbol);

      if (curr_symbol_id == symbol_id) {
        /* Create a new item with dot advanced */
        LRItem *new_item;

        if (curr_item->lookahead_count > 0) {
          /* Copy lookaheads for LR(1) items */
          new_item = lr_item_create(
              curr_item->production_id, curr_item->dot_position + 1,
              curr_item->lookaheads, curr_item->lookahead_count);
        } else {
          /* LR(0) item */
          new_item = lr_item_create_lr0(curr_item->production_id,
                                        curr_item->dot_position + 1);
        }

        if (!new_item) {
          lr_state_destroy(new_state);
          return NULL;
        }

        /* Add item to new state */
        if (!lr_state_add_item(new_state, new_item)) {
          lr_item_destroy(new_item);
          lr_state_destroy(new_state);
          return NULL;
        }
      }
    }
  }

  /* Check if new state is empty */
  if (new_state->item_count == 0) {
    lr_state_destroy(new_state);
    return NULL;
  }

  /* Perform closure on new state */
  bool use_lookaheads = new_state->items[0]->lookahead_count > 0;
  if (!lr_closure(automaton, new_state, use_lookaheads)) {
    lr_state_destroy(new_state);
    return NULL;
  }

  /* Check if state already exists in automaton */
  for (int s = 0; s < automaton->state_count; s++) {
    /* Fix: Check for state equality regardless of lookaheads flag */
    if (use_lookaheads) {
      if (lr_state_equals_with_lookaheads(new_state, automaton->states[s])) {
        /* State already exists, use existing state */
        lr_state_destroy(new_state);
        return automaton->states[s];
      }
    } else {
      if (lr_state_equals(new_state, automaton->states[s])) {
        /* State already exists, use existing state */
        lr_state_destroy(new_state);
        return automaton->states[s];
      }
    }
  }

  /* Add new state to automaton */
  if (!lr_automaton_add_state(automaton, new_state)) {
    lr_state_destroy(new_state);
    return NULL;
  }

  return new_state;
}

/**
 * @brief Create canonical collection of LR items
 */
bool lr_create_canonical_collection(LRAutomaton *automaton, Grammar *grammar,
                                    bool use_lookaheads) {
  if (!automaton || !grammar) {
    return false;
  }

  /* Create initial state with [S' -> .S] */
  LRState *initial_state = lr_state_create(0);
  if (!initial_state) {
    return false;
  }

  /* Create item for augmented grammar start symbol */
  LRItem *start_item = NULL;

  if (use_lookaheads) {
    /* Find end token index for lookahead */
    int end_token_idx = get_terminal_index(grammar, TK_SEMI);

    if (end_token_idx >= 0) {
      int lookaheads[1] = {end_token_idx};
      start_item = lr_item_create(0, 0, lookaheads, 1);
    } else {
      start_item = lr_item_create_lr0(0, 0);
    }
  } else {
    start_item = lr_item_create_lr0(0, 0);
  }

  if (!start_item) {
    lr_state_destroy(initial_state);
    return false;
  }

  /* Add item to initial state */
  if (!lr_state_add_item(initial_state, start_item)) {
    lr_item_destroy(start_item);
    lr_state_destroy(initial_state);
    return false;
  }

  /* Perform closure on initial state */
  if (!lr_closure(automaton, initial_state, use_lookaheads)) {
    lr_state_destroy(initial_state);
    return false;
  }

  /* Add initial state to automaton */
  if (!lr_automaton_add_state(automaton, initial_state)) {
    lr_state_destroy(initial_state);
    return false;
  }
  automaton->start_state = initial_state;

  /* Process states until no new states are added */
  int processed = 0;
  while (processed < automaton->state_count) {
    LRState *state = automaton->states[processed++];

    /* Find all symbols after dot in this state */
    bool symbols_after_dot[MAX_SYMBOLS] = {false};

    for (int i = 0; i < state->item_count; i++) {
      LRItem *item = state->items[i];

      /* Check if dot is not at the end */
      if (item->dot_position <
          grammar->productions[item->production_id].rhs_length) {
        /* Get symbol after dot */
        Symbol *symbol =
            &grammar->productions[item->production_id].rhs[item->dot_position];

        /* Get symbol ID */
        int symbol_id = get_symbol_id(grammar, symbol);

        if (symbol_id >= 0 && symbol_id < MAX_SYMBOLS) {
          symbols_after_dot[symbol_id] = true;
        }
      }
    }

    /* Compute GOTO for each symbol */
    for (int symbol_id = 0; symbol_id < MAX_SYMBOLS; symbol_id++) {
      if (symbols_after_dot[symbol_id]) {
        LRState *target = lr_goto(automaton, state, symbol_id);

        if (target) {
          /* Add transition */
          if (!lr_state_add_transition(state, symbol_id, target)) {
            return false;
          }
        }
      }
    }
  }

  DEBUG_PRINT("Created canonical collection with %d states",
              automaton->state_count);
  return true;
}

/**
 * @brief Calculate FIRST set for a sequence of symbols
 */
bool lr_calculate_first_of_sequence(Grammar *grammar, Symbol *symbols,
                                    int symbol_count, bool *first_set) {
  if (!grammar || !symbols || !first_set || symbol_count < 0) {
    return false;
  }

  /* Start with empty set */
  memset(first_set, 0, (grammar->terminals_count + 1) * sizeof(bool));

  /* Epsilon is at index terminals_count */
  int eps_idx = grammar->terminals_count;

  /* Empty sequence generates epsilon */
  if (symbol_count == 0) {
    first_set[eps_idx] = true;
    return true;
  }

  /* Check each symbol in the sequence */
  bool all_can_derive_epsilon = true;

  for (int i = 0; i < symbol_count && all_can_derive_epsilon; i++) {
    bool symbol_can_derive_epsilon = false;
    Symbol *symbol = &symbols[i];

    if (symbol->type == SYMBOL_TERMINAL) {
      /* Terminal's FIRST is itself */
      int term_idx = get_terminal_index(grammar, symbol->token);
      if (term_idx >= 0) {
        first_set[term_idx] = true;
      }
      all_can_derive_epsilon = false;
    } else if (symbol->type == SYMBOL_NONTERMINAL) {
      /* Non-terminal, add its FIRST set (except epsilon) */
      int nt_idx = get_nonterminal_index(grammar, symbol->nonterminal);

      if (nt_idx >= 0) {
        /* Copy non-epsilon terminals from FIRST set */
        for (int t = 0; t < grammar->terminals_count; t++) {
          if (grammar->first_sets[nt_idx][t]) {
            first_set[t] = true;
          }
        }

        /* Check if epsilon is in FIRST */
        if (grammar->first_sets[nt_idx][eps_idx]) {
          symbol_can_derive_epsilon = true;
        } else {
          all_can_derive_epsilon = false;
        }
      } else {
        /* Unknown non-terminal */
        all_can_derive_epsilon = false;
      }
    } else if (symbol->type == SYMBOL_EPSILON) {
      /* Epsilon can derive epsilon */
      symbol_can_derive_epsilon = true;
    } else {
      /* Unknown symbol type */
      all_can_derive_epsilon = false;
    }

    /* If this symbol cannot derive epsilon, stop adding FIRST sets */
    if (!symbol_can_derive_epsilon) {
      all_can_derive_epsilon = false;
    }
  }

  /* If all symbols can derive epsilon, add epsilon to FIRST */
  if (all_can_derive_epsilon) {
    first_set[eps_idx] = true;
  }

  return true;
}
