# -*- coding: utf-8 -*-
"""Batch 14 (Chinese): Operation::description() sentences and words fed to %s.

The Chinese renderings try to avoid clumsy hybrid forms while staying consistent
with the catalogue's existing word choices (螺纹, 抽壳, 折痕/压线, 区域). For
"%(s)" plurals we translate the noun naturally and keep the pattern close to
"%d 个区域".
"""


ZH_TRANSLATIONS_14 = {
    # ── Thread side ────────────────────────────────────────────────────────
    "Internal": "内螺纹",
    "External": "外螺纹",

    # ── Snap toggle ────────────────────────────────────────────────────────
    "Snap ON":  "吸附：开",
    "Snap off": "吸附：关",

    # ── Cylinder side ──────────────────────────────────────────────────────
    "hole":  "孔",
    "outer": "外圆",

    # ── Cylinder end picker ────────────────────────────────────────────────
    "Both ends":   "两端",
    "Bottom end":  "下端",
    "Top end":     "上端",
    "outer face":  "外表面",

    # ── Merge faces scope ─────────────────────────────────────────────────
    "selected faces": "选中的面",
    "whole body":     "整个实体",

    # ── Resize heading ────────────────────────────────────────────────────
    "Hole": "孔",

    # ── ScaleFaceOp mode ───────────────────────────────────────────────────
    "Extend": "拉伸",
    "extend": "拉伸",
    "pinch":  "收缩",

    # ── Thread depth caps ─────────────────────────────────────────────────
    "Depth caps at 0.45 × pitch (multi-start rounded).": "深度限制为 0.45 × 螺距（多头螺纹取圆角）。",
    "Depth caps at 0.65 × pitch (ISO is 0.61).":         "深度限制为 0.65 × 螺距（ISO 标准为 0.61）。",

    # ── Descriptions ───────────────────────────────────────────────────────
    "Move hole %s":                "移动孔 %s",
    "Engrave sketch %s":           "雕刻草图 %s",
    "Emboss sketch %s":            "浮雕草图 %s",
    "Push/Pull %s (%zu region(s))": "推/拉 %s（%zu 个区域）",
    "Resize %s D %s → %s":         "调整 %s 尺寸：直径 %s → %s",
    "Shape %s: %s / %s (bottom / top)": "塑形 %s：%s / %s（底部 / 顶部）",
    "Scale face to %.0f%%/%.0f%% over %s (%s)": "缩放面至 %.0f%%/%.0f%%，范围 %s（%s）",

    # ── (s) plurals ────────────────────────────────────────────────────────
    "%zu fold/score line(s)":                          "%zu 条折痕/压线",
    "Selection: %d point(s), %d line(s), %d curve(s)": "选区：%d 个点、%d 条直线、%d 条曲线",
    "%s  (%d step(s))":                                "%s（%d 个步骤）",
    "%d region(s) selected":                           "已选 %d 个区域",
    "Selected: %d body(s)":                            "已选：%d 个实体",
    "Selected: %d edge(s)":                            "已选：%d 条边",
}