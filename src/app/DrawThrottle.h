#pragma once
// DrawThrottle - paces the progress frames a long main-thread loop draws
// between its units of work.
//
// A progress frame ends in swapBuffers, and with vsync on that blocks until
// the next refresh: 17 ms at 60 Hz. Drawing one per unit therefore turns a
// 2 ms body into a 17 ms one - the 145-body load measured 2453 ms, of which
// about 300 ms was tessellation. The rule here is the one the heavy-op
// keep-alive in Application.cpp already applies: the first draw is due at
// once; after a draw that cost `cost`, the next is due at
// after + max(200 ms, 4 x cost), so a cheap frame animates smoothly and an
// expensive one can never eat more than a fifth of the wall clock.
//
// Header-only and free of Application/GL/SDL, so the loop's decision is
// tested with a fake clock. The keep-alive still carries its own copy of the
// rule: switching it over is a separate change with its own measurement, and
// this one is scoped to the load loop.
#include <algorithm>
#include <chrono>

namespace materializr {

struct DrawThrottle {
    using clock = std::chrono::steady_clock;

    // Epoch: the first draw is due immediately.
    clock::time_point next{};

    bool due(clock::time_point now) const { return now >= next; }

    // Back off from the draw's OWN cost, as measured by the caller.
    void drew(clock::time_point before, clock::time_point after) {
        next = after + std::max(std::chrono::duration_cast<clock::duration>(
                                    std::chrono::milliseconds(200)),
                                (after - before) * 4);
    }
};

// The reporter's refusal rule, kept here so the loop and the tests share one
// definition: no frame while one is open or without a window, and none once
// Cancel is latched - except at fraction 0, which is the reset itself.
inline bool progressFrameWouldDraw(bool frameOpen, bool hasWindow, bool cancelled,
                                   float fraction)
{
    return !frameOpen && hasWindow && (fraction == 0.0f || !cancelled);
}

// One step of a pumped loop. Poll FIRST and unconditionally - that is what
// keeps the window answering the compositor, and it touches no GL - then draw
// only when the throttle allows AND the reporter would actually draw
// (`canDraw`: Application::renderProgressFrame returns without polling or
// drawing while an ImGui frame is open or the cancel latch is set). Selecting
// a draw the reporter refuses would skip the poll and count a frame that
// never happened. `now` is read AFTER the poll, so a slow event drain is not
// charged to the draw's cost (the keep-alive samples after pollEvents too);
// `draw` returns the time the frame finished. Both come from the caller's
// clock, a fake one in the tests. Returns whether a frame was drawn.
template <class Now, class Draw, class Poll>
bool pumpStep(DrawThrottle& throttle, bool canDraw, Now&& now, Draw&& draw,
              Poll&& poll)
{
    poll();
    const DrawThrottle::clock::time_point before = now();
    if (!canDraw || !throttle.due(before)) return false;
    const DrawThrottle::clock::time_point after = draw();
    throttle.drew(before, after);
    return true;
}

} // namespace materializr
