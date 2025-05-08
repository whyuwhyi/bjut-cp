/**
 * @file codegen.h
 * @brief Syntax-directed translation code generator interface
 */

#ifndef SDT_CODEGEN_H
#define SDT_CODEGEN_H

#include "codegen/tac.h"
#include "lexer_analyzer/lexer.h"
#include "parser/parser.h"
#include <stdbool.h>

typedef struct SDTCodeGen SDTCodeGen;

/**
 * @brief Create a new SDT code generator
 *
 * @return SDTCodeGen* Created code generator, or NULL on failure
 */
SDTCodeGen *sdt_codegen_create(void);

/**
 * @brief Initialize SDT code generator
 *
 * @param gen Code generator to initialize
 * @return bool Success status
 */
bool sdt_codegen_init(SDTCodeGen *gen);

/**
 * @brief Generate three-address code during parsing
 *
 * Generates code on-the-fly during syntax analysis
 *
 * @param gen Initialized code generator
 * @param parser Initialized parser
 * @param lexer Initialized lexer with input
 * @return TACProgram* Generated three-address code program, or NULL on failure
 */
TACProgram *sdt_codegen_generate(SDTCodeGen *gen, Parser *parser, Lexer *lexer);

/**
 * @brief Execute semantic action for a production
 *
 * This function is called by the parser when a production is recognized
 *
 * @param gen Code generator
 * @param production_id Production ID
 * @param node Syntax tree node representing the production
 * @return bool Success status
 */
bool sdt_execute_action(SDTCodeGen *gen, int production_id,
                        SyntaxTreeNode *node);

/**
 * @brief Get error message from the code generator
 *
 * @param gen Code generator
 * @return const char* Error message, or NULL if no error occurred
 */
const char *sdt_codegen_get_error(const SDTCodeGen *gen);

/**
 * @brief Free SDT code generator resources
 *
 * @param gen Code generator to destroy
 */
void sdt_codegen_destroy(SDTCodeGen *gen);

#endif /* CODEGEN_H */
