// Adopting the result the preview worker already computed.
//
// The snapshot-body operations recompute on Confirm what their worker just
// produced. Adoption skips that, but only when the result is provably still
// valid: the live body must be IsEqual the one the worker started from, and
// the operation's post-rebind selection must match the one the worker used.
//
// HOW THESE TESTS PROVE ADOPTION HAPPENED. Handing the operation a result that
// could never arise from the real computation (a marker box) and then finding
// that shape on the body is direct evidence the candidate was adopted. The
// alternative - comparing an adopted result against a computed one - cannot
// distinguish "adopted" from "recomputed and coincidentally equal", which is
// exactly the vacuous pass this suite has to avoid.
#include "core/Document.h"
#include "core/MeshParams.h"
#include "modeling/MoveFaceOp.h"
#include "modeling/ScaleFaceOp.h"
#include "modeling/ShellOp.h"
#include "modeling/SubShapeIndex.h"
#include "modeling/TaperOp.h"

#include <gtest/gtest.h>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Plane.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Compound.hxx>
#include <functional>
#include <set>
#include <vector>

namespace {

TopoDS_Shape plate(int nx, int ny) {
    TopoDS_Shape p = BRepPrimAPI_MakeBox(nx * 15.0, ny * 15.0, 10.0).Shape();
    TopoDS_Compound holes;
    BRep_Builder bb;
    bb.MakeCompound(holes);
    for (int i = 0; i < nx; ++i)
        for (int j = 0; j < ny; ++j)
            bb.Add(holes, BRepPrimAPI_MakeCylinder(
                              gp_Ax2(gp_Pnt(7.5 + i * 15.0, 7.5 + j * 15.0, -1.0),
                                     gp_Dir(0, 0, 1)), 3.0, 12.0).Shape());
    return BRepAlgoAPI_Cut(p, holes).Shape();
}

TopoDS_Face topFace(const TopoDS_Shape& s) {
    TopoDS_Face best;
    double bz = -1e9;
    for (TopExp_Explorer e(s, TopAbs_FACE); e.More(); e.Next()) {
        GProp_GProps g;
        BRepGProp::SurfaceProperties(e.Current(), g);
        if (g.CentreOfMass().Z() > bz && g.Mass() > 100) {
            bz = g.CentreOfMass().Z();
            best = TopoDS::Face(e.Current());
        }
    }
    return best;
}

double volumeOf(const TopoDS_Shape& s) {
    GProp_GProps g;
    BRepGProp::VolumeProperties(s, g);
    return g.Mass();
}

// A shape the real operation could never produce, so finding it on the body is
// unambiguous evidence of adoption.
TopoDS_Shape marker() {
    return BRepPrimAPI_MakeBox(gp_Pnt(-500, -500, -500), 7.0, 7.0, 7.0).Shape();
}
bool isMarker(const TopoDS_Shape& s) {
    return !s.IsNull() && std::abs(volumeOf(s) - 343.0) < 1e-6;
}

std::unique_ptr<ShellOp> shellOp(int id, const TopoDS_Face& f, double t = 1.0) {
    auto op = std::make_unique<ShellOp>();
    op->setBody(id);
    op->setThickness(t);
    op->addFaceToRemove(f);
    return op;
}

} // namespace

TEST(PreviewAdoption, AdoptsWhenTheBodyAndSelectionStillMatch) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const TopoDS_Face f = topFace(base);

    auto op = shellOp(id, f);
    op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));

    ASSERT_TRUE(op->execute(doc));
    EXPECT_TRUE(isMarker(doc.getBody(id)))
        << "the candidate was not adopted; the operation recomputed instead";
}

TEST(PreviewAdoption, RejectsWhenTheBodyChangedUnderIt) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const TopoDS_Face f = topFace(base);

    auto op = shellOp(id, f);
    op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));

    // Something else edited the body between the preview and the commit.
    doc.updateBody(id, BRepPrimAPI_MakeBox(60.0, 50.0, 10.0).Shape());

    // It must not adopt, and it must still do its job.
    EXPECT_TRUE(op->execute(doc)) << "the fallback computation failed";
    EXPECT_FALSE(isMarker(doc.getBody(id)))
        << "a result computed against a stale body was adopted";
}

