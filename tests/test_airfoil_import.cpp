// Airfoil coordinate import: the two dialects in circulation, the blunt
// trailing edge, decimation, and placement into a sketch.
//
// The fixtures are generated NACA 4-digit sections rather than copied files,
// because the analytic form gives an EXACT expected answer: a NACA 0012 is
// 12% thick with zero camber, a 2412 has 2% camber at 40% chord. A parser that
// silently swaps surfaces, drops the nose or mis-splits the loop cannot
// reproduce those numbers.

#include "modeling/AirfoilImport.h"
#include "modeling/Sketch.h"

#include <gtest/gtest.h>
#include <TopoDS_Wire.hxx>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

using materializr::AirfoilImport;
using materializr::AirfoilProfile;
using materializr::Sketch;

namespace {

// NACA 4-digit half-thickness. The last coefficient decides the trailing edge:
// -0.1015 is the original (blunt, ~0.25% gap at t=12%), -0.1036 closes it.
double yt(double x, double t, bool sharpTE) {
    const double c4 = sharpTE ? -0.1036 : -0.1015;
    return 5.0 * t * (0.2969 * std::sqrt(x) - 0.1260 * x - 0.3516 * x * x
                      + 0.2843 * x * x * x + c4 * x * x * x * x);
}
// Camber line and slope for m (max camber) at p (its chord position).
double yc(double x, double m, double p) {
    if (m <= 0.0) return 0.0;
    return (x < p) ? m / (p * p) * (2 * p * x - x * x)
                   : m / ((1 - p) * (1 - p)) * ((1 - 2 * p) + 2 * p * x - x * x);
}
double dyc(double x, double m, double p) {
    if (m <= 0.0) return 0.0;
    return (x < p) ? 2 * m / (p * p) * (p - x)
                   : 2 * m / ((1 - p) * (1 - p)) * (p - x);
}

struct Surfaces { std::vector<std::pair<double,double>> up, lo; };

// Cosine-spaced stations: dense at the nose, like a real coordinate file.
Surfaces naca(double t, double m, double p, int n, bool sharpTE) {
    Surfaces s;
    for (int i = 0; i <= n; ++i) {
        const double beta = M_PI * i / n;
        const double x = 0.5 * (1.0 - std::cos(beta));
        const double th = std::atan(dyc(x, m, p));
        const double c = yc(x, m, p), h = yt(x, t, sharpTE);
        s.up.push_back({x - h * std::sin(th), c + h * std::cos(th)});
        s.lo.push_back({x + h * std::sin(th), c - h * std::cos(th)});
    }
    return s;
}

// Selig: one loop, trailing edge -> over the top -> nose -> under -> trailing.
std::string seligText(const Surfaces& s, const char* name) {
    std::ostringstream o;
    o.precision(6);
    o << std::fixed;
    if (name) o << name << "\n";
    for (std::size_t i = s.up.size(); i-- > 0;) o << s.up[i].first << " " << s.up[i].second << "\n";
    for (std::size_t i = 1; i < s.lo.size(); ++i) o << s.lo[i].first << " " << s.lo[i].second << "\n";
    return o.str();
}

// Lednicer: a count line, then each surface nose -> trailing edge.
std::string lednicerText(const Surfaces& s, const char* name) {
    std::ostringstream o;
    o.precision(6);
    o << std::fixed;
    o << name << "\n";
    o << "       " << s.up.size() << ".      " << s.lo.size() << ".\n\n";
    for (const auto& p : s.up) o << p.first << " " << p.second << "\n";
    o << "\n";
    for (const auto& p : s.lo) o << p.first << " " << p.second << "\n";
    return o.str();
}

} // namespace

TEST(AirfoilImport, ParsesASeligSymmetricSection) {
    const std::string txt = seligText(naca(0.12, 0, 0, 80, true), "NACA 0012");
    AirfoilProfile p;
    std::string err;
    ASSERT_TRUE(AirfoilImport::parse(txt, p, &err)) << err;
    EXPECT_EQ(p.name, "NACA 0012");
    // Both surfaces run leading edge -> trailing edge.
    EXPECT_NEAR(p.upper.front().x, 0.0f, 1e-3f);
    EXPECT_NEAR(p.lower.front().x, 0.0f, 1e-3f);
    EXPECT_NEAR(p.upper.back().x, 1.0f, 1e-3f);
    EXPECT_NEAR(p.lower.back().x, 1.0f, 1e-3f);
    // 0012 = 12% thick, no camber.
    EXPECT_NEAR(p.thickness(), 0.12f, 0.005f);
    EXPECT_NEAR(p.camber(), 0.0f, 0.002f);
    EXPECT_FALSE(p.bluntTrailingEdge);
}

