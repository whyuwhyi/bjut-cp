#include "lexer-analyzer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  printf("Compiler v%s\n", PROJECT_VERSION_STRING);
  printf("Reading source from stdin (end with Ctrl+D on Unix or Ctrl+Z on "
         "Windows)\n");

  // --- Read all of stdin into one buffer ---
  size_t capacity = 1024;
  size_t length = 0;
  char *source = malloc(capacity);
  if (!source) {
    fprintf(stderr, "Out of memory\n");
    return EXIT_FAILURE;
  }

  int ch;
  while ((ch = getchar()) != EOF) {
    // grow buffer if needed
    if (length + 1 >= capacity) {
      capacity *= 2;
      char *newbuf = realloc(source, capacity);
      if (!newbuf) {
        free(source);
        fprintf(stderr, "Out of memory\n");
        return EXIT_FAILURE;
      }
      source = newbuf;
    }
    source[length++] = (char)ch;
  }
  source[length] = '\0'; // null-terminate

  // --- Initialize and run the lexer ---
  Lexer *lexer = lexer_create();
  if (!lexer || !lexer_init(lexer)) {
    fprintf(stderr, "Failed to initialize lexer\n");
    free(source);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  printf("Tokenizing standard input...\n");
  if (!lexer_tokenize(lexer, source)) {
    fprintf(stderr, "Tokenization failed\n");
    lexer_destroy(lexer);
    free(source);
    return EXIT_FAILURE;
  }

  printf("Tokenization result:\n");
  lexer_print_tokens(lexer);

  // --- Clean up ---
  lexer_destroy(lexer);
  free(source);
  return EXIT_SUCCESS;
}
