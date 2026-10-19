#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "subjects.h"

/** A question counts only when answered correctly before any hint/retry. */
static inline bool quiz_policy_should_count(bool is_correct, bool hint_used) {
    return is_correct && !hint_used;
}

/** Exam preparation is deliberate practice and never creates Time+ credit. */
static inline bool quiz_policy_grants_timeplus_credit(int subject_id) {
    return subject_id != AHAVA_SUBJECT_EXAM_PREP;
}

/** Only Judaism/Halacha is capped at ten counted questions per day. */
static inline uint32_t quiz_policy_daily_limit(int profile, int subject_id) {
    (void)profile;
    return subject_id == AHAVA_SUBJECT_RELIGION ? 10u : UINT32_MAX;
}

typedef enum {
    QUIZ_DAILY_KEEP = 0,
    QUIZ_DAILY_RESET,
    QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS
} QuizDailyBucketAction_t;

static inline QuizDailyBucketAction_t quiz_policy_daily_bucket_action(
    uint32_t today, uint32_t saved_date, bool has_unsynced_activity) {
    if (today == 0 || saved_date == today) return QUIZ_DAILY_KEEP;
    if (has_unsynced_activity) return QUIZ_DAILY_ADOPT_DATE_KEEP_COUNTS;
    return QUIZ_DAILY_RESET;
}

/** How a question should collect its final answer. AUTO is for per-question override storage. */
typedef enum {
    QUIZ_ANSWER_AUTO = 0,
    QUIZ_ANSWER_MULTIPLE_CHOICE = 1,
    QUIZ_ANSWER_TEXT = 2,
    QUIZ_ANSWER_NUMBER = 3
} QuizAnswerMode_t;

static inline size_t quiz_policy_utf8_decode(const unsigned char *s, uint32_t *cp) {
    if (!s || !*s || !cp) return 0;
    if (s[0] < 0x80) {
        *cp = s[0];
        return 1;
    }
    if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(s[0] & 0x1F) << 6) | (uint32_t)(s[1] & 0x3F);
        return 2;
    }
    if ((s[0] & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(s[0] & 0x0F) << 12) |
              ((uint32_t)(s[1] & 0x3F) << 6) | (uint32_t)(s[2] & 0x3F);
        return 3;
    }
    if ((s[0] & 0xF8) == 0xF0 && (s[1] & 0xC0) == 0x80 &&
        (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(s[0] & 0x07) << 18) |
              ((uint32_t)(s[1] & 0x3F) << 12) |
              ((uint32_t)(s[2] & 0x3F) << 6) | (uint32_t)(s[3] & 0x3F);
        return 4;
    }
    *cp = s[0];
    return 1;
}

static inline size_t quiz_policy_utf8_encode(uint32_t cp, char *out, size_t cap) {
    if (!out || cap == 0) return 0;
    if (cp < 0x80 && cap >= 1) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800 && cap >= 2) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000 && cap >= 3) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    if (cap >= 4) {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0;
}

/** Normalize user-entered text without fuzzy guessing: case, niqqud, punctuation and whitespace only. */
static inline void quiz_policy_normalize_text(const char *input, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!input) return;

    size_t wr = 0;
    bool pending_space = false;
    const unsigned char *p = (const unsigned char *)input;
    while (*p && wr + 1 < cap) {
        uint32_t cp = 0;
        size_t used = quiz_policy_utf8_decode(p, &cp);
        if (used == 0) break;
        p += used;

        /* Hebrew cantillation/niqqud and related combining marks. */
        if (cp >= 0x0591 && cp <= 0x05C7 && !(cp >= 0x05D0 && cp <= 0x05EA)) {
            continue;
        }

        bool keep = false;
        if (cp < 0x80) {
            if (isalnum((int)cp)) {
                cp = (uint32_t)tolower((int)cp);
                keep = true;
            }
        } else if (cp >= 0x05D0 && cp <= 0x05EA) {
            keep = true;
        }

        if (keep) {
            if (pending_space && wr > 0 && wr + 1 < cap) out[wr++] = ' ';
            pending_space = false;
            size_t added = quiz_policy_utf8_encode(cp, out + wr, cap - wr - 1);
            wr += added;
        } else {
            pending_space = wr > 0;
        }
    }
    while (wr > 0 && out[wr - 1] == ' ') --wr;
    out[wr] = '\0';
}

