// Mates live on the Document the way constraints live on a Sketch: stored
// with the model, not replayed as history steps.

#include "core/Document.h"
#include "modeling/Mate.h"
#include "modeling/FaceAnchor.h"
#include "io/ProjectIO.h"
#include "test_tmp_path.h"

#include <gtest/gtest.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <fstream>
#include <iterator>
#include <zlib.h>

namespace {
// Project files are gzip-compressed (ProjectIO.cpp's gzipDeflate/gunzipInflate,
// not exposed via the header) - a test that wants to hand-edit the on-disk
// text has to speak the same wire format itself. Same window bits as the
// production code so the bytes this test writes are what a real loader reads.
std::string gunzip(const std::string& src) {
    z_stream zs{};
    if (inflateInit2(&zs, 15 + 32) != Z_OK) return {};
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(src.data()));
    zs.avail_in = static_cast<uInt>(src.size());
    std::string out;
    char buf[1 << 15];
    int ret;
    do {
        zs.next_out = reinterpret_cast<Bytef*>(buf);
        zs.avail_out = sizeof(buf);
        ret = inflate(&zs, Z_NO_FLUSH);
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret == Z_OK);
    inflateEnd(&zs);
    return (ret == Z_STREAM_END) ? out : std::string{};
}
std::string gzip(const std::string& src) {
    z_stream zs{};
    if (deflateInit2(&zs, 6, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) return {};
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(src.data()));
    zs.avail_in = static_cast<uInt>(src.size());
    std::string out;
    char buf[1 << 15];
    int ret;
    do {
        zs.next_out = reinterpret_cast<Bytef*>(buf);
        zs.avail_out = sizeof(buf);
        ret = deflate(&zs, Z_FINISH);
        out.append(buf, sizeof(buf) - zs.avail_out);
    } while (ret == Z_OK);
    deflateEnd(&zs);
    return (ret == Z_STREAM_END) ? out : std::string{};
}
} // namespace

using materializr::Mate;
using materializr::MateType;

namespace {
Mate makeFasten(int a, int b, double offset) {
    Mate m{};
    m.type = MateType::Fasten;
    m.bodyA = a;
    m.bodyB = b;
    m.offset = offset;
    return m;
}
} // namespace

TEST(MateStore, AddReturnsUniqueIdsAndStores) {
    Document doc;
    int a = doc.addBody(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape(), "A");
    int b = doc.addBody(BRepPrimAPI_MakeBox(4.0, 4.0, 4.0).Shape(), "B");

    int m1 = doc.addMate(makeFasten(a, b, 2.0));
    int m2 = doc.addMate(makeFasten(b, a, 3.0));

    EXPECT_NE(m1, m2);
    ASSERT_EQ(doc.getMates().size(), 2u);
    EXPECT_EQ(doc.getMates()[0].id, m1);
    EXPECT_DOUBLE_EQ(doc.getMates()[0].offset, 2.0);
}

TEST(MateStore, RemoveDropsOnlyTheNamedMate) {
    Document doc;
    int a = doc.addBody(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape(), "A");
    int b = doc.addBody(BRepPrimAPI_MakeBox(4.0, 4.0, 4.0).Shape(), "B");

    int m1 = doc.addMate(makeFasten(a, b, 2.0));
    int m2 = doc.addMate(makeFasten(b, a, 3.0));
    doc.removeMate(m1);

    ASSERT_EQ(doc.getMates().size(), 1u);
    EXPECT_EQ(doc.getMates()[0].id, m2);
}

TEST(MateStore, GroundedBodyDefaultsToNoneAndIsSettable) {
    Document doc;
    EXPECT_EQ(doc.getGroundedBody(), -1);
    int a = doc.addBody(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape(), "A");
    doc.setGroundedBody(a);
    EXPECT_EQ(doc.getGroundedBody(), a);
}


TEST(MatePersistence, RoundTripsThroughProjectFile) {
    Document doc;
    int a = doc.addBody(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape(), "A");
    int b = doc.addBody(BRepPrimAPI_MakeBox(4.0, 4.0, 4.0).Shape(), "B");
    doc.setGroundedBody(a);

    Mate m = makeFasten(a, b, 7.5);
    m.angle = 0.25;
    m.flipped = true;
    int mid = doc.addMate(m);

    std::string path = mzrtest::tmpPath("mate_roundtrip.mzr");
    ASSERT_TRUE(materializr::ProjectIO::save(path, doc).success);

    Document loaded;
    ASSERT_TRUE(materializr::ProjectIO::load(path, loaded).success);

    ASSERT_EQ(loaded.getMates().size(), 1u);
    const Mate& r = loaded.getMates()[0];
    EXPECT_EQ(r.id, mid);
    EXPECT_EQ(r.type, MateType::Fasten);
    EXPECT_EQ(r.bodyA, a);
    EXPECT_EQ(r.bodyB, b);
    EXPECT_DOUBLE_EQ(r.offset, 7.5);
    EXPECT_DOUBLE_EQ(r.angle, 0.25);
    EXPECT_TRUE(r.flipped);
    EXPECT_EQ(loaded.getGroundedBody(), a);
}

