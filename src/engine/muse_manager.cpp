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
static char s_last_query[128] = "מהו הכוכב הכי גדול במערכת השמש?";
static char s_last_response[512] = "הכוכב הגדול ביותר במערכת השמש הוא כוכב הלכת צדק!";
static volatile bool s_has_new_response = false;
static uint32_t s_query_index = 0;
static TaskHandle_t s_worker_task_handle = NULL;
static volatile bool s_cancel_requested = false;

static void set_state(MuseState_t new_state) {
    s_state = new_state;
}

static void muse_process_query_async(void *pvParameters) {
    (void)pvParameters;
    Serial.printf("[MUSE] Async worker started on Core %d\n", xPortGetCoreID());

    while (1) {
        // Wait until unblocked after recording release
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        s_cancel_requested = false;

        Serial.println("[MUSE] Processing speech input...");

        // 1. Update UI Status to Thinking / Processing
        if (hal_lvgl_lock(portMAX_DELAY)) {
            ui_screen_muse_update_status("חושב על תשובה...", 0xF59E0B);
            hal_lvgl_unlock();
        }

        // 2. Select Question and Answer
        const char *query_text = nullptr;
        const char *answer_text = nullptr;

        bool online_success = false;
        if (!s_cancel_requested && WiFi.status() == WL_CONNECTED && strlen(AHAVA_MUSE_API_TOKEN) > 0) {
            Serial.printf("[MUSE] Sending query to endpoint: %s\n", AHAVA_MUSE_API_URL);
            esp_http_client_config_t config = {};
            config.url = AHAVA_MUSE_API_URL;
            config.crt_bundle_attach = esp_crt_bundle_attach;
            config.timeout_ms = 6000;
            config.keep_alive_enable = false;

            esp_http_client_handle_t client = esp_http_client_init(&config);
            if (client) {
                esp_http_client_set_method(client, HTTP_METHOD_POST);
                esp_http_client_set_header(client, "Content-Type", "application/json");
                const String auth = String("Bearer ") + AHAVA_MUSE_API_TOKEN;
                esp_http_client_set_header(client, "Authorization", auth.c_str());

                const char *post_data = "{\"action\":\"ask\"}";
                esp_http_client_set_post_field(client, post_data, strlen(post_data));

                esp_err_t err = esp_http_client_perform(client);
                if (err == ESP_OK) {
                    int status_code = esp_http_client_get_status_code(client);
                    if (status_code >= 200 && status_code < 300) {
                        Serial.printf("[MUSE] Server response HTTP %d\n", status_code);
                        online_success = true;
                    }
                }
                esp_http_client_cleanup(client);
            }
        }

        if (s_cancel_requested) {
            set_state(MUSE_STATE_IDLE);
            continue;
        }

        // 3. Fallback to on-device knowledge base if offline or direct
        if (!online_success) {
            uint32_t idx = s_query_index % KNOWLEDGE_BASE_SIZE;
            s_query_index++;
            query_text = MUSE_KNOWLEDGE_BASE[idx].query;
            answer_text = MUSE_KNOWLEDGE_BASE[idx].answer;
        }

        // Store into safe buffers
        strncpy(s_last_query, query_text, sizeof(s_last_query) - 1);
        s_last_query[sizeof(s_last_query) - 1] = '\0';

        strncpy(s_last_response, answer_text, sizeof(s_last_response) - 1);
        s_last_response[sizeof(s_last_response) - 1] = '\0';
        s_has_new_response = true;

        if (s_cancel_requested) {
            set_state(MUSE_STATE_IDLE);
            continue;
        }

        // 4. Update UI with text display on screen (Core 1 thread safety)
        set_state(MUSE_STATE_SPEAKING);
        if (hal_lvgl_lock(portMAX_DELAY)) {
            ui_screen_muse_update_response(s_last_query, s_last_response);
            ui_screen_muse_update_status("משמיע תשובה...", 0x10B981);
            hal_lvgl_unlock();
        }

        // 5. Play voice / audio feedback
        audio_play_voice_success();

        // 6. Wait for audio playback or yield
        for (int i = 0; i < 20; i++) {
            vTaskDelay(pdMS_TO_TICKS(100));
            if (s_cancel_requested) break;
            if (!audio_is_playing()) break;
        }

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
        4096,
        NULL,
        2,
        &s_worker_task_handle,
        0 // Core 0
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
