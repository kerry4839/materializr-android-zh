#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Check that every catalogue translation keeps its printf specifiers in order.

The catalogue's keys are printf format strings - "Sew %d surfaces (%d edge%s
still open)" is handed straight to snprintf. A translation that drops, adds or
reorders a specifier does not fail to build; it prints garbage, or reads a
double as a char* and crashes. Nothing else in the toolchain would notice, so
this is the only place it is caught.

Run from the repo root:  python3 tools/check_specifiers.py
"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                os.pardir, "tools"))

from i18n_catalogue import CAT, LANGS

# %[arg$][flags][width][.prec]conv  - "%%" and a trailing lone % are literals.
SPEC = re.compile(r"%(?:%|[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|z|t|j|L)?[diouxXeEfFgGaAcspn])")


def specs(s):
    """The conversion characters, in order, skipping %% and any positional
    argument index (which is a rendering nicety, not a reordering)."""
    out = []
    for m in SPEC.finditer(s):
        tok = m.group(0)
        if tok == "%%":
            continue
        out.append(tok[-1])
    return out


def main():
    bad = 0
    for en, tr in CAT:
        want = specs(en)
        if not want:
            continue
        for lang, text in tr.items():
            got = specs(text)
            if got != want:
                bad += 1
                print(f"  [{lang}] {en!r}")
                print(f"        want {want}  got {got}")
                print(f"        -> {text!r}")
    total = sum(1 for en, _ in CAT if specs(en))
    print(f"\n{len(CAT)} keys, {total} of them format strings, "
          f"{bad} specifier mismatch(es)")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
