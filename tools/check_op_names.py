#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Find UI text that reaches the screen without ever passing through tr().

Two shapes, both of which the existing five checkers structurally cannot see.

── 1. Operation::name() ────────────────────────────────────────────────────

    std::string name() const override { return "Boolean"; }

name() is a lookup KEY rather than prose - it feeds serializeParams
("label=..."), the undo tooltip, the persisted history and OperationFactory's
typeId matching - so it must stay English at the source, and translation belongs
at the display site. But until a display site calls tr() on it, the value that
reaches the screen is English, and nothing reports it:

  * check_tr_coverage.py enumerates tr() CALL SITES. There is no call to read.
  * check_tr_arguments.py looks for literals inside a format call's arguments.
    This is a return value in a header, not an argument anywhere.
  * check_ui_string_arrays.py looks for indexed arrays. These are 39 separate
    inline overrides, not arrays.
  * check_specifiers.py reads the catalogue and never opens a source file.
  * i18n_extract.py audits ImGui sinks for a literal in the FIRST argument
    position. op->name() arrives through timelineBox's last parameter.

This is the fifth ADJACENT shape: the other four each cover one segment of the
pipeline, and a string entering by a fifth path escapes all of them. Same lesson
as every other checker here - they are not overlapping, they are adjacent, and
each fix has to ask "who else walks this path?".

The 39 names found are not all missing from the catalogue - most already have
rows, because earlier batches wrapped the display sites and those wrappings are
what put the names in. This tool reports the ones with no row.

── 2. A const char* holding English, displayed later ───────────────────────

    const char* what = isFillet ? "FILLET" : "CHAMFER";
    ...
    ImGui::Text("%s", what);

The literal is nowhere near a tr() call and not in an argument list, so both
argument-aware checkers miss it. Splitting the decision from the display is a
reasonable way to write the code and a terrible way to localise it.

This shape is genuinely ambiguous, so it reports rather than forbids:

  * NOT reported when either branch is already a tr()/trf()/trFormat() call -
    that is the fix, and these are how the tool proves it is looking at
    something.
  * NOT reported when a branch has no letter in it: "?", "…", "%.3f", "s".
    Notation and markers are not words the user reads.
  * NOT reported when the whole conditional is one line AND the variable is used
    only once, immediately, in a printf-style call - that shape is close enough
    to the argument case that check_tr_arguments.py sees it anyway.
  * Reported otherwise, with the note that fprintf(stderr) destinations are
    diagnostics, not UI, and can be ignored by eye.

Also excluded: ##-prefixed ImGui ids, which must stay byte-identical.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from i18n_catalogue import CAT

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "src")

# ── shape 1 ────────────────────────────────────────────────────────────────
INLINE = re.compile(r'std::string\s+name\s*\(\s*\)\s*const\s*override\s*\{\s*'
                    r'return\s+"((?:[^"\\]|\\.)*)"\s*;\s*\}')
OUTLINE = re.compile(r'std::string\s+\w+::name\s*\(\s*\)\s*const\s*'
                     r'(?:override\s*)?\{\s*return\s+"((?:[^"\\]|\\.)*)"\s*;\s*\}')
CTOR = re.compile(r'(?:make_unique|new)\s*<\s*'
                  r'(?:Plane|Axis|Batch)TransformOp\s*>\s*\(\s*"([^"]*)"')
LABELLED_OPS = ("AxisTransformOp", "PlaneTransformOp", "BatchTransformOp")

# ── shape 2 ────────────────────────────────────────────────────────────────
# `const char* v = "a";` and `const char* v = cond ? "a" : "b";`
ASSIGN = re.compile(
    r'\b(?:const\s+)?char\s*\*\s*(\w+)\s*=\s*([^;]+);', re.S)
# Calls that put a string on screen.
SINK = re.compile(
    r'ImGui::(?:MenuItem|Text|TextUnformatted|Button|SmallButton|SetTooltip'
    r'|Begin|TextColored|TextDisabled|TextWrapped|Selectable|InputText)\s*\(')
