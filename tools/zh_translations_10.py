# -*- coding: utf-8 -*-
"""Batch 10 (Chinese): the last seventeen gaps tools/check_tr_coverage.py found.

Pairs with catalogue_entries_10.py. No source change was needed for this batch
- every string was already wrapped in tr() and simply had no catalogue row.

The embedded newlines in the shear-mode help are real newlines in the key (see
esc() in add_catalogue_entries.py); zh_translations keys must match byte for
byte, so they are written the same way here.
"""

ZH_TRANSLATIONS_10 = {
    # ── Fillet timeout tooltip (Settings > fillet budget) ──────────────────
    "How long a fillet may take before it is refused. OCCT's blend cannot be cancelled once started, so a fillet is first run on a background copy and abandoned if it exceeds this. Raise it for large or heavy bodies where fillets legitimately take longer; lower it if a fillet that cannot be built keeps you waiting.":
        "圆角最长可以耗时多久才判定为失败。OCCT 的过渡运算一旦启动就无法取消，因此圆角会先在后台的一份副本上运算，若超过该时长就放弃。对于圆角确实需要更久的大尺寸或重型实体，请调大该值；若因某个无法构建的圆角而一直等待，请调小它。",

    # ── Sew / Patch: why the patch landed as its own surface ───────────────
    "This body is already closed, so there's nothing to sew into - the patch lands as its own surface. A hole that goes right through needs both ends capped, then Sew.":
        "该实体已经封闭，因此没有可供缝合的目标，补丁会作为独立曲面落下。贯穿的孔需要先把两端封住，然后再执行缝合。",
    "The patch wouldn't join the opening, so it lands as its own surface - the fitted edge may not reach the rim.":
        "补丁无法与开口接合，因此会作为独立曲面落下，拟合边可能没有到达边界。",
    "The section's own trailing edge has thickness, which is what you want for a printed or machined part; a knife edge cannot be manufactured.":
        "该截面自身的后缘带有厚度，这正是打印件或机加工件所需要的；刀刃状的零厚度边无法制造。",

    # ── Sketch chain-selection tooltips ────────────────────────────────────
    "Hover a line, arc or circle - the whole connected chain highlights. Click to choose it.":
        "将光标悬停在直线、圆弧或圆上，整条相连的链都会高亮。点击即可选中它。",
    "Move the cursor to set the distance; the side it is on picks the direction. Click or press Enter to place it.":
        "移动光标以设定距离；光标所在的一侧决定方向。点击或按回车键放置。",
    "Put the offset on the other side of the chain.":
        "请把偏移放到该链的另一侧。",
    "Round bridges an opening corner with an arc; Sharp extends both edges to their intersection.":
        "「圆角」用一段圆弧跨过开口的角；「尖角」则把两条边都延伸到它们的交点。",

    # ── Shear-mode help (embedded newlines) ────────────────────────────────
    "Off: the whole body shears, so holes and features inside it lean with the face.\nOn: only the faces touching this one are rebuilt - everything else stays put.\nThe face you tilt has to be flat; what it meets can curve.":
        "关闭：整个实体一起发生剪切，因此内部的孔和特征会随该面一起倾斜。\n开启：只重建与该面相接的面，其余部分保持不动。\n你倾斜的那个面必须是平面；与它相接的面可以是曲面。",

    # ── Sketch history label ───────────────────────────────────────────────
    "Rotate sketch": "旋转草图",

    # ── Dimension-mode banner ──────────────────────────────────────────────
    "DIMENSION - Click an entity to dimension.":
        "标注：点击一个实体进行标注。",
    "DIMENSION - Click a second entity, or empty space to place.":
        "标注：点击第二个实体，或点击空白处直接放置。",
    "DIMENSION - Click to place the label.":
        "标注：点击以放置标签。",

    # ── Measure tool: the result-row labels ────────────────────────────────
    "Bounding Box": "包围盒",
    "Bounding Box (%d bodies)": "包围盒（%d 个实体）",
    "Edge Length": "棱边长度",
    "Total Edge Length (%d edges)": "棱边总长（%d 条边）",
}