TEST(AirfoilImport, ParsesACamberedSection) {
    const std::string txt = seligText(naca(0.12, 0.02, 0.4, 80, true), "NACA 2412");
    AirfoilProfile p;
    ASSERT_TRUE(AirfoilImport::parse(txt, p));
    EXPECT_NEAR(p.thickness(), 0.12f, 0.006f);
    EXPECT_NEAR(p.camber(), 0.02f, 0.004f);
    // The upper surface must actually be the upper one.
    float upMax = -1e9f, loMin = 1e9f;
    for (auto& q : p.upper) upMax = std::max(upMax, q.y);
    for (auto& q : p.lower) loMin = std::min(loMin, q.y);
    EXPECT_GT(upMax, 0.0f);
    EXPECT_LT(loMin, 0.0f);
}

TEST(AirfoilImport, ParsesLednicerAndAgreesWithSelig) {
    const Surfaces s = naca(0.12, 0.02, 0.4, 60, true);
    AirfoilProfile a, b;
    ASSERT_TRUE(AirfoilImport::parse(seligText(s, "NACA 2412"), a));
    ASSERT_TRUE(AirfoilImport::parse(lednicerText(s, "NACA 2412"), b));
    // Same section, two dialects: the derived quantities must agree.
    EXPECT_NEAR(a.thickness(), b.thickness(), 1e-3f);
    EXPECT_NEAR(a.camber(), b.camber(), 1e-3f);
    EXPECT_EQ(a.upper.size(), b.upper.size());
}

// A knife trailing edge cannot be printed or machined, so the open case is
// normal and must be detected rather than quietly welded shut.
TEST(AirfoilImport, DetectsABluntTrailingEdge) {
    AirfoilProfile sharp, blunt;
    ASSERT_TRUE(AirfoilImport::parse(seligText(naca(0.12, 0, 0, 60, true), "sharp"), sharp));
    ASSERT_TRUE(AirfoilImport::parse(seligText(naca(0.12, 0, 0, 60, false), "blunt"), blunt));
    EXPECT_FALSE(sharp.bluntTrailingEdge);
    EXPECT_TRUE(blunt.bluntTrailingEdge);
    // NACA's original coefficient leaves ~0.25% of chord at t=12%.
    EXPECT_NEAR(blunt.trailingGap, 0.0025f, 0.0015f);
}

TEST(AirfoilImport, AcceptsAHeaderlessFileAndDosLineEndings) {
    std::string txt = seligText(naca(0.12, 0, 0, 60, true), nullptr);
    AirfoilProfile p;
    ASSERT_TRUE(AirfoilImport::parse(txt, p)) << "headerless Selig must parse";
    EXPECT_NEAR(p.thickness(), 0.12f, 0.006f);
    // Same content, CRLF.
    std::string dos;
    for (char c : txt) { if (c == '\n') dos += '\r'; dos += c; }
    AirfoilProfile q;
    ASSERT_TRUE(AirfoilImport::parse(dos, q)) << "CRLF must parse";
    EXPECT_NEAR(q.thickness(), p.thickness(), 1e-4f);
}

TEST(AirfoilImport, RejectsFilesThatAreNotCoordinates) {
    AirfoilProfile p;
    std::string err;
    EXPECT_FALSE(AirfoilImport::parse("", p, &err));
    EXPECT_FALSE(AirfoilImport::parse("hello\nworld\n", p, &err));
    // Number pairs, but not a chord-normalised section: a G-code-ish list.
    EXPECT_FALSE(AirfoilImport::parse("pts\n10 20\n30 40\n50 60\n70 80\n90 100\n", p, &err));
    EXPECT_FALSE(err.empty()) << "a refusal must say why";
}

