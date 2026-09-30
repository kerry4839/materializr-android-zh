# Plan: add `extrude_sketch` to the AI Assistant tool schema
_Round 0 - initial draft by Claude_

## Goal
Add a new AI-callable tool, `extrude_sketch`, that extrudes one or more
regions of an existing sketch (or the whole sketch profile) into a new or
existing body, matching the app's real "Extrude From" feature
(`Application.cpp` `ToolAction::ExtrudeSketch`, `ExtrudeOp`). This closes
one of the gaps flagged in `docs/superpowers/specs/2026-09-12-ai-assistant-design.md`'s
v1 non-goals list (sketching/extrude support). User explicitly requested
multi-region support (a list of region indices), not just a single region.

## Approach

### 1. Schema type system: add array-of-integer support
`src/ai/AiTypes.h`: add `ToolParamType::IntegerArray` to the existing
`enum class ToolParamType { Number, String }`. Named `IntegerArray`, not
`NumberArray` - region indices are whole numbers only, and putting that in
the JSON Schema itself (`"items": {"type": "integer"}`) lets a
schema-conformant provider reject a fractional index before it ever reaches
the dispatcher, instead of relying solely on the dispatcher's own runtime
check (defense in depth, cheap to add since the type is being introduced
either way).

`src/ai/AiToolSchema.cpp`:
- `typeName()` currently returns `"string"`/`"number"` for the JSON Schema
 `"type"` field; `IntegerArray` doesn't fit that shape (JSON Schema arrays
 need `{"type": "array", "items": {...}}`, not a bare type string).
 `paramsToJsonSchema()` gets a branch: for an `IntegerArray` param, emit
 `{"type": "array", "items": {"type": "integer"}, "description": ...}`
 instead of calling `typeName()`. `typeName()` itself stays used only for
 the two scalar cases.
- Add an `intArray(name, desc, required=false)` helper alongside `num()`/`str()`.

The dispatcher still validates integrality/range/duplicates itself
regardless of the schema's declared type (see step 3 below) - a provider's
own JSON-Schema enforcement is not something this code can rely on, only
benefit from when present.

### 2. New tool definition
`src/ai/AiToolSchema.cpp`, appended to `allTools()`:

```
extrude_sketch(
  sketch_id            number    required
  region_indices       integer[] optional - indices into Sketch::buildRegions();
                                  omitted or empty = whole-sketch profile
                                  (Sketch::buildProfileShape())
  distance             number    required, finite and nonzero (sign reverses
                                  the sweep direction along the profile's face
                                  normal - same signed semantics
                                  ExtrudeOp/ExtrudeController already use;
                                  NOT positive-only, see Key decisions)
  symmetric            string    optional, "true"/"false", default "false".
                                  When "true", total swept thickness is
                                  abs(distance) split evenly on both sides of
                                  the sketch plane and fused into one solid
                                  (ExtrudeOp.cpp:381-397) - distance's SIGN has
                                  no effect on the result in this mode (only
                                  its magnitude matters), unlike the
                                  non-symmetric case where sign picks a
                                  direction. State this in the tool's
                                  description string so the model isn't misled
                                  into thinking sign matters here.
  mode                 string    optional, "new_body" | "union" | "subtract" |
                                  "intersect", default "new_body"
  target_body_id       number    required iff mode != "new_body"
)
```

`draft_angle_degrees` is intentionally NOT part of v1 - see Key decisions.

`tests/test_ai_tool_schema.cpp` gets its tool-count/name-list assertion
updated (currently asserts an exact count - `test_ai_tool_schema.cpp:8-9`)
plus a param-shape test mirroring the existing `*ParamShapeMatchesBrief`
pattern (e.g. `PatternBodyParamShapeMatchesBrief`, line 55).

