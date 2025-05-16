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
 * @brief Calculate FIRST sets for all non-terminals
 */
static void compute_first_sets(Grammar *g) {
  if (!g)
    return;

  // Calculate FIRST sets until no changes are made
  bool first_changed;
  do {
    first_changed = false;

    // Process each production
    for (int pid = 0; pid < g->productions_count; pid++) {
      Production *p = &g->productions[pid];
      int A = p->lhs;
      Symbol *rhs = p->rhs;
      int L = p->rhs_length;

      // Empty production: A -> ε
      if (L == 0 || (L == 1 && rhs[0].type == SYMBOL_EPSILON)) {
        if (!g->first_sets[A][g->terminals_count]) { // Last position is for
                                                     // epsilon
          g->first_sets[A][g->terminals_count] = true;
          first_changed = true;
          DEBUG_PRINT("FIRST(%s) += ε",
                      g->symbols[g->nonterminal_indices[A]].name);
        }
        continue;
      }

      // Non-empty production
      bool all_nullable = true;

      // Process right-hand side symbols until finding a non-nullable symbol or
      // terminal
      for (int i = 0; i < L; i++) {
        if (rhs[i].type == SYMBOL_TERMINAL) {
          // If terminal, add to FIRST(A)
          int term_idx = -1;
          for (int j = 0; j < g->terminals_count; j++) {
            if (g->symbols[g->terminal_indices[j]].token == rhs[i].token) {
              term_idx = j;
              break;
            }
          }

          if (term_idx >= 0 && !g->first_sets[A][term_idx]) {
            g->first_sets[A][term_idx] = true;
            first_changed = true;
            DEBUG_PRINT("FIRST(%s) += %s (from terminal)",
                        g->symbols[g->nonterminal_indices[A]].name,
                        g->symbols[g->terminal_indices[term_idx]].name);
          }

          all_nullable = false;
          break; // Terminals block further consideration
        } else if (rhs[i].type == SYMBOL_NONTERMINAL) {
          int B = rhs[i].nonterminal;

          // Add FIRST(B) - {ε} to FIRST(A)
          for (int t = 0; t < g->terminals_count; t++) {
            if (g->first_sets[B][t] && !g->first_sets[A][t]) {
              g->first_sets[A][t] = true;
              first_changed = true;
              DEBUG_PRINT("FIRST(%s) += %s (from FIRST(%s))",
                          g->symbols[g->nonterminal_indices[A]].name,
                          g->symbols[g->terminal_indices[t]].name,
                          g->symbols[g->nonterminal_indices[B]].name);
            }
          }

          // If B cannot derive ε, stop processing
          if (!g->first_sets[B][g->terminals_count]) {
            all_nullable = false;
            break;
          }
          // Otherwise, continue with next symbol
        }
      }

      // If all symbols can derive ε, add ε to FIRST(A)
      if (all_nullable && !g->first_sets[A][g->terminals_count]) {
        g->first_sets[A][g->terminals_count] = true;
        first_changed = true;
        DEBUG_PRINT("FIRST(%s) += ε (all RHS nullable)",
                    g->symbols[g->nonterminal_indices[A]].name);
      }
    }
  } while (first_changed);
}

/**
 * @brief Calculate FOLLOW sets for all non-terminals
 */
