/**
 * @file rd_parser.c
 * @brief Recursive descent parser implementation
 */

#include "rd_parser.h"
#include "../parser_common.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef CONFIG_TAC
#include "codegen/sdt_codegen.h"
#endif

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
    snprintf(data->error_message, sizeof(data->error_message),
             "Unexpected end of input, expected token type: %s",
             token_type_to_string(token_type));
    data->has_error = true;
    return false;
  }

  if (token->type != token_type) {
    char token_str[128];
    token_to_string(token, token_str, sizeof(token_str));
    snprintf(data->error_message, sizeof(data->error_message),
             "Unexpected token: %s, expected token type: %s", token_str,
             token_type_to_string(token_type));
    data->has_error = true;
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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_P, "P", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  /* Try production P → L T */
  SyntaxTreeNode *l_node = parse_L(parser, data);
  if (!l_node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to parse statement list (non-terminal L)");
    return NULL;
  }

  syntax_tree_add_child(node, l_node);

  SyntaxTreeNode *t_node = parse_T(parser, data);
  if (!t_node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to parse program tail (non-terminal T)");
    return NULL;
  }

  syntax_tree_add_child(node, t_node);
  node->production_id = 0; /* P → L T */
  production_tracker_add(parser->production_tracker, 0);

#ifdef CONFIG_TAC
  /* Execute semantic action for this production */
  if (parser->sdt_gen) {
    if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
      data->has_error = true;
      const char *error = sdt_codegen_get_error(parser->sdt_gen);
      if (error) {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed: %s", error);
      } else {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed");
      }
      // return NULL;
    }
  }
#endif

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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_T, "T", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  int save_token_index = data->current_token_index;

  /* Try production T → P T */
  SyntaxTreeNode *p_node = parse_P(parser, data);
  if (p_node) {
    syntax_tree_add_child(node, p_node);

    SyntaxTreeNode *t_node = parse_T(parser, data);
    if (t_node) {
      syntax_tree_add_child(node, t_node);
      node->production_id = 1; /* T → P T */
      production_tracker_add(parser->production_tracker, 1);

#ifdef CONFIG_TAC
      /* Execute semantic action for this production */
      if (parser->sdt_gen) {
        if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
          data->has_error = true;
          const char *error = sdt_codegen_get_error(parser->sdt_gen);
          if (error) {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed: %s", error);
          } else {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed");
          }
          // return NULL;
        }
      }
#endif
      return node;
    }
  }

  /* Backtrack if first production failed */
  data->current_token_index = save_token_index;
  data->has_error = false; /* Reset error flag */

  /* Try production T → ε */
  node->production_id = 2; /* T → ε */
  production_tracker_add(parser->production_tracker, 2);

  /* Add epsilon node */
  SyntaxTreeNode *epsilon = syntax_tree_create_epsilon();
  if (epsilon) {
    syntax_tree_add_child(node, epsilon);
  }

#ifdef CONFIG_TAC
  /* Execute semantic action for epsilon production */
  if (parser->sdt_gen) {
    if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
      data->has_error = true;
      const char *error = sdt_codegen_get_error(parser->sdt_gen);
      if (error) {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed: %s", error);
      } else {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed");
      }
      // return NULL;
    }
  }
#endif

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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_L, "L", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  /* Try production L → S ; */
  SyntaxTreeNode *s_node = parse_S(parser, data);
  if (!s_node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to parse statement (non-terminal S)");
    return NULL;
  }

  syntax_tree_add_child(node, s_node);

  if (!match_token(data, TK_SEMI, node, ";")) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Expected semicolon after statement");
    return NULL;
  }

  node->production_id = 3; /* L → S ; */
  production_tracker_add(parser->production_tracker, 3);

#ifdef CONFIG_TAC
  /* Execute semantic action for this production */
  if (parser->sdt_gen) {
    if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
      data->has_error = true;
      const char *error = sdt_codegen_get_error(parser->sdt_gen);
      if (error) {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed: %s", error);
      } else {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed");
      }
      // return NULL;
    }
  }
