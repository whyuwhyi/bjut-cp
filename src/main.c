/**
 * @file main.c
 * @brief Compiler main entry point
 */

#include "common.h"
#include "lexer-analyzer/lexer.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
  printf("Compiler v%s\n", PROJECT_VERSION_STRING);

  if (argc < 2) {
    printf("Usage: %s <source_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // Read source file
  char *source = read_file(argv[1]);
  if (!source) {
    fprintf(stderr, "Failed to read file: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  // Create lexer
  Lexer *lexer = lexer_create();
  if (!lexer_init(lexer)) {
    fprintf(stderr, "Failed to initialize lexer\n");
    free(source);
    return EXIT_FAILURE;
  }

  // Perform lexical analysis
  printf("Tokenizing file: %s\n", argv[1]);
  if (!lexer_tokenize(lexer, source)) {
    fprintf(stderr, "Tokenization failed\n");
    lexer_destroy(lexer);
    free(source);
    return EXIT_FAILURE;
  }

  // Print tokens
  printf("Tokenization result:\n");
  lexer_print_tokens(lexer);

  // Clean up resources
  lexer_destroy(lexer);
  free(source);

  return EXIT_SUCCESS;
}
