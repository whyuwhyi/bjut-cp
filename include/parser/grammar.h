/**
 * @file grammar.h
 * @brief Grammar representation for the parser
 */

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "common.h"
#include "lexer/token.h"
#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief Symbol type - terminal or non-terminal
 */
typedef enum {
  SYMBOL_TERMINAL,    /* Terminal symbol (token) */
  SYMBOL_NONTERMINAL, /* Non-terminal symbol */
  SYMBOL_EPSILON,     /* Empty production (ε) */
  SYMBOL_END          /* End of input (#) */
} SymbolType;

/**
 * @brief Symbol structure
 */
typedef struct {
  SymbolType type; /* Type of symbol */
  union {
    TokenType token; /* Terminal token type */
    int nonterminal; /* Non-terminal ID */
  };
  char *name; /* Symbol name (for display/debug) */
} Symbol;

/**
 * @brief Production rule
 */
typedef struct {
  int id;            /* Production ID */
  int lhs;           /* Left-hand side (non-terminal ID) */
  Symbol *rhs;       /* Right-hand side symbols */
  int rhs_length;    /* Number of symbols in right-hand side */
  char *display_str; /* String representation for display */
} Production;

/**
 * @brief Grammar structure
 */
typedef struct {
  Symbol *symbols;         /* Array of all symbols */
  int symbols_count;       /* Number of symbols */
  Production *productions; /* Array of all productions */
  int productions_count;   /* Number of productions */
  int start_symbol;        /* ID of the start symbol */

  /* Symbol lookup tables */
  int nonterminals_count; /* Number of non-terminals */
  int terminals_count;    /* Number of terminals */
  int *
      nonterminal_indices; /* Map from non-terminal ID to symbols array index */
  int *terminal_indices;   /* Map from terminal ID to symbols array index */

  /* First and Follow sets (for LL and SLR parsing) */
  bool **first_sets;  /* FIRST sets for non-terminals */
  bool **follow_sets; /* FOLLOW sets for non-terminals */
} Grammar;

/**
 * @brief Create a new grammar
 *
 * @return Grammar* Newly created grammar
 */
Grammar *grammar_create(void);

/**
 * @brief Destroy grammar and free resources
 *
 * @param grammar Grammar to destroy
 */
void grammar_destroy(Grammar *grammar);

/**
 * @brief Initialize grammar with productions for this assignment
 *
 * @param grammar Grammar to initialize
 * @return bool Success status
 */
bool grammar_init(Grammar *grammar);

/**
 * @brief Add a new non-terminal symbol to the grammar
 *
 * @param grammar Grammar to add to
 * @param name Symbol name
 * @return int Symbol ID
 */
int grammar_add_nonterminal(Grammar *grammar, const char *name);

/**
 * @brief Add a new terminal symbol to the grammar
 *
 * @param grammar Grammar to add to
 * @param token Terminal token type
 * @param name Symbol name
 * @return int Symbol ID
 */
int grammar_add_terminal(Grammar *grammar, TokenType token, const char *name);

/**
 * @brief Add a new production to the grammar
 *
 * @param grammar Grammar to add to
 * @param lhs Left-hand side non-terminal ID
 * @param rhs Array of right-hand side symbols
 * @param rhs_length Number of symbols in right-hand side
 * @return int Production ID
 */
int grammar_add_production(Grammar *grammar, int lhs, Symbol *rhs,
                           int rhs_length);

/**
 * @brief Set the start symbol of the grammar
 *
 * @param grammar Grammar to modify
 * @param start_symbol ID of the start symbol
 */
void grammar_set_start_symbol(Grammar *grammar, int start_symbol);

/**
 * @brief Compute FIRST and FOLLOW sets for the grammar
 *
 * @param grammar Grammar to compute sets for
 * @return bool Success status
 */
bool grammar_compute_first_follow_sets(Grammar *grammar);

/**
 * @brief Get the name of a symbol
 *
 * @param grammar Grammar containing the symbol
 * @param symbol_id ID of the symbol
 * @return const char* Symbol name
 */
const char *grammar_get_symbol_name(Grammar *grammar, int symbol_id);

/**
 * @brief Get a string representation of a production
 *
 * @param grammar Grammar containing the production
 * @param production_id ID of the production
 * @return const char* Production string
 */
const char *grammar_get_production_str(Grammar *grammar, int production_id);

/**
 * @brief Print all productions in the grammar
 *
 * @param grammar Grammar to print
 */
void grammar_print_productions(Grammar *grammar);

/**
 * @brief Check if a terminal is in the FIRST set of a non-terminal
 *
 * @param grammar Grammar containing the symbol
 * @param nonterminal Non-terminal ID
 * @param terminal Terminal token type
 * @return bool True if terminal is in FIRST set
 */
bool grammar_is_in_first(Grammar *grammar, int nonterminal, TokenType terminal);

/**
 * @brief Check if a terminal is in the FOLLOW set of a non-terminal
 *
 * @param grammar Grammar containing the symbol
 * @param nonterminal Non-terminal ID
 * @param terminal Terminal token type
 * @return bool True if terminal is in FOLLOW set
 */
