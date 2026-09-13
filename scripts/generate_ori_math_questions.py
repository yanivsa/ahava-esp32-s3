Import("env")

import json
import random
import re
from collections import Counter
from pathlib import Path

project_dir = Path(env["PROJECT_DIR"])
source_dir = project_dir / "data" / "questions" / "ori_math"
output_path = project_dir / "src" / "engine" / "generated_ori_math.inc"

EXPECTED_FILES = {
    "01_linear_equations_systems.json": ("משוואות ומערכות משוואות", 200, 6000, 6199),
    "02_functions_graphs.json": ("פונקציות וגרפים", 150, 6200, 6349),
    "03_algebraic_expressions_powers.json": ("ביטויים אלגבריים וחזקות", 150, 6350, 6499),
    "04_probability_statistics.json": ("הסתברות וסטטיסטיקה", 100, 6500, 6599),
    "05_geometry_pythagoras_angles.json": ("גאומטריה ופיתגורס", 200, 6600, 6799),
    "06_word_problems_percentages_ratios.json": ("בעיות מילוליות ואחוזים", 200, 6800, 6999),
}


def cpp_string(value):
    text = str(value)
    text = text.replace("\\", "\\\\").replace('"', '\\"')
    text = text.replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t")
    return f'"{text}"'


def normalize(text):
    text = str(text).strip().lower()
    text = text.replace("״", '"').replace("׳", "'")
    return re.sub(r"\s+", " ", text)


# Keep answer positions balanced across the 1000 questions (250 each).
answer_slots = [0] * 250 + [1] * 250 + [2] * 250 + [3] * 250
random.Random(0x081081).shuffle(answer_slots)
answer_slot_by_id = {6000 + i: slot for i, slot in enumerate(answer_slots)}

questions = []
source_files = sorted(source_dir.glob("*.json"))
actual_names = {p.name for p in source_files}
if actual_names != set(EXPECTED_FILES):
    missing = sorted(set(EXPECTED_FILES) - actual_names)
    extra = sorted(actual_names - set(EXPECTED_FILES))
    raise RuntimeError(f"Unexpected Ori math source files. missing={missing}, extra={extra}")

seen_ids = set()
seen_texts = set()
rows = []
generated_answer_positions = []

for source_path in source_files:
    topic, expected_count, first_id, last_id = EXPECTED_FILES[source_path.name]
    chunk = json.loads(source_path.read_text(encoding="utf-8"))
    if not isinstance(chunk, list):
        raise RuntimeError(f"{source_path.name} must contain a JSON array")
    if len(chunk) != expected_count:
        raise RuntimeError(
            f"{source_path.name} expected {expected_count} questions, got {len(chunk)}"
        )

    expected_ids = list(range(first_id, last_id + 1))
    actual_ids = [q.get("id") for q in chunk]
    if actual_ids != expected_ids:
        raise RuntimeError(
            f"{source_path.name} IDs must be contiguous {first_id}-{last_id}; got {actual_ids}"
        )

    for q in chunk:
        qid = q.get("id")
        options = q.get("options")
        answer = q.get("answer")
        if not isinstance(qid, int) or qid in seen_ids:
            raise RuntimeError(f"Invalid or duplicate question id: {qid}")
        seen_ids.add(qid)

        if not isinstance(options, list) or len(options) != 4 or len(set(map(str, options))) != 4:
            raise RuntimeError(f"Question {qid} must have four distinct options")
        if not isinstance(answer, int) or not 0 <= answer <= 3:
            raise RuntimeError(f"Question {qid} has invalid answer index")

        for field in ("text", "hint", "explanation"):
            if not str(q.get(field, "")).strip():
                raise RuntimeError(f"Question {qid} missing {field}")

        text_norm = normalize(q["text"])
        seen_texts.add(text_norm)

        if len(str(q["text"])) > 140:
            raise RuntimeError(f"Question {qid} text is too long for the handheld ({len(str(q['text']))})")
        if max(len(str(option)) for option in options) > 50:
            raise RuntimeError(f"Question {qid} has an option too long for the handheld")
        if len(str(q["hint"])) > 140:
            raise RuntimeError(f"Question {qid} hint is too long for the handheld")
        if len(str(q["explanation"])) > 180:
            raise RuntimeError(f"Question {qid} explanation is too long for the handheld")

        correct_option = options[answer]
        distractors = [option for idx, option in enumerate(options) if idx != answer]
        generated_answer = answer_slot_by_id[qid]
        generated_options = list(distractors)
        generated_options.insert(generated_answer, correct_option)
        generated_answer_positions.append(generated_answer)

        rows.append(
            "    {"
            + f"{qid}, 0, PROFILE_ORI, {cpp_string(q['text'])}, "
            + "{" + ", ".join(cpp_string(option) for option in generated_options) + "}, "
            + f"{generated_answer}, {cpp_string(q['explanation'])}, {cpp_string(q['hint'])}"
            + "}"
        )
        questions.append(q)

if len(questions) != 1000:
    raise RuntimeError(f"Expected exactly 1000 Ori math questions, got {len(questions)}")
if seen_ids != set(range(6000, 7000)):
    raise RuntimeError("Ori math IDs must cover exactly 6000-6999")

position_counts = Counter(generated_answer_positions)
if sorted(position_counts.values()) != [250, 250, 250, 250]:
    raise RuntimeError(f"Unbalanced generated answer positions: {dict(position_counts)}")

generated = (
    "// Generated from data/questions/ori_math/*.json. Do not edit manually.\n"
    "static const Question_t ORI_MATH_QUESTIONS[] = {\n"
    + ",\n".join(rows)
    + "\n};\n"
)
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text(generated, encoding="utf-8")
print(
    f"QA PASS: generated {len(rows)} Ori Grade 8 math questions from "
    f"{len(source_files)} topic files; answer positions={dict(sorted(position_counts.items()))} "
    f"-> {output_path}"
)