// Isolates the base check specifically. The previous test's body swap also
// broke the selection, so the KEY check caught it and the base check could be
// deleted with every test still passing - the exact vacuous pass this suite
// exists to prevent. Here the replacement body is a topological COPY: the
// face indices still resolve, so the key matches, and only IsEqual can tell
// that this is not the shape the worker measured.
TEST(PreviewAdoption, RejectsARebuiltBodyEvenWhenTheSelectionStillResolves) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const TopoDS_Face f = topFace(base);

    auto op = shellOp(id, f);
    op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));

    // Same topology, fresh TShapes: what an upstream rebuild produces.
    const TopoDS_Shape rebuilt =
        BRepBuilderAPI_Copy(base, Standard_True, Standard_False).Shape();
    ASSERT_FALSE(rebuilt.IsEqual(base)) << "the copy must not be the same shape";
    doc.updateBody(id, rebuilt);

    // The selection still resolves against the rebuilt body, so the key alone
    // would accept it.
    auto probe = shellOp(id, topFace(rebuilt));
    ASSERT_FALSE(probe->previewKey(rebuilt).empty());
    ASSERT_EQ(probe->previewKey(rebuilt), op->previewKey(base))
        << "the copy should index identically; otherwise this test proves nothing";

    EXPECT_TRUE(op->execute(doc)) << "the fallback computation failed";
    EXPECT_FALSE(isMarker(doc.getBody(id)))
        << "a result computed against the pre-rebuild body was adopted";
}

TEST(PreviewAdoption, RejectsWhenTheSelectionDiffers) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");

    // The worker's key describes the TOP face; the committed op opens a
    // different one. Body identity alone would not catch this.
    auto op = shellOp(id, topFace(base));
    TopoDS_Face other;
    for (TopExp_Explorer e(base, TopAbs_FACE); e.More(); e.Next()) {
        GProp_GProps g;
        BRepGProp::SurfaceProperties(e.Current(), g);
        if (g.CentreOfMass().Z() < 1.0 && g.Mass() > 100) {
            other = TopoDS::Face(e.Current());
            break;
        }
    }
    ASSERT_FALSE(other.IsNull());
    auto keyOp = shellOp(id, other);
    op->setPrecomputedResult(doc.getBody(id), marker(), keyOp->previewKey(base));

    EXPECT_TRUE(op->execute(doc)) << "the fallback computation failed";
    EXPECT_FALSE(isMarker(doc.getBody(id)))
        << "a result computed for a different face was adopted";
}

TEST(PreviewAdoption, TheCandidateIsConsumedOnTheFirstAttemptEvenIfRejected) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const TopoDS_Face f = topFace(base);

    auto op = shellOp(id, f);
    op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));

    // Attempt 1: rejected, because the body moved.
    const TopoDS_Shape decoy = BRepPrimAPI_MakeBox(60.0, 50.0, 10.0).Shape();
    doc.updateBody(id, decoy);
    ASSERT_TRUE(op->execute(doc));
    ASSERT_FALSE(isMarker(doc.getBody(id)));

    // Put the original body back, so the candidate's base would match again.
    doc.updateBody(id, base);
    EXPECT_TRUE(op->execute(doc));
    EXPECT_FALSE(isMarker(doc.getBody(id)))
        << "a candidate rejected once was still armed and got adopted later";
}

TEST(PreviewAdoption, UndoAfterAnAdoptedCommitRestoresThePreviousShape) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const double vol0 = volumeOf(doc.getBody(id));

    auto op = shellOp(id, topFace(base));
    op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));
    ASSERT_TRUE(op->execute(doc));
    ASSERT_TRUE(isMarker(doc.getBody(id)));

    ASSERT_TRUE(op->undo(doc));
    EXPECT_NEAR(volumeOf(doc.getBody(id)), vol0, 1e-6)
        << "undo did not restore the body an adopted commit replaced";
}

