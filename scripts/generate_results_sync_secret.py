Import("env")

import os
import re
from pathlib import Path

project_dir = Path(env.subst("$PROJECT_DIR"))
header = project_dir / "include" / "results_sync_secret.h"
token = os.environ.get("AHAVA_RESULTS_SYNC_TOKEN", "")

if token and not re.fullmatch(r"[A-Za-z0-9_-]{32,128}", token):
    raise RuntimeError("AHAVA_RESULTS_SYNC_TOKEN contains unsupported characters")

header.write_text(
    "#pragma once\n"
    '#define AHAVA_RESULTS_SYNC_TOKEN "' + token + '"\n',
    encoding="utf-8",
)

if token:
    print("[RESULT_SYNC] Device result sync token injected for this build.")
else:
    print("[RESULT_SYNC] No device result sync token; cloud sync will remain disabled.")
