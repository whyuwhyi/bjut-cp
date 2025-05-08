/**
 * @file grammar.c
 * @brief Grammar implementation
 */

#include "parser/grammar.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new grammar
 */
Grammar *grammar_create(void) {
  Grammar *grammar = (Grammar *)safe_malloc(sizeof(Grammar));
  if (!grammar) {
    return NULL;
  }

  /* Initialize grammar */
  memset(grammar, 0, sizeof(Grammar));

  DEBUG_PRINT("Created new grammar");
  return grammar;
}

/**
 * @brief Destroy grammar and free resources
 */
void grammar_destroy(Grammar *grammar) {
  if (!grammar) {
    return;
  }

  /* Free symbols */
  if (grammar->symbols) {
    for (int i = 0; i < grammar->symbols_count; i++) {
      if (grammar->symbols[i].name) {
        free(grammar->symbols[i].name);
      }
    }
    free(grammar->symbols);
  }

  /* Free productions */
  if (grammar->productions) {
    for (int i = 0; i < grammar->productions_count; i++) {
      if (grammar->productions[i].rhs) {
        free(grammar->productions[i].rhs);
      }
      if (grammar->productions[i].display_str) {
        free(grammar->productions[i].display_str);
      }
    }
    free(grammar->productions);
  }

  /* Free symbol lookup tables */
  if (grammar->nonterminal_indices) {
    free(grammar->nonterminal_indices);
  }
  if (grammar->terminal_indices) {
    free(grammar->terminal_indices);
  }

  /* Free FIRST and FOLLOW sets */
  if (grammar->first_sets) {
    for (int i = 0; i < grammar->nonterminals_count; i++) {
      if (grammar->first_sets[i]) {
        free(grammar->first_sets[i]);
      }
    }
    free(grammar->first_sets);
  }
  if (grammar->follow_sets) {
    for (int i = 0; i < grammar->nonterminals_count; i++) {
      if (grammar->follow_sets[i]) {
        free(grammar->follow_sets[i]);
      }
    }
    free(grammar->follow_sets);
  }

  free(grammar);
  DEBUG_PRINT("Destroyed grammar");
}

/**
 * @brief Add a new non-terminal symbol to the grammar
 */
int grammar_add_nonterminal(Grammar *grammar, const char *name) {
  if (!grammar || !name) {
    return -1;
  }

  /* Resize symbols array */
  int new_index = grammar->symbols_count;
  grammar->symbols = (Symbol *)safe_realloc(grammar->symbols,
                                            (new_index + 1) * sizeof(Symbol));
  if (!grammar->symbols) {
    return -1;
  }

  /* Initialize new symbol */
  grammar->symbols[new_index].type = SYMBOL_NONTERMINAL;
  grammar->symbols[new_index].nonterminal = grammar->nonterminals_count;
  grammar->symbols[new_index].name = safe_strdup(name);
  if (!grammar->symbols[new_index].name) {
    return -1;
  }

  /* Update grammar */
  grammar->symbols_count++;
  grammar->nonterminals_count++;

  /* Resize non-terminal indices array */
  grammar->nonterminal_indices = (int *)safe_realloc(
      grammar->nonterminal_indices, grammar->nonterminals_count * sizeof(int));
  if (!grammar->nonterminal_indices) {
    return -1;
  }

  /* Update non-terminal indices */
  grammar->nonterminal_indices[grammar->nonterminals_count - 1] = new_index;

  DEBUG_PRINT("Added non-terminal: %s (ID: %d)", name,
              grammar->nonterminals_count - 1);
  return grammar->nonterminals_count - 1;
}

/**
 * @brief Add a new terminal symbol to the grammar
 */
