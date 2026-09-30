#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Audit user-visible strings that reach the UI through a VARIABLE.

The other three checkers each have a blind spot, and this file exists because
the blind spots are adjacent rather than overlapping:

  i18n_extract.py     sees only a sink call whose FIRST argument is a literal
                      run. `ImGui::Begin(titles[k])` is invisible.
  check_tr_coverage.py  sees only tr()/trf()/trFormat() call sites, so it can
                      only ask "does this key have a row", never "is there a
                      user-visible string with no key at all".
  check_specifiers.py works on the catalogue table and never reads the source.

So a `static const char* names[] = {"New Box", ...}` that is indexed and used
somewhere is reported by none of them, and ships in English forever. Two real
instances, both found only by looking at the device:

  Application_Dialogs.cpp  `const char* titles[]` -> ImGui::Begin(titles[k]),
                           so the primitive popup's title read "New Box".
  ImTouchLayout.cpp        `kNames[]` -> a std::string concat -> dl->AddText,
                           so the project chip read "New Project  -  Face (1)".
                           The second one is why the rule below is "indexed
                           ANYWHERE", not "indexed into a sink": a self-drawn
                           AddText is not an ImGui sink, and the checklist
                           cannot tell the two apart from the outside.

An array nothing indexes is dead data, so it is not reported. The heuristic is
deliberately narrow - only SHORT elements (a word or two, no sentence) are
flagged, so this stays a list of "labels someone forgot" rather than a list of
every English literal in the tree.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from i18n_catalogue import CAT
from i18n_srclit import decode

KEYS = {en for en, _ in CAT}
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "src")

LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')
# `static const char* names[] = {` ... `};` and the `const char* x[] = {` form.
ARRAY = re.compile(
    r'(?:static\s+)?const\s+char\s*\*\s*(?:\*\s*)?(\w+)\s*\[\s*\]\s*=\s*\{',
    re.S)
# Any subscript of the array, in any expression. Deliberately broad: the point
# is "does this data reach the UI at all", which the source cannot answer.
INDEXED = re.compile(r'\b(\w+)\s*\[')

# Short, prose-shaped element. Sentences are handled by the tr()-site checker
# and by i18n_extract; this rule is for the word-sized label tables.
SHORT = re.compile(r'^[A-Za-z][A-Za-z0-9 \'\-\(\)\./]{0,30}$')
PROSE = re.compile(r'[A-Za-z]{2,}')
SKIP = re.compile(r'^(?:[A-Z_][A-Z0-9_]*|__\w+|#\w+)$')

# Arrays whose elements are DATA, not labels. Each entry says why, because an
# unexplained exclusion list is how a checker starts being ignored.
#
#   android_platform fonts        FONT FILENAMES passed to SDL_TTF_RenderUTF8
#   Application_Dialogs
#     kFontFiles                  font filenames paired with kFontNames
#     providerNames               first element is the vendor's own name
#   BrepIO kKeywords              BREP file-format section names, matched
#                                 literally against what OCCT writes
#   FileDialogs progs             executable names, handed to execvp
#   SvgImport keep                SVG presentation attribute names, matched
#                                 literally against the file's contents
NOT_UI = {
    ("android_platform.cpp", "fonts"): "font filenames passed to SDL_ttf",
    ("app/Application_Dialogs.cpp", "kFontFiles"): "font filenames paired with kFontNames",
    ("app/Application_Dialogs.cpp", "providerNames"): "vendor name + driver description",
    ("io/BrepIO.cpp", "kKeywords"): "BREP file-format section names matched against OCCT output",
    ("io/FileDialogs.cpp", "progs"): "executable names handed to execvp",
    ("modeling/SvgImport.cpp", "keep"): "SVG attribute names matched against file contents",
}


def strip_comments(text):
    """Blank out // and /* */ comments, preserving offsets and newlines."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in '"\'':
            q = c
            i += 1
            while i < n and text[i] != q:
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
    indexed = {m.group(1) for m in INDEXED.finditer(text)}
    rel = os.path.relpath(path, ROOT)
    out = []
    for m in ARRAY.finditer(text):
        name = m.group(1)
        if name not in indexed:
            continue
        if (rel, name) in NOT_UI:
            continue
        # Collect the brace-balanced body.
        i = text.index("{", m.end() - 1)
        depth = 0
        for j in range(i, len(text)):
            if text[j] == "{":
                depth += 1
            elif text[j] == "}":
                depth -= 1
                if depth == 0:
                    break
        body = text[i + 1:j]
        base_line = text.count("\n", 0, m.start()) + 1
        for lit in LIT.findall(body):
            try:
                val = decode(lit)
            except Exception:
                continue
            vis = val.split("##", 1)[0].strip()
            if not vis or SKIP.match(vis) or not PROSE.search(vis):
                continue
            if not SHORT.match(vis):
                continue
            if vis in KEYS:
                continue
            out.append((base_line, name, vis))
    return out


def main():
    bad = 0
    files = 0
    for dirpath, _dirs, names in os.walk(ROOT):
        for n in sorted(names):
            if not n.endswith((".cpp", ".h")):
                continue
            p = os.path.join(dirpath, n)
            files += 1
            for line, arr, val in scan(p):
                print("  %s:%d  %s[] -> %r" % (os.path.relpath(p, ROOT), line,
                                               arr, val))
                bad += 1
    print("\n%d catalogue keys; %d source files scanned; "
          "%d indexed array element(s) with no catalogue entry "
          "(%d array(s) excluded as non-UI data)"
          % (len(CAT), files, bad, len(NOT_UI)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
