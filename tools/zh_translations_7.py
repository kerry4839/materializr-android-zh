# -*- coding: utf-8 -*-
"""Chinese translations, batch 7 - the strings the final audit turned up.

Every key here is an ENGLISH STRING ALREADY WRAPPED IN tr() that the catalogue
simply had no row for, so tr() was handing the raw English straight back to the
user. They fall into five groups:

  * undo-step / history labels built with snprintf ("Draft %.1f deg ...",
    "Sew %d surfaces ...") - these are what the History timeline and the Items
    tree show for every modelling operation;
  * modal titles, which gained a "###MZxxx" ID so the visible half can be
    translated without changing the ImGui window identity;
  * the Patch fit dialog's solver vocabulary (Stiffness, Tangency tolerance,
    Max degree, ...);
  * tooltips on the im-touch / modern layout rails;
  * short status fragments.

Two keys are deliberately fragmentary and must stay that way:

  "Rotate (" / " bodies)" and "Scale ("
      the gizmo caption is assembled as tr("Rotate (") + count + tr(" bodies)"),
      so both halves need a row or the label comes out half-translated.

  "No closed silhouette here - the mesh may be open/non-manifold from this direction."
      a toast whose tr() spans TWO adjacent string literals. The catalogue key
      is the JOINED result, not the first fragment - tools/i18n_extract.py
      documents the same trap, and getting it wrong means the whole toast stays
      English while the catalogue looks 100% full.

Note the printf specifiers MUST keep their order and count - update_catalogue_zh.py
checks that, because a reordered %s/%d prints garbage rather than failing.
"""

