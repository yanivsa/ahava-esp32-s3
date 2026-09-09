#include <unity.h>
#include "quiz_policy.h"

void test_only_first_try_correct_counts() {
    TEST_ASSERT_TRUE(quiz_policy_should_count(true, false));
    TEST_ASSERT_FALSE(quiz_policy_should_count(false, false));
    TEST_ASSERT_FALSE(quiz_policy_should_count(true, true));
    TEST_ASSERT_FALSE(quiz_policy_should_count(false, true));
}

void test_ori_only_math_is_unlimited() {
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(1, 0));
    TEST_ASSERT_EQUAL_UINT32(10, quiz_policy_daily_limit(1, 1));
    TEST_ASSERT_EQUAL_UINT32(10, quiz_policy_daily_limit(1, 2));
    TEST_ASSERT_EQUAL_UINT32(10, quiz_policy_daily_limit(1, 3));

    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(2, 0));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(2, 1));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(2, 2));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(2, 3));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(3, 0));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(3, 1));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(3, 2));
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(3, 3));
}

void test_valid_date_rollover_resets_only_without_unsynced_activity() {
    TEST_ASSERT_EQUAL(QUIZ_DAILY_KEEP,
                      quiz_policy_daily_bucket_action(0, 20260908, false));
    TEST_ASSERT_EQUAL(QUIZ_DAILY_KEEP,
                      quiz_policy_daily_bucket_action(20260909, 20260909, false));
    TEST_ASSERT_EQUAL(QUIZ_DAILY_RESET,
                      quiz_policy_daily_bucket_action(20260909, 20260908, false));
    TEST_ASSERT_EQUAL(QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS,
                      quiz_policy_daily_bucket_action(20260909, 20260908, true));
    TEST_ASSERT_EQUAL(QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS,
                      quiz_policy_daily_bucket_action(20260909, 0, true));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_only_first_try_correct_counts);
    RUN_TEST(test_ori_only_math_is_unlimited);
    RUN_TEST(test_valid_date_rollover_resets_only_without_unsynced_activity);
    return UNITY_END();
}
