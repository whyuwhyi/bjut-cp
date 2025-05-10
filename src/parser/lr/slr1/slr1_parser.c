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

  /* Find EOF token index */
  int eof_idx = -1;
  for (int i = 0; i < grammar->terminals_count; i++) {
    if (grammar->symbols[grammar->terminal_indices[i]].token == TK_EOF) {
      eof_idx = i;
      break;
    }
  }

  /* Fill parsing table */
  for (int state_idx = 0; state_idx < automaton->state_count; state_idx++) {
    LRState *state = automaton->states[state_idx];

    /* Fill action table based on items */
    for (int item_idx = 0; item_idx < state->item_count; item_idx++) {
      LRItem *item = state->items[item_idx];

      /* Check for reduction */
      if (lr_item_is_reduction(item, grammar)) {
        Production *prod = &grammar->productions[item->production_id];

        /* Accept if this is [S' -> S.] or [S' -> S. #] */
        if (prod->lhs == grammar->start_symbol &&
            item->dot_position >= prod->rhs_length) {

          /* Set ACCEPT action for EOF */
          if (eof_idx >= 0) {
            action_table_set_action(common->table, state_idx, eof_idx,
                                    ACTION_ACCEPT, 0);
            DEBUG_PRINT("Set ACCEPT action for state %d on EOF", state_idx);
          }

          /* Also accept on semicolon for compatibility */
          int semi_idx = -1;
          for (int i = 0; i < grammar->terminals_count; i++) {
            if (grammar->symbols[grammar->terminal_indices[i]].token ==
                TK_SEMI) {
              semi_idx = i;
              break;
            }
          }

          if (semi_idx >= 0) {
            action_table_set_action(common->table, state_idx, semi_idx,
                                    ACTION_ACCEPT, 0);
            DEBUG_PRINT("Set ACCEPT action for state %d on semicolon",
                        state_idx);
          }
        } else {
          /* Reduce by this production for terminals in FOLLOW(LHS) */
          int lhs = prod->lhs;

          for (int term = 0; term < grammar->terminals_count; term++) {
            TokenType token =
                grammar->symbols[grammar->terminal_indices[term]].token;

            if (grammar_is_in_follow(grammar, lhs, token)) {
              /* For empty productions, prioritize reduction over shifts */
              bool is_empty_production =
                  (prod->rhs_length == 0 ||
                   (prod->rhs_length == 1 &&
                    prod->rhs[0].type == SYMBOL_EPSILON));

              /* Set the reduction action */
              action_table_set_action(common->table, state_idx, term,
                                      ACTION_REDUCE, item->production_id);

              /* Debug output for important reductions */
              if (is_empty_production) {
                const char *term_name =
                    grammar->symbols[grammar->terminal_indices[term]].name;
                const char *nt_name =
                    grammar->symbols[grammar->nonterminal_indices[lhs]].name;
                DEBUG_PRINT("Set REDUCE action for state %d on %s by empty "
                            "production %s -> ε (id: %d)",
                            state_idx, term_name, nt_name, item->production_id);
              }
            }
          }
        }
      }
    }

    /* Fill action and goto tables based on transitions */
    for (int trans = 0; trans < state->transition_count; trans++) {
      int symbol_id = state->transitions[trans].symbol_id;
      LRState *target = state->transitions[trans].state;

      /* Check if this is a terminal */
      for (int term = 0; term < grammar->terminals_count; term++) {
        if (grammar->terminal_indices[term] == symbol_id) {
          /* Handle shift-reduce conflicts - prefer shift except for empty
           * productions */
          Action existing =
              action_table_get_action(common->table, state_idx, term);

          if (existing.type == ACTION_REDUCE) {
            /* Get production info */
            Production *reduce_prod = &grammar->productions[existing.value];

            /* If reduction production is an empty production, prefer reduce */
            if (reduce_prod->rhs_length == 0 ||
                (reduce_prod->rhs_length == 1 &&
                 reduce_prod->rhs[0].type == SYMBOL_EPSILON)) {

              /* Keep reduce action, don't add shift */
              const char *term_name =
                  grammar->symbols[grammar->terminal_indices[term]].name;
              DEBUG_PRINT("Resolved shift-reduce conflict in state %d for %s "
                          "in favor of reduce (empty production)",
                          state_idx, term_name);
              continue;
            }
            /* Otherwise, prefer shift (default SLR behavior) */
          }

          action_table_set_action(common->table, state_idx, term, ACTION_SHIFT,
                                  target->id);
          break;
        }
      }

      /* Check if this is a non-terminal */
      for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
        if (grammar->nonterminal_indices[nt] == symbol_id) {
          action_table_set_goto(common->table, state_idx, nt, target->id);
          break;
        }
      }
    }
  }

  /* Special handling for start states to ensure proper EOF acceptance */
  for (int state_idx = 0; state_idx < automaton->state_count; state_idx++) {
    LRState *state = automaton->states[state_idx];

    for (int item_idx = 0; item_idx < state->item_count; item_idx++) {
      LRItem *item = state->items[item_idx];
      Production *prod = &grammar->productions[item->production_id];

      /* Look for items representing full program parse */
      if (prod->lhs == grammar->start_symbol) {
        /* S' -> P • or similar forms */
        if (item->dot_position == prod->rhs_length) {
          /* Set ACCEPT action for EOF */
          if (eof_idx >= 0) {
            action_table_set_action(common->table, state_idx, eof_idx,
                                    ACTION_ACCEPT, 0);
            DEBUG_PRINT("Set additional ACCEPT action for state %d on EOF",
                        state_idx);
          }
        }
        /* S' -> P • # or similar forms */
        else if (item->dot_position == 1 && prod->rhs_length == 2 &&
                 prod->rhs[1].type == SYMBOL_TERMINAL &&
                 prod->rhs[1].token == TK_EOF) {

          /* Look for transition on EOF */
          for (int trans = 0; trans < state->transition_count; trans++) {
            int symbol_id = state->transitions[trans].symbol_id;
            LRState *target = state->transitions[trans].state;

            /* See if this transition is on EOF */
            for (int term = 0; term < grammar->terminals_count; term++) {
              if (grammar->terminal_indices[term] == symbol_id &&
                  term == eof_idx) {
                action_table_set_action(common->table, state_idx, eof_idx,
                                        ACTION_SHIFT, target->id);
                DEBUG_PRINT("Set SHIFT action for state %d on EOF to state %d",
                            state_idx, target->id);
                break;
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
