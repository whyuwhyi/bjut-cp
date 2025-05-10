/**
 * @file lr1_parser.c
 * @brief LR(1) parser implementation
 */
#include "lr1_parser.h"
#include "../lr_parser.h"
#include "parser/grammar.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Build the LR(1) automaton
 */
bool lr1_build_automaton(Parser *parser, LR1ParserData *data) {
  if (!parser || !data) {
    return false;
  }

  LRParserData *common = &data->common;

  /* Create automaton */
  common->automaton = lr_automaton_create(parser->grammar);
  if (!common->automaton) {
    return false;
  }

  /* Build canonical collection of LR(1) items */
  if (!lr_create_canonical_collection(common->automaton, parser->grammar,
                                      true)) {
    return false;
  }

  DEBUG_PRINT("Built LR(1) automaton with %d states",
              common->automaton->state_count);

  return true;
}

/**
 * @brief Build the LR(1) parsing table
 */
bool lr1_build_parsing_table(Parser *parser, LR1ParserData *data) {
  if (!parser || !data)
    return false;

  LRParserData *common = &data->common;
  LRAutomaton *automaton = common->automaton;
  Grammar *g = parser->grammar;

  /* Create Action/Goto table */
  common->table = action_table_create(
      automaton->state_count, g->terminals_count, g->nonterminals_count);
  if (!common->table)
    return false;

  /* Find EOF index in terminals */
  int eof_idx = -1;
  for (int t = 0; t < g->terminals_count; t++) {
    if (g->symbols[g->terminal_indices[t]].token == TK_EOF) {
      eof_idx = t;
      break;
    }
  }
  if (eof_idx < 0)
    return false;

  /* Fill the table for each state */
  for (int st = 0; st < automaton->state_count; st++) {
    LRState *state = automaton->states[st];

    /* 1) First handle all reduction/accept items (dot at the end) */
    for (int i = 0; i < state->item_count; i++) {
      LRItem *item = state->items[i];
      Production *p = &g->productions[item->production_id];

      /* Check if it's a reduction item */
      bool is_reduction = false;

      /* Check if dot is at the end of the RHS */
      if (item->dot_position == p->rhs_length) {
        is_reduction = true;
      }
      /* Check if it's an empty production (NULL, 0 form) */
      else if (p->rhs_length == 0) {
        is_reduction = true;
      }
      /* Check if it's a production containing ε symbol */
      else if (p->rhs_length == 1 && p->rhs[0].type == SYMBOL_EPSILON) {
        is_reduction = true;
      }

      /* If it's a reduction item, add appropriate reduction action */
      if (is_reduction) {
        bool is_augmented_start = (p->lhs == g->start_symbol);

        if (is_augmented_start) {
          /* S' -> P • #, lookahead=EOF => ACCEPT */
          /* or S' -> P # •, lookahead=any => ACCEPT */
          if (p->rhs_length == 2 && p->rhs[0].type == SYMBOL_NONTERMINAL &&
              p->rhs[1].type == SYMBOL_TERMINAL && p->rhs[1].token == TK_EOF) {

            /* If dot is at the end, set accept action */
            if (item->dot_position == 2) {
              action_table_set_action(common->table, st, eof_idx, ACTION_ACCEPT,
                                      0);
              DEBUG_PRINT("Added ACCEPT action for state %d on EOF (complete "
                          "S' -> P #)",
                          st);
            }
            /* If dot is after P, before EOF, set shift action for EOF */
            else if (item->dot_position == 1) {
              /* Find transition on EOF for this state, if exists */
              for (int tr = 0; tr < state->transition_count; tr++) {
                int sym = state->transitions[tr].symbol_id;
                LRState *to = state->transitions[tr].state;

                /* Check if it's EOF symbol */
                for (int t = 0; t < g->terminals_count; t++) {
                  if (g->terminal_indices[t] == sym && t == eof_idx) {
                    action_table_set_action(common->table, st, eof_idx,
                                            ACTION_SHIFT, to->id);
                    DEBUG_PRINT(
                        "Added SHIFT action for state %d on EOF to state %d",
                        st, to->id);
                    break;
                  }
                }
              }
            }
          }
        }

        /* For each lookahead of this item, add REDUCE */
        for (int la = 0; la < item->lookahead_count; la++) {
          int term = item->lookaheads[la];
          if (term < 0 || term >= g->terminals_count)
            continue;

          /* Skip if it's augmented start + EOF (already handled with ACCEPT) */
          if (is_augmented_start && term == eof_idx &&
              item->dot_position == p->rhs_length)
            continue;

          /* Add reduction action */
          action_table_set_action(common->table, st, term, ACTION_REDUCE,
                                  item->production_id);

          /* Log empty production reductions */
          if (p->rhs_length == 0 ||
              (p->rhs_length == 1 && p->rhs[0].type == SYMBOL_EPSILON)) {
            const char *term_name = g->symbols[g->terminal_indices[term]].name;
            const char *nt_name =
                g->symbols[g->nonterminal_indices[p->lhs]].name;
            DEBUG_PRINT("Added REDUCE action for %s by empty production %s -> "
                        "ε (id: %d)",
                        term_name, nt_name, item->production_id);
          }
        }
      }
    }

    /* 2) Handle all transitions - Shift and Goto */
    for (int tr = 0; tr < state->transition_count; tr++) {
      int sym = state->transitions[tr].symbol_id;
      LRState *to = state->transitions[tr].state;

      /* If it's a terminal, do Shift */
      for (int t = 0; t < g->terminals_count; t++) {
        if (g->terminal_indices[t] == sym) {
          /* Check if there's already a reduce action (handle shift-reduce
           * conflict) */
          Action existing = action_table_get_action(common->table, st, t);

          /* If there's already a reduce action, and we're adding a shift,
           * resolve conflict */
          if (existing.type == ACTION_REDUCE) {
            /* Get production info */
            Production *reduce_prod = &g->productions[existing.value];

            /* If reduction production is an empty production (like Y->ε),
             * prefer reduce */
            if (reduce_prod->rhs_length == 0 ||
                (reduce_prod->rhs_length == 1 &&
                 reduce_prod->rhs[0].type == SYMBOL_EPSILON)) {
              /* Keep reduce action, don't add shift */
              const char *term_name = g->symbols[g->terminal_indices[t]].name;
              DEBUG_PRINT("Resolved shift-reduce conflict in state %d for %s "
                          "in favor of reduce (empty production)",
                          st, term_name);
              continue;
            }
          }

          /* Add shift action */
          action_table_set_action(common->table, st, t, ACTION_SHIFT, to->id);

          /* Log the shift action */
          const char *term_name = g->symbols[g->terminal_indices[t]].name;
          DEBUG_PRINT("Added SHIFT action for state %d on %s to state %d", st,
                      term_name, to->id);
          break;
        }
      }

      /* If it's a non-terminal, do Goto */
      for (int nt = 0; nt < g->nonterminals_count; nt++) {
        if (g->nonterminal_indices[nt] == sym) {
          action_table_set_goto(common->table, st, nt, to->id);

          /* Log the goto action */
          const char *nt_name = g->symbols[g->nonterminal_indices[nt]].name;
          DEBUG_PRINT("Added GOTO action for state %d on %s to state %d", st,
                      nt_name, to->id);
          break;
        }
      }
    }
  }

  /* 3) Special handling: ensure correct handling of EOF */
  /* Find all states containing "S' -> P •" or "S' -> P • #" */
  for (int st = 0; st < automaton->state_count; st++) {
    LRState *state = automaton->states[st];

    for (int i = 0; i < state->item_count; i++) {
      LRItem *item = state->items[i];
      Production *p = &g->productions[item->production_id];

      /* Check if it's an augmented start production with dot after P */
      if (p->lhs == g->start_symbol && p->rhs_length >= 1 &&
          p->rhs[0].type == SYMBOL_NONTERMINAL &&
          p->rhs[0].nonterminal == NT_P) {

        /* If it's "S' -> P •" form (no explicit EOF) */
        if (p->rhs_length == 1 && item->dot_position == 1) {
          action_table_set_action(common->table, st, eof_idx, ACTION_ACCEPT, 0);
          DEBUG_PRINT("Added ACCEPT action for state %d on EOF (S' -> P •)",
                      st);
        }
        /* If it's "S' -> P • #" form (dot after P, before EOF) */
        else if (p->rhs_length == 2 && p->rhs[1].type == SYMBOL_TERMINAL &&
                 p->rhs[1].token == TK_EOF && item->dot_position == 1) {

          /* Look for transition on EOF */
          for (int tr = 0; tr < state->transition_count; tr++) {
            int sym = state->transitions[tr].symbol_id;
            LRState *to = state->transitions[tr].state;

            for (int t = 0; t < g->terminals_count; t++) {
              if (g->terminal_indices[t] == sym && t == eof_idx) {
                action_table_set_action(common->table, st, eof_idx,
                                        ACTION_SHIFT, to->id);
                DEBUG_PRINT(
                    "Added SHIFT action for state %d on EOF to state %d", st,
                    to->id);
                break;
              }
            }
          }
        }
        /* If it's "S' -> P # •" form (dot at the end) */
        else if (p->rhs_length == 2 && p->rhs[1].type == SYMBOL_TERMINAL &&
                 p->rhs[1].token == TK_EOF && item->dot_position == 2) {

          action_table_set_action(common->table, st, eof_idx, ACTION_ACCEPT, 0);
          DEBUG_PRINT("Added ACCEPT action for state %d on EOF (S' -> P # •)",
                      st);
        }
      }
    }
  }

  DEBUG_PRINT("LR(1) parsing table built successfully with %d states",
              automaton->state_count);
  return true;
}

