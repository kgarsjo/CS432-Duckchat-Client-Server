#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#define true 0
#define false 1

int init_suite1(void) {
	return 0;
}

int clean_suite1(void) {
	return 0;
}

void testFunct(void) {
	CU_ASSERT(true == true);
}

void testFunct2(void) {
	CU_ASSERT(false == false);
}

int main() {
	CU_pSuite pSuite= NULL;
	
	if (CUE_SUCCESS != CU_initialize_registry()) {
		return CU_get_error();
	}

	pSuite= CU_add_suite("Suite1", init_suite1, clean_suite1);
	if (pSuite == NULL) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if ((NULL == CU_add_test(pSuite, "test1", testFunct)) ||
	(NULL == CU_add_test(pSuite, "test2", testFunct2))) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();
	return CU_get_error();
}
