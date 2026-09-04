#include "LSMotion.hpp"

#include "plugin/UserSettings.hpp"

namespace tsplug::ui {

bool LSMotion::enabled()
{
    SharedUserSettings settings;
    return !settings->reduceMotion() && !platformReducesMotion();
}

void LSMotion::fade(juce::Component& component, bool visible, int durationMs)
{
    auto& animator = juce::Desktop::getInstance().getAnimator();
    if (!enabled()) {
        animator.cancelAnimation(&component, false);
        component.setAlpha(visible ? 1.0F : 0.0F);
        component.setVisible(visible);
        return;
    }
    if (visible) {
        animator.fadeIn(&component, durationMs);
    } else {
        animator.fadeOut(&component, durationMs);
    }
}

#if !JUCE_MAC
bool LSMotion::platformReducesMotion()
{
    return false;
}
#endif

} // namespace tsplug::ui
