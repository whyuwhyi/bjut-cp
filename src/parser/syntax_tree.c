/**
 * @file syntax_tree.c
 * @brief Syntax tree implementation
 */

#include "parser/syntax_tree.h"
#include "lexer/token.h"
#include "parser_common.h"
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
void destroy_syntax_tree_node(SyntaxTreeNode *node) {
  if (!node) {
    return;
  }

  /* Free symbol name */
  if (node->symbol_name) {
    free(node->symbol_name);
  }

#ifdef CONFIG_TAC
  void sdt_attributes_destroy(struct SDTAttributes * attributes);
  /* Free semantic attributes */
  if (node->attributes) {
    sdt_attributes_destroy(node->attributes);
  }
#endif

  /* Free children and recursively destroy them */
  if (node->children) {
    for (int i = 0; i < node->children_count; i++) {
      destroy_syntax_tree_node(node->children[i]);
    }
    free(node->children);
  }

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
  node->children = NULL;
  node->children_count = 0;
  node->children_capacity = 0;
  node->production_id = production_id;

#ifdef CONFIG_TAC
  node->attributes = NULL;
#endif

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
  DEBUG_PRINT("Created identifier node with value: '%s'", node->token.str_val);
  node->symbol_name = safe_strdup(symbol_name);
  if (!node->symbol_name) {
    free(node);
    return NULL;
  }

  node->parent = NULL;
  node->children = NULL;
  node->children_count = 0;
  node->children_capacity = 0;
  node->production_id = -1;

#ifdef CONFIG_TAC
  node->attributes = NULL;
#endif
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
  node->children = NULL;
  node->children_count = 0;
  node->children_capacity = 0;
  node->production_id = -1;

#ifdef CONFIG_TAC
  node->attributes = NULL;
#endif

  DEBUG_PRINT("Created epsilon node");
  return node;
}

/**
 * @brief Add a child node to a parent node
 */
bool syntax_tree_add_child(SyntaxTreeNode *parent, SyntaxTreeNode *child) {
  if (!parent || !child) {
    return false;
  }

  /* Set parent-child relationship */
  child->parent = parent;

  /* Add to children array */
  if (parent->children == NULL) {
    /* Initial allocation with space for 4 children */
    parent->children =
        (SyntaxTreeNode **)safe_malloc(4 * sizeof(SyntaxTreeNode *));
    if (!parent->children) {
      fprintf(stderr, "Error: Failed to allocate memory for children array\n");
      return false;
    }
    parent->children_count = 0;
    parent->children_capacity = 4;
  } else if (parent->children_count >= parent->children_capacity) {
    /* Double capacity when full */
    int new_capacity = parent->children_capacity * 2;
    SyntaxTreeNode **new_children = (SyntaxTreeNode **)safe_realloc(
        parent->children, new_capacity * sizeof(SyntaxTreeNode *));
    if (!new_children) {
      fprintf(stderr,
              "Error: Failed to reallocate memory for children array\n");
      return false;
    }
    parent->children = new_children;
    parent->children_capacity = new_capacity;
  }

  /* Add child to array */
  parent->children[parent->children_count++] = child;

  if (parent->children_count == 1) {
    DEBUG_PRINT("Added first child (%s) to node (%s)", child->symbol_name,
                parent->symbol_name);
  } else {
    DEBUG_PRINT("Added child (%s) to node (%s) at index %d", child->symbol_name,
                parent->symbol_name, parent->children_count - 1);
  }

  return true;
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
 * @brief Get the root node of a syntax tree
 */
SyntaxTreeNode *syntax_tree_get_root(const SyntaxTree *tree) {
  if (!tree) {
    return NULL;
  }
  return tree->root;
}

/**
 * @brief Internal recursive function: print a single node and its subtree
 * @param node     The node to print
 * @param prefix   Current prefix string (passed from parent node)
 * @param is_last  Whether this node is the last child of its parent
 */
static void print_tree(const SyntaxTreeNode *node, const char *prefix,
                       bool is_last) {
  if (!node)
    return;
  // Print prefix
  printf("%s", prefix);
  // Print branch symbol
  if (is_last) {
    printf("└─");
  } else {
    printf("├─");
  }
  // Print the node's information
  switch (node->type) {
  case NODE_NONTERMINAL:
    printf("%s", node->symbol_name);
    if (node->production_id >= 0) {
      printf(" (Prod:%d)", node->production_id);
    }
    break;
  case NODE_TERMINAL: {
    char token_str[128];
    token_to_string(&node->token, token_str, sizeof(token_str));
    printf("%s [%s]", node->symbol_name, token_str);
    break;
  }
  case NODE_EPSILON:
    printf("%s", node->symbol_name);
    break;
  }
  printf("\n");
  // Prepare new prefix for child nodes
  int n = node->children_count;
  for (int i = 0; i < n; i++) {
    // If current node is not the last child, add "│  " to prefix,
    // otherwise add spaces "   "
    char child_prefix[1024];
    snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix,
             is_last ? "   " : "│  ");
    // Recursively print the i-th child, passing whether it's the last one
    print_tree(node->children[i], child_prefix, i == n - 1);
  }
}

/**
 * @brief Print the entire syntax tree
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
  // Root node is treated as "last" (to avoid extra vertical lines after it)
  print_tree(tree->root, "", true);
}
