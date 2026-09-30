#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""One-off: remove every em dash this branch introduced. Kept as the record.

The project rule (tools/no_em_dashes.py) bans U+2014 everywhere, and upstream
v1.7.1 has none. Every hit was therefore ours: 27 zh cells of rows this
localization added, plus two pt/it cells of one row it added too.

Chinese typography does not use the em dash anyway, so each replacement picks
the punctuation that fits the clause - comma for a continuation, colon for a
consequence, semicolon where two independent clauses meet.

    python3 tools/strip_zh_em_dashes.py

It is a no-op on a clean tree, and is kept in the tree for one reason: the
mapping below is the only written account of which punctuation was chosen for
which sentence, and a reviewer comparing this branch against upstream will want
to see the before text. Every other change is already in zh_translations_*.py.

The em dash is spelled {em} below and expanded at run time, because
tools/no_em_dashes.py reads every tracked file - including this one - and a
mapping that failed its own gate would have to be excluded from it by hand.
tools/no_em_dashes.py does the same thing with chr(0x2014), for the same
reason, and says so in its own comment.

{em} matches a RUN of one or more em dashes with optional spaces either side,
because the text being fixed uses both spellings: "A - B" and "A）- B" with a
double dash. Writing the run into the pattern rather than counting dashes by
hand is the difference between a mapping that still works after a refactor and
one that silently matches nothing - and silently matching nothing looks exactly
like a clean tree, which is the worst failure mode for a repair tool.

