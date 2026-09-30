# Plan Review Log: add describe_scene to the AI Assistant tool schema
Started 2026-09-15 (session time). MAX_ROUNDS=5. Model: gpt-6-astra (config default).

## Round 1  -  Codex
Confirmed the world<->user-space coordinate math correct against
`PrimitiveOp.cpp:33-35` (this was the highest-risk detail flagged going
in). Five findings:
1. High - flat 100-object truncation permanently hides ids beyond the cap,
   with pattern_body's 500-body case as a concrete reachable scenario.
2. High - raw names embedded unsanitized let the model's own stored data
   forge fake output lines (newline + fabricated `id=...` line) or blow
   the size budget with one oversized name.
3. High - bbox extraction has no failure path; a null/void/degenerate
   body shape could throw, aborting the whole scene listing.
4. Medium - the "no mutation" test only checked `stepCount()`, which
   misses `markMeshesDirty()` entirely and (per extrude_sketch's own
   established lesson) can't distinguish "nothing happened" from "a
   failed pushOperation still bumped the revision."
5. Medium - test fixtures (cube at origin, zero-origin sketch plane)
   can't expose a Y/Z coordinate swap bug.

VERDICT: REVISE

### Claude's response
Verified each finding against the actual source before acting (`Bnd_Box::
IsVoid()` is indeed the guard every other call site in this repo already
uses; `setBodyName`/`setSketchName` confirmed to have zero validation;
`PluginContext::_bind`'s `meshesDirtyFlag` param confirmed testable).
Adopted all five in full:
1. Replaced the flat cap with per-category cursor pagination
   (`after_body_id`/`after_sketch_id`/`after_axis_id`/`after_plane_id`),
   sorted-ascending + filter + 100-per-page + a continuation trailer
   naming the next cursor.
2. Added a `sanitizeName()` helper (strip newlines, escape quotes, cap
   length) applied to every name before embedding.
3. Added `box.IsVoid()` guard before `.Get()`; a void body reports
   `bounds=unavailable` and the loop continues rather than aborting.
4. Test plan now asserts `revision()` unchanged (valid here specifically
   because describe_scene calls zero History-mutating functions, unlike
   extrude_sketch which had to avoid this exact check) AND binds a real
   local mesh-dirty flag to prove `markMeshesDirty()` was never called.
5. Replaced the cube-at-origin/zero-origin-plane fixtures with asymmetric,
   nonzero ones and independently-computed expected values, plus added a
   dedicated malicious-name test and a null-body-shape test.
No findings rejected - all five held up under verification.

## Round 2  -  Codex
Confirmed pagination, coordinate conversion, and the stronger read-only
assertions sound. Two material gaps remained:
1. High - `IsVoid()` alone is insufficient: no `shape.IsNull()` pre-check,
   no per-body exception boundary around `Add()`/`Get()`, and a non-void
   box can still have open/infinite bounds that need a finite check.
2. Medium - "truncate to 80 chars" doesn't specify UTF-8 handling; a naive
   `substr(0,80)` can split a multi-byte character, and both provider
   clients serialize the conversation via bare `nlohmann::json::dump()`
   with strict UTF-8, which throws on the NEXT request, not this one.
Plus a test-spec correction: `construction_plane` only accepts standard
xy/xz/yz + offset, it cannot be given an arbitrary normal - my test plan
had asked for something the tool can't produce.

VERDICT: REVISE

### Claude's response
Verified before acting: confirmed `AiSessionController.cpp:88` already
wraps the whole `executeTool()` call in a try (so the per-body catch's
job is listing-completeness, not app-crash safety, which is already
covered - narrowed the framing rather than overstating the fix); and
directly confirmed all three `.dump()` call sites
(`AnthropicClient.cpp:108`, `OpenAiCompatibleClient.cpp:33,123`) pass no
`error_handler`, so nlohmann's default strict UTF-8 mode applies and the
bug is real, not theoretical. Adopted both findings in full: added
`shape.IsNull()` pre-check, a per-body try/catch around the bbox
extraction, and a `std::isfinite` check on all six bounds; replaced the
naive 80-char cut with a UTF-8-boundary-aware truncation and added a test
that round-trips a multibyte-boundary name through the SAME bare
`nlohmann::json::dump()` the provider clients use. Corrected the
construction_plane test to use offset instead of an arbitrary normal,
moving the asymmetric-direction coverage to the axis test's `two_points`
type (which does support it). Declined one sub-point (backslash-escaping)
with a stated reason: that concern assumes JSON-string-literal embedding,
but this tool's format is plain text with quote-replacement already
handling the `"` case - no backslash-escaping contract exists to satisfy
in a plain-text format.

## Round 3  -  Codex
Three gaps remained:
1. High - the round-2 fix's catch type was wrong: `Standard_Failure`
   derives from `Standard_Transient`, NOT `std::exception`
   (`Standard_Failure.hxx:30`), so `catch (const std::exception&)` misses
   it entirely - and round-2's own claim that the outer executeTool() try
   made this "not about app safety" was only true if the catch actually
   caught everything relevant, which it didn't.
2. Medium - `std::isfinite()` does not detect OCCT's "unbounded" sentinel:
   `Precision::Infinite()` is a large but FINITE double (`2e+100`), so an
   open/unbounded box's bounds pass the finite check and become bogus
   geometric summaries.
3. Medium - the UTF-8 boundary-aware truncation only protects a name that
   was ALREADY valid UTF-8 going in; a short name (under 80 bytes, never
   reaching truncation) with a genuinely malformed byte sequence sails
   through untouched and still breaks the same strict `dump()` call later.

VERDICT: REVISE

### Claude's response
Verified all three directly: confirmed `Standard_Failure : public
Standard_Transient` in the installed OCCT headers, confirmed this
codebase already has an established 3-tier catch idiom (`Standard_Failure`
-> `std::exception` -> `...`) used verbatim at `BrepIO.cpp:261-268` and 7+
other sites, confirmed `Bnd_Box::IsOpen()` exists at `Bnd_Box.hxx:182`
guarding exactly the sentinel-bounds case. Adopted all three in full:
replaced the catch with this repo's own established 3-tier pattern rather
than inventing a new one; added `box.IsOpen()` as an independent check
BEFORE the (retained, not replaced) `isfinite` check; changed
`sanitizeName` to validate and repair the WHOLE string structurally before
truncation, not only at the cut boundary, so a short malformed name is
caught too. Corrected my own round-2 framing along the way: the "outer
try already covers app safety" claim only holds now that the per-body
catch actually catches the right exception type - stated that explicitly
rather than silently fixing the code and leaving the wrong reasoning in
the log. Added dedicated tests for the open-box case and the
short-malformed-UTF-8-name case, both asserting the SAME `dump()`
round-trip the provider clients actually use.

## Round 4  -  Codex
VERDICT: APPROVED. "No remaining material blockers... coordinate
conversion and pagination remain sound." Two nonblocking clarifications:
1. UTF-8 validation must reject overlong encodings, surrogate code
   points, and values above U+10FFFF - not merely check that each byte
   has the right continuation-byte shape.
2. The open-box case should be a focused unit test of the bbox-extraction
   helper itself; the null-body case is the one that should exercise a
   full `describe_scene` dispatcher call to prove listing continuity -
   conflating the two into one test would leave the dispatcher-level
   claim unproven.

### Claude's response
Adopted both. Rewrote the UTF-8 validation description to require
decoding each sequence to its codepoint and validating the
codepoint/shortest-form rule, not just byte-pattern shape. Split the
open-box test out as a standalone helper-level unit test, distinct from
and no longer conflated with the null-body dispatcher-level test. Plan
converged after 4 rounds - every material finding in every round held up
under direct verification against the actual source before being
adopted; no finding was rejected.

## Post-implementation review (validate-code gate)

After implementing, ran the full suite (119/119), all 3 repo gates, and
dispatched `standards-spec-review` (two parallel sub-agents), same
discipline as `extrude_sketch`'s post-implementation pass.

**Standards sub-agent** found no hard CONTRIBUTING.md violations. One real
judgment call adopted: the four category-listing blocks in `describeScene`
(bodies/sketches/axes/planes) repeated ~15 lines of pure pagination
boilerplate (sort, cursor-filter, page-cap, header/empty/trailer text)
byte-for-byte four times - genuine Duplicated Code, not Speculative
Generality, since only the boilerplate was identical and the per-category
detail (bbox vs. plane origin+normal vs. axis origin+direction) stayed
separate. Extracted a generic `appendPagedSection` helper taking
name/visibility/detail lambdas per category, cutting ~100 lines of
repetition while keeping the differing geometry logic untouched; all 12
`describe_scene` tests still pass identically after the refactor,
confirming it's behavior-preserving. Also adopted two minor clarity notes:
a comment explaining why the body-size line deliberately does NOT go
through `worldToUser` (raw world extents already match the w/h/d label
order - an algebraic property of the permutation, not a coincidence to
leave unexplained), and swapping the `-0` suppression's `if (v == 0.0)`
for a self-documenting `if (std::signbit(v) && v == 0.0)`.

**Spec sub-agent** found two real bugs in the TESTS themselves (not the
implementation): `DescribeSceneSanitizesAMaliciousNameWithoutForgingOutputLines`'s
final assertion was `EXPECT_EQ(x.dump(), x.dump())` - a copy-paste error
comparing a call to itself, tautologically true regardless of what the
code does, silently failing to exercise the "re-serializes cleanly"
regression guard the plan specified. Fixed to `EXPECT_NO_THROW(...dump())`,
matching the two neighboring UTF-8 tests that already used the correct
idiom. Separately, `DescribeSceneTruncatesALongNameAtAValidUtf8Boundary`
never actually asserted the plan's stated `"..."` trailing marker
appeared - only that the raw long name was absent and re-serialization
didn't throw, missing a marker-dropped regression. Added the assertion.

Re-ran the full suite (119/119) and all 3 repo gates after every fix in
this addendum - all green throughout. One deliberate, logged deviation
from round 4's literal ask: no standalone unit test of the bbox-helper's
`IsOpen()` branch (the helper is anonymous-namespace-private, and this
dispatcher's tests only ever go through the public `executeTool()` entry
point - no precedent anywhere in this file for testing an internal helper
in isolation). The `IsOpen()` guard itself is in the shipped code; only
its dedicated test was scoped out, for a stated reason.
