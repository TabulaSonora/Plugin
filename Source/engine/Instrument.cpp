#include "Instrument.hpp"

#include "tabulasonora/part.hpp"
#include "tabulasonora/voice_pool.hpp"

#include <algorithm>

namespace tsplug {

Instrument::Instrument()
    : session_{std::make_unique<Session>(channels_)}
{
    // A rate and a block the host will replace at `prepareToPlay`. Sized now so an instance that
    // is rendered before it is prepared -- which should not happen, and does -- has buffers rather
    // than a null pointer.
    prepare(engineRate, 4096);
}

Instrument::~Instrument() = default;

void Instrument::loadRom(const std::string& path, bool verifyFully)
{
    // A whole second session, built with no lock held. The 27 MB of tables and the parse that
    // reads them are the entire cost of this call, and holding the audio thread out for it would
    // be heard as the plugin dying at the moment it was inserted.
    EngineSettings wanted;
    int rate = 0;
    {
        const std::lock_guard<std::mutex> guard{lock_};
        wanted = settings_;
        rate = static_cast<int>(outputRate_);
    }

    auto next = std::make_unique<Session>(channels_);
    next->setSettings(wanted);
    // Carried onto the new session, which starts at the engine's own rate and would otherwise
    // stamp live messages against it.
    next->setHostRate(rate);
    next->loadRom(path, verifyFully);

    std::unique_ptr<Session> previous;
    {
        const std::lock_guard<std::mutex> guard{lock_};
        previous = std::move(session_);
        session_ = std::move(next);
        catmullRom_.reset();
        module_.reset();
        hasRom_.store(session_->hasRom(), std::memory_order_relaxed);
    }
    // Outside the lock: freeing the outgoing tables is the same 27 MB going the other way.
}

void Instrument::unloadRom()
{
    std::unique_ptr<Session> previous;
    {
        const std::lock_guard<std::mutex> guard{lock_};
        previous = std::move(session_);
        session_ = std::make_unique<Session>(channels_);
        session_->setSettings(settings_);
        session_->setHostRate(static_cast<int>(outputRate_));
        hasRom_.store(false, std::memory_order_relaxed);
        voices_.store(0, std::memory_order_relaxed);
        capacity_.store(0, std::memory_order_relaxed);
        xg_.store(false, std::memory_order_relaxed);
    }
}

void Instrument::setSettings(const EngineSettings& settings)
{
    const std::lock_guard<std::mutex> guard{lock_};
    const bool resamplerMoved =
        settings.extendedOutputResampler != settings_.extendedOutputResampler;
    settings_ = settings;
    session_->setSettings(settings);
    if (resamplerMoved) {
        catmullRom_.reset();
        module_.reset();
    }
    gain_.store(settings.outputGain, std::memory_order_relaxed);
    gainChanged_.store(false, std::memory_order_release);
}

EngineSettings Instrument::settings() const
{
    const std::lock_guard<std::mutex> guard{lock_};
    return settings_;
}

std::string Instrument::romName() const
{
    const std::lock_guard<std::mutex> guard{lock_};
    return session_->romName();
}

std::string Instrument::romPath() const
{
    const std::lock_guard<std::mutex> guard{lock_};
    return session_->romPath();
}

const ts::BuildProfile* Instrument::romBuild() const
{
    // Safe to hand out: build profiles live in the library's own registry, parsed once and never
    // freed.
    const std::lock_guard<std::mutex> guard{lock_};
    return session_->romBuild();
}

void Instrument::prepare(double outputRate, std::size_t maxFrames)
{
    const std::lock_guard<std::mutex> guard{lock_};

    outputRate_ = outputRate > 0.0 ? outputRate : engineRate;
    // Below the engine's rate the Catmull-Rom would fold back, and no host asks for it.
    rateSupported_ = outputRate_ >= engineRate;

    catmullRom_.prepare(static_cast<double>(engineRate) / outputRate_, maxFrames);
    module_.prepare(static_cast<int>(outputRate_));
    session_->setHostRate(static_cast<int>(outputRate_));
}

void Instrument::applyPending() noexcept
{
    if (panicPending_.exchange(false, std::memory_order_acquire)) {
        session_->panic();
    }
    if (silencePending_.exchange(false, std::memory_order_acquire)) {
        session_->silence();
    }
    if (gainChanged_.exchange(false, std::memory_order_acquire)) {
        session_->setOutputGain(gain_.load(std::memory_order_relaxed));
    }
}

void Instrument::renderLocked(float* left, float* right, std::size_t frames) noexcept
{
    if (frames == 0) {
        return;
    }

    if (!settings_.extendedOutputResampler) {
        module_.process(
            [this](float& l, float& r) {
                session_->render(std::span<float>{&l, 1}, std::span<float>{&r, 1});
            },
            left, right, frames);
        return;
    }

    const bool fitted = catmullRom_.process(
        [this](float* l, float* r, std::size_t count) {
            session_->render(std::span<float>{l, count}, std::span<float>{r, count});
        },
        left, right, frames);
    if (!fitted) {
        silence(left, right, frames);
    }
}

void Instrument::publishLocked() noexcept
{
    const bool hasRom = session_->hasRom();
    hasRom_.store(hasRom, std::memory_order_relaxed);
    voices_.store(session_->activeVoices(), std::memory_order_relaxed);
    capacity_.store(session_->voiceSlots(), std::memory_order_relaxed);
    xg_.store(session_->xgMode(), std::memory_order_relaxed);

    EngineSnapshot& snapshot = snapshots_.writable();
    snapshot.hasRom = hasRom;
    const ts::ToneGenerator* engine = session_->engine();
    if (engine == nullptr) {
        snapshot = EngineSnapshot{};
        snapshots_.publish();
        return;
    }

    snapshot.xg = engine->xg_mode();
    snapshot.activeVoices = engine->active_voices();
    snapshot.voiceSlots = engine->voice_slots();
    snapshot.noteCount = engine->note_count();

    // Per-part activity from the pool's slots, not from `active()`, which builds a vector: at most
    // a few hundred handles, walked in place.
    std::array<std::uint8_t, EngineSettings::parts> counts{};
    const ts::VoicePool& pool = engine->voices();
    for (int slot = 0; slot < pool.capacity(); ++slot) {
        if (pool.state_of(slot) == ts::VoiceState::free) {
            continue;
        }
        const int part = pool.channel_of(slot);
        if (part >= 0 && part < EngineSettings::parts) {
            auto& count = counts[static_cast<std::size_t>(part)];
            if (count < 255) {
                ++count;
            }
        }
    }

    const int partCount = std::min(engine->parts(), EngineSettings::parts);
    for (int index = 0; index < partCount; ++index) {
        const ts::Part& part = engine->part(index);
        PartSnapshot& into = snapshot.parts[static_cast<std::size_t>(index)];
        into.program = static_cast<std::int16_t>(part.program);
        into.bank = static_cast<std::int16_t>(part.bank);
        into.bankLsb = static_cast<std::int16_t>(part.bank_lsb);
        into.volume = static_cast<std::int16_t>(part.volume());
        into.expression = static_cast<std::int16_t>(part.expression());
        into.pan = static_cast<std::int16_t>(part.pan);
        into.rxChannel = static_cast<std::int16_t>(engine->part_rx_channel(index));
        into.drums = engine->part_is_drum(index);
        into.kit = static_cast<std::int16_t>(into.drums ? engine->part_drum_kit(index) : -1);
        into.map = static_cast<std::int16_t>(engine->part_tone_map(index));
        into.lookupBank = static_cast<std::int16_t>(engine->part_lookup_bank(index));
        into.voices = counts[static_cast<std::size_t>(index)];
    }
    snapshots_.publish();
}

void Instrument::silence(float* left, float* right, std::size_t frames) noexcept
{
    std::fill_n(left, frames, 0.0F);
    std::fill_n(right, frames, 0.0F);
}

} // namespace tsplug
