/**
 * @file parser.h
 * @brief Parser interface
 */

#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "grammar.h"
#include "lexer_analyzer/lexer.h"
#include "syntax_tree.h"
#include <stdbool.h>

/**
 * @brief Parser type identifiers
 */
typedef enum {
  PARSER_TYPE_RECURSIVE_DESCENT, /* Recursive descent parser */
  PARSER_TYPE_LR0,               /* LR(0) parser */
  PARSER_TYPE_SLR1,              /* SLR(1) parser */
  PARSER_TYPE_LR1                /* LR(1) parser */
} ParserType;

/**
 * @brief Parser structure definition
 */
typedef struct Parser {
  ParserType type;  /* Type of parser */
  Grammar *grammar; /* Grammar for the language */
  struct ProductionTracker
      *production_tracker; /* Production tracker for leftmost derivation */

  /* SDT code generator (optional, only set when doing SDT) */
  struct SDTCodeGen *sdt_gen;

  /* Methods */
  bool (*init)(struct Parser *parser); /* Initialize parser */
  SyntaxTree *(*parse)(struct Parser *parser, Lexer *lexer); /* Parse input */
  void (*print_leftmost_derivation)(
      struct Parser *parser); /* Output leftmost derivation */
  void (*destroy)(
      struct Parser *parser); /* Destroy parser and free resources */

  /* Parser-specific data */
  void *data; /* Parser-specific data */
} Parser;

/**
 * @brief Create a parser of the specified type
 *
 * @param type Type of parser to create
 * @return Parser* The created parser, or NULL on failure
 */
Parser *parser_create(ParserType type);

/**
 * @brief Initialize parser with grammar and prepare for parsing
 *
 * @param parser Parser to initialize
 * @return bool Success status
 */
bool parser_init(Parser *parser);

/**
 * @brief Parse input using the lexer
 *
 * @param parser Initialized parser
 * @param lexer Initialized lexer with input
 * @return SyntaxTree* Resulting syntax tree, or NULL on failure
 */
SyntaxTree *parser_parse(Parser *parser, Lexer *lexer);

/**
 * @brief Print the leftmost derivation of the parsed input
 *
 * @param parser Parser that has parsed input
 */
void parser_print_leftmost_derivation(Parser *parser);

/**
 * @brief Destroy parser and free resources
 *
 * @param parser Parser to destroy
 */
void parser_destroy(Parser *parser);

/**
 * @brief Get parser type as string
 *
 * @param type Parser type
 * @return const char* String description
 */
const char *parser_type_to_string(ParserType type);

#endif /* PARSER_H */
