/**
 * @file sdt_attributes.h
 * @brief Attribute structure for syntax-directed translation
 */

#ifndef SDT_ATTRIBUTES_H
#define SDT_ATTRIBUTES_H

/**
 * @brief Attributes for syntax-directed translation
 *
 * Stores both synthesized and inherited attributes for grammar symbols
 */
typedef struct {
  char *code;        /* Generated code */
  char *place;       /* Storage location (variable or temporary) */
  char *true_label;  /* Label to jump to if condition is true */
  char *false_label; /* Label to jump to if condition is false */
  char *next_label;  /* Label for the next statement */
  char *begin_label; /* Label for the beginning of a loop */
} SDTAttributes;

/**
 * @brief Create a new attributes structure
 *
 * @return SDTAttributes* Created attributes, or NULL on failure
 */
SDTAttributes *sdt_attributes_create(void);

/**
 * @brief Free attributes resources
 *
 * @param attrs Attributes to destroy
 */
void sdt_attributes_destroy(SDTAttributes *attrs);

/**
 * @brief Concatenate two code sequences
 *
 * @param code1 First code sequence (can be NULL)
 * @param code2 Second code sequence (can be NULL)
 * @return char* Concatenated code, or NULL on failure
 */
char *sdt_code_concat(const char *code1, const char *code2);

/**
 * @brief Make a copy of attributes
 *
 * @param attrs Attributes to copy
 * @return SDTAttributes* Copied attributes, or NULL on failure
 */
SDTAttributes *sdt_attributes_copy(const SDTAttributes *attrs);

#endif /* SDT_ATTRIBUTES_H */