int grammar_add_terminal(Grammar *grammar, TokenType token, const char *name) {
  if (!grammar || !name) {
    return -1;
  }

  /* Resize symbols array */
  int new_index = grammar->symbols_count;
  grammar->symbols = (Symbol *)safe_realloc(grammar->symbols,
                                            (new_index + 1) * sizeof(Symbol));
  if (!grammar->symbols) {
    return -1;
  }

  /* Initialize new symbol */
  grammar->symbols[new_index].type = SYMBOL_TERMINAL;
  grammar->symbols[new_index].token = token;
  grammar->symbols[new_index].name = safe_strdup(name);
  if (!grammar->symbols[new_index].name) {
    return -1;
  }

  /* Update grammar */
  grammar->symbols_count++;
  grammar->terminals_count++;

  /* Resize terminal indices array */
  grammar->terminal_indices = (int *)safe_realloc(
      grammar->terminal_indices, grammar->terminals_count * sizeof(int));
  if (!grammar->terminal_indices) {
    return -1;
  }

  /* Update terminal indices */
  grammar->terminal_indices[grammar->terminals_count - 1] = new_index;

  DEBUG_PRINT("Added terminal: %s (Token: %d)", name, token);
  return grammar->terminals_count - 1;
}

/**
 * @brief Add a new production to the grammar
 */
int grammar_add_production(Grammar *grammar, int lhs, Symbol *rhs,
                           int rhs_length) {
  if (!grammar || lhs < 0 || lhs >= grammar->nonterminals_count ||
      (rhs_length > 0 && !rhs)) {
    return -1;
  }

  /* Resize productions array */
  int new_index = grammar->productions_count;
  grammar->productions = (Production *)safe_realloc(
      grammar->productions, (new_index + 1) * sizeof(Production));
  if (!grammar->productions) {
    return -1;
  }

  /* Initialize new production */
  grammar->productions[new_index].id = new_index;
  grammar->productions[new_index].lhs = lhs;

  /* Copy right-hand side symbols */
  if (rhs_length > 0) {
    grammar->productions[new_index].rhs =
        (Symbol *)safe_malloc(rhs_length * sizeof(Symbol));
    if (!grammar->productions[new_index].rhs) {
      return -1;
    }
    memcpy(grammar->productions[new_index].rhs, rhs,
           rhs_length * sizeof(Symbol));
  } else {
    /* Epsilon production */
    grammar->productions[new_index].rhs = (Symbol *)safe_malloc(sizeof(Symbol));
    if (!grammar->productions[new_index].rhs) {
      return -1;
    }
    grammar->productions[new_index].rhs[0].type = SYMBOL_EPSILON;
    grammar->productions[new_index].rhs[0].name = safe_strdup("ε");
    if (!grammar->productions[new_index].rhs[0].name) {
      free(grammar->productions[new_index].rhs);
      return -1;
    }
    rhs_length = 1;
  }
  grammar->productions[new_index].rhs_length = rhs_length;

  /* Create display string */
  /* Format: LHS -> RHS1 RHS2 ... */
  int nt_index = grammar->nonterminal_indices[lhs];
  size_t str_len = strlen(grammar->symbols[nt_index].name) + 5; /* "LHS -> " */

  for (int i = 0; i < rhs_length; i++) {
    if (grammar->productions[new_index].rhs[i].type == SYMBOL_EPSILON) {
      str_len += 2; /* "ε " */
    } else if (grammar->productions[new_index].rhs[i].type == SYMBOL_TERMINAL) {
      int term_idx = -1;
      TokenType token = grammar->productions[new_index].rhs[i].token;

      /* Find terminal symbol index */
      for (int j = 0; j < grammar->terminals_count; j++) {
        if (grammar->symbols[grammar->terminal_indices[j]].token == token) {
          term_idx = j;
          break;
        }
      }

      if (term_idx >= 0) {
        str_len +=
            strlen(grammar->symbols[grammar->terminal_indices[term_idx]].name) +
            1;
      } else {
        str_len += 10; /* "<unknown> " */
      }
    } else if (grammar->productions[new_index].rhs[i].type ==
               SYMBOL_NONTERMINAL) {
      int nt_id = grammar->productions[new_index].rhs[i].nonterminal;
      if (nt_id >= 0 && nt_id < grammar->nonterminals_count) {
        str_len +=
            strlen(grammar->symbols[grammar->nonterminal_indices[nt_id]].name) +
            1;
      } else {
        str_len += 10; /* "<unknown> " */
      }
    }
  }

  grammar->productions[new_index].display_str = (char *)safe_malloc(str_len);
  if (!grammar->productions[new_index].display_str) {
    free(grammar->productions[new_index].rhs);
    return -1;
  }

  /* Create display string */
  strcpy(grammar->productions[new_index].display_str,
         grammar->symbols[nt_index].name);
  strcat(grammar->productions[new_index].display_str, " -> ");

  for (int i = 0; i < rhs_length; i++) {
    if (grammar->productions[new_index].rhs[i].type == SYMBOL_EPSILON) {
      strcat(grammar->productions[new_index].display_str, "ε");
    } else if (grammar->productions[new_index].rhs[i].type == SYMBOL_TERMINAL) {
      int term_idx = -1;
      TokenType token = grammar->productions[new_index].rhs[i].token;

      /* Find terminal symbol index */
      for (int j = 0; j < grammar->terminals_count; j++) {
        if (grammar->symbols[grammar->terminal_indices[j]].token == token) {
          term_idx = j;
          break;
        }
      }

      if (term_idx >= 0) {
        strcat(grammar->productions[new_index].display_str,
               grammar->symbols[grammar->terminal_indices[term_idx]].name);
      } else {
        strcat(grammar->productions[new_index].display_str, "<unknown>");
      }
    } else if (grammar->productions[new_index].rhs[i].type ==
               SYMBOL_NONTERMINAL) {
      int nt_id = grammar->productions[new_index].rhs[i].nonterminal;
      if (nt_id >= 0 && nt_id < grammar->nonterminals_count) {
        strcat(grammar->productions[new_index].display_str,
               grammar->symbols[grammar->nonterminal_indices[nt_id]].name);
      } else {
        strcat(grammar->productions[new_index].display_str, "<unknown>");
      }
    }

    if (i < rhs_length - 1) {
      strcat(grammar->productions[new_index].display_str, " ");
    }
  }

  /* Update grammar */
  grammar->productions_count++;

  DEBUG_PRINT("Added production: %s (ID: %d)",
              grammar->productions[new_index].display_str, new_index);
  return new_index;
}

