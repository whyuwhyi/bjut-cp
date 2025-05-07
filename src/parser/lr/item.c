/**
 * @file item.c
 * @brief LR item representation implementation
 */

#include "item.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create an LR item
 */
LRItem *lr_item_create(int production_id, int dot_position, int *lookaheads,
                       int lookahead_count) {
  LRItem *item = (LRItem *)safe_malloc(sizeof(LRItem));
  if (!item) {
    return NULL;
  }

  item->production_id = production_id;
  item->dot_position = dot_position;
  item->lookaheads = NULL;
  item->lookahead_count = 0;

  /* Copy lookaheads if provided */
  if (lookahead_count > 0 && lookaheads) {
    item->lookaheads = (int *)safe_malloc(lookahead_count * sizeof(int));
    if (!item->lookaheads) {
      free(item);
      return NULL;
    }

    memcpy(item->lookaheads, lookaheads, lookahead_count * sizeof(int));
    item->lookahead_count = lookahead_count;
  }

  DEBUG_PRINT("Created LR item: [%d, %d]", production_id, dot_position);
  return item;
}

/**
 * @brief Destroy an LR item
 */
void lr_item_destroy(LRItem *item) {
  if (!item) {
    return;
  }

  if (item->lookaheads) {
    free(item->lookaheads);
  }

  free(item);
  DEBUG_PRINT("Destroyed LR item");
}

/**
 * @brief Compare two LR items (without looking at lookaheads)
 */
bool lr_item_equals(LRItem *item1, LRItem *item2) {
  if (!item1 || !item2) {
    return false;
  }

  return (item1->production_id == item2->production_id &&
          item1->dot_position == item2->dot_position);
}

/**
 * @brief Compare two LR items including lookaheads
 */
bool lr_item_equals_with_lookaheads(LRItem *item1, LRItem *item2) {
  if (!lr_item_equals(item1, item2)) {
    return false;
  }

  if (item1->lookahead_count != item2->lookahead_count) {
    return false;
  }

  for (int i = 0; i < item1->lookahead_count; i++) {
    bool found = false;
    for (int j = 0; j < item2->lookahead_count; j++) {
      if (item1->lookaheads[i] == item2->lookaheads[j]) {
        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Add lookaheads to an item
 */
bool lr_item_add_lookaheads(LRItem *item, int *lookaheads,
                            int lookahead_count) {
  if (!item || (lookahead_count > 0 && !lookaheads)) {
    return false;
  }

  bool added = false;

  /* Add each new lookahead */
  for (int i = 0; i < lookahead_count; i++) {
    bool exists = false;

    for (int j = 0; j < item->lookahead_count; j++) {
      if (item->lookaheads[j] == lookaheads[i]) {
        exists = true;
        break;
      }
    }

    if (!exists) {
      /* Add lookahead to array */
      int new_count = item->lookahead_count + 1;
      int *new_lookaheads =
          (int *)safe_realloc(item->lookaheads, new_count * sizeof(int));
      if (!new_lookaheads) {
        return added;
      }

      item->lookaheads = new_lookaheads;
      item->lookaheads[item->lookahead_count] = lookaheads[i];
      item->lookahead_count = new_count;

      added = true;
    }
  }

  return added;
}

/**
 * @brief Check if an item is a core item
 */
bool lr_item_is_core(LRItem *item) {
  if (!item) {
    return false;
  }

  return (item->dot_position > 0 || item->production_id == 0);
}

/**
 * @brief Get the symbol after the dot
 */
int lr_item_get_symbol_after_dot(LRItem *item, Grammar *grammar) {
  if (!item || !grammar) {
    return -1;
  }

  Production *prod = &grammar->productions[item->production_id];

  /* Check if dot is at the end */
  if (item->dot_position >= prod->rhs_length) {
    return -1;
  }

  /* Check for epsilon production */
  if (prod->rhs_length == 1 && prod->rhs[0].type == SYMBOL_EPSILON) {
    return -1;
  }

  /* Get symbol after dot */
  Symbol *symbol = &prod->rhs[item->dot_position];

  if (symbol->type == SYMBOL_TERMINAL) {
    /* Find terminal index */
    for (int i = 0; i < grammar->terminals_count; i++) {
      if (grammar->symbols[grammar->terminal_indices[i]].token ==
          symbol->token) {
        return grammar->terminal_indices[i];
      }
    }
  } else if (symbol->type == SYMBOL_NONTERMINAL) {
    return grammar->nonterminal_indices[symbol->nonterminal];
  }

  return -1;
}

/**
 * @brief Check if an item has the dot at the end
 */
bool lr_item_is_reduction(LRItem *item, Grammar *grammar) {
  if (!item || !grammar) {
    return false;
  }

  Production *prod = &grammar->productions[item->production_id];

  /* Check if dot is at the end */
  if (item->dot_position >= prod->rhs_length) {
    return true;
  }

  /* Check for epsilon production */
  if (prod->rhs_length == 1 && prod->rhs[0].type == SYMBOL_EPSILON) {
    return true;
  }

  return false;
}

/**
 * @brief Print an LR item
 */
void lr_item_print(LRItem *item, Grammar *grammar) {
  if (!item || !grammar) {
    return;
  }

  Production *prod = &grammar->productions[item->production_id];
  const char *lhs_name =
      grammar->symbols[grammar->nonterminal_indices[prod->lhs]].name;

  printf("%s -> ", lhs_name);

  /* Print RHS with dot */
  for (int i = 0; i < prod->rhs_length; i++) {
    if (i == item->dot_position) {
      printf("• ");
    }

    Symbol *symbol = &prod->rhs[i];

    if (symbol->type == SYMBOL_TERMINAL) {
      /* Find terminal name */
      for (int j = 0; j < grammar->terminals_count; j++) {
        if (grammar->symbols[grammar->terminal_indices[j]].token ==
            symbol->token) {
          printf("%s ", grammar->symbols[grammar->terminal_indices[j]].name);
          break;
        }
      }
    } else if (symbol->type == SYMBOL_NONTERMINAL) {
      printf("%s ",
             grammar->symbols[grammar->nonterminal_indices[symbol->nonterminal]]
                 .name);
    } else if (symbol->type == SYMBOL_EPSILON) {
      printf("ε ");
    }
  }

  /* Print dot at end if needed */
  if (item->dot_position == prod->rhs_length) {
    printf("• ");
  }

  /* Print lookaheads */
  if (item->lookahead_count > 0) {
    printf(", [");
    for (int i = 0; i < item->lookahead_count; i++) {
      int lookahead = item->lookaheads[i];

      if (lookahead >= 0 && lookahead < grammar->terminals_count) {
        printf("%s",
               grammar->symbols[grammar->terminal_indices[lookahead]].name);
      } else {
        printf("?");
      }

      if (i < item->lookahead_count - 1) {
        printf(", ");
      }
    }
    printf("]");
  }
}
