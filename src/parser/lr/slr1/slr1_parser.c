/**
 * @file slr1_parser.c
 * @brief SLR(1) parser implementation
 */
#include "slr1_parser.h"
#include "../lr_parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Build the SLR(1) automaton
 */
bool slr1_build_automaton(Parser *parser, SLR1ParserData *data) {
  if (!parser || !data) {
    return false;
  }

  LRParserData *common = &data->common;

  /* Create automaton */
  common->automaton = lr_automaton_create(parser->grammar);
  if (!common->automaton) {
    return false;
  }

  /* Build canonical collection of LR(0) items */
  /* SLR(1) uses LR(0) items but FOLLOW sets for reductions */
  if (!lr_create_canonical_collection(common->automaton, parser->grammar,
                                      false)) {
    return false;
  }

  DEBUG_PRINT("Built SLR(1) automaton with %d states",
              common->automaton->state_count);
  return true;
}

/**
 * @brief Build the SLR(1) parsing table
 */
bool slr1_build_parsing_table(Parser *parser, SLR1ParserData *data) {
  if (!parser || !data) {
    return false;
  }

  LRParserData *common = &data->common;
  LRAutomaton *automaton = common->automaton;
  Grammar *grammar = parser->grammar;

  /* Create parsing table */
  common->table =
      action_table_create(automaton->state_count, grammar->terminals_count,
                          grammar->nonterminals_count);

  if (!common->table) {
    return false;
  }

  /* Find EOF and semicolon token indices */
  int eof_idx = -1;
  int semi_idx = -1;
  for (int i = 0; i < grammar->terminals_count; i++) {
    TokenType token = grammar->symbols[grammar->terminal_indices[i]].token;
    if (token == TK_EOF) {
      eof_idx = i;
    } else if (token == TK_SEMI) {
      semi_idx = i;
    }
  }

  /* Fill parsing table */
  for (int state_idx = 0; state_idx < automaton->state_count; state_idx++) {
    LRState *state = automaton->states[state_idx];

    /* First pass: Add all shift actions from transitions */
    for (int trans = 0; trans < state->transition_count; trans++) {
      int symbol_id = state->transitions[trans].symbol_id;
      LRState *target = state->transitions[trans].state;

      /* Handle terminal shifts */
      for (int term = 0; term < grammar->terminals_count; term++) {
        if (grammar->terminal_indices[term] == symbol_id) {
          action_table_set_action(common->table, state_idx, term, ACTION_SHIFT,
                                  target->id);

          /* Debug info for semicolon shifts */
          if (term == semi_idx) {
            DEBUG_PRINT(
                "Set SHIFT action for state %d on SEMICOLON to state %d",
                state_idx, target->id);
          }
          break;
        }
      }

      /* Handle non-terminal gotos */
      for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
        if (grammar->nonterminal_indices[nt] == symbol_id) {
          action_table_set_goto(common->table, state_idx, nt, target->id);
          break;
        }
      }
    }

    /* Second pass: Add reduction actions based on FOLLOW sets */
    for (int item_idx = 0; item_idx < state->item_count; item_idx++) {
      LRItem *item = state->items[item_idx];

      /* Check for reduction */
      if (lr_item_is_reduction(item, grammar)) {
        Production *prod = &grammar->productions[item->production_id];

        /* Check if this is a special case for the augmented start production */
        if (prod->lhs == grammar->start_symbol &&
            item->dot_position >= prod->rhs_length) {

          /* Accept on EOF */
          if (eof_idx >= 0) {
            action_table_set_action(common->table, state_idx, eof_idx,
                                    ACTION_ACCEPT, 0);
            DEBUG_PRINT("Set ACCEPT action for state %d on EOF", state_idx);
          }
        } else {
          /* For all other reductions, use FOLLOW set of LHS */
          int lhs = prod->lhs;

          /* Extra debug for statement productions */
          bool is_stmt_production = false;
          for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
            if (grammar->nonterminal_indices[nt] == lhs) {
              const char *nt_name =
                  grammar->symbols[grammar->nonterminal_indices[nt]].name;
              if (strcmp(nt_name, "S") == 0 || strcmp(nt_name, "L") == 0) {
                is_stmt_production = true;
              }
              break;
            }
          }

          /* Add reduction actions for terminals in FOLLOW(LHS) */
          for (int term = 0; term < grammar->terminals_count; term++) {
            TokenType token =
                grammar->symbols[grammar->terminal_indices[term]].token;

            if (grammar_is_in_follow(grammar, lhs, token)) {
              /* Get existing action if any */
              Action existing =
                  action_table_get_action(common->table, state_idx, term);

              /* Determine if we should overwrite with reduction */
              bool add_reduction = true;

              /* Check for conflicts */
              if (existing.type == ACTION_SHIFT) {
                /* Special handling for empty productions */
                bool is_empty_production =
                    (prod->rhs_length == 0 ||
                     (prod->rhs_length == 1 &&
                      prod->rhs[0].type == SYMBOL_EPSILON));

                /* Prefer reduce for empty productions, shift otherwise */
                if (!is_empty_production) {
                  add_reduction = false;
                }

                /* Special case for semicolon - prioritize shift for statements
                 */
                if (token == TK_SEMI && !is_stmt_production) {
                  add_reduction = false;
                  DEBUG_PRINT(
                      "Preferring SHIFT over REDUCE for SEMICOLON in state %d",
                      state_idx);
                }
              }

              if (add_reduction) {
                action_table_set_action(common->table, state_idx, term,
                                        ACTION_REDUCE, item->production_id);

                /* Debug for semicolon reductions */
                if (token == TK_SEMI) {
                  DEBUG_PRINT("Set REDUCE action for state %d on SEMICOLON by "
                              "production %d",
                              state_idx, item->production_id);
                }
              }
            }
          }
        }
      }
    }
  }

  /* Special handling for semicolon-terminated statements */
  if (semi_idx >= 0) {
    for (int state_idx = 0; state_idx < automaton->state_count; state_idx++) {
      LRState *state = automaton->states[state_idx];

      /* Look for statement list states */
      bool is_statement_list_state = false;
      for (int item_idx = 0; item_idx < state->item_count; item_idx++) {
        LRItem *item = state->items[item_idx];
        Production *prod = &grammar->productions[item->production_id];

        /* Check productions related to statements */
        for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
          const char *nt_name =
              grammar->symbols[grammar->nonterminal_indices[nt]].name;
          if ((strcmp(nt_name, "L") == 0 || strcmp(nt_name, "S") == 0) &&
              prod->lhs == grammar->nonterminal_indices[nt]) {
            is_statement_list_state = true;
            break;
          }
        }

        if (is_statement_list_state)
          break;
      }

      /* For statement list states, ensure semicolon has correct handling */
      if (is_statement_list_state) {
        Action semi_action =
            action_table_get_action(common->table, state_idx, semi_idx);

        /* If we don't have an action for semicolon, check if we need one */
        if (semi_action.type == ACTION_ERROR) {
          for (int trans = 0; trans < state->transition_count; trans++) {
            int symbol_id = state->transitions[trans].symbol_id;

            /* Check if there's a transition after statement */
            for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
              const char *nt_name =
                  grammar->symbols[grammar->nonterminal_indices[nt]].name;
              if (strcmp(nt_name, "S") == 0 &&
                  grammar->nonterminal_indices[nt] == symbol_id) {

                /* This may be a state that needs semicolon handling */
                DEBUG_PRINT("Special check for semicolon handling in state %d",
                            state_idx);
              }
            }
          }
        }
      }
    }
  }

  /* Add special handling for end-of-statement contexts */
  for (int state_idx = 0; state_idx < automaton->state_count; state_idx++) {
    LRState *state = automaton->states[state_idx];

    /* Look for statement-end states */
    for (int item_idx = 0; item_idx < state->item_count; item_idx++) {
      LRItem *item = state->items[item_idx];
      Production *prod = &grammar->productions[item->production_id];

      /* For S -> ... • ; forms */
      if (item->dot_position > 0 && item->dot_position == prod->rhs_length) {
        for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
          const char *nt_name =
              grammar->symbols[grammar->nonterminal_indices[nt]].name;
          if (strcmp(nt_name, "S") == 0 &&
              prod->lhs == grammar->nonterminal_indices[nt]) {

            /* Make sure semicolon can be accepted after statement */
            if (semi_idx >= 0) {
              Action semi_action =
                  action_table_get_action(common->table, state_idx, semi_idx);
              if (semi_action.type == ACTION_ERROR) {
                /* There might be a missing transition or reduction */
                DEBUG_PRINT("Statement end state %d missing semicolon handling",
                            state_idx);
              }
            }
          }
        }
      }
    }
  }

  DEBUG_PRINT("Built SLR(1) parsing table");
  return true;
}

