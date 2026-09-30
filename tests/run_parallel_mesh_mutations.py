#!/usr/bin/env python3
"""Apply each reviewed mutation to production code, prove failure, restore.

Only test_parallel_mesh is rebuilt. A finally block restores the source even
if a build or assertion check fails; the restored suite must pass each time.
"""
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
APP = ROOT / "src/app/Application.cpp"
POOL = ROOT / "src/viewport/ParallelMesh.cpp"

MUTATIONS = [
    ("a", APP,
     "            m_shapeRenderer->notePreMeshed(jobs[i].shape, deflection, angularDeflection);",
     "            // Mutation: omit the success stamp.",
     "SuccessfulLoadSkipsEveryUploadMeshCall", "successful pooled jobs must skip all upload mesh calls"),
    ("b", POOL, "if (!entry.second) return entry.first->second == job;",
     "if (!entry.second) return false;",
     "SplitSiblingsFallBackButOrdinaryBodiesPool", "ordinary independent bodies must pool"),
    ("c", POOL,
     "const size_t workers = std::min(jobs.size(), size_t(std::max(1u,\n"
     "        options.workerCount ? options.workerCount : std::thread::hardware_concurrency())));",
     "const size_t workers = 1;",
     "MeshCallsOverlap", "mesh-call rendezvous timed out"),
    ("d", POOL, "                            OCC_CATCH_SIGNALS",
     "                            static std::mutex meshMutex;\n"
     "                            std::lock_guard<std::mutex> serial(meshMutex);\n"
     "                            OCC_CATCH_SIGNALS",
     "MeshCallsOverlap", "mesh-call rendezvous timed out"),
    ("e", APP, "            m_shapeRenderer->forgetPreMeshed(jobs[i].shape);",
     "            // Mutation: leave stale tags behind.",
     "FailedLoadInvalidatesBothTagGenerations", "failed load must invalidate both tag generations"),
    ("f", POOL,
     "while (completedJobs.load() != jobs.size() && liveWorkers.load() != 0)",
     "while (completedJobs.load() != jobs.size())",
     "OuterFaultLoadReturnsBeforeDeadline", "outer-fault load must return before the 3-second deadline"),
]


def run(args, tail=None):
    print("$ " + " ".join(map(str, args)), flush=True)
    result = subprocess.run(args, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=300)
    lines = result.stdout.splitlines()
    print("\n".join(lines[-tail:] if tail else lines), flush=True)
    print(f"exit={result.returncode}", flush=True)
    return result


def force_recompile(path):
    # Make can compare timestamps at one-second precision. A fast restore
    # can otherwise leave the mutant object looking current.
    targets = ["test_parallel_mesh.dir"]
    if path == POOL:
        targets.append("materializr_core.dir")
    for target in targets:
        obj = BUILD / "tests/CMakeFiles" / target / "__" / path.relative_to(ROOT)
        obj.with_suffix(obj.suffix + ".o").unlink(missing_ok=True)


def build():
    result = run(["cmake", "--build", "build", "--target", "test_parallel_mesh", "-j10"], 15)
    if result.returncode:
        raise RuntimeError("test target failed to build")


def suite():
    result = run(["ctest", "--test-dir", "build", "-R", "^test_parallel_mesh$", "--output-on-failure"])
    if result.returncode:
        raise RuntimeError("restored suite failed")


def main():
    force_recompile(APP)
    force_recompile(POOL)
    build()
    suite()
    for name, path, before, after, test, assertion in MUTATIONS:
        original = path.read_text()
        if original.count(before) != 1:
            raise RuntimeError(f"mutation {name} requires exactly one source match")
        print(f"\nMUTATION ({name}): {test}", flush=True)
        try:
            path.write_text(original.replace(before, after))
            force_recompile(path)
            build()
            result = run([str(BUILD / "tests/test_parallel_mesh"),
                          f"--gtest_filter=ParallelMesh.{test}"])
            if result.returncode == 0 or assertion not in result.stdout:
                raise RuntimeError(f"mutation {name} did not fail its required assertion")
            if name == "f" and "signal 14" not in result.stdout:
                raise RuntimeError("mutation f must hit the subprocess alarm")
            print(f"mutation ({name}): expected assertion FAILED", flush=True)
        finally:
            path.write_text(original)
            force_recompile(path)
            build()
            suite()
            print(f"mutation ({name}): restored suite PASSED", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
