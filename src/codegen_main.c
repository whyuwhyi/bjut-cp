/**
 * @file codegen-main.c
 * @brief Driver program for three-address code generation
 */

#include "codegen/codegen.h"
#include "common.h"
#include "lexer_analyzer/lexer.h"
#include "parser/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Print usage information
 */
static void print_usage(const char *program_name) {
  printf("Usage: %s [options] <input_file>\n", program_name);
  printf("Options:\n");
  printf("  -o <output_file>  Output file (default: stdout)\n");
  printf("  -p <parser_type>  Parser type (rd, lr0, slr1, lr1; default: rd)\n");
  printf("  -h                Display this help message\n");
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
  char *input_filename = NULL;
  char *output_filename = NULL;
  ParserType parser_type = PARSER_TYPE_RECURSIVE_DESCENT;

  /* Parse command line arguments */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        output_filename = argv[++i];
      } else {
        fprintf(stderr, "Error: Missing output file\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
      }
    } else if (strcmp(argv[i], "-p") == 0) {
      if (i + 1 < argc) {
        const char *type_str = argv[++i];

        if (strcmp(type_str, "rd") == 0) {
          parser_type = PARSER_TYPE_RECURSIVE_DESCENT;
        } else if (strcmp(type_str, "lr0") == 0) {
          parser_type = PARSER_TYPE_LR0;
        } else if (strcmp(type_str, "slr1") == 0) {
          parser_type = PARSER_TYPE_SLR1;
        } else if (strcmp(type_str, "lr1") == 0) {
          parser_type = PARSER_TYPE_LR1;
        } else {
          fprintf(stderr, "Error: Unknown parser type: %s\n", type_str);
          print_usage(argv[0]);
          return EXIT_FAILURE;
        }
      } else {
        fprintf(stderr, "Error: Missing parser type\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
      }
    } else if (strcmp(argv[i], "-h") == 0) {
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    } else {
      /* Assume this is the input filename */
      if (!input_filename) {
        input_filename = argv[i];
      } else {
        fprintf(stderr, "Error: Multiple input files specified\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
      }
    }
  }

  /* Check for input file */
  if (!input_filename) {
    fprintf(stderr, "Error: No input file specified\n");
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  /* Create lexer */
  Lexer *lexer = lexer_create();
  if (!lexer) {
    fprintf(stderr, "Error: Failed to create lexer\n");
    return EXIT_FAILURE;
  }

  /* Initialize lexer */
  if (!lexer_init(lexer)) {
    fprintf(stderr, "Error: Failed to initialize lexer\n");
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Read input file */
  if (!lexer_read_file(lexer, input_filename)) {
    fprintf(stderr, "Error: Failed to read input file '%s'\n", input_filename);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Create parser */
  Parser *parser = parser_create(parser_type);
  if (!parser) {
    fprintf(stderr, "Error: Failed to create %s parser\n",
            parser_type_to_string(parser_type));
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Initialize parser */
  if (!parser_init(parser)) {
    fprintf(stderr, "Error: Failed to initialize parser\n");
    parser_destroy(parser);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Create code generator */
  CodeGenerator *codegen = codegen_create();
  if (!codegen) {
    fprintf(stderr, "Error: Failed to create code generator\n");
    parser_destroy(parser);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Initialize code generator */
  if (!codegen_init(codegen)) {
    fprintf(stderr, "Error: Failed to initialize code generator\n");
    codegen_destroy(codegen);
    parser_destroy(parser);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Generate three-address code */
  TACProgram *program = codegen_generate_from_source(codegen, lexer, parser);
  if (!program) {
    fprintf(stderr, "Error: Failed to generate three-address code\n");
    codegen_destroy(codegen);
    parser_destroy(parser);
    lexer_destroy(lexer);
    return EXIT_FAILURE;
  }

  /* Output three-address code */
  if (output_filename) {
    /* Write to file */
    if (!tac_program_write_to_file(program, output_filename)) {
      fprintf(stderr, "Error: Failed to write output to file '%s'\n",
              output_filename);
      tac_program_destroy(program);
      codegen_destroy(codegen);
      parser_destroy(parser);
      lexer_destroy(lexer);
      return EXIT_FAILURE;
    }

    printf("Three-address code written to '%s'\n", output_filename);
  } else {
    /* Print to stdout */
    tac_program_print(program);
  }

  /* Clean up */
  tac_program_destroy(program);
  codegen_destroy(codegen);
  parser_destroy(parser);
  lexer_destroy(lexer);

  return EXIT_SUCCESS;
}
