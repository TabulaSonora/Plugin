#pragma once

#include "tabulasonora/output_filter.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace tsplug {

/// 32 kHz to the host's rate by four-point cubic interpolation, the Catmull-Rom form.
///
/// The engine's output is band-limited well inside 16 kHz, so going up to 44.1 or 48 kHz asks
/// nothing of the kernel beyond not adding anything of its own. A host running *below* 32 kHz is
/// the case this does not cover: the fold-back would need a decimating filter, and no host does it.
///
/// Ported from the Apple player's AUv3, including the four-frame carry that fixed a click. The
/// carried frames have to be the *last* frames the engine produced or the stream has a hole in it,
/// and with only the three the interpolator reads behind there are blocks where that cannot be
/// arranged: when a block ends without the read position crossing an input frame boundary, the
/// last frame the interpolator needed is one further on than the last frame that can be carried
/// while keeping the next read position at or above 1. The fourth frame reconciles the two.
class CatmullRomResampler {
public:
    static constexpr std::size_t history = 4;

    /// Sizes the input buffer for a ratio and the largest block the host promised, and resets.
    /// The one place this allocates.
    void prepare(double ratio, std::size_t maxFrames)
    {
        ratio_ = ratio;
        // The worst case for one block: the read position starts just short of 2, walks
        // `maxFrames` steps of `ratio`, and the interpolator wants a frame past the end of that.
        const double span = 2.0 + (static_cast<double>(maxFrames) * ratio_);
        const std::size_t capacity = static_cast<std::size_t>(span) + history + 4;
        left_.assign(capacity, 0.0F);
        right_.assign(capacity, 0.0F);
        reset();
    }

    void reset() noexcept
    {
        std::fill(left_.begin(), left_.end(), 0.0F);
        std::fill(right_.begin(), right_.end(), 0.0F);
        position_ = 1.0;
    }

    /// Fills `frames` output frames, pulling engine frames through `pull(left, right, count)`.
    /// Returns false, having written nothing, if the host asked for more than it declared as its
    /// maximum -- growing the buffer here is the one thing an audio thread must not do.
    template <typename Pull>
    [[nodiscard]] bool process(Pull&& pull, float* left, float* right, std::size_t frames) noexcept
    {
        if (frames == 0) {
            return true;
        }

        const double last = position_ + (static_cast<double>(frames - 1) * ratio_);
        const double end = position_ + (static_cast<double>(frames) * ratio_);

        // Two demands on the input, and the larger wins: the last output frame interpolates over
        // `floor(last) - 1` to `floor(last) + 2`, so the buffer has to reach the last of those; and
        // the next call must start reading at a position of at least 1 once the tail has been
        // shifted to the front, which bounds how far ahead this call may pull.
        const std::size_t required = std::max(static_cast<std::size_t>(last) + 3,
                                              static_cast<std::size_t>(end) + history - 1);
        if (required > left_.size()) {
            return false;
        }

        pull(left_.data() + history, right_.data() + history, required - history);

        for (std::size_t frame = 0; frame < frames; ++frame) {
            const double read = position_ + (static_cast<double>(frame) * ratio_);
            const auto index = static_cast<std::size_t>(read);
            const auto fraction = static_cast<float>(read - static_cast<double>(index));
            left[frame] = interpolate(left_.data() + index - 1, fraction);
            right[frame] = interpolate(right_.data() + index - 1, fraction);
        }

        // Carry the last frames the engine produced to the front, and bring the read position back
        // with them, so the buffer never has to be longer than one block. `required - history` and
        // nothing else: whatever is not carried here is never seen again.
        const std::size_t shift = required - history;
        std::copy_n(left_.begin() + static_cast<std::ptrdiff_t>(shift), history, left_.begin());
        std::copy_n(right_.begin() + static_cast<std::ptrdiff_t>(shift), history, right_.begin());
        position_ = end - static_cast<double>(shift);
        return true;
    }

private:
    static float interpolate(const float* frames, float fraction) noexcept
    {
        const float y0 = frames[0];
        const float y1 = frames[1];
        const float y2 = frames[2];
        const float y3 = frames[3];
        const float c1 = 0.5F * (y2 - y0);
        const float c2 = y0 - (2.5F * y1) + (2.0F * y2) - (0.5F * y3);
        const float c3 = (0.5F * (y3 - y0)) + (1.5F * (y1 - y2));
        return (((((c3 * fraction) + c2) * fraction) + c1) * fraction) + y1;
    }

    /// Engine frames consumed per output frame.
    double ratio_ = 1.0;

    /// Where in the input buffer the next output frame is read from, in [1, 2]. At least one,
    /// because the interpolator reads from `index - 1`.
    double position_ = 1.0;

    std::vector<float> left_;
    std::vector<float> right_;
};

/// The module's own output stage doing the conversion: one `ts::OutputFilter` run at the ratio
/// between the engine's rate and the host's, pulling a 32 kHz frame whenever its phase says it
/// needs one. This is the arrangement the original plugin has. It exists to be compared against
/// the module rather than to be the fast one, and it costs a call into the engine per frame.
class ModuleResampler {
public:
    void prepare(int hostRate) noexcept
    {
        filter_.set_host_rate(hostRate);
        reset();
    }

    void reset() noexcept
    {
        filter_.reset();
        primed_ = false;
    }

    /// `pullOne(left, right)` renders exactly one engine frame.
    template <typename PullOne>
    void process(PullOne&& pullOne, float* left, float* right, std::size_t frames) noexcept
    {
        for (std::size_t frame = 0; frame < frames; ++frame) {
            if (!primed_) {
                // It interpolates between the last two inputs, so it needs one before the first
                // output frame can mean anything.
                push(pullOne);
                primed_ = true;
            }
            const auto [outLeft, outRight] = filter_.at();
            left[frame] = outLeft;
            right[frame] = outRight;
            for (int wanted = filter_.advance(); wanted > 0; --wanted) {
                push(pullOne);
            }
        }
    }

private:
    template <typename PullOne>
    void push(PullOne& pullOne) noexcept
    {
        float l = 0.0F;
        float r = 0.0F;
        pullOne(l, r);
        filter_.push(l, r);
    }

    ts::OutputFilter filter_;
    bool primed_ = false;
};

} // namespace tsplug