TR_CALL = re.compile(r'(?<![A-Za-z0-9_])(?:materializr::)?'
                     r'(?:tr|trf|trFormat)\s*\(')
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')
WORD = re.compile(r'[A-Za-z]')
STDERR = re.compile(r'\bfprintf\s*\(\s*stderr')


_HEX = re.compile(r'\\x([0-9A-Fa-f]{2})')


def unescape(s):
    """Decode the escapes C++ would, so notation stops looking like words.

    \\xNN matters here and not by accident. The degree suffix in
    PropertiesPanel.cpp is written "\\xC2\\xB0" - no letter in what the user
    sees, but the literal SOURCE spells x, C, 2, B, 0, and WORD matches five of
    those. A tool that decides "is this a word?" by looking at the source rather
    than the decoded value counts every escaped symbol as prose, and then the
    only way to silence it is a whitelist entry - which is how a notation check
    ends up maintained by hand instead of by rule.

    Hex escapes are gathered into a byte string and decoded as UTF-8 together,
    not one codepoint at a time. "\\xE2\\x88\\x92" is U+2192 RIGHTWARDS ARROW
    written in UTF-8; mapping each byte to chr() separately yields three
    mojibake characters, which then misses the catalogue row for the real key and
    reports a translation that is already there.

    A malformed escape, or bytes that are not valid UTF-8, fall back to the
    Latin-1 reading. This is an audit tool: it must never raise, because a
    crash on one weird literal would hide every literal after it.
    """
    s = s.replace('\\"', '"').replace("\\'", "'")
    s = s.replace("\\n", "\n").replace("\\t", "\t").replace("\\\\", "\\")
    if "\\x" not in s:
        return s
    return _decode_hex_runs(s)


def _decode_hex_runs(s):
    """Replace every maximal \\xNN run with its UTF-8 text."""
    out, i, n = [], 0, len(s)
    while i < n:
        m = _HEX.match(s, i)
        if not m:
            out.append(s[i])
            i += 1
            continue
        run = bytearray()
        end = m.end()
        while True:                      # one maximal run of \xNN
            run.append(int(m.group(1), 16))
            nxt = _HEX.match(s, end)
            if not nxt:
                break
            m, end = nxt, nxt.end()
        i = end
        try:
            out.append(run.decode('utf-8'))
        except UnicodeDecodeError:
            out.append(run.decode('latin-1'))
    return ''.join(out)


ADJACENT_LITS = re.compile(r'^\s*"(?:[^"\\]|\\.)*"(?:\s*"(?:[^"\\]|\\.)*")*\s*$')


def joined_literal(arg):
    """The single C++ string an argument denotes, or None if it is not one.

    Adjacent string literals are ONE value: btn(t="foo " "bar ") looks up
    "foo bar ". Splitting it into fragments is how a scan invents a key per
    line of a wrapped sentence - thirty-odd phantom "missing translations" for
    text that is in fact translated as a whole, which is the worst possible
    report: confidently wrong, and verbose.
    """
    if not ADJACENT_LITS.match(arg):
        return None
    parts = re.findall(r'"((?:[^"\\]|\\.)*)"', arg)
    return unescape(''.join(parts))


# A locator, not prose. Translating any part of one breaks the link, and the
# "words" in it are hostnames and path segments that no language owns.
URL = re.compile(r'\w+://|^www\.', re.I)

# Locally-declared, not-in-the-catalogue findings that are correct as they are.
# Same convention and same reasoning as check_ui_string_arrays.py's NOT_UI: a
# gate that cannot go green teaches people to ignore it.
#
# Keyed by the literal ALONE, not by (file, literal). An earlier version keyed on
# the pair so that two different "Materializr" could carry different reasons -
# but it cannot, because the product name is the product name in every file, and
# a file-keyed table is untestable without planting the exact file and breaks
# silently on a rename (the entry stops matching, which is indistinguishable from
# a real finding). The reason string still says which sites it covers.
NOT_UI = {
    '"M A T E R I A L I Z R"':
        "splash wordmark (Application.cpp), letterspaced by hand; translating "
        "it changes the product name",
    '"Materializr"':
        "vendor name (Application_Dialogs.cpp), used as the export/import "
        "filename stem",
    '"PATH"':
        "environment variable NAME read by getenv (FileDialogs.cpp), not "
        "displayed text",
}
NOT_UI_SUBSTR = ("https://", "http://", "discord.gg")