/**
 * @brief Set the start symbol of the grammar
 */
void grammar_set_start_symbol(Grammar *grammar, int start_symbol) {
  if (!grammar || start_symbol < 0 ||
      start_symbol >= grammar->nonterminals_count) {
    return;
  }

  grammar->start_symbol = start_symbol;
  DEBUG_PRINT("Set start symbol to: %s (ID: %d)",
              grammar->symbols[grammar->nonterminal_indices[start_symbol]].name,
              start_symbol);
}

/**
 * @brief Helper function for computing FIRST sets
 */
static bool compute_first_set(Grammar *grammar, int nonterminal) {
  if (!grammar || nonterminal < 0 ||
      nonterminal >= grammar->nonterminals_count) {
    return false;
  }

  bool changed = false;

  /* For each production with this non-terminal on the left-hand side */
  for (int i = 0; i < grammar->productions_count; i++) {
    if (grammar->productions[i].lhs != nonterminal) {
      continue;
    }

    /* Process the right-hand side */
    Symbol *rhs = grammar->productions[i].rhs;
    int rhs_length = grammar->productions[i].rhs_length;

    /* If epsilon production, add epsilon to FIRST set */
    if (rhs_length == 1 && rhs[0].type == SYMBOL_EPSILON) {
      if (!grammar->first_sets[nonterminal][grammar->terminals_count]) {
        grammar->first_sets[nonterminal][grammar->terminals_count] = true;
        changed = true;
        DEBUG_PRINT(
            "Added epsilon to FIRST set of %s",
            grammar->symbols[grammar->nonterminal_indices[nonterminal]].name);
      }
      continue;
    }

    /* Process each symbol in the right-hand side */
    bool can_derive_epsilon = true;
    for (int j = 0; j < rhs_length && can_derive_epsilon; j++) {
      Symbol *symbol = &rhs[j];
      can_derive_epsilon = false;

      if (symbol->type == SYMBOL_TERMINAL) {
        /* Terminal symbol - add to FIRST set */
        if (!grammar->first_sets[nonterminal][symbol->token]) {
          grammar->first_sets[nonterminal][symbol->token] = true;
          changed = true;
          DEBUG_PRINT(
              "Added terminal %s to FIRST set of %s",
              grammar->symbols[grammar->terminal_indices[symbol->token]].name,
              grammar->symbols[grammar->nonterminal_indices[nonterminal]].name);
        }
        break;
      } else if (symbol->type == SYMBOL_NONTERMINAL) {
        /* Non-terminal symbol - add its FIRST set to this non-terminal's FIRST
         * set */
        int other_nt = symbol->nonterminal;

        for (int k = 0; k < grammar->terminals_count; k++) {
          if (grammar->first_sets[other_nt][k] &&
              !grammar->first_sets[nonterminal][k]) {
            grammar->first_sets[nonterminal][k] = true;
            changed = true;
            DEBUG_PRINT(
                "Added terminal %s to FIRST set of %s (from %s)",
                grammar->symbols[grammar->terminal_indices[k]].name,
                grammar->symbols[grammar->nonterminal_indices[nonterminal]]
                    .name,
                grammar->symbols[grammar->nonterminal_indices[other_nt]].name);
          }
        }

        /* Check if this non-terminal can derive epsilon */
        if (grammar->first_sets[other_nt][grammar->terminals_count]) {
          can_derive_epsilon = true;
        }
      }
    }

    /* If all symbols in the right-hand side can derive epsilon, add epsilon to
     * FIRST set */
    if (can_derive_epsilon &&
        !grammar->first_sets[nonterminal][grammar->terminals_count]) {
      grammar->first_sets[nonterminal][grammar->terminals_count] = true;
      changed = true;
      DEBUG_PRINT(
          "Added epsilon to FIRST set of %s (all RHS can derive epsilon)",
          grammar->symbols[grammar->nonterminal_indices[nonterminal]].name);
    }
  }

  return changed;
}

