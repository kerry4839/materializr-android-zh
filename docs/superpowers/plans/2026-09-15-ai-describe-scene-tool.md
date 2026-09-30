# Plan: add `describe_scene` to the AI Assistant tool schema
_Round 0  -  initial draft by Claude_

## Goal
Add a read-only AI tool that lists every body, sketch, construction axis, and
construction plane currently in the document (id, name, visibility, and
cheap geometric summary), so the model can discover ids to pass into
`move_body`/`boolean_op`/`extrude_sketch`/etc. instead of only ever knowing
about objects it created itself in the same conversation. This is the gap
flagged during `extrude_sketch`'s review (`AiSessionController.cpp` sends no
scene state alongside the conversation, and no inspection tool exists
anywhere in `AiToolSchema.cpp`) - it affects every id-referencing tool
shipped so far, not just one.

## Approach

### 1. New tool definition
`src/ai/AiToolSchema.cpp`, appended to `allTools()`:

```
describe_scene(
  after_body_id     number, optional, default omitted - resume a body
                             listing after this id (see Truncation below)
  after_sketch_id   number, optional, same for sketches
  after_axis_id     number, optional, same for construction axes
  after_plane_id    number, optional, same for construction planes
)
```
Read-only; does not modify the document.

### 2. Dispatcher handler - the first read-only tool in this file
(param validation for the four `after_*_id` cursors happens here too: each
is optional, and unlike `requireBodyId`/`requireSketchId` a cursor does
NOT need to correspond to a real existing id - a stale cursor from a
deleted object should just filter out nothing extra, not error. Validate
finite + integral only, via the same `parseWholeNumber` helper
`extrude_sketch` already introduced, no existence check.)

`src/ai/AiToolDispatcher.cpp`, new `describeScene` handler wired into
`executeTool`. Unlike every existing handler it builds NO `Operation` and
never calls `ctx.history().pushOperation(...)` or `ctx.markMeshesDirty()` -
there is nothing to execute or redraw. This needs calling out explicitly in
review: it is a real architectural first for this dispatcher, not an
oversight.

For each of the four object kinds, iterate `doc.getAll<Kind>Ids()` (all four
confirmed on `Document`: `getAllBodyIds()` D.h:213, `getAllSketchIds()`
D.h:248, `getAllPlaneIds()` D.h:375, `getAllAxisIds()` D.h:414) and append
one line per object to a `std::string` built with `std::ostringstream`,
grouped under a header per kind. Per-kind detail:

- **Bodies**: `getBodyName(id)` (D.h:200), `isBodyVisible(id)` (D.h:203),
  plus a cheap bounding box: `Bnd_Box box; BRepBndLib::Add(doc.getBody(id),
  box);` - the exact pattern already used in `Picker.cpp:74`, and (per
  review) the same guard that EVERY other `Bnd_Box` call site in this repo
  already applies before reading it (`Picker.cpp:78`,
  `MeasureTool.cpp:102`, `Application_Dialogs.cpp` x5, all `if
  (box.IsVoid()) ...`): check `box.IsVoid()` before calling `.Get()`. Per
  round-2/3 review this is necessary but not sufficient, and the failure
  handling must match this codebase's OWN established idiom, not an
  invented one:
  - `shape.IsNull()` checked FIRST (skip `BRepBndLib::Add` entirely for a
    definitely-null shape).
  - The `Add()`/`Get()` pair wrapped in this repo's existing three-tier
    OCCT catch pattern (used verbatim at `BrepIO.cpp:261-268` and 7+ other
    sites - `main.cpp`, `ParallelMesh.cpp`, `StepIO.cpp`, `IgesIO.cpp`,
    `ObjExport.cpp`, `ProjectIO.cpp` x2, `StlIO.cpp`): `catch (const
    Standard_Failure&)` FIRST, then `catch (const std::exception&)`, then
    `catch (...)`. Round-3 review corrected an error in round-2's own fix:
    `Standard_Failure` derives from `Standard_Transient`
    (`Standard_Failure.hxx:30`), NOT `std::exception` - a bare `catch
    (const std::exception&)` would silently miss every OCCT-native
    exception, which is exactly the kind `BRepBndLib::Add` can throw. Also
    correcting my own round-2 framing: I said the outer
    `AiSessionController.cpp:88` try makes this "not about app safety" -
    that claim was ONLY true if the per-body catch actually catches
    everything relevant, which the wrong exception type would not have.
    With the correct type it holds: an uncaught escape would be an actual
    gap, not a redundant safety net.
  - After a successful `.Get()`: `box.IsOpen()` checked BEFORE trusting
    any of the six bounds (round-3 review: OCCT's "unbounded" sentinel,
    `Precision::Infinite()`, is a large but FINITE double - `2e+100` in
    the installed 7.9.3 headers - so `std::isfinite()` alone does not
    detect it; `IsOpen()` is the actual "this box has an unbounded side"
    flag, `Bnd_Box.hxx:182`). `std::isfinite` on all six values is KEPT as
    a second, independent check (defense in depth - `IsOpen()` catches
    OCCT's own documented sentinel, `isfinite` catches anything else
    pathological), not replaced by it.
  Any of these failure modes reports that body's line as
  `bounds=unavailable` and the loop continues to the next body (one bad
  object must not hide discovery of everything else, mirroring the
  null-sketch guard below). No mesh rebuild, no caching needed for a
  once-per-call scene listing.
  Convert world -> user-space using the EXACT INVERSE of `PrimitiveOp.cpp`'s
  `worldPnt(ox,oy,oz) = gp_Pnt(ox,oz,oy)` (confirmed correct in review
  against `PrimitiveOp.cpp:33-35`, cross-checked against the existing test
  `AddBoxWithDifferentHeightAndDepthLandsOnTheCorrectWorldAxes`: world Y is
  requested height, world Z is requested depth):
  - `origin` (user-space min corner) = `(x0, z0, y0)`
  - `size` (user-space width/height/depth, in THAT order/labeling - not raw
    XYZ, label the tuple explicitly in the output so it can't be misread)
    = `(x1-x0, y1-y0, z1-z0)`
  This is the first tool to convert World -> user-space rather than the
  other way around; every existing tool only ever goes user-space -> world
  on the way IN.
- **Sketches**: `getSketchName(id)` (D.h:244), `isSketchVisible(id)`
  (D.h:247), `doc.getSketch(id)->getPlane()` (`Sketch.h:82`, `const
  gp_Pln&`) for plane origin (`.Location()`) and normal
  (`.Axis().Direction()`), both converted to user-space with the same
  component swap as above (a direction has no origin, so no translation,
  just the axis reindex: `user.x=world.X, user.y=world.Z, user.z=world.Y`).
  No region count (`buildRegions()` is a real OCCT fuse/splitter, too
  expensive to run once per sketch on every scene query - `regionsCached()`
  only tells you whether it WOULD be free, not the count itself).
- **Construction axes**: `getAxisName(id)` (D.h:410), `isAxisVisible(id)`
  (D.h:413), `getAxis(id)` (D.h:409, `const AxisEntry*`) for `.origin`
  (`gp_Pnt`) and `.direction` (`gp_Dir`), same user-space conversion.
- **Construction planes**: `getPlaneName(id)` (D.h:371), `isPlaneVisible(id)`
  (D.h:374), `getPlane(id)` (D.h:370, `const PlaneEntry*`) for `.plane`
  (`gp_Pln`, same origin/normal extraction as sketches above).

No provenance/"kind" tag (e.g. "came from an extrude") - confirmed no such
field exists anywhere on `BodyEntry`/`SketchEntry`/`PlaneEntry`/`AxisEntry`,
and reconstructing it means walking `History::getStep()` per body with no
existing id->creating-op map to look it up by. Out of scope; the object's
own name is the disambiguation signal instead.

### 3. Truncation with real pagination, not silent data loss
A flat 100-object cap with no way to see the rest would permanently hide
ids beyond the cap - a real problem since `pattern_body` alone can mint up
to 500 bodies in one call (`AiToolDispatcher.cpp:406`), a case this tool
must actually support discovering. Each category is paginated
independently via its own `after_<kind>_id` param (see step 1): sort that
category's ids ascending, filter to `id > after_<kind>_id` (omitted/absent
= no filter, start from the lowest id), take the first 100 of what
remains, and if any are left after that, end the category with `"... N
more (continue with after_<kind>_id=<last shown id>)"` - the model can
issue a follow-up `describe_scene` call with that cursor to walk the rest.
Every object is reachable, just not necessarily in one call.

