#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fail if any of the five languages upstream owns has changed.

    python3 tools/check_catalogue_regressions.py

This branch adds a language. It does not get to revise the other five: those
rows are upstream's translation work, they ship in every Materializr install
already, and a fork that quietly rewrites them makes the next upstream merge a
diff nobody can read.

Every other check in tools/ looks at this repository on its own terms - do the
keys line up, do the specifiers match, is anything untranslated. None of them
can see a cell that changed from one plausible translation to another, which is
why this one exists: it compares against a record of what upstream v1.7.1
actually shipped, one hash per key, in tools/upstream_fingerprints_1.7.1.txt.

Exits 1 and lists the keys on any mismatch. Nothing to regenerate, nothing to
keep in sync except the fingerprint file, which tools/make_upstream_fingerprints.py
rebuilds from an upstream checkout.
"""
import ast
import hashlib
import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)

FINGERPRINTS = os.path.join(HERE, "upstream_fingerprints_1.7.1.txt")
LANGS = ("es", "pt", "fr", "de", "it")

# Upstream keys this branch deliberately dropped, with the reason. An upstream
# key the source no longer contains is a legitimate removal; an upstream key
# that vanishes silently is data loss, so the list of exceptions is written out
# here rather than inferred.
#
# One entry: the string carried a broken conversion in its English ("Under 100%
# tapers it in"), so tr() reached it with a stray "% t" specifier and every
# language had to reproduce the typo to pass check_specifiers.py. The English
# was corrected to "100%% shrinks it", which makes it a different key; the row
# for the old key is gone on purpose and the corrected row is in the catalogue.
SUPERSEDED = {
    "Scale this face; the side walls re-slope to follow. Under 100% tapers it "
    "in, over 100% flares it out. Shorten the length to blend near the face "
    "instead of from the base.",
}


def fingerprint(row):
    blob = "\x1f".join(row.get(lang, "") for lang in LANGS)
    return hashlib.sha256(blob.encode("utf-8", "surrogateescape")).hexdigest()[:16]


def load_fingerprints():
    if not os.path.exists(FINGERPRINTS):
        sys.exit("missing %s - run tools/make_upstream_fingerprints.py against an "
                 "upstream checkout" % FINGERPRINTS)
    want = {}
    for n, line in enumerate(io.open(FINGERPRINTS, encoding="utf-8"), 1):
        if line.startswith("#") or not line.strip():
            continue
        digest, _, key = line.strip().partition(" ")
        try:
            want[ast.literal_eval(key)] = digest
        except (SyntaxError, ValueError) as exc:
            sys.exit("%s:%d: key is not a Python literal (%s) - the file is "
                     "generated, do not hand-edit it" % (FINGERPRINTS, n, exc))
    return want


def main():
    want = load_fingerprints()
    from i18n_catalogue import CAT
    have = dict(CAT)

    changed, missing = [], []
    for key, digest in want.items():
        if key not in have:
            if key in SUPERSEDED:
                continue
            missing.append(key)
            continue
        got = fingerprint(have[key])
        if got != digest:
            changed.append((key, digest, got))

    extra = [key for key in SUPERSEDED if key in have]

    for key, digest, got in changed:
        print("changed: %r\n  upstream %s\n  this     %s" % (key, digest, got))
    for key in missing:
        print("missing: %r" % key)
    for key in extra:
        print("note: %r is listed as superseded but is still present" % key)

    if changed or missing or extra:
        print("FAIL: %d changed, %d missing of %d upstream key(s)"
              % (len(changed), len(missing), len(want)))
        return 1
    print("all %d upstream key(s) unchanged across %s"
          % (len(want), "/".join(LANGS)))
    return 0


if __name__ == "__main__":
    sys.exit(main())