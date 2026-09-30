#include "MateSolver.h"
#include "../core/Document.h"
#include "Sketch.h"

#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <set>
#include <vector>
#include <algorithm>
#include <Standard_Failure.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

namespace materializr {

gp_Trsf MateSolver::fastenTrsf(const Mate& m, const gp_Trsf& refPlacement) {
    gp_Trsf local;
    local.SetTranslation(gp_Vec(m.offset, 0.0, 0.0));
    // Compose: the reference's placement, then this mate's own offset.
    return refPlacement * local;
}

// A frame for a body that carries no anchors: the minimum corner of its
// bounding box, axis-aligned. Crude, but it is DERIVED FROM THE GEOMETRY, which
// is the property that matters - an anchorless Fasten built on a constant
// translation ignored where its reference actually was, so moving the
// reference left the mated body behind. That was the feature's headline claim
// failing outright.
static bool bboxFrame(const TopoDS_Shape& shape, gp_Ax3& out) {
    if (shape.IsNull()) return false;
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (box.IsVoid()) return false;
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    out = gp_Ax3(gp_Pnt(xmin, ymin, zmin), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));
    return true;
}

gp_Trsf MateSolver::alignTrsf(const Mate& m, const gp_Ax3& frameA,
                              const gp_Ax3& frameB) {
    // Bring frameB onto frameA. Flipped reverses B's normal, which is the
    // other of the two ways any two faces can be brought together - without it
    // half the mates a user can draw are unreachable.
    gp_Ax3 b = frameB;
    if (m.flipped) b.SetDirection(frameB.Direction().Reversed());

    // SetDisplacement, NOT SetTransformation. SetTransformation builds the
    // change-of-COORDINATES map b^-1 . frameA; what a mate needs is the rigid
    // MOTION frameA . b^-1 that carries b's frame onto A's. The two agree only
    // when the frames commute - which is exactly the configuration every
    // hand-written test fixture happened to use, so a full suite passed while
    // every Concentric and Planar mate placed its body wrong the moment frame
    // A was rotated and frame B sat off the origin.
    gp_Trsf toA;
    toA.SetDisplacement(b, frameA);

    // The mate's own parameters act in frameA: offset along its Z, roll about
    // the same axis.
    // Fasten's offset has always meant "along X"; Concentric and Planar mean
    // "along the frame normal". The bbox frame puts X where Fasten wants it
    // and Z where a face normal would be, so each type reads its own axis
    // rather than the two silently sharing one and disagreeing.
    const gp_Dir slideDir = (m.type == MateType::Fasten)
                                ? frameA.XDirection()
                                : frameA.Direction();
    gp_Trsf slide;
    slide.SetTranslation(gp_Vec(slideDir) * m.offset);

    gp_Trsf roll;
    if (m.angle != 0.0)
        roll.SetRotation(gp_Ax1(frameA.Location(), frameA.Direction()), m.angle);

    return slide * roll * toA;
}

bool MateSolver::frameFor(const Document& doc, int bodyId,
                          const std::vector<FaceAnchor::Anchor>& anchors,
                          gp_Ax3& out) {
    if (anchors.empty()) return false;

    std::vector<FaceAnchor::SketchRef> sketches;
    for (int sid : doc.getAllSketchIds())
        if (auto sk = doc.getSketch(sid))
            sketches.push_back({sid, sk.get()});

    std::vector<TopoDS_Face> faces;
    // Resolve against the BASE geometry, not the placed shape: reading a frame
    // off the shape a previous solve already moved makes the next solve
    // compose that placement a second time.
    const TopoDS_Shape& against =
        doc.hasMateBase(bodyId) ? doc.getMateBase(bodyId) : doc.getBody(bodyId);
    if (!FaceAnchor::resolve(anchors, sketches, against, faces))
        return false;
    if (faces.empty()) return false;

    BRepAdaptor_Surface surf(faces.front());
    if (surf.GetType() == GeomAbs_Plane) {
        const gp_Pln pln = surf.Plane();
        out = pln.Position();
        return true;
    }
    if (surf.GetType() == GeomAbs_Cylinder) {
        const gp_Cylinder cyl = surf.Cylinder();
        out = cyl.Position();
        return true;
    }
    // Any other surface has no single frame a mate can mean; refuse rather
    // than inventing one.
    return false;
}

