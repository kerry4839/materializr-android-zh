# -*- coding: utf-8 -*-
"""Catalogue rows for batch 7 - the strings the final audit found untranslated.

SIX arguments per tuple on purpose: the zh column is filled from
tools/zh_translations*.py by update_catalogue_zh.py, so each language's text
lives in exactly one file and there is no way for the two to drift.

printf specifiers must keep their order and count. update_catalogue_zh.py
verifies that for zh and it applies equally here - "Sew %d surfaces (%d edge%s
still open)" has three specifiers in a fixed order, and reordering them prints
garbage rather than failing to compile.
"""

NEW = [
    # ── History timeline: the per-operation label of every modelling step ──
    ("Add %s", "Agregar %s", "Adicionar %s", "Ajouter %s", "%s hinzufügen", "Aggiungi %s"),
    ("Add Ø %s", "Agregar Ø %s", "Adicionar Ø %s", "Ajouter Ø %s", "Ø %s hinzufügen", "Aggiungi Ø %s"),
    ("Add Distance %s", "Agregar distancia %s", "Adicionar distância %s", "Ajouter une distance %s", "Abstand %s hinzufügen", "Aggiungi distanza %s"),
    ("Add Gap %s", "Agregar holgura %s", "Adicionar folga %s", "Ajouter un jeu %s", "Spiel %s hinzufügen", "Aggiungi gioco %s"),
    ("Add Angle %.1f°", "Agregar ángulo %.1f°", "Adicionar ângulo %.1f°", "Ajouter un angle %.1f°", "Winkel %.1f° hinzufügen", "Aggiungi angolo %.1f°"),
    ("Add constraint", "Agregar restricción", "Adicionar restrição", "Ajouter une contrainte", "Beziehung hinzufügen", "Aggiungi vincolo"),
    ("Edit %s", "Editar %s", "Editar %s", "Modifier %s", "%s bearbeiten", "Modifica %s"),
    ("Edit Angle %.1f° → %.1f°", "Editar ángulo %.1f° → %.1f°", "Editar ângulo %.1f° → %.1f°", "Modifier l'angle %.1f° → %.1f°", "Winkel %.1f° → %.1f° bearbeiten", "Modifica angolo %.1f° → %.1f°"),
    ("Remove %s", "Quitar %s", "Remover %s", "Supprimer %s", "%s entfernen", "Rimuovi %s"),
    ("Remove constraint", "Quitar restricción", "Remover restrição", "Supprimer la contrainte", "Beziehung entfernen", "Rimuovi vincolo"),
    ("Draft %.1f deg (%d face(s))", "Desmolde %.1f° (%d cara(s))", "Desmold %.1f° (%d face(s))", "Dépouille %.1f° (%d face(s))", "Entwurf %.1f° (%d Fläche(n))", "Smusso %.1f° (%d faccia/e)"),
    ("Move Face by (%.2f, %.2f, %.2f)", "Mover cara por (%.2f, %.2f, %.2f)", "Mover face por (%.2f, %.2f, %.2f)", "Déplacer la face de (%.2f, %.2f, %.2f)", "Fläche um (%.2f, %.2f, %.2f) verschieben", "Sposta faccia di (%.2f, %.2f, %.2f)"),
    ("Twist Face %.1f°", "Torcer cara %.1f°", "Torcer face %.1f°", "Tordre la face de %.1f°", "Fläche um %.1f° verdrehen", "Torci faccia di %.1f°"),
    ("Move sketch (%.2f, %.2f, %.2f)", "Mover boceto (%.2f, %.2f, %.2f)", "Mover esboço (%.2f, %.2f, %.2f)", "Déplacer le plan (%.2f, %.2f, %.2f)", "Skizze verschieben (%.2f, %.2f, %.2f)", "Sposta schizzo (%.2f, %.2f, %.2f)"),
    ("Tweak face %.2f deg", "Ajustar cara %.2f°", "Ajustar face %.2f°", "Ajuster la face de %.2f°", "Fläche um %.2f° anpassen", "Regola faccia di %.2f°"),
    ("Tweak face ", "Ajustar cara ", "Ajustar face ", "Ajuster la face ", "Fläche anpassen ", "Regola faccia "),
    ("Sew %d surfaces into a solid", "Coser %d superficies en un sólido", "Costurar %d superfícies em um sólido", "Recoudre %d surfaces en un solide", "%d Flächen zu einem Volumenkörper vernähen", "Cuci %d superfici in un solido"),
    ("Sew %d surfaces (%d edge(s) still open)", "Coser %d superficies (%d arista(s) sin cerrar)", "Costurar %d superfícies (%d aresta(s) ainda abertas)", "Recoudre %d surfaces (%d arête(s) encore ouverte(s))", "%d Flächen vernähen (%d Kante(n) noch offen)", "Cuci %d superfici (%d bordo(i) ancora aperto(i))"),
    ("Faces sewn: %d", "Caras cosidas: %d", "Faces costuradas: %d", "Faces recousues : %d", "Vernähte Flächen: %d", "Facce cucite: %d"),
    ("Closed - this is a solid.", "Cerrado: esto es un sólido.", "Fechado - isto é um sólido.", "Fermé - ceci est un solide.", "Geschlossen - das ist ein Volumenkörper.", "Chiuso - questo è un solido."),
    ("Joined at %.4f mm.", "Unido a %.4f mm.", "Unido a %.4f mm.", "Joint à %.4f mm.", "Verbunden bei %.4f mm.", "Unito a %.4f mm."),
    ("%d edge(s) still open.", "Aún quedan %d arista(s) sin cerrar.", "Ainda restam %d aresta(s) abertas.", "%d arête(s) encore ouverte(s).", "%d Kante(n) noch offen.", "%d bordo/i ancora aperto/i."),
    ("Rotate %d bodies by %.1f°", "Rotar %d cuerpos %.1f°", "Girar %d corpos %.1f°", "Faire pivoter %d solides de %.1f°", "%d Körper um %.1f° drehen", "Ruota %d solidi di %.1f°"),
    ("Rotate %d bodies by X %.2f° Y %.2f° Z %.2f° around centroid", "Rotar %d cuerpos X %.2f° Y %.2f° Z %.2f° alrededor del centroide", "Girar %d corpos X %.2f° Y %.2f° Z %.2f° em torno ao centroide", "Faire pivoter %d solides de X %.2f° Y %.2f° Z %.2f° autour du centroïde", "%d Körper um X %.2f° Y %.2f° Z %.2f° um den Schwerpunkt drehen", "Ruota %d solidi di X %.2f° Y %.2f° Z %.2f° attorno al baricentro"),
    ("Rotate %d Bodies", "Rotar %d cuerpos", "Girar %d corpos", "Faire pivoter %d solides", "%d Körper drehen", "Ruota %d solidi"),
    ("Rotate (", "Rotar (", "Girar (", "Faire pivoter (", "Drehen (", "Ruota ("),
    (" bodies)", " cuerpos)", " corpos)", " corps)", " Körper)", " solidi)"),
    ("Scale (", "Escalar (", "Escalar (", "Mettre à l'échelle (", "Skalieren (", "Scala ("),
    ("Scale (%% of current)", "Escalar (%% del actual)", "Escalar (%% do atual)", "Échelle (%% de l'actuel)", "Skalieren (%% des Aktuellen)", "Scala (%% dell'attuale)"),
    ("Scale (target %s)", "Escalar (destino %s)", "Escalar (alvo %s)", "Échelle (cible %s)", "Skalieren (Ziel %s)", "Scala (obiettivo %s)"),
    ("Rebuilding history - step %d of %d", "Reconstruyendo el historial: paso %d de %d", "Reconstruindo o histórico - etapa %d de %d", "Reconstruction de l'historique - étape %d sur %d", "Verlauf wird neu aufgebaut - Schritt %d von %d", "Ricostruzione della cronologia - passo %d di %d"),

    # ── Selection / step readouts ────────────────────────────────────────
    ("%d bodies selected", "%d cuerpos seleccionados", "%d corpos selecionados", "%d solides sélectionnés", "%d Körper ausgewählt", "%d solidi selezionati"),
    ("%zu bodies", "%zu cuerpos", "%zu corpos", "%zu solides", "%zu Körper", "%zu solidi"),
    ("ID: %d", "ID: %d", "ID: %d", "ID : %d", "ID: %d", "ID: %d"),
    (" [disabled]", " [desactivado]", " [desativado]", " [désactivé]", " [deaktiviert]", " [disattivato]"),
    (" (frozen)", " (congelado)", " (congelado)", " (figé)", " (eingefroren)", " (congelato)"),
    ("Step %d/%d", "Paso %d/%d", "Etapa %d/%d", "Étape %d/%d", "Schritt %d/%d", "Passo %d/%d"),
    ("Total: %s", "Total: %s", "Total: %s", "Total : %s", "Gesamt: %s", "Totale: %s"),
    ("Disable", "Desactivar", "Desativar", "Désactiver", "Deaktivieren", "Disattiva"),
    ("Enable", "Activar", "Ativar", "Activer", "Aktivieren", "Attiva"),
    ("(unknown time)", "(hora desconocida)", "(hora desconhecida)", "(heure inconnue)", "(Zeit unbekannt)", "(ora sconosciuta)"),
    ("Next auto-save in ~%d:%02d", "Próximo autoguardado en ~%d:%02d", "Próximo salvamento automático em ~%d:%02d", "Prochaine sauvegarde auto dans ~%d:%02d", "Nächste Auto-Speicherung in ~%d:%02d", "Prossimo salvataggio automatico tra ~%d:%02d"),
    ("version %s", "versión %s", "versão %s", "version %s", "Version %s", "versione %s"),
    ("Restore all", "Restaurar todo", "Restaurar tudo", "Tout restaurer", "Alle wiederherstellen", "Ripristina tutto"),

    # ── Boolean / Mate readouts ──────────────────────────────────────────
    ("(cutter)", "(sustractor)", "(cortante)", "(outil de soustraction)", "(Abzugsvolumen)", "(sottraente)"),
    ("(keep)", "(conservar)", "(manter)", "(conserver)", "(behalten)", "(conserva)"),
    ("Mate", "Acoplar", "Acoplar", "Contrainte d'assemblage", "Passfläche", "Accoppia"),
    ("%s (reference lost)", "%s (referencia perdida)", "%s (referência perdida)", "%s (référence perdue)", "%s (Referenz verloren)", "%s (riferimento perso)"),
    ("%s: body %d -> body %d", "%s: cuerpo %d -> cuerpo %d", "%s: corpo %d -> corpo %d", "%s : solide %d -> solide %d", "%s: Körper %d -> Körper %d", "%s: solido %d -> solido %d"),

    # ── Viewport dimension / gizmo captions ──────────────────────────────
    ("%.1f (drag)", "%.1f (arrastrar)", "%.1f (arrastar)", "%.1f (faire glisser)", "%.1f (ziehen)", "%.1f (trascina)"),
    ("Angle Snap (%d°)", "Ajuste angular (%d°)", "Ajuste angular (%d°)", "Accrochage angulaire (%d°)", "Winkelrasterung (%d°)", "Aggancio angolare (%d°)"),
    ("R %s · %d-gon", "R %s · %d-gono", "R %s · %d-gono", "R %s · %d-gone", "R %s · %d-Eck", "R %s · %d-gono"),
    ("X %.0f%%  Y %.0f%%  Z %.0f%%", "X %.0f%%  Y %.0f%%  Z %.0f%%", "X %.0f%%  Y %.0f%%  Z %.0f%%", "X %.0f%%  Y %.0f%%  Z %.0f%%", "X %.0f%%  Y %.0f%%  Z %.0f%%", "X %.0f%%  Y %.0f%%  Z %.0f%%"),
    ("(controls geometry)", "(controla la geometría)", "(controla a geometria)", "(pilote la géométrie)", "(steuert die Geometrie)", "(controlla la geometria)"),
    ("(measures only)", "(solo mide)", "(apenas mede)", "(mesure seulement)", "(misst nur)", "(misura solo)"),
    ("%s-drag", "arrastrar con %s", "arrastar com %s", "glisser avec %s", "mit %s ziehen", "trascina con %s"),

    # ── Patch fit dialog: the surface-matching solver ────────────────────
    ("Advanced", "Avanzado", "Avançado", "Avancé", "Erweitert", "Avanzate"),
    ("Gap tolerance (mm)", "Tolerancia de holgura (mm)", "Tolerância de folga (mm)", "Tolérance de jeu (mm)", "Spieltoleranz (mm)", "Tolleranza di gioco (mm)"),
    ("Degree of the surface the fit starts from. Higher lets it bulge more freely - and invent waves the boundary never asked for.",
     "Grado de la superficie desde la que parte el ajuste. Mayor permite abombar más libremente, e inventar ondas que el borde nunca pidió.",
     "Grau da superfície a partir da qual o ajuste parte. Maior permite abaular mais livremente - e inventar ondas que o contorno nunca pediu.",
     "Degré de la surface dont part l'ajustement. Plus élevé permet de bomber plus librement - et d'inventer des ondes que le bord n'a jamais demandées.",
     "Grad der Fläche, von der aus die Anpassung startet. Höher lässt sie freier wölben - und erfindet Wellen, die die Grenze nie verlangt hat.",
     "Grado della superficie da cui parte l'adattamento. Più alto consente di incurvare più liberamente - e inventare onde che il bordo non ha mai chiesto."),
    ("Sample points per boundary curve. More follows the rim more closely; fewer smooths it out.",
     "Puntos de muestreo por curva de límite. Más sigue el borde con más fidelidad; menos lo suaviza.",
     "Pontos de amostragem por curva de limite. Mais segue o contorno com mais fidelidade; menos o suaviza.",
     "Points d'échantillonnage par courbe de bord. Davantage suit le contour de plus près ; moins l'lisse.",
     "Abtastpunkte je Randkurve. Mehr folgt der Kante genauer; weniger glättet sie aus.",
     "Punti di campionamento per curva di bordo. Più segue il contorno con maggiore fedeltà; meno lo leviga."),
    ("Stiffness", "Rigidez", "Rigidez", "Raideur", "Steifigkeit", "Rigidezza"),
    ("Tangency tolerance (deg)", "Tolerancia de tangencia (°)", "Tolerância de tangência (°)", "Tolérance de tangence (°)", "Tangententoleranz (°)", "Tolleranza di tangenza (°)"),
    ("Curvature tolerance", "Tolerancia de curvatura", "Tolerância de curvatura", "Tolérance de courbure", "Krümmungstoleranz", "Tolleranza di curvatura"),
    ("Iterations", "Iteraciones", "Iterações", "Itérations", "Iterationen", "Iterazioni"),
    ("Max degree", "Grado máximo", "Grau máximo", "Degré maximal", "Maximaler Grad", "Grado massimo"),
    ("Max segments", "Segmentos máximos", "Segmentos máximos", "Segments maximum", "Maximale Segmente", "Segmenti massimi"),
    ("Anisotropic", "Anisotrópico", "Anisotrópico", "Anisotrope", "Anisotrop", "Anisotropo"),
    ("Let the fit stretch differently along and across the patch. Helps on long thin openings.",
     "Deja que el ajuste se estire de forma distinta a lo largo y a lo ancho del parche. Ayuda en aberturas largas y estrechas.",
     "Deixa o ajuste esticar-se de forma diferente ao longo e através do remendo. Ajuda em aberturas longas e estreitas.",
     "Laisse l'ajustement s'étirer différemment le long et en travers du correctif. Utile pour les ouvertures longues et étroites.",
     "Lässt die Anpassung längs und quer des Patches unterschiedlich dehnen. Hilft bei langen, schmalen Öffnungen.",
     "Lascia che l'adattamento si estenda in modo diverso lungo e di traverso alla patch. Aiuta su aperture lunghe e strette."),
    ("Continuity", "Continuidad", "Continuidade", "Continuité", "Stetigkeit", "Continuità"),
    ("Continuity: %s", "Continuidad: %s", "Continuidade: %s", "Continuité : %s", "Stetigkeit: %s", "Continuità: %s"),
    ("Gap %.4f mm  -  tangency %.2f deg", "Holgura %.4f mm  -  tangencia %.2f°", "Folga %.4f mm  -  tangência %.2f°", "Jeu %.4f mm  -  tangence %.2f°", "Spiel %.4f mm  -  Tangente %.2f°", "Gioco %.4f mm  -  tangenza %.2f°"),
    ("Fit: gap %.4f mm, tangency %.2f deg", "Ajuste: holgura %.4f mm, tangencia %.2f°", "Ajuste: folga %.4f mm, tangência %.2f°", "Ajustement : jeu %.4f mm, tangence %.2f°", "Anpassung: Spiel %.4f mm, Tangente %.2f°", "Adattamento: gioco %.4f mm, tangenza %.2f°"),
    ("No surface fits these edges.", "Ninguna superficie se ajusta a estas aristas.", "Nenhuma superfície se ajusta a estas arestas.", "Aucune surface ne s'ajuste à ces arêtes.", "Keine Fläche passt zu diesen Kanten.", "Nessuna superficie si adatta a questi bordi."),
    ("No tangent surface exists here - the faces\naround the opening stand square to it.\nThe hole is filled, held in position only.",
     "Aquí no existe una superficie tangente: las caras\nalrededor de la abertura le quedan perpendiculares.\nEl hueco solo se rellena y se mantiene en su sitio.",
     "Aqui não existe superfície tangente: as faces\nao redor da abertura ficam perpendiculares a ela.\nO buraco é apenas preenchido e mantido no lugar.",
     "Aucune surface tangente n'existe ici - les faces\nautour de l'ouverture lui sont perpendiculaires.\nLe trou est simplement rempli, maintenu en place.",
     "Hier existiert keine Tangentialfläche - die Flächen\num die Öffnung stehen rechtwinklig dazu.\nDas Loch wird nur gefüllt und an Ort und Stelle gehalten.",
     "Qui non esiste una superficie tangente: le facce\nattorno all'apertura sono ad essa perpendicolari.\nIl foro viene solo riempito e mantenuto in posizione."),

    # ── Sew results ─────────────────────────────────────────────────────
    ("Sewn into the body.", "Coser al cuerpo.", "Costurado ao corpo.", "Recousu au solide.", "In den Körper vernäht.", "Cucito nel solido."),
    ("Boundary edges: %d", "Aristas de borde: %d", "Arestas de contorno: %d", "Arêtes de bord : %d", "Randkanten: %d", "Bordi di confine: %d"),
    ("%d edge(s) have no neighbouring face, so\nthey only hold position.",
     "%d arista(s) no tienen cara vecina, por lo que\nsolo mantienen la posición.",
     "%d aresta(s) não têm face vizinha, portanto\napenas mantêm a posição.",
     "%d arête(s) n'ont aucune face voisine et ne\nservent donc qu'à maintenir la position.",
     "%d Kante(n) haben keine Nachbarfläche und\nhalten daher nur die Position.",
     "%d bordo/i non ha faccia adiacente, quindi\nserva solo a mantenere la posizione."),
    ("Standalone surface (the edges\ndon't bound one body).",
     "Superficie independiente (las aristas\nno delimitan un mismo cuerpo).",
     "Superfície independente (as arestas\nnão delimitam um mesmo corpo).",
     "Surface autonome (les arêtes\nn'enclosent pas un même solide).",
     "Eigenständige Fläche (die Kanten\numschließen keinen gemeinsamen Körper).",
     "Superficie indipendente (i bordi\nnon delimitano un unico solido)."),
    ("%d picked support face(s)", "%d cara(s) de apoyo seleccionada(s)", "%d face(s) de suporte selecionada(s)", "%d face(s) d'appui sélectionnée(s)", "%d Stützfläche(n) ausgewählt", "%d faccia/e di supporto selezionata/e"),

    # ── Modal titles ─────────────────────────────────────────────────────
    ("About Materializr", "Acerca de Materializr", "Sobre o Materializr", "À propos de Materializr", "Über Materializr", "Informazioni su Materializr"),
    ("Check for Updates", "Buscar actualizaciones", "Verificar atualizações", "Rechercher des mises à jour", "Nach Updates suchen", "Controlla aggiornamenti"),
    ("Cutting thread…", "Cortando la rosca…", "Cortando a rosca…", "Usinage du filetage…", "Gewinde wird geschnitten…", "Filettatura in corso…"),
    ("Re-cutting thread…", "Recortando la rosca…", "Recortando a rosca…", "Réusinage du filetage…", "Gewinde wird neu geschnitten…", "Rifilettatura in corso…"),
    ("Discard sketch?", "¿Descartar el boceto?", "Descartar o esboço?", "Supprimer le plan ?", "Skizze verwerfen?", "Scartare lo schizzo?"),
    ("Import Parts", "Importar piezas", "Importar peças", "Importer des pièces", "Teile importieren", "Importa parti"),
    ("Recover Project?", "¿Recuperar el proyecto?", "Recuperar o projeto?", "Récupérer le projet ?", "Projekt wiederherstellen?", "Recuperare il progetto?"),
    ("Recover Sketch?", "¿Recuperar el boceto?", "Recuperar o esboço?", "Récupérer le plan ?", "Skizze wiederherstellen?", "Recuperare lo schizzo?"),
    ("Small screen", "Pantalla pequeña", "Tela pequena", "Petit écran", "Kleiner Bildschirm", "Schermo piccolo"),
    ("Unsaved Changes", "Cambios sin guardar", "Alterações não salvas", "Modifications non enregistrées", "Nicht gespeicherte Änderungen", "Modifiche non salvate"),

    # ── Items panel / file actions ───────────────────────────────────────
    ("New Folder", "Nueva carpeta", "Nova pasta", "Nouveau dossier", "Neuer Ordner", "Nuova cartella"),
    ("Send to Open Project", "Enviar al proyecto abierto", "Enviar para o projeto aberto", "Envoyer au projet ouvert", "An offenes Projekt senden", "Invia al progetto aperto"),
    ("Offset created - 1 new element", "Desplazamiento creado: 1 elemento nuevo", "Deslocamento criado - 1 novo elemento", "Décalage créé - 1 nouvel élément", "Versatz erzeugt - 1 neues Element", "Offset creato - 1 nuovo elemento"),
    ("Offset created - %d new elements", "Desplazamiento creado: %d elementos nuevos", "Deslocamento criado - %d novos elementos", "Décalage créé - %d nouveaux éléments", "Versatz erzeugt - %d neue Elemente", "Offset creato - %d nuovi elementi"),
    ("Offset a chain: click an edge to take the whole connected run, then move the cursor or type a distance. Which side you are on picks the direction.",
     "Desplaza una cadena: haz clic en una arista para tomar toda la cadena conectada, luego mueve el cursor o escribe una distancia. El lado en el que estés marca la dirección.",
     "Desloque uma cadeia: clique numa aresta para tomar toda a cadeia conectada, depois mova o cursor ou digite uma distância. O lado em que você estiver define a direção.",
     "Décaler une chaîne : cliquez sur une arête pour prendre toute la chaîne connectée, puis déplacez le curseur ou saisissez une distance. Le côté où vous vous trouvez choisit le sens.",
     "Eine Kette versetzen: auf eine Kante klicken, um die ganze zusammenhängende Kette zu übernehmen, dann den Cursor bewegen oder einen Abstand eingeben. Die Seite, auf der Sie sind, bestimmt die Richtung.",
     "Offset di una catena: fai clic su un bordo per prendere tutta la catena connessa, poi muovi il cursore o digita una distanza. Il lato in cui ti trovi determina la direzione."),
    ("PDF: %d page(s)  (%d x %d, %s)", "PDF: %d página(s)  (%d x %d, %s)", "PDF: %d página(s)  (%d x %d, %s)", "PDF : %d page(s)  (%d x %d, %s)", "PDF: %d Seite(n)  (%d x %d, %s)", "PDF: %d pagina/e  (%d x %d, %s)"),
    ("%d x %d px  ·  %s x %s", "%d x %d px  ·  %s x %s", "%d x %d px  ·  %s x %s", "%d x %d px  ·  %s x %s", "%d x %d px  ·  %s x %s", "%d x %d px  ·  %s x %s"),

    # ── Layout rails: tooltips ───────────────────────────────────────────
    ("Hide the model tree", "Ocultar el árbol de modelo", "Ocultar a árvore do modelo", "Masquer l'arbre du modèle", "Baumansicht ausblenden", "Nascondi l'albero del modello"),
    ("Hide the history timeline", "Ocultar la línea de tiempo del historial", "Ocultar a linha do tempo do histórico", "Masquer la frise de l'historique", "Verlaufsleiste ausblenden", "Nascondi la linea del tempo della cronologia"),
    ("Show the history timeline", "Mostrar la línea de tiempo del historial", "Mostrar a linha do tempo do histórico", "Afficher la frise de l'historique", "Verlaufsleiste anzeigen", "Mostra la linea del tempo della cronologia"),
    ("Snap ON - tap for step / toggle", "Ajuste ACTIVO: toca para el paso / conmutar", "Ajuste ATIVO - toque para o passo / alternar", "Accrochage ACTIF - appuyez pour le pas / basculer", "Rasterung AN - für Schritt tippen / umschalten", "Aggancio ATTIVO - tocca per il passo / commuta"),
    ("Snap off - tap for step / toggle", "Ajuste inactivo: toca para el paso / conmutar", "Ajuste inativo - toque para o passo / alternar", "Accrochage inactif - appuyez pour le pas / basculer", "Rasterung AUS - für Schritt tippen / umschalten", "Aggancio disattivato - tocca per il passo / commuta"),
    ("Focus: hide the side panel (tap again for viewport only)", "Enfoque: oculta el panel lateral (toca de nuevo para solo el visor)", "Foco: oculta o painel lateral (toque de novo para só a viewport)", "Focus : masquer le panneau latéral (appuyez à nouveau pour la vue seule)", "Fokus: Seitenleiste ausblenden (erneut tippen für nur Viewport)", "Focus: nascondi il pannello laterale (tocca di nuovo per solo il viewport)"),
    ("Focus: hide the tool rail too (viewport only)", "Enfoque: oculta también la barra de herramientas (solo el visor)", "Foco: oculta também a barra de ferramentas (só a viewport)", "Focus : masquer aussi la barre d'outils (vue seule)", "Fokus: auch die Werkzeugleiste ausblenden (nur Viewport)", "Focus: nascondi anche la barra degli strumenti (solo viewport)"),
    ("Bring the panels back", "Volver a mostrar los paneles", "Trazer os painéis de volta", "Réafficher les panneaux", "Bereiche wieder einblenden", "Riapri i pannelli"),
    ("New Sketch", "Nuevo boceto", "Novo esboço", "Nouveau plan", "Neue Skizze", "Nuovo schizzo"),
    ("Sketch on selection", "Boceto sobre la selección", "Esboço na seleção", "Plan sur la sélection", "Skizze auf der Auswahl", "Schizzo sulla selezione"),

    # ── Face-region cycle modes ──────────────────────────────────────────
    ("all", "todo", "tudo", "tout", "alle", "tutto"),
    ("loops", "bucles", "loops", "boucles", "Schleifen", "anelli"),
    ("islands", "islas", "ilhas", "îlots", "Inseln", "isole"),
    ("Local (rebuild neighbours)", "Local (reconstruye las vecinas)", "Local (reconstrói as vizinhas)", "Local (reconstruit les voisines)", "Lokal (Nachbarflächen neu bauen)", "Locale (ricostruisce le adiacenti)"),

    # ── Tool help text ───────────────────────────────────────────────────
    ("Repair", "Reparar", "Reparar", "Réparer", "Reparieren", "Ripara"),
    ("Import an aerofoil section (Selig or Lednicer coordinates, e.g. from airfoiltools.com): pick the file, set the chord in the popup, click to place the leading edge. Stack sections on planes and Loft for a wing.",
     "Importa una sección de ala (coordenadas Selig o Lednicer, p. ej. de airfoiltools.com): elige el archivo, fija la cuerda en el diálogo y haz clic para colocar el borde de ataque. Apila secciones en planos y usa Loft para formar un ala.",
     "Importe uma seção de asa (coordenadas Selig ou Lednicer, por exemplo de airfoiltools.com): escolha o arquivo, defina a corda no diálogo e clique para posicionar o bordo de ataque. Empilhe seções em planos e use Loft para formar uma asa.",
     "Importer une section d'aile (coordonnées Selig ou Lednicer, par ex. depuis airfoiltools.com) : choisissez le fichier, réglez la corde dans la fenêtre, cliquez pour placer le bord d'attaque. Empilez des sections sur des plans et utilisez Loft pour former une aile.",
     "Importieren Sie einen Flügelquerschnitt (Selig- oder Lednicer-Koordinaten, z. B. von airfoiltools.com): Datei wählen, im Fenster die Spannweite setzen, anklicken, um die Nasenkante zu platzieren. Stapeln Sie Schnitte auf Ebenen und verwenden Sie Loft für eine Tragfläche.",
     "Importa una sezione alare (coordinate Selig o Lednicer, ad es. da airfoiltools.com): scegli il file, imposta la corda nel popup, fai clic per posizionare il bordo d'attacco. Impila le sezioni su piani e usa Loft per un'ala."),
    ("Spline, Polygon, Trim, Offset) are on the toolbar only.", "Spline, polígono, recorte, desplazamiento) están solo en la barra de herramientas.", "Spline, polígono, recorte, deslocamento) estão apenas na barra de ferramentas.", "Spline, polygone, découpe, décalage) sont uniquement dans la barre d'outils.", "Spline, Polygon, Trim, Versatz) gibt es nur in der Werkzeugleiste.", "Spline, poligono, rifilatura, offset) sono solo nella barra degli strumenti."),

    # ── Settings: autosave ───────────────────────────────────────────────
    ("Autosave on close", "Autoguardar al cerrar", "Salvar automaticamente ao fechar", "Sauvegarde auto à la fermeture", "Beim Schließen automatisch speichern", "Salvataggio automatico alla chiusura"),
    ("Saves the project automatically when you close it or quit, once it has been saved to a file at least once - no prompt needed. Crash recovery is separate and always on.",
     "Guarda el proyecto automáticamente al cerrarlo o salir, una vez que se ha guardado en un archivo al menos una vez, sin pedir confirmación. La recuperación tras fallos es aparte y siempre está activa.",
     "Salva o projeto automaticamente ao fechá-lo ou sair, depois de ter sido salvo em um arquivo ao menos uma vez - sem pedir confirmação. A recuperação após falha é separada e está sempre ativa.",
     "Enregistre le projet automatiquement à la fermeture ou à la sortie, une fois qu'il a été enregistré dans un fichier au moins une fois - sans invite. La récupération après plantage est distincte et toujours active.",
     "Speichert das Projekt beim Schließen oder Beenden automatisch, sobald es mindestens einmal in einer Datei gespeichert wurde - ohne Rückfrage. Die Absturz wiederherstellung ist davon getrennt und immer aktiv.",
     "Salva il progetto automaticamente alla chiusura o all'uscita, dopo che è stato salvato su file almeno una volta - senza chiedere conferma. Il riprino dopo crash è separato e sempre attivo."),

    # ── Mesh-trace refusals. A pre-existing tr() covers only the first of
    #    two adjacent string literals, so only the first half has a row. ───
    ("No closed cross-section here - move the plane through the mesh first.",
     "Aquí no hay sección cerrada: mueve el plano a través de la ",
     "Aqui não há seção fechada - mova o plano através da ",
     "Pas de section fermée ici - faites passer le plan à travers la ",
     "Hier gibt es keinen geschlossenen Querschnitt - führen Sie die Ebene durch das ",
     "Qui non c'è una sezione chiusa - fai passare il piano attraverso la "),
    ("No closed silhouette here - the mesh may be open/non-manifold from this direction.",
     "Aquí no hay silueta cerrada: la malla puede ser abierta o no orientada",
     "Aqui não há silhueta fechada - a malha pode ser aberta ou não orientada",
     "Pas de silhouette fermée ici - le maillage peut être ouvert ou non orientable ",
     "Hier gibt es keine geschlossene Silhouette - das Netz ist eventuell offen oder nicht orientierbar ",
     "Qui non c'è una sagoma chiusa - la mesh può essere aperta o non manifold "),
]

