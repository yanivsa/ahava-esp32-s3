Import("env")

import json
import re
from pathlib import Path

project_dir = Path(env["PROJECT_DIR"])
source_dir = project_dir / "data" / "questions" / "ori_religion"
output_path = project_dir / "src" / "engine" / "generated_ori_religion.inc"

EXPECTED_FILES = {
    "01_selichot.json": ("סליחות", 12, 5000, 5011),
    "02_hatarat_nedarim.json": ("התרת נדרים", 12, 5012, 5023),
    "03_shofar.json": ("תקיעות שופר", 12, 5024, 5035),
    "04_tashlich.json": ("תשליך", 10, 5036, 5045),
    "05_shabbat_shuva.json": ("שבת תשובה", 10, 5046, 5055),
    "06_kapparot.json": ("כפרות", 10, 5056, 5065),
    "07_vidui.json": ("וידוי", 12, 5066, 5077),
    "08_neilah.json": ("תפילת נעילה", 12, 5078, 5089),
    "09_simchat_beit_hashoeva.json": ("שמחת בית השואבה", 10, 5090, 5099),
    "10_hoshana_rabba.json": ("הושענא רבה", 10, 5100, 5109),
    "11_birkat_ha-ilanot.json": ("ברכת האילנות", 10, 5110, 5119),
    "12_sefirat_haomer.json": ("ספירת העומר", 15, 5120, 5134),
    "13_bein_hametzarim.json": ("בין המצרים", 15, 5135, 5149),
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

questions = []
source_files = sorted(source_dir.glob("*.json"))
actual_names = {p.name for p in source_files}
if actual_names != set(EXPECTED_FILES):
    missing = sorted(set(EXPECTED_FILES) - actual_names)
    extra = sorted(actual_names - set(EXPECTED_FILES))
    raise RuntimeError(f"Unexpected Ori religion source files. missing={missing}, extra={extra}")

seen_ids = set()
seen_texts = set()
rows = []

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

        if q.get("topic") != topic:
            raise RuntimeError(f"Question {qid} must use topic {topic!r}")
        if not isinstance(options, list) or len(options) != 4 or len(set(map(str, options))) != 4:
            raise RuntimeError(f"Question {qid} must have four distinct options")
        if not isinstance(answer, int) or not 0 <= answer <= 3:
            raise RuntimeError(f"Question {qid} has invalid answer index")

        for field in ("text", "hint", "explanation"):
            if not str(q.get(field, "")).strip():
                raise RuntimeError(f"Question {qid} missing {field}")

        text_norm = normalize(q["text"])
        if text_norm in seen_texts:
            raise RuntimeError(f"Duplicate question text at id {qid}")
        seen_texts.add(text_norm)

        if len(str(q["text"])) > 120:
            raise RuntimeError(f"Question {qid} text is too long for the handheld")
        if max(len(str(option)) for option in options) > 45:
            raise RuntimeError(f"Question {qid} has an option too long for the handheld")
        if len(str(q["hint"])) > 120:
            raise RuntimeError(f"Question {qid} hint is too long for the handheld")
        if len(str(q["explanation"])) > 160:
            raise RuntimeError(f"Question {qid} explanation is too long for the handheld")

        correct_norm = normalize(options[answer])
        hint_norm = normalize(q["hint"])
        if len(correct_norm) >= 5 and correct_norm in hint_norm:
            raise RuntimeError(
                f"Question {qid} hint contains the correct option verbatim: {options[answer]!r}"
            )
        if normalize(q["hint"]) == normalize(q["explanation"]):
            raise RuntimeError(f"Question {qid} hint must differ from final explanation")

        rows.append(
            "    {"
            + f"{qid}, 3, PROFILE_ORI, {cpp_string(q['text'])}, "
            + "{" + ", ".join(cpp_string(option) for option in options) + "}, "
            + f"{answer}, {cpp_string(q['explanation'])}, {cpp_string(q['hint'])}"
            + "}"
        )
        questions.append(q)

if len(questions) != 150:
    raise RuntimeError(f"Expected exactly 150 Ori religion questions, got {len(questions)}")
if seen_ids != set(range(5000, 5150)):
    raise RuntimeError("Ori religion IDs must cover exactly 5000-5149")

generated = (
    "// Generated from data/questions/ori_religion/*.json. Do not edit manually.\n"
    "static const Question_t ORI_RELIGION_QUESTIONS[] = {\n"
    + ",\n".join(rows)
    + "\n};\n"
)
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text(generated, encoding="utf-8")
print(
    f"QA PASS: generated {len(rows)} Ori religion questions from "
    f"{len(source_files)} topic files -> {output_path}"
)
