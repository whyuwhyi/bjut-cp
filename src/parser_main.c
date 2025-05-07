/**
 * @file parser-main.c
 * @brief Parser driver program
 */
#include "common.h"
#include "lexer_analyzer/lexer.h"
#include "parser/parser.h"
#include "parser/syntax_tree.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Command-line options */
static struct option long_options[] = {{"help", no_argument, NULL, 'h'},
                                       {"file", required_argument, NULL, 'f'},
                                       {"verbose", no_argument, NULL, 'v'},
                                       {NULL, 0, NULL, 0}};

/**
 * @brief Print usage information
 */
static void print_usage(const char *program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
  printf("  -h, --help                Display this help message\n");
  printf("  -f, --file FILEPATH       Input file path (default: stdin)\n");
  printf("  -v, --verbose             Enable verbose output\n");
}

/**
 * @brief Read contents from stdin into a string
 */
static char *read_stdin() {
  size_t capacity = 1024;
  size_t length = 0;
  char *source = malloc(capacity);
  if (!source) {
    fprintf(stderr, "Out of memory\n");
    return NULL;
  }

  int ch;
  while ((ch = getchar()) != EOF) {
    /* Grow buffer if needed */
    if (length + 1 >= capacity) {
      capacity *= 2;
      char *newbuf = realloc(source, capacity);
      if (!newbuf) {
        free(source);
        fprintf(stderr, "Out of memory\n");
        return NULL;
      }
      source = newbuf;
    }
    source[length++] = (char)ch;
  }
  source[length] = '\0'; /* Null-terminate */
  return source;
}

int main(int argc, char *argv[]) {
  /* Parse command-line arguments */
  bool verbose = false;
  char *input_file = NULL;
  int c;
  int option_index = 0;

  while ((c = getopt_long(argc, argv, "hf:v", long_options, &option_index)) !=
         -1) {
    switch (c) {
    case 'h':
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    case 'f':
      input_file = optarg;
      break;
    case 'v':
      verbose = true;
      break;
    case '?':
      /* getopt_long already printed an error message */
      print_usage(argv[0]);
      return EXIT_FAILURE;
    default:
      return EXIT_FAILURE;
    }
  }

  printf("Compiler v%s\n", PROJECT_VERSION_STRING);

  /* Determine parser type from Kconfig settings */
  ParserType parser_type;

#ifdef CONFIG_PARSER_RECURSIVE_DESCENT
  parser_type = PARSER_TYPE_RECURSIVE_DESCENT;
#elif defined(CONFIG_PARSER_LR0)
  parser_type = PARSER_TYPE_LR0;
#elif defined(CONFIG_PARSER_SLR1)
  parser_type = PARSER_TYPE_SLR1;
#elif defined(CONFIG_PARSER_LR1)
  parser_type = PARSER_TYPE_LR1;
#else
  parser_type = PARSER_TYPE_RECURSIVE_DESCENT; // Default
#endif

  /* Read input source */
  char *source;
  if (input_file) {
    printf("Reading source from file: %s\n", input_file);
    source = read_file(input_file);
  } else {
    printf("Reading source from stdin (end with Ctrl+D on Unix or Ctrl+Z on "
           "Windows)\n");
    source = read_stdin();
  }

  if (!source) {
    return EXIT_FAILURE;
  }

  /* Create and initialize lexer */
  Lexer *lexer = lexer_create();
  if (!lexer || !lexer_init(lexer)) {
    fprintf(stderr, "Failed to initialize lexer\n");
    free(source);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Tokenize input */
  printf("Tokenizing input...\n");
  if (!lexer_tokenize(lexer, source)) {
    fprintf(stderr, "Tokenization failed\n");
    lexer_destroy(lexer);
    free(source);
    return EXIT_FAILURE;
  }

  /* Print tokens if verbose */
  if (verbose) {
    printf("Tokenization result:\n");
    lexer_print_tokens(lexer);
  }

  /* Create parser */
  printf("Creating %s parser...\n", parser_type_to_string(parser_type));
  Parser *parser = parser_create(parser_type);
  if (!parser) {
    fprintf(stderr, "Failed to create parser\n");
    free(source);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Initialize parser */
  printf("Initializing parser...\n");
  if (!parser_init(parser)) {
    fprintf(stderr, "Failed to initialize parser\n");
    parser_destroy(parser);
    free(source);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Parse input */
  printf("Parsing input...\n");
  SyntaxTree *tree = parser_parse(parser, lexer);
  if (!tree) {
    fprintf(stderr, "Failed to parse input\n");
    parser_destroy(parser);
    free(source);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Print parsing result */
  printf("\nParsing successful!\n");

  /* Print syntax tree if enabled */
#ifdef CONFIG_OUTPUT_SYNTAX_TREE
  printf("\n");
  syntax_tree_print(tree);
#endif

  /* Print leftmost derivation if enabled */
#ifdef CONFIG_OUTPUT_LEFTMOST_DERIVATION
  printf("\n");
  parser_print_leftmost_derivation(parser);
#endif

  /* Clean up */
  // syntax_tree_destroy(tree);
  parser_destroy(parser);
  free(source);
  lexer_destroy(lexer);
  return EXIT_SUCCESS;
}
