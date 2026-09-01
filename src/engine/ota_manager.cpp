/**
 * @file ota_manager.cpp
 * @brief Wizard Academy HTTPS OTA Firmware Updater & Wi-Fi Management
 */

#include "ota_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static OtaStatus_t current_status = OTA_STATUS_IDLE;
static uint8_t progress_pct = 0;
static size_t bytes_downloaded = 0;
static size_t total_firmware_bytes = 0;
static TaskHandle_t ota_task_handle = NULL;

// ISRG Root X1, valid until 2035. Used explicitly because the Arduino ESP32
// build does not include the optional runtime certificate bundle.
static const char OTA_ROOT_CA[] = R"PEM(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";

bool ota_manager_init(void) {
    current_status = OTA_STATUS_IDLE;
    progress_pct = 0;
    bytes_downloaded = 0;
    total_firmware_bytes = 0;
    
    // Set Wi-Fi to Station mode with auto-reconnect
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    if (DEFAULT_WIFI_SSID[0]) {
        WiFi.persistent(true);
        WiFi.begin(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS);
        Serial.println("[OTA] Connecting to configured Wi-Fi network in background.");
    } else {
        WiFi.begin();
        Serial.println("[OTA] Restoring saved Wi-Fi credentials in background.");
    }
    
    Serial.println("[OTA] Wi-Fi & OTA Manager initialized (Station mode active).");
    return true;
}

bool ota_is_wifi_connected(void) {
    return (WiFi.status() == WL_CONNECTED);
}

void ota_wifi_disconnect(void) {
    if (WiFi.getMode() != WIFI_OFF) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        Serial.println("[OTA] Wi-Fi radio powered down (WIFI_OFF) for battery preservation.");
    }
}

bool ota_wifi_connect(const char *ssid, const char *pass, uint32_t timeout_ms) {
    const char *target_ssid = ssid ? ssid : DEFAULT_WIFI_SSID;
    const char *target_pass = pass ? pass : DEFAULT_WIFI_PASS;

    current_status = OTA_STATUS_CONNECTING_WIFI;
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    if (target_ssid[0]) {
        Serial.printf("[OTA] Connecting to Wi-Fi SSID: %s ...\n", target_ssid);
        WiFi.begin(target_ssid, target_pass);
    } else {
        Serial.println("[OTA] Connecting with saved Wi-Fi credentials...");
        WiFi.begin();
    }

    uint32_t start_time = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < timeout_ms) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED && !target_ssid[0]) {
        Serial.println("[OTA] No saved network. Starting captive portal Ahava-Setup...");
        current_status = OTA_STATUS_PORTAL_ACTIVE;
        WiFiManager manager;
        manager.setConfigPortalTimeout(180);
        manager.setConnectTimeout(20);
        if (!manager.autoConnect("Ahava-Setup", "Ahava1234")) {
            Serial.println("[OTA] Wi-Fi provisioning timed out.");
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[OTA] Wi-Fi Connected! IP: %s | RSSI: %d dBm\n", 
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        current_status = OTA_STATUS_IDLE;
        return true;
    } else {
        Serial.println("[OTA] Wi-Fi connection timed out!");
        current_status = OTA_STATUS_FAILED;
        ota_wifi_disconnect();
        return false;
    }
}

bool ota_perform_update(const char *url) {
    const char *target_url = (url && strlen(url) > 0) ? url : DEFAULT_OTA_FIRMWARE_URL;

    if (!target_url[0]) {
        Serial.println("[OTA] Update URL is not provisioned; refusing OTA.");
        current_status = OTA_STATUS_FAILED;
        return false;
    }

    // 1. Ensure Wi-Fi is connected
    if (!ota_is_wifi_connected()) {
        Serial.println("[OTA] Wi-Fi not connected. Attempting auto-connect...");
        if (!ota_wifi_connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS, 10000)) {
            Serial.println("[OTA] ERROR: Cannot update without Wi-Fi connection.");
            current_status = OTA_STATUS_FAILED;
            return false;
        }
    }

    current_status = OTA_STATUS_DOWNLOADING;
    progress_pct = 0;
    bytes_downloaded = 0;
    total_firmware_bytes = 0;
    Serial.printf("[OTA] Starting HTTPS OTA update from: %s\n", target_url);

    // 2. Configure HTTPS with the ESP certificate bundle and hostname checks.
    esp_http_client_config_t http_config = {};
    http_config.url = target_url;
    http_config.cert_pem = OTA_ROOT_CA;
    // Never weaken TLS hostname validation. OTA stays disabled until a trusted
    // endpoint and its CA certificate are provisioned for this device.
    http_config.skip_cert_common_name_check = false;
    http_config.crt_bundle_attach = NULL;
    http_config.timeout_ms = 20000;
    http_config.keep_alive_enable = true;
    http_config.max_redirection_count = 5;

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK) {
        Serial.printf("[OTA] ERROR: esp_https_ota_begin failed (0x%x)\n", err);
        current_status = OTA_STATUS_FAILED;
        return false;
    }

    int image_size = esp_https_ota_get_image_size(https_ota_handle);
    total_firmware_bytes = (image_size > 0) ? (size_t)image_size : 0;
    Serial.printf("[OTA] Total firmware binary size: %d bytes\n", image_size);

    // 3. Download & Flash loop
    uint8_t last_reported_pct = 255;
    while (1) {
        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        int read_len = esp_https_ota_get_image_len_read(https_ota_handle);
        bytes_downloaded = (read_len > 0) ? (size_t)read_len : 0;
        if (image_size > 0) {
            progress_pct = (uint8_t)((read_len * 100) / image_size);
        }
        if (progress_pct != last_reported_pct) {
            Serial.printf("[OTA] Flash progress: %d bytes (%u%%)\n", read_len, progress_pct);
            last_reported_pct = progress_pct;
        }
        taskYIELD();
    }

    if (err != ESP_OK) {
        Serial.printf("[OTA] ERROR: esp_https_ota_perform failed (0x%x)\n", err);
        esp_https_ota_abort(https_ota_handle);
        current_status = OTA_STATUS_FAILED;
        ota_wifi_disconnect();
        return false;
    }

    esp_err_t finish_err = esp_https_ota_finish(https_ota_handle);
    if (finish_err != ESP_OK) {
        Serial.printf("[OTA] ERROR: esp_https_ota_finish failed (0x%x)\n", finish_err);
        current_status = OTA_STATUS_FAILED;
        ota_wifi_disconnect();
        return false;
    }

    current_status = OTA_STATUS_SUCCESS;
    progress_pct = 100;
    Serial.println("[OTA] ===============================================");
    Serial.println("[OTA] FIRMWARE UPDATE SUCCESSFUL! Restarting MCU...");
    Serial.println("[OTA] ===============================================");
    
    delay(1500);
    esp_restart();
    return true;
}

static void ota_task_worker(void *pvParameters) {
    char *url_copy = (char *)pvParameters;
    ota_perform_update(url_copy);
    if (url_copy) free(url_copy);
    ota_task_handle = NULL;
    vTaskDelete(NULL);
}

void ota_start_async_update(const char *url) {
    if (ota_task_handle != NULL) {
        Serial.println("[OTA] WARN: OTA task already running!");
        return;
    }

    char *url_copy = NULL;
    if (url) {
        url_copy = strdup(url);
    }

    xTaskCreatePinnedToCore(
        ota_task_worker,
        "ota_worker",
        8192,
        url_copy,
        4,
        &ota_task_handle,
        0 // Pinned to Core 0 (leaving Core 1 for GUI)
    );
}

OtaStatus_t ota_get_status(void) {
    return current_status;
}

const char* ota_get_status_message(void) {
    switch (current_status) {
        case OTA_STATUS_IDLE:
            return "מוכן לעדכון קושחה";
        case OTA_STATUS_CONNECTING_WIFI:
            return "מתחבר לרשת ה-Wi-Fi...";
        case OTA_STATUS_PORTAL_ACTIVE:
            return "נקודת הגדרות פעילה:\nהתחברו ל-Ahava-Setup בדפדפן";
        case OTA_STATUS_DOWNLOADING:
            return "מוריד ומתקין קושחה חדשה...";
        case OTA_STATUS_SUCCESS:
            return "העדכון הושלם! המכשיר מאתחל...";
        case OTA_STATUS_FAILED:
            return "העדכון נכשל. בדקו את הרשת.";
        default:
            return "";
    }
}

uint8_t ota_get_progress_pct(void) {
    return progress_pct;
}

size_t ota_get_bytes_read(void) {
    return bytes_downloaded;
}

size_t ota_get_total_bytes(void) {
    return total_firmware_bytes;
}

void ota_reset_status(void) {
    if (ota_task_handle == NULL) {
        current_status = OTA_STATUS_IDLE;
        progress_pct = 0;
        bytes_downloaded = 0;
        total_firmware_bytes = 0;
    }
}
