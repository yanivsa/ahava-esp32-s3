#pragma once

#define AHAVA_SUBJECT_MATH 0
#define AHAVA_SUBJECT_HEBREW 1
#define AHAVA_SUBJECT_ENGLISH 2
#define AHAVA_SUBJECT_RELIGION 3
#define AHAVA_SUBJECT_CHALLENGES 4
#define AHAVA_SUBJECT_COUNT 5

static inline bool ahava_subject_valid(int subject_id) {
    return subject_id >= 0 && subject_id < AHAVA_SUBJECT_COUNT;
}