static void compute_follow_sets(Grammar *g) {
  if (!g)
    return;
  int T = g->terminals_count;
  // First, add EOF to FOLLOW set of start symbol
  int eof_idx = -1;
  for (int i = 0; i < T; i++) {
    if (g->symbols[g->terminal_indices[i]].token == TK_EOF) {
      eof_idx = i;
      break;
    }
  }
  if (eof_idx >= 0) {
    g->follow_sets[g->start_symbol][eof_idx] = true;
  }
  // Repeatedly calculate FOLLOW sets until no more changes
  bool changed;
  do {
    changed = false;
    // Process all productions
    for (int p = 0; p < g->productions_count; p++) {
      int A = g->productions[p].lhs;
      Symbol *rhs = g->productions[p].rhs;
      int rhs_len = g->productions[p].rhs_length;
      // Skip epsilon productions
      if (rhs_len == 0 || (rhs_len == 1 && rhs[0].type == SYMBOL_EPSILON)) {
        continue;
      }
      // Process right-hand side
      for (int i = 0; i < rhs_len; i++) {
        // Only process non-terminals
        if (rhs[i].type != SYMBOL_NONTERMINAL) {
          continue;
        }
        int B = rhs[i].nonterminal;
        // Handle symbols after B
        if (i == rhs_len - 1) {
          // B is the last symbol, add FOLLOW(A) to FOLLOW(B)
          for (int t = 0; t < T; t++) {
            if (g->follow_sets[A][t] && !g->follow_sets[B][t]) {
              g->follow_sets[B][t] = true;
              changed = true;
            }
          }
        } else {
          // Check next symbol
          if (rhs[i + 1].type == SYMBOL_TERMINAL) {
            // Next is terminal, directly add to FOLLOW(B)
            int term_idx = -1;
            for (int j = 0; j < T; j++) {
              if (g->symbols[g->terminal_indices[j]].token ==
                  rhs[i + 1].token) {
                term_idx = j;
                break;
              }
            }
            if (term_idx >= 0 && !g->follow_sets[B][term_idx]) {
              g->follow_sets[B][term_idx] = true;
              changed = true;
            }
          } else if (rhs[i + 1].type == SYMBOL_NONTERMINAL) {
            int C = rhs[i + 1].nonterminal;
            // Add FIRST(C) - {ε} to FOLLOW(B)
            for (int t = 0; t < T; t++) {
              if (g->first_sets[C][t] && !g->follow_sets[B][t]) {
                g->follow_sets[B][t] = true;
                changed = true;
              }
            }
            // If C can derive ε, consider subsequent symbols or FOLLOW(A)
            if (g->first_sets[C][T]) {
              bool all_nullable = true;
              // Check symbols from i+2 to rhs_len-1
              for (int j = i + 2; j < rhs_len; j++) {
                if (rhs[j].type == SYMBOL_TERMINAL) {
                  // Terminal
                  int term_idx = -1;
                  for (int k = 0; k < T; k++) {
                    if (g->symbols[g->terminal_indices[k]].token ==
                        rhs[j].token) {
                      term_idx = k;
                      break;
                    }
                  }
                  if (term_idx >= 0 && !g->follow_sets[B][term_idx]) {
                    g->follow_sets[B][term_idx] = true;
                    changed = true;
                  }
                  all_nullable = false;
                  break;
                } else if (rhs[j].type == SYMBOL_NONTERMINAL) {
                  int D = rhs[j].nonterminal;
                  // Add FIRST(D) - {ε} to FOLLOW(B)
                  for (int t = 0; t < T; t++) {
                    if (g->first_sets[D][t] && !g->follow_sets[B][t]) {
                      g->follow_sets[B][t] = true;
                      changed = true;
                    }
                  }
                  // If D cannot derive ε, stop
                  if (!g->first_sets[D][T]) {
                    all_nullable = false;
                    break;
                  }
                }
              }
              // If all subsequent symbols can derive ε, add FOLLOW(A)
              if (all_nullable) {
                for (int t = 0; t < T; t++) {
                  if (g->follow_sets[A][t] && !g->follow_sets[B][t]) {
                    g->follow_sets[B][t] = true;
                    changed = true;
                  }
                }
              }
            }
          }
        }
      }
    }
  } while (changed);
}

/**
 * @brief Compute FIRST and FOLLOW sets for the grammar
 */
