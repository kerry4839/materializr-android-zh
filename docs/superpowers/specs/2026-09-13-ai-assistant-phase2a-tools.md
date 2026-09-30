# AI Assistant Phase 2A: Additional Tool Coverage

## Goal

Extend the AI Assistant's tool schema (currently 9 tools: 5 primitives, 3
transforms, 1 boolean) with 8 more tools wrapping existing `Operation`
subclasses whose constructor/setter args are all plain numbers, strings, or
body IDs - no face/edge/sketch selection required. This is Phase 2A of
"expose all operations as tool calls"; Phase 2B (face/edge-selection ops:
ShellOp, TaperOp, DefeatureOp, ExtrudeOp, FilletOp, etc.) needs a face/edge
reference scheme and is out of scope here.

**Scope change (Codex adversarial review, 2 rounds):** `split_body` and
`sew_bodies` were dropped from this phase. `SplitBodyOp` silently discards
solids beyond the first two when a cut produces 3+ pieces, and `SewOp` can
retain only one shell (deleting the rest as "consumed") even for genuinely
disjoint input bodies - both are pre-existing data-loss bugs in the
underlying operations, not something a tool wrapper can safely paper over.
See the plan's "Follow-up" section for the two bug tickets to file; those
two AI tools become a small follow-up once the underlying ops are fixed.

## Platform Scope

Desktop only, unchanged from the original AI Assistant feature - same
`MZ_MOBILE` exclusion, no new files outside `src/ai/` and
`tests/test_ai_tool_*.cpp`.

## Non-goals

- No face/edge/sketch-profile selection scheme (Phase 2B).
- No new Operation classes - every op below already exists and already has
  a working UI path.
- No change to the agentic loop, LLM clients, or Settings UI.
- No fix to `SplitBodyOp`/`SewOp`'s data-loss bugs (separate follow-up).

## New tools (8)

All follow the existing `AiToolDispatcher.cpp` pattern exactly: validate
args with the existing `requireNumber`/`requirePositive`/`requireBodyId`/
`optionalNumber` helpers plus two new ones this plan adds
(`requireFiniteNumber`/`optionalFiniteNumber`, which layer a
`std::isfinite` check on top of the existing helpers), construct the
`Operation` via `std::make_unique`, call setters, push via
`ctx.history().pushOperation(...)`, call `ctx.markMeshesDirty()` on
success, return a `ToolResult`.

**Coordinate convention reminder** (from `PrimitiveOp`/`TransformOp`): the
AI-facing schema is user-space Z-up; world space is Y-up. Any tool taking a
direction, axis, or point/delta in 3-space must apply the same
`(x,y,z) -> (x,z,y)` remap `TransformOp` uses for move/rotate. A rotation
angle carried through that remap must also be negated (the remap has
determinant -1, reversing handedness) - see `rotate_body`'s existing
`-angle` and `pattern_body`'s radial case below. Tools taking only body
IDs, counts, or scalar (non-directional) distances need no remap.

**Finite-value validation:** every numeric value that feeds an OCCT
construction, and every value COMPUTED from two args (a subtraction, a
cross product), must be checked with `std::isfinite()` - a finite input
pair can still produce a non-finite or degenerate result (e.g. two
opposite-sign extreme coordinates subtracting to infinity).

1. **copy_body** -> `CopyOp`
   - Params: `body_id` (number, required), `dx`,`dy`,`dz` (number, optional,
     default 0 each; user-space, remapped to world before `setOffset`)
   - `op->setSourceBodyId(id); op->setOffset(wx,wy,wz);`
   - Success message includes the new body's id via
     `CopyOp* raw = op.get(); ... raw->getCreatedBodyId()` (capture the raw
     pointer before `std::move`, matching `PrimitiveOp`'s existing idiom in
     `addPrimitive`).

2. **delete_body** -> `DeleteOp`
   - Params: `body_id` (number, required)
   - `op->setBodyId(id);`

3. **separate_body** -> `SeparateBodyOp`
   - Params: `body_id` (number, required)
   - `op->setBody(id);`
   - Success message: report the total body count (`getNewBodyIds().size()
     + 1`, since the getter excludes the retained original) AND the new
     ids, not just the new-body count alone.

4. **mirror_body** -> `MirrorOp`
   - Params: `body_id` (number, required), `plane` (string, required, one
     of `"xy"`,`"xz"`,`"yz"` case-insensitive), `keep_original` (string,
     optional, default `"true"`)
   - Reject any other `plane` value with a clear error (Custom plane is
     Phase 2B - no face/point picking here).
   - **Plane mapping is NOT a straight name-through map**: `MirrorPlane`
     values are world planes, and under the `(x,y,z) -> (x,z,y)` remap,
     user-space `"xy"` (world X/Z) is `MirrorPlane::XZ`, user-space `"xz"`
     (world X/Y) is `MirrorPlane::XY`, and `"yz"` is unchanged. Verified
     independently against `MirrorOp.cpp` during Codex review - do not
     map plane names straight through by identical name.
   - `op->setBody(id); op->setPlane(planeEnum); op->setKeepOriginal(keep);`
   - If `MirrorOp` exposes a getter for the id of the newly-created
     mirrored body, include it in the success message (check the header;
     do not guess the name).

