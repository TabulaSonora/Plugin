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

# Environment, for CI:
#   TSPLUG_SIGN_IDENTITY   the codesign identity; `-` for an ad-hoc signature (unsigned build)
#   TSPLUG_NOTARY_KEY      path to an App Store Connect .p8, with TSPLUG_NOTARY_KEY_ID and
#                          TSPLUG_NOTARY_ISSUER, instead of the keychain profile
#   TSPLUG_SKIP_NOTARISE   set to 1 to sign and pack without submitting to Apple
#   TSPLUG_BUILD_DIR       the configured build tree (default build/universal)
#   TSPLUG_ARCH_LABEL      what the DMG name says about the architecture (default universal)
PROFILE="${1:-TabulaSonora}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${TSPLUG_BUILD_DIR:-$ROOT/build/universal}"
ART="$BUILD/TSPlug_artefacts/RelWithDebInfo"
OUT="$ROOT/build/release-out"
IDENTITY="${TSPLUG_SIGN_IDENTITY:-Developer ID Application}"
ARCH="${TSPLUG_ARCH_LABEL:-universal}"
VERSION="$(sed -nE 's/^[[:space:]]*VERSION ([0-9.]+)$/\1/p' "$ROOT/CMakeLists.txt" | head -1)"
ENTITLEMENTS="$ROOT/tools/hardened.entitlements"

# An ad-hoc signature has no certificate to timestamp, and nothing Apple would notarise.
ADHOC=0
if [[ "$IDENTITY" == "-" ]]; then
    ADHOC=1
fi
NOTARISE=1
if [[ "$ADHOC" == 1 || "${TSPLUG_SKIP_NOTARISE:-0}" == 1 ]]; then
    NOTARISE=0
fi
NOTARY_ARGS=()
if [[ -n "${TSPLUG_NOTARY_KEY:-}" ]]; then
    NOTARY_ARGS=(--key "$TSPLUG_NOTARY_KEY" --key-id "$TSPLUG_NOTARY_KEY_ID" --issuer "$TSPLUG_NOTARY_ISSUER")
else
    NOTARY_ARGS=(--keychain-profile "$PROFILE")
fi

[[ -d "$ART" ]] || { echo "No universal build at $ART; run: cmake --preset universal && cmake --build --preset universal" >&2; exit 1; }

rm -rf "$OUT" && mkdir -p "$OUT/stage"
cp -R "$ART/Standalone/Tabula Sonora.app" "$ART/VST3/Tabula Sonora.vst3" "$ART/AU/Tabula Sonora.component" \
      "$ART/CLAP/Tabula Sonora.clap" "$ART/LV2/Tabula Sonora.lv2" "$OUT/stage/"
cp "$ROOT/LICENSE" "$ROOT/NOTICE.md" "$ROOT/README.md" "$OUT/stage/"

sign() {
    if [[ "$ADHOC" == 1 ]]; then
        codesign --force --options runtime --entitlements "$ENTITLEMENTS" --sign - "$@"
    else
        codesign --force --timestamp --options runtime --entitlements "$ENTITLEMENTS" --sign "$IDENTITY" "$@"
    fi
}
notarise() {
    xcrun notarytool submit "$1" "${NOTARY_ARGS[@]}" --wait
}

echo "== signing"
# Bundles: --deep is discouraged, so the nested binary is signed by signing the bundle itself
# (each has exactly one Mach-O). The LV2 is a folder holding a bare dylib, signed directly.
for bundle in "$OUT/stage/Tabula Sonora.vst3" "$OUT/stage/Tabula Sonora.component" \
              "$OUT/stage/Tabula Sonora.clap" "$OUT/stage/Tabula Sonora.app"; do
    sign "$bundle"
done
sign "$OUT/stage/Tabula Sonora.lv2/libTabula Sonora.so"

if [[ "$NOTARISE" == 1 ]]; then
    echo "== notarising the bundles"
    # One zip per bundle so a failure names the format. The LV2 dylib rides in its own zip; it
    # cannot be stapled, so hosts check its ticket online, which is what Apple does for bare
    # dylibs anyway.
    mkdir -p "$OUT/zips"
    for item in "Tabula Sonora.vst3" "Tabula Sonora.component" "Tabula Sonora.clap" "Tabula Sonora.app" "Tabula Sonora.lv2"; do
        zip="$OUT/zips/${item// /-}.zip"
        ditto -c -k --keepParent "$OUT/stage/$item" "$zip"
        notarise "$zip"
    done
    for bundle in "Tabula Sonora.vst3" "Tabula Sonora.component" "Tabula Sonora.clap" "Tabula Sonora.app"; do
        xcrun stapler staple "$OUT/stage/$bundle"
    done
else
    echo "== not notarising (${ADHOC:+ad-hoc signature})"
fi

echo "== packing"
DMG="$OUT/TabulaSonora-Plugin-$VERSION-macos-$ARCH.dmg"
hdiutil create -volname "Tabula Sonora $VERSION" -srcfolder "$OUT/stage" -ov -format UDZO "$DMG" >/dev/null
sign "$DMG"
if [[ "$NOTARISE" == 1 ]]; then
    notarise "$DMG"
    xcrun stapler staple "$DMG"
fi

echo "== verifying"
for bundle in "Tabula Sonora.vst3" "Tabula Sonora.component" "Tabula Sonora.clap" "Tabula Sonora.app"; do
    codesign --verify --deep --strict --verbose=1 "$OUT/stage/$bundle"
    if [[ "$NOTARISE" == 1 ]]; then
        xcrun stapler validate "$OUT/stage/$bundle" | tail -1
    fi
done
if [[ "$NOTARISE" == 1 ]]; then
    spctl -a -vv -t install "$DMG"
fi
echo "$DMG"