/**
 * @brief Helper function for computing FOLLOW sets
 */
static bool compute_follow_set(Grammar *grammar) {
  if (!grammar) {
    return false;
  }

  bool changed = false;

  /* For each production */
  for (int i = 0; i < grammar->productions_count; i++) {
    int lhs = grammar->productions[i].lhs;
    Symbol *rhs = grammar->productions[i].rhs;
    int rhs_length = grammar->productions[i].rhs_length;

    /* Skip epsilon productions */
    if (rhs_length == 1 && rhs[0].type == SYMBOL_EPSILON) {
      continue;
    }

    /* Process each symbol in the right-hand side */
    for (int j = 0; j < rhs_length; j++) {
      if (rhs[j].type != SYMBOL_NONTERMINAL) {
        continue;
      }

      int nt = rhs[j].nonterminal;

      /* If this is the last symbol or all remaining symbols can derive epsilon,
         add FOLLOW(LHS) to FOLLOW(NT) */
      bool all_derive_epsilon = true;
      for (int k = j + 1; k < rhs_length; k++) {
        if (rhs[k].type == SYMBOL_TERMINAL ||
            (rhs[k].type == SYMBOL_NONTERMINAL &&
             !grammar
                  ->first_sets[rhs[k].nonterminal][grammar->terminals_count])) {
          all_derive_epsilon = false;
          break;
        }
      }

      if (j == rhs_length - 1 || all_derive_epsilon) {
        for (int k = 0; k < grammar->terminals_count; k++) {
          if (grammar->follow_sets[lhs][k] && !grammar->follow_sets[nt][k]) {
            grammar->follow_sets[nt][k] = true;
            changed = true;
            DEBUG_PRINT(
                "Added terminal %s to FOLLOW set of %s (from FOLLOW of %s)",
                grammar->symbols[grammar->terminal_indices[k]].name,
                grammar->symbols[grammar->nonterminal_indices[nt]].name,
                grammar->symbols[grammar->nonterminal_indices[lhs]].name);
          }
        }
      }

      /* Add FIRST of remaining symbols to FOLLOW(NT) */
      for (int k = j + 1; k < rhs_length; k++) {
        if (rhs[k].type == SYMBOL_TERMINAL) {
          if (!grammar->follow_sets[nt][rhs[k].token]) {
            grammar->follow_sets[nt][rhs[k].token] = true;
            changed = true;
            DEBUG_PRINT(
                "Added terminal %s to FOLLOW set of %s (from terminal in RHS)",
                grammar->symbols[grammar->terminal_indices[rhs[k].token]].name,
                grammar->symbols[grammar->nonterminal_indices[nt]].name);
          }
          break;
        } else if (rhs[k].type == SYMBOL_NONTERMINAL) {
          int next_nt = rhs[k].nonterminal;

          /* Add FIRST(next_nt) - {ε} to FOLLOW(nt) */
          for (int l = 0; l < grammar->terminals_count; l++) {
            if (grammar->first_sets[next_nt][l] &&
                !grammar->follow_sets[nt][l]) {
              grammar->follow_sets[nt][l] = true;
              changed = true;
              DEBUG_PRINT(
                  "Added terminal %s to FOLLOW set of %s (from FIRST of %s)",
                  grammar->symbols[grammar->terminal_indices[l]].name,
                  grammar->symbols[grammar->nonterminal_indices[nt]].name,
                  grammar->symbols[grammar->nonterminal_indices[next_nt]].name);
            }
          }

          /* If FIRST(next_nt) contains epsilon, continue with next symbol */
          if (!grammar->first_sets[next_nt][grammar->terminals_count]) {
            break;
          }
        }
      }
    }
  }

  return changed;
}

