#include "LSComponents.hpp"

#include "LSLookAndFeel.hpp"

namespace tsplug::ui {

const LSPalette& paletteFor(const juce::Component& component)
{
    if (const auto* laf = dynamic_cast<const LSLookAndFeel*>(&component.getLookAndFeel())) {
        return laf->palette();
    }
    return lightPalette;
}

LSFonts* fontsFor(const juce::Component& component)
{
    if (auto* laf = dynamic_cast<LSLookAndFeel*>(&const_cast<juce::Component&>(component).getLookAndFeel())) {
        return &laf->fonts();
    }
    return nullptr;
}

// -- LSButton ------------------------------------------------------------------------------------

LSButton::LSButton(const juce::String& text)
    : juce::TextButton{text}
{
    setHasFocusOutline(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void LSButton::setPrimary(bool primary)
{
    LSLookAndFeel::setPrimary(*this, primary);
    if (primary) {
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    } else {
        removeColour(juce::TextButton::textColourOffId);
        removeColour(juce::TextButton::textColourOnId);
    }
}

// -- LSToggle ------------------------------------------------------------------------------------

LSToggle::LSToggle(const juce::String& text)
    : juce::ToggleButton{text}
{
    setHasFocusOutline(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

// -- LSChoice ------------------------------------------------------------------------------------

LSChoice::LSChoice()
{
    setHasFocusOutline(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

// -- LSSectionLabel ------------------------------------------------------------------------------

LSSectionLabel::LSSectionLabel(const juce::String& text)
    : text_{text.toUpperCase()}
{
    setInterceptsMouseClicks(false, false);
}

void LSSectionLabel::setText(const juce::String& text)
{
    text_ = text.toUpperCase();
    repaint();
}

void LSSectionLabel::paint(juce::Graphics& g)
{
    const LSPalette& p = paletteFor(*this);
    g.setColour(p.muted);
    if (auto* fonts = fontsFor(*this)) {
        g.setFont(fonts->label());
    }
    g.drawText(text_, getLocalBounds(), juce::Justification::centredLeft, false);
}

// -- LSText --------------------------------------------------------------------------------------

LSText::LSText(const juce::String& text, Style style)
    : text_{text}
    , style_{style}
{
    setInterceptsMouseClicks(false, false);
}

void LSText::setText(const juce::String& text)
{
    if (text_ != text) {
        text_ = text;
        repaint();
    }
}

void LSText::setStyle(Style style)
{
    style_ = style;
    repaint();
}

void LSText::setColourRole(juce::Colour LSPalette::*role)
{
    role_ = role;
    repaint();
}

void LSText::setJustification(juce::Justification justification)
{
    justification_ = justification;
    repaint();
}

void LSText::paint(juce::Graphics& g)
{
    const LSPalette& p = paletteFor(*this);
    g.setColour(p.*role_);
    if (auto* fonts = fontsFor(*this)) {
        switch (style_) {
        case Style::body: g.setFont(fonts->body()); break;
        case Style::bodyMedium: g.setFont(fonts->bodyMedium()); break;
        case Style::technical: g.setFont(fonts->technical()); break;
        case Style::subheading: g.setFont(fonts->subheading()); break;
        }
    }
    g.drawText(text_, getLocalBounds(), justification_, true);
}

// -- LSStatusDot ---------------------------------------------------------------------------------

void LSStatusDot::setTone(Tone tone)
{
    if (tone_ != tone) {
        tone_ = tone;
        repaint();
    }
}

void LSStatusDot::setLevel(float level)
{
    level = juce::jlimit(0.0F, 1.0F, level);
    if (!juce::approximatelyEqual(level_, level)) {
        level_ = level;
        repaint();
    }
}

void LSStatusDot::paint(juce::Graphics& g)
{
    const LSPalette& p = paletteFor(*this);
    const auto dot = getLocalBounds().toFloat().withSizeKeepingCentre(8.0F, 8.0F);

    juce::Colour colour;
    switch (tone_) {
    case Tone::idle: colour = p.muted2; break;
    case Tone::active: colour = p.interactive; break;
    case Tone::error: colour = p.error; break;
    case Tone::accent: colour = p.accent; break;
    }

    // Activity blends from the idle grey to the live colour rather than blinking on and off.
    if (tone_ == Tone::active) {
        colour = p.muted2.interpolatedWith(p.interactive, level_);
    }
    g.setColour(colour);
    g.fillEllipse(dot);
}

// -- LSMeter -------------------------------------------------------------------------------------

void LSMeter::setValue(int value, int capacity)
{
    if (value_ != value || capacity_ != capacity) {
        value_ = value;
        capacity_ = capacity;
        repaint();
    }
}

void LSMeter::paint(juce::Graphics& g)
{
    const LSPalette& p = paletteFor(*this);
    auto area = getLocalBounds();

    const juce::String figure = capacity_ > 0 ? juce::String{value_} + " / " + juce::String{capacity_}
                                              : juce::String{"-"};
    const auto textArea = area.removeFromRight(64);
    g.setColour(p.muted);
    if (auto* fonts = fontsFor(*this)) {
        g.setFont(fonts->technical());
    }
    g.drawText(figure, textArea, juce::Justification::centredRight, false);

    area.removeFromRight(space::sm);
    const auto bar = area.toFloat().withSizeKeepingCentre(static_cast<float>(area.getWidth()), 4.0F);
    g.setColour(p.border);
    g.fillRoundedRectangle(bar, 2.0F);
    if (capacity_ > 0 && value_ > 0) {
        const float fraction = juce::jlimit(0.0F, 1.0F,
                                            static_cast<float>(value_) / static_cast<float>(capacity_));
        g.setColour(fraction >= 0.98F ? p.accent : p.interactive);
        g.fillRoundedRectangle(bar.withWidth(juce::jmax(4.0F, bar.getWidth() * fraction)), 2.0F);
    }
}

} // namespace tsplug::ui