TEST(PreviewAdoption, AnEmptyKeyNeverAdopts) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");

    auto op = shellOp(id, topFace(base));
    // An operation that has not opted in returns an empty key, and an
    // unresolved selection returns one too. Neither may ever match.
    op->setPrecomputedResult(doc.getBody(id), marker(), std::string());

    EXPECT_TRUE(op->execute(doc));
    EXPECT_FALSE(isMarker(doc.getBody(id))) << "an empty key was treated as a match";
}

TEST(PreviewAdoption, AnUnresolvableSelectionYieldsNoKey) {
    const TopoDS_Shape base = plate(6, 5);
    const TopoDS_Shape unrelated = BRepPrimAPI_MakeBox(5.0, 5.0, 5.0).Shape();
    Document doc;
    const int id = doc.addBody(base, "plate");

    // A face that is not part of `base` at all.
    auto op = shellOp(id, topFace(unrelated));
    EXPECT_TRUE(op->previewKey(base).empty())
        << "a selection that does not resolve against the base produced a key";
}

// SubShapeIndex::serialize skips sub-shapes it cannot resolve, so
// [live, stale] serialises identically to [live]. A cache keyed on that would
// reuse a result computed for a different selection.
TEST(PreviewAdoption, TheKeyRejectsAPartiallyResolvableSelection) {
    const TopoDS_Shape base = plate(6, 5);
    const TopoDS_Shape unrelated = BRepPrimAPI_MakeBox(5.0, 5.0, 5.0).Shape();
    std::vector<TopoDS_Shape> mixed{topFace(base), topFace(unrelated)};

    std::string key;
    EXPECT_FALSE(SubShapeIndex::orientedKey(base, mixed, TopAbs_FACE, key));
    EXPECT_TRUE(key.empty());

    std::vector<TopoDS_Shape> justTheGoodOne{topFace(base)};
    std::string good;
    ASSERT_TRUE(SubShapeIndex::orientedKey(base, justTheGoodOne, TopAbs_FACE, good));
    EXPECT_NE(good, key) << "a partial selection must not key like a whole one";
}

// The same contract on the other two operations. Shell carries the detailed
// cases above; these prove the mechanism is actually wired into each op rather
// than only into the base class.
TEST(PreviewAdoption, DraftAdoptsAndRejectsOnTheSameTerms) {
    const TopoDS_Shape base = BRepPrimAPI_MakeBox(40.0, 30.0, 20.0).Shape();
    TopoDS_Face side;
    for (TopExp_Explorer e(base, TopAbs_FACE); e.More(); e.Next()) {
        GProp_GProps g;
        BRepGProp::SurfaceProperties(e.Current(), g);
        if (std::abs(g.CentreOfMass().X()) < 1e-9) { side = TopoDS::Face(e.Current()); break; }
    }
    ASSERT_FALSE(side.IsNull());

    auto mk = [&](Document& d, int id) {
        auto op = std::make_unique<TaperOp>();
        op->setBody(id);
        op->setAngleDeg(5.0);
        op->addFace(side);
        op->setDirection(0, 0, 1);
        op->setNeutralPoint(0, 0, 0);
        return op;
    };
    {   Document doc; const int id = doc.addBody(base, "b");
        auto op = mk(doc, id);
        op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));
        ASSERT_TRUE(op->execute(doc));
        EXPECT_TRUE(isMarker(doc.getBody(id))) << "Draft did not adopt";
    }
    {   // Base mismatch, expressed by handing a candidate whose base is some
        // other shape rather than by rebuilding the body. A fresh TaperOp
        // cannot re-bind onto a rebuilt body at all (it has no minted topo
        // names yet, so execute refuses), which would make the fallback fail
        // for a reason that has nothing to do with adoption. Shell carries the
        // rebuilt-body case.
        Document doc; const int id = doc.addBody(base, "b");
        auto op = mk(doc, id);
        op->setPrecomputedResult(BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape(),
                                 marker(), op->previewKey(base));
        EXPECT_TRUE(op->execute(doc)) << "Draft's fallback computation failed";
        EXPECT_FALSE(isMarker(doc.getBody(id))) << "Draft adopted a stale result";
    }
}

