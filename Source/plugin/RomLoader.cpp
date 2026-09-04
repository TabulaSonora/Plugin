#include "RomLoader.hpp"

#include "tabulasonora/build_registry.hpp"

#include <exception>

namespace tsplug {

RomLoader::RomLoader(Instrument& instrument)
    : juce::Thread{"Tabula Sonora ROM loader"}
    , instrument_{instrument}
{
}

RomLoader::~RomLoader()
{
    cancelPendingUpdate();
    signalThreadShouldExit();
    notify();
    // Long enough for a full hash to finish; a load that has been going for longer than this is
    // stuck on IO and the process is on its way out anyway.
    stopThread(15000);
}

void RomLoader::load(const juce::File& file, bool verifyFully)
{
    loadFirstOf({RomCandidate{file, !verifyFully, "chosen"}});
}

void RomLoader::loadFirstOf(std::vector<RomCandidate> candidates)
{
    {
        const juce::ScopedLock guard{lock_};
        pending_ = std::move(candidates);
        status_.state = State::loading;
        status_.message.clear();
    }
    broadcaster.sendChangeMessage();
    if (!isThreadRunning()) {
        startThread(Priority::normal);
    } else {
        notify();
    }
}

RomLoader::Status RomLoader::status() const
{
    const juce::ScopedLock guard{lock_};
    return status_;
}

void RomLoader::run()
{
    while (!threadShouldExit()) {
        std::optional<std::vector<RomCandidate>> request;
        {
            const juce::ScopedLock guard{lock_};
            request.swap(pending_);
        }
        if (!request) {
            wait(-1);
            continue;
        }

        Status result;
        if (request->empty()) {
            result.state = State::none;
            publish(result);
            continue;
        }

        for (const auto& candidate : *request) {
            if (threadShouldExit()) {
                return;
            }
            try {
                const bool verifyFully = !candidate.markedVerified;
                instrument_.loadRom(candidate.file.getFullPathName().toStdString(), verifyFully);
                result.state = State::ready;
                result.file = candidate.file;
                result.verifiedFully = verifyFully;
                result.message = candidate.source;
                if (const auto* build = instrument_.romBuild()) {
                    result.buildId = juce::String{build->id()};
                }
                break;
            } catch (const std::exception& error) {
                result.state = request->size() == 1 ? State::failed : State::none;
                result.file = candidate.file;
                result.message = juce::String{error.what()};
            }
        }
        publish(result);
    }
}

void RomLoader::publish(Status status)
{
    {
        const juce::ScopedLock guard{lock_};
        status_ = std::move(status);
    }
    triggerAsyncUpdate();
}

void RomLoader::handleAsyncUpdate()
{
    const Status current = status();
    if (current.state == State::ready && onLoaded) {
        onLoaded(current);
    }
    broadcaster.sendChangeMessage();
}

} // namespace tsplug
