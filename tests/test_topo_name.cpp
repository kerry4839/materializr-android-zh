// Unit tests for the unified sub-shape naming layer (topo::Ref / registry).
// These pin the "reach full coverage without backtracking" properties:
//   - a Ref carries multiple names and resolves best-first,
//   - an UNKNOWN scheme (a future namer, read by an older build) is skipped on
//     resolve and PRESERVED on serialize round-trip,
//   - the registry is priority-sorted so mint() emits names best-first.
// Uses a plain OCCT box + the always-available "ordinal" scheme, so it needs
// no sketch/body plumbing.

#include "modeling/TopoName.h"

#include <gtest/gtest.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <vector>

using namespace materializr;

static std::vector<TopoDS_Face> facesOf(const TopoDS_Shape& s) {
    std::vector<TopoDS_Face> f;
    for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
        f.push_back(TopoDS::Face(ex.Current()));
    return f;
}

TEST(TopoName, OrdinalMintResolveRoundTrip) {
    TopoDS_Shape box = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape();
    auto faces = facesOf(box);
    ASSERT_GE(faces.size(), 6u);
    topo::Context ctx; ctx.shape = box; ctx.type = TopAbs_FACE;

    topo::Ref ref = topo::mint(faces[2], ctx);
    ASSERT_FALSE(ref.empty());
    EXPECT_EQ(ref.names.back().scheme, "ordinal");  // no doc -> ordinal only

    TopoDS_Shape out;
    ASSERT_TRUE(topo::resolve(ref, ctx, out));
    EXPECT_TRUE(out.IsSame(faces[2]));

    // serialize -> parse -> resolve lands the same face.
    topo::Ref rt = topo::Ref::parse(ref.serialize());
    TopoDS_Shape out2;
    ASSERT_TRUE(topo::resolve(rt, ctx, out2));
    EXPECT_TRUE(out2.IsSame(faces[2]));
}

TEST(TopoName, SkipsUnknownSchemeAndFallsThrough) {
    // An older build reading a file whose sub-shape was named by a FUTURE
    // scheme ("gen") must skip it and resolve via the ordinal fallback.
    TopoDS_Shape box = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape();
    auto faces = facesOf(box);
    topo::Context ctx; ctx.shape = box; ctx.type = TopAbs_FACE;

    topo::Ref ordRef = topo::mint(faces[3], ctx);
    ASSERT_FALSE(ordRef.empty());
    const std::string ordPayload = ordRef.names.back().payload;

    topo::Ref ref;
    ref.names.push_back({ "gen", "lineage://future/unresolvable" });
    ref.names.push_back({ "ordinal", ordPayload });

    TopoDS_Shape out;
    ASSERT_TRUE(topo::resolve(ref, ctx, out));
    EXPECT_TRUE(out.IsSame(faces[3]));

    // The unknown "gen" name round-trips untouched (forward compatibility).
    topo::Ref rt = topo::Ref::parse(ref.serialize());
    ASSERT_EQ(rt.names.size(), 2u);
    EXPECT_EQ(rt.names[0].scheme, "gen");
    EXPECT_EQ(rt.names[0].payload, "lineage://future/unresolvable");
    EXPECT_EQ(rt.names[1].scheme, "ordinal");
}

TEST(TopoName, ResolveTriesNamesBestFirst) {
    TopoDS_Shape box = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape();
    auto faces = facesOf(box);
    topo::Context ctx; ctx.shape = box; ctx.type = TopAbs_FACE;

    // Two resolvable names; the one listed FIRST wins.
    topo::Ref ref;
    ref.names.push_back(topo::mint(faces[1], ctx).names.back());
    ref.names.push_back(topo::mint(faces[4], ctx).names.back());

    TopoDS_Shape out;
    ASSERT_TRUE(topo::resolve(ref, ctx, out));
    EXPECT_TRUE(out.IsSame(faces[1]));
}

TEST(TopoName, EmptyPayloadsPreservedAndParsedSafely) {
    // Round-trip of a payload containing the delimiter and an empty payload.
    topo::Ref ref;
    ref.names.push_back({ "weird", "has:colons:and 12:34 numbers" });
    ref.names.push_back({ "empty", "" });
    topo::Ref rt = topo::Ref::parse(ref.serialize());
    ASSERT_EQ(rt.names.size(), 2u);
    EXPECT_EQ(rt.names[0].scheme, "weird");
    EXPECT_EQ(rt.names[0].payload, "has:colons:and 12:34 numbers");
    EXPECT_EQ(rt.names[1].scheme, "empty");
    EXPECT_EQ(rt.names[1].payload, "");
}

TEST(TopoName, RegistryIsPrioritySorted) {
    // mint() emits names best-first because the registry is priority-sorted:
    // the generative sketchface scheme must precede the ordinal fallback.
    const auto& ss = topo::Registry::instance().strategies();
    int iFace = -1, iOrd = -1;
    for (int i = 0; i < static_cast<int>(ss.size()); ++i) {
        if (ss[i].scheme == "sketchface") iFace = i;
        if (ss[i].scheme == "ordinal")    iOrd = i;
    }
    ASSERT_GE(iFace, 0);
    ASSERT_GE(iOrd, 0);
    EXPECT_LT(iFace, iOrd);
}