/**
 * @brief Create an LR(1) parser
 */
Parser *lr1_parser_create(void) {
  Parser *parser = (Parser *)safe_malloc(sizeof(Parser));
  if (!parser) {
    return NULL;
  }

  /* Initialize parser */
  parser->type = PARSER_TYPE_LR1;
  parser->grammar = NULL;
  parser->production_tracker = NULL;
  parser->init = lr1_parser_init;
  parser->parse = lr1_parser_parse;
  parser->print_leftmost_derivation = lr1_parser_print_leftmost_derivation;
  parser->destroy = lr1_parser_destroy;

  /* Create parser-specific data */
  parser->data = safe_malloc(sizeof(LR1ParserData));
  if (!parser->data) {
    free(parser);
    return NULL;
  }
  memset(parser->data, 0, sizeof(LR1ParserData));

  DEBUG_PRINT("Created LR(1) parser");
  return parser;
}

/**
 * @brief Initialize the LR(1) parser
 */
bool lr1_parser_init(Parser *parser) {
  if (!parser || !parser->data) {
    return false;
  }

  LR1ParserData *data = (LR1ParserData *)parser->data;

  /* Initialize common LR parser data */
  if (!lr_parser_data_init(&data->common)) {
    return false;
  }

  /* Build LR(1) automaton */
  if (!lr1_build_automaton(parser, data)) {
    lr_parser_data_cleanup(&data->common);
    return false;
  }

  /* Build LR(1) parsing table */
  if (!lr1_build_parsing_table(parser, data)) {
    lr_parser_data_cleanup(&data->common);
    return false;
  }

  DEBUG_PRINT("Initialized LR(1) parser");
  return true;
}

/**
 * @brief Parse input using LR(1) parsing
 */
SyntaxTree *lr1_parser_parse(Parser *parser, Lexer *lexer) {
  if (!parser || !parser->data) {
    return NULL;
  }

  LR1ParserData *data = (LR1ParserData *)parser->data;

  /* Use common LR parsing algorithm */
  return lr_parser_parse(parser, &data->common, lexer);
}

/**
 * @brief Print the leftmost derivation from the LR(1) parser
 */
void lr1_parser_print_leftmost_derivation(Parser *parser) {
  if (!parser) {
    return;
  }

  /* Use common production tracker */
  production_tracker_print(parser->production_tracker, parser->grammar);
}

/**
 * @brief Destroy LR(1) parser and free resources
 */
void lr1_parser_destroy(Parser *parser) {
  if (!parser) {
    return;
  }

  if (parser->data) {
    LR1ParserData *data = (LR1ParserData *)parser->data;

    /* Clean up common LR parser data */
    lr_parser_data_cleanup(&data->common);
    free(parser->data);
  }

  free(parser);
  DEBUG_PRINT("Destroyed LR(1) parser");
}
