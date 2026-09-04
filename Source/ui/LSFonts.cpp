#include "LSFonts.hpp"

#include "TSPlugData.h"

#include <array>
#include <cmath>

namespace tsplug::ui {

LSFonts::LSFonts()
    : base_{juce::Typeface::createSystemTypefaceFor(TSPlugData::InterVariableLoSnoCo_ttf,
                                                    TSPlugData::InterVariableLoSnoCo_ttfSize)}
{
}

juce::Typeface::Ptr LSFonts::weighted(float weight)
{
    if (base_ == nullptr) {
        return nullptr;
    }
    const int key = static_cast<int>(std::lround(weight));
    if (const auto found = weights_.find(key); found != weights_.end()) {
        return found->second;
    }
    const std::array settings{juce::FontVariableSetting{"wght", weight}};
    auto clone = base_->cloneWithVariableSettings(settings);
    if (clone == nullptr) {
        clone = base_;
    }
    weights_[key] = clone;
    return clone;
}

juce::Font LSFonts::font(float pixels, float weight)
{
    auto options = juce::FontOptions{}.withPointHeight(pixels);
    if (auto typeface = weighted(weight)) {
        options = options.withTypeface(typeface).withPointHeight(pixels);
    }
    return juce::Font{options};
}

juce::Font LSFonts::label()
{
    return font(11.0F, 700.0F).withExtraKerningFactor(0.2F);
}

} // namespace tsplug::ui
