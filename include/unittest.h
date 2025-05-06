/**
 * @file unittest.h
 * @brief Simple unit testing framework for C programs
 *
 * This file provides a lightweight unit testing framework for C programs,
 * with test discovery, assertion macros, and test reporting.
 */

#ifndef UNITTEST_H
#define UNITTEST_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Test function type
 */
typedef void (*TestFunction)(void);

/**
 * @brief Test structure
 */
typedef struct {
  const char *name;
  TestFunction func;
  bool passed;
  const char *file;
  int line;
  const char *message;
} Test;

/**
 * @brief Test suite structure
 */
typedef struct {
  const char *name;
  Test *tests;
  int count;
  int capacity;
  int passed;
  int failed;
} TestSuite;

/**
 * @brief Initialize a test suite
 */
#define TEST_SUITE_INIT(name)                                                  \
  TestSuite name##_suite;                                                      \
  test_suite_init(&name##_suite, #name)

/**
 * @brief Add a test to a test suite
 */
#define TEST_SUITE_ADD_TEST(suite, test_func)                                  \
  test_suite_add_test(&suite##_suite, #test_func, test_func, __FILE__, __LINE__)

/**
 * @brief Run a test suite
 */
#define TEST_SUITE_RUN(suite) test_suite_run(&suite##_suite)

/**
 * @brief Initialize a test suite
 *
 * @param suite Pointer to the test suite
 * @param name Name of the test suite
 */
void test_suite_init(TestSuite *suite, const char *name);

/**
 * @brief Add a test to a test suite
 *
 * @param suite Pointer to the test suite
 * @param name Name of the test
 * @param func Test function
 * @param file Source file containing the test
 * @param line Line number in the source file
 */
void test_suite_add_test(TestSuite *suite, const char *name, TestFunction func,
                         const char *file, int line);

/**
 * @brief Run all tests in a test suite
 *
 * @param suite Pointer to the test suite
 */
void test_suite_run(TestSuite *suite);

/**
 * @brief Mark a test as failed
 *
 * @param file Source file containing the failure
 * @param line Line number in the source file
 * @param message Failure message
 */
void test_fail(const char *file, int line, const char *message);

/**
 * @brief Assert condition with message
 */
#define ASSERT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      test_fail(__FILE__, __LINE__, msg);                                      \
      return;                                                                  \
    }                                                                          \
  } while (0)

/**
 * @brief Assert equality with message
 */
#define ASSERT_EQ(a, b, msg) ASSERT((a) == (b), msg)

/**
 * @brief Assert inequality with message
 */
#define ASSERT_NE(a, b, msg) ASSERT((a) != (b), msg)

/**
 * @brief Assert true with message
 */
#define ASSERT_TRUE(a, msg) ASSERT((a), msg)

/**
 * @brief Assert false with message
 */
#define ASSERT_FALSE(a, msg) ASSERT(!(a), msg)

/**
 * @brief Assert string equality with message
 */
#define ASSERT_STR_EQ(a, b, msg) ASSERT(strcmp((a), (b)) == 0, msg)

/**
 * @brief Assert string inequality with message
 */
#define ASSERT_STR_NE(a, b, msg) ASSERT(strcmp((a), (b)) != 0, msg)

/**
 * @brief Assert float approximate equality with message
 */
#define ASSERT_FLOAT_EQ(a, b, epsilon, msg)                                    \
  ASSERT(fabs((a) - (b)) < (epsilon), msg)

/**
 * @brief Test suite context, used internally by each test file
 *
 * Each test file using this framework should define these global variables:
 * static TestSuite *current_suite = NULL;
 * static Test *current_test = NULL;
 */
extern TestSuite *current_suite;
extern Test *current_test;

#endif /* UNITTEST_H */
