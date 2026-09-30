# AI Assistant Phase 2A Tools Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add 8 new AI tool wrappers (copy_body, delete_body, separate_body, align_body, mirror_body, pattern_body, construction_axis, construction_plane) to the existing AI Assistant feature, bringing tool coverage from 9 (5 primitives, 3 transforms, 1 boolean) to 17.

**Architecture:** Extend the existing `src/ai/AiToolSchema.cpp` table and `src/ai/AiToolDispatcher.cpp` dispatch switch - no new files, no new architecture. Every new tool follows the established validate -> construct Operation -> setters -> pushOperation -> markMeshesDirty pattern exactly.

**Tech Stack:** C++17, OCCT (gp_Pnt/gp_Dir/gp_Pln), nlohmann::json, GoogleTest.

**Spec:** docs/superpowers/specs/2026-09-13-ai-assistant-phase2a-tools.md

**Scope change from the original draft (Codex review rounds 1-2):** `split_body` and `sew_bodies` were dropped from this phase. Both wrap `Operation` subclasses with genuine, pre-existing data-loss bugs that no tool wrapper can safely paper over without modifying the underlying op:

- `SplitBodyOp` (`src/modeling/SplitBodyOp.cpp`) accepts any cut result with 2+ solids and silently keeps only the first two, discarding the rest - a plane through a shape that splits into 3+ pieces reports success while quietly losing geometry.
- `SewOp` (`src/modeling/SewOp.cpp`) can retain only the first shell when zero free edges remain and delete every other input body as "consumed," even when the inputs are genuinely disjoint (e.g. two separated closed boxes) - this loses a whole body's geometry while reporting success.

These are real bugs in the underlying operations, not AI-wrapper design gaps, and fixing them (audit every cut/sew result path in each op) is its own bounded piece of work, independent of any AI tooling. Flag both as separate bug-fix tickets; `split_body`/`sew_bodies` AI tools can be added once those fixes land, as a small follow-up to this plan.

## Global Constraints

