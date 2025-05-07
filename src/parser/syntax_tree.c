/**
 * @file syntax_tree.c
 * @brief Syntax tree implementation
 */

#include "parser/syntax_tree.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new syntax tree
 */
SyntaxTree *syntax_tree_create(void) {
  SyntaxTree *tree = (SyntaxTree *)safe_malloc(sizeof(SyntaxTree));
  if (!tree) {
    return NULL;
  }

  tree->root = NULL;
  DEBUG_PRINT("Created new syntax tree");
  return tree;
}

/**
 * @brief Helper function to destroy a syntax tree node recursively
 */
static void destroy_syntax_tree_node(SyntaxTreeNode *node) {
  if (!node) {
    return;
  }

  /* Free symbol name */
  if (node->symbol_name) {
    free(node->symbol_name);
  }

  /* Recursively free children and siblings */
  destroy_syntax_tree_node(node->first_child);
  destroy_syntax_tree_node(node->next_sibling);

  /* Free the node itself */
  free(node);
}

/**
 * @brief Destroy a syntax tree and free resources
 */
void syntax_tree_destroy(SyntaxTree *tree) {
  if (!tree) {
    return;
  }

  destroy_syntax_tree_node(tree->root);
  free(tree);
  DEBUG_PRINT("Destroyed syntax tree");
}

/**
 * @brief Create a non-terminal node
 */
SyntaxTreeNode *syntax_tree_create_nonterminal(int nonterminal_id,
                                               const char *symbol_name,
                                               int production_id) {
  SyntaxTreeNode *node = (SyntaxTreeNode *)safe_malloc(sizeof(SyntaxTreeNode));
  if (!node) {
    return NULL;
  }

  /* Initialize node */
  node->type = NODE_NONTERMINAL;
  node->nonterminal_id = nonterminal_id;
  node->symbol_name = safe_strdup(symbol_name);
  if (!node->symbol_name) {
    free(node);
    return NULL;
  }

  node->parent = NULL;
  node->first_child = NULL;
  node->next_sibling = NULL;
  node->production_id = production_id;

  DEBUG_PRINT("Created non-terminal node: %s (ID: %d, Production: %d)",
              symbol_name, nonterminal_id, production_id);
  return node;
}

/**
 * @brief Create a terminal node
 */
SyntaxTreeNode *syntax_tree_create_terminal(Token token,
                                            const char *symbol_name) {
  SyntaxTreeNode *node = (SyntaxTreeNode *)safe_malloc(sizeof(SyntaxTreeNode));
  if (!node) {
    return NULL;
  }

  /* Initialize node */
  node->type = NODE_TERMINAL;
  node->token = token;
  node->symbol_name = safe_strdup(symbol_name);
  if (!node->symbol_name) {
    free(node);
    return NULL;
  }

  node->parent = NULL;
  node->first_child = NULL;
  node->next_sibling = NULL;
  node->production_id = -1; /* Not applicable for terminals */

  DEBUG_PRINT("Created terminal node: %s", symbol_name);
  return node;
}

/**
 * @brief Create an epsilon node
 */
SyntaxTreeNode *syntax_tree_create_epsilon(void) {
  SyntaxTreeNode *node = (SyntaxTreeNode *)safe_malloc(sizeof(SyntaxTreeNode));
  if (!node) {
    return NULL;
  }

  /* Initialize node */
  node->type = NODE_EPSILON;
  node->symbol_name = safe_strdup("ε");
  if (!node->symbol_name) {
    free(node);
    return NULL;
  }

  node->parent = NULL;
  node->first_child = NULL;
  node->next_sibling = NULL;
  node->production_id = -1; /* Not applicable for epsilon */

  DEBUG_PRINT("Created epsilon node");
  return node;
}

/**
 * @brief Add a child node to a parent node
 */
void syntax_tree_add_child(SyntaxTreeNode *parent, SyntaxTreeNode *child) {
  if (!parent || !child) {
    return;
  }

  /* Set parent-child relationship */
  child->parent = parent;

  /* Add as first child if none exists */
  if (!parent->first_child) {
    parent->first_child = child;
    DEBUG_PRINT("Added first child (%s) to node (%s)", child->symbol_name,
                parent->symbol_name);
    return;
  }

  /* Otherwise, find the last sibling and add there */
  SyntaxTreeNode *sibling = parent->first_child;
  while (sibling->next_sibling) {
    sibling = sibling->next_sibling;
  }

  sibling->next_sibling = child;
  DEBUG_PRINT("Added child (%s) to node (%s)", child->symbol_name,
              parent->symbol_name);
}

/**
 * @brief Set the root node of a syntax tree
 */
void syntax_tree_set_root(SyntaxTree *tree, SyntaxTreeNode *root) {
  if (!tree) {
    return;
  }

  tree->root = root;
  if (root) {
    DEBUG_PRINT("Set tree root to node: %s", root->symbol_name);
  } else {
    DEBUG_PRINT("Set tree root to NULL");
  }
}

/**
 * @brief Helper function to print a syntax tree node with indentation
 */
static void print_syntax_tree_node(const SyntaxTreeNode *node, int indent) {
  if (!node) {
    return;
  }

  /* Print indentation */
  for (int i = 0; i < indent; i++) {
    printf("  ");
  }

  /* Print node information */
  switch (node->type) {
  case NODE_NONTERMINAL:
    printf("%s", node->symbol_name);
    if (node->production_id >= 0) {
      printf(" (Production: %d)", node->production_id);
    }
    printf("\n");
    break;

  case NODE_TERMINAL: {
    char token_str[128];
    token_to_string(&node->token, token_str, sizeof(token_str));
    printf("%s [%s]\n", node->symbol_name, token_str);
  } break;

  case NODE_EPSILON:
    printf("%s\n", node->symbol_name);
    break;
  }

  /* Print children with increased indentation */
  print_syntax_tree_node(node->first_child, indent + 1);

  /* Print siblings with same indentation */
  print_syntax_tree_node(node->next_sibling, indent);
}

/**
 * @brief Get the root node of a syntax tree
 */
SyntaxTreeNode *syntax_tree_get_root(const SyntaxTree *tree) {
  if (!tree) {
    return NULL;
  }
  return tree->root;
}

/**
 * @brief Print the syntax tree to stdout
 */
void syntax_tree_print(const SyntaxTree *tree) {
  if (!tree) {
    printf("Syntax tree is NULL\n");
    return;
  }

  if (!tree->root) {
    printf("Syntax tree is empty\n");
    return;
  }

  printf("Syntax Tree:\n");
  print_syntax_tree_node(tree->root, 0);
}