#define EXTRA_TERMINALS 2
// Why is here?
// Segmentation fault make me so confused.

/**
 * @brief Compute FIRST and FOLLOW sets for the grammar
 */
bool grammar_compute_first_follow_sets(Grammar *grammar) {
  if (!grammar) {
    return false;
  }

  /* Allocate FIRST sets */
  grammar->first_sets =
      (bool **)safe_malloc(grammar->nonterminals_count * sizeof(bool *));
  if (!grammar->first_sets) {
    return false;
  }

  for (int i = 0; i < grammar->nonterminals_count; i++) {
    /* +1 for epsilon */
    grammar->first_sets[i] = (bool *)safe_malloc(
        (grammar->terminals_count + EXTRA_TERMINALS) * sizeof(bool));
    if (!grammar->first_sets[i]) {
      for (int j = 0; j < i; j++) {
        free(grammar->first_sets[j]);
      }
      free(grammar->first_sets);
      grammar->first_sets = NULL;
      return false;
    }
    memset(grammar->first_sets[i], 0,
           (grammar->terminals_count + EXTRA_TERMINALS) * sizeof(bool));
  }

  /* Allocate FOLLOW sets */
  grammar->follow_sets =
      (bool **)safe_malloc(grammar->nonterminals_count * sizeof(bool *));
  if (!grammar->follow_sets) {
    for (int i = 0; i < grammar->nonterminals_count; i++) {
      free(grammar->first_sets[i]);
    }
    free(grammar->first_sets);
    grammar->first_sets = NULL;
    return false;
  }

  for (int i = 0; i < grammar->nonterminals_count; i++) {
    grammar->follow_sets[i] = (bool *)safe_malloc(
        (grammar->terminals_count + EXTRA_TERMINALS) * sizeof(bool));
    if (!grammar->follow_sets[i]) {
      for (int j = 0; j < i; j++) {
        free(grammar->follow_sets[j]);
      }
      free(grammar->follow_sets);
      grammar->follow_sets = NULL;
      for (int j = 0; j < grammar->nonterminals_count; j++) {
        free(grammar->first_sets[j]);
      }
      free(grammar->first_sets);
      grammar->first_sets = NULL;
      return false;
    }
    memset(grammar->follow_sets[i], 0,
           (grammar->terminals_count + EXTRA_TERMINALS) * sizeof(bool));
  }

  /* Add # to FOLLOW set of start symbol */
  int end_token_idx = -1;
  for (int i = 0; i < grammar->terminals_count; i++) {
    if (grammar->symbols[grammar->terminal_indices[i]].token == TK_SEMI) {
      end_token_idx = i;
      break;
    }
  }

  if (end_token_idx >= 0) {
    grammar->follow_sets[grammar->start_symbol][end_token_idx] = true;
    DEBUG_PRINT(
        "Added # to FOLLOW set of %s (start symbol)",
        grammar->symbols[grammar->nonterminal_indices[grammar->start_symbol]]
            .name);
  }

  /* Compute FIRST sets */
  bool first_changed;
  do {
    first_changed = false;
    for (int i = 0; i < grammar->nonterminals_count; i++) {
      if (compute_first_set(grammar, i)) {
        first_changed = true;
      }
    }
  } while (first_changed);

  /* Compute FOLLOW sets */
  bool follow_changed;
  do {
    follow_changed = compute_follow_set(grammar);
  } while (follow_changed);

  DEBUG_PRINT("Computed FIRST and FOLLOW sets for grammar");
  return true;
}

