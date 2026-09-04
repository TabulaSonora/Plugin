// The engine layer without JUCE. Runs with no ROM always, and exercises a real load when
// TS_SCCORE_DLL names one, skipping that half otherwise rather than failing.

#include "engine/Instrument.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

bool allZero(const std::vector<float>& samples)
{
    for (const float sample : samples) {
        if (sample != 0.0F) {
            return false;
        }
    }
    return true;
}

bool anyNonZero(const std::vector<float>& samples)
{
    return !allZero(samples);
}

} // namespace

int main()
{
    tsplug::Instrument instrument;
    instrument.prepare(48000.0, 512);

    std::vector<float> left(512, 1.0F);
    std::vector<float> right(512, 1.0F);
    for (int block = 0; block < 4; ++block) {
        instrument.process(left.data(), right.data(), left.size(), [](tsplug::Session&) {});
        check(allZero(left) && allZero(right), "no ROM renders silence");
    }
    check(!instrument.hasRom(), "no ROM reported");
    check(tsplug::Instrument::latencyEngineFrames() == 133, "latency is 133 engine frames");

    // Every map and both resamplers are accepted without a ROM.
    for (const ts::ToneMap map : {ts::ToneMap::sc55, ts::ToneMap::sc88, ts::ToneMap::sc88pro,
                                  ts::ToneMap::sc8820, ts::ToneMap::xg}) {
        tsplug::EngineSettings settings;
        settings.map = map;
        settings.extendedOutputResampler = (map == ts::ToneMap::xg);
        instrument.setSettings(settings);
        instrument.process(left.data(), right.data(), left.size(), [](tsplug::Session&) {});
        check(allZero(left), "settings change without a ROM still silent");
    }

    const char* dll = std::getenv("TS_SCCORE_DLL");
    if (dll == nullptr || *dll == '\0') {
        std::puts("TS_SCCORE_DLL not set; skipping the loaded-ROM half");
        return failures == 0 ? 0 : 1;
    }

    try {
        instrument.loadRom(dll, false);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: loadRom: %s\n", error.what());
        return 1;
    }
    check(instrument.romBuild() != nullptr, "build identified");

    for (const bool extended : {false, true}) {
        tsplug::EngineSettings settings;
        settings.extendedOutputResampler = extended;
        instrument.setSettings(settings);

        static constexpr std::array<std::uint8_t, 11> gsReset{0xF0, 0x41, 0x10, 0x42, 0x12, 0x40,
                                                              0x00, 0x7F, 0x00, 0x41, 0xF7};
        bool sounded = false;
        for (int block = 0; block < 40; ++block) {
            instrument.process(left.data(), right.data(), left.size(),
                               [block](tsplug::Session& session) {
                                   if (block == 0) {
                                       session.sendSysExAt(0, gsReset);
                                       session.sendChannelAt(0, 0xC0, 48, 0);
                                       session.sendChannelAt(10, 0x90, 60, 100);
                                   }
                               });
            sounded = sounded || anyNonZero(left) || anyNonZero(right);
        }
        check(sounded, extended ? "note sounds through the Catmull-Rom path"
                                : "note sounds through the module's output stage");
        check(instrument.hasRom(), "ROM reported after load");
    }

    tsplug::EngineSnapshot snapshot;
    check(instrument.readSnapshot(snapshot), "a snapshot was published");
    check(snapshot.hasRom, "snapshot sees the ROM");
    check(snapshot.parts[0].program == 48, "snapshot carries the program change");

    return failures == 0 ? 0 : 1;
}