TEST(AirfoilImport, SimplifyKeepsTheShape) {
    AirfoilProfile p;
    ASSERT_TRUE(AirfoilImport::parse(seligText(naca(0.12, 0.02, 0.4, 120, true), "n"), p));
    const std::size_t before = p.upper.size();
    const float t0 = p.thickness(), c0 = p.camber();
    AirfoilImport::simplify(p, 30);
    EXPECT_LE((int)p.upper.size(), 30);
    EXPECT_LE((int)p.lower.size(), 30);
    EXPECT_LT(p.upper.size(), before);
    // Decimation is allowed to move the surface a little, not to change what
    // section it is.
    EXPECT_NEAR(p.thickness(), t0, 0.004f);
    EXPECT_NEAR(p.camber(), c0, 0.004f);
    // Endpoints are never dropped: the nose and the trailing edge define it.
    EXPECT_NEAR(p.upper.front().x, 0.0f, 1e-3f);
    EXPECT_NEAR(p.upper.back().x, 1.0f, 1e-3f);
}

TEST(AirfoilImport, PlacesAClosedProfileAtTheRequestedChord) {
    AirfoilProfile p;
    ASSERT_TRUE(AirfoilImport::parse(seligText(naca(0.12, 0.02, 0.4, 60, false), "n"), p));
    AirfoilImport::simplify(p, 24);
    Sketch sk;
    const int added = AirfoilImport::place(&sk, p, glm::vec2(10.0f, 5.0f), 100.0f, 0.0f);
    EXPECT_GE(added, 3) << "two splines plus a trailing-edge line for a blunt section";
    EXPECT_EQ(sk.getSplines().size(), 2u);
    EXPECT_EQ(sk.getLines().size(), 1u);

    // Chord length lands where asked, measured on the placed geometry.
    float xmin = 1e30f, xmax = -1e30f;
    for (const auto& pt : sk.getPoints()) {
        xmin = std::min(xmin, pt.pos.x);
        xmax = std::max(xmax, pt.pos.x);
    }
    EXPECT_NEAR(xmax - xmin, 100.0f, 0.5f);
    EXPECT_NEAR(xmin, 10.0f, 0.5f) << "the leading edge is the placement anchor";

    // The nose is ONE point shared by both surfaces, not a coincident pair --
    // a duplicate there leaves the region walker with a gap it cannot close.
    int atNose = 0;
    for (const auto& pt : sk.getPoints())
        if (std::abs(pt.pos.x - 10.0f) < 1e-3f && std::abs(pt.pos.y - 5.0f) < 1e-3f) ++atNose;
    EXPECT_EQ(atNose, 1);
}

// Wing twist is specified about the leading edge, so stacked stations at
// different incidences keep that datum.
TEST(AirfoilImport, RotatesAboutTheLeadingEdge) {
    AirfoilProfile p;
    ASSERT_TRUE(AirfoilImport::parse(seligText(naca(0.12, 0, 0, 40, true), "n"), p));
    AirfoilImport::simplify(p, 16);
    Sketch sk;
    const glm::vec2 anchor(3.0f, -2.0f);
    ASSERT_GT(AirfoilImport::place(&sk, p, anchor, 50.0f, 10.0f), 0);
    bool foundAnchor = false;
    float far = 0.0f;
    for (const auto& pt : sk.getPoints()) {
        if (glm::length(pt.pos - anchor) < 1e-3f) foundAnchor = true;
        far = std::max(far, glm::length(pt.pos - anchor));
    }
    EXPECT_TRUE(foundAnchor) << "the leading edge must stay put under rotation";
    EXPECT_NEAR(far, 50.0f, 0.6f) << "the chord length is unchanged by rotation";
}

// The point of the whole exercise: a placed profile must be a CLOSED region
// that extrudes. If the two surfaces don't share the nose, or the blunt
// trailing edge isn't bridged, buildWires produces an open wire and the
// section is undeployable no matter how accurate the coordinates are.
TEST(AirfoilImport, PlacedProfileClosesIntoAnExtrudableWire) {
    for (bool sharpTE : {true, false}) {
        AirfoilProfile p;
        ASSERT_TRUE(AirfoilImport::parse(
            seligText(naca(0.12, 0.02, 0.4, 80, sharpTE), "NACA 2412"), p));
        AirfoilImport::simplify(p, 28);
        Sketch sk;
        ASSERT_GT(AirfoilImport::place(&sk, p, glm::vec2(0.0f, 0.0f), 120.0f, 0.0f), 0);

        const std::vector<TopoDS_Wire> wires = sk.buildWires();
        ASSERT_FALSE(wires.empty())
            << (sharpTE ? "sharp" : "blunt") << " TE: no wire was built";
        bool closed = false;
        for (const TopoDS_Wire& w : wires) if (w.Closed()) closed = true;
        EXPECT_TRUE(closed)
            << (sharpTE ? "sharp" : "blunt") << " TE: the profile is not closed";
    }
}