/**
 * @brief Initialize grammar with productions for this assignment
 */
bool grammar_init(Grammar *grammar) {
  if (!grammar) {
    return false;
  }

  /* Add non-terminal symbols */
  int p_nt = grammar_add_nonterminal(grammar, "P"); /* Program */
  int l_nt = grammar_add_nonterminal(grammar, "L"); /* Statement list */
  int s_nt = grammar_add_nonterminal(grammar, "S"); /* Statement */
  int n_nt = grammar_add_nonterminal(grammar, "N"); /* Statement Tail */
  int c_nt = grammar_add_nonterminal(grammar, "C"); /* Condition */
  int e_nt = grammar_add_nonterminal(grammar, "E"); /* Expression */
  int x_nt = grammar_add_nonterminal(grammar, "X"); /* Expression Tail */
  int r_nt = grammar_add_nonterminal(grammar, "R"); /* Term */
  int y_nt = grammar_add_nonterminal(grammar, "Y"); /* Term Tail */
  int f_nt = grammar_add_nonterminal(grammar, "F"); /* Factor */
  int t_nt = grammar_add_nonterminal(grammar, "T"); /* Program Tail */

  if (p_nt != NT_P || l_nt != NT_L || s_nt != NT_S || n_nt != NT_N ||
      c_nt != NT_C || e_nt != NT_E || x_nt != NT_X || r_nt != NT_R ||
      y_nt != NT_Y || f_nt != NT_F || t_nt != NT_T) {
    DEBUG_PRINT("Failed to add non-terminal symbols");
    return false;
  }

  /* Add terminal symbols */
  grammar_add_terminal(grammar, TK_IDN, "id");
  grammar_add_terminal(grammar, TK_DEC, "int10");
  grammar_add_terminal(grammar, TK_OCT, "int8");
  grammar_add_terminal(grammar, TK_HEX, "int16");
  grammar_add_terminal(grammar, TK_ADD, "+");
  grammar_add_terminal(grammar, TK_SUB, "-");
  grammar_add_terminal(grammar, TK_MUL, "*");
  grammar_add_terminal(grammar, TK_DIV, "/");
  grammar_add_terminal(grammar, TK_GT, ">");
  grammar_add_terminal(grammar, TK_LT, "<");
  grammar_add_terminal(grammar, TK_EQ, "=");
  grammar_add_terminal(grammar, TK_GE, ">=");
  grammar_add_terminal(grammar, TK_LE, "<=");
  grammar_add_terminal(grammar, TK_NEQ, "<>");
  grammar_add_terminal(grammar, TK_SLP, "(");
  grammar_add_terminal(grammar, TK_SRP, ")");
  grammar_add_terminal(grammar, TK_SEMI, ";");
  grammar_add_terminal(grammar, TK_IF, "if");
  grammar_add_terminal(grammar, TK_THEN, "then");
  grammar_add_terminal(grammar, TK_ELSE, "else");
  grammar_add_terminal(grammar, TK_WHILE, "while");
  grammar_add_terminal(grammar, TK_DO, "do");
  grammar_add_terminal(grammar, TK_BEGIN, "begin");
  grammar_add_terminal(grammar, TK_END, "end");

  /* Set start symbol */
  grammar_set_start_symbol(grammar, NT_P);

  /* Create temporary symbol for production rules */
  Symbol rhs[10]; /* Assume max 10 symbols in RHS */

  /* Add production rules */
  /* P → L T */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_L;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_T;
  grammar_add_production(grammar, NT_P, rhs, 2);

  /* T → P T */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_P;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_T;
  grammar_add_production(grammar, NT_T, rhs, 2);

  /* T → ε */
  grammar_add_production(grammar, NT_T, NULL, 0);

  /* L → S ; */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_S;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_SEMI;
  grammar_add_production(grammar, NT_L, rhs, 2);

  /* S → id = E */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_IDN;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_EQ;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_E;
  grammar_add_production(grammar, NT_S, rhs, 3);

  /* S → if C then S N */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_IF;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_C;
  rhs[2].type = SYMBOL_TERMINAL;
  rhs[2].token = TK_THEN;
  rhs[3].type = SYMBOL_NONTERMINAL;
  rhs[3].nonterminal = NT_S;
  rhs[4].type = SYMBOL_NONTERMINAL;
  rhs[4].nonterminal = NT_N;
  grammar_add_production(grammar, NT_S, rhs, 5);

  /* S → while C do S */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_WHILE;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_C;
  rhs[2].type = SYMBOL_TERMINAL;
  rhs[2].token = TK_DO;
  rhs[3].type = SYMBOL_NONTERMINAL;
  rhs[3].nonterminal = NT_S;
  grammar_add_production(grammar, NT_S, rhs, 4);

  /* S → begin L end */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_BEGIN;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_L;
  rhs[2].type = SYMBOL_TERMINAL;
  rhs[2].token = TK_END;
  grammar_add_production(grammar, NT_S, rhs, 3);

  /* N → else S */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_ELSE;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_S;
  grammar_add_production(grammar, NT_N, rhs, 2);

  /* N → ε */
  grammar_add_production(grammar, NT_N, NULL, 0);

  /* C → E > E */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_E;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_GT;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_E;
  grammar_add_production(grammar, NT_C, rhs, 3);

  /* C → E < E */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_E;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_LT;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_E;
  grammar_add_production(grammar, NT_C, rhs, 3);

  /* C → E = E */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_E;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_EQ;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_E;
  grammar_add_production(grammar, NT_C, rhs, 3);

  /* C → E >= E */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_E;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_GE;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_E;
  grammar_add_production(grammar, NT_C, rhs, 3);

  /* C → E <= E */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_E;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_LE;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_E;
  grammar_add_production(grammar, NT_C, rhs, 3);

  /* C → E <> E */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_E;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_NEQ;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_E;
  grammar_add_production(grammar, NT_C, rhs, 3);

  /* C → ( C ) */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_SLP;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_C;
  rhs[2].type = SYMBOL_TERMINAL;
  rhs[2].token = TK_SRP;
  grammar_add_production(grammar, NT_C, rhs, 3);

  /* E → R X */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_R;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_X;
  grammar_add_production(grammar, NT_E, rhs, 2);

  /* X → + R X */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_ADD;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_R;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_X;
  grammar_add_production(grammar, NT_X, rhs, 3);

  /* X → - R X */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_SUB;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_R;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_X;
  grammar_add_production(grammar, NT_X, rhs, 3);

  /* X → ε */
  grammar_add_production(grammar, NT_X, NULL, 0);

  /* R → F Y */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_F;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_Y;
  grammar_add_production(grammar, NT_R, rhs, 2);

  /* Y → * F Y */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_MUL;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_F;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_Y;
  grammar_add_production(grammar, NT_Y, rhs, 3);

  /* Y → / F Y */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_DIV;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_F;
  rhs[2].type = SYMBOL_NONTERMINAL;
  rhs[2].nonterminal = NT_Y;
  grammar_add_production(grammar, NT_Y, rhs, 3);

  /* Y → ε */
  grammar_add_production(grammar, NT_Y, NULL, 0);

  /* F → ( E ) */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_SLP;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_E;
  rhs[2].type = SYMBOL_TERMINAL;
  rhs[2].token = TK_SRP;
  grammar_add_production(grammar, NT_F, rhs, 3);

  /* F → id */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_IDN;
  grammar_add_production(grammar, NT_F, rhs, 1);

  /* F → int8 */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_OCT;
  grammar_add_production(grammar, NT_F, rhs, 1);

  /* F → int10 */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_DEC;
  grammar_add_production(grammar, NT_F, rhs, 1);

  /* F → int16 */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_HEX;
  grammar_add_production(grammar, NT_F, rhs, 1);

  DEBUG_PRINT("Initialized grammar with %d productions",
              grammar->productions_count);
  return true;
}

