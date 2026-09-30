# -*- coding: utf-8 -*-
"""Batch 14: Operation::description() sentences, and words fed to a %s.

Two shapes, both invisible to the five existing checkers, both found by hand on
the device while auditing what the History timeline renders.

── 1. description() composed with snprintf ──────────────────────────────────

Six of the forty Operation::description() implementations built their sentence in
C++ instead of looking it up:

    char buf[96];
    std::snprintf(buf, sizeof(buf), "Move hole %s", ...);
    return buf;

The audit that should have caught this needs care. A first attempt searched the
function body for the substring "tr(" and reported 0 hits - because "c_str()"
CONTAINS the substring "tr(". Six of the forty functions have a .c_str() in
their description(), so the false negative hid exactly the cases it claimed to
test. The shipped checkers were never affected: check_tr_coverage.py and
check_tr_calls.py both anchor on a word boundary or a negative lookbehind, which
c_str() defeats. Any future audit over C++ source must do the same - a bare
substring search for a function name is not a search for that function.

Where a description is a mode word plus a noun plus a measurement, the mode and
the noun became their OWN keys rather than one "%s"-shaped sentence. The reason
is the specifier rule: check_specifiers.py requires all six languages to use the
same conversions in the same order, because the catalogue is a table of format
strings. So a single "%s sketch %s" would force Chinese to keep the English
order (mode, then measurement) or drop a placeholder. Two whole-sentence keys -
"Engrave sketch %s" / "Emboss sketch %s" - let each language order its own words
around a conversion it cannot move.

PrimitiveOp::description() is the opposite case and deliberately stays half
English. Only its KIND word is looked up; the tail is notation - "10x10x10",
"R5 x H20", "R10→0 x H15", "R10 r2" - which is the dimension shorthand CAD uses
in every language. Translating the sentence would force six languages to re-spell
the x and → separators, which is where a translator's reformatting does real
damage. R and H are conventional radius/height in Chinese CAD too.

── 2. A translated frame fed an English argument ───────────────────────────

    ImGui::Text(materializr::tr("Mode: %s"), m == Mode::Extend ? "Extend"
                                                              : "Pinch");

The format IS translated; the value is not. The rendered result is "模式: Extend"
- a translated wrapper around an English noun, which reads worse than either
language alone. Twenty-three instances across ten files.

Nothing caught these. check_tr_coverage.py asks whether the KEY has a row - and
"Mode: %s" has one, complete in all six languages, and it passes the specifier
check. i18n_extract.py only inspects literals in the FIRST argument position,
which is precisely the one that is translated. The argument list was simply
unexamined by any tool. tools/check_tr_arguments.py (new) covers it, with the
negatives it needs to avoid crying wolf:

  * a literal with no letter in it is a marker, not a word - "?", "...", "", "s"
  * a literal inside strstr/strcmp/find is a needle, not displayed text - this
    is what Application_Viewport.cpp:7335 was doing with "%s"
  * TextUnformatted is NOT a printf sink; its second parameter is text_end, so
    it is excluded from the sink list entirely

── 3. Plural "s" smuggled through an argument ──────────────────────────────

    tr("%d region%s selected"), n, n == 1 ? "" : "s"

Half of this is translated and half is not: the frame is a catalogue key, the
suffix is a raw C string concatenated onto the number. Spanish and Italian mark
the plural on the noun (región/regiones), not by appending "s" to the digit, so
the concatenation produced "%d región%s" with an English s. Twenty-seven of
these. All are rewritten to the "(s)" convention the catalogue already uses for
"%zu face(s) selected" - it is not grammatical in every language either, but it
degrades predictably, and the alternative here was a raw English s.

These five keys are now orphaned in the catalogue:
  "%zu fold/score line%s"                        "%d region%s selected"
  "Selection: %d point%s, %d line%s, %d curve%s" "%s  (%d step%s)"
  "Selected: %d %s"
Nothing reads them - add_catalogue_entries.py is first-writer-wins and has no
removal path - so they cost six generated rows each and nothing else. They are
left in place rather than hand-editing tools/i18n_catalogue.py, which is the
generated source of truth.
"""


