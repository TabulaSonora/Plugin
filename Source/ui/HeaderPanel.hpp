#pragma once

#include "LSComponents.hpp"
#include "plugin/PluginProcessor.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace tsplug::ui {

/// The strip across the top: which ROM is loaded (or why none is), the voice meter, and the one
/// primary action in the whole view -- loading a ROM -- shown only while there is none.
class HeaderPanel final : public juce::Component,
                          private juce::ChangeListener,
                          private juce::Timer {
public:
    explicit HeaderPanel(PluginProcessor& processor);
    ~HeaderPanel() override;

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refreshStatus();

    PluginProcessor& processor_;
    LSStatusDot dot_;
    LSText title_{"Tabula Sonora", LSText::Style::subheading};
    LSText status_{{}, LSText::Style::technical};
    LSText xg_{"XG", LSText::Style::technical};
    LSMeter meter_;
    LSButton load_{"Load SCCore.dll"};
    LSButton change_{"Change"};
};

} // namespace tsplug::ui
