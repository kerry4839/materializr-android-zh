#pragma once
#include <cstdio>
#include <string>

namespace materializr {

// Compact number → string for UI text (history captions, etc.). std::to_string
// on a double always prints 6 decimals ("2.000000", "12.500002"); "%g" drops
// the trailing-zero noise (2, 12.5) while keeping real precision when present.
inline std::string numStr(double v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%g", v);
    return b;
}

} // namespace materializr
