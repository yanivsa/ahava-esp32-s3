/** Offline Ahava question bank adapted for the MAX35 handheld. */
#include "quiz_engine.h"
#include <Arduino.h>

namespace {
#include "generated_questions.inc"

constexpr size_t QUESTION_COUNT = sizeof(QUESTIONS) / sizeof(QUESTIONS[0]);
static size_t next_match[PROFILE_MAX][4] = {};

static bool valid_profile(WizardProfile_t profile) {
    return profile > PROFILE_NONE && profile < PROFILE_MAX;
}
}

bool quiz_validate_database(void) {
    bool ok = true;
    for (size_t i = 0; i < QUESTION_COUNT; ++i) {
        const Question_t &q = QUESTIONS[i];
        bool row_ok = valid_profile(q.target_profile) && q.subject_id >= 0 && q.subject_id <= 3 &&
                      q.correct_idx <= 3 && q.text && *q.text && q.feedback && *q.feedback;
        for (const char *answer : q.answers) if (!answer || !*answer) row_ok = false;
        for (size_t j = 0; j < i; ++j) if (QUESTIONS[j].id == q.id) row_ok = false;
        if (!row_ok) {
            Serial.printf("[QUIZ] Invalid question at index %u (id=%d)\n", (unsigned)i, q.id);
            ok = false;
        }
    }

    for (int profile = PROFILE_ORI; profile < PROFILE_MAX; ++profile) {
        for (int subject = 0; subject < 4; ++subject) {
            size_t count = 0;
            for (const Question_t &q : QUESTIONS) {
                if (q.target_profile == profile && q.subject_id == subject) ++count;
            }
            if (count < 3) {
                Serial.printf("[QUIZ] Missing coverage profile=%d subject=%d count=%u\n",
                              profile, subject, (unsigned)count);
                ok = false;
            }
        }
    }
    Serial.printf("[QUIZ] Database validation: %u profile-question rows, %s\n",
                  (unsigned)QUESTION_COUNT, ok ? "PASS" : "FAIL");
    return ok;
}

const Question_t* quiz_get_next_question(WizardProfile_t profile, int subject_id) {
    if (!valid_profile(profile) || subject_id < 0 || subject_id > 3) return nullptr;
    size_t count = 0;
    for (const Question_t &q : QUESTIONS) {
        if (q.target_profile == profile && q.subject_id == subject_id) ++count;
    }
    if (!count) return nullptr;

    size_t target = next_match[profile][subject_id]++ % count;
    for (const Question_t &q : QUESTIONS) {
        if (q.target_profile == profile && q.subject_id == subject_id && target-- == 0) {
            Serial.printf("[QUIZ] Question id=%d profile=%d subject=%d\n", q.id, (int)profile, subject_id);
            return &q;
        }
    }
    return nullptr;
}

size_t quiz_get_total_questions(void) {
    return QUESTION_COUNT;
}

size_t quiz_get_question_count(WizardProfile_t profile, int subject_id) {
    if (!valid_profile(profile) || subject_id < -1 || subject_id > 3) return 0;
    size_t count = 0;
    for (const Question_t &q : QUESTIONS) {
        if (q.target_profile == profile && (subject_id < 0 || q.subject_id == subject_id)) ++count;
    }
    return count;
}
