# -*- coding: utf-8 -*-
"""Batch 16: the properties panel's surface/curve type names, and toolbar labels.

The same shape as batch 15, found by the same tool: a switch assigns English
literals into a local `const char* kind`, and the variable is passed to
ImGui::TextColored some lines later. The literal is nowhere near a tr() call, in
no argument list, and in no array - so all five earlier checkers were blind to
nine surface names and four curve names that render in the selection properties
panel every single time the user clicks a face or an edge.

This is now the third batch to come out of check_op_names.py, and the pattern
it keeps finding is worth stating once: splitting a decision from its display is
good style and terrible for localisation, and it is invisible to every checker
whose input is a literal at a known position. The tool reports rather than
forbids, because the same shape is legitimately used for data - a URL, a
filesystem path, a cache key - and those must not be translated.
"""


NEW = [
    # ── Surface kinds (PropertiesPanel.cpp face selection). BRepAdaptor_Surface
    #    ::GetType() enum, one word each. "Revolved" is the surface of
    #    revolution, not a rotation of an existing face. ─────────────────────
    ("Planar face",      "Cara plana",        "Face plana",       "Face plane",       "Planare Fläche",    "Faccia piana"),
    ("Cylindrical face", "Cara cilíndrica",   "Face cilíndrica",  "Face cylindrique", "Zylindrische Fläche", "Faccia cilindrica"),
    ("Conical face",     "Cara cónica",       "Face cônica",      "Face conique",     "Kegelfläche",      "Faccia conica"),
    ("Spherical face",   "Cara esférica",     "Face esférica",    "Face sphérique",   "Kugelfläche",      "Faccia sferica"),
    ("Toroidal face",    "Cara toroidal",     "Face toroidal",    "Face torique",     "Torusfläche",      "Faccia toroidale"),
    ("Extruded face",    "Cara extruida",     "Face extrudada",   "Face extrudée",    "Extrusionsfläche", "Faccia estrusa"),
    ("Revolved face",    "Cara revolución",   "Face de revolução","Face de révolution","Rotationsfläche", "Faccia di rivoluzione"),
    ("Freeform face",    "Cara libre",        "Face livre",       "Face libre",       "Freiformfläche",   "Faccia libera"),

    # ── Curve kinds (edge selection) ────────────────────────────────────────
    ("Straight edge",    "Arista recta",      "Aresta reta",      "Arête droite",     "Gerade Kante",     "Spigolo dritto"),
    ("Circular edge",    "Arista circular",   "Aresta circular",  "Arête circulaire", "Kreiskante",       "Spigolo circolare"),
    ("Elliptical edge",  "Arista elíptica",   "Aresta elíptica",  "Arête elliptique", "Ellipsenkante",    "Spigolo ellittico"),
    ("Curved edge",      "Arista curva",      "Aresta curva",     "Arête courbe",     "Gebogene Kante",   "Spigolo curvo"),

    # ── Toolbar: sketch draw-origin toggle ─────────────────────────────────
    ("Draw from: Corner",  "Dibujar desde: Esquina", "Desenhar de: Canto",   "Dessiner depuis : Coin", "Zeichnen ab: Ecke",     "Disegna da: Angolo"),
    ("Draw from: Center",  "Dibujar desde: Centro",  "Desenhar de: Centro",  "Dessiner depuis : Centre","Zeichnen ab: Mitte",    "Disegna da: Centro"),
    ("Draw from: 2-Point", "Dibujar desde: 2 Puntos","Desenhar de: 2 Pontos","Dessiner depuis : 2 points","Zeichnen ab: 2 Punkten","Disegna da: 2 punti"),
]