#pragma once
#include <glm/glm.hpp>

namespace materializr {

// User-facing axis convention (3D-printer style: X = left/right,
// Y = forward/back, Z = up) mapped onto the Y-up world. Shared by the
// Application's toolbar radios and the interactive-op controllers.
inline glm::vec3 userAxisToWorldVec(int userIdx) {
    switch (userIdx) {
        case 0: return glm::vec3(1.0f, 0.0f, 0.0f); // user X → world X
        case 1: return glm::vec3(0.0f, 0.0f, 1.0f); // user Y → world Z
        case 2: return glm::vec3(0.0f, 1.0f, 0.0f); // user Z → world Y (up)
    }
    return glm::vec3(1.0f, 0.0f, 0.0f);
}

inline int userAxisToWorldIdx(int userIdx) {
    switch (userIdx) { case 0: return 0; case 1: return 2; case 2: return 1; }
    return 0;
}

} // namespace materializr
