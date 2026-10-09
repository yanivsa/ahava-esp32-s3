# Fractions Exam Prep Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** להוסיף לאקדמיית הקוסמים עבור אורי נושא „הכנה למבחן” עם 70 שאלות כפל שברים/מספרים מעורבים ב־7 רמות, שלב דרך חובה לפני תשובה סופית, וללא זיכוי בדקות Time+.

**Architecture:** הנושא החדש יהיה קטגוריית ניווט נפרדת (`AHAVA_SUBJECT_EXAM_PREP`) אך לא קטגוריה אקדמית לצורכי סטטיסטיקה/Time+. בנק השאלות יהיה מקור נתונים ייעודי שמייצר include לקושחה, כאשר כל שאלה מכילה רמה, שלב דרך ותשובה סופית. התקדמות 10×7 תישמר ב־NVS, והרמה הבאה תיפתח רק אחרי שכל 10 השאלות ברמה הנוכחית נפתרו נכון לפחות פעם אחת.

**Tech Stack:** C++17/Arduino + LVGL, ESP-IDF NVS, PlatformIO native tests, Python 3.12 generator/validator, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-10-09-exam-prep-fractions.md`

## Global Constraints

- בדיוק 70 שאלות מקוריות: 7 רמות × 10 שאלות.
- רמה 1 ברמת החוברת; כל 10 שאלות הרמה עולה משמעותית.
- הנושא מוצג לאורי בלבד.
- כל שאלה מחייבת שלב דרך דיגיטלי לפני הצגת התשובות הסופיות.
- בכל שאלה יוצג: `✍️ כתוב דרך מלאה במחברת לפני בחירת תשובה.`
- „הכנה למבחן” לא מזכה בדקות Time+, לא נכנסת ל־results sync ולא לסטטיסטיקה האקדמית.
- רק יהדות מוגבלת ל־10 שאלות נספרות ביום; הכנה למבחן אינה מוגבלת.
- `AHAVA_ACADEMIC_SUBJECT_COUNT` נשאר 4.
- שאלות קיימות והתנהגות קיימת של יתר הנושאים לא ישתנו, למעט תיקון מדיניות מגבלת 10 השאלות.
- אין להעתיק שאלות מהחוברת מילה במילה.
- חישובי שברים ב־generator/validator יהיו רציונליים מדויקים, ללא floating point.

## Review Focus

1. **דליפת זיכוי Time+:** פתרון שאלה בנושא הכנה למבחן חייב להשאיר את מוני הזיכוי/`results_sync` ללא שינוי.
2. **פער ID 4/5 בפרופיל אורי:** חידות הקוסמים הן subject 4 והכנה למבחן subject 5; אין להסתמך על לולאת subject IDs רציפה בדשבורד.
3. **RTL/LTR בשברים מעורבים:** ביטויים כגון `7 × 2 1/3` חייבים להופיע בסדר נכון על 480×320.
4. **עקיפת שלב הדרך:** אסור להגיע לתשובה הסופית או לקבל השלמת שאלה לפני בחירה נכונה בשלב הדרך.
5. **התקדמות/אתחול:** reboot באמצע רמה לא יאפס התקדמות, לא ידלג רמה ולא יחזיר שאלות שכבר סומנו solved אלא אם נבחר מצב תרגול מאסטר.

---

### Task 1: Subject contract and reward policy

**Files:**
- Modify: `include/subjects.h`
- Modify: `include/quiz_policy.h`
- Modify: `test/test_subject_contract/test_main.cpp`
- Modify: `test/test_quiz_policy/test_main.cpp`

**Interfaces:**
- Produces: `AHAVA_SUBJECT_EXAM_PREP = 5`
- Produces: `AHAVA_SUBJECT_COUNT = 6`
- Preserves: `AHAVA_ACADEMIC_SUBJECT_COUNT = 4`
- Produces: `bool quiz_policy_grants_timeplus_credit(int subject_id)`
- Changes: `quiz_policy_daily_limit(...)` returns `10` only for `AHAVA_SUBJECT_RELIGION`; all other subjects return `UINT32_MAX`.

- [ ] **Step 1: Write failing subject-contract tests**

Assert:
- subject count is 6;
- exam prep ID is 5;
- challenges ID remains 4;
- academic subject count remains 4;
- weekly stats subject count remains 4.

- [ ] **Step 2: Write failing reward-policy tests**

Assert:
- `quiz_policy_grants_timeplus_credit(AHAVA_SUBJECT_EXAM_PREP) == false`;
- math/hebrew/english/religion remain credit-eligible;
- exam prep daily limit is unlimited;
- religion daily limit is exactly 10;
- math/hebrew/english/challenges are unlimited.

- [ ] **Step 3: Run native tests and confirm failure**

Run: `pio test -e native`
Expected: subject/policy tests fail because subject 5 and the new policy helper do not exist yet.

- [ ] **Step 4: Implement the subject and policy constants**

Keep academic buckets at 0..3. Do not add exam prep to weekly academic stats.

- [ ] **Step 5: Run native tests**

Run: `pio test -e native`
Expected: PASS.

- [ ] **Step 6: Commit**

Commit message: `feat: define exam prep subject policy`

---

### Task 2: Extend question model for mandatory work steps

**Files:**
- Modify: `include/quiz_engine.h`
- Modify: `src/engine/quiz_engine.cpp`
- Test: `test/test_subject_contract/test_main.cpp` or a new `test/test_exam_prep_question_contract/test_main.cpp`

**Interfaces:**
Append these fields to `Question_t` so existing aggregate initializers remain source-compatible and default to zero/null:
- `uint8_t difficulty_tier` — 0 for existing questions, 1..7 for exam prep.
- `const char *work_prompt` — null/empty for existing questions.
- `const char *work_answers[4]` — four intermediate-step options.
- `uint8_t work_correct_idx` — 0..3.

Produce:
- `typedef enum { QUIZ_PHASE_FINAL = 0, QUIZ_PHASE_WORK = 1 } QuizPhase_t;`
- `QuizPhase_t quiz_get_phase(void);`
- `bool quiz_check_work_choice(uint8_t selected_idx);`

Behavior:
- `quiz_get_next_question()` sets WORK phase only when `work_prompt` exists.
- During WORK phase the proxy must not finalize, reward, count, or reveal the final answer.
- `quiz_check_work_choice()` returns true only for the correct intermediate step and then changes phase to FINAL.

- [ ] **Step 1: Add failing tests for legacy compatibility and work-phase contract**

Verify a normal question defaults to FINAL; an exam-prep-shaped question starts in WORK; wrong work choice stays WORK; correct work choice moves to FINAL.

- [ ] **Step 2: Run tests and confirm failure**

Run: `pio test -e native`
Expected: FAIL on missing fields/API.

- [ ] **Step 3: Append fields and implement phase state**

Do not alter the meaning/order of the existing first 9 fields in `Question_t`.

- [ ] **Step 4: Run tests**

Run: `pio test -e native`
Expected: PASS.

- [ ] **Step 5: Commit**

Commit message: `feat: add mandatory work phase to quiz questions`

---

### Task 3: Build and validate the 70-question source bank

**Files:**
- Create: `data/questions/ori_exam_prep/fractions_multiplication.json`
- Create: `scripts/generate_ori_exam_prep_questions.py`
- Generate: `src/engine/generated_ori_exam_prep.inc`

**Interfaces:**
Each source question must contain:
- `id` — fixed unique ID in range `60000..60069`;
- `tier` — 1..7;
- `prompt`;
- `workPrompt`;
- `workOptions[4]`;
- `workCorrect`;
- `options[4]`;
- `correct`;
- `hint`;
- `feedback`;
- optional exact rational metadata used by validator when the answer is numeric.

Generator command:
- `python scripts/generate_ori_exam_prep_questions.py`
- `python scripts/generate_ori_exam_prep_questions.py --check`

Generated rows must use:
- subject: `AHAVA_SUBJECT_EXAM_PREP`;
- profile: `PROFILE_ORI`;
- `stats_subject_id = -1`;
- `difficulty_tier = 1..7`.

- [ ] **Step 1: Implement generator validation before adding all content**

`--check` must fail unless:
- total = 70;
- exactly 10 questions per tier;
- IDs are unique and exactly 60000..60069;
- every final/work option set has four distinct entries;
- both correct indexes are 0..3;
- every question has non-empty prompt, work prompt, hint and feedback;
- numeric answer metadata, where supplied, matches the selected answer using Python `fractions.Fraction`.

- [ ] **Step 2: Add the 70 original questions with this exact tier mix**

Tier 1 — workbook level:
- 4 direct whole × proper fraction;
- 3 whole × mixed number;
- 1 comparison;
- 1 short word problem;
- 1 rectangle-area problem.

Tier 2 — stronger computation:
- 3 calculations requiring simplification/conversion;
- 2 mixed-number products with larger values;
- 2 comparisons/estimation;
- 2 word/geometry applications;
- 1 choose-the-efficient-method question.

Tier 3 — multi-step:
- 2 two-step word problems;
- 2 reverse/missing-factor problems;
- 2 compare-without-full-calculation problems;
- 2 area/perimeter applications;
- 2 mixed reasoning calculations.

Tier 4 — algebra/error analysis:
- 3 missing-value/equation questions;
- 2 error-analysis questions;
- 2 distributive-property questions;
- 2 geometry applications;
- 1 constrained-choice question.

Tier 5 — gifted:
- 2 rate/unit questions;
- 2 multi-step geometry questions;
- 2 inequality/comparison questions;
- 2 reverse-construction questions;
- 2 multi-condition word problems.

Tier 6 — olympiad-focused:
- 3 integer-result/divisibility questions;
- 2 parameter-search questions;
- 2 optimization questions;
- 2 comparison/proof-style questions;
- 1 synthesis word problem.

Tier 7 — elite:
- 2 “find all values” questions;
- 2 proof/equivalence questions;
- 2 min/max questions;
- 2 multi-condition construction/modeling questions;
- 2 olympiad-style synthesis questions.

Every one of the 70 questions must have a meaningful work step that is not merely the final answer rewritten.

- [ ] **Step 3: Run generator validation**

Run: `python scripts/generate_ori_exam_prep_questions.py --check`
Expected: `70 questions valid; 10 per tier` and exit code 0.

- [ ] **Step 4: Generate the C++ include and check deterministic output**

Run twice and verify no diff on the second run.

- [ ] **Step 5: Commit**

Commit message: `feat: add 70-tier fractions exam prep bank`

---

### Task 4: Persist 7-level mastery progress

**Files:**
- Create: `include/exam_prep_progress.h`
- Modify: `include/player_data.h`
- Modify: `src/engine/player_data.cpp`
- Create: `test/test_exam_prep_progress/test_main.cpp`

**Interfaces:**
Pure progress helpers:
- `bool exam_prep_progress_is_solved(const uint32_t words[3], uint16_t ordinal);`
- `void exam_prep_progress_mark_solved(uint32_t words[3], uint16_t ordinal);`
- `uint16_t exam_prep_progress_total_solved(const uint32_t words[3]);`
- `uint8_t exam_prep_progress_current_tier(const uint32_t words[3]);`
- `uint8_t exam_prep_progress_tier_solved(const uint32_t words[3], uint8_t tier);`

Persistent API:
- `bool player_data_exam_prep_is_solved(WizardProfile_t profile, uint16_t ordinal);`
- `void player_data_exam_prep_mark_solved(WizardProfile_t profile, uint16_t ordinal);`
- `uint16_t player_data_exam_prep_total_solved(WizardProfile_t profile);`
- `uint8_t player_data_exam_prep_current_tier(WizardProfile_t profile);`
- `uint8_t player_data_exam_prep_tier_solved(WizardProfile_t profile, uint8_t tier);`

Storage:
- 70 solved bits in a 3×`uint32_t` NVS blob per profile.
- Only `PROFILE_ORI` is allowed to mutate exam-prep progress.

- [ ] **Step 1: Write failing pure progress tests**

Cover ordinals 0, 9, 10, 69; exact tier boundaries; duplicate mark idempotency; tier advance only after all ten bits are set; all 70 solved returns tier 7/completed state.

- [ ] **Step 2: Run tests and confirm failure**

Run: `pio test -e native`
Expected: FAIL because progress helpers do not exist.

- [ ] **Step 3: Implement pure bitset helpers and NVS wrapper**

Do not reuse Time+/academic counters for this progress.

- [ ] **Step 4: Run tests**

Run: `pio test -e native`
Expected: PASS.

- [ ] **Step 5: Commit**

Commit message: `feat: persist exam prep mastery progress`

---

### Task 5: Integrate the new bank and tier-aware selection

**Files:**
- Modify: `src/engine/quiz_engine.cpp`
- Modify: `include/quiz_engine.h`
- Test: `test/test_exam_prep_progress/test_main.cpp`

**Interfaces:**
- Include `generated_ori_exam_prep.inc` as a fourth bank after base/religion/math.
- `quiz_get_question_count(PROFILE_ORI, AHAVA_SUBJECT_EXAM_PREP)` returns exactly 70.
- Ethan/Ayala count for exam prep returns 0.
- Stable exam-prep ordinal is 0..69 across the full subject, independent of current tier.

Selection rules:
- only questions from `player_data_exam_prep_current_tier(PROFILE_ORI)` are eligible for new selection;
- solved questions in the current tier are skipped while unsolved ones exist;
- wrong questions continue to use the existing retry/cooldown mechanism;
- on correct FINAL answer after successful WORK phase, mark that question solved;
- once 10/10 in a tier are solved, the next fetch uses the next tier;
- after 70/70, enter master-practice mode and allow all 70 to rotate again without changing Time+ credit behavior.

- [ ] **Step 1: Write failing selection tests/pure helpers**

Cover:
- tier 1 cannot return tier 2 before 10/10 solved;
- solved items are not selected while unsolved items remain;
- tier changes from 1→2 at exactly ten solved;
- non-Ori profile gets no exam prep question;
- 70/70 activates master pool.

- [ ] **Step 2: Run tests and confirm failure**

Run: `pio test -e native`
Expected: FAIL.

- [ ] **Step 3: Add bank count, question lookup and tier filtering**

Keep `question_ordinal()` stable across all 70; do not calculate ordinals from a tier-filtered list.

- [ ] **Step 4: Mark solved only after final correctness**

A correct intermediate work step alone must never mark the question solved.

- [ ] **Step 5: Run tests**

Run: `pio test -e native`
Expected: PASS.

- [ ] **Step 6: Commit**

Commit message: `feat: add tiered exam prep question selection`

---

### Task 6: Implement the two-stage “show your work” UI

**Files:**
- Modify: `src/ui/screen_manager.cpp`
- Modify: `include/screen_manager.h` only if a small rendering helper must be exposed internally
- Modify: `src/engine/quiz_engine.cpp`

**Interfaces:**
Screen state additions in `screen_manager.cpp`:
- persistent handle for question label;
- persistent handles for four answer labels;
- helper `static void render_quiz_stage(const Question_t *q, QuizPhase_t phase);`

Behavior:
- initial render for exam prep shows instruction + `work_prompt` and `work_answers`;
- work-choice click calls `quiz_check_work_choice(index)`;
- wrong work choice stays in WORK and uses a hint/cooldown without calling finalization/reward paths;
- correct work choice rerenders the same card with `q->text` + final answers and phase FINAL;
- only FINAL choice reaches the existing final-answer path;
- new question resets to WORK.

Exact instruction copy:
`✍️ כתוב דרך מלאה במחברת לפני בחירת תשובה.`

- [ ] **Step 1: Add a regression test/helper contract that WORK cannot finalize**

At minimum, a native policy test must prove the work phase never produces an eligible finalization event.

- [ ] **Step 2: Implement stage renderer without duplicating the entire quiz screen**

Reuse the existing question card and four buttons; update text in place when advancing WORK→FINAL.

- [ ] **Step 3: Verify wrong work answer behavior**

Expected: hint/retry only; no green final-answer state, no progress mark, no coins/XP, no counter change.

- [ ] **Step 4: Verify correct work then correct final behavior**

Expected: final feedback appears, question is marked solved, next question loads according to tier policy.

- [ ] **Step 5: Commit**

Commit message: `feat: require work step before exam prep answers`

---

### Task 7: Hard-block Time+ credit for exam prep

**Files:**
- Modify: `src/engine/quiz_engine.cpp`
- Modify: `src/ui/screen_manager.cpp`
- Modify: `scripts/verify_results_cloud_contract.py`
- Modify: relevant native tests under `test/`

**Interfaces:**
Use only `quiz_policy_grants_timeplus_credit(subject_id)` to decide whether the final answer may call:
- `player_data_prepare_question_count(...)`;
- `player_data_increment_questions_today(...)`;
- any path that ultimately records `weekly_stats_store_record_correct(...)` or `results_sync_store_record_correct(...)`.

Exam prep must still be allowed to update its own mastery bitset.

Local XP/coins may remain on correct FINAL answers because the requirement forbids Time+ minutes, not local gamification.

- [ ] **Step 1: Write failing no-credit regression tests**

Assert an exam-prep correct answer:
- does not increment academic/day credit counters;
- cannot create a dirty result-sync record;
- does update exam-prep solved progress.

- [ ] **Step 2: Extend cloud-contract verifier**

Assert results sync accepts only the four academic keys: `math`, `hebrew`, `english`, `religion`; there is no `exam_prep` outbound subject key.

- [ ] **Step 3: Implement explicit reward gating**

Do not rely only on `stats_subject_id = -1` accidentally failing validation; make the no-credit rule explicit in quiz policy.

- [ ] **Step 4: Run tests and cloud contract**

Run:
- `pio test -e native`
- `python scripts/verify_results_cloud_contract.py`

Expected: PASS.

- [ ] **Step 5: Commit**

Commit message: `fix: exclude exam prep from Time+ credits`

---

### Task 8: Add the Ori-only dashboard card and progress display

**Files:**
- Modify: `src/ui/screen_manager.cpp`
- Modify: `preview/index.html`
- Modify: `src/ui/ui_validation.cpp` if card-count validation exists there

**Interfaces:**
Use explicit subject-ID lists per profile instead of assuming `0..subject_count-1`:
- Ayala: `[0,1,2,3]`
- Ethan: `[0,1,2,3,4]`
- Ori: `[0,1,2,3,5]`

New Ori card:
- Title: `הכנה למבחן`
- Icon: `📝`
- Subtitle/progress: `כפל שברים • רמה X/7 • Y/70`

Quiz header for exam prep should show current tier and solved count.

- [ ] **Step 1: Add/adjust UI validation for explicit subject IDs**

Verify Ori has five cards and the fifth navigates to subject ID 5, not subject 4.

- [ ] **Step 2: Implement card and progress labels**

Do not expose the card to Ethan/Ayala.

- [ ] **Step 3: Update browser preview**

Preview must include the new Ori card and a representative WORK-stage screen.

- [ ] **Step 4: Manual 480×320 layout check**

Check long mixed-number expressions, `X/10`, `Y/70`, Hebrew instruction and four answer buttons without clipping.

- [ ] **Step 5: Commit**

Commit message: `feat: show Ori exam prep progress card`

---

### Task 9: Content QA, build QA and OTA-safe release gate

**Files:**
- Modify: `.github/workflows/firmware-release.yml`
- Optional create: `scripts/verify_exam_prep_bank.py` if validation is cleaner outside the generator
- Update: `README.md` or `docs/` with exam-prep QA note

**Interfaces:**
CI must run the exam-prep bank validator before firmware build.

- [ ] **Step 1: Add CI bank validation**

Add before build:
`python scripts/generate_ori_exam_prep_questions.py --check`

- [ ] **Step 2: Run full local/CI-equivalent verification**

Run:
- `python scripts/generate_ori_exam_prep_questions.py --check`
- `pio test -e native`
- `python scripts/verify_results_cloud_contract.py`
- `pio run -e spotpear-esp32s3-max35`

Expected: all pass and `.pio/build/spotpear-esp32s3-max35/firmware.bin` is non-empty.

- [ ] **Step 3: Manual pedagogical QA of all 70 questions**

For each tier confirm:
- 10 questions exactly;
- no copied workbook wording;
- no ambiguous distractor;
- work step is genuinely intermediate;
- answer key is exact;
- level increase from previous block is obvious.

Sample at least 3 questions per tier on the physical/preview layout.

- [ ] **Step 4: Time+ isolation smoke test**

On a test device/profile:
1. record Time+-eligible daily count and pending sync state;
2. solve 3 exam-prep questions correctly;
3. verify exam-prep progress increases by 3;
4. verify Time+-eligible daily count is unchanged;
5. verify no new exam-prep result-sync payload/credit appears.

- [ ] **Step 5: Persistence smoke test**

Solve part of a tier, reboot, reopen exam prep and verify current tier/progress persists exactly.

- [ ] **Step 6: Final review before merge**

Do not merge if any of these fail: 70/70 count, 10-per-tier distribution, work-stage gate, Ori-only visibility, Time+ isolation, persistence, 480×320 rendering.

- [ ] **Step 7: Commit**

Commit message: `test: gate exam prep content and release QA`

---

## בדיקת-עצמית

- כיסוי מפרט: 70 שאלות, 7×10, רמת חוברת בהתחלה ועלייה משמעותית, ילד מחונן, דרך חובה, Ori-only, אפס דקות Time+, התקדמות מתמשכת ו־QA — כולם משויכים למשימות.
- סיכון מרכזי שטופל: subject 5 אינו יכול להיכנס בטעות ל־academic stats/Time+, ואינו נחתך בגלל מגבלת 10 שאלות.
- תאימות: `AHAVA_ACADEMIC_SUBJECT_COUNT` נשאר 4, כך שאין שינוי בסכמת תוצאות הענן.
- UI: תוכנן פתרון מפורש לפער subject ID 4/5 ול־RTL של ביטויי שברים.
- היקף: אין שינוי באפליקציית Time+ עצמה; החסימה נעשית במקור באקדמיה/מכשיר לפני sync.