ZH_TRANSLATIONS_7 = {
    # ── History timeline: the per-operation label of every modelling step ──
    "Add %s": "添加%s",
    "Add Ø %s": "添加直径%s",
    "Add Distance %s": "添加距离%s",
    "Add Gap %s": "添加间隙%s",
    "Add Angle %.1f°": "添加角度%.1f°",
    "Add constraint": "添加约束",
    "Edit %s": "编辑%s",
    "Edit Angle %.1f° → %.1f°": "编辑角度 %.1f° → %.1f°",
    "Remove %s": "移除%s",
    "Remove constraint": "移除约束",
    "Draft %.1f deg (%d face(s))": "拔模 %.1f°（%d 个面）",
    "Move Face by (%.2f, %.2f, %.2f)": "移动面 (%.2f, %.2f, %.2f)",
    "Twist Face %.1f°": "扭转面 %.1f°",
    "Move sketch (%.2f, %.2f, %.2f)": "移动草图 (%.2f, %.2f, %.2f)",
    "Tweak face %.2f deg": "微调面 %.2f°",
    "Tweak face ": "微调面 ",
    "Sew %d surfaces into a solid": "将 %d 个面缝合成实体",
    "Sew %d surfaces (%d edge(s) still open)": "缝合 %d 个面（仍有 %d 条边未闭合）",
    "Faces sewn: %d": "已缝合面数：%d",
    "Closed - this is a solid.": "已闭合，这是实体。",
    "Joined at %.4f mm.": "已在 %.4f mm 处接合。",
    "%d edge(s) still open.": "仍有 %d 条边未闭合。",
    "Rotate %d bodies by %.1f°": "将 %d 个实体旋转 %.1f°",
    "Rotate %d bodies by X %.2f° Y %.2f° Z %.2f° around centroid":
        "绕形心旋转 %d 个实体 X %.2f° Y %.2f° Z %.2f°",
    "Rotate %d Bodies": "旋转 %d 个实体",
    "Rotate (": "旋转（",
    " bodies)": " 个实体）",
    "Scale (": "缩放（",
    "Scale (%% of current)": "缩放（当前尺寸的%%）",
    "Scale (target %s)": "缩放（目标 %s）",
    "Rebuilding history - step %d of %d": "正在重建历史：第 %d 步，共 %d 步",

    # ── Selection / step readouts ────────────────────────────────────────
    "%d bodies selected": "已选 %d 个实体",
    "%zu bodies": "%zu 个实体",
    "ID: %d": "ID：%d",
    " [disabled]": "[已禁用]",
    " (frozen)": "（已冻结）",
    "Step %d/%d": "步骤 %d/%d",
    "Total: %s": "合计：%s",
    "Disable": "禁用",
    "Enable": "启用",
    "(unknown time)": "（时间未知）",
    "Next auto-save in ~%d:%02d": "距下次自动保存约 %d:%02d",
    "version %s": "版本 %s",
    "Restore all": "全部恢复",

    # ── Boolean / Mate readouts ──────────────────────────────────────────
    "(cutter)": "（切除体）",
    "(keep)": "（保留体）",
    "Mate": "配合",
    "%s (reference lost)": "%s（引用已丢失）",
    "%s: body %d -> body %d": "%s：实体 %d → 实体 %d",

    # ── Viewport dimension / gizmo captions ──────────────────────────────
    "%.1f (drag)": "%.1f（可拖动）",
    "Angle Snap (%d°)": "角度捕捉 (%d°)",
    "R %s · %d-gon": "R %s · %d 边形",
    "X %.0f%%  Y %.0f%%  Z %.0f%%": "X %.0f%%  Y %.0f%%  Z %.0f%%",
    "(controls geometry)": "（驱动几何）",
    "(measures only)": "（仅测量）",
    "%s-drag": "%s 拖动",

    # ── Patch fit dialog: the surface-matching solver ────────────────────
    "Advanced": "高级",
    "Gap tolerance (mm)": "间隙容差 (mm)",
    "Degree of the surface the fit starts from. Higher lets it bulge more "
    "freely - and invent waves the boundary never asked for.":
        "拟合所依据的曲面次数。次数越高鼓包越自由，也可能生成边界本身根本没有的波纹。",
    "Sample points per boundary curve. More follows the rim more closely; "
    "fewer smooths it out.":
        "每条边界曲线的采样点数。越多越贴合边缘轮廓；越少越趋于平滑。",
    "Stiffness": "刚度",
    "Tangency tolerance (deg)": "相切容差 (°)",
    "Curvature tolerance": "曲率容差",
    "Iterations": "迭代次数",
    "Max degree": "最大次数",
    "Max segments": "最大段数",
    "Anisotropic": "各向异性",
    "Let the fit stretch differently along and across the patch. Helps on "
    "long thin openings.":
        "让拟合沿补丁纵向与横向采用不同的伸缩比例。对细长开口尤其有效。",
    "Continuity": "连续性",
    "Continuity: %s": "连续性：%s",
    "Gap %.4f mm  -  tangency %.2f deg": "间隙 %.4f mm  -  相切 %.2f°",
    "Fit: gap %.4f mm, tangency %.2f deg": "拟合：间隙 %.4f mm，相切 %.2f°",
    "No surface fits these edges.": "没有曲面能贴合这些边。",
    "No tangent surface exists here - the faces\naround the opening stand "
    "square to it.\nThe hole is filled, held in position only.":
        "此处不存在相切曲面：开口周围的面与它成直角。\n孔洞仅被填充并保持原位。",

    # ── Sew results ─────────────────────────────────────────────────────
    "Sewn into the body.": "已缝入实体。",
    "Boundary edges: %d": "边界边：%d",
    "%d edge(s) have no neighbouring face, so\nthey only hold position.":
        "%d 条边没有相邻面，仅起定位作用。",
    "Standalone surface (the edges\ndon't bound one body).":
        "独立曲面（这些边不围成同一个实体）。",
    "%d picked support face(s)": "已选 %d 个支撑面",

    # ── Modal titles ─────────────────────────────────────────────────────
    "About Materializr": "关于 Materializr",
    "Check for Updates": "检查更新",
    "Cutting thread…": "正在切削螺纹…",
    "Re-cutting thread…": "正在重新切削螺纹…",
    "Discard sketch?": "放弃草图？",
    "Import Parts": "导入零件",
    "Recover Project?": "恢复项目？",
    "Recover Sketch?": "恢复草图？",
    "Small screen": "小屏幕",
    "Unsaved Changes": "未保存的更改",

    # ── Items panel / file actions ───────────────────────────────────────
    "New Folder": "新建文件夹",
    "Send to Open Project": "发送到“打开的项目”",
    "Offset created - 1 new element": "已创建偏置，新增 1 个元素",
    "Offset created - %d new elements": "已创建偏置，新增 %d 个元素",
    "Offset a chain: click an edge to take the whole connected run, then "
    "move the cursor or type a distance. Which side you are on picks the "
    "direction.":
        "偏置链：点按一条边即可选中整条相连的边链，然后移动光标或直接输入距离。偏置方向取决于你位于哪一侧。",
    "PDF: %d page(s)  (%d x %d, %s)": "PDF：%d 页(s)（%d x %d，%s）",
    "%d x %d px  ·  %s x %s": "%d x %d 像素  ·  %s x %s",

    # ── Layout rails: tooltips ───────────────────────────────────────────
    "Hide the model tree": "隐藏模型树",
    "Hide the history timeline": "隐藏历史时间线",
    "Show the history timeline": "显示历史时间线",
    "Snap ON - tap for step / toggle": "捕捉已开：点按设置步长／切换",
    "Snap off - tap for step / toggle": "捕捉已关：点按设置步长／切换",
    "Focus: hide the side panel (tap again for viewport only)":
        "专注模式：隐藏侧边面板（再点一次仅显示视口）",
    "Focus: hide the tool rail too (viewport only)":
        "专注模式：同时隐藏工具栏（仅显示视口）",
    "Bring the panels back": "恢复面板",
    "New Sketch": "新建草图",
    "Sketch on selection": "在所选面上画草图",

    # ── Face-region cycle modes ──────────────────────────────────────────
    "all": "全部",
    "loops": "环",
    "islands": "孤岛",
    "Local (rebuild neighbours)": "局部（重建相邻面）",

    # ── Tool help text ───────────────────────────────────────────────────
    "Repair": "修复",
    "Import an aerofoil section (Selig or Lednicer coordinates, e.g. from "
    "airfoiltools.com): pick the file, set the chord in the popup, click to "
    "place the leading edge. Stack sections on planes and Loft for a wing.":
        "导入翼型剖面（Selig 或 Lednicer 坐标，例如来自 airfoiltools.com）："
        "选择文件，在弹窗中设置弦长，然后点击放置前缘。"
        "把多个剖面分别放在平面上，再用放样生成机翼。",
    "Spline, Polygon, Trim, Offset) are on the toolbar only.":
        "样条、多边形、修剪、偏置）仅在工具栏上。",

    # ── Settings: autosave ───────────────────────────────────────────────
    "Autosave on close": "关闭时自动保存",
    "Saves the project automatically when you close it or quit, once it has "
    "been saved to a file at least once - no prompt needed. Crash recovery is "
    "separate and always on.":
        "项目至少保存到文件一次后，在关闭或退出时自动保存，无需任何提示。"
        "崩溃恢复是另一套机制，始终开启。",

    # ── Mesh-trace refusals (a pre-existing tr() covers only the first of
    #    two adjacent literals, so only the first half has a row) ─────────
    "No closed cross-section here - move the plane through the mesh first.":
        "此处没有封闭截面，请让平面穿过该部分再试。",
    "No closed silhouette here - the mesh may be open/non-manifold from this direction.":
        "此处没有封闭轮廓，网格可能不封闭或非流形。",
}

# ── Batch 7b: the three interface layouts. The names are the Combo entries in
#    Settings > Appearance AND the card titles in the first-run layout picker.
ZH_TRANSLATIONS_7.update({
    "Classic": "经典",
    "Modern": "现代",
    "im-touch": "触控",
    "Desktop-style. A menu bar on top and every panel visible at once. Best with a mouse and keyboard.":
        "桌面风格：顶部是菜单栏，所有面板同时可见。适合鼠标和键盘操作。",
    "Clean and focused. A tool rail on the left, tabbed panels on the right, everything else out of the way. Great on any screen.":
        "简洁专注：左侧是工具栏，右侧是带标签页的面板，其余元素全部让开。在任何屏幕上都很合适。",
    "Built for fingers. A full-screen model with floating buttons and big touch targets. Made for tablets.":
        "为手指而做：全屏模型，浮动按钮，大号触控目标。为平板而生。",
})
