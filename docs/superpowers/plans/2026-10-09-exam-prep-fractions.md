# Fractions Exam Prep Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** להוסיף לאקדמיית הקוסמים עבור אורי נושא „הכנה למבחן” עם 70 תרגילים בלבד של כפל שבר בשלם וכפל מספר מעורב בשלם, ב־7 רמות קושי, עם דרך חובה וללא זיכוי בדקות Time+.

**Architecture:** הנושא החדש הוא קטגוריית ניווט נפרדת (`AHAVA_SUBJECT_EXAM_PREP`) ולא קטגוריה אקדמית. בנק השאלות יישמר כמקור JSON ייעודי וייוצר ממנו include לקושחה. לכל שאלה סוג פעולה מוגדר, רמת קושי ושלב/שלבי דרך לפני תשובה סופית. התקדמות 10×7 נשמרת ב־NVS.

**Tech Stack:** C++17/Arduino + LVGL, ESP-IDF NVS, PlatformIO native tests, Python 3.12 generator/validator, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-10-09-exam-prep-fractions.md`

## Global Constraints

- בדיוק 70 שאלות: 7 רמות × 10.
- בכל רמה בדיוק 5 שאלות שבר×שלם ו־5 שאלות מספר מעורב×שלם.
- כל 70 השאלות נשארות בתוך שתי פעולות הכפל האלה בלבד.
- אסור להוסיף אלגברה, גאומטריה, שטח/היקף, בעיות מילוליות, משוואות, יחס, אחוזים, פרמטרים, אופטימיזציה או „מצא את הנעלם”.
- רמה 1 ברמת החוברת; כל 10 שאלות הרמה עולה משמעותית באמצעות המספרים, המכנים, הצמצום, ההמרה ומספר שלבי הדרך בלבד.
- כל תשובה סופית מצומצמת; תוצאה גדולה מ־1 מוצגת כמספר מעורב כשמתאים.
- בכל שאלה יוצג: `✍️ כתוב דרך מלאה במחברת לפני בחירת תשובה.`
- כל שאלה כוללת לפחות work checkpoint אחד; ברמות 4–7 מותר/מומלץ checkpoint נוסף כאשר נדרש.
- הנושא מוצג לאורי בלבד.
- „הכנה למבחן” לא מזכה בדקות Time+, לא נכנסת ל־results sync ולא לסטטיסטיקה האקדמית.
- רק יהדות מוגבלת ל־10 שאלות נספרות ביום; הכנה למבחן אינה מוגבלת.
- `AHAVA_ACADEMIC_SUBJECT_COUNT` נשאר 4.
- אין להעתיק שאלות מהחוברת מילה במילה.
- validator משתמש ב־`fractions.Fraction`; אין floating point.

## Review Focus

1. **Scope leak:** validator חייב לדחות שאלה שאינה בדיוק `fraction_times_whole` או `mixed_times_whole`.
2. **Time+ leak:** פתרון שאלת הכנה למבחן לא משנה שום מונה/ledger/sync שמזכה בדקות.
3. **Work-step bypass:** אי אפשר להגיע לתשובה סופית בלי להשלים checkpoint נכון.
4. **Equivalent answers:** אין שתי אפשרויות תשובה שקולות אחרי צמצום/המרה.
5. **RTL/LTR:** `37 × 12 7/18` ושברים דומים מוצגים בסדר יציב על 480×320.

---

### Task 1: Define subject and reward policy

**Files:**
- Modify: `include/subjects.h`
- Modify: `include/quiz_policy.h`
- Modify: `test/test_subject_contract/test_main.cpp`
- Modify: `test/test_quiz_policy/test_main.cpp`

**Interfaces:**
- `AHAVA_SUBJECT_EXAM_PREP = 5`
- `AHAVA_SUBJECT_COUNT = 6`
- `AHAVA_ACADEMIC_SUBJECT_COUNT = 4` unchanged
- `bool quiz_policy_grants_timeplus_credit(int subject_id)`
- `quiz_policy_daily_limit(...)` returns `10` only for `AHAVA_SUBJECT_RELIGION`; all others `UINT32_MAX`.

- [ ] Write failing tests for subject count, IDs, academic count and reward policy.
- [ ] Run `pio test -e native`; verify expected failures.
- [ ] Implement subject constant and policy helper.
- [ ] Run `pio test -e native`; expect PASS.
- [ ] Commit: `feat: define exam prep subject policy`.

---

### Task 2: Add explicit exam-prep operation contract

**Files:**
- Create: `include/exam_prep_contract.h`
- Create: `test/test_exam_prep_contract/test_main.cpp`

**Interfaces:**
```cpp
typedef enum {
    EXAM_PREP_FRACTION_TIMES_WHOLE = 1,
    EXAM_PREP_MIXED_TIMES_WHOLE = 2
} ExamPrepOperation_t;

