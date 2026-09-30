#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Find materializr::tr() calls whose argument is not a string literal.

tr() is declared as tr(const char*) - it is a LOOKUP, not a printf wrapper. So
every call must be exactly:

    tr("literal")             or    tr("literal"
                                      "continued")   tr(var)  tr(expr)

Anything else - tr("fmt %d", n), tr("a" + b) with the paren in the wrong place -
is either a compile error or, worse, a translation silently applied to half a
sentence. Bulk-wrapping hundreds of call sites is exactly where that happens,
and the C++ compiler only sees the file it happens to be compiling.

Scans across line breaks, so multi-line concatenations are handled.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')
# tr() only. trFormat() IS variadic (it is a printf wrapper), so the
# "one literal, nothing after it" rule does not apply to it - the compiler
# catches a malformed trFormat, whereas a malformed tr() can compile and
# silently translate half a sentence.
CALL = re.compile(r'(?<![A-Za-z0-9_])tr\(')


def scan(text, path):
    bad = []
    for m in CALL.finditer(text):
        i = m.end()          # just past the '('
        depth = 1
        lits = 0
        j = i
        closed_at = None
        while j < len(text) and j < m.end() + 4000:
            c = text[j]
            if c == '"':
                mm = LIT.match(text, j)
                if not mm:
                    break
                lits += 1
                j = mm.end()
                continue
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    closed_at = j
                    break
            j += 1
        if closed_at is None:
            bad.append((path, text.count("\n", 0, m.start()) + 1,
                        "no closing paren", snippet(text, m.start())))
            continue

        arg = text[i:closed_at]
        first = arg.lstrip()
        # tr(var) / tr(expr) is legal - the catalogue just won't have it.
        if not first.startswith('"'):
            continue
        # Every literal in the argument must be followed only by whitespace,
        # more literals, and the closing paren. A bare ',' or operator here
        # means someone wrapped a printf format string in tr() by mistake.
        rest = first
        consumed = 0
        while True:
            mm = LIT.match(rest, consumed)
            if not mm:
                break
            consumed = mm.end()
            k = consumed
            while k < len(rest) and rest[k] in " \t\r\n":
                k += 1
            if k >= len(rest):
                break
            if rest[k] == '"':
                consumed = k
                continue
            bad.append((path, text.count("\n", 0, m.start()) + 1,
                        "trailing %r after literal" % rest[k:k + 12],
                        snippet(text, m.start())))
            break
    return bad


def snippet(text, at):
    line = at
    while line > 0 and text[line - 1] != "\n":
        line -= 1
    end = text.find("\n", at)
    if end < 0:
        end = len(text)
    return text[line:end].strip()


def main():
    bad = []
    for p in sorted(ROOT.glob("src/**/*.cpp")) + sorted(ROOT.glob("src/**/*.h")):
        bad += scan(p.read_text(errors="replace"), p.relative_to(ROOT))
    for path, line, why, txt in bad:
        print(f"{path}:{line}  {why}")
        print(f"    {txt[:150]}")
    print(f"\n{len(bad)} 处可疑的 tr() 调用")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
