/**
 * @file syntax_tree.h
 * @brief Syntax tree representation
 */

#ifndef SYNTAX_TREE_H
#define SYNTAX_TREE_H

#include "common.h"
#include "lexer-analyzer/token.h"
#include <stdbool.h>

/**
 * @brief Syntax tree node
 */
typedef struct SyntaxTreeNode SyntaxTreeNode;

/**
 * @brief Node types for syntax tree
 */
typedef enum {
  NODE_NONTERMINAL, /* Non-terminal node */
  NODE_TERMINAL,    /* Terminal node */
  NODE_EPSILON      /* Epsilon (empty) node */
} NodeType;

/**
 * @brief Syntax tree node structure
 */
struct SyntaxTreeNode {
  NodeType type; /* Node type */
  union {
    int nonterminal_id; /* Non-terminal ID */
    Token token;        /* Terminal token */
  };
  char *symbol_name; /* Symbol name for display */

  /* Tree structure */
  SyntaxTreeNode *parent;       /* Parent node */
  SyntaxTreeNode *first_child;  /* First child node */
  SyntaxTreeNode *next_sibling; /* Next sibling node */

  /* Production information (for non-terminals) */
  int production_id; /* ID of the production used */
};

/**
 * @brief Syntax tree structure
 */
typedef struct {
  SyntaxTreeNode *root; /* Root node of the tree */
} SyntaxTree;

/**
 * @brief Create a new syntax tree
 *
 * @return SyntaxTree* Newly created tree (must be freed with
 * syntax_tree_destroy)
 */
SyntaxTree *syntax_tree_create(void);

/**
 * @brief Destroy a syntax tree and free resources
 *
 * @param tree Tree to destroy
 */
void syntax_tree_destroy(SyntaxTree *tree);

/**
 * @brief Create a non-terminal node
 *
 * @param nonterminal_id ID of the non-terminal
 * @param symbol_name Symbol name for display
 * @param production_id ID of the production used
 * @return SyntaxTreeNode* Created node
 */
SyntaxTreeNode *syntax_tree_create_nonterminal(int nonterminal_id,
                                               const char *symbol_name,
                                               int production_id);

/**
 * @brief Create a terminal node
 *
 * @param token Terminal token
 * @param symbol_name Symbol name for display
 * @return SyntaxTreeNode* Created node
 */
SyntaxTreeNode *syntax_tree_create_terminal(Token token,
                                            const char *symbol_name);

/**
 * @brief Create an epsilon node
 *
 * @return SyntaxTreeNode* Created node
 */
SyntaxTreeNode *syntax_tree_create_epsilon(void);

/**
 * @brief Add a child node to a parent node
 *
 * @param parent Parent node
 * @param child Child node to add
 */
void syntax_tree_add_child(SyntaxTreeNode *parent, SyntaxTreeNode *child);

/**
 * @brief Set the root node of a syntax tree
 *
 * @param tree Tree to modify
 * @param root Root node to set
 */
void syntax_tree_set_root(SyntaxTree *tree, SyntaxTreeNode *root);

/**
 * @brief Print the syntax tree to stdout
 *
 * @param tree Tree to print
 */
void syntax_tree_print(const SyntaxTree *tree);

#endif /* SYNTAX_TREE_H */
