# -*- coding: utf-8 -*-
"""Batch 15 (Chinese): Operation::name() values, gizmo labels, and the
conditional-label strings that reached a sink without a tr().

Two groups with different constraints:

* The op names are the word on a history step box - icon plus one word in a
  fixed-width chip - so 布尔 rather than 布尔运算, 编辑草图 rather than 草图编辑.
* The popup titles and menu labels are full sentences or noun phrases, and are
  allowed the length a real menu needs.

The maths symbols (Ø, ⊥) and the unit tags (mm) stay Latin: they are notation,
and transliterating them changes what the symbol means.
"""


ZH_TRANSLATIONS_15 = {
    # ── Inline name() overrides: step-box labels, kept to two characters ────
    "Boolean":          "布尔",
    "Combine Sketches": "合并草图",
    "Guided Loft":      "引导放样",
    "Sew":              "缝合",
    "Sketch Edit":      "编辑草图",
    "Sketch Transform": "变换草图",
    "Tweak Face":       "调整面",

    # ── Gizmo commit labels ────────────────────────────────────────────────
    "Rotate Plane": "旋转平面",
    "Move Plane":   "移动平面",
    "Rotate Axis":  "旋转轴",
    "Move Axis":    "移动轴",

    # ── Edge-op banner name ────────────────────────────────────────────────
    "FILLET":  "圆角",
    "CHAMFER": "倒角",

    # ── ItemsPanel folder context menu ─────────────────────────────────────
    "(root - no folder)":                "(根目录 - 无文件夹)",
    "(root - no folder) - all selected": "(根目录 - 无文件夹) - 全选",
    "all selected":                      "全选",
    "New folder…":                       "新建文件夹…",
    "New folder… (all selected)":        "新建文件夹…(全选)",

    # ── Toolbar inference-level button ─────────────────────────────────────
    "Inferences: Full":    "推理:完全",
    "Inferences: Reduced": "推理:精简",
    "Inferences: Off":     "推理:关闭",
    "Inferences: Max":     "推理:最大",

    # ── Sketch pattern popup title ─────────────────────────────────────────
    "Sketch Linear Pattern":   "草图线性阵列",
    "Sketch Circular Pattern": "草图环形阵列",

    # ── Pattern / Patch continuity words ───────────────────────────────────
    "Linear":    "线性",
    "Radial":    "放射",
    "curvature": "曲率",
    "tangent":   "相切",
    "position":  "位置",

    # ── Sketch constraint types: 直径 keeps the symbol, 间隙 for Gap (the CAD
    #    sense is the clearance between two circles, not a literal gap). ─────
    "\u00d8 (diameter)": "\u76f4\u5f84(\u00d8)",
    "Dist \u22a5": "Dist \u22a5",
    "Gap": "间隙",
}