#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "screen_manager.h"
#include "weekly_stats.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Archive the currently persisted daily buckets before normal UI access can roll them over. */
bool weekly_stats_store_init(void);

/** Merge the current NVS daily counters into seven-day history. Safe to call repeatedly. */
void weekly_stats_store_capture_profile(WizardProfile_t profile);
void weekly_stats_store_capture_all(void);

/** Load raw retained activity history for a profile. */
bool weekly_stats_store_get_profile(WizardProfile_t profile, WeeklyStats_t *out_stats);

/**
 * Build exactly seven calendar rows, newest first. Missing activity days are zero-filled.
 * Returns false only when no trusted or persisted date is available yet.
 */
bool weekly_stats_store_get_last_seven(WizardProfile_t profile,
                                       WeeklyStatsDay_t out_days[WEEKLY_STATS_DAYS]);

#ifdef __cplusplus
}
#endif