static inline bool quiz_policy_parse_plain_number(const char *input, double *value) {
    if (!input || !value) return false;
    char buf[96];
    size_t wr = 0;
    for (const unsigned char *p = (const unsigned char *)input; *p; ++p) {
        unsigned char c = *p;
        if (c == ' ' || c == '\t' || c == ',') continue;
        if (!(c == '+' || c == '-' || c == '.' || (c >= '0' && c <= '9'))) return false;
        if (wr + 1 >= sizeof(buf)) return false;
        buf[wr++] = (char)c;
    }
    if (wr == 0) return false;
    buf[wr] = '\0';
    char *end = nullptr;
    double parsed = strtod(buf, &end);
    if (!end || *end != '\0' || !isfinite(parsed)) return false;
    *value = parsed;
    return true;
}

static inline bool quiz_policy_extract_single_number(const char *text, double *value) {
    if (!text || !value) return false;
    int count = 0;
    double found = 0.0;
    const char *p = text;
    while (*p) {
        bool start = (*p >= '0' && *p <= '9') ||
                     ((*p == '-' || *p == '+') && p[1] >= '0' && p[1] <= '9');
        if (!start) {
            ++p;
            continue;
        }
        char *end = nullptr;
        double parsed = strtod(p, &end);
        if (!end || end == p) {
            ++p;
            continue;
        }
        found = parsed;
        ++count;
        if (count > 1) return false;
        p = end;
    }
    if (count != 1 || !isfinite(found)) return false;
    *value = found;
    return true;
}

static inline bool quiz_policy_scalar_alias_allowed(const char *canonical) {
    if (!canonical) return false;
    bool has_unit_or_assignment = false;
    const unsigned char *p = (const unsigned char *)canonical;
    while (*p) {
        uint32_t cp = 0;
        size_t used = quiz_policy_utf8_decode(p, &cp);
        if (!used) break;
        p += used;
        if (cp >= 0x05D0 && cp <= 0x05EA) has_unit_or_assignment = true;
        if (cp == '=' || cp == 0x00B0 || cp == 0x2032 || cp == 0x2033 || cp == '%' || cp == '"') {
            has_unit_or_assignment = true;
        }
        if (cp == '^' || cp == '/' || cp == '<' || cp == '>') return false;
    }
    return has_unit_or_assignment;
}

static inline void quiz_policy_normalize_math_expression(const char *input, char *out, size_t cap) {
    if (!out || cap == 0) return;
    out[0] = '\0';
    if (!input) return;
    size_t wr = 0;
    const unsigned char *p = (const unsigned char *)input;
    while (*p && wr + 1 < cap) {
        uint32_t cp = 0;
        size_t used = quiz_policy_utf8_decode(p, &cp);
        if (!used) break;
        p += used;
        if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == ',') continue;
        if (cp == 0x00D7) cp = '*';      /* multiplication sign */
        else if (cp == 0x00F7) cp = '/'; /* division sign */
        else if (cp == 0x2212) cp = '-'; /* unicode minus */
        else if (cp == 0x00B0 || cp == 0x2032 || cp == 0x2033) continue; /* unit marks */

        if (cp < 0x80) {
            if (isalpha((int)cp)) cp = (uint32_t)tolower((int)cp);
            const bool keep = isalnum((int)cp) || cp == '=' || cp == '^' || cp == '"' ||
                              cp == '-' || cp == '/' || cp == '%' || cp == '>' || cp == '<' ||
                              cp == '.' || cp == '+' || cp == '*' || cp == ':' ||
                              cp == '(' || cp == ')';
            if (!keep) return;
            out[wr++] = (char)cp;
        } else {
            return;
        }
    }
    out[wr] = '\0';
}

