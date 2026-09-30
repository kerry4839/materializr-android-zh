# -*- coding: utf-8 -*-
"""Batch 11: the four strings tools/i18n_extract.py reported once it was fixed
to actually scan something.

Two separate tool bugs hid these, and both are worth recording because the
symptom in each case was a clean bill of health:

  1. i18n_extract.py took its file list from sys.argv[1:] with no default. Run
     bare - which is how it was always run - it iterated an EMPTY list and
     printed "# 0 untranslated strings" while looking at nothing. A pass that
     reports success without reading the source is worse than no pass. It now
     walks src/ when given no files, and exits non-zero on any hit.

  2. i18n_extract.py's first-argument pattern required a literal to START the
     argument, so `ImGui::BeginMenu(MZ_ICON_PRIMITIVE "  Primitive")` was
     invisible: the argument opens with a macro, not a quote. The two create-menu
     headers below were shipping in English because of it. There is now a
     MACRO_ARG pattern for that shape.

So: three of these four were always-reported-by-nobody, and the fourth pair
(Primitive / Construction) was reported by nobody for a different reason. Both
tool fixes are in i18n_extract.py; this batch is only the translations.

Keys here are the WHOLE joined literal run, byte for byte, including the
trailing period - see the note at the top of catalogue_entries_8.py.
"""

NEW = [
    # ── AI Assistant settings: how to point the app at a local model server ─
    ("Point the base URL at http://localhost:11434/v1 for Ollama, or LM Studio's local server address, to run without any cloud key.",
     "Apunte la URL base a http://localhost:11434/v1 para Ollama, o a la dirección del servidor local de LM Studio, para funcionar sin ninguna clave en la nube.",
     "Aponte o URL base para http://localhost:11434/v1 no Ollama, ou para o endereço do servidor local do LM Studio, para funcionar sem nenhuma chave na nuvem.",
     "Renseignez l'URL de base sur http://localhost:11434/v1 pour Ollama, ou sur l'adresse du serveur local de LM Studio, pour fonctionner sans clé cloud.",
     "Setzen Sie die Basis-URL auf http://localhost:11434/v1 für Ollama oder auf die Adresse des lokalen LM-Studio-Servers, um ohne Cloud-Schlüssel zu arbeiten.",
     "Imposta l'URL di base su http://localhost:11434/v1 per Ollama, oppure sull'indirizzo del server locale di LM Studio, per funzionare senza chiavi cloud."),

    # ── AI Assistant: the pre-key warning ─────────────────────────────────
    ("Set up your API key in Settings -> AI Assistant.",
     "Configure su clave de API en Ajustes -> Asistente de IA.",
     "Configure a sua chave de API em Definições -> Assistente de IA.",
     "Configurez votre clé d'API dans Paramètres -> Assistant IA.",
     "Richten Sie Ihren API-Schlüssel unter Einstellungen -> KI-Assistent ein.",
     "Configura la tua chiave API in Impostazioni -> Assistente IA."),

    # ── Align (Mate): why Fasten is the only type these bodies can use ──────
    ("Only Fasten is available: these bodies have no sketch-derived faces to align to. Primitive and imported bodies cannot carry face references.",
     "Solo está disponible Fijar: estos cuerpos no tienen caras derivadas de bocetos onto los que alinear. Los cuerpos primitivos e importados no pueden llevar referencias de cara.",
     "Só Fixar está disponível: estes corpos não têm caras derivadas de esboços às quais alinhar. Corpos primitivos e importados não podem transportar referências de face.",
     "Seul Fixage est disponible : ces corps n'ont pas de faces issues d'esquisses auxquelles s'aligner. Les corps primitifs et importés ne peuvent pas porter de références de face.",
     "Nur Fixieren ist verfügbar: Diese Körper haben keine aus Skizzen abgeleiteten Flächen, mit denen sie ausgerichtet werden könnten. Primitive und importierte Körper können keine Flächenreferenzen tragen.",
     "È disponibile solo Fissa: questi corpi non hanno facce derivate da schizzi su cui allinearsi. I corpi primitivi e importati non possono portare riferimenti di faccia."),

    # ── Align (Mate): why Flip is disabled without anchored faces ──────────
    ("Only available with anchored faces: a rotation or flip recaptured from this body's current (already-rotated) position, instead of a real face's orientation, would silently compound on the next solve.",
     "Solo disponible con caras ancladas: una rotación o volteo recapturado desde la posición actual de este cuerpo (ya rotada), en lugar de la orientación de una cara real, se acumularía silenciosamente en la siguiente resolución.",
     "Só disponível com caras ancoradas: uma rotação ou inversão capturada da posição atual deste corpo (já rodada), em vez da orientação de uma cara real, acumular-se-ia silenciosamente na próxima resolução.",
     "Disponible uniquement avec des faces ancrées : une rotation ou un retournement recapturé depuis la position actuelle de ce corps (déjà tournée), plutôt que depuis l'orientation d'une vraie face, se cumulerait silencieusement à la résolution suivante.",
     "Nur mit verankerten Flächen verfügbar: Eine Drehung oder ein Spiegeln, das aus der aktuellen (bereits gedrehten) Lage dieses Körpers erfasst würde statt aus der Ausrichtung einer echten Fläche, würde sich bei der nächsten Lösung stillschweigend aufsummieren.",
     "Disponibile solo con facce ancorate: una rotazione o un ribaltamento ricatturato dalla posizione attuale di questo corpo (già ruotato), anziché dall'orientamento di una faccia reale, si accumulerebbe silenziosamente alla risoluzione successiva."),
]
