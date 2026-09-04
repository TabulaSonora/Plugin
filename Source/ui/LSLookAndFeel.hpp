#pragma once

#include "LSFonts.hpp"
#include "Tokens.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace tsplug::ui {

/// The LoSnoCo language over JUCE's stock widgets: flat fills, hairline borders, one typeface,
/// teal focus rings, and nothing that gradients or shadows.
class LSLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    explicit LSLookAndFeel(LSFonts& fonts);

    void setDark(bool dark);
    [[nodiscard]] bool isDark() const noexcept { return dark_; }
    [[nodiscard]] const LSPalette& palette() const noexcept { return *palette_; }
    [[nodiscard]] LSFonts& fonts() noexcept { return fonts_; }

    /// Marks a button as the view's one primary (orange) action.
    static void setPrimary(juce::Button& button, bool primary);

    // Buttons
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
    void drawTickBox(juce::Graphics&, juce::Component&, float x, float y, float w, float h,
                     bool ticked, bool isEnabled, bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

    // Combo boxes and menus
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown, int buttonX,
                      int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackgroundWithOptions(juce::Graphics&, int width, int height,
                                            const juce::PopupMenu::Options&) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator,
                           bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override;
    void getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                   int standardMenuItemHeight, int& idealWidth,
                                   int& idealHeight) override;
    int getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options&) override;

    // Sliders
    int getSliderThumbRadius(juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                          float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle,
                          juce::Slider&) override;

    // Labels and focus
    juce::Font getLabelFont(juce::Label&) override;
    std::unique_ptr<juce::FocusOutline> createFocusOutlineForComponent(juce::Component&) override;

private:
    void applyPalette();

    LSFonts& fonts_;
    const LSPalette* palette_ = &lightPalette;
    bool dark_ = false;
};

} // namespace tsplug::ui
