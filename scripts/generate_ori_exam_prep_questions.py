#!/usr/bin/env python3
"""Generate and validate Ori's 70-question fraction-multiplication exam-prep bank."""

import argparse
import json
import math
import random
import re
from fractions import Fraction
from pathlib import Path

try:
    Import("env")  # type: ignore[name-defined]  # PlatformIO/SCons
except NameError:
    env = None

ROOT = Path(env["PROJECT_DIR"]) if env is not None else Path(__file__).resolve().parents[1]
SOURCE = ROOT / "data" / "questions" / "ori_exam_prep" / "fractions_multiplication.json"
OUTPUT = ROOT / "src" / "engine" / "generated_ori_exam_prep.inc"

# Exactly five fraction×whole and five mixed×whole cases per tier.
CASES = {
    1: {
        "fraction": [(3,1,4),(5,2,7),(6,3,8),(4,5,9),(7,2,5)],
        "mixed": [(2,1,1,2),(3,2,1,3),(4,1,3,4),(5,2,2,5),(6,1,5,6)],
    },
    2: {
        "fraction": [(12,5,8),(15,7,10),(18,5,12),(14,9,11),(20,7,12)],
        "mixed": [(8,3,3,4),(12,2,5,6),(15,4,7,10),(18,3,5,12),(20,5,7,8)],
    },
    3: {
        "fraction": [(24,7,12),(28,9,14),(30,11,15),(27,10,12),(25,14,15)],
        "mixed": [(24,4,5,12),(28,6,9,14),(30,5,11,15),(27,7,8,9),(25,8,14,15)],
    },
    4: {
        "fraction": [(36,11,18),(42,13,14),(45,17,20),(48,7,16),(50,19,20)],
        "mixed": [(36,8,7,18),(42,9,11,14),(45,7,13,20),(48,10,5,16),(50,11,17,20)],
    },
    5: {
        "fraction": [(60,17,24),(72,19,30),(75,23,25),(64,21,28),(80,29,30)],
        "mixed": [(48,12,7,18),(72,17,11,24),(75,14,13,25),(64,15,19,28),(80,18,23,30)],
    },
    6: {
        "fraction": [(84,31,42),(96,35,48),(108,37,45),(112,41,48),(120,43,45)],
        "mixed": [(84,21,17,42),(96,24,23,48),(108,27,31,45),(112,29,35,48),(120,32,37,45)],
    },
    7: {
        "fraction": [(126,47,63),(144,55,72),(168,59,64),(180,61,72),(192,65,72)],
        "mixed": [(126,36,31,63),(144,42,35,72),(168,48,47,64),(180,52,49,72),(192,57,53,72)],
    },
}

DISALLOWED = re.compile(r"שטח|היקף|מלבן|משווא|נעלם|אחוז|יחס|מהירות|מרחק|משקל|קבוצה|פרמטר|מקסימום|מינימום")


def fmt(value: Fraction) -> str:
    value = Fraction(value)
    if value.denominator == 1:
        return str(value.numerator)
    if abs(value.numerator) > value.denominator:
        sign = "-" if value < 0 else ""
        n = abs(value.numerator)
        whole, rem = divmod(n, value.denominator)
        return f"{sign}{whole} {rem}/{value.denominator}"
    return f"{value.numerator}/{value.denominator}"


def cpp_string(value: str | None) -> str:
    if value is None:
        return "nullptr"
    text = str(value).replace("\\", "\\\\").replace('"', '\\"')
    text = text.replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t")
    return f'"{text}"'


def permute(correct: str, wrongs: list[str], slot: int) -> tuple[list[str], int]:
    values = [correct]
    for item in wrongs:
        if item not in values:
            values.append(item)
        if len(values) == 4:
            break
    k = 1
    while len(values) < 4:
        candidate = f"{correct} ({k})"
        if candidate not in values:
            values.append(candidate)
        k += 1
    answer = values.pop(0)
    values.insert(slot, answer)
    return values, slot


