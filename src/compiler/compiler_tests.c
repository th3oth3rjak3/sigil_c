#include "unity.h"

void
setUp() {}

void
tearDown() {}

void
test_add_works(void) {
    TEST_ASSERT_EQUAL(2, 4 - 2);
}

int
main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_add_works);
    return UNITY_END();
}
