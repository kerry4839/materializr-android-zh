# Plan Review Log: Move Face async preview

Started 2026-09-11 (session start). MAX_ROUNDS=5. Model: gpt-6-astra (user
chose to try codex-review anyway despite CLAUDE.md noting this model is not
review-feature-compatible).

Plan under review: `docs/superpowers/plans/2026-09-10-move-face-async-preview.md`

## Round 1 - Codex

thread_id: 01a08ea0-8356-74e2-b9fd-be34925f5a6f

The gpt-6-astra model DID work for this review despite the compatibility
note (contrary to expectation) - a substantive, source-verified critique
came back, not an error.

1. **Blocking: no worker ever launches.** `PreviewDispatch` defaults to
   synchronous and `shouldLaunch()` refuses until `inlinePreviewTook()`
   enables async; the plan never called it, and `reset()` re-disables it.
   Fix: call `inlinePreviewTook(kAsyncPreviewMs)` before checking
   `shouldLaunch()`.
2. **High: an in-flight general preview overwrites Local mode.** Tick Local
   mid-gesture before a launched job lands; `moveFaceLocal` isn't in the
   key, so the stale general result would overwrite the local rebuild on
   landing.
3. **High: restoring the snapshot breaks dispatch deduplication.** The
   unconditional restore-to-snapshot at the top of `updateMoveFace` erases
   an already-landed preview on the next call, and returning to a
   previously-applied value would never relaunch (dispatch dedup) nor
   restore it (nothing else would).
4. **High: cancel/commit leave the app rendering forever.** `pollPreview`
   returning before `take()` whenever the gesture is inactive means
   `AsyncJob::running()` (and `previewPending()` → `hasActiveWork()`) never
   goes false for a job still in flight past commit/cancel.
5. **Medium: polling silently reintroduces rebuilds during dragging.**
   Auto-relaunching on a stale key from inside `pollPreview` reads
   mid-drag `m_st` state that `updateMoveFace()` was never called for
   (the viewport drag defers that call to release), bypassing grid-snap
   and sketch-follow.
6. **Blocking: the specified build changes are insufficient.** The app
   target has its own explicit source list (confirmed at
   `CMakeLists.txt:205`) and does not link `materializr_core` - adding
   `MoveFacePreview.cpp` only to the latter leaves the shipped app
   unlinkable. Also: `materializr_core` doesn't compile
   `FaceOpControllers.cpp`, so Task 2's test can't link either.
7. **High: test gaps.** Missing `#include "modeling/MoveFaceOp.h"`;
   volume-only assertions can't distinguish original/stale/correct on a
   volume-preserving Translate; the superseding test never asserts a
   worker launched; the 50ms threshold is machine-dependent; no coverage
   of begin/cancel/commit interacting with an in-flight job or the Local
   mid-flight case.

VERDICT: REVISE

### Claude's response

All 7 findings accepted and fixed - every one held up against the actual
source (verified the `CMakeLists.txt:205` claim directly before accepting
finding 6). Changes, by finding:

1. `launchMoveFacePreviewIfWanted` now calls
   `m_mfDispatch.inlinePreviewTook(PreviewDispatch<MoveFaceKey>::kAsyncPreviewMs)`
   on every call (idempotent once already async).
2. `pollPreview` now discards (not applies) a landed result whenever
   `localTweakApplies()` is true, rather than growing `MoveFaceKey` with
   `moveFaceLocal` - simpler and covers hole-move by the same mechanism.
3. Deleted the unconditional restore from `updateMoveFace`'s general path
   entirely; restore + `m_mfDispatch.retracted()` now happen only in the
   explicit `!faceXformNontrivial()` branch, mirroring
   `PushPullController`'s zero-distance handling.
4. `pollPreview` now calls `reap()`/`take()` unconditionally before any
   active/mode check; added explicit `m_mfJob.abandon(); m_mfDispatch.reset();`
   to `commitMoveFace` and `cancelMoveFace` (previously only in
   `beginMoveFace`).
