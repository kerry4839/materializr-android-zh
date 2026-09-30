#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Find a translated FORMAT fed an untranslated ARGUMENT.

tr() only ever translates the string it is handed. For a format call - trf(),
trFormat(), or tr() with a printf wrapper at the sink - that means the frame
gets translated and the %s arguments do NOT. The result is a sentence that is
half-translated, which reads worse than either language alone:

    ImGui::Text(materializr::tr("Mode: %s"), mode == A ? "Extend" : "Pinch");
      -> "模式: Extend"

The existing checkers cannot see this. check_tr_coverage.py only asks whether the
KEY has a catalogue row - and "Mode: %s" has one, is complete in all six
languages, and passes the specifier check. i18n_extract.py only looks at literals
in the FIRST argument position, which is precisely the one that IS translated.
check_specifiers.py works on the catalogue and never reads a call site.

So the whole argument list is unexamined by anything. Two real instances were
found by hand on the device before this file existed:

    ProjectSketchOp::renderProperties()   tr("Mode: %s")  <- "Engrave"/"Emboss"
    ScaleFaceOp::renderProperties()       tr("Mode: %s")  <- "Extend"/"Pinch"

and the same pattern turned up again in three more operation descriptions.

The rule: in a format call, any argument that is a bare string literal is
suspicious. It is not automatically wrong - passing a literal straight through
a "%s" that the caller supplied is legitimate for data like a filename - so
this reports rather than forbids, and the fix is usually to wrap the argument in
tr() and add a catalogue row for it.

Deliberately NOT reported:
  * arguments that are already tr() / trf() / trFormat() results
  * numeric arguments, fmtLength()/fmtVec3() results, .c_str() of those
  * non-literal arguments (variables) - already handled by check_tr_calls.py
    for tr(), and not decidable here for the others
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from i18n_catalogue import CAT

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "src")

# A printf specifier of any width/precision/length decoration.
SPEC = re.compile(r'%(?!%)[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|z|t|j|L)?'
                  r'[diouxXeEfFgGaAcspn]')

# The printf-style ImGui sinks: first string argument is a FORMAT, not a label.
# TextColored takes the colour first, so its format is argument 2.
SINK_FORMAT_ARG = {
    "Text": 0, "TextColored": 1, "TextDisabled": 0, "TextWrapped": 0,
    "SetTooltip": 0, "SetItemTooltip": 0,
}
# TextUnformatted is deliberately ABSENT: its second parameter is text_end, not
# a printf argument list, so `TextUnformatted(a, b)` does not mean what this rule
# assumes.
# trf / trFormat are printf wrappers: argument 0 is the format, the rest are
# its arguments. Plain tr() is NOT a format call - its single argument is the
# whole string - so it only reaches this file as the FORMAT of a sink.
FMT_FORMAT_ARG = {"trf": 0, "trFormat": 0}
CALL = re.compile(r'(?<![A-Za-z0-9_])(ImGui::)?([A-Za-z_][A-Za-z0-9_]*)\s*\(')

LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')


def strip_comments(text):
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in '\"':
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


def skip_string(text, i):
    """Index just past the string literal starting at text[i].

    Every scanner below needs this, and they all need it to CONSUME the closing
    quote. An earlier version advanced two characters per iteration and left the
    closing quote in place; the caller then treated that quote as the start of a
    fresh string and swallowed everything up to the next literal, so a two-
    argument call parsed as one argument and the whole checker reported nothing.
    """
    q = text[i]
    n = len(text)
    i += 1
    while i < n:
        c = text[i]
        if c == "\\":
            i += 2
            continue
        if c == q:
            return i + 1
        i += 1
    return n


def split_args(argstr):
    """Split a call's argument text on top-level commas."""
    args, depth, cur, i, n = [], 0, [], 0, len(argstr)
    while i < n:
        c = argstr[i]
        if c in '"\'':
            j = skip_string(argstr, i)
            cur.append(argstr[i:j])
            i = j
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 0:
            args.append("".join(cur))
            cur = []
            i += 1
            continue
        cur.append(c)
        i += 1
    if "".join(cur).strip():
        args.append("".join(cur))
    return args


