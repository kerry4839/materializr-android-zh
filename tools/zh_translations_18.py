# -*- coding: utf-8 -*-
"""Batch 18 (Chinese): the strings shape 4 of check_op_names.py found.

Same cause as the batch's six-language file - these all reached the screen
through a local UI helper instead of an ImGui sink - so the same two
conventions apply.

The axis letters X, Y, Z map to themselves. They are notation, and a row that
repeats the key is not a missing translation: it is the catalogue recording
that a tr() call here was deliberate. The call site still needs tr() so that
the intent is explicit and the row exists if a language ever does want to
rename an axis, but nothing is gained by writing anything else.

The On/Off halves of the nav toggles are written 开/关 with an ASCII colon, not
：. The surrounding chips are compact status readouts where the colon is a
separator, not sentence punctuation - a full-width colon there takes a whole
em of width in a chip that is already tight. The tutorial's 'Touch:' / 'Mouse:'
are the opposite case: they introduce a sentence, so they take '：'.
"""

ZH_TRANSLATIONS_18 = {
    # ── Axis notation: the key is the answer ─────────────────────────────
    "X": "X",
    "Y": "Y",
    "Z": "Z",

    # ── Nav overlay toggles ───────────────────────────────────────────────
    # 「移动:开」 rather than 「移动：开」 - see the module docstring.
    "Move: On":        "移动:开",
    "Move: Off":       "移动:关",
    "Multi-Select: On":  "多点选择:开",
    "Multi-Select: Off": "多点选择:关",
    "Cancel Shape":    "取消图形",

    # ── Model-tree and colour-picker tooltips ─────────────────────────────
    "Show the model tree (bodies, sketches, construction)":
        "显示模型树（实体、草图、构造）",
    "Body colour":     "实体颜色",
    "Folder colour":   "文件夹颜色",
    "Show the model tree": "显示模型树",

    # ── Sketch commit / discard tooltips ─────────────────────────────────
    "Throw the sketch away and leave (asks to confirm)":
        "丢弃草图并退出（会要求确认）",
    "Finish the sketch and leave sketch mode":
        "完成草图并退出草图模式",
    "Leave sketch mode, keeping the sketch":
        "退出草图模式并保留草图",

    # ── Tutorial tour platform labels ────────────────────────────────────
    # Full-width colon here: these introduce a sentence.
    "Touch:": "触摸：",
    "Mouse:": "鼠标：",

    # ── Shortcut-table section headings ──────────────────────────────────
    "While a tool is running": "工具运行时",
    "In a sketch":             "在草图中",

    # ── The other half of two pairs ──────────────────────────────────────
    # 'Mouse' was already in the catalogue and 'Touch' was not, so the touch
    # table's heading was English while the mouse table's was translated. They
    # are fixed together: a pair split across two batches is a pair nobody
    # notices is half done.
    # 'Touch' is 触摸 and not 触摸手势 - the section lists the individual
    # gestures below it, and the heading names the input device.
    "Touch":   "触摸",
    "Cancel the in-progress shape": "取消正在绘制的图形",
}
