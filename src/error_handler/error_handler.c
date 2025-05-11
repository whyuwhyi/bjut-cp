/**
 * @file error_handler.c
 * @brief Implementation of general error handling functions
 */

#include "error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Extract a line from the input string
 */
int extract_line_from_input(const char *input, int line, char *buffer,
                            int buffer_size) {
  if (!input || !buffer || buffer_size <= 0 || line <= 0) {
    return -1;
  }

  int current_line = 1;
  const char *line_start = input;
  const char *p = input;

  /* Find the start of the requested line */
  while (*p && current_line < line) {
    if (*p == '\n') {
      current_line++;
      line_start = p + 1;
    }
    p++;
  }

  if (current_line != line) {
    /* Line number is out of bounds */
    return -1;
  }

  /* Find the end of the line */
  const char *line_end = line_start;
  while (*line_end && *line_end != '\n') {
    line_end++;
  }

  /* Calculate line length */
  int line_length = line_end - line_start;

  int copy_length =
      (line_length < buffer_size - 1) ? line_length : buffer_size - 1;

  if (copy_length > 80)
    copy_length = 80; /* Limit to 80 characters for display */

  /* Copy the line to the buffer */
  strncpy(buffer, line_start, copy_length);
  buffer[copy_length] = '\0';

  return copy_length;
}
