/**
 * @file test_lexer.c
 * @brief Unit tests for the lexical analyzer
 */

#include "common.h"
#include "lexer_analyzer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global variables for test tracking */
TestSuite *current_suite = NULL;
Test *current_test = NULL;

/* Test function declarations */
static void test_lexer_create(void);
static void test_lexer_init(void);
static void test_tokenize_empty(void);
static void test_tokenize_spaces(void);
static void test_tokenize_identifiers(void);
static void test_tokenize_numbers(void);
static void test_tokenize_invalid_numbers(void);
static void test_tokenize_operators(void);
static void test_tokenize_keywords(void);
static void test_tokenize_mixed(void);
static void test_tokenize_example(void);

/* Test function implementations */
static void test_lexer_create(void) {
  /* Test lexer creation */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  lexer_destroy(lexer);
}

static void test_lexer_init(void) {
  /* Test lexer initialization */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");

  bool result = lexer_init(lexer);
  ASSERT(result, "Lexer initialization failed");
  ASSERT(lexer->initialized, "Lexer not marked as initialized");

  lexer_destroy(lexer);
}

static void test_tokenize_empty(void) {
  /* Test tokenizing an empty string */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  bool result = lexer_tokenize(lexer, "");
  ASSERT(result, "Tokenizing empty string failed");
  ASSERT_EQ(lexer->nr_token, 0, "Empty string should produce 0 tokens");

  lexer_destroy(lexer);
}

static void test_tokenize_spaces(void) {
  /* Test tokenizing whitespace */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  bool result = lexer_tokenize(lexer, "   \t\n   ");
  ASSERT(result, "Tokenizing whitespace failed");
  ASSERT_EQ(lexer->nr_token, 0, "Whitespace should produce 0 tokens");

  lexer_destroy(lexer);
}

static void test_tokenize_identifiers(void) {
  /* Test tokenizing identifiers */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  bool result = lexer_tokenize(lexer, "abc x y123 z");
  ASSERT(result, "Tokenizing identifiers failed");
  ASSERT_EQ(lexer->nr_token, 4, "Expected 4 identifier tokens");

  ASSERT_EQ(lexer->tokens[0].type, TK_IDN, "First token should be identifier");
  ASSERT_STR_EQ(lexer->tokens[0].str_val, "abc",
                "First identifier should be 'abc'");

  ASSERT_EQ(lexer->tokens[1].type, TK_IDN, "Second token should be identifier");
  ASSERT_STR_EQ(lexer->tokens[1].str_val, "x",
                "Second identifier should be 'x'");

  ASSERT_EQ(lexer->tokens[2].type, TK_IDN, "Third token should be identifier");
  ASSERT_STR_EQ(lexer->tokens[2].str_val, "y123",
                "Third identifier should be 'y123'");

  ASSERT_EQ(lexer->tokens[3].type, TK_IDN, "Fourth token should be identifier");
  ASSERT_STR_EQ(lexer->tokens[3].str_val, "z",
                "Fourth identifier should be 'z'");

  lexer_destroy(lexer);
}

static void test_tokenize_numbers(void) {
  /* Test tokenizing numbers */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  bool result = lexer_tokenize(lexer, "0 123 07 0x1F");
  ASSERT(result, "Tokenizing numbers failed");
  ASSERT_EQ(lexer->nr_token, 4, "Expected 4 number tokens");

  ASSERT_EQ(lexer->tokens[0].type, TK_DEC, "First token should be decimal");
  ASSERT_EQ(lexer->tokens[0].num_val, 0, "First decimal should be 0");

  ASSERT_EQ(lexer->tokens[1].type, TK_DEC, "Second token should be decimal");
  ASSERT_EQ(lexer->tokens[1].num_val, 123, "Second decimal should be 123");

  ASSERT_EQ(lexer->tokens[2].type, TK_OCT, "Third token should be octal");
  ASSERT_EQ(lexer->tokens[2].num_val, 7, "Third octal should be 7 (decimal)");

  ASSERT_EQ(lexer->tokens[3].type, TK_HEX,
            "Fourth token should be hexadecimal");
  ASSERT_EQ(lexer->tokens[3].num_val, 31, "Fourth hex should be 31 (decimal)");

  lexer_destroy(lexer);
}

