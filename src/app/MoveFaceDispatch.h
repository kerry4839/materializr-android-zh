#pragma once

#include "MoveFaceState.h" // FaceXform
#include <glm/glm.hpp>

namespace materializr {

// Everything a Move Face preview call varies frame-to-frame (NOT loop-motion
// or which face/body - those are fixed for the whole gesture, set once at
// begin). Deliberately hand-built rather than derived from
// MoveFaceOp::serializeParams(): that string is a RELOAD format that only
// round-trips Translate/Twist (see MoveFaceOp.cpp's own comment on
// serializeParams), so it silently ignores Rotate's transform and all of
// Scale - two frames with different rotations would compare equal and the
// dispatch would show a stale result forever.
//
// MUST stay in sync with MoveFaceController::configureFaceOp(): every field
// that function reads from MoveFaceState to configure the op belongs here
// too. There is no compiler check for this; a new configureFaceOp parameter
// needs a new field here in the same change.
//
// Deliberately does NOT include m_st.moveFaceLocal: that flag doesn't change
// what MoveFaceOp computes, it changes whether MoveFaceOp runs at all
// (localTweakApplies() routes to FaceTweakOp instead). MoveFaceController's
// pollPreview() discards any landed general-path result while
// localTweakApplies() is true, precisely so a stale key match can never
// overwrite a local rebuild - do not "fix" that by adding the flag here.
struct MoveFaceKey {
    // Round 4 addition: a cross-gesture safety net, not something
    // configureFaceOp reads. If a job launched by gesture N is still
    // computing when gesture N+1 begins on a DIFFERENT body,
    // MoveFaceController::pollPreview() must never write gesture N's result
    // onto gesture N+1's body id. beginMoveFace() resets m_mfDispatch, which
    // already makes this vanishingly unlikely on its own (the reset key
    // essentially never matches a real gesture's key) - this field makes it
    // impossible rather than merely unlikely.
    int bodyId = -1;
    FaceXform kind = FaceXform::Translate;
    bool isTwist = false;
    glm::vec3 moveVec{0.0f};       // Translate
    glm::mat3 rotMat{1.0f};        // Rotate: faceRotTotal()'s output
    glm::vec3 pivot{0.0f};         // Rotate/Scale pivot (fixed per gesture, kept for safety)
    float twistAngle = 0.0f;       // Twist
    bool scaleUniform = true;
    float scaleFactor = 1.0f;      // Scale, uniform
    float scaleA = 1.0f, scaleB = 1.0f; // Scale, non-uniform
    glm::vec3 scaleAxisA{1.0f, 0.0f, 0.0f}, scaleAxisB{0.0f, 1.0f, 0.0f};

    bool operator==(const MoveFaceKey& o) const
    {
        return bodyId == o.bodyId && kind == o.kind && isTwist == o.isTwist &&
               moveVec == o.moveVec && rotMat == o.rotMat && pivot == o.pivot &&
               twistAngle == o.twistAngle && scaleUniform == o.scaleUniform &&
               scaleFactor == o.scaleFactor && scaleA == o.scaleA && scaleB == o.scaleB &&
               scaleAxisA == o.scaleAxisA && scaleAxisB == o.scaleAxisB;
    }
};

} // namespace materializr
