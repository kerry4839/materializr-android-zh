#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Report tr() call sites whose key has no entry in the catalogue.

tools/i18n_extract.py answers a DIFFERENT question. It scans ImGui SINK calls
(Button("Foo"), TextUnformatted("Foo"), ...) looking for literals that were
never wrapped in tr() at all. Once a call site reads tr("Foo"), that scan can
no longer see the string, so it reports "0 untranslated" whether or not "Foo"
actually has a translation.

That gap is load-bearing here: a catalogue can be 100% complete as a TABLE and
still leave English on screen, because some call site asks for a key nobody
ever added. The English string IS the key, so a missing catalogue entry and a
missing tr() are indistinguishable at runtime - both render the English. This
tool is the only thing that tells them apart.

It also catches the adjacent-literal trap: C++ joins "a" "b" into a single
argument, so the catalogue key is the JOINED text, not the first fragment.

Run from the repo root:  python3 tools/check_tr_coverage.py
Exit status 0 when every statically-auditable tr() site is covered.
"""
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                os.pardir, "tools"))

from i18n_catalogue import CAT
from i18n_srclit import decode
from zh_translations import get_zh_translation

KEYS = {en for en, _ in CAT}
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "src")

# tr( / trf( / trFormat( - trf is the format-string alias of tr.
CALL = re.compile(r'\b(?:tr|trf|trFormat)\s*\(')
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')
# A string literal plus any trailing comment and whitespace, repeated.
WS = re.compile(r'\s*(?://[^\n]*\n\s*)?')


def leading_key(text, start):
    """From `start` (just past the open paren) return the joined literal run.

    Returns None when the first argument is not a literal - a variable, a
    function call, a ternary. Those cannot be audited statically; the runtime
    missingReport() is what covers them.
    """
    pos = start
    parts = []
    while True:
        m = WS.match(text, pos)
        if not m:
            return None
        pos = m.end()
        lit = LIT.match(text, pos)
        if not lit:
            # Not a literal, OR literals already collected and this call takes
            # real arguments (trFormat) - either way there is no key to check.
            return "".join(parts) if parts else None
        parts.append(lit.group(1))
        pos = lit.end()
        # A literal immediately followed by another literal (no comma) means
        # adjacent concatenation: keep going.
        nxt = WS.match(text, pos)
        if not nxt or not LIT.match(text, nxt.end()):
            return "".join(parts)


def strip_comments(text):
    """Blank out // line comments, preserving every offset and newline.

    A tr() mentioned in a comment is prose about tr(), not a call site - and
    i18n.h documents the API with examples like trf("%d faces"), which would
    otherwise be reported as missing forever. String literals are left intact:
    a // inside "http://..." is data, not a comment. Block comments are blanked
    too, but their delimiters are kept so offsets cannot shift.
    """
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            i += 1
            while i < n and text[i] != '"':
                i += 2 if text[i] == "\\" else 1
            i += 1
            continue
        if c == "'":
            i += 1
            while i < n and text[i] != "'":
                i += 2 if text[i] == "\\" else 1
            i += 1
            continue
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
            continue
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
            continue
        i += 1
    return "".join(out)


def scan(path):
    raw = open(path, encoding="utf-8", errors="replace").read()
    text = strip_comments(raw)
    out = []
    for m in CALL.finditer(text):
        raw_key = leading_key(text, m.end())
        if not raw_key:
            continue
        try:
            key = decode(raw_key)
        except Exception:
            continue
        # tr() splits "Visible##id" itself, so the catalogue holds the visible
        # half only.
        visible = key.split("##", 1)[0]
        if not visible or visible in KEYS:
            continue
        if get_zh_translation(visible):
            continue
        line = text.count("\n", 0, m.start()) + 1
        out.append((line, key))
    return out


def main():
    bad = 0
    files = 0
    for dirpath, _dirs, names in os.walk(ROOT):
        for name in sorted(names):
            if not name.endswith((".cpp", ".h")):
                continue
            path = os.path.join(dirpath, name)
            files += 1
            for line, key in scan(path):
                rel = os.path.relpath(path, os.path.join(ROOT, os.pardir))
                print(f"  {rel}:{line}  {key!r}")
                bad += 1
    print(f"\n{len(CAT)} catalogue keys; {files} source files scanned; "
          f"{bad} tr() site(s) with no catalogue entry")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
