/**
 * @file test_framework.h
 * @brief Simple unit test framework for uAT library
 * 
 * This provides a minimal test framework that can run without external dependencies
 * to test the uAT library functionality.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Test statistics
extern int test_total;
extern int test_passed;
extern int test_failed;

// Macros for test assertions
#define TEST_ASSERT(condition, description) \
    do { \
        test_total++; \
        if (condition) { \
            test_passed++; \
            printf("PASS: %s\n", description); \
        } else { \
            test_failed++; \
            printf("FAIL: %s (line %d)\n", description, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual, description) \
    do { \
        test_total++; \
        if ((expected) == (actual)) { \
            test_passed++; \
            printf("PASS: %s\n", description); \
        } else { \
            test_failed++; \
            printf("FAIL: %s - Expected: %d, Got: %d (line %d)\n", description, expected, actual, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual, description) \
    do { \
        test_total++; \
        if (strcmp(expected, actual) == 0) { \
            test_passed++; \
            printf("PASS: %s\n", description); \
        } else { \
            test_failed++; \
            printf("FAIL: %s - Expected: '%s', Got: '%s' (line %d)\n", description, expected, actual, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition, description) \
    TEST_ASSERT((condition) == true, description)

#define TEST_ASSERT_FALSE(condition, description) \
    TEST_ASSERT((condition) == false, description)

#define TEST_ASSERT_NULL(pointer, description) \
    TEST_ASSERT((pointer) == NULL, description)

#define TEST_ASSERT_NOT_NULL(pointer, description) \
    TEST_ASSERT((pointer) != NULL, description)

// Test suite management
#define TEST_SUITE_START(name) \
    printf("\n=== Starting Test Suite: %s ===\n", name)

#define TEST_SUITE_END(name) \
    printf("=== Finished Test Suite: %s ===\n", name); \
    printf("Results: %d passed, %d failed, %d total\n\n", test_passed, test_failed, test_total)

// Test result reporting
void test_framework_init(void);
void test_framework_summary(void);
int test_framework_get_result(void);

#endif // TEST_FRAMEWORK_H