5. **pattern_body** -> `PatternOp`
   - Params: `body_id` (number, required), `type` (string, required,
     `"linear"` or `"radial"`), `count` (number, required, whole number,
     2 to 500 inclusive - `PatternOp` itself rejects `count < 2`), and
     then type-specific:
     - linear: `spacing_x`,`spacing_y`,`spacing_z` (number, optional,
       default 0 each; user-space remapped to world)
     - radial: `axis_x`,`axis_y`,`axis_z` (number, optional, default
       `(0,0,1)` in USER space = straight up, remapped to world before
       use; reject the zero vector), `origin_x`,`origin_y`,`origin_z`
       (number, optional default 0,0,0; remapped),
       `total_angle_degrees` (number, optional, default 360, **negated**
       before `setTotalAngle` to match `rotate_body`'s handedness fix)
   - Reject `type` outside the two values. Reject `count` that is
     non-finite, non-integral, or outside `[2, 500]`.
   - **Spacing convention:** `PatternOp` divides the angle as
     `angle / count`, not `angle / (count - 1)` - a 180-degree total with
     2 instances places the second at 90 degrees, not 180. State this in
     the tool description so the model doesn't assume the "endpoints
     included" convention.

6. **construction_axis** -> `ConstructionAxisOp`
   - Params: `type` (string, required: `"x"`,`"y"`,`"z"`,`"two_points"`),
     for two_points: `p1_x,p1_y,p1_z,p2_x,p2_y,p2_z` (number, required when
     type is two_points; remapped to world), `name` (string, optional)
   - x/y/z map to `AxisCreationType::WorldX/WorldZ/WorldY` respectively (NOT
     WorldX/WorldY/WorldZ straight through) using the SAME user-Z-up
     convention as the rest of the AI tools: the tool's "z" (up) maps to
     `AxisCreationType::WorldY`, and "y" (user depth) maps to
     `AxisCreationType::WorldZ`, per the existing
     `Application_InteractiveOps.cpp:2157` call-site's own remap comment -
     read that call site before implementing to copy the exact mapping.
   - Reject `two_points` inputs closer than `1e-6` apart -
     `ConstructionAxisOp` itself silently falls back to World X below
     `1e-9` and reports success, which would surprise the model.
   - Reject a `name` argument that is present but not a string.
   - `op->setType(...)`; for two_points also
     `op->setPoints(gp_Pnt(...), gp_Pnt(...))`; `op->setName(name)` if given.
   - Success message includes `raw->getCreatedAxisId()`.

7. **construction_plane** -> `ConstructionPlaneOp`
   - Params: `type` (string, required: `"xy"`,`"xz"`,`"yz"`, already
     user-space - `ConstructionPlaneOp` interprets these directly in
     user-space terms, unlike `construction_axis`; do NOT apply the world
     remap here), `offset` (number, optional, default 0), `name` (string,
     optional)
   - OffsetFromPlane/ThreeQPoints/etc. are Phase 2B (need a base
     plane/face or 3 arbitrary points beyond simple standard planes) -
     only the three standard planes ship in this tool.
   - Reject a `name` argument that is present but not a string.
   - `op->setType(...); op->setOffset(offset);` `setName` if given.

8. **align_body** -> implemented as a `TransformOp` translation, NOT `AlignOp`
   - Params: `body_id` (number, required), `source_x,source_y,source_z`
     (number, required), `target_x,target_y,target_z` (number, required)
   - **Not `AlignOp`:** `AlignOp::execute()` calls `Document::updateBody()`,
     which drops face lineage, and does not carry `TransformOp`'s existing
     mate-placed-body rejection; it also has zero verified UI call-sites in
     this branch. Instead, compute `(target - source)`, remap to world
     space, and push a `TransformOp` translation by that delta - the same
     fully-verified path `move_body` already uses.
   - Check the computed delta for finiteness after subtraction (two finite
     inputs at opposite extremes can subtract to infinity).
   - `op->setBodyId(id); op->setType(TransformType::Translate);
     op->setTranslation(dx,dz,dy);`

## Error handling

Same as existing tools: every validation failure returns
`ToolResult{false, "<clear message>"}` before any `Operation` is
constructed; `pushOperation` failure returns
`ToolResult{false, "the operation failed to execute"}`; success always
calls `ctx.markMeshesDirty()` before returning `ToolResult{true, ...}`.

## Testing

Extend `tests/test_ai_tool_schema.cpp` (schema shape: exactly 17 tools
total - 9 pre-existing + 8 new - each new one present with correct
required/optional params, and every pre-existing tool name still present
verbatim) and `tests/test_ai_tool_dispatcher.cpp` (one success + one
validation-failure test per new tool, minimum; `pattern_body` needs a
linear case, a radial handedness-vs-rotate_body case at the CORRECT
angle/count spacing, a nonzero-origin case, and a zero-radial-axis
rejection case; `mirror_body` needs an off-origin asymmetric-coordinate
case that would fail under a wrong plane mapping; `construction_axis`
needs a coincident-points rejection case; `align_body` needs a real
bounding-box-shift assertion with asymmetric offsets, since no UI
reference exists for `AlignOp` and this tool doesn't use it anyway).

## Open risk (resolved during review)

The original draft wrapped `AlignOp` directly, which had zero real call
sites in this branch to verify setter order or edge-case behavior against.
Resolved by not using `AlignOp` at all - `align_body` is a `TransformOp`
translation instead (see above), reusing `move_body`'s fully-verified code
path.