// The anchor decides where the click lands AND what the section rotates about.
// One function serves the ghost, the preview box and the placed geometry, so a
// bug here shows up in all three together rather than as a preview that lies.
TEST(AirfoilImport, AnchorChoicePlacesAndPivotsThere) {
    AirfoilProfile p;
    ASSERT_TRUE(AirfoilImport::parse(seligText(naca(0.12, 0.02, 0.4, 60, true), "n"), p));
    AirfoilImport::simplify(p, 20);

    const glm::vec2 le = AirfoilImport::anchorPoint(p, materializr::AirfoilAnchor::LeadingEdge);
    const glm::vec2 qc = AirfoilImport::anchorPoint(p, materializr::AirfoilAnchor::QuarterChord);
    const glm::vec2 ct = AirfoilImport::anchorPoint(p, materializr::AirfoilAnchor::Centre);
    EXPECT_NEAR(le.x, 0.0f, 1e-3f);
    EXPECT_NEAR(qc.x, 0.25f, 0.01f) << "quarter chord is a quarter along the chord line";
    EXPECT_NEAR(ct.x, 0.5f, 0.05f);

    // The quarter chord and the centre are INTERIOR points -- no vertex sits
    // there -- so "is a vertex on the click" only holds for the leading edge.
    // Test the transform itself instead: the leading edge, which IS a vertex,
    // must land exactly where the anchor maths says, at every angle.
    const glm::vec2 click(7.0f, -3.0f);
    const float chord = 80.0f;
    for (auto a : {materializr::AirfoilAnchor::LeadingEdge,
                   materializr::AirfoilAnchor::QuarterChord,
                   materializr::AirfoilAnchor::Centre}) {
        const glm::vec2 a0 = AirfoilImport::anchorPoint(p, a);
        for (float ang : {0.0f, 12.0f, -30.0f}) {
            Sketch sk;
            ASSERT_GT(AirfoilImport::place(&sk, p, click, chord, ang, a), 0);
            const float r = ang * 3.14159265358979323846f / 180.0f;
            const float cs = std::cos(r), sn = std::sin(r);
            const glm::vec2 d = (le - a0) * chord;
            const glm::vec2 wantLE =
                click + glm::vec2(d.x * cs - d.y * sn, d.x * sn + d.y * cs);
            float best = 1e9f;
            for (const auto& pt : sk.getPoints())
                best = std::min(best, glm::length(pt.pos - wantLE));
            EXPECT_LT(best, 0.5f)
                << "anchor " << (int)a << " at " << ang
                << " deg: leading edge is not where the anchor maths puts it";
            // Rotating about the anchor keeps every vertex within a chord of
            // the click -- it pivots, it does not fly off.
            float far = 0.0f;
            for (const auto& pt : sk.getPoints())
                far = std::max(far, glm::length(pt.pos - click));
            EXPECT_LT(far, chord * 1.05f) << "anchor " << (int)a;
        }
    }
}

// Chord length is a property of the section, not of where it was anchored.
TEST(AirfoilImport, ChordSurvivesEveryAnchor) {
    AirfoilProfile p;
    ASSERT_TRUE(AirfoilImport::parse(seligText(naca(0.12, 0, 0, 60, true), "n"), p));
    AirfoilImport::simplify(p, 20);
    for (auto a : {materializr::AirfoilAnchor::LeadingEdge,
                   materializr::AirfoilAnchor::QuarterChord,
                   materializr::AirfoilAnchor::Centre}) {
        Sketch sk;
        ASSERT_GT(AirfoilImport::place(&sk, p, glm::vec2(0, 0), 150.0f, 0.0f, a), 0);
        float xmin = 1e30f, xmax = -1e30f;
        for (const auto& pt : sk.getPoints()) {
            xmin = std::min(xmin, pt.pos.x);
            xmax = std::max(xmax, pt.pos.x);
        }
        EXPECT_NEAR(xmax - xmin, 150.0f, 0.8f) << "anchor " << (int)a;
    }
}