Files touched: tools/zh_translations_*.py, tools/catalogue_entries_*.py and
tools/i18n_catalogue.py. Afterwards run tools/update_catalogue_zh.py to push the
corrected zh column into the catalogue and regenerate src/i18n_catalogue.h.
"""
import io
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                    ".."))

EM = chr(0x2014)

# (old, new) with the em dash written as {em}.
ZH = [
    ("请输入圆弧两端之间的直线距离并按回车 {em} 或者直接点击端点。",
     "请输入圆弧两端之间的直线距离并按回车，也可以直接点击端点。"),
    ("此处没有封闭轮廓 {em} 网格可能不封闭或非流形。",
     "此处没有封闭轮廓，网格可能不封闭或非流形。"),
    ("此处没有封闭截面 {em} 请让平面穿过该部分再试。",
     "此处没有封闭截面，请让平面穿过该部分再试。"),
    ("已闭合 {em} 这是实体。",
     "已闭合，这是实体。"),
    ("正在重建历史 {em} 第 %d 步，共 %d 步",
     "正在重建历史：第 %d 步，共 %d 步"),
    ("次数越高鼓包越自由 {em} 也可能生成边界本身根本没有的波纹。",
     "次数越高鼓包越自由，也可能生成边界本身根本没有的波纹。"),
    ("此处不存在相切曲面 {em} 开口周围的面与它成直角。",
     "此处不存在相切曲面：开口周围的面与它成直角。"),
    ("已创建偏置 {em} 新增 1 个元素",
     "已创建偏置，新增 1 个元素"),
    ("已创建偏置 {em} 新增 %d 个元素",
     "已创建偏置，新增 %d 个元素"),
    ("捕捉已开 {em} 点按设置步长／切换",
     "捕捉已开：点按设置步长／切换"),
    ("捕捉已关 {em} 点按设置步长／切换",
     "捕捉已关：点按设置步长／切换"),
    ("而这是一个导入的网格 {em} 属于参考实体。",
     "而这是一个导入的网格，它属于参考实体。"),
    ("而所选实体中有一个是导入的网格 {em} 属于参考实体。",
     "而所选实体中有一个是导入的网格，它属于参考实体。"),
    ("在实体上已经找不到这些面了 {em} 它们可能已经被合并了。",
     "在实体上已经找不到这些面了，它们可能已经被合并了。"),
    ("无法根据这项草图改动更新模型 {em} 下游特征%s 无法跟随它",
     "无法根据这项草图改动更新模型：下游特征%s 无法跟随它"),
    ("不会产生任何变化 {em} 请改为倾斜它",
     "不会产生任何变化：请改为倾斜它"),
    ("并且只重建了与它相接的面 {em} 实体的其余部分未受影响。",
     "并且只重建了与它相接的面，实体的其余部分未受影响。"),
    ("BREP 文件过大（> 512 MB）{em}拒绝加载",
     "BREP 文件过大（> 512 MB）：拒绝加载"),
    ("是同一个实体 {em} 请另选一个。",
     "是同一个实体，请另选一个。"),
    ("单个点没有尺寸 {em} 请再选一个实体。",
     "单个点没有尺寸，请再选一个实体。"),
    ("没有可供缝合的目标 {em} 补丁会作为独立曲面落下。",
     "没有可供缝合的目标，补丁会作为独立曲面落下。"),
    ("因此会作为独立曲面落下 {em} 拟合边可能没有到达边界。",
     "因此会作为独立曲面落下，拟合边可能没有到达边界。"),
    ("将光标悬停在直线、圆弧或圆上 {em} 整条相连的链都会高亮。",
     "将光标悬停在直线、圆弧或圆上，整条相连的链都会高亮。"),
    ("开启：只重建与该面相接的面 {em} 其余部分保持不动。",
     "开启：只重建与该面相接的面，其余部分保持不动。"),
    ("标注 {em} 点击一个实体进行标注。",
     "标注：点击一个实体进行标注。"),
    ("标注 {em} 点击第二个实体，或点击空白处直接放置。",
     "标注：点击第二个实体，或点击空白处直接放置。"),
    ("标注 {em} 点击以放置标签。",
     "标注：点击以放置标签。"),
    # Comment in batch 12, not user-visible, but the gate reads every line.
    ("（不是 trFormat {em} 这些不是格式串）",
     "（不是 trFormat，这些不是格式串）"),
]

# One row of six-language data this branch added; its pt and it cells used the
# em dash. A semicolon keeps the two clauses apart without the dash.
OTHER = [
    ("apenas as faces que tocam esta são reconstruídas {em} "
     "tudo o resto fica no lugar.",
     "apenas as faces que tocam esta são reconstruídas; "
     "tudo o resto fica no lugar."),
    ("vengono ricostruite solo le facce che toccano questa {em} "
     "tutto il resto resta in posizione.",
     "vengono ricostruite solo le facce che toccano questa; "
     "tutto il resto resta in posizione."),
]


def expand(pairs):
    """(pattern, replacement) per pair; {em} becomes a run of em dashes.

    The pattern is a regex because the run's length and the spaces around it
    are not ours to remember: both spellings occur in the same file. The
    replacement is a plain string, which re.sub reads as literal - the '%' in
    "step %d" below must not be read as a group reference.
    """
    out = []
    for old, new in pairs:
        parts = re.split(r"\{em\}", old)
        pattern = r" *\u2014+ *".join(re.escape(p) for p in parts)
        out.append((re.compile(pattern), new.replace("{em}", EM)))
    return out


def patch(path, pairs):
    with io.open(path, encoding="utf-8") as f:
        text = f.read()
    hits = 0
    for pattern, new in pairs:
        text, n = pattern.subn(lambda _m, s=new: s, text)
        hits += n
    if hits:
        with io.open(path, "w", encoding="utf-8") as f:
            f.write(text)
    return hits


def main():
    zh, other = expand(ZH), expand(OTHER)
    tools = os.path.join(ROOT, "tools")
    total = 0
    for prefix, pairs in (("zh_translations", zh), ("catalogue_entries", other)):
        for name in sorted(os.listdir(tools)):
            if not name.startswith(prefix) or not name.endswith(".py"):
                continue
            n = patch(os.path.join(tools, name), pairs)
            if n:
                print("tools/%s: %d replacement(s)" % (name, n))
            total += n
    # The same cells live in the catalogue already; keep it byte-identical to
    # its batch files so add_catalogue_entries.py still matches on content.
    n = patch(os.path.join(tools, "i18n_catalogue.py"), other + zh)
    print("tools/i18n_catalogue.py: %d replacement(s)" % n)
    total += n
    print("total %d replacement(s)" % total)
    return 0


if __name__ == "__main__":
    sys.exit(main())