#endif
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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_S, "S", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  const Token *token = get_current_token(data);
  if (!token) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Unexpected end of input");
    return NULL;
  }

  int save_token_index = data->current_token_index;

  /* Try production S → id = E */
  if (token->type == TK_IDN) {
    if (match_token(data, TK_IDN, node, "id")) {
      if (match_token(data, TK_EQ, node, "=")) {
        SyntaxTreeNode *e_node = parse_E(parser, data);
        if (e_node) {
          syntax_tree_add_child(node, e_node);
          node->production_id = 4; /* S → id = E */
          production_tracker_add(parser->production_tracker, 4);

#ifdef CONFIG_TAC
          /* Execute semantic action for this production */
          if (parser->sdt_gen) {
            if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                    node)) {
              data->has_error = true;
              const char *error = sdt_codegen_get_error(parser->sdt_gen);
              if (error) {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed: %s", error);
              } else {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed");
              }
              // return NULL;
            }
          }
#endif

          return node;
        }
      }
    }
    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */
  }

  /* Try production S → if C then S N */
  if (token->type == TK_IF) {
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
              node->production_id = 5; /* S → if C then S N */
              production_tracker_add(parser->production_tracker, 5);

#ifdef CONFIG_TAC
              /* Execute semantic action for this production */
              if (parser->sdt_gen) {
                if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                        node)) {
                  data->has_error = true;
                  const char *error = sdt_codegen_get_error(parser->sdt_gen);
                  if (error) {
                    snprintf(data->error_message, sizeof(data->error_message),
                             "Semantic action failed: %s", error);
                  } else {
                    snprintf(data->error_message, sizeof(data->error_message),
                             "Semantic action failed");
                  }
                  // return NULL;
                }
              }
#endif

              return node;
            }
          }
        }
      }
    }
    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */
  }

  /* Try production S → while C do S */
  if (token->type == TK_WHILE) {
    if (match_token(data, TK_WHILE, node, "while")) {
      SyntaxTreeNode *c_node = parse_C(parser, data);
      if (c_node) {
        syntax_tree_add_child(node, c_node);

        if (match_token(data, TK_DO, node, "do")) {
          SyntaxTreeNode *s_node = parse_S(parser, data);
          if (s_node) {
            syntax_tree_add_child(node, s_node);
            node->production_id = 6; /* S → while C do S */
            production_tracker_add(parser->production_tracker, 6);

#ifdef CONFIG_TAC
            /* Execute semantic action for this production */
            if (parser->sdt_gen) {
              if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                      node)) {
                data->has_error = true;
                const char *error = sdt_codegen_get_error(parser->sdt_gen);
                if (error) {
                  snprintf(data->error_message, sizeof(data->error_message),
                           "Semantic action failed: %s", error);
                } else {
                  snprintf(data->error_message, sizeof(data->error_message),
                           "Semantic action failed");
                }
                // return NULL;
              }
            }
#endif

            return node;
          }
        }
      }
    }
    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */
  }

  /* Try production S → begin L end */
  if (token->type == TK_BEGIN) {
    if (match_token(data, TK_BEGIN, node, "begin")) {
      SyntaxTreeNode *l_node = parse_L(parser, data);
      if (l_node) {
        syntax_tree_add_child(node, l_node);

        if (match_token(data, TK_END, node, "end")) {
          node->production_id = 7; /* S → begin L end */
          production_tracker_add(parser->production_tracker, 7);

#ifdef CONFIG_TAC
          /* Execute semantic action for this production */
          if (parser->sdt_gen) {
            if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                    node)) {
              data->has_error = true;
              const char *error = sdt_codegen_get_error(parser->sdt_gen);
              if (error) {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed: %s", error);
              } else {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed");
              }
              // return NULL;
            }
          }
#endif

          return node;
        }
      }
    }
    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */
  }

  /* No valid production found */
  data->has_error = true;
  char token_str[128];
  token_to_string(token, token_str, sizeof(token_str));
  snprintf(data->error_message, sizeof(data->error_message),
           "Failed to parse statement (non-terminal S), unexpected token: %s",
           token_str);
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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_N, "N", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  const Token *token = get_current_token(data);
  if (!token) {
    /* Handle epsilon production */
    node->production_id = 9; /* N → ε */
    production_tracker_add(parser->production_tracker, 9);
    SyntaxTreeNode *epsilon = syntax_tree_create_epsilon();
    if (epsilon) {
      syntax_tree_add_child(node, epsilon);
    }

#ifdef CONFIG_TAC
    /* Execute semantic action for epsilon production */
    if (parser->sdt_gen) {
      if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
        data->has_error = true;
        const char *error = sdt_codegen_get_error(parser->sdt_gen);
        if (error) {
          snprintf(data->error_message, sizeof(data->error_message),
                   "Semantic action failed: %s", error);
        } else {
          snprintf(data->error_message, sizeof(data->error_message),
                   "Semantic action failed");
        }
        // return NULL;
      }
    }
