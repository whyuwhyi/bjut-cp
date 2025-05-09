/**
 * @file lr_common.c
 * @brief Common LR parser utilities implementation
 */

#include "lr_common.h"
#include "lexer_analyzer/lexer.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Get the index of a terminal in the grammar
 */
int get_terminal_index(Grammar *grammar, TokenType token) {
  if (!grammar) {
    return -1;
  }

  for (int i = 0; i < grammar->terminals_count; i++) {
    if (grammar->symbols[grammar->terminal_indices[i]].token == token) {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Get the index of a non-terminal in the grammar
 */
int get_nonterminal_index(Grammar *grammar, int nonterminal_id) {
  if (!grammar) {
    return -1;
  }

  for (int i = 0; i < grammar->nonterminals_count; i++) {
    if (grammar->nonterminal_indices[i] == nonterminal_id) {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Get the symbol ID from a grammar index
 */
int get_symbol_id_from_index(Grammar *grammar, bool is_terminal, int index) {
  if (!grammar) {
    return -1;
  }

  if (is_terminal) {
    if (index >= 0 && index < grammar->terminals_count) {
      return grammar->terminal_indices[index];
    }
  } else {
    if (index >= 0 && index < grammar->nonterminals_count) {
      return grammar->nonterminal_indices[index];
    }
  }

  return -1;
}

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

  if (lexer->nr_token < CONFIG_MAX_TOKENS) {
    lexer->tokens[lexer->nr_token++].type = TK_EOF;
  }

  /* Reset parser data */
  if (!lr_parser_data_reset(data, lexer)) {
    fprintf(stderr, "Failed to reset LR parser data\n");
    return NULL;
  }

  lexer_print_tokens(lexer);

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
    int terminal_idx = get_terminal_index(parser->grammar, token->type);
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
      if (token->type == TK_EOF) {
        /* EOF通常不应该被shift，但这取决于你的文法 */
        DEBUG_PRINT("WARNING: Trying to shift on EOF in state %d",
                    current_state);

        /* 检查该状态的EOF动作 */
        Action eof_action =
            action_table_get_action(data->table, current_state, terminal_idx);
        if (eof_action.type != ACTION_SHIFT) {
          /* 如果这个状态对EOF的动作不是shift，说明有问题 */
          data->has_error = true;
          snprintf(data->error_message, sizeof(data->error_message),
                   "Unexpected shift on EOF. This should be an accept.");
          break;
        }

        /* 这种情况可能发生在增广文法中，我们需要shift EOF然后在下一个状态接受
         */
        DEBUG_PRINT("Shifting EOF token as part of augmented grammar");
      }

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
          (prod->rhs_length == 1 && prod->rhs[0].type == SYMBOL_EPSILON)
              ? 0
              : prod->rhs_length;
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

      /* Convert non-terminal ID to non-terminal index for GOTO table lookup */
      int nt_idx = -1;
      for (int i = 0; i < parser->grammar->nonterminals_count; i++) {
        if (parser->grammar->nonterminal_indices[i] == prod->lhs) {
          nt_idx = i;
          break;
        }
      }

      if (nt_idx < 0) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Unknown non-terminal ID: %d", prod->lhs);
        break;
      }

      /* Get new state from goto table */
      int current_state = data->state_stack[data->stack_top];
      int new_state = action_table_get_goto(data->table, current_state, nt_idx);

      if (new_state < 0) {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Invalid goto state for non-terminal %d (%s) from state %d",
                 prod->lhs, nt_name, current_state);

        /* Debug output for error diagnosis */
        DEBUG_PRINT("GOTO Error Details:");
        DEBUG_PRINT("  Non-terminal: %s (ID: %d, Index: %d)", nt_name,
                    prod->lhs, nt_idx);
        DEBUG_PRINT("  Current state: %d", current_state);
        DEBUG_PRINT("  Production: %s", prod->display_str);

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
      if (data->stack_top >= 1) {
        /* 检查栈顶元素是否为原始开始符号 */
        SyntaxTreeNode *root_node = data->node_stack[data->stack_top];
        if (root_node) {
          /* 设置语法树根节点 */
          syntax_tree_set_root(data->syntax_tree, root_node);
          accepted = true;
          DEBUG_PRINT("Accepted input with root node type: %d",
                      root_node->type);
        } else {
          data->has_error = true;
          snprintf(data->error_message, sizeof(data->error_message),
                   "Invalid stack state at accept action: NULL node at top");
        }
      } else {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Invalid stack state at accept action: stack too small");
      }
      break;

    case ACTION_ERROR:
    default:
      /* Error */
      data->has_error = true;
      /* Try to provide a helpful error message */
      char token_str[128];
      token_to_string(token, token_str, sizeof(token_str));

      /* 检查是否有可用的语法动作 */
      bool has_actions = false;
      for (int t = 0; t < parser->grammar->terminals_count; t++) {
        Action a = action_table_get_action(data->table, current_state, t);
        if (a.type != ACTION_ERROR) {
          has_actions = true;
          break;
        }
      }

      if (has_actions) {
        /* 有其他可用的动作，这是一个非期望的标记 */
        snprintf(data->error_message, sizeof(data->error_message),
                 "Syntax error at token '%s' (type %s) in state %d. Unexpected "
                 "token.",
                 token_str, token_type_to_string(token->type), current_state);
      } else {
        /* 没有可用的动作，这可能是一个无效的状态 */
        snprintf(data->error_message, sizeof(data->error_message),
                 "Syntax error at token '%s' (type %s) in state %d. Invalid "
                 "parser state.",
                 token_str, token_type_to_string(token->type), current_state);
      }
      break;
    }
  }

  /* Check if parsing was successful */
  if (accepted) {
    DEBUG_PRINT("Successfully parsed input");
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