### 4. Name sanitization - the model's own conversation history is not
trusted input
`Document::setBodyName`/`setSketchName` (`Document.cpp:155` etc.) accept
any string with no validation - a body/sketch/axis/plane name embedded
raw into this tool's plain-text output could contain a newline plus a
hand-crafted `id=... "..." visible=... origin=...` line, forging a fake
scene object in what the model reads back as its own trusted tool result,
or simply be long enough to blow the per-category size budget past the
truncation cap. New `sanitizeName(const std::string&)` helper, applied to
every name before it's embedded: replace any `\n`/`\r` and any other C0
control byte (`< 0x20`) with a space, replace any `"` with `'` (names are
always wrapped in `"..."` in the output), then truncate to 80 bytes WITH A
UTF-8-SAFE BOUNDARY. Round-2 review caught a real bug here: a naive
`substr(0, 80)` can split a multi-byte UTF-8 character, and both provider
clients serialize the full conversation (this tool result included) via
plain `nlohmann::json::dump()` with nlohmann's default STRICT UTF-8
handling (confirmed: `AnthropicClient.cpp:108`,
`OpenAiCompatibleClient.cpp:123`, `OpenAiCompatibleClient.cpp:33` all call
bare `.dump()`, no `error_handler` argument) - that throws on the very
next API request, not this one, making it a delayed, hard-to-trace
failure. Fix: after cutting at byte 80, walk backward while the byte at
the cut point is a UTF-8 continuation byte (`(b & 0xC0) == 0x80`) to land
on a real character boundary before truncating.

Round-3 review found this still incomplete: the boundary-aware cut only
protects a name that was ALREADY valid UTF-8 going in. `setBodyName` never
validates its input at all, so a short (under 80 bytes - never even
reaches the truncation path) name containing a genuinely malformed byte
sequence would sail through untouched and still break the same
`nlohmann::json::dump()` call later. Fix: `sanitizeName` validates the
WHOLE string as REAL UTF-8, not merely "does each byte have the right
continuation-byte shape" - round-4 review: that alone would still accept
overlong encodings (e.g. a multi-byte encoding of a codepoint that fits
in fewer bytes), encoded UTF-16 surrogate codepoints (U+D800-U+DFFF, never
valid in UTF-8), and values above U+10FFFF, all of which are structurally
"continuation bytes in the right place" but not valid UTF-8. Decode each
sequence to its codepoint and validate the codepoint range/shortest-form
rule, not just the byte pattern; any byte that isn't part of a fully
valid sequence at its position is replaced with `?`. This runs BEFORE the
length truncation, not only at the cut boundary - so truncation is
guaranteed to operate on already-valid UTF-8 and the boundary-walk logic
above stays correct without also needing to handle
malformed input itself.

(Backslash-escaping, which round-2 also suggested, does not apply here -
that concern assumes embedding the name as a JSON string literal; this
format is plain text with `"..."` wrapping and `"` already replaced with
`'` above, so there is no backslash-escaping contract to satisfy - round-3
agreed this framing was correct.)

### 5. Output format - plain text, not JSON
`ToolResult.message` is documented (`AiToolDispatcher.h:8-13`) as doubling
for the human-readable scrollback line, and every existing tool returns one
plain sentence. `describe_scene` is the first tool whose result is
inherently a list, not a sentence - format as readable, grouped plain text
(one line per object, a header per category, `"(none)"` for an empty
category) rather than JSON, so it still reads sanely in the scrollback UI
while staying trivially parseable by the model. Example shape:

```
Bodies (3):
  id=1 "Box 1" visible=true origin=(0,0,0)mm size=10x20x15mm (w x h x d)
  id=2 "Cylinder 1" visible=true origin=(5,0,0)mm size=8x8x30mm
  id=5 "Extrude 1" visible=false origin=(-10,0,0)mm size=20x5x20mm

Sketches (2):
  id=0 "Sketch 1" visible=true plane_origin=(0,0,0)mm plane_normal=(0,1,0)
  id=3 "Sketch 2" visible=true plane_origin=(0,0,10)mm plane_normal=(0,0,1)

Construction Axes (0): (none)
Construction Planes (0): (none)
```

