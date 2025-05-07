/**
 * @file lr_common.c
 * @brief Common LR parser utilities implementation
 */

#include "lr_common.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize LR parser data
 */
bool lr_parser_data_init(LRParserData *data) {
  if (!data) {
    return false;
  }

  /* Initialize parser data */
  memset(data, 0, sizeof(LRParserData));

  /* Allocate stacks */
  data->state_stack = (int *)safe_malloc(INITIAL_STACK_CAPACITY * sizeof(int));
  if (!data->state_stack) {
    return false;
  }

  data->node_stack = (SyntaxTreeNode **)safe_malloc(INITIAL_STACK_CAPACITY *
                                                    sizeof(SyntaxTreeNode *));
  if (!data->node_stack) {
    free(data->state_stack);
    return false;
  }

  data->stack_capacity = INITIAL_STACK_CAPACITY;
  data->stack_top = -1;

  DEBUG_PRINT("Initialized LR parser data");
  return true;
}

/**
 * @brief Reset LR parser data for a new parse
 */
bool lr_parser_data_reset(LRParserData *data, Lexer *lexer) {
  if (!data || !lexer) {
    return false;
  }

  data->lexer = lexer;
  data->current_token = 0;
  data->stack_top = 0;
  data->state_stack[0] = 0; /* Start in state 0 */
  data->node_stack[0] = NULL;
  data->has_error = false;
  memset(data->error_message, 0, sizeof(data->error_message));

  /* Create new syntax tree */
  data->syntax_tree = syntax_tree_create();
  if (!data->syntax_tree) {
    return false;
  }

  DEBUG_PRINT("Reset LR parser data for new parse");
  return true;
}

/**
 * @brief Clean up LR parser data
 */
void lr_parser_data_cleanup(LRParserData *data) {
  if (!data) {
    return;
  }

  /* Free stacks */
  if (data->state_stack) {
    free(data->state_stack);
    data->state_stack = NULL;
  }

  if (data->node_stack) {
    /* Nodes are owned by the syntax tree, don't free them */
    free(data->node_stack);
    data->node_stack = NULL;
  }

  /* Free automaton */
  if (data->automaton) {
    lr_automaton_destroy(data->automaton);
    data->automaton = NULL;
  }

  /* Free parsing table */
  if (data->table) {
    action_table_destroy(data->table);
    data->table = NULL;
  }

  DEBUG_PRINT("Cleaned up LR parser data");
}

/**
 * @brief Get the current token from the lexer
 */
static const Token *get_current_token(LRParserData *data) {
  if (!data || !data->lexer) {
    return NULL;
  }

  return lexer_get_token(data->lexer, data->current_token);
}

/**
 * @brief Move to the next token
 */
static const Token *next_token(LRParserData *data) {
  if (!data) {
    return NULL;
  }

  data->current_token++;
  return get_current_token(data);
}

/**
 * @brief Push a state and node onto the stacks
 */
static bool push_stacks(LRParserData *data, int state, SyntaxTreeNode *node) {
  if (!data) {
    return false;
  }

  /* Check if stack needs to be resized */
  if (data->stack_top + 1 >= data->stack_capacity) {
    int new_capacity = data->stack_capacity * 2;

    int *new_state_stack =
        (int *)safe_realloc(data->state_stack, new_capacity * sizeof(int));
    if (!new_state_stack) {
      return false;
    }
    data->state_stack = new_state_stack;

    SyntaxTreeNode **new_node_stack = (SyntaxTreeNode **)safe_realloc(
        data->node_stack, new_capacity * sizeof(SyntaxTreeNode *));
    if (!new_node_stack) {
      return false;
    }
    data->node_stack = new_node_stack;

    data->stack_capacity = new_capacity;
    DEBUG_PRINT("Resized LR parser stacks to capacity %d", new_capacity);
  }

  /* Push onto stacks */
  data->stack_top++;
  data->state_stack[data->stack_top] = state;
  data->node_stack[data->stack_top] = node;

  DEBUG_PRINT("Pushed state %d onto stack at position %d", state,
              data->stack_top);
  return true;
}

/**
 * @brief Pop N items from the stacks
 */
static bool pop_stacks(LRParserData *data, int count) {
  if (!data || count < 0 || count > data->stack_top + 1) {
    return false;
  }

  data->stack_top -= count;
  DEBUG_PRINT("Popped %d items from stack, new top at position %d", count,
              data->stack_top);
  return true;
}

/**
 * @brief Parse input using LR parsing algorithm
 */
