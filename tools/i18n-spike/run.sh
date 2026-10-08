#!/usr/bin/env bash
set -euo pipefail

spike_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
build_dir=${RODINA_I18N_SPIKE_BUILD_DIR:-/tmp/rodina-i18n-spike-build}
cargo_bin=${RODINA_I18N_CARGO:-cargo}
mkdir -p "$build_dir"

CARGO_TARGET_DIR="$build_dir/target" "$cargo_bin" build --manifest-path "$spike_dir/Cargo.toml" --release --locked
g++ -std=c++17 -O2 -Wall -Wextra -Werror "$spike_dir/main.cpp" \
    "$build_dir/target/release/librodina_fluent_spike.a" \
    $(pkg-config --cflags --libs icu-i18n icu-uc) -ldl -lpthread -lm \
    -o "$build_dir/i18n-spike"
"$build_dir/i18n-spike" "$spike_dir"