/**
 * @brief Create an SLR(1) parser
 */
Parser *slr1_parser_create(void) {
  Parser *parser = (Parser *)safe_malloc(sizeof(Parser));
  if (!parser) {
    return NULL;
  }

  /* Initialize parser */
  parser->type = PARSER_TYPE_SLR1;
  parser->grammar = NULL;
  parser->production_tracker = NULL;
  parser->init = slr1_parser_init;
  parser->parse = slr1_parser_parse;
  parser->print_leftmost_derivation = slr1_parser_print_leftmost_derivation;
  parser->destroy = slr1_parser_destroy;

  /* Create parser-specific data */
  parser->data = safe_malloc(sizeof(SLR1ParserData));
  if (!parser->data) {
    free(parser);
    return NULL;
  }

  memset(parser->data, 0, sizeof(SLR1ParserData));
  DEBUG_PRINT("Created SLR(1) parser");
  return parser;
}

/**
 * @brief Initialize the SLR(1) parser
 */
bool slr1_parser_init(Parser *parser) {
  if (!parser || !parser->data) {
    return false;
  }

  SLR1ParserData *data = (SLR1ParserData *)parser->data;

  /* Initialize common LR parser data */
  if (!lr_parser_data_init(&data->common)) {
    return false;
  }

  /* Build SLR(1) automaton */
  if (!slr1_build_automaton(parser, data)) {
    lr_parser_data_cleanup(&data->common);
    return false;
  }

  /* Build SLR(1) parsing table */
  if (!slr1_build_parsing_table(parser, data)) {
    lr_parser_data_cleanup(&data->common);
    return false;
  }

  DEBUG_PRINT("Initialized SLR(1) parser");
  return true;
}

/**
 * @brief Parse input using SLR(1) parsing
 */
SyntaxTree *slr1_parser_parse(Parser *parser, Lexer *lexer) {
  if (!parser || !parser->data) {
    return NULL;
  }

  SLR1ParserData *data = (SLR1ParserData *)parser->data;

  /* Use common LR parsing algorithm */
  return lr_parser_parse(parser, &data->common, lexer);
}

/**
 * @brief Print the leftmost derivation from the SLR(1) parser
 */
void slr1_parser_print_leftmost_derivation(Parser *parser) {
  if (!parser) {
    return;
  }

  /* Use common production tracker */
  production_tracker_print(parser->production_tracker, parser->grammar);
}

/**
 * @brief Destroy SLR(1) parser and free resources
 */
void slr1_parser_destroy(Parser *parser) {
  if (!parser) {
    return;
  }

  if (parser->data) {
    SLR1ParserData *data = (SLR1ParserData *)parser->data;

    /* Clean up common LR parser data */
    lr_parser_data_cleanup(&data->common);
    free(parser->data);
  }

  free(parser);
  DEBUG_PRINT("Destroyed SLR(1) parser");
}
