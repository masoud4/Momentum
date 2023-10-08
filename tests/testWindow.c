#include <check.h>
#include "my_functions.c"

// Define your test cases
START_TEST(test_addition) {
        ck_assert_int_eq(add(2, 3), 5);
        ck_assert_int_eq(add(-1, 1), 0);
        ck_assert_int_eq(add(0, 0), 0);
} END_TEST

int main(void) {
    // Create a test suite
    Suite *s = suite_create("MyTestSuite");

    // Create a test case and add the test case to the suite
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_addition);
    suite_add_tcase(s, tc_core);

    // Create a test runner and run the tests
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);

    // Get the number of failures
    int num_failures = srunner_ntests_failed(sr);

    // Clean up and return exit code based on test results
    srunner_free(sr);
    return (num_failures == 0) ? 0 : 1;
}