def literals_of(expr):
    """Literals in expr that are NOT inside a tr()-family call."""
    out, i, n = [], 0, len(expr)
    while i < n:
        m = TR_CALL.match(expr, i)
        if m:
            depth, j = 1, m.end()
            while j < n and depth:
                if expr[j] == '"':
                    j += 1
                    while j < n and expr[j] != '"':
                        j += 2 if expr[j] == "\\" else 1
                    j += 1
                    continue
                if expr[j] == "(":
                    depth += 1
                elif expr[j] == ")":
                    depth -= 1
                j += 1
            i = j
            continue
        if expr[i] == '"':
            j = i + 1
            while j < n and expr[j] != '"':
                j += 2 if expr[j] == "\\" else 1
            out.append(expr[i:min(j + 1, n)])
            i = j + 1
            continue
        i += 1
    return out


def has_hidden_id(lits):
    """True when any branch is an ImGui "##id" / "###Visible##id" literal.

    Those must stay byte-identical: the part after ## is the widget ID, and
    translating it silently changes focus and the saved .ini layout.
    """
    return any("##" in unescape(l) for l in lits)


def is_not_ui_value(s):
    """True when a DECODED string value is a locator or a known non-UI string."""
    if any(sub in s for sub in NOT_UI_SUBSTR):
        return True
    return '"%s"' % s in NOT_UI


def is_not_ui(rel, lits):
    """True when every literal is a whitelisted non-UI string, or a locator."""
    return all(is_not_ui_value(unescape(l).strip('"')) for l in lits)


def report_shape2(text, rel):
    """const char* holding bare English literals, displayed through a sink."""
    assigns = []
    for m in ASSIGN.finditer(text):
        var, expr = m.group(1), m.group(2).strip()
        lits = literals_of(expr)
        # Keep only branches that carry a word; "…", "%.3f", "?" are notation.
        # unescape() first, so "\xC2\xB0" (degrees) is not read as the five
        # letters x C 2 B 0 that its source spelling happens to contain.
        lits = [l for l in lits if WORD.search(unescape(l))]
        if not lits or has_hidden_id(lits) or is_not_ui(rel, lits):
            continue
        assigns.append((var, lits, m.start(), m.end()))

    hits = []
    for var, lits, a0, a1 in assigns:
        # Where is the variable used, and is it inside a UI sink?
        uses = [m.start() for m in re.finditer(r'(?<![A-Za-z0-9_])%s\b' % var, text)]
        displayed = False
        for u in uses:
            if a0 <= u < a1:
                continue                       # the declaration itself
            # Walk back to the opening paren of the call the use sits in, then
            # read that call's name. The slice must INCLUDE the paren: SINK
            # matches up to and including "\(", so slicing up to op EXCLUSIVE
            # hands it the identifier without the paren and the search can never
            # succeed - which is exactly what made this tool report nothing.
            op = text.rfind("(", 0, u)
            if op < 0:
                continue
            head = text[max(0, op - 48):op + 1]
            if SINK.search(head):
                displayed = True
                break
        if not displayed:
            continue
        # A lone use, on one line, inside the sink call itself: close enough to
        # the argument case that check_tr_arguments.py already covers it.
        line_of = text.count("\n", 0, a0) + 1
        one_line = ("\n" not in text[a0:a1])
        hits.append((line_of, var, lits, one_line))
    return hits


def scan_names(text):
    """Operation::name() literals in one file, as {name: line_number}.

    Split out so the tool can be exercised on synthetic input: a checker whose
    only entry point walks the whole tree cannot be given a positive control
    without planting a file, which is why an earlier version of this file was
    able to report a clean sheet while actually matching nothing.
    """
    out = {}
    for rx in (INLINE, OUTLINE):
        for m in rx.finditer(text):
            out.setdefault(m.group(1), text.count("\n", 0, m.start()) + 1)
    return out


