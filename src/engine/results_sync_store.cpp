#include "results_sync_store.h"

#include <Arduino.h>
#include "nvs.h"
#include "results_sync_ledger.h"
#include "results_sync_policy.h"

namespace {

static constexpr const char *NVS_NAMESPACE = "result_sync";

static bool valid_profile(WizardProfile_t profile) {
    return results_sync_profile_id_enabled((int)profile);
}

static const char *ledger_key(WizardProfile_t profile) {
    return profile == PROFILE_ORI ? "ori" : "eitan";
}

static bool load_ledger(WizardProfile_t profile, ResultSyncLedger_t *ledger) {
    if (!valid_profile(profile) || !ledger) return false;
    results_sync_ledger_clear(ledger);

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return true;

    size_t len = sizeof(*ledger);
    const esp_err_t err = nvs_get_blob(handle, ledger_key(profile), ledger, &len);
    nvs_close(handle);

    if (err != ESP_OK || len != sizeof(*ledger) ||
        ledger->version != RESULT_SYNC_LEDGER_VERSION) {
        results_sync_ledger_clear(ledger);
    }
    return true;
}

static bool save_ledger(WizardProfile_t profile, const ResultSyncLedger_t *ledger) {
    if (!valid_profile(profile) || !ledger) return false;
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;
    esp_err_t err = nvs_set_blob(handle, ledger_key(profile), ledger, sizeof(*ledger));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

} // namespace

bool results_sync_store_record_correct(WizardProfile_t profile,
                                       uint32_t date,
                                       int subject) {
    if (!valid_profile(profile) || date == 0 ||
        subject < 0 || subject >= RESULT_SYNC_LEDGER_SUBJECTS) return false;

    ResultSyncLedger_t ledger{};
    load_ledger(profile, &ledger);
    if (!results_sync_ledger_increment(&ledger, date, subject)) {
        Serial.printf("[RESULT_SYNC] WARN: ledger full; cannot retain profile=%d date=%u subject=%d.\n",
                      (int)profile, (unsigned)date, subject);
        return false;
    }
    return save_ledger(profile, &ledger);
}

bool results_sync_store_set_floor(WizardProfile_t profile,
                                  uint32_t date,
                                  const uint16_t correct[4]) {
    if (!valid_profile(profile) || date == 0 || !correct) return false;

    ResultSyncLedger_t ledger{};
    load_ledger(profile, &ledger);
    bool changed = false;
    for (int subject = 0; subject < RESULT_SYNC_LEDGER_SUBJECTS; ++subject) {
        changed |= results_sync_ledger_set_floor(&ledger, date, subject, correct[subject]);
    }
    if (!changed) return true;
    return save_ledger(profile, &ledger);
}

size_t results_sync_store_collect_dirty(ResultSyncRecord_t *out,
                                        size_t max_records) {
    if (!out || max_records == 0) return 0;
    size_t count = 0;
    const WizardProfile_t profiles[] = {PROFILE_ORI, PROFILE_ETHAN};

    for (WizardProfile_t profile : profiles) {
        ResultSyncLedger_t ledger{};
        load_ledger(profile, &ledger);

        for (int i = 0; i < RESULT_SYNC_LEDGER_DAYS && count < max_records; ++i) {
            const ResultSyncLedgerDay_t &day = ledger.days[i];
            if (day.date == 0 || day.dirty_mask == 0) continue;

            for (int subject = 0;
                 subject < RESULT_SYNC_LEDGER_SUBJECTS && count < max_records;
                 ++subject) {
                if ((day.dirty_mask & (1u << subject)) == 0) continue;
                out[count++] = {
                    profile,
                    day.date,
                    (uint8_t)subject,
                    day.correct[subject]
                };
            }
        }
    }
    return count;
}

void results_sync_store_ack(const ResultSyncRecord_t *records,
                            size_t count) {
    if (!records || count == 0) return;

    const WizardProfile_t profiles[] = {PROFILE_ORI, PROFILE_ETHAN};
    for (WizardProfile_t profile : profiles) {
        ResultSyncLedger_t ledger{};
        load_ledger(profile, &ledger);
        bool changed = false;

        for (size_t i = 0; i < count; ++i) {
            if (records[i].profile != profile) continue;
            changed |= results_sync_ledger_ack(&ledger,
                                               records[i].date,
                                               records[i].subject,
                                               records[i].correct_first_try);
        }
        if (changed) save_ledger(profile, &ledger);
    }
}
