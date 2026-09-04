#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace tsplug {

/// The per-user settings file, shared by every instance in a process.
///
/// One key per fact rather than an encoded blob, so a later addition reads back as its default.
/// The ROM record carries the file's size and modification time beside the path, which is what
/// lets a later instance skip the full SHA-256: a file that is the same size with the same mtime
/// as the one that was hashed is the one that was hashed, and the engine's quick identification
/// (size and PE timestamp) still refuses a different build.
class UserSettings {
public:
    UserSettings()
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "Tabula Sonora";
        options.filenameSuffix = "settings";
        options.folderName = "LoSnoCo/Tabula Sonora";
        options.osxLibrarySubFolder = "Application Support";
        options.commonToAllUsers = false;
        options.storageFormat = juce::PropertiesFile::storeAsXML;
        options.millisecondsBeforeSaving = 500;
        properties_.setStorageParameters(options);
    }

    struct RememberedRom {
        juce::File file;
        bool verified = false;
    };

    [[nodiscard]] RememberedRom rememberedRom()
    {
        auto& file = *properties_.getUserSettings();
        const juce::String path = file.getValue("romPath");
        if (path.isEmpty()) {
            return {};
        }
        const juce::File rom{path};
        if (!rom.existsAsFile()) {
            return {};
        }
        const bool unchanged = file.getValue("romSize").getLargeIntValue() == rom.getSize()
                               && file.getValue("romMtime").getLargeIntValue()
                                      == rom.getLastModificationTime().toMilliseconds();
        return {rom, unchanged && file.getBoolValue("romVerified", false)};
    }

    void rememberRom(const juce::File& rom, bool verified)
    {
        auto& file = *properties_.getUserSettings();
        file.setValue("romPath", rom.getFullPathName());
        file.setValue("romSize", rom.getSize());
        file.setValue("romMtime", rom.getLastModificationTime().toMilliseconds());
        file.setValue("romVerified", verified);
        file.setValue("lastRomDirectory", rom.getParentDirectory().getFullPathName());
    }

    [[nodiscard]] juce::File lastRomDirectory()
    {
        const juce::String path = properties_.getUserSettings()->getValue("lastRomDirectory");
        return path.isNotEmpty() ? juce::File{path} : juce::File{};
    }

    [[nodiscard]] bool reduceMotion()
    {
        return properties_.getUserSettings()->getBoolValue("reduceMotion", false);
    }

private:
    juce::ApplicationProperties properties_;
};

using SharedUserSettings = juce::SharedResourcePointer<UserSettings>;

} // namespace tsplug
