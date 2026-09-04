#pragma once

#include "EngineSettings.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace tsplug {

/// One part's state, as of the last block rendered. Numbers only: names are resolved on the
/// message thread from these, and cached there.
struct PartSnapshot {
    std::int16_t program = 0;
    std::int16_t bank = 0;
    std::int16_t bankLsb = 0;
    std::int16_t volume = 0;
    std::int16_t expression = 0;
    std::int16_t pan = 64;
    /// The MIDI channel this part listens on, zero-based -- not its slot. GS can point several
    /// parts at one channel or detach one from every channel, so a row is labelled with this.
    std::int16_t rxChannel = -1;
    /// The kit sounding on a drum part, or -1.
    std::int16_t kit = -1;
    /// `ts::ToneMap` this part's program resolves against, per part and per moment.
    std::int16_t map = 0;
    std::int16_t lookupBank = 0;
    /// Whether this part is sounding drums *now*, which is not "is this the drum channel".
    bool drums = false;
    /// Voices this part is sounding, including any still releasing.
    std::uint8_t voices = 0;
};

struct EngineSnapshot {
    bool hasRom = false;
    bool xg = false;
    int activeVoices = 0;
    int voiceSlots = 0;
    int noteCount = 0;
    std::array<PartSnapshot, EngineSettings::parts> parts{};
};

/// A latest-value exchange from the audio thread to the message thread: three slots and one
/// atomic. The writer always has a slot to fill and never waits; the reader always gets the most
/// recent complete snapshot and never a torn one. Nothing here allocates or blocks.
class SnapshotExchange {
public:
    /// Audio thread. The returned slot is written in place, then `publish` swaps it in.
    [[nodiscard]] EngineSnapshot& writable() noexcept { return slots_[write_]; }

    void publish() noexcept
    {
        const auto previous = middle_.exchange(static_cast<std::uint8_t>(write_ | dirtyBit),
                                               std::memory_order_acq_rel);
        write_ = previous & indexMask;
    }

    /// Message thread. Returns whether anything new arrived since the last read.
    bool read(EngineSnapshot& into) noexcept
    {
        if ((middle_.load(std::memory_order_acquire) & dirtyBit) != 0) {
            const auto previous =
                middle_.exchange(static_cast<std::uint8_t>(read_), std::memory_order_acq_rel);
            read_ = previous & indexMask;
            into = slots_[read_];
            return true;
        }
        return false;
    }

private:
    static constexpr std::uint8_t indexMask = 0x03;
    static constexpr std::uint8_t dirtyBit = 0x04;

    std::array<EngineSnapshot, 3> slots_{};
    std::size_t write_ = 0;
    std::size_t read_ = 1;
    std::atomic<std::uint8_t> middle_{2};
};

} // namespace tsplug
