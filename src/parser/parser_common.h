/**
 * @file parser_common.h
 * @brief Common parser utilities (internal)
 */

#ifndef PARSER_COMMON_H
#define PARSER_COMMON_H

#include "common.h"
#include "parser/grammar.h"
#include "parser/parser.h"
#include "parser/syntax_tree.h"
#include <stdbool.h>

/**
 * @brief Structure for production tracking (leftmost derivation)
 */
typedef struct ProductionTracker {
  int *production_sequence; /* Sequence of productions used */
  int length;               /* Length of production sequence */
  int capacity;             /* Capacity of production sequence */
} ProductionTracker;

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
