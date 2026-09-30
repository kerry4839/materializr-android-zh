#!/usr/bin/env python3
"""List user-visible strings that have no translation yet.

Handles ADJACENT STRING CONCATENATION: C++ joins "a" "b" into one argument, and
the catalogue key has to be the joined result. Missing this is why the Settings
descriptions and half the tooltips never translated -- the wrapper only ever
matched the first fragment.
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
# A run of one or more adjacent literals in the FIRST argument position.
RUN = r'((?:"(?:[^"\\]|\\.)*"\s*)+)'
# A first argument of the shape `MZ_ICON_FOO "  Label"` - an icon-font MACRO
# concatenated with a literal. RUN alone cannot see these: the argument does not
# start with a quote, so the pattern above never fires and the label ships in
# English forever. That is how the create menu's "Primitive" / "Construction"
# stayed untranslated. Match the macro separately and audit the literal beside
# it; the fix at the call site is to wrap the literal in tr() and concatenate,
# e.g. MZ_ICON_FOO "  " + std::string(tr("Label")).
MACRO_ARG = r'[A-Za-z_][A-Za-z0-9_:.]*\s+(?=' + RUN + r')'
PATTERNS = [
    re.compile(r'ImGui::(?:' + CALLS + r')\s*\(\s*' + RUN, re.S),
    re.compile(r'\btip\s*\(\s*' + RUN, re.S),
    re.compile(r'railButton\(\s*[^,]+?,\s*[^,]+?,\s*' + RUN, re.S),
    re.compile(r'pillButton\(\s*[^,]+?,\s*[^,]+?,\s*' + RUN, re.S),
    re.compile(r'pillButtonWidth\(\s*[^,]+?,\s*' + RUN, re.S),
    re.compile(r'ImGui::TextColored\(\s*(?:[^,()"]|\([^()]*\))+?,\s*' + RUN, re.S),
    re.compile(r'amountField\(\s*[^,]+?,\s*' + RUN, re.S),
    re.compile(r'inputNumber\(\s*' + RUN, re.S),
    re.compile(r'twoRowButton\(\s*[^,]+?,\s*' + RUN, re.S),
    re.compile(r'twoRowButtonWidth\(\s*' + RUN, re.S),
]
# Same sinks, but the first argument is MACRO + literal run.
MACRO_PATTERNS = [
    re.compile(r'ImGui::(?:' + CALLS + r')\s*\(\s*' + MACRO_ARG, re.S),
    re.compile(r'\btip\s*\(\s*' + MACRO_ARG, re.S),
]

def joined(run):
    return "".join(LIT.findall(run))

def scan(path):
    s = open(path, encoding="utf-8", errors="replace").read()
    found = []
    for pat in PATTERNS:
        for m in pat.finditer(s):
            v = decode(joined(m.group(1)))
            vis = v.split("##")[0]
            if not vis.strip() or not re.search(r"[A-Za-z]{2}", vis):
                continue
            if vis in KEYS:
                continue
            found.append(vis)
    for pat in MACRO_PATTERNS:
        for m in pat.finditer(s):
            # A macro-concatenated label is the WHOLE joined run minus the
            # leading padding spaces the icon gap contributes.
            v = decode(joined(m.group(1))).strip()
            if not v or not re.search(r"[A-Za-z]{2}", v):
                continue
            if v in KEYS:
                continue
            found.append(v)
    return found

def main():
    args = sys.argv[1:]
    if not args:
        # Default to the whole tree. With no arguments this used to scan an EMPTY
        # list and cheerfully report "# 0 untranslated strings" no matter what the
        # source said - a vacuous pass that reads exactly like a clean bill of
        # health. An audit that can report success without looking at anything is
        # worse than no audit, so a bare run now means "audit src/".
        src = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           os.pardir, "src")
        for dirpath, _dirs, names in os.walk(src):
            for n in sorted(names):
                if n.endswith((".cpp", ".h")):
                    args.append(os.path.join(dirpath, n))
    allf = {}
    for p in args:
        for v in scan(p):
            allf.setdefault(v, set()).add(os.path.relpath(p))
    print("# %d untranslated strings in %d file(s)" % (len(allf), len(args)))
    for v in sorted(allf, key=lambda x: (len(x), x)):
        print("%s\t%s" % (v.replace("\t", " "), ",".join(sorted(allf[v]))))
    return 1 if allf else 0

if __name__ == "__main__":
    sys.exit(main())