bool exam_prep_operation_valid(int operation);
bool exam_prep_tier_valid(uint8_t tier);
```

**Behavior:**
- only operation IDs 1 and 2 are valid;
- only tiers 1..7 are valid.

- [ ] Write failing tests for valid and invalid operation/tier values.
- [ ] Run native tests and verify failure.
- [ ] Implement minimal contract helpers.
- [ ] Run native tests and verify PASS.
- [ ] Commit: `feat: define exam prep operation contract`.

---

### Task 3: Extend question model with mandatory work checkpoints

**Files:**
- Modify: `include/quiz_engine.h`
- Modify: `src/engine/quiz_engine.cpp`
- Create: `test/test_exam_prep_question_contract/test_main.cpp`

**Interfaces:**
Append fields to `Question_t` after existing fields:
- `uint8_t difficulty_tier` — 0 for legacy, 1..7 for exam prep.
- `uint8_t exam_prep_operation` — 0 for legacy, 1/2 for exam prep.
- `const char *work_prompt_1`
- `const char *work_answers_1[4]`
- `uint8_t work_correct_idx_1`
- `const char *work_prompt_2` — optional; null for one-checkpoint questions.
- `const char *work_answers_2[4]`
- `uint8_t work_correct_idx_2`

Produce phase enum:
```cpp
typedef enum {
    QUIZ_PHASE_WORK_1 = 1,
    QUIZ_PHASE_WORK_2 = 2,
    QUIZ_PHASE_FINAL = 3
} QuizPhase_t;
```

**Behavior:**
- legacy questions go directly to FINAL;
- exam-prep question starts at WORK_1;
- correct WORK_1 goes to WORK_2 if present, else FINAL;
- wrong work answer does not finalize/count/reward;
- FINAL behaves through normal answer handling.

- [ ] Write failing phase-transition tests.
- [ ] Run native tests; verify failure.
- [ ] Append fields without changing the meaning/order of existing fields.
- [ ] Implement phase state and transitions.
- [ ] Run native tests; expect PASS.
- [ ] Commit: `feat: require work checkpoints for exam prep`.

---

### Task 4: Build the strict 70-question bank and validator

**Files:**
- Create: `data/questions/ori_exam_prep/fractions_multiplication.json`
- Create: `scripts/generate_ori_exam_prep_questions.py`
- Generate: `src/engine/generated_ori_exam_prep.inc`

**Source schema per question:**
- `id`: 60000..60069 unique
- `tier`: 1..7
- `operation`: `fraction_times_whole` or `mixed_times_whole`
- exact numeric operands (`whole`, and rational/mixed components)
- `prompt`
- `workPrompt1`, `workOptions1[4]`, `workCorrect1`
- optional `workPrompt2`, `workOptions2[4]`, `workCorrect2`
- `options[4]`, `correct`
- `hint`, `feedback`

**Validator invariants:**
- exactly 70 total;
- exactly 10 per tier;
- exactly 5 `fraction_times_whole` + 5 `mixed_times_whole` per tier;
- operation outside these two => hard failure;
- no text patterns for geometry/area/perimeter/equations/word-problem domains;
- all four answers are mathematically distinct using `Fraction` normalization;
- correct final answer equals exact multiplication result;
- all final answers are fully reduced;
- each question has at least work checkpoint 1;
- tiers 4–7 contain a meaningful second checkpoint when the generated metadata marks the question `two_step_work=true`.

**Difficulty ladder — same two operations only:**

Tier 1 — workbook level:
- whole numbers 2–10;
- common denominators 2,3,4,5,6,8,10;
- easy simplification/conversion.

Tier 2 — nontrivial reduction:
- whole numbers up to ~20;
- denominators up to 12;
- more improper results and conversion to mixed numbers.

Tier 3 — reduce before multiplying:
- whole numbers up to ~30;
- denominators up to 15;
- numbers selected so early cancellation is the efficient route.

Tier 4 — larger arithmetic:
- whole numbers up to ~50;
- denominators up to 20;
- both pre-cancellation and final reduction/conversion frequently required.

Tier 5 — gifted numerical level:
- whole numbers up to ~80;
- denominators up to 30;
- mixed numbers such as `12 7/18`, `17 11/24`;
- distractors based on realistic calculation mistakes.

Tier 6 — very hard numerical level:
- whole numbers up to ~120;
- denominators up to 48;
- multiple possible cancellations;
- 2–3 meaningful intermediate transformations.

Tier 7 — elite numerical level:
- still exactly one multiplication expression of an allowed type;
- whole numbers up to ~200;
- denominators up to 72;
- naive arithmetic is long but factor recognition/cancellation makes it manageable;
- no variables, proofs, geometry or word problems.

- [ ] Implement validator first; create a deliberately invalid fixture and verify `--check` fails.
- [ ] Add all 70 questions to JSON according to the exact 5+5 tier balance.
- [ ] Run `python scripts/generate_ori_exam_prep_questions.py --check`; expect `70 valid; 10/tier; 5+5 operations/tier`.
- [ ] Generate include twice and verify second run has no diff.
- [ ] Commit: `feat: add strict 70-question fraction multiplication bank`.

---

### Task 5: Persist mastery progress without academic counters

**Files:**
- Create: `include/exam_prep_progress.h`
- Modify: `include/player_data.h`
- Modify: `src/engine/player_data.cpp`
- Create: `test/test_exam_prep_progress/test_main.cpp`

**Interfaces:**
- 70 solved bits in 3×`uint32_t`.
- `player_data_exam_prep_mark_solved(PROFILE_ORI, ordinal)`
- `player_data_exam_prep_is_solved(...)`
- `player_data_exam_prep_total_solved(...)`
- `player_data_exam_prep_current_tier(...)`
- `player_data_exam_prep_tier_solved(...)`

**Constraints:**
- this progress store is independent of academic daily counters and Time+ sync;
- only Ori may mutate it;
- tier N+1 unlocks only after all 10 bits of tier N are solved.

- [ ] Write bit-boundary and tier-unlock tests for ordinals 0,9,10,69.
- [ ] Run native tests; verify failure.
- [ ] Implement pure bitset + NVS wrapper.
- [ ] Run native tests; expect PASS.
- [ ] Commit: `feat: persist exam prep mastery`.

---

### Task 6: Integrate question bank into quiz engine without Time+ credit

**Files:**
- Modify: `src/engine/quiz_engine.cpp`
- Modify: `src/engine/player_data.cpp`
- Modify: `src/engine/results_sync.cpp` only if needed to add an explicit guard
- Add/modify native tests for reward isolation

**Behavior:**
- include `generated_ori_exam_prep.inc` as a separate bank;
- selectable only for `PROFILE_ORI` + `AHAVA_SUBJECT_EXAM_PREP`;
- selection stays within current unlocked tier until its 10 questions are mastered;
- solved questions are deprioritized until all unsolved questions in tier have been served;
- final correct answer marks mastery progress;
- do not call academic question counting / weekly stats / result sync for this subject;
- Time+ credit helper returns false for this subject.

- [ ] Write failing test proving three correct exam-prep answers change mastery but leave academic/sync credit unchanged.
- [ ] Run native tests; verify failure.
- [ ] Wire bank and isolation guards.
- [ ] Run native tests; expect PASS.
- [ ] Commit: `feat: isolate exam prep from Time+ rewards`.

---

### Task 7: Add Ori-only dashboard card and work-step UI

**Files:**
- Modify: `src/ui/screen_manager.cpp`
- Modify: `preview/index.html`
- Modify/add UI validation tests as appropriate

**UI:**
- Ori dashboard gets a new card: `הכנה למבחן`.
- Ethan/Ayala do not see it.
- Card shows current tier and progress, e.g. `רמה 4 · 6/10 · 36/70`.
- Quiz header shows `הכנה למבחן · רמה N`.
- Work card always shows `✍️ כתוב דרך מלאה במחברת לפני בחירת תשובה.`
- Work checkpoint(s) render before final answers.
- Fraction/mixed expressions remain readable and stable in RTL/LTR.

- [ ] Add failing contract/UI checks for Ori-only visibility and current tier text.
- [ ] Implement card without relying on a contiguous 0..5 dashboard loop.
- [ ] Implement WORK_1/WORK_2/FINAL rendering.
- [ ] Verify preview at 480×320 with representative Tier 1 and Tier 7 expressions.
- [ ] Commit: `feat: add exam prep UI`.

---

### Task 8: CI, regression and release gates

**Files:**
- Modify: `.github/workflows/firmware-release.yml`
- Add validator invocation if not already covered by tests.

**Required gates:**
1. `python scripts/generate_ori_exam_prep_questions.py --check`
2. `pio test -e native`
3. `python scripts/verify_results_cloud_contract.py`
4. `pio run -e spotpear-esp32s3-max35`

**Regression checks:**
- existing Math/Hebrew/English/Judaism/Challenges still load;
- only Judaism daily limit = 10;
- exam prep produces zero Time+ credit;
- exactly 70 questions, 10/tier, 5+5 operations/tier;
- no exam-prep question contains a disallowed topic;
- reboot preserves mastery and tier;
- final 70/70 unlocks master practice only over the same 70 multiplication questions.

- [ ] Add validator gate.
- [ ] Run all gates locally/CI.
- [ ] Record artifact SHA and QA result.
- [ ] Commit: `ci: gate exam prep question scope and rewards`.

---

## Self-review

- Spec coverage: all 70 questions restricted to the two requested multiplication types.
- Difficulty increase: achieved only through operand size, denominator complexity, cancellation, conversion and number of work steps.
- No geometry/algebra/word-problem scope remains.
- 35/35 balance is mechanically validated.
- Time+ isolation has an explicit policy test and integration regression.
- Mastery progression remains 10 questions per tier and 70/70 overall.
