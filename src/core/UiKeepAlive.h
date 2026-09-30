#pragma once

#include <functional>

namespace materializr {

// UI keep-alive for work that holds the main thread for seconds at a time.
//
// An OCCT boolean on contact geometry takes seconds on a slow CPU -- and every
// one of them RE-RUNS during a project load's history replay. Measured on a
// 4-core i7-6650U opening robot dog cover-rear.mzr: 52 steps, 24.4 s, of which
// four extrude booleans were ~6 s each, all inside ONE main-loop iteration.
// Nothing touched the event queue for that whole stretch, so the compositor
// pinged the window, got no answer, and put up "Application is not responding"
// over most of the load. The work itself is honest; being unable to answer a
// ping while doing it is not.
//
// The hook is installed by Application ONLY while it runs a heavy task in the
// between-frames deferred slot, and cleared afterwards. That is what makes it
// safe for modeling code to call from deep inside an OCCT progress callback:
// the same ops also run mid-frame during a live drag preview, and there the
// hook is null, so this is a no-op and no nested ImGui frame is possible.
void setUiKeepAlive(std::function<void()> fn);

// Call from a long op's progress / interrupt callback. Rate-limited internally
// and ignored off the main thread (OCCT runs its booleans with RunParallel and
// hands progress sub-ranges to workers), so it is safe and cheap to call on
// every progress tick.
void uiKeepAlive();

} // namespace materializr
