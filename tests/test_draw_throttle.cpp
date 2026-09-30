// The load loop's progress pump: one poll per body, a draw only when the
// throttle allows, and the fraction-0 draw always allowed because it is the
// cancel-latch reset. Driven with a fake clock: no window, no GL. The
// production loop (Application::rebuildMeshes) calls the same pumpStep.
#include "app/DrawThrottle.h"
#include <gtest/gtest.h>
#include <vector>

using materializr::DrawThrottle;
using materializr::progressFrameWouldDraw;
using materializr::pumpStep;
using Clock = DrawThrottle::clock;
using Ms = std::chrono::milliseconds;

namespace {

Clock::time_point at(long long millis) { return Clock::time_point{} + Ms(millis); }

// Models the loop: `cancelled` stands in for m_progressCancelled (a
// fraction-0 draw clears it, as renderProgressFrame does), `frameOpen` for
// m_imguiFrameOpen; the guard is the production predicate itself, not a
// copy. Time advances by the poll's cost, the draw's cost, and then the
// body's.
struct Sim {
    DrawThrottle throttle;
    Clock::time_point now = at(0);
    int polls = 0, draws = 0;
    int pollMs = 0;
    std::vector<float> fractions;
    std::vector<Clock::time_point> drawStarts;
    bool cancelled = false;
    bool frameOpen = false;
    bool cancelOnFirstDraw = false;
    bool frameOpenOnFirstDraw = false;

    void body(float frac, int bodyMs, int drawMs) {
        const bool canDraw = progressFrameWouldDraw(frameOpen, true, cancelled, frac);
        pumpStep(throttle, canDraw,
                 [&] { return now; },
                 [&] {
                     if (frac == 0.0f) cancelled = false;
                     fractions.push_back(frac);
                     drawStarts.push_back(now);
                     if (++draws == 1 && cancelOnFirstDraw) cancelled = true;
                     if (draws == 1 && frameOpenOnFirstDraw) frameOpen = true;
                     now += Ms(drawMs);
                     return now;
                 },
                 [&] { ++polls; now += Ms(pollMs); });
        now += Ms(bodyMs);
    }
    void run(int n, int bodyMs = 2, int drawMs = 17) {
        for (int i = 0; i < n; ++i) body(float(i) / float(n), bodyMs, drawMs);
    }
};

} // namespace

// The keep-alive's rule - 200 ms floor, four times the draw's own cost.
TEST(DrawThrottle, FirstDrawIsDueAtOnceThenFloorOrFourTimesCost) {
    DrawThrottle t;
    EXPECT_TRUE(t.due(at(0)));
    t.drew(at(0), at(0));
    EXPECT_FALSE(t.due(at(199)));
    EXPECT_TRUE(t.due(at(200)));
    t.drew(at(200), at(300));   // a 100 ms draw: next at 300 + max(200, 400)
    EXPECT_FALSE(t.due(at(699)));
    EXPECT_TRUE(t.due(at(700)));
}

// 145 bodies at 2 ms with 17 ms draws yield exactly two frames.
TEST(DrawThrottle, OneHundredFortyFiveBodiesDrawTwice) {
    Sim s;
    s.run(145);
    EXPECT_EQ(s.draws, 2);
    EXPECT_EQ(s.polls, 145);
    // Pinned, not just counted: the first frame at 0, the second at the first
    // body that lands past 17 + 200 ms, and 145 x 2 ms + 2 x 17 ms in total.
    ASSERT_EQ(s.drawStarts.size(), 2u);
    EXPECT_EQ(s.drawStarts[0], at(0));
    EXPECT_EQ(s.drawStarts[1], at(217));
    EXPECT_EQ(s.now, at(324));
}

// The other half of the rule, through the pump: a 100 ms frame backs off
// 4 x 100 = 400 ms, not the 200 ms floor, so 145 two-millisecond bodies fit
// inside it and only the first frame is drawn. A floor-only throttle draws
// twice here.
TEST(PumpStep, ExpensiveFrameBacksOffFourTimesItsCost) {
    Sim s;
    s.run(145, 2, 100);
    EXPECT_EQ(s.draws, 1);
    EXPECT_EQ(s.throttle.next, at(100 + 400));
    EXPECT_EQ(s.polls, 145);
}

// A slow event drain is not the draw's cost: the back-off after a 17 ms
// frame is the 200 ms floor even when the poll before it took 100 ms.
TEST(PumpStep, PollCostIsNotChargedToTheDraw) {
    Sim s;
    s.pollMs = 100;
    s.body(0.0f, 2, 17);
    ASSERT_EQ(s.draws, 1);
    EXPECT_EQ(s.drawStarts[0], at(100));
    EXPECT_EQ(s.throttle.next, at(117 + 200));
}

