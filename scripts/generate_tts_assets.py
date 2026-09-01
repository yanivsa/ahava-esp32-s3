import urllib.request
import urllib.parse
import subprocess
import os
import re

PHRASES = [
    # General feedback
    ("VOICE_SUCCESS_1", "כל הכבוד!"),
    ("VOICE_SUCCESS_2", "נכון מאוד!"),
    ("VOICE_SUCCESS_3", "מצוין!"),
    ("VOICE_SUCCESS_4", "איזה יופי!"),
    ("VOICE_SUCCESS_5", "אלופים!"),
    ("VOICE_RETRY_1", "בוא ננסה שוב"),
    ("VOICE_RETRY_2", "כמעט, נסה שוב"),
    
    # Core preschool prompts for Ayala (Age 3)
    ("VOICE_Q_CIRCLE_RED", "איפה העיגול האדום?"),
    ("VOICE_Q_SQUARE_BLUE", "איפה הריבוע הכחול?"),
    ("VOICE_Q_TRIANGLE_YELLOW", "איפה המשולש הצהוב?"),
    ("VOICE_Q_STAR", "איפה הכוכב הנוצץ?"),
    ("VOICE_Q_HEART", "איפה הלב הוורוד?"),
    ("VOICE_Q_APPLES", "כמה תפוחים יש כאן?"),
    ("VOICE_Q_STARS", "כמה כוכבים יש כאן?"),
    ("VOICE_Q_BUTTERFLIES", "כמה פרפרים יש כאן?"),
    ("VOICE_Q_BALLOONS", "כמה בלונים יש כאן?"),
    ("VOICE_Q_BIG_ELEPHANT", "מי יותר גדול, פיל או עכבר?"),
    ("VOICE_Q_SUN_COLOR", "מה הצבע של השמש הזורחת?"),
    ("VOICE_Q_GRASS_COLOR", "מה הצבע של הדשא בגינה?"),
    ("VOICE_Q_DOG_BARK", "איזו חיה נובחת האו האו?"),
    ("VOICE_Q_CAT_MEOW", "איזו חיה מגרגרת מיאו?"),
    ("VOICE_Q_COW_MOO", "איזו חיה גדולה עושה מווו?"),
    ("VOICE_Q_SHEEP_BAA", "איזו חיה רכה עושה מההה?"),
    ("VOICE_Q_DUCK_QUACK", "איזו חיה שוחה באגם ואומרת גע גע?"),
    ("VOICE_Q_ROOSTER", "מי קורא קוקוריקו בבוקר?"),
    ("VOICE_Q_RABBIT", "איזו חיה קופצת ואוכלת גזר?"),
    ("VOICE_Q_MONKEY", "איזו חיה מטפסת ואוהבת בננה?"),
    ("VOICE_Q_LION", "איזו חיה היא מלך החיות?"),
    ("VOICE_Q_FISH", "איזה דגיגון שוחה במים?"),
    ("VOICE_Q_SHABBAT_CANDLES", "מה אמא מדליקה בערב שבת?"),
    ("VOICE_Q_KIDDUSH", "מה שותים בקידוש של שבת?"),
    ("VOICE_Q_CHALLAH", "איזו חלה טעימה בוצעים בשבת?"),
    ("VOICE_Q_HONEY_APPLE", "מה טובלים בדבש בראש השנה?"),
    ("VOICE_Q_SHOFAR", "באיזה כלי מיוחד תוקעים בראש השנה?"),
    ("VOICE_Q_SUKKAH", "באיזה חג בונים סוכה?"),
    ("VOICE_Q_DREIDEL", "מה מסובבים בחנוכה?"),
    ("VOICE_Q_MENORAH", "מה מדליקים שמונה ימים בחנוכה?"),
    ("VOICE_Q_PURIM", "באיזה חג מתחפשים ושמחים?"),
    ("VOICE_Q_KIPPAH", "מה שמים הבנים על הראש?")
]

OUTPUT_HEADER = "include/voice_assets.h"
SAMPLE_RATE = 16000

print(f"Generating {len(PHRASES)} Hebrew TTS voice clips at {SAMPLE_RATE}Hz 16-bit Mono...")

temp_mp3 = "temp_tts.mp3"
temp_raw = "temp_tts.raw"

c_arrays = []
index_entries = []

