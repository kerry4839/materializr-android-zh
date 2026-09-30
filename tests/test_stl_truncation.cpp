// An STL that only partly parses must FAIL, not import a fragment.
//
// RWStl::ReadFile does not fail on a malformed ASCII facet: it writes a
// complaint to the OCCT message channel, stops, and returns what it read so
// far. A real file written by assimp puts a blank line between every endfacet
// and the next facet, which that reader rejects - a 26988-facet model came
// back as ONE triangle, was sewn into a zero-volume sliver, and reported
// success. The viewport looked empty and nothing said why.
#include "core/Document.h"
#include "io/StlIO.h"

#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::string tmpPath(const char* name) {
    // Not a hardcoded /tmp: Windows CI runs this suite and has no such path.
    return (std::filesystem::temp_directory_path() /
            (std::string("materializr_stl_") + name + ".stl")).string();
}

// A stack of facets. `gap` inserts the blank line that trips the reader;
// without it the same content is well formed.
//
// Deliberately 40 facets, not two or three: the guard tolerates a small
// shortfall, because RWStl legitimately drops zero-area facets. A fixture too
// small to exceed that allowance would not exercise the guard at all.
std::string asciiStl(bool gap, int facets = 40) {
    const std::string sep = gap ? "\n" : "";
    std::string s = "solid test\n";
    for (int i = 0; i < facets; ++i) {
        s += " facet normal 0 0 1\n"
             "  outer loop\n";
        s += "  vertex 0 0 " + std::to_string(i) + "\n";
        s += "  vertex 1 0 " + std::to_string(i) + "\n";
        s += "  vertex 0 1 " + std::to_string(i) + "\n";
        s += "  endloop\n"
             " endfacet\n";
        s += sep;
    }
    s += "endsolid test\n";
    return s;
}

// Writes the fixture and PROVES it landed. Without this, a sandbox that
// denies /tmp writes makes the failure-path tests pass for the wrong reason:
// the import fails because the file is missing, not because it is malformed.
void write(const std::string& path, const std::string& body) {
    {
        std::ofstream f(path, std::ios::binary);
        f << body;
    }
    std::ifstream check(path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(check.good()) << "could not write the fixture at " << path
                              << " - the test would otherwise pass vacuously";
    ASSERT_EQ(static_cast<std::size_t>(check.tellg()), body.size())
        << "the fixture at " << path << " is short";
}

} // namespace

TEST(StlTruncation, BlankLinesBetweenFacetsFailRatherThanImportAFragment) {
    const std::string path = tmpPath("gapped");
    ASSERT_NO_FATAL_FAILURE(write(path, asciiStl(/*gap=*/true)));

    Document doc;
    materializr::ImportResult r = materializr::StlIO::import(path, doc, 0.5);

    EXPECT_FALSE(r.success)
        << "a file the reader could only partly parse was imported anyway";
    EXPECT_NE(r.errorMessage.find("of 40"), std::string::npos)
        << "the failure came from the pre-existing empty-mesh path, not the "
           "truncation guard: " << r.errorMessage;
    EXPECT_TRUE(doc.getAllBodyIds().empty())
        << "a fragment was left in the document";
    std::remove(path.c_str());
}

// The guard must not fire on a file that is merely small.
TEST(StlTruncation, AWellFormedAsciiFileStillImports) {
    const std::string path = tmpPath("clean");
    ASSERT_NO_FATAL_FAILURE(write(path, asciiStl(/*gap=*/false)));

    Document doc;
    materializr::ImportResult r = materializr::StlIO::import(path, doc, 0.5);

    EXPECT_TRUE(r.success) << "a valid file was rejected: " << r.errorMessage;
    EXPECT_EQ(doc.getAllBodyIds().size(), 1u);
    std::remove(path.c_str());
}

