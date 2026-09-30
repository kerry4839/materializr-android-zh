# -*- coding: utf-8 -*-
"""Batch 17: the User Guide, the toolbar tooltips, and the touch-gesture table.

Three sources, all fed through local UI helpers (section / btn / row), which is
why they needed shape 4 of check_op_names.py to surface at all: the helper's
BODY decides whether a literal is a lookup key or on-screen text, and only the
body knows.

The guide bodies are long. That is deliberate and it is the point of this
batch. A paragraph of help text is read as prose, not scanned as a label, so it
is kept as one whole string rather than split per sentence: split it and the
Chinese can no longer choose its own word order, which for running text is the
difference between a translation and a transliteration.

Note '1%%' in the Scale body. It is not a typo in the source: the string goes
through ImGui::TextWrapped("%s", body), and the doubled percent is what the
catalogue key must contain to match the bytes at the call site.
"""

_HISTORY = (
    "Every modelling step goes into History on the right. Click a step to "
    "edit its parameters in Properties; right-click for Delete, Disable, "
    "Set Breakpoint, or Edit. Click a fillet/chamfer face in the viewport "
    "to jump straight to its history step. Reloaded projects show "
    "steps marked (reloaded) - undo/redo still walks through them but "
    "their parameters can't be re-edited.")

_SKETCHING = (
    "Pick Sketch on XY/XZ/YZ from the Tools panel (or Sketch on Face after "
    "selecting one), then choose a tool: Line, Rectangle, Circle, Arc, "
    "Spline, Polygon, Trim, or Offset. Type a numeric dimension while "
    "placing to lock the size. Offset takes a whole connected chain: click "
    "one edge and the run it belongs to is highlighted, then move the "
    "cursor (the side you are on picks the direction) or type the "
    "distance. Press D for the Dimension tool: click a line, circle, "
    "two points, or two lines (parallel = distance, angled = angle), "
    "click to place the label, then type the value. Switch to Select / "
    "Move to drag existing points and lines - double-click empties to "
    "select the whole sketch, then use Copy / Mirror / Rotate. Click "
    "Finish Sketch (or press Enter) to exit.")

_NAVIGATING = (
    "Drag the middle mouse button to orbit, the right button to pan, and "
    "the scroll wheel to zoom. The cube in the top-right is a navigation "
    "widget: click a face for an orthographic view, click a corner dot for "
    "an isometric view, click an arrow for a 90° rotation, or drag the "
    "cube itself to free-orbit. Reset the camera with Home or "
    "View → Reset Camera.")

_MODELLING = (
    "With a sketch region or face selected, click Extrude or Push/Pull. "
    "An arrow gizmo appears on the face - drag it for live preview, or "
    "type a value in the popup. Negative values cut into the body. After "
    "you have a body, use Fillet or Chamfer on edges (drag the handle "
    "outward to grow the radius/distance) and Mirror / Pattern from the "
    "tools sidebar.")

_SELECTING = (
    "Single-click a face or edge to select it; Ctrl+click adds to the "
    "selection. Double-click selects the whole body. Drag in empty space "
    "to box-select multiple bodies at once. Esc clears or cancels the "
    "current operation.")

_MOVE_ROTATE_SCALE = (
    "Select a body (or several) and pick the Move, Rotate or Scale gizmo "
    "from the toolbar. Drag a handle to transform. Rotate snaps softly to "
    "45°; Scale snaps to 1%% and shows a live readout. Scale has a "
    "side panel for typed per-axis values and a uniform-scale checkbox.")

_SAVING = (
    "Ctrl+S saves; the project includes bodies, colours, sketches and the "
    "operation history. Settings → Autosave enables a periodic re-save "
    "once the project has a file on disk. Application preferences "
    "(theme, mouse buttons, orbit mode) are stored separately under your "
    "user config directory so they follow you between projects.")


def zh_only(entries):
    """Strip the Chinese column, leaving the SIX the ingester expects.

    Every batch module in this directory carries seven fields, the last being
    zh, but add_catalogue_entries.py writes six because the zh column is owned
    by tools/zh_translations*.py and applied afterwards by
    update_catalogue_zh.py. Two owners for one column means one of them
    silently wins - and the ingester crashing on a 7-tuple is the better
    failure, because it happens at once instead of after the catalogue has been
    rewritten with a language nobody checked.
    """
    return [tuple(e[:6]) for e in entries]


# REMOVE is for keys the SOURCE invalidated - English that was edited so the
# row in CAT no longer matches any call site. Empty here.
#
# The Scale Face tooltip was one: Toolbar.cpp:998 now writes "100%%" so that
# "100% tapers" is not parsed as a printf conversion (in "% t a" the space is a
# flag, 't' the length modifier and 'a' the conversion), which had made all six
# languages reproduce a specifier that existed only because of a typo. The stale
# row is already gone, so nothing re-adds it.
#
# A key fixed only in its TRANSLATIONS does NOT belong here. add_catalogue_entries.py
# compares each row's five non-zh columns against this file and rewrites the ones
# that differ, so a corrected translation lands on the next run with no bookkeeping.
# Nine Chinese rows here used a break-dash and were corrected that way.
#
# A hand edit to i18n_catalogue.py is not an alternative: the table is generated,
# and a row deleted or fixed by hand returns when the batches are replayed.


REMOVE = []


