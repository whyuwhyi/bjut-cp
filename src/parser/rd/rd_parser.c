/**
 * @file rd_parser.c
 * @brief Recursive descent parser implementation
 */
#include "rd_parser.h"
#include "../production_tracker.h"
#include "parser/grammar.h"
#include "parser/syntax_tree.h"
#include "utils.h"
#include <fenv.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Get the current token from the lexer
 *
 * @param data Parser data
 * @return const Token* Current token, or NULL if out of tokens
 */
static const Token *get_current_token(RDParserData *data) {
  if (!data || !data->lexer) {
    return NULL;
  }
  return lexer_get_token(data->lexer, data->current_token_index);
}

/**
 * @brief Move to the next token
 *
 * @param data Parser data
 * @return const Token* Next token, or NULL if out of tokens
 */
static const Token *next_token(RDParserData *data) {
  if (!data) {
    return NULL;
  }
  data->current_token_index++;
  return get_current_token(data);
}

/**
 * @brief Set error message and flag in parser data
 *
 * @param data Parser data
 * @param format Error message format string
 * @param ... Additional arguments for format string
 */
static void set_error(RDParserData *data, const char *format, ...) {
  if (!data) {
    return;
  }
  va_list args;
  va_start(args, format);
  vsnprintf(data->error_message, sizeof(data->error_message), format, args);
  va_end(args);
  data->has_error = true;
}

/**
 * @brief Set production ID and add to tracker, with backtracking support
 *
 * @param node Node to set production ID for
 * @param prod_id Production ID
 * @param tracker Production tracker
 * @return bool Success status of adding to tracker
 */
static bool set_production(SyntaxTreeNode *node, ProductionID prod_id,
                           ProductionTracker *tracker) {
  if (node) {
    node->production_id = prod_id;
  }
  if (tracker) {
    return production_tracker_add(tracker, prod_id);
  }
  return false;
}

/**
 * @brief Get the current size of production tracker
 *
 * @param tracker Production tracker
 * @return int Current size of the tracker
 */
static int production_tracker_get_size(ProductionTracker *tracker) {
  if (!tracker)
    return 0;
  return tracker->length;
}

/**
 * @brief Roll back the production tracker to a specific size
 *
 * @param tracker Production tracker
 * @param size Size to roll back to
 */
static void production_tracker_rollback_to(ProductionTracker *tracker,
                                           int size) {
  if (!tracker)
    return;
  tracker->length = size;
}

/**
 * @brief Create syntax tree node for non-terminal
 *
 * @param nt Non-terminal type
 * @param name Non-terminal name
 * @param data Parser data to set error if creation fails
 * @return SyntaxTreeNode* Created node or NULL on failure
 */
static SyntaxTreeNode *create_nt_node(Nonterminal nt, const char *name,
                                      RDParserData *data) {
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(nt, name, -1);
  if (!node && data) {
    set_error(data, "Failed to create syntax tree node for %s", name);
  }
  return node;
}

/**
 * @brief Create and add epsilon node to parent
 *
 * @param parent Parent node
 * @return bool True if successful, false otherwise
 */
static bool add_epsilon(SyntaxTreeNode *parent) {
  if (!parent) {
    return false;
  }
  SyntaxTreeNode *epsilon = syntax_tree_create_epsilon();
  if (epsilon) {
    syntax_tree_add_child(parent, epsilon);
    return true;
  }
  return false;
}

/**
 * @brief Match current token with expected token type
 *
 * @param data Parser data
 * @param token_type Expected token type
 * @param parent_node Parent node to add matched token to
 * @param symbol_name Symbol name for the token
 * @return bool True if token matched, false otherwise
 */
static bool match_token(RDParserData *data, TokenType token_type,
                        SyntaxTreeNode *parent_node, const char *symbol_name) {
  const Token *token = get_current_token(data);
  if (!token) {
    set_error(data, "Unexpected end of input, expected token type: %s",
              token_type_to_string(token_type));
    return false;
  }
  if (token->type != token_type) {
    char token_str[128];
    token_to_string(token, token_str, sizeof(token_str));
    set_error(data, "Unexpected token: %s, expected token type: %s", token_str,
              token_type_to_string(token_type));
    return false;
  }
  /* Add token to syntax tree */
  if (parent_node) {
    SyntaxTreeNode *token_node =
        syntax_tree_create_terminal(*token, symbol_name);
    if (token_node) {
      syntax_tree_add_child(parent_node, token_node);
    }
  }
  /* Advance to next token */
  next_token(data);
  return true;
}

