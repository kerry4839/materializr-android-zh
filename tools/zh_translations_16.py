# -*- coding: utf-8 -*-
"""Batch 16 (Chinese): properties-panel surface/curve type names, toolbar labels.

These appear as the accent-coloured heading line of the selection properties
panel, so they are read at a glance and want to be short nouns rather than
sentences: 「平面」 not 「平面类型的面」.

The CAD terms follow the existing catalogue's choices so the same word is not
rendered two ways in two panels - 面/边 for Face/Edge, 拉伸面 for Extruded face,
回转面 for Revolved face (the surface of revolution, not "rotated face").
"""

ZH_TRANSLATIONS_16 = {
    # ── Surface kinds ──────────────────────────────────────────────────────
    "Planar face":      "平面",
    "Cylindrical face": "圆柱面",
    "Conical face":     "圆锥面",
    "Spherical face":   "球面",
    "Toroidal face":    "圆环面",
    "Extruded face":    "拉伸面",
    "Revolved face":    "回转面",
    "Freeform face":    "自由曲面",

    # ── Curve kinds ────────────────────────────────────────────────────────
    "Straight edge":    "直线边",
    "Circular edge":    "圆弧边",
    "Elliptical edge":  "椭圆边",
    "Curved edge":      "曲线边",

    # ── Toolbar: sketch draw-origin toggle ─────────────────────────────────
    # 「起点」 rather than 「从…绘制」: the button cycles an origin, so what
    # matters is which point the shape grows from, not the drawing verb.
    "Draw from: Corner":  "起点:角点",
    "Draw from: Center":  "起点:中心",
    "Draw from: 2-Point": "起点:两点",
}