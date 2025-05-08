/**
 * @file codegen/sdt_codegen.c
 * @brief Syntax-directed translation code generator implementation
 */

#include "codegen/sdt_codegen.h"
#include "parser/syntax_tree.h"
#include "sdt/label_manager/label_manager.h"
#include "sdt/sdt_actions.h"
#include "sdt/sdt_attributes.h"
#include "sdt/symbol_table/symbol_table.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new SDT code generator
 */
SDTCodeGen *sdt_codegen_create(void) {
  SDTCodeGen *gen = (SDTCodeGen *)safe_malloc(sizeof(SDTCodeGen));
  if (!gen) {
    return NULL;
  }

  /* Initialize fields */
  gen->program = tac_program_create();
  gen->symbol_table = symbol_table_create();
  gen->label_manager = label_manager_create();
  gen->curr_attr = NULL;
  gen->has_error = false;
  memset(gen->error_message, 0, sizeof(gen->error_message));

  if (!gen->program || !gen->symbol_table || !gen->label_manager) {
    sdt_codegen_destroy(gen);
    return NULL;
  }

  DEBUG_PRINT("Created SDT code generator");
  return gen;
}

/**
 * @brief Initialize SDT code generator
 */
bool sdt_codegen_init(SDTCodeGen *gen) {
  if (!gen) {
    return false;
  }

  /* Reset state */
  gen->has_error = false;
  memset(gen->error_message, 0, sizeof(gen->error_message));

  DEBUG_PRINT("Initialized SDT code generator");
  return true;
}

/**
 * @brief Generate three-address code for a syntax tree
 */
void sdt_codegen_generate(SDTCodeGen *gen, SyntaxTreeNode *node) {

  if (!gen || !node)
    return;
  sdt_execute_action(gen, node);
}

/**
 * @brief Get error message from the code generator
 */
const char *sdt_codegen_get_error(const SDTCodeGen *gen) {
  if (!gen || !gen->has_error) {
    return NULL;
  }

  return gen->error_message;
}

/**
 * @brief Generate a new temporary variable
 */
char *sdt_new_temp(SDTCodeGen *gen) {
  if (!gen || !gen->symbol_table) {
    return NULL;
  }

  return symbol_table_new_temp(gen->symbol_table);
}

/**
 * @brief Generate a new label
 */
char *sdt_new_label(SDTCodeGen *gen) {
  if (!gen || !gen->label_manager) {
    return NULL;
  }

  return label_manager_new_label(gen->label_manager);
}

/**
 * @brief Add instruction to the program
 */
bool sdt_add_instruction(SDTCodeGen *gen, TACOpType op, const char *result,
                         const char *arg1, const char *arg2, int lineno) {
  if (!gen || !gen->program) {
    return false;
  }

  int index =
      tac_program_add_inst(gen->program, op, result, arg1, arg2, lineno);
  return (index >= 0);
}

/**
 * @brief Set error message
 */
void sdt_set_error(SDTCodeGen *gen, const char *format, ...) {
  if (!gen) {
    return;
  }

  va_list args;
  va_start(args, format);
  vsnprintf(gen->error_message, sizeof(gen->error_message), format, args);
  va_end(args);

  gen->has_error = true;
  DEBUG_PRINT("SDT error: %s", gen->error_message);
}

/**
 * @brief Free SDT code generator resources
 */
void sdt_codegen_destroy(SDTCodeGen *gen) {
  if (!gen) {
    return;
  }

  /* Free allocated resources */
  if (gen->program) {
    tac_program_destroy(gen->program);
  }

  if (gen->symbol_table) {
    symbol_table_destroy(gen->symbol_table);
  }

  if (gen->label_manager) {
    label_manager_destroy(gen->label_manager);
  }

  if (gen->curr_attr) {
    sdt_attributes_destroy(gen->curr_attr);
  }

  /* Free the generator itself */
  free(gen);

  DEBUG_PRINT("Destroyed SDT code generator");
}
