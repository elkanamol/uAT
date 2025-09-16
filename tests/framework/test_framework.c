/**
 * @file test_framework.c
 * @brief Implementation of simple unit test framework
 */

#include "test_framework.h"

// Global test statistics
int test_total = 0;
int test_passed = 0;
int test_failed = 0;

void test_framework_init(void)
{
    test_total = 0;
    test_passed = 0;
    test_failed = 0;
}

void test_framework_summary(void)
{
    printf("\n========== TEST SUMMARY ==========\n");
    printf("Total tests: %d\n", test_total);
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("Success rate: %.1f%%\n", 
           test_total > 0 ? (100.0 * test_passed / test_total) : 0.0);
    printf("==================================\n");
}

int test_framework_get_result(void)
{
    return test_failed == 0 ? 0 : 1;
}