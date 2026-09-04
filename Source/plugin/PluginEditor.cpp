#include "PluginEditor.hpp"

#include "ui/Tokens.hpp"

namespace tsplug {

PluginEditor::PluginEditor(PluginProcessor& processor)
    : AudioProcessorEditor{&processor}
    , processor_{processor}
    , lookAndFeel_{*fonts_}
    , header_{processor}
    , settings_{processor}
    , parts_{processor}
{
    lookAndFeel_.setDark(juce::Desktop::getInstance().isDarkModeActive());
    setLookAndFeel(&lookAndFeel_);
    juce::Desktop::getInstance().addDarkModeSettingListener(this);

    addAndMakeVisible(header_);
    addAndMakeVisible(settings_);
    addAndMakeVisible(parts_);

    setSize(760, 540);
}

PluginEditor::~PluginEditor()
{
    juce::Desktop::getInstance().removeDarkModeSettingListener(this);
    setLookAndFeel(nullptr);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(lookAndFeel_.palette().bg);
}

void PluginEditor::resized()
{
    using namespace ui;
    auto area = getLocalBounds().reduced(space::md);
    header_.setBounds(area.removeFromTop(44));
    area.removeFromTop(space::md);
    settings_.setBounds(area.removeFromLeft(250));
    area.removeFromLeft(space::md);
    parts_.setBounds(area);
}

void PluginEditor::darkModeSettingChanged()
{
    lookAndFeel_.setDark(juce::Desktop::getInstance().isDarkModeActive());
    sendLookAndFeelChange();
    repaint();
}

} // namespace tsplug
