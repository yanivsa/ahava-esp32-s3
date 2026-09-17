#include "weekly_stats_store.h"
#include "player_data.h"

#include <Arduino.h>
#include "nvs.h"
#include "esp_err.h"
#include <string.h>

#define NVS_STORAGE_NAMESPACE "wizard_nvs"

namespace {

static bool valid_profile(WizardProfile_t profile) {
    return profile > PROFILE_NONE && profile < PROFILE_MAX;
}

static void make_daily_date_key(char *buf, size_t len, WizardProfile_t profile) {
    snprintf(buf, len, "d_%d", (int)profile);
}

static void make_daily_subject_key(char *buf, size_t len,
                                   WizardProfile_t profile, int subject_id) {
    snprintf(buf, len, "qs%d_%d", (int)profile, subject_id);
}

static void make_history_key(char *buf, size_t len, WizardProfile_t profile) {
    snprintf(buf, len, "wh_%d", (int)profile);
}

static uint32_t read_u32(const char *key, uint32_t default_value) {
    nvs_handle_t handle;
    if (nvs_open(NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return default_value;
    }
    uint32_t value = default_value;
    esp_err_t err = nvs_get_u32(handle, key, &value);
    nvs_close(handle);
    return err == ESP_OK ? value : default_value;
}

static bool load_history(WizardProfile_t profile, WeeklyStats_t *out_stats) {
    if (!out_stats || !valid_profile(profile)) return false;
    weekly_stats_clear(out_stats);

    nvs_handle_t handle;
    if (nvs_open(NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    char key[16];
    make_history_key(key, sizeof(key), profile);
    size_t size = sizeof(*out_stats);
    esp_err_t err = nvs_get_blob(handle, key, out_stats, &size);
    nvs_close(handle);

    if (err != ESP_OK || size != sizeof(*out_stats) ||
        out_stats->version != WEEKLY_STATS_VERSION) {
        weekly_stats_clear(out_stats);
        return false;
    }
    return true;
}

static bool save_history(WizardProfile_t profile, const WeeklyStats_t *stats) {
    if (!stats || !valid_profile(profile)) return false;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    char key[16];
    make_history_key(key, sizeof(key), profile);
    err = nvs_set_blob(handle, key, stats, sizeof(*stats));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

static uint32_t saved_daily_date(WizardProfile_t profile) {
    char key[16];
    make_daily_date_key(key, sizeof(key), profile);
    return read_u32(key, 0);
}

static uint32_t saved_subject_count(WizardProfile_t profile, int subject_id) {
    char key[16];
    make_daily_subject_key(key, sizeof(key), profile, subject_id);
    return read_u32(key, 0);
}

static bool merge_saved_daily_bucket(WizardProfile_t profile) {
    if (!valid_profile(profile)) return false;

    const uint32_t date = saved_daily_date(profile);
    if (date == 0) return false;

    WeeklyStats_t stats{};
    load_history(profile, &stats);
    WeeklyStats_t before = stats;

    for (int subject = 0; subject < WEEKLY_STATS_SUBJECTS; ++subject) {
        uint32_t count = saved_subject_count(profile, subject);
        if (count > UINT16_MAX) count = UINT16_MAX;
        weekly_stats_set_correct_floor(&stats, date, subject, (uint16_t)count);
    }

    if (memcmp(&before, &stats, sizeof(stats)) == 0) return true;
    if (!save_history(profile, &stats)) {
        Serial.printf("[STATS] WARN: failed to persist weekly history for profile %d.\n",
                      (int)profile);
        return false;
    }

    Serial.printf("[STATS] Profile %d archived daily bucket %u.\n",
                  (int)profile, (unsigned)date);
    return true;
}

} // namespace

bool weekly_stats_store_init(void) {
    bool ok = true;
    // This must run immediately after player_data_init(), before dashboard getters
    // can rotate the daily bucket at a date boundary.
    for (int p = (int)PROFILE_ORI; p < (int)PROFILE_MAX; ++p) {
        const uint32_t date = saved_daily_date((WizardProfile_t)p);
        if (date != 0 && !merge_saved_daily_bucket((WizardProfile_t)p)) ok = false;
    }
    Serial.println("[STATS] Seven-day history store initialized.");
    return ok;
}

void weekly_stats_store_capture_profile(WizardProfile_t profile) {
    if (!valid_profile(profile)) return;
    merge_saved_daily_bucket(profile);
}

void weekly_stats_store_capture_all(void) {
    for (int p = (int)PROFILE_ORI; p < (int)PROFILE_MAX; ++p) {
        weekly_stats_store_capture_profile((WizardProfile_t)p);
    }
}

bool weekly_stats_store_get_profile(WizardProfile_t profile, WeeklyStats_t *out_stats) {
    if (!valid_profile(profile) || !out_stats) return false;
    weekly_stats_store_capture_profile(profile);
    return load_history(profile, out_stats);
}

bool weekly_stats_store_get_last_seven(WizardProfile_t profile,
                                       WeeklyStatsDay_t out_days[WEEKLY_STATS_DAYS]) {
    if (!valid_profile(profile) || !out_days) return false;

    weekly_stats_store_capture_profile(profile);

    WeeklyStats_t history{};
    load_history(profile, &history);

    uint32_t anchor = player_data_get_current_date();
    if (anchor == 0) anchor = saved_daily_date(profile);
    if (anchor == 0 && history.days[0].date != 0) anchor = history.days[0].date;

    memset(out_days, 0, sizeof(WeeklyStatsDay_t) * WEEKLY_STATS_DAYS);
    if (anchor == 0) return false;

    uint32_t date = anchor;
    for (int day = 0; day < WEEKLY_STATS_DAYS; ++day) {
        out_days[day].date = date;
        for (int subject = 0; subject < WEEKLY_STATS_SUBJECTS; ++subject) {
            out_days[day].correct[subject] =
                weekly_stats_get_correct(&history, date, subject);
        }
        date = weekly_stats_previous_date(date);
        if (date == 0 && day + 1 < WEEKLY_STATS_DAYS) break;
    }
    return true;
}
