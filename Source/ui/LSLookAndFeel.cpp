#include "LSLookAndFeel.hpp"

namespace tsplug::ui {

namespace {

constexpr const char* primaryProperty = "ls-primary";

bool isPrimary(const juce::Component& component)
{
    return static_cast<bool>(component.getProperties()[primaryProperty]);
}

juce::Path tickPath(juce::Rectangle<float> box)
{
    // A two-stroke tick inset in the box, drawn as a stroke rather than a filled glyph.
    juce::Path path;
    const auto inner = box.reduced(box.getWidth() * 0.22F);
    path.startNewSubPath(inner.getX(), inner.getCentreY() + inner.getHeight() * 0.05F);
    path.lineTo(inner.getX() + inner.getWidth() * 0.38F, inner.getBottom());
    path.lineTo(inner.getRight(), inner.getY());
    return path;
}

juce::Path chevronPath(juce::Rectangle<float> area)
{
    juce::Path path;
    const auto inner = area.withSizeKeepingCentre(8.0F, 5.0F);
    path.startNewSubPath(inner.getX(), inner.getY());
    path.lineTo(inner.getCentreX(), inner.getBottom());
    path.lineTo(inner.getRight(), inner.getY());
    return path;
}

} // namespace

LSLookAndFeel::LSLookAndFeel(LSFonts& fonts)
    : fonts_{fonts}
{
    if (auto typeface = fonts_.typeface()) {
        setDefaultSansSerifTypeface(typeface);
    }
    applyPalette();
}

void LSLookAndFeel::setDark(bool dark)
{
    dark_ = dark;
    palette_ = dark ? &darkPalette : &lightPalette;
    applyPalette();
}

void LSLookAndFeel::setPrimary(juce::Button& button, bool primary)
{
    button.getProperties().set(primaryProperty, primary);
    button.repaint();
}

void LSLookAndFeel::applyPalette()
{
    const LSPalette& p = *palette_;

    setColour(juce::ResizableWindow::backgroundColourId, p.bg);
    setColour(juce::DocumentWindow::textColourId, p.text);

    setColour(juce::TextButton::buttonColourId, p.surface);
    setColour(juce::TextButton::buttonOnColourId, p.surface2);
    setColour(juce::TextButton::textColourOffId, p.text);
    setColour(juce::TextButton::textColourOnId, p.text);

    setColour(juce::ToggleButton::textColourId, p.text);
    setColour(juce::ToggleButton::tickColourId, p.interactive);
    setColour(juce::ToggleButton::tickDisabledColourId, p.muted2);

    setColour(juce::ComboBox::backgroundColourId, p.surface);
    setColour(juce::ComboBox::textColourId, p.text);
    setColour(juce::ComboBox::outlineColourId, p.border);
    setColour(juce::ComboBox::buttonColourId, p.surface);
    setColour(juce::ComboBox::arrowColourId, p.muted);
    setColour(juce::ComboBox::focusedOutlineColourId, p.interactive);

    setColour(juce::PopupMenu::backgroundColourId, p.surface);
    setColour(juce::PopupMenu::textColourId, p.text);
    setColour(juce::PopupMenu::headerTextColourId, p.muted);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, p.surface2);
    setColour(juce::PopupMenu::highlightedTextColourId, p.text);

    setColour(juce::Slider::backgroundColourId, p.border);
    setColour(juce::Slider::trackColourId, p.interactive);
    setColour(juce::Slider::thumbColourId, p.interactive);
    setColour(juce::Slider::textBoxTextColourId, p.text);
    setColour(juce::Slider::textBoxBackgroundColourId, p.surface);
    setColour(juce::Slider::textBoxOutlineColourId, p.border);

    setColour(juce::Label::textColourId, p.text);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour(juce::TooltipWindow::backgroundColourId, p.surface);
    setColour(juce::TooltipWindow::textColourId, p.text);
    setColour(juce::TooltipWindow::outlineColourId, p.border);

    setColour(juce::ScrollBar::thumbColourId, p.borderStrong);
    setColour(juce::TextEditor::backgroundColourId, p.surface);
    setColour(juce::TextEditor::textColourId, p.text);
    setColour(juce::TextEditor::outlineColourId, p.border);
    setColour(juce::TextEditor::focusedOutlineColourId, p.interactive);
    setColour(juce::TextEditor::highlightColourId, p.interactive.withAlpha(0.3F));
    setColour(juce::CaretComponent::caretColourId, p.interactive);

    setColour(juce::AlertWindow::backgroundColourId, p.bg);
    setColour(juce::AlertWindow::textColourId, p.text);
    setColour(juce::AlertWindow::outlineColourId, p.border);
}

// -- Buttons -------------------------------------------------------------------------------------

juce::Font LSLookAndFeel::getTextButtonFont(juce::TextButton&, int)
{
    return fonts_.font(14.0F, 500.0F);
}

void LSLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                         bool highlighted, bool down)
{
    const LSPalette& p = *palette_;
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5F);
    const bool primary = isPrimary(button);

    juce::Colour fill;
    if (primary) {
        fill = down ? p.accent.darker(0.18F) : highlighted ? p.accent.darker(0.08F) : p.accent;
    } else {
        fill = down ? p.surface3 : highlighted ? p.surface2 : p.surface;
    }
    if (!button.isEnabled()) {
        fill = fill.withAlpha(0.4F);
    }

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, radius::button);
    if (!primary) {
        g.setColour(highlighted ? p.borderStrong : p.border);
        g.drawRoundedRectangle(bounds, radius::button, 1.0F);
    }
}

void LSLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool highlighted,
                                     bool down)
{
    const LSPalette& p = *palette_;
    const float box = 16.0F;
    const auto bounds = button.getLocalBounds().toFloat();
    const bool compact = button.getButtonText().isEmpty();

    const auto boxArea = compact ? bounds.withSizeKeepingCentre(box, box)
                                 : juce::Rectangle<float>{0.0F, (bounds.getHeight() - box) * 0.5F,
                                                          box, box};
    drawTickBox(g, button, boxArea.getX(), boxArea.getY(), box, box, button.getToggleState(),
                button.isEnabled(), highlighted, down);

    if (!compact) {
        g.setColour(button.isEnabled() ? p.text : p.muted);
        g.setFont(fonts_.font(14.0F));
        const auto textArea = bounds.withTrimmedLeft(box + space::sm);
        g.drawText(button.getButtonText(), textArea, juce::Justification::centredLeft, true);
    }
}

void LSLookAndFeel::drawTickBox(juce::Graphics& g, juce::Component&, float x, float y, float w,
                                float h, bool ticked, bool enabled, bool highlighted, bool down)
{
    const LSPalette& p = *palette_;
    const auto box = juce::Rectangle<float>{x, y, w, h}.reduced(0.5F);

    juce::Colour fill = ticked ? p.interactive : (down ? p.surface3 : highlighted ? p.surface2 : p.surface);
    juce::Colour border = ticked ? p.interactive : (highlighted ? p.borderStrong : p.border);
    if (!enabled) {
        fill = fill.withAlpha(0.4F);
        border = border.withAlpha(0.4F);
    }
    g.setColour(fill);
    g.fillRoundedRectangle(box, radius::control);
    g.setColour(border);
    g.drawRoundedRectangle(box, radius::control, 1.0F);

    if (ticked) {
        // The tick in the page colour, so it reads on teal in both schemes.
        g.setColour(dark_ ? p.bg : juce::Colours::white);
        g.strokePath(tickPath(box), juce::PathStrokeType{2.0F, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded});
    }
}

// -- Combo boxes and menus -----------------------------------------------------------------------

void LSLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int, int,
                                 int, int, juce::ComboBox& box)
{
    const LSPalette& p = *palette_;
    const auto bounds = juce::Rectangle<int>{0, 0, width, height}.toFloat().reduced(0.5F);
    const bool hover = box.isMouseOver(true) || box.isPopupActive();

    g.setColour(isButtonDown ? p.surface3 : hover ? p.surface2 : p.surface);
    g.fillRoundedRectangle(bounds, radius::input);
    g.setColour(hover ? p.borderStrong : p.border);
    g.drawRoundedRectangle(bounds, radius::input, 1.0F);

    g.setColour(p.muted);
    g.strokePath(chevronPath(bounds.withLeft(bounds.getRight() - 28.0F)),
                 juce::PathStrokeType{1.5F, juce::PathStrokeType::curved, juce::PathStrokeType::rounded});
}

juce::Font LSLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return fonts_.font(14.0F);
}

void LSLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(space::sm, 0, box.getWidth() - 28 - space::sm, box.getHeight());
    label.setFont(getComboBoxFont(box));
}

void LSLookAndFeel::drawPopupMenuBackgroundWithOptions(juce::Graphics& g, int width, int height,
                                                       const juce::PopupMenu::Options&)
{
    const LSPalette& p = *palette_;
    const auto bounds = juce::Rectangle<int>{0, 0, width, height}.toFloat().reduced(0.5F);
    g.setColour(p.surface);
    g.fillRoundedRectangle(bounds, radius::button);
    g.setColour(p.borderStrong);
    g.drawRoundedRectangle(bounds, radius::button, 1.0F);
}

void LSLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                      bool isSeparator, bool isActive, bool isHighlighted,
                                      bool isTicked, bool hasSubMenu, const juce::String& text,
                                      const juce::String& shortcutKeyText, const juce::Drawable*,
                                      const juce::Colour* textColour)
{
    const LSPalette& p = *palette_;

    if (isSeparator) {
        g.setColour(p.border);
        g.fillRect(area.reduced(space::sm, 0).withHeight(1).withY(area.getCentreY()));
        return;
    }

    auto row = area.reduced(space::xs, 1);
    if (isHighlighted && isActive) {
        g.setColour(p.surface2);
        g.fillRoundedRectangle(row.toFloat(), radius::control);
    }

    auto textArea = row.reduced(space::sm + 2, 0);
    const auto tickArea = textArea.removeFromLeft(16);
    if (isTicked) {
        g.setColour(p.interactive);
        g.fillEllipse(tickArea.toFloat().withSizeKeepingCentre(6.0F, 6.0F));
    }
    textArea.removeFromLeft(space::xs);

    juce::Colour colour = textColour != nullptr ? *textColour : p.text;
    if (!isActive) {
        colour = p.muted;
    }
    g.setColour(colour);
    g.setFont(getPopupMenuFont());
    g.drawText(text, textArea, juce::Justification::centredLeft, true);

    if (shortcutKeyText.isNotEmpty()) {
        g.setColour(p.muted);
        g.setFont(fonts_.technical());
        g.drawText(shortcutKeyText, textArea, juce::Justification::centredRight, true);
    }
    if (hasSubMenu) {
        g.setColour(p.muted);
        auto arrow = chevronPath(textArea.removeFromRight(16).toFloat());
        arrow.applyTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi,
                                                             arrow.getBounds().getCentreX(),
                                                             arrow.getBounds().getCentreY()));
        g.strokePath(arrow, juce::PathStrokeType{1.5F});
    }
}

juce::Font LSLookAndFeel::getPopupMenuFont()
{
    return fonts_.font(14.0F);
}

void LSLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator, int,
                                              int& idealWidth, int& idealHeight)
{
    if (isSeparator) {
        idealWidth = 40;
        idealHeight = space::sm + 1;
        return;
    }
    const auto font = getPopupMenuFont();
    idealHeight = 28;
    idealWidth = juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, text)) + 56;
}

int LSLookAndFeel::getPopupMenuBorderSizeWithOptions(const juce::PopupMenu::Options&)
{
    return space::xs;
}

// -- Sliders -------------------------------------------------------------------------------------

int LSLookAndFeel::getSliderThumbRadius(juce::Slider&)
{
    return 6;
}

void LSLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                     float sliderPos, float minSliderPos, float maxSliderPos,
                                     juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal) {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos,
                                               maxSliderPos, style, slider);
        return;
    }

    const LSPalette& p = *palette_;
    const auto radius = static_cast<float>(getSliderThumbRadius(slider));
    const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5F;
    const float left = static_cast<float>(x) + radius;
    const float right = static_cast<float>(x + width) - radius;
    const float pos = juce::jlimit(left, right, sliderPos);

    g.setColour(p.border);
    g.fillRoundedRectangle(left, centreY - 1.0F, right - left, 2.0F, 1.0F);
    g.setColour(p.interactive);
    g.fillRoundedRectangle(left, centreY - 1.0F, pos - left, 2.0F, 1.0F);

    const auto thumb = juce::Rectangle<float>{pos - radius, centreY - radius, radius * 2.0F,
                                              radius * 2.0F};
    g.setColour(p.interactive);
    g.fillEllipse(thumb);
    if (slider.isMouseOverOrDragging()) {
        g.setColour(dark_ ? p.bg : juce::Colours::white);
        g.drawEllipse(thumb.reduced(2.0F), 2.0F);
    }
}

// -- Labels and focus ----------------------------------------------------------------------------

juce::Font LSLookAndFeel::getLabelFont(juce::Label& label)
{
    const auto requested = label.getFont();
    return fonts_.font(requested.getHeight() > 0.0F ? juce::jmin(requested.getHeight(), 15.0F) : 15.0F);
}

std::unique_ptr<juce::FocusOutline>
LSLookAndFeel::createFocusOutlineForComponent(juce::Component&)
{
    struct Properties final : public juce::FocusOutline::OutlineWindowProperties {
        explicit Properties(juce::Colour c)
            : colour{c}
        {
        }

        juce::Rectangle<int> getOutlineBounds(juce::Component& focused) override
        {
            return focused.getScreenBounds().expanded(3);
        }

        void drawOutline(juce::Graphics& g, int width, int height) override
        {
            // A 2 px ring in the interactive colour, offset from the control. Never optional:
            // keyboard navigation is not an afterthought.
            g.setColour(colour);
            g.drawRoundedRectangle(juce::Rectangle<int>{0, 0, width, height}.toFloat().reduced(1.5F),
                                   radius::button + 2.0F, 2.0F);
        }

        juce::Colour colour;
    };

    return std::make_unique<juce::FocusOutline>(std::make_unique<Properties>(palette_->interactive));
}

} // namespace tsplug::ui
