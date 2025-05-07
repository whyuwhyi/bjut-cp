/**
 * @file rd_parser.h
 * @brief Recursive descent parser implementation
 */

#ifndef RD_PARSER_H
#define RD_PARSER_H

#include "../parser_common.h"

/**
 * @brief Recursive descent parser data
 */
typedef struct {
  /* Parsing state */
  Lexer *lexer;            /* Current lexer */
  int current_token_index; /* Current token index */

  /* Error handling */
  bool has_error;          /* Error flag */
  char error_message[256]; /* Error message */

  /* Syntax tree building */
  SyntaxTree *syntax_tree;      /* The syntax tree being built */
  SyntaxTreeNode *current_node; /* Current node in the tree */
} RDParserData;

/**
 * @brief Create a recursive descent parser
 *
 * @return Parser* The created parser
 */
Parser *rd_parser_create(void);

/**
 * @brief Initialize the recursive descent parser
 *
 * @param parser Parser to initialize
 * @return bool Success status
 */
bool rd_parser_init(Parser *parser);

/**
 * @brief Parse input using recursive descent
 *
 * @param parser Initialized parser
 * @param lexer Initialized lexer with input
 * @return SyntaxTree* Resulting syntax tree, or NULL on failure
 */
SyntaxTree *rd_parser_parse(Parser *parser, Lexer *lexer);

/**
 * @brief Print the leftmost derivation from the recursive descent parser
 *
 * @param parser Parser that has parsed input
 */
void rd_parser_print_leftmost_derivation(Parser *parser);

/**
 * @brief Destroy recursive descent parser and free resources
 *
 * @param parser Parser to destroy
 */
void rd_parser_destroy(Parser *parser);

#endif /* RD_PARSER_H */
