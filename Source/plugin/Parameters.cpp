#include "Parameters.hpp"

namespace tsplug::params {

namespace {

template <typename T>
int indexOf(const T& values, typename T::value_type value, int fallback)
{
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (values[i] == value) {
            return static_cast<int>(i);
        }
    }
    return fallback;
}

} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using juce::AudioParameterBool;
    using juce::AudioParameterChoice;
    using juce::AudioParameterFloat;
    using juce::ParameterID;

    const EngineSettings defaults;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID{toneMap, 1}, "Vintage",
        juce::StringArray{"SC-55", "SC-88", "SC-88Pro", "SC-8820", "XG"},
        indexOf(toneMaps, defaults.map, 3)));

    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID{polyphony, 1}, "Polyphony", juce::StringArray{"64 (hardware)", "128", "256"},
        indexOf(polyphonies, defaults.polyphony, 0)));

    layout.add(std::make_unique<AudioParameterBool>(ParameterID{reverb, 1}, "Reverb", defaults.reverb));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID{chorus, 1}, "Chorus", defaults.chorus));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID{delay, 1}, "Delay", defaults.delay));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID{insertionEffects, 1},
                                                    "Insertion effects", defaults.efx));

    layout.add(std::make_unique<AudioParameterBool>(ParameterID{extendedResampler, 1},
                                                    "Extended interpolation",
                                                    defaults.extendedInterpolation));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID{extendedOutputResampler, 1},
                                                    "Extended output resampler",
                                                    defaults.extendedOutputResampler));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID{deliverDroppedSysEx, 1},
                                                    "Deliver dropped SysEx",
                                                    defaults.flushBeforeSysEx));

    // A trim on the way out of the synth, and cutting there is the wrong place to cut: the
    // engine's own level is the floor of the range, not its middle.
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID{gain, 1}, "Output gain", juce::NormalisableRange<float>{1.0F, 2.0F, 0.01F},
        static_cast<float>(defaults.outputGain),
        juce::AudioParameterFloatAttributes{}.withLabel("x")));

    return layout;
}

EngineSettings settingsFrom(const juce::AudioProcessorValueTreeState& state)
{
    const auto value = [&state](const char* id) {
        const auto* raw = state.getRawParameterValue(id);
        return raw != nullptr ? raw->load(std::memory_order_relaxed) : 0.0F;
    };
    const auto choice = [&value](const char* id, std::size_t count) {
        const int index = static_cast<int>(std::lround(value(id)));
        return static_cast<std::size_t>(juce::jlimit(0, static_cast<int>(count) - 1, index));
    };

    EngineSettings settings;
    settings.map = toneMaps[choice(toneMap, toneMaps.size())];
    settings.polyphony = polyphonies[choice(polyphony, polyphonies.size())];
    settings.reverb = value(reverb) >= 0.5F;
    settings.chorus = value(chorus) >= 0.5F;
    settings.delay = value(delay) >= 0.5F;
    settings.efx = value(insertionEffects) >= 0.5F;
    settings.extendedInterpolation = value(extendedResampler) >= 0.5F;
    settings.extendedOutputResampler = value(extendedOutputResampler) >= 0.5F;
    settings.flushBeforeSysEx = value(deliverDroppedSysEx) >= 0.5F;
    settings.outputGain = static_cast<double>(value(gain));
    return settings;
}

} // namespace tsplug::params