TEST(PreviewAdoption, ScaleFaceAdoptsAndRejectsOnTheSameTerms) {
    const TopoDS_Shape base = BRepPrimAPI_MakeBox(40.0, 30.0, 20.0).Shape();
    const TopoDS_Face top = topFace(base);
    auto mk = [&](int id) {
        auto op = std::make_unique<ScaleFaceOp>();
        op->setBody(id);
        op->setFace(top);
        op->setScaleUV(120, 120);
        op->setLength(5.0);
        op->setMode(ScaleFaceOp::Mode::Pinch);
        return op;
    };
    {   Document doc; const int id = doc.addBody(base, "b");
        auto op = mk(id);
        op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));
        ASSERT_TRUE(op->execute(doc));
        EXPECT_TRUE(isMarker(doc.getBody(id))) << "Scale Face did not adopt";
    }
    {   Document doc; const int id = doc.addBody(base, "b");
        auto op = mk(id);
        op->setPrecomputedResult(BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape(),
                                 marker(), op->previewKey(base));
        EXPECT_TRUE(op->execute(doc)) << "Scale Face's fallback failed";
        EXPECT_FALSE(isMarker(doc.getBody(id))) << "Scale Face adopted a stale result";
    }
}

TEST(PreviewAdoption, MoveFaceAdoptsAndRejectsOnTheSameTerms) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const TopoDS_Face f = topFace(base);

    auto mk = [&] {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(id);
        op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Translate);
        op->setMoveVector(gp_Vec(2.0, 0.0, 0.0));
        return op;
    };

    {   // Adopts when the body and selection still match.
        auto op = mk();
        op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));
        ASSERT_TRUE(op->execute(doc));
        EXPECT_TRUE(isMarker(doc.getBody(id))) << "Move Face did not adopt";
    }
    {   // Rejects when the body changed under it (fresh Document, so this op
        // has no minted face name yet - matches TaperOp's equivalent test).
        Document doc2; const int id2 = doc2.addBody(base, "plate");
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(id2);
        op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Translate);
        op->setMoveVector(gp_Vec(2.0, 0.0, 0.0));
        op->setPrecomputedResult(BRepPrimAPI_MakeBox(1.0, 1.0, 1.0).Shape(),
                                 marker(), op->previewKey(base));
        EXPECT_TRUE(op->execute(doc2)) << "Move Face's fallback computation failed";
        EXPECT_FALSE(isMarker(doc2.getBody(id2))) << "Move Face adopted a stale result";
    }
}

TEST(PreviewAdoption, MoveFaceOneShotConsumptionAndUndo) {
    const TopoDS_Shape base = plate(6, 5);
    Document doc;
    const int id = doc.addBody(base, "plate");
    const TopoDS_Face f = topFace(base);

    auto op = std::make_unique<MoveFaceOp>();
    op->setBody(id);
    op->setFace(f);
    op->setKind(MoveFaceOp::Kind::Translate);
    op->setMoveVector(gp_Vec(2.0, 0.0, 0.0));
    op->setPrecomputedResult(doc.getBody(id), marker(), op->previewKey(base));

    ASSERT_TRUE(op->execute(doc));
    ASSERT_TRUE(isMarker(doc.getBody(id))) << "did not adopt on first execute";

    ASSERT_TRUE(op->undo(doc));
    EXPECT_TRUE(doc.getBody(id).IsEqual(base)) << "undo did not restore the previous shape";
}

TEST(PreviewAdoption, MoveFaceDifferentKindsNeverCrossAdopt) {
    const TopoDS_Shape base = plate(6, 5);
    const TopoDS_Face f = topFace(base);

    auto translate = std::make_unique<MoveFaceOp>();
    translate->setBody(0);
    translate->setFace(f);
    translate->setKind(MoveFaceOp::Kind::Translate);
    translate->setMoveVector(gp_Vec(2.0, 0.0, 0.0));

    auto rotate = std::make_unique<MoveFaceOp>();
    rotate->setBody(0);
    rotate->setFace(f);
    rotate->setKind(MoveFaceOp::Kind::Rotate);
    rotate->setRotation(gp_Dir(1.0, 0.0, 0.0), 2.0); // numerically unrelated to
                                                      // Translate's key on purpose -
                                                      // the kind tag alone must
                                                      // already make these differ
    const std::string keyT = translate->previewKey(base);
    const std::string keyR = rotate->previewKey(base);
    ASSERT_FALSE(keyT.empty());
    ASSERT_FALSE(keyR.empty());
    EXPECT_NE(keyT, keyR) << "Translate and Rotate must never produce the same key";
}

