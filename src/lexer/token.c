/**
 * @file token.c
 * @brief Implementation of token functions
 */

#include "lexer/token.h"
#include <stdio.h>
#include <string.h>

/**
 * Token type string representations
 */
static const char *token_type_strings[] = {
    "NOTYPE", "SPC",
    /* Keywords */
    "IF", "THEN", "ELSE", "WHILE", "DO", "BEGIN", "END",
    /* Operators */
    "ADD", "SUB", "MUL", "DIV", "GT", "LT", "EQ", "GE", "LE", "NEQ",
    /* Delimiters */
    "SLP", "SRP", "SEMI",
    /* Identifiers and literals */
    "IDN", "DEC", "OCT", "HEX", "ILOCT", "ILHEX", "EOF"};

/**
 * Get string representation of token type
 */
const char *token_type_to_string(TokenType type) {
  if (type >= 0 &&
      type < sizeof(token_type_strings) / sizeof(token_type_strings[0])) {
    return token_type_strings[type];
  }
  return "UNKNOWN";
}

/**
 * Format token into string
 */
void token_to_string_famt(const Token *token, char *buffer,
                          size_t buffer_size) {
  if (!token || !buffer || buffer_size == 0) {
    return;
  }

  const char *type_str = token_type_to_string(token->type);

  switch (token->type) {
  case TK_DEC:
  case TK_OCT:
  case TK_HEX:
    snprintf(buffer, buffer_size, "%-10s %d", type_str, token->num_val);
    break;
  case TK_IDN:
    snprintf(buffer, buffer_size, "%-10s %s", type_str, token->str_val);
    break;
  default:
    snprintf(buffer, buffer_size, "%-10s - ", type_str);
    break;
  }
}

/**
 * @brief Convert token to its string representation
 *
 * Returns the appropriate symbol for operators (e.g., "=" for TK_EQ),
 * identifier name for TK_IDN, and numeric string for number tokens.
 *
 * @param token       The token to convert
 * @param buffer      Output buffer
 * @param buffer_size Size of the output buffer
 */
void token_to_string(const Token *token, char *buffer, size_t buffer_size) {
  if (!token || !buffer || buffer_size == 0) {
    return;
  }

  switch (token->type) {
  // Numeric tokens
  case TK_DEC:
    snprintf(buffer, buffer_size, "%d", token->num_val);
    break;
  case TK_OCT:
    snprintf(buffer, buffer_size, "0%o", token->num_val);
    break;
  case TK_HEX:
    snprintf(buffer, buffer_size, "0x%X", token->num_val);
    break;

  // Identifier token
  case TK_IDN:
    snprintf(buffer, buffer_size, "%s", token->str_val);
    break;

  // Operators and keywords
  case TK_EQ:
    snprintf(buffer, buffer_size, "=");
    break;
  case TK_NEQ:
    snprintf(buffer, buffer_size, "<>");
    break;
  case TK_LT:
    snprintf(buffer, buffer_size, "<");
    break;
  case TK_LE:
    snprintf(buffer, buffer_size, "<=");
    break;
  case TK_GT:
    snprintf(buffer, buffer_size, ">");
    break;
  case TK_GE:
    snprintf(buffer, buffer_size, ">=");
    break;
  case TK_ADD:
    snprintf(buffer, buffer_size, "+");
    break;
  case TK_SUB:
    snprintf(buffer, buffer_size, "-");
    break;
  case TK_MUL:
    snprintf(buffer, buffer_size, "*");
    break;
  case TK_DIV:
    snprintf(buffer, buffer_size, "/");
    break;
  case TK_SLP:
    snprintf(buffer, buffer_size, "(");
    break;
  case TK_SRP:
    snprintf(buffer, buffer_size, ")");
    break;
  case TK_SEMI:
    snprintf(buffer, buffer_size, ";");
    break;

  // Keywords
  case TK_BEGIN:
    snprintf(buffer, buffer_size, "begin");
    break;
  case TK_END:
    snprintf(buffer, buffer_size, "end");
    break;
  case TK_IF:
    snprintf(buffer, buffer_size, "if");
    break;
  case TK_THEN:
    snprintf(buffer, buffer_size, "then");
    break;
  case TK_ELSE:
    snprintf(buffer, buffer_size, "else");
    break;
  case TK_WHILE:
    snprintf(buffer, buffer_size, "while");
    break;
  case TK_DO:
    snprintf(buffer, buffer_size, "do");
    break;
  case TK_EOF:
    snprintf(buffer, buffer_size, "EOF");
    break;

  // Default case
  default:
    snprintf(buffer, buffer_size, "%s", token_type_to_string(token->type));
    break;
  }
}

/**
 * Create a token with numeric value
 */
Token token_create_num(TokenType type, int value, int line, int column) {
  Token token;
  token.type = type;
  token.num_val = value;
  token.line = line;
  token.column = column;
  return token;
}

/**
 * Create a token with string value
 */
Token token_create_str(TokenType type, const char *value, int line,
                       int column) {
  Token token;
  token.type = type;
  token.line = line;
  token.column = column;

  if (value) {
    strncpy(token.str_val, value, CONFIG_MAX_TOKEN_LEN - 1);
    token.str_val[CONFIG_MAX_TOKEN_LEN - 1] = '\0';
  } else {
    token.str_val[0] = '\0';
  }

  return token;
}

/**
 * Create a token with no value
 */
Token token_create(TokenType type, int line, int column) {
  Token token;
  token.type = type;
  token.num_val = 0;
  token.line = line;
  token.column = column;
  return token;
}
