#include "PluginProcessor.hpp"

#include "Parameters.hpp"
#include "RomLocator.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

#include <span>

namespace tsplug {

namespace {

constexpr const char* stateType = "TSPlug";
constexpr int stateVersion = 1;

} // namespace

PluginProcessor::PluginProcessor()
    : AudioProcessor{BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)}
    , parameters_{*this, nullptr, "state", params::createLayout()}
    , romLoader_{instrument_}
{
    gain_ = parameters_.getRawParameterValue(params::gain);
    for (const char* id : params::structural) {
        parameters_.addParameterListener(id, this);
    }
    instrument_.setSettings(params::settingsFrom(parameters_));

    // A host restores state moments after construction; when it does, the session's own ROM is
    // tried first. Until then, or when it does not, the discovery chain runs on its own.
    loadRomFromKnownPlaces();
}

PluginProcessor::~PluginProcessor()
{
    cancelPendingUpdate();
    for (const char* id : params::structural) {
        parameters_.removeParameterListener(id, this);
    }
}

// -- Audio ---------------------------------------------------------------------------------------

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    instrument_.prepare(sampleRate, static_cast<std::size_t>(juce::jmax(1, samplesPerBlock)));
    setLatencySamples(juce::roundToInt(Instrument::latencySeconds() * sampleRate));
    wasPlaying_ = false;
}

void PluginProcessor::releaseResources()
{
    // Buffers are kept: some hosts render after this, and silence beats a null pointer.
}

void PluginProcessor::reset()
{
    instrument_.requestPanic();
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo only. The module's output stage is a stereo mix and the parts have pan positions in
    // it; folding that to mono is the host's business.
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int frames = buffer.getNumSamples();
    for (int channel = 2; channel < buffer.getNumChannels(); ++channel) {
        buffer.clear(channel, 0, frames);
    }
    if (buffer.getNumChannels() < 2) {
        buffer.clear();
        midi.clear();
        return;
    }

    instrument_.setOutputGain(static_cast<double>(gain_->load(std::memory_order_relaxed)));

    // A stopped transport gets All Sound Off: a note the host was holding would otherwise never
    // meet its note-off and would sound until play was pressed again.
    if (auto* head = getPlayHead()) {
        if (const auto position = head->getPosition()) {
            const bool playing = position->getIsPlaying();
            if (wasPlaying_ && !playing) {
                instrument_.requestSilence();
            }
            wasPlaying_ = playing;
        }
    }

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    instrument_.process(left, right, static_cast<std::size_t>(frames), [&midi](Session& session) {
        for (const juce::MidiMessageMetadata metadata : midi) {
            if (metadata.numBytes <= 0) {
                continue;
            }
            const juce::uint8 status = metadata.data[0];
            if (status == 0xF0) {
                // One complete message, F0 first and F7 last; the engine parses it whole.
                if (metadata.numBytes >= 2 && metadata.data[metadata.numBytes - 1] == 0xF7) {
                    session.sendSysExAt(metadata.samplePosition,
                                        std::span<const std::uint8_t>{
                                            metadata.data,
                                            static_cast<std::size_t>(metadata.numBytes)});
                }
            } else if (status >= 0x80 && status < 0xF0) {
                // Channel voice only. System common and real-time carry no part state, and the
                // engine has no transport of its own for a clock to drive.
                session.sendChannelAt(metadata.samplePosition, status,
                                      metadata.numBytes > 1 ? metadata.data[1] : 0,
                                      metadata.numBytes > 2 ? metadata.data[2] : 0);
            }
        }
    });

    midi.clear();
}

// -- Editor --------------------------------------------------------------------------------------

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor{*this};
}

// -- Parameters ----------------------------------------------------------------------------------

void PluginProcessor::parameterChanged(const juce::String&, float)
{
    // May arrive on the audio thread (automation). Nothing is done here but a wake-up: every
    // structural parameter rebuilds the generator, which allocates, so that happens on the
    // message thread, once, for however many parameters moved together.
    triggerAsyncUpdate();
}

void PluginProcessor::handleAsyncUpdate()
{
    applySettings();
}

void PluginProcessor::applySettings()
{
    instrument_.setSettings(params::settingsFrom(parameters_));
}

// -- Parts ---------------------------------------------------------------------------------------

