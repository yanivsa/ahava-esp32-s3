#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "screen_manager.h"

typedef struct {
    WizardProfile_t profile;
    uint32_t date;
    uint8_t subject;
    uint16_t correct_first_try;
} ResultSyncRecord_t;

/** Increment one dated first-try-correct aggregate and mark it pending cloud sync. */
bool results_sync_store_record_correct(WizardProfile_t profile,
                                       uint32_t date,
                                       int subject);

/** Raise stored aggregates to known historical floors (migration / delayed time sync). */
bool results_sync_store_set_floor(WizardProfile_t profile,
                                  uint32_t date,
                                  const uint16_t correct[4]);

/** Collect up to max_records pending aggregate rows across Ori and Eitan. */
size_t results_sync_store_collect_dirty(ResultSyncRecord_t *out,
                                        size_t max_records);

/** Mark exactly the sent values as acknowledged; newer local increments remain dirty. */
void results_sync_store_ack(const ResultSyncRecord_t *records,
                            size_t count);
