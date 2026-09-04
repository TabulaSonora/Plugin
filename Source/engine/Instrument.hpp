#pragma once

#include "EngineSettings.hpp"
#include "Resampler.hpp"
#include "Session.hpp"
#include "Snapshot.hpp"

#include "tabulasonora/render_options.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <string>

namespace tsplug {

/// A session driven from a plugin's audio callback, with no render thread and no ring.
///
/// A plugin owns nothing. Its host calls it when it likes, for whatever block size it likes, and
/// during a bounce it calls far faster than realtime -- which a free-running producer cannot
/// follow, so a ring between the two would render silence for exactly the operation people most
/// need to come out right. So the engine runs *on the host's audio thread*, in the callback, for
/// exactly the frames asked for. `ToneGenerator` wants one owning thread, and here that thread is
/// the host's; live MIDI arrives on it too, before the audio between the events is asked for.
///
/// What is left needing a lock is control: loading a ROM and changing settings, which come from
/// the UI. Both do their expensive work *outside* the lock, so the callback's `try_lock` finds it
/// free except for the moment a swap takes. A miss costs one block of silence, and the only
/// operation that can cause one already resets every sounding voice.
///
/// Ported from the Apple player's AUv3 wrapper, with the JUCE-facing parts left to the processor.
class Instrument {
public:
    static constexpr int engineRate = Session::sampleRate;

    Instrument();
    ~Instrument();

    Instrument(const Instrument&) = delete;
    Instrument& operator=(const Instrument&) = delete;

    // -- Control. From one controlling thread; these may block briefly and may throw. --

    /// Opens a `SCCore.dll` and builds a whole new session over it, then swaps it in. The build
    /// is where the 27 MB goes, and it happens with no lock held. Throws on failure and leaves the
    /// running session untouched.
    void loadRom(const std::string& path, bool verifyFully);
    void unloadRom();

    /// Rebuilds the generator under the lock: the tables stay, only the generator over them is
    /// remade, and that is milliseconds.
    void setSettings(const EngineSettings& settings);
    [[nodiscard]] EngineSettings settings() const;

    [[nodiscard]] std::string romName() const;
    [[nodiscard]] std::string romPath() const;
    [[nodiscard]] const ts::BuildProfile* romBuild() const;

    /// Sizes every buffer for a rate and a maximum block, and resets the resamplers. Called from
    /// `prepareToPlay`, which is the one place a plugin is allowed to allocate.
    void prepare(double outputRate, std::size_t maxFrames);

    /// What to report as the plugin's latency, in engine frames: the module's own event staging
    /// (four 1 ms chunks) plus what the resampler costs. Measured rather than derived, in the
    /// Apple port, by where a note's first sound lands: 133 engine frames for the Catmull-Rom,
    /// 130 for the module's stage. The larger is reported whichever path runs, because a latency
    /// that moves with a parameter is one a host cannot use.
    [[nodiscard]] static constexpr int latencyEngineFrames() noexcept
    {
        return Session::eventLatencyFrames() + 5;
    }
    [[nodiscard]] static constexpr double latencySeconds() noexcept
    {
        return static_cast<double>(latencyEngineFrames()) / engineRate;
    }

    /// Runs `f(const ts::NoteRenderer&)` under the lock, for the message thread to resolve names.
    /// Microseconds, and only when a name is not already cached. Not called with no ROM.
    template <typename F>
    void withNotes(F&& f) const
    {
        const std::lock_guard<std::mutex> guard{lock_};
        if (const auto* notes = session_->notes()) {
            f(*notes);
        }
    }

    /// Mute and solo per part, thread-safe atomics read by the engine at the mix.
    [[nodiscard]] ts::ChannelMask& channels() noexcept { return channels_; }
    [[nodiscard]] const ts::ChannelMask& channels() const noexcept { return channels_; }

    // -- What the panel reads. Published by the audio thread, never asked for under the lock. --

    [[nodiscard]] bool hasRom() const noexcept { return hasRom_.load(std::memory_order_relaxed); }
    [[nodiscard]] int activeVoices() const noexcept
    {
        return voices_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] int voiceSlots() const noexcept
    {
        return capacity_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] bool xgMode() const noexcept { return xg_.load(std::memory_order_relaxed); }
    bool readSnapshot(EngineSnapshot& into) noexcept { return snapshots_.read(into); }

    // -- Real-time. The host's audio thread only; nothing here blocks, throws or allocates. --

    /// Takes effect at the top of the next block.
    void setOutputGain(double gain) noexcept
    {
        gain_.store(gain, std::memory_order_relaxed);
        gainChanged_.store(true, std::memory_order_release);
    }

    /// All Sound Off on every part, at the top of the next block. What a stopped transport needs.
    void requestSilence() noexcept { silencePending_.store(true, std::memory_order_release); }

    /// Every part back to power-on, at the top of the next block.
    void requestPanic() noexcept { panicPending_.store(true, std::memory_order_release); }

    /// Fills one block at the output rate. `feed(Session&)` is called first, with the lock held,
    /// to deliver this block's MIDI with sample offsets; then the engine renders exactly the
    /// frames asked for through the selected resampler; then the snapshot is published.
    template <typename Feed>
    void process(float* left, float* right, std::size_t frames, Feed&& feed) noexcept
    {
        const std::unique_lock<std::mutex> guard{lock_, std::try_to_lock};
        if (!guard.owns_lock() || !rateSupported_) {
            // A rebuild or swap is in progress: one block of silence, and it is the same block
            // whose voices the rebuild was going to take anyway.
            silence(left, right, frames);
            return;
        }

        applyPending();
        feed(*session_);
        renderLocked(left, right, frames);
        publishLocked();
    }

private:
    void applyPending() noexcept;
    void renderLocked(float* left, float* right, std::size_t frames) noexcept;
    void publishLocked() noexcept;
    static void silence(float* left, float* right, std::size_t frames) noexcept;

    std::unique_ptr<Session> session_;

    /// Guards `session_`. Taken with `try_lock` on the audio thread and never held there across
    /// anything that could block.
    mutable std::mutex lock_;

    /// Owned here rather than in the session so it outlives every session and generator.
    ts::ChannelMask channels_;

    EngineSettings settings_;
    double outputRate_ = engineRate;
    bool rateSupported_ = true;
    CatmullRomResampler catmullRom_;
    ModuleResampler module_;

    std::atomic<double> gain_{1.0};
    std::atomic<bool> gainChanged_{false};
    std::atomic<bool> silencePending_{false};
    std::atomic<bool> panicPending_{false};

    std::atomic<bool> hasRom_{false};
    std::atomic<int> voices_{0};
    std::atomic<int> capacity_{0};
    std::atomic<bool> xg_{false};
    SnapshotExchange snapshots_;
};

} // namespace tsplug
