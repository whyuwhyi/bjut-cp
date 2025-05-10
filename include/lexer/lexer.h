/**
 * @file lexer.h
 * @brief Lexical analyzer header
 *
 * This file defines the lexical analyzer interface for the compiler.
 */

#ifndef LEXER_H
#define LEXER_H

#include "common.h"
#include "lexer/token.h"
#include <stdbool.h>

#ifdef CONFIG_LEXER_REGEX
#include <regex.h>
/**
 * @brief Number of regular expression patterns
 */
#define NR_REGEX 27
#endif

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
#ifdef CONFIG_LEXER_REGEX
  Rule rules[NR_REGEX]; /**< Regular expression rules */
  regex_t re[NR_REGEX]; /**< Compiled regular expressions */
#endif
  Token tokens[CONFIG_MAX_TOKENS]; /**< Array of recognized tokens */
  int nr_token;                    /**< Number of tokens recognized */
  bool initialized;                /**< Initialization flag */
  bool has_error;                  /**< Error flag */
  int error_count;                 /**< Count of errors */

  /* Line tracking for error reporting */
  int current_line;   /**< Current line during tokenization */
  int current_column; /**< Current column during tokenization */
  const char *input;  /**< Reference to input string for error reporting */
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
 * @return true if tokenization succeeded with no errors, false otherwise
 */
bool lexer_tokenize(Lexer *lexer, const char *input);

#ifdef CONFIG_LEXER_REGEX
/**
 * @brief Tokenize an input string using regular expressions
 *
 * @param lexer The initialized lexer
 * @param input The input string to tokenize
 * @return true if tokenization succeeded with no errors, false otherwise
 */
bool lexer_tokenize_regex(Lexer *lexer, const char *input);
#endif

#ifdef CONFIG_LEXER_STATE_MACHINE
/**
 * @brief Tokenize an input string using state machine
 *
 * @param lexer The initialized lexer
 * @param input The input string to tokenize
 * @return true if tokenization succeeded with no errors, false otherwise
 */
bool lexer_tokenize_state_machine(Lexer *lexer, const char *input);
#endif

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

/**
 * @brief Report a lexical error with highlighting
 *
 * @param lexer The lexer
 * @param line Line number where error occurred
 * @param column Column where error occurred
 * @param length Length of the erroneous text
 * @param format Format string for error message
 * @param ... Additional arguments for format
 */
void lexer_report_error(Lexer *lexer, int line, int column, int length,
                        const char *format, ...);

/**
 * @brief Check if lexer has encountered any errors
 *
 * @param lexer The lexer
 * @return true if errors were encountered, false otherwise
 */
bool lexer_has_errors(const Lexer *lexer);

/**
 * @brief Extract a line from the input string
 *
 * @param input Input string
 * @param line Line number (1-based)
 * @param buffer Buffer to store the line
 * @param buffer_size Size of the buffer
 * @return int Length of the extracted line or -1 if error
 */
int extract_line_from_input(const char *input, int line, char *buffer,
                            int buffer_size);

#endif /* LEXER_H */
