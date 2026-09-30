#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fingerprints of the five languages upstream owns, at upstream v1.7.1.

    python3 tools/make_upstream_fingerprints.py /path/to/upstream/checkout

This localization adds a language; it must not touch the five that were already
there. That is easy to honour by hand and easy to break by accident, because
tools/catalogue_entries_*.py carry a six-language row per key and a row written
for a key upstream already has looks exactly like a row upstream does not.

One batch learned that the hard way. Its proposals for keys like "Rotate" and
"On Line" were *improvements* to the existing es/pt/fr/de/it text, but the
catalogue ingester refreshes on content, not on presence, so they silently
overwrote 54 upstream cells. Nothing broke visibly, and no existing gate
noticed: the translations were still non-English, so no check had anything to
say.

So the ownership boundary gets recorded as data. For every key upstream v1.7.1
ships, this file stores a hash over that key's five non-zh columns. Then:

  * tools/add_catalogue_entries.py refuses to REFRESH those rows - upstream
    wrote them, first writer keeps them, and a proposal for an upstream key is
    a comment rather than an edit;
  * tools/check_catalogue_regressions.py fails if any of those cells ever moves.

Fingerprints, not the text: 1192 rows of five languages is a vendored copy of
someone else's translation work, which is exactly the thing this file exists to
keep out of the tree. A hash says "unchanged" without saying it twice.

The key is written with repr() on purpose. Several keys contain runs of spaces
and one contains a quote; splitting on whitespace and rejoining would silently
rewrite them, and a fingerprint list whose keys do not round-trip is worse than
none.
"""
import hashlib
import importlib.util
import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
LANGS = ("es", "pt", "fr", "de", "it")


def fingerprint(row):
    """Stable short hash over one key's five non-zh columns."""
    blob = "\x1f".join(row.get(lang, "") for lang in LANGS)
    return hashlib.sha256(blob.encode("utf-8", "surrogateescape")).hexdigest()[:16]


def load_upstream(path):
    """CAT from an upstream checkout's tools/i18n_catalogue.py."""
    spec = importlib.util.spec_from_file_location(
        "_upstream_catalogue", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.CAT


def render(cat, note):
    out = [line.rstrip("\n") for line in io.open(__file__, encoding="utf-8")
           if line.startswith("#") and not line.startswith("#!")]
    out[-1] = "#"
    out.append("# " + note)
    out.append("#")
    out.append("# columns: %s" % ", ".join(LANGS))
    for en, row in cat:
        out.append("%s %r" % (fingerprint(row), en))
    return "\n".join(out) + "\n"


def main():
    if len(sys.argv) != 2:
        print(__doc__.strip())
        return 2
    cat = load_upstream(os.path.join(sys.argv[1], "tools", "i18n_catalogue.py"))
    dest = os.path.join(HERE, "upstream_fingerprints_1.7.1.txt")
    note = ("%d keys from upstream v1.7.1 (git 9893275), "
            "regenerated with tools/make_upstream_fingerprints.py" % len(cat))
    with io.open(dest, "w", encoding="utf-8") as f:
        f.write(render(cat, note))
    print("wrote %s (%d keys)" % (dest, len(cat)))
    return 0


if __name__ == "__main__":
    sys.exit(main())