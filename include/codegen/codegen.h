/**
 * @file codegen.h
 * @brief Three-address code generator interface
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "codegen/tac.h"
#include "lexer_analyzer/lexer.h"
#include "parser/parser.h"
#include "parser/syntax_tree.h"

/**
 * @brief Code generator structure
 */
typedef struct CodeGenerator CodeGenerator;

/**
 * @brief Create a new code generator
 *
 * @return CodeGenerator* Created code generator, or NULL on failure
 */
CodeGenerator *codegen_create(void);

/**
 * @brief Initialize code generator
 *
 * @param gen Code generator to initialize
 * @return bool Success status
 */
bool codegen_init(CodeGenerator *gen);

/**
 * @brief Generate three-address code from a syntax tree
 *
 * @param gen Initialized code generator
 * @param tree Syntax tree to generate code from
 * @return TACProgram* Generated three-address code program, or NULL on failure
 */
TACProgram *codegen_generate(CodeGenerator *gen, SyntaxTree *tree);

/**
 * @brief Generate three-address code directly from input
 *
 * @param gen Initialized code generator
 * @param lexer Initialized lexer with input
 * @param parser Initialized parser
 * @return TACProgram* Generated three-address code program, or NULL on failure
 */
TACProgram *codegen_generate_from_source(CodeGenerator *gen, Lexer *lexer,
                                         Parser *parser);

/**
 * @brief Free code generator resources
 *
 * @param gen Code generator to destroy
 */
void codegen_destroy(CodeGenerator *gen);

#endif /* CODEGEN_H */
