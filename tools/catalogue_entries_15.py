# -*- coding: utf-8 -*-
"""Batch 15: Operation::name() values and the labels callers pass them.

Operation::name() is a lookup KEY, not prose. It reaches serializeParams
("label=Rotate Plane"), the undo tooltip, the persisted history, and
OperationFactory's typeId matching - so it stays English at the source. The
translation belongs at the point it becomes visible, which for the touch
timeline is ImTouchLayout.cpp's step box.

That display site had no tr() at all, so the strip showed "Boolean",
"Sketch Edit", "Rotate Plane" and friends in English. Nothing could report it:

  * check_tr_coverage.py reads tr() CALL SITES; there was no call to read.
  * check_tr_arguments.py looks for literals inside format calls; name() is a
    return value in an inline header, not a literal at a call site.
  * check_ui_string_arrays.py looks for indexed arrays; this is 39 inline
    one-line overrides.
  * check_specifiers.py works on the catalogue and never touches source.

So the four shapes were not overlapping, they were ADJACENT - and this string
entered by a fifth path (a virtual method returning a literal). The same gap
existed in HistoryPanel.cpp, which papered over it with a heuristic: it
translated the leading run of letters in description(). That heuristic is why
"Box 10x10x10" was half-translated - "Box" became 长方体 but the dimensions
stayed, and for "Move hole 5 mm" the cut landed mid-word and matched nothing.
Now that description() returns a whole translated sentence, the heuristic has
nothing left to do; it is left in place because it still helps unlisted ops,
but the strip no longer depends on it.

The 11 keys below are the names and caller-supplied labels that had no row.
Not here: "Transform", which already exists (变换).
"""


