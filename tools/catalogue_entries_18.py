# -*- coding: utf-8 -*-
"""Batch 18: the strings shape 4 of check_op_names.py found on its own.

Every key here reached the screen without going through tr() until this pass,
because it arrived through a LOCAL UI HELPER rather than an ImGui sink:

    tip(text)          - tooltip, passed straight to TextUnformatted
    colorPopup(title)  - its title param, popupId param is an identity
    section(title)     - ShortcutsPanel headings; the tableId param is not text
    guide(tag)         - the touch/mouse labels in the tutorial tour
    rotField(label)    - the axis letters

They are one batch because they share a cause, not because they share a
subject. The batch that fixed the call sites wrapped them in tr() and these are
the rows that wrapping now needs.

The axis letters X, Y and Z are in here on purpose, and map to themselves. They
are notation, so nothing is gained by translating the glyphs - but the keys
have to exist, because the call site goes through tr() and tr() hands back the
English unchanged on a miss. An axis label that is deliberately left alone
still needs a row saying so, otherwise the row is missing for a reason nobody
can read off the source.

'Touch:' and 'Mouse:' keep their colons in the source. The colon is punctuation
the layout does not care about, so a translation may move or drop it, and
Chinese uses the full-width '：' - the first row in this batch where the
punctuation genuinely changes shape between languages.
"""

# Nothing in this batch is invalidated: every key here was added by it, and the
# English still matches its call site. A corrected translation does not belong in
# REMOVE either - add_catalogue_entries.py compares each row's five non-zh
# columns against this file and rewrites the ones that differ. See the comment
# above REMOVE in catalogue_entries_17.py.
REMOVE = []