5. Removed the "stale key → relaunch" branch from `pollPreview` entirely -
   a stale result is now just discarded; only the four real trigger sites
   (which all go through `updateMoveFace`) ever launch.
6. Added `src/app/MoveFacePreview.cpp` to the root `CMakeLists.txt` app
   target's source list (Task 1) and `src/app/FaceOpControllers.cpp` to
   `materializr_core` (Task 2).
7. Added the missing include; added a `topFaceCentroidX` helper and used it
   alongside volume in both existing tests; replaced the fixed 50ms bound
   with a same-run-measured relative one (`directMs * 0.2`); added
   `previewPending()` assertions around both `updateMoveFace` calls in the
   superseding test; added two new tests (`CommitAbandonsAnInFlightJob...`,
   `SwitchingToLocalMidFlightIsNotOverwritten...`).

Nothing was rejected this round - every finding reproduced against the
actual source before being accepted.

## Round 2 - Codex

Confirmed the async-enabling fix and explicit commit/cancel cleanup as
correct; found new problems introduced by round 1's own fixes plus one
build gap round 1 missed:

1. **High: newer submitted requests are lost.** Round 1's fix for finding 5
   (no auto-relaunch from `pollPreview`) combined with `PreviewDispatch::
   shouldLaunch()`'s busy-gate (unchanged from round 1) to silently drop a
   second request submitted while the first job is still running - nothing
   launches it and nothing retries it.
2. **High: zero/Local transitions wedge dispatch or retain a stale applied
   key.** `abandon()` without a matching `finished()` leaves `m_running`
   stuck true in `PreviewDispatch`, so `shouldLaunch` never allows a
   relaunch after a zero/Local detour; separately "general A landed →
   Local → general A" leaves A marked `m_applied` even though the document
   now holds Local's geometry.
3. **High: removing the snapshot restore breaks Local rebuilding.**
   `applyLocalTweak()`/`tweak::moveFace()` resolves `m_st.moveFaceFace`
   against whatever `ctx.doc` currently holds and throws `FaceNotFound` if
   a landed general-path result left the body without that face live -
   round 1's fix removed the restore that used to guarantee this.
4. **Build integration incomplete.** `FaceOpControllers.cpp` also needs
   `CylindricalPick.cpp` (`detectCylindricalPick()` at `:906`), not just
   itself, in `materializr_core`'s source list.
5. **Test gaps.** Local test accepts either success or failure from
   `applyLocalTweak`; no cancel test; no A→zero→A test; no coverage of
   explicit rotation/nonuniform-scale/Twist; timing threshold still
   scheduler-sensitive in spirit.
6. **Global Constraints self-contradiction.** Forbids touching
   `commitMoveFace()`/`cancelMoveFace()` while Step 7 requires editing
   both.

VERDICT: REVISE

### Claude's response

