#include <unity.h>
#include "quiz_policy.h"

void test_only_first_try_correct_counts() {
    TEST_ASSERT_TRUE(quiz_policy_should_count(true, false));
    TEST_ASSERT_FALSE(quiz_policy_should_count(false, false));
    TEST_ASSERT_FALSE(quiz_policy_should_count(true, true));
    TEST_ASSERT_FALSE(quiz_policy_should_count(false, true));
}

void test_only_ori_judaism_has_daily_limit() {
    TEST_ASSERT_EQUAL_UINT32(10, quiz_policy_daily_limit(1, 3));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(1, 0));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(1, 1));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(1, 2));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(2, 3));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(3, 3));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_only_first_try_correct_counts);
    RUN_TEST(test_only_ori_judaism_has_daily_limit);
    return UNITY_END();
}
