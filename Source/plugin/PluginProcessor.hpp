#pragma once

#include "RomLoader.hpp"
#include "engine/Instrument.hpp"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <memory>

namespace tsplug {

class PluginProcessor final : public juce::AudioProcessor,
                              private juce::AudioProcessorValueTreeState::Listener,
                              private juce::AsyncUpdater {
public:
    PluginProcessor();
    ~PluginProcessor() override;

    // -- juce::AudioProcessor --
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // -- For the editor --
    Instrument& instrument() noexcept { return instrument_; }
    RomLoader& romLoader() noexcept { return romLoader_; }
    juce::AudioProcessorValueTreeState& parameters() noexcept { return parameters_; }

    void setPartMuted(int part, bool muted) noexcept;
    void setPartSoloed(int part, bool soloed) noexcept;
    [[nodiscard]] bool isPartMuted(int part) const noexcept;
    [[nodiscard]] bool isPartSoloed(int part) const noexcept;

    /// Opens the native file chooser for a `SCCore.dll`. Owned here rather than by the editor so a
    /// load outlives the window it was started from.
    void chooseRom();

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void applySettings();
    void loadRomFromKnownPlaces();

    Instrument instrument_;
    juce::AudioProcessorValueTreeState parameters_;
    RomLoader romLoader_;
    std::atomic<float>* gain_ = nullptr;

    bool wasPlaying_ = false;

    /// The ROM a saved session names, tried before the discovery chain when the session opens.
    juce::File sessionRom_;
    bool sessionRomVerified_ = false;

    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

} // namespace tsplug