// RoundTripsThroughProjectFile above uses a Fasten mate with EMPTY anchors
// and hasRelPose left at its false default - the anchor blob ("aa"/"ab"
// tokens) and the hasRelPose/relX/relY/relZ trailing fields are only ever
// written and read in their empty/absent form there, leaving the two
// hardest parts of the format's own round trip completely unasserted (a
// review-panel test-coverage finding, 2026-09). This test exercises both.
TEST(MatePersistence, RoundTripsAnchorsAndRelativePose) {
    Document doc;
    int a = doc.addBody(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape(), "A");
    int b = doc.addBody(BRepPrimAPI_MakeBox(4.0, 4.0, 4.0).Shape(), "B");
    doc.setGroundedBody(a);

    Mate m = makeFasten(a, b, 3.0);
    FaceAnchor::Anchor anchorA;
    anchorA.kind = FaceAnchor::Anchor::Wall;
    anchorA.sketchId = 11; anchorA.elemId = 22;
    anchorA.h = 1.5; anchorA.cu = 2.5; anchorA.cv = -3.5;
    FaceAnchor::Anchor anchorB;
    anchorB.kind = FaceAnchor::Anchor::Cyl;
    anchorB.sketchId = 33; anchorB.elemId = 44;
    anchorB.h = -0.5; anchorB.cu = 0.25; anchorB.cv = 0.75;
    m.anchorsA.push_back(anchorA);
    m.anchorsB.push_back(anchorB);
    m.hasRelPose = true;
    m.relX = 1.1; m.relY = -2.2; m.relZ = 3.3;
    int mid = doc.addMate(m);

    std::string path = mzrtest::tmpPath("mate_anchors_roundtrip.mzr");
    ASSERT_TRUE(materializr::ProjectIO::save(path, doc).success);

    Document loaded;
    ASSERT_TRUE(materializr::ProjectIO::load(path, loaded).success);

    ASSERT_EQ(loaded.getMates().size(), 1u);
    const Mate& r = loaded.getMates()[0];
    EXPECT_EQ(r.id, mid);
    ASSERT_EQ(r.anchorsA.size(), 1u);
    EXPECT_EQ(r.anchorsA[0].kind, FaceAnchor::Anchor::Wall);
    EXPECT_EQ(r.anchorsA[0].sketchId, 11);
    EXPECT_EQ(r.anchorsA[0].elemId, 22);
    EXPECT_DOUBLE_EQ(r.anchorsA[0].h, 1.5);
    EXPECT_DOUBLE_EQ(r.anchorsA[0].cu, 2.5);
    EXPECT_DOUBLE_EQ(r.anchorsA[0].cv, -3.5);
    ASSERT_EQ(r.anchorsB.size(), 1u);
    EXPECT_EQ(r.anchorsB[0].kind, FaceAnchor::Anchor::Cyl);
    EXPECT_EQ(r.anchorsB[0].sketchId, 33);
    EXPECT_EQ(r.anchorsB[0].elemId, 44);
    EXPECT_TRUE(r.hasRelPose);
    EXPECT_DOUBLE_EQ(r.relX, 1.1);
    EXPECT_DOUBLE_EQ(r.relY, -2.2);
    EXPECT_DOUBLE_EQ(r.relZ, 3.3);
    EXPECT_FALSE(r.broken) << "a valid, well-formed anchor blob must not "
                              "load as broken just because it doesn't "
                              "resolve to a real sketch in this fixture";
}

// An overstated MATE_COUNT reads one line past the real mate lines. That line
// is CPLANE_COUNT (the next section written) - if the loader kept consuming
// (`continue`) instead of stopping (`break`) on the mismatch, CPLANE_COUNT
// itself would be swallowed as a bogus mate line and every construction plane
// after it would silently vanish on load (review-panel finding, 2026-09).
TEST(MatePersistence, OverstatedMateCountDoesNotSwallowTheNextSection) {
    Document doc;
    int a = doc.addBody(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape(), "A");
    int b = doc.addBody(BRepPrimAPI_MakeBox(4.0, 4.0, 4.0).Shape(), "B");
    doc.addMate(makeFasten(a, b, 5.0));
    doc.addPlane(gp_Pln(gp_Pnt(0, 0, 7), gp_Dir(0, 0, 1)), "TestPlane");

    std::string path = mzrtest::tmpPath("mate_overstated_count.mzr");
    ASSERT_TRUE(materializr::ProjectIO::save(path, doc).success);

    // Bump "MATE_COUNT 1" to "MATE_COUNT 2" with no second mate line added,
    // so the loader's line right after the one real mate is CPLANE_COUNT.
    // The file is gzip-compressed on disk, so round-trip through the same
    // wire format the loader expects.
    std::ifstream in(path, std::ios::binary);
    std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    std::string contents = gunzip(raw);
    ASSERT_FALSE(contents.empty()) << "could not inflate the saved project";
    const std::string from = "MATE_COUNT 1\n";
    const size_t pos = contents.find(from);
    ASSERT_NE(pos, std::string::npos) << "test fixture assumption about the save format changed";
    contents.replace(pos, from.size(), "MATE_COUNT 2\n");
    std::string recompressed = gzip(contents);
    ASSERT_FALSE(recompressed.empty()) << "could not deflate the edited project";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << recompressed;
    out.close();

    Document loaded;
    ASSERT_TRUE(materializr::ProjectIO::load(path, loaded).success);
    ASSERT_EQ(loaded.getMates().size(), 1u) << "the one real mate must still load";
    ASSERT_EQ(loaded.getAllPlaneIds().size(), 1u)
        << "CPLANE_COUNT must not be swallowed as a bogus mate line";
}

TEST(MatePersistence, FileWithNoMateBlockLoadsClean) {
    // Every project written before mates existed has no MATE_COUNT block. It
    // must load with zero mates and no grounded body rather than failing.
    Document doc;
    doc.addBody(BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape(), "A");
    std::string path = mzrtest::tmpPath("mate_legacy.mzr");
    ASSERT_TRUE(materializr::ProjectIO::save(path, doc).success);

    Document loaded;
    ASSERT_TRUE(materializr::ProjectIO::load(path, loaded).success);
    EXPECT_TRUE(loaded.getMates().empty());
    EXPECT_EQ(loaded.getGroundedBody(), -1);
}
