#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]


def patch(rel, old, new):
    path = root / rel
    text = path.read_text(encoding="utf-8")
    if text.count(old) != 1:
        raise SystemExit(f"{rel}: expected one match, got {text.count(old)}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")

# Ethan has the four academic subjects plus Wizard Challenges. Exam Prep is
# intentionally Ori-only and is validated separately below this loop.
patch(
    "src/engine/quiz_engine.cpp",
    "const int subject_count = profile == PROFILE_ETHAN ? AHAVA_SUBJECT_COUNT : AHAVA_SUBJECT_CHALLENGES;",
    "const int subject_count = profile == PROFILE_ETHAN ? (AHAVA_SUBJECT_CHALLENGES + 1) : AHAVA_SUBJECT_CHALLENGES;",
)

# Add the bank validator to the permanent PR/firmware CI so scope/count drift
# cannot merge later without failing.
workflow = root / ".github" / "workflows" / "firmware-release.yml"
text = workflow.read_text(encoding="utf-8")
needle = "      - name: Run quiz policy tests\n        run: pio test -e native\n"
replacement = "      - name: Validate exam prep question bank\n        run: python scripts/generate_ori_exam_prep_questions.py --check\n\n" + needle
if text.count(needle) != 1:
    raise SystemExit(f"firmware workflow: expected one native-test insertion point, got {text.count(needle)}")
workflow.write_text(text.replace(needle, replacement, 1), encoding="utf-8")

Path(__file__).unlink()
print("Runtime coverage validation and permanent CI guard fixed.")
