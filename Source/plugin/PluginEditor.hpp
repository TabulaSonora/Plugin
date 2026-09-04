#pragma once

#include "PluginProcessor.hpp"
#include "ui/HeaderPanel.hpp"
#include "ui/LSFonts.hpp"
#include "ui/LSLookAndFeel.hpp"
#include "ui/LSPartGrid.hpp"
#include "ui/SettingsPanel.hpp"

#include <juce_audio_processors/juce_audio_processors.h>

namespace tsplug {

class PluginEditor final : public juce::AudioProcessorEditor,
                           private juce::DarkModeSettingListener {
public:
    explicit PluginEditor(PluginProcessor& processor);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void darkModeSettingChanged() override;

    PluginProcessor& processor_;
    ui::SharedFonts fonts_;
    ui::LSLookAndFeel lookAndFeel_;
    ui::HeaderPanel header_;
    ui::SettingsPanel settings_;
    ui::LSPartGrid parts_;
    juce::TooltipWindow tooltips_{this, 700};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace tsplug
