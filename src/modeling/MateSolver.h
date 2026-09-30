#pragma once
#include "Mate.h"
#include <TopoDS_Shape.hxx>
#include <gp_Trsf.hxx>
#include <gp_Pln.hxx>
#include <gp_Ax3.hxx>
#include <vector>
#include <string>
#include <map>

class Document;

namespace materializr {

// Places mated bodies by walking the mate graph from the grounded body.
//
// This is deliberately NOT a constraint system. Each mate type is a direct
// placement computation, so the solve is a topological sort plus transform
// composition: no relaxation, no residual, no iteration budget. A body reached
// by two mates, or a cycle, is an error rather than something to compromise
// between - which makes over-constraint an exact graph property instead of the
// equation count the sketch solver has to guess with.
class MateSolver {
public:
    struct Result {
        bool ok = true;
        std::string error;
        // Mates whose anchors stopped resolving after a regeneration. The
        // solve still succeeds; these are reported so the UI can flag them
        // instead of the geometry silently drifting.
        std::vector<int> brokenMateIds;
    };

    // Places every non-suppressed mate's bodyB. Bodies not reachable from the
    // grounded body are left where history put them.
    Result solve(Document& doc);

    // The transform placing bodyB, given its reference's resolved placement.
    // Fasten with no anchors is a translation of `offset` along X.
    static gp_Trsf fastenTrsf(const Mate& m, const gp_Trsf& refPlacement);

    // The transform bringing frameB onto frameA, then applying the mate's own
    // offset and roll. All three mate types share this: what differs between
    // them is only which frame the anchor yields (a plane's origin+normal, a
    // cylinder's axis), not how the two frames are brought together. Keeping
    // the composition in one place means Concentric and Planar cannot drift
    // apart in their sign conventions.
    //
    // `offset` runs along frameA's Z, `angle` rolls about it, and `flipped`
    // reverses frameB's direction - the second of the two alignments any
    // face-to-face mate admits.
    static gp_Trsf alignTrsf(const Mate& m, const gp_Ax3& frameA,
                             const gp_Ax3& frameB);

    // All base state - body shapes and sketch planes - lives on the Document,
    // so a solver instance carries no placement state at all and this has
    // nothing left to clear. Kept because History still distinguishes a
    // regenerating path from an in-place one.
    void reset() {}

    // The mate frame for one side: resolves the anchors against the body's
    // current shape and reads the geometry off the face they name. Returns
    // false when the anchors do not resolve - the caller marks the mate broken
    // rather than guessing a frame.
    static bool frameFor(const Document& doc, int bodyId,
                         const std::vector<FaceAnchor::Anchor>& anchors,
                         gp_Ax3& out);

private:
    // Geometry as history produced it, before any mate placement. Solving
    // always starts here so a solve is a function of the model, not of the
    // previous solve - the property SolvingTwiceIsIdempotent pins.
};

} // namespace materializr