/**
 * @brief Get the name of a symbol
 */
const char *grammar_get_symbol_name(Grammar *grammar, int symbol_id) {
  if (!grammar || symbol_id < 0 || symbol_id >= grammar->symbols_count) {
    return "<unknown>";
  }

  return grammar->symbols[symbol_id].name;
}

/**
 * @brief Get a string representation of a production
 */
const char *grammar_get_production_str(Grammar *grammar, int production_id) {
  if (!grammar || production_id < 0 ||
      production_id >= grammar->productions_count) {
    return "<unknown production>";
  }

  return grammar->productions[production_id].display_str;
}

/**
 * @brief Print all productions in the grammar
 */
void grammar_print_productions(Grammar *grammar) {
  if (!grammar) {
    return;
  }

  printf("Grammar Productions:\n");
  for (int i = 0; i < grammar->productions_count; i++) {
    printf("  %d: %s\n", i, grammar->productions[i].display_str);
  }
}

/**
 * @brief Check if a terminal is in the FIRST set of a non-terminal
 */
bool grammar_is_in_first(Grammar *grammar, int nonterminal,
                         TokenType terminal) {
  if (!grammar || nonterminal < 0 ||
      nonterminal >= grammar->nonterminals_count) {
    return false;
  }

  /* Find terminal index */
  int term_idx = -1;
  for (int i = 0; i < grammar->terminals_count; i++) {
    if (grammar->symbols[grammar->terminal_indices[i]].token == terminal) {
      term_idx = i;
      break;
    }
  }

  if (term_idx < 0) {
    return false;
  }

  return grammar->first_sets[nonterminal][term_idx];
}

