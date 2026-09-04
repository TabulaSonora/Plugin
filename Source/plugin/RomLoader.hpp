#pragma once

#include "RomLocator.hpp"
#include "engine/Instrument.hpp"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <functional>
#include <optional>
#include <vector>

namespace tsplug {

/// Loads a ROM into the instrument on its own thread and reports on the message thread.
///
/// One persistent thread rather than one per request, because the plugin can be destroyed at any
/// moment by the host and a thread that is still hashing 27 MB has to be joined before the
/// instrument it writes into goes away. Results cross back through an `AsyncUpdater`, which the
/// destructor cancels, so nothing ever calls into a dead loader.
class RomLoader : private juce::Thread, private juce::AsyncUpdater {
public:
    enum class State { none, loading, failed, ready };

    struct Status {
        State state = State::none;
        /// Why it failed, or where a candidate was found.
        juce::String message;
        juce::String buildId;
        juce::File file;
        /// Whether this load hashed the whole file, so a caller may record it as verified.
        bool verifiedFully = false;
    };

    explicit RomLoader(Instrument& instrument);
    ~RomLoader() override;

    /// One specific file. A failure is reported as `failed`.
    void load(const juce::File& file, bool verifyFully);

    /// Tries `candidates` in order until one loads. With nothing loadable the state goes back to
    /// `none` (with the last failure as the message), which is what shows the Load button.
    void loadFirstOf(std::vector<RomCandidate> candidates);

    [[nodiscard]] Status status() const;

    /// Message thread; fires after every state change.
    juce::ChangeBroadcaster broadcaster;

    /// Message thread; fires once per successful load, for whoever persists the path.
    std::function<void(const Status&)> onLoaded;

private:
    void run() override;
    void handleAsyncUpdate() override;
    void publish(Status status);

    Instrument& instrument_;

    mutable juce::CriticalSection lock_;
    Status status_;
    std::optional<std::vector<RomCandidate>> pending_;
};

} // namespace tsplug
