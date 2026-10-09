#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
path = root / "src" / "ui" / "screen_manager.cpp"
text = path.read_text(encoding="utf-8")
needle = "lv_obj_t *s_muse_wifi_lbl = NULL;\n\nstatic void ui_live_status_timer_cb"
replacement = "lv_obj_t *s_muse_wifi_lbl = NULL;\n\nstatic void format_exam_prep_progress(char *buf, size_t len);\n\nstatic void ui_live_status_timer_cb"
if text.count(needle) != 1:
    raise SystemExit(f"expected one forward-declaration insertion point, got {text.count(needle)}")
path.write_text(text.replace(needle, replacement, 1), encoding="utf-8")
# One-shot helper: the verified integration commit should not retain bootstrap tooling.
Path(__file__).unlink()
print("Added exam-prep formatter forward declaration.")
