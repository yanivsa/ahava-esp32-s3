#include "results_sync.h"

#include <Arduino.h>
#include <WiFi.h>
#include "esp_http_client.h"

#include "results_sync_config.h"
#include "results_sync_policy.h"
#include "results_sync_store.h"
#include "screen_manager.h"
#include "time_service.h"
#include "weekly_stats_store.h"

static_assert((int)PROFILE_ORI == 1, "PROFILE_ORI id changed");
static_assert((int)PROFILE_ETHAN == 2, "PROFILE_ETHAN id changed");

namespace {

static constexpr uint32_t MIN_ATTEMPT_INTERVAL_MS = 15000;
static constexpr uint32_t RETRY_INTERVAL_MS = 60000;
static constexpr size_t MAX_BATCH_RECORDS = 64;

static uint32_t s_last_attempt_ms = 0;
static bool s_force = true;
static bool s_seeded_recent = false;

extern "C" esp_err_t esp_crt_bundle_attach(void *conf);

static const char *subject_key(int subject) {
    static const char *NAMES[4] = {"math", "hebrew", "english", "religion"};
    if (subject < 0 || subject >= 4) return nullptr;
    return NAMES[subject];
}

static String format_date(uint32_t date) {
    if (date == 0) return String();
    char buf[11];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u",
             (unsigned)(date / 10000u),
             (unsigned)((date / 100u) % 100u),
             (unsigned)(date % 100u));
    return String(buf);
}

static String device_id(void) {
    const uint64_t mac = ESP.getEfuseMac();
    char buf[32];
    snprintf(buf, sizeof(buf), "ahava-esp32-%04X%08X",
             (unsigned)((mac >> 32) & 0xFFFFu),
             (unsigned)(mac & 0xFFFFFFFFu));
    return String(buf);
}

static void seed_recent_history(void) {
    if (s_seeded_recent) return;
    const WizardProfile_t profiles[] = {PROFILE_ORI, PROFILE_ETHAN};
    for (WizardProfile_t profile : profiles) {
        WeeklyStatsDay_t days[WEEKLY_STATS_DAYS]{};
        if (!weekly_stats_store_get_last_seven(profile, days)) continue;
        for (int day = 0; day < WEEKLY_STATS_DAYS; ++day) {
            if (days[day].date == 0) continue;
            results_sync_store_set_floor(profile, days[day].date, days[day].correct);
        }
    }
    s_seeded_recent = true;
}

static bool build_payload(const ResultSyncRecord_t *records,
                          size_t count,
                          String *payload) {
    if (!records || count == 0 || !payload) return false;
    payload->remove(0);
    payload->reserve(7600);
    payload->concat(R"({"deviceId":")");
    payload->concat(device_id());
    payload->concat(R"(","results":[)");

    for (size_t i = 0; i < count; ++i) {
        const char *pkey = results_sync_profile_key_from_id((int)records[i].profile);
        const char *skey = subject_key(records[i].subject);
        if (!pkey || !skey) return false;

        if (i != 0) payload->concat(',');
        payload->concat(R"({"profileKey":")");
        payload->concat(pkey);
        payload->concat(R"(","activityDate":")");
        payload->concat(format_date(records[i].date));
        payload->concat(R"(","subject":")");
        payload->concat(skey);
        payload->concat(R"(","correctFirstTry":)");
        payload->concat(String((unsigned)records[i].correct_first_try));
        payload->concat(R"(,"sourceRevision":)");
        payload->concat(String((unsigned)records[i].correct_first_try));
        payload->concat('}');
    }
    payload->concat("]}");
    return true;
}

static bool post_payload(const String &payload) {
    esp_http_client_config_t config = {};
    config.url = AHAVA_RESULTS_SYNC_URL;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 12000;
    config.keep_alive_enable = true;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    const String auth = String("Bearer ") + AHAVA_RESULTS_SYNC_TOKEN;
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth.c_str());
    esp_http_client_set_post_field(client, payload.c_str(), payload.length());

    const esp_err_t err = esp_http_client_perform(client);
    const int status = (err == ESP_OK) ? esp_http_client_get_status_code(client) : 0;
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        Serial.printf("[RESULT_SYNC] HTTPS request failed: 0x%x\n", err);
        return false;
    }
    if (status < 200 || status >= 300) {
        Serial.printf("[RESULT_SYNC] Server rejected snapshot: HTTP %d\n", status);
        return false;
    }
    return true;
}

} // namespace

bool results_sync_is_configured(void) {
    return AHAVA_RESULTS_SYNC_TOKEN[0] != '\0';
}

void results_sync_force(void) {
    s_force = true;
    s_seeded_recent = false;
}

void results_sync_poll(void) {
    if (!results_sync_is_configured()) return;
    if (!time_service_has_trusted_time()) return;

    // Backfill the previous seven days when this firmware first becomes active,
    // or after a delayed clock sync. Future answers are written directly to the
    // durable ledger and therefore survive much longer offline.
    seed_recent_history();

    if (WiFi.status() != WL_CONNECTED) return;

    const uint32_t now = millis();
    if (!s_force && s_last_attempt_ms != 0 &&
        (now - s_last_attempt_ms) < RETRY_INTERVAL_MS) return;
    if (s_last_attempt_ms != 0 &&
        (now - s_last_attempt_ms) < MIN_ATTEMPT_INTERVAL_MS) return;

    ResultSyncRecord_t records[MAX_BATCH_RECORDS]{};
    const size_t count = results_sync_store_collect_dirty(records, MAX_BATCH_RECORDS);
    if (count == 0) {
        s_force = false;
        return;
    }

    String payload;
    if (!build_payload(records, count, &payload)) return;

    s_last_attempt_ms = now;
    Serial.printf("[RESULT_SYNC] Uploading %u pending daily aggregate rows.\n",
                  (unsigned)count);

    if (post_payload(payload)) {
        results_sync_store_ack(records, count);
        s_force = true; // Immediately allow the next batch if more than 64 rows remain.
        Serial.println("[RESULT_SYNC] Cloud batch accepted.");
    } else {
        s_force = false;
    }
}