// Table-driven: every field previewKey() reads must actually change the key
// when it changes, for every Kind branch - an omitted field would let a
// changed-but-unkeyed parameter silently adopt a stale shape.
TEST(PreviewAdoption, MoveFacePreviewKeyIsSensitiveToEveryField) {
    const TopoDS_Shape base = plate(6, 5);
    const TopoDS_Face f = topFace(base);
    // A second, distinct face on the same base, for the face-selection case.
    TopoDS_Face f2;
    { double bestZ = -1e300;
      for (TopExp_Explorer e(base, TopAbs_FACE); e.More(); e.Next()) {
          if (TopoDS::Face(e.Current()).IsSame(f)) continue;
          GProp_GProps g; BRepGProp::SurfaceProperties(e.Current(), g);
          if (g.Mass() > bestZ) { bestZ = g.Mass(); f2 = TopoDS::Face(e.Current()); }
      }
      ASSERT_FALSE(f2.IsNull());
    }
    std::vector<bool> hs9(9, false), hv9(9, false);

    auto baseline = [&] {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(0);
        op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Translate);
        op->setMoveVector(gp_Vec(1.0, 0.0, 0.0));
        op->setLoopMotion(true, hs9, hv9);
        return op;
    };
    const std::string baseKey = baseline()->previewKey(base);
    ASSERT_FALSE(baseKey.empty());

    struct Case { const char* name; std::function<void(MoveFaceOp&)> mutate; };
    std::vector<Case> cases = {
        {"move vector", [](MoveFaceOp& op) { op.setMoveVector(gp_Vec(1.0, 0.0, 1e-6)); }},
        {"moveOuter", [&](MoveFaceOp& op) { op.setLoopMotion(false, hs9, hv9); }},
        {"holeSlant", [&](MoveFaceOp& op) {
            std::vector<bool> hs = hs9; hs[0] = true;
            op.setLoopMotion(true, hs, hv9);
        }},
        {"holeVertical", [&](MoveFaceOp& op) {
            std::vector<bool> hv = hv9; hv[0] = true;
            op.setLoopMotion(true, hs9, hv);
        }},
        {"face selection", [&](MoveFaceOp& op) { op.setFace(f2); }},
    };
    for (const auto& c : cases) {
        auto op = baseline();
        c.mutate(*op);
        const std::string k = op->previewKey(base);
        EXPECT_NE(k, baseKey) << "field not reflected in key: " << c.name;
    }

    // Rotate: implicit axis, implicit angle, explicit transform, and the
    // switch between implicit/explicit must all key differently.
    {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(0); op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Rotate);
        op->setRotation(gp_Dir(1, 0, 0), 0.1);
        const std::string k1 = op->previewKey(base);
        op->setRotation(gp_Dir(0, 1, 0), 0.1); // axis changed, angle unchanged
        const std::string k2 = op->previewKey(base);
        EXPECT_NE(k1, k2) << "Rotate axis not reflected in key";
        op->setRotation(gp_Dir(0, 1, 0), 0.2); // angle changed, axis unchanged
        const std::string k3 = op->previewKey(base);
        EXPECT_NE(k2, k3) << "Rotate angle not reflected in key";
        op->setRotationExplicit(gp_Trsf());
        const std::string k4 = op->previewKey(base);
        EXPECT_NE(k3, k4) << "switching implicit/explicit Rotate not reflected in key";
        gp_Trsf t2; t2.SetTranslation(gp_Vec(0, 0, 1));
        op->setRotationExplicit(t2);
        const std::string k5 = op->previewKey(base);
        EXPECT_NE(k4, k5) << "explicit rotation transform not reflected in key";
    }
    // Twist.
    {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(0); op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Twist);
        op->setTwist(0.1);
        const std::string k1 = op->previewKey(base);
        op->setTwist(0.2);
        EXPECT_NE(k1, op->previewKey(base)) << "Twist angle not reflected in key";
    }
    // Scale: uniform factor, the switch to non-uniform, and BOTH non-uniform
    // axes and BOTH non-uniform factors independently.
    {
        auto op = std::make_unique<MoveFaceOp>();
        op->setBody(0); op->setFace(f);
        op->setKind(MoveFaceOp::Kind::Scale);
        op->setScaleFactor(1.1);
        const std::string k1 = op->previewKey(base);
        op->setScaleFactor(1.2);
        const std::string k2 = op->previewKey(base);
        EXPECT_NE(k1, k2) << "uniform scale factor not reflected in key";

        op->setScaleNonUniform(gp_Dir(1, 0, 0), gp_Dir(0, 1, 0), 1.1, 1.3);
        const std::string k3 = op->previewKey(base);
        EXPECT_NE(k2, k3) << "switching uniform/non-uniform scale not reflected in key";

        op->setScaleNonUniform(gp_Dir(0, 0, 1), gp_Dir(0, 1, 0), 1.1, 1.3); // axis A changed
        const std::string k4 = op->previewKey(base);
        EXPECT_NE(k3, k4) << "non-uniform scale axis A not reflected in key";

        op->setScaleNonUniform(gp_Dir(0, 0, 1), gp_Dir(1, 0, 0), 1.1, 1.3); // axis B changed
        const std::string k5 = op->previewKey(base);
        EXPECT_NE(k4, k5) << "non-uniform scale axis B not reflected in key";

        op->setScaleNonUniform(gp_Dir(0, 0, 1), gp_Dir(1, 0, 0), 1.2, 1.3); // factor A changed
        const std::string k6 = op->previewKey(base);
        EXPECT_NE(k5, k6) << "non-uniform scale factor A not reflected in key";

        op->setScaleNonUniform(gp_Dir(0, 0, 1), gp_Dir(1, 0, 0), 1.2, 1.4); // factor B changed
        EXPECT_NE(k6, op->previewKey(base)) << "non-uniform scale factor B not reflected in key";
    }
}

