#pragma once

#include <juce_core/juce_core.h>

#include <vector>

namespace tsplug {

/// A place a `SCCore.dll` might already be.
struct RomCandidate {
    juce::File file;

    /// Whether whoever put it there has already verified it against the registry, so a quick
    /// identification (size and PE timestamp) is enough rather than the full SHA-256.
    bool markedVerified = false;

    /// For the status line: where it was found.
    juce::String source;
};

/// Every location worth trying before asking the user, in order. The remembered path first (from
/// the plugin's own settings or a saved session), then what the other Tabula Sonora front ends
/// leave behind, then the engine's own environment-variable search.
[[nodiscard]] std::vector<RomCandidate> romCandidates(const juce::File& remembered,
                                                      bool rememberedVerified);

} // namespace tsplug
