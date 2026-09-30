#pragma once
#include "FaceAnchor.h"
#include <vector>

class Document;

namespace materializr {

// A persistent placement relationship between two bodies. bodyA is the
// reference and is never moved by this mate; bodyB is placed relative to it.
// That asymmetry is what makes solve order a topological sort rather than a
// constraint system.
//
// See docs/superpowers/specs/2026-08-16-assembly-mates-design.md
enum class MateType { Fasten, Concentric, Planar };

struct Mate {
    int id = -1;
    MateType type = MateType::Fasten;
    int bodyA = -1;              // reference; not moved by this mate
    int bodyB = -1;              // placed
    // Anchors resolve through the existing persistent-naming machinery, so a
    // mate survives a regeneration that renumbers faces.
    std::vector<FaceAnchor::Anchor> anchorsA;
    std::vector<FaceAnchor::Anchor> anchorsB;
    double offset = 0.0;         // along the mate axis / normal
    double angle = 0.0;          // roll about it
    bool flipped = false;        // which of the two alignments was chosen
    bool suppressed = false;     // ignored by the solver, kept in the file
    bool broken = false;         // an anchor failed to resolve; see the spec

    // The reference-to-body offset as it stood when the mate was created, for
    // an anchorless mate. Without it a fresh Fasten with offset 0 slammed the
    // two bodies' bounding-box corners together, so making a mate MOVED the
    // part - and the only way back was one scalar along X. With it, offset 0
    // means "keep what I had" and offset is a delta on that.
    bool hasRelPose = false;
    double relX = 0.0, relY = 0.0, relZ = 0.0;
};

// True when a mate places this body, so its position is owned by the solve.
// Shared rather than reimplemented: TransformOp had this check and
// BatchTransformOp did not, so the multi-body gizmo walked straight past a
// guard that already existed.
bool bodyIsMatePlaced(const Document& doc, int bodyId);

} // namespace materializr
