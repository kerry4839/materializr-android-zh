#!/usr/bin/env python3
"""Wrap user-visible string literals in tr().

Handles ADJACENT STRING CONCATENATION. C++ joins

    ImGui::TextWrapped("Classic: docked panels. "
                       "Modern: a tool rail.");

into ONE argument, so the catalogue key is the joined text and the whole run has
to be replaced together. An earlier version matched only the first fragment,
which is why every multi-line Settings description and half the tooltips stayed
English however many catalogue entries were added.

Conservative by design: a run is only touched when its joined visible half is an
exact catalogue key. Untranslated strings are left alone.

It does NOT touch data-structure initialisers -- ModernLayout / ImTouchLayout
strcmp tool labels against English, so the STORED label must stay English and
only the draw call may translate.
"""
import re, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from i18n_catalogue import CAT
from i18n_srclit import decode

KEYS = {en for en, _ in CAT}
LIT = re.compile(r'"((?:[^"\\]|\\.)*)"')

CALLS = ("Text|TextUnformatted|TextDisabled|TextWrapped|Button|SmallButton|"
         "MenuItem|BeginMenu|Checkbox|RadioButton|CollapsingHeader|BeginTabItem|"
         "SeparatorText|BulletText|Selectable|SetTooltip|SetItemTooltip|TreeNode|"
         "SliderFloat|SliderInt|DragFloat|DragInt|InputText|InputFloat|InputInt|"
         "Combo|BeginCombo|ProgressBar|TableSetupColumn")
RUN = r'((?:"(?:[^"\\]|\\.)*"\s*)+)'

# Widgets whose IDENTITY comes from the visible label AND which carry state
# across frames. Translating the label without pinning the id makes ImGui treat
# it as a different widget -- the Settings tab bar jumped tabs the instant the
# language changed. "###EnglishName" pins the id.
STATEFUL = ("BeginTabItem", "CollapsingHeader", "TreeNode", "TreeNodeEx")

PATTERNS = [
    (re.compile(r'(ImGui::(?:' + CALLS + r')\s*\(\s*)' + RUN, re.S), True),
    (re.compile(r'(\btip\s*\(\s*)' + RUN, re.S), False),
    (re.compile(r'(railButton\(\s*[^,]+?,\s*[^,]+?,\s*)' + RUN, re.S), False),
    # pillButton(id, icon, "Label", ...) and its width measure -- the measure
    # must use the TRANSLATED text or the layout reserves the wrong width.
    (re.compile(r'(pillButton\(\s*[^,]+?,\s*[^,]+?,\s*)' + RUN, re.S), False),
    (re.compile(r'(pillButtonWidth\(\s*[^,]+?,\s*)' + RUN, re.S), False),
    # TextColored(colour, "..."): string is the SECOND arg, and the colour is
    # often an inline ImVec4(...) whose own commas break a naive [^,]+ match --
    # tolerate one level of parentheses in the first argument.
    (re.compile(r'(ImGui::TextColored\(\s*(?:[^,()"]|\([^()]*\))+?,\s*)' + RUN, re.S), False),
    # touchui::amountField(id, "Label", ...) -- label is the 2nd arg.
    (re.compile(r'(amountField\(\s*[^,]+?,\s*)' + RUN, re.S), False),
    # materializr::inputNumber("Label", ...) -- label first.
    (re.compile(r'(inputNumber\(\s*)' + RUN, re.S), False),
    # touchui::twoRowButton(id, "Top", "Bottom") / twoRowButtonWidth("Top", x)
    (re.compile(r'(twoRowButton\(\s*[^,]+?,\s*)' + RUN, re.S), False),
    (re.compile(r'(twoRowButtonWidth\(\s*)' + RUN, re.S), False),
]


def cliteral(s):
    """Re-emit a python str as ONE C string literal, escaping what must be."""
    out = ['"']
    for ch in s:
        if ch == '"':    out.append('\\"')
        elif ch == '\\': out.append('\\\\')
        elif ch == '\n': out.append('\\n')
        elif ch == '\t': out.append('\\t')
        else:            out.append(ch)
    out.append('"')
    return "".join(out)


def include_for(path):
    depth = len(os.path.relpath(path, "src").split(os.sep)) - 1
    return '#include "%si18n.h"' % ("../" * depth)


def wrap(path):
    src = open(path, encoding="utf-8").read()
    hits = [0]
    out = src

    for pat, stateful_ok in PATTERNS:
        def repl(m):
            head, run = m.group(1), m.group(2)
            # The fragments are already C-escaped source text; joining them
            # reproduces exactly what the compiler builds.
            frags = LIT.findall(run)
            key_src = "".join(frags)
            key = decode(key_src)
            # Compare against the catalogue using the SOURCE form, because the
            # catalogue is authored from the same source text.
            vis = key.split("##")[0]
            if vis not in KEYS:
                return m.group(0)
            hits[0] += 1
            lit = key_src
            if stateful_ok and "##" not in lit and \
               any(("ImGui::" + w + "(") in head.replace(" ", "") for w in STATEFUL):
                lit = "%s###%s" % (lit, vis)
            return '%str("%s")' % (head, lit)
        out = pat.sub(repl, out)

    if hits[0]:
        if '"i18n.h"' not in out:
            lines = out.split("\n")
            # After the FIRST include, not the last: the last include of a
            # header block can sit inside a platform #ifdef, and an include
            # inserted there vanishes on the other platform (MSVC broke on
            # FileDialogs.cpp exactly this way).
            first = min(i for i, l in enumerate(lines[:90]) if l.startswith("#include"))
            lines.insert(first + 1, include_for(path))
            out = "\n".join(lines)
        open(path, "w", encoding="utf-8").write(out)
    return hits[0]


if __name__ == "__main__":
    total = 0
    for p in sys.argv[1:]:
        n = wrap(p)
        total += n
        print("  %-48s %3d" % (p, n))
    print("total: %d call sites wrapped" % total)
