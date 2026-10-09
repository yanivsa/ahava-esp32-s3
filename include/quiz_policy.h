#pragma once

#include <stdint.h>
#include "subjects.h"

/** A question counts only when answered correctly before any hint/retry. */
static inline bool quiz_policy_should_count(bool is_correct, bool hint_used) {
    return is_correct && !hint_used;
}

/** Exam preparation is deliberate practice and never creates Time+ credit. */
static inline bool quiz_policy_grants_timeplus_credit(int subject_id) {
    return subject_id != AHAVA_SUBJECT_EXAM_PREP;
}

/** Only Judaism/Halacha is capped at ten counted questions per day. */
static inline uint32_t quiz_policy_daily_limit(int profile, int subject_id) {
    (void)profile;
    return subject_id == AHAVA_SUBJECT_RELIGION ? 10u : UINT32_MAX;
}

typedef enum {
    QUIZ_DAILY_KEEP = 0,
    QUIZ_DAILY_RESET,
    QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS
} QuizDailyBucketAction_t;

static inline QuizDailyBucketAction_t quiz_policy_daily_bucket_action(
    uint32_t today, uint32_t saved_date, bool has_unsynced_activity) {
    if (today == 0 || saved_date == today) return QUIZ_DAILY_KEEP;
    if (has_unsynced_activity) return QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS;
    return QUIZ_DAILY_RESET;
}
