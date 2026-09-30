#pragma once

#include <imgui.h>

// Global UI scale used to size fixed-pixel windows/widgets for HiDPI/touch.
// 1.0 on desktop (Application sets it from Window::uiScale at startup), so
// uiSz()/uiW() are identity there and desktop layout is unchanged. On a tablet
// it matches the font scale, so dialogs that hard-code a width in pixels grow
// proportionally instead of clipping their contents.
//
// Header-only: the inline function-local static gives a single shared instance
// across translation units without a separate .cpp / CMake entry.
namespace materializr {

inline float& uiScaleRef() { static float s = 1.0f; return s; }
inline float  uiScale() { return uiScaleRef(); }
inline void   setUiScale(float s) { uiScaleRef() = (s > 0.0f ? s : 1.0f); }

inline float  uiW(float w) { return w * uiScale(); }
inline ImVec2 uiSz(float w, float h) { return ImVec2(w * uiScale(), h * uiScale()); }

} // namespace materializr
