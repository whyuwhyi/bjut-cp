/**
 * @file parser_common.h
 * @brief Common parser utilities (internal)
 */

#ifndef PARSER_COMMON_H
#define PARSER_COMMON_H

#include "common.h"
#include "grammar.h"
#include "lexer_analyzer/lexer.h"
#include "parser/parser.h"
#include "parser/syntax_tree.h"
#include <stdbool.h>

/**
 * @brief Structure for production tracking (leftmost derivation)
 */
typedef struct {
  int *production_sequence; /* Sequence of productions used */
  int length;               /* Length of production sequence */
  int capacity;             /* Capacity of production sequence */
} ProductionTracker;

/**
 * @brief Actual parser structure definition (for internal use)
 */
struct Parser {
  ParserType type;  /* Type of parser */
  Grammar *grammar; /* Grammar for the language */
  ProductionTracker
      *production_tracker; /* Production tracker for leftmost derivation */

  /* Methods */
  bool (*init)(struct Parser *parser); /* Initialize parser */
  SyntaxTree *(*parse)(struct Parser *parser, Lexer *lexer); /* Parse input */
  void (*print_leftmost_derivation)(
      struct Parser *parser); /* Output leftmost derivation */
  void (*destroy)(
      struct Parser *parser); /* Destroy parser and free resources */

  /* Parser-specific data */
  void *data; /* Parser-specific data */
};

/**
 * @brief Create a new production tracker
 *
 * @return ProductionTracker* Created tracker
 */
ProductionTracker *production_tracker_create(void);

/**
 * @brief Destroy a production tracker and free resources
 *
 * @param tracker Tracker to destroy
 */
void production_tracker_destroy(ProductionTracker *tracker);

/**
 * @brief Add a production to the tracker
 *
 * @param tracker Tracker to add to
 * @param production_id Production ID to add
 * @return bool Success status
 */
bool production_tracker_add(ProductionTracker *tracker, int production_id);

/**
 * @brief Print the production sequence (leftmost derivation)
 *
 * @param tracker Tracker containing the sequence
 * @param grammar Grammar containing the productions
 */
void production_tracker_print(ProductionTracker *tracker, Grammar *grammar);

#endif /* PARSER_COMMON_H */