# ── shape 3 ────────────────────────────────────────────────────────────────
# A literal handed straight to a UI helper defined in the same file:
#
#     auto wideButton = [&](const char* label) -> bool { ... ImGui::Button(label) ... };
#     ...
#     wideButton("Delete");
#
# Shape 2 catches the same string once a variable holds it; this catches it when
# it never becomes one. Both were blind to the nav-overlay buttons until a
# device screenshot showed "Move: Off" rendering in English - which is how a
# checker that reports 0 gets disproved.
#
# The helpers are DISCOVERED, not listed. A hand-maintained name list would go
# stale the first time someone adds a lambda, and a stale list reports nothing
# while looking green - the exact failure this file already made once. A
# definition qualifies when it takes a const char* AND its body calls an ImGui
# sink, which is what "puts text on screen" means here.
LAMBDA_DEF = re.compile(
    r'auto\s+(\w+)\s*=\s*\[[^\]]*\]\s*\(([^)]*)\)\s*(?:->[^{]*)?\{')
FUNC_DEF = re.compile(
    r'^(?:static\s+)?(?:inline\s+)?(?:bool|void|float|int|ImVec2|ImVec4|const char\*)'
    r'\s+(\w+)\s*\(([^)]*)\)\s*(?:const\s*)?(?:->[^{;]*)?\{', re.M)
CHARP = re.compile(r'const\s+char\s*\*')


def brace_body(text, open_brace):
    """The {...} block starting at open_brace, or '' if unbalanced."""
    depth, i = 0, open_brace
    n = len(text)
    while i < n:
        ch = text[i]
        if ch == '"':
            i += 1
            while i < n and text[i] != '"':
                i += 2 if text[i] == "\\" else 1
            i += 1
            continue
        if ch == "'":
            i += 1
            while i < n and text[i] != "'":
                i += 2 if text[i] == "\\" else 1
            i += 1
            continue
        if ch == '/' and i + 1 < n and text[i + 1] == '/':
            i = text.find('\n', i)
            if i < 0:
                return ''
            continue
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                return text[open_brace:i + 1]
        i += 1
    return ''


UI_HELPER_PARAMS = re.compile(r'(?:\bconst\s+)?char\s*\*\s*(\w+)')
PARAM_SPLIT = re.compile(r',')
PARAM_DEF = re.compile(
    r'auto\s+(\w+)\s*=\s*\[[^\]]*\]\s*\(([^)]*)\)'
    r'|(?:^|\n)[ \t]*(?:static\s+)?(?:bool|void|float|int|ImVec2|ImVec4)'
    r'\s+(\w+)\s*\(([^)]*)\)\s*(?:const\s*)?(?:->[^{;]*)?\{', re.M)


def ui_helpers(text):
    """{helper: {param: role}} for helpers defined in this file that draw.

    The role is what decides whether a bare literal at a call site is a bug, and
    it can only be read out of the helper body. Three roles exist:

      key   - the helper passes the param to tr()/trf()/trFormat() itself, so a
              literal at the call site is the LOOKUP KEY. Correct as written.
              It is still worth reporting when the key is missing from the
              catalogue, because tr() returns English unchanged on a miss and
              the defect is invisible until someone reads the other language.
      shown - the helper hands the param straight to an ImGui sink. A bare
              literal is displayed verbatim in every language, so it is a
              defect whether or not a catalogue row exists. "Delete" with a
              row but no tr() is exactly this: green catalogue, English screen.
      meta  - the param is never drawn (an ImGui id, a comparison key). Must
              stay byte-identical; a literal there is correct.

    Guessing one role for the whole helper is what made the first version of
    this scan unusable: Toolbar's btn translates both of its strings internally,
    so flagging them as untranslated text was pure noise, while the same
    uniform rule would have had to whitelist btn by name - and a name list goes
    stale silently the moment the next helper appears.
    """
    out = {}
    for m in PARAM_DEF.finditer(text):
        name = m.group(1) or m.group(3)
        params = m.group(2) if m.group(1) else m.group(4)
        if params is None or not CHARP.search(params):
            continue
        brace = text.find('{', m.end() - 1)
        if brace < 0:
            continue
        body = brace_body(text, brace)
        if not body or not SINK.search(body):
            continue
        # Every parameter keeps its slot, including non-char* ones, so that
        # call-site arguments line up by POSITION. Dropping the gaps would shift
        # the mapping by one for any helper like btn(ToolAction a, const char*
        # label, const char* t) - and a shift that size does not look like a
        # bug, it looks like a finding about the wrong string.
        roles = []
        for p in PARAM_SPLIT.split(params):
            p = p.strip()
            if not p:
                continue
            ident = p.split('=')[0].strip().split()[-1].lstrip('&*')
            if CHARP.search(p) and re.fullmatch(r'\w+', ident):
                roles.append((ident, param_role(body, ident)))
            else:
                roles.append((None, None))
        out[name] = roles
    return out


