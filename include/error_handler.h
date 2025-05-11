/**
 * @file error_handler.h
 * @brief Error handling and reporting utilities for the compiler
 *
 * This module provides functions and macros for error reporting,
 * error highlighting, and recovery mechanisms for both lexical
 * and syntax analysis phases.
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include "lexer/token.h"
#include <stdbool.h>

/* Forward declarations for parser-related types */
struct Lexer;
struct Parser;
struct LRParserData;
struct Grammar;

/* Terminal colors for error reporting */
#define COLOR_RESET "\033[0m"
#define COLOR_ERROR "\033[1;31m"     /* Bold Red */
#define COLOR_WARNING "\033[1;33m"   /* Bold Yellow */
#define COLOR_HIGHLIGHT "\033[1;36m" /* Bold Cyan */
#define COLOR_NOTE "\033[1;32m"      /* Bold Green */
#define COLOR_LOCATION "\033[1;34m"  /* Bold Blue */
#define COLOR_CODE "\033[0;36m"      /* Cyan */
#define COLOR_POINTER "\033[1;31m"   /* Bold Red (for error pointer) */
#define COLOR_UNDERLINE "\033[4m"    /* Underline */

/* Error reporting macros */
#define PRINT_ERROR(line, col, fmt, ...)                                       \
  do {                                                                         \
    fprintf(stderr, COLOR_ERROR "error" COLOR_RESET ": " fmt "\n",             \
            ##__VA_ARGS__);                                                    \
    fprintf(stderr,                                                            \
            "  " COLOR_LOCATION "--> " COLOR_RESET "[line:%d col:%d]\n", line, \
            col);                                                              \
  } while (0)

/* Print Error Message with source highlight */
#define PRINT_ERROR_HIGHLIGHT(line, col, source_line, col_pos, error_len, fmt, \
                              ...)                                             \
  do {                                                                         \
    fprintf(stderr, COLOR_ERROR "error" COLOR_RESET ": " fmt "\n",             \
            ##__VA_ARGS__);                                                    \
    fprintf(stderr,                                                            \
            "  " COLOR_LOCATION "--> " COLOR_RESET "[line:%d col:%d]\n", line, \
            col);                                                              \
    fprintf(stderr, "   " COLOR_LOCATION "|\n" COLOR_RESET);                   \
    fprintf(stderr, COLOR_LOCATION "%4d" COLOR_RESET " | %s\n", line,          \
            source_line);                                                      \
    fprintf(stderr, "   " COLOR_LOCATION "|" COLOR_RESET " ");                 \
    int pos = (col_pos > 0) ? (col_pos + 1) : 0;                               \
    int len = (error_len > 0) ? error_len : 1;                                 \
    if (len > 20)                                                              \
      len = 20;                                                                \
    for (int i = 0; i < pos; i++)                                              \
      fprintf(stderr, " ");                                                    \
    for (int i = 0; i < len; i++)                                              \
      fprintf(stderr, COLOR_POINTER "^" COLOR_RESET);                          \
    fprintf(stderr, " " COLOR_ERROR "here\n" COLOR_RESET);                     \
    fprintf(stderr, "   " COLOR_LOCATION "|\n" COLOR_RESET);                   \
  } while (0)

/* Print additional help message for an error */
#define PRINT_ERROR_HELP(help_message)                                         \
  do {                                                                         \
    fprintf(stderr,                                                            \
            "   " COLOR_LOCATION "= " COLOR_NOTE "help" COLOR_RESET ": %s\n",  \
            help_message);                                                     \
  } while (0)

/* Print warning message */
#define PRINT_WARNING(line, col, fmt, ...)                                     \
  do {                                                                         \
    fprintf(stderr, COLOR_WARNING "warning" COLOR_RESET ": " fmt "\n",         \
            ##__VA_ARGS__);                                                    \
    fprintf(stderr,                                                            \
            "  " COLOR_LOCATION "--> " COLOR_RESET "[line:%d col:%d]\n", line, \
            col);                                                              \
  } while (0)

/* Common error handling functions */

/**
 * @brief Extract a line from the input string
 *
 * @param input        Input string
 * @param line         Line number to extract (1-based)
 * @param buffer       Buffer to store the extracted line
 * @param buffer_size  Size of the buffer
 * @return int         Length of the extracted line or -1 if error
 */
int extract_line_from_input(const char *input, int line, char *buffer,
                            int buffer_size);

/* Lexer error handling functions */

/**
 * @brief Report a lexical error with source highlighting
 *
 * @param lexer        The lexer instance
 * @param line         Line number where error occurred
 * @param column       Column number where error occurred
 * @param length       Length of the erroneous text
 * @param format       Error message format
 * @param ...          Format arguments
 */
void lexer_report_error(struct Lexer *lexer, int line, int column, int length,
                        const char *format, ...);

/* Synchronization point types for error recovery */
typedef enum {
  SYNC_NONE = 0,  /* Not a synchronization point */
  SYNC_STATEMENT, /* Statement-level sync point (e.g., semicolon) */
  SYNC_BLOCK,     /* Block-level sync point (e.g., right brace) */
  SYNC_EXPRESSION /* Expression-level sync point (e.g., right parenthesis) */
} SyncPointType;

/* Parser error handling functions */

/**
 * @brief Report a syntax error with source highlighting and recovery
 * suggestions
 */
void report_syntax_error(struct Parser *parser, struct LRParserData *data,
                         const Token *token, const char *source_line);

/**
 * @brief Determine expected tokens in the current parser state
 */
int determine_expected_tokens(struct Parser *parser, struct LRParserData *data,
                              int current_state, TokenType *expected,
                              int max_expected);

/**
 * @brief Check if a token is a synchronization point for error recovery
 */
SyncPointType is_sync_point(TokenType token_type);

/**
 * @brief Try to find a possible missing token based on current context
 */
TokenType find_missing_token(struct Parser *parser, struct LRParserData *data,
                             int current_state, const Token *token);

/**
 * @brief Skip tokens until a synchronization point is found
 */
bool skip_to_sync_point(struct LRParserData *data,
                        SyncPointType min_sync_level);

/**
 * @brief Enhanced error recovery for LR parsers
 */
bool enhanced_error_recovery(struct Parser *parser, struct LRParserData *data,
                             const Token *error_token);

/**
 * @brief Check if parser is currently in an expression context
 */
bool is_expression_context(struct Parser *parser, struct LRParserData *data);

/**
 * @brief Check if parser is currently in a statement context
 */
bool is_statement_context(struct Parser *parser, struct LRParserData *data);

#endif /* ERROR_HANDLER_H */
