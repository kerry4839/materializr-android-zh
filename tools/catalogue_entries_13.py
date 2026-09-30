# -*- coding: utf-8 -*-
"""Batch 13: the Combo item tables that tools/check_ui_string_arrays.py found.

These were `const char* x[] = {...}` handed straight to ImGui::Combo, whose
`items` parameter is `const char* const[]` - so the array WAS the translation
and there was nowhere to put tr(). materializr::trList() (new in i18n.h) builds
a translated vector at the call site instead, preserving order and length so the
combo's integer index still selects the right entry.

Deliberately NOT here, and why:
  * font FILENAMES and the Anthropic vendor name - data, not labels. They stay
    literal in the source and are listed in check_ui_string_arrays.py's NOT_UI
    with the reason.
  * BREP section names, SVG attribute names, dialog helper executables -
    strings matched literally against a file format or handed to execvp.

Month abbreviations are here as keys, but note trDate() does NOT use them as a
format string: the year/month/day ORDER differs per language, which a single key
with a fixed specifier order cannot express, so the order is picked in code and
only the month name is looked up.
"""

NEW = [
    # ── Thread cross-section profile. Two lists of the same five names exist
    #    (Application_Dialogs.cpp and ThreadOp.cpp); both read the catalogue, so
    #    they cannot drift apart. ────────────────────────────────────────────
    ("Standard (V)", "Estándar (V)", "Padrão (V)", "Standard (V)", "Standard (V)", "Standard (V)"),
    ("Trapezoidal (ACME)", "Trapezoidal (ACME)", "Trapezoidal (ACME)", "Trapézoïdal (ACME)", "Trapezförmig (ACME)", "Trapezoidale (ACME)"),
    ("Square", "Cuadrada", "Quadrada", "Carrée", "Quadratisch", "Quadrata"),
    ("Buttress", "De contrafuerte", "De contraforte", "À sifflet", "Schenkelgewinde", "A denti invertiti"),
    ("Rounded (print)", "Redondeada (impresión)", "Arredondada (impressão)", "Arrondie (impression)", "Gerundet (Druck)", "Arrotondata (stampa)"),

    # ── Patch continuity ───────────────────────────────────────────────────
    ("Position (C0)",  "Posición (C0)",  "Posição (C0)",  "Position (C0)",  "Position (C0)",  "Posizione (C0)"),
    ("Tangent (G1)",   "Tangente (G1)",  "Tangente (G1)",  "Tangente (G1)",  "Tangente (G1)",  "Tangente (G1)"),
    ("Curvature (G2)", "Curvatura (G2)", "Curvatura (G2)", "Courbure (G2)",  "Krümmung (G2)",  "Curvatura (G2)"),

    # ── Sketch offset: how to treat a chain's opening corner ────────────────
    ("Round corners", "Esquinas redondeadas", "Cantos arredondados", "Coins arrondis", "Ecken runden", "Angoli arrotondati"),
    ("Sharp corners", "Esquinas agudas", "Cantos afiados", "Coins vifs", "Scharfe Ecken", "Angoli acuti"),

    # ── Unfold rigidity. "Material" is the label above it; these are how
    #    foldable the stock is, not which material it is. ───────────────────
    ("Pliable",     "Flexible",     "Flexível",     "Souple",     "Formbar",     "Pieghevole"),
    ("Semi-rigid",  "Semirrígido",  "Semirrigido",  "Semi-rigide", "Halbstarr",   "Semirrigido"),
    ("Rigid",       "Rígido",       "Rígido",       "Rigide",      "Starr",        "Rigido"),

    # ── AI provider driver description (the vendor name beside it is a
    #    proper noun and stays as-is in the source) ────────────────────────
    ("OpenAI-compatible (OpenAI/Ollama/LM Studio)",
     "Compatible con OpenAI (OpenAI/Ollama/LM Studio)",
     "Compatível com OpenAI (OpenAI/Ollama/LM Studio)",
     "Compatible OpenAI (OpenAI/Ollama/LM Studio)",
     "OpenAI-kompatibel (OpenAI/Ollama/LM Studio)",
     "Compatibile con OpenAI (OpenAI/Ollama/LM Studio)"),

    # ── Sketch text: bundled font families. The family name stays Latin (it
    #    is a proper noun); the descriptor around it is translated. ────────
    ("Mono (JetBrains)",     "Monoespaciada (JetBrains)", "Monoespaçada (JetBrains)", "Monospace (JetBrains)",   "Monospace (JetBrains)",   "等宽 (JetBrains)"),
    ("Sans (DejaVu)",        "De palo seco (DejaVu)", "Sem serifa (DejaVu)", "Sans-serif (DejaVu)", "Serifenlos (DejaVu)",   "无衬线 (DejaVu)"),
    ("Serif (DejaVu)",       "Con serifas (DejaVu)", "Com serifa (DejaVu)", "À empattements (DejaVu)", "Serifenbetont (DejaVu)", "衬线 (DejaVu)"),
    ("Times (Liberation)",   "Times (Liberation)", "Times (Liberation)", "Times (Liberation)", "Times (Liberation)", "Times (Liberation)"),
    ("Arial (Liberation)",   "Arial (Liberation)", "Arial (Liberation)", "Arial (Liberation)", "Arial (Liberation)", "Arial (Liberation)"),
    ("Ubuntu",               "Ubuntu", "Ubuntu", "Ubuntu", "Ubuntu", "Ubuntu"),
    ("Comic (Neue)",         "Cómic (Neue)", "Cómic (Neue)", "Comic (Neue)", "Comic (Neue)", "漫画风 (Neue)"),
    ("Stencil (Black Ops)",  "Plantilla (Black Ops)", "Stencil (Black Ops)", "Pochoir (Black Ops)", "Schablonenschrift (Black Ops)", "模板体 (Black Ops)"),
    ("Impact (Anton)",       "Impacto (Anton)", "Impacto (Anton)", "Impact (Anton)", "Impact (Anton)", "冲击体 (Anton)"),
    ("Script (Pacifico)",    "Manuscrita (Pacifico)", "Cursiva (Pacifico)", "Script (Pacifico)", "Schreibschrift (Pacifico)", "手写体 (Pacifico)"),

    # ── Construction plane type. The first three are plane NAMES (XY/XZ/YZ),
    #    which are the same words in every language. ───────────────────────
    ("Offset from Plane",  "Desplazado del plano", "Deslocado do plano", "Décalé par rapport au plan", "Versatz von Ebene", "Scostato dal piano"),
    ("Through 3 Points",  "Por 3 puntos", "Por 3 pontos", "Par 3 points", "Durch 3 Punkte", "Attraverso 3 punti"),
    ("Parallel to Face",  "Paralelo a la cara", "Paralelo à face", "Parallèle à la face", "Parallel zur Fläche", "Parallelo alla faccia"),
    ("Midplane",          "Plano medio", "Plano médio", "Plan milieu", "Mittelsenebene", "Piano medio"),
    ("Normal to Axis",    "Normal al eje", "Normal ao eixo", "Normale à l'axe", "Normal zur Achse", "Normale all'asse"),
    ("Tangent to Cylinder", "Tangente al cilindro", "Tangente ao cilindro", "Tangente au cylindre", "Tangential zum Zylinder", "Tangente al cilindro"),
    ("Through Axis",      "A través del eje", "Através do eixo", "Traversant l'axe", "Durch Achse", "Attraverso l'asse"),

    # ── Body transform type ────────────────────────────────────────────────
    ("Translate", "Trasladar", "Transladar", "Déplacer", "Verschieben", "Trasla"),
    ("Rotate",    "Rotar",     "Rodar",      "Faire pivoter", "Drehen",   "Ruota"),
    ("Scale",     "Escalar",   "Escalar",    "Échelle",   "Skalieren",  "Scala"),

    # ── Align (Mate) type ──────────────────────────────────────────────────
    ("Fasten",     "Fijar",        "Fixar",       "Fixage",    "Fixieren",  "Fissa"),
    ("Concentric", "Concéntrico",  "Concêntrico", "Concentrique", "Konzentrisch", "Concentrico"),
    ("Planar",     "Planar",       "Planar",      "Plan",       "Planar",    "Piano"),

    # ── Section view: which plane to cut on ────────────────────────────────
    ("YZ (X normal)", "YZ (normal X)", "YZ (normal X)", "YZ (normale X)", "YZ (X-normal)", "YZ (normale X)"),
    ("XZ (Y normal)", "XZ (normal Y)", "XZ (normal Y)", "XZ (normale Y)", "XZ (Y-normal)", "XZ (normale Y)"),
    ("XY (Z normal)", "XY (normal Z)", "XY (normal Z)", "XY (normale Z)", "XY (Z-normal)", "XY (normale Z)"),

    # ── Month abbreviations, for trDate(). See the module docstring: the
    #    ORDER of year/month/day is chosen in code, not here. ─────────────
    ("Jan", "Ene", "Jan", "janv.", "Jan", "gen"),
    ("Feb", "Feb", "Fev", "févr.", "Feb", "feb"),
    ("Mar", "Mar", "Mar", "mars",  "Mär", "mar"),
    ("Apr", "Abr", "Abr", "avr.",  "Apr", "apr"),
    ("May", "May", "Mai", "mai",   "Mai", "mag"),
    ("Jun", "Jun", "Jun", "juin",  "Jun", "giu"),
    ("Jul", "Jul", "Jul", "juil.",  "Jul", "lug"),
    ("Aug", "Ago", "Ago", "août",  "Aug", "ago"),
    ("Sep", "Sep", "Set", "sept.", "Sep", "set"),
    ("Oct", "Oct", "Out", "oct.",  "Okt", "ott"),
    ("Nov", "Nov", "Nov", "nov.",  "Nov", "nov"),
    ("Dec", "Dic", "Dez", "déc.",  "Dez", "dic"),
]
