Import("env")

import os
from pathlib import Path

project_dir = Path(env.subst("$PROJECT_DIR"))
header = project_dir / "include" / "results_sync_secret.h"
token = os.environ.get("AHAVA_RESULTS_SYNC_TOKEN", "")

escaped = token.replace("\", "\\").replace('"', '\"')
header.write_text(
    "#pragma once\n"
    f'#define AHAVA_RESULTS_SYNC_TOKEN "{escaped}"\n',
    encoding="utf-8",
)

if token:
    print("[RESULT_SYNC] Device result sync token injected for this build.")
else:
    print("[RESULT_SYNC] No device result sync token; cloud sync will remain disabled.")