bool grammar_is_in_follow(Grammar *grammar, int nonterminal,
                          TokenType terminal);

/**
 * @brief Check if epsilon is in the FIRST set of a non-terminal
 *
 * @param grammar Grammar containing the symbol
 * @param nonterminal Non-terminal ID
 * @return bool True if epsilon is in FIRST set
 */
bool grammar_has_epsilon_in_first(Grammar *grammar, int nonterminal);

/**
 * @brief Find a production by its left-hand side and right-hand side
 *
 * @param grammar Grammar to search
 * @param lhs Left-hand side non-terminal ID
 * @param rhs Right-hand side symbols
 * @param rhs_length Length of right-hand side symbols
 * @return int Production ID, or -1 if not found
 */
int grammar_find_production(Grammar *grammar, int lhs, Symbol *rhs,
                            int rhs_length);

/**
 * @brief Print the FIRST sets for all non-terminals in the grammar
 *
 * @param grammar Grammar to print FIRST sets for
 */
void grammar_print_first_sets(Grammar *grammar);

/**
 * @brief Print the FOLLOW sets for all non-terminals in the grammar
 *
 * @param grammar Grammar to print FOLLOW sets for
 */
void grammar_print_follow_sets(Grammar *grammar);

/**
 * @brief Define non-terminal symbol IDs for the grammar
 */
typedef enum {
  NT_P, /* Program */
  NT_L, /* Statement list */
  NT_S, /* Statement */
  NT_N, /* Statement Tail (for else part) */
  NT_C, /* Condition */
  NT_E, /* Expression */
  NT_X, /* Expression Tail */
  NT_R, /* Term */
  NT_Y, /* Term Tail */
  NT_F, /* Factor */
  NT_T, /* Program Tail */
  NT_O  /* Operator followed by expression */
} Nonterminal;

/**
 * @brief Production IDs from grammar
 */
// typedef enum {
//   PROD_P_LT = 0,
//   PROD_T_PT = 1,
//   PROD_T_EPSILON = 2,
//   PROD_L_S_SEMI = 3,
//   PROD_L_S_SEMI_L = 4,
//   PROD_L_EPSILON = 5,
//   PROD_S_ASSIGN = 6,
//   PROD_S_IF_C_THEN_S_N = 7,
//   PROD_S_WHILE_C_DO_S = 8,
//   PROD_S_BEGIN_L_END = 9,
//   PROD_N_ELSE_S = 10,
//   PROD_N_EPSILON = 11,
//   PROD_C_E_O = 12,
//   PROD_C_PAREN = 13,
//   PROD_O_GT = 14,
//   PROD_O_LT = 15,
//   PROD_O_EQ = 16,
//   PROD_O_GE = 17,
//   PROD_O_LE = 18,
//   PROD_O_NE = 19,
//   PROD_E_R_X = 20,
//   PROD_X_PLUS_R_X = 21,
//   PROD_X_MINUS_R_X = 22,
//   PROD_X_EPSILON = 23,
//   PROD_R_F_Y = 24,
//   PROD_Y_MUL_F_Y = 25,
//   PROD_Y_DIV_F_Y = 26,
//   PROD_Y_EPSILON = 27,
//   PROD_F_PAREN = 28,
//   PROD_F_ID = 29,
//   PROD_F_INT8 = 30,
//   PROD_F_INT10 = 31,
//   PROD_F_INT16 = 32,
//   PROD_UNVALID
// } ProductionID;

typedef enum {
  PROD_P_LT = 0,
  PROD_T_PT = 1,
  PROD_T_EPSILON = 2,
  PROD_L_S_SEMI = 3,
  PROD_S_ASSIGN = 4,
  PROD_S_IF_C_THEN_S_N = 5,
  PROD_S_WHILE_C_DO_S = 6,
  PROD_S_BEGIN_L_END = 7,
  PROD_N_ELSE_S = 8,
  PROD_N_EPSILON = 9,
  PROD_C_E_O = 10,
  PROD_C_PAREN = 11,
  PROD_O_GT = 12,
  PROD_O_LT = 13,
  PROD_O_EQ = 14,
  PROD_O_GE = 15,
  PROD_O_LE = 16,
  PROD_O_NE = 17,
  PROD_E_R_X = 18,
  PROD_X_PLUS_R_X = 19,
  PROD_X_MINUS_R_X = 20,
  PROD_X_EPSILON = 21,
  PROD_R_F_Y = 22,
  PROD_Y_MUL_F_Y = 23,
  PROD_Y_DIV_F_Y = 24,
  PROD_Y_EPSILON = 25,
  PROD_F_PAREN = 26,
  PROD_F_ID = 27,
  PROD_F_INT8 = 28,
  PROD_F_INT10 = 29,
  PROD_F_INT16 = 30,
  PROD_UNVALID
} ProductionID;

#endif /* GRAMMAR_H */
