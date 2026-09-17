#include <unity.h>
#include <stdint.h>

#if __has_include("weekly_stats.h")
#include "weekly_stats.h"
#define AHAVA_HAS_WEEKLY_STATS 1
#else
#define AHAVA_HAS_WEEKLY_STATS 0
#endif

void test_weekly_stats_feature_exists() {
#if AHAVA_HAS_WEEKLY_STATS
    TEST_ASSERT_TRUE(true);
#else
    TEST_FAIL_MESSAGE("weekly_stats.h is not implemented yet");
#endif
}

#if AHAVA_HAS_WEEKLY_STATS
void test_records_correct_answers_by_day_and_subject() {
    WeeklyStats_t stats{};
    weekly_stats_clear(&stats);

    weekly_stats_record_correct(&stats, 20260917u, 0);
    weekly_stats_record_correct(&stats, 20260917u, 0);
    weekly_stats_record_correct(&stats, 20260917u, 3);

    TEST_ASSERT_EQUAL_UINT16(2, weekly_stats_get_correct(&stats, 20260917u, 0));
    TEST_ASSERT_EQUAL_UINT16(1, weekly_stats_get_correct(&stats, 20260917u, 3));
    TEST_ASSERT_EQUAL_UINT16(0, weekly_stats_get_correct(&stats, 20260917u, 1));
}

void test_keeps_only_seven_most_recent_activity_dates() {
    WeeklyStats_t stats{};
    weekly_stats_clear(&stats);

    const uint32_t dates[] = {
        20260910u, 20260911u, 20260912u, 20260913u,
        20260914u, 20260915u, 20260916u, 20260917u
    };
    for (uint32_t date : dates) {
        weekly_stats_record_correct(&stats, date, 1);
    }

    TEST_ASSERT_EQUAL_UINT16(0, weekly_stats_get_correct(&stats, 20260910u, 1));
    for (size_t i = 1; i < sizeof(dates) / sizeof(dates[0]); ++i) {
        TEST_ASSERT_EQUAL_UINT16(1, weekly_stats_get_correct(&stats, dates[i], 1));
    }
}

void test_out_of_order_date_does_not_evict_newer_history() {
    WeeklyStats_t stats{};
    weekly_stats_clear(&stats);

    for (uint32_t d = 20260911u; d <= 20260917u; ++d) {
        weekly_stats_record_correct(&stats, d, 2);
    }
    weekly_stats_record_correct(&stats, 20260909u, 2);

    TEST_ASSERT_EQUAL_UINT16(0, weekly_stats_get_correct(&stats, 20260909u, 2));
    TEST_ASSERT_EQUAL_UINT16(1, weekly_stats_get_correct(&stats, 20260911u, 2));
    TEST_ASSERT_EQUAL_UINT16(1, weekly_stats_get_correct(&stats, 20260917u, 2));
}

void test_invalid_subject_or_date_is_ignored() {
    WeeklyStats_t stats{};
    weekly_stats_clear(&stats);

    weekly_stats_record_correct(&stats, 0u, 0);
    weekly_stats_record_correct(&stats, 20260917u, -1);
    weekly_stats_record_correct(&stats, 20260917u, 4);

    for (int subject = 0; subject < WEEKLY_STATS_SUBJECTS; ++subject) {
        TEST_ASSERT_EQUAL_UINT16(0, weekly_stats_get_correct(&stats, 20260917u, subject));
    }
}
#endif

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_weekly_stats_feature_exists);
#if AHAVA_HAS_WEEKLY_STATS
    RUN_TEST(test_records_correct_answers_by_day_and_subject);
    RUN_TEST(test_keeps_only_seven_most_recent_activity_dates);
    RUN_TEST(test_out_of_order_date_does_not_evict_newer_history);
    RUN_TEST(test_invalid_subject_or_date_is_ignored);
#endif
    return UNITY_END();
}
