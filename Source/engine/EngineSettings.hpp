#pragma once

#include "tabulasonora/patch_directory.hpp"
#include "tabulasonora/render_options.hpp"
#include "tabulasonora/tone_generator.hpp"

namespace tsplug {

/// Everything a host can set on the engine, as plain values with no JUCE in them.
///
/// The defaults are the plugin's, not the engine's: the engine ships `extended_interpolation` on,
/// which is right for a player whose point is to sound good, but a plugin's claim is narrower --
/// that it is the Sound Canvas voice -- so both departures from the module default to off here.
struct EngineSettings {
    ts::ToneMap map = ts::ToneMap::sc8820;

    /// Voices before stealing. The hardware's own limit is 64; never 0, which makes the pool grow
    /// and allocate inside the block loop.
    int polyphony = 64;

    bool reverb = true;
    bool chorus = true;
    bool delay = true;
    /// The insertion EFX block.
    bool efx = true;

    /// `SincInterpolator` with no pitch-increment ceiling: the engine's one knowing departure.
    bool extendedInterpolation = false;

    /// Convert 32 kHz to the host rate with this plugin's Catmull-Rom rather than the module's own
    /// output stage.
    bool extendedOutputResampler = false;

    /// Deliver SysEx the module's 2048-packet queue would drop.
    bool flushBeforeSysEx = false;

    /// Linear gain on the finished mix. Live, no rebuild.
    double outputGain = 1.0;

    /// One port, sixteen parts. JUCE hands a plugin one sixteen-channel MIDI stream with no cable
    /// nibble in any of its formats, so a second port could never be addressed from a host.
    static constexpr int ports = 1;
    static constexpr int parts = ports * 16;

    /// The module's own note-on staging: four one-millisecond chunks. Not a setting on the
    /// hardware and so not a setting here.
    static constexpr int eventDelayBlocks = 4;

    /// Whether moving from `*this` to `other` needs a new tone generator. Gain is the one thing
    /// that does not.
    [[nodiscard]] bool structurallyDiffers(const EngineSettings& other) const noexcept
    {
        return map != other.map || polyphony != other.polyphony || reverb != other.reverb
               || chorus != other.chorus || delay != other.delay || efx != other.efx
               || extendedInterpolation != other.extendedInterpolation
               || extendedOutputResampler != other.extendedOutputResampler
               || flushBeforeSysEx != other.flushBeforeSysEx;
    }

    [[nodiscard]] ts::ToneGeneratorOptions toOptions(const ts::ChannelMask* channels,
                                                     int hostRate) const noexcept
    {
        ts::ToneGeneratorOptions options;
        options.map = map;
        options.polyphony = polyphony > 0 ? polyphony : 64;
        options.ports = ports;
        options.reverb = reverb;
        options.chorus = chorus;
        options.delay = delay;
        options.efx = efx;
        options.extended_interpolation = extendedInterpolation;
        options.flush_before_sysex = flushBeforeSysEx;
        options.output_gain = outputGain;
        options.channels = channels;
        options.event_delay_blocks = eventDelayBlocks;

        // The module runs one output stage, at the ratio between its rate and the host's. When the
        // plugin's own resampler is doing the conversion, the generator's copy of that stage runs at
        // 1:1 as the module's would at 32 kHz; when the module's stage is the converter (driven by
        // the plugin one frame at a time), the generator's copy would be a second, spurious pass.
        options.bypass_output_filter = !extendedOutputResampler;

        // The rate a live message's sample offset is read against; the engine turns it into
        // milliseconds. Left at 32 kHz it would stamp a 44.1 kHz host nearly forty per cent early.
        options.host_sample_rate = hostRate > 0 ? hostRate : ts::OutputFilter::engine_rate;
        return options;
    }
};

} // namespace tsplug
