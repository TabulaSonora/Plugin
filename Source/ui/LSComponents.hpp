#pragma once

#include "LSFonts.hpp"
#include "Tokens.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace tsplug::ui {

/// Finds the LoSnoCo look-and-feel a component is drawn with; null under any other.
class LSLookAndFeel;
[[nodiscard]] const LSPalette& paletteFor(const juce::Component& component);
[[nodiscard]] LSFonts* fontsFor(const juce::Component& component);

/// A text button; `setPrimary(true)` makes it the view's one orange action.
class LSButton final : public juce::TextButton {
public:
    explicit LSButton(const juce::String& text = {});
    void setPrimary(bool primary);
};

class LSToggle final : public juce::ToggleButton {
public:
    explicit LSToggle(const juce::String& text = {});
};

class LSChoice final : public juce::ComboBox {
public:
    LSChoice();
};

/// The label register: uppercase, 11 px, 700, tracked. Not a `juce::Label`, which would want to
/// be editable and bordered.
class LSSectionLabel final : public juce::Component {
public:
    explicit LSSectionLabel(const juce::String& text = {});
    void setText(const juce::String& text);
    void paint(juce::Graphics&) override;

private:
    juce::String text_;
};

/// Body or technical text, read-only.
class LSText final : public juce::Component {
public:
    enum class Style { body, bodyMedium, technical, subheading };

    explicit LSText(const juce::String& text = {}, Style style = Style::body);
    void setText(const juce::String& text);
    void setStyle(Style style);
    void setColourRole(juce::Colour LSPalette::*role);
    void setJustification(juce::Justification justification);
    void paint(juce::Graphics&) override;

private:
    juce::String text_;
    Style style_;
    juce::Colour LSPalette::*role_ = &LSPalette::text;
    juce::Justification justification_ = juce::Justification::centredLeft;
};

/// An 8 px status dot. `setLevel` is a 0-1 activity that the caller decays.
class LSStatusDot final : public juce::Component {
public:
    enum class Tone { idle, active, error, accent };

    void setTone(Tone tone);
    void setLevel(float level);
    void paint(juce::Graphics&) override;

private:
    Tone tone_ = Tone::idle;
    float level_ = 1.0F;
};

/// Voices in use against the pool, as a hairline bar and a `n / cap` figure.
class LSMeter final : public juce::Component {
public:
    void setValue(int value, int capacity);
    void paint(juce::Graphics&) override;

private:
    int value_ = 0;
    int capacity_ = 0;
};

} // namespace tsplug::ui
