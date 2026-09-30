# Plan Review Log: add extrude_sketch to the AI Assistant tool schema
Started 2026-09-14 (session time). MAX_ROUNDS=5. Model: gpt-6-astra (config default).

## Round 1 - Codex
VERDICT: REVISE. Seven findings (5 P1, 2 P2) plus two smaller corrections:
1. P1 - `pushOperation()`'s return value was never checked, unlike every
 existing handler - a failed extrude (empty profile, degenerate cut)
 would be reported as success.
2. P1 - `distance` was validated as positive-only, but `ExtrudeOp`/
 `ExtrudeController` already treat it as signed (sign reverses sweep
 direction) - would have blocked a normal reverse-direction extrude.
3. P1 - `draft_angle_degrees` exposed a feature with real, silent-failure
 bugs (skipped for Symmetric, world-origin-anchored neutral plane,
 swallowed exceptions keep the undrafted shape with no error).
4. P1 - the AI has no way to discover a sketch's id or region indices at
 all (no scene-context injection, no inspection tool anywhere).
5. P1 - `region_indices` validation didn't reject fractional/non-array/
 duplicate values.
6. P2 - the planned `distance` validator (`requirePositive`) doesn't check
 finiteness (NaN/Infinity both pass its `<= 0.0` check).
7. P2 - the planned schema tests only check `ToolParam` shape, not the
 actual serialized provider JSON, so a broken array serializer could
 pass silently.
Smaller: `buildRegions()` is geometry-hash-cached, not "recomputed every
call"; "union of region prisms" is topologically imprecise (a compound of
separate prisms, not a fused solid).

### Claude's response
Verified every finding against the actual source (`AiToolDispatcher.cpp`,
`ExtrudeOp.cpp`, `ExtrudeController.cpp`, `Sketch.cpp`, `AiSessionController.cpp`)
before acting - all seven were real. Adopted 1, 2, 3, 5, 6, 7 and both
smaller corrections in full: pushOperation return now checked; distance is
now finite+nonzero (signed) instead of positive-only; draft_angle_degrees
removed from the tool entirely (not fixed - the underlying ExtrudeOp bugs
are unscoped, separate work); region_indices now requires a real array of
finite non-negative integers with duplicates rejected, and the JSON Schema
itself declares `items: {"type": "integer"}` (renamed the new enum value
IntegerArray, not NumberArray); new schema tests assert the actual
serialized provider JSON, not just the ToolParam struct; wording/test
assertions corrected for the cache and compound-of-solids facts.

Finding 4 (sketch/region discoverability) is real but explicitly NOT
folded into this plan: verified it's a pre-existing, cross-cutting gap
already affecting every id-referencing tool shipped today (move_body,
boolean_op, etc.), not something new_body introduced by extrude_sketch.
Building a scene-inspection tool is valuable but is its own separate
feature/plan - scope-creeping this plan to build it would go well beyond
"add one more tool following the existing referencing convention." Logged
as a Risk in PLAN.md rather than silently dropped.

