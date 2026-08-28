Import("env")

import json
from pathlib import Path

project_dir = Path(env["PROJECT_DIR"])
source_dir = project_dir / "data" / "questions" / "ori_religion"
output_path = project_dir / "src" / "engine" / "generated_ori_religion.inc"

def cpp_string(value):
    text = str(value)
    text = text.replace("\\", "\\\\").replace('"', '\\"')
    text = text.replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t")
    return f'"{text}"'

questions = []
source_files = sorted(source_dir.glob("*.json"))
if not source_files:
    raise RuntimeError(f"No Ori religion source files found in {source_dir}")

for source_path in source_files:
    chunk = json.loads(source_path.read_text(encoding="utf-8"))
    if not isinstance(chunk, list):
        raise RuntimeError(f"{source_path.name} must contain a JSON array")
    questions.extend(chunk)

if len(questions) != 150:
    raise RuntimeError(f"Expected exactly 150 Ori religion questions, got {len(questions)}")

seen_ids = set()
rows = []
for q in questions:
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
    if len(str(q["text"])) > 120:
        raise RuntimeError(f"Question {qid} text is too long for the handheld")
    if max(len(str(option)) for option in options) > 45:
        raise RuntimeError(f"Question {qid} has an option too long for the handheld")

    rows.append(
        "    {"
        + f"{qid}, 3, PROFILE_ORI, {cpp_string(q['text'])}, "
        + "{" + ", ".join(cpp_string(option) for option in options) + "}, "
        + f"{answer}, {cpp_string(q['explanation'])}, {cpp_string(q['hint'])}"
        + "}"
    )

generated = (
    "// Generated from data/questions/ori_religion/*.json. Do not edit manually.\n"
    "static const Question_t ORI_RELIGION_QUESTIONS[] = {\n"
    + ",\n".join(rows)
    + "\n};\n"
)
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text(generated, encoding="utf-8")
print(f"Generated {len(rows)} Ori religion questions from {len(source_files)} files -> {output_path}")