// The prerequisite fix. ScaleFaceOp derives `depth` (and so the fullDepth
// branch) from BRepBndLib, which defaults to using a triangulation when the
// shape carries one. The live body is meshed and the preview worker's copy is
// not, so before this was pinned to geometry the two paths could take
// different branches on identical inputs - a torus bound differs by 5.65 mm.
// Pins a CONTRACT, and does not currently discriminate. Read this before
// trusting it.
//
// Meshing a body must not change what an operation computes from it. The
// fixture is chosen to straddle the fullDepth threshold: a cone measures
// 20.000 deep by geometry and 20.125 through its triangulation, so at length
// 20.05 the old code took a different branch depending on whether the body
// happened to be meshed. Verified by instrumenting the branch.
//
// But this test passes with the fix reverted, because the two branches agree
// wherever they can both apply - a sweep of 123 lengths at three scales on a
// cone and a cylinder found no input where the flip changed the result. It is
// kept as a guard against a future divergence rather than as a regression
// test for a defect that was observed, and it is named for the contract it
// holds rather than for a bug it caught. An earlier version of it was weaker
// still: it used a growing scale, which takes a branch that never reads the
// bounds at all.
TEST(PreviewAdoption, MeshingTheBodyDoesNotChangeWhatScaleFaceComputes) {
    const TopoDS_Shape solid = BRepPrimAPI_MakeCone(10.0, 4.0, 20.0).Shape();
    TopoDS_Shape meshed = BRepBuilderAPI_Copy(solid, Standard_True, Standard_False).Shape();
    BRepMesh_IncrementalMesh(meshed, materializr::meshParams(0.1f, 0.3f, true));

    // topFace() filters by area for the plate fixtures; the cone's small top
    // cap is under that threshold, so pick the highest PLANAR face directly.
    auto capOf = [](const TopoDS_Shape& sh) {
        TopoDS_Face best;
        double bz = -1e9;
        for (TopExp_Explorer e(sh, TopAbs_FACE); e.More(); e.Next()) {
            TopoDS_Face f = TopoDS::Face(e.Current());
            if (Handle(Geom_Plane)::DownCast(BRep_Tool::Surface(f)).IsNull()) continue;
            GProp_GProps g;
            BRepGProp::SurfaceProperties(f, g);
            if (g.CentreOfMass().Z() > bz) { bz = g.CentreOfMass().Z(); best = f; }
        }
        return best;
    };
    auto run = [&](const TopoDS_Shape& body) {
        Document doc;
        const int id = doc.addBody(body, "b");
        auto op = std::make_unique<ScaleFaceOp>();
        op->setBody(id);
        op->setFace(capOf(body));
        op->setScaleUV(80, 80);        // shrink: reaches the fullDepth branch
        op->setLength(20.05);          // between the two measured depths
        op->setMode(ScaleFaceOp::Mode::Pinch);
        const bool ok = op->execute(doc);
        return std::make_pair(ok, ok ? volumeOf(doc.getBody(id)) : 0.0);
    };
    const auto fromGeometry = run(solid);
    const auto fromMeshed = run(meshed);
    ASSERT_TRUE(fromGeometry.first) << "the fixture itself failed to execute";
    ASSERT_EQ(fromGeometry.first, fromMeshed.first)
        << "meshing the body changed whether the operation succeeded";
    EXPECT_NEAR(fromGeometry.second, fromMeshed.second, 1e-6)
        << "meshing the body changed the result, so the depth bound is still "
           "triangulation-dependent and preview and commit can disagree";
}