/**
 * @brief Forward declarations for recursive descent functions
 */
static SyntaxTreeNode *parse_P(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_L(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_S(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_N(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_C(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_O(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_E(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_X(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_R(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_Y(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_F(Parser *parser, RDParserData *data);
static SyntaxTreeNode *parse_T(Parser *parser, RDParserData *data);

/**
 * @brief Parse non-terminal P (Program)
 */
static SyntaxTreeNode *parse_P(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal P */
  SyntaxTreeNode *node = create_nt_node(NT_P, "P", data);
  if (!node) {
    return NULL;
  }

  /* Save tracker state for potential backtracking */
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  /* Try production P → L T */
  set_production(node, PROD_P_LT, parser->production_tracker);

  SyntaxTreeNode *l_node = parse_L(parser, data);
  if (!l_node) {
    set_error(data, "Failed to parse statement list (non-terminal L)");
    destroy_syntax_tree_node(node);
    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    return NULL;
  }
  syntax_tree_add_child(node, l_node);

  SyntaxTreeNode *t_node = parse_T(parser, data);
  if (!t_node) {
    set_error(data, "Failed to parse program tail (non-terminal T)");
    destroy_syntax_tree_node(node);
    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    return NULL;
  }
  syntax_tree_add_child(node, t_node);

  return node;
}

/**
 * @brief Parse non-terminal T (Program Tail)
 */
static SyntaxTreeNode *parse_T(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal T */
  SyntaxTreeNode *node = create_nt_node(NT_T, "T", data);
  if (!node) {
    return NULL;
  }

  int save_token_index = data->current_token_index;
  data->has_error = false;

  /* Save tracker state for potential backtracking */
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  /* Try production T → P T */
  set_production(node, PROD_T_PT, parser->production_tracker);

  SyntaxTreeNode *p_node = parse_P(parser, data);
  if (p_node) {
    syntax_tree_add_child(node, p_node);
    SyntaxTreeNode *t_node = parse_T(parser, data);
    if (t_node) {
      syntax_tree_add_child(node, t_node);
      return node;
    }
  }

  /* Backtrack if first production failed */
  data->current_token_index = save_token_index;
  data->has_error = false; /* Reset error flag */

  /* Rollback production tracker */
  if (tracker_save_idx >= 0 && parser->production_tracker) {
    production_tracker_rollback_to(parser->production_tracker,
                                   tracker_save_idx);
  }

  /* Try production T → ε */
  set_production(node, PROD_T_EPSILON, parser->production_tracker);
  add_epsilon(node);
  return node;
}

/**
 * @brief Parse non-terminal L (Statement list)
 */
static SyntaxTreeNode *parse_L(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal L */
  SyntaxTreeNode *node = create_nt_node(NT_L, "L", data);
  if (!node) {
    return NULL;
  }

  /* Save tracker state for potential backtracking */
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  /* Try production L → S ; */
  set_production(node, PROD_L_S_SEMI, parser->production_tracker);

  SyntaxTreeNode *s_node = parse_S(parser, data);
  if (!s_node) {
    set_error(data, "Failed to parse statement (non-terminal S)");
    destroy_syntax_tree_node(node);
    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    return NULL;
  }
  syntax_tree_add_child(node, s_node);

  if (!match_token(data, TK_SEMI, node, ";")) {
    destroy_syntax_tree_node(node);
    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    return NULL;
  }

  return node;
}

/**
 * @brief Parse non-terminal S (Statement)
 */
static SyntaxTreeNode *parse_S(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal S */
  SyntaxTreeNode *node = create_nt_node(NT_S, "S", data);
  if (!node) {
    return NULL;
  }

  const Token *token = get_current_token(data);
  if (!token) {
    set_error(data, "Unexpected end of input");
    destroy_syntax_tree_node(node);
    return NULL;
  }

  /* Save current state for potential backtracking */
  int save_token_index = data->current_token_index;
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  /* Try production S → id = E */
  if (token->type == TK_IDN) {
    set_production(node, PROD_S_ASSIGN, parser->production_tracker);

    if (match_token(data, TK_IDN, node, "id") &&
        match_token(data, TK_EQ, node, "=")) {
      SyntaxTreeNode *e_node = parse_E(parser, data);
      if (e_node) {
        syntax_tree_add_child(node, e_node);
        return node;
      }
    }

    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
  }

  /* Try production S → if C then S N */
  if (token->type == TK_IF) {
    set_production(node, PROD_S_IF_C_THEN_S_N, parser->production_tracker);

    if (match_token(data, TK_IF, node, "if")) {
      SyntaxTreeNode *c_node = parse_C(parser, data);
      if (c_node) {
        syntax_tree_add_child(node, c_node);
        if (match_token(data, TK_THEN, node, "then")) {
          SyntaxTreeNode *s_node = parse_S(parser, data);
          if (s_node) {
            syntax_tree_add_child(node, s_node);
            SyntaxTreeNode *n_node = parse_N(parser, data);
            if (n_node) {
              syntax_tree_add_child(node, n_node);
              return node;
            }
          }
        }
      }
    }

    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
  }

  /* Try production S → while C do S */
  if (token->type == TK_WHILE) {
    set_production(node, PROD_S_WHILE_C_DO_S, parser->production_tracker);

    if (match_token(data, TK_WHILE, node, "while")) {
      SyntaxTreeNode *c_node = parse_C(parser, data);
      if (c_node) {
        syntax_tree_add_child(node, c_node);
        if (match_token(data, TK_DO, node, "do")) {
          SyntaxTreeNode *s_node = parse_S(parser, data);
          if (s_node) {
            syntax_tree_add_child(node, s_node);
            return node;
          }
        }
      }
    }

    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
  }

  /* Try production S → begin L end */
  if (token->type == TK_BEGIN) {
    set_production(node, PROD_S_BEGIN_L_END, parser->production_tracker);

    if (match_token(data, TK_BEGIN, node, "begin")) {
      SyntaxTreeNode *l_node = parse_L(parser, data);
      if (l_node) {
        syntax_tree_add_child(node, l_node);
        if (match_token(data, TK_END, node, "end")) {
          return node;
        }
      }
    }

    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
  }

  /* No valid production found */
  char token_str[128];
  token_to_string(token, token_str, sizeof(token_str));
  set_error(data,
            "Failed to parse statement (non-terminal S), unexpected token: %s",
            token_str);
  destroy_syntax_tree_node(node);
  return NULL;
}

/**
 * @brief Parse non-terminal N (Statement Tail)
 */
static SyntaxTreeNode *parse_N(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal N */
  SyntaxTreeNode *node = create_nt_node(NT_N, "N", data);
  if (!node) {
    return NULL;
  }

  /* Save current state for potential backtracking */
  int save_token_index = data->current_token_index;
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  const Token *token = get_current_token(data);

  /* Try production N → else S */
  if (token && token->type == TK_ELSE) {
    set_production(node, PROD_N_ELSE_S, parser->production_tracker);

    if (match_token(data, TK_ELSE, node, "else")) {
      SyntaxTreeNode *s_node = parse_S(parser, data);
      if (s_node) {
        syntax_tree_add_child(node, s_node);
        return node;
      }

      /* Rollback production tracker if matching failed after 'else' */
      if (tracker_save_idx >= 0 && parser->production_tracker) {
        production_tracker_rollback_to(parser->production_tracker,
                                       tracker_save_idx);
      }
      destroy_syntax_tree_node(node);
      return NULL;
    }

    /* Backtrack if 'else' token doesn't match */
    data->current_token_index = save_token_index;

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
  }

  /* Handle epsilon production N → ε */
  set_production(node, PROD_N_EPSILON, parser->production_tracker);
  add_epsilon(node);
  data->has_error = false;
  return node;
}

/**
 * @brief Parse non-terminal C (Condition)
 *
 * First scans for top-level relational operators, prioritizing C → E relop E
 * If not found, then tries to parse a nested condition with C → '(' C ')'
 */
/**
 * @brief Parse non-terminal C (Condition)
 */
static SyntaxTreeNode *parse_C(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal C */
  SyntaxTreeNode *node = create_nt_node(NT_C, "C", data);
  if (!node) {
    return NULL;
  }

  /* Save current state for potential backtracking */
  int save_token_index = data->current_token_index;
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  const Token *token = get_current_token(data);
  if (!token) {
    set_error(data, "Unexpected end of input");
    destroy_syntax_tree_node(node);
    return NULL;
  }

  /* Try production C → E O */
  if (token->type == TK_IDN || token->type == TK_SLP || token->type == TK_OCT ||
      token->type == TK_DEC || token->type == TK_HEX) {
    set_production(node, PROD_C_E_O, parser->production_tracker);

    SyntaxTreeNode *e_node = parse_E(parser, data);
    if (e_node) {
      syntax_tree_add_child(node, e_node);
      SyntaxTreeNode *o_node = parse_O(parser, data);
      if (o_node) {
        syntax_tree_add_child(node, o_node);
        return node;
      }
    }

    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
  }

  /* Try production C → ( C ) */
  if (token->type == TK_SLP) {
    set_production(node, PROD_C_PAREN, parser->production_tracker);

    if (match_token(data, TK_SLP, node, "(")) {
      SyntaxTreeNode *c_node = parse_C(parser, data);
      if (c_node) {
        syntax_tree_add_child(node, c_node);
        if (match_token(data, TK_SRP, node, ")")) {
          return node;
        }
      }
    }

    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
  }

  /* No valid production found */
  char token_str[128];
  token_to_string(token, token_str, sizeof(token_str));
  set_error(data,
            "Failed to parse condition (non-terminal C), unexpected token: %s",
            token_str);
  destroy_syntax_tree_node(node);
  return NULL;
}

/**
 * @brief Parse non-terminal O (Operator followed by expression)
 */
static SyntaxTreeNode *parse_O(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal O */
  SyntaxTreeNode *node = create_nt_node(NT_O, "O", data);
  if (!node) {
    return NULL;
  }

  /* Save current state for potential backtracking */
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  const Token *token = get_current_token(data);
  if (!token) {
    set_error(data, "Unexpected end of input");
    destroy_syntax_tree_node(node);
    return NULL;
  }

  /* Check token type and determine production */
  ProductionID prod_id = PROD_UNVALID;
  const char *op_str = NULL;

  switch (token->type) {
  case TK_GT:
    prod_id = PROD_O_GT;
    op_str = ">";
    break;
  case TK_LT:
    prod_id = PROD_O_LT;
    op_str = "<";
    break;
  case TK_EQ:
    prod_id = PROD_O_EQ;
    op_str = "=";
    break;
  case TK_GE:
    prod_id = PROD_O_GE;
    op_str = ">=";
    break;
  case TK_LE:
    prod_id = PROD_O_LE;
    op_str = "<=";
    break;
  case TK_NEQ:
    prod_id = PROD_O_NE;
    op_str = "<>";
    break;
  default:
    break;
  }

  if (prod_id != PROD_UNVALID && op_str != NULL) {
    set_production(node, prod_id, parser->production_tracker);

    if (match_token(data, token->type, node, op_str)) {
      SyntaxTreeNode *e_node = parse_E(parser, data);
      if (e_node) {
        syntax_tree_add_child(node, e_node);
        return node;
      }
    }

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
  }

  /* No valid production found */
  char token_str[128];
  token_to_string(token, token_str, sizeof(token_str));
  set_error(data,
            "Failed to parse operator (non-terminal O), unexpected token: %s",
            token_str);
  destroy_syntax_tree_node(node);

  /* Rollback production tracker */
  if (tracker_save_idx >= 0 && parser->production_tracker) {
    production_tracker_rollback_to(parser->production_tracker,
                                   tracker_save_idx);
  }
  return NULL;
}

/**
 * @brief Parse non-terminal E (Expression)
 */
static SyntaxTreeNode *parse_E(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal E */
  SyntaxTreeNode *node = create_nt_node(NT_E, "E", data);
  if (!node) {
    return NULL;
  }

  /* Save tracker state for potential backtracking */
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  /* Try production E → R X */
  set_production(node, PROD_E_R_X, parser->production_tracker);

  SyntaxTreeNode *r_node = parse_R(parser, data);
  if (!r_node) {
    set_error(data, "Failed to parse term (non-terminal R)");
    destroy_syntax_tree_node(node);

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    return NULL;
  }
  syntax_tree_add_child(node, r_node);

  SyntaxTreeNode *x_node = parse_X(parser, data);
  if (!x_node) {
    set_error(data, "Failed to parse expression tail (non-terminal X)");
    destroy_syntax_tree_node(node);

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    return NULL;
  }
  syntax_tree_add_child(node, x_node);

  return node;
}

/**
 * @brief Parse non-terminal X (Expression Tail)
 */
static SyntaxTreeNode *parse_X(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal X */
  SyntaxTreeNode *node = create_nt_node(NT_X, "X", data);
  if (!node) {
    return NULL;
  }

  /* Save current state for potential backtracking */
  int save_token_index = data->current_token_index;
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  const Token *token = get_current_token(data);

  // Handle X → + R X or X → - R X
  if (token) {
    ProductionID prod_id = PROD_UNVALID;
    if (token->type == TK_ADD) {
      prod_id = PROD_X_PLUS_R_X;
    } else if (token->type == TK_SUB) {
      prod_id = PROD_X_MINUS_R_X;
    }

    if (prod_id != PROD_UNVALID) {
      set_production(node, prod_id, parser->production_tracker);

      if (match_token(data, token->type, node,
                      token->type == TK_ADD ? "+" : "-")) {
        SyntaxTreeNode *r_node = parse_R(parser, data);
        if (r_node) {
          syntax_tree_add_child(node, r_node);
          SyntaxTreeNode *x_node = parse_X(parser, data);
          if (x_node) {
            syntax_tree_add_child(node, x_node);
            return node;
          }
        }

        /* Rollback production tracker */
        if (tracker_save_idx >= 0 && parser->production_tracker) {
          production_tracker_rollback_to(parser->production_tracker,
                                         tracker_save_idx);
        }
        destroy_syntax_tree_node(node);
        return NULL;
      }

      /* Backtrack if operator doesn't match */
      data->current_token_index = save_token_index;

      /* Rollback production tracker */
      if (tracker_save_idx >= 0 && parser->production_tracker) {
        production_tracker_rollback_to(parser->production_tracker,
                                       tracker_save_idx);
      }
    }
  }

  /* Handle epsilon production X → ε */
  set_production(node, PROD_X_EPSILON, parser->production_tracker);
  add_epsilon(node);
  data->has_error = false;
  return node;
}

/**
 * @brief Parse non-terminal R (Term)
 */
static SyntaxTreeNode *parse_R(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal R */
  SyntaxTreeNode *node = create_nt_node(NT_R, "R", data);
  if (!node) {
    return NULL;
  }

  /* Save tracker state for potential backtracking */
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  /* Try production R → F Y */
  set_production(node, PROD_R_F_Y, parser->production_tracker);

  SyntaxTreeNode *f_node = parse_F(parser, data);
  if (!f_node) {
    set_error(data, "Failed to parse factor (non-terminal F)");
    destroy_syntax_tree_node(node);

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    return NULL;
  }
  syntax_tree_add_child(node, f_node);

  SyntaxTreeNode *y_node = parse_Y(parser, data);
  if (!y_node) {
    set_error(data, "Failed to parse term tail (non-terminal Y)");
    destroy_syntax_tree_node(node);

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    return NULL;
  }
  syntax_tree_add_child(node, y_node);

  return node;
}

/**
 * @brief Parse non-terminal Y (Term Tail)
 */
static SyntaxTreeNode *parse_Y(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal Y */
  SyntaxTreeNode *node = create_nt_node(NT_Y, "Y", data);
  if (!node) {
    return NULL;
  }

  /* Save current state for potential backtracking */
  int save_token_index = data->current_token_index;
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  const Token *token = get_current_token(data);

  // Handle Y → * F Y or Y → / F Y
  if (token) {
    ProductionID prod_id = PROD_UNVALID;
    if (token->type == TK_MUL) {
      prod_id = PROD_Y_MUL_F_Y;
    } else if (token->type == TK_DIV) {
      prod_id = PROD_Y_DIV_F_Y;
    }

    if (prod_id != PROD_UNVALID) {
      set_production(node, prod_id, parser->production_tracker);

      if (match_token(data, token->type, node,
                      token->type == TK_MUL ? "*" : "/")) {
        SyntaxTreeNode *f_node = parse_F(parser, data);
        if (f_node) {
          syntax_tree_add_child(node, f_node);
          SyntaxTreeNode *y_node = parse_Y(parser, data);
          if (y_node) {
            syntax_tree_add_child(node, y_node);
            return node;
          }
        }

        /* Rollback production tracker */
        if (tracker_save_idx >= 0 && parser->production_tracker) {
          production_tracker_rollback_to(parser->production_tracker,
                                         tracker_save_idx);
        }
        destroy_syntax_tree_node(node);
        return NULL;
      }

      /* Backtrack if operator doesn't match */
      data->current_token_index = save_token_index;

      /* Rollback production tracker */
      if (tracker_save_idx >= 0 && parser->production_tracker) {
        production_tracker_rollback_to(parser->production_tracker,
                                       tracker_save_idx);
      }
    }
  }

  /* Handle epsilon production Y → ε */
  set_production(node, PROD_Y_EPSILON, parser->production_tracker);
  add_epsilon(node);
  data->has_error = false;
  return node;
}

/**
 * @brief Parse non-terminal F (Factor)
 */
static SyntaxTreeNode *parse_F(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }
  /* Create node for non-terminal F */
  SyntaxTreeNode *node = create_nt_node(NT_F, "F", data);
  if (!node) {
    return NULL;
  }

  /* Save tracker state for potential backtracking */
  int tracker_save_idx = -1;
  if (parser->production_tracker) {
    tracker_save_idx = production_tracker_get_size(parser->production_tracker);
  }

  const Token *token = get_current_token(data);
  if (!token) {
    set_error(data, "Unexpected end of input");
    destroy_syntax_tree_node(node);
    return NULL;
  }

  /* Check token type and determine production */
  switch (token->type) {
  case TK_SLP: {
    /* F → ( E ) */
    set_production(node, PROD_F_PAREN, parser->production_tracker);

    if (match_token(data, TK_SLP, node, "(")) {
      SyntaxTreeNode *e_node = parse_E(parser, data);
      if (e_node) {
        syntax_tree_add_child(node, e_node);
        if (match_token(data, TK_SRP, node, ")")) {
          return node;
        }
      }
    }

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    break;
  }
  case TK_IDN: {
    /* F → id */
    set_production(node, PROD_F_ID, parser->production_tracker);

    if (match_token(data, TK_IDN, node, "id")) {
      return node;
    }

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    break;
  }
  case TK_OCT: {
    /* F → int8 */
    set_production(node, PROD_F_INT8, parser->production_tracker);

    if (match_token(data, TK_OCT, node, "int8")) {
      return node;
    }

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    break;
  }
  case TK_DEC: {
    /* F → int10 */
    set_production(node, PROD_F_INT10, parser->production_tracker);

    if (match_token(data, TK_DEC, node, "int10")) {
      return node;
    }
    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    break;
  }
  case TK_HEX: {
    /* F → int16 */
    set_production(node, PROD_F_INT16, parser->production_tracker);

    if (match_token(data, TK_HEX, node, "int16")) {
      return node;
    }

    /* Rollback production tracker */
    if (tracker_save_idx >= 0 && parser->production_tracker) {
      production_tracker_rollback_to(parser->production_tracker,
                                     tracker_save_idx);
    }
    break;
  }
  default:
    break;
  }

  /* No valid production found */
  char token_str[128];
  token_to_string(token, token_str, sizeof(token_str));
  set_error(data,
            "Failed to parse factor (non-terminal F), unexpected token: %s",
            token_str);
  destroy_syntax_tree_node(node);

  /* Rollback production tracker */
  if (tracker_save_idx >= 0 && parser->production_tracker) {
    production_tracker_rollback_to(parser->production_tracker,
                                   tracker_save_idx);
  }
  return NULL;
}

/**
 * @brief Create a recursive descent parser
 */
Parser *rd_parser_create(void) {
  Parser *parser = (Parser *)safe_malloc(sizeof(Parser));
  if (!parser) {
    return NULL;
  }

  /* Initialize parser */
  parser->type = PARSER_TYPE_RECURSIVE_DESCENT;
  parser->grammar = NULL;
  parser->production_tracker = NULL;
  parser->sdt_gen = NULL; /* Initialize SDT code generator to NULL */
  parser->init = rd_parser_init;
  parser->parse = rd_parser_parse;
  parser->print_leftmost_derivation = rd_parser_print_leftmost_derivation;
  parser->destroy = rd_parser_destroy;

  /* Create parser-specific data */
  parser->data = safe_malloc(sizeof(RDParserData));
  if (!parser->data) {
    free(parser);
    return NULL;
  }
  memset(parser->data, 0, sizeof(RDParserData));
  DEBUG_PRINT("Created recursive descent parser");
  return parser;
}

/**
 * @brief Initialize the recursive descent parser
 */
bool rd_parser_init(Parser *parser) {
  if (!parser || !parser->data) {
    return false;
  }
  /* Reset parser data */
  RDParserData *data = (RDParserData *)parser->data;
  memset(data, 0, sizeof(RDParserData));
  DEBUG_PRINT("Initialized recursive descent parser");
  return true;
}

/**
 * @brief Parse input using recursive descent
 */
SyntaxTree *rd_parser_parse(Parser *parser, Lexer *lexer) {
  if (!parser || !parser->data || !lexer) {
    return NULL;
  }

  /* Save sdt_gen and set to NULL to prevent semantic actions during
   * parsing */
  void *saved_sdt_gen = parser->sdt_gen;
  parser->sdt_gen = NULL;

  /* Set up parser data */
  RDParserData *data = (RDParserData *)parser->data;
  data->lexer = lexer;
  data->current_token_index = 0;
  data->has_error = false;
  memset(data->error_message, 0, sizeof(data->error_message));

  /* Create syntax tree */
  data->syntax_tree = syntax_tree_create();
  if (!data->syntax_tree) {
    fprintf(stderr, "Failed to create syntax tree\n");
    parser->sdt_gen = saved_sdt_gen; // Restore sdt_gen
    return NULL;
  }

  /* Start parsing from the start symbol */
  SyntaxTreeNode *root = parse_P(parser, data);
  if (!root) {
    if (data->has_error) {
      fprintf(stderr, "Parsing error: %s\n", data->error_message);
    } else {
      fprintf(stderr, "Failed to parse program\n");
    }
    syntax_tree_destroy(data->syntax_tree);
    data->syntax_tree = NULL;
    parser->sdt_gen = saved_sdt_gen; // Restore sdt_gen
    return NULL;
  }
  syntax_tree_set_root(data->syntax_tree, root);

  /* Check for trailing tokens */
  const Token *token = get_current_token(data);
  if (token && token->type != TK_EOF) {
    char token_str[128];
    token_to_string(token, token_str, sizeof(token_str));
    fprintf(stderr, "Warning: Trailing tokens in input starting with: %s\n",
            token_str);
  }

  DEBUG_PRINT("Parsing completed successfully");

  /* Restore sdt_gen */
  parser->sdt_gen = saved_sdt_gen;
  return data->syntax_tree;
}

/**
 * @brief Print the leftmost derivation from the recursive descent parser
 */
void rd_parser_print_leftmost_derivation(Parser *parser) {
  if (!parser) {
    return;
  }
  /* Use the common production tracker to print the derivation */
  production_tracker_print(parser->production_tracker, parser->grammar);
}

/**
 * @brief Destroy recursive descent parser and free resources
 */
void rd_parser_destroy(Parser *parser) {
  if (!parser) {
    return;
  }

  /* Free parser-specific data */
  if (parser->data) {
    RDParserData *data = (RDParserData *)parser->data;
    /* Free syntax tree if it exists and hasn't been returned */
    if (data->syntax_tree) {
      syntax_tree_destroy(data->syntax_tree);
    }
    free(parser->data);
  }

  /* Free the parser itself */
  free(parser);
  DEBUG_PRINT("Destroyed recursive descent parser");
}
