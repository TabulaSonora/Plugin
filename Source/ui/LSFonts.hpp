#pragma once

#include <juce_graphics/juce_graphics.h>

#include <map>

namespace tsplug::ui {

/// Inter LoSnoCo, from the binary data, once per process.
///
/// One variable file covers every sans weight: JUCE 9 clones a typeface with a `wght` axis value,
/// so 400, 500 and 700 are the same bytes configured three ways rather than three embedded files.
class LSFonts {
public:
    LSFonts();

    [[nodiscard]] juce::Typeface::Ptr typeface() const noexcept { return base_; }

    /// A font at a pixel size and a weight on the 100-900 axis.
    [[nodiscard]] juce::Font font(float pixels, float weight = 400.0F);

    [[nodiscard]] juce::Font body() { return font(15.0F); }
    [[nodiscard]] juce::Font bodyMedium() { return font(15.0F, 500.0F); }
    [[nodiscard]] juce::Font technical() { return font(13.0F); }
    [[nodiscard]] juce::Font subheading() { return font(17.0F, 500.0F); }

    /// The label register: 11 px, 700, uppercase (the caller's job), +0.2 em tracking.
    [[nodiscard]] juce::Font label();

private:
    [[nodiscard]] juce::Typeface::Ptr weighted(float weight);

    juce::Typeface::Ptr base_;
    std::map<int, juce::Typeface::Ptr> weights_;
};

using SharedFonts = juce::SharedResourcePointer<LSFonts>;

} // namespace tsplug::ui