def final_options(result: Fraction, seed_den: int, qid: int) -> tuple[list[str], int]:
    step = Fraction(1, max(2, seed_den))
    candidates = [result + step, result + 1, result + Fraction(2, max(3, seed_den))]
    if result > step:
        candidates[0] = result - step
    correct = fmt(result)
    wrongs = []
    for value in candidates:
        text = fmt(value)
        if text != correct and text not in wrongs:
            wrongs.append(text)
    slot = (qid * 3 + 1) % 4
    return permute(correct, wrongs, slot)


def raw_work_options(correct: str, wrongs: list[str], qid: int, salt: int) -> tuple[list[str], int]:
    slot = (qid * (salt + 2) + salt) % 4
    return permute(correct, wrongs, slot)


def build_question(qid: int, tier: int, kind: str, values: tuple[int, ...]) -> dict:
    second = tier >= 4
    if kind == "fraction":
        whole, num, den = values
        result = Fraction(whole * num, den)
        prompt = f"חשב: {whole} × {num}/{den}"
        g = math.gcd(whole, den)
        if g > 1:
            cw, cd = whole // g, den // g
            w1 = f"{cw} × {num}/{cd}"
            w1_wrong = [f"{whole} × {num}/{cd}", f"{cw} × {num}/{den}", f"{g} × {num}/{cd}"]
            work_prompt_1 = "מהו הצמצום הנכון לפני הכפל?"
        else:
            cw, cd = whole, den
            w1 = f"{whole * num}/{den}"
            w1_wrong = [f"{whole + num}/{den}", f"{whole * num}/{den + 1}", f"{whole * num + 1}/{den}"]
            work_prompt_1 = "מהו שבר הביניים הנכון אחרי הכפל?"
        w1_options, w1_idx = raw_work_options(w1, w1_wrong, qid, 1)
        work_prompt_2 = "אחרי הצמצום, מהו שבר המכפלה לפני המרה למספר מעורב?" if second else None
        raw2 = f"{cw * num}/{cd}"
        w2_options, w2_idx = raw_work_options(
            raw2,
            [f"{cw * num + 1}/{cd}", f"{cw + num}/{cd}", f"{cw * num}/{cd + 1}"],
            qid, 2,
        ) if second else (["", "", "", ""], 0)
        answers, correct_idx = final_options(result, den, qid)
        return {
            "id": qid, "tier": tier, "operation": "fraction_times_whole",
            "whole": whole, "numerator": num, "denominator": den,
            "prompt": prompt, "workPrompt1": work_prompt_1,
            "workOptions1": w1_options, "workCorrect1": w1_idx,
            "workPrompt2": work_prompt_2, "workOptions2": w2_options, "workCorrect2": w2_idx,
            "options": answers, "correct": correct_idx,
            "hint": "חפש גורם משותף בין המספר השלם למכנה לפני שמכפילים.",
            "feedback": f"דרך יעילה: צמצום אם אפשר, כפל, צמצום סופי והמרה. התוצאה: {fmt(result)}.",
            "exact": f"{result.numerator}/{result.denominator}", "two_step_work": second,
        }

    whole, mixed_whole, num, den = values
    improper = mixed_whole * den + num
    result = Fraction(whole * improper, den)
    prompt = f"חשב: {whole} × {mixed_whole} {num}/{den}"
    work_prompt_1 = "מהי ההמרה הנכונה של המספר המעורב לשבר מדומה?"
    w1 = f"{whole} × {improper}/{den}"
    w1_options, w1_idx = raw_work_options(
        w1,
        [f"{whole} × {mixed_whole + num}/{den}", f"{whole} × {mixed_whole * den - num}/{den}", f"{whole} × {improper}/{den + 1}"],
        qid, 1,
    )
    g = math.gcd(whole, den)
    cw, cd = whole // g, den // g
    work_prompt_2 = "לאחר ההמרה, איזה צמצום נכון לבצע לפני הכפל?" if second else None
    w2 = f"{cw} × {improper}/{cd}"
    w2_options, w2_idx = raw_work_options(
        w2,
        [f"{whole} × {improper}/{cd}", f"{cw} × {improper}/{den}", f"{max(1, cw-1)} × {improper}/{cd}"],
        qid, 2,
    ) if second else (["", "", "", ""], 0)
    answers, correct_idx = final_options(result, den, qid)
    return {
        "id": qid, "tier": tier, "operation": "mixed_times_whole",
        "whole": whole, "mixedWhole": mixed_whole, "numerator": num, "denominator": den,
        "prompt": prompt, "workPrompt1": work_prompt_1,
        "workOptions1": w1_options, "workCorrect1": w1_idx,
        "workPrompt2": work_prompt_2, "workOptions2": w2_options, "workCorrect2": w2_idx,
        "options": answers, "correct": correct_idx,
        "hint": "המר תחילה את המספר המעורב לשבר מדומה; אחר כך חפש צמצום עם המכנה.",
        "feedback": f"המרה: {mixed_whole} {num}/{den} = {improper}/{den}. לאחר צמצום וכפל התוצאה היא {fmt(result)}.",
        "exact": f"{result.numerator}/{result.denominator}", "two_step_work": second,
    }


