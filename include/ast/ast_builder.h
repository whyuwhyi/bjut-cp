/**
 * @file ast_builder.h
 * @brief AST Builder - converts syntax trees to abstract syntax trees
 */

#ifndef AST_BUILDER_H
#define AST_BUILDER_H

#include "ast.h"
#include "parser/syntax_tree.h"

/**
 * @brief Build an AST from a syntax tree
 *
 * @param tree The syntax tree to convert
 * @return AST* The built abstract syntax tree, or NULL on failure
 */
AST *ast_builder_build(const SyntaxTree *tree);

/**
 * @brief Helper to build an AST node from a syntax tree node
 *
 * @param node The syntax tree node to convert
 * @return ASTNode* The built AST node, or NULL on failure
 */
ASTNode *ast_builder_build_node(const SyntaxTreeNode *node);

#endif /* AST_BUILDER_H */
