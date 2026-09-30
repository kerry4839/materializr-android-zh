# Materializr 安卓版 · 深度简体中文汉化版

<div align="center">

**[English](README.md) · 简体中文**

[Materializr](https://github.com/materializr-cad/materializr)（开源参数化三维 CAD 工具）
**安卓端**的深度简体中文（zh-CN）本地化版本。

在完全保留原版建模内核、几何算法、交互逻辑与文件格式的前提下，对应用界面做了系统性汉化。

上游的英文原始 README（含完整功能介绍）原样保留在
[`docs/upstream-README.md`](docs/upstream-README.md)。

</div>

---

> **本仓库是非官方的社区本地化版本。** 未对建模内核、几何算法、交互逻辑与文件格式
> 做任何改动，只替换用户可见的界面字符串。全部功能与品牌标识归 Materializr 原作者所有。
> 详见[致谢](#致谢)与[许可证](#许可证)。

## 关于本项目

Materializr 是一款自由、开源的参数化 CAD 建模工具，专注于流畅、直观的实体建模体验。

本仓库是该软件的**安卓（arm64-v8a）深度简体中文汉化版**，构建在项目自带的 C++ i18n 机制之上。
汉化覆盖几乎所有用户可见字符串：

| 区域 | 汉化内容 |
| --- | --- |
| 工具栏 | 主工具栏按钮、工具分组标题 |
| 基元菜单 | 长方体、圆柱、球体、圆锥、圆环等 |
| 弹窗 | 各操作弹窗的标题与正文 |
| 属性面板 | 实体/草图属性、颜色选择器 |
| 历史记录 | 建模步骤名称、编辑与断点菜单 |
| 导航叠层 | 触摸导航条上的状态读数（如「移动:开」「多点选择:关」） |
| 快捷键面板 | 章节标题与每一项按键说明 |
| 帮助面板 | 用户指南全部章节标题与长段落正文 |
| 教程 | 引导流程的标签与文案 |
| 工具提示 | 各处工具提示与状态提示 |

另外五种内置语言（es、pt、fr、de、it）**原样保留**，没有任何一条回退成英文。

## 特点

- **深度汉化**：不只是菜单，工具提示、参数说明、错误提示、历史步骤名、快捷键说明、教程长文本全部汉化。
- **非破坏性**：不修改建模内核、几何运算与文件格式，仅替换界面字符串，所有建模行为与原版完全一致。
- **可维护**：沿用项目自带的 `tr()` / i18n 机制，全部译文集中维护在词库中，便于审阅，也便于后续同步上游更新。
- **有据可查**：配套一整套 i18n 校验脚本，本仓库汉化部分全部通过校验（详见[质量校验](#质量校验)）。

## 安装

从 [Releases](RELEASES_URL) 页面下载 APK，装到 Android 设备上即可（需允许「安装未知来源应用」）。
强烈建议在平板上使用，应用本就是为大屏触控设计的。

自行构建：

```bash
cd android
./gradlew assembleDebug
```

产物位于 `android/app/build/outputs/apk/debug/app-debug.apk`。

## 使用

安装后直接启动，界面即为简体中文。语言切换逻辑沿用原版。

## 技术说明

汉化沿用项目原有的 C++ i18n 机制：**英文原串作为查表键**，`tr()` 按当前语言查表返回译文。这带来几条约定：

- 窗口与弹窗标题使用 `tr("VisibleTitle###StableID")` 形式，`###` 后的稳定 ID 不参与翻译。
- 需要插入参数的文案使用 `materializr::trFormat(fmt, args...)`。
- 译文键里的字面百分号写作 `%%`，以免被当作 printf 转换符。
- `tr()` 的返回值随后会被交给 printf 风格函数，所以无参数的译文不能直接当格式串，要包一层参数：

```cpp
ImGui::SetTooltip("%s", tr("Measure"));
```

### 本次汉化用到的工具链

`tools/` 目录下的脚本一部分来自上游的 i18n 工作流，一部分是本次汉化为灌入中文列而补充的批量工具：

| 脚本 | 作用 |
| --- | --- |
| `i18n_extract.py` | 从源码提取界面字符串并核对覆盖率 |
| `check_specifiers.py` | 校验各语言译文保留 printf 占位符及其顺序 |
| `check_tr_calls.py` | 检查 `tr()` 调用形态是否正确 |
| `check_tr_coverage.py` | 查找缺少词条的 `tr()` 调用点 |
| `check_tr_arguments.py` | 查找译后格式串里仍传英文实参的情况 |
| `check_ui_string_arrays.py` | 检查 UI 字符串数组是否全部有词条 |
| `check_op_names.py` | 检查操作名与本地 UI helper 传递的字符串 |
| `no_em_dashes.py` | 排版门禁：不允许出现破折号（明文与 `\xNN` 转义都查） |
| `check_catalogue_regressions.py` | 校验上游自带的五种语言与上游 v1.7.1 逐字节一致 |
| `add_catalogue_entries.py` | 词库灌入工具（按内容比对刷新译文，且不碰上游的行） |
| `update_catalogue_zh.py` | 灌入中文列并重新生成 `src/i18n_catalogue.h` |
| `make_upstream_fingerprints.py` | 从上游检出重新生成 `upstream_fingerprints_1.7.1.txt` |
| `strip_zh_em_dashes.py` | 一次性脚本，保留下来作为 27 条去破折号改动的记录 |
| `catalogue_entries_*.py` | 分批次的六语言词条 |
| `zh_translations_*.py` | 分批次的中文译文 |

`upstream_fingerprints_1.7.1.txt` 为上游每个键记录一个哈希。本仓库只增加一种语言，
无权改动原本就有的五种；而批次文件里一条六语言词条，在外观上与「新键」毫无区别。
所以归属边界写成数据而不是靠自觉：灌入工具会跳过这些键，
`check_catalogue_regressions.py` 会在任何一个上游单元被改动时报错。

### 质量校验

在仓库根目录运行：

```bash
python3 tools/i18n_extract.py
python3 tools/check_specifiers.py
python3 tools/check_tr_calls.py
python3 tools/check_tr_coverage.py
python3 tools/check_tr_arguments.py
python3 tools/check_ui_string_arrays.py
python3 tools/check_op_names.py
python3 tools/check_catalogue_regressions.py
python3 tools/no_em_dashes.py
```

九项全部 exit 0。当前结果（版本 1.7.1，词库 1749 条 × 6 语言）：

| 检查项 | 结果 |
| --- | --- |
| `i18n_extract.py` | 444 个文件，0 条未翻译 |
| `check_specifiers.py` | 1749 keys / 282 个格式串 / 0 处不匹配 |
| `check_tr_calls.py` | 0 处可疑调用 |
| `check_tr_coverage.py` | 0 处缺词条 |
| `check_tr_arguments.py` | 0 处未翻译实参 |
| `check_ui_string_arrays.py` | 0 处缺词条 |
| `check_op_names.py` | 39 个操作名 / 92 处 helper 调用，0 处缺词条 |
| `check_catalogue_regressions.py` | 1192 个上游键，es/pt/fr/de/it 全部未变 |
| `no_em_dashes.py` | 无破折号 |

构建链路也做了端到端验证：NDK CMake 配置与编译（产出的 `libmain.so` 中 U+2014 字节数为 0，
每条修正后的译文各出现一次）、Gradle `assembleDebug`、侧载到 Android 10 真机平板，
并在设备上逐屏走查了欢迎页、项目列表、历史记录面板、基元菜单与各类空状态界面。

## 版本对应

| 项目 | 版本 |
| --- | --- |
| 上游 Materializr | 1.7.1 |
| 本汉化版 | `v1.7.1-zh.1` |

本仓库**只收录汉化后的代码**，不含上游的提交历史。要与上游对比，请把
[materializr-cad/materializr](https://github.com/materializr-cad/materializr) 的 `v1.7.1` 标签
与本分支比对。

上游的发布说明见 [`docs/changelog.md`](docs/changelog.md)。

## 致谢

- **[Materializr](https://github.com/materializr-cad/materializr)**：原项目，以及原作者与全体上游贡献者。
  英文原始 README 原样保留在 [`docs/upstream-README.md`](docs/upstream-README.md)，其中完整保留了
  R4stl1n、stevebushwa、TechHQUSA 等上游署名与社区致谢。
- **[OpenCode](https://opencode.ai/)**：本次本地化中的字符串提取、翻译、校验与打包流程借助了 AI 编码工具。

感谢 Materializr 原作者与贡献者构建了这个项目，并维护了一套让本地化成为可能的 i18n 机制。

## 许可证

Materializr 采用 GNU GPLv3，详见 [`LICENSE`](LICENSE)；另含 GPLv3 第 7 条下的附加许可，
覆盖应用商店分发与平台 SDK 链接，详见 [`LICENSE-EXCEPTIONS.md`](LICENSE-EXCEPTIONS.md)。
本汉化版沿用同样的许可条款，再分发时请遵守上游许可证的规定。

上游的 [`PRIVACY.md`](PRIVACY.md) 同样适用。