#include "UiKeepAlive.h"

#include <chrono>
#include <thread>

namespace materializr {

namespace {

std::function<void()> g_keepAlive;
std::thread::id g_mainThread{};
std::chrono::steady_clock::time_point g_lastPump{};

// Slower than a frame, far faster than any desktop's unresponsive timeout
// (~5 s on GNOME and KDE). Cheap enough to sit in an OCCT progress callback
// that fires thousands of times a second.
constexpr auto kMinInterval = std::chrono::milliseconds(100);

} // namespace

void setUiKeepAlive(std::function<void()> fn) {
    // Called on the main thread, before any OCCT worker exists. The recorded
    // id is what lets uiKeepAlive() bail out on a worker without touching
    // g_keepAlive at all -- workers only ever READ g_mainThread.
    g_mainThread = std::this_thread::get_id();
    g_keepAlive = std::move(fn);
    g_lastPump = {};   // the first call after arming always goes through
}

void uiKeepAlive() {
    if (std::this_thread::get_id() != g_mainThread) return;
    if (!g_keepAlive) return;
    const auto now = std::chrono::steady_clock::now();
    if (g_lastPump.time_since_epoch().count() != 0 && now - g_lastPump < kMinInterval)
        return;
    g_lastPump = now;
    g_keepAlive();
}

} // namespace materializr
