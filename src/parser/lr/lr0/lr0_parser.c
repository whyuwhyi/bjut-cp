/**
 * @file lr0_parser.c
 * @brief LR(0) parser implementation
 */

#include "lr0_parser.h"
#include "../lr_parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Build the LR(0) automaton
 */
bool lr0_build_automaton(Parser *parser, LR0ParserData *data) {
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
  if (!lr_create_canonical_collection(common->automaton, parser->grammar,
                                      false)) {
    return false;
  }

  DEBUG_PRINT("Built LR(0) automaton with %d states",
              common->automaton->state_count);
  return true;
}

/**
 * @brief Build the LR(0) parsing table
 */
bool lr0_build_parsing_table(Parser *parser, LR0ParserData *data) {
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

  /* Fill parsing table */
  for (int state_idx = 0; state_idx < automaton->state_count; state_idx++) {
    LRState *state = automaton->states[state_idx];

    /* Fill action table based on items */
    for (int item_idx = 0; item_idx < state->item_count; item_idx++) {
      LRItem *item = state->items[item_idx];

      /* Check for reduction */
      if (lr_item_is_reduction(item, grammar)) {
        /* Check if this item represents the augmented start production */
        int start_symbol = grammar->start_symbol;
        bool is_start_production =
            (grammar->productions[item->production_id].lhs == start_symbol);

        /* Determine if it's a completed start production (dot at end) */
        bool is_dot_at_end =
            (item->dot_position >=
             grammar->productions[item->production_id].rhs_length);

        if (is_start_production && is_dot_at_end) {
          /* This is an accept state - set accept action for EOF */
          int eof_idx = get_terminal_index(grammar, TK_EOF);
          if (eof_idx >= 0) {
            DEBUG_PRINT("Setting ACCEPT action in state %d for EOF", state_idx);
            action_table_set_action(common->table, state_idx, eof_idx,
                                    ACTION_ACCEPT, 0);
          }
        } else {
          /* Reduce by this production for all terminals */
          for (int term = 0; term < grammar->terminals_count; term++) {
            DEBUG_PRINT("Setting REDUCE action in state %d for terminal %d by "
                        "production %d",
                        state_idx, term, item->production_id);
            action_table_set_action(common->table, state_idx, term,
                                    ACTION_REDUCE, item->production_id);
          }
        }
      }
    }

    /* Fill action and goto tables based on transitions */
    for (int trans = 0; trans < state->transition_count; trans++) {
      int symbol_id = state->transitions[trans].symbol_id;
      LRState *target = state->transitions[trans].state;

      /* Check each symbol type */
      bool is_terminal = false;
      bool is_nonterminal = false;
      int term_idx = -1;
      int nt_idx = -1;

      /* Check if this is a terminal */
      for (int t = 0; t < grammar->terminals_count; t++) {
        if (grammar->terminal_indices[t] == symbol_id) {
          is_terminal = true;
          term_idx = t;
          break;
        }
      }

      /* Check if this is a non-terminal */
      for (int nt = 0; nt < grammar->nonterminals_count; nt++) {
        if (grammar->nonterminal_indices[nt] == symbol_id) {
          is_nonterminal = true;
          nt_idx = nt;
          break;
        }
      }

      if (is_terminal && term_idx >= 0) {
        /* Add shift action */
        DEBUG_PRINT(
            "Setting SHIFT action in state %d for terminal %d to state %d",
            state_idx, term_idx, target->id);
        action_table_set_action(common->table, state_idx, term_idx,
                                ACTION_SHIFT, target->id);
      }

      if (is_nonterminal && nt_idx >= 0) {
        /* Add goto action */
        DEBUG_PRINT(
            "Setting GOTO action in state %d for non-terminal %d to state %d",
            state_idx, nt_idx, target->id);
        action_table_set_goto(common->table, state_idx, nt_idx, target->id);
      }
    }
  }

  DEBUG_PRINT("Built LR(0) parsing table");
  return true;
}

/**
 * @brief Create an LR(0) parser
 */
Parser *lr0_parser_create(void) {
  Parser *parser = (Parser *)safe_malloc(sizeof(Parser));
  if (!parser) {
    return NULL;
  }

  /* Initialize parser */
  parser->type = PARSER_TYPE_LR0;
  parser->grammar = NULL;
  parser->production_tracker = NULL;
  parser->init = lr0_parser_init;
  parser->parse = lr0_parser_parse;
  parser->print_leftmost_derivation = lr0_parser_print_leftmost_derivation;
  parser->destroy = lr0_parser_destroy;

  /* Create parser-specific data */
  parser->data = safe_malloc(sizeof(LR0ParserData));
  if (!parser->data) {
    free(parser);
    return NULL;
  }
  memset(parser->data, 0, sizeof(LR0ParserData));

  DEBUG_PRINT("Created LR(0) parser");
  return parser;
}

/**
 * @brief Initialize the LR(0) parser
 */
bool lr0_parser_init(Parser *parser) {
  if (!parser || !parser->data) {
    return false;
  }

  LR0ParserData *data = (LR0ParserData *)parser->data;

  /* Initialize common LR parser data */
  if (!lr_parser_data_init(&data->common)) {
    return false;
  }

  /* Build LR(0) automaton */
  if (!lr0_build_automaton(parser, data)) {
    lr_parser_data_cleanup(&data->common);
    return false;
  }

  /* Build LR(0) parsing table */
  if (!lr0_build_parsing_table(parser, data)) {
    lr_parser_data_cleanup(&data->common);
    return false;
  }

  DEBUG_PRINT("Initialized LR(0) parser");
  return true;
}

/**
 * @brief Parse input using LR(0) parsing
 */
SyntaxTree *lr0_parser_parse(Parser *parser, Lexer *lexer) {
  if (!parser || !parser->data) {
    return NULL;
  }

  LR0ParserData *data = (LR0ParserData *)parser->data;

  /* Use common LR parsing algorithm */
  return lr_parser_parse(parser, &data->common, lexer);
}

/**
 * @brief Print the leftmost derivation from the LR(0) parser
 */
void lr0_parser_print_leftmost_derivation(Parser *parser) {
  if (!parser) {
    return;
  }

  /* Use common production tracker */
  production_tracker_print(parser->production_tracker, parser->grammar);
}

/**
 * @brief Destroy LR(0) parser and free resources
 */
void lr0_parser_destroy(Parser *parser) {
  if (!parser) {
    return;
  }

  if (parser->data) {
    LR0ParserData *data = (LR0ParserData *)parser->data;

    /* Clean up common LR parser data */
    lr_parser_data_cleanup(&data->common);

    free(parser->data);
  }

  free(parser);

  DEBUG_PRINT("Destroyed LR(0) parser");
}
