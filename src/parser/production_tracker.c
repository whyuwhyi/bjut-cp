/**
 * @file parser_common.c
 * @brief Common parser utilities implementation
 */

#include "production_tracker.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initial capacity for production sequence
 */
#define INITIAL_PRODUCTION_CAPACITY 64

/**
 * @brief Create a new production tracker
 */
ProductionTracker *production_tracker_create(void) {
  ProductionTracker *tracker =
      (ProductionTracker *)safe_malloc(sizeof(ProductionTracker));
  if (!tracker) {
    return NULL;
  }

  tracker->production_sequence =
      (int *)safe_malloc(INITIAL_PRODUCTION_CAPACITY * sizeof(int));
  if (!tracker->production_sequence) {
    free(tracker);
    return NULL;
  }

  tracker->length = 0;
  tracker->capacity = INITIAL_PRODUCTION_CAPACITY;

  DEBUG_PRINT("Created production tracker");
  return tracker;
}

/**
 * @brief Destroy a production tracker and free resources
 */
void production_tracker_destroy(ProductionTracker *tracker) {
  if (!tracker) {
    return;
  }

  if (tracker->production_sequence) {
    free(tracker->production_sequence);
  }

  free(tracker);
  DEBUG_PRINT("Destroyed production tracker");
}

/**
 * @brief Add a production to the tracker
 */
bool production_tracker_add(ProductionTracker *tracker, int production_id) {
  if (!tracker) {
    return false;
  }

  /* Check if we need to resize */
  if (tracker->length >= tracker->capacity) {
    int new_capacity = tracker->capacity * 2;
    int *new_sequence = (int *)safe_realloc(tracker->production_sequence,
                                            new_capacity * sizeof(int));
    if (!new_sequence) {
      return false;
    }

    tracker->production_sequence = new_sequence;
    tracker->capacity = new_capacity;
    DEBUG_PRINT("Resized production tracker to capacity %d", new_capacity);
  }

  /* Add production */
  tracker->production_sequence[tracker->length++] = production_id;
  DEBUG_PRINT("Added production %d to tracker", production_id);
  return true;
}

/**
 * @brief Remove the last production from the tracker
 *
 * @param tracker Tracker to remove from
 * @return bool Success status
 */
bool production_tracker_remove_last(ProductionTracker *tracker) {
  if (!tracker || tracker->length <= 0) {
    return false;
  }

  // Simply decrease the length
  tracker->length--;
  return true;
}

/**
 * @brief Print the production sequence (leftmost derivation)
 */
void production_tracker_print(ProductionTracker *tracker, Grammar *grammar) {
  if (!tracker || !grammar) {
    return;
  }

  printf("Leftmost Derivation:\n");
  for (int i = 0; i < tracker->length; i++) {
    int production_id = tracker->production_sequence[i];
    if (production_id >= 0 && production_id < grammar->productions_count) {
      printf("  %d: %s\n", i + 1,
             grammar->productions[production_id].display_str);
    } else {
      printf("  %d: <unknown production>\n", i + 1);
    }
  }
}