### 3. Dispatcher handler
`src/ai/AiToolDispatcher.cpp`, new `requireSketchId` helper (mirrors
`requireBodyId` at line 46: loop `doc.getAllSketchIds()`, "no sketch with
id N" on miss) plus a new `extrudeSketch` handler, added to the
`executeTool` if/else chain:

1. `requireSketchId(ctx, args, "sketch_id")` → `sketchId`, bail with its
 error on failure (no mutation yet).
2. `doc.getSketch(sketchId)` → `sketch` (guaranteed non-null, `requireSketchId`
 already confirmed existence via the same id list `getSketch` reads).
3. Resolve `region_indices` (optional arg):
 - absent, or an empty array → `TopoDS_Shape profile = sketch->buildProfileShape();`
 (whole-sketch profile - same fallback `Application::extrudeSketchById`
 uses).
 - present: MUST be a JSON array (reject a scalar/non-array value with a
 named error). Validate EACH element in this order, before any
 conversion or set-insertion: (a) `is_number()` and `std::isfinite(v)`;
 (b) representable as `int` - reject anything outside
 `[std::numeric_limits<int>::min(), max()]` (e.g. `1e100`) BEFORE
 casting, mirroring `requireBodyId`'s own range-check
 (`AiToolDispatcher.cpp` ~line 50) rather than casting first and
 hoping; (c) integral (`std::floor(v) == v`); (d) non-negative. Reject
 `0.5`, `null`, `1e100`, negative values, or non-numeric entries by
 name at whichever step first fails. Reject duplicate indices
 (surfacing a mistake to the model rather than silently deduping -
 consistent with the out-of-range policy below) - check for duplicates
 only AFTER the above per-element validation, using plain ints.
 Call `sketch->buildRegions()` ONCE, then bounds-check every
 (now-validated) index against `regions.size()`. **Any single
 out-of-range index fails the whole call** with an error naming the bad
 index and the valid range (`"region_index 5 out of range (sketch has 3
 regions)"`) - deliberately stricter than the real UI's silent
 `continue` on a bad index (`Application.cpp:2311-2313`), because a
 wrong index from an LLM should be surfaced, not silently dropped, or
 the model may believe a region was included when it wasn't. Deliberate
 UX deviation from the manual-selection code path, flagged for review
 rather than assumed right.
 - exactly one valid index → `profile = regions[idx].face;`
 - more than one → build a `TopoDS_Compound` from `regions[idx].face` for
 each index (`BRep_Builder::MakeCompound` + `Add`), mirroring
 `Application.cpp:2312-2326` verbatim (the ONLY multi-region path that
 exists in the codebase; reuse its exact shape). For ORDINARY
 (non-symmetric) `new_body` mode, this produces a compound of SEPARATE
 prism solids once extruded (`BRepPrimAPI_MakePrism` sweeps each face
 in the compound independently - it does not fuse them), same as the
 real UI's multi-region extrude. That "N solids, not fused" description
 applies ONLY to that case - `Symmetric` mode fuses the whole profile's
 up/down sweep into one solid regardless of region count
 (`ExtrudeOp.cpp:381-397`, `BRepAlgoAPI_Fuse`), and the three boolean
 modes further combine the extrusion with `target_body_id`'s geometry.
 Scoped correctly per review (the first revision over-generalized this).
 - After resolving `profile` by any of the three paths above, if it is
 null (`profile.IsNull()`) fail immediately with a clear, specific error
 ("sketch has no valid profile to extrude" / naming which region index
 produced a null face) rather than letting a doomed `ExtrudeOp` reach
 `pushOperation()` and surface only the generic "the operation failed to
 execute" - an observability improvement flagged in review.
4. Validate `distance`: finite (`std::isfinite`) and nonzero - NOT
 `requirePositive` (see Key decisions on signed distance). New local
 check, not a new shared helper (single call site so far).
5. `optionalBoolString(args, "symmetric", false)` - new small helper (no
 existing bool-from-string parser; `mode`-like strings today are compared
 directly with `==`, so this is one `if (s == "true") ... else if (s ==
 "false") ... else error` block, not a new abstraction).
6. Validate `mode` against the 4 allowed strings (mirror `booleanOp`'s
 if/else-if chain at line ~239), map to `ExtrudeMode`.
7. If `mode != "new_body"`: `requireBodyId(ctx, args, "target_body_id")`.
 Else target stays `-1`.
8. Build the op:
 ```cpp
 auto op = std::make_unique<ExtrudeOp>();
 op->setProfile(profile);
 op->setSketchSource(sketchId);
 op->setDistance(distance);
 op->setDirection(symmetric ? ExtrudeDirection::Symmetric : ExtrudeDirection::Normal);
 op->setMode(mode);
 op->setTargetBody(targetBodyId); // -1 for NewBody
 ```
9. `if (!ctx.history().pushOperation(std::move(op), ctx.document())) return
 {false, "the operation failed to execute"};` - **checked**, mirroring
 every existing handler (`AiToolDispatcher.cpp:159` etc.); the previous
 draft omitted this check, a real gap caught in review. Only after a
 successful push: `ctx.markMeshesDirty();` and return `{true, "<summary
 mentioning body/mode/distance>"}`.

**Explicitly NOT exposed**: `ExtrudeDirection::Custom` (dead in
`ExtrudeOp::execute()`, no backing field) and `draft_angle_degrees` (see
Key decisions - the underlying feature has real, silent-failure bugs).

### 4. Tests
`tests/test_ai_tool_dispatcher.cpp`, new fixture reusing
`tests/test_extrude_regions.cpp:24-35`'s pattern (`Sketch::setPlane` +
`addPoint`/`addLine` to build a sketch with 2+ regions, `doc.addSketch`) - 
cases:
- whole-sketch extrude (no `region_indices`) → succeeds, new body appears.
- single-region extrude → succeeds, extruded volume matches that region's
 area × distance (same assertion style as `test_extrude_regions.cpp`).
