# -*- coding: utf-8 -*-
"""Batch 8a: Operation::description() sentences.

These are what the History timeline, the Properties panel and the Items tree
all show, and they are the most-repeated English on screen. Each is ONE key
holding the WHOLE sentence rather than a set of fragments: trFormat() lets the
translation choose the word order, and a fragment ("Rotate " + n + " deg around
(" ...) reads backwards in Chinese and German.

Specifier rules the whole batch obeys:
  * %s  for anything pre-formatted (fmtLength / fmtVec3 return std::string) -
    never a bare %f with a unit, because the unit is inside the formatted text;
  * %d  for counts, and every count is cast at the call site so trFormat's
    static_assert in i18n.h cannot fire on a size_t;
  * every language keeps the same specifiers in the same ORDER, which
    tools/check_specifiers.py enforces. That is a real constraint on Chinese -
    "绕 X 旋转 Y 度" would be more natural, but it reorders the arguments, so
    the phrasing keeps the noun order instead.
"""

# NOTE: 4 of these 70 keys were ALREADY in CAT when this batch ran, and
# add_catalogue_entries.py is first-writer-wins, so the catalogue keeps the
# older translation and the value here is only this batch's PROPOSAL. The
# zh column is unaffected (zh is keyed off zh_translations_*.py, which
# update_catalogue_zh.py rewrites unconditionally). Pre-existing keys:
#   'Along edge'
#   'Duplicate sketch'
#   'Offset (%s)'
#   'Transform'

