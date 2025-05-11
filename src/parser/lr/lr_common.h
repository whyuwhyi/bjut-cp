/**
 * @file lr_common.h
 * @brief Common LR parser utilities (internal)
 */

#ifndef LR_COMMON_H
#define LR_COMMON_H

#include "../production_tracker.h"
#include "action_table.h"
#include "automaton.h"
#include "item.h"
#include <stdbool.h>

/**
 * @brief Generic LR parser data
 */
typedef struct LRParserData {
  /* Parser automaton */
  LRAutomaton *automaton; /* LR automaton */

  /* Parsing table */
  ActionTable *table; /* Parsing table */

  /* Parsing state */
  Lexer *lexer;                /* Current lexer */
  int current_token;           /* Current token index */
  int *state_stack;            /* State stack */
  SyntaxTreeNode **node_stack; /* Node stack */
  int stack_top;               /* Stack top index */
  int stack_capacity;          /* Stack capacity */

  /* Error handling */
  bool has_error;          /* Error flag */
  char error_message[256]; /* Error message */

  /* Syntax tree building */
  SyntaxTree *syntax_tree; /* The syntax tree being built */
} LRParserData;

/**
 * @brief Initial capacity for stacks
 */
#define INITIAL_STACK_CAPACITY 128

/**
 * @brief Get the index of a terminal in the grammar
 *
 * @param grammar Grammar to search in
 * @param token TokenType to find
 * @return int Terminal index, or -1 if not found
 */
int get_terminal_index(Grammar *grammar, TokenType token);

/**
 * @brief Get the index of a non-terminal in the grammar
 *
 * @param grammar Grammar to search in
 * @param nonterminal_id Non-terminal ID to find
 * @return int Non-terminal index, or -1 if not found
 */
int get_nonterminal_index(Grammar *grammar, int nonterminal_id);

/**
 * @brief Get the symbol ID from a grammar index
 *
 * @param grammar Grammar containing the symbol
 * @param is_terminal Whether the symbol is a terminal
 * @param index Index in the respective type's array
 * @return int Symbol ID
 */
int get_symbol_id_from_index(Grammar *grammar, bool is_terminal, int index);

/**
 * @brief Initialize LR parser data
 *
 * @param data Parser data to initialize
 * @return bool Success status
 */
bool lr_parser_data_init(LRParserData *data);

/**
 * @brief Reset LR parser data for a new parse
 *
 * @param data Parser data to reset
 * @param lexer Lexer to use for parsing
 * @return bool Success status
 */
bool lr_parser_data_reset(LRParserData *data, Lexer *lexer);

/**
 * @brief Clean up LR parser data
 *
 * @param data Parser data to clean up
 */
void lr_parser_data_cleanup(LRParserData *data);

/**
 * @brief Parse input using LR parsing algorithm
 *
 * @param parser Parser
 * @param data Parser data
 * @param lexer Lexer with tokenized input
 * @return SyntaxTree* Resulting syntax tree, or NULL on failure
 */
SyntaxTree *lr_parser_parse(Parser *parser, LRParserData *data, Lexer *lexer);

#endif /* LR_COMMON_H */
