#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Chinese translations, part 6.

Two groups, both found by sweeping src/ for literal strings that reach ImGui
WITHOUT passing through materializr::tr():

  * window titles - every ImGui::Begin("Literal") is wrapped as
    tr("Literal###slug") so the visible half translates while the "###" id
    stays byte-identical and the saved .ini layout keeps matching;
  * the handful of widgets that genuinely forgot tr() (the AI overlay, the
    Mates panel, the provider/provider-key fields).

Titles that begin with "##" are deliberately absent: they have no visible
half, so there is nothing for a user to read.
"""

ZH_TRANSLATIONS_6 = {
    # ---- Window titles -------
    "2D Drawing": "二维绘图",
    "AI Assistant": "AI 助手",
    "Boundary Fill": "边界填充",
    "Calibrate Image Scale": "标定图像比例",
    "Flat Pattern": "展开图",
    "Import STL": "导入 STL",
    "Import SVG": "导入 SVG",
    "Lay Flat on Plane": "平铺到平面",
    "Materials": "材料",
    "Mesh Trace": "网格描摹",
    "Move Sketch": "移动草图",
    "Patch": "修补",
    "Pick more sketches": "选择更多草图",
    "Rotate Plane About Axis": "绕轴旋转平面",
    "Set Up Tracing Planes": "设置描摹平面",
    "Variables": "变量",
    "Versions": "版本",

    # ---- AI assistant overlay (src/plugins/AiAssistantPlugin.cpp) -------
    "Clear Chat": "清空对话",
    "AI: %s": "AI:%s",
    "Thinking... %.0fs": "思考中... %.0fs",
    "Stop": "停止",
    "Send": "发送",

    # ---- Mates panel (src/plugins/MatePlugin.cpp) -------
    "Mates": "配合",
    "Mates not applied: %s": "配合未生效:%s",
    "Type": "类型",
    "Flip": "翻转",
    "Suppress": "抑制",

    # ---- AI provider / key fields (Application_Dialogs.cpp) -------
    "Provider": "服务商",
    "API Key": "API 密钥",
    "Model": "模型",
    "Base URL": "基础 URL",
    "Test Connection": "测试连接",
    "Testing...": "测试中...",

    # ---- Delete: the Mates row button. Shared with ItemsPanel, which
    #      already translated it, so the key simply did not exist until now.
    "Delete": "删除",

    # ---- Runtime-COMPOSED strings. These are the ones that look translated
    #      but aren't: the label is a prefix glued to a number, so tr() has to
    #      wrap the PREFIX, and the trailing space is part of the key.
    "Version ": "版本 ",
    "Auto-save ": "自动保存 ",
    "Plane ": "平面 ",
    "Axis ": "轴 ",
    "Not an airfoil file: ": "不是翼型文件:",
    "Failed: ": "失败:",
    "Couldn't unfold that: ": "无法展开:",

    # ---- Welcome modal + About dialog ----
    "Welcome": "欢迎",
    "I already support - don't show this again": "我已支持 - 不要再显示",
    "Already a supporter? Restore purchase": "已经是支持者?恢复购买",
    "Contacting the App Store...": "正在连接 App Store...",
    "Support Materializr": "支持 Materializr",
    "Open-source parametric 3D CAD": "开源参数化 3D CAD",

    # ---- Landing page. The label is glued to an icon glyph at the call site
    #      (ICON_IC_FOLDER "  Open File..."), so tr() sees only the word half.
    "Open File...": "打开文件...",
    "Start an empty project": "新建一个空白项目",

    # ---- 交互参考面板 (Interactions)。这是两块并排的列：左边是操作名，
    #      右边是触发它的手势/按键，两列都是给人读的，所以都要翻。
    #      触屏和桌面两套措辞都在这里，因为 touchMode() 决定渲染哪一套。
    "1-finger drag": "单指拖动",
    "Pan": "平移",
    "2-finger drag": "双指拖动",
    "Zoom": "缩放",
    "Pinch": "捏合",
    "Scroll wheel": "滚轮",
    "Reset view": "重置视图",
    "View menu": "视图菜单",
    "Face": "面",
    "Tap": "轻点",
    "Context menu": "上下文菜单",
    "Long-press": "长按",
    "Body": "实体",
    "Hold \u25b8 Body": "长按 \u25b8 实体",
    "Add to sel.": "加入选择",
    "Add to selection": "加入选择",
    "Multi-Select": "多选",
    "Select face": "选择面",
    "Select body": "选择实体",
    "Click": "单击",
    "Double-click": "双击",
    "Select behind": "选择被遮挡部分",
    "Slow double-click": "慢速双击",
    "Ctrl+Click": "Ctrl+单击",
    "Draw": "绘制",
    "Tap or drag": "轻点或拖动",
    "Finish / cancel": "完成/取消",
    "Finish / Cancel": "完成/取消",
    "Buttons": "按钮",
    "Dimension": "标注",
    "Tap + type": "轻点后输入",
    "Type value + Enter": "输入数值 + Enter",
    "Enter / Esc": "Enter / Esc",
    "General": "常规",
    "Nav lock": "导航锁定",
    "Move toggle": "移动开关",
    "Undo / redo": "撤销/重做",
    "Undo / Redo": "撤销/重做",
    "Start": "开始",
    "Delete selected": "删除所选",
    "Del": "Del",
    "Move / Rotate / Scale": "移动/旋转/缩放",
    "Pick a base plane / face": "选取基准平面/面",
    "W / E / R": "W / E / R",
    "Ctrl+Z / Ctrl+Y": "Ctrl+Z / Ctrl+Y",
    "Tool: %s": "工具:%s",

    # ---- Items 树的分组标题（ImTouchLayout 传给 treeGroup）
    "Bodies": "实体",
    "Sketches": "草图",
    "Construction": "构造",
    "Materials": "材料",

    # ---- 状态栏。注意这些都是 printf 格式串：说明符的顺序和种类必须与
    #      英文原串完全一致，否则 tools/update_catalogue_zh.py 会直接报错退出。
    "Bodies: %d": "实体:%d",
    "Selection: %s (%d)": "选区:%s (%d)",
    "None": "无",
    "New project": "新工程",
}
