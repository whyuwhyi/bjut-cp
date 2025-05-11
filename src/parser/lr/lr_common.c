/**
 * @file lr_common.c
 * @brief Common LR parser utilities implementation
 */
#include "lr_common.h"
#include "error_handler.h"
#include "lexer/lexer.h"
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
 * @brief Parse input using LR parsing algorithm with enhanced error recovery
 *
 * @param parser Parser object
 * @param data LR parser data
 * @param lexer Lexer with tokenized input
 * @return SyntaxTree* Resulting syntax tree, or NULL on failure
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

  lexer_print_tokens(lexer);

  const Token *token = get_current_token(data);
  if (!token) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "No input tokens");
    return NULL;
  }

  /* Find EOF token index */
  int eof_idx = -1;
  for (int t = 0; t < parser->grammar->terminals_count; t++) {
    if (parser->grammar->symbols[parser->grammar->terminal_indices[t]].token ==
        TK_EOF) {
      eof_idx = t;
      break;
    }
  }
  if (eof_idx < 0) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "EOF token not found in grammar");
    return NULL;
  }

  /* Main parsing loop */
  bool accepted = false;
  SyntaxTreeNode *program_node =
      NULL; /* Store the program node for use as root */

  while (!accepted && !data->has_error) {
    int current_state = data->state_stack[data->stack_top];

    /* Get action for current state and input symbol */
    int terminal_idx = get_terminal_index(parser->grammar, token->type);
    if (terminal_idx < 0) {
      /* If current token is not in grammar, check if it's EOF */
      if (token->type == TK_EOF) {
        terminal_idx = eof_idx;
      } else {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Unknown token type: %d", token->type);
        report_syntax_error(parser, data, token, NULL);
        break;
      }
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

      /* Special handling for EOF token - don't create a node */
      if (token->type == TK_EOF) {
        /* Just push state without creating a node */
        if (!push_stacks(data, action.value, NULL)) {
          data->has_error = true;
          snprintf(data->error_message, sizeof(data->error_message),
                   "Failed to push onto parser stacks");
          break;
        }
      } else {
        /* Create syntax tree node for the terminal */
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
      }

      /* Move to next token */
      token = next_token(data);

      /* Handle case when no more tokens are available */
      if (!token) {
        int new_state = data->state_stack[data->stack_top];
        Action eof_action =
            action_table_get_action(data->table, new_state, eof_idx);

        /* Check if we can accept EOF at this point */
        if (eof_action.type == ACTION_ACCEPT) {
          /* Find appropriate root node for the syntax tree */
          if (data->stack_top >= 1) {
            /* First try to find a program node */
            for (int i = data->stack_top; i >= 0; i--) {
              if (data->node_stack[i] &&
                  data->node_stack[i]->type == NODE_NONTERMINAL &&
                  data->node_stack[i]->nonterminal_id == NT_P) {
                program_node = data->node_stack[i];
                break;
              }
            }

            /* Set appropriate root node */
            if (program_node) {
              syntax_tree_set_root(data->syntax_tree, program_node);
              accepted = true;
              DEBUG_PRINT("Accepted input at EOF with program node (type: %d)",
                          program_node->nonterminal_id);
              break;
            } else if (data->stack_top > 0 &&
                       data->node_stack[data->stack_top - 1]) {
              /* Use node below EOF as root if no program node found */
              syntax_tree_set_root(data->syntax_tree,
                                   data->node_stack[data->stack_top - 1]);
              accepted = true;
              DEBUG_PRINT("Accepted input at EOF with root node");
              break;
            }
          }
        } else if (eof_action.type == ACTION_REDUCE) {
          /* Create an artificial EOF token to continue parsing */
          Token eof_token;
          eof_token.type = TK_EOF;
          eof_token.line = token ? token->line : 0;
          eof_token.column = token ? token->column : 0;
          token = &eof_token;
          DEBUG_PRINT("Created artificial EOF token to continue parsing");
          continue;
        } else {
          /* Error - unexpected end of input */
          data->has_error = true;
          snprintf(data->error_message, sizeof(data->error_message),
                   "Unexpected end of input");

          /* Create temporary EOF token for error reporting */
          Token eof_token;
          eof_token.type = TK_EOF;
          eof_token.line =
              lexer_token_count(lexer) > 0
                  ? lexer_get_token(lexer, lexer_token_count(lexer) - 1)->line
                  : 1;
          eof_token.column = 1;

          report_syntax_error(parser, data, &eof_token, NULL);
          break;
        }
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

      /* Store program node if this is a program (P) node */
      if (prod->lhs == NT_P) {
        program_node = node;
      }

      /* Check if this is an epsilon production */
      bool is_epsilon_production =
          (prod->rhs_length == 1 && prod->rhs[0].type == SYMBOL_EPSILON);

      /* Handle epsilon productions */
      if (is_epsilon_production) {
        /* Create and add epsilon node as child */
        SyntaxTreeNode *epsilon_node = syntax_tree_create_epsilon();
        if (!epsilon_node) {
          data->has_error = true;
          snprintf(data->error_message, sizeof(data->error_message),
                   "Failed to create epsilon node");
          break;
        }

        /* Add epsilon node as child */
        syntax_tree_add_child(node, epsilon_node);

        /* For epsilon productions, we don't pop anything from stack */
        DEBUG_PRINT("Reduced by epsilon production %d (%s)", production_id,
                    prod->display_str);
      } else {
        /* Handle normal productions */
        int rhs_length = prod->rhs_length;

        /* Fix: Add children in correct order (not reversed) */
        /* Create a temporary array to hold children in correct order */
        SyntaxTreeNode **temp_children = NULL;
        if (rhs_length > 0) {
          temp_children =
              (SyntaxTreeNode **)malloc(rhs_length * sizeof(SyntaxTreeNode *));
          if (!temp_children) {
            data->has_error = true;
            snprintf(data->error_message, sizeof(data->error_message),
                     "Failed to allocate memory for syntax tree children");
            break;
          }

          /* Collect children in correct order */
          for (int i = 0; i < rhs_length; i++) {
            int stack_index = data->stack_top - rhs_length + 1 + i;
            if (stack_index < 0) {
              data->has_error = true;
              snprintf(data->error_message, sizeof(data->error_message),
                       "Stack underflow during reduction");
              free(temp_children);
              break;
            }
            temp_children[i] = data->node_stack[stack_index];
          }

          if (data->has_error) {
            break;
          }

          /* Add children in correct order */
          for (int i = 0; i < rhs_length; i++) {
            if (temp_children[i]) { /* Skip NULL nodes (EOF) */
              syntax_tree_add_child(node, temp_children[i]);
            }
          }

          free(temp_children);
        }

        /* Pop the RHS symbols from the stack */
        if (!pop_stacks(data, rhs_length)) {
          data->has_error = true;
          snprintf(data->error_message, sizeof(data->error_message),
                   "Failed to pop from parser stacks");
          break;
        }
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
      if (parser->production_tracker) {
        production_tracker_add(parser->production_tracker, production_id);
      }

      DEBUG_PRINT("Reduced by production %d (%s) to state %d", production_id,
                  prod->display_str, new_state);

      /* Check if we can accept after reducing to start symbol */
      if (prod->lhs == parser->grammar->start_symbol || prod->lhs == NT_P ||
          (nt_idx == parser->grammar->nonterminals_count - 1)) {

        Action eof_action =
            action_table_get_action(data->table, new_state, eof_idx);
        if (eof_action.type == ACTION_ACCEPT && token->type == TK_EOF) {
          /* Set syntax tree root to program node */
          syntax_tree_set_root(data->syntax_tree, node);
          accepted = true;
          DEBUG_PRINT("Accepted input after reducing to start symbol");
          break;
        }
      }
      break;
    }

    case ACTION_ACCEPT:
      /* Set the root of the syntax tree */
      if (data->stack_top >= 0) {
        /* Look for program node first */
        if (program_node) {
          syntax_tree_set_root(data->syntax_tree, program_node);
          accepted = true;
          DEBUG_PRINT("Accepted input with program node (type: %d)",
                      program_node->nonterminal_id);
        }
        /* Otherwise use the top node on the stack */
        else {
          SyntaxTreeNode *root_node = data->node_stack[data->stack_top];
          /* Skip NULL (EOF) nodes as root */
          if (root_node) {
            syntax_tree_set_root(data->syntax_tree, root_node);
            accepted = true;
            DEBUG_PRINT("Accepted input with root node");
          } else if (data->stack_top > 0) {
            /* Try using the node below EOF */
            root_node = data->node_stack[data->stack_top - 1];
            if (root_node) {
              syntax_tree_set_root(data->syntax_tree, root_node);
              accepted = true;
              DEBUG_PRINT("Accepted input with root node below EOF");
            } else {
              data->has_error = true;
              snprintf(
                  data->error_message, sizeof(data->error_message),
                  "Invalid stack state at accept action: NULL nodes at top");
            }
          } else {
            data->has_error = true;
            snprintf(data->error_message, sizeof(data->error_message),
                     "Invalid stack state at accept action: NULL node at top");
          }
        }
      } else {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Invalid stack state at accept action: stack too small");
      }
      break;

    case ACTION_ERROR:
    default:
      /* Use enhanced error recovery mechanism */
      if (enhanced_error_recovery(parser, data, token)) {
        /* Get the updated current token and continue parsing */
        token = get_current_token(data);
        if (!token) {
          DEBUG_PRINT("No more tokens after error recovery");
          break;
        }

        /* Continue parsing with new token */
        continue;
      } else {
        /* Error recovery failed */
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Syntax error recovery failed, cannot continue parsing");
        break;
      }
    }
  }

  /* Special handling for EOF without explicit acceptance */
  if (!accepted && !data->has_error && (!token || token->type == TK_EOF)) {
    int current_state = data->state_stack[data->stack_top];

    /* Check if current state can accept EOF */
    Action eof_action =
        action_table_get_action(data->table, current_state, eof_idx);

    if (eof_action.type == ACTION_ACCEPT) {
      /* Try to set program node as root if available */
      if (program_node) {
        syntax_tree_set_root(data->syntax_tree, program_node);
        accepted = true;
        DEBUG_PRINT(
            "Accepted input at EOF with program node (special handling)");
      }
      /* Otherwise, set stack top node as root if not NULL */
      else if (data->stack_top >= 0) {
        SyntaxTreeNode *root_node = data->node_stack[data->stack_top];
        if (root_node) {
          syntax_tree_set_root(data->syntax_tree, root_node);
          accepted = true;
          DEBUG_PRINT("Accepted input at EOF with special handling");
        } else if (data->stack_top > 0) {
          /* Try node below EOF */
          root_node = data->node_stack[data->stack_top - 1];
          if (root_node) {
            syntax_tree_set_root(data->syntax_tree, root_node);
            accepted = true;
            DEBUG_PRINT("Accepted input at EOF with node below EOF");
          }
        }
      }
    } else if (eof_action.type == ACTION_REDUCE) {
      /* Avoid infinite loop for reduce actions at EOF */
      DEBUG_PRINT("Found REDUCE action for EOF in state %d, production %d",
                  current_state, eof_action.value);

      data->has_error = true;
      snprintf(data->error_message, sizeof(data->error_message),
               "Parsing incomplete: expected more input after the last token");
    } else {
      /* Special handling for EOF without explicit accept action */
      if (program_node) {
        syntax_tree_set_root(data->syntax_tree, program_node);
        accepted = true;
        DEBUG_PRINT("Forced accept at EOF with program node");
      } else if (data->stack_top >= 0) {
        SyntaxTreeNode *top_node = data->node_stack[data->stack_top];

        if (top_node && top_node->type == NODE_NONTERMINAL &&
            (top_node->nonterminal_id == parser->grammar->start_symbol ||
             top_node->nonterminal_id == NT_P)) {
          /* Stack top is start symbol or program symbol, force accept */
          syntax_tree_set_root(data->syntax_tree, top_node);
          accepted = true;
          DEBUG_PRINT("Forced accept at EOF with start symbol on stack");
        } else {
          data->has_error = true;
          snprintf(data->error_message, sizeof(data->error_message),
                   "Unexpected end of input, incomplete parse");

          /* Create temporary EOF token for error reporting */
          Token eof_token;
          eof_token.type = TK_EOF;
          eof_token.line =
              lexer_token_count(lexer) > 0
                  ? lexer_get_token(lexer, lexer_token_count(lexer) - 1)->line
                  : 1;
          eof_token.column = 1;

          report_syntax_error(parser, data, &eof_token, NULL);
        }
      } else {
        data->has_error = true;
        snprintf(data->error_message, sizeof(data->error_message),
                 "Unexpected end of input, parser stack empty");

        /* Create temporary EOF token for error reporting */
        Token eof_token;
        eof_token.type = TK_EOF;
        eof_token.line =
            lexer_token_count(lexer) > 0
                ? lexer_get_token(lexer, lexer_token_count(lexer) - 1)->line
                : 1;
        eof_token.column = 1;

        report_syntax_error(parser, data, &eof_token, NULL);
      }
    }
  }

  /* Return the syntax tree if parsing was successful */
  if (accepted) {
    DEBUG_PRINT("Successfully parsed input");
    return data->syntax_tree;
  } else {
    /* Handle parse failure */
    if (data->has_error) {
      fprintf(stderr, "Parsing failed: %s\n", data->error_message);
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
