#pragma once

#include "EngineSettings.hpp"

#include "tabulasonora/build_registry.hpp"
#include "tabulasonora/note_renderer.hpp"
#include "tabulasonora/render_options.hpp"
#include "tabulasonora/rom_image.hpp"
#include "tabulasonora/tone_generator.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace tsplug {

/// The engine chain and everything that can be done to it, for a plugin: no song, no export.
///
/// Holds the objects in the order the library builds them -- a `RomImage` over the file, one
/// `NoteRenderer` that loads the tables, a `ToneGenerator` over that -- because each borrows the one
/// above. Changing a setting rebuilds the generator but not the note renderer, so switching vintage
/// or effects costs only the sounding voices, never the 27 MB read.
///
/// Not thread-safe, and does not need to be: `Instrument` gives it one owning thread at a time,
/// which is the contract `ToneGenerator` asks for.
class Session {
public:
    static constexpr int sampleRate = ts::ToneGenerator::sample_rate;

    /// The mask lives outside the session, in `Instrument`, so mute and solo survive both a
    /// generator rebuild and a whole new session over a different ROM.
    explicit Session(const ts::ChannelMask& channels) noexcept
        : channels_{&channels}
    {
    }

    /// Opens a `SCCore.dll` and builds the engine over it. Throws `ts::RomIdentityError` for a file
    /// that is not a build the engine knows, `std::runtime_error` when it cannot be read.
    void loadRom(const std::string& path, bool verifyFully);
    void unloadRom();

    [[nodiscard]] bool hasRom() const noexcept { return engine_.has_value(); }
    [[nodiscard]] const std::string& romName() const noexcept { return romName_; }
    [[nodiscard]] const std::string& romPath() const noexcept { return romPath_; }
    [[nodiscard]] const ts::BuildProfile* romBuild() const noexcept
    {
        return rom_ ? &rom_->build() : nullptr;
    }
    [[nodiscard]] const ts::RomImage* rom() const noexcept { return rom_ ? &*rom_ : nullptr; }

    /// All the generator settings at once: one rebuild, with part state carried across it.
    void setSettings(const EngineSettings& settings);
    [[nodiscard]] const EngineSettings& settings() const noexcept { return settings_; }

    /// The rate the host is asking for. A construction option, so it rebuilds.
    void setHostRate(int hostRate);
    [[nodiscard]] int hostRate() const noexcept { return hostRate_; }

    /// The gain on the finished mix, without a rebuild.
    void setOutputGain(double gain) noexcept;

    /// Silences everything and returns every part to its power-on state.
    void panic();

    /// All Sound Off (CC 120) on every part: stops every note without touching any controller, so
    /// the parts resume into exactly the state they were in.
    void silence();

    /// The generator's next block at 32 kHz, planar. Silence when there is no ROM.
    void render(std::span<float> left, std::span<float> right);

    void sendChannelAt(int sampleOffset, int status, int data1, int data2);
    void sendSysExAt(int sampleOffset, std::span<const std::uint8_t> bytes);

    [[nodiscard]] int activeVoices() const noexcept;
    [[nodiscard]] int voiceSlots() const noexcept;
    [[nodiscard]] bool xgMode() const noexcept;

    /// What the generator's event pipeline delays a message by, in engine frames.
    [[nodiscard]] static constexpr int eventLatencyFrames() noexcept
    {
        return EngineSettings::eventDelayBlocks * ts::ToneGenerator::block_size;
    }

    /// For snapshots and name lookups. Null without a ROM.
    [[nodiscard]] const ts::ToneGenerator* engine() const noexcept
    {
        return engine_ ? &*engine_ : nullptr;
    }
    [[nodiscard]] const ts::NoteRenderer* notes() const noexcept
    {
        return notes_ ? &*notes_ : nullptr;
    }

private:
    void rebuild();
    void sendControl(int channel, int controller, int value);

    std::optional<ts::RomImage> rom_;
    std::optional<ts::NoteRenderer> notes_;
    std::optional<ts::ToneGenerator> engine_;

    std::string romName_;
    std::string romPath_;
    EngineSettings settings_;
    int hostRate_ = 0;
    const ts::ChannelMask* channels_;
};

} // namespace tsplug