// The guard must tolerate what the reader legitimately drops. RWStl discards
// zero-area facets, which real scanner and CAD exports routinely contain, and
// rejecting those files would be a worse bug than the one being fixed.
TEST(StlTruncation, ADegenerateFacetDoesNotFailTheWholeFile) {
    const std::string path = tmpPath("degen");
    std::string body = "solid test\n";
    for (int i = 0; i < 40; ++i) {
        body += " facet normal 0 0 1\n  outer loop\n";
        body += "  vertex 0 0 " + std::to_string(i) + "\n";
        body += "  vertex 1 0 " + std::to_string(i) + "\n";
        body += "  vertex 0 1 " + std::to_string(i) + "\n";
        body += "  endloop\n endfacet\n";
    }
    // One the reader will throw away.
    body += " facet normal 0 0 1\n  outer loop\n"
            "  vertex 5 5 5\n  vertex 5 5 5\n  vertex 5 5 5\n"
            "  endloop\n endfacet\n";
    body += "endsolid test\n";
    ASSERT_NO_FATAL_FAILURE(write(path, body));

    Document doc;
    materializr::ImportResult r = materializr::StlIO::import(path, doc, 0.5);
    EXPECT_TRUE(r.success)
        << "a valid file was rejected for one degenerate facet: " << r.errorMessage;
    // Without this the test could pass because the reader kept all 41, which
    // would mean the allowance was never exercised at all.
    EXPECT_EQ(r.trianglesBefore, 40)
        << "the reader did not discard the degenerate facet, so this test did "
           "not exercise the shortfall allowance";
    std::remove(path.c_str());
}

// Codex's case against the first version of this guard: a valid file with
// ENOUGH degenerate facets. A percentage allowance rejected 40 good plus 9
// zero-area, because 40 < 49 - 8. The bar is now half the declared count, so
// legitimate drops cannot fail a file no matter how many there are.
TEST(StlTruncation, ManyDegenerateFacetsStillImport) {
    const std::string path = tmpPath("manydegen");
    std::string body = "solid test\n";
    for (int i = 0; i < 40; ++i) {
        body += " facet normal 0 0 1\n  outer loop\n";
        body += "  vertex 0 0 " + std::to_string(i) + "\n";
        body += "  vertex 1 0 " + std::to_string(i) + "\n";
        body += "  vertex 0 1 " + std::to_string(i) + "\n";
        body += "  endloop\n endfacet\n";
    }
    for (int i = 0; i < 9; ++i)
        body += " facet normal 0 0 1\n  outer loop\n"
                "  vertex 5 5 5\n  vertex 5 5 5\n  vertex 5 5 5\n"
                "  endloop\n endfacet\n";
    body += "endsolid test\n";
    ASSERT_NO_FATAL_FAILURE(write(path, body));

    Document doc;
    materializr::ImportResult r = materializr::StlIO::import(path, doc, 0.5);
    EXPECT_TRUE(r.success)
        << "a valid file was rejected for carrying 9 degenerate facets: "
        << r.errorMessage;
    EXPECT_EQ(r.trianglesBefore, 40)
        << "the reader kept facets it was expected to drop, so the allowance "
           "was not exercised";
    std::remove(path.c_str());
}

// Upper case is a real ASCII STL - OCCT's reader accepts either case. The
// first version of the detector tested for a literal "solid" prefix, so an
// upper-case file was taken for binary and skipped validation completely: the
// truncation bug this guard exists for went straight through.
TEST(StlTruncation, UpperCaseAsciiIsStillValidated) {
    const std::string path = tmpPath("upper");
    std::string body = "SOLID t\n";
    for (int i = 0; i < 40; ++i) {
        body += " FACET NORMAL 0 0 1\n  OUTER LOOP\n";
        body += "  VERTEX 0 0 " + std::to_string(i) + "\n";
        body += "  VERTEX 1 0 " + std::to_string(i) + "\n";
        body += "  VERTEX 0 1 " + std::to_string(i) + "\n";
        body += "  ENDLOOP\n ENDFACET\n\n";   // the blank line that truncates
    }
    body += "ENDSOLID t\n";
    ASSERT_NO_FATAL_FAILURE(write(path, body));

    Document doc;
    materializr::ImportResult r = materializr::StlIO::import(path, doc, 0.5);
    EXPECT_FALSE(r.success) << "an upper-case file bypassed the guard";
    EXPECT_NE(r.errorMessage.find("of 40"), std::string::npos) << r.errorMessage;
    std::remove(path.c_str());
}