#endif

    return node;
  }

  /* Try production N → else S */
  if (token->type == TK_ELSE) {
    if (match_token(data, TK_ELSE, node, "else")) {
      SyntaxTreeNode *s_node = parse_S(parser, data);
      if (s_node) {
        syntax_tree_add_child(node, s_node);
        node->production_id = 8; /* N → else S */
        production_tracker_add(parser->production_tracker, 8);

#ifdef CONFIG_TAC
        /* Execute semantic action for this production */
        if (parser->sdt_gen) {
          if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
            data->has_error = true;
            const char *error = sdt_codegen_get_error(parser->sdt_gen);
            if (error) {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed: %s", error);
            } else {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed");
            }
            // return NULL;
          }
        }
#endif

        return node;
      }
    }
  }

  /* If ELSE not found, handle as epsilon production */
  node->production_id = 9; /* N → ε */
  production_tracker_add(parser->production_tracker, 9);
  SyntaxTreeNode *epsilon = syntax_tree_create_epsilon();
  if (epsilon) {
    syntax_tree_add_child(node, epsilon);
  }

  /* Reset error flag as this is a valid epsilon production */
  data->has_error = false;

#ifdef CONFIG_TAC
  /* Execute semantic action for epsilon production */
  if (parser->sdt_gen) {
    if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
      data->has_error = true;
      const char *error = sdt_codegen_get_error(parser->sdt_gen);
      if (error) {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed: %s", error);
      } else {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed");
      }
      // return NULL;
    }
  }
#endif

  return node;
}

/**
 * @brief Parse non-terminal C (Condition)
 */
