#!/usr/bin/env python3

import argparse
import json
import subprocess
from pathlib import Path


def git_object(revision):
    return subprocess.check_output(
        ["git", "rev-parse", revision], text=True
    ).strip()


def named_values(values):
    result = {}
    for value in values:
        name, separator, detail = value.partition("=")
        if not separator or not name or not detail or name in result:
            raise ValueError(f"Invalid or duplicate manifest value: {value}")
        result[name] = detail
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--sha", required=True)
    parser.add_argument("--platform", choices=("linux-x64", "windows-x64"), required=True)
    parser.add_argument("--toolchain", action="append", required=True)
    parser.add_argument("--dependency", action="append", default=[])
    parser.add_argument("--output", type=Path, default=Path("BUILD-MANIFEST.json"))
    args = parser.parse_args()

    if args.sha != git_object("HEAD"):
        parser.error("Build SHA does not match the checked-out commit")

    try:
        toolchain = named_values(args.toolchain)
        dependencies = named_values(args.dependency)
    except ValueError as error:
        parser.error(str(error))

    if not {"compiler", "build_tool", "gettext"} <= toolchain.keys():
        parser.error("Compiler, build tool, and gettext versions are required")

    manifest = {
        "schema_version": 1,
        "product": "Cataclysm: RODINA",
        "rodina_version": args.version,
        "git_sha": args.sha,
        "cdda_base": "0.I-1",
        "platform": args.platform,
        "bundled_resources": {
            "data_tree": git_object("HEAD:data"),
            "gfx_tree": git_object("HEAD:gfx"),
            "sound_tree": git_object("HEAD:data/sound/Basic"),
            "russian_po_blob": git_object("HEAD:lang/po/ru.po"),
        },
        "external_game_resources": [],
        "toolchain": toolchain,
        "build_dependencies": dependencies,
    }
    args.output.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