def matching_paren(text, open_idx):
    depth = 0
    i, n = open_idx, len(text)
    while i < n:
        c = text[i]
        if c in '"\'':
            i = skip_string(text, i)
            continue
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def calls(text):
    """Every CALL-named invocation, with its top-level argument list.

    CALL deliberately does not require the ImGui:: qualifier: materializr::tr
    and bare tr are both wanted, and the name alone tells us which.
    """
    for m in CALL.finditer(text):
        name = m.group(2)
        popen = m.end() - 1
        pclose = matching_paren(text, popen)
        if pclose < 0:
            continue
        yield name, m.start(), popen, split_args(text[popen + 1:pclose])


def format_literal(argtext):
    """The literal of a tr()/trf()/trFormat() wrapping argtext, or None.

    Returns the DECODED key only when the whole argument is exactly one such
    call with one string-literal argument - so `tr("a") + var` and `tr(x)`
    do not qualify, and neither does `tr("Visible###Id")` without a specifier.
    """
    a = argtext.strip()
    m = re.match(r'^(?:materializr::)?(tr|trf|trFormat)\s*\((.*)\)$', a, re.S)
    if not m:
        return None
    inner = split_args(m.group(2))
    if len(inner) != 1:
        return None
    lits = LIT.findall(inner[0])
    if len(lits) != 1 or not LIT.fullmatch(inner[0].strip()):
        return None
    return lits[0]


TR_OPEN = re.compile(r'(?<![A-Za-z0-9_])(?:materializr::)?(?:tr|trf|trFormat)\s*\(')

# A literal with no letter in it is punctuation or a marker, never a word the
# user is asked to read: "?", "...", "", "s", "%s". The plural suffix case is
# the important one - `"%d region%s", n, n == 1 ? "" : "s"` is a real defect,
# but it is a defect in the KEY's shape, not in the argument, and it is
# reported as an empty string either way.
WORD = re.compile(r'[A-Za-z]')

# Functions whose string arguments are needles, not displayed text. Comparing
# a label against a key is not shipping the key to the screen.
NEEDLE = re.compile(r'(?<![A-Za-z0-9_])(?:std::)?'
                    r'(?:strstr|strcmp|strncmp|strrchr|strchr|find|rfind|'
                    r'starts_with|ends_with|compare)\s*\(')


def bare_literals(argtext):
    """String literals in argtext that are NOT inside a tr()/trf()/trFormat().

    Not "the first literal" and not "the whole argument is a literal": the real
    shape is an expression whose literal branches were never wrapped, e.g.

        m == Mode::Extend ? "Extend" : "Pinch"

    so the argument text has to be walked. Literals inside a translation call
    are skipped whole - including their nested parentheses - because those are
    translated by definition; the same goes for a literal that is a search
    needle, since comparing a key is not displaying it.
    """
    out = []
    i, n = 0, len(argtext)
    while i < n:
        m = TR_OPEN.match(argtext, i)
        if m:
            j = matching_paren(argtext, m.end() - 1)
            if j < 0:
                break
            i = j + 1
            continue
        m = NEEDLE.match(argtext, i)
        if m:
            j = matching_paren(argtext, m.end() - 1)
            if j < 0:
                break
            i = j + 1
            continue
        if argtext[i] == '"':
            j = skip_string(argtext, i)
            lit = argtext[i:j]
            if WORD.search(lit):
                out.append(lit)
            i = j
            continue
        i += 1
    return out


def scan(path):
    raw = open(path, encoding="utf-8", errors="replace").read()
    text = strip_comments(raw)
    out = []
    for name, start, popen, args in calls(text):
        if name in FMT_FORMAT_ARG:
            fi = FMT_FORMAT_ARG[name]
            shown = name + "("
        elif name in SINK_FORMAT_ARG:
            fi = SINK_FORMAT_ARG[name]
            shown = ("ImGui::" if name in ("Text", "TextColored", "TextDisabled",
                                           "TextWrapped")
                     else "") + name + "("
        else:
            continue
        if fi >= len(args):
            continue
        # The format slot itself is exempt: it is the thing being translated.
        line = text.count("\n", 0, start) + 1
        for k, a in enumerate(args):
            if k == fi:
                continue
            lits = bare_literals(a)
            if lits:
                out.append((line, shown, k + 1, ", ".join(lits)))
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
            rel = os.path.relpath(p, ROOT)
            for line, op, argno, lit in scan(p):
                print("  %s:%d  %s arg %d = %s"
                      % (rel, line, op + "(", argno, lit))
                bad += 1
    print("\n%d catalogue keys; %d source files scanned; "
          "%d untranslated literal argument(s) to a translated format"
          % (len(CAT), files, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())