static SyntaxTreeNode *parse_C(Parser *parser, RDParserData *data) {
  if (!parser || !data) {
    return NULL;
  }

  /* Create node for non-terminal C */
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_C, "C", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  const Token *token = get_current_token(data);
  if (!token) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Unexpected end of input");
    return NULL;
  }

  int save_token_index = data->current_token_index;

  /* Try production C → ( C ) */
  if (token->type == TK_SLP) {
    if (match_token(data, TK_SLP, node, "(")) {
      SyntaxTreeNode *c_node = parse_C(parser, data);
      if (c_node) {
        syntax_tree_add_child(node, c_node);

        if (match_token(data, TK_SRP, node, ")")) {
          node->production_id = 16; /* C → ( C ) */
          production_tracker_add(parser->production_tracker, 16);

#ifdef CONFIG_TAC
          /* Execute semantic action for this production */
          if (parser->sdt_gen) {
            if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                    node)) {
              data->has_error = true;
              const char *error = sdt_codegen_get_error(parser->sdt_gen);
              if (error) {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed: %s", error);
              } else {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed");
              }
              // return NULL;
            }
          }
#endif

          return node;
        }
      }
    }
    /* Backtrack if production failed */
    data->current_token_index = save_token_index;
    data->has_error = false; /* Reset error flag */
  }

  /* Parse left-hand expression for relational operations */
  SyntaxTreeNode *left_e_node = parse_E(parser, data);
  if (!left_e_node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to parse left expression in condition");
    return NULL;
  }
  syntax_tree_add_child(node, left_e_node);

  token = get_current_token(data);
  if (!token) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Unexpected end of input in condition");
    return NULL;
  }

  /* Try different condition operators */
  if (token->type == TK_GT) {
    /* C → E > E */
    if (match_token(data, TK_GT, node, ">")) {
      SyntaxTreeNode *right_e_node = parse_E(parser, data);
      if (right_e_node) {
        syntax_tree_add_child(node, right_e_node);
        node->production_id = 10; /* C → E > E */
        production_tracker_add(parser->production_tracker, 10);

#ifdef CONFIG_TAC
        /* Execute semantic action for this production */
        if (parser->sdt_gen) {
          if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
            data->has_error = true;
            const char *error = sdt_codegen_get_error(parser->sdt_gen);
            if (error) {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed: %s", error);
            } else {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed");
            }
            // return NULL;
          }
        }
#endif
        return node;
      }
    }
  } else if (token->type == TK_LT) {
    /* C → E < E */
    if (match_token(data, TK_LT, node, "<")) {
      SyntaxTreeNode *right_e_node = parse_E(parser, data);
      if (right_e_node) {
        syntax_tree_add_child(node, right_e_node);
        node->production_id = 11; /* C → E < E */
        production_tracker_add(parser->production_tracker, 11);

#ifdef CONFIG_TAC
        /* Execute semantic action for this production */
        if (parser->sdt_gen) {
          if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
            data->has_error = true;
            const char *error = sdt_codegen_get_error(parser->sdt_gen);
            if (error) {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed: %s", error);
            } else {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed");
            }
            // return NULL;
          }
        }
#endif

        return node;
      }
    }
  } else if (token->type == TK_EQ) {
    /* C → E = E */
    if (match_token(data, TK_EQ, node, "=")) {
      SyntaxTreeNode *right_e_node = parse_E(parser, data);
      if (right_e_node) {
        syntax_tree_add_child(node, right_e_node);
        node->production_id = 12; /* C → E = E */
        production_tracker_add(parser->production_tracker, 12);

#ifdef CONFIG_TAC
        /* Execute semantic action for this production */
        if (parser->sdt_gen) {
          if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
            data->has_error = true;
            const char *error = sdt_codegen_get_error(parser->sdt_gen);
            if (error) {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed: %s", error);
            } else {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed");
            }
            // return NULL;
          }
        }
#endif
        return node;
      }
    }
  } else if (token->type == TK_GE) {
    /* C → E >= E */
    if (match_token(data, TK_GE, node, ">=")) {
      SyntaxTreeNode *right_e_node = parse_E(parser, data);
      if (right_e_node) {
        syntax_tree_add_child(node, right_e_node);
        node->production_id = 13; /* C → E >= E */
        production_tracker_add(parser->production_tracker, 13);

#ifdef CONFIG_TAC
        /* Execute semantic action for this production */
        if (parser->sdt_gen) {
          if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
            data->has_error = true;
            const char *error = sdt_codegen_get_error(parser->sdt_gen);
            if (error) {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed: %s", error);
            } else {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed");
            }
            // return NULL;
          }
        }
#endif

        return node;
      }
    }
  } else if (token->type == TK_LE) {
    /* C → E <= E */
    if (match_token(data, TK_LE, node, "<=")) {
      SyntaxTreeNode *right_e_node = parse_E(parser, data);
      if (right_e_node) {
        syntax_tree_add_child(node, right_e_node);
        node->production_id = 14; /* C → E <= E */
        production_tracker_add(parser->production_tracker, 14);

#ifdef CONFIG_TAC
        /* Execute semantic action for this production */
        if (parser->sdt_gen) {
          if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
            data->has_error = true;
            const char *error = sdt_codegen_get_error(parser->sdt_gen);
            if (error) {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed: %s", error);
            } else {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed");
            }
            // return NULL;
          }
        }
#endif

        return node;
      }
    }
  } else if (token->type == TK_NEQ) {
    /* C → E <> E */
    if (match_token(data, TK_NEQ, node, "<>")) {
      SyntaxTreeNode *right_e_node = parse_E(parser, data);
      if (right_e_node) {
        syntax_tree_add_child(node, right_e_node);
        node->production_id = 15; /* C → E <> E */
        production_tracker_add(parser->production_tracker, 15);

#ifdef CONFIG_TAC
        /* Execute semantic action for this production */
        if (parser->sdt_gen) {
          if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
            data->has_error = true;
            const char *error = sdt_codegen_get_error(parser->sdt_gen);
            if (error) {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed: %s", error);
            } else {
              snprintf(data->error_message, sizeof(data->error_message),
                       "Semantic action failed");
            }
            // return NULL;
          }
        }
#endif

        return node;
      }
    }
  }

  /* No valid production found */
  data->has_error = true;
  char token_str[128];
  token_to_string(token, token_str, sizeof(token_str));
  snprintf(data->error_message, sizeof(data->error_message),
           "Failed to parse condition (non-terminal C), unexpected token: %s",
           token_str);
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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_E, "E", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  /* Try production E → R X */
  SyntaxTreeNode *r_node = parse_R(parser, data);
  if (!r_node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to parse term (non-terminal R)");
    return NULL;
  }

  syntax_tree_add_child(node, r_node);

  SyntaxTreeNode *x_node = parse_X(parser, data);
  if (!x_node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to parse expression tail (non-terminal X)");
    return NULL;
  }

  syntax_tree_add_child(node, x_node);
  node->production_id = 17; /* E → R X */
  production_tracker_add(parser->production_tracker, 17);

