#!/usr/bin/env bash
# Packs a Windows or Linux build into one archive: every format the platform has, with the
# licence and notice beside them. Bash on both, because the Windows runner has Git's, and
# `cmake -E tar` writes zip and tar.gz alike, so there is one script and no zip binary to find.
#
# Usage: tools/package.sh <windows|linux> [build dir]
set -euo pipefail

PLATFORM="$1"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${2:-$ROOT/build/release}"
ART="$BUILD/TSPlug_artefacts/RelWithDebInfo"
OUT="$ROOT/build/release-out"
VERSION="$(sed -nE 's/^[[:space:]]*VERSION ([0-9.]+)$/\1/p' "$ROOT/CMakeLists.txt" | head -1)"
ARCH="$(uname -m | sed 's/AMD64/x86_64/')"
NAME="TabulaSonora-Plugin-$VERSION-$PLATFORM-$ARCH"

[[ -d "$ART" ]] || { echo "No build at $ART" >&2; exit 1; }

rm -rf "$OUT/$NAME" && mkdir -p "$OUT/$NAME"
cp -R "$ART/VST3/Tabula Sonora.vst3" "$ART/LV2/Tabula Sonora.lv2" "$OUT/$NAME/"
cp "$ART/CLAP/Tabula Sonora.clap" "$OUT/$NAME/"
if [[ "$PLATFORM" == windows ]]; then
    cp "$ART/Standalone/Tabula Sonora.exe" "$OUT/$NAME/"
else
    cp "$ART/Standalone/Tabula Sonora" "$OUT/$NAME/"
fi
cp "$ROOT/LICENSE" "$ROOT/NOTICE.md" "$ROOT/README.md" "$OUT/$NAME/"

# RelWithDebInfo leaves the debug info in every ELF, and there are four copies of the engine
# here: stripped they are a fifth of the size. MSVC keeps its symbols in separate .pdb files,
# which JUCE writes beside each binary inside the bundle folders; those go too.
if [[ "$PLATFORM" == linux ]]; then
    find "$OUT/$NAME" -type f \( -name '*.so' -o -name 'Tabula Sonora' \) -exec strip --strip-unneeded {} +
else
    find "$OUT/$NAME" -type f \( -name '*.pdb' -o -name '*.ilk' -o -name '*.exp' -o -name '*.lib' \) -delete
fi

cd "$OUT"
if [[ "$PLATFORM" == windows ]]; then
    cmake -E tar cf "$NAME.zip" --format=zip "$NAME"
    echo "$OUT/$NAME.zip"
else
    cmake -E tar czf "$NAME.tar.gz" "$NAME"
    echo "$OUT/$NAME.tar.gz"
fi
