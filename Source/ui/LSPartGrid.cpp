#include "LSPartGrid.hpp"

#include "tabulasonora/drum_kit_table.hpp"
#include "tabulasonora/note_renderer.hpp"
#include "tabulasonora/patch_directory.hpp"
#include "tabulasonora/tone.hpp"

namespace tsplug::ui {

namespace {

std::uint64_t nameKey(const PartSnapshot& part)
{
    const auto field = [](int value, int shift) {
        return static_cast<std::uint64_t>(static_cast<std::uint16_t>(value)) << shift;
    };
    return (part.drums ? 1ULL : 0ULL) | field(part.kit, 1) | field(part.program, 17)
           | field(part.map, 33) | field(part.lookupBank, 49);
}

juce::String trimmed(const std::string& text)
{
    return juce::String{text}.trim();
}

} // namespace

LSPartGrid::LSPartGrid(PluginProcessor& processor)
    : processor_{processor}
{
    for (int index = 0; index < EngineSettings::parts; ++index) {
        Row& row = rows_[static_cast<std::size_t>(index)];
        row.mute.setTooltip("Mute part " + juce::String{index + 1});
        row.solo.setTooltip("Solo part " + juce::String{index + 1});
        row.mute.onClick = [this, index] {
            processor_.setPartMuted(index, rows_[static_cast<std::size_t>(index)].mute.getToggleState());
        };
        row.solo.onClick = [this, index] {
            processor_.setPartSoloed(index, rows_[static_cast<std::size_t>(index)].solo.getToggleState());
        };
        addAndMakeVisible(row.mute);
        addAndMakeVisible(row.solo);
    }
    startTimerHz(10);
}

LSPartGrid::~LSPartGrid()
{
    stopTimer();
}

juce::Rectangle<int> LSPartGrid::rowBounds(int index) const
{
    return {0, headerHeight + index * rowHeight, getWidth(), rowHeight};
}

void LSPartGrid::resized()
{
    for (int index = 0; index < EngineSettings::parts; ++index) {
        Row& row = rows_[static_cast<std::size_t>(index)];
        auto bounds = rowBounds(index);
        row.solo.setBounds(bounds.removeFromRight(toggleWidth));
        row.mute.setBounds(bounds.removeFromRight(toggleWidth));
    }
}

void LSPartGrid::paint(juce::Graphics& g)
{
    const LSPalette& p = paletteFor(*this);
    auto* fonts = fontsFor(*this);
    const auto bounds = getLocalBounds();

    g.setColour(p.surface);
    g.fillRoundedRectangle(bounds.toFloat(), radius::card);

    // Header.
    auto header = juce::Rectangle<int>{0, 0, getWidth(), headerHeight};
    if (fonts != nullptr) {
        g.setFont(fonts->label());
    }
    g.setColour(p.muted);
    header.removeFromRight(toggleWidth * 2);
    {
        auto h = header;
        g.drawText("CH", h.removeFromLeft(channelWidth).reduced(space::sm, 0),
                   juce::Justification::centredLeft, false);
        h.removeFromRight(dotWidth);
        g.drawText("INSTRUMENT", h.reduced(space::sm, 0), juce::Justification::centredLeft, false);
    }
    g.drawText("M", juce::Rectangle<int>{getWidth() - toggleWidth * 2, 0, toggleWidth, headerHeight},
               juce::Justification::centred, false);
    g.drawText("S", juce::Rectangle<int>{getWidth() - toggleWidth, 0, toggleWidth, headerHeight},
               juce::Justification::centred, false);

    // Rows.
    for (int index = 0; index < EngineSettings::parts; ++index) {
        const Row& row = rows_[static_cast<std::size_t>(index)];
        auto area = rowBounds(index);

        if (row.solo.getToggleState()) {
            g.setColour(p.selected.withAlpha(0.12F));
            g.fillRect(area);
        }

        // The hairline above each row, in the border colour: the grid, with no cell borders.
        g.setColour(p.border);
        g.fillRect(area.getX(), area.getY(), area.getWidth(), 1);

        area.removeFromRight(toggleWidth * 2);
        const auto dotArea = area.removeFromRight(dotWidth);
        const auto channelArea = area.removeFromLeft(channelWidth).reduced(space::sm, 0);
        const auto nameArea = area.reduced(space::sm, 0);

        const bool muted = row.mute.getToggleState();
        if (fonts != nullptr) {
            g.setFont(fonts->technical());
        }
        g.setColour(p.muted);
        const juce::String channel = row.rxChannel < 0 ? juce::String{index + 1}
                                     : row.rxChannel >= 16 ? juce::String{"-"}
                                                           : juce::String{row.rxChannel + 1};
        g.drawText(channel, channelArea, juce::Justification::centredLeft, false);

        if (fonts != nullptr) {
            g.setFont(fonts->font(14.0F));
        }
        g.setColour(muted ? p.muted : p.text);
        g.drawText(hasRom_ ? row.name : juce::String{}, nameArea, juce::Justification::centredLeft, true);

        // Activity: grey when silent, the interactive colour while a voice sounds, decaying.
        g.setColour(p.muted2.interpolatedWith(p.interactive, row.activity));
        g.fillEllipse(dotArea.toFloat().withSizeKeepingCentre(7.0F, 7.0F));
    }

    g.setColour(p.border);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5F), radius::card, 1.0F);
}