def enclosing_call(body, at):
    """(kind, callee) for the call whose argument list contains offset `at`.

    The slice handed to the matcher MUST include the '(' - SINK and TR_CALL both
    end at "\\(", so stopping one byte short makes every probe fail and the
    whole report silently come out empty. That mistake is why this file had a
    checker which read as green while matching nothing.
    """
    op = body.rfind('(', 0, at)
    if op < 0:
        return None
    head = body[max(0, op - 48):op + 1]
    if TR_CALL.search(head):
        return 'tr'
    if SINK.search(head):
        return 'sink'
    return None


def param_role(body, param):
    """'key' | 'shown' | 'meta' for one helper parameter, read from its body.

    The initial value is 'meta' - no evidence of display, no evidence of
    translation - and only a real tr() or a real sink moves it. Starting at
    'sink' instead makes every param default to "displayed", which flags
    BeginPopup(popupId) as on-screen text: popupId is an identity, and its
    literal is supposed to stay exactly as written. A default must be the
    conclusion least likely to invent a finding, or the report fills up with
    parameter names and the real defects get lost in it.
    """
    role = 'meta'
    for m in re.finditer(r'(?<![A-Za-z0-9_])%s\b' % re.escape(param), body):
        kind = enclosing_call(body, m.start())
        if kind == 'tr':
            role = 'key'              # translating wins over raw
        elif kind == 'sink' and role != 'key':
            role = 'shown'
    return role


def split_args(expr):
    """Top-level comma-separated argument texts, skipping strings and parens."""
    out, cur, depth, i, n = [], [], 0, 0, len(expr)
    while i < n:
        ch = expr[i]
        if ch == '"':
            j = i + 1
            while j < n and expr[j] != '"':
                j += 2 if expr[j] == "\\" else 1
            cur.append(expr[i:min(j + 1, n)])
            i = j + 1
            continue
        if ch in '([':
            depth += 1
        elif ch in ')]':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(''.join(cur))
            cur = []
            i += 1
            continue
        cur.append(ch)
        i += 1
    if cur:
        out.append(''.join(cur))
    return [a.strip() for a in out]


def scan_helper_calls(text, rel):
    """Literals handed to a UI helper, as (line, helper, param, literal, role)."""
    helpers = ui_helpers(text)
    if not helpers:
        return []
    # Blank out the definitions themselves, then look at what is left.
    stripped = text
    for m in PARAM_DEF.finditer(text):
        if (m.group(1) or m.group(3)) not in helpers:
            continue
        brace = text.find('{', m.end() - 1)
        body = brace_body(text, brace)
        if body:
            s = text.index(body, brace)
            stripped = stripped[:s] + (' ' * len(body)) + stripped[s + len(body):]

    hits = []
    call = re.compile(r'(?<![A-Za-z0-9_.>])(%s)\s*\(' % '|'.join(
        re.escape(h) for h in sorted(helpers, key=len, reverse=True)))
    for m in call.finditer(stripped):
        name = m.group(1)
        op = stripped.index('(', m.end() - 1)
        close = matching_paren(stripped, op)
        if close < 0:
            continue
        positional = helpers[name]
        for idx, arg in enumerate(split_args(stripped[op + 1:close])):
            if idx >= len(positional):
                break              # extra args: skipped, not guessed at
            param, role = positional[idx]
            if role in (None, 'meta'):
                continue
            lits = literals_of(arg)
            whole = joined_literal(arg)
            if whole is not None:
                # One C++ string: judge the whole thing, not its fragments.
                if '##' in whole or not WORD.search(whole):
                    continue
                if is_not_ui_value(whole):
                    continue
                hits.append((stripped.count('\n', 0, m.start()) + 1, name,
                             param, whole, role))
                continue
            for lit in lits:
                s = unescape(lit).strip('"')
                if not WORD.search(s) or has_hidden_id([lit]):
                    continue
                if is_not_ui(rel, [lit]):
                    continue
                hits.append((stripped.count('\n', 0, m.start()) + 1, name,
                             param, s, role))
    return hits