NEW = [
    # ── Extrude / Revolve: same four result tags, one shared set ────────────
    ("Extrude %s%s",
     "Extruir %s%s", "Extruir %s%s", "Extruder %s%s",
     "Extrudiert %s%s", "Estrudi %s%s"),
    ("Lathe %d deg%s",
     "Revolver %d°%s", "Revolver %d°%s", "Tourner %d°%s",
     "Drehen %d°%s", "Tornire %d°%s"),
    (" (New Body)", "(Nuevo cuerpo)", "(Novo corpo)", "(Nouveau corps)",
     "(Neuer Körper)", "(Nuovo corpo)"),
    (" (Union)", "(Unión)", "(União)", "(Union)", "(Vereinigung)", "(Unione)"),
    (" (Subtract)", "(Resta)", "(Subtração)", "(Soustraction)", "(Subtraktion)", "(Sottrazione)"),
    (" (Intersect)", "(Intersección)", "(Interseção)", "(Intersection)", "(Schnittmenge)", "(Intersezione)"),

    # ── Boolean / Mirror ────────────────────────────────────────────────────
    ("Boolean %s (body %d with body %d)",
     "Booleana %s (cuerpo %d con cuerpo %d)", "Booleana %s (corpo %d com corpo %d)",
     "Booléenne %s (corps %d avec corps %d)", "Boolesch %s (Körper %d mit Körper %d)",
     "Booleana %s (corpo %d con corpo %d)"),
    ("Mirror across %s%s",
     "Reflejar respecto a %s%s", "Espelhar em relação a %s%s",
     "Miroir par rapport à %s%s", "Spiegeln an %s%s", "Rifletti rispetto a %s%s"),
    (" (keep original)", "(conservar el original)", "(manter o original)",
     "(garder l'original)", "(Original behalten)", "(mantieni l'originale)"),

    # ── Pattern / Split / Defeature / Combine / Separate ────────────────────
    ("%s Pattern, %d copies",
     "Patrón %s, %d copias", "Padrão %s, %d cópias", "Motif %s, %d copies",
     "%s-Muster, %d Kopien", "Schema %s, %d copie"),
    ("Split Body %d by plane",
     "Dividir el cuerpo %d por un plano", "Dividir o corpo %d por um plano",
     "Scinder le corps %d par un plan", "Körper %d mit einer Ebene teilen",
     "Dividi il corpo %d con un piano"),
    ("Remove %d face(s)",
     "Eliminar %d cara(s)", "Remover %d face(s)", "Supprimer %d face(s)",
     "%d Fläche(n) entfernen", "Rimuovi %d faccia/e"),
    ("Combine %d sketches",
     "Combinar %d esquemas", "Combinar %d esboços", "Combiner %d esquisses",
     "%d Skizzen kombinieren", "Combina %d schizzi"),
    ("Separate body %d into %d bodies",
     "Separar el cuerpo %d en %d cuerpos", "Separar o corpo %d em %d corpos",
     "Séparer le corps %d en %d corps", "Körper %d in %d Körper aufteilen",
     "Separa il corpo %d in %d corpi"),
    ("Loft through %d profiles%s%s",
     "Loft a través de %d perfiles%s%s", "Loft através de %d perfis%s%s",
     "Loft à travers %d profils%s%s", "Loft durch %d Profile%s%s",
     "Loft attraverso %d profili%s%s"),
    (" (Solid)", "(Sólido)", "(Sólido)", "(Solide)", "(Volumenkörper)", "(Solido)"),
    (" (Shell)", "(Cáscara)", "(Casca)", "(Coque)", "(Schale)", "(Guscio)"),
    (" Ruled", " Regida", " Regido", " Guidée", " Gerade", " Retta"),
    ("Guided loft: base profile + %d rail(s)",
     "Loft guiado: perfil base + %d guía(s)", "Loft guiado: perfil base + %d guia(s)",
     "Loft guidé : profil de base + %d rail(s)", "Geführter Loft: Basisprofil + %d Schiene(n)",
     "Loft guidato: profilo base + %d binari"),
    ("Patch across %d edges (%s)",
     "Parchear sobre %d arista(s) (%s)", "Patch sobre %d aresta(s) (%s)",
     "Correctif sur %d arête(s) (%s)", "Patch über %d Kante(n) (%s)",
     "Patch su %d spigolo(i) (%s)"),
    ("Boundary fill from %d silhouettes",
     "Relleno de contorno desde %d siluetas", "Preenchimento de contorno a partir de %d silhuetas",
     "Remplissage par contour à partir de %d silhouettes", "Konturflächenfüllung aus %d Silhouetten",
     "Riempimento da contorno di %d silhouette"),
    ("Merge faces (%d → %d)",
     "Combinar caras (%d → %d)", "Mesclar faces (%d → %d)", "Fusionner les faces (%d → %d)",
     "Flächen vereinigen (%d → %d)", "Unisci facce (%d → %d)"),

    # ── Fillets / chamfers / shell / sweep ──────────────────────────────────
    ("Chamfer D%s on %d edge(s)",
     "Chaflán D%s en %d arista(s)", "Chanfro D%s em %d aresta(s)",
     "Chanfrein D%s sur %d arête(s)", "Fase D%s an %d Kante(n)", "Smusso D%s su %d spigolo(i)"),
    ("Chamfer D%s/%s on %d edge(s)",
     "Chaflán D%s/%s en %d arista(s)", "Chanfro D%s/%s em %d aresta(s)",
     "Chanfrein D%s/%s sur %d arête(s)", "Fase D%s/%s an %d Kante(n)", "Smusso D%s/%s su %d spigolo(i)"),
    ("Fillet R%s on %d edge(s)",
     "Redondeo R%s en %d arista(s)", "Arredondamento R%s em %d aresta(s)",
     "Congé R%s sur %d arête(s)", "Verrundung R%s an %d Kante(n)", "Smussatura R%s su %d spigolo(i)"),
    ("Shell thickness %s (%d open face(s))",
     "Espesor de carcasa %s (%d cara(s) abierta(s))", "Espessura da casca %s (%d face(s) aberta(s))",
     "Épaisseur de coque %s (%d face(s) ouverte(s))", "Wandstärke %s (%d offene Fläche(n))",
     "Spessore guscio %s (%d faccia/e aperta/e)"),
    ("Sweep profile along path",
     "Barrer el perfil a lo largo de la ruta", "Varrer o perfil ao longo do caminho",
     "Balayer le profil le long du chemin", "Profil entlang des Pfades ziehen",
     "Sweep del profilo lungo il percorso"),
    ("Mesh trace setup (3 planes + sketches)",
     "Configuración de trazado de malla (3 planos + esquemas)",
     "Configuração de traçado de malha (3 planos + esboços)",
     "Configuration du tracé de maillage (3 plans + esquisses)",
     "Netzabzeichnung einrichten (3 Ebenen + Skizzen)",
     "Configurazione tracciatura mesh (3 piani + schizzi)"),

    # ── Transforms ──────────────────────────────────────────────────────────
    ("Align body %d from %s to %s",
     "Alinear el cuerpo %d de %s a %s", "Alinhar o corpo %d de %s a %s",
     "Aligner le corps %d de %s à %s", "Körper %d von %s nach %s ausrichten",
     "Allinea il corpo %d da %s a %s"),
    ("Translate %s", "Trasladar %s", "Transladar %s", "Déplacer %s",
     "Verschieben %s", "Trasla %s"),
    ("Rotate %.1f deg around (%.1f, %.1f, %.1f)",
     "Rotar %.1f° alrededor de (%.1f, %.1f, %.1f)", "Rodar %.1f° em torno de (%.1f, %.1f, %.1f)",
     "Faire pivoter de %.1f° autour de (%.1f, %.1f, %.1f)",
     "Um %.1f° um (%.1f, %.1f, %.1f) drehen", "Ruota di %.1f° attorno a (%.1f, %.1f, %.1f)"),
    ("Scale by %.2f", "Escalar ×%.2f", "Escalar ×%.2f", "Mettre à l'échelle ×%.2f",
     "Skalieren ×%.2f", "Scala di %.2f"),
    ("Transform", "Transformar", "Transformar", "Transformer", "Transformieren", "Trasforma"),
    (" (axis %d)", " (eje %d)", " (eixo %d)", " (axe %d)", " (Achse %d)", " (asse %d)"),
    (" (%d axes)", " (%d ejes)", " (%d eixos)", " (%d axes)", " (%d Achsen)", " (%d assi)"),
    (" (plane %d)", " (plano %d)", " (plano %d)", " (plan %d)", " (Ebene %d)", " (piano %d)"),
    (" (%d planes)", " (%d planos)", " (%d planos)", " (%d plans)", " (%d Ebenen)", " (%d piani)"),

    # ── Construction geometry: the type name goes in as a %s ────────────────
    ("Construction Axis (%s)",
     "Eje de construcción (%s)", "Eixo de construção (%s)", "Axe de construction (%s)",
     "Konstruktionsachse (%s)", "Asse di costruzione (%s)"),
    ("Construction Plane: %s (%s)",
     "Plano de construcción: %s (%s)", "Plano de construção: %s (%s)",
     "Plan de construction : %s (%s)", "Konstruktionsebene: %s (%s)", "Piano di costruzione: %s (%s)"),
    ("Offset (%s)", "Desplazamiento (%s)", "Deslocamento (%s)", "Décalage (%s)",
     "Versatz (%s)", "Scostamento (%s)"),
    ("World X", "X del mundo", "X do mundo", "X du monde", "Welt-X", "X del mondo"),
    ("World Y (Z-up)", "Y del mundo (Z arriba)", "Y do mundo (Z para cima)",
     "Y du monde (Z vers le haut)", "Welt-Y (Z oben)", "Y del mondo (Z in alto)"),
    ("World Z", "Z del mundo", "Z do mundo", "Z du monde", "Welt-Z", "Z del mondo"),
    ("Two points", "Dos puntos", "Dois pontos", "Deux points", "Zwei Punkte", "Due punti"),
    ("Face normal", "Normal de la cara", "Normal da face", "Normale de la face",
     "Flächennormale", "Normale della faccia"),
    ("Cylinder axis", "Eje del cilindro", "Eixo do cilindro", "Axe du cylindre",
     "Zylinderachse", "Asse del cilindro"),
    ("Along edge", "A lo largo de la arista", "Ao longo da aresta", "Le long de l'arête",
     "Entlang einer Kante", "Lungo lo spigolo"),
    ("Plane intersection", "Intersección de planos", "Interseção de planos",
     "Intersection de plans", "Ebenenschnitt", "Intersezione di piani"),
    ("3 Points", "3 puntos", "3 pontos", "3 points", "3 Punkte", "3 punti"),
    ("Parallel to Face", "Paralelo a la cara", "Paralelo à face", "Parallèle à la face",
     "Parallel zur Fläche", "Parallelo alla faccia"),
    ("Midplane", "Plano medio", "Plano médio", "Plan médian", "Mittlere Ebene", "Piano medio"),
    ("Normal to Axis", "Normal al eje", "Normal ao eixo", "Normale à l'axe",
     "Normal zur Achse", "Normale all'asse"),
    ("Tangent to Cylinder", "Tangente al cilindro", "Tangente ao cilindro",
     "Tangente au cylindre", "Tangential am Zylinder", "Tangente al cilindro"),
    ("Through Axis", "A través del eje", "Através do eixo", "À travers l'axe",
     "Durch die Achse", "Attraverso l'asse"),

    # ── Copy / delete / duplicate ───────────────────────────────────────────
    ("Duplicate body %d offset %s",
     "Duplicar el cuerpo %d con desplazamiento %s",
     "Duplicar o corpo %d com deslocamento %s",
     "Dupliquer le corps %d avec un décalage de %s",
     "Körper %d um %s versetzt duplizieren", "Duplica il corpo %d con scostamento %s"),
    ("Delete body %d", "Eliminar el cuerpo %d", "Excluir o corpo %d",
     "Supprimer le corps %d", "Körper %d löschen", "Elimina il corpo %d"),
    ("Delete \"%s\"", "Eliminar « %s »", "Excluir «%s»", "Supprimer « %s »",
     "„%s“ löschen", "Elimina \"%s\""),
    ("Duplicate → %s", "Duplicar → %s", "Duplicar → %s", "Dupliquer → %s",
     "Duplizieren → %s", "Duplica → %s"),
    ("Duplicate sketch", "Duplicar el esquema", "Duplicar o esboço", "Dupliquer l'esquisse",
     "Skizze duplizieren", "Duplica lo schizzo"),

    # ── Sketch edits ────────────────────────────────────────────────────────
    ("Sketch edit", "Edición de esquema", "Edição de esboço", "Modification du croquis",
     "Skizze bearbeiten", "Modifica schizzo"),
    ("Add sketch element", "Añadir elemento al esquema", "Adicionar elemento ao esboço",
     "Ajouter un élément au croquis", "Skizzenelement hinzufügen", "Aggiungi elemento allo schizzo"),
    ("Remove sketch element", "Eliminar elemento del esquema", "Remover elemento do esboço",
     "Supprimer un élément du croquis", "Skizzenelement entfernen", "Rimuovi elemento dallo schizzo"),
    ("Modify sketch", "Modificar el esquema", "Modificar o esboço", "Modifier le croquis",
     "Skizze ändern", "Modifica lo schizzo"),
    ("Circle Ø%s", "Círculo Ø%s", "Círculo Ø%s", "Cercle Ø%s", "Kreis Ø%s", "Cerchio Ø%s"),
    ("Arc R%s", "Arco R%s", "Arco R%s", "Arc R%s", "Bogen R%s", "Arco R%s"),
    ("Rectangle %s × %s", "Rectángulo %s × %s", "Retângulo %s × %s", "Rectangle %s × %s",
     "Rechteck %s × %s", "Rettangolo %s × %s"),
    ("Line %s", "Línea %s", "Linha %s", "Ligne %s", "Linie %s", "Linea %s"),

    # ── Thread ──────────────────────────────────────────────────────────────
    # "%s thread ..." - the head noun lives inside the key rather than being
    # borrowed from renderProperties, so a language may place the %s anywhere.
    ("%s thread Ø%s, pitch %s%s%s",
     "Rosca %s Ø%s, paso %s%s%s", "Rosca %s Ø%s, passo %s%s%s",
     "Filetage %s Ø%s, pas %s%s%s", "Gewinde %s Ø%s, Steigung %s%s%s",
     "Filettatura %s Ø%s, passo %s%s%s"),
    ("%d-start", "%d inicios", "%d inícios", "%d départs", "%d-gängig", "%d filetti"),
    (" (LH)", " (izquierdo)", " (esquerdo)", " (gauche)", " (linksgängig)", " (sinistra)"),
]