void LSPartGrid::timerCallback()
{
    bool changed = false;
    EngineSnapshot fresh;
    if (processor_.instrument().readSnapshot(fresh)) {
        snapshot_ = fresh;
        changed = true;
    }
    refresh(snapshot_);

    // Mute and solo may be set from a restored session rather than a click, so the boxes follow
    // the mask rather than the other way round.
    for (int index = 0; index < EngineSettings::parts; ++index) {
        Row& row = rows_[static_cast<std::size_t>(index)];
        const bool muted = processor_.isPartMuted(index);
        const bool soloed = processor_.isPartSoloed(index);
        if (row.mute.getToggleState() != muted) {
            row.mute.setToggleState(muted, juce::dontSendNotification);
            changed = true;
        }
        if (row.solo.getToggleState() != soloed) {
            row.solo.setToggleState(soloed, juce::dontSendNotification);
            changed = true;
        }
    }

    if (changed) {
        repaint();
    }
}

void LSPartGrid::refresh(const EngineSnapshot& snapshot)
{
    const bool hadRom = hasRom_;
    hasRom_ = snapshot.hasRom;
    if (hasRom_ != hadRom) {
        names_.clear();
        repaint();
    }

    bool changed = false;
    for (int index = 0; index < EngineSettings::parts; ++index) {
        Row& row = rows_[static_cast<std::size_t>(index)];
        const PartSnapshot& part = snapshot.parts[static_cast<std::size_t>(index)];

        const juce::String name = hasRom_ ? resolveName(part) : juce::String{};
        if (row.name != name || row.rxChannel != part.rxChannel || row.drums != part.drums) {
            row.name = name;
            row.rxChannel = part.rxChannel;
            row.drums = part.drums;
            changed = true;
        }

        // The dot decays over about 160 ms so a single short note is visible.
        const float target = part.voices > 0 ? 1.0F : 0.0F;
        const float next = target > row.activity ? target : juce::jmax(0.0F, row.activity - 0.6F);
        if (!juce::approximatelyEqual(next, row.activity)) {
            row.activity = next;
            changed = true;
        }
    }
    if (changed) {
        repaint();
    }
}

juce::String LSPartGrid::resolveName(const PartSnapshot& part)
{
    const auto key = nameKey(part);
    if (const auto found = names_.find(key); found != names_.end()) {
        return found->second;
    }

    juce::String name;
    processor_.instrument().withNotes([&](const ts::NoteRenderer& notes) {
        if (part.drums) {
            name = trimmed(notes.drums().kit_name(part.kit));
            if (name.isEmpty() && part.kit >= 0) {
                name = "Kit " + juce::String{part.kit};
            }
        } else {
            const auto& directory = notes.directory();
            const int tone = directory.program_to_tone(part.program, static_cast<ts::ToneMap>(part.map),
                                                       part.lookupBank);
            if (tone >= 0) {
                if (const auto record = directory.tone(tone); record && record->is_defined()) {
                    name = trimmed(record->name());
                }
            }
        }
    });
    names_[key] = name;
    return name;
}

} // namespace tsplug::ui
