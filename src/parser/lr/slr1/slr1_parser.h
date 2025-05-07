/**
 * @file slr1_parser.h
 * @brief SLR(1) parser implementation (internal)
 */

#ifndef SLR1_PARSER_H
#define SLR1_PARSER_H

#include "../lr_common.h"

/**
 * @brief SLR(1) parser data
 */
typedef struct {
  LRParserData common; /* Common LR parser data */
} SLR1ParserData;

/**
 * @brief Create an SLR(1) parser
 *
 * @return Parser* The created parser
 */
Parser *slr1_parser_create(void);

/**
 * @brief Initialize the SLR(1) parser
 *
 * @param parser Parser to initialize
 * @return bool Success status
 */
bool slr1_parser_init(Parser *parser);

/**
 * @brief Parse input using SLR(1) parsing
 *
 * @param parser Initialized parser
 * @param lexer Initialized lexer with input
 * @return SyntaxTree* Resulting syntax tree, or NULL on failure
 */
SyntaxTree *slr1_parser_parse(Parser *parser, Lexer *lexer);

/**
 * @brief Print the leftmost derivation from the SLR(1) parser
 *
 * @param parser Parser that has parsed input
 */
void slr1_parser_print_leftmost_derivation(Parser *parser);

/**
 * @brief Destroy SLR(1) parser and free resources
 *
 * @param parser Parser to destroy
 */
void slr1_parser_destroy(Parser *parser);

/**
 * @brief Build the SLR(1) automaton
 *
 * @param parser Parser
 * @param data Parser data
 * @return bool Success status
 */
bool slr1_build_automaton(Parser *parser, SLR1ParserData *data);

/**
 * @brief Build the SLR(1) parsing table
 *
 * @param parser Parser
 * @param data Parser data
 * @return bool Success status
 */
bool slr1_build_parsing_table(Parser *parser, SLR1ParserData *data);

#endif /* SLR1_PARSER_H */
