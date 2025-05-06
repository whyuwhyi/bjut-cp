/**
 * @file lexer.h
 * @brief Lexical analyzer header
 *
 * This file defines the lexical analyzer interface for the compiler.
 */

#ifndef LEXER_H
#define LEXER_H

#include "lexer-analyzer/token.h"
#include <regex.h>
#include <stdbool.h>

/**
 * @brief Number of regular expression patterns
 */
#define NR_REGEX 27

/**
 * @brief Rule structure for lexical analysis
 */
typedef struct {
  const char *regex;    /**< Regular expression pattern */
  TokenType token_type; /**< Token type for this pattern */
} Rule;

/**
 * @brief Lexer structure for lexical analysis
 */
typedef struct {
  Rule rules[NR_REGEX];            /**< Regular expression rules */
  regex_t re[NR_REGEX];            /**< Compiled regular expressions */
  Token tokens[CONFIG_MAX_TOKENS]; /**< Array of recognized tokens */
  int nr_token;                    /**< Number of tokens recognized */
  bool initialized;                /**< Initialization flag */
} Lexer;

/**
 * @brief Create a new lexer
 *
 * @return Lexer* Pointer to the created lexer (must be freed with
 * lexer_destroy)
 */
Lexer *lexer_create(void);

/**
 * @brief Destroy a lexer and free all resources
 *
 * @param lexer The lexer to destroy
 */
void lexer_destroy(Lexer *lexer);

/**
 * @brief Initialize the lexer with regular expressions
 *
 * @param lexer The lexer to initialize
 * @return true if initialization succeeded, false otherwise
 */
bool lexer_init(Lexer *lexer);

/**
 * @brief Tokenize an input string
 *
 * @param lexer The initialized lexer
 * @param input The input string to tokenize
 * @return true if tokenization succeeded, false otherwise
 */
bool lexer_tokenize(Lexer *lexer, const char *input);

/**
 * @brief Print all tokens in the lexer
 *
 * @param lexer The lexer containing tokens
 */
void lexer_print_tokens(Lexer *lexer);

/**
 * @brief Get the token at a specific index
 *
 * @param lexer The lexer containing tokens
 * @param index The index of the token
 * @return const Token* Pointer to the token, or NULL if index is out of bounds
 */
const Token *lexer_get_token(const Lexer *lexer, int index);

/**
 * @brief Get the number of tokens in the lexer
 *
 * @param lexer The lexer containing tokens
 * @return int The number of tokens
 */
int lexer_token_count(const Lexer *lexer);

#endif /* LEXER_H */
