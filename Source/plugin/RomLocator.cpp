#include "RomLocator.hpp"

#include "tabulasonora/rom_locator.hpp"

namespace tsplug {

namespace {

constexpr const char* romFileName = "SCCore.dll";

void addIfPresent(std::vector<RomCandidate>& into, const juce::File& file, bool verified,
                  const juce::String& source)
{
    if (!file.existsAsFile()) {
        return;
    }
    for (const auto& existing : into) {
        if (existing.file == file) {
            return;
        }
    }
    into.push_back({file, verified, source});
}

} // namespace

std::vector<RomCandidate> romCandidates(const juce::File& remembered, bool rememberedVerified)
{
    std::vector<RomCandidate> found;

    if (remembered != juce::File{}) {
        addIfPresent(found, remembered, rememberedVerified, "remembered");
    }

#if JUCE_MAC
    // The Apple player imports the ROM into an app-group container it shares with its AUv3, and
    // leaves an empty `SCCore.dll.verified` beside it once the hash has checked out. The group is
    // prefixed by the signing team's identifier, which is why the directory is matched by suffix
    // rather than named.
    const auto containers = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                .getChildFile("Library/Group Containers");
    for (const auto& group :
         containers.findChildFiles(juce::File::findDirectories, false,
                                   "*group.co.losno.tabula-sonora")) {
        const auto rom = group.getChildFile(romFileName);
        addIfPresent(found, rom, rom.getSiblingFile("SCCore.dll.verified").existsAsFile(),
                     "Tabula Sonora Player");
    }
#endif

    // The Linux player keeps it in the XDG data directory. Its verified flag lives in GSettings,
    // which is not worth reading from here, so that copy is always hashed once.
    {
        juce::File dataHome;
        if (const auto xdg = juce::SystemStats::getEnvironmentVariable("XDG_DATA_HOME", {});
            xdg.isNotEmpty()) {
            dataHome = juce::File{xdg};
        } else {
            dataHome = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                           .getChildFile(".local/share");
        }
        addIfPresent(found, dataHome.getChildFile("tabula-sonora").getChildFile(romFileName), false,
                     "Tabula Sonora Player (Linux)");
    }

    // Then the engine's own rule: $TS_SCCORE_DLL, then ./SCCore.dll.
    if (const auto located = ts::locate_rom(""); located.found()) {
        addIfPresent(found, juce::File{juce::String{located.path.string()}}, false,
                     "TS_SCCORE_DLL");
    }

    return found;
}

} // namespace tsplug