# ---- Batch 7b: the three interface layouts. The names are the Combo entries
#      in Settings > Appearance AND the card titles in the first-run layout
#      picker, so they must agree; the blurbs are the picker's descriptions,
#      drawn with ImDrawList::AddText rather than an ImGui text call, which is
#      why the layout picker's cards stayed English through several passes of
#      auditing ImGui::Text-shaped call sites.
NEW += [
    ("Classic", "Clásico", "Clássico", "Classique", "Klassisch", "Classico"),
    ("Modern", "Moderno", "Moderno", "Moderne", "Modern", "Moderno"),
    ("im-touch", "Táctil", "Toque", "Tactile", "Touch", "Tattile"),
    ("Desktop-style. A menu bar on top and every panel visible at once. Best with a mouse and keyboard.",
     "Estilo escritorio: una barra de menús arriba y todos los paneles visibles a la vez. Ideal con ratón y teclado.",
     "Estilo desktop: uma barra de menus no topo e todos os painéis visíveis de uma vez. Melhor com mouse e teclado.",
     "Style bureau : une barre de menus en haut et tous les panneaux visibles en même temps. Idéal avec souris et clavier.",
     "Desktop-Stil: eine Menüleiste oben und alle Panels gleichzeitig sichtbar. Am besten mit Maus und Tastatur.",
     "Stile desktop: una barra dei menu in alto e tutti i pannelli visibili contemporaneamente. Ideale con mouse e tastiera."),
    ("Clean and focused. A tool rail on the left, tabbed panels on the right, everything else out of the way. Great on any screen.",
     "Limpio y enfocado. Una barra de herramientas a la izquierda, paneles con pestañas a la derecha y todo lo demás fuera de en medio. Ideal en cualquier pantalla.",
     "Limpo e focado. Uma barra de ferramentas à esquerda, painéis com abas à direita e todo o resto fora de cena. Ótimo em qualquer tela.",
     "Sobre et épuré. Une barre d'outils à gauche, des panneaux à onglets à droite, tout le reste mis à l'écart. Parfait sur n'importe quel écran.",
     "Klar und fokussiert. Eine Werkzeugleiste links, Panels mit Reitern rechts, alles andere aus dem Weg. Ideal auf jedem Bildschirm.",
     "Pulito e concentrato. Una barra degli strumenti a sinistra, pannelli a schede a destra, tutto il resto fuori dai piedi. Ottimo su qualsiasi schermo."),
    ("Built for fingers. A full-screen model with floating buttons and big touch targets. Made for tablets.",
     "Pensado para los dedos. Un modelo a pantalla completa con botones flotantes y grandes áreas táctiles. Hecho para tabletas.",
     "Feito para os dedos. Um modelo em tela cheia com botões flutuantes e alvos de toque grandes. Feito para tablets.",
     "Conçu pour les doigts. Un modèle plein écran avec des boutons flottants et de grandes cibles tactiles. Fait pour tablettes.",
     "Für Finger gemacht. Ein Modell im Vollbild mit schwebenden Schaltflächen und großen Touchflächen. Für Tablets gemacht.",
     "Progettato per le dita. Un modello a schermo intero con pulsanti mobili e grandi aree touch. Fatto per i tablet."),
]
