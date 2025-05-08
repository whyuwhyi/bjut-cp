/**
 * @file label_manager.h
 * @brief Label management for code generation (internal)
 */

#ifndef LABEL_MANAGER_H
#define LABEL_MANAGER_H

#include <stdbool.h>

/**
 * @brief Label manager structure
 */
typedef struct LabelManager {
  int label_counter; /* Counter for generating unique label names */
} LabelManager;

/**
 * @brief Create a new label manager
 *
 * @return LabelManager* Created label manager, or NULL on failure
 */
LabelManager *label_manager_create(void);

/**
 * @brief Generate a new label
 *
 * @param manager Label manager
 * @return char* Generated label name, or NULL on failure
 */
char *label_manager_new_label(LabelManager *manager);

/**
 * @brief Free label manager resources
 *
 * @param manager Label manager to destroy
 */
void label_manager_destroy(LabelManager *manager);

#endif /* LABEL_MANAGER_H */
