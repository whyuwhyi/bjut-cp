/**
 * @file test_token.c
 * @brief Unit tests for the token module
 */

#include "common.h"
#include "lexer_analyzer/token.h"
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
  Token token = token_create(TK_ADD);
  ASSERT_EQ(token.type, TK_ADD, "Token type should be TK_ADD");
}

static void test_token_create_num(void) {
  /* Test creating a token with numeric value */
  Token token = token_create_num(TK_DEC, 123);
  ASSERT_EQ(token.type, TK_DEC, "Token type should be TK_DEC");
  ASSERT_EQ(token.num_val, 123, "Token numeric value should be 123");

  token = token_create_num(TK_OCT, 7);
  ASSERT_EQ(token.type, TK_OCT, "Token type should be TK_OCT");
  ASSERT_EQ(token.num_val, 7, "Token numeric value should be 7");

  token = token_create_num(TK_HEX, 0x1F);
  ASSERT_EQ(token.type, TK_HEX, "Token type should be TK_HEX");
  ASSERT_EQ(token.num_val, 31, "Token numeric value should be 31");
}

static void test_token_create_str(void) {
  /* Test creating a token with string value */
  Token token = token_create_str(TK_IDN, "abc");
  ASSERT_EQ(token.type, TK_IDN, "Token type should be TK_IDN");
  ASSERT_STR_EQ(token.str_val, "abc", "Token string value should be 'abc'");

  token = token_create_str(TK_ILOCT, "09");
  ASSERT_EQ(token.type, TK_ILOCT, "Token type should be TK_ILOCT");
  ASSERT_STR_EQ(token.str_val, "09", "Token string value should be '09'");

  token = token_create_str(TK_ILHEX, "0xAZ");
  ASSERT_EQ(token.type, TK_ILHEX, "Token type should be TK_ILHEX");
  ASSERT_STR_EQ(token.str_val, "0xAZ", "Token string value should be '0xAZ'");

  /* Test with NULL string */
  token = token_create_str(TK_IDN, NULL);
  ASSERT_EQ(token.type, TK_IDN, "Token type should be TK_IDN");
  ASSERT_STR_EQ(token.str_val, "",
                "Token string value should be empty with NULL input");

  /* Test with very long string (should truncate) */
  char long_str[100];
  memset(long_str, 'a', sizeof(long_str));
  long_str[sizeof(long_str) - 1] = '\0';

  token = token_create_str(TK_IDN, long_str);
  ASSERT_EQ(token.type, TK_IDN, "Token type should be TK_IDN");
  ASSERT_EQ(strlen(token.str_val), CONFIG_MAX_TOKEN_LEN - 1,
            "Long string should be truncated");
}

static void test_token_to_string(void) {
  /* Test formatting tokens to strings */
  char buffer[64];

  /* Test numeric token */
  Token dec_token = token_create_num(TK_DEC, 123);
  token_to_string(&dec_token, buffer, sizeof(buffer));
  ASSERT_STR_EQ(buffer, "DEC        123",
                "Decimal token string format incorrect");

  /* Test string token */
  Token idn_token = token_create_str(TK_IDN, "abc");
  token_to_string(&idn_token, buffer, sizeof(buffer));
  ASSERT_STR_EQ(buffer, "IDN        abc",
                "Identifier token string format incorrect");

  /* Test operator token */
  Token op_token = token_create(TK_ADD);
  token_to_string(&op_token, buffer, sizeof(buffer));
  ASSERT_STR_EQ(buffer, "ADD        -",
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
