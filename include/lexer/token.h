/**
 * @file token.h
 * @brief Token definitions for the lexical analyzer
 *
 * This file defines the token structure and token types
 * used by the lexical analyzer.
 */

#ifndef TOKEN_H
#define TOKEN_H

#include "common.h"
#include <stddef.h>

/**
 * @brief Token types enum
 */
typedef enum {
  TK_NOTYPE = 0, /**< Default/uninitialized token */
  TK_SPC,        /**< Whitespace */

  /* Keywords */
  TK_IF,    /**< 'if' keyword */
  TK_THEN,  /**< 'then' keyword */
  TK_ELSE,  /**< 'else' keyword */
  TK_WHILE, /**< 'while' keyword */
  TK_DO,    /**< 'do' keyword */
  TK_BEGIN, /**< 'begin' keyword */
  TK_END,   /**< 'end' keyword */

  /* Operators */
  TK_ADD, /**< '+' addition operator */
  TK_SUB, /**< '-' subtraction operator */
  TK_MUL, /**< '*' multiplication operator */
  TK_DIV, /**< '/' division operator */
  TK_GT,  /**< '>' greater than operator */
  TK_LT,  /**< '<' less than operator */
  TK_EQ,  /**< '=' equal operator */
  TK_GE,  /**< '>=' greater than or equal operator */
  TK_LE,  /**< '<=' less than or equal operator */
  TK_NEQ, /**< '<>' not equal operator */

  /* Delimiters */
  TK_SLP,  /**< '(' left parenthesis */
  TK_SRP,  /**< ')' right parenthesis */
  TK_SEMI, /**< ';' semicolon */

  /* Identifiers and literals */
  TK_IDN,   /**< Identifier */
  TK_DEC,   /**< Decimal integer */
  TK_OCT,   /**< Octal integer */
  TK_HEX,   /**< Hexadecimal integer */
  TK_ILOCT, /**< Invalid octal integer */
  TK_ILHEX, /**< Invalid hexadecimal integer */

  TK_EOF /** End of file token */
} TokenType;

/**
 * @brief Token structure
 */
typedef struct {
  TokenType type; /**< Type of token */
  int line;       /**< Line number where token appears */
  int column;     /**< Column position where token starts */
  union {
    int num_val;                        /**< Numeric value for number tokens */
    char str_val[CONFIG_MAX_TOKEN_LEN]; /**< String value for identifiers */
  };
} Token;

/**
 * @brief Get string representation of token type
 *
 * @param type Token type
 * @return const char* String representation
 */
const char *token_type_to_string(TokenType type);

/**
 * @brief Format token into string
 *
 * @param token Token to format
 * @param buffer Buffer to store formatted string
 * @param buffer_size Size of buffer
 */
void token_to_string_famt(const Token *token, char *buffer, size_t buffer_size);

/**
 * @brief Format token into string
 *
 * @param token Token to format
 * @param buffer Buffer to store formatted string
 * @param buffer_size Size of buffer
 */
void token_to_string(const Token *token, char *buffer, size_t buffer_size);

/**
 * @brief Create a token with numeric value
 *
 * @param type Token type
 * @param value Numeric value
 * @param line Line number where token appears
 * @param column Column position where token starts
 * @return Token Initialized token
 */
Token token_create_num(TokenType type, int value, int line, int column);

/**
 * @brief Create a token with string value
 *
 * @param type Token type
 * @param value String value
 * @param line Line number where token appears
 * @param column Column position where token starts
 * @return Token Initialized token
 */
Token token_create_str(TokenType type, const char *value, int line, int column);

/**
 * @brief Create a token with no value
 *
 * @param type Token type
 * @param line Line number where token appears
 * @param column Column position where token starts
 * @return Token Initialized token
 */
Token token_create(TokenType type, int line, int column);

#endif /* TOKEN_H */