// A single ASCII file can hold several solid/endsolid blocks. The facet count
// spans all of them, so if the reader stopped at the first endsolid the guard
// would reject a perfectly good file. Measured: it reads on and returns every
// facet - RWStl's single-triangulation reader does not override AddSolid, so
// the blocks accumulate.
TEST(StlTruncation, MultipleSolidBlocksInOneFileStillImport) {
    const std::string path = tmpPath("multisolid");
    std::string body;
    for (int b = 0; b < 3; ++b) {
        body += "solid part\n";
        for (int i = 0; i < 20; ++i) {
            const int z = b * 100 + i;
            body += " facet normal 0 0 1\n  outer loop\n";
            body += "  vertex 0 0 " + std::to_string(z) + "\n";
            body += "  vertex 1 0 " + std::to_string(z) + "\n";
            body += "  vertex 0 1 " + std::to_string(z) + "\n";
            body += "  endloop\n endfacet\n";
        }
        body += "endsolid part\n";
    }
    ASSERT_NO_FATAL_FAILURE(write(path, body));

    Document doc;
    materializr::ImportResult r = materializr::StlIO::import(path, doc, 0.5);
    EXPECT_TRUE(r.success)
        << "a multi-block file was rejected: " << r.errorMessage;
    EXPECT_EQ(r.trianglesBefore, 60)
        << "the reader did not return every block's facets, so the guard is "
           "comparing a whole-file count against a partial read";
    std::remove(path.c_str());
}

// Binary STL declares its own count in the header, so a file whose body is
// short of that count is detectable the same way.
// A binary file whose BODY ends early. Distinct from a merely stale header
// count, which the next test covers: OCCT's reader carries a comment that the
// header number "is sometimes wrong", so the body length is the ground truth
// for what the file actually holds.
TEST(StlTruncation, BinaryWhoseBodyEndsEarlyIsRejected) {
    const std::string path = tmpPath("shortbin");
    // 80 REAL triangles behind a header claiming 200. Deliberately not two or
    // three: with a tiny body the reader returns nothing and the pre-existing
    // empty-mesh check fires, so the test would pass without the truncation
    // guard existing at all.
    const int declared = 200, present = 80;
    {
        std::ofstream f(path, std::ios::binary);
        std::vector<char> header(80, 0);
        f.write(header.data(), 80);
        const unsigned char n[4] = {static_cast<unsigned char>(declared), 0, 0, 0};
        f.write(reinterpret_cast<const char*>(n), 4);
        for (int t = 0; t < present; ++t) {
            float tri[12] = {0, 0, 1,  0, 0, float(t),  1, 0, float(t),  0, 1, float(t)};
            f.write(reinterpret_cast<const char*>(tri), sizeof(tri));
            const unsigned short attr = 0;
            f.write(reinterpret_cast<const char*>(&attr), 2);
        }
    }
    {
        std::ifstream check(path, std::ios::binary | std::ios::ate);
        ASSERT_TRUE(check.good()) << "could not write the binary fixture";
        ASSERT_EQ(static_cast<long>(check.tellg()), 84 + present * 50);
    }
    Document doc;
    materializr::ImportResult r = materializr::StlIO::import(path, doc, 0.5);
    EXPECT_FALSE(r.success) << "a truncated binary file was imported anyway";
    EXPECT_NE(r.errorMessage.find("ends before"), std::string::npos)
        << "the failure did not come from the short-body check: "
        << r.errorMessage;
    std::remove(path.c_str());
}

// The mirror case, and the one a naive guard gets wrong: a COMPLETE binary
// file whose header count is stale. Every facet is present, the reader returns
// them all, and rejecting it would fail a perfectly good file. OCCT documents
// the header count as untrustworthy for exactly this reason.
TEST(StlTruncation, BinaryWithAStaleHeaderCountButAWholeBodyImports) {
    const std::string path = tmpPath("stalehdr");
    const int present = 80;
    {
        std::ofstream f(path, std::ios::binary);
        std::vector<char> header(80, 0);
        f.write(header.data(), 80);
        // Header UNDER-states: 40 declared, 80 actually present.
        const unsigned char n[4] = {40, 0, 0, 0};
        f.write(reinterpret_cast<const char*>(n), 4);
        for (int t = 0; t < present; ++t) {
            float tri[12] = {0, 0, 1,  0, 0, float(t),  1, 0, float(t),  0, 1, float(t)};
            f.write(reinterpret_cast<const char*>(tri), sizeof(tri));
            const unsigned short attr = 0;
            f.write(reinterpret_cast<const char*>(&attr), 2);
        }
    }
    Document doc;
    materializr::ImportResult r = materializr::StlIO::import(path, doc, 0.5);
    EXPECT_TRUE(r.success)
        << "a complete file was rejected over its header count: " << r.errorMessage;
    std::remove(path.c_str());
}