All 6 findings accepted (verified `CylindricalPick.cpp`'s absence from
`materializr_core` and its presence in the app's list directly before
accepting finding 4). Root-caused findings 1 and 2 to the same source:
`PreviewDispatch::shouldLaunch()`'s busy-gate exists for Push/Pull's
per-frame `update()` (wait for the current job before deciding whether to
relaunch, so it doesn't spawn 60 threads/second) - Move Face's four trigger
sites are discrete events with no such flood to guard against, so the gate
only drops legitimate work here. Fixed by dropping `shouldLaunch()` entirely
(a deliberate, documented deviation from the Push/Pull precedent, called
out in the plan's Architecture section) and always launching immediately;
`AsyncJob::launch()`'s own abandon-and-replace makes this safe. This also
resolves finding 2 as a side effect: with no busy-gate and no "already
applied" dedup, there is nothing left to wedge.

Finding 3: restored `m_st.moveFacePreviousShape` unconditionally as the
first line inside the `localTweakApplies()` branch specifically (not
reintroducing it at the top of the function, which round 1 correctly
identified as wrong for the async branch).

Finding 4: added `CylindricalPick.cpp` alongside `FaceOpControllers.cpp`.

Finding 5: fixed the Local test to set a correct pivot and assert
`moveFaceLocalRefusal == nullptr`; added a cancel test mirroring the
existing commit test; added an A→zero→A test. Declined to add per-kind
(Rotate/Scale/Twist) coverage at the controller level - Task 1's
`MoveFacePreviewJob` tests already cover all four kinds, and the
controller/dispatch logic under test here doesn't branch on which kind is
active, so duplicating that coverage at this layer would test the same
thing twice without proportionate value. Logged as a deliberate scope
boundary, not silently dropped.

Finding 6: narrowed the Global Constraint to "don't change modeling
behavior" and named the two-line async-bookkeeping exception explicitly.

## Round 3 - Codex

Asked specifically to stress-test round 2's "drop shouldLaunch(), always
launch immediately" deviation for races/correctness issues the busy-gate
was implicitly also protecting against. It found one, real and material:

1. **High: "abandon" does not cancel computation - unbounded concurrent
   rebuilds.** `AsyncJob::launch()` starts another thread while `abandon()`
   only stops tracking the old one; it keeps running to completion,
   holding its own scratch Document, consuming CPU. Several rapid events
   during one still-running ~15s computation would each spawn another
   concurrent rebuild with no cap. The busy-gate was providing backpressure,
   not just deduplication.
2. **Medium: claimed transform coverage doesn't exist.** Task 1 tests only
   exercised `setRotation()` (not `setRotationExplicit()`, what
   `configureFaceOp` actually calls) and uniform `setScaleFactor()`;
   `setTwist()` was never called. Separately: worker-only tests can't
   verify `currentMoveFaceKey()` tracks controller configuration correctly
   - the exact risk that motivated a dedicated key in the first place.
3. **Medium: tests don't stress the replacement policy or finish lifecycle
   checks.** No assertion of a bound on concurrent workers; the Local test
   never switched back to general; the A→zero→A test didn't exercise
   abandonment during the transition; commit/cancel tests didn't verify
   document/history integrity after completion.

VERDICT: REVISE

### Claude's response

Finding 1 accepted in full - this was a real, serious correctness gap in
round 2's own fix that I introduced while fixing round 1's finding 1.
Redesigned to bound the system to at most ONE worker thread in flight at
any time (checked via `AsyncJob::running()` directly before ever calling
`launch()`) plus a single `m_mfPendingRelaunch` flag remembering "a real
trigger arrived while busy" - set only inside `launchMoveFacePreviewIfWanted`
(reached exclusively from `updateMoveFace`'s real trigger sites), never
inferred from `pollPreview` reading ambient `m_st`. This preserves round 1's
finding-5 fix (no reading of uncommitted mid-drag state) while also fixing
the resource-pileup problem: two genuinely different mechanisms doing two
genuinely different jobs, not a reintroduction of the busy-gate.

Finding 2 accepted in full: added the missing kind coverage to Task 1
(`ExplicitRotationNonUniformScaleAndTwistAlsoMatchADirectExecute`), and -
the more important half - added a Task 2 test
(`ARealExplicitRotationGestureLandsTheCorrectGeometry`) that drives a real
gesture through `MoveFaceController`'s actual `configureFaceOp()`/
`currentMoveFaceKey()` rather than hand-built lambdas, verified against a
reference built from the controller's own `faceRotTotal()`.

Finding 3: accepted the boundedness point as now satisfied BY CONSTRUCTION
by finding 1's fix (a second concurrent launch is structurally impossible,
not just discouraged), so declined to add a redundant private-state-poking
assertion. Declined "controllable worker barriers" for deterministic test
synchronization - no other async-preview test in this codebase uses such a
seam (bounded-deadline polling is the established pattern throughout;
`test_pushpull_preview.cpp` doesn't use one either), and adding a
production-code test seam isn't justified by what's actually under test
here (key-matching and landing logic, not scheduler fairness). Logged as a
scope boundary rather than silently dropped.

**The "boundedness by construction" claim above was wrong - round 4 found
the actual bug.**

## Round 4 - Codex

Asked directly to re-verify against source, not the plan's claims. Found
the round-3 fix was itself broken, in three connected ways:

1. **High: polling still launches unsubmitted drag state.** The pending
   flag records only THAT a request arrived, not WHAT it was. By the time
   it drains, `launchMoveFacePreviewIfWanted` re-reads live `m_st`, which
   the deferred viewport drag can have moved on from since (round 1
   finding 5, reintroduced).
2. **High: the one-worker bound still fails across abandonment.** `A →
   zero/Local → submit B`: those branches call `abandon()`, which makes
   `running()` false immediately while A's thread keeps computing; B
   launches concurrently with it. Repeatable without bound.
3. **High: zero/Local can leave `previewPending()` true forever.**
   `abandon()`-ed jobs never surface via `take()`, so `if (!result)
   return` (guarding the pending-flag check) fires on every subsequent
   poll - the flag, and `previewPending()`, can get stuck true
   indefinitely even after the abandoned thread actually finishes.
4. **Medium: the rotation test doesn't test key discrimination.** It
   submits one unchanged rotation and waits; a constant `MoveFaceKey`
   would pass identically.

VERDICT: REVISE

### Claude's response

All 4 findings accepted - root-caused 1-3 to two separate, connected
mistakes in round 3's fix: (a) storing only a bare flag rather than the
actual configuration, and (b) trusting `AsyncJob::running()` alone to mean
"nothing is computing," when `abandon()` can make it lie. Fixed by:
launches now gated on `running() || abandonedCount() > 0` (after `reap()`)
so a second launch is refused while ANY thread - tracked or merely parked -
is still genuinely active; the retry mechanism now stores a fully prepared
`MoveFacePreviewJob` (its configuration frozen from live `m_st` at the
moment of the real trigger call that queued it, per finding 1) instead of a
flag; and the pending-job check in `pollPreview` now runs unconditionally,
never behind `if (!result) return` (finding 3). `previewPending()` now also
reports true while `abandonedCount() > 0`, matching
`InteractiveOpController::pollPreview`'s own documented contract that
abandoned jobs are polled regardless of gesture state. Added
`MoveFaceKey::bodyId` as a cross-gesture safety net. Fixed the commit/cancel
tests' timing expectations to match (previewPending() now correctly takes
as long as the computation does, not ~200ms) and added document-integrity
assertions to both. Added a key-discrimination step (two different angles
submitted before either lands) to the real-rotation controller test per
finding 4.

## Round 5 - Codex

Asked directly whether the round-4 design (bounded launches, prepared
pending job) is actually correct, and to stress-test two specific
questions: does the pending job's captured scratch-copy ever go stale if
begin/commit/cancel reset `m_st` before it drains, and can the pending and
running jobs ever unsafely share resources. Answer on both: no - "The
prepared-job ownership looks sound... I found no unsafe resource sharing
between the pending and running jobs." The core design held. Three
narrower issues remained:

1. **Medium: an immediate launch can leave an older pending request
   queued.** Abandoned A finishing, B pending; a THIRD event C arrives
   after A is reaped elsewhere and launches immediately without clearing
   B - B fires later anyway, wasting a rebuild and delaying anything after
   it.
2. **Medium: abandoned-only work unnecessarily forces continuous
   rendering.** The app already polls every controller at an idle floor
   regardless of `previewPending()`, per `PushPullController`'s own
   comment - `previewPending()` counting `abandonedCount()` (round 4's
   fix) forces full-rate rendering for the remaining duration of a job
   nobody wants any more, for no benefit.
3. **Medium: the key-discrimination test still passes with a constant
   key.** It checks only the final landed state; stale A landing first
   then correct B landing second and overwriting it is indistinguishable
   from correct behaviour if you only look at the end.

VERDICT: REVISE

### Claude's response

All 3 accepted. Finding 1: one-line fix, `m_mfPendingJob.reset()` right
before an immediate launch (launching a new request makes any older queued
one obsolete by definition). Finding 2: reverted `previewPending()` to
`running() || pendingJob`, matching `PushPullController`'s own documented
precedent exactly - also reverted the commit/cancel tests' timing back
toward "goes false promptly" while KEEPING round 4's document-integrity
assertions (which poll for a fixed duration independent of
`previewPending()`, so they still prove the abandoned result never lands
regardless of what `previewPending()` reports). Finding 3: rewrote the
discrimination test to assert INSIDE the polling loop, every iteration
after B is submitted, that A's result never appears - not just at the end.
While building that fix, discovered a real flaw in the test's OWN chosen
metric: `topFaceCentroidX` cannot distinguish two Rotate angles at all
(rotation pivots about the face's own centroid, which doesn't move under
rotation about itself) - added `topFaceZSpread` (tilt-sensitive) as the
correct metric for Rotate, keeping `topFaceCentroidX` for Translate where
it's valid. This was caught by trying to actually implement Codex's
suggested fix correctly, not by Codex itself - worth noting since it means
the discrimination test would have been silently wrong even after
"fixing" it per the literal finding.

## Round 6 - Codex

Asked for a final, thorough pass and to say explicitly if nothing further
was found rather than searching for something to flag.

> I found no further material issues after rechecking the revised plan
> against the current source... Immediate launches discard older queued
> requests. Abandoned workers still block additional launches without
> forcing continuous rendering. Prepared jobs own separate scratch
> documents and frozen configurations; lifecycle cleanup discards pending
> work safely. The rotation test now detects intermediate stale adoption
> using a tilt-sensitive metric. The plan is sound enough to implement.
> This is approval of the design; the specified build, regression, and
> in-app checks still need to pass.

**VERDICT: APPROVED** (5 revision rounds, converged on round 6's review).

---

## Summary of what the argument improved

Every round found a real, source-verified bug in the PREVIOUS round's own
fix - this was not a one-shot design that sailed through:

- Round 1: async mode never actually activated; a stale general-path
  result could overwrite a Local rebuild; an unconditional snapshot
  restore would erase already-landed results; `pollPreview` could leave
  `previewPending()` stuck true forever after commit/cancel; polling could
  reintroduce rebuilds during an active drag; the build was missing from
  two of three required source lists; several test gaps.
- Round 2: the fix for round 1's async-mode bug (always launch
  immediately) silently broke Local-mode safety and the "return to a
  previous value" case in ways round 1 had already half-fixed; a
  synchronous Local rebuild broke because an earlier async result had
  moved the live body out from under it; a second missing build
  dependency (`CylindricalPick.cpp`); a self-contradictory Global
  Constraint.
- Round 3: round 2's "always launch immediately" was itself the actual
  bug - unbounded concurrent rebuilds under rapid input, since
  `AsyncJob::abandon()` doesn't stop a thread; claimed test coverage that
  didn't exist; a currentMoveFaceKey()/configureFaceOp desync risk no test
  could have caught.
- Round 4: round 3's "one worker plus a flag" fix didn't actually bound
  concurrency across abandon+relaunch cycles, could launch on stale
  mid-drag state, and had a structural bug that could leave
  `previewPending()` stuck forever.
- Round 5: round 4's fix was structurally sound (confirmed directly) but
  had three narrower gaps - a stale queued request surviving an immediate
  launch, an unnecessary continuous-rendering cost, and a discrimination
  test that couldn't actually detect the bug it was named for (plus a
  separately-discovered flaw in that test's own chosen metric).

The final design: at most one `MoveFacePreviewJob` worker thread ever
computing (checked via both `AsyncJob::running()` and
`AsyncJob::abandonedCount()`), a single fully-prepared pending job frozen
from live state at the moment of each real trigger call (never re-read
later), `PreviewDispatch<MoveFaceKey>` used only for key-matching (not its
busy-gate, which is wrong for Move Face's discrete-event shape),
`MoveFaceKey::bodyId` as a cross-gesture safety net, and `previewPending()`
matching `PushPullController`'s own established idle-floor-polling
precedent exactly.

---

## Act 3 - Build

### Round 1 - Codex build

Launched via `codex-build` (`codex exec --yolo`) with a prompt contract
derived from the approved spec, run by the user directly in their own
terminal (the harness's auto-mode classifier refused to let Claude launch
`codex exec --yolo` itself, foreground or background - the user ran it and
reported back). Codex's own `-o` report file (`/tmp/codex-build.txt`) came
back containing content from a completely unrelated project (a Zendesk/
crypto-core build in a different repo) - the SAME `/tmp` cross-session
contamination issue observed earlier with `codex-verdict.txt` during plan
review. Codex's self-reported proof is therefore **not used as evidence
anywhere in this verdict** - everything below is Claude's own independent
verification against the actual repository state.

### Claude's verdict

Read the full diff (`git diff` + every new file in full) against the
spec, task by task:

- `src/app/MoveFaceDispatch.h`, `src/app/MoveFacePreview.h/.cpp`: byte-for-
  byte match to the spec's code blocks.
- `src/app/FaceOpControllers.h/.cpp`: faithful transcription of the FINAL
  (round-5-corrected) design, including every inline `CORRECTION`/`round N
  finding` comment verbatim - `currentMoveFaceKey()`'s `bodyId` field,
  `launchMoveFacePreviewIfWanted`'s prepare-then-gate-on-running-or-
  abandonedCount ordering with the pending-job reset before an immediate
  launch, `pollPreview`'s unconditional (never behind `if (!result)
  return`) pending-drain with the `wanted`/`stillWanted` checks,
  `previewPending()` as `running() || pendingJob` (no `abandonedCount()`),
  the removed unconditional snapshot-restore with the targeted restore
  moved into the Local branch, and the three lifecycle reset lines in
  `beginMoveFace`/`commitMoveFace`/`cancelMoveFace`.
- `tests/test_move_face_preview.cpp` (5 cases) and `tests/
  test_move_face_async.cpp` (7 cases): faithful transcription, including
  the `topFaceZSpread` metric fix and the in-loop key-discrimination
  assertion from round 5.
- Build wiring: `MoveFacePreview.cpp` added to BOTH the root
  `CMakeLists.txt` app target and `tests/CMakeLists.txt`'s
  `materializr_core`; `FaceOpControllers.cpp` and `CylindricalPick.cpp`
  added to `materializr_core`. Confirmed by building `materializr`,
  `test_move_face_preview`, and `test_move_face_async` from scratch -
  all three link cleanly.
- `docs/changelog.md`: the exact entry from the spec, verbatim.
- Confirmed untouched (diff-checked directly): `src/app/AsyncJob.h`,
  `src/app/PreviewDispatch.h`, `src/modeling/MoveFaceOp.h/.cpp` - no
  modifications, matching the Global Constraints.
- No other controller in `FaceOpControllers.cpp` touched.

**Proof, run independently:**
- `./build/tests/test_move_face_preview`: 5/5 passed.
- `./build/tests/test_move_face_async`: 7/7 passed (the commit/cancel
  abandon tests each ran their full 5s poll window as designed - 5.5s and
  5.1s respectively).
- Full suite: `ctest --output-on-failure` (unsandboxed, excluding the 6
  known sandboxed-tmp-write suites) - **105/105 passed** (103 pre-existing
  + the 2 new binaries), zero regressions.

**Deviation from spec:** Task 3's manual in-app GUI verification was
explicitly skipped (Codex has no way to drive the app interactively) -
pre-approved in the build prompt. No other deviations found.

**Verdict: build matches the approved spec. No fix round needed.**
