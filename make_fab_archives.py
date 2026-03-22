#!/usr/bin/env python3
"""
Creates Fab-publishable zip archives of the Tentacle plugin, one per engine version.

Each archive:
  - Sets SupportedTargetPlatforms on the uplugin
  - Sets PlatformAllowList on every module in the uplugin
  - Sets EngineVersion to the target engine version
  - Excludes files not required by the engine (VCS files, extra logos, docs, etc.)
"""

import copy
import json
import logging
import os
import pathlib
import zipfile

logging.basicConfig(level=logging.DEBUG, format="%(levelname)s  %(message)s")
log = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

PLUGIN_ROOT = pathlib.Path(__file__).parent.resolve()
UPLUGIN_FILE = PLUGIN_ROOT / "Tentacle.uplugin"
OUTPUT_DIR = PLUGIN_ROOT  # zips are written next to this script

ENGINE_VERSIONS = ["5.5", "5.6", "5.7"]

SUPPORTED_PLATFORMS = ["Win64", "Linux"]

# Top-level directory names (relative to PLUGIN_ROOT) that are pruned entirely
# during traversal - never entered, never listed.
EXCLUDE_DIRS = {
    ".git",
    "Binaries",
    "Intermediate",
}

# Individual file paths (relative to PLUGIN_ROOT, forward slashes) to exclude.
EXCLUDE_FILES = {
    ".gitignore",
    ".p4ignore",
    "Resources/logo.png",
    "Resources/logo.svg",
    "Resources/logo512.png",
    # This script itself
    "make_fab_archives.py",
}


def collect_files() -> list[pathlib.Path]:
    """Walk the plugin tree, pruning excluded directories, and return all includable files."""
    result: list[pathlib.Path] = []
    for dirpath, dirnames, filenames in os.walk(PLUGIN_ROOT):
        current = pathlib.Path(dirpath)
        rel_dir = current.relative_to(PLUGIN_ROOT).as_posix()

        # Prune excluded directories in-place so os.walk won't descend into them.
        # For the root level we match by name; for deeper levels we match the top segment.
        dirnames[:] = [
            d for d in dirnames
            if not (rel_dir == "." and d in EXCLUDE_DIRS)
            and not (rel_dir != "." and rel_dir.split("/")[0] in EXCLUDE_DIRS)
        ]

        for filename in filenames:
            abs_path = current / filename
            rel_posix = abs_path.relative_to(PLUGIN_ROOT).as_posix()

            # Skip Tentacle_Fab_*.zip archives (from previous runs)
            if filename.endswith(".zip") and filename.startswith("Tentacle_Fab_"):
                log.debug("SKIP (fab zip)    %s", rel_posix)
                continue

            if rel_posix in EXCLUDE_FILES:
                log.debug("SKIP (excluded)   %s", rel_posix)
                continue

            log.debug("INCLUDE           %s", rel_posix)
            result.append(abs_path)

    return result


def build_uplugin(engine_version: str) -> str:
    """Return a modified uplugin JSON string for the given engine version."""
    data = json.loads(UPLUGIN_FILE.read_text(encoding="utf-8"))

    data["EngineVersion"] = engine_version
    del data["EnabledByDefault"]
    data["SupportedTargetPlatforms"] = copy.deepcopy(SUPPORTED_PLATFORMS)

    for module in data.get("Modules", []):
        module["PlatformAllowList"] = copy.deepcopy(SUPPORTED_PLATFORMS)

    return json.dumps(data, indent="\t")


def make_archive(engine_version: str, files: list[pathlib.Path]) -> pathlib.Path:
    tag = engine_version.replace(".", "_")
    zip_path = OUTPUT_DIR / f"Tentacle_Fab_{tag}.zip"

    log.info("Building %s (%d files)...", zip_path.name, len(files))

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for abs_path in files:
            rel_posix = abs_path.relative_to(PLUGIN_ROOT).as_posix()

            if abs_path == UPLUGIN_FILE:
                log.debug("  uplugin (patched) -> %s", rel_posix)
                zf.writestr(rel_posix, build_uplugin(engine_version))
            else:
                log.debug("  adding            -> %s", rel_posix)
                zf.write(abs_path, rel_posix)

    log.info("Done: %s  (%.1f KB)", zip_path.name, zip_path.stat().st_size / 1024)
    return zip_path


def main():
    log.info("Plugin root: %s", PLUGIN_ROOT)

    # Collect files once; all archives share the same file list
    files = sorted(collect_files())
    log.info("Collected %d files to archive", len(files))

    for version in ENGINE_VERSIONS:
        make_archive(version, files)


if __name__ == "__main__":
    main()
