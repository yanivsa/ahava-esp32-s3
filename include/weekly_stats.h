#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "subjects.h"

#define WEEKLY_STATS_DAYS 7
#define WEEKLY_STATS_SUBJECTS AHAVA_SUBJECT_COUNT
#define WEEKLY_STATS_VERSION 2u
#define WEEKLY_STATS_HAS_DATE_HELPER 1

typedef struct {
    uint32_t date; // YYYYMMDD; 0 means unused/unknown
    uint16_t correct[WEEKLY_STATS_SUBJECTS];
} WeeklyStatsDay_t;

typedef struct {
    uint32_t version;
    WeeklyStatsDay_t days[WEEKLY_STATS_DAYS]; // Newest date first
} WeeklyStats_t;

static inline bool weekly_stats_is_leap_year(uint32_t year) {
    return (year % 4u == 0u && year % 100u != 0u) || (year % 400u == 0u);
}

static inline uint8_t weekly_stats_days_in_month(uint32_t year, uint32_t month) {
    static const uint8_t DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1u || month > 12u) return 0;
    if (month == 2u && weekly_stats_is_leap_year(year)) return 29;
    return DAYS[month - 1u];
}

static inline uint32_t weekly_stats_previous_date(uint32_t date) {
    if (date == 0) return 0;
    uint32_t year = date / 10000u;
    uint32_t month = (date / 100u) % 100u;
    uint32_t day = date % 100u;
    if (year == 0 || month < 1u || month > 12u ||
        day < 1u || day > weekly_stats_days_in_month(year, month)) return 0;

    if (day > 1u) {
        --day;
    } else {
        if (month > 1u) {
            --month;
        } else {
            if (year <= 1u) return 0;
            --year;
            month = 12u;
        }
        day = weekly_stats_days_in_month(year, month);
    }
    return year * 10000u + month * 100u + day;
}

static inline void weekly_stats_clear(WeeklyStats_t *stats) {
    if (!stats) return;
    memset(stats, 0, sizeof(*stats));
    stats->version = WEEKLY_STATS_VERSION;
}

static inline int weekly_stats_find_date(const WeeklyStats_t *stats, uint32_t date) {
    if (!stats || date == 0) return -1;
    for (int i = 0; i < WEEKLY_STATS_DAYS; ++i) {
        if (stats->days[i].date == date) return i;
    }
    return -1;
}

static inline uint16_t weekly_stats_get_correct(const WeeklyStats_t *stats,
                                                 uint32_t date,
                                                 int subject_id) {
    if (!stats || date == 0 || subject_id < 0 || subject_id >= WEEKLY_STATS_SUBJECTS) return 0;
    const int idx = weekly_stats_find_date(stats, date);
    return idx >= 0 ? stats->days[idx].correct[subject_id] : 0;
}

static inline void weekly_stats_insert_empty_day(WeeklyStats_t *stats, uint32_t date) {
    if (!stats || date == 0 || weekly_stats_find_date(stats, date) >= 0) return;

    int insert_at = WEEKLY_STATS_DAYS;
    for (int i = 0; i < WEEKLY_STATS_DAYS; ++i) {
        if (stats->days[i].date == 0 || date > stats->days[i].date) {
            insert_at = i;
            break;
        }
    }

    // If the history is full and the new date is older than every stored date,
    // ignore it rather than evicting a newer day.
    if (insert_at >= WEEKLY_STATS_DAYS) return;

    for (int i = WEEKLY_STATS_DAYS - 1; i > insert_at; --i) {
        stats->days[i] = stats->days[i - 1];
    }
    memset(&stats->days[insert_at], 0, sizeof(stats->days[insert_at]));
    stats->days[insert_at].date = date;
}

static inline void weekly_stats_record_correct(WeeklyStats_t *stats,
                                                uint32_t date,
                                                int subject_id) {
    if (!stats || date == 0 || subject_id < 0 || subject_id >= WEEKLY_STATS_SUBJECTS) return;
    if (stats->version != WEEKLY_STATS_VERSION) weekly_stats_clear(stats);

    weekly_stats_insert_empty_day(stats, date);
    const int idx = weekly_stats_find_date(stats, date);
    if (idx < 0) return;

    uint16_t *value = &stats->days[idx].correct[subject_id];
    if (*value < UINT16_MAX) ++(*value);
}

static inline void weekly_stats_set_correct_floor(WeeklyStats_t *stats,
                                                   uint32_t date,
                                                   int subject_id,
                                                   uint16_t minimum_value) {
    if (!stats || date == 0 || subject_id < 0 || subject_id >= WEEKLY_STATS_SUBJECTS) return;
    if (stats->version != WEEKLY_STATS_VERSION) weekly_stats_clear(stats);

    weekly_stats_insert_empty_day(stats, date);
    const int idx = weekly_stats_find_date(stats, date);
    if (idx < 0) return;
    if (stats->days[idx].correct[subject_id] < minimum_value) {
        stats->days[idx].correct[subject_id] = minimum_value;
    }
}