### 6. Tests
`tests/test_ai_tool_schema.cpp`: bump the count/name-list assertion to 19,
add a param-shape test asserting all four `after_*_id` params on
`describe_scene` are optional.

`tests/test_ai_tool_dispatcher.cpp`, new cases:
- empty document -> succeeds, every category reports `(none)`.
- one body with UNEQUAL width/height/depth at a NONZERO, asymmetric origin
  (not `addTestBox`'s cube-at-origin, which can't expose a Y/Z swap - per
  review, this is the actual coordinate-conversion proof, not the trivial
  case) -> reported `origin`/`size` match independently-computed
  user-space expectations, not merely re-derived from the same formula the
  implementation uses (assert against literal expected numbers, the same
  discipline `AddBoxWithDifferentHeightAndDepthLandsOnTheCorrectWorldAxes`
  already uses for its axis assertions).
- a hidden body (no AI tool exposes visibility yet, so set it directly via
  `doc.setBodyVisible(id, false)` - `Document.h:202` - the test fixture
  already reaches into `Document` directly for sketches, same pattern) ->
  reports `visible=false`.
- a sketch built on a plane with a NONZERO origin and a non-axis-aligned
  normal (not `addTwoRegionSketch`'s zero-origin plane) -> reported plane
  origin/normal match independently-computed user-space expectations.
- a body with a null/void shape (construct one directly via
  `doc.addBody(TopoDS_Shape())` or equivalent degenerate input reachable
  from a test, bypassing the AI tools which already reject this) -> a
  FULL `describe_scene` dispatcher call still succeeds, that body's line
  reads `bounds=unavailable`, and every OTHER body still reports normally
  - this is the "listing continuity" proof (round-4 review: this is the
  right level for that claim; the open-box case below is a different kind
  of test and must not be conflated with it).
- separately, a FOCUSED UNIT TEST of the bbox-extraction helper itself
  (not a full `describe_scene` dispatcher call) against a `Bnd_Box`
  constructed directly with an open side (`Bnd_Box::OpenXmin()`/etc.,
  no real body/document involved) -> the helper reports
  `bounds=unavailable` for it. Kept separate from the dispatcher-level
  test above per round-4 review: proving the helper rejects an open box
  is a different claim from proving the dispatcher's listing survives one
  bad object, and conflating the two into one test would leave the
  dispatcher-level claim actually unproven.
- a construction axis via `construction_axis`'s `"two_points"` type with
  two arbitrary non-axis-aligned points (the only way this tool exposes a
  non-trivial direction) -> appears with correct name/visibility/origin/
  direction. A construction plane via `construction_plane` with a
  standard type (`"xy"`/`"xz"`/`"yz"`) and a NONZERO `offset` (corrected
  per round-2 review: `construction_plane` only accepts a standard plane
  type plus an offset, it cannot be given an arbitrary normal - the
  offset alone is enough to prove its origin is reported correctly,
  asymmetric-normal coverage is the axis case's job, not this one's).
- a body/sketch name containing an embedded `"`, a `\n`, a name longer
  than 80 BYTES whose 80-byte cut point lands in the middle of a
  multi-byte UTF-8 character (e.g. a name built from a repeated 3-byte
  UTF-8 character positioned so byte 80 is its middle byte), AND - round-3
  review's added case - a SHORT name (well under 80 bytes, never reaching
  the truncation path at all) containing a genuinely malformed/invalid
  UTF-8 byte sequence -> the sanitizer neutralizes all of these: no
  embedded newline in the output, the quote is replaced (not a raw
  unescaped `"`), the long name is truncated at a valid UTF-8 boundary
  with a trailing `...`, the short malformed name has its invalid bytes
  replaced even though it was never truncated, and - the concrete bug
  round-2/3 caught - EVERY case's returned `ToolResult.message`
  re-serializes cleanly through `nlohmann::json(...).dump()` with NO
  `error_handler` override (the same strict call
  `AnthropicClient.cpp`/`OpenAiCompatibleClient.cpp` actually use),
  proving neither the multibyte cut NOR the pre-existing malformed bytes
  produced invalid UTF-8 in the output. Assert the RAW malicious substring
  is absent from the returned text, not just that SOMETHING was returned.
