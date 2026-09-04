#pragma once

#include "LSComponents.hpp"
#include "engine/Snapshot.hpp"
#include "plugin/PluginProcessor.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstdint>
#include <unordered_map>

namespace tsplug::ui {

/// One row per part: receive channel, the instrument sounding, activity, mute and solo.
///
/// A hairline grid: the background is the surface, and one-pixel lines in the border colour
/// separate the rows, so cells carry no borders of their own. Names are resolved on the message
/// thread from the numbers the audio thread publishes, and cached by their key, so after the first
/// few blocks the timer never takes the engine's lock.
class LSPartGrid final : public juce::Component, private juce::Timer {
public:
    explicit LSPartGrid(PluginProcessor& processor);
    ~LSPartGrid() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int headerHeight = 26;
    static constexpr int rowHeight = 26;
    static constexpr int channelWidth = 44;
    static constexpr int dotWidth = 28;
    static constexpr int toggleWidth = 34;

    struct Row {
        juce::String name;
        int rxChannel = -1;
        bool drums = false;
        float activity = 0.0F;
        LSToggle mute;
        LSToggle solo;
    };

    void timerCallback() override;
    void refresh(const EngineSnapshot& snapshot);
    [[nodiscard]] juce::String resolveName(const PartSnapshot& part);
    [[nodiscard]] juce::Rectangle<int> rowBounds(int index) const;

    PluginProcessor& processor_;
    std::array<Row, EngineSettings::parts> rows_;
    EngineSnapshot snapshot_;
    bool hasRom_ = false;
    std::unordered_map<std::uint64_t, juce::String> names_;
};

} // namespace tsplug::ui
