/**
 * @file lr_parser.c
 * @brief Common LR parser implementation
 */

#include "lr_parser.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define missing constant */
#define MAX_TERMINALS 100

/**
 * @brief Initialize LR parser data
 */
bool lr_parser_data_init(LRParserData *data) {
  if (!data) {
    return false;
  }

  data->automaton = NULL;
  data->table = NULL;
  data->state_stack = NULL;
  data->state_stack_size = 0;
  data->state_stack_capacity = 0;

  return true;
}

/**
 * @brief Clean up LR parser data
 */
void lr_parser_data_cleanup(LRParserData *data) {
  if (!data) {
    return;
  }

  if (data->automaton) {
    lr_automaton_destroy(data->automaton);
    data->automaton = NULL;
  }

  if (data->table) {
    action_table_destroy(data->table);
    data->table = NULL;
  }

  if (data->state_stack) {
    free(data->state_stack);
    data->state_stack = NULL;
    data->state_stack_size = 0;
    data->state_stack_capacity = 0;
  }
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
      int symbol_after_dot = grammar->productions[curr_item->production_id]
                                 .rhs[curr_item->dot_position];

      /* Skip if symbol is a terminal */
      bool is_nonterminal = false;
      for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
        if (grammar->nonterminal_indices[nt] == symbol_after_dot) {
          is_nonterminal = true;
          break;
        }
      }

      if (!is_nonterminal) {
        continue;
      }

      /* Find all productions with this non-terminal on the LHS */
      for (int p = 0; p < grammar->production_count; p++) {
        if (grammar->productions[p].lhs == symbol_after_dot) {
          /* Add new item to closure */
          LRItem *new_item = lr_item_create(p, 0);
          if (!new_item) {
            return false;
          }

          /* Handle lookaheads for LR(1) */
          if (use_lookaheads) {
            /* Calculate FIRST of the string after dot position */
            // For now, we'll simplify and just use the current lookaheads
            // In a complete implementation, you'd calculate FIRST(beta a)
            // where beta is the remainder of the current production after dot
            // and a is the current lookahead

            // Note: We're commenting out these unused variables to avoid
            // warnings bool first_set[MAX_TERMINALS + 1]; memset(first_set, 0,
            // sizeof(first_set));

            /* Get remainder of current production after dot */
            int beta_start = curr_item->dot_position + 1;
            int beta_end =
                grammar->productions[curr_item->production_id].rhs_length;

            /* Calculate FIRST set for the remainder */
            // This would involve a more complex calculation
            // For simplicity, we'll just copy the current lookaheads

            // int lookaheads[MAX_TERMINALS];
            int lookahead_count = 0;

            if (curr_item->lookahead_count > 0) {
              new_item->lookaheads =
                  (int *)safe_malloc(curr_item->lookahead_count * sizeof(int));
              if (!new_item->lookaheads) {
                lr_item_destroy(new_item);
                return false;
              }

              for (int la = 0; la < curr_item->lookahead_count; la++) {
                new_item->lookaheads[la] = curr_item->lookaheads[la];
              }

              new_item->lookahead_count = curr_item->lookahead_count;
            }
          }

          /* Add item to state if not already present */
          if (lr_state_add_item(state, new_item)) {
            added = true;
          } else {
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
LRState *lr_goto(LRAutomaton *automaton, LRState *state, int symbol) {
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
            grammar->productions[curr_item->production_id].rhs_length &&
        grammar->productions[curr_item->production_id]
                .rhs[curr_item->dot_position] == symbol) {
      /* Create a new item with dot advanced */
      LRItem *new_item =
          lr_item_create(curr_item->production_id, curr_item->dot_position + 1);
      if (!new_item) {
        lr_state_destroy(new_state);
        return NULL;
      }

      /* Copy lookaheads if this is an LR(1) item */
      if (curr_item->lookahead_count > 0) {
        new_item->lookaheads =
            (int *)safe_malloc(curr_item->lookahead_count * sizeof(int));
        if (!new_item->lookaheads) {
          lr_item_destroy(new_item);
          lr_state_destroy(new_state);
          return NULL;
        }

        for (int la = 0; la < curr_item->lookahead_count; la++) {
          new_item->lookaheads[la] = curr_item->lookaheads[la];
        }

        new_item->lookahead_count = curr_item->lookahead_count;
      }

      /* Add item to new state */
      if (!lr_state_add_item(new_state, new_item)) {
        lr_item_destroy(new_item);
        lr_state_destroy(new_state);
        return NULL;
      }
    }
  }

  /* Check if new state is empty */
  if (new_state->item_count == 0) {
    lr_state_destroy(new_state);
    return NULL;
  }

  /* Perform closure on new state */
  if (!lr_closure(automaton, new_state,
                  new_state->items[0]->lookahead_count > 0)) {
    lr_state_destroy(new_state);
    return NULL;
  }

  /* Check if state already exists in automaton */
  for (int s = 0; s < automaton->state_count; s++) {
    if (lr_state_equals(new_state, automaton->states[s])) {
      /* State already exists, use existing state */
      lr_state_destroy(new_state);
      return automaton->states[s];
    }
  }

  /* Add new state to automaton */
  automaton->states[automaton->state_count++] = new_state;

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
  LRItem *start_item = lr_item_create(0, 0); /* Production 0 is S' -> S */
  if (!start_item) {
    lr_state_destroy(initial_state);
    return false;
  }

  /* Add end-of-input lookahead for LR(1) */
  if (use_lookaheads) {
    /* Find end token index */
    int end_token_idx = -1;
    for (int i = 0; i < grammar->terminals_count; i++) {
      if (grammar->symbols[grammar->terminal_indices[i]].token == TK_SEMI) {
        end_token_idx = i;
        break;
      }
    }

    if (end_token_idx >= 0) {
      start_item->lookaheads = (int *)safe_malloc(sizeof(int));
      if (!start_item->lookaheads) {
        lr_item_destroy(start_item);
        lr_state_destroy(initial_state);
        return false;
      }

      start_item->lookaheads[0] = end_token_idx;
      start_item->lookahead_count = 1;
    }
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

  /* Process states until no new states are added */
  int processed = 0;
  while (processed < automaton->state_count) {
    LRState *state = automaton->states[processed++];

    /* Find all symbols after dot in this state */
    bool symbols_after_dot[MAX_SYMBOLS];
    memset(symbols_after_dot, 0, sizeof(symbols_after_dot));

    for (int i = 0; i < state->item_count; i++) {
      LRItem *item = state->items[i];

      /* Check if dot is not at the end */
      if (item->dot_position <
          grammar->productions[item->production_id].rhs_length) {
        int symbol =
            grammar->productions[item->production_id].rhs[item->dot_position];
        symbols_after_dot[symbol] = true;
      }
    }

    /* Compute GOTO for each symbol */
    for (int symbol = 0; symbol < MAX_SYMBOLS; symbol++) {
      if (symbols_after_dot[symbol]) {
        LRState *target = lr_goto(automaton, state, symbol);

        if (target) {
          /* Add transition */
          if (!lr_state_add_transition(state, symbol, target)) {
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
 * @brief Parse input using LR parsing
 */
SyntaxTree *lr_parser_parse(Parser *parser, LRParserData *data, Lexer *lexer) {
  if (!parser || !data || !lexer) {
    return NULL;
  }

  Grammar *grammar = parser->grammar;
  ProductionTracker *tracker = parser->production_tracker;

  /* Create syntax tree */
  SyntaxTree *tree = syntax_tree_create();
  if (!tree) {
    return NULL;
  }

  /* Initialize state stack */
  data->state_stack_capacity = 1024;
  data->state_stack =
      (int *)safe_malloc(data->state_stack_capacity * sizeof(int));
  if (!data->state_stack) {
    syntax_tree_destroy(tree);
    return NULL;
  }

  data->state_stack[0] = 0; /* Start in state 0 */
  data->state_stack_size = 1;

  /* Initialize semantic value stack */
  SyntaxTreeNode **value_stack = (SyntaxTreeNode **)safe_malloc(
      data->state_stack_capacity * sizeof(SyntaxTreeNode *));
  if (!value_stack) {
    syntax_tree_destroy(tree);
    return NULL;
  }

  int value_stack_size = 0;

  /* Get first token */
  Token token = lexer_get_token(lexer);

  /* Main parsing loop */
  while (1) {
    int state = data->state_stack[data->state_stack_size - 1];
    int token_idx = -1;

    /* Find token index in grammar */
    for (int i = 0; i < grammar->terminals_count; i++) {
      if (grammar->symbols[grammar->terminal_indices[i]].token == token.type) {
        token_idx = i;
        break;
      }
    }

    if (token_idx < 0) {
      fprintf(stderr,
              "Syntax error: Unexpected token '%.*s' at line %d, column %d\n",
              token.length, token.lexeme, token.line, token.column);
      syntax_tree_destroy(tree);
      free(value_stack);
      return NULL;
    }

    /* Get action from table */
    int action = action_table_get_action(data->table, state, token_idx);
    int target = action_table_get_target(data->table, state, token_idx);

    /* Debug output */
    DEBUG_PRINT("State %d, Token %.*s, Action %d, Target %d", state,
                token.length, token.lexeme, action, target);

    /* Process action */
    if (action == ACTION_SHIFT) {
      /* Shift token and go to next state */
      DEBUG_PRINT("Shift to state %d", target);

      /* Create node for token */
      SyntaxTreeNode *node = syntax_tree_create_node(tree);
      if (!node) {
        syntax_tree_destroy(tree);
        free(value_stack);
        return NULL;
      }

      node->type = token.type;
      node->value.string_val = safe_strndup(token.lexeme, token.length);
      node->lineno = token.line;

      /* Push node to value stack */
      value_stack[value_stack_size++] = node;

      /* Push next state to state stack */
      data->state_stack[data->state_stack_size++] = target;

      /* Get next token */
      token = lexer_get_token(lexer);

    } else if (action == ACTION_REDUCE) {
      /* Reduce by production */
      Production *prod = &grammar->productions[target];
      DEBUG_PRINT("Reduce by production %d: %s -> ...", target,
                  grammar->symbols[prod->lhs].name);

      /* Create node for non-terminal */
      SyntaxTreeNode *node = syntax_tree_create_node(tree);
      if (!node) {
        syntax_tree_destroy(tree);
        free(value_stack);
        return NULL;
      }

      node->type = grammar->symbols[prod->lhs].nonterminal_type;
      node->lineno = value_stack[value_stack_size - prod->rhs_length]->lineno;

      /* Add children in reverse order */
      for (int i = prod->rhs_length - 1; i >= 0; i--) {
        int child_idx = value_stack_size - prod->rhs_length + i;
        syntax_tree_add_child(node, value_stack[child_idx]);
      }

      /* Pop states and values from stacks */
      data->state_stack_size -= prod->rhs_length;
      value_stack_size -= prod->rhs_length;

      /* Get goto state */
      state = data->state_stack[data->state_stack_size - 1];
      int nt_idx = -1;

      /* Find non-terminal index in grammar */
      for (int i = 0; i < grammar->nonterminals_count; i++) {
        if (grammar->nonterminal_indices[i] == prod->lhs) {
          nt_idx = i;
          break;
        }
      }

      int goto_state = action_table_get_goto(data->table, state, nt_idx);
      DEBUG_PRINT("Goto state %d", goto_state);

      /* Push node to value stack */
      value_stack[value_stack_size++] = node;

      /* Push goto state to state stack */
      data->state_stack[data->state_stack_size++] = goto_state;

      /* Record production for derivation */
      production_tracker_add(tracker, target);

    } else if (action == ACTION_ACCEPT) {
      /* Accept input */
      DEBUG_PRINT("Accept input");

      /* Set root of syntax tree */
      syntax_tree_set_root(tree, value_stack[0]);
      free(value_stack);
      return tree;

    } else {
      /* Syntax error */
      fprintf(stderr,
              "Syntax error: Unexpected token '%.*s' at line %d, column %d\n",
              token.length, token.lexeme, token.line, token.column);
      syntax_tree_destroy(tree);
      free(value_stack);
      return NULL;
    }
  }
}

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
      Symbol symbol_after_dot = grammar->productions[curr_item->production_id]
                                    .rhs[curr_item->dot_position];

      /* Skip if symbol is a terminal */
      bool is_nonterminal = false;
      for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
        if (grammar->nonterminal_indices[nt] == symbol_after_dot.id) {
          is_nonterminal = true;
          break;
        }
      }

      if (!is_nonterminal) {
        continue;
      }

      /* Find all productions with this non-terminal on the LHS */
      for (int p = 0; p < grammar->productions_count; p++) {
        if (grammar->productions[p].lhs == symbol_after_dot.id) {
          /* Add new item to closure */
          LRItem *new_item = lr_item_create(p, 0, NULL, 0);
          if (!new_item) {
            return false;
          }

          /* Handle lookaheads for LR(1) */
          if (use_lookaheads) {
            /* Get remainder of current production after dot */
            int beta_start = curr_item->dot_position + 1;
            int beta_end =
                grammar->productions[curr_item->production_id].rhs_length;

            /* Copy lookaheads if present */
            if (curr_item->lookahead_count > 0) {
              new_item->lookaheads =
                  (int *)safe_malloc(curr_item->lookahead_count * sizeof(int));
              if (!new_item->lookaheads) {
                lr_item_destroy(new_item);
                return false;
              }

              for (int la = 0; la < curr_item->lookahead_count; la++) {
                new_item->lookaheads[la] = curr_item->lookaheads[la];
              }

              new_item->lookahead_count = curr_item->lookahead_count;
            }
          }

          /* Add item to state if not already present */
          if (lr_state_add_item(state, new_item)) {
            added = true;
          } else {
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
    if (curr_item->dot_position grammar->productions[curr_item->production_id]
            .rhs_length &&
        grammar->productions[curr_item->production_id]
                .rhs[curr_item->dot_position]
                .id == symbol_id) {
      /* Create a new item with dot advanced */
      LRItem *new_item = lr_item_create(curr_item->production_id,
                                        curr_item->dot_position + 1, NULL, 0);
      if (!new_item) {
        lr_state_destroy(new_state);
        return NULL;
      }

      /* Copy lookaheads if this is an LR(1) item */
      if (curr_item->lookahead_count > 0) {
        new_item->lookaheads =
            (int *)safe_malloc(curr_item->lookahead_count * sizeof(int));
        if (!new_item->lookaheads) {
          lr_item_destroy(new_item);
          lr_state_destroy(new_state);
          return NULL;
        }

        for (int la = 0; la < curr_item->lookahead_count; la++) {
          new_item->lookaheads[la] = curr_item->lookaheads[la];
        }

        new_item->lookahead_count = curr_item->lookahead_count;
      }

      /* Add item to new state */
      if (!lr_state_add_item(new_state, new_item)) {
        lr_item_destroy(new_item);
        lr_state_destroy(new_state);
        return NULL;
      }
    }
  }

  /* Check if new state is empty */
  if (new_state->item_count == 0) {
    lr_state_destroy(new_state);
    return NULL;
  }

  /* Perform closure on new state */
  if (!lr_closure(automaton, new_state,
                  new_state->items[0]->lookahead_count > 0)) {
    lr_state_destroy(new_state);
    return NULL;
  }

  /* Check if state already exists in automaton */
  for (int s = 0; s < automaton->state_count; s++) {
    if (lr_state_equals(new_state, automaton->states[s])) {
      /* State already exists, use existing state */
      lr_state_destroy(new_state);
      return automaton->states[s];
    }
  }

  /* Add new state to automaton */
  automaton->states[automaton->state_count++] = new_state;

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
  LRItem *start_item =
      lr_item_create(0, 0, NULL, 0); /* Production 0 is S' -> S */
  if (!start_item) {
    lr_state_destroy(initial_state);
    return false;
  }

  /* Add end-of-input lookahead for LR(1) */
  if (use_lookaheads) {
    /* Find end token index */
    int end_token_idx = -1;
    for (int i = 0; i < grammar->terminals_count; i++) {
      if (grammar->symbols[grammar->terminal_indices[i]].token == TK_SEMI) {
        end_token_idx = i;
        break;
      }
    }

    if (end_token_idx >= 0) {
      start_item->lookaheads = (int *)safe_malloc(sizeof(int));
      if (!start_item->lookaheads) {
        lr_item_destroy(start_item);
        lr_state_destroy(initial_state);
        return false;
      }

      start_item->lookaheads[0] = end_token_idx;
      start_item->lookahead_count = 1;
    }
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

  /* Process states until no new states are added */
  int processed = 0;
  while (processed < automaton->state_count) {
    LRState *state = automaton->states[processed++];

    /* Find all symbols after dot in this state */
    bool symbols_after_dot[MAX_SYMBOLS];
    memset(symbols_after_dot, 0, sizeof(symbols_after_dot));

    for (int i = 0; i < state->item_count; i++) {
      LRItem *item = state->items[i];

      /* Check if dot is not at the end */
      if (item->dot_position grammar->productions[item->production_id]
              .rhs_length) {
        int symbol_id = grammar->productions[item->production_id]
                            .rhs[item->dot_position]
                            .id;
        symbols_after_dot[symbol_id] = true;
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

  /* Check if first symbol is a terminal */
  bool is_terminal = false;
  int terminal_idx = -1;

  for (int i = 0; i < grammar->terminals_count; i++) {
    if (grammar->terminal_indices[i] == symbols[0].id) {
      is_terminal = true;
      terminal_idx = i;
      break;
    }
  }

  if (is_terminal) {
    /* First symbol is a terminal, FIRST is just that terminal */
    first_set[terminal_idx] = true;
    return true;
  }

  /* First symbol is a non-terminal, use its FIRST set */
  bool all_can_derive_epsilon = true;

  for (int i = 0; i < symbol_count && all_can_derive_epsilon; i++) {
    bool symbol_can_derive_epsilon = false;

    /* Check if symbol is a terminal */
    is_terminal = false;
    terminal_idx = -1;

    for (int t = 0; t < grammar->terminals_count; t++) {
      if (grammar->terminal_indices[t] == symbols[i].id) {
        is_terminal = true;
        terminal_idx = t;
        break;
      }
    }

    if (is_terminal) {
      /* Terminal's FIRST is itself */
      first_set[terminal_idx] = true;
      all_can_derive_epsilon = false;
    } else {
      /* Non-terminal, add its FIRST set (except epsilon) */
      int nt_idx = -1;

      for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
        if (grammar->nonterminal_indices[nt] == symbols[i].id) {
          nt_idx = nt;
          break;
        }
      }

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
