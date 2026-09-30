# -*- coding: utf-8 -*-
"""Batch 11 (Chinese). Pairs with catalogue_entries_11.py - see its docstring
for why tools/i18n_extract.py needed two separate fixes before it could see
these four strings at all.
"""

ZH_TRANSLATIONS_11 = {
    "Point the base URL at http://localhost:11434/v1 for Ollama, or LM Studio's local server address, to run without any cloud key.":
        "将基准 URL 指向 Ollama 的 http://localhost:11434/v1 ，或指向 LM Studio 本地服务器的地址，即可在不使用云端密钥的情况下运行。",

    "Set up your API key in Settings -> AI Assistant.":
        "请在「设置 -> AI 助手」中配置你的 API 密钥。",

    "Only Fasten is available: these bodies have no sketch-derived faces to align to. Primitive and imported bodies cannot carry face references.":
        "仅「紧固」可用：这些实体没有可对齐的、由草图生成的面。图元实体和导入的实体无法携带面引用。",

    "Only available with anchored faces: a rotation or flip recaptured from this body's current (already-rotated) position, instead of a real face's orientation, would silently compound on the next solve.":
        "仅在存在锚定面时可用：若从该实体当前（已旋转）的位置而非真实面的取向来重新捕捉旋转或翻转，误差会在下一次求解时悄悄累积。",
}
