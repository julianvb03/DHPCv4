#include <CUnit/Basic.h>

void test_example() {
    CU_ASSERT(2 + 2 == 4);
}

int main() {
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Example_Test_Suite", 0, 0);

    CU_add_test(suite, "test_example", test_example);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}