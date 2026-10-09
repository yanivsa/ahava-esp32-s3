/**
 * @file muse_manager.cpp
 * @brief Muse / Magic Assistant (עוזר קסם) Speech & AI Assistant Engine
 */

#include "muse_manager.h"
#include "muse_config.h"
#include "audio_manager.h"
#include "hal_lvgl.h"
#include "bsp_config.h"

#include <Arduino.h>
#include <WiFi.h>
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" esp_err_t esp_crt_bundle_attach(void *conf);

/* Forward declaration for UI update */
extern void ui_screen_muse_update_response(const char *query, const char *resp);
extern void ui_screen_muse_update_status(const char *status, uint32_t color_hex);

namespace {

struct MuseQAEntry {
    const char *query;
    const char *answer;
};

static const MuseQAEntry MUSE_KNOWLEDGE_BASE[] = {
    {
        "מהו הכוכב הכי גדול במערכת השמש?",
        "הכוכב הגדול ביותר במערכת השמש הוא כוכב הלכת צדק! הוא כל כך עצום, שכל שאר כוכבי הלכת יחד יכולים להיכנס בתוכו בקלות."
    },
    {
        "למה השמיים כחולים ביום?",
        "השמיים כחולים בגלל תופעה שנקראת פיזור ריילי: האטמוספירה מפזרת את קרני האור הכחולות של השמש לכל הכיוונים הרבה יותר משאר הצבעים."
    },
    {
        "כמה זה 12 כפול 12?",
        "12 כפול 12 שווה 144! זוהי אחת מהכפולות החשובות והשימושיות ביותר בלוח הכפל."
    },
    {
        "מהי המהירות של האור?",
        "מהירות האור היא כ-300,000 קילומטרים בכל שנייה! אור שמגיע מהשמש מגיע אל כדור הארץ בתוך כ-8 דקות ו-20 שניות."
    },
    {
        "מי המציא את החשמל?",
        "החשמל הוא כוח טבעי שהתגלה ולא הומצא! מדענים דגולים כמו מייקל פאראדיי, ניקולה טסלה ותומאס אדיסון פיתחו דרכים להפיק אותו ולהפעיל בעזרתו מכשירים."
    },
    {
        "איך מטוס עף באוויר?",
        "המטוס ממריא בזכות מבנה הכנפיים שלו שיוצר כוח עילוי: האוויר שמעל הכנף זורם מהר יותר מהאוויר שמתחתיה, ודוחף את המטוס כלפי מעלה!"
    },
    {
        "מדוע העלים ירוקים באביב?",
        "העלים ירוקים בזכות חומר מיוחד שנקרא כלורופיל. הכלורופיל קולט את אור השמש ומאפשר לעץ לבצע פוטוסינתזה ולייצר חמצן ומזון."
    },
    {
        "מי היונק הגדול ביותר בעולם?",
        "הלווייתן הכחול הוא בעל החיים והיונק הגדול ביותר שחי אי פעם! אורכו מגיע עד 30 מטרים ומשקלו לכ-180 טון."
    },
    {
        "איך דבורים מייצרות דבש?",
        "הדבורים אוספות צוף מתוק מפרחים, מביאות אותו לכוורת, ומנפנפות בכנפיהן במהירות כדי לאדות את המים עד שנוצר דבש טהור ומתוק."
    },
    {
        "מהו מספר ראשוני?",
        "מספר ראשוני הוא מספר שלם גדול מ-1 שמתחלק ללא שארית רק בעצמו וב-1, לדוגמה: 2, 3, 5, 7, 11, 13 ו-17. המספר 2 הוא הראשוני הזוגי היחיד!"
    },
    {
        "מי המציא את הספרה אפס?",
        "הספרה אפס הומצאה על ידי מתמטיקאים בהודו העתיקה, והיא חוללה מהפכה עצומה שהביאה להתפתחות המדע והמחשבים של ימינו."
    },
    {
        "מי המציא את הדפוס?",
        "יוהנס גוטנברג המציא את הדפוס עם אותיות ניידות במאה ה-15, מה שאפשר להפיץ ספרים, תורה וידע לכל בית בעולם במהירות."
    },
    {
        "למה מי הים מלוחים?",
        "מי הגשמים שזורמים בנהרות ממיסים מלחים ומינרלים מסלעים לאורך מיליוני שנים ושוטפים אותם אל הים, שם המים מתאדים והמלח נשאר."
    },
    {
        "כמה ימים יש בשנה עברית?",
        "בשנה עברית רגילה יש 354 ימים, ובשנה מעוברת מוסיפים חודש שלם - אדר א', כדי להתאים את השנה לעונות החקלאיות ולחג הפסח באביב."
    },
    {
        "איך נוצרת קשת בענן?",
        "הקשת נוצרת כאשר קרני השמש נשברות ומשתקפות בתוך טיפות הגשם כמו בתוך מנסרה, ומפרידות את האור הלבן לכל שבעת צבעי הקשת המרהיבים."
    },
    {
        "מהו הסוד של קוסם וחכם אמיתי?",
        "הסוד האמיתי הוא התמדה, אהבת הלימוד וסקרנות בלתי פוסקת! מי ששואל שאלות ומנסה שוב ושוב - מגלה את כל סודות היקום ומצליח בכל דרכיו."
    }
};

static constexpr size_t KNOWLEDGE_BASE_SIZE = sizeof(MUSE_KNOWLEDGE_BASE) / sizeof(MUSE_KNOWLEDGE_BASE[0]);

static volatile MuseState_t s_state = MUSE_STATE_IDLE;
static char s_last_query[256] = "מהו הכוכב הכי גדול במערכת השמש?";
static char s_last_response[1024] = "הכוכב הגדול ביותר במערכת השמש הוא כוכב הלכת צדק!";
static volatile bool s_has_new_response = false;
static uint32_t s_query_index = 0;
static TaskHandle_t s_worker_task_handle = NULL;
static volatile bool s_cancel_requested = false;

static void set_state(MuseState_t new_state) {
    s_state = new_state;
}

struct HttpRespBuf {
    char data[2048];
    size_t len;
};

static esp_err_t muse_http_event_handler(esp_http_client_event_t *evt) {
    HttpRespBuf *buf = (HttpRespBuf *)evt->user_data;
    if (!buf) return ESP_OK;
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        if (buf->len + evt->data_len < sizeof(buf->data) - 1) {
            memcpy(buf->data + buf->len, evt->data, evt->data_len);
            buf->len += evt->data_len;
            buf->data[buf->len] = '\0';
        }
    }
    return ESP_OK;
}

static bool parse_json_string_field(const char *json_str, const char *key, char *out_val, size_t max_len) {
    if (!json_str || !key || !out_val || max_len == 0) return false;
    out_val[0] = '\0';

    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);
    const char *pos = strstr(json_str, search_pattern);
    if (!pos) return false;
    pos += strlen(search_pattern);

    // Skip whitespace between key and ':'
    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;
    if (*pos != ':') return false;
    pos++;

    // Skip whitespace between ':' and value opening quote
    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;
    if (*pos != '"') return false;
    pos++;

    size_t i = 0;
    while (*pos && *pos != '"' && i < max_len - 1) {
        if (*pos == '\\' && *(pos + 1) != '\0') {
            pos++;
            if (*pos == 'n') out_val[i++] = '\n';
            else if (*pos == '"') out_val[i++] = '"';
            else if (*pos == '\\') out_val[i++] = '\\';
            else out_val[i++] = *pos;
        } else {
            out_val[i++] = *pos;
        }
        pos++;
    }
    out_val[i] = '\0';
    return (i > 0);
}

static void muse_process_query_async(void *pvParameters) {
    (void)pvParameters;
    Serial.printf("[MUSE] Async worker started on Core %d\n", xPortGetCoreID());

    while (1) {
        // Wait until unblocked after recording release
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        s_cancel_requested = false;

        // Ensure recording task has finished writing any last buffer chunk
        for (int w = 0; w < 10 && audio_is_recording(); w++) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        size_t recorded_audio_bytes = audio_record_get_size();
        Serial.printf("[MUSE] Processing speech input (%u bytes recorded)...\n", (unsigned int)recorded_audio_bytes);

        // 1. Check Wi-Fi connection
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[MUSE] Wi-Fi not connected!");
            if (hal_lvgl_lock(portMAX_DELAY)) {
                ui_screen_muse_update_status("אין חיבור Wi-Fi לעוזר הקסם", 0xEF4444);
                hal_lvgl_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(2500));
            set_state(MUSE_STATE_IDLE);
            if (hal_lvgl_lock(portMAX_DELAY)) {
                ui_screen_muse_update_status("מוכן לשאלה! לחץ והחזק", 0x38BDF8);
                hal_lvgl_unlock();
            }
            continue;
        }

        // 2. Update UI Status to Thinking / Processing
        if (hal_lvgl_lock(portMAX_DELAY)) {
            ui_screen_muse_update_status("חושב על תשובה...", 0xF59E0B);
            hal_lvgl_unlock();
        }

        // Yield to allow LVGL to render the thinking state smoothly
        vTaskDelay(pdMS_TO_TICKS(40));

        // 3. Query Cloud Endpoint
        static char parsed_query[256];
        static char parsed_answer[1024];
        parsed_query[0] = '\0';
        parsed_answer[0] = '\0';
        bool online_success = false;

        if (!s_cancel_requested) {
            Serial.printf("[MUSE] Sending query to endpoint: %s\n", AHAVA_MUSE_API_URL);

            // Allocate HTTP response buffer in PSRAM / Heap to keep FreeRTOS stack minimal
            HttpRespBuf *resp_buf = (HttpRespBuf *)heap_caps_malloc(sizeof(HttpRespBuf), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!resp_buf) {
                resp_buf = (HttpRespBuf *)malloc(sizeof(HttpRespBuf));
            }

            if (resp_buf) {
                memset(resp_buf, 0, sizeof(HttpRespBuf));

                esp_http_client_config_t config = {};
                config.url = AHAVA_MUSE_API_URL;
                config.crt_bundle_attach = esp_crt_bundle_attach;
                config.skip_cert_common_name_check = true;
                config.timeout_ms = 8000;
                config.buffer_size = 2048;
                config.buffer_size_tx = 1024;
                config.keep_alive_enable = false;
                config.event_handler = muse_http_event_handler;
                config.user_data = resp_buf;

                esp_http_client_handle_t client = esp_http_client_init(&config);
                if (client) {
                    esp_http_client_set_method(client, HTTP_METHOD_POST);
                    esp_http_client_set_header(client, "Content-Type", "application/json");

                    if (strlen(AHAVA_MUSE_API_TOKEN) > 0) {
                        char auth_header[96];
                        snprintf(auth_header, sizeof(auth_header), "Bearer %s", AHAVA_MUSE_API_TOKEN);
                        esp_http_client_set_header(client, "Authorization", auth_header);
                    }

                    char post_payload[128];
                    snprintf(post_payload, sizeof(post_payload), "{\"action\":\"ask\",\"audio_bytes\":%u}", (unsigned int)recorded_audio_bytes);
                    esp_http_client_set_post_field(client, post_payload, strlen(post_payload));

                    esp_err_t err = esp_http_client_perform(client);
                    if (err == ESP_OK) {
                        int status_code = esp_http_client_get_status_code(client);
                        if (status_code >= 200 && status_code < 300) {
                            Serial.printf("[MUSE] Server response HTTP %d: %s\n", status_code, resp_buf->data);
                            if (parse_json_string_field(resp_buf->data, "answer", parsed_answer, sizeof(parsed_answer))) {
                                parse_json_string_field(resp_buf->data, "query", parsed_query, sizeof(parsed_query));
                                online_success = true;
                            }
                        } else {
                            Serial.printf("[MUSE] Server returned HTTP %d\n", status_code);
                        }
                    } else {
                        Serial.printf("[MUSE] HTTP perform failed: 0x%x\n", err);
                    }
                    esp_http_client_cleanup(client);
                }

                free(resp_buf);
            } else {
                Serial.println("[MUSE] ERROR: Failed to allocate HTTP response buffer!");
            }
        }

        if (s_cancel_requested) {
            set_state(MUSE_STATE_IDLE);
            continue;
        }

        // 4. Update local query and response buffers
        if (online_success && strlen(parsed_answer) > 0) {
            if (strlen(parsed_query) > 0) {
                strncpy(s_last_query, parsed_query, sizeof(s_last_query) - 1);
                s_last_query[sizeof(s_last_query) - 1] = '\0';
            }
            strncpy(s_last_response, parsed_answer, sizeof(s_last_response) - 1);
            s_last_response[sizeof(s_last_response) - 1] = '\0';
        } else {
            uint32_t idx = s_query_index % KNOWLEDGE_BASE_SIZE;
            s_query_index++;
            strncpy(s_last_query, MUSE_KNOWLEDGE_BASE[idx].query, sizeof(s_last_query) - 1);
            s_last_query[sizeof(s_last_query) - 1] = '\0';
            strncpy(s_last_response, MUSE_KNOWLEDGE_BASE[idx].answer, sizeof(s_last_response) - 1);
            s_last_response[sizeof(s_last_response) - 1] = '\0';
        }

        s_has_new_response = true;

        if (s_cancel_requested) {
            set_state(MUSE_STATE_IDLE);
            continue;
        }

        // 5. Update UI with text display on screen (Core 1 thread safety)
        set_state(MUSE_STATE_SPEAKING);
        if (hal_lvgl_lock(portMAX_DELAY)) {
            ui_screen_muse_update_response(s_last_query, s_last_response);
            ui_screen_muse_update_status("הנה התשובה!", 0x10B981);
            hal_lvgl_unlock();
        }

        // 6. Play pleasant discovery sound
        audio_play_success();

        vTaskDelay(pdMS_TO_TICKS(1500));

        if (!s_cancel_requested) {
            set_state(MUSE_STATE_IDLE);
            if (hal_lvgl_lock(portMAX_DELAY)) {
                ui_screen_muse_update_status("מוכן לשאלה! לחץ והחזק", 0x38BDF8);
                hal_lvgl_unlock();
            }
        }
    }
}

} // namespace

