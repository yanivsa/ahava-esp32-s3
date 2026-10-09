#include <unity.h>
#include <stdint.h>

#if __has_include("exam_prep_contract.h")
#include "exam_prep_contract.h"
#else
static inline bool exam_prep_operation_valid(int) { return false; }
static inline bool exam_prep_tier_valid(uint8_t) { return false; }
#define EXAM_PREP_FRACTION_TIMES_WHOLE 1
#define EXAM_PREP_MIXED_TIMES_WHOLE 2
#endif

void test_only_two_multiplication_operations_are_valid() {
    TEST_ASSERT_TRUE(exam_prep_operation_valid(EXAM_PREP_FRACTION_TIMES_WHOLE));
    TEST_ASSERT_TRUE(exam_prep_operation_valid(EXAM_PREP_MIXED_TIMES_WHOLE));
    TEST_ASSERT_FALSE(exam_prep_operation_valid(0));
    TEST_ASSERT_FALSE(exam_prep_operation_valid(3));
    TEST_ASSERT_FALSE(exam_prep_operation_valid(99));
}

void test_only_tiers_one_through_seven_are_valid() {
    TEST_ASSERT_FALSE(exam_prep_tier_valid(0));
    for (uint8_t tier = 1; tier <= 7; ++tier) TEST_ASSERT_TRUE(exam_prep_tier_valid(tier));
    TEST_ASSERT_FALSE(exam_prep_tier_valid(8));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_only_two_multiplication_operations_are_valid);
    RUN_TEST(test_only_tiers_one_through_seven_are_valid);
    return UNITY_END();
}