- multi-region (2 indices) → succeeds, resulting body's TOTAL volume equals
 the SUM of both regions' area × distance (a compound of two independent
 prisms, not one fused solid - see step 3's compound-semantics note).
- negative `distance` → succeeds, sweeps opposite direction (reverse cut) -
 covers the signed-distance fix from review.
- `symmetric = "true"` with a POSITIVE and with a NEGATIVE `distance` of the
 same magnitude → both succeed and produce the SAME resulting volume/bounds
 (sign is irrelevant in symmetric mode, only magnitude matters - per
 review's symmetric-semantics correction), and total thickness equals
 `abs(distance)`, not `distance` (it's split ± half before fusing).
- out-of-range index → fails, no operation pushed (see "unchanged" note
 below), error message names the bad index.
- an index outside `int` range (e.g. `1e100`) → fails by name, distinct from
 the ordinary out-of-range case - covers the representable-range check
 added in review (reject before cast, not after).
- non-integer index (e.g. `0.5`), non-array `region_indices`, and duplicate
 indices → each fails with a named error, no operation pushed.
- a sketch with zero regions and no `region_indices` given, where
 `buildProfileShape()` legitimately returns a null shape → fails with the
 new specific null-profile error, not the generic "operation failed to
 execute" (covers the null-profile check added in review).
- missing `sketch_id` / nonexistent id → fails via `requireSketchId`.
- `distance == 0`, and non-finite `distance` (`NaN`/`Infinity` via
 `nlohmann::json` numeric literal) → each fails.
- `mode = "subtract"` with no `target_body_id` → fails.
- `mode = "subtract"` with a valid `target_body_id` → succeeds, target
 body's volume decreases (mirror `BooleanOpUnionMergesTwoBodiesIntoOne`'s
 assertion style).
- a case where `pushOperation` itself fails (e.g. `mode = "subtract"` whose
 cut consumes the entire target body, mirroring the existing
 `SplitBodyOp`-adjacent "operation legitimately fails" test pattern) →
 handler returns `{false, ...}`.
- undo/redo: a successful `extrude_sketch` call, then `ctx.history().undo()`
 removes the new body / restores the target body's prior geometry, and
 `redo()` reapplies it - same pattern any other body-creating/-mutating
 tool's dispatcher test already uses.

**"Unchanged" precision, per review**: `History::pushOperation()`
increments `m_revision` unconditionally, even on failure
(`History.cpp:34`) - so every "fails, nothing changed" assertion above must
check the operation COUNT/current position (e.g.
`ctx.history().operationCount()` or equivalent, whatever the class actually
exposes - confirm the real accessor while implementing) and, where
relevant, the target body's actual geometry (volume/shape), NOT
`ctx.history()`'s revision counter, which is not a valid "nothing happened"
signal.

`tests/test_ai_tool_schema.cpp`: beyond the count/name-list and
param-shape updates, assert the SERIALIZED schema for both providers
(`toolsToAnthropicJson`/`toolsToOpenAiJson`) actually emits
`{"type": "array", "items": {"type": "integer"}}` for `region_indices` -
the existing `*ParamShapeMatchesBrief` tests only check the `ToolParam`
struct, not the JSON a provider actually receives, so a broken serializer
could pass them silently (caught in review).

## Key decisions & tradeoffs
- **Strict (fail-loud) out-of-range/non-integer/duplicate region index**,
 deviating from the real UI's silent-skip behavior. Chosen because an LLM
 tool call has no visual feedback loop the way a human clicking regions
 does - a silently-dropped or misinterpreted region is a wrong result the
 model can't detect. Confirmed sound in review (Codex flagged the
 validation as incomplete, not the fail-loud policy itself).
- **`region_indices` as a real JSON-array schema param** (new
 `ToolParamType::IntegerArray`) rather than a comma-separated string.
 Slightly more schema-type-system surface, but every provider's function-
 calling format supports JSON arrays natively, and a string-encoded list
 would need its own parsing/validation code anyway with none of the
 provider-side type-checking benefit.
