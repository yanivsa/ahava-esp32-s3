/**
 * @file player_data.cpp
 * @brief Wizard Academy Persistent Player Data Implementation using ESP-IDF NVS
 */

#include "player_data.h"
#include "quiz_policy.h"
#include <Arduino.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"
#include <limits.h>
#include <string.h>
#include <time.h>

#define NVS_STORAGE_NAMESPACE "wizard_nvs"
#define ISRAEL_TZ "IST-2IDT,M3.4.4/26,M10.5.0"
#define RETRY_MAX_QUESTIONS 256
#define RETRY_WORDS (RETRY_MAX_QUESTIONS / 32)

namespace {

struct RetryBits {
    uint32_t words[RETRY_WORDS];
};

static bool pending_count_valid = false;
static bool pending_count_eligible = false;
static WizardProfile_t pending_count_profile = PROFILE_NONE;
static int pending_count_subject = -1;

static uint32_t nvs_read_u32_val(const char *key, uint32_t default_val) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return default_val;

    uint32_t value = default_val;
    err = nvs_get_u32(handle, key, &value);
    nvs_close(handle);
    return (err == ESP_OK) ? value : default_val;
}

static bool nvs_write_u32_val(const char *key, uint32_t val) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        Serial.printf("[NVS] ERROR: Failed to open NVS for writing (err=0x%x)\n", err);
        return false;
    }

    err = nvs_set_u32(handle, key, val);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

static bool nvs_read_retry_bits(const char *key, RetryBits *bits) {
    if (!bits) return false;
    memset(bits, 0, sizeof(*bits));

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return false;

    size_t size = sizeof(*bits);
    err = nvs_get_blob(handle, key, bits, &size);
    nvs_close(handle);
    return err == ESP_OK && size == sizeof(*bits);
}

static bool nvs_write_retry_bits(const char *key, const RetryBits &bits) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    err = nvs_set_blob(handle, key, &bits, sizeof(bits));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

static bool valid_profile(WizardProfile_t profile) {
    return profile > PROFILE_NONE && profile < PROFILE_MAX;
}

static bool valid_subject(int subject_id) {
    return subject_id >= 0 && subject_id < 4;
}

static uint32_t saturating_add(uint32_t a, uint32_t b) {
    return (UINT32_MAX - a < b) ? UINT32_MAX : a + b;
}

static void make_subject_key(char *buf, size_t len, const char *prefix,
                             WizardProfile_t profile, int subject_id) {
    snprintf(buf, len, "%s%d_%d", prefix, (int)profile, subject_id);
}

static void clear_unsynced_counts(WizardProfile_t profile) {
    char key[16];
    for (int subject = 0; subject < 4; ++subject) {
        make_subject_key(key, sizeof(key), "uq", profile, subject);
        nvs_write_u32_val(key, 0);
    }
}

static uint32_t get_unsynced_total(WizardProfile_t profile) {
    char key[16];
    uint32_t total = 0;
    for (int subject = 0; subject < 4; ++subject) {
        make_subject_key(key, sizeof(key), "uq", profile, subject);
        total = saturating_add(total, nvs_read_u32_val(key, 0));
    }
    return total;
}

static void reset_daily_counts(WizardProfile_t profile, uint32_t today) {
    char key[16];

    snprintf(key, sizeof(key), "d_%d", (int)profile);
    nvs_write_u32_val(key, today);

    snprintf(key, sizeof(key), "qt_%d", (int)profile);
    nvs_write_u32_val(key, 0);

    for (int subject = 0; subject < 4; ++subject) {
        make_subject_key(key, sizeof(key), "qs", profile, subject);
        nvs_write_u32_val(key, 0);
    }
    clear_unsynced_counts(profile);

    Serial.printf("[NVS] Profile %d: new valid day %u -> daily counters reset.\n",
                  (int)profile, today);
}

static void restore_unsynced_counts_for_date(WizardProfile_t profile, uint32_t today) {
    char key[16];
    uint32_t total = 0;

    for (int subject = 0; subject < 4; ++subject) {
        make_subject_key(key, sizeof(key), "uq", profile, subject);
        const uint32_t unsynced = nvs_read_u32_val(key, 0);

        make_subject_key(key, sizeof(key), "qs", profile, subject);
        nvs_write_u32_val(key, unsynced);
        total = saturating_add(total, unsynced);
    }

    snprintf(key, sizeof(key), "qt_%d", (int)profile);
    nvs_write_u32_val(key, total);
    snprintf(key, sizeof(key), "d_%d", (int)profile);
    nvs_write_u32_val(key, today);
    clear_unsynced_counts(profile);

    Serial.printf("[NVS] Profile %d: trusted date %u adopted; preserved %u unsynced counted questions.\n",
                  (int)profile, today, (unsigned)total);
}