#ifdef CONFIG_TAC
  /* Execute semantic action for this production */
  if (parser->sdt_gen) {
    if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
      data->has_error = true;
      const char *error = sdt_codegen_get_error(parser->sdt_gen);
      if (error) {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed: %s", error);
      } else {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed");
      }
      // return NULL;
    }
  }
#endif

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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_X, "X", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  const Token *token = get_current_token(data);
  if (!token) {
    /* Handle epsilon production */
    node->production_id = 20; /* X → ε */
    production_tracker_add(parser->production_tracker, 20);
    SyntaxTreeNode *epsilon = syntax_tree_create_epsilon();
    if (epsilon) {
      syntax_tree_add_child(node, epsilon);
    }

#ifdef CONFIG_TAC
    /* Execute semantic action for epsilon production */
    if (parser->sdt_gen) {
      if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
        data->has_error = true;
        const char *error = sdt_codegen_get_error(parser->sdt_gen);
        if (error) {
          snprintf(data->error_message, sizeof(data->error_message),
                   "Semantic action failed: %s", error);
        } else {
          snprintf(data->error_message, sizeof(data->error_message),
                   "Semantic action failed");
        }
        // return NULL;
      }
    }
#endif

    return node;
  }

  if (token->type == TK_ADD) {
    /* X → + R X */
    if (match_token(data, TK_ADD, node, "+")) {
      SyntaxTreeNode *r_node = parse_R(parser, data);
      if (r_node) {
        syntax_tree_add_child(node, r_node);

        SyntaxTreeNode *x_node = parse_X(parser, data);
        if (x_node) {
          syntax_tree_add_child(node, x_node);
          node->production_id = 18; /* X → + R X */
          production_tracker_add(parser->production_tracker, 18);

#ifdef CONFIG_TAC
          /* Execute semantic action for this production */
          if (parser->sdt_gen) {
            if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                    node)) {
              data->has_error = true;
              const char *error = sdt_codegen_get_error(parser->sdt_gen);
              if (error) {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed: %s", error);
              } else {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed");
              }
              // return NULL;
            }
          }
#endif

          return node;
        }
      }
    }
  } else if (token->type == TK_SUB) {
    /* X → - R X */
    if (match_token(data, TK_SUB, node, "-")) {
      SyntaxTreeNode *r_node = parse_R(parser, data);
      if (r_node) {
        syntax_tree_add_child(node, r_node);

        SyntaxTreeNode *x_node = parse_X(parser, data);
        if (x_node) {
          syntax_tree_add_child(node, x_node);
          node->production_id = 19; /* X → - R X */
          production_tracker_add(parser->production_tracker, 19);

#ifdef CONFIG_TAC
          /* Execute semantic action for this production */
          if (parser->sdt_gen) {
            if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                    node)) {
              data->has_error = true;
              const char *error = sdt_codegen_get_error(parser->sdt_gen);
              if (error) {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed: %s", error);
              } else {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed");
              }
              // return NULL;
            }
          }
#endif

          return node;
        }
      }
    }
  }

  /* If no '+' or '-', handle as epsilon production */
  node->production_id = 20; /* X → ε */
  production_tracker_add(parser->production_tracker, 20);
  SyntaxTreeNode *epsilon = syntax_tree_create_epsilon();
  if (epsilon) {
    syntax_tree_add_child(node, epsilon);
  }

  /* Reset error flag as this is a valid epsilon production */
  data->has_error = false;

#ifdef CONFIG_TAC
  /* Execute semantic action for epsilon production */
  if (parser->sdt_gen) {
    if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
      data->has_error = true;
      const char *error = sdt_codegen_get_error(parser->sdt_gen);
      if (error) {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed: %s", error);
      } else {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed");
      }
      // return NULL;
    }
  }
#endif

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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_R, "R", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  /* Try production R → F Y */
  SyntaxTreeNode *f_node = parse_F(parser, data);
  if (!f_node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to parse factor (non-terminal F)");
    return NULL;
  }

  syntax_tree_add_child(node, f_node);

  SyntaxTreeNode *y_node = parse_Y(parser, data);
  if (!y_node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to parse term tail (non-terminal Y)");
    return NULL;
  }

  syntax_tree_add_child(node, y_node);
  node->production_id = 21; /* R → F Y */
  production_tracker_add(parser->production_tracker, 21);