// The worker computes its key against its own deep copy of the body, and the
// commit computes one against the live body. If BRepBuilderAPI_Copy did not
// preserve sub-shape order, those indices would disagree and every adoption
// would silently miss - the same class of failure as getting the base wrong,
// and just as invisible.
TEST(PreviewAdoption, TheKeySurvivesTheWorkersDeepCopy) {
    const TopoDS_Shape base = plate(6, 5);
    const TopoDS_Shape copy =
        BRepBuilderAPI_Copy(base, Standard_True, Standard_False).Shape();
    ASSERT_FALSE(copy.IsEqual(base));

    // The same face, as the copier hands it back.
    TopTools_IndexedMapOfShape subs;
    TopExp::MapShapes(base, subs);
    BRepBuilderAPI_Copy copier(base, Standard_True, Standard_False);
    const TopoDS_Face f = topFace(base);
    const TopoDS_Shape onCopy = copier.ModifiedShape(f).Oriented(f.Orientation());

    auto live = shellOp(1, f);
    auto worker = shellOp(1, TopoDS::Face(onCopy));
    const std::string liveKey = live->previewKey(base);
    const std::string workerKey = worker->previewKey(copier.Shape());
    ASSERT_FALSE(liveKey.empty());
    ASSERT_FALSE(workerKey.empty());
    EXPECT_EQ(liveKey, workerKey)
        << "the worker's key does not match the live one, so no adoption "
           "could ever fire: live=" << liveKey << " worker=" << workerKey;
}

TEST(PreviewAdoption, TheKeyCarriesOrientation) {
    const TopoDS_Shape base = plate(6, 5);
    const TopoDS_Face f = topFace(base);
    TopoDS_Shape reversed = f.Reversed();

    TopoDS_Shape internal = f.Oriented(TopAbs_INTERNAL);
    TopoDS_Shape external = f.Oriented(TopAbs_EXTERNAL);

    std::string a, b, c, d;
    ASSERT_TRUE(SubShapeIndex::orientedKey(base, {f}, TopAbs_FACE, a));
    ASSERT_TRUE(SubShapeIndex::orientedKey(base, {reversed}, TopAbs_FACE, b));
    ASSERT_TRUE(SubShapeIndex::orientedKey(base, {internal}, TopAbs_FACE, c));
    ASSERT_TRUE(SubShapeIndex::orientedKey(base, {external}, TopAbs_FACE, d));
    // All four distinct: encoding only REVERSED would collapse INTERNAL and
    // EXTERNAL onto FORWARD.
    std::set<std::string> keys{a, b, c, d};
    EXPECT_EQ(keys.size(), 4u)
        << "orientations collapsed: " << a << " " << b << " " << c << " " << d;
}
