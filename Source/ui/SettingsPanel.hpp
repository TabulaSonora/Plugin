#pragma once

#include "LSComponents.hpp"
#include "plugin/PluginProcessor.hpp"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace tsplug::ui {

/// The engine's settings, one control per parameter, each attached to the parameter tree so the
/// host, automation and this panel agree.
class SettingsPanel final : public juce::Component {
public:
    explicit SettingsPanel(PluginProcessor& processor);

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    struct ToggleRow {
        std::unique_ptr<LSToggle> toggle;
        std::unique_ptr<juce::ButtonParameterAttachment> attachment;
    };

    void addToggle(const char* parameterId, const juce::String& text);
    void updateGainText();

    PluginProcessor& processor_;

    LSSectionLabel vintageLabel_{"Vintage"};
    LSChoice vintage_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> vintageAttachment_;

    LSSectionLabel polyphonyLabel_{"Polyphony"};
    LSChoice polyphony_;
    std::unique_ptr<juce::ComboBoxParameterAttachment> polyphonyAttachment_;

    LSSectionLabel effectsLabel_{"Effects"};
    LSSectionLabel resamplingLabel_{"Past the module"};
    LSSectionLabel sysexLabel_{"System exclusive"};
    std::vector<ToggleRow> toggles_;

    LSSectionLabel gainLabel_{"Output gain"};
    juce::Slider gain_;
    std::unique_ptr<juce::SliderParameterAttachment> gainAttachment_;
    LSText gainText_{{}, LSText::Style::technical};
};

} // namespace tsplug::ui
