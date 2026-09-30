# -*- coding: utf-8 -*-
"""Batch 8a (Chinese): Operation::description() sentences.

Pairs with catalogue_entries_8.py. The specifier ORDER is fixed by
check_specifiers.py, so each line keeps the English argument order - for
example 旋转 %.1f° about (%.1f, ...) rather than the more natural
绕 (x, y, z) 旋转 n 度, which would swap two arguments.
"""

ZH_TRANSLATIONS_8 = {
    # ── Extrude / Revolve ──────────────────────────────────────────────────
    "Extrude %s%s": "拉伸 %s%s",
    "Lathe %d deg%s": "旋转成形 %d 度%s",
    " (New Body)": "（新建实体）",
    " (Union)": "（并集）",
    " (Subtract)": "（差集）",
    " (Intersect)": "（交集）",

    # ── Boolean / Mirror ────────────────────────────────────────────────────
    "Boolean %s (body %d with body %d)": "布尔 %s（实体 %d 与实体 %d）",
    "Mirror across %s%s": "关于 %s 镜像%s",
    " (keep original)": "（保留原件）",

    # ── Pattern / Split / Defeature / Combine / Separate ────────────────────
    "%s Pattern, %d copies": "%s阵列，共 %d 个",
    "Split Body %d by plane": "按平面拆分实体 %d",
    "Remove %d face(s)": "移除 %d 个面",
    "Combine %d sketches": "合并 %d 个草图",
    "Separate body %d into %d bodies": "将实体 %d 拆分为 %d 个实体",
    "Loft through %d profiles%s%s": "通过 %d 个轮廓放样%s%s",
    " (Solid)": "（实体）",
    " (Shell)": "（壳体）",
    " Ruled": " 直纹",
    "Guided loft: base profile + %d rail(s)": "引导放样：基础轮廓 + %d 条导轨",
    "Patch across %d edges (%s)": "跨 %d 条边打补丁（%s）",
    "Boundary fill from %d silhouettes": "由 %d 条轮廓线做边界填充",
    "Merge faces (%d → %d)": "合并面（%d → %d）",

    # ── Fillets / chamfers / shell / sweep ──────────────────────────────────
    "Chamfer D%s on %d edge(s)": "倒角 D%s，共 %d 条边",
    "Chamfer D%s/%s on %d edge(s)": "倒角 D%s/%s，共 %d 条边",
    "Fillet R%s on %d edge(s)": "圆角 R%s，共 %d 条边",
    "Shell thickness %s (%d open face(s))": "抽壳厚度 %s（%d 个开放面）",
    "Sweep profile along path": "沿路径扫掠轮廓",
    "Mesh trace setup (3 planes + sketches)": "网格描摹设置（3 个平面 + 草图）",

    # ── Transforms ──────────────────────────────────────────────────────────
    "Align body %d from %s to %s": "对齐实体 %d：从 %s 到 %s",
    "Translate %s": "平移 %s",
    "Rotate %.1f deg around (%.1f, %.1f, %.1f)": "绕 (%.1f, %.1f, %.1f) 旋转 %.1f 度",
    "Scale by %.2f": "缩放 %.2f 倍",
    "Transform": "变换",
    " (axis %d)": "（轴 %d）",
    " (%d axes)": "（%d 条轴）",
    " (plane %d)": "（平面 %d）",
    " (%d planes)": "（%d 个平面）",

    # ── Construction geometry ───────────────────────────────────────────────
    "Construction Axis (%s)": "构造轴（%s）",
    "Construction Plane: %s (%s)": "构造平面：%s（%s）",
    "Offset (%s)": "偏移（%s）",
    "World X": "世界 X",
    "World Y (Z-up)": "世界 Y（Z 轴向上）",
    "World Z": "世界 Z",
    "Two points": "两点",
    "Face normal": "面法线",
    "Cylinder axis": "圆柱轴线",
    "Along edge": "沿边",
    "Plane intersection": "平面交线",
    "3 Points": "3 点",
    "Parallel to Face": "平行于面",
    "Midplane": "中面",
    "Normal to Axis": "垂直于轴",
    "Tangent to Cylinder": "与圆柱相切",
    "Through Axis": "过轴",

    # ── Copy / delete / duplicate ───────────────────────────────────────────
    "Duplicate body %d offset %s": "复制实体 %d，偏移 %s",
    "Delete body %d": "删除实体 %d",
    "Delete \"%s\"": "删除「%s」",
    "Duplicate → %s": "复制 → %s",
    "Duplicate sketch": "复制草图",

    # ── Sketch edits ────────────────────────────────────────────────────────
    "Sketch edit": "草图编辑",
    "Add sketch element": "添加草图元素",
    "Remove sketch element": "删除草图元素",
    "Modify sketch": "修改草图",
    "Circle Ø%s": "圆 Ø%s",
    "Arc R%s": "圆弧 R%s",
    "Rectangle %s × %s": "矩形 %s × %s",
    "Line %s": "直线 %s",

    # ── Thread ──────────────────────────────────────────────────────────────
    "%s thread Ø%s, pitch %s%s%s": "%s螺纹 Ø%s，螺距 %s%s%s",
    "%d-start": "%d 头",
    " (LH)": "（左旋）",
}
