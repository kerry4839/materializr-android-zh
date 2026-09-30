# 更新日志

本文件只记录**汉化分支**的改动，不重复上游的发布说明（上游发布说明见
[`docs/changelog.md`](docs/changelog.md)）。

格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)。本项目使用语义化版本号，
并以 `-zh.N` 后缀标记纯本地化版本。

## [v1.7.1-zh.1] - 2026-10-01

基线：上游 Materializr 1.7.1。

### 新增

- 基于项目自带 C++ i18n 机制，完成安卓应用全部用户可见界面的简体中文（zh-CN）翻译。
  覆盖范围：工具栏、基元菜单、各操作弹窗、实体与草图属性面板、历史步骤名称与菜单、
  触摸导航叠层读数、快捷键面板、帮助面板用户指南的全部章节标题与长段落正文、
  引导教程，以及散落各处的工具提示与状态提示。
- `tools/` 下的批量灌入工具：`add_catalogue_entries.py`（改为按内容比对，
  译文修正不需要再维护删除清单）、`update_catalogue_zh.py`，
  以及分批次的 `catalogue_entries_*.py` / `zh_translations_*.py` 数据文件。
- `tools/check_catalogue_regressions.py`、`tools/make_upstream_fingerprints.py`
  与 `tools/upstream_fingerprints_1.7.1.txt`：为上游每个键记录一个哈希，
  使上游自带的五种语言可被验证为「未被改动」，灌入工具也会拒绝刷新这些行。
  批次文件里一条六语言词条，肉眼与「新键」毫无区别，归属只能落成数据。
- 中英双语的 `README.md` / `README.zh-CN.md`、本更新日志，
  以及原样保留的上游英文 README（`docs/upstream-README.md`）。

### 变更

- `tools/i18n_catalogue.py` 与其产物 `src/i18n_catalogue.h`：中文列补齐到 1749 条，
  与其余五种内置语言对齐。es、pt、fr、de、it 五列原样保留，没有任何一条回退成英文。
- 无参数的译文改为作为实参传入，不再直接当 printf 格式串使用
  （`SetTooltip("%s", tr(...))`），在 `-Werror=format-security` 下也能干净通过编译。
- 27 条中文译文去掉项目禁止的破折号，按分句关系改用逗号、冒号或分号；
  另有 2 个 pt/it 单元（属于本仓库新增的行）同样修正。
  对应关系记录在 `tools/strip_zh_em_dashes.py`。

### 修复

- `add_catalogue_entries.py` 在刷新其余五列时可能把该行的中文单元清空，
  并且拿整行去和五元组比较，导致每次运行都报几百条虚假刷新。
  现在中文单元会被带过去，只比较自己负责的列，
  并且一旦有已翻译的行丢掉中文就拒绝写入。
- 上游 54 个（五种语言中的）单元曾被批次提案覆盖，现已逐字还原为上游 v1.7.1。

### 校验结果

九道门禁全部 exit 0：

- `i18n_extract.py`：444 个源文件，0 条未翻译。
- `check_specifiers.py`：1749 keys、282 个格式串、0 处不匹配。
- `check_tr_calls.py`、`check_tr_coverage.py`、`check_tr_arguments.py`、
  `check_ui_string_arrays.py`、`check_op_names.py`：均为 0 处问题
  （39 个操作名、92 处 helper 调用）。
- `check_catalogue_regressions.py`：1192 个上游键，es/pt/fr/de/it 全部未变。
- `no_em_dashes.py`：无破折号，明文与 `\xNN` 转义都查。
- 构建：NDK CMake 配置与编译产出 `libmain.so`（U+2014 字节数为 0，
  每条修正后的译文各出现一次），随后 `./gradlew assembleDebug`。
- 真机：安装到 Android 10 平板（arm64-v8a，2000x1200 横屏），
  逐屏走查欢迎页、项目列表、历史记录面板、基元菜单与各类空状态界面。

### 已知缺口

- `ShortcutsPanel` 的 `Binding{keys, action}` 表尚未汉化。该结构体初始化数组的形态
  目前不在 `check_ui_string_arrays.py` 的覆盖范围内。

### 说明

- 非官方社区本地化版本。未改动建模内核、几何算法、交互逻辑与文件格式。
- 本仓库只收录汉化后的代码，不含上游提交历史；如需对比，请与上游 `v1.7.1` 标签比对。