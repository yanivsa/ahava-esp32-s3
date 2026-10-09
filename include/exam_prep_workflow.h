#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EXAM_WORK_1 = 1,
    EXAM_WORK_2 = 2,
    EXAM_WORK_FINAL = 3
} ExamPrepWorkPhase_t;

typedef struct {
    ExamPrepWorkPhase_t phase;
    uint8_t correct1;
    uint8_t correct2;
    bool has_second;
} ExamPrepWorkState_t;

static inline void exam_prep_workflow_reset(ExamPrepWorkState_t *state,
                                            uint8_t correct1,
                                            bool has_second,
                                            uint8_t correct2) {
    if (!state) return;
    state->phase = EXAM_WORK_1;
    state->correct1 = correct1;
    state->correct2 = correct2;
    state->has_second = has_second;
}

static inline bool exam_prep_workflow_choose(ExamPrepWorkState_t *state, uint8_t selected) {
    if (!state) return false;
    if (state->phase == EXAM_WORK_1) {
        if (selected != state->correct1) return false;
        state->phase = state->has_second ? EXAM_WORK_2 : EXAM_WORK_FINAL;
        return true;
    }
    if (state->phase == EXAM_WORK_2) {
        if (selected != state->correct2) return false;
        state->phase = EXAM_WORK_FINAL;
        return true;
    }
    return false;
}
