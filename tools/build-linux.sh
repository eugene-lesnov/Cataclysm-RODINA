#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"

if [[ $(uname -s) != Linux ]]; then
    echo "This script builds the Linux version only." >&2
    exit 1
fi

for tool in make g++ pkg-config msgfmt python3 tar timeout; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Missing build tool: $tool" >&2
        exit 1
    fi
done

for package in sdl2 SDL2_image SDL2_ttf SDL2_mixer freetype2 zlib; do
    if ! pkg-config --exists "$package"; then
        echo "Missing development package for $package (pkg-config)." >&2
        echo "Install the SDL2, SDL2_image, SDL2_ttf, SDL2_mixer, FreeType and zlib development packages." >&2
        exit 1
    fi
done

jobs=${JOBS:-3}
if [[ ! $jobs =~ ^[1-9][0-9]*$ ]]; then
    echo "JOBS must be a positive integer." >&2
    exit 1
fi

build_flags=(TILES=1 SOUND=1 RELEASE=1 LOCALIZE=1 LANGUAGES=ru
    BACKTRACE=1 LIBBACKTRACE=0 PCH=0 ASTYLE=0 LINTJSON=0 TESTS=0
    USE_XDG_DIR=1)
make -j"$jobs" "${build_flags[@]}" all
make -C lang LANGUAGES=ru
make "${build_flags[@]}" bindist

binary="$repo_root/cataclysm-rodina-tiles"
catalog="$repo_root/lang/mo/ru/LC_MESSAGES/cataclysm-dda.mo"
archive="$repo_root/cataclysm-rodina-0.1-dev.tar.gz"
if [[ ! -x $binary || ! -s $catalog || ! -s $archive ]]; then
    echo "Build is missing the executable, Russian localization catalog, or archive." >&2
    exit 1
fi

version=$("$binary" --version)
if [[ $version != *+tiles* ]]; then
    echo "The executable does not report graphical tiles support." >&2
    exit 1
fi

python3 build-scripts/validate_json.py

smoke_dir=$(mktemp -d)
trap 'rm -r -- "$smoke_dir"' EXIT
mkdir -p "$smoke_dir/cataclysm-rodina"
if ! XDG_CONFIG_HOME="$smoke_dir" XDG_DATA_HOME="$smoke_dir" \
    timeout 300 "$binary" --userdir "$smoke_dir" --check-mods dda; then
    echo "Game data smoke check failed. Recent game log:" >&2
    tail -n 40 "$smoke_dir/cataclysm-rodina/debug.log" >&2 || true
    exit 1
fi

echo "Linux graphical build and smoke check passed: $archive"
