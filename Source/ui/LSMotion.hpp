#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace tsplug::ui {

/// Whether the interface may animate. The brand's motion layer is short and functional, and it
/// is suppressed when the person has asked their platform for less motion or has said so in the
/// plugin's own settings.
class LSMotion {
public:
    [[nodiscard]] static bool enabled();

    /// Fades a component in or out over one of the token durations, or snaps it when motion is
    /// off. Safe to call repeatedly; a running fade on the same component is replaced.
    static void fade(juce::Component& component, bool visible, int durationMs);

private:
    [[nodiscard]] static bool platformReducesMotion();
};

} // namespace tsplug::ui
