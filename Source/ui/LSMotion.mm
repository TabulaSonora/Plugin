#include "LSMotion.hpp"

#import <AppKit/AppKit.h>

namespace tsplug::ui {

bool LSMotion::platformReducesMotion()
{
    return [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];
}

} // namespace tsplug::ui