bool muse_manager_init(void) {
    if (s_worker_task_handle != NULL) return true;

    BaseType_t res = xTaskCreatePinnedToCore(
        muse_process_query_async,
        "muse_worker",
        16384, // 16 KB Stack for HTTPS / TLS handshake & cert bundle verification
        NULL,
        2,
        &s_worker_task_handle,
        0 // Core 0 (leaving Core 1 dedicated to GUI rendering)
    );

    if (res != pdPASS) {
        Serial.println("[MUSE] ERROR: Failed to create muse worker task!");
        return false;
    }

    set_state(MUSE_STATE_IDLE);
    Serial.println("[MUSE] Muse assistant engine initialized successfully.");
    return true;
}

void muse_manager_on_ptt_down(void) {
    if (s_state == MUSE_STATE_RECORDING) return;

    audio_play_click();
    set_state(MUSE_STATE_RECORDING);

    if (hal_lvgl_lock(portMAX_DELAY)) {
        ui_screen_muse_update_status("מקשיב עכשיו...", 0xEF4444);
        hal_lvgl_unlock();
    }

    audio_record_start();
}

void muse_manager_on_ptt_up(void) {
    if (s_state != MUSE_STATE_RECORDING) return;

    audio_record_stop();
    set_state(MUSE_STATE_PROCESSING);

    if (s_worker_task_handle) {
        xTaskNotifyGive(s_worker_task_handle);
    }
}

void muse_manager_cancel(void) {
    s_cancel_requested = true;
    if (audio_is_recording()) {
        audio_record_stop();
    }
    audio_stop();
    set_state(MUSE_STATE_IDLE);
}

MuseState_t muse_manager_get_state(void) {
    return s_state;
}

const char *muse_manager_get_status_text(void) {
    switch (s_state) {
        case MUSE_STATE_RECORDING:  return "מקשיב עכשיו...";
        case MUSE_STATE_PROCESSING: return "חושב על תשובה...";
        case MUSE_STATE_SPEAKING:   return "משמיע תשובה...";
        case MUSE_STATE_ERROR:      return "שגיאה בתקשורת";
        default:                    return "מוכן לשאלה! לחץ והחזק";
    }
}

const char *muse_manager_get_last_query(void) {
    return s_last_query;
}

const char *muse_manager_get_last_response(void) {
    return s_last_response;
}

bool muse_manager_has_new_response(void) {
    if (s_has_new_response) {
        s_has_new_response = false;
        return true;
    }
    return false;
}