// The indeterminate fraction never resets the latch: with Cancel latched, a
// -1 step polls and does not draw.
TEST(PumpStep, IndeterminateFractionDoesNotResetTheLatch) {
    Sim s;
    s.cancelled = true;
    s.body(-1.0f, 2, 17);
    EXPECT_EQ(s.polls, 1);
    EXPECT_EQ(s.draws, 0);
    EXPECT_TRUE(s.cancelled);
}

// The poll never depends on the draw. A Cancel latched during the first
// frame stops the draws, keeps every poll, and leaves the throttle where the
// last real draw put it.
TEST(PumpStep, PollsEveryBodyAfterCancelStopsTheDraws) {
    Sim s;
    s.cancelOnFirstDraw = true;
    s.body(0.0f, 2, 17);
    ASSERT_EQ(s.draws, 1);
    const Clock::time_point nextAfterRealDraw = s.throttle.next;
    for (int i = 1; i < 145; ++i) s.body(float(i) / 145.0f, 2, 17);
    EXPECT_EQ(s.polls, 145);
    EXPECT_EQ(s.draws, 1);
    EXPECT_EQ(s.throttle.next, nextAfterRealDraw);
}

// A Cancel latched BEFORE the first body does not suppress the load's
// frames: the fraction-0 draw is selected anyway, it resets the latch, and
// the later draws proceed as normal.
TEST(PumpStep, CancelLatchedBeforeFirstBodyIsResetByTheFractionZeroDraw) {
    Sim s;
    s.cancelled = true;
    s.run(145);
    EXPECT_FALSE(s.cancelled);
    EXPECT_EQ(s.draws, 2);
    EXPECT_EQ(s.polls, 145);
}

// An open ImGui frame refuses every draw; polling continues regardless.
TEST(PumpStep, OpenFrameRefusesDrawsButNotPolls) {
    Sim s;
    s.frameOpen = true;
    s.run(145);
    EXPECT_EQ(s.draws, 0);
    EXPECT_EQ(s.polls, 145);
    EXPECT_EQ(s.throttle.next, Clock::time_point{});
}

// A frame that opens after the first draw: the draws stop, the polls do not,
// and the throttle stays where the one real draw left it. Production cannot
// reach this state today (the flag flips only in beginFrame/endFrame and the
// rebuild is synchronous); it pins the pump's contract in case that changes.
TEST(PumpStep, FrameOpeningAfterFirstDrawStopsDrawsNotPolls) {
    Sim s;
    s.frameOpenOnFirstDraw = true;
    s.body(0.0f, 2, 17);
    ASSERT_EQ(s.draws, 1);
    const Clock::time_point nextAfterRealDraw = s.throttle.next;
    for (int i = 1; i < 145; ++i) s.body(float(i) / 145.0f, 2, 17);
    EXPECT_EQ(s.polls, 145);
    EXPECT_EQ(s.draws, 1);
    EXPECT_EQ(s.throttle.next, nextAfterRealDraw);
    // An open frame beats the fraction-0 exception, which a latched Cancel
    // never can (fraction 0 clears it): still no draw.
    s.body(0.0f, 2, 17);
    EXPECT_EQ(s.draws, 1);
    EXPECT_EQ(s.polls, 146);
}

// Without a window, the shared predicate never selects a draw - fraction 0
// included - matching Application::progressFrameWouldDraw's own no-window
// case (the loop simply never calls the reporter, so the latch outlives it;
// documented at Application.cpp, not exercised here since it needs no
// Application to reach).
TEST(ProgressFrameWouldDraw, NoWindowNeverDraws) {
    EXPECT_FALSE(progressFrameWouldDraw(false, false, false, 0.0f));
    EXPECT_FALSE(progressFrameWouldDraw(false, false, false, 0.5f));
    EXPECT_FALSE(progressFrameWouldDraw(false, false, true, 0.0f));
}

// The first frame is at fraction 0 (the reset); every later one is
// positive and never goes backwards.
TEST(PumpStep, FractionSequenceStartsAtZeroThenClimbs) {
    Sim s;
    s.run(145);
    ASSERT_GE(s.fractions.size(), 2u);
    EXPECT_EQ(s.fractions[0], 0.0f);
    for (size_t i = 1; i < s.fractions.size(); ++i) {
        EXPECT_GT(s.fractions[i], 0.0f);
        EXPECT_GE(s.fractions[i], s.fractions[i - 1]);
    }
}
