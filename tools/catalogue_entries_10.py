# -*- coding: utf-8 -*-
"""Batch 10: the last seventeen gaps tools/check_tr_coverage.py found.

That tool is what produced this batch, and it is worth saying why it exists at
all. tools/i18n_extract.py scans ImGui SINK calls for literals that were never
wrapped in tr() - but once a site reads tr("Foo"), that scan can no longer see
the string, so it reports "0 untranslated" whether or not "Foo" has a
translation. A catalogue can be 100% complete as a TABLE and still leave
English on screen, because some call site asks for a key nobody ever added.
Since the English string IS the key, a missing entry and a missing tr() are
indistinguishable at runtime: both render English. Only this checker tells them
apart.

Everything here was already correctly wrapped in tr() and was simply absent
from the catalogue, so it is the one batch where no source change was needed.

Two of these carry an embedded \n (FaceOpControllers' shear-mode help), which
tr() passes through untouched - the catalogue key holds the real newline, and
esc() in add_catalogue_entries.py writes it back as \\n.
"""

NEW = [
    # ── Fillet timeout tooltip (Settings > fillet budget) ──────────────────
    ("How long a fillet may take before it is refused. OCCT's blend cannot be cancelled once started, so a fillet is first run on a background copy and abandoned if it exceeds this. Raise it for large or heavy bodies where fillets legitimately take longer; lower it if a fillet that cannot be built keeps you waiting.",
     "Cuánto puede tardar un redondeo antes de rechazarse. El blend de OCCT no se puede cancelar una vez iniciado, así que el redondeo se ejecuta primero sobre una copia en segundo plano y se abandona si supera este tiempo. Auméntalo para cuerpos grandes o pesados donde los redondeos legítimamente tardan más; bájalo si un redondeo que no se puede construir te mantiene esperando.",
     "Quanto tempo um arredondamento pode levar antes de ser recusado. O blend do OCCT não pode ser cancelado depois de iniciado, por isso o arredondamento é executado primeiro numa cópia em segundo plano e abandonado se exceder este tempo. Aumente para corpos grandes ou pesados onde os arredondamentos legivelmente demoram mais; diminua se um arredondamento que não pode ser construído estiver a manter-te à espera.",
     "Combien de temps un congé peut prendre avant d'être refusé. Le blend d'OCCT ne peut pas être annulé une fois lancé : le congé est donc d'abord calculé sur une copie en arrière-plan, puis abandonné s'il dépasse ce délai. Augmentez-la pour les grands corps ou les corps lourds où les congés prennent légitimement plus de temps ; diminuez-la si un congé impossible à construire vous fait attendre.",
     "Wie lange eine Verrundung dauern darf, bevor sie abgelehnt wird. Der Blend von OCCT lässt sich nach dem Start nicht abbrechen, deshalb wird die Verrundung zuerst auf einer Kopie im Hintergrund berechnet und verworfen, wenn sie diesen Wert überschreitet. Erhöhen Sie ihn für große oder schwere Körper, in denen Verrundungen berechtigt länger dauern; verringern Sie ihn, wenn eine nicht baubare Verrundung Sie warten lässt.",
     "Quanto tempo può impiegare uno smusso prima di essere rifiutato. Il blend di OCCT non può essere annullato una volta avviato, quindi lo smusso viene prima calcolato su una copia in background e abbandonato se supera questo tempo. Aumentalo per corpi grandi o pesanti in cui gli smussi impiegano legittimamente più tempo; riducilo se uno smusso che non può essere costruito ti fa aspettare."),

    # ── Sew / Patch: why the patch landed as its own surface ───────────────
    ("This body is already closed, so there's nothing to sew into - the patch lands as its own surface. A hole that goes right through needs both ends capped, then Sew.",
     "Este cuerpo ya está cerrado, así que no hay nada a lo que coser: el parche queda como una superficie propia. Un agujero que atraviesa de parte a parte necesita ambos extremos tapados y luego Coser.",
     "Este corpo já está fechado, por isso não há nada a que costurar: o remendo fica como uma superfície própria. Um furo que atravessa de um lado ao outro precisa de ambas as extremidades tapadas e depois Costurar.",
     "Ce corps est déjà fermé, il n'y a donc rien à coudre : le correctif reste une surface à part entière. Un trou qui traverse de part en part nécessite que les deux extrémités soient bouchées, puis Couture.",
     "Dieser Körper ist bereits geschlossen, es gibt also nichts zum Vernähen: Die Patch-Fläche landet als eigenständige Oberfläche. Ein durchgehendes Loch braucht beide Enden verschlossen und danach Vernähen.",
     "Questo corpo è già chiuso, quindi non c'è nulla da cusire: la patch resta una superficie a sé. Un foro che attraversa da parte a parte richiede che entrambe le estremità siano chiuse e poi Cucitura."),
    ("The patch wouldn't join the opening, so it lands as its own surface - the fitted edge may not reach the rim.",
     "El parche no se unía con la abertura, así que queda como una superficie propia: puede que el borde ajustado no llegue al perímetro.",
     "O remendo não se uneu à abertura, por isso fica como uma superfície própria: a aresta ajustada pode não atingir o limite.",
     "Le correctif ne rejoignait pas l'ouverture, il reste donc une surface à part entière : l'arête ajustée n'atteint peut-être pas le bord.",
     "Die Patch-Fläche hätte die Öffnung nicht verbunden und landet deshalb als eigenständige Oberfläche – die angepasste Kante erreicht den Rand womöglich nicht.",
     "La patch non avrebbe unito l'apertura, quindi resta una superficie a sé: il bordo adattato potrebbe non raggiungere il limite."),
    ("The section's own trailing edge has thickness, which is what you want for a printed or machined part; a knife edge cannot be manufactured.",
     "El borde de salida de la propia sección tiene grosor, que es justo lo que quieres para una pieza impresa o mecanizada; un filo de cuchillo no se puede fabricar.",
     "A aresta de saída da própria secção tem espessura, que é exatamente o que se quer para uma peça impressa ou maquinada; um fio de faca não pode ser fabricado.",
     "Le bord de sortie de la section a lui-même une épaisseur, ce qui est précisément ce qu'on veut pour une pièce imprimée ou usinée ; une arête vive ne peut pas être fabriquée.",
     "Die hintere Kante des Profils hat selbst eine Dicke – genau das ist für ein gedrucktes oder gefrästes Teil richtig; eine Nullkante lässt sich nicht herstellen.",
     "Lo spessore del bordo d'uscita della sezione stessa è proprio ciò che serve per un pezzo stampato o lavorato: uno spigolo a zero non è realizzabile."),

    # ── Sketch chain-selection tooltips ────────────────────────────────────
    ("Hover a line, arc or circle - the whole connected chain highlights. Click to choose it.",
     "Pasa el cursor sobre una recta, un arco o un círculo: se resalta toda la cadena conectada. Haz clic para elegirla.",
     "Passa o rato sobre uma linha, um arco ou um círculo: toda a cadeia ligada fica realçada. Clica para a escolher.",
     "Survolez une droite, un arc ou un cercle : toute la chaîne connectée s'illumine. Cliquez pour la sélectionner.",
     "Fahren Sie mit dem Zeiger über eine Linie, einen Bogen oder einen Kreis – die gesamte zusammenhängende Kette wird hervorgehoben. Klicken Sie, um sie zu wählen.",
     "Passa su una linea, un arco o un cerchio: l'intera catena connessa viene evidenziata. Fai clic per selezionarla."),
    ("Move the cursor to set the distance; the side it is on picks the direction. Click or press Enter to place it.",
     "Mueve el cursor para fijar la distancia; el lado en el que esté determina la dirección. Haz clic o pulsa Intro para colocarla.",
     "Move o rato para definir a distância; o lado em que está escolhe a direção. Clica ou prime Enter para a colocar.",
     "Déplacez le curseur pour fixer la distance ; le côté où il se trouve choisit le sens. Cliquez ou appuyez sur Entrée pour la placer.",
     "Bewegen Sie den Zeiger, um den Abstand festzulegen; die Seite, auf der er liegt, bestimmt die Richtung. Klicken Sie oder drücken Sie die Eingabetaste, um ihn zu setzen.",
     "Sposta il cursore per impostare la distanza; il lato in cui si trova determina il verso. Fai clic o premi Invio per collocarla."),
    ("Put the offset on the other side of the chain.",
     "Coloca el desplazamiento en el otro lado de la cadena.",
     "Coloca o deslocamento do outro lado da cadeia.",
     "Placez le décalage de l'autre côté de la chaîne.",
     "Legen Sie den Versatz auf die andere Seite der Kette.",
     "Metti lo scostamento dall'altro lato della catena."),
    ("Round bridges an opening corner with an arc; Sharp extends both edges to their intersection.",
     "Redondeada une el rincón de una abertura con un arco; Sharp extiende ambos bordes hasta su intersección.",
     "Arredondada une o canto de uma abertura com um arco; Sharp estende ambas as arestas até à sua interseção.",
     "Arrondi comble le coin d'une ouverture par un arc ; Sharp prolonge les deux arêtes jusqu'à leur intersection.",
     "Rund schließt eine Öffnungsecke mit einem Bogen; Scharf verlängert beide Kanten bis zu ihrem Schnittpunkt.",
     "Tondo colma l'angolo di un'apertura con un arco; Sharp prolunga entrambi i bordi fino al loro incrocio."),

    # ── Shear-mode help. Embedded newlines: tr() passes them through, and esc()
    #    writes them back into the catalogue as \n. ──────────────────────────
    ("Off: the whole body shears, so holes and features inside it lean with the face.\nOn: only the faces touching this one are rebuilt - everything else stays put.\nThe face you tilt has to be flat; what it meets can curve.",
     "Desactivado: todo el cuerpo se deforma por cizallamiento, así que los agujeros y las características interiores se inclinan con la cara.\nActivado: solo se reconstruyen las caras que tocan esta; el resto no se mueve.\nLa cara que inclines debe ser plana; la que encuentra puede ser curva.",
     "Desativado: todo o corpo é cisalhado, por isso furos e caraterísticas interiores inclinam-se com a face.\nAtivado: apenas as faces que tocam esta são reconstruídas; tudo o resto fica no lugar.\nA face que inclinar tem de ser plana; a que ela encontra pode ser curva.",
     "Désactivé : tout le corps est cisaillé, si bien que les trous et les éléments intérieurs penchent avec la face.\nActivé : seules les faces qui touchent celle-ci sont reconstruites ; tout le reste reste en place.\nLa face que vous inclinez doit être plane ; celle qu'elle rencontre peut être courbe.",
     "Aus: Der gesamte Körper wird geschert, sodass Löcher und innere Features mit der Fläche kippen.\nEin: Nur die Flächen, die diese berühren, werden neu aufgebaut – alles andere bleibt stehen.\nDie Fläche, die Sie kippen, muss eben sein; womit sie zusammentrifft, darf gekrümmt sein.",
     "Disattivato: tutto il corpo viene cisallato, quindi fori e feature interne si inclinano con la faccia.\nAttivato: vengono ricostruite solo le facce che toccano questa; tutto il resto resta in posizione.\nLa faccia che inclini deve essere piana; quella che incontra può essere curva."),

    # ── Sketch history label ───────────────────────────────────────────────
    ("Rotate sketch", "Rotar el esquema", "Rodar o esboço", "Faire pivoter le plan",
     "Skizze drehen", "Ruota lo schizzo"),

    # ── Dimension-mode banner (the viewport caption while dimensioning) ────
    # The English keeps a shouty "DIMENSION - " prefix so it reads as a mode
    # rather than a sentence; each language shouts its own way.
    ("DIMENSION - Click an entity to dimension.",
     "COTA: haz clic en una entidad para acotarla.",
     "COTA: clica numa entidade para a cotar.",
     "COTE : cliquez sur une entité pour la coter.",
     "BEMASSUNG - Auf eine Entität klicken, um sie zu bemaßen.",
     "QUOTA - Fai clic su un'entità per quotarla."),
    ("DIMENSION - Click a second entity, or empty space to place.",
     "COTA: haz clic en una segunda entidad o en un espacio vacío para colocarla.",
     "COTA: clica numa segunda entidade ou num espaço vazio para a colocar.",
     "COTE : cliquez sur une deuxième entité ou sur un espace vide pour la placer.",
     "BEMASSUNG - Auf eine zweite Entität oder in einen leeren Bereich klicken, um sie zu setzen.",
     "QUOTA - Fai clic su una seconda entità o su uno spazio vuoto per collocarla."),
    ("DIMENSION - Click to place the label.",
     "COTA: haz clic para colocar la etiqueta.",
     "COTA: clica para colocar a etiqueta.",
     "COTE : cliquez pour placer le libellé.",
     "BEMASSUNG - Klicken, um die Beschriftung zu setzen.",
     "QUOTA - Fai clic per collocare l'etichetta."),

    # ── Measure tool: the result-row labels ────────────────────────────────
    ("Bounding Box", "Rectángulo delimitador", "Caixa delimitadora", "Boîte englobante",
     "Umschließender Quader", "Riquadro delimitante"),
    ("Bounding Box (%d bodies)", "Rectángulo delimitador (%d cuerpos)",
     "Caixa delimitadora (%d corpos)", "Boîte englobante (%d corps)",
     "Umschließender Quader (%d Körper)", "Riquadro delimitante (%d corpi)"),
    ("Edge Length", "Longitud de arista", "Comprimento da aresta", "Longueur d'arête",
     "Kantenlänge", "Lunghezza spigolo"),
    ("Total Edge Length (%d edges)", "Longitud total de aristas (%d aristas)",
     "Comprimento total das arestas (%d arestas)", "Longueur totale des arêtes (%d arêtes)",
     "Gesamte Kantenlänge (%d Kanten)", "Lunghezza totale spigoli (%d spigoli)"),
]