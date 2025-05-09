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

      /* Calculate FIRST set of β part */
      /* Use dynamic allocation instead of stack array */
      bool *beta_first =
          (bool *)calloc(grammar->terminals_count + 1, sizeof(bool));
      if (!beta_first) {
        return false;
      }

      /* Get the number of symbols on the right side of the current production
       */
      Production *curr_prod = &grammar->productions[curr_item->production_id];
      int rhs_length = curr_prod->rhs_length;

      /* If there are multiple symbols after the dot, calculate FIRST(β) */
      if (curr_item->dot_position + 1 < rhs_length) {
        /* Dynamically allocate memory for beta symbols */
        Symbol *beta_symbols = (Symbol *)malloc(
            (rhs_length - curr_item->dot_position - 1) * sizeof(Symbol));
        if (!beta_symbols) {
          free(beta_first);
          return false;
        }

        int beta_length = 0;
        for (int j = curr_item->dot_position + 1; j < rhs_length; j++) {
          beta_symbols[beta_length++] = curr_prod->rhs[j];
        }

        /* Calculate FIRST(β) */
        lr_calculate_first_of_sequence(grammar, beta_symbols, beta_length,
                                       beta_first);

        free(beta_symbols);
      } else {
        /* If β is empty, then FIRST(β) contains ε */
        beta_first[grammar->terminals_count] = true; /* epsilon position */
      }

      /* Find all productions with this non-terminal on the LHS */
      for (int p = 0; p < grammar->productions_count; p++) {
        if (grammar->productions[p].lhs == nonterminal_id) {
          /* Calculate lookaheads for the new item */
          int *new_lookaheads = NULL;
          int new_lookahead_count = 0;

          if (use_lookaheads) {
            /* For simplicity, first count the new lookaheads */
            for (int t = 0; t < grammar->terminals_count; t++) {
              if (beta_first[t]) {
                new_lookahead_count++;
              }
            }

            /* If FIRST(β) contains ε, add all lookaheads from the parent item
             */
            if (beta_first[grammar->terminals_count] &&
                curr_item->lookahead_count > 0) {
              new_lookahead_count += curr_item->lookahead_count;
            }

            /* Allocate lookahead array */
            if (new_lookahead_count > 0) {
              new_lookaheads = (int *)malloc(new_lookahead_count * sizeof(int));
              if (!new_lookaheads) {
                free(beta_first);
                return false;
              }

              /* First add terminals from FIRST(β) */
              int idx = 0;
              for (int t = 0; t < grammar->terminals_count; t++) {
                if (beta_first[t]) {
                  new_lookaheads[idx++] = t;
                }
              }

              /* If FIRST(β) contains ε, add lookaheads from parent item */
              if (beta_first[grammar->terminals_count] &&
                  curr_item->lookahead_count > 0) {
                for (int j = 0; j < curr_item->lookahead_count; j++) {
                  /* Check if already exists */
                  bool exists = false;
                  for (int k = 0; k < idx; k++) {
                    if (new_lookaheads[k] == curr_item->lookaheads[j]) {
                      exists = true;
                      break;
                    }
                  }

                  if (!exists && idx < new_lookahead_count) {
                    new_lookaheads[idx++] = curr_item->lookaheads[j];
                  }
                }
              }

              /* Adjust the actual lookahead count */
              new_lookahead_count = idx;
            }
          }

          /* Create new item */
          LRItem *new_item =
              lr_item_create(p, 0, new_lookaheads, new_lookahead_count);

          /* Free the temporarily allocated lookahead array */
          if (new_lookaheads) {
            free(new_lookaheads);
          }

          if (!new_item) {
            free(beta_first);
            return false;
          }

          /* Try to add the item to the state */
          if (lr_state_add_item(state, new_item)) {
            /* Item was added to the state's items array */
            added = true;
          } else {
            /* Item was not added, need to free it */
            lr_item_destroy(new_item);
          }
        }
      }

      /* Free the beta_first array */
      free(beta_first);
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

        if (curr_item->lookahead_count > 0 && curr_item->lookaheads) {
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

  /* Find augmented grammar start production */
  int augmented_prod_idx = -1;
  for (int i = 0; i < grammar->productions_count; i++) {
    if (grammar->productions[i].lhs == grammar->start_symbol) {
      augmented_prod_idx = i;
      DEBUG_PRINT("Found augmented grammar start production at index %d",
                  augmented_prod_idx);
      break;
    }
  }

  if (augmented_prod_idx < 0) {
    DEBUG_PRINT("ERROR: Could not find augmented grammar start production");
    lr_state_destroy(initial_state);
    return false;
  }

  /* Create item for augmented grammar start symbol */
  LRItem *start_item = NULL;

  if (use_lookaheads) {
    /* Find EOF token index for lookahead */
    int end_token_idx = -1;
    for (int i = 0; i < grammar->terminals_count; i++) {
      if (grammar->symbols[grammar->terminal_indices[i]].token == TK_EOF) {
        end_token_idx = i;
        DEBUG_PRINT("Found EOF token at index %d", end_token_idx);
        break;
      }
    }

    if (end_token_idx < 0) {
      DEBUG_PRINT("WARNING: EOF token not found, using fallbacks");
      /* If no EOF token found, use semicolon or first terminal */
      for (int i = 0; i < grammar->terminals_count; i++) {
        if (grammar->symbols[grammar->terminal_indices[i]].token == TK_SEMI) {
          end_token_idx = i;
          DEBUG_PRINT("Using semicolon as fallback at index %d", end_token_idx);
          break;
        }
      }

      if (end_token_idx < 0 && grammar->terminals_count > 0) {
        end_token_idx = 0; /* Use first terminal if nothing else found */
        DEBUG_PRINT("Using first terminal as fallback at index %d",
                    end_token_idx);
      }
    }

    if (end_token_idx >= 0) {
      int lookaheads[1] = {end_token_idx};
      start_item = lr_item_create(augmented_prod_idx, 0, lookaheads, 1);
      DEBUG_PRINT("Created start item with lookahead %d", end_token_idx);
    } else {
      /* Fall back to LR(0) if no terminals found */
      start_item = lr_item_create_lr0(augmented_prod_idx, 0);
      use_lookaheads = false;
      DEBUG_PRINT("WARNING: Falling back to LR(0) due to missing terminals");
    }
  } else {
    start_item = lr_item_create_lr0(augmented_prod_idx, 0);
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
  automaton->states[automaton->state_count++] = initial_state;
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