NEW = [
    # ── Axis notation: present so tr() resolves, unchanged in every language ─
    ("X", "X", "X", "X", "X", "X", "X"),
    ("Y", "Y", "Y", "Y", "Y", "Y", "Y"),
    ("Z", "Z", "Z", "Z", "Z", "Z", "Z"),

    # ── Nav overlay toggles (Application_Viewport.cpp) ─────────────────────
    # "Move:" and "Multi-Select:" are ON/OFF state readouts on the touch nav bar,
    # so the wording is a status line rather than a menu item. The On/Off halves
    # are kept as separate words in the Latin scripts because they sit at the end
    # of a short chip and a shorter word keeps the chip from wrapping.
    ("Move: On",
     "Mover: activado",
     "Mover: ligado",
     "Deplacer : active",
     "Verschieben: an",
     "Sposta: attivo",
     "移动:开"),
    ("Move: Off",
     "Mover: desactivado",
     "Mover: desligado",
     "Deplacer : inactif",
     "Verschieben: aus",
     "Sposta: disattivo",
     "移动:关"),
    ("Multi-Select: On",
     "Multi-seleccion: activada",
     "Multi-selecao: ligada",
     "Multi-selection : activee",
     "Mehrfachauswahl: an",
     "Multi-selezione: attiva",
     "多点选择:开"),
    ("Multi-Select: Off",
     "Multi-seleccion: desactivada",
     "Multi-selecao: desligada",
     "Multi-selection : inactivée",
     "Mehrfachauswahl: aus",
     "Multi-selezione: disattiva",
     "多点选择:关"),
    ("Cancel Shape",
     "Cancelar forma",
     "Cancelar forma",
     "Annuler la forme",
     "Form abbrechen",
     "Annulla forma",
     "取消图形"),

    # ── Model-tree and colour-picker tooltips (ImTouchLayout.cpp) ──────────
    ("Show the model tree (bodies, sketches, construction)",
     "Mostrar el arbol del modelo (solidos, bocetos, construccion)",
     "Mostrar a arvore do modelo (corpos, esbocos, construcao)",
     "Afficher l'arborescence du modele (solides, plans, construction)",
     "Baumstruktur einblenden (Koerper, Skizzen, Konstruktion)",
     "Mostra l'albero del modello (solidi, schizzi, costruzione)",
     "显示模型树（实体、草图、构造）"),
    ("Body colour",
     "Color del solido",
     "Cor do corpo",
     "Couleur du solide",
     "Koerperfarbe",
     "Colore del solido",
     "实体颜色"),
    ("Folder colour",
     "Color de carpeta",
     "Cor da pasta",
     "Couleur du dossier",
     "Ordnerfarbe",
     "Colore della cartella",
     "文件夹颜色"),

    # ── Sketch commit / discard tooltips ──────────────────────────────────
    # Paired with the existing "Finish the current shape, keeping the points
    # placed" and "Cancel the in-progress shape" rows, so the on/off halves of
    # the same toggle carry matching wording in all six languages.
    ("Show the model tree",
     "Mostrar el arbol del modelo",
     "Mostrar a arvore do modelo",
     "Afficher l'arborescence du modele",
     "Baumstruktur einblenden",
     "Mostra l'albero del modello",
     "显示模型树"),
    ("Throw the sketch away and leave (asks to confirm)",
     "Descartar el boceto y salir (pide confirmacion)",
     "Descartar o esboco e sair (pede confirmacao)",
     "Jeter le plan et quitter (demande confirmation)",
     "Skizze verwerfen und verlassen (fragt nach)",
     "Scarta lo schizzo ed esci (chiede conferma)",
     "丢弃草图并退出（会要求确认）"),
    ("Finish the sketch and leave sketch mode",
     "Finalizar el boceto y salir del modo boceto",
     "Finalizar o esboco e sair do modo esboco",
     "Terminer le plan et quitter le mode plan",
     "Skizze fertigstellen und Skizzenmodus verlassen",
     "Termina lo schizzo ed esci dalla modalita schizzo",
     "完成草图并退出草图模式"),
    ("Leave sketch mode, keeping the sketch",
     "Salir del modo boceto y conservarlo",
     "Sair do modo esboco e mantê-lo",
     "Quitter le mode plan en gardant le plan",
     "Skizzenmodus verlassen und Skizze behalten",
     "Esci dalla modalita schizzo mantenendo lo schizzo",
     "退出草图模式并保留草图"),

    # ── Tutorial tour platform labels (TutorialPlugin.cpp) ────────────────
    ("Touch:", "Tocar:", "Toque:", "Toucher :", "Tippen:", "Tocca:", "触摸："),
    ("Mouse:", "Raton:", "Mouse:", "Souris :", "Maus:", "Mouse:", "鼠标："),

    # ── Shortcut-table section headings (ShortcutsPanel.cpp) ──────────────
    ("While a tool is running",
     "Mientras se ejecuta una herramienta",
     "Enquanto uma ferramenta esta em execucao",
     "Pendant l'execution d'un outil",
     "Waehrend ein Werkzeug laeuft",
     "Mentre uno strumento e in esecuzione",
     "工具运行时"),
    ("In a sketch",
     "En un boceto",
     "Em um esboco",
     "Dans un plan",
     "In einer Skizze",
     "In uno schizzo",
     "在草图中"),

    # ── The other half of two pairs the earlier batches left open ─────────
    # 'Mouse' and 'In a sketch' were already in CAT; 'Touch' and this one were
    # not, so a table whose two columns are the same kind of thing had one
    # heading translated and the other not. Fixed as pairs rather than as
    # individual strings, because a mismatched pair is invisible in a diff of
    # one line and obvious on screen.
    ("Touch",
     "Tactil",
     "Toque",
     "Tactile",
     "Touch",
     "Tattile",
     "触摸"),
    ("Cancel the in-progress shape",
     "Cancelar la forma en curso",
     "Cancelar a forma em andamento",
     "Annuler la forme en cours",
     "Form in Arbeit abbrechen",
     "Annulla la forma in corso",
     "取消正在绘制的图形"),
]


def zh_only(entries):
    """Strip the Chinese column, leaving the SIX the ingester expects.

    add_catalogue_entries.py writes six because the zh column is owned by
    tools/zh_translations*.py and applied afterwards by update_catalogue_zh.py.
    One column, one owner: two owners means one of them silently wins.
    """
    return [tuple(e[:6]) for e in entries]


NEW = zh_only(NEW)
