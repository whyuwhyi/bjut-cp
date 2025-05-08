/**
 * @file unittest.c
 * @brief Implementation of unit testing framework
 */

#include "unittest.h"

/**
 * Initialize a test suite
 */
void test_suite_init(TestSuite *suite, const char *name) {
  suite->name = name;
  suite->tests = (Test *)malloc(10 * sizeof(Test));
  suite->capacity = 10;
  suite->count = 0;
  suite->passed = 0;
  suite->failed = 0;
}

/**
 * Add a test to a test suite
 */
void test_suite_add_test(TestSuite *suite, const char *name, TestFunction func,
                         const char *file, int line) {
  /* Expand capacity if needed */
  if (suite->count >= suite->capacity) {
    suite->capacity *= 2;
    suite->tests =
        (Test *)realloc(suite->tests, suite->capacity * sizeof(Test));
  }

  /* Initialize the test */
  Test *test = &suite->tests[suite->count++];
  test->name = name;
  test->func = func;
  test->passed = true;
  test->file = file;
  test->line = line;
  test->message = NULL;
}

/**
 * Run all tests in a test suite
 */
void test_suite_run(TestSuite *suite) {
  printf("Running test suite: %s\n", suite->name);
  printf("====================================\n");

  /* Run each test in the suite */
  for (int i = 0; i < suite->count; i++) {
    Test *test = &suite->tests[i];

    // Set current test context
    TestSuite *old_suite = current_suite;
    Test *old_test = current_test;
    current_suite = suite;
    current_test = test;

    printf("  - Running test: %s... ", test->name);
    fflush(stdout); /* Ensure output is visible before test runs */

    test->func(); /* Run the test */

    if (test->passed) {
      printf("PASSED\n");
      suite->passed++;
    } else {
      printf("FAILED: %s:%d: %s\n", test->file, test->line, test->message);
      suite->failed++;
    }

    // Restore context
    current_suite = old_suite;
    current_test = old_test;
  }

  printf("====================================\n");
  printf("Test suite %s: %d tests, %d passed, %d failed\n\n", suite->name,
         suite->count, suite->passed, suite->failed);

  /* Free the tests array */
  free(suite->tests);
}

/**
 * Mark a test as failed
 */
void test_fail(const char *file, int line, const char *message) {
  if (current_test) {
    current_test->passed = false;
    current_test->file = file;
    current_test->line = line;
    current_test->message = message;
  }
}
