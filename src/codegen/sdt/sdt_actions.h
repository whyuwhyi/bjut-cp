/**
 * @file codegen/sdt/sdt_actions.h
 * @brief Semantic actions for syntax-directed translation
 */

#ifndef SDT_ACTIONS_H
#define SDT_ACTIONS_H

#include "parser/syntax_tree.h"
#include <stdbool.h>

struct SDTCodeGen; /* Forward declaration */

/**
 * @brief Execute the semantic actions for a node based on its production ID
 *
 * @param gen Code generator
 * @param node Syntax tree node
 * @return bool Success status
 */
bool sdt_execute_action(struct SDTCodeGen *gen, SyntaxTreeNode *node);

#endif /* SDT_ACTIONS_H */