for idx, (var_name, text) in enumerate(PHRASES):
    clean_prompt = text.replace("?", "").replace("!", "").strip()
    encoded = urllib.parse.quote(clean_prompt)
    url = f"https://translate.google.com/translate_tts?ie=UTF-8&q={encoded}&tl=he&client=tw-ob"
    
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    try:
        with urllib.request.urlopen(req) as resp, open(temp_mp3, 'wb') as f:
            f.write(resp.read())
        
        # Convert to 16kHz Mono 16-bit signed raw PCM
        subprocess.run([
            'ffmpeg', '-y', '-i', temp_mp3,
            '-ar', str(SAMPLE_RATE), '-ac', '1',
            '-f', 's16le', temp_raw
        ], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        
        with open(temp_raw, 'rb') as f:
            raw_bytes = f.read()
            
        byte_count = len(raw_bytes)
        # Format as C hex array
        hex_values = [f"0x{b:02x}" for b in raw_bytes]
        lines = []
        for i in range(0, len(hex_values), 16):
            lines.append("    " + ", ".join(hex_values[i:i+16]))
            
        array_code = f"// \"{clean_prompt}\" ({byte_count} bytes, {byte_count / (SAMPLE_RATE * 2):.2f}s)\n"
        array_code += f"static const uint8_t {var_name}_DATA[] PROGMEM = {{\n"
        array_code += ",\n".join(lines)
        array_code += "\n};\n"
        
        c_arrays.append(array_code)
        index_entries.append(f'    {{ "{clean_prompt}", {var_name}_DATA, {byte_count} }}')
        print(f"  [{idx+1}/{len(PHRASES)}] Synthesized '{clean_prompt}' -> {byte_count} bytes")
        
    except Exception as e:
        print(f"  ERROR generating '{text}': {e}")

if os.path.exists(temp_mp3): os.remove(temp_mp3)
if os.path.exists(temp_raw): os.remove(temp_raw)

header_content = f"""/**
 * @file voice_assets.h
 * @brief Hebrew TTS Offline Speech Assets for ESP32-S3 Wizard Academy
 * Generated automatically by scripts/generate_tts_assets.py
 * Sample Rate: {SAMPLE_RATE} Hz, 16-bit Mono PCM
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <pgmspace.h>

#ifdef __cplusplus
extern "C" {{
#endif

typedef struct {{
    const char *prompt;
    const uint8_t *data;
    size_t size;
}} VoiceClip_t;

{chr(10).join(c_arrays)}

static const VoiceClip_t VOICE_SUCCESS_CLIPS[] = {{
    {{ "כל הכבוד", VOICE_SUCCESS_1_DATA, sizeof(VOICE_SUCCESS_1_DATA) }},
    {{ "נכון מאוד", VOICE_SUCCESS_2_DATA, sizeof(VOICE_SUCCESS_2_DATA) }},
    {{ "מצוין", VOICE_SUCCESS_3_DATA, sizeof(VOICE_SUCCESS_3_DATA) }},
    {{ "איזה יופי", VOICE_SUCCESS_4_DATA, sizeof(VOICE_SUCCESS_4_DATA) }},
    {{ "אלופים", VOICE_SUCCESS_5_DATA, sizeof(VOICE_SUCCESS_5_DATA) }}
}};
#define VOICE_SUCCESS_COUNT 5

static const VoiceClip_t VOICE_RETRY_CLIPS[] = {{
    {{ "בוא ננסה שוב", VOICE_RETRY_1_DATA, sizeof(VOICE_RETRY_1_DATA) }},
    {{ "כמעט, נסה שוב", VOICE_RETRY_2_DATA, sizeof(VOICE_RETRY_2_DATA) }}
}};
#define VOICE_RETRY_COUNT 2

static const VoiceClip_t ALL_VOICE_CLIPS[] = {{
{",\n".join(index_entries)}
}};
#define TOTAL_VOICE_CLIPS {len(index_entries)}

#ifdef __cplusplus
}}
#endif
"""

with open(OUTPUT_HEADER, 'w', encoding='utf-8') as f:
    f.write(header_content)

total_kb = sum(os.path.getsize(OUTPUT_HEADER) for _ in [1]) / 1024
print(f"\nSuccessfully generated {OUTPUT_HEADER} ({total_kb:.1f} KB) with {len(PHRASES)} offline voice clips!")