- Every tool validates ALL args and returns `ToolResult{false, "<message>"}` before constructing any `Operation` - never partially construct then fail.
- Every success path calls `ctx.markMeshesDirty()` before returning `ToolResult{true, ...}` (the Critical bug fixed in commit ef8b3c1 for the original 9 tools - do not reintroduce it here).
- User-space is Z-up; world space is Y-up. Any tool taking a direction/axis/normal/point-in-3-space must remap `(x,y,z) -> (x,z,y)` before constructing OCCT types, matching `TransformOp`'s existing convention. Read `src/modeling/PrimitiveOp.cpp`'s `worldPnt()` and `src/ai/AiToolDispatcher.cpp`'s `moveBody`/`rotateBody` before writing any new tool with a spatial arg - copy their exact remap, do not re-derive it.
- Every numeric value read from tool args that feeds an OCCT construction (a point, a direction, a distance, a delta) must be checked with `std::isfinite()` after retrieval, and any value COMPUTED from two args (a subtraction, a cross product) must be checked with `std::isfinite()` again after computing it - a finite input pair can still produce a non-finite or degenerate result (e.g. two huge opposite-sign coordinates subtracting to infinity, or a near-zero cross product). Add a small `requireFiniteNumber(args, key, out, err)` wrapper around the existing `requireNumber` (and `optionalNumber` variant) that adds the `std::isfinite` check, and use it for every new tool's spatial/numeric args instead of the bare `requireNumber`/`optionalNumber`.
- No em dashes in code or comments (`tools/no_em_dashes.py` gate).
- Final tool count in `allTools()` must be exactly 17 (9 existing + 8 new); `tests/test_ai_tool_schema.cpp` must assert this exact count, not `>= 17`, and must keep asserting every one of the 9 pre-existing tool names verbatim.
- Every tool whose underlying `Operation` exposes a getter for a newly-created body id (check the op's header directly, do not guess) must include that id in its success message, so a follow-up AI turn can reference it without guessing.
- Build the WHOLE project (`cmake --build build_new -j`), never just a single target, and always AFTER all test code for the task is written, immediately before running `ctest` - a target-scoped or premature build can let a stale test binary silently pass.
- **Codex review rounds 1-2 findings incorporated below inline, per-task.** Full critique transcript: `docs/superpowers/plans/2026-09-13-ai-assistant-phase2a-tools-REVIEW-LOG.md`.

---

### Task 1: copy_body, delete_body, separate_body, align_body (4 body-id-based ops)

**Files:**
- Modify: `src/ai/AiToolSchema.cpp` (add 4 tool defs to `allTools()`)
- Modify: `src/ai/AiToolDispatcher.cpp` (add 4 functions + dispatch cases, plus the new `requireFiniteNumber`/`optionalFiniteNumber` helpers)
- Modify: `src/ai/AiToolDispatcher.h` (declare new functions if the header declares them individually; if `executeTool` is the only declared entry point, no header change needed for the others)
- Modify: `tests/test_ai_tool_schema.cpp`, `tests/test_ai_tool_dispatcher.cpp`

**Interfaces:**
- Consumes: `PluginContext::document()`, `::history()`, `::markMeshesDirty()`; `Document::getAllBodyIds()`; `requireNumber`, `requirePositive`, `requireBodyId`, `optionalNumber` from `AiToolDispatcher.cpp`; `CopyOp`, `DeleteOp`, `SeparateBodyOp`, `TransformOp` from `src/modeling/*.h`.
- Produces: `requireFiniteNumber`/`optionalFiniteNumber` helpers (new, used by every task in this plan); 4 new entries in `allTools()`; 4 new `if (toolName == "...")` branches in `executeTool()`.

**Codex round-1 ruling on `align_body` (still applies):** implemented as a `TransformOp` translation by `(target - source)`, remapped to world space exactly like `move_body`, instead of the unverified `AlignOp` (which drops face lineage and has no verified call-site in this branch). `AlignOp` itself is not touched or exposed by this plan.

**Codex round-2 finding on finite-value validation (applies to every task in this plan):** the existing `requireNumber`/`optionalNumber` helpers check type only, not finiteness, and a computed value (like `align_body`'s `target - source`) can overflow to infinity even from two finite inputs. Fixed by adding `requireFiniteNumber`/`optionalFiniteNumber` wrappers and checking every computed delta again after the arithmetic.

- [ ] **Step 1: Add the finite-value validation helpers**

In `src/ai/AiToolDispatcher.cpp`, near the existing `requireNumber`/`requirePositive`/`requireBodyId`/`optionalNumber` helpers, add:

```cpp
bool requireFiniteNumber(const nlohmann::json& args, const char* key, double& out, std::string& err) {
    if (!requireNumber(args, key, out, err)) return false;
    if (!std::isfinite(out)) {
        err = std::string(key) + " must be a finite number";
        return false;
    }
    return true;
}

bool optionalFiniteNumber(const nlohmann::json& args, const char* key, double& out, double fallback, std::string& err) {
    if (!optionalNumber(args, key, out, fallback, err)) return false;
    if (!std::isfinite(out)) {
        err = std::string(key) + " must be a finite number";
        return false;
    }
    return true;
}
```

Add `#include <cmath>` to the top of `AiToolDispatcher.cpp` if not already present.

- [ ] **Step 2: Add tool schemas**

In `src/ai/AiToolSchema.cpp`, inside `allTools()`'s returned vector, add (placement: after the existing `boolean_op` entry, before the closing `};`):

```cpp
        ToolDef{"copy_body", "Duplicate an existing body, optionally offset by (dx, dy, dz).",
            {num("body_id", "The id of the body to copy."),
             num("dx", "Offset along X in mm.", false),
             num("dy", "Offset along Y (user-space depth) in mm.", false),
             num("dz", "Offset along Z (user-space height/up) in mm.", false)}},
        ToolDef{"delete_body", "Permanently remove a body from the document.",
            {num("body_id", "The id of the body to delete.")}},
        ToolDef{"separate_body", "Split a body with multiple disconnected solid shells into separate bodies, one per shell.",
            {num("body_id", "The id of the body to separate.")}},
        ToolDef{"align_body", "Move a body so a chosen point on it lands on a chosen target point in space.",
            {num("body_id", "The id of the body to align."),
             num("source_x", "X of the point on the body, in mm."),
             num("source_y", "Depth (user-space Y) of the point on the body, in mm."),
             num("source_z", "Height (user-space Z, up) of the point on the body, in mm."),
             num("target_x", "X of the destination point, in mm."),
             num("target_y", "Depth (user-space Y) of the destination point, in mm."),
             num("target_z", "Height (user-space Z, up) of the destination point, in mm.")}},
```

- [ ] **Step 3: Implement the 4 dispatcher functions**

```cpp
ToolResult copyBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err)) return {false, err};
    double dx = 0, dy = 0, dz = 0;
    if (!optionalFiniteNumber(args, "dx", dx, 0.0, err)) return {false, err};
    if (!optionalFiniteNumber(args, "dy", dy, 0.0, err)) return {false, err};
    if (!optionalFiniteNumber(args, "dz", dz, 0.0, err)) return {false, err};
    auto op = std::make_unique<CopyOp>();
    op->setSourceBodyId(bodyId);
    op->setOffset(dx, dz, dy);
    CopyOp* raw = op.get();
    if (!ctx.history().pushOperation(std::move(op), ctx.document())) {
        return {false, "the operation failed to execute"};
    }
    ctx.markMeshesDirty();
    return {true, "Copied body " + std::to_string(bodyId) + " to new body " +
                  std::to_string(raw->getCreatedBodyId()) + "."};
}

ToolResult deleteBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err)) return {false, err};
    auto op = std::make_unique<DeleteOp>();
    op->setBodyId(bodyId);
    if (!ctx.history().pushOperation(std::move(op), ctx.document())) {
        return {false, "the operation failed to execute"};
    }
    ctx.markMeshesDirty();
    return {true, "Deleted body " + std::to_string(bodyId) + "."};
}

ToolResult separateBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err)) return {false, err};
    auto op = std::make_unique<SeparateBodyOp>();
    op->setBody(bodyId);
    SeparateBodyOp* raw = op.get();
    if (!ctx.history().pushOperation(std::move(op), ctx.document())) {
        return {false, "the operation failed to execute"};
    }
    ctx.markMeshesDirty();
    // getNewBodyIds() returns only the NEW bodies split off, not counting the
    // original body id that survives - include both counts explicitly.
    std::string idList;
    for (int id : raw->getNewBodyIds()) idList += std::to_string(id) + " ";
    return {true, "Separated body " + std::to_string(bodyId) + " into " +
                  std::to_string(raw->getNewBodyIds().size() + 1) + " bodies total: original id " +
                  std::to_string(bodyId) + ", new ids " + idList + "."};
}

ToolResult alignBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err)) return {false, err};
    double sx, sy, sz, tx, ty, tz;
    if (!requireFiniteNumber(args, "source_x", sx, err)) return {false, err};
    if (!requireFiniteNumber(args, "source_y", sy, err)) return {false, err};
    if (!requireFiniteNumber(args, "source_z", sz, err)) return {false, err};
    if (!requireFiniteNumber(args, "target_x", tx, err)) return {false, err};
    if (!requireFiniteNumber(args, "target_y", ty, err)) return {false, err};
    if (!requireFiniteNumber(args, "target_z", tz, err)) return {false, err};
    double dx = tx - sx, dy = ty - sy, dz = tz - sz;
    if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz)) {
        return {false, "the distance between source and target is too large to represent"};
    }
    // Implemented as a TransformOp translation by (target - source), the same
    // verified path move_body already uses, rather than AlignOp directly - see
    // the Codex round-1 ruling above (AlignOp drops face lineage and has no
    // verified call-site in this branch).
    auto op = std::make_unique<TransformOp>();
    op->setBodyId(bodyId);
    op->setType(TransformType::Translate);
    op->setTranslation(dx, dz, dy);
    if (!ctx.history().pushOperation(std::move(op), ctx.document())) {
        return {false, "the operation failed to execute"};
    }
    ctx.markMeshesDirty();
    return {true, "Aligned body " + std::to_string(bodyId) + "."};
}
```

Add `#include "../modeling/CopyOp.h"`, `"../modeling/DeleteOp.h"`, `"../modeling/SeparateBodyOp.h"` to the top of `AiToolDispatcher.cpp` if not already present (`TransformOp.h` is already included since `move_body`/`rotate_body`/`scale_body` use it). Do NOT include `AlignOp.h` - it is not used by this plan.

- [ ] **Step 4: Wire the 4 new cases into `executeTool()`**

```cpp
    if (toolName == "copy_body") return copyBody(ctx, args);
    if (toolName == "delete_body") return deleteBody(ctx, args);
    if (toolName == "separate_body") return separateBody(ctx, args);
    if (toolName == "align_body") return alignBody(ctx, args);
```

- [ ] **Step 5: Write dispatcher tests**

In `tests/test_ai_tool_dispatcher.cpp`, following the file's existing fixture pattern (a `Document`/`History`/`PluginContext` set up per test, one body added via a prior `PrimitiveOp` push where a body is needed), add:

```cpp
TEST_F(AiToolDispatcherTest, CopyBodyCreatesANewBody) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"dx", 5.0}, {"dy", 0.0}, {"dz", 0.0}};
    ToolResult result = executeTool(ctx(), "copy_body", args);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 2u);
}

TEST_F(AiToolDispatcherTest, CopyBodyRejectsAnUnknownBodyId) {
    nlohmann::json args = {{"body_id", 9999}};
    ToolResult result = executeTool(ctx(), "copy_body", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, DeleteBodyRemovesIt) {
    int bodyId = addTestBox();
    ToolResult result = executeTool(ctx(), "delete_body", {{"body_id", bodyId}});
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 0u);
}

TEST_F(AiToolDispatcherTest, DeleteBodyRejectsAnUnknownBodyId) {
    ToolResult result = executeTool(ctx(), "delete_body", {{"body_id", 9999}});
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, SeparateBodyRejectsAnUnknownBodyId) {
    ToolResult result = executeTool(ctx(), "separate_body", {{"body_id", 9999}});
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, SeparateBodySplitsATwoShellBodyIntoTwoBodies) {
    // Two disjoint boxes unioned with keep-tool-off (or however the fixture's
    // existing boolean_op test creates a two-shell body) so separate_body has
    // a real multi-shell input to split, not just an id-rejection path.
    int bodyId = addTestBoxWithTwoDisjointShells();
    ToolResult result = executeTool(ctx(), "separate_body", {{"body_id", bodyId}});
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 2u);
}

TEST_F(AiToolDispatcherTest, AlignBodyMovesTheSourcePointToTheTargetPoint) {
    int bodyId = addTestBox(); // assume addTestBox() creates a box with a known bbox min at origin
    Bnd_Box before = boundingBoxForBody(bodyId); // use the same bbox helper the existing
                                                  // move_body/rotate_body tests already use -
                                                  // do not invent a new one; read the fixture first.
    nlohmann::json args = {
        {"body_id", bodyId},
        {"source_x", 0.0}, {"source_y", 0.0}, {"source_z", 0.0},
        {"target_x", 10.0}, {"target_y", 3.0}, {"target_z", 7.0}
    };
    ToolResult result = executeTool(ctx(), "align_body", args);
    EXPECT_TRUE(result.ok);
    Bnd_Box after = boundingBoxForBody(bodyId);
    // Deliberately asymmetric (10, 3, 7) offset: a wrong y/z remap in align_body
    // would shift the bbox by (10, 7, 3) instead, which this assertion catches
    // and a symmetric or on-axis-only offset would not.
    double bx0, by0, bz0, bx1, by1, bz1, ax0, ay0, az0, ax1, ay1, az1;
    before.Get(bx0, by0, bz0, bx1, by1, bz1);
    after.Get(ax0, ay0, az0, ax1, ay1, az1);
    EXPECT_NEAR(ax0 - bx0, 10.0, 1e-6);
    EXPECT_NEAR(ay0 - by0, 7.0, 1e-6);  // world Y == user-space Z (up)
    EXPECT_NEAR(az0 - bz0, 3.0, 1e-6);  // world Z == user-space Y (depth)
}

TEST_F(AiToolDispatcherTest, AlignBodyRejectsAnUnknownBodyId) {
    nlohmann::json args = {
        {"body_id", 9999},
        {"source_x", 0.0}, {"source_y", 0.0}, {"source_z", 0.0},
        {"target_x", 1.0}, {"target_y", 1.0}, {"target_z", 1.0}
    };
    ToolResult result = executeTool(ctx(), "align_body", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, AlignBodyRejectsNonFiniteCoordinates) {
    int bodyId = addTestBox();
    nlohmann::json args = {
        {"body_id", bodyId},
        {"source_x", 0.0}, {"source_y", 0.0}, {"source_z", 0.0},
        {"target_x", 1e308}, {"target_y", 0.0}, {"target_z", 0.0}
    };
    // Not non-finite on its own, but paired with a source at the opposite
    // extreme the subtraction overflows - use a source that makes dx infinite.
    args["source_x"] = -1e308;
    ToolResult result = executeTool(ctx(), "align_body", args);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 1u); // no mutation happened
}
```

Read the existing `tests/test_ai_tool_dispatcher.cpp` fixture first to get the real helper name for "add a test box" and the real bounding-box accessor - the snippets above use placeholder names (`addTestBox()`, `boundingBoxForBody()`) that must be replaced with whatever the file's existing tests for `add_box`/`move_body` actually call. Do not invent new test infrastructure; reuse what's there.

- [ ] **Step 6: Build the whole project, then run tests**

Run: `cmake --build build_new -j` (full build, not a single target - a target-scoped build can leave a stale test binary that silently passes)
Run: `ctest --test-dir build_new -R test_ai_tool_dispatcher --output-on-failure --no-tests=error`
Expected: all pass, including pre-existing tests.

- [ ] **Step 7: Commit**

```bash
git add src/ai/AiToolSchema.cpp src/ai/AiToolDispatcher.cpp src/ai/AiToolDispatcher.h tests/test_ai_tool_dispatcher.cpp
git commit -m "ai: add copy_body, delete_body, separate_body, align_body tools"
```

---

### Task 2: mirror_body

**Files:**
- Modify: `src/ai/AiToolSchema.cpp`, `src/ai/AiToolDispatcher.cpp`, `tests/test_ai_tool_schema.cpp`, `tests/test_ai_tool_dispatcher.cpp`

**Interfaces:**
- Consumes: `MirrorOp` (+ `MirrorPlane` enum) from `src/modeling/`.

**Codex round-1 ruling on `mirror_body`'s plane mapping (verified independently by Codex against `MirrorOp.cpp` in round 2 - confirmed correct):** `MirrorPlane` values are WORLD planes, and this codebase's `(x,y,z) -> (x,z,y)` user-to-world remap means a user-space "xy" plane (built from world axes X and Z) is `MirrorPlane::XZ` in world terms, and a user-space "xz" plane (world axes X and Y) is `MirrorPlane::XY`. `"yz"` is unaffected. Mapping: `"xy"` -> `MirrorPlane::XZ`, `"xz"` -> `MirrorPlane::XY`, `"yz"` -> `MirrorPlane::YZ`.

- [ ] **Step 1: Add tool schema**

```cpp
        ToolDef{"mirror_body", "Create a mirrored copy of a body across a standard plane.",
            {num("body_id", "The id of the body to mirror."),
             str("plane", "Which standard plane to mirror across: \"xy\", \"xz\", or \"yz\"."),
             str("keep_original", "\"true\" to keep the original body, \"false\" to replace it. Defaults to \"true\".", false)}},
```

- [ ] **Step 2: Implement the dispatcher function**

`MirrorOp` exposes `getMirroredBodyId()` for the id of the newly-created mirrored body (confirmed by Codex against the header during review) - capture the raw pointer before `std::move` (as `copyBody` does) and include the new id in the success message.

```cpp
ToolResult mirrorBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err)) return {false, err};
    if (!args.contains("plane") || !args["plane"].is_string()) {
        return {false, "plane is required and must be a string"};
    }
    std::string plane = args["plane"].get<std::string>();
    std::transform(plane.begin(), plane.end(), plane.begin(), ::tolower);
    // User-space xy (world X/Z) -> MirrorPlane::XZ; user-space xz (world X/Y)
    // -> MirrorPlane::XY. See the Codex ruling above - do not map these two
    // straight through by name, that mirrors the wrong axis.
    MirrorPlane mp;
    if (plane == "xy") mp = MirrorPlane::XZ;
    else if (plane == "xz") mp = MirrorPlane::XY;
    else if (plane == "yz") mp = MirrorPlane::YZ;
    else return {false, "plane must be \"xy\", \"xz\", or \"yz\""};
    bool keep = true;
    if (args.contains("keep_original")) {
        if (!args["keep_original"].is_string()) return {false, "keep_original must be a string"};
        std::string k = args["keep_original"].get<std::string>();
        if (k == "false") keep = false;
        else if (k != "true") return {false, "keep_original must be \"true\" or \"false\""};
    }
    auto op = std::make_unique<MirrorOp>();
    op->setBody(bodyId);
    op->setPlane(mp);
    op->setKeepOriginal(keep);
    MirrorOp* raw = op.get();
    if (!ctx.history().pushOperation(std::move(op), ctx.document())) {
        return {false, "the operation failed to execute"};
    }
    ctx.markMeshesDirty();
    // getMirroredBodyId() is -1 when keep_original is false (MirrorOp.cpp
    // updates the original body in place instead of creating a new one) -
    // report the retained bodyId in that case, not the sentinel.
    if (keep) {
        return {true, "Mirrored body " + std::to_string(bodyId) + " across the " + plane +
                      " plane. New body id " + std::to_string(raw->getMirroredBodyId()) + "."};
    }
    return {true, "Mirrored body " + std::to_string(bodyId) + " across the " + plane +
                  " plane in place (original replaced, body id " + std::to_string(bodyId) + " unchanged)."};
}
```

Verify `MirrorOp.h`'s exact `MirrorPlane` enum name/values against the actual header before writing this. Add `#include "../modeling/MirrorOp.h"` and `#include <algorithm>` to `AiToolDispatcher.cpp` if not already present.

- [ ] **Step 3: Wire dispatch case**

```cpp
    if (toolName == "mirror_body") return mirrorBody(ctx, args);
```

- [ ] **Step 4: Write tests**

```cpp
TEST_F(AiToolDispatcherTest, MirrorBodyAcrossXyPlaneMirrorsTheHeightAxis) {
    // xy in user-space maps to world XZ (see the ruling above): mirroring
    // across it must flip user-space Z (height), not Y (depth). Place the
    // test box off-origin on both axes so a wrong mapping (flipping depth
    // instead of height) is distinguishable. Compose the off-origin
    // placement from the existing add_box + move_body tools rather than
    // inventing new fixture infrastructure.
    int bodyId = addTestBox();
    executeTool(ctx(), "move_body", {{"body_id", bodyId}, {"dx", 5.0}, {"dy", 3.0}, {"dz", 4.0}});
    Bnd_Box before = boundingBoxForBody(bodyId);
    nlohmann::json args = {{"body_id", bodyId}, {"plane", "xy"}};
    ToolResult result = executeTool(ctx(), "mirror_body", args);
    EXPECT_TRUE(result.ok);
    ASSERT_EQ(document().getAllBodyIds().size(), 2u); // keep_original defaults true
    // Find the new body's id (whichever id in getAllBodyIds() is not bodyId,
    // using the fixture's existing id-diffing helper if one exists) and get
    // its bounding box. Its user-space height (world Y) coordinates should be
    // negated relative to `before`'s, while X and user-space depth (world Z)
    // stay the same.
}

TEST_F(AiToolDispatcherTest, MirrorBodyRejectsAnInvalidPlane) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"plane", "diagonal"}};
    ToolResult result = executeTool(ctx(), "mirror_body", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, MirrorBodyWithKeepOriginalFalseReplacesTheBody) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"plane", "xy"}, {"keep_original", "false"}};
    ToolResult result = executeTool(ctx(), "mirror_body", args);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 1u);
    // getMirroredBodyId() is -1 when keep_original is false - the message
    // must report the retained bodyId, not that sentinel.
    EXPECT_EQ(result.message.find("-1"), std::string::npos);
    EXPECT_NE(result.message.find(std::to_string(bodyId)), std::string::npos);
}
```

- [ ] **Step 5: Build the whole project, then run tests**

Run: `cmake --build build_new -j`
Run: `ctest --test-dir build_new -R test_ai_tool_dispatcher --output-on-failure --no-tests=error`

- [ ] **Step 6: Commit**

```bash
git add src/ai/AiToolSchema.cpp src/ai/AiToolDispatcher.cpp tests/test_ai_tool_dispatcher.cpp
git commit -m "ai: add mirror_body tool"
```

---

### Task 3: pattern_body, construction_axis, construction_plane (+ final schema-count test)

**Files:**
- Modify: `src/ai/AiToolSchema.cpp`, `src/ai/AiToolDispatcher.cpp`, `tests/test_ai_tool_schema.cpp`, `tests/test_ai_tool_dispatcher.cpp`

**Interfaces:**
- Consumes: `PatternOp` (+`PatternType`), `ConstructionAxisOp` (+`AxisCreationType`), `ConstructionPlaneOp` (+`PlaneCreationType`) from `src/modeling/`.
- Produces: final `allTools()` with exactly 17 entries - this task also updates the schema test's tool-count assertion.

**Codex rulings on `pattern_body`, accepted (rounds 1-2):**
1. **Radial handedness:** `rotate_body` already negates its angle before calling `TransformOp::setRotation` because the coordinate swap reverses handedness (determinant -1). `PatternOp`'s radial angle is applied directly in world coordinates with no such compensation inside `PatternOp` itself, so this tool negates `total_angle_degrees` before passing it to `setTotalAngle`, mirroring `rotateBody`'s existing `-angle`. Codex independently re-derived this from `PatternOp.cpp`/`TransformOp.cpp` in round 2 and confirmed it.
2. **Spacing convention:** `PatternOp` divides the total angle evenly as `angle/count` (not `angle/(count-1)`), so a caller asking for a 180-degree total angle with 2 instances gets the second instance at 90 degrees, not 180. The tool description says this explicitly.
3. **Minimum count:** `PatternOp::execute()` itself rejects `count < 2` - the schema and validation require `count >= 2`, not `>= 1`.
4. **Count bounds:** added an explicit `std::isfinite` check and a hard cap (500) before the `double->int` cast.
5. **Test bug (round 2):** an earlier draft of this plan's handedness regression test compared a count=2/180-degree pattern (whose second instance sits at 90 degrees, per point 2 above) against a `rotate_body` call at 180 degrees - a mismatched comparison that a CORRECT implementation would fail. Fixed below: the reference rotation is now 90 degrees, and a second case with a nonzero origin is added to verify origin remapping too.

**Codex ruling on `construction_axis`, accepted:** `ConstructionAxisOp::execute()` silently falls back to World X when the two points passed to `two_points` are within `1e-9` of each other, and reports success. This tool rejects two points closer than `1e-6` apart before constructing the op, and rejects non-finite point coordinates. Both construction tools also reject a `name` argument that is present but not a string, rather than silently ignoring it.

- [ ] **Step 1: Add tool schemas**

```cpp
        ToolDef{"pattern_body", "Create a linear or radial array of copies of a body. For a radial pattern, instances are spaced evenly at total_angle_degrees / count, not across (count - 1) steps - a 180 degree total with 2 instances places the second at 90 degrees, not 180.",
            {num("body_id", "The id of the body to pattern."),
             str("type", "\"linear\" or \"radial\"."),
             num("count", "How many total instances, including the original. Must be a whole number from 2 to 500."),
             num("spacing_x", "Linear pattern: spacing along X per step, in mm. Ignored for radial.", false),
             num("spacing_y", "Linear pattern: spacing along user-space depth (Y) per step, in mm. Ignored for radial.", false),
             num("spacing_z", "Linear pattern: spacing along user-space height (Z, up) per step, in mm. Ignored for radial.", false),
             num("axis_x", "Radial pattern: X component of the rotation axis. Ignored for linear. Defaults to 0.", false),
             num("axis_y", "Radial pattern: user-space depth (Y) component of the rotation axis. Ignored for linear. Defaults to 0.", false),
             num("axis_z", "Radial pattern: user-space height (Z, up) component of the rotation axis. Ignored for linear. Defaults to 1 (straight up).", false),
             num("origin_x", "Radial pattern: X of the rotation center. Ignored for linear. Defaults to 0.", false),
             num("origin_y", "Radial pattern: user-space depth (Y) of the rotation center. Ignored for linear. Defaults to 0.", false),
             num("origin_z", "Radial pattern: user-space height (Z, up) of the rotation center. Ignored for linear. Defaults to 0.", false),
             num("total_angle_degrees", "Radial pattern: total angle spanned by the array, in degrees. Ignored for linear. Defaults to 360.", false)}},
        ToolDef{"construction_axis", "Create a construction (reference) axis, expressed in user-space (Z-up) coordinates.",
            {str("type", "\"x\", \"y\", \"z\" for a standard axis, or \"two_points\" for a custom axis through two points."),
             num("p1_x", "two_points only: X of the first point.", false),
             num("p1_y", "two_points only: user-space depth (Y) of the first point.", false),
             num("p1_z", "two_points only: user-space height (Z, up) of the first point.", false),
             num("p2_x", "two_points only: X of the second point.", false),
             num("p2_y", "two_points only: user-space depth (Y) of the second point.", false),
             num("p2_z", "two_points only: user-space height (Z, up) of the second point.", false),
             str("name", "Optional name for the new axis.", false)}},
        ToolDef{"construction_plane", "Create a construction (reference) plane on a standard plane, expressed in user-space (Z-up) coordinates, optionally offset.",
            {str("type", "\"xy\", \"xz\", or \"yz\" (user-space)."),
             num("offset", "Offset distance from the plane along its normal, in mm. Defaults to 0.", false),
             str("name", "Optional name for the new plane.", false)}},
```

- [ ] **Step 2: Update schema test's tool count**

In `tests/test_ai_tool_schema.cpp`, find the existing assertion of the tool count and update its expected value to `17` (read the actual current assertion - it may be an `EXPECT_EQ(allTools().size(), N)` or a size check on a returned vector; find and edit the real one, don't assume its exact current form). Add one test per new tool confirming its name is present with the right required/optional param split, following the pattern of the existing per-tool schema tests. Also add a test that every one of the original 9 tool names (`add_box`, `add_cylinder`, `add_sphere`, `add_cone`, `add_torus`, `move_body`, `rotate_body`, `scale_body`, `boolean_op`) is still present by exact name. Also add a schema-shape test for `mirror_body`/`pattern_body`/`construction_axis`/`construction_plane`'s param types matching what Task 1/2 defined.

- [ ] **Step 3: Implement dispatcher functions**

Read `src/app/Application_InteractiveOps.cpp:2157`'s `ConstructionAxisOp` call site and its surrounding comment IN FULL before writing the axis-type mapping below - it documents the exact user-Z-up-to-WorldY/Z remap this codebase uses for axis creation, and that mapping must be copied exactly, not re-derived:

```cpp
ToolResult patternBody(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    int bodyId;
    if (!requireBodyId(ctx.document(), args, "body_id", bodyId, err)) return {false, err};
    if (!args.contains("type") || !args["type"].is_string()) {
        return {false, "type is required and must be a string"};
    }
    std::string type = args["type"].get<std::string>();
    double countRaw;
    if (!requirePositive(args, "count", countRaw, err)) return {false, err};
    if (!std::isfinite(countRaw) || countRaw != std::floor(countRaw)) {
        return {false, "count must be a finite whole number"};
    }
    if (countRaw < 2.0 || countRaw > 500.0) {
        return {false, "count must be between 2 and 500"};
    }
    int count = static_cast<int>(countRaw);

    auto op = std::make_unique<PatternOp>();
    op->setBody(bodyId);
    op->setCount(count);
    PatternOp* rawPattern = op.get();

    // PatternOp multiplies spacing/angle by instance index internally
    // (count up to 500) - a per-step value near the double range limit
    // would overflow partway through the array. Cap per-step inputs so
    // count * value stays comfortably finite (1e15 leaves enormous
    // headroom below overflow even at count=500 while accepting every
    // realistic model dimension).
    constexpr double kMaxPerStepMagnitude = 1e15;

    if (type == "linear") {
        double sx = 0, sy = 0, sz = 0;
        if (!optionalFiniteNumber(args, "spacing_x", sx, 0.0, err)) return {false, err};
        if (!optionalFiniteNumber(args, "spacing_y", sy, 0.0, err)) return {false, err};
        if (!optionalFiniteNumber(args, "spacing_z", sz, 0.0, err)) return {false, err};
        if (std::fabs(sx) > kMaxPerStepMagnitude || std::fabs(sy) > kMaxPerStepMagnitude ||
            std::fabs(sz) > kMaxPerStepMagnitude) {
            return {false, "spacing values are too large"};
        }
        op->setType(PatternType::Linear);
        op->setLinearSpacing(sx, sz, sy);
    } else if (type == "radial") {
        double ax = 0, ay = 0, az = 1, ox = 0, oy = 0, oz = 0, angle = 360;
        if (!optionalFiniteNumber(args, "axis_x", ax, 0.0, err)) return {false, err};
        if (!optionalFiniteNumber(args, "axis_y", ay, 0.0, err)) return {false, err};
        if (!optionalFiniteNumber(args, "axis_z", az, 1.0, err)) return {false, err};
        if (!optionalFiniteNumber(args, "origin_x", ox, 0.0, err)) return {false, err};
        if (!optionalFiniteNumber(args, "origin_y", oy, 0.0, err)) return {false, err};
        if (!optionalFiniteNumber(args, "origin_z", oz, 0.0, err)) return {false, err};
        if (!optionalFiniteNumber(args, "total_angle_degrees", angle, 360.0, err)) return {false, err};
        double axisLen = std::sqrt(ax * ax + ay * ay + az * az);
        if (!std::isfinite(axisLen) || axisLen < 1e-9) {
            return {false, "the radial axis must not be the zero vector or too large to represent"};
        }
        if (std::fabs(ox) > kMaxPerStepMagnitude || std::fabs(oy) > kMaxPerStepMagnitude ||
            std::fabs(oz) > kMaxPerStepMagnitude || std::fabs(angle) > kMaxPerStepMagnitude) {
            // PatternOp.cpp converts (angle / count) to radians per instance -
            // a finite but huge angle (e.g. 1.7e308) overflows that
            // multiplication even though it passed the plain isfinite check.
            return {false, "origin or angle values are too large"};
        }
        op->setType(PatternType::Radial);
        op->setRadialAxis(ax, az, ay);
        op->setRadialOrigin(ox, oz, oy);
        // Negate, matching rotateBody's existing -angle: the user-to-world
        // coordinate swap has determinant -1, reversing rotation handedness.
        op->setTotalAngle(-angle);
    } else {
        return {false, "type must be \"linear\" or \"radial\""};
    }

    if (!ctx.history().pushOperation(std::move(op), ctx.document())) {
        return {false, "the operation failed to execute"};
    }
    ctx.markMeshesDirty();
    std::string newIds;
    for (int id : rawPattern->getCreatedBodyIds()) newIds += std::to_string(id) + " ";
    return {true, "Created a " + type + " pattern of body " + std::to_string(bodyId) +
                  " with " + std::to_string(count) + " instances. New body ids: " + newIds};
}

ToolResult constructionAxis(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    if (!args.contains("type") || !args["type"].is_string()) {
        return {false, "type is required and must be a string"};
    }
    std::string type = args["type"].get<std::string>();
    auto op = std::make_unique<ConstructionAxisOp>();
    if (type == "x") {
        op->setType(AxisCreationType::WorldX);
    } else if (type == "y") {
        // User-space Y (depth) maps to world Z - confirmed against
        // Application_InteractiveOps.cpp:2157.
        op->setType(AxisCreationType::WorldZ);
    } else if (type == "z") {
        // User-space Z (up) maps to world Y - confirmed against
        // Application_InteractiveOps.cpp:2157.
        op->setType(AxisCreationType::WorldY);
    } else if (type == "two_points") {
        double p1x, p1y, p1z, p2x, p2y, p2z;
        if (!requireFiniteNumber(args, "p1_x", p1x, err)) return {false, err};
        if (!requireFiniteNumber(args, "p1_y", p1y, err)) return {false, err};
        if (!requireFiniteNumber(args, "p1_z", p1z, err)) return {false, err};
        if (!requireFiniteNumber(args, "p2_x", p2x, err)) return {false, err};
        if (!requireFiniteNumber(args, "p2_y", p2y, err)) return {false, err};
        if (!requireFiniteNumber(args, "p2_z", p2z, err)) return {false, err};
        double dist = std::sqrt((p2x - p1x) * (p2x - p1x) + (p2y - p1y) * (p2y - p1y) +
                                 (p2z - p1z) * (p2z - p1z));
        if (!std::isfinite(dist) || dist < 1e-6) {
            // ConstructionAxisOp itself silently falls back to World X for
            // near-coincident points (< 1e-9) - reject explicitly instead of
            // letting the model get an axis it never asked for.
            return {false, "p1 and p2 must not be the same point"};
        }
        op->setType(AxisCreationType::TwoPoints);
        op->setPoints(gp_Pnt(p1x, p1z, p1y), gp_Pnt(p2x, p2z, p2y));
    } else {
        return {false, "type must be \"x\", \"y\", \"z\", or \"two_points\""};
    }
    if (args.contains("name")) {
        if (!args["name"].is_string()) return {false, "name must be a string"};
        op->setName(args["name"].get<std::string>());
    }
    ConstructionAxisOp* raw = op.get();
    if (!ctx.history().pushOperation(std::move(op), ctx.document())) {
        return {false, "the operation failed to execute"};
    }
    ctx.markMeshesDirty();
    return {true, "Created construction axis " + std::to_string(raw->getCreatedAxisId()) + "."};
}

ToolResult constructionPlane(PluginContext& ctx, const nlohmann::json& args) {
    std::string err;
    if (!args.contains("type") || !args["type"].is_string()) {
        return {false, "type is required and must be a string"};
    }
    std::string type = args["type"].get<std::string>();
    PlaneCreationType pt;
    if (type == "xy") pt = PlaneCreationType::XY;
    else if (type == "xz") pt = PlaneCreationType::XZ;
    else if (type == "yz") pt = PlaneCreationType::YZ;
    else return {false, "type must be \"xy\", \"xz\", or \"yz\""};
    double offset = 0;
    if (!optionalFiniteNumber(args, "offset", offset, 0.0, err)) return {false, err};
    auto op = std::make_unique<ConstructionPlaneOp>();
    op->setType(pt);
    op->setOffset(offset);
    if (args.contains("name")) {
        if (!args["name"].is_string()) return {false, "name must be a string"};
        op->setName(args["name"].get<std::string>());
    }
    if (!ctx.history().pushOperation(std::move(op), ctx.document())) {
        return {false, "the operation failed to execute"};
    }
    ctx.markMeshesDirty();
    return {true, "Created a " + type + " construction plane."};
}
```

`ConstructionPlaneOp`'s `PlaneCreationType::XY/XZ/YZ` already interpret their planes in user-space terms internally (confirmed by Codex against the header/call-sites) - do NOT apply the world remap to `construction_plane`'s type selection, only `construction_axis`'s world-axis and two-point cases need it.

- [ ] **Step 4: Wire dispatch cases**

```cpp
    if (toolName == "pattern_body") return patternBody(ctx, args);
    if (toolName == "construction_axis") return constructionAxis(ctx, args);
    if (toolName == "construction_plane") return constructionPlane(ctx, args);
```

- [ ] **Step 5: Write tests**

```cpp
TEST_F(AiToolDispatcherTest, PatternBodyLinearCreatesTheRightCount) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "linear"}, {"count", 3}, {"spacing_x", 10.0}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 3u);
}

TEST_F(AiToolDispatcherTest, PatternBodyRadialCreatesTheRightCount) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "radial"}, {"count", 4}, {"total_angle_degrees", 360.0}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 4u);
}

TEST_F(AiToolDispatcherTest, PatternBodyRadialMatchesRotateBodysHandedness) {
    // count=2, total_angle_degrees=180 places the second instance at
    // angle/count = 90 degrees (see the spacing-convention ruling above) -
    // NOT 180. Compare against a single rotate_body call of 90 degrees on an
    // identical box at an off-axis position (x=10, away from the Z axis) so
    // a wrong handedness or a wrong spacing convention both produce a
    // detectable mismatch.
    int a = addTestBox();
    executeTool(ctx(), "move_body", {{"body_id", a}, {"dx", 10.0}, {"dy", 0.0}, {"dz", 0.0}});
    nlohmann::json patternArgs = {{"body_id", a}, {"type", "radial"}, {"count", 2},
                                   {"total_angle_degrees", 180.0},
                                   {"axis_x", 0.0}, {"axis_y", 0.0}, {"axis_z", 1.0},
                                   {"origin_x", 0.0}, {"origin_y", 0.0}, {"origin_z", 0.0}};
    ToolResult patternResult = executeTool(ctx(), "pattern_body", patternArgs);
    EXPECT_TRUE(patternResult.ok);
    ASSERT_EQ(document().getAllBodyIds().size(), 2u);
    int b = addTestBox();
    executeTool(ctx(), "move_body", {{"body_id", b}, {"dx", 10.0}, {"dy", 0.0}, {"dz", 0.0}});
    nlohmann::json rotateArgs = {{"body_id", b}, {"angle_degrees", 90.0}, {"axis_x", 0.0},
                                  {"axis_y", 0.0}, {"axis_z", 1.0}};
    ToolResult rotateResult = executeTool(ctx(), "rotate_body", rotateArgs);
    EXPECT_TRUE(rotateResult.ok);
    // Find the pattern's newly-created body id (the one that isn't `a`) using
    // whatever the fixture's existing id-diffing pattern is, then compare its
    // bounding box against b's post-rotate bounding box - they must match
    // (within tolerance) if both handedness and spacing agree.
}

TEST_F(AiToolDispatcherTest, PatternBodyRadialWithNonzeroOriginRotatesAboutThatPoint) {
    // count=2, total_angle_degrees=180 -> the second instance sits at 90
    // degrees (angle/count, per the spacing convention above) about
    // origin=(5, 3, 0), rotating about the up axis. Origin has unequal
    // nonzero x/y so a dropped or mis-signed origin term is detectable.
    //
    // add_box places the box's CORNER at the given origin, not its center
    // (PrimitiveOp.cpp) - do NOT assume a fixed box position and hand-compute
    // the expected result from box dimensions. Instead: measure the box's
    // actual bbox CENTER before the pattern call, apply the standard
    // rotate-about-a-point formula to that measured center, and assert the
    // new instance's bbox center matches the computed value. For a standard
    // +90-degree rotation about (ox, oy) in the user X/(user-depth Y) plane:
    // x' = ox - (y - oy), y' = oy + (x - ox), z' = z (unchanged, axis is up).
    int a = addTestBox();
    executeTool(ctx(), "move_body", {{"body_id", a}, {"dx", 15.0}, {"dy", 0.0}, {"dz", 0.0}});
    Bnd_Box beforeBox = boundingBoxForBody(a);
    double bx0, by0, bz0, bx1, by1, bz1;
    beforeBox.Get(bx0, by0, bz0, bx1, by1, bz1);
    // NOTE: this bbox is in WORLD coordinates (world Y = user Z/up, world Z =
    // user Y/depth) - convert to user-space before applying the rotation
    // formula, matching this codebase's established remap, then convert the
    // rotated result back to world space to compare against the post-pattern
    // bbox. Read the align_body test above for the same before/after bbox
    // pattern applied to a simpler (translation-only) case first.
    double cxUser = (bx0 + bx1) / 2.0;
    double cyUser = (bz0 + bz1) / 2.0; // world Z == user depth (Y)
    double czUser = (by0 + by1) / 2.0; // world Y == user height (Z)
    double ox = 5.0, oy = 3.0;
    double expectedXUser = ox - (cyUser - oy);
    double expectedYUser = oy + (cxUser - ox);

    nlohmann::json patternArgs = {{"body_id", a}, {"type", "radial"}, {"count", 2},
                                   {"total_angle_degrees", 180.0},
                                   {"axis_x", 0.0}, {"axis_y", 0.0}, {"axis_z", 1.0},
                                   {"origin_x", ox}, {"origin_y", oy}, {"origin_z", 0.0}};
    ToolResult result = executeTool(ctx(), "pattern_body", patternArgs);
    EXPECT_TRUE(result.ok);
    ASSERT_EQ(document().getAllBodyIds().size(), 2u);
    // Find the new body's id (the one that isn't `a`), get its bbox, convert
    // its center back to user-space the same way as above, and assert it is
    // near (expectedXUser, expectedYUser, czUser) - not near the pre-pattern
    // position (origin ignored) or a mirrored/wrong-signed variant.
}

TEST_F(AiToolDispatcherTest, PatternBodyRejectsOversizedLinearSpacing) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "linear"}, {"count", 3},
                            {"spacing_x", 1e300}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 1u);
}

TEST_F(AiToolDispatcherTest, PatternBodyRejectsOversizedRadialOrigin) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "radial"}, {"count", 3},
                            {"origin_x", 1e300}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 1u);
}

TEST_F(AiToolDispatcherTest, PatternBodyRejectsOversizedRadialAngle) {
    // A finite angle can still overflow PatternOp's internal degrees-to-
    // radians-per-instance conversion even though it passes a plain
    // std::isfinite check - this must be caught by the magnitude cap, not
    // just the finiteness check.
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "radial"}, {"count", 3},
                            {"total_angle_degrees", 1.7e308}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(document().getAllBodyIds().size(), 1u);
}

TEST_F(AiToolDispatcherTest, PatternBodyRejectsAnInvalidType) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "spiral"}, {"count", 3}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, PatternBodyRejectsANonIntegerCount) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "linear"}, {"count", 2.5}, {"spacing_x", 10.0}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, PatternBodyRejectsACountBelowTwo) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "linear"}, {"count", 1}, {"spacing_x", 10.0}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, PatternBodyRejectsACountAboveFiveHundred) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "linear"}, {"count", 501}, {"spacing_x", 10.0}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, PatternBodyRejectsAZeroRadialAxis) {
    int bodyId = addTestBox();
    nlohmann::json args = {{"body_id", bodyId}, {"type", "radial"}, {"count", 4},
                            {"axis_x", 0.0}, {"axis_y", 0.0}, {"axis_z", 0.0}};
    ToolResult result = executeTool(ctx(), "pattern_body", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, ConstructionAxisWorldXSucceeds) {
    ToolResult result = executeTool(ctx(), "construction_axis", {{"type", "x"}});
    EXPECT_TRUE(result.ok);
}

TEST_F(AiToolDispatcherTest, ConstructionAxisTwoPointsSucceeds) {
    nlohmann::json args = {{"type", "two_points"},
        {"p1_x", 0.0}, {"p1_y", 0.0}, {"p1_z", 0.0},
        {"p2_x", 10.0}, {"p2_y", 0.0}, {"p2_z", 0.0}};
    ToolResult result = executeTool(ctx(), "construction_axis", args);
    EXPECT_TRUE(result.ok);
}

TEST_F(AiToolDispatcherTest, ConstructionAxisRejectsCoincidentPoints) {
    nlohmann::json args = {{"type", "two_points"},
        {"p1_x", 5.0}, {"p1_y", 5.0}, {"p1_z", 5.0},
        {"p2_x", 5.0}, {"p2_y", 5.0}, {"p2_z", 5.0}};
    ToolResult result = executeTool(ctx(), "construction_axis", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, ConstructionAxisRejectsAnInvalidType) {
    ToolResult result = executeTool(ctx(), "construction_axis", {{"type", "diagonal"}});
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, ConstructionAxisRejectsANonStringName) {
    nlohmann::json args = {{"type", "x"}, {"name", 42}};
    ToolResult result = executeTool(ctx(), "construction_axis", args);
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, ConstructionPlaneXySucceeds) {
    ToolResult result = executeTool(ctx(), "construction_plane", {{"type", "xy"}, {"offset", 5.0}});
    EXPECT_TRUE(result.ok);
}

TEST_F(AiToolDispatcherTest, ConstructionPlaneRejectsAnInvalidType) {
    ToolResult result = executeTool(ctx(), "construction_plane", {{"type", "diagonal"}});
    EXPECT_FALSE(result.ok);
}

TEST_F(AiToolDispatcherTest, ConstructionPlaneRejectsANonStringName) {
    nlohmann::json args = {{"type", "xy"}, {"name", 42}};
    ToolResult result = executeTool(ctx(), "construction_plane", args);
    EXPECT_FALSE(result.ok);
}
```

Every off-origin box placement above is composed from the existing `add_box` (via `addTestBox()`) plus a `move_body` tool call - both already exist and are already tested - rather than inventing new fixture infrastructure.

- [ ] **Step 6: Build the whole project, then run the full AI test suite**

Run: `cmake --build build_new -j`
Run: `ctest --test-dir build_new -R "test_ai_" --output-on-failure --no-tests=error`
Expected: all 6 AI test binaries pass, including all pre-existing tests plus every test added across Tasks 1-3.

- [ ] **Step 7: Run the full project test suite**

Run: `ctest --test-dir build_new --output-on-failure --no-tests=error`
Expected: all tests pass (no regression in unrelated suites).

- [ ] **Step 8: Run style gates**

Run: `python3 tools/no_em_dashes.py && python3 tools/units_audit.py`
Expected: both pass; if `units_audit.py` flags new literal mm/degree strings in the new tool descriptions, add narrowly-scoped `LITERAL_ALLOW` entries following the exact pattern used for the original 9 tools (see `docs/units-audit-allow.txt`), never a blanket suppression.

- [ ] **Step 9: Commit**

```bash
git add src/ai/AiToolSchema.cpp src/ai/AiToolDispatcher.cpp tests/test_ai_tool_schema.cpp tests/test_ai_tool_dispatcher.cpp docs/units-audit.md docs/units-audit-allow.txt
git commit -m "ai: add pattern_body, construction_axis, construction_plane tools; 17 tools total"
```

## Follow-up (out of scope for this plan)

File two bug tickets for the pre-existing `Operation` defects found during this plan's review, so `split_body`/`sew_bodies` AI tools can be added once fixed:
1. `SplitBodyOp` silently discards solids beyond the first two when a cutting plane produces 3+ pieces.
2. `SewOp` can retain only one shell (deleting the rest as "consumed") even when the input bodies are genuinely disjoint and should all survive.