## Round 2 - Codex
VERDICT: APPROVED. Agreed with deferring scene-inspection scope (with a
qualification: the gap is worse for sketches than bodies since the AI
can't create sketches to learn their ids the way it can with bodies -
logged, doesn't change the scope call). Five nonblocking corrections:
1. Symmetric mode's total thickness is `abs(distance)`; sign has no effect
 on the result in that mode (only magnitude matters) - undocumented.
2. "N regions -> N separate solids" was too broad - only true for ordinary
 non-symmetric `new_body` mode; Symmetric fuses, boolean modes combine
 with the target.
3. Missing test coverage: symmetric execution, undo/redo, and confirming a
 failed execution leaves the target body's geometry unchanged - plus
 "history unchanged" needs precision since `History::pushOperation()`
 increments its revision even on failure (`History.cpp:34`).
4. Bounds/range-check region indices for `int`-representability BEFORE
 casting/dedup-set-insertion, not just integrality - `1e100` passes
 `floor(v)==v` but isn't a safe cast.
5. (Non-blocking upgrade) reject a null resolved profile before building
 the op, for a specific error instead of the generic execution failure.

### Claude's response
Verified `History.cpp:34` (`++m_revision` unconditional) and
`ExtrudeOp.cpp:381-397` (Symmetric fuses both half-sweeps into one solid)
directly before accepting. Adopted all five in full: documented
symmetric's sign-irrelevance and abs(distance) thickness in the tool
description; scoped the "N separate solids" claim to ordinary non-symmetric
new_body mode only; added symmetric (+/- sign, same result), undo/redo, and
null-profile test cases; added an explicit int-range check before
integrality/dedup for region indices, with an oversized-index (`1e100`)
test; added the null-profile pre-check with a specific error; corrected
every "unchanged" test assertion to check operation count/geometry, not
`History`'s revision counter. Converged round 2, no rejections this round -
every finding held up under verification.

## Post-implementation review (validate-code gate)

After implementing, ran the full test suite (119/119), all 3 repo gates, a
security/perf pass, and dispatched `standards-spec-review` (two parallel
sub-agents: Standards against CONTRIBUTING.md + Fowler baseline, Spec
against this PLAN.md/log).

**Security pass (self, before the sub-agents ran):** found `region_indices`
accumulated and validated the ENTIRE JSON array before the real bounds
check (against `buildRegions().size()`) ever ran - an oversized array did
unbounded work before failing. Fixed with a cap; later replaced by the
Standards sub-agent's better suggestion (see below).

**Spec sub-agent findings (2, both test-coverage gaps, not code bugs):**
1. No test for the null-profile path (empty sketch, no region_indices).
2. No test for `pushOperation` itself failing (subtract consuming the
 entire target). Round 2's item 3 explicitly asked for "confirming a
 failed execution leaves the target body's geometry unchanged" - this
 specific case was missing.
Both adopted: `ExtrudeSketchRejectsAnEmptySketchWithNoValidProfile` and
`ExtrudeSketchSubtractConsumingTheEntireTargetFailsAndLeavesItUnchanged`
added, both pass on the first run.

**Standards sub-agent findings (6):**
1. **Validation ordering** (hard violation of CONTRIBUTING's "match the
 surrounding style" - every sibling handler validates cheap args before
 doing real kernel work). `extrudeSketch` built the profile (real OCCT
 work) before validating `distance`/`symmetric`/`mode`/`target_body_id`.
 Adopted: reordered so all cheap argument checks run first, region
 resolution (the only OCCT-touching part) runs last.
2. **Duplicated Code** - `requireSketchId` was a near-clone of
 `requireBodyId`, and `validateRegionIndex` repeated the same finite/
 range/integral cascade a third time. Adopted: extracted a shared
 `parseWholeNumber(raw, out, label, err)` all three now call, matching
 the file's own composition idiom (`requirePositive` already calls
 `requireNumber`). Re-verified byte-identical error text for the
 existing `requireBodyId` call sites (no test regressions).
3. **Feature Envy / cross-file duplication** - the region-index-to-compound
 logic mirrors `Application.cpp`'s interactive multi-region extrude path.
 Suggested moving it into a shared `Sketch` method. **Declined**: this
 was an explicit, already-argued tradeoff in this plan's own "Key
 decisions" section ("two call sites... is not yet a shared-abstraction
 case per this project's YAGNI stance; revisit if a third caller
 appears"), reviewed and not flagged by Codex in either round. Adopting
 it now would mean refactoring a separate, already-shipped interactive
 feature outside this task's approved scope, with no way to manually
 smoke-test the UI path in this environment. Logged, not acted on.
4. **Speculative Generality** - the `kMaxRegionIndices = 1000` magic-number
 cap (my own security-pass addition) was arbitrary. Adopted the
 sub-agent's fix, which is strictly better: bound the array length
 against the sketch's actual `regions.size()` (computed right there
 anyway) instead of a constant - exact, not a guess, and dissolves once
 validation was reordered per #1.
5. **Repeated Switches** (minor) - `typeName()` handles 2 of 3
 `ToolParamType`s, caller special-cases the third. **Declined**: the
 sub-agent itself called this minor; restructuring the type system for
 one array type isn't a clear readability win.
6. **Comment style drift** - comments cited process artifacts ("per Codex
 review round 2", "PLAN.md's compound-semantics correction") instead of
 code facts, denser than sibling handlers. This is a direct violation of
 this session's own operating rule (comments shouldn't reference the
 current task/fix/review - they rot as the codebase evolves). Adopted:
 stripped every process reference, rewrote comments to state the
 underlying fact only.

Re-ran the full suite (119/119) and all 3 repo gates after every fix in
this addendum - all green throughout.
