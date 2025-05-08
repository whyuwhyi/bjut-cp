/**
 * @file sdt_attributes.c
 * @brief Implementation of attribute structure for syntax-directed translation
 */

#include "codegen/sdt_attributes.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new attributes structure
 */
SDTAttributes *sdt_attributes_create(void) {
  SDTAttributes *attrs = (SDTAttributes *)safe_malloc(sizeof(SDTAttributes));
  if (!attrs) {
    return NULL;
  }

  /* Initialize all fields to NULL */
  attrs->code = NULL;
  attrs->place = NULL;
  attrs->true_label = NULL;
  attrs->false_label = NULL;
  attrs->next_label = NULL;
  attrs->begin_label = NULL;

  DEBUG_PRINT("Created SDT attributes");
  return attrs;
}

/**
 * @brief Free attributes resources
 */
void sdt_attributes_destroy(SDTAttributes *attrs) {
  if (!attrs) {
    return;
  }

  /* Free all dynamically allocated strings */
  if (attrs->code)
    free(attrs->code);
  if (attrs->place)
    free(attrs->place);
  if (attrs->true_label)
    free(attrs->true_label);
  if (attrs->false_label)
    free(attrs->false_label);
  if (attrs->next_label)
    free(attrs->next_label);
  if (attrs->begin_label)
    free(attrs->begin_label);

  /* Free the attributes structure itself */
  free(attrs);

  DEBUG_PRINT("Destroyed SDT attributes");
}

/**
 * @brief Concatenate two code sequences
 */
char *sdt_code_concat(const char *code1, const char *code2) {
  /* Handle NULL inputs */
  if (!code1 && !code2) {
    return safe_strdup("");
  }
  if (!code1) {
    return safe_strdup(code2);
  }
  if (!code2) {
    return safe_strdup(code1);
  }

  /* Allocate memory for concatenated code */
  size_t len1 = strlen(code1);
  size_t len2 = strlen(code2);
  char *result = (char *)safe_malloc(len1 + len2 + 1);
  if (!result) {
    return NULL;
  }

  /* Concatenate the code sequences */
  strcpy(result, code1);
  strcat(result, code2);

  DEBUG_PRINT("Concatenated code sequences");
  return result;
}

/**
 * @brief Make a copy of attributes
 */
SDTAttributes *sdt_attributes_copy(const SDTAttributes *attrs) {
  if (!attrs) {
    return NULL;
  }

  /* Create a new attributes structure */
  SDTAttributes *copy = sdt_attributes_create();
  if (!copy) {
    return NULL;
  }

  /* Copy all fields */
  if (attrs->code)
    copy->code = safe_strdup(attrs->code);
  if (attrs->place)
    copy->place = safe_strdup(attrs->place);
  if (attrs->true_label)
    copy->true_label = safe_strdup(attrs->true_label);
  if (attrs->false_label)
    copy->false_label = safe_strdup(attrs->false_label);
  if (attrs->next_label)
    copy->next_label = safe_strdup(attrs->next_label);
  if (attrs->begin_label)
    copy->begin_label = safe_strdup(attrs->begin_label);

  DEBUG_PRINT("Copied SDT attributes");
  return copy;
}
