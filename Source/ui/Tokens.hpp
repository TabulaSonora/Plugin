#pragma once

#include <juce_graphics/juce_graphics.h>

namespace tsplug::ui {

/// The LoSnoCo tokens this UI uses, as one palette per colour scheme. Values come from the design
/// system's tokens.json; nothing here is chosen by eye, and nothing below reaches for a hex.
struct LSPalette {
    juce::Colour bg;
    juce::Colour surface;
    juce::Colour surface2;
    juce::Colour surface3;
    juce::Colour border;
    juce::Colour borderStrong;
    juce::Colour muted;
    juce::Colour muted2;
    juce::Colour text;
    /// Orange. At most one primary action per view.
    juce::Colour accent;
    /// Teal-light on light surfaces, teal on dark: links, focus, active state.
    juce::Colour interactive;
    /// Purple: selected state, tags.
    juce::Colour selected;
    juce::Colour error;
};

inline const LSPalette lightPalette{
    juce::Colour{0xFFF4F3F0}, juce::Colour{0xFFECEAE5}, juce::Colour{0xFFE4E2DC},
    juce::Colour{0xFFD8D5CE}, juce::Colour{0xFFD8D5CE}, juce::Colour{0xFFB8B5AE},
    juce::Colour{0xFF888880}, juce::Colour{0xFFADABA5}, juce::Colour{0xFF1A1917},
    juce::Colour{0xFFF25E0D}, juce::Colour{0xFF00967F}, juce::Colour{0xFF7B41B9},
    juce::Colour{0xFFC0392B}};

inline const LSPalette darkPalette{
    juce::Colour{0xFF111C1A}, juce::Colour{0xFF182926}, juce::Colour{0xFF1E332F},
    juce::Colour{0xFF243D38}, juce::Colour{0xFF243D38}, juce::Colour{0xFF2F524B},
    juce::Colour{0xFF7A9E98}, juce::Colour{0xFF567873}, juce::Colour{0xFFE5EFED},
    juce::Colour{0xFFF25E0D}, juce::Colour{0xFF0DF2C9}, juce::Colour{0xFF7B41B9},
    juce::Colour{0xFFC0392B}};

namespace radius {
inline constexpr float container = 10.0F;
inline constexpr float card = 8.0F;
inline constexpr float button = 6.0F;
inline constexpr float input = 6.0F;
inline constexpr float control = 4.0F;
} // namespace radius

namespace space {
inline constexpr int xs = 4;
inline constexpr int sm = 8;
inline constexpr int md = 15;
inline constexpr int lg = 24;
inline constexpr int xl = 38;
inline constexpr int xxl = 60;
} // namespace space

namespace type {
/// Base size is 15 px; labels are 11 px, uppercase, weight 700, +0.2 em tracking.
inline constexpr float body = 15.0F;
inline constexpr float technical = 13.0F;
inline constexpr float label = 11.0F;
inline constexpr float subheading = 17.0F;
inline constexpr float labelTracking = 0.2F;
} // namespace type

namespace motion {
inline constexpr int fastMs = 100;
inline constexpr int quickMs = 160;
inline constexpr int mediumMs = 240;
inline constexpr int slowMs = 420;
} // namespace motion

} // namespace tsplug::ui
