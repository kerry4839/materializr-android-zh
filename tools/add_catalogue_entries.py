#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Append new keys to CAT in tools/i18n_catalogue.py.

update_catalogue_zh.py can only REFRESH the zh column of keys that already
exist in CAT - it has no way to invent a key, because it never looks at the
source. This closes that gap: give it a (en, es, pt, fr, de, it) tuple and it
inserts the E(...) line for you.

Idempotent: a key already in CAT is skipped, so re-running after editing the
table is safe. Entries are written with SIX arguments on purpose - the zh column
is then filled from tools/zh_translations*.py by update_catalogue_zh.py, so the
translation lives in exactly one place.

A batch module may also export REMOVE = [key, ...] for keys it invalidates.
See the comment at the batch loop for why the catalogue needs a way to shrink
and not just grow.

A batch may also propose a key upstream v1.7.1 already has. That proposal is
ignored: upstream wrote those five columns and this branch only adds the sixth.
See upstream_owned() for why that is enforced by data rather than by care.
"""
import ast
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from i18n_catalogue import CAT

CATALOGUE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "i18n_catalogue.py")
FINGERPRINTS = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "upstream_fingerprints_1.7.1.txt")

NEW = [
    # ---- Window titles. See tools/zh_translations_6.py for why each one was
    #      invisible before: the ImGui::Begin("Literal") title never went
    #      through tr(), so the harvester never saw it.
    ("2D Drawing",              "Dibujo 2D",              "Desenho 2D",              "Dessin 2D",                 "2D-Zeichnung",           "Disegno 2D"),
    ("AI Assistant",           "Asistente de IA",        "Assistente de IA",        "Assistant IA",              "KI-Assistent",            "Assistente IA"),
    ("Boundary Fill",          "Relleno por contorno",   "Preenchimento por contorno","Remplissage par contour", "Randfüllung",             "Riempimento da contorno"),
    ("Calibrate Image Scale",  "Calibrar escala de imagen", "Calibrar escala da imagem", "Calibrer l'échelle de l'image", "Bildmaßstab kalibrieren", "Calibra la scala dell'immagine"),
    ("Flat Pattern",           "Patrón plano",           "Padrão plano",            "Dépliage à plat",           "Abwickelbild",            "Schema piatto"),
    ("Import STL",             "Importar STL",           "Importar STL",            "Importer STL",              "STL importieren",         "Importa STL"),
    ("Import SVG",             "Importar SVG",           "Importar SVG",            "Importer SVG",              "SVG importieren",         "Importa SVG"),
    ("Lay Flat on Plane",      "Aplanar sobre plano",    "Deitar no plano",         "Mettre à plat sur le plan","Auf Ebene abflachen",     "Distendi sul piano"),
    ("Materials",              "Materiales",             "Materiais",               "Matériaux",                 "Materialien",             "Materiali"),
    ("Mesh Trace",             "Calcar malla",           "Traçar malha",            "Tracer le maillage",        "Netz abzeichnen",         "Traccia mesh"),
    ("Move Sketch",            "Mover boceto",           "Mover esboço",            "Déplacer le plan",          "Skizze verschieben",      "Sposta schizzo"),
    ("Patch",                  "Parche",                 "Remendo",                 "Correctif",                 "Patch",                   "Patch"),
    ("Pick more sketches",     "Elegir más bocetos",     "Escolher mais esboços",   "Choisir plus de plans",     "Weitere Skizzen wählen",  "Scegli altri schizzi"),
    ("Rotate Plane About Axis", "Rotar plano sobre eje", "Rodar plano em torno do eixo", "Faire pivoter le plan autour de l'axe", "Ebene um Achse drehen", "Ruota il piano attorno all'asse"),
    ("Set Up Tracing Planes",  "Configurar planos de calco", "Configurar planos de traçado", "Configurer les plans de tracé", "Abzeichnebenen einrichten", "Imposta i piani di tracciatura"),
    ("Variables",              "Variables",              "Variáveis",               "Variables",                 "Variablen",               "Variabili"),
    ("Versions",               "Versiones",              "Versões",                 "Versions",                  "Versionen",               "Versioni"),

    # ---- Widgets that forgot tr() entirely. src/plugins/AiAssistantPlugin.cpp
    ("Clear Chat",             "Borrar conversación",    "Limpar conversa",         "Effacer la conversation",   "Chat leeren",            "Cancella conversazione"),
    ("AI: %s",                 "IA: %s",                 "IA: %s",                  "IA : %s",                    "KI: %s",                 "IA: %s"),
    ("Thinking... %.0fs",      "Pensando... %.0fs",      "Pensando... %.0fs",       "Réflexion... %.0fs",        "Denkt nach... %.0fs",     "Sto pensando... %.0fs"),
    ("Stop",                   "Detener",                "Parar",                   "Arrêter",                   "Stopp",                  "Interrompi"),
    ("Send",                   "Enviar",                 "Enviar",                  "Envoyer",                   "Senden",                 "Invia"),

    # ---- src/plugins/MatePlugin.cpp
    ("Mates",                  "Acoplados",              "Acoplados",               "Matériau adjacent",         "Passflächen",            "Accoppiate"),
    ("Mates not applied: %s",  "Acoplados no aplicados: %s", "Acoplados não aplicados: %s", "Matériaux adjacents non appliqués : %s", "Passflächen nicht angewendet: %s", "Accoppiate non applicate: %s"),
    ("Type",                   "Tipo",                   "Tipo",                    "Type",                      "Typ",                    "Tipo"),
    ("Flip",                   "Voltear",                "Inverter",                "Retourner",                 "Umdrehen",               "Rifletti"),
    ("Suppress",               "Suprimir",               "Suprimir",                "Supprimer",                 "Unterdrücken",           "Sopprimi"),

    # ---- AI provider / key fields in Application_Dialogs.cpp
    ("Provider",               "Proveedor",              "Provedor",                "Fournisseur",               "Anbieter",               "Provider"),
    ("API Key",                "Clave API",              "Chave de API",            "Clé API",                   "API-Schlüssel",          "Chiave API"),
    ("Model",                  "Modelo",                 "Modelo",                  "Modèle",                    "Modell",                 "Modello"),
    ("Base URL",               "URL base",               "URL base",                "URL de base",               "Basis-URL",              "URL di base"),
    ("Test Connection",        "Probar conexión",        "Testar conexão",          "Tester la connexion",        "Verbindung testen",      "Prova connessione"),
    ("Testing...",             "Probando...",            "Testando...",             "Test en cours...",          "Testen...",              "Prova in corso..."),
    ("Version ",               "Versión ",              "Versão ",                 "Version ",                 "Version ",                "Version "),
    ("Auto-save ",             "Guardado automático ",   "Salvamento automático ",  "Sauvegarde auto ",         "Auto-Speichern ",         "Salvataggio automatico "),
    ("Plane ",                 "Plano ",                "Plano ",                  "Plan ",                    "Ebene ",                  "Piano "),
    ("Axis ",                  "Eje ",                  "Eixo ",                   "Axe ",                     "Achse ",                  "Asse "),
    ("Not an airfoil file: ",  "No es un archivo de perfil alar: ", "Não é um arquivo de perfil alar: ", "Ce n'est pas un fichier de profil d'aile : ", "Keine Luftfoil-Datei: ", "Non è un file di profilo alare: "),
    ("Failed: ",               "Falló: ",               "Falhou: ",                "Échec : ",                 "Fehlgeschlagen: ",       "Non riuscito: "),
    ("Couldn't unfold that: ", "No se pudo desplegar eso: ", "Não foi possível desenvolver isso: ", "Impossible de déplier cela : ", "Konnte das nicht abwickeln: ", "Impossibile sviluppare questo: "),
    ("Welcome",                "Bienvenido",            "Bem-vindo",               "Bienvenue",                "Willkommen",              "Benvenuto"),
    ("I already support - don't show this again", "Ya apoyo el proyecto - no mostrar de nuevo", "Já apoio o projeto - não mostrar de novo", "Je soutiens déjà le projet - ne plus afficher", "Ich unterstütze bereits - nicht mehr anzeigen", "Sostengo già il progetto - non mostrare più"),
    ("Already a supporter? Restore purchase", "¿Ya eres patrocinador? Restaurar compra", "Já é patrocinador? Restaurar compra", "Déjà soutien ? Restaurer l'achat", "Schon Unterstützer? Kauf wiederherstellen", "Già sostenitore? Ripristina acquisto"),
    ("Contacting the App Store...", "Contactando con la App Store...", "Contatando a App Store...", "Contact de l'App Store...", "Verbindung zum App Store...", "Contatto con l'App Store..."),
    ("Support Materializr",    "Apoyar a Materializr",   "Apoiar o Materializr",   "Soutenir Materializr",     "Materializr unterstützen","Sostieni Materializr"),
    ("Open-source parametric 3D CAD", "CAD 3D paramétrico de código abierto", "CAD 3D paramétrico de código aberto", "CAO 3D paramétrique open source", "Open-Source parametrisches 3D-CAD", "CAD 3D parametrico open source"),
    ("Open File...",           "Abrir archivo...",       "Abrir arquivo...",        "Ouvrir un fichier...",     "Datei öffnen...",         "Apri file..."),
    ("Start an empty project", "Iniciar un proyecto vacío", "Iniciar um projeto vazio", "Démarrer un projet vide", "Leeres Projekt starten", "Inizia un progetto vuoto"),
    # ---- Interactions reference panel. Two columns of prose: the action on
    #      the left, the gesture/keys that trigger it on the right. Both render
    #      through row(), which translates both halves.
    ("1-finger drag",          "Arrastrar con 1 dedo",   "Arrastar com 1 dedo",     "Glisser à 1 doigt",         "Mit 1 Finger ziehen",     "Trascina con 1 dito"),
    ("2-finger drag",          "Arrastrar con 2 dedos",  "Arrastar com 2 dedos",    "Glisser à 2 doigts",        "Mit 2 Fingern ziehen",    "Trascina con 2 dita"),
    ("Pinch",                  "Pellizcar",              "Beliscar",                "Pincer",                    "Auf- und Zuziehen",       "Pizzicare"),
    ("Scroll wheel",           "Rueda del ratón",        "Roda do mouse",           "Molette",                   "Mausrad",                 "Rotellina del mouse"),
    ("View menu",              "Menú Ver",               "Menu Exibir",             "Menu Affichage",            "Ansichtsmenü",            "Menu Visualizza"),
    ("Reset view",             "Restablecer vista",      "Redefinir vista",         "Réinitialiser la vue",      "Ansicht zurücksetzen",    "Reimposta vista"),
    ("Tap",                    "Tocar",                  "Toque",                   "Appui",                     "Tippen",                  "Tocca"),
    ("Context menu",           "Menú contextual",        "Menu de contexto",        "Menu contextuel",           "Kontextmenü",             "Menu contestuale"),
    ("Long-press",             "Mantener pulsado",       "Pressionar e segurar",    "Appui long",                "Lange drücken",           "Tieni premuto"),
    ("Hold \u25b8 Body",      "Mantener \u25b8 Cuerpo", "Segurar \u25b8 Corpo",  "Appui long \u25b8 Corps",  "Halten \u25b8 Körper",   "Tieni \u25b8 Corpo"),
    ("Add to sel.",            "Añadir a la selección",  "Adicionar à seleção",     "Ajouter à la sélection",    "Zur Auswahl hinzufügen",  "Aggiungi alla selezione"),
    ("Add to selection",       "Añadir a la selección",  "Adicionar à seleção",     "Ajouter à la sélection",    "Zur Auswahl hinzufügen",  "Aggiungi alla selezione"),
    ("Multi-Select",           "Multi-selección",        "Multi-seleção",           "Multi-sélection",           "Mehrfachauswahl",         "Multi-selezione"),
    ("Select face",            "Seleccionar cara",      "Selecionar face",         "Sélectionner une face",    "Fläche auswählen",        "Seleziona faccia"),
    ("Select body",            "Seleccionar cuerpo",    "Selecionar corpo",        "Sélectionner un solide",    "Körper auswählen",        "Seleziona solido"),
    ("Click",                  "Clic",                   "Clique",                  "Clic",                      "Klick",                   "Clic"),
    ("Double-click",           "Doble clic",             "Duplo clique",            "Double-clic",               "Doppelklick",             "Doppio clic"),
    ("Select behind",          "Seleccionar detrás",     "Selecionar atrás",        "Sélectionner derrière",     "Dahinter auswählen",      "Seleziona dietro"),
    ("Slow double-click",      "Doble clic lento",       "Duplo clique lento",      "Double-clic lent",          "Langsamer Doppelklick",   "Doppio clic lento"),
    ("Ctrl+Click",             "Ctrl+clic",              "Ctrl+clique",             "Ctrl+clic",                 "Strg+Klick",              "Ctrl+clic"),
    ("Draw",                   "Dibujar",                "Desenhar",                "Dessiner",                  "Zeichnen",                "Disegna"),
    ("Tap or drag",            "Tocar o arrastrar",      "Toque ou arraste",        "Appui ou glissement",       "Tippen oder ziehen",      "Tocca o trascina"),
    ("Finish / cancel",        "Finalizar / cancelar",   "Concluir / cancelar",     "Terminer / annuler",        "Fertig / Abbrechen",      "Conferma / annulla"),
    ("Finish / Cancel",        "Finalizar / cancelar",   "Concluir / cancelar",     "Terminer / annuler",        "Fertig / Abbrechen",      "Conferma / annulla"),
    ("Buttons",                "Botones",                "Botões",                  "Boutons",                   "Schaltflächen",           "Pulsanti"),
    ("Dimension",              "Cota",                   "Cota",                    "Cotation",                  "Bemaßung",                "Quota"),
    ("Tap + type",             "Tocar y escribir",       "Toque e digitar",         "Appui puis saisie",         "Tippen und tippen",       "Tocca e digita"),
    ("Type value + Enter",     "Escribir valor + Enter", "Digitar valor + Enter",  "Saisir la valeur + Entrée", "Wert eingeben + Enter",   "Digita valore + Invio"),
    ("Enter / Esc",            "Enter / Esc",            "Enter / Esc",             "Entrée / Échap",            "Enter / Esc",             "Invio / Esc"),
    ("Nav lock",               "Bloqueo de navegación",  "Bloqueio de navegação",   "Verrouillage navigation",   "Navigationssperre",       "Blocco navigazione"),
    ("Move toggle",            "Interruptor de mover",   "Alternar movimento",      "Bouton déplacement",        "Verschieben-Umschalter",  "Interruttore spostamento"),
    ("Undo / redo",            "Deshacer / rehacer",     "Desfazer / refazer",      "Annuler / rétablir",        "Rückgängig / Wiederholen","Annulla / ripeti"),
    ("Undo / Redo",            "Deshacer / rehacer",     "Desfazer / refazer",      "Annuler / rétablir",        "Rückgängig / Wiederholen","Annulla / ripeti"),
    ("Start",                  "Comenzar",               "Começar",                 "Démarrer",                 "Starten",                 "Inizia"),
    ("Delete selected",        "Eliminar selección",     "Excluir seleção",         "Supprimer la sélection",    "Auswahl löschen",         "Elimina selezione"),
    ("Del",                    "Supr",                   "Del",                     "Suppr",                    "Entf",                    "Canc"),
    ("Move / Rotate / Scale",  "Mover / Rotar / Escalar","Mover / Rotar / Escalar", "Déplacer / Pivoter / Échelle", "Verschieben / Drehen / Skalieren", "Sposta / Ruota / Scala"),
    ("Pick a base plane / face","Elegir plano / cara base","Escolher plano / face base","Choisir un plan / une face de base", "Basis-Ebene / -Fläche wählen", "Scegli piano / faccia base"),
    ("W / E / R",              "W / E / R",              "W / E / R",               "W / E / R",                 "W / E / R",               "W / E / R"),
    ("Ctrl+Z / Ctrl+Y",        "Ctrl+Z / Ctrl+Y",        "Ctrl+Z / Ctrl+Y",         "Ctrl+Z / Ctrl+Y",           "Strg+Z / Strg+Y",         "Ctrl+Z / Ctrl+Y"),
    ("Tool: %s",               "Herramienta: %s",        "Ferramenta: %s",          "Outil : %s",                "Werkzeug: %s",            "Strumento: %s"),
    ("Bodies: %d",            "Cuerpos: %d",             "Corpos: %d",              "Corps : %d",               "Körper: %d",              "Corpi: %d"),
    ("Selection: %s (%d)",    "Selección: %s (%d)",      "Seleção: %s (%d)",        "Sélection : %s (%d)",      "Auswahl: %s (%d)",        "Selezione: %s (%d)"),
    ("None",                  "Ninguno",                 "Nenhum",                  "Aucun",                     "Keine",                   "Nessuno"),
    ("New project",           "Nuevo proyecto",          "Novo projeto",            "Nouveau projet",            "Neues Projekt",           "Nuovo progetto"),

    # ---- Batch 7. Kept in its own file because it is large: the strings the
    #      final audit found reachable through tr() but absent from CAT, so
    #      tr() was handing the raw English back. Mostly history-step labels,
    #      modal titles, and the Patch solver vocabulary.
]

# Keys that a batch invalidates: English edited in the source, so the row in CAT
# no longer matches any call site. Filled in by the batch loop below.
REMOVE = []

# Batches 7-9 live in their own files because they are large and because each
# one is a coherent group worth reviewing on its own:
#   7  - strings tr() could already see but the catalogue had no row for
#   8  - Operation::description() sentences, one key per whole sentence
#   9  - refusals, validation messages, and prose that reaches the UI by RETURN
#       rather than through an ImGui sink (i18n_extract.py cannot see these)
#  10  - the tail that check_tr_coverage.py found: already-wrapped tr() sites
#       with no catalogue row at all
for _batch in ("catalogue_entries_7", "catalogue_entries_8", "catalogue_entries_9",
               "catalogue_entries_10", "catalogue_entries_11",
               "catalogue_entries_12", "catalogue_entries_13",
               "catalogue_entries_14", "catalogue_entries_15",
               "catalogue_entries_16", "catalogue_entries_17",
               "catalogue_entries_18"):
    try:
        _mod = __import__(_batch)
    except ImportError:
        continue
    NEW = NEW + list(_mod.NEW)
    # A batch may also list keys it INVALIDATES - a key whose English was
    # edited in the source, so the old row no longer matches any call site.
    #
    # Without this the catalogue can only grow, and a superseded key is not
    # harmless: check_specifiers.py audits every row, so a key like
    # "... under 100% tapers ..." keeps demanding that all six languages
    # reproduce the accidental conversion "% t" that lives in the stale key
    # itself. The gate then fails forever on a row nothing can reach, which is
    # the worst failure mode for a gate - it teaches you to ignore it.
    #
    # Editing tools/i18n_catalogue.py by hand is not the alternative: the
    # catalogue is generated, and a hand edit is lost the next time the table
    # is regenerated from these batches.
    REMOVE = REMOVE + list(getattr(_mod, "REMOVE", ()))


def esc(s):
    return s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")


def cat_span(content):
    """(start, end) of the CAT list body, brackets excluded."""
    marker = "CAT = ["
    start = content.find(marker)
    if start == -1:
        sys.exit("CAT start marker not found")

    depth = 0
    for i, ch in enumerate(content[start:], start):
        if ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
            if depth == 0:
                return start, i
    sys.exit("could not find the closing bracket of CAT")


def split_row(line):
    """(key, values) for one E(...) line, values unescaped. None if not a row.

    The rows are one-per-line, so this does not need to handle wrapping. It is
    written to return None rather than guess when a line does not parse: a row
    the tool cannot read is a row it must not silently rewrite, because
    rewriting it from a mis-parse would replace good data with bad.
    """
    m = re.match(r'\s*E\("((?:[^"\\]|\\.)*)",\s*(.*)\),\s*$', line)
    if not m:
        return None
    args = []
    rest = m.group(2)
    while rest:
        mm = re.match(r'\s*"((?:[^"\\]|\\.)*)"\s*(,|$)', rest)
        if not mm:
            return None
        args.append(unescape(mm.group(1)))
        rest = rest[mm.end(2):]
    return unescape(m.group(1)), args


def upstream_owned():
    """Set of keys whose five non-zh columns belong to upstream v1.7.1.

    Read from tools/upstream_fingerprints_1.7.1.txt, which records a hash per
    key rather than the text - see tools/make_upstream_fingerprints.py for why
    the text itself is the thing being kept out of this tree.

    Missing file means every key is this branch's own, which is the correct
    default for a checkout that has no upstream history: better to refresh and
    notice than to freeze and wonder.
    """
    if not os.path.exists(FINGERPRINTS):
        return set()
    keys = set()
    with open(FINGERPRINTS, encoding="utf-8") as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            _, _, key = line.strip().partition(" ")
            keys.add(ast.literal_eval(key))
    return keys


def unescape(s):
    """Turn a C-escaped source literal into the string it denotes.

    Handles \\, \\" and \\xNN - the escapes the catalogue actually uses. Anything
    else is left as written rather than guessed at, so a row this cannot fully
    understand compares unequal and gets rewritten rather than silently kept.

    A run of \\xNN is decoded as a BYTE STREAM and then UTF-8 decoded as a unit,
    not byte by byte. Per-byte chr() turns "\\xE2\\x86\\x92" into three Latin-1
    characters instead of U+2192, and because the batch file spells that arrow as
    a real character while the row on disk spells it as escapes, every run would
    see a difference and rewrite the row for ever.

    Bytes that are not valid UTF-8 are kept verbatim with surrogateescape rather
    than replaced. A replacement character would decode to itself and no longer
    equal the raw byte, so the row would look changed on every single run and
    the tool would rewrite it for ever - the same churn this function exists to
    stop. surrogateescape round-trips exactly, which is the property that
    matters: a byte this tool cannot interpret must compare equal to itself, not
    to some lossy guess.
    """
    out, i, raw = [], 0, bytearray()
    while i < len(s):
        c = s[i]
        if c == "\\" and re.match(r"x[0-9A-Fa-f]{2}", s[i + 1:i + 4] or ""):
            raw.append(int(s[i + 2:i + 4], 16))
            i += 4
            continue
        if raw:
            out.append(raw.decode("utf-8", "surrogateescape"))
            raw.clear()
        two = s[i + 1:i + 2]
        if c == "\\" and two in ('"', "\\"):
            out.append(two)
            i += 2
            continue
        if c == "\\" and two == "n":
            out.append("\n")
            i += 2
            continue
        out.append(c)
        i += 1
    if raw:
        out.append(raw.decode("utf-8", "surrogateescape"))
    return "".join(out)


def remove_rows(content, doomed):
    """Drop the E(...) lines whose key is in `doomed`. Idempotent.

    Matching is on the ESCAPED key text, because that is how the row is spelled
    on disk. Comparing against the unescaped key would silently remove nothing -
    the row for a key containing a quote or a backslash spells them as \\" and \\\\
    - and the script would report success. A removal tool that can fail open is
    worse than no removal tool, because the caller stops checking.
    """
    out, dropped = [], 0
    for line in content.split("\n"):
        m = re.match(r'\s*E\("((?:[^"\\]|\\.)*)"', line)
        if m and m.group(1) in doomed:
            dropped += 1
            continue
        out.append(line)
    return "\n".join(out), dropped


def refresh_rows(content, wanted, upstream):
    """Rewrite rows whose five non-zh columns no longer match `wanted`.

    This is what makes the ingester self-healing. Key-existence alone is not
    enough: a translation corrected in the batch file would never reach the
    catalogue, because the key is already there and first-writer-wins would keep
    the old text for ever. Comparing content instead of presence means a fix
    lands the next time this runs, with no bookkeeping list to maintain.

    `upstream` holds the keys upstream owns, and those are skipped. Refreshing on
    content is only safe within the rows this branch is responsible for; against
    upstream's rows it turns every re-run into a chance to overwrite somebody
    else's translation with a proposal of ours, which is how 54 upstream cells
    in five languages were lost before this gate existed. See
    tools/check_catalogue_regressions.py for the other half of that guard.

    The zh column is deliberately NOT compared. It is owned by
    tools/zh_translations*.py and applied afterwards by update_catalogue_zh.py,
    so at this point CAT's zh is whatever that script last wrote - comparing it
    would make this tool fight its successor on every run.
    """
    out, fixed, foreign = [], [], []
    for line in content.split("\n"):
        row = split_row(line)
        if row is None:
            out.append(line)
            continue
        key, values = row
        want = wanted.get(key)
        # Compare the five columns the batch supplies, not the whole row: a row
        # that already carries zh has six values, so comparing against a
        # five-tuple never matches and every row reads as changed on every run.
        if want is None or values[:5] == list(want):
            out.append(line)
            continue
        if key in upstream:
            foreign.append(key)
            out.append(line)
            continue
        fixed.append(key)
        # Carry the existing zh cell across. The row is rewritten with five
        # columns here because that is what a batch supplies, but dropping the
        # sixth would blank the translation until update_catalogue_zh.py ran
        # again - and if it did not, the catalogue would look complete to every
        # other check (keys present, specifiers matched) while the app showed
        # English. So take zh from the row being replaced.
        zh = values[5] if len(values) > 5 else None
        row_args = [esc(key)] + [esc(v) for v in want]
        if zh is not None:
            row_args.append(esc(zh))
        out.append('    E(%s),' % ", ".join('"%s"' % a for a in row_args))
    return "\n".join(out), fixed, foreign


def zh_rows(content):
    """Keys whose row carries a non-empty zh cell, as spelled on disk."""
    have = set()
    for line in content.split("\n"):
        row = split_row(line)
        if row and len(row[1]) > 5 and row[1][5]:
            have.add(row[0])
    return have


def main():
    # Content, not presence, decides what to write: a key already in CAT whose
    # five columns differ from the batch file is REWRITTEN, not skipped. See
    # refresh_rows for why existence alone is not enough.
    #
    # A key in REMOVE is still treated as absent, because a REMOVE-then-NEW pair
    # means the batch is replacing the row outright. With refresh_rows in place
    # such a pair is only needed when the ENGLISH changed - the key itself is
    # different - since a translation fix is now handled without it.
    doomed_keys = set(REMOVE)
    known = {en for en, _ in CAT} - doomed_keys
    fresh = [t for t in NEW if t[0] not in known]

    with open(CATALOGUE, encoding="utf-8") as f:
        content = f.read()
    translated_before = zh_rows(content)

    removed = 0
    if REMOVE:
        present = {en for en, _ in CAT}
        doomed = {esc(k) for k in REMOVE if k in present}
        missing = [k for k in REMOVE if k not in present]
        if missing:
            print(f"note: {len(missing)} REMOVE key(s) already absent - "
                  + "; ".join(repr(k)[:60] for k in missing))
        if doomed:
            content, removed = remove_rows(content, doomed)

    if fresh:
        start, end = cat_span(content)
        block = "\n".join(
            f'    E("{esc(en)}", "{esc(es)}", "{esc(pt)}", '
            f'"{esc(fr)}", "{esc(de)}", "{esc(it)}"),'
            for en, es, pt, fr, de, it in fresh)
        content = content[:end] + block + "\n" + content[end:]

    wanted = {row[0]: tuple(row[1:]) for row in NEW}
    content, fixed, foreign = refresh_rows(content, wanted, upstream_owned())

    # Last line of defence for the zh column: a row that had a translation must
    # still have one after this tool has been through it. Counts are not enough
    # on their own - a blank cell reads to every other check as a row that simply
    # has no zh yet - so compare the key sets and refuse to write on any loss.
    lost = sorted(translated_before - zh_rows(content) - doomed_keys)
    if lost:
        sys.exit("%d row(s) lost their zh column during this run, refusing to "
                 "write; check refresh_rows: %s"
                 % (len(lost), ", ".join(repr(k)[:40] for k in lost[:5])))

    if foreign:
        # Not an error: a batch proposing a key upstream already has is a
        # comment, not an edit. Say so, because silence here reads like the
        # proposal was taken, and the next person goes looking for their change.
        print("note: %d proposed row(s) belong to upstream v1.7.1 and were left "
              "alone: %s"
              % (len(foreign), ", ".join(repr(k)[:40] for k in foreign[:6])
                 + (", ..." if len(foreign) > 6 else "")))

    if not fresh and not removed and not fixed:
        print(f"nothing to do - all {len(NEW)} keys already in CAT")
        return

    with open(CATALOGUE, "w", encoding="utf-8") as f:
        f.write(content)

    if removed:
        print(f"removed {removed} superseded key(s) from CAT")
    if fresh:
        print(f"appended {len(fresh)} new keys to CAT"
              + (f" ({len(NEW) - len(fresh)} already present)"
                 if len(NEW) - len(fresh) else ""))
    if fixed:
        print(f"refreshed {len(fixed)} row(s) whose translations had changed"
              + (": " + ", ".join(repr(k)[:44] for k in fixed[:4])
                 if len(fixed) > 4 else ""))


if __name__ == "__main__":
    main()