#ifdef CONFIG_TAC
  /* Execute semantic action for this production */
  if (parser->sdt_gen) {
    if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
      data->has_error = true;
      const char *error = sdt_codegen_get_error(parser->sdt_gen);
      if (error) {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed: %s", error);
      } else {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed");
      }
      // return NULL;
    }
  }
#endif

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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_Y, "Y", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  const Token *token = get_current_token(data);
  if (!token) {
    /* Handle epsilon production */
    node->production_id = 24; /* Y → ε */
    production_tracker_add(parser->production_tracker, 24);
    SyntaxTreeNode *epsilon = syntax_tree_create_epsilon();
    if (epsilon) {
      syntax_tree_add_child(node, epsilon);
    }

#ifdef CONFIG_TAC
    /* Execute semantic action for epsilon production */
    if (parser->sdt_gen) {
      if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
        data->has_error = true;
        const char *error = sdt_codegen_get_error(parser->sdt_gen);
        if (error) {
          snprintf(data->error_message, sizeof(data->error_message),
                   "Semantic action failed: %s", error);
        } else {
          snprintf(data->error_message, sizeof(data->error_message),
                   "Semantic action failed");
        }
        // return NULL;
      }
    }
#endif

    return node;
  }

  if (token->type == TK_MUL) {
    /* Y → * F Y */
    if (match_token(data, TK_MUL, node, "*")) {
      SyntaxTreeNode *f_node = parse_F(parser, data);
      if (f_node) {
        syntax_tree_add_child(node, f_node);

        SyntaxTreeNode *y_node = parse_Y(parser, data);
        if (y_node) {
          syntax_tree_add_child(node, y_node);
          node->production_id = 22; /* Y → * F Y */
          production_tracker_add(parser->production_tracker, 22);

#ifdef CONFIG_TAC
          /* Execute semantic action for this production */
          if (parser->sdt_gen) {
            if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                    node)) {
              data->has_error = true;
              const char *error = sdt_codegen_get_error(parser->sdt_gen);
              if (error) {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed: %s", error);
              } else {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed");
              }
              // return NULL;
            }
          }
#endif

          return node;
        }
      }
    }
  } else if (token->type == TK_DIV) {
    /* Y → / F Y */
    if (match_token(data, TK_DIV, node, "/")) {
      SyntaxTreeNode *f_node = parse_F(parser, data);
      if (f_node) {
        syntax_tree_add_child(node, f_node);

        SyntaxTreeNode *y_node = parse_Y(parser, data);
        if (y_node) {
          syntax_tree_add_child(node, y_node);
          node->production_id = 23; /* Y → / F Y */
          production_tracker_add(parser->production_tracker, 23);

#ifdef CONFIG_TAC
          /* Execute semantic action for this production */
          if (parser->sdt_gen) {
            if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                    node)) {
              data->has_error = true;
              const char *error = sdt_codegen_get_error(parser->sdt_gen);
              if (error) {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed: %s", error);
              } else {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed");
              }
              // return NULL;
            }
          }
#endif

          return node;
        }
      }
    }
  }

  /* If no '*' or '/', handle as epsilon production */
  node->production_id = 24; /* Y → ε */
  production_tracker_add(parser->production_tracker, 24);
  SyntaxTreeNode *epsilon = syntax_tree_create_epsilon();
  if (epsilon) {
    syntax_tree_add_child(node, epsilon);
  }

  /* Reset error flag as this is a valid epsilon production */
  data->has_error = false;

#ifdef CONFIG_TAC
  /* Execute semantic action for epsilon production */
  if (parser->sdt_gen) {
    if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
      data->has_error = true;
      const char *error = sdt_codegen_get_error(parser->sdt_gen);
      if (error) {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed: %s", error);
      } else {
        snprintf(data->error_message, sizeof(data->error_message),
                 "Semantic action failed");
      }
      // return NULL;
    }
  }
