/**
 * @file lr0_parser.h
 * @brief LR(0) parser implementation (internal)
 */

#ifndef LR0_PARSER_H
#define LR0_PARSER_H

#include "../lr_common.h"

/**
 * @brief LR(0) parser data
 */
typedef struct {
  LRParserData common; /* Common LR parser data */
} LR0ParserData;

/**
 * @brief Create an LR(0) parser
 *
 * @return Parser* The created parser
 */
Parser *lr0_parser_create(void);

/**
 * @brief Initialize the LR(0) parser
 *
 * @param parser Parser to initialize
 * @return bool Success status
 */
bool lr0_parser_init(Parser *parser);

/**
 * @brief Parse input using LR(0) parsing
 *
 * @param parser Initialized parser
 * @param lexer Initialized lexer with input
 * @return SyntaxTree* Resulting syntax tree, or NULL on failure
 */
SyntaxTree *lr0_parser_parse(Parser *parser, Lexer *lexer);

/**
 * @brief Print the leftmost derivation from the LR(0) parser
 *
 * @param parser Parser that has parsed input
 */
void lr0_parser_print_leftmost_derivation(Parser *parser);

/**
 * @brief Destroy LR(0) parser and free resources
 *
 * @param parser Parser to destroy
 */
void lr0_parser_destroy(Parser *parser);

/**
 * @brief Build the LR(0) automaton
 *
 * @param parser Parser
 * @param data Parser data
 * @return bool Success status
 */
bool lr0_build_automaton(Parser *parser, LR0ParserData *data);

/**
 * @brief Build the LR(0) parsing table
 *
 * @param parser Parser
 * @param data Parser data
 * @return bool Success status
 */
bool lr0_build_parsing_table(Parser *parser, LR0ParserData *data);

#endif /* LR0_PARSER_H */
