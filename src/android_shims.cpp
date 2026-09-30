// Mobile (Android + iOS) stub for the libcurl update checker (UpdateChecker), which
// is excluded from the Android build. FileDialogs is now built directly (its
// in-app ImGui browser works on Android), so it is no longer stubbed here.
// Entirely compiled out on desktop.
#include "platform_defs.h"
#if defined(MZ_MOBILE)

#include "ui/UpdateChecker.h"

#include <cctype>
#include <string>
#include <vector>

namespace materializr {

// ── UpdateChecker (no network check on mobile; keep version compare) ──────────
UpdateChecker::Result UpdateChecker::check(const std::string& /*owner*/,
                                           const std::string& /*repo*/,
                                           bool /*includePrereleases*/) {
    Result r;
    r.ok = false;
    r.current = MATERIALIZR_VERSION;
    r.errorMessage = "Update checks are disabled on this platform.";
    return r;
}

int UpdateChecker::compareVersions(const std::string& a, const std::string& b) {
    auto parse = [](const std::string& s) {
        std::vector<int> parts;
        int cur = 0; bool any = false;
        for (char c : s) {
            if (std::isdigit(static_cast<unsigned char>(c))) { cur = cur * 10 + (c - '0'); any = true; }
            else if (c == '.') { parts.push_back(any ? cur : 0); cur = 0; any = false; }
            // ignore a leading 'v' and any other non-digit separators
        }
        parts.push_back(any ? cur : 0);
        return parts;
    };
    std::vector<int> va = parse(a), vb = parse(b);
    std::size_t n = va.size() > vb.size() ? va.size() : vb.size();
    for (std::size_t i = 0; i < n; ++i) {
        int x = i < va.size() ? va[i] : 0;
        int y = i < vb.size() ? vb[i] : 0;
        if (x < y) return -1;
        if (x > y) return 1;
    }
    return 0;
}

} // namespace materializr

#endif // MZ_MOBILE