MateSolver::Result MateSolver::solve(Document& doc) {
    Result res;
    const auto& mates = doc.getMates();
    std::vector<int> brokenIds;
    std::set<int> skip;   // bodies whose mate broke: leave them alone

    // One placing mate per body, at most. More than one is over-constraint and
    // is reported rather than reconciled.
    std::map<int, const Mate*> placedBy;
    // A body can be deleted while mates still reference it - nothing prunes
    // them. getBody THROWS for an unknown id, and this runs inside
    // History::pushOperation AFTER the op has executed and been pushed, so an
    // escaping exception leaves history and document out of step. Filter first.
    std::set<int> live;
    for (int bid : doc.getAllBodyIds()) live.insert(bid);

    for (const auto& m : mates) {
        // I1: broken mates are re-evaluated, not skipped. Skipping them meant
        // the flag flipped on and off on alternate solves, which also let the
        // over-constraint check below miss them.
        if (m.suppressed) continue;
        if (m.bodyA < 0 || m.bodyB < 0) continue;
        if (!live.count(m.bodyA) || !live.count(m.bodyB)) {
            brokenIds.push_back(m.id);
            // bodyA can vanish while bodyB survives with a base cached from a
            // PRIOR successful solve (when bodyA still existed). Left in
            // place, a future mate re-targeting bodyB would read that stale
            // pre-loss base instead of bodyB's real current shape.
            if (live.count(m.bodyB)) doc.clearMateBase(m.bodyB);
            continue;
        }
        auto it = placedBy.find(m.bodyB);
        if (it != placedBy.end()) {
            // Over-constraint: mark just this mate broken (same treatment as
            // a lost anchor reference below) instead of failing the whole
            // solve - one bad mate anywhere in the document must not freeze
            // every other, unrelated one. The mate that claimed the body
            // first keeps placing it.
            brokenIds.push_back(m.id);
            continue;
        }
        placedBy[m.bodyB] = &m;
    }

    // Resolved placement per body. The grounded body anchors the walk at
    // identity; anything else placed by a mate inherits from its reference.
    std::map<int, gp_Trsf> placement;
    int grounded = doc.getGroundedBody();
    if (grounded >= 0) {
        // The grounded body is fixed by definition, so a mate that also places
        // it is a contradiction. Catching it here names the real problem; left
        // to the walk below it would either pass silently (the grounded
        // placement already exists, so the cycle is never entered) or surface
        // as a confusing "cycle" further along.
        auto git = placedBy.find(grounded);
        if (git != placedBy.end()) {
            // The grounded body is fixed by definition - a mate trying to
            // place it too is broken, not a reason to freeze every other
            // mate in the document.
            brokenIds.push_back(git->second->id);
            placedBy.erase(git);
            // The grounded body is no longer in placedBy, so the apply loop
            // below never revisits it - a base cached from BEFORE it was
            // grounded (while this same mate still legitimately placed it)
            // would otherwise survive and mislead any OTHER mate that later
            // reads it as bodyA's reference frame, into using its stale
            // pre-conflict shape instead of its real current one.
            doc.clearMateBase(grounded);
        }
        placement[grounded] = gp_Trsf();
    }

    // Walk each placed body up to its reference, then apply on the way back.
    // The visited set makes a cycle an error instead of an infinite walk.
    for (const auto& kv : placedBy) {
        std::vector<int> chain;
        std::set<int> seen;
        int cur = kv.first;
        bool cyclic = false;
        while (placement.find(cur) == placement.end()) {
            if (seen.count(cur)) {
                cyclic = true;
                break;
            }
            seen.insert(cur);
            auto pit = placedBy.find(cur);
            if (pit == placedBy.end()) {
                // Not mated and not grounded: it sits where history left it.
                placement[cur] = gp_Trsf();
                break;
            }
            chain.push_back(cur);
            cur = pit->second->bodyA;
        }
        if (cyclic) {
            // Isolate the cycle to the bodies actually walked into it (a
            // "tail" leading into a cycle depends on the cycle transitively,
            // so it belongs here too - same reasoning as the descendant
            // safety note below), not every other, unrelated mate in the
            // document. `chain` already holds every body visited this walk;
            // the repeated node itself was recorded on its first visit.
            for (int bodyId : chain) {
                auto pit = placedBy.find(bodyId);
                if (pit != placedBy.end())
                    brokenIds.push_back(pit->second->id);
                placement[bodyId] = gp_Trsf();
                skip.insert(bodyId);
                doc.clearMateBase(bodyId);
            }
            continue;
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            const Mate* m = placedBy[*it];
            gp_Ax3 fa, fb;
            const bool haveFrames =
                frameFor(doc, m->bodyA, m->anchorsA, fa) &&
                frameFor(doc, m->bodyB, m->anchorsB, fb);
            if (haveFrames) {
                // The reference's own placement composes on top, so a chain
                // carries through the middle body as it does for Fasten.
                placement[*it] = placement[m->bodyA] * alignTrsf(*m, fa, fb);
            } else if (m->type == MateType::Fasten) {
                // Anchorless Fasten. Both frames come from geometry - A's from
                // where it CURRENTLY sits, B's from its base - so the placement
                // tracks the reference instead of being a fixed translation
                // that ignores it.
                gp_Ax3 ga, gb;
                const TopoDS_Shape& aBase = doc.hasMateBase(m->bodyA)
                                                ? doc.getMateBase(m->bodyA)
                                                : doc.getBody(m->bodyA);
                const TopoDS_Shape& bBase = doc.hasMateBase(m->bodyB)
                                                ? doc.getMateBase(m->bodyB)
                                                : doc.getBody(m->bodyB);
                if (bboxFrame(aBase, ga) && bboxFrame(bBase, gb)) {
                    // Preserve the arrangement the mate was created in. The
                    // corner is only a REFERENCE POINT for tracking the
                    // reference body; the stored relative pose is what decides
                    // where this body sits, so offset 0 changes nothing.
                    if (m->hasRelPose) {
                        gp_Ax3 shifted = ga;
                        shifted.SetLocation(gp_Pnt(
                            ga.Location().X() + m->relX,
                            ga.Location().Y() + m->relY,
                            ga.Location().Z() + m->relZ));
                        ga = shifted;
                    }
                    // Mid-walk the reference has not been APPLIED yet - the
                    // apply loop runs after the whole walk - so its frame has
                    // to be carried to where the walk has decided it will be.
                    // Reading the live shape instead left a chained body
                    // measuring from the middle body's old position.
                    ga.Transform(placement[m->bodyA]);
                    // angle/flipped are forced off here regardless of what the
                    // Mate record stores: bboxFrame is a FIXED axis convention
                    // (always Z=(0,0,1), X=(1,0,0) at the bbox min corner), not
                    // one derived from the shape's actual orientation the way
                    // frameFor's real face geometry is. The panel already
                    // disables editing these fields without real anchors (see
                    // MatePlugin.cpp), but a project file predating that fix,
                    // or a hand-edited one, could still carry a nonzero value
                    // here - applying it would compound a rotation on every
                    // base recapture (undo/redo, Suppress toggle, any History
                    // op that clears cached mate bases) instead of reproducing
                    // the same pose, a silent, unreported drift.
                    Mate mNoRoll = *m;
                    mNoRoll.angle = 0.0;
                    mNoRoll.flipped = false;
                    placement[*it] = alignTrsf(mNoRoll, ga, gb);
                } else {
                    placement[*it] = fastenTrsf(*m, placement[m->bodyA]);
                }
            } else {
                // A regeneration renamed the faces out from under this mate.
                // Marking it broken and leaving the body where it is beats
                // both alternatives: dropping the mate loses the user's work
                // silently, and hard-failing the solve would strand every
                // OTHER mate in the document over one lost reference.
                brokenIds.push_back(m->id);
                // Do NOT hand descendants a placement this body never receives
                // (it is in `skip` and is left where it is). A healthy B->C
                // behind a broken A->B would otherwise put C where B is not.
                // Identity means "wherever it already sits", which is the
                // truth for a body the solve declined to move.
                placement[*it] = gp_Trsf();
                skip.insert(*it);
                // Drop the base HERE, not after the walk: a descendant
                // resolved later in this same walk reads this body as its
                // reference, and a stale base would have it measure from the
                // unplaced shape while the body renders where an earlier solve
                // left it.
                doc.clearMateBase(*it);
            }
        }
    }

    // Apply, in two passes. VALIDATE every candidate transform first, without
    // touching the document, then COMMIT only the survivors. placedBy is a
    // std::map keyed by body id, so this loop's order is ascending id, not
    // dependency order - a body chained through another (A->B->C) can be
    // reached before or after its reference. Every placement was already
    // computed as a pure matrix during the walk above, so a downstream body's
    // placement is well-defined even when its reference's OWN kernel
    // transform later fails - but committing that placement anyway would move
    // it to where its reference WOULD be if that transform had succeeded, not
    // where the reference actually is. A failure must therefore propagate to
    // every mate chained through the failed body BEFORE anything is written,
    // exactly like a lost anchor already does in the walk phase above.
    std::map<int, TopoDS_Shape> candidates;
    std::set<int> transformFailed;
    for (const auto& kv : placedBy) {
        int bodyId = kv.first;
        if (skip.count(bodyId)) continue;
        if (!doc.hasMateBase(bodyId)) doc.setMateBase(bodyId, doc.getBody(bodyId));
        try {
            BRepBuilderAPI_Transform xf(doc.getMateBase(bodyId), placement[bodyId],
                                        /*copy=*/true);
            if (xf.IsDone()) {
                candidates[bodyId] = xf.Shape();
            } else {
                transformFailed.insert(bodyId);
            }
        } catch (const std::exception&) {
            // A null or degenerate base shape throws rather than returning.
            // Do not let it escape into the history push above.
            transformFailed.insert(bodyId);
        } catch (...) {
            // Non-Standard_Failure / non-std::exception (e.g. deep OCCT
            // plumbing) must not escape either - this runs after the op that
            // triggered it has already been pushed onto history, so an
            // escaping exception here would leave history and document
            // permanently out of step, not just fail one mate.
            transformFailed.insert(bodyId);
        }
    }

    // Propagate: any mate whose bodyA is a failed body was computed assuming
    // that body's placement would be reached. Repeat until nothing new is
    // added, so a failure two or more links down a chain still reaches C via
    // B even though B itself is a valid transform.
    {
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& kv : placedBy) {
                int bodyId = kv.first;
                if (transformFailed.count(bodyId)) continue;
                if (!transformFailed.count(kv.second->bodyA)) continue;
                transformFailed.insert(bodyId);
                candidates.erase(bodyId);
                changed = true;
            }
        }
    }

    for (int bodyId : transformFailed) {
        // A kernel-level transform failure isolates to this body's own mate
        // (and every mate chained through it) exactly like a lost anchor, a
        // graph cycle, or an over-constraint conflict above - one bad body
        // must not strand every other, unrelated mate still waiting in
        // placedBy.
        auto it = placedBy.find(bodyId);
        if (it != placedBy.end()) brokenIds.push_back(it->second->id);
        skip.insert(bodyId);
        doc.clearMateBase(bodyId);
    }

    for (const auto& kv : candidates) {
        int bodyId = kv.first;
        doc.updateBody(bodyId, kv.second, /*fromMateSolve=*/true);

        // Sketches anchored to this body travel with it. TransformOp does the
        // same at src/modeling/TransformOp.cpp:138 - a sketch left behind
        // detaches from the face it was drawn on, and every feature built from
        // it regenerates in the wrong place. A detached sketch has been
        // deliberately unlinked and must not follow.
        const gp_Trsf& t = placement[bodyId];
        for (int sid : doc.getAllSketchIds()) {
            auto sk = doc.getSketch(sid);
            if (!sk) continue;
            if (sk->getSourceBody() != bodyId) continue;
            if (sk->isDetachedFromBody()) continue;
            // From the BASE plane for the same reason placement starts from the
            // base shape: transforming the current plane each solve compounds.
            if (!doc.hasMateSketchPlane(sid)) doc.setMateSketchPlane(sid, sk->getPlane());
            sk->setPlane(doc.getMateSketchPlane(sid).Transformed(t));
        }
    }

    // Report broken mates on the Document so the UI can colour them and say
    // which reference was lost. The solve itself still succeeds - the rest of
    // the assembly is fine.
    // Base hygiene, in one place. m_mateBases must describe exactly the bodies
    // THIS solve placed. A base left behind for a body the solve did not place
    // - deleted mate, suppressed mate, or a mate skipped because it broke -
    // still describes the UNPLACED shape while the body renders where an
    // earlier solve put it. Anything that later reads that body as a reference
    // then measures from a position it does not occupy: a healthy B->C behind
    // a newly broken A->B jumped 25mm, and a fresh mate onto a body whose
    // previous mate was deleted jumped by the old placement delta.
    {
        std::set<int> placedNow;
        for (const auto& kv : placedBy)
            if (!skip.count(kv.first)) placedNow.insert(kv.first);
        std::vector<int> drop;
        for (const auto& kv : placedBy)
            if (!placedNow.count(kv.first)) drop.push_back(kv.first);
        for (const auto& m : doc.getMates())
            if ((m.suppressed || m.broken) && m.bodyB >= 0 &&
                !placedNow.count(m.bodyB))
                drop.push_back(m.bodyB);
        for (int id : drop) doc.clearMateBase(id);
    }

    for (auto& m : doc.getMutableMates()) {
        // A suppressed mate never reaches the walk, so it never re-enters the
        // broken list - clearing its flag here would render it healthy and it
        // would break again the moment it was un-suppressed.
        if (m.suppressed) continue;
        m.broken = (std::find(brokenIds.begin(), brokenIds.end(), m.id) !=
                    brokenIds.end());
    }
    res.brokenMateIds = brokenIds;

    return res;
}

} // namespace materializr