bool grammar_compute_first_follow_sets(Grammar *g) {
  if (!g)
    return false;
  // Free old sets if they exist
  if (g->first_sets) {
    for (int i = 0; i < g->nonterminals_count; i++) {
      if (g->first_sets[i])
        free(g->first_sets[i]);
    }
    free(g->first_sets);
  }
  if (g->follow_sets) {
    for (int i = 0; i < g->nonterminals_count; i++) {
      if (g->follow_sets[i])
        free(g->follow_sets[i]);
    }
    free(g->follow_sets);
  }
  // Allocate new set space
  g->first_sets = malloc(g->nonterminals_count * sizeof(bool *));
  g->follow_sets = malloc(g->nonterminals_count * sizeof(bool *));
  if (!g->first_sets || !g->follow_sets)
    return false;
  // Initialize FIRST and FOLLOW sets for each non-terminal
  for (int i = 0; i < g->nonterminals_count; i++) {
    g->first_sets[i] =
        calloc(g->terminals_count + 1, sizeof(bool)); // +1 for epsilon
    g->follow_sets[i] =
        calloc(g->terminals_count, sizeof(bool)); // No epsilon in FOLLOW
    if (!g->first_sets[i] || !g->follow_sets[i])
      return false;
  }

  // Calculate FIRST sets
  compute_first_sets(g);

  // Calculate FOLLOW sets
  compute_follow_sets(g);

  // Print calculation results
  grammar_print_first_sets(g);
  grammar_print_follow_sets(g);

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
  int o_nt = grammar_add_nonterminal(grammar,
                                     "O"); /* Operator followed by expression */
  if (p_nt != NT_P || l_nt != NT_L || s_nt != NT_S || n_nt != NT_N ||
      c_nt != NT_C || e_nt != NT_E || x_nt != NT_X || r_nt != NT_R ||
      y_nt != NT_Y || f_nt != NT_F || t_nt != NT_T || o_nt != NT_O) {
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
  grammar_add_terminal(grammar, TK_EOF, "#");
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

  /* C → E O */
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_E;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_O;
  grammar_add_production(grammar, NT_C, rhs, 2);

  /* C → ( C ) */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_SLP;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_C;
  rhs[2].type = SYMBOL_TERMINAL;
  rhs[2].token = TK_SRP;
  grammar_add_production(grammar, NT_C, rhs, 3);

  /* O → > E */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_GT;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_E;
  grammar_add_production(grammar, NT_O, rhs, 2);

  /* O → < E */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_LT;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_E;
  grammar_add_production(grammar, NT_O, rhs, 2);

  /* O → = E */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_EQ;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_E;
  grammar_add_production(grammar, NT_O, rhs, 2);

  /* O → >= E */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_GE;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_E;
  grammar_add_production(grammar, NT_O, rhs, 2);

  /* O → <= E */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_LE;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_E;
  grammar_add_production(grammar, NT_O, rhs, 2);

  /* O → <> E */
  rhs[0].type = SYMBOL_TERMINAL;
  rhs[0].token = TK_NEQ;
  rhs[1].type = SYMBOL_NONTERMINAL;
  rhs[1].nonterminal = NT_E;
  grammar_add_production(grammar, NT_O, rhs, 2);

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
  int sprime = grammar_add_nonterminal(grammar, "S'");
  rhs[0].type = SYMBOL_NONTERMINAL;
  rhs[0].nonterminal = NT_P;
  rhs[1].type = SYMBOL_TERMINAL;
  rhs[1].token = TK_EOF;
  grammar_add_production(grammar, sprime, rhs, 2);
  /* Set start symbol */
  grammar_set_start_symbol(grammar, sprime);
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

/**
 * @brief Print FIRST sets for all non-terminals
 */
void grammar_print_first_sets(Grammar *grammar) {
  if (!grammar || !grammar->first_sets) {
    printf("FIRST sets not computed.\n");
    return;
  }
  printf("\nFIRST Sets:\n");
  for (int A = 0; A < grammar->nonterminals_count; A++) {
    int nt_idx = grammar->nonterminal_indices[A];
    printf("FIRST(%s) = { ", grammar->symbols[nt_idx].name);
    bool empty = true;
    // Print terminals
    for (int t = 0; t < grammar->terminals_count; t++) {
      if (grammar->first_sets[A][t]) {
        int term_idx = grammar->terminal_indices[t];
        printf("%s ", grammar->symbols[term_idx].name);
        empty = false;
      }
    }
    // Check for epsilon
    if (grammar->first_sets[A][grammar->terminals_count]) {
      printf("ε ");
      empty = false;
    }
    if (empty) {
      printf("∅");
    }
    printf("}\n");
  }
}

/**
 * @brief Print FOLLOW sets for all non-terminals
 */
void grammar_print_follow_sets(Grammar *grammar) {
  if (!grammar || !grammar->follow_sets) {
    printf("FOLLOW sets not computed.\n");
    return;
  }
  printf("\nFOLLOW Sets:\n");
  for (int A = 0; A < grammar->nonterminals_count; A++) {
    int nt_idx = grammar->nonterminal_indices[A];
    printf("FOLLOW(%s) = { ", grammar->symbols[nt_idx].name);
    bool empty = true;
    // Print terminals
    for (int t = 0; t < grammar->terminals_count; t++) {
      if (grammar->follow_sets[A][t]) {
        int term_idx = grammar->terminal_indices[t];
        printf("%s ", grammar->symbols[term_idx].name);
        empty = false;
      }
    }
    if (empty) {
      printf("∅");
    }
    printf("}\n");
  }
}