static void ensure_daily_bucket_for_date(WizardProfile_t profile, uint32_t today) {
    if (!valid_profile(profile) || today == 0) return;

    char date_key[16];
    snprintf(date_key, sizeof(date_key), "d_%d", (int)profile);
    const uint32_t saved_date = nvs_read_u32_val(date_key, 0);
    const uint32_t unsynced_total = get_unsynced_total(profile);
    const QuizDailyBucketAction_t action = quiz_policy_daily_bucket_action(
        today, saved_date, unsynced_total > 0);

    if (action == QUIZ_DAILY_RESET) {
        reset_daily_counts(profile, today);
    } else if (action == QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS) {
        // The persisted counters may include an older day's activity. Restore
        // only the eligible questions counted while the clock was untrusted.
        restore_unsynced_counts_for_date(profile, today);
    } else if (saved_date == today && unsynced_total > 0) {
        // The clock came back and confirms the same day. The regular counters
        // already include the unsynced activity, so only the temporary deltas
        // need to be cleared.
        clear_unsynced_counts(profile);
        Serial.printf("[NVS] Profile %d: trusted date confirms current bucket %u.\n",
                      (int)profile, today);
    }
}

static void ensure_daily_bucket(WizardProfile_t profile) {
    ensure_daily_bucket_for_date(profile, player_data_get_current_date());
}

}  // namespace

bool player_data_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Serial.println("[NVS] WARN: NVS partition corrupted or updated. Formatting...");
        esp_err_t erase_err = nvs_flash_erase();
        if (erase_err != ESP_OK) {
            Serial.printf("[NVS] ERROR: nvs_flash_erase failed (0x%x)\n", erase_err);
            return false;
        }
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        Serial.printf("[NVS] FATAL: nvs_flash_init failed (0x%x)\n", err);
        return false;
    }

    Serial.println("[NVS] Non-Volatile Storage initialized successfully.");
    return true;
}

uint32_t player_data_get_coins(WizardProfile_t profile) {
    if (!valid_profile(profile)) return 0;
    char key[16];
    snprintf(key, sizeof(key), "c_%d", (int)profile);
    return nvs_read_u32_val(key, 0);
}

void player_data_add_coins(WizardProfile_t profile, uint32_t amount) {
    if (!valid_profile(profile)) return;
    char key[16];
    snprintf(key, sizeof(key), "c_%d", (int)profile);

    uint32_t current = nvs_read_u32_val(key, 0);
    uint32_t next = saturating_add(current, amount);
    if (nvs_write_u32_val(key, next)) {
        Serial.printf("[NVS] Profile %d: Added %u coins -> Total: %u 🪙\n",
                      (int)profile, amount, next);
    }
}

uint32_t player_data_get_xp(WizardProfile_t profile) {
    if (!valid_profile(profile)) return 0;
    char key[16];
    snprintf(key, sizeof(key), "xp_%d", (int)profile);
    return nvs_read_u32_val(key, 0);
}

void player_data_add_xp(WizardProfile_t profile, uint32_t amount) {
    if (!valid_profile(profile)) return;
    char key[16];
    snprintf(key, sizeof(key), "xp_%d", (int)profile);

    uint32_t current = nvs_read_u32_val(key, 0);
    uint32_t next = saturating_add(current, amount);
    if (nvs_write_u32_val(key, next)) {
        Serial.printf("[NVS] Profile %d: Added %u XP -> Total: %u ⚡\n",
                      (int)profile, amount, next);
    }
}

void player_data_sync_time(void) {
    Serial.println("[TIME] Configuring SNTP for Israel timezone...");
    configTzTime(ISRAEL_TZ, "pool.ntp.org", "time.google.com");
}

bool player_data_is_time_synced(void) {
    time_t now = time(NULL);
    struct tm timeinfo;
    return localtime_r(&now, &timeinfo) != NULL && (timeinfo.tm_year + 1900 >= 2025);
}

uint32_t player_data_get_current_date(void) {
    time_t now = time(NULL);
    struct tm timeinfo;
    if (localtime_r(&now, &timeinfo) != NULL && (timeinfo.tm_year + 1900 >= 2025)) {
        return (uint32_t)((timeinfo.tm_year + 1900) * 10000 +
                          (timeinfo.tm_mon + 1) * 100 + timeinfo.tm_mday);
    }
    return 0;
}

uint32_t player_data_get_questions_today(WizardProfile_t profile) {
    if (!valid_profile(profile)) return 0;
    ensure_daily_bucket(profile);

    char key[16];
    snprintf(key, sizeof(key), "qt_%d", (int)profile);
    return nvs_read_u32_val(key, 0);
}

uint32_t player_data_get_subject_questions_today(WizardProfile_t profile, int subject_id) {
    if (!valid_profile(profile) || !valid_subject(subject_id)) return 0;
    ensure_daily_bucket(profile);

    char key[16];
    make_subject_key(key, sizeof(key), "qs", profile, subject_id);
    return nvs_read_u32_val(key, 0);
}

