/**
 * @file item.h
 * @brief LR item representation (internal)
 */

#ifndef LR_ITEM_H
#define LR_ITEM_H

#include "../grammar.h"
#include <stdbool.h>

/**
 * @brief LR item
 */
typedef struct {
  int production_id;   /* Production ID */
  int dot_position;    /* Position of the dot in the production */
  int *lookaheads;     /* Lookahead tokens (for LR(1)) */
  int lookahead_count; /* Number of lookahead tokens */
} LRItem;

/**
 * @brief Create an LR item
 *
 * @param production_id Production ID
 * @param dot_position Dot position
 * @param lookaheads Lookahead tokens (can be NULL)
 * @param lookahead_count Number of lookahead tokens
 * @return LRItem* Created item or NULL on failure
 */
LRItem *lr_item_create(int production_id, int dot_position, int *lookaheads,
                       int lookahead_count);

/**
 * @brief Destroy an LR item
 *
 * @param item Item to destroy
 */
void lr_item_destroy(LRItem *item);

/**
 * @brief Compare two LR items (without looking at lookaheads)
 *
 * @param item1 First item
 * @param item2 Second item
 * @return bool True if items have same production and dot position
 */
bool lr_item_equals(LRItem *item1, LRItem *item2);

/**
 * @brief Compare two LR items including lookaheads
 *
 * @param item1 First item
 * @param item2 Second item
 * @return bool True if items are identical
 */
bool lr_item_equals_with_lookaheads(LRItem *item1, LRItem *item2);

/**
 * @brief Add lookaheads to an item
 *
 * @param item Item to add to
 * @param lookaheads Lookaheads to add
 * @param lookahead_count Number of lookaheads
 * @return bool True if any lookaheads were added
 */
bool lr_item_add_lookaheads(LRItem *item, int *lookaheads, int lookahead_count);

/**
 * @brief Check if an item is a core item
 *
 * @param item Item to check
 * @return bool True if core item
 */
bool lr_item_is_core(LRItem *item);

/**
 * @brief Get the symbol after the dot
 *
 * @param item Item to check
 * @param grammar Grammar
 * @return int Symbol ID, or -1 if dot is at end
 */
int lr_item_get_symbol_after_dot(LRItem *item, Grammar *grammar);

/**
 * @brief Check if an item has the dot at the end
 *
 * @param item Item to check
 * @param grammar Grammar
 * @return bool True if dot is at end
 */
bool lr_item_is_reduction(LRItem *item, Grammar *grammar);

/**
 * @brief Print an LR item
 *
 * @param item Item to print
 * @param grammar Grammar
 */
void lr_item_print(LRItem *item, Grammar *grammar);

#endif /* LR_ITEM_H */
