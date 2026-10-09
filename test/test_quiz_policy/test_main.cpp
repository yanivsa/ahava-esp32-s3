#include <unity.h>
#include "quiz_policy.h"
#include "subjects.h"

void test_only_first_try_correct_counts() {
    TEST_ASSERT_TRUE(quiz_policy_should_count(true, false));
    TEST_ASSERT_FALSE(quiz_policy_should_count(false, false));
    TEST_ASSERT_FALSE(quiz_policy_should_count(true, true));
    TEST_ASSERT_FALSE(quiz_policy_should_count(false, true));
}

void test_only_religion_has_daily_cap() {
    for (int p = 1; p <= 3; ++p) {
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_MATH));
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_HEBREW));
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_ENGLISH));
        TEST_ASSERT_EQUAL_UINT32(10u, quiz_policy_daily_limit(p, AHAVA_SUBJECT_RELIGION));
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_CHALLENGES));
#ifdef AHAVA_SUBJECT_EXAM_PREP
        TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, quiz_policy_daily_limit(p, AHAVA_SUBJECT_EXAM_PREP));
        TEST_ASSERT_FALSE(quiz_policy_grants_timeplus_credit(AHAVA_SUBJECT_EXAM_PREP));
#endif
    }
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
    RUN_TEST(test_only_religion_has_daily_cap);
    RUN_TEST(test_valid_date_rollover_resets_only_without_unsynced_activity);
    return UNITY_END();
}