NEW = [
    # ── Thread side. ThreadOp.cpp and Application_Dialogs.cpp each splice one
    #    of these straight into a "%s thread" / "%s Thread" frame, so the noun
    #    has to stand alone as a key of its own. ────────────────────────────
    ("Internal", "Interna", "Interna", "Interne", "Innen", "Interna"),
    ("External", "Externa", "Externa", "Externe", "Außen", "Esterna"),

    # ── Snap toggle readout ─────────────────────────────────────────────────
    ("Snap ON", "Ajuste activado", "Ajuste ativado", "Accrochage activé", "Fangen ein", "Aggancio attivo"),
    ("Snap off", "Ajuste desactivado", "Ajuste desativado", "Accrochage désactivé", "Fangen aus", "Aggancio disattivato"),

    # ── Lowercase cylinder side, from ResizeCylindricalOp::description().
    #    Lowercase because it is spliced into "Resize %s D %s" mid-sentence,
    #    which is also why "Hole"/"Cylinder" below keeps its capital. ────────
    ("hole", "taladro", "furo", "trou", "Bohrung", "foro"),
    ("outer", "exterior", "externo", "extérieur", "Außen", "esterno"),

    # ── Cylinder end picker (FaceOpControllers.cpp) ─────────────────────────
    ("Both ends", "Ambos extremos", "Ambas as extremidades", "Les deux extrémités", "Beide Enden", "Entrambe le estremità"),
    ("Bottom end", "Extremo inferior", "Extremidade inferior", "Extrémité inférieure", "Unteres Ende", "Estremo inferiore"),
    ("Top end", "Extremo superior", "Extremidade superior", "Extrémité supérieure", "Oberes Ende", "Estremo superiore"),
    ("outer face", "cara exterior", "face externa", "face extérieure", "Außenfläche", "faccia esterna"),

    # ── Merge-faces scope ───────────────────────────────────────────────────
    ("selected faces", "caras seleccionadas", "faces selecionadas", "faces sélectionnées", "ausgewählte Flächen", "facce selezionate"),
    ("whole body", "cuerpo completo", "corpo inteiro", "corps entier", "gesamter Körper", "corpo intero"),

    # ── ResizeCylindricalOp::renderProperties() heading ────────────────────
    ("Hole", "Taladro", "Furo", "Trou", "Bohrung", "Foro"),

    # ── ScaleFaceOp mode. Both cases exist in source - capitalised in
    #    renderProperties(), lowercased in description()'s "(%s)" - and both
    #    need a row, because the catalogue is keyed by exact bytes. ─────────
    ("Extend", "Extender", "Estender", "Étendre", "Verlängern", "Estendi"),
    ("extend", "extender", "estender", "étendre", "verlängern", "estendi"),
    ("pinch", "estrechar", "estreitar", "rétrécir", "verengen", "restringi"),

    # ── Thread depth-cap notes. The × here is U+00D7, matching the \xC3\x97
    #    escape that the C++ source literal uses - tr() receives the decoded
    #    character, so the key must hold the character, not the escape. ──────
    ("Depth caps at 0.45 × pitch (multi-start rounded).",
     "La profundidad se limita a 0,45 × paso (redondeado en arranque múltiple).",
     "A profundidade é limitada a 0,45 × passo (arredondado em partida múltipla).",
     "La profondeur est limitée à 0,45 × pas (arrondi en démarrage multiple).",
     "Die Tiefe ist auf 0,45 × Steigung begrenzt (gerundet bei Mehrgang).",
     "La profondità è limitata a 0,45 × passo (arrotondato in avviamento multiplo)."),
    ("Depth caps at 0.65 × pitch (ISO is 0.61).",
     "La profundidad se limita a 0,65 × paso (ISO es 0,61).",
     "A profundidade é limitada a 0,65 × passo (ISO é 0,61).",
     "La profondeur est limitée à 0,65 × pas (l'ISO vaut 0,61).",
     "Die Tiefe ist auf 0,65 × Steigung begrenzt (ISO ist 0,61).",
     "La profondità è limitata a 0,65 × passo (ISO è 0,61)."),

    # ── MoveHoleOp::description() ──────────────────────────────────────────
    ("Move hole %s", "Mover el taladro %s", "Mover o furo %s", "Déplacer le trou %s", "Loch %s verschieben", "Sposta il foro %s"),

    # ── ProjectSketchOp::description(). Two whole sentences, not one
    #    "%s sketch %s" - see the module docstring on specifier order. ──────
    ("Engrave sketch %s", "Grabar el trazo %s", "Gravar o traço %s", "Graver le trait %s", "Skizze %s gravieren", "Grava il tratto %s"),
    ("Emboss sketch %s", "Embosar el trazo %s", "Estampar o traço %s", "Embosser le trait %s", "Skizze %s prägen", "Embossa il tratto %s"),

    # ── PushPullOp::description() ──────────────────────────────────────────
    ("Push/Pull %s (%zu region(s))",
     "Empujar/Tirar %s (%zu región(es))",
     "Empurrar/Puxar %s (%zu região(ões))",
     "Pouser/Tirer %s (%zu région(s))",
     "Ziehen/Drücken %s (%zu Region(en))",
     "Spingi/Tira %s (%zu regione/i)"),

    # ── ResizeCylindricalOp::description() ─────────────────────────────────
    ("Resize %s D %s → %s",
     "Redimensionar %s, diámetro %s → %s",
     "Redimensionar %s, diâmetro %s → %s",
     "Redimensionner %s, diamètre %s → %s",
     "%s im Durchmesser %s → %s ändern",
     "Ridimensiona %s, diametro %s → %s"),
    ("Shape %s: %s / %s (bottom / top)",
     "Forma %s: %s / %s (inferior / superior)",
     "Forma %s: %s / %s (inferior / superior)",
     "Forme %s: %s / %s (bas / haut)",
     "Form %s: %s / %s (unten / oben)",
     "Forma %s: %s / %s (inferiore / superiore)"),

    # ── ScaleFaceOp::description(). %% is a literal percent sign and has to
    #    survive into all six languages, or the number renders as "150". ─────
    ("Scale face to %.0f%%/%.0f%% over %s (%s)",
     "Escalar la cara a %.0f%%/%.0f%% en %s (%s)",
     "Escalar a face para %.0f%%/%.0f%% em %s (%s)",
     "Échelle la face à %.0f%%/%.0f%% sur %s (%s)",
     "Fläche auf %.0f%%/%.0f%% über %s (%s)",
     "Scala la faccia a %.0f%%/%.0f%% su %s (%s)"),

    # ── "(s)" plurals, replacing the "%s" + ""/"s" argument pairs ──────────
    ("%zu fold/score line(s)",
     "%zu línea(s) de pliegue/marcado",
     "%zu linha(s) de dobra/marcação",
     "%zu ligne(s) de pliage/marquage",
     "%zu Knick-/Ritzlinie(n)",
     "%zu linea/e di piegatura/incisione"),
    ("Selection: %d point(s), %d line(s), %d curve(s)",
     "Selección: %d punto(s), %d línea(s), %d curva(s)",
     "Seleção: %d ponto(s), %d linha(s), %d curva(s)",
     "Sélection : %d point(s), %d ligne(s), %d courbe(s)",
     "Auswahl: %d Punkt(e), %d Linie(n), %d Kurve(n)",
     "Selezione: %d punto/i, %d linea/e, %d curva/e"),
    ("%s  (%d step(s))",
     "%s  (%d paso(s))",
     "%s  (%d etapa(s))",
     "%s  (%d étape(s))",
     "%s  (%d Schritt(e))",
     "%s  (%d passo/i)"),
    ("%d region(s) selected",
     "%d región(es) seleccionada(s)",
     "%d região(ões) selecionada(s)",
     "%d région(s) sélectionnée(s)",
     "%d Region(en) ausgewählt",
     "%d regione/i selezionata/e"),
    ("Selected: %d body(s)",
     "Seleccionado: %d cuerpo(s)",
     "Selecionado: %d corpo(s)",
     "Sélection : %d corps",
     "Ausgewählt: %d Körper",
     "Selezionato: %d corpo/i"),
    ("Selected: %d edge(s)",
     "Seleccionado: %d arista(s)",
     "Selecionado: %d aresta(s)",
     "Sélection : %d arête(s)",
     "Ausgewählt: %d Kante(n)",
     "Selezionato: %d spigolo/i"),
]