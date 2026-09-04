#include "HeaderPanel.hpp"

#include "LSMotion.hpp"

namespace tsplug::ui {

HeaderPanel::HeaderPanel(PluginProcessor& processor)
    : processor_{processor}
{
    addAndMakeVisible(dot_);
    addAndMakeVisible(title_);
    addAndMakeVisible(status_);
    addAndMakeVisible(meter_);
    addChildComponent(xg_);
    addChildComponent(load_);
    addChildComponent(change_);

    status_.setColourRole(&LSPalette::muted);
    xg_.setColourRole(&LSPalette::interactive);
    xg_.setJustification(juce::Justification::centred);

    load_.setPrimary(true);
    load_.onClick = [this] { processor_.chooseRom(); };
    change_.onClick = [this] { processor_.chooseRom(); };

    processor_.romLoader().broadcaster.addChangeListener(this);
    refreshStatus();
    startTimerHz(10);
}

HeaderPanel::~HeaderPanel()
{
    stopTimer();
    processor_.romLoader().broadcaster.removeChangeListener(this);
}

void HeaderPanel::resized()
{
    auto area = getLocalBounds();

    dot_.setBounds(area.removeFromLeft(16));
    area.removeFromLeft(space::xs);

    // Right side first: the action, then the meter, then whatever room is left is the status.
    const auto buttonWidth = load_.isVisible() ? 150 : 88;
    auto action = area.removeFromRight(buttonWidth).withSizeKeepingCentre(buttonWidth, 30);
    load_.setBounds(action);
    change_.setBounds(action);
    area.removeFromRight(space::md);

    meter_.setBounds(area.removeFromRight(180).reduced(0, space::sm));
    area.removeFromRight(space::md);

    xg_.setBounds(area.removeFromRight(30));
    area.removeFromRight(space::xs);

    title_.setBounds(area.removeFromTop(area.getHeight() / 2));
    status_.setBounds(area);
}

void HeaderPanel::paint(juce::Graphics&)
{
}

void HeaderPanel::changeListenerCallback(juce::ChangeBroadcaster*)
{
    refreshStatus();
}

void HeaderPanel::timerCallback()
{
    auto& instrument = processor_.instrument();
    meter_.setValue(instrument.activeVoices(), instrument.voiceSlots());
    const bool xg = instrument.hasRom() && instrument.xgMode();
    if (xg_.isVisible() != xg) {
        xg_.setVisible(xg);
        LSMotion::fade(xg_, xg, motion::quickMs);
    }
}

void HeaderPanel::refreshStatus()
{
    const auto status = processor_.romLoader().status();
    using State = RomLoader::State;

    juce::String text;
    LSStatusDot::Tone tone = LSStatusDot::Tone::idle;
    bool showLoad = false;

    switch (status.state) {
    case State::none:
        text = status.message.isNotEmpty() ? status.message : juce::String{"No ROM. Point this at SCCore.dll from a Sound Canvas VA install."};
        showLoad = true;
        break;
    case State::loading:
        text = "Reading the ROM...";
        tone = LSStatusDot::Tone::accent;
        break;
    case State::failed:
        text = status.message;
        tone = LSStatusDot::Tone::error;
        showLoad = true;
        break;
    case State::ready:
        text = status.file.getFileName() + juce::String::fromUTF8(" \xC2\xB7 ") + status.buildId;
        if (status.message.isNotEmpty() && status.message != "chosen") {
            text += juce::String::fromUTF8(" \xC2\xB7 ") + status.message;
        }
        tone = LSStatusDot::Tone::active;
        break;
    }

    dot_.setTone(tone);
    dot_.setLevel(1.0F);
    status_.setText(text);
    status_.setColourRole(status.state == State::failed ? &LSPalette::error : &LSPalette::muted);
    // The two actions trade places with a short fade; the layout is settled first so a button
    // fading in already sits where it belongs.
    const bool showChange = status.state == State::ready;
    if (load_.isVisible() != showLoad) {
        load_.setVisible(showLoad);
        resized();
        LSMotion::fade(load_, showLoad, motion::quickMs);
    }
    if (change_.isVisible() != showChange) {
        change_.setVisible(showChange);
        resized();
        LSMotion::fade(change_, showChange, motion::quickMs);
    }
    resized();
}

} // namespace tsplug::ui
