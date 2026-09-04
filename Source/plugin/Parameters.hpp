#pragma once

#include "engine/EngineSettings.hpp"

#include <juce_audio_processors/juce_audio_processors.h>

namespace tsplug::params {

// Identifiers are part of the plugin's saved format. Append, never rename or reorder; every new
// parameter takes the next version hint so AU hosts keep their ordering stable.
inline constexpr const char* toneMap = "toneMap";
inline constexpr const char* polyphony = "polyphony";
inline constexpr const char* reverb = "reverb";
inline constexpr const char* chorus = "chorus";
inline constexpr const char* delay = "delay";
inline constexpr const char* insertionEffects = "insertionEffects";
inline constexpr const char* extendedResampler = "extendedResampler";
inline constexpr const char* extendedOutputResampler = "extendedOutputResampler";
inline constexpr const char* deliverDroppedSysEx = "deliverDroppedSysEx";
inline constexpr const char* gain = "gain";

/// Every parameter that rebuilds the tone generator; the processor listens to these and applies
/// them off the audio thread in one go.
inline constexpr std::array<const char*, 9> structural{
    toneMap, polyphony, reverb, chorus, delay, insertionEffects,
    extendedResampler, extendedOutputResampler, deliverDroppedSysEx};

/// The tone maps in the order the choice parameter lists them.
inline constexpr std::array<ts::ToneMap, 5> toneMaps{
    ts::ToneMap::sc55, ts::ToneMap::sc88, ts::ToneMap::sc88pro, ts::ToneMap::sc8820,
    ts::ToneMap::xg};
inline constexpr std::array<int, 3> polyphonies{64, 128, 256};

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/// Reads the whole engine settings block out of the tree. Message thread.
EngineSettings settingsFrom(const juce::AudioProcessorValueTreeState& state);

} // namespace tsplug::params