#endif

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
  SyntaxTreeNode *node = syntax_tree_create_nonterminal(NT_F, "F", -1);
  if (!node) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Failed to create syntax tree node");
    return NULL;
  }

  const Token *token = get_current_token(data);
  if (!token) {
    data->has_error = true;
    snprintf(data->error_message, sizeof(data->error_message),
             "Unexpected end of input");
    return NULL;
  }

  /* Try different factor productions */
  if (token->type == TK_SLP) {
    /* F → ( E ) */
    if (match_token(data, TK_SLP, node, "(")) {
      SyntaxTreeNode *e_node = parse_E(parser, data);
      if (e_node) {
        syntax_tree_add_child(node, e_node);

        if (match_token(data, TK_SRP, node, ")")) {
          node->production_id = 25; /* F → ( E ) */
          production_tracker_add(parser->production_tracker, 25);

#ifdef CONFIG_TAC
          /* Execute semantic action for this production */
          if (parser->sdt_gen) {
            if (!sdt_execute_action(parser->sdt_gen, node->production_id,
                                    node)) {
              data->has_error = true;
              const char *error = sdt_codegen_get_error(parser->sdt_gen);
              if (error) {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed: %s", error);
              } else {
                snprintf(data->error_message, sizeof(data->error_message),
                         "Semantic action failed");
              }
              // return NULL;
            }
          }
#endif

          return node;
        }
      }
    }
  } else if (token->type == TK_IDN) {
    /* F → id */
    if (match_token(data, TK_IDN, node, "id")) {
      node->production_id = 26; /* F → id */
      production_tracker_add(parser->production_tracker, 26);

#ifdef CONFIG_TAC
      /* Execute semantic action for this production */
      if (parser->sdt_gen) {
        if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
          data->has_error = true;
          const char *error = sdt_codegen_get_error(parser->sdt_gen);
          if (error) {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed: %s", error);
          } else {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed");
          }
          // return NULL;
        }
      }
#endif

      return node;
    }
  } else if (token->type == TK_OCT) {
    /* F → int8 */
    if (match_token(data, TK_OCT, node, "int8")) {
      node->production_id = 27; /* F → int8 */
      production_tracker_add(parser->production_tracker, 27);

#ifdef CONFIG_TAC
      /* Execute semantic action for this production */
      if (parser->sdt_gen) {
        if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
          data->has_error = true;
          const char *error = sdt_codegen_get_error(parser->sdt_gen);
          if (error) {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed: %s", error);
          } else {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed");
          }
          // return NULL;
        }
      }
#endif

      return node;
    }
  } else if (token->type == TK_DEC) {
    /* F → int10 */
    if (match_token(data, TK_DEC, node, "int10")) {
      node->production_id = 28; /* F → int10 */
      production_tracker_add(parser->production_tracker, 28);

#ifdef CONFIG_TAC
      /* Execute semantic action for this production */
      if (parser->sdt_gen) {
        if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
          data->has_error = true;
          const char *error = sdt_codegen_get_error(parser->sdt_gen);
          if (error) {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed: %s", error);
          } else {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed");
          }
          // return NULL;
        }
      }
#endif

      return node;
    }
  } else if (token->type == TK_HEX) {
    /* F → int16 */
    if (match_token(data, TK_HEX, node, "int16")) {
      node->production_id = 29; /* F → int16 */
      production_tracker_add(parser->production_tracker, 29);

#ifdef CONFIG_TAC
      /* Execute semantic action for this production */
      if (parser->sdt_gen) {
        if (!sdt_execute_action(parser->sdt_gen, node->production_id, node)) {
          data->has_error = true;
          const char *error = sdt_codegen_get_error(parser->sdt_gen);
          if (error) {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed: %s", error);
          } else {
            snprintf(data->error_message, sizeof(data->error_message),
                     "Semantic action failed");
          }
          // return NULL;
        }
      }
#endif

      return node;
    }
  }

  /* No valid production found */
  data->has_error = true;
  char token_str[128];
  token_to_string(token, token_str, sizeof(token_str));
  snprintf(data->error_message, sizeof(data->error_message),
           "Failed to parse factor (non-terminal F), unexpected token: %s",
           token_str);
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
    return NULL;
  }

  /* Set root of syntax tree */
  syntax_tree_set_root(data->syntax_tree, root);

  /* Check for trailing tokens */
  const Token *token = get_current_token(data);
  if (token) {
    char token_str[128];
    token_to_string(token, token_str, sizeof(token_str));
    fprintf(stderr, "Warning: Trailing tokens in input starting with: %s\n",
            token_str);
  }

  DEBUG_PRINT("Parsing completed successfully");
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
