/**
 * @file parser.c
 * @brief Parser interface implementation
 */

#include "parser/parser.h"
#include "grammar.h"
#include "parser_common.h"
#include "recursive_descent/rd_parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for parser implementations */
Parser *rd_parser_create(void);
Parser *lr0_parser_create(void);
Parser *slr1_parser_create(void);
Parser *lr1_parser_create(void);

/**
 * @brief Actual parser structure
 */
struct Parser {
  ParserType type;  /* Type of parser */
  Grammar *grammar; /* Grammar for the language */
  ProductionTracker
      *production_tracker; /* Production tracker for leftmost derivation */

  /* Methods */
  bool (*init)(Parser *parser);                       /* Initialize parser */
  SyntaxTree *(*parse)(Parser *parser, Lexer *lexer); /* Parse input */
  void (*print_leftmost_derivation)(
      Parser *parser);             /* Output leftmost derivation */
  void (*destroy)(Parser *parser); /* Destroy parser and free resources */

  /* Parser-specific data */
  void *data; /* Parser-specific data */
};

/**
 * @brief Create a parser of the specified type
 */
Parser *parser_create(ParserType type) {
  Parser *parser = NULL;

  switch (type) {
  case PARSER_TYPE_RECURSIVE_DESCENT:
    parser = rd_parser_create();
    break;
  case PARSER_TYPE_LR0:
    fprintf(stderr, "LR(0) parser not implemented yet\n");
    return NULL;
  case PARSER_TYPE_SLR1:
    fprintf(stderr, "SLR(1) parser not implemented yet\n");
    return NULL;
  case PARSER_TYPE_LR1:
    fprintf(stderr, "LR(1) parser not implemented yet\n");
    return NULL;
  default:
    fprintf(stderr, "Unknown parser type: %d\n", type);
    return NULL;
  }

  if (parser) {
    parser->type = type;

    /* Create grammar */
    parser->grammar = grammar_create();
    if (!parser->grammar) {
      fprintf(stderr, "Failed to create grammar\n");
      parser->destroy(parser);
      return NULL;
    }

    /* Create production tracker */
    parser->production_tracker = production_tracker_create();
    if (!parser->production_tracker) {
      fprintf(stderr, "Failed to create production tracker\n");
      grammar_destroy(parser->grammar);
      parser->destroy(parser);
      return NULL;
    }

    DEBUG_PRINT("Created %s parser", parser_type_to_string(type));
  }

  return parser;
}

/**
 * @brief Initialize parser with grammar and prepare for parsing
 */
bool parser_init(Parser *parser) {
  if (!parser) {
    return false;
  }

  /* Initialize grammar */
  if (!grammar_init(parser->grammar)) {
    fprintf(stderr, "Failed to initialize grammar\n");
    return false;
  }

  /* Compute FIRST and FOLLOW sets for grammar */
  if (!grammar_compute_first_follow_sets(parser->grammar)) {
    fprintf(stderr, "Failed to compute FIRST and FOLLOW sets\n");
    return false;
  }

  /* Initialize parser-specific data */
  if (!parser->init(parser)) {
    fprintf(stderr, "Failed to initialize parser-specific data\n");
    return false;
  }

  DEBUG_PRINT("Initialized %s parser", parser_type_to_string(parser->type));
  return true;
}

/**
 * @brief Parse input using the lexer
 */
SyntaxTree *parser_parse(Parser *parser, Lexer *lexer) {
  if (!parser || !lexer) {
    return NULL;
  }

  DEBUG_PRINT("Parsing with %s parser", parser_type_to_string(parser->type));
  return parser->parse(parser, lexer);
}

/**
 * @brief Print the leftmost derivation of the parsed input
 */
void parser_print_leftmost_derivation(Parser *parser) {
  if (!parser) {
    return;
  }

  if (parser->print_leftmost_derivation) {
    parser->print_leftmost_derivation(parser);
  } else {
    production_tracker_print(parser->production_tracker, parser->grammar);
  }
}

/**
 * @brief Destroy parser and free resources
 */
void parser_destroy(Parser *parser) {
  if (!parser) {
    return;
  }

  /* Free grammar */
  if (parser->grammar) {
    grammar_destroy(parser->grammar);
  }

  /* Free production tracker */
  if (parser->production_tracker) {
    production_tracker_destroy(parser->production_tracker);
  }

  /* Call parser-specific destructor */
  if (parser->destroy) {
    parser->destroy(parser);
  } else {
    free(parser);
  }

  DEBUG_PRINT("Destroyed parser");
}

/**
 * @brief Get parser type as string
 */
const char *parser_type_to_string(ParserType type) {
  switch (type) {
  case PARSER_TYPE_RECURSIVE_DESCENT:
    return "Recursive Descent";
  case PARSER_TYPE_LR0:
    return "LR(0)";
  case PARSER_TYPE_SLR1:
    return "SLR(1)";
  case PARSER_TYPE_LR1:
    return "LR(1)";
  default:
    return "Unknown";
  }
}