SyntaxTree *lr_parser_parse(Parser *parser, LRParserData *data, Lexer *lexer) {
  if (!parser || !data || !lexer) {
    return NULL;
  }

  /* Reset parser data */
  if (!lr_parser_data_reset(data, lexer)) {
    fprintf(stderr, "Failed to reset LR parser data\n");
    return NULL;
  }

  const Token *token = get_current_token(data);
  if (!token) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "No input tokens");
    return NULL;
  }

  /* Main parsing loop */
  bool accepted = false;

  while (!accepted && !data->has_error) {
    int current_state = data->state_stack[data->stack_top];

    /* Get action for current state and input symbol */
    int terminal_idx = -1;
    for (int i = 0; i < parser->grammar->terminals_count; i++) {
      if (parser->grammar->symbols[parser->grammar->terminal_indices[i]]
              .token == token->type) {
        terminal_idx = i;
        break;
      }
    }

    if (terminal_idx < 0) {
      data->has_error = true;
      snprintf(data->error_message, sizeof(data->error_message),
               "Unknown token type: %d", token->type);
      break;
    }

    Action action =
        action_table_get_action(data->table, current_state, terminal_idx);

    /* Handle action */
    switch (action.type) {
    case ACTION_SHIFT: {
      /* Create node for the token */
      const char *symbol_name =
          parser->grammar
              ->symbols[parser->grammar->terminal_indices[terminal_idx]]
              .name;
      SyntaxTreeNode *node = syntax_tree_create_terminal(*token, symbol_name);
      if (!node) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Failed to create syntax tree node for token");
        break;
      }

      /* Push new state and node onto stacks */
      if (!push_stacks(data, action.value, node)) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Failed to push onto parser stacks");
        break;
      }

      /* Move to next token */
      token = next_token(data);
      if (!token && !accepted) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Unexpected end of input");
        break;
      }

      DEBUG_PRINT("Shifted to state %d", action.value);
      break;
    }

    case ACTION_REDUCE: {
      /* Get the production to reduce by */
      int production_id = action.value;
      Production *prod = &parser->grammar->productions[production_id];

      /* Create node for the non-terminal */
      const char *nt_name =
          parser->grammar
              ->symbols[parser->grammar->nonterminal_indices[prod->lhs]]
              .name;
      SyntaxTreeNode *node =
          syntax_tree_create_nonterminal(prod->lhs, nt_name, production_id);
      if (!node) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Failed to create syntax tree node for non-terminal");
        break;
      }

      /* Add children in reverse order (to maintain correct order in tree) */
      int rhs_length =
          (prod->rhs[0].type == SYMBOL_EPSILON) ? 0 : prod->rhs_length;
      for (int i = rhs_length - 1; i >= 0; i--) {
        syntax_tree_add_child(
            node, data->node_stack[data->stack_top - rhs_length + 1 + i]);
      }

      /* Pop the RHS from the stack */
      if (!pop_stacks(data, rhs_length)) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Failed to pop from parser stacks");
        break;
      }

      /* Get new state from goto table */
      int new_state = action_table_get_goto(
          data->table, data->state_stack[data->stack_top], prod->lhs);

      if (new_state < 0) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Invalid goto state for non-terminal %d", prod->lhs);
        break;
      }

      /* Push new state and node onto stacks */
      if (!push_stacks(data, new_state, node)) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Failed to push onto parser stacks");
        break;
      }

      /* Record production in leftmost derivation */
      production_tracker_add(parser->production_tracker, production_id);

      DEBUG_PRINT("Reduced by production %d (%s) to state %d", production_id,
                  prod->display_str, new_state);
      break;
    }

    case ACTION_ACCEPT:
      /* Set the root of the syntax tree */
      if (data->stack_top == 1) {
        syntax_tree_set_root(data->syntax_tree, data->node_stack[1]);
        accepted = true;
        DEBUG_PRINT("Accepted input");
      } else {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Invalid stack state at accept action");
      }
      break;

    default:
      /* Error */
      data->has_error = true;

      /* Try to provide a helpful error message */
      char token_str[128];
      token_to_string(token, token_str, sizeof(token_str));

      snprintf(data->error_message, sizeof(data->error_message),
               "Syntax error at token '%s' (type %s) in state %d", token_str,
               token_type_to_string(token->type), current_state);
      break;
    }
  }

  /* Check if parsing was successful */
  if (accepted) {
    return data->syntax_tree;
  } else {
    if (data->has_error) {
      fprintf(stderr, "Parsing error: %s\n", data->error_message);
    } else {
      fprintf(stderr, "Failed to parse input\n");
    }

    if (data->syntax_tree) {
      syntax_tree_destroy(data->syntax_tree);
      data->syntax_tree = NULL;
    }

    return NULL;
  }
}