static inline bool quiz_policy_typed_answer_matches(
    QuizAnswerMode_t mode, const char *input, const char *canonical,
    const char *alias1, const char *alias2, const char *alias3) {
    if (mode != QUIZ_ANSWER_TEXT && mode != QUIZ_ANSWER_NUMBER) return false;
    if (!input || !canonical) return false;

    if (mode == QUIZ_ANSWER_NUMBER) {
        double a = 0.0, b = 0.0;
        if (quiz_policy_parse_plain_number(input, &a)) {
            if (quiz_policy_parse_plain_number(canonical, &b)) {
                return fabs(a - b) < 1e-9;
            }
            if (quiz_policy_scalar_alias_allowed(canonical) &&
                quiz_policy_extract_single_number(canonical, &b)) {
                return fabs(a - b) < 1e-9;
            }
        }

        char normalized_math_input[192];
        quiz_policy_normalize_math_expression(input, normalized_math_input, sizeof(normalized_math_input));
        if (!normalized_math_input[0]) return false;
        const char *math_accepted[4] = {canonical, alias1, alias2, alias3};
        for (const char *candidate : math_accepted) {
            if (!candidate || !*candidate) continue;
            char normalized_math_candidate[192];
            quiz_policy_normalize_math_expression(candidate, normalized_math_candidate,
                                                  sizeof(normalized_math_candidate));
            if (normalized_math_candidate[0] &&
                strcmp(normalized_math_input, normalized_math_candidate) == 0) return true;
        }
        return false;
    }

    char normalized_input[192];
    quiz_policy_normalize_text(input, normalized_input, sizeof(normalized_input));
    if (!normalized_input[0]) return false;

    const char *accepted[4] = {canonical, alias1, alias2, alias3};
    for (const char *candidate : accepted) {
        if (!candidate || !*candidate) continue;
        char normalized_candidate[192];
        quiz_policy_normalize_text(candidate, normalized_candidate, sizeof(normalized_candidate));
        if (strcmp(normalized_input, normalized_candidate) == 0) return true;
    }
    return false;
}

static inline size_t quiz_policy_codepoint_count(const char *text) {
    size_t n = 0;
    const unsigned char *p = (const unsigned char *)(text ? text : "");
    while (*p) {
        uint32_t cp = 0;
        size_t used = quiz_policy_utf8_decode(p, &cp);
        if (!used) break;
        p += used;
        ++n;
    }
    return n;
}

static inline size_t quiz_policy_word_count(const char *text) {
    if (!text) return 0;
    size_t words = 0;
    bool in_word = false;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        bool space = (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r');
        if (space) {
            in_word = false;
        } else if (!in_word) {
            ++words;
            in_word = true;
        }
    }
    return words;
}

static inline bool quiz_policy_contains_ascii_ci(const char *haystack, const char *needle) {
    if (!haystack || !needle || !*needle) return false;
    size_t needle_len = strlen(needle);
    for (const char *p = haystack; *p; ++p) {
        size_t i = 0;
        while (i < needle_len && p[i] &&
               tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) ++i;
        if (i == needle_len) return true;
    }
    return false;
}

static inline bool quiz_policy_choice_style_question(const char *question) {
    if (!question) return true;
    static const char *hebrew_cues[] = {
        "איזה משפט", "איזו מה", "איזה מה", "איזו אפשרות", "איזה צירוף",
        "איזו הגדרה", "איזה פרט", "מהו הפרט", "מי מהבאים", "מה מהבאים",
        "אינו תואם", "אינה תואמת", "המדויקת ביותר", "בחר"
    };
    for (const char *cue : hebrew_cues) if (strstr(question, cue)) return true;
    static const char *english_cues[] = {
        "which of", "which statement", "choose", "select", "best describes", "following"
    };
    for (const char *cue : english_cues) if (quiz_policy_contains_ascii_ci(question, cue)) return true;
    return false;
}

static inline bool quiz_policy_looks_numeric_answer(const char *answer) {
    if (!answer || !*answer) return false;
    double value = 0.0;
    if (quiz_policy_parse_plain_number(answer, &value)) return true;

    bool has_digit = false;
    for (const unsigned char *p = (const unsigned char *)answer; *p; ++p) {
        unsigned char c = *p;
        if (c >= '0' && c <= '9') {
            has_digit = true;
            continue;
        }
        if (c == ' ' || c == '\t' || c == '/' || c == '.' || c == ',' ||
            c == '+' || c == '-' || c == '%') continue;
        return false;
    }
    return has_digit;
}


static inline bool quiz_policy_looks_math_expression(const char *answer) {
    if (!answer || !*answer) return false;
    bool has_value = false;
    const unsigned char *p = (const unsigned char *)answer;
    while (*p) {
        uint32_t cp = 0;
        size_t used = quiz_policy_utf8_decode(p, &cp);
        if (!used) break;
        p += used;
        if ((cp >= '0' && cp <= '9') || cp == 'x' || cp == 'X' || cp == 'y' || cp == 'Y' ||
            cp == 'a' || cp == 'A' || cp == 'b' || cp == 'B' || cp == 'm' || cp == 'M') {
            has_value = true;
            continue;
        }
        if (cp == ' ' || cp == '\t' || cp == '=' || cp == '^' || cp == '"' || cp == '-' ||
            cp == '/' || cp == '%' || cp == '>' || cp == '<' || cp == '.' || cp == '+' ||
            cp == ',' || cp == '*' || cp == ':' || cp == '(' || cp == ')' ||
            cp == 0x00B0 || cp == 0x2032 || cp == 0x2033) {
            continue;
        }
        return false;
    }
    return has_value;
}

