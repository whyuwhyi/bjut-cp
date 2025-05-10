/**
 * @file test_token.c
 * @brief Unit tests for the token module
 */
#include "../unittest.h"
#include "common.h"
#include "lexer/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global variables for test tracking */
TestSuite *current_suite = NULL;
Test *current_test = NULL;

/* Test function declarations */
static void test_token_create(void);
static void test_token_create_num(void);
static void test_token_create_str(void);
static void test_token_to_string(void);
static void test_token_type_to_string(void);

/* Test function implementations */
static void test_token_create(void) {
  /* Test creating a token with no value */
  Token token = token_create(TK_ADD, 1, 5);
  ASSERT_EQ(token.type, TK_ADD, "Token type should be TK_ADD");
  ASSERT_EQ(token.line, 1, "Token line should be 1");
  ASSERT_EQ(token.column, 5, "Token column should be 5");
}

static void test_token_create_num(void) {
  /* Test creating a token with numeric value */
  Token token = token_create_num(TK_DEC, 123, 2, 10);
  ASSERT_EQ(token.type, TK_DEC, "Token type should be TK_DEC");
  ASSERT_EQ(token.num_val, 123, "Token numeric value should be 123");
  ASSERT_EQ(token.line, 2, "Token line should be 2");
  ASSERT_EQ(token.column, 10, "Token column should be 10");

  token = token_create_num(TK_OCT, 7, 3, 15);
  ASSERT_EQ(token.type, TK_OCT, "Token type should be TK_OCT");
  ASSERT_EQ(token.num_val, 7, "Token numeric value should be 7");
  ASSERT_EQ(token.line, 3, "Token line should be 3");
  ASSERT_EQ(token.column, 15, "Token column should be 15");

  token = token_create_num(TK_HEX, 0x1F, 4, 20);
  ASSERT_EQ(token.type, TK_HEX, "Token type should be TK_HEX");
  ASSERT_EQ(token.num_val, 31, "Token numeric value should be 31");
  ASSERT_EQ(token.line, 4, "Token line should be 4");
  ASSERT_EQ(token.column, 20, "Token column should be 20");
}

static void test_token_create_str(void) {
  /* Test creating a token with string value */
  Token token = token_create_str(TK_IDN, "abc", 5, 25);
  ASSERT_EQ(token.type, TK_IDN, "Token type should be TK_IDN");
  ASSERT_STR_EQ(token.str_val, "abc", "Token string value should be 'abc'");
  ASSERT_EQ(token.line, 5, "Token line should be 5");
  ASSERT_EQ(token.column, 25, "Token column should be 25");

  token = token_create_str(TK_ILOCT, "09", 6, 30);
  ASSERT_EQ(token.type, TK_ILOCT, "Token type should be TK_ILOCT");
  ASSERT_STR_EQ(token.str_val, "09", "Token string value should be '09'");
  ASSERT_EQ(token.line, 6, "Token line should be 6");
  ASSERT_EQ(token.column, 30, "Token column should be 30");

  token = token_create_str(TK_ILHEX, "0xAZ", 7, 35);
  ASSERT_EQ(token.type, TK_ILHEX, "Token type should be TK_ILHEX");
  ASSERT_STR_EQ(token.str_val, "0xAZ", "Token string value should be '0xAZ'");
  ASSERT_EQ(token.line, 7, "Token line should be 7");
  ASSERT_EQ(token.column, 35, "Token column should be 35");

  /* Test with NULL string */
  token = token_create_str(TK_IDN, NULL, 8, 40);
  ASSERT_EQ(token.type, TK_IDN, "Token type should be TK_IDN");
  ASSERT_STR_EQ(token.str_val, "",
                "Token string value should be empty with NULL input");
  ASSERT_EQ(token.line, 8, "Token line should be 8");
  ASSERT_EQ(token.column, 40, "Token column should be 40");

  /* Test with very long string (should truncate) */
  char long_str[100];
  memset(long_str, 'a', sizeof(long_str));
  long_str[sizeof(long_str) - 1] = '\0';
  token = token_create_str(TK_IDN, long_str, 9, 45);
  ASSERT_EQ(token.type, TK_IDN, "Token type should be TK_IDN");
  ASSERT_EQ(strlen(token.str_val), CONFIG_MAX_TOKEN_LEN - 1,
            "Long string should be truncated");
  ASSERT_EQ(token.line, 9, "Token line should be 9");
  ASSERT_EQ(token.column, 45, "Token column should be 45");
}

static void test_token_to_string(void) {
  /* Test formatting tokens to strings */
  char buffer[128]; // Increased buffer size to accommodate line/column info

  /* Test numeric token */
  Token dec_token = token_create_num(TK_DEC, 123, 10, 50);
  token_to_string_famt(&dec_token, buffer, sizeof(buffer));
  ASSERT_TRUE(strstr(buffer, "DEC") != NULL && strstr(buffer, "123") != NULL,
              "Decimal token string format incorrect");

  /* Test string token */
  Token idn_token = token_create_str(TK_IDN, "abc", 11, 55);
  token_to_string_famt(&idn_token, buffer, sizeof(buffer));
  ASSERT_TRUE(strstr(buffer, "IDN") != NULL && strstr(buffer, "abc") != NULL,
              "Identifier token string format incorrect");

  /* Test operator token */
  Token op_token = token_create(TK_ADD, 12, 60);
  token_to_string_famt(&op_token, buffer, sizeof(buffer));
  ASSERT_TRUE(strstr(buffer, "ADD") != NULL,
              "Operator token string format incorrect");

  /* Test edge cases */
  token_to_string(NULL, buffer, sizeof(buffer));
  token_to_string(&dec_token, NULL, 0);
}

static void test_token_type_to_string(void) {
  /* Test converting token types to strings */
  ASSERT_STR_EQ(token_type_to_string(TK_ADD), "ADD", "TK_ADD string incorrect");
  ASSERT_STR_EQ(token_type_to_string(TK_IF), "IF", "TK_IF string incorrect");
  ASSERT_STR_EQ(token_type_to_string(TK_DEC), "DEC", "TK_DEC string incorrect");
  ASSERT_STR_EQ(token_type_to_string(TK_IDN), "IDN", "TK_IDN string incorrect");
  /* Test invalid token type */
  ASSERT_STR_EQ(token_type_to_string((TokenType)999), "UNKNOWN",
                "Invalid token type string incorrect");
}

/**
 * Main function for running the tests
 */
int main(void) {
  /* Initialize test suite */
  TEST_SUITE_INIT(token);

  /* Add tests to suite */
  TEST_SUITE_ADD_TEST(token, test_token_create);
  TEST_SUITE_ADD_TEST(token, test_token_create_num);
  TEST_SUITE_ADD_TEST(token, test_token_create_str);
  TEST_SUITE_ADD_TEST(token, test_token_to_string);
  TEST_SUITE_ADD_TEST(token, test_token_type_to_string);

  /* Run the test suite */
  TEST_SUITE_RUN(token);

  return token_suite.failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
