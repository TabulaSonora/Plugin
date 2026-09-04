#include "Session.hpp"

#include "tabulasonora/part.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace tsplug {

namespace {

std::string fileName(const std::string& path)
{
    const auto slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

} // namespace

void Session::loadRom(const std::string& path, bool verifyFully)
{
    // Built before anything is torn down, so a bad file leaves the running engine alone.
    auto opened = ts::RomImage::open(
        path, verifyFully ? ts::RomVerification::full : ts::RomVerification::quick);

    unloadRom();

    rom_.emplace(std::move(opened));
    notes_.emplace(*rom_);
    romName_ = fileName(path);
    romPath_ = path;
    rebuild();
}

void Session::unloadRom()
{
    // Reverse of the order they were built in: each borrows the one above it.
    engine_.reset();
    notes_.reset();
    rom_.reset();
    romName_.clear();
    romPath_.clear();
}

void Session::setSettings(const EngineSettings& settings)
{
    const bool structural = settings_.structurallyDiffers(settings);
    settings_ = settings;

    if (!engine_) {
        return;
    }
    if (structural) {
        rebuild();
    } else {
        engine_->set_output_gain(settings_.outputGain);
    }
}

void Session::setHostRate(int hostRate)
{
    if (hostRate == hostRate_) {
        return;
    }
    hostRate_ = hostRate;

    // With no ROM there is no generator to rebuild; the rate is picked up when one is loaded. A
    // plugin sets its rate before it has a ROM as a matter of course.
    if (engine_) {
        rebuild();
    }
}

void Session::setOutputGain(double gain) noexcept
{
    settings_.outputGain = gain;
    if (engine_) {
        engine_->set_output_gain(gain);
    }
}

void Session::panic()
{
    if (engine_) {
        engine_->reset();
    }
}

void Session::silence()
{
    if (!engine_) {
        return;
    }
    constexpr int allSoundOff = 120;
    for (int channel = 0; channel < EngineSettings::parts; ++channel) {
        sendControl(channel, allSoundOff, 0);
    }
}

void Session::render(std::span<float> left, std::span<float> right)
{
    if (engine_) {
        engine_->render(left, right);
        return;
    }
    // The caller reuses its block, so leaving it untouched would repeat the previous one.
    std::fill(left.begin(), left.end(), 0.0F);
    std::fill(right.begin(), right.end(), 0.0F);
}

void Session::sendChannelAt(int sampleOffset, int status, int data1, int data2)
{
    if (engine_) {
        engine_->send_channel_at(sampleOffset, 0, status, data1, data2);
    }
}

void Session::sendSysExAt(int sampleOffset, std::span<const std::uint8_t> bytes)
{
    if (engine_ && !bytes.empty()) {
        engine_->send_sysex_at(sampleOffset, 0, bytes);
    }
}

int Session::activeVoices() const noexcept
{
    return engine_ ? engine_->active_voices() : 0;
}

int Session::voiceSlots() const noexcept
{
    return engine_ ? engine_->voice_slots() : 0;
}

bool Session::xgMode() const noexcept
{
    return engine_ && engine_->xg_mode();
}

void Session::sendControl(int channel, int controller, int value)
{
    if (engine_) {
        engine_->send_channel(0, 0xB0 | (channel & 0x0F), controller, value);
    }
}

void Session::rebuild()
{
    // A rebuild makes fresh parts at their power-on values; capture the outgoing ones first, or a
    // vintage change would silently reset every part to piano.
    std::vector<std::array<int, 7>> previous;
    if (engine_) {
        previous.reserve(static_cast<std::size_t>(engine_->parts()));
        for (int index = 0; index < engine_->parts(); ++index) {
            const ts::Part& part = engine_->part(index);
            previous.push_back({part.bank, part.program, part.volume(), part.pan, part.expression(),
                                part.reverb_send, part.chorus_send});
        }
    }

    engine_.emplace(*notes_, settings_.toOptions(channels_, hostRate_));

    // Parts are restored by replaying MIDI, because `part()` is const: the engine owns its part
    // state and a controller is the only way in. Bank before program, as anything selecting a
    // sound must -- the program change is what latches the pair.
    for (int index = 0; index < static_cast<int>(previous.size()) && index < engine_->parts();
         ++index) {
        const auto& [bank, program, volume, pan, expression, reverb, chorus] =
            previous[static_cast<std::size_t>(index)];
        sendControl(index, 0, bank);
        engine_->send_channel(0, 0xC0 | (index & 0x0F), program, 0);
        sendControl(index, 7, volume);
        sendControl(index, 10, pan);
        sendControl(index, 11, expression);
        sendControl(index, 91, reverb);
        sendControl(index, 93, chorus);
    }
}

} // namespace tsplug