void PluginProcessor::setPartMuted(int part, bool muted) noexcept
{
    if (part >= 0 && part < EngineSettings::parts) {
        instrument_.channels().set_muted(part, muted);
    }
}

void PluginProcessor::setPartSoloed(int part, bool soloed) noexcept
{
    if (part >= 0 && part < EngineSettings::parts) {
        instrument_.channels().set_soloed(part, soloed);
    }
}

bool PluginProcessor::isPartMuted(int part) const noexcept
{
    return part >= 0 && part < EngineSettings::parts && instrument_.channels().is_muted(part);
}

bool PluginProcessor::isPartSoloed(int part) const noexcept
{
    return part >= 0 && part < EngineSettings::parts && instrument_.channels().is_soloed(part);
}

// -- ROM -----------------------------------------------------------------------------------------

void PluginProcessor::loadRomFromKnownPlaces()
{
    romLoader_.loadFirstOf(romCandidates(sessionRom_, sessionRomVerified_));
}

void PluginProcessor::chooseRom()
{
    auto start = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    if (const auto current = romLoader_.status().file; current.existsAsFile()) {
        start = current.getParentDirectory();
    }

    chooser_ = std::make_unique<juce::FileChooser>("Choose SCCore.dll from a Sound Canvas VA install",
                                                   start, "*.dll;*.DLL");
    chooser_->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [this](const juce::FileChooser& chooser) {
                              const auto chosen = chooser.getResult();
                              if (chosen.existsAsFile()) {
                                  romLoader_.load(chosen, true);
                              }
                          });
}

// -- State ---------------------------------------------------------------------------------------

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree root{stateType};
    root.setProperty("version", stateVersion, nullptr);
    root.appendChild(parameters_.copyState(), nullptr);

    juce::ValueTree rom{"rom"};
    if (const auto status = romLoader_.status(); status.state == RomLoader::State::ready) {
        rom.setProperty("path", status.file.getFullPathName(), nullptr);
        rom.setProperty("buildId", status.buildId, nullptr);
        rom.setProperty("verified", true, nullptr);
    } else if (sessionRom_ != juce::File{}) {
        rom.setProperty("path", sessionRom_.getFullPathName(), nullptr);
        rom.setProperty("verified", sessionRomVerified_, nullptr);
    }
    root.appendChild(rom, nullptr);

    juce::ValueTree mixer{"mixer"};
    for (int part = 0; part < EngineSettings::parts; ++part) {
        juce::ValueTree entry{"part"};
        entry.setProperty("index", part, nullptr);
        entry.setProperty("mute", isPartMuted(part), nullptr);
        entry.setProperty("solo", isPartSoloed(part), nullptr);
        mixer.appendChild(entry, nullptr);
    }
    root.appendChild(mixer, nullptr);

    if (const auto xml = root.createXml()) {
        copyXmlToBinary(*xml, destData);
    }
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr) {
        return;
    }
    const auto root = juce::ValueTree::fromXml(*xml);
    if (!root.hasType(stateType)) {
        return;
    }

    if (const auto state = root.getChildWithName(parameters_.state.getType()); state.isValid()) {
        parameters_.replaceState(state);
    }

    if (const auto mixer = root.getChildWithName("mixer"); mixer.isValid()) {
        for (const auto& entry : mixer) {
            const int part = entry.getProperty("index", -1);
            setPartMuted(part, entry.getProperty("mute", false));
            setPartSoloed(part, entry.getProperty("solo", false));
        }
    }

    // replaceState does not notify parameter listeners for values that did not change through
    // the host, so the engine is told directly.
    applySettings();

    if (const auto rom = root.getChildWithName("rom"); rom.isValid()) {
        const juce::String path = rom.getProperty("path", {});
        if (path.isNotEmpty()) {
            sessionRom_ = juce::File{path};
            sessionRomVerified_ = rom.getProperty("verified", false);
            const auto current = romLoader_.status();
            const bool alreadyThatFile =
                current.state == RomLoader::State::ready && current.file == sessionRom_;
            if (!alreadyThatFile) {
                loadRomFromKnownPlaces();
            }
        }
    }
}

} // namespace tsplug

// The one symbol every wrapper needs.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new tsplug::PluginProcessor{};
}
