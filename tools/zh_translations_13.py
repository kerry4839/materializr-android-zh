# -*- coding: utf-8 -*-
"""Batch 13 (Chinese): the Combo item tables.

Pinyin-free where a real Chinese term exists (螺纹牙型、刚度、字体的衬线/无衬线
分类), and transliterated only where the name IS the thing the user is picking:
月份缩写在界面上就是数字前面的那两个字，用「1月」而不是「一月」更贴近中文
日期写法。
"""

ZH_TRANSLATIONS_13 = {
    # ── 螺纹牙型 ───────────────────────────────────────────────────────────
    "Standard (V)": "普通（V 型）",
    "Trapezoidal (ACME)": "梯形（ACME）",
    "Square": "方形",
    "Buttress": "锯齿形",
    "Rounded (print)": "圆弧形（打印用）",

    # ── 曲面片连续性 ───────────────────────────────────────────────────────
    "Position (C0)":  "位置连续（C0）",
    "Tangent (G1)":   "切线连续（G1）",
    "Curvature (G2)": "曲率连续（G2）",

    # ── 草图偏移：开口角点的处理方式 ───────────────────────────────────────
    "Round corners": "圆角",
    "Sharp corners": "尖角",

    # ── 展开刚度。面板上的标签是「材料」，这里选的是料的软硬，不是种类。 ──
    "Pliable":     "软性",
    "Semi-rigid":  "半硬",
    "Rigid":       "硬性",

    # ── AI 服务商驱动说明（旁边的厂商名是专有名词，保留原文） ─────────────
    "OpenAI-compatible (OpenAI/Ollama/LM Studio)":
        "OpenAI 兼容接口（OpenAI/Ollama/LM Studio）",

    # ── 草图文字的内置字体。字体名本身是专有名词，保留；外面的分类译。 ────
    "Mono (JetBrains)":    "等宽 (JetBrains)",
    "Sans (DejaVu)":       "无衬线 (DejaVu)",
    "Serif (DejaVu)":      "衬线 (DejaVu)",
    "Times (Liberation)":  "Times (Liberation)",
    "Arial (Liberation)":  "Arial (Liberation)",
    "Ubuntu":              "Ubuntu",
    "Comic (Neue)":        "漫画体 (Neue)",
    "Stencil (Black Ops)": "模板体 (Black Ops)",
    "Impact (Anton)":      "标题体 (Anton)",
    "Script (Pacifico)":   "手写体 (Pacifico)",

    # ── 构造平面类型。前三个 XY/XZ/YZ 是平面名，各语言一致，不在此列。 ────
    "Offset from Plane":   "相对平面偏移",
    "Through 3 Points":    "通过三点",
    "Parallel to Face":    "平行于面",
    "Midplane":            "中平面",
    "Normal to Axis":      "垂直于轴",
    "Tangent to Cylinder": "相切于圆柱",
    "Through Axis":        "通过轴",

    # ── 变换类型 ───────────────────────────────────────────────────────────
    "Translate": "平移",
    "Rotate":    "旋转",
    "Scale":     "缩放",

    # ── 贴合类型 ───────────────────────────────────────────────────────────
    "Fasten":     "紧固",
    "Concentric": "同心",
    "Planar":     "平面",

    # ── 剖视：切在哪个平面上 ───────────────────────────────────────────────
    "YZ (X normal)": "YZ（X 法向）",
    "XZ (Y normal)": "XZ（Y 法向）",
    "XY (Z normal)": "XY（Z 法向）",

    # ── 月份缩写，供 trDate() 使用（年月日顺序由代码决定，不在此列） ──────
    "Jan": "1月", "Feb": "2月", "Mar": "3月", "Apr": "4月",
    "May": "5月", "Jun": "6月", "Jul": "7月", "Aug": "8月",
    "Sep": "9月", "Oct": "10月", "Nov": "11月", "Dec": "12月",
}
