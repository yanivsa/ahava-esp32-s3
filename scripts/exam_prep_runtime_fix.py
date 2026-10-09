#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

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

# Never manufacture nonsense distractors such as "3/4 (1)". A bank build must
# fail instead if a question generator did not supply three genuinely distinct
# alternatives.
patch(
    "scripts/generate_ori_exam_prep_questions.py",
    '''    k = 1\n    while len(values) < 4:\n        candidate = f"{correct} ({k})"\n        if candidate not in values:\n            values.append(candidate)\n        k += 1\n''',
    '''    if len(values) != 4:\n        raise RuntimeError(f"Need three distinct plausible distractors for {correct!r}; got {values!r}")\n''',
)

# Avoid a duplicate first-level intermediate distractor (e.g. 3×1/4 used to
# generate both 4/4 and another equivalent text fallback).
patch(
    "scripts/generate_ori_exam_prep_questions.py",
    '''            w1_wrong = [f"{whole + num}/{den}", f"{whole * num}/{den + 1}", f"{whole * num + 1}/{den}"]\n''',
    '''            w1_wrong = [f"{whole + num}/{den}", f"{whole * num}/{den + 1}", f"{whole * num + den}/{den}"]\n''',
)

# The hint must match the actual path: when cancellation is impossible, tell
# Ori to multiply the numerator and keep the denominator rather than hunting
# for a nonexistent common factor.
patch(
    "scripts/generate_ori_exam_prep_questions.py",
    '''            "hint": "חפש גורם משותף בין המספר השלם למכנה לפני שמכפילים.",\n''',
    '''            "hint": ("חפש גורם משותף בין המספר השלם למכנה לפני שמכפילים." if g > 1\n                     else "אין צמצום מקדים: כפל את המספר השלם במונה והשאר את המכנה."),\n''',
)

# Regenerate both the reviewed JSON source and embedded C++ include, then let
# the workflow's --check validate exact counts, scope and rational answers.
subprocess.run([
    sys.executable,
    str(root / "scripts/generate_ori_exam_prep_questions.py"),
    "--write-source",
], check=True)

Path(__file__).unlink()
print("Runtime coverage and exam-prep distractor quality fixed.")