def matching_paren(text, op):
    depth, i = 0, op
    n = len(text)
    while i < n:
        ch = text[i]
        if ch == '"':
            i += 1
            while i < n and text[i] != '"':
                i += 2 if text[i] == "\\" else 1
            i += 1
            continue
        if ch == "'":
            i += 1
            while i < n and text[i] != "'":
                i += 2 if text[i] == "\\" else 1
            i += 1
            continue
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def main():
    known = {en for en, _ in CAT}
    names, labels, cond = {}, {}, []
    helper_hits = []

    for dirpath, _dirs, files in os.walk(ROOT):
        for fn in sorted(files):
            if not fn.endswith((".cpp", ".h")):
                continue
            p = os.path.join(dirpath, fn)
            rel = os.path.relpath(p, ROOT)
            text = open(p, encoding="utf-8", errors="replace").read()

            for rx in (INLINE, OUTLINE):
                for m in rx.finditer(text):
                    if any(op in rel for op in LABELLED_OPS):
                        continue                 # name() is m_label; CTOR covers it
                    names.setdefault(m.group(1),
                                     (rel, text.count("\n", 0, m.start()) + 1))
            for m in CTOR.finditer(text):
                labels.setdefault(m.group(1),
                                  (rel, text.count("\n", 0, m.start()) + 1))
            for line, var, lits, _one in report_shape2(text, rel):
                cond.append((rel, line, var, lits, text))
            for line, helper, param, lit, role in scan_helper_calls(text, rel):
                helper_hits.append((rel, line, helper, param, lit, role))

    bad = 0
    print("── 1. Operation::name() literals with no catalogue row ──")
    for v in sorted(names):
        if v not in known:
            print("  %s:%d  %r" % (names[v] + (v,)))
            bad += 1

    print("\n── 2. transform-op labels with no catalogue row ──")
    for v in sorted(labels):
        if v not in known:
            print("  %s:%d  %r" % (labels[v] + (v,)))
            bad += 1

    print("\n── 3. const char* holding English, displayed ──")
    seen = set()
    for rel, line, var, lits, text in sorted(cond, key=lambda r: (r[0], r[1])):
        key = (rel, line, var)
        if key in seen:
            continue
        seen.add(key)
        note = ""
        m = STDERR.search(text[max(0, text.find(lits[0]) - 400):
                                text.find(lits[0]) + 400])
        if m:
            note = "   (near a fprintf(stderr) - diagnostics, not UI?)"
        print("  %s:%d  %s = %s%s" % (rel, line, var, ", ".join(lits), note))
        bad += 1

    print("\n── 4. literal handed to a local UI helper ──")
    print("   shown = 裸传即上屏（必坏） / key = helper 内部自己 tr()")
    print("          （漏词条则坏）")
    hseen = set()
    for rel, line, helper, param, lit, role in sorted(set(helper_hits)):
        key = (rel, line, helper, lit)
        if key in hseen:
            continue
        hseen.add(key)
        if role == 'shown':
            print("  %s:%d  %s(%s=%r)  ← 裸传上屏" % (rel, line, helper, param, lit))
            bad += 1
        elif lit not in known:
            print("  %s:%d  %s(%s=%r)  ← 目录缺词条，tr() 会原样返回"
                  % (rel, line, helper, param, lit))
            bad += 1

    print("\n%d op names, %d labels, %d conditional labels, %d helper calls; "
          "%d with no catalogue entry"
          % (len(names), len(labels), len(seen), len(hseen), bad))
    if not names and not labels and not seen and not hseen:
        print("WARNING: nothing matched at all - the patterns are probably "
              "stale, so this clean sheet means nothing was inspected")
        return 1
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())