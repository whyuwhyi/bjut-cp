/**
 * @file grammar.h
 * @brief Grammar representation for the parser
 */

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "common.h"
#include "lexer_analyzer/token.h"
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
 * @brief Define non-terminal symbol IDs for the grammar
 */
typedef enum {
  NT_P, /* Program */
  NT_L, /* Statement list */
  NT_S, /* Statement */
  NT_C, /* Condition */
  NT_E, /* Expression */
  NT_T, /* Term */
  NT_F  /* Factor */
} Nonterminal;

#endif /* GRAMMAR_H */