NEW = [
    # ── Inline name() overrides with no catalogue row ──────────────────────
    ("Boolean",          "Booleano",        "Booleano",        "Booléen",        "Boolesch",        "Booleano"),
    ("Combine Sketches", "Combinar bocetos", "Combinar esboços", "Combiner les esquisses", "Skizzen kombinieren", "Combina schizzi"),
    ("Guided Loft",      "Loft guiado",     "Loft guiado",     "Loft guidé",     "Geführter Loft",  "Loft guidato"),
    ("Sew",              "Costura",         "Costura",         "Recoudre",       "Nähen",           "Cucitura"),
    ("Sketch Edit",      "Editar boceto",   "Editar esboço",   "Modifier le trait", "Skizze bearbeiten", "Modifica schizzo"),
    ("Sketch Transform", "Transformar boceto", "Transformar esboço", "Transformer le trait", "Skizze transformieren", "Trasforma schizzo"),
    ("Tweak Face",       "Ajustar cara",    "Ajustar face",    "Ajuster la face", "Fläche anpassen", "Modifica faccia"),

    # ── Labels the gizmo commits pass in as constructor arguments ──────────
    ("Rotate Plane", "Girar plano",   "Girar plano",   "Pivoter le plan",  "Ebene drehen",        "Ruota piano"),
    ("Move Plane",   "Mover plano",   "Mover plano",   "Déplacer le plan", "Ebene verschieben",  "Sposta piano"),
    ("Rotate Axis",  "Girar eje",     "Girar eixo",    "Pivoter l'axe",    "Achse drehen",        "Ruota asse"),
    ("Move Axis",    "Mover eje",     "Mover eixo",    "Déplacer l'axe",   "Achse verschieben",  "Sposta asse"),

    # ── Edge-op banner name. Feeds a "%s - Type value..." format, so it is a
    #    WORD the user reads, not an internal enum tag - hence all-caps
    #    preserved in the other five languages to keep the banner's shout. ──
    ("FILLET",  "REDONDEO", "ARREDONDO", "CONCAVE",    "RUNDUNG",   "SMUSSATO"),
    ("CHAMFER", "CHAFLAN",  "CHANFRO",    "CHANFREIN",  "FASE",      "SMUSSATURA"),

    # ── ItemsPanel folder context-menu labels ─────────────────────────────
    ("(root - no folder)",                    "(raíz - sin carpeta)",   "(raiz - sem pasta)",        "(racine - aucun dossier)",  "(Wurzel - kein Ordner)",  "(radice - nessuna cartella)"),
    ("(root - no folder) - all selected",     "(raíz - sin carpeta) - todo seleccionado", "(raiz - sem pasta) - tudo selecionado", "(racine - aucun dossier) - tout sélectionné", "(Wurzel - kein Ordner) - alle ausgewählt", "(radice - nessuna cartella) - tutto selezionato"),
    ("all selected",     "todo seleccionado", "tudo selecionado",   "tout sélectionné",  "alle ausgewählt",   "tutto selezionato"),
    ("New folder…",      "Nueva carpeta…",   "Nova pasta…",     "Nouveau dossier…", "Neuer Ordner…",    "Nuova cartella…"),
    ("New folder… (all selected)", "Nueva carpeta… (todo seleccionado)", "Nova pasta… (tudo selecionado)", "Nouveau dossier… (tout sélectionné)", "Neuer Ordner… (alle ausgewählt)", "Nuova cartella… (tutto selezionato)"),

    # ── Toolbar inference-level button ─────────────────────────────────────
    ("Inferences: Full",    "Inferencias: Completa",  "Inferências: Completa",  "Inférences : Complet",   "Ableitungen: Voll",       "Inferenze: Completa"),
    ("Inferences: Reduced", "Inferencias: Reducida",  "Inferências: Reduzida",  "Inférences : Réduites",  "Ableitungen: Reduziert",  "Inferenze: Ridotta"),
    ("Inferences: Off",     "Inferencias: Desactivadas", "Inferências: Desativadas", "Inférences : Désactivées", "Ableitungen: Aus",    "Inferenze: Disattivate"),
    ("Inferences: Max",     "Inferencias: Máxima",    "Inferências: Máxima",    "Inférences : Max",       "Ableitungen: Maximal",    "Inferenze: Massima"),

    # ── Sketch pattern popup title ─────────────────────────────────────────
    ("Sketch Linear Pattern",   "Patrón lineal de boceto",   "Padrão linear de esboço",   "Motif linéaire de trait",   "Lineares Skizzenmuster",  "Motivo lineare di schizzo"),
    ("Sketch Circular Pattern", "Patrón circular de boceto", "Padrão circular de esboço", "Motif circulaire de trait", "Kreisförmiges Skizzenmuster", "Motivo circolare di schizzo"),

    # ── Pattern / Patch continuity words, already tr()-wrapped at the call
    #    site but never given a row - the wrapper existed, the value did not.
    ("Linear",   "Lineal",    "Linear",    "Linéaire",  "Linear",    "Lineare"),
    ("Radial",   "Radial",    "Radial",    "Radial",    "Radial",    "Radiale"),
    ("curvature","curvatura", "curvatura", "courbure",  "Krümmung",  "curvatura"),
    ("tangent",  "tangente",  "tangente",  "tangente",  "tangente",  "tangente"),
    ("position", "posición", "posição",  "position",  "Position",  "posizione"),
    # ── Sketch constraint type labels (PropertiesPanel.cpp). The maths symbols
    #    ride along inside the key because check_specifiers.py compares whole
    #    strings; Ø and ⊥ are notation and stay Latin in every language. ─────
    ("\u00d8 (diameter)", "Di\u00e1metro (\u00d8)", "Di\u00e2metro (\u00d8)",
     "Diam\u00e8tre (\u00d8)", "Durchmesser (\u00d8)", "Diametro (\u00d8)"),
    ("Dist \u22a5", "Dist \u22a5", "Dist \u22a5", "Dist \u22a5", "Dist \u22a5", "Dist \u22a5"),
    ("Gap", "Holgura", "Folga", "Jeu", "Spalt", "Gioco"),
]