def build_source() -> list[dict]:
    questions = []
    qid = 60000
    for tier in range(1, 8):
        for values in CASES[tier]["fraction"]:
            questions.append(build_question(qid, tier, "fraction", values)); qid += 1
        for values in CASES[tier]["mixed"]:
            questions.append(build_question(qid, tier, "mixed", values)); qid += 1
    return questions


def parse_answer(text: str) -> Fraction:
    text = text.strip()
    if " " in text:
        whole, frac = text.split(" ", 1)
        n, d = frac.split("/")
        return Fraction(int(whole) * int(d) + int(n), int(d))
    if "/" in text:
        n, d = text.split("/")
        return Fraction(int(n), int(d))
    return Fraction(int(text), 1)


def validate(questions: list[dict]) -> None:
    if len(questions) != 70:
        raise RuntimeError(f"Expected exactly 70 questions, got {len(questions)}")
    ids = [q.get("id") for q in questions]
    if ids != list(range(60000, 60070)):
        raise RuntimeError("Question IDs must be exactly 60000..60069 in order")
    for tier in range(1, 8):
        chunk = [q for q in questions if q.get("tier") == tier]
        if len(chunk) != 10:
            raise RuntimeError(f"Tier {tier} must contain 10 questions")
        counts = {op: sum(q.get("operation") == op for q in chunk) for op in ("fraction_times_whole", "mixed_times_whole")}
        if counts != {"fraction_times_whole": 5, "mixed_times_whole": 5}:
            raise RuntimeError(f"Tier {tier} operation balance invalid: {counts}")

    for q in questions:
        op = q.get("operation")
        if op not in ("fraction_times_whole", "mixed_times_whole"):
            raise RuntimeError(f"Question {q.get('id')} has disallowed operation {op}")
        if DISALLOWED.search(q.get("prompt", "")):
            raise RuntimeError(f"Question {q['id']} leaks outside requested topic")
        tier = int(q["tier"])
        if not 1 <= tier <= 7:
            raise RuntimeError(f"Question {q['id']} invalid tier")
        den = int(q["denominator"]); num = int(q["numerator"]); whole = int(q["whole"])
        if den <= 1 or not 0 < num < den or whole <= 0:
            raise RuntimeError(f"Question {q['id']} invalid operands")
        if op == "fraction_times_whole":
            expected = Fraction(whole * num, den)
        else:
            mixed_whole = int(q["mixedWhole"])
            expected = Fraction(whole * (mixed_whole * den + num), den)
        exact_n, exact_d = map(int, q["exact"].split("/"))
        if Fraction(exact_n, exact_d) != expected:
            raise RuntimeError(f"Question {q['id']} exact metadata mismatch")
        options = q.get("options")
        if not isinstance(options, list) or len(options) != 4 or len(set(options)) != 4:
            raise RuntimeError(f"Question {q['id']} final options must be four distinct values")
        normalized = [parse_answer(x) for x in options]
        if len(set(normalized)) != 4:
            raise RuntimeError(f"Question {q['id']} has mathematically equivalent final options")
        ci = int(q.get("correct", -1))
        if not 0 <= ci <= 3 or normalized[ci] != expected:
            raise RuntimeError(f"Question {q['id']} correct final answer mismatch")
        w1 = q.get("workOptions1")
        if not q.get("workPrompt1") or not isinstance(w1, list) or len(w1) != 4 or len(set(w1)) != 4:
            raise RuntimeError(f"Question {q['id']} invalid first work checkpoint")
        if not 0 <= int(q.get("workCorrect1", -1)) <= 3:
            raise RuntimeError(f"Question {q['id']} invalid first work answer index")
        if tier >= 4:
            w2 = q.get("workOptions2")
            if not q.get("workPrompt2") or not isinstance(w2, list) or len(w2) != 4 or len(set(w2)) != 4:
                raise RuntimeError(f"Question {q['id']} needs a distinct second checkpoint")
            if not 0 <= int(q.get("workCorrect2", -1)) <= 3:
                raise RuntimeError(f"Question {q['id']} invalid second work answer index")
        if not q.get("hint") or not q.get("feedback"):
            raise RuntimeError(f"Question {q['id']} missing hint/feedback")


def generate_include(questions: list[dict]) -> str:
    rows = []
    for q in questions:
        op_id = 1 if q["operation"] == "fraction_times_whole" else 2
        answers = ", ".join(cpp_string(x) for x in q["options"])
        w1 = ", ".join(cpp_string(x) for x in q["workOptions1"])
        w2 = ", ".join(cpp_string(x) for x in q["workOptions2"])
        rows.append(
            "    {" + ", ".join([
                str(q["id"]), "AHAVA_SUBJECT_EXAM_PREP", "PROFILE_ORI", cpp_string(q["prompt"]),
                "{" + answers + "}", str(q["correct"]), cpp_string(q["feedback"]), cpp_string(q["hint"]), "-1",
                str(q["tier"]), str(op_id), cpp_string(q["workPrompt1"]), "{" + w1 + "}", str(q["workCorrect1"]),
                cpp_string(q["workPrompt2"]), "{" + w2 + "}", str(q["workCorrect2"])
            ]) + "}"
        )
    return "// Generated. Do not edit by hand.\nstatic const Question_t ORI_EXAM_PREP_QUESTIONS[] = {\n" + ",\n".join(rows) + "\n};\n"


def run(write_source: bool, check_only: bool) -> None:
    if write_source:
        SOURCE.parent.mkdir(parents=True, exist_ok=True)
        SOURCE.write_text(json.dumps(build_source(), ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    if not SOURCE.exists():
        raise RuntimeError(f"Missing source bank: {SOURCE}. Run with --write-source once.")
    questions = json.loads(SOURCE.read_text(encoding="utf-8"))
    validate(questions)
    generated = generate_include(questions)
    if check_only:
        if OUTPUT.exists() and OUTPUT.read_text(encoding="utf-8") != generated:
            raise RuntimeError("Generated include is stale; regenerate it")
        print("70 valid; 10/tier; 5+5 operations/tier")
        return
    OUTPUT.write_text(generated, encoding="utf-8")
    print("70 valid; 10/tier; 5+5 operations/tier")


if env is not None:
    # PlatformIO pre-script: source must already be committed; regenerate deterministically.
    run(False, False)
else:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write-source", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    run(args.write_source, args.check)
