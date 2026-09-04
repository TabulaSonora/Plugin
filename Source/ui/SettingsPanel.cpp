#include "SettingsPanel.hpp"

#include "plugin/Parameters.hpp"

namespace tsplug::ui {

namespace {

void fillChoices(juce::ComboBox& box, const juce::AudioProcessorValueTreeState& state, const char* id)
{
    if (const auto* choice = dynamic_cast<const juce::AudioParameterChoice*>(state.getParameter(id))) {
        box.addItemList(choice->choices, 1);
    }
}

} // namespace

SettingsPanel::SettingsPanel(PluginProcessor& processor)
    : processor_{processor}
{
    auto& state = processor_.parameters();

    addAndMakeVisible(vintageLabel_);
    addAndMakeVisible(vintage_);
    fillChoices(vintage_, state, params::toneMap);
    vintageAttachment_ = std::make_unique<juce::ComboBoxParameterAttachment>(
        *state.getParameter(params::toneMap), vintage_);

    addAndMakeVisible(polyphonyLabel_);
    addAndMakeVisible(polyphony_);
    fillChoices(polyphony_, state, params::polyphony);
    polyphonyAttachment_ = std::make_unique<juce::ComboBoxParameterAttachment>(
        *state.getParameter(params::polyphony), polyphony_);

    addAndMakeVisible(effectsLabel_);
    addToggle(params::reverb, "Reverb");
    addToggle(params::chorus, "Chorus");
    addToggle(params::delay, "Delay");
    addToggle(params::insertionEffects, "Insertion");

    addAndMakeVisible(resamplingLabel_);
    addToggle(params::extendedResampler, "Extended interpolation");
    addToggle(params::extendedOutputResampler, "Extended output resampler");

    addAndMakeVisible(sysexLabel_);
    addToggle(params::deliverDroppedSysEx, "Deliver dropped SysEx");

    addAndMakeVisible(gainLabel_);
    addAndMakeVisible(gain_);
    addAndMakeVisible(gainText_);
    gain_.setSliderStyle(juce::Slider::LinearHorizontal);
    gain_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gain_.setHasFocusOutline(true);
    gain_.onValueChange = [this] { updateGainText(); };
    gainAttachment_ = std::make_unique<juce::SliderParameterAttachment>(
        *state.getParameter(params::gain), gain_);
    gainText_.setColourRole(&LSPalette::muted);
    gainText_.setJustification(juce::Justification::centredRight);
    updateGainText();
}

void SettingsPanel::addToggle(const char* parameterId, const juce::String& text)
{
    ToggleRow row;
    row.toggle = std::make_unique<LSToggle>(text);
    addAndMakeVisible(*row.toggle);
    row.attachment = std::make_unique<juce::ButtonParameterAttachment>(
        *processor_.parameters().getParameter(parameterId), *row.toggle);
    toggles_.push_back(std::move(row));
}

void SettingsPanel::updateGainText()
{
    gainText_.setText(juce::String::fromUTF8("\xC3\x97") + juce::String{gain_.getValue(), 2});
}

void SettingsPanel::resized()
{
    auto area = getLocalBounds().reduced(space::md);
    constexpr int labelHeight = 18;
    constexpr int controlHeight = 30;
    constexpr int toggleHeight = 26;

    const auto section = [&area, labelHeight](juce::Component& label) {
        label.setBounds(area.removeFromTop(labelHeight));
        area.removeFromTop(space::xs);
    };

    section(vintageLabel_);
    vintage_.setBounds(area.removeFromTop(controlHeight));
    area.removeFromTop(space::md);

    section(polyphonyLabel_);
    polyphony_.setBounds(area.removeFromTop(controlHeight));
    area.removeFromTop(space::md);

    // Effects: two columns of two.
    section(effectsLabel_);
    {
        auto row = area.removeFromTop(toggleHeight);
        const int half = row.getWidth() / 2;
        toggles_[0].toggle->setBounds(row.removeFromLeft(half));
        toggles_[1].toggle->setBounds(row);
        row = area.removeFromTop(toggleHeight);
        toggles_[2].toggle->setBounds(row.removeFromLeft(half));
        toggles_[3].toggle->setBounds(row);
    }
    area.removeFromTop(space::md);

    section(resamplingLabel_);
    toggles_[4].toggle->setBounds(area.removeFromTop(toggleHeight));
    toggles_[5].toggle->setBounds(area.removeFromTop(toggleHeight));
    area.removeFromTop(space::md);

    section(sysexLabel_);
    toggles_[6].toggle->setBounds(area.removeFromTop(toggleHeight));
    area.removeFromTop(space::md);

    {
        auto labelRow = area.removeFromTop(labelHeight);
        gainText_.setBounds(labelRow.removeFromRight(60));
        gainLabel_.setBounds(labelRow);
    }
    area.removeFromTop(space::xs);
    gain_.setBounds(area.removeFromTop(controlHeight));
}

void SettingsPanel::paint(juce::Graphics& g)
{
    const LSPalette& p = paletteFor(*this);
    g.setColour(p.surface);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), radius::card);
    g.setColour(p.border);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5F), radius::card, 1.0F);
}

} // namespace tsplug::ui
