#!/bin/zsh
# Signs, notarises and staples a universal release of every format, and packs it into a DMG.
#
# Usage: tools/release.sh [notarytool keychain profile]   (default: TabulaSonora)
#
# The profile is created once with
#     xcrun notarytool store-credentials TabulaSonora --apple-id ... --team-id ... --password ...
# so this script never sees an Apple password. The build itself is `cmake --preset universal`;
# this runs after it. Hardened runtime is applied at signing time rather than through JUCE's
# Xcode attribute, because the presets use Ninja and the attribute only reaches the Xcode
# generator. The DMG is what is notarised: notarytool takes a zip, dmg or pkg, and stapling a DMG
# staples nothing inside it, so every bundle is stapled first and then packed.
set -euo pipefail

PROFILE="${1:-TabulaSonora}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ART="$ROOT/build/universal/TSPlug_artefacts/RelWithDebInfo"
OUT="$ROOT/build/release-out"
IDENTITY="${TSPLUG_SIGN_IDENTITY:-Developer ID Application}"
VERSION="$(sed -nE 's/^[[:space:]]*VERSION ([0-9.]+)$/\1/p' "$ROOT/CMakeLists.txt" | head -1)"
ENTITLEMENTS="$ROOT/tools/hardened.entitlements"

[[ -d "$ART" ]] || { echo "No universal build at $ART; run: cmake --preset universal && cmake --build --preset universal" >&2; exit 1; }

rm -rf "$OUT" && mkdir -p "$OUT/stage"
cp -R "$ART/Standalone/Tabula Sonora.app" "$ART/VST3/Tabula Sonora.vst3" "$ART/AU/Tabula Sonora.component" \
      "$ART/CLAP/Tabula Sonora.clap" "$ART/LV2/Tabula Sonora.lv2" "$OUT/stage/"
cp "$ROOT/LICENSE" "$ROOT/NOTICE.md" "$ROOT/README.md" "$OUT/stage/"

sign() {
    codesign --force --timestamp --options runtime --entitlements "$ENTITLEMENTS" --sign "$IDENTITY" "$@"
}

echo "== signing"
# Bundles: --deep is discouraged, so the nested binary is signed by signing the bundle itself
# (each has exactly one Mach-O). The LV2 is a folder holding a bare dylib, signed directly.
for bundle in "$OUT/stage/Tabula Sonora.vst3" "$OUT/stage/Tabula Sonora.component" \
              "$OUT/stage/Tabula Sonora.clap" "$OUT/stage/Tabula Sonora.app"; do
    sign "$bundle"
done
sign "$OUT/stage/Tabula Sonora.lv2/libTabula Sonora.so"

echo "== notarising the bundles"
# One zip per bundle so a failure names the format. The LV2 dylib rides in its own zip; it cannot
# be stapled, so hosts check its ticket online, which is what Apple does for bare dylibs anyway.
mkdir -p "$OUT/zips"
for item in "Tabula Sonora.vst3" "Tabula Sonora.component" "Tabula Sonora.clap" "Tabula Sonora.app" "Tabula Sonora.lv2"; do
    zip="$OUT/zips/${item// /-}.zip"
    ditto -c -k --keepParent "$OUT/stage/$item" "$zip"
    xcrun notarytool submit "$zip" --keychain-profile "$PROFILE" --wait
done
for bundle in "Tabula Sonora.vst3" "Tabula Sonora.component" "Tabula Sonora.clap" "Tabula Sonora.app"; do
    xcrun stapler staple "$OUT/stage/$bundle"
done

echo "== packing"
DMG="$OUT/TabulaSonora-Plugin-$VERSION.dmg"
hdiutil create -volname "Tabula Sonora $VERSION" -srcfolder "$OUT/stage" -ov -format UDZO "$DMG" >/dev/null
sign "$DMG"
xcrun notarytool submit "$DMG" --keychain-profile "$PROFILE" --wait
xcrun stapler staple "$DMG"

echo "== verifying"
spctl -a -vv -t install "$DMG"
for bundle in "Tabula Sonora.vst3" "Tabula Sonora.component" "Tabula Sonora.clap" "Tabula Sonora.app"; do
    codesign --verify --deep --strict --verbose=1 "$OUT/stage/$bundle"
    xcrun stapler validate "$OUT/stage/$bundle" | tail -1
done
echo "$DMG"
