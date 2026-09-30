# -*- coding: utf-8 -*-
"""Batch 9 (Chinese): refusals, validation messages, and prose that reaches the
UI by return rather than through an ImGui sink.

Pairs with catalogue_entries_9.py. The specifier ORDER is fixed by
check_specifiers.py, so each line keeps the English argument order - for example
旋转 %.1f 度 rather than the more natural 绕 (x, y, z) 旋转 n 度, which would
swap arguments.
"""

ZH_TRANSLATIONS_9 = {
    # ── The mesh gate ───────────────────────────────────────────────────────
    "%s needs solid geometry, and this is an imported mesh - a reference body. Sketch on it and snap to it all you like, then model the part alongside it.":
        "%s 需要实体几何，而这是一个导入的网格，它属于参考实体。你可以在它上面画草图、随意对它捕捉，然后把零件建模在它旁边。",
    "%s needs solid geometry, and one of the selected bodies is an imported mesh - a reference body. Leave the import out of the selection.":
        "%s 需要实体几何，而所选实体中有一个是导入的网格，它属于参考实体。请把导入体从选区中去掉。",
    "That operation": "该操作",

    # ── Merge-faces refusals ────────────────────────────────────────────────
    "Those faces point in opposite directions - they lie in the same plane, but the material is on opposite sides, so they are different surfaces of the part rather than two halves of one. Merging can't join them, and wouldn't add material to either.":
        "这些面的法向相反：它们位于同一平面，但材料在相反的两侧，所以它们是零件的两个不同表面，而不是同一个面的两半。合并无法把它们接合，也不会给任何一个面补料。",
    "Those faces don't touch - merging dissolves the edge between two faces, and there isn't one. If a small step separates them, level it first, then merge.":
        "这些面并不相接：合并会消除两个面之间的棱边，而这里并没有。如果它们之间隔着一道小台阶，请先把它找平再合并。",
    "That merge would have moved material, so it was refused - the faces are one surface, but joining them would reshape the part rather than tidy it.":
        "这次合并会移动材料，因此已被拒绝：这两个面确实是同一个表面，但把它们连起来会改变零件形状，而不是把零件整理干净。",
    "Couldn't find those faces on the body any more - they may already have been merged.":
        "在实体上已经找不到这些面了，它们可能已经被合并了。",

    # ── Thread-step ordering toast ──────────────────────────────────────────
    "Couldn't reorder this change beneath the Thread step. Delete the Thread step, make the change, then re-thread.":
        "无法把这项改动移到螺纹步骤之下。请先删除螺纹步骤，做完改动后再重新螺纹。",

    # ── Undo / export prompts ───────────────────────────────────────────────
    "You have unsaved changes. Save before closing the project?":
        "有未保存的更改。关闭项目前要保存吗？",
    "You have unsaved changes. Save before opening another project?":
        "有未保存的更改。打开另一个项目前要保存吗？",
    "You have unsaved changes. Save before exiting?":
        "有未保存的更改。退出前要保存吗？",
    "Can't export to %s.": "无法导出到 %s。",
    "Not an airfoil file: %s": "不是翼型文件：%s",

    # ── Sketch-change rejection ─────────────────────────────────────────────
    " (step %d: %s)": "（第 %d 步：%s）",
    "Couldn't update the model for that sketch change - a downstream feature%s couldn't follow it, so the model was left unchanged.":
        "无法根据这项草图改动更新模型：下游特征%s 无法跟随它，因此模型保持不变。",

    # ── Move-face refusals ──────────────────────────────────────────────────
    "that face isn't part of this body": "该面不属于这个实体",
    "only flat faces can be tweaked yet": "目前只能调整平面",
    "a corner of this face doesn't have a single edge running into the body":
        "该面的某个角没有唯一一条棱边引入实体",
    "sliding a flat face inside its own plane doesn't move anything - tilt it, or push it along its normal":
        "让平面在自己的平面内滑动不会产生任何变化：请改为倾斜它，或沿法线方向推移",
    "that move takes the face past an edge it has to stay attached to":
        "该移动会使这个面越过一条它必须保持相连的棱边",
    "a neighbouring face no longer meets this one after that move":
        "该移动之后，相邻的面不再与这个面相交",
    "an edge running off this face can't reach its new corner":
        "从该面伸出的棱边无法到达它的新角点",
    "the rebuilt body wouldn't close up": "重建后的实体无法封闭",
    "the rebuilt body failed validation": "重建后的实体未通过校验",
    "The face moved and only the faces meeting it were rebuilt - the rest of the body is untouched.":
        "该面已移动，并且只重建了与它相接的面，实体的其余部分未受影响。",

    # ── Primitive validation ────────────────────────────────────────────────
    "All three extents must be > 0.": "三个方向的尺寸都必须 > 0。",
    "Radius and height must be > 0.": "半径和高度都必须 > 0。",
    "Radius and height must both be > 0.": "半径和高度必须都 > 0。",
    "Radius must be > 0.": "半径必须 > 0。",
    "Height > 0 and at least one positive radius.": "高度 > 0，且至少有一个半径为正。",
    "Height must be > 0 and at least one radius must be positive (the other may be 0 for a tip).":
        "高度必须 > 0，且至少有一个半径为正（另一个可以为 0，表示尖端）。",
    "Major and minor must both be > 0.": "大半径和小半径必须都 > 0。",
    "Major and minor radii must both be > 0.": "大半径和小半径必须都 > 0。",
    "Major must exceed minor (equal = horn-torus singularity; smaller = self-intersecting spindle).":
        "大半径必须大于小半径（相等 = 喇叭环面奇点；更小 = 自相交的纺锤环面）。",
    "Major radius must exceed minor (equal = horn torus with zero-diameter hole; smaller = self-intersecting spindle torus).":
        "大半径必须大于小半径（相等 = 直径为零的孔的喇叭环面；更小 = 自相交的纺锤环面）。",
    "Importing STL…": "正在导入 STL…",
    "Select a body to unfold.": "请选择一个要展开的实体。",

    # ── Split-half names ────────────────────────────────────────────────────
    "left / right": "左 / 右",
    "front / back": "前 / 后",
    "top / bottom": "上 / 下",

    # ── Import / export failures ────────────────────────────────────────────
    "BREP file too large (> 512 MB) - refusing to load":
        "BREP 文件过大（> 512 MB）：拒绝加载",
    "DXF is too large or too complex to import safely (entity, vertex or output budget exceeded).":
        "DXF 过大或过于复杂，无法安全导入（超出了实体、顶点或输出预算）。",
    "Save File": "保存文件",
    "Open File": "打开文件",
    "Failed to open %s": "无法打开 %s",
    "Write failed: %s": "写入失败：%s",
    "Failed to initialise libcurl.": "libcurl 初始化失败。",

    # ── Airfoil import ──────────────────────────────────────────────────────
    "not enough lines to be a coordinate file": "行数太少，不可能是坐标文件",
    "no coordinates after the header": "文件头之后没有坐标",
    "fewer coordinates than the count line promises": "坐标数量少于计数行所声明的数量",
    "too few coordinate pairs": "坐标对太少",
    "could not find the leading edge (is this a Selig file?)":
        "找不到前缘（这是 Selig 格式的文件吗？）",
    "not enough points on one of the surfaces": "其中一个曲面上的点太少",
    "coordinates are not chord-normalised (expected x from 0 to 1)":
        "坐标未按弦长归一化（x 应在 0 到 1 之间）",
    "could not open the file": "无法打开文件",

    # ── Unfold warnings ─────────────────────────────────────────────────────
    "No faces to unfold.": "没有可展开的面。",
    "No planar faces to unfold.": "没有可展开的平面。",
    "Nothing to unfold.": "没有可展开的内容。",
    "Nothing to tessellate.": "没有可细分的内容。",

    # ── Sketch offset rejections ────────────────────────────────────────────
    "Nothing to offset": "没有可偏移的内容",
    "Offset distance is zero": "偏移距离为零",

    # ── Dimension-pick rejections ───────────────────────────────────────────
    "Same entity - pick a different one.": "是同一个实体，请另选一个。",
    "These lines can't be dimensioned to each other (they meet, or are collinear).":
        "这两条直线之间无法标注尺寸（它们相交或共线）。",
    "That pair can't be dimensioned.": "这一对无法标注尺寸。",
    "A single point has no dimension - pick a second entity.":
        "单个点没有尺寸，请再选一个实体。",

    # ── Mode names and constraint-state labels ──────────────────────────────
    "Sketch Mode": "草图模式",
    "Gizmo Drag": "拖动手柄",
    "Fully constrained": "完全约束",
    "Under-constrained": "约束不足",
    "Over-constrained": "约束过度",

    # ── Inference-guide names ───────────────────────────────────────────────
    "Endpoint": "端点",
    "Midpoint": "中点",
    "On Line": "在线上",
    "On Circle": "在圆上",
    "On Horizontal Axis": "在水平轴上",
    "On Vertical Axis": "在竖直轴上",
    "Perpendicular": "垂直",
    "Parallel": "平行",
    "On Line Extension": "在线的延长线上",
    "Tangent": "相切",
    "Perpendicular from Point": "过点的垂线",
    "Symmetry": "对称",
    "Corner Bisector": "角平分线",
    "Corner Tangent": "角相切",

    # ── Dimension-mode labels and hints ─────────────────────────────────────
    "Chord (%s)": "弦长 (%s)",
    "Sweep (deg)": "张角（度）",
    "Radius (%s)": "半径 (%s)",
    "Type the straight-line distance between the arc's two ends and press Enter - or just click the end.":
        "请输入圆弧两端之间的直线距离并按回车，也可以直接点击端点。",

    # ── Entity-name prefixes ────────────────────────────────────────────────
    "Body ": "实体 ",
    "Sketch ": "草图 ",
    "Folder ": "文件夹 ",

    # ── Material library names ──────────────────────────────────────────────
    "Steel": "钢",
    "Aluminum": "铝",
    "Copper": "铜",
    "Gold": "金",
    "Plastic White": "白色塑料",
    "Plastic Black": "黑色塑料",
    "Plastic Red": "红色塑料",
    "Wood": "木材",
    "Glass": "玻璃",
    "Rubber": "橡胶",
    "Ceramic": "陶瓷",
    "Concrete": "混凝土",
}
