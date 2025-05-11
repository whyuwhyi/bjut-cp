/**
 * @file lexer_error_handler.c
 * @brief Implementation of lexer-specific error handling functions
 */

#include "error_handler.h"
#include "lexer/lexer.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Report a lexical error with source highlighting
 */
void lexer_report_error(Lexer *lexer, int line, int column, int length,
                        const char *format, ...) {
  if (!lexer) {
    return;
  }

  lexer->has_error = true;
  lexer->error_count++;

  char error_message[256];
  va_list args;
  va_start(args, format);
  vsnprintf(error_message, sizeof(error_message), format, args);
  va_end(args);

  char line_buffer[256] = {0}; /* Initialize to empty */

  /* Ensure length value is reasonable */
  if (length <= 0)
    length = 1;
  if (length > 20)
    length = 20; /* Limit maximum error marker length */

  if (lexer->input && extract_line_from_input(lexer->input, line, line_buffer,
                                              sizeof(line_buffer)) > 0) {
    PRINT_ERROR_HIGHLIGHT(line, column, line_buffer, column, length, "%s",
                          error_message);

    /* Provide specific help messages based on error type */
    if (strstr(error_message, "Unrecognized character")) {
      PRINT_ERROR_HELP("This character is not part of the language syntax");
    } else if (strstr(error_message, "Invalid hexadecimal")) {
      PRINT_ERROR_HELP("Hexadecimal literals must start with '0x' followed by "
                       "valid hex digits (0-9, a-f, A-F)");
    } else if (strstr(error_message, "Invalid octal")) {
      PRINT_ERROR_HELP(
          "Octal literals must start with '0' followed by octal digits (0-7)");
    } else if (strstr(error_message, "Token is too long")) {
      PRINT_ERROR_HELP(
          "The maximum token length is defined as CONFIG_MAX_TOKEN_LEN");
    }
  } else {
    PRINT_ERROR(line, column, "%s", error_message);
  }
}