static inline bool quiz_policy_is_single_number_with_units(const char *answer) {
    if (!answer || !*answer) return false;
    double value = 0.0;
    if (!quiz_policy_extract_single_number(answer, &value)) return false;
    bool has_hebrew_or_unit = false;
    const unsigned char *p = (const unsigned char *)answer;
    while (*p) {
        uint32_t cp = 0;
        size_t used = quiz_policy_utf8_decode(p, &cp);
        if (!used) break;
        p += used;
        if ((cp >= 0x05D0 && cp <= 0x05EA) || cp == 0x00B0 || cp == 0x2032 ||
            cp == 0x2033 || cp == '%' || cp == '"') {
            has_hebrew_or_unit = true;
        }
    }
    return has_hebrew_or_unit;
}

static inline bool quiz_policy_is_hebrew_or_xy_answer(const char *answer) {
    if (!answer || !*answer) return false;
    bool has_hebrew = false;
    const unsigned char *p = (const unsigned char *)answer;
    while (*p) {
        uint32_t cp = 0;
        size_t used = quiz_policy_utf8_decode(p, &cp);
        if (!used) break;
        p += used;
        if (cp >= 0x05D0 && cp <= 0x05EA) {
            has_hebrew = true;
            continue;
        }
        if (cp >= 0x0591 && cp <= 0x05C7) continue;
        if (cp == 'x' || cp == 'X' || cp == 'y' || cp == 'Y' || cp == ' ' || cp == '\t' ||
            cp == '-' || cp == '"' || cp == 0x05F3 || cp == 0x05F4) {
            continue;
        }
        return false;
    }
    return has_hebrew;
}

/**
 * Conservative automatic conversion policy.
 * Profile IDs intentionally mirror WizardProfile_t without importing LVGL-heavy screen_manager.h:
 * 1=Ori, 2=Eitan, 3=Ayala.
 */
static inline QuizAnswerMode_t quiz_policy_answer_mode(
    int profile, int subject_id, const char *question, const char *correct_answer) {
    if (profile == 3 || profile <= 0 || !question || !correct_answer) {
        return QUIZ_ANSWER_MULTIPLE_CHOICE;
    }
    if (subject_id == AHAVA_SUBJECT_EXAM_PREP || subject_id == AHAVA_SUBJECT_CHALLENGES) {
        return QUIZ_ANSWER_MULTIPLE_CHOICE;
    }
    if (quiz_policy_choice_style_question(question)) return QUIZ_ANSWER_MULTIPLE_CHOICE;

    const bool short_answer = quiz_policy_codepoint_count(correct_answer) <= 32 &&
                              quiz_policy_word_count(correct_answer) <= 4;
    if (!short_answer) return QUIZ_ANSWER_MULTIPLE_CHOICE;

    if (profile == 2) {
        if (subject_id == AHAVA_SUBJECT_MATH && quiz_policy_looks_numeric_answer(correct_answer)) {
            return QUIZ_ANSWER_NUMBER;
        }
        return QUIZ_ANSWER_MULTIPLE_CHOICE;
    }

    if (profile != 1) return QUIZ_ANSWER_MULTIPLE_CHOICE;
    if (subject_id == AHAVA_SUBJECT_MATH) {
        if (quiz_policy_looks_math_expression(correct_answer) ||
            quiz_policy_is_single_number_with_units(correct_answer)) {
            return QUIZ_ANSWER_NUMBER;
        }
        if (quiz_policy_is_hebrew_or_xy_answer(correct_answer)) return QUIZ_ANSWER_TEXT;
        return QUIZ_ANSWER_MULTIPLE_CHOICE;
    }
    if (subject_id == AHAVA_SUBJECT_HEBREW || subject_id == AHAVA_SUBJECT_ENGLISH ||
        subject_id == AHAVA_SUBJECT_RELIGION) {
        return QUIZ_ANSWER_TEXT;
    }
    return QUIZ_ANSWER_MULTIPLE_CHOICE;
}
