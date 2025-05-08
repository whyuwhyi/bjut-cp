/**
 * @file label_manager.c
 * @brief Label management for code generation implementation
 */

#include "label_manager.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new label manager
 */
LabelManager *label_manager_create(void) {
  LabelManager *manager = (LabelManager *)safe_malloc(sizeof(LabelManager));
  if (!manager) {
    return NULL;
  }

  manager->label_counter = 0;

  DEBUG_PRINT("Created label manager");
  return manager;
}

/**
 * @brief Generate a new label
 */
char *label_manager_new_label(LabelManager *manager) {
  if (!manager) {
    return NULL;
  }

  char label_name[32];
  snprintf(label_name, sizeof(label_name), "L%d", manager->label_counter++);

  DEBUG_PRINT("Generated label '%s'", label_name);
  return safe_strdup(label_name);
}

/**
 * @brief Free label manager resources
 */
void label_manager_destroy(LabelManager *manager) {
  if (!manager) {
    return;
  }

  free(manager);

  DEBUG_PRINT("Destroyed label manager");
}