void player_data_prepare_question_count(WizardProfile_t profile, int subject_id, bool eligible) {
    pending_count_valid = valid_profile(profile) && valid_subject(subject_id);
    pending_count_profile = profile;
    pending_count_subject = subject_id;
    pending_count_eligible = eligible;
}

uint32_t player_data_increment_questions_today(WizardProfile_t profile) {
    if (!valid_profile(profile)) return 0;

    const uint32_t count_date = player_data_get_current_date();
    ensure_daily_bucket_for_date(profile, count_date);

    const bool can_count = pending_count_valid &&
                           pending_count_profile == profile &&
                           pending_count_eligible &&
                           valid_subject(pending_count_subject);
    const int subject_id = pending_count_subject;

    // Consume the gate exactly once, regardless of outcome.
    pending_count_valid = false;
    pending_count_eligible = false;
    pending_count_profile = PROFILE_NONE;
    pending_count_subject = -1;

    char total_key[16];
    snprintf(total_key, sizeof(total_key), "qt_%d", (int)profile);
    const uint32_t current_total = nvs_read_u32_val(total_key, 0);
    if (!can_count) {
        Serial.printf("[NVS] Profile %d: finalized answer not counted (not first-try correct).\n",
                      (int)profile);
        return current_total;
    }

    char subject_key[16];
    make_subject_key(subject_key, sizeof(subject_key), "qs", profile, subject_id);
    const uint32_t subject_count = nvs_read_u32_val(subject_key, 0);
    const uint32_t limit = quiz_policy_daily_limit((int)profile, subject_id);
    if (limit != UINT32_MAX && subject_count >= limit) {
        Serial.printf("[NVS] Profile %d subject %d: daily limit %u already reached.\n",
                      (int)profile, subject_id, (unsigned)limit);
        return current_total;
    }

    if (count_date == 0) {
        // Track exactly how many eligible answers occurred while the date was
        // untrusted. If SNTP later reveals a new date, these deltas survive
        // the rollover instead of being erased or mixed with the old day.
        char unsynced_key[16];
        make_subject_key(unsynced_key, sizeof(unsynced_key), "uq", profile, subject_id);
        const uint32_t unsynced = nvs_read_u32_val(unsynced_key, 0);
        nvs_write_u32_val(unsynced_key, saturating_add(unsynced, 1));
    }

    const uint32_t next_total = saturating_add(current_total, 1);
    nvs_write_u32_val(total_key, next_total);

    const uint32_t next_subject = saturating_add(subject_count, 1);
    nvs_write_u32_val(subject_key, next_subject);

    Serial.printf("[NVS] Profile %d subject %d: first-try correct -> total=%u subject=%u%s.\n",
                  (int)profile, subject_id, (unsigned)next_total, (unsigned)next_subject,
                  count_date == 0 ? " (date unsynced)" : "");
    return next_total;
}

void player_data_retry_set(WizardProfile_t profile, int subject_id,
                           uint16_t ordinal, bool pending) {
    if (!valid_profile(profile) || !valid_subject(subject_id) ||
        ordinal >= RETRY_MAX_QUESTIONS) return;

    char key[16];
    make_subject_key(key, sizeof(key), "r", profile, subject_id);

    RetryBits bits{};
    nvs_read_retry_bits(key, &bits);
    const uint16_t word = ordinal / 32;
    const uint8_t bit = ordinal % 32;
    if (pending) bits.words[word] |= (1u << bit);
    else bits.words[word] &= ~(1u << bit);

    if (!nvs_write_retry_bits(key, bits)) {
        Serial.printf("[NVS] WARN: failed to persist retry profile=%d subject=%d ordinal=%u.\n",
                      (int)profile, subject_id, (unsigned)ordinal);
    }
}

int player_data_retry_find_next(WizardProfile_t profile, int subject_id,
                                uint16_t start_ordinal, uint16_t question_count) {
    if (!valid_profile(profile) || !valid_subject(subject_id) || question_count == 0) return -1;

    const uint16_t capped_count = question_count > RETRY_MAX_QUESTIONS
        ? RETRY_MAX_QUESTIONS : question_count;
    if (start_ordinal >= capped_count) start_ordinal = 0;

    char key[16];
    make_subject_key(key, sizeof(key), "r", profile, subject_id);
    RetryBits bits{};
    if (!nvs_read_retry_bits(key, &bits)) return -1;

    for (uint16_t offset = 0; offset < capped_count; ++offset) {
        const uint16_t ordinal = (uint16_t)((start_ordinal + offset) % capped_count);
        const uint16_t word = ordinal / 32;
        const uint8_t bit = ordinal % 32;
        if (bits.words[word] & (1u << bit)) return ordinal;
    }
    return -1;
}

void player_data_reset_all(void) {
    nvs_handle_t handle;
    if (nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
        pending_count_valid = false;
        pending_count_eligible = false;
        pending_count_profile = PROFILE_NONE;
        pending_count_subject = -1;
        Serial.println("[NVS] All wizard profiles reset to 0.");
    }
}