- more than 100 bodies (loop `add_box` 105 times) -> first call returns
  exactly 100 plus a `"... 5 more (continue with after_body_id=<n>)"`
  trailer; a follow-up call passing that `after_body_id` returns the
  remaining 5 and no trailer - proves the cursor is real and round-trips,
  not just that truncation happens once.
- confirm `describe_scene` never mutates state: assert BOTH
  `ctx.history().stepCount()` AND `ctx.history().revision()` are unchanged
  (per review: `revision()` only bumps inside `pushOperation`/`undo`/
  `redo`/etc., and `describe_scene` calls none of them - unlike
  `extrude_sketch`'s tests, which had to avoid `revision()` because a
  FAILED `pushOperation` still bumps it, `describe_scene` never calls
  `pushOperation` at all, so asserting `revision()` unchanged is valid
  here and a stronger check than `stepCount()` alone), AND that
  `markMeshesDirty()` was never invoked - bind a real local `bool
  meshesDirty = false` to `PluginContext::_bind`'s `meshesDirtyFlag`
  parameter (a small test-local helper, not a change to the shared
  `makeCtx` fixture every other test uses) and assert it stays `false`.

## Key decisions & tradeoffs
- **Plain text over JSON** - matches the file's existing convention
  (`ToolResult.message` is scrollback-facing), at the cost of being the
  first tool whose result needs internal structure at all. Revisit if a
  second list-shaped tool arrives and the two formats diverge.
- **No region count for sketches** - correctness (not paying for an
  expensive rebuild on every scene query) over completeness. The model can
  still learn a sketch's region count for free once it tries
  `extrude_sketch` and gets an out-of-range error naming the real count.
- **No provenance/kind tag** - no such data exists in `Document` today;
  adding it would be a much larger change (a body->creating-op map) outside
  this tool's scope.
- **World->user-space bounding-box conversion** - the highest-risk new code
  in this plan (first tool going this direction). Confirmed correct in
  Codex review against `PrimitiveOp.cpp:33-35` directly, and the test plan
  uses independently-computed expected values on asymmetric fixtures
  (not `addTestBox`'s cube-at-origin, which can't expose a Y/Z swap), so a
  regression here fails loudly, not silently.
- **Name sanitization before embedding** - `setBodyName`/`setSketchName`
  accept arbitrary strings with no validation, and this tool's own output
  becomes trusted-looking context for the SAME model on its next turn - an
  unsanitized name could inject a newline plus a fabricated `id=...` line,
  forging a scene object, or blow the per-category output budget with one
  oversized name. Escape/strip before embedding rather than trusting the
  document's stored names as safe to print raw.
- **Per-category cursor pagination over a flat cap** - a flat 100-object
  cap with no way to see the rest would have permanently hidden ids beyond
  it, defeating the tool's purpose for the `pattern_body`-created-500-
  bodies case this tool must actually support. Cursor-based (`after_body_id`
  etc.) rather than an opaque continuation token, matching the numeric-id
  vocabulary every other tool already speaks instead of introducing a new
  concept.
- **Bounding-box failures degrade per-object, not per-call** - `box.IsVoid()`
  before `.Get()` (the guard every other `Bnd_Box` call site in this repo
  already applies), reporting `bounds=unavailable` for that one body
  rather than letting a single degenerate shape abort discovery of every
  other object in the document.
- **100-per-category page size** - judgment call, not a mirrored
  convention (first tool where output scales with document size).

## Risks / open questions
- `getSketch(id)` can theoretically return null even though `id` came from
  `getAllSketchIds()` (same defensive pattern `extrude_sketch` already
  applies to `requireSketchId`+`getSketch`) - guard the same way, skip a
  null sketch's line with a name-only fallback rather than crashing.
- No existing tool has ever returned a multi-hundred-object result; the cap
  is untested against how the LLM actually consumes a long structured
  block in a ToolResult message (no length limit anywhere in
  `AiSessionController.cpp`, confirmed) - this is a reasonable default, not
  a proven one.

## Out of scope
- Any provenance/"kind" field on scene objects (see Key decisions).
- Sketch region counts (see Key decisions).
- Filtering the listing (e.g. `describe_scene(kind="bodies")`) - v1 always
  lists everything; add filtering only if a real prompt-size problem shows
  up in practice.
- Any change to how `AiSessionController` builds a turn - this tool is
  purely reactive (the model must choose to call it), no automatic
  scene-state injection into every turn.
