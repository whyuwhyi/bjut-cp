/**
 * @file lr1_parser.h
 * @brief LR(1) parser implementation (internal)
 */

#ifndef LR1_PARSER_H
#define LR1_PARSER_H

#include "../lr_common.h"

/**
 * @brief LR(1) parser data
 */
typedef struct {
  LRParserData common; /* Common LR parser data */
} LR1ParserData;

/**
 * @brief Create an LR(1) parser
 *
 * @return Parser* The created parser
 */
Parser *lr1_parser_create(void);

/**
 * @brief Initialize the LR(1) parser
 *
 * @param parser Parser to initialize
 * @return bool Success status
 */
bool lr1_parser_init(Parser *parser);

/**
 * @brief Parse input using LR(1) parsing
 *
 * @param parser Initialized parser
 * @param lexer Initialized lexer with input
 * @return SyntaxTree* Resulting syntax tree, or NULL on failure
 */
SyntaxTree *lr1_parser_parse(Parser *parser, Lexer *lexer);

/**
 * @brief Print the leftmost derivation from the LR(1) parser
 *
 * @param parser Parser that has parsed input
 */
void lr1_parser_print_leftmost_derivation(Parser *parser);

/**
 * @brief Destroy LR(1) parser and free resources
 *
 * @param parser Parser to destroy
 */
void lr1_parser_destroy(Parser *parser);

/**
 * @brief Build the LR(1) automaton
 *
 * @param parser Parser
 * @param data Parser data
 * @return bool Success status
 */
bool lr1_build_automaton(Parser *parser, LR1ParserData *data);

/**
 * @brief Build the LR(1) parsing table
 *
 * @param parser Parser
 * @param data Parser data
 * @return bool Success status
 */
bool lr1_build_parsing_table(Parser *parser, LR1ParserData *data);

#endif /* LR1_PARSER_H */