NEW = [
    # ── User Guide: section headings ─────────────────────────────────────
    ("Navigating the viewport",
     "Navegacion de la vista",
     "Navegacao na vista",
     "Navigation dans la vue",
     "Navigation in der Ansicht",
     "Navigazione nella vista",
     "视图导航"),
    ("Selecting things",
     "Seleccion de elementos",
     "Selecao de elementos",
     "Selection d'elements",
     "Objekte auswahlen",
     "Selezione degli elementi",
     "选择对象"),
    ("Sketching",
     "Dibujado de bocetos",
     "Desenho de esbocos",
     "Dessin de plans",
     "Skizzieren zeichnen",
     "Disegno di schizzi",
     "绘制草图"),
    ("Modelling from a sketch",
     "Modelado a partir de un boceto",
     "Modelagem a partir de um esboco",
     "Modelage a partir d'un plan",
     "Modellieren aus einer Skizze",
     "Modellazione da uno schizzo",
     "从草图建模"),
    ("Saving and autosave",
     "Guardado y autoguardado",
     "Salvar e autosalvar",
     "Enregistrement et auto-enregistrement",
     "Speichern und Autospeichern",
     "Salvataggio e salvataggio automatico",
     "保存与自动保存"),

    # ── User Guide: section bodies ───────────────────────────────────────
    (_NAVIGATING,
     "Arrastra el boton central del raton para orbitar, el derecho para "
     "desplazar y la rueda para acercar. El cubo de la esquina superior "
     "derecha es un control de navegacion: pulsa una cara para una vista "
     "ortografica, pulsa un punto de esquina para una vista isometrica, "
     "pulsa una flecha para un giro de 90 grados, o arrastra el propio cubo "
     "para orbitar libremente. Restablece la camara con Inicio o "
     "Ver → Restablecer camara.",
     "Arraste o botao central do mouse para orbitar, o direito para mover e "
     "a roda para aproximar. O cubo no canto superior direito e um controle "
     "de navegacao: toque uma face para vista ortografica, um ponto de canto "
     "para vista isometrica, uma seta para rotacao de 90 graus, ou arraste o "
     "proprio cubo para orbitar livremente. Redefina a camera com Home ou "
     "Exibir → Redefinir camera.",
     "Faites glisser le bouton du milieu pour orbiter, le bouton droit pour "
     "deplacer et la molette pour zoomer. Le cube en haut a droite est un "
     "widget de navigation : cliquez une face pour une vue orthographique, un "
     "point d'angle pour une vue isometrique, une fleche pour une rotation de "
     "90 degres, ou faites glisser le cube pour une orbite libre. Reinitez la "
     "camera avec Home ou Vue → Reinitialiser la camera.",
     "Mausrad mittel ziehen zum Umkreisen, rechte Taste zum Verschieben, "
     "Mausrad zum Zoomen. Der Wuerfel oben rechts ist ein Navigations-Widget: "
     "auf eine Flaeche klicken fuer eine orthografische Ansicht, auf einen "
     "Eckpunkt fuer eine isometrische Ansicht, auf einen Pfeil fuer eine "
     "90-Grad-Drehung, oder den Wuerfel selbst ziehen zum freien Umkreisen. "
     "Kamera mit Pos1 oder Ansicht → Kamera zuruecksetzen zuruecksetzen.",
     "Trascina il pulsante centrale del mouse per orbitare, il destro per "
     "spostare e la rotella per lo zoom. Il cubo in alto a destra e un widget "
     "di navigazione: fai clic su una faccia per una vista ortografica, su un "
     "punto d'angolo per una vista isometrica, su una freccia per una "
     "rotazione di 90 gradi, oppure trascina il cubo stesso per orbitare "
     "liberamente. Reimposta la camera con Home o "
     "Visualizza → Reimposta camera.",
     "拖动鼠标中键旋转视图，右键平移，滚轮缩放。右上角的立方体是导航控件："
     "点击某个面切换正交视图，点击角点切换等轴测视图，点击箭头旋转 90°，"
     "或直接拖动立方体自由旋转。按 Home 键或通过"
     "视图 → 重置视图 来重置相机。"),
    (_SELECTING,
     "Haz clic en una cara o arista para seleccionarla; Ctrl+clic anade a la "
     "seleccion. El doble clic selecciona el cuerpo entero. Arrastra en un "
     "espacio vacio para seleccionar varios cuerpos a la vez. Esc limpia o "
     "cancela la operacion actual.",
     "Clique em uma face ou aresta para seleciona-la; Ctrl+clique adiciona a "
     "selecao. Clique duplo seleciona o corpo inteiro. Arraste em espaco "
     "vazio para selecionar varios corpos de uma vez. Esc limpa ou cancela a "
     "operacao atual.",
     "Cliquez sur une face ou une arete pour la selectionner ; Ctrl+clic "
     "ajoute a la selection. Le double-clic selectionne tout le solide. "
     "Faites glisser dans le vide pour selectionner plusieurs solides d'un "
     "coup. Esc efface ou annule l'operation en cours.",
     "Auf eine Flaeche oder Kante klicken, um sie auszuwaehlen; Strg+Klick "
     "erweitert die Auswahl. Doppelklick waehlt den ganzen Koerper aus. Ziehen "
     "Sie in leeren Bereich, um mehrere Koerper auf einmal zu waehlen. Esc "
     "bricht die aktuelle Operation ab.",
     "Fai clic su una faccia o uno spigolo per selezionarlo; Ctrl+clic "
     "aggiunge alla selezione. Il doppio clic seleziona l'intero solido. "
     "Trascina in uno spazio vuoto per selezionare piu solidi insieme. Esc "
     "azzera o annulla l'operazione corrente.",
     "单击面或边即可选中；Ctrl+单击可加入选择。双击选中整个实体。"
     "在空白处拖动可一次框选多个实体。Esc 清除选择或取消当前操作。"),
    (_SKETCHING,
     "Elige Boceto en XY/XZ/YZ en el panel Herramientas (o Boceto sobre cara "
     "tras seleccionar una), y luego elige una herramienta: Linea, "
     "Rectangulo, Circulo, Arco, Spline, Poligono, Recortar o Desplazar. "
     "Escribe una dimension numerica mientras colocas para fijar el tamano. "
     "Desplazar toma una cadena conectada completa: haz clic en una arista y "
     "se resalta la cadena a la que pertenece, despues mueve el cursor (el "
     "lado en el que estas elige la direccion) o escribe la distancia. Pulsa D "
     "para la herramienta Cota: haz clic en una linea, circulo, dos puntos o "
     "dos lineas (paralelas = distancia, en angulo = angulo), haz clic para "
     "colocar la etiqueta y escribe el valor. Cambia a Seleccionar / Mover "
     "para arrastrar puntos y lineas existentes; el doble clic en vacio "
     "selecciona todo el boceto, y luego usa Copiar / Reflejar / Rotar. Pulsa "
     "Finalizar boceto (o Enter) para salir.",
     "Escolha Esboco em XY/XZ/YZ no painel Ferramentas (ou Esboco sobre face "
     "depois de selecionar uma), entao escolha uma ferramenta: Linha, "
     "Retangulo, Circulo, Arco, Spline, Poligono, Recortar ou Deslocar. "
     "Digite uma dimensao numerica ao posicionar para fixar o tamanho. "
     "Deslocar percorre uma cadeia inteira: clique numa aresta e a cadeia "
     "correspondente e destacada; depois mova o cursor (o lado em que voce "
     "esta escolhe a direcao) ou digite a distancia. Pressione D para a "
     "ferramenta Cota: clique numa linha, circulo, dois pontos ou duas linhas "
     "(paralelas = distancia, em angulo = angulo), clique para posicionar o "
     "rotulo e digite o valor. Troque para Selecionar / Mover para arrastar "
     "pontos e linhas existentes; clique duplo no vazio seleciona todo o "
     "esboco, e entao use Copiar / Espelhar / Rotacionar. Pressione Finalizar "
     "esboco (ou Enter) para sair.",
     "Choisissez Dessin sur XY/XZ/YZ dans le panneau Outils (ou Dessin sur "
     "face apres avoir selectionne une face), puis un outil : Ligne, "
     "Rectangle, Cercle, Arc, Spline, Polygone, Ajuster ou Decaler. Saisissez "
     "une cote numerique pendant le placement pour fixer la taille. Decaler "
     "prend une chaine connectee entiere : cliquez une arete et la chaine "
     "correspondante est mise en evidence, puis deplacez le curseur (le cote "
     "sur lequel vous vous trouvez choisit la direction) ou saisissez la "
     "distance. Appuyez sur D pour l'outil Cotation : cliquez une ligne, un "
     "cercle, deux points ou deux lignes (paralleles = distance, en biais = "
     "angle), cliquez pour placer le libelle puis saisissez la valeur. "
     "Passez en Selectionner / Deplacer pour deplacer les points et lignes "
     "existants ; le double-clic sur le vide selectionne tout le plan, puis "
     "utilisez Copier / Refléter / Pivoter. Appuyez sur Terminer le plan (ou "
     "Entree) pour sortir.",
     "Waehlen Sie Sketch auf XY/XZ/YZ im Werkzeugbereich (oder Sketch auf "
     "Flaeche nach Auswahl einer Flaeche) und dann ein Werkzeug: Linie, "
     "Rechteck, Kreis, Bogen, Spline, Polygon, Trimmen oder Versetzen. Geben "
     "Sie beim Platzieren einen Zahlenwert ein, um die Groesse festzulegen. "
     "Versetzen folgt einer ganzen verbundenen Kette: klicken Sie eine Kante "
     "an, die zugehoerige Kette wird hervorgehoben, dann bewegen Sie den "
     "Cursor (die Seite, auf der Sie stehen, waehlt die Richtung) oder geben "
     "Sie den Abstand ein. Taste D startet das Bemaessungswerkzeug: klicken "
     "Sie eine Linie, einen Kreis, zwei Punkte oder zwei Linien (parallel = "
     "Abstand, schraeg = Winkel), klicken Sie zum Platzieren des Textes und "
     "geben Sie den Wert ein. Wechseln Sie zu Auswaehlen / Verschieben, um "
     "Punkte und Linien zu ziehen; Doppelklick ins Leere waehlt die gesamte "
     "Skizze aus, dann Kopieren / Spiegeln / Drehen. Mit Sketch fertigstellen "
     "(oder Eingabetaste) verlassen.",
     "Scegli Schizzo su XY/XZ/YZ dal pannello Strumenti (o Schizzo su una "
     "faccia dopo averne selezionata una), poi scegli uno strumento: Linea, "
     "Rettangolo, Cerchio, Arco, Spline, Poligono, Rifila o Offset. Digita "
     "una quota numerica mentre posizioni per fissare la dimensione. Offset "
     "lavora su un'intera catena connessa: fai clic su uno spigolo e la "
     "catena corrispondente viene evidenziata, poi muovi il cursore (il "
     "lato su cui ti trovi sceglie la direzione) o digita la distanza. Premi "
     "D per lo strumento Quota: fai clic su una linea, un cerchio, due punti "
     "o due linee (parallele = distanza, inclinate = angolo), fai clic per "
     "posizionare l'etichetta e digita il valore. Passa a "
     "Seleziona/Sposta per trascinare punti e linee esistenti; il doppio "
     "clic nel vuoto seleziona l'intero schizzo, poi usa Copia/Specchia/"
     "Ruota. Premi Termina schizzo (o Invio) per uscire.",
     "在工具面板中选择「XY/XZ/YZ 平面草图」（选中一个面后则用"
     "「在面上画草图」），再挑选工具：直线、矩形、圆、圆弧、样条、"
     "多边形、修剪或偏移。放置时输入数值尺寸即可锁定大小。偏移针对"
     "整段相连的链：点一条边后其所属的整条链会高亮，随后移动光标"
     "（光标在哪一侧就朝哪个方向偏移）或直接输入距离。按 D 键用"
     "「标注」工具：点直线、圆、两点或两条线（平行线取距离，成角线取角度），"
     "点击放置标签位置后输入数值。切到「选择/移动」即可拖动已有的点和线；"
     "在空白处双击选中整张草图，然后用复制/镜像/旋转。按"
     "「完成草图」（或回车）退出。"),
    (_MODELLING,
     "Con un.region o cara de boceto seleccionada, haz clic en Extruir o "
     "Empujar/Tirar. Aparece un manipulador de flecha sobre la cara: "
     "arrastralo para ver la vista previa en vivo, o escribe un valor en el "
     "dialogo. Los valores negativos cortan dentro del cuerpo. Cuando tengas "
     "un cuerpo, usa Redondear o Chaflan en las aristas (arrastra el "
     "manejador hacia fuera para增大 radio/distancia) y Simetrizar / "
     "Patron desde la barra de herramientas.",
     "Com um esboco de regiao ou face selecionado, clique em Extrudar ou "
     "Empurrar/Puxar. Um manipulador de seta aparece na face - arraste para "
     "visualizar ao vivo, ou digite um valor na janela. Valores negativos "
     "recortam o corpo. Depois que voce tiver um corpo, use Arredondar ou "
     "Chanfro nas arestas (arraste o manipulador para fora para aumentar o "
     "raio/distancia) e Espelhar / Padrao pela barra de ferramentas.",
     "Avec une region de plan ou une face selectionnee, cliquez sur Extruder "
     "ou Tirer/Pouser. Une gache flechee apparait sur la face - faites-la "
     "glisser pour un apercu en direct, ou saisissez une valeur dans la boite. "
     "Les valeurs negatives entailent une soustraction dans le solide. Une "
     "fois un solide cree, utilisez Congedier ou Chanfrein sur les aretes "
     "(faites glisser la poignee vers l'exterieur pour augmenter le "
     "rayon/distance) et Reflet / Motif dans la barre d'outils.",
     "Bei ausgewaehlter Skizzenregion oder Flaeche auf Extrudieren oder "
     "Ziehen klicken. Auf der Flaeche erscheint ein Pfeil-Manipulator - "
     "ziehen Sie fuer eine Live-Vorschau, oder geben Sie im Dialog einen "
     "Wert ein. Negative Werte schneiden in den Koerper ein. Sobald ein "
     "Koerper existiert, verwenden Sie Verrunden oder Fasen an Kanten "
     "(Griff nach aussen ziehen, um Radius/Abstand zu vergroessern) sowie "
     "Spiegeln / Muster aus der Werkzeugleiste.",
     "Con una regione di schizzo o una faccia selezionata, fai clic su "
     "Estrudi o Spingi/Tira. Sulla faccia compare un manipolatore a freccia: "
     "trascinalo per l'anteprima dal vivo, oppure digita un valore nella "
     "finestra. I valori negativi esercano un riserbo nel solido. Una volta "
     "creato un solido, usa Smusso o raccordo sugli spigoli (trascina la "
     "maniglia verso l'esterno per aumentare raggio/distanza) e Rifletti / "
     "Pattern dalla barra degli strumenti.",
     "选中草图区域或面后，点击「拉伸」或「推/拉」。面上会出现一个"
     "箭头操纵器，拖动它可实时预览，或在弹窗中输入数值。"
     "负值表示向实体内部切除。有了实体之后，用「圆角」或"
     "「倒角」处理边（向外拖动手柄以增大半径/距离），"
     "以及工具栏中的「镜像 / 阵列」。"),
    (_MOVE_ROTATE_SCALE,
     "Selecciona un cuerpo (o varios) y elige el manipulador Mover, Rotar o "
     "Escalar de la barra. Arrastra un manejador para transformar. Rotar "
     "ajusta suavemente a 45 grados; Escalar ajusta a 1%% y muestra una "
     "lectura en vivo. Escalar tiene un panel lateral para valores "
     "introducidos por eje y una casilla de escala uniforme.",
     "Selecione um corpo (ou varios) e escolha o manipulador Mover, Rotar ou "
     "Escalar na barra. Arraste uma alca para transformar. Rotar ajusta "
     "suavemente a 45 graus; Escalar ajusta a 1%% e mostra uma leitura ao "
     "vivo. Escalar tem um painel lateral para valores digitados por eixo e "
     "uma caixa de escala uniforme.",
     "Selectionnez un solide (ou plusieurs) et choisissez la gache Deplacer, "
     "Pivoter ou Echelle dans la barre d'outils. Faites glisser une poignee "
     "pour transformer. Pivoter s'aligne doucement sur 45 degres ; Echelle "
     "s'aligne sur 1%% et affiche une valeur en direct. Echelle dispose d'un "
     "panneau lateral pour les valeurs par axe et d'une case d'echelle "
     "uniforme.",
     "Waehlen Sie einen Koerper (oder mehrere) und im Werkzeugleisten- "
     "Bereich den Manipulator Verschieben, Drehen oder Skalieren. Ziehen Sie "
     "einen Griff, um zu transformieren. Drehen rastet weich auf 45 Grad "
     "ein; Skalieren rastet auf 1%% ein und zeigt eine Live-Anzeige. "
     "Skalieren hat einen seitlichen Bereich fuer eingegebene Werte pro Achse "
     "und ein Kontrollkaestchen fuer Gleichmaessigkeit.",
     "Seleziona un solido (o piu) e scegli il manipolatore Sposta, Ruota o "
     "Scala dalla barra. Trascina una maniglia per trasformare. Ruota si "
     "allinea dolcemente a 45 gradi; Scala si allinea a 1%% e mostra una "
     "lettura dal vivo. Scala ha un pannello laterale per valori digitati per "
     "asse e una casella di scala uniforme.",
     "选中一个（或多个）实体，然后在工具栏中挑选"
     "「移动」「旋转」或「缩放」操纵器。拖动手柄即可变换。"
     "旋转会柔和地吸附到 45°；缩放吸附到 1%% 并显示实时读数。"
     "缩放还有一个侧面板，可逐轴输入数值，并带"
     "「等比缩放」复选框。"),
    (_HISTORY,
     "Cada paso de modelado va a Historial, a la derecha. Haz clic en un paso "
     "para editar sus parametros en Propiedades; clic derecho para Borrar, "
     "Desactivar, Poner punto de interrupcion o Editar. Haz clic en una cara "
     "con redondeo/chaflan en la vista para saltar directamente a su paso. "
     "Los proyectos recargados muestran los pasos marcados como (recargados) - "
     "deshacer/rehacer aun los recorre pero sus parametros no se pueden "
     "volver a editar.",
     "Cada etapa de modelagem vai para Historico, a direita. Clique em uma "
     "etapa para editar seus parametros em Propriedades; clique direito para "
     "Excluir, Desativar, Definir ponto de parada ou Editar. Clique em uma "
     "face com arredondamento/chanfro na janela para ir direto a essa etapa. "
     "Projetos recarregados mostram etapas marcadas como (recarregadas) - "
     "desfazer/refazer ainda passa por elas, mas seus parametros nao podem "
     "ser editados novamente.",
     "Chaque etape de modelisation rejoint l'Historique, a droite. Cliquez sur "
     "une etape pour modifier ses parametres dans Proprietes ; clic droit pour "
     "Supprimer, Desactiver, Poser un point d'arret ou Modifier. Cliquez sur "
     "une face congediee/chanfreinee dans la vue pour acceder directement a "
     "son etape. Les projets recharges marquent les etapes (rechargees) - "
     "annuler/refaire les parcourt toujours, mais leurs parametres ne sont "
     "plus modifiables.",
     "Jeder Modellierungsschritt landet im Verlauf rechts. Klicken Sie einen "
     "Schritt an, um seine Parameter in den Eigenschaften zu bearbeiten; "
     "Rechtsklick fuer Loeschen, Deaktivieren, Haltepunkt setzen oder "
     "Bearbeiten. Klicken Sie im Fenster eine verrundete/fasierte Flaeche an, "
     "um direkt zu deren Verlaufsschritt zu springen. Neu geladene Projekte "
     "markieren Schritte als (neu geladen) - Rueckgaengig/Wiederholen laeuft "
     "weiterhin durch sie, ihre Parameter lassen sich aber nicht erneut "
     "bearbeiten.",
     "Ogni passo di modellazione finisce nella Cronologia a destra. Fai clic "
     "su un passo per modificarne i parametri in Proprieta; clic destro per "
     "Elimina, Disattiva, Imposta punto di controllo o Modifica. Fai clic su "
     "una faccia smussata/raccordata nella vista per saltare direttamente al "
     "suo passo. I progetti ricaricati marcano i passi come (ricaricati): "
     "annulla/ripeti li attraversa comunque, ma i loro parametri non sono "
     "piu modificabili.",
     "每一个建模步骤都会进入右侧的「历史」。点击某一步可在"
     "「属性」中编辑其参数；右键可删除、禁用、"
     "设置断点或编辑。在视口中点击一个带圆角/倒角的面可直接"
     "跳到对应的历史步骤。重新载入的项目会把步骤标记为"
     "「（已重新载入）」；撤销/重做仍会经过这些步骤，"
     "但它们的参数不能再编辑。"),
    (_SAVING,
     "Ctrl+S guarda; el proyecto incluye cuerpos, colores, bocetos y el "
     "historial de operaciones. Configuracion → Autoguardado activa un "
     "re-guardado periodico una vez que el proyecto tiene archivo en disco. "
     "Las preferencias de la aplicacion (tema, botones del raton, modo de "
     "orbita) se guardan aparte en tu directorio de configuracion de usuario "
     "para que te sigan entre proyectos.",
     "Ctrl+S salva; o projeto inclui corpos, cores, esbocos e o historico de "
     "operacoes. Configuracoes -> Salvamento automatico ativa um novo "
     "salvamento periodico assim que o projeto tem arquivo em disco. As "
     "preferencias do aplicativo (tema, botoes do mouse, modo de orbita) sao "
     "armazenadas separadamente no seu diretorio de configuracao do usuario "
     "para acompanhar voce entre projetos.",
     "Ctrl+S enregistre ; le projet inclut les solides, les couleurs, les "
     "plans et l'historique des operations. Reglages → Sauvegarde auto "
     "active une re-enregistrement periodique une fois le projet enregistre "
     "sur disque. Les preferences de l'application (theme, boutons souris, "
     "mode d'orbite) sont stockees a part dans votre repertoire de "
     "configuration utilisateur afin de vous suivre d'un projet a l'autre.",
     "Strg+S speichert; das Projekt enthaelt Koerper, Farben, Skizzen und "
     "den Vorgangsverlauf. Einstellungen → Autospeichern aktiviert eine "
     "regelmaessige Neu-speicherung, sobald das Projekt eine Datei auf der "
     "Platte hat. Anwendungseinstellungen (Design, Maustasten, Orbit-Modus) "
     "werden getrennt im Benutzerkonfigurationsverzeichnis abgelegt, damit sie "
     "von Projekt zu Projekt mitgehen.",
     "Ctrl+S salva; il progetto include solidi, colori, schizzi e la "
     "cronologia delle operazioni. Impostazioni -> Salvataggio automatico "
     "abilita un nuovo salvataggio periodico una volta che il progetto ha un "
     "file su disco. Le preferenze dell'applicazione (tema, pulsanti del "
     "mouse, modalita orbita) sono memorizzate separatamente nella tua "
     "cartella di configurazione utente, cosi ti seguono da un progetto "
     "all'altro.",
     "Ctrl+S 保存；工程包含实体、颜色、草图以及操作历史。"
     "设置 → 自动保存 会在工程写入磁盘后启用定期重新保存。"
     "应用程序偏好（主题、鼠标按键、旋转模式）单独存放在用户配置"
     "目录中，因此在各工程之间也能沿用。"),

    # ── Toolbar tooltips (btn(t=), already tr()'d inside the helper) ─────
    ("Start a new sketch lying on the picked face.",
     "Iniciar un boceto nuevo sobre la cara seleccionada.",
     "Iniciar um novo esboco sobre a face selecionada.",
     "Demarrer un nouveau plan sur la face selectionnee.",
     "Neue Skizze auf der angepickten Flaeche beginnen.",
     "Inizia un nuovo schizzo sulla faccia scelta.",
     "在所选面上新建一张草图。"),
    ("Drag the face along its normal to extrude (+) or cut (−) into the body.",
     "Arrastra la cara a lo largo de su normal para extruir (+) o cortar (−) "
     "dentro del cuerpo.",
     "Arraste a face ao longo da sua normal para extrudar (+) ou recortar "
     "(−) dentro do corpo.",
     "Faites glisser la face le long de sa normale pour extruder (+) ou "
     "decouper (−) dans le solide.",
     "Ziehen Sie die Flaeche entlang ihrer Normalen, um zu extrudieren (+) "
     "oder (-) in den Koerper zu schneiden.",
     "Trascina la faccia lungo la sua normale per estrudere (+) o ritagliare "
     "(−) dentro il solido.",
     "沿面的法线方向拖动该面可拉伸（+）或向实体内部"
     "切除（−）。"),
    ("Make a NEW body by extruding this face's silhouette (source body unchanged).",
     "Crear un cuerpo NUEVO extruyendo la silueta de esta cara (el cuerpo de "
     "origen no cambia).",
     "Criar um corpo NOVO extrudando a silhueta desta face (o corpo de origem "
     "nao muda).",
     "Creer un NOUVEAU solide en extrudant la silhouette de cette face (le "
     "solide source reste inchange).",
     "Erzeugen Sie einen NEUEN Koerper, indem Sie die Silhouette dieser "
     "Flaeche extrudieren (der Quellkoerper bleibt unveraendert).",
     "Crea un NUOVO solido estendendo la sagoma di questa faccia (il solido di "
     "origine resta invariato).",
     "将该面的轮廓拉伸可生成一个全新实体（源实体保持不变）。"),
    ("Hollow the body, removing the picked face. Wall thickness in the popup.",
     "Vacial el cuerpo eliminando la cara seleccionada. Grosor de pared en el "
     "dialogo.",
     "Torne o corpo oco removendo a face selecionada. Espessura da parede na "
     "janela.",
     "Creuser le solide en supprimant la face choisie. Epaisseur de paroi dans "
     "la boite de dialogue.",
     "Koerper aushoehlen und die gewaehlte Flaeche entfernen. "
     "Wandstaerke im Dialog.",
     "Svuota il solido rimuovendo la faccia scelta. Spessore parete nella "
     "finestra.",
     "抽空该实体并移除所选的面。壁厚在弹窗中设置。"),
    # The live spelling. The key must stay byte-identical to the call site, and
    # 13 other rows in the catalogue already write "100%%" for the same reason.
    # The bare-% spelling this replaced is listed in REMOVE above.
    #
    # Two percents in, two out, in the same places. i18n_catalogue.py's emitter
    # compares the %% TOKENS with FMT.findall, not the rendered text, so a
    # translation that keeps one percent and drops the other - "por encima las
    # abre" instead of "por encima del 100%% las abre" - is rejected even though
    # it reads better. That check is right: the count of literal percents is
    # part of the contract, and silently losing one changes what is displayed.
    ("Re-slope the side walls toward a scaled copy of this face - under "
     "100%% tapers it in, over 100%% flares it out. Blend length in the popup.",
     "Cambia la inclinacion de las paredes laterales hacia una copia escalada "
     "de esta cara: por debajo del 100%% las estrecha, por encima del 100%% las "
     "abre. Longitud de transicion en el dialogo.",
     "Re-incline as paredes laterais em direcao a uma copia escalada desta "
     "face: abaixo de 100%% estreita, acima de 100%% abre. Comprimento da "
     "transicao na janela.",
     "Redonne aux parois laterales une pente vers une copie mise a l'echelle "
     "de cette face : sous 100%% elles rentrent, au-dela de 100%% elles "
     "s'ecartent. Longueur de transition dans la boite de dialogue.",
     "Aendert die Neigung der Seitenwaende auf eine skalierte Kopie dieser "
     "Flaeche - unter 100%% verengen sie, ueber 100%% weiten sie sich auf. "
     "Uebergangslaenge im Dialog.",
     "Ricalca l'inclinazione delle pareti laterali verso una copia in scala "
     "di questa faccia: sotto il 100%% si restringono, sopra il 100%% si "
     "allargano. Lunghezza di transizione nella finestra.",
     "按该面的等比缩放副本重新调整侧壁倾斜度：低于 100%% 时"
     "收窄，超过 100%% 时外扩。过渡长度在弹窗中设置。"),
    ("Draft angle for moulding or printing: tilt the picked face(s) by an "
     "angle about a fixed neutral plane. Unlike Rotate it takes SEVERAL "
     "faces at one angle (all four walls of a box) and works on cylindrical "
     "and conical faces too - a cylinder drafts into a cone.",
     "Angulo de desmoldeo para moldeado o impresion: inclina las caras "
     "seleccionadas un angulo respecto a un plano neutro fijo. A diferencia "
     "de Rotar, admite VARIAS caras con el mismo angulo (las cuatro paredes "
     "de una caja) y tambien funciona con caras cilindricas y conicas: un "
     "cilindro se desmolea hacia un cono.",
     "Angulo de 탈型 para moldagem ou impressao: incline as faces selecionadas "
     "em um angulo em relacao a um plano neutro fixo. Diferente de Rotacionar, "
     "aceita VARIAS faces com o mesmo angulo (as quatro paredes de uma caixa) "
     "e tambem funciona em faces cilindricas e conicas: um cilindro torna-se "
     "um cone.",
     "Angle de demoulage pour le moulage ou l'impression : inclinez les faces "
     "selectionnees d'un angle autour d'un plan neutre fixe. Contrairement a "
     "Pivoter, il accepte PLUSIEURS faces avec le meme angle (les quatre "
     "parois d'une boite) et fonctionne aussi sur les faces cylindriques et "
     "coniques : un cylindre se demoule en cone.",
     "Auswurfwinkel fuer Formguss oder Druck: Neigen Sie die gewaehlten "
     "Flaechen um einen Winkel bezueglich einer festen neutralen Ebene. Anders "
     "als Drehen nimmt er MEHRERE Flaechen mit einem Winkel an (alle vier "
     "Waende eines Kastens) und wirkt auch auf zylindrische und kegelfoermige "
     "Flaechen - aus einem Zylinder wird ein Kegel.",
     "Angolo di sformo per stampaggio o stampa: inclina le facce selezionate "
     "di un angolo rispetto a un piano neutro fisso. Diversamente da Ruota, "
     "accetta PIU' facce con lo stesso angolo (tutte e quattro le pareti di "
     "una scatola) e funziona anche su facce cilindriche e coniche: un "
     "cilindro si sforma in un cono.",
     "用于开模或打印的脱模角：让所选的面绕固定的中间平面"
     "倾斜一定角度。与「旋转」不同，它可以对多个面施加同一"
     "角度（例如长方体的四个侧面），对圆柱面和锥面同样"
     "有效：圆柱会脱模成圆锥。"),
    ("Project a sketch onto this face along the sketch's normal, then "
     "engrave (cut in) or emboss (raise out) to a depth - wrap a logo "
     "or text onto a cylinder. Sketch, mode and depth in the popup; "
     "click sketch regions in the viewport to project only those.",
     "Proyecta un boceto sobre esta cara segun la normal del boceto y luego "
     "graba (hacia dentro) o realza (hacia fuera) con una profundidad: "
     "asienta un logotipo o texto sobre un cilindro. Boceto, modo y "
     "profundidad en el dialogo; haz clic en las regiones del boceto en la "
     "vista para proyectar solo esas.",
     "Projete um esboco nesta face ao longo da normal do esboco e depois "
     "grave (para dentro) ou relevo (para fora) ate uma profundidade - "
     "aplique um logotipo ou texto sobre um cilindro. Esboco, modo e "
     "profundidade na janela; clique nas regioes do esboco na janela de "
     "visualizacao para projetar somente essas.",
     "Projetez un plan sur cette face le long de sa normale, puis gravez "
     "(vers l'interieur) ou relevez (vers l'exterieur) sur une profondeur - "
     "habillez un logo ou un texte sur un cylindre. Plan, mode et profondeur "
     "dans la boite de dialogue ; cliquez les regions du plan dans la vue "
     "pour ne projeter que celles-ci.",
     "Projizieren Sie eine Skizze entlang ihrer Normalen auf diese Flaeche und "
     "gravieren Sie dann (einschneiden) oder praegen Sie (hervorheben) bis zu "
     "einer Tiefe - so bringen Sie ein Logo oder einen Text auf einen Zylinder. "
     "Skizze, Modus und Tiefe im Dialog; klicken Sie in der Ansicht auf "
     "Skizzenbereiche, um nur diese zu projizieren.",
     "Proietta uno schizzo su questa faccia lungo la sua normale, poi incidi "
     "(verso l'interno) o rilieva (verso l'esterno) fino a una profondita: "
     "applica un logo o un testo su un cilindro. Schizzo, modalita e "
     "profondita nella finestra; fai clic sulle regioni dello schizzo nella "
     "vista per proiettare solo quelle.",
     "沿草图法线把草图投影到该面上，然后按给定深度"
     "雕刻（切入）或浮雕（凸起），可把商标或文字"
     "包到圆柱面上。草图、模式和深度在弹窗中设置；"
     "在视口中点击草图区域可只投影所选区域。"),
    ("Resize a cylindrical hole / pin to an exact diameter.",
     "Redimensiona un agujero / pasador cilindrico a un diametro exacto.",
     "Redimensione um furo / pino cilindrico a um diametro exato.",
     "Redimensionnez un trou / une goupille cylindrique a un diametre exact.",
     "Bohren Sie eine runde Bohrung / einen Zapfen auf einen genauen "
     "Durchmesser.",
     "Ridimensiona un foro / perno cilindrico a un diametro esatto.",
     "把圆柱孔或圆柱销调整为精确直径。"),
    ("Cut a helical screw thread into the picked cylindrical face - "
     "external on a boss, internal in a hole. Pitch / depth / handedness "
     "in the popup.",
     "Roca una rosca helicoidal en la cara cilindrica seleccionada: "
     "externa en un resalte, interna en un agujero. Paso / profundidad / "
     "sentido en el dialogo.",
     "Corte uma rosca helicoidal na face cilindrica selecionada: externa em "
     "um ressalto, interna em um furo. Passo / profundidade / sentido na janela.",
     "Taillez un filetage helicoidal sur la face cylindrique choisie - exterieur "
     "sur un bossage, interieur dans un trou. Pas / profondeur / sens dans la "
     "boite de dialogue.",
     "Schneiden Sie ein Gewinde in die gewaehlte zylindrische Flaeche - "
     "aussen auf einem Vorsprung, innen in einer Bohrung. Steigung / Tiefe / "
     "Richtung im Dialog.",
     "Rettifica un filetto elicoidale sulla faccia cilindrica scelta: esterno "
     "su un rialzo, interno in un foro. Passo / profondita / senso nella "
     "finestra.",
     "在所选圆柱面上切出螺旋螺纹：外螺纹用于凸台，"
     "内螺纹用于孔。螺距、深度和旋向在弹窗中设置。"),
    ("Flatten the SELECTED faces into a 2D pattern (cut + fold lines) for a "
     "laser/CNC/printed template. Pick the faces of one panel (e.g. a skin) "
     "- unfolding a whole closed body rarely makes sense.",
     "Aplana las caras SELECCIONADAS en un patron 2D (lineas de corte y "
     "plegado) para una plantilla laser/CNC/impresa. Elige las caras de un "
     "panel (p. ej. una piel): desplegar un cuerpo cerrado entero rara vez "
     "tiene sentido.",
     "Achate as faces SELECIONADAS em um padrao 2D (linhas de corte e dobra) "
     "para um gabarito laser/CNC/impresso. Escolha as faces de um painel (ex. "
     "uma pele) - abrir um corpo fechado inteiro raramente faz sentido.",
     "Aplatissez les faces SELECTIONNEES en patron 2D (traits de coupe et de "
     "pliage) pour un gabarit laser/CNC/imprime. Choisissez les faces d'un "
     "panneau (par ex. une peau) - deplier un solide ferme entier n'a guere de "
     "sens.",
     "Flachen Sie die AUSGEWAEHLTEN Flaechen zu einem 2D-Muster ab "
     "(Schnitt- und Faltlinien) als Laser-/CNC-/Druckvorlage. Waehlen Sie die "
     "Flaechen eines Panels (z. B. einer Haut) - ein vollstaendig geschlossener "
     "Koerper laesst sich selten sinnvoll abwickeln.",
     "Appiattisci le facce SELEZIONATE in un modello 2D (linee di taglio e di "
     "piegatura) per un dima laser/CNC/stampato. Scegli le facce di un "
     "pannello (es. una pelle): sviluppare un solido chiuso intero ha poco "
     "senso.",
     "把「所选的面」展平为二维展开图（含切割线和"
     "折线），用作激光/CNC/打印样板。请选择同一个"
     "面板的面（例如蒙皮）；把整个封闭实体展平"
     "通常没有意义。"),
    ("Delete the picked face(s) and heal the surrounding faces back "
     "together - take a baked fillet/chamfer back to a sharp edge so "
     "it can be re-applied, or clean a round/hole off an imported part.",
     "Elimina las caras seleccionadas y une de nuevo las caras vecinas: "
     "devuelve un redondeo o chaflan ya aplicado a una arista viva para "
     "poder reaplicarlo, o limpia un redondeo o agujero de una pieza "
     "importada.",
     "Exclua as faces selecionadas e una novamente as faces vizinhas: "
     "devolve um arredondamento ou chanfro ja aplicado a uma aresta viva "
     "para poder reaplicar, ou limpa um arredondamento/furo de uma peca "
     "importada.",
     "Supprimez les faces choisies et reunissez les faces voisines : "
     "retransformez un congediement ou un chanfrein deja applique en arete "
     "vive pour pouvoir le reappliquer, ou supprimez un arrondi/trou sur une "
     "piece importee.",
     "Loeschen Sie die gewaehlten Flaechen und heilen Sie die benachbarten "
     "Flaechen wieder zusammen - machen Sie ein bereits angewandtes "
     "Verrunden/Fasen zur lebenden Kante zurueck, um es erneut anwenden zu "
     "koennen, oder entfernen Sie eine Rundung/Offnung aus einem importierten "
     "Bauteil.",
     "Elimina le facce scelte e ricucisce le facce adiacenti: riporta uno "
     "smusso o raccordo gia applicato a uno spigolo vivo per poterlo "
     "riapplicare, oppure elimina uno svasamento/foro da un pezzo importato.",
     "删除所选的面并把周边面重新缝合，可把已"
     "烘焙的圆角/倒角恢复成锐边以便重新施加，也"
     "可清除导入零件上的圆角或孔。"),
    ("Merge faces that are really one surface into one face. Imported "
     "STEP parts arrive with flat surfaces split into pieces - the seam "
     "lines across an otherwise flat face, which also confuse Unfold and "
     "sketch-on-face.",
     "Fusiona caras que en realidad son una sola superficie en una cara. Las "
     "piezas STEP importadas llegan con superficies planas divididas en "
     "trozos: las lineas de union en una cara por lo demas plana, que "
     "confunden tambien a Desplegar y a Boceto sobre cara.",
     "Una faces que sao realmente uma unica superficie em uma so face. Pecas "
     "STEP importadas chegam com superficies planas divididas em pedacos - as "
     "linhas de emenda numa face de resto plana, que tambem confundem Abrir e "
     "Esboco sobre face.",
     "Fusionnez des faces qui ne sont reellement qu'une seule surface en une "
     "seule face. Les pieces STEP importees arrivent avec des surfaces planes "
     "decoupees en morceaux - les lignes de raccord sur une face par ailleurs "
     "plane, qui perturbent aussi Deplier et Dessin sur face.",
     "Fassen Sie Flaechen, die in Wahrheit eine einzige Flaeche sind, zu einer "
     "Flaeche zusammen. Importierte STEP-Teile kommen mit ebenen Flaechen vor, "
     "die in Stuecke zerteilt sind - die Stosslinien quer ueber einer sonst "
     "ebenen Flaeche, die auch Abwickeln und Skizze-auf-Flaeche verwirren.",
     "Unisci in una sola faccia le superfici che in realta ne sono una sola. I "
     "pezzi STEP importati arrivano con superfici piane spezzate in parti - le "
     "linee di giunzione su una faccia altrimenti piana, che confondono "
     "anche Sviluppa e Schizzo su faccia.",
     "把实际上是同一张曲面的面合并为一张面。导入的"
     "STEP 零件常常把平面拆成若干块，那些横穿"
     "平坦面的接缝线也会干扰「展开」和"
     "「在面上画草图」。"),

    # ── Sketch toolbar labels ────────────────────────────────────────────
    ("Select / Move",
     "Seleccionar / Mover",
     "Selecionar / Mover",
     "Selectionner / Deplacer",
     "Auswaehlen / Verschieben",
     "Seleziona / Sposta",
     "选择/移动"),
    ("Trim",
     "Recortar",
     "Recortar",
     "Ajuster",
     "Trimmen",
     "Rifila",
     "修剪"),

    # ── Touch-gesture reference table (row(keysIn=)) ─────────────────────
    ("Alt+Drag",
     "Alt+Arrastrar",
     "Alt+Arraste",
     "Alt+Glisser",
     "Alt+Ziehen",
     "Alt+Trascina",
     "Alt+拖动"),
    ("Drag empty space",
     "Arrastrar espacio vacio",
     "Arraste espaco vazio",
     "Glisser sur le vide",
     "Leeren Bereich ziehen",
     "Trascina spazio vuoto",
     "拖动空白区域"),
    ("Box-select",
     "Seleccion por recuadro",
     "Selecao por caixa",
     "Selection au rectangle",
     "Rechteckauswahl ziehen",
     "Selezione a rettangolo",
     "框选"),
]


NEW = zh_only(NEW)