- **Signed, nonzero `distance` (not positive-only)**, correcting the first
 draft. `ExtrudeOp`/`ExtrudeController` already treat distance as signed
 (`ExtrudeOp.cpp:401`'s `direction = faceNormal * m_distance`, and the
 interactive field is built with `allowSign=true`) - rejecting negative
 values would have silently blocked a normal reverse-direction extrude
 that the real feature already supports. Caught in review; verified
 against both files before accepting.
- **`draft_angle_degrees` dropped from v1 entirely**, not just deferred.
 Verified against `ExtrudeOp.cpp:381-439`: the draft branch never runs for
 `Symmetric` extrudes at all, anchors its neutral plane at world origin
 regardless of the sketch's actual position/offset, and a failed
 `BRepOffsetAPI_DraftAngle` build is swallowed by a bare `catch (...)`
 that silently keeps the undrafted shape - the caller (including this new
 dispatcher handler) has no way to tell a requested draft was ignored.
 Exposing a parameter whose failure mode is silent wrong-but-successful
 output is worse than not exposing it; fixing `ExtrudeOp` itself is a
 separate, unscoped piece of work.
- **Reuse `buildRegions()` + manual compound-building** exactly as
 `Application.cpp` does, rather than adding a new `Sketch` method (e.g.
 `buildCompoundProfile(indices)`) that would abstract the same 10 lines.
 Two call sites (existing UI code + this dispatcher) doing the same small
 thing inline is not yet a shared-abstraction case per this project's
 YAGNI stance; revisit if a third caller appears.
- **No new `ExtrudeMode`/`ExtrudeDirection` values** - the four existing
 modes and Normal/Symmetric directions are exposed as-is; `Custom`
 direction is excluded (dead code, no backing data).

## Risks / open questions
- **The AI has no way to discover a sketch's id or its region indices**
 (raised in review, verified: `AiSessionController.cpp` sends no scene
 state alongside the conversation, and no `list_sketches`/inspection tool
 exists anywhere in `AiToolSchema.cpp`). This is real, but it is not new
 or unique to `extrude_sketch` - `move_body`/`boolean_op`/every other tool
 that takes a `body_id` has the exact same discoverability gap today (the
 AI can only reference an id it created itself in-session, or one the
 human types into chat). Building a scene-inspection tool would meaningfully
 improve usability for ALL of these tools at once, but is a separate,
 larger feature (a new read-only tool + deciding what to expose to the
 model) deserving its own plan - not folded into "add one more
 id-referencing tool following the existing convention." Noting this
 clearly rather than silently accepting the gap or silently expanding
 scope to fix it here.
- `Sketch::buildRegions()` caches its result keyed on a geometry hash
 (`Sketch.cpp:1553-1562`: `m_regionCacheValid && h == m_regionHash` short-
 circuits to the cached vector) - corrected from the first draft's wrong
 claim that it "recomputes every call". Calling it once per dispatcher
 invocation (not once per index) still avoids redundant work regardless.
- Region index stability across sketch edits is a pre-existing property of
 the real UI's own scheme (no stable region id exists anywhere in the
 codebase) - not a new gap introduced by this tool, but worth naming so
 Codex doesn't flag it as a novel bug.
- `ExtrudeOp`'s boolean modes (`Union`/`Subtract`/`Intersect`) return
 `false` with no error string when `m_targetBodyId < 0`
 (`ExtrudeOp.cpp:494/515/534`) - the dispatcher's own pre-validation
 (`requireBodyId` for non-`new_body` modes) must be the only thing standing
 between the AI and that silent-`false` path, since the op itself won't
 explain a failure there.

## Out of scope
- Any UI-visible change (the whole feature is backend-only, driven by the
 AI tool-calling loop).
- `ExtrudeDirection::Custom` and `draft_angle_degrees` (both excluded - see
 Key decisions for why each is a real, separate bug fix, not a v1 feature).
- A shared `Sketch::buildCompoundProfile(indices)` helper (YAGNI - see
 Key decisions).
- A scene-inspection/`list_sketches` tool for the AI to discover sketch ids
 and region indices - real gap, but pre-existing and cross-cutting (see
 Risks), not this plan's problem to solve.
- Fixing `ExtrudeOp`'s draft-angle bugs (symmetric skip, world-origin
 anchor, silent catch-and-keep-undrafted) - unscoped, separate work;
 simply not exposing the parameter sidesteps it for this tool.
- Anything about sketching/mates/patterns beyond this one tool - those
 remain separate future work per the v1 design spec's non-goals list.
