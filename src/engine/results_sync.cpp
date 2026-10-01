#include "results_sync.h"

#include <Arduino.h>
#include <WiFi.h>
#include "esp_http_client.h"

#include "results_sync_config.h"
#include "results_sync_policy.h"
#include "screen_manager.h"
#include "time_service.h"
#include "weekly_stats_store.h"

static_assert((int)PROFILE_ORI == 1, "PROFILE_ORI id changed");
static_assert((int)PROFILE_ETHAN == 2, "PROFILE_ETHAN id changed");

namespace {

static constexpr uint32_t MIN_ATTEMPT_INTERVAL_MS = 15000;
static constexpr uint32_t RETRY_INTERVAL_MS = 60000;
static constexpr uint32_t RESEND_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;

static uint32_t s_last_attempt_ms = 0;
static uint32_t s_last_success_ms = 0;
static uint32_t s_last_success_hash = 0;
static bool s_force = true;

extern "C" esp_err_t esp_crt_bundle_attach(void *conf);

static const char *subject_key(int subject) {
    static const char *NAMES[WEEKLY_STATS_SUBJECTS] = {
        "math", "hebrew", "english", "religion"
    };
    if (subject < 0 || subject >= WEEKLY_STATS_SUBJECTS) return nullptr;
    return NAMES[subject];
}

static void append_hash(uint32_t *hash, uint32_t value) {
    *hash ^= value;
    *hash *= 16777619u;
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

static bool build_payload(String *payload, uint32_t *snapshot_hash, int *record_count) {
    if (!payload || !snapshot_hash || !record_count) return false;

    payload->remove(0);
    payload->reserve(7600);
    *snapshot_hash = 2166136261u;
    *record_count = 0;

    payload->concat("{\"deviceId\":\"");
    payload->concat(device_id());
    payload->concat("\",\"results\":[");

    bool first = true;
    const WizardProfile_t profiles[] = {PROFILE_ORI, PROFILE_ETHAN};
    for (WizardProfile_t profile : profiles) {
        WeeklyStatsDay_t days[WEEKLY_STATS_DAYS]{};
        if (!weekly_stats_store_get_last_seven(profile, days)) continue;

        const char *pkey = results_sync_profile_key_from_id((int)profile);
        if (!pkey) continue;

        for (int day = 0; day < WEEKLY_STATS_DAYS; ++day) {
            if (days[day].date == 0) continue;
            for (int subject = 0; subject < WEEKLY_STATS_SUBJECTS; ++subject) {
                const uint16_t count = days[day].correct[subject];
                if (count == 0) continue;

                const char *skey = subject_key(subject);
                if (!skey) continue;

                if (!first) payload->concat(',');
                first = false;

                payload->concat("{\"profileKey\":\"");
                payload->concat(pkey);
                payload->concat("\",\"activityDate\":\"");
                payload->concat(format_date(days[day].date));
                payload->concat("\",\"subject\":\"");
                payload->concat(skey);
                payload->concat("\",\"correctFirstTry\":");
                payload->concat(String((unsigned)count));
                payload->concat(",\"sourceRevision\":");
                payload->concat(String((unsigned)count));
                payload->concat('}');

                append_hash(snapshot_hash, (uint32_t)profile);
                append_hash(snapshot_hash, days[day].date);
                append_hash(snapshot_hash, (uint32_t)subject);
                append_hash(snapshot_hash, count);
                ++(*record_count);
            }
        }
    }

    payload->concat("]}");
    return *record_count > 0;
}

static bool post_payload(const String &payload) {
    esp_http_client_config_t config = {};
    config.url = AHAVA_RESULTS_SYNC_URL;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 12000;
    config.keep_alive_enable = true;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        Serial.println("[RESULT_SYNC] Failed to initialize HTTPS client.");
        return false;
    }

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
}

void results_sync_poll(void) {
    if (!results_sync_is_configured()) return;
    if (WiFi.status() != WL_CONNECTED) return;
    if (!time_service_has_trusted_time()) return;

    const uint32_t now = millis();
    const uint32_t since_attempt = now - s_last_attempt_ms;
    if (s_last_attempt_ms != 0 && since_attempt < MIN_ATTEMPT_INTERVAL_MS) return;

    String payload;
    uint32_t snapshot_hash = 0;
    int record_count = 0;
    if (!build_payload(&payload, &snapshot_hash, &record_count)) return;

    const bool unchanged = snapshot_hash == s_last_success_hash;
    if (!s_force && unchanged && s_last_success_ms != 0 &&
        (now - s_last_success_ms) < RESEND_INTERVAL_MS) {
        return;
    }

    if (s_last_attempt_ms != 0 && !unchanged &&
        since_attempt < RETRY_INTERVAL_MS && s_last_success_hash != 0) {
        return;
    }

    s_last_attempt_ms = now;
    Serial.printf("[RESULT_SYNC] Uploading %d daily aggregate rows.\n", record_count);

    if (post_payload(payload)) {
        s_last_success_hash = snapshot_hash;
        s_last_success_ms = now;
        s_force = false;
        Serial.println("[RESULT_SYNC] Cloud snapshot accepted.");
    } else {
        s_force = true;
    }
}