static void test_tokenize_invalid_numbers(void) {
  /* Test tokenizing invalid numbers */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  bool result = lexer_tokenize(lexer, "09 0xAZ");
  ASSERT(result, "Tokenizing invalid numbers failed");
  ASSERT_EQ(lexer->nr_token, 2, "Expected 2 invalid number tokens");

  ASSERT_EQ(lexer->tokens[0].type, TK_ILOCT,
            "First token should be invalid octal");
  ASSERT_STR_EQ(lexer->tokens[0].str_val, "09",
                "First invalid octal should be '09'");

  ASSERT_EQ(lexer->tokens[1].type, TK_ILHEX,
            "Second token should be invalid hex");
  ASSERT_STR_EQ(lexer->tokens[1].str_val, "0xAZ",
                "Second invalid hex should be '0xAZ'");

  lexer_destroy(lexer);
}

static void test_tokenize_operators(void) {
  /* Test tokenizing operators */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  bool result = lexer_tokenize(lexer, "+ - * / > < = >= <= <> ( ) ;");
  ASSERT(result, "Tokenizing operators failed");
  ASSERT_EQ(lexer->nr_token, 13, "Expected 13 operator tokens");

  TokenType expected_types[] = {TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_GT,
                                TK_LT,  TK_EQ,  TK_GE,  TK_LE,  TK_NEQ,
                                TK_SLP, TK_SRP, TK_SEMI};

  for (int i = 0; i < 13; i++) {
    ASSERT_EQ(lexer->tokens[i].type, expected_types[i],
              "Operator type mismatch");
  }

  lexer_destroy(lexer);
}

static void test_tokenize_keywords(void) {
  /* Test tokenizing keywords */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  bool result = lexer_tokenize(lexer, "if then else while do begin end");
  ASSERT(result, "Tokenizing keywords failed");
  ASSERT_EQ(lexer->nr_token, 7, "Expected 7 keyword tokens");

  TokenType expected_types[] = {TK_IF, TK_THEN,  TK_ELSE, TK_WHILE,
                                TK_DO, TK_BEGIN, TK_END};

  for (int i = 0; i < 7; i++) {
    ASSERT_EQ(lexer->tokens[i].type, expected_types[i],
              "Keyword type mismatch");
  }

  lexer_destroy(lexer);
}

static void test_tokenize_mixed(void) {
  /* Test tokenizing a mixed expression */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  bool result = lexer_tokenize(lexer, "if x > 10 then begin y = x + 5; end");
  ASSERT(result, "Tokenizing mixed expression failed");
  ASSERT_EQ(lexer->nr_token, 13, "Expected 13 tokens");

  TokenType expected_types[] = {TK_IF,    TK_IDN,  TK_GT, TK_DEC, TK_THEN,
                                TK_BEGIN, TK_IDN,  TK_EQ, TK_IDN, TK_ADD,
                                TK_DEC,   TK_SEMI, TK_END};

  for (int i = 0; i < 13; i++) {
    ASSERT_EQ(lexer->tokens[i].type, expected_types[i], "Token type mismatch");
  }

  lexer_destroy(lexer);
}

static void test_tokenize_example(void) {
  /* Test the example from the project requirements */
  Lexer *lexer = lexer_create();
  ASSERT(lexer != NULL, "Lexer creation failed");
  ASSERT(lexer_init(lexer), "Lexer initialization failed");

  const char *example = "0 92+data>= 0x1f 09 ;\nwhile";
  bool result = lexer_tokenize(lexer, example);
  ASSERT(result, "Tokenizing example failed");

  ASSERT_EQ(lexer->nr_token, 9, "Expected 9 tokens from example");

  TokenType expected_types[] = {TK_DEC, TK_DEC,   TK_ADD,  TK_IDN,  TK_GE,
                                TK_HEX, TK_ILOCT, TK_SEMI, TK_WHILE};

  const char *expected_names[] = {"DEC", "DEC",   "ADD",  "IDN",  "GE",
                                  "HEX", "ILOCT", "SEMI", "WHILE"};

  for (int i = 0; i < lexer->nr_token; i++) {
    ASSERT_EQ(lexer->tokens[i].type, expected_types[i],
              "Example token type mismatch");
    ASSERT_STR_EQ(token_type_to_string(lexer->tokens[i].type),
                  expected_names[i], "Example token name mismatch");
  }

  lexer_destroy(lexer);
}

/**
 * Main function for running the tests
 */
int main(void) {
  /* Initialize test suite */
  TEST_SUITE_INIT(lexer);

  /* Add tests to suite */
  TEST_SUITE_ADD_TEST(lexer, test_lexer_create);
  TEST_SUITE_ADD_TEST(lexer, test_lexer_init);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_empty);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_spaces);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_identifiers);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_numbers);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_invalid_numbers);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_operators);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_keywords);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_mixed);
  TEST_SUITE_ADD_TEST(lexer, test_tokenize_example);

  /* Run the test suite */
  TEST_SUITE_RUN(lexer);

  return lexer_suite.failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
