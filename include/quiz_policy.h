#pragma once

#include <stdint.h>

/**
 * A question counts only when the current display was answered correctly
 * before any hint/retry was used.
 */
static inline bool quiz_policy_should_count(bool is_correct, bool hint_used) {
    return is_correct && !hint_used;
}

/**
 * Ori's daily subject caps: mathematics (subject 0) is unlimited; Hebrew/
 * language, English, and Judaism/Halacha (subjects 1-3) are capped at 10
 * counted questions per day. Other profiles remain unlimited.
 */
static inline uint32_t quiz_policy_daily_limit(int profile, int subject_id) {
    if (profile == 1 && subject_id >= 1 && subject_id <= 3) return 10u;
    return UINT32_MAX;
}

typedef enum {
    QUIZ_DAILY_KEEP = 0,
    QUIZ_DAILY_RESET,
    QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS
} QuizDailyBucketAction_t;

/**
 * Decide how to reconcile persisted daily counters with a newly available
 * trusted calendar date.
 *
 * If questions were counted while the clock was unsynchronized, resetting
 * them when SNTP later supplies a different date could allow a child to exceed
 * the real daily cap. In that case we conservatively adopt the trusted date
 * while preserving the already-counted activity.
 */
static inline QuizDailyBucketAction_t quiz_policy_daily_bucket_action(
    uint32_t today, uint32_t saved_date, bool has_unsynced_activity) {
    if (today == 0 || saved_date == today) return QUIZ_DAILY_KEEP;
    if (has_unsynced_activity) return QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS;
    return QUIZ_DAILY_RESET;
}
