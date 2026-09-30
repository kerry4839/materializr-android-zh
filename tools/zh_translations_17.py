# -*- coding: utf-8 -*-
"""Batch 17 (Chinese): the User Guide, the toolbar tooltips, gesture labels.

The guide bodies stay whole sentences in one key. Breaking a paragraph into
per-sentence keys is the usual way to keep catalogue rows short, and it is
wrong here: Chinese word order is not English word order, so the sentences have
to be free to move relative to each other, and only the whole paragraph can do
that. It also means a translator reads prose instead of fragments.

Two conventions carried over from earlier batches:

* Latin notation stays Latin. Ctrl, Alt, Home, STEP, CNC, XY/XZ/YZ and the
  single 100% are identifiers and standards, not words to be rendered.
* 100%% is not a typo. The key must match the bytes at the call site, and the
  Move/Rotate/Scale paragraph in HelpPanel.cpp writes "1%%" - doubled because
  that text goes through a printf-style path.
"""

ZH_TRANSLATIONS_17 = {
    # ── User Guide: section headings ─────────────────────────────────────
    "Navigating the viewport": "视图导航",
    "Selecting things":       "选择对象",
    "Sketching":              "绘制草图",
    "Modelling from a sketch": "从草图建模",
    "Saving and autosave":    "保存与自动保存",

    # ── User Guide: section bodies ───────────────────────────────────────
    "Drag the middle mouse button to orbit, the right button to pan, and the "
    "scroll wheel to zoom. The cube in the top-right is a navigation "
    "widget: click a face for an orthographic view, click a corner dot for "
    "an isometric view, click an arrow for a 90° rotation, or drag the "
    "cube itself to free-orbit. Reset the camera with Home or "
    "View → Reset Camera.":
        "拖动鼠标中键旋转视图，右键平移，滚轮缩放。右上角的立方体是"
        "导航控件：点击某个面切换正交视图，点击角点切换等轴测"
        "视图，点击箭头旋转 90°，或直接拖动立方体自由旋转。"
        "按 Home 键或通过视图 → 重置视图 来重置相机。",

    "Single-click a face or edge to select it; Ctrl+click adds to the "
    "selection. Double-click selects the whole body. Drag in empty space "
    "to box-select multiple bodies at once. Esc clears or cancels the "
    "current operation.":
        "单击面或边即可选中；Ctrl+单击可加入选择。双击选中整个"
        "实体。在空白处拖动可一次框选多个实体。Esc 清除选择或取消"
        "当前操作。",

    "Pick Sketch on XY/XZ/YZ from the Tools panel (or Sketch on Face after "
    "selecting one), then choose a tool: Line, Rectangle, Circle, Arc, "
    "Spline, Polygon, Trim, or Offset. Type a numeric dimension while "
    "placing to lock the size. Offset takes a whole connected chain: click "
    "one edge and the run it belongs to is highlighted, then move the "
    "cursor (the side you are on picks the direction) or type the "
    "distance. Press D for the Dimension tool: click a line, circle, "
    "two points, or two lines (parallel = distance, angled = angle), "
    "click to place the label, then type the value. Switch to Select / "
    "Move to drag existing points and lines - double-click empties to "
    "select the whole sketch, then use Copy / Mirror / Rotate. Click "
    "Finish Sketch (or press Enter) to exit.":
        "在工具面板中选择「XY/XZ/YZ 平面草图」（选中一个面后"
        "则用「在面上画草图」），再挑选工具：直线、"
        "矩形、圆、圆弧、样条、多边形、修剪或偏移。放置时"
        "输入数值尺寸即可锁定大小。偏移针对整段相连的链："
        "点一条边后其所属的整条链会高亮，随后移动光标"
        "（光标在哪一侧就朝哪个方向偏移）或直接输入距离。"
        "按 D 键用「标注」工具：点直线、圆、两点或两条线"
        "（平行线取距离，成角线取角度），点击放置标签位置"
        "后输入数值。切到「选择/移动」即可拖动已有的点和线；"
        "在空白处双击选中整张草图，然后用复制/镜像/旋转。"
        "按「完成草图」（或回车）退出。",

    "With a sketch region or face selected, click Extrude or Push/Pull. "
    "An arrow gizmo appears on the face - drag it for live preview, or "
    "type a value in the popup. Negative values cut into the body. After "
    "you have a body, use Fillet or Chamfer on edges (drag the handle "
    "outward to grow the radius/distance) and Mirror / Pattern from the "
    "tools sidebar.":
        "选中草图区域或面后，点击「拉伸」或「推/拉」。面上会"
        "出现一个箭头操纵器，拖动它可实时预览，或在弹窗"
        "中输入数值。负值表示向实体内部切除。有了实体之后，"
        "用「圆角」或「倒角」处理边（向外拖动手柄以增大"
        "半径/距离），以及工具栏中的「镜像 / 阵列」。",

    "Select a body (or several) and pick the Move, Rotate or Scale gizmo "
    "from the toolbar. Drag a handle to transform. Rotate snaps softly to "
    "45°; Scale snaps to 1%% and shows a live readout. Scale has a "
    "side panel for typed per-axis values and a uniform-scale checkbox.":
        "选中一个（或多个）实体，然后在工具栏中挑选「移动」"
        "「旋转」或「缩放」操纵器。拖动手柄即可变换。旋转会"
        "柔和地吸附到 45°；缩放吸附到 1%% 并显示实时读数。"
        "缩放还有一个侧面板，可逐轴输入数值，并带「等比缩放」"
        "复选框。",

    "Every modelling step goes into History on the right. Click a step to "
    "edit its parameters in Properties; right-click for Delete, Disable, "
    "Set Breakpoint, or Edit. Click a fillet/chamfer face in the viewport "
    "to jump straight to its history step. Reloaded projects show "
    "steps marked (reloaded) - undo/redo still walks through them but "
    "their parameters can't be re-edited.":
        "每一个建模步骤都会进入右侧的「历史」。点击某一步可"
        "在「属性」中编辑其参数；右键可删除、禁用、设置断点"
        "或编辑。在视口中点击一个带圆角/倒角的面可直接跳到"
        "对应的历史步骤。重新载入的项目会把步骤标记为"
        "「（已重新载入）」；撤销/重做仍会经过这些步骤，"
        "但它们的参数不能再编辑。",

    "Ctrl+S saves; the project includes bodies, colours, sketches and the "
    "operation history. Settings → Autosave enables a periodic re-save "
    "once the project has a file on disk. Application preferences "
    "(theme, mouse buttons, orbit mode) are stored separately under your "
    "user config directory so they follow you between projects.":
        "Ctrl+S 保存；工程包含实体、颜色、草图以及操作历史。"
        "设置 → 自动保存 会在工程写入磁盘后启用定期重新"
        "保存。应用程序偏好（主题、鼠标按键、旋转模式）单独"
        "存放在用户配置目录中，因此在各工程之间也能沿用。",

    # ── Toolbar tooltips ─────────────────────────────────────────────────
    "Start a new sketch lying on the picked face.":
        "在所选面上新建一张草图。",
    "Drag the face along its normal to extrude (+) or cut (−) into the body.":
        "沿面的法线方向拖动该面可拉伸（+）或向实体内部切除（−）。",
    "Make a NEW body by extruding this face's silhouette (source body unchanged).":
        "将该面的轮廓拉伸可生成一个全新实体（源实体保持不变）。",
    "Hollow the body, removing the picked face. Wall thickness in the popup.":
        "抽空该实体并移除所选的面。壁厚在弹窗中设置。",
    # Only the "100%%" spelling, which is the one Toolbar.cpp:998 now uses; the
    # bare-% row it replaced is removed by catalogue_entries_17.REMOVE, so a
    # translation for it here would have nothing to attach to.
    "Re-slope the side walls toward a scaled copy of this face - under "
    "100%% tapers it in, over 100%% flares it out. Blend length in the popup.":
        "按该面的等比缩放副本重新调整侧壁倾斜度：低于 100%% 时收窄，"
        "超过 100%% 时外扩。过渡长度在弹窗中设置。",
    "Draft angle for moulding or printing: tilt the picked face(s) by an "
    "angle about a fixed neutral plane. Unlike Rotate it takes SEVERAL "
    "faces at one angle (all four walls of a box) and works on cylindrical "
    "and conical faces too - a cylinder drafts into a cone.":
        "用于开模或打印的脱模角：让所选的面绕固定的中间平面倾斜一定"
        "角度。与「旋转」不同，它可以对多个面施加同一角度（例如"
        "长方体的四个侧面），对圆柱面和锥面同样有效：圆柱"
        "会脱模成圆锥。",
    "Project a sketch onto this face along the sketch's normal, then "
    "engrave (cut in) or emboss (raise out) to a depth - wrap a logo "
    "or text onto a cylinder. Sketch, mode and depth in the popup; "
    "click sketch regions in the viewport to project only those.":
        "沿草图法线把草图投影到该面上，然后按给定深度雕刻（切入）或浮雕"
        "（凸起），可把商标或文字包到圆柱面上。草图、模式和"
        "深度在弹窗中设置；在视口中点击草图区域可只投影所选区域。",
    "Resize a cylindrical hole / pin to an exact diameter.":
        "把圆柱孔或圆柱销调整为精确直径。",
    "Cut a helical screw thread into the picked cylindrical face - "
    "external on a boss, internal in a hole. Pitch / depth / handedness "
    "in the popup.":
        "在所选圆柱面上切出螺旋螺纹：外螺纹用于凸台，内螺纹"
        "用于孔。螺距、深度和旋向在弹窗中设置。",
    "Flatten the SELECTED faces into a 2D pattern (cut + fold lines) for a "
    "laser/CNC/printed template. Pick the faces of one panel (e.g. a skin) "
    "- unfolding a whole closed body rarely makes sense.":
        "把「所选的面」展平为二维展开图（含切割线和折线），用作"
        "激光/CNC/打印样板。请选择同一个面板的面（例如蒙皮）；把"
        "整个封闭实体展平通常没有意义。",
    "Delete the picked face(s) and heal the surrounding faces back "
    "together - take a baked fillet/chamfer back to a sharp edge so "
    "it can be re-applied, or clean a round/hole off an imported part.":
        "删除所选的面并把周边面重新缝合，可把已烘焙的圆角/倒角"
        "恢复成锐边以便重新施加，也可清除导入零件上的圆角或孔。",
    "Merge faces that are really one surface into one face. Imported "
    "STEP parts arrive with flat surfaces split into pieces - the seam "
    "lines across an otherwise flat face, which also confuse Unfold and "
    "sketch-on-face.":
        "把实际上是同一张曲面的面合并为一张面。导入的STEP 零件"
        "常常把平面拆成若干块，那些横穿平坦面的接缝线也会干扰"
        "「展开」和「在面上画草图」。",

    # ── Sketch toolbar labels ────────────────────────────────────────────
    "Select / Move": "选择/移动",
    "Trim":         "修剪",

    # ── Touch-gesture reference table ────────────────────────────────────
    "Alt+Drag":          "Alt+拖动",
    "Drag empty space":  "拖动空白区域",
    "Box-select":        "框选",
}