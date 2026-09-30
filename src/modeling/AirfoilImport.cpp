#include "AirfoilImport.h"
#include "../i18n.h"
#include "Sketch.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

namespace materializr {
namespace {

// A line is "data" if it holds exactly two finite numbers and nothing else.
bool twoNumbers(const std::string& line, double& a, double& b) {
    std::istringstream is(line);
    if (!(is >> a) || !(is >> b)) return false;
    std::string rest;
    if (is >> rest) return false;              // a third token: not a coord pair
    return std::isfinite(a) && std::isfinite(b);
}

std::vector<std::string> nonEmptyLines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream is(text);
    std::string l;
    while (std::getline(is, l)) {
        // Strip CR (DAT files are frequently DOS-ended) and surrounding space.
        while (!l.empty() && (l.back() == '\r' || std::isspace((unsigned char)l.back())))
            l.pop_back();
        std::size_t b = 0;
        while (b < l.size() && std::isspace((unsigned char)l[b])) ++b;
        if (b) l = l.substr(b);
        if (!l.empty()) out.push_back(l);
    }
    return out;
}

// Index of the leading edge: the point nearest the origin. Selig files nearly
// always contain an exact (0,0), but not always -- some are re-sampled and the
// nose lands at x = 1e-5, which an equality test would miss entirely.
std::size_t leadingEdgeIndex(const std::vector<glm::vec2>& pts) {
    std::size_t best = 0;
    float bestD = 1e30f;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        const float d = pts[i].x * pts[i].x + pts[i].y * pts[i].y;
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

// Douglas-Peucker on an open polyline.
void dp(const std::vector<glm::vec2>& pts, std::size_t i0, std::size_t i1,
        float tol, std::vector<bool>& keep) {
    if (i1 <= i0 + 1) return;
    const glm::vec2 a = pts[i0], b = pts[i1];
    const glm::vec2 ab = b - a;
    const float len2 = ab.x * ab.x + ab.y * ab.y;
    std::size_t worst = i0;
    float worstD = -1.0f;
    for (std::size_t i = i0 + 1; i < i1; ++i) {
        const glm::vec2 ap = pts[i] - a;
        float d;
        if (len2 < 1e-20f) {
            d = std::sqrt(ap.x * ap.x + ap.y * ap.y);
        } else {
            const float t = std::max(0.0f, std::min(1.0f, (ap.x * ab.x + ap.y * ab.y) / len2));
            const glm::vec2 proj = a + ab * t;
            const glm::vec2 e = pts[i] - proj;
            d = std::sqrt(e.x * e.x + e.y * e.y);
        }
        if (d > worstD) { worstD = d; worst = i; }
    }
    if (worstD <= tol) return;
    keep[worst] = true;
    dp(pts, i0, worst, tol, keep);
    dp(pts, worst, i1, tol, keep);
}

std::vector<glm::vec2> decimate(const std::vector<glm::vec2>& pts, int maxPts,
                                float tol) {
    if (pts.size() <= 2 || (int)pts.size() <= maxPts) return pts;
    // Raise the tolerance until the count fits. Starting from the caller's
    // tolerance keeps the shape when the budget is generous.
    for (int iter = 0; iter < 40; ++iter) {
        std::vector<bool> keep(pts.size(), false);
        keep.front() = keep.back() = true;
        dp(pts, 0, pts.size() - 1, tol, keep);
        std::vector<glm::vec2> out;
        for (std::size_t i = 0; i < pts.size(); ++i)
            if (keep[i]) out.push_back(pts[i]);
        if ((int)out.size() <= maxPts || tol > 0.2f) return out;
        tol *= 1.6f;
    }
    return pts;
}

} // namespace

float AirfoilProfile::thickness() const {
    // Upper minus lower at matching chord stations, sampled on the upper's x.
    if (empty()) return 0.0f;
    float t = 0.0f;
    for (const glm::vec2& u : upper) {
        // linear interpolation of the lower surface at u.x
        for (std::size_t i = 1; i < lower.size(); ++i) {
            const glm::vec2 a = lower[i - 1], b = lower[i];
            if ((u.x >= a.x && u.x <= b.x) || (u.x >= b.x && u.x <= a.x)) {
                const float dx = b.x - a.x;
                const float f = (std::fabs(dx) < 1e-9f) ? 0.0f : (u.x - a.x) / dx;
                t = std::max(t, u.y - (a.y + (b.y - a.y) * f));
                break;
            }
        }
    }
    return t;
}

float AirfoilProfile::camber() const {
    if (empty()) return 0.0f;
    float c = 0.0f;
    for (const glm::vec2& u : upper) {
        for (std::size_t i = 1; i < lower.size(); ++i) {
            const glm::vec2 a = lower[i - 1], b = lower[i];
            if ((u.x >= a.x && u.x <= b.x) || (u.x >= b.x && u.x <= a.x)) {
                const float dx = b.x - a.x;
                const float f = (std::fabs(dx) < 1e-9f) ? 0.0f : (u.x - a.x) / dx;
                const float lo = a.y + (b.y - a.y) * f;
                c = std::max(c, std::fabs((u.y + lo) * 0.5f));
                break;
            }
        }
    }
    return c;
}

bool AirfoilImport::parse(const std::string& text, AirfoilProfile& out,
                          std::string* err) {
    auto fail = [&](const char* m) { if (err) *err = m; return false; };
    out = AirfoilProfile{};

    const std::vector<std::string> lines = nonEmptyLines(text);
    if (lines.size() < 4) return fail(materializr::tr("not enough lines to be a coordinate file"));

    std::size_t i = 0;
    double a = 0.0, b = 0.0;
    // A header line is anything that is not a coordinate pair. Selig files
    // occasionally omit it, so treat the first line as data when it parses.
    if (!twoNumbers(lines[0], a, b)) { out.name = lines[0]; i = 1; }

    if (i >= lines.size()) return fail(materializr::tr("no coordinates after the header"));

    // LEDNICER detection: the first data line is a pair of COUNTS, not
    // coordinates -- both well above 1, and a chord-normalised x never is.
    bool lednicer = false;
    if (twoNumbers(lines[i], a, b) && a > 1.5 && b > 1.5) lednicer = true;

    if (lednicer) {
        const std::size_t nUpper = (std::size_t)(a + 0.5);
        const std::size_t nLower = (std::size_t)(b + 0.5);
        ++i;
        std::vector<glm::vec2> pts;
        for (; i < lines.size(); ++i) {
            if (!twoNumbers(lines[i], a, b)) continue;   // blank/section break
            pts.push_back(glm::vec2((float)a, (float)b));
        }
        if (pts.size() < nUpper + nLower)
            return fail(materializr::tr("fewer coordinates than the count line promises"));
        out.upper.assign(pts.begin(), pts.begin() + nUpper);
        out.lower.assign(pts.begin() + nUpper, pts.begin() + nUpper + nLower);
    } else {
        std::vector<glm::vec2> pts;
        for (; i < lines.size(); ++i) {
            if (!twoNumbers(lines[i], a, b)) continue;
            pts.push_back(glm::vec2((float)a, (float)b));
        }
        if (pts.size() < 5) return fail(materializr::tr("too few coordinate pairs"));
        // Selig: TE -> upper -> LE -> lower -> TE. Split at the nose; the
        // upper half arrives reversed, so flip it to leading-edge-first.
        const std::size_t le = leadingEdgeIndex(pts);
        if (le == 0 || le + 1 >= pts.size())
            return fail(materializr::tr("could not find the leading edge (is this a Selig file?)"));
        out.upper.assign(pts.begin(), pts.begin() + le + 1);
        std::reverse(out.upper.begin(), out.upper.end());
        out.lower.assign(pts.begin() + le, pts.end());
    }

    if (out.empty()) return fail(materializr::tr("not enough points on one of the surfaces"));

    // Both surfaces must start at the nose and run aft; a file listing a
    // surface backwards would otherwise produce a bow-tie loop.
    if (out.upper.front().x > out.upper.back().x)
        std::reverse(out.upper.begin(), out.upper.end());
    if (out.lower.front().x > out.lower.back().x)
        std::reverse(out.lower.begin(), out.lower.end());

    // Sanity: a chord-normalised section spans ~0..1 in x. Anything else is a
    // different kind of file that happens to hold number pairs.
    float xmin = 1e30f, xmax = -1e30f;
    for (const auto& p : out.upper) { xmin = std::min(xmin, p.x); xmax = std::max(xmax, p.x); }
    for (const auto& p : out.lower) { xmin = std::min(xmin, p.x); xmax = std::max(xmax, p.x); }
    if (xmin < -0.05f || xmax > 1.05f || xmax - xmin < 0.5f)
        return fail(materializr::tr("coordinates are not chord-normalised (expected x from 0 to 1)"));

    const glm::vec2 teU = out.upper.back(), teL = out.lower.back();
    out.trailingGap = std::fabs(teU.y - teL.y);
    out.bluntTrailingEdge = out.trailingGap > 1e-5f;
    return true;
}

bool AirfoilImport::load(const std::string& path, AirfoilProfile& out,
                         std::string* err) {
    std::ifstream f(path);
    if (!f) { if (err) *err = materializr::tr("could not open the file"); return false; }
    std::ostringstream ss;
    ss << f.rdbuf();
    if (!parse(ss.str(), out, err)) return false;
    if (out.name.empty()) {
        const std::size_t slash = path.find_last_of("/\\");
        const std::size_t dot = path.find_last_of('.');
        out.name = path.substr(slash == std::string::npos ? 0 : slash + 1,
                               dot == std::string::npos ? std::string::npos
                                                        : dot - (slash == std::string::npos ? 0 : slash + 1));
    }
    return true;
}

void AirfoilImport::simplify(AirfoilProfile& prof, int maxPerSurface,
                             float tolerance) {
    if (maxPerSurface < 4) maxPerSurface = 4;
    prof.upper = decimate(prof.upper, maxPerSurface, tolerance);
    prof.lower = decimate(prof.lower, maxPerSurface, tolerance);
}

glm::vec2 AirfoilImport::anchorPoint(const AirfoilProfile& prof,
                                     AirfoilAnchor a) {
    if (prof.empty()) return glm::vec2(0.0f, 0.0f);
    const glm::vec2 le = prof.upper.front();
    switch (a) {
    case AirfoilAnchor::LeadingEdge:
        return le;
    case AirfoilAnchor::QuarterChord: {
        // A quarter of the way along the CHORD LINE (leading edge to trailing
        // edge), not simply x = 0.25: on a cambered section with a blunt tail
        // the chord line is not the x axis.
        const glm::vec2 teU = prof.upper.back(), teL = prof.lower.back();
        const glm::vec2 te = 0.5f * (teU + teL);
        return le + (te - le) * 0.25f;
    }
    case AirfoilAnchor::Centre:
    default: {
        glm::vec2 mn(1e30f, 1e30f), mx(-1e30f, -1e30f);
        auto acc = [&](const std::vector<glm::vec2>& v) {
            for (const glm::vec2& p : v) {
                mn = glm::vec2(std::min(mn.x, p.x), std::min(mn.y, p.y));
                mx = glm::vec2(std::max(mx.x, p.x), std::max(mx.y, p.y));
            }
        };
        acc(prof.upper);
        acc(prof.lower);
        return 0.5f * (mn + mx);
    }
    }
}

int AirfoilImport::place(Sketch* sketch, const AirfoilProfile& prof,
                         glm::vec2 pos, float chordMm, float angleDeg,
                         AirfoilAnchor anchor) {
    if (!sketch || prof.empty() || chordMm <= 0.0f) return 0;

    const float rad = angleDeg * 3.14159265358979323846f / 180.0f;
    const float cs = std::cos(rad), sn = std::sin(rad);
    // Rotation happens ABOUT THE ANCHOR, so changing the incidence pivots the
    // section around the point the user placed rather than sliding it away.
    const glm::vec2 a0 = anchorPoint(prof, anchor);
    auto toSketch = [&](glm::vec2 n) {
        const glm::vec2 s((n.x - a0.x) * chordMm, (n.y - a0.y) * chordMm);
        return pos + glm::vec2(s.x * cs - s.y * sn, s.x * sn + s.y * cs);
    };

    // Shared nose point: both surfaces start there, so the loop closes without
    // a duplicate vertex the region walker would have to reconcile.
    const int nose = sketch->addPoint(toSketch(prof.upper.front()));
    // A SHARP section must share its trailing edge the same way. Two points at
    // the same coordinate are still two vertices, and buildWires then walks an
    // open chain and produces no closed wire at all -- the profile looks right
    // on screen and refuses to extrude.
    const int tail = prof.bluntTrailingEdge
                         ? -1
                         : sketch->addPoint(toSketch(prof.upper.back()));

    auto emit = [&](const std::vector<glm::vec2>& surf) {
        std::vector<int> ids;
        ids.reserve(surf.size());
        ids.push_back(nose);
        const std::size_t last = surf.size() - 1;
        for (std::size_t k = 1; k < last; ++k)
            ids.push_back(sketch->addPoint(toSketch(surf[k])));
        ids.push_back(tail >= 0 ? tail : sketch->addPoint(toSketch(surf[last])));
        return ids;
    };

    const std::vector<int> up = emit(prof.upper);
    const std::vector<int> lo = emit(prof.lower);
    if (up.size() < 2 || lo.size() < 2) return 0;

    int added = 0;
    if (sketch->addSpline(up) >= 0) ++added;
    if (sketch->addSpline(lo) >= 0) ++added;

    // Blunt trailing edge: close it with a straight segment so the profile is
    // one closed region. A sharp section already meets at a single point.
    if (prof.bluntTrailingEdge) {
        if (sketch->addLine(up.back(), lo.back()) >= 0) ++added;
    }
    return added;
}

} // namespace materializr