/**
 * @brief Check if a terminal is in the FOLLOW set of a non-terminal
 */
bool grammar_is_in_follow(Grammar *grammar, int nonterminal,
                          TokenType terminal) {
  if (!grammar || nonterminal < 0 ||
      nonterminal >= grammar->nonterminals_count) {
    return false;
  }

  /* Find terminal index */
  int term_idx = -1;
  for (int i = 0; i < grammar->terminals_count; i++) {
    if (grammar->symbols[grammar->terminal_indices[i]].token == terminal) {
      term_idx = i;
      break;
    }
  }

  if (term_idx < 0) {
    return false;
  }

  return grammar->follow_sets[nonterminal][term_idx];
}

/**
 * @brief Check if epsilon is in the FIRST set of a non-terminal
 */
bool grammar_has_epsilon_in_first(Grammar *grammar, int nonterminal) {
  if (!grammar || nonterminal < 0 ||
      nonterminal >= grammar->nonterminals_count) {
    return false;
  }

  return grammar->first_sets[nonterminal][grammar->terminals_count];
}

/**
 * @brief Find a production by its left-hand side and right-hand side
 */
int grammar_find_production(Grammar *grammar, int lhs, Symbol *rhs,
                            int rhs_length) {
  if (!grammar || lhs < 0 || lhs >= grammar->nonterminals_count ||
      (rhs_length > 0 && !rhs)) {
    return -1;
  }

  for (int i = 0; i < grammar->productions_count; i++) {
    if (grammar->productions[i].lhs != lhs ||
        grammar->productions[i].rhs_length != rhs_length) {
      continue;
    }

    bool match = true;
    for (int j = 0; j < rhs_length; j++) {
      if (grammar->productions[i].rhs[j].type != rhs[j].type) {
        match = false;
        break;
      }

      if (rhs[j].type == SYMBOL_TERMINAL &&
          grammar->productions[i].rhs[j].token != rhs[j].token) {
        match = false;
        break;
      } else if (rhs[j].type == SYMBOL_NONTERMINAL &&
                 grammar->productions[i].rhs[j].nonterminal !=
                     rhs[j].nonterminal) {
        match = false;
        break;
      }
    }

    if (match) {
      return i;
    }
  }

  return -1;
}
