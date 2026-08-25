"""Exclude ARM-only LVGL Helium assembly when building for Xtensa ESP32-S3."""

Import("env")
import json
import os


def skip_arm_helium(node):
    path = node.get_abspath().replace("\\", "/")
    if "/lvgl/src/draw/sw/blend/helium/" in path:
        return None
    return node


env.AddBuildMiddleware(skip_arm_helium, "*lv_blend_helium.S")

# PlatformIO's library builder clones the environment before general middleware
# is applied. Add an equivalent source filter to LVGL's generated manifest so a
# clean dependency install is also safe on Xtensa.
manifest = os.path.join(
    env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"), "lvgl", "library.json"
)
if os.path.exists(manifest):
    with open(manifest, "r", encoding="utf-8") as stream:
        metadata = json.load(stream)
    build = metadata.setdefault("build", {})
    filters = build.setdefault("srcFilter", ["+<*>"])
    exclusions = [
        "-<draw/sw/blend/helium/*.S>",
        "-<draw/sw/blend/neon/*.S>",
    ]
    changed = False
    for exclusion in exclusions:
        if exclusion not in filters:
            filters.append(exclusion)
            changed = True
    if changed:
        with open(manifest, "w", encoding="utf-8") as stream:
            json.dump(metadata, stream, indent=2)
