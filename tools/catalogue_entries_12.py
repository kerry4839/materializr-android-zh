# -*- coding: utf-8 -*-
"""Batch 12: the five primitive-popup window titles.

A fourth blind spot, found by tools/check_ui_string_arrays.py: the titles live
in `const char* titles[]` and reach ImGui as `ImGui::Begin(titles[k])`. No
existing checker can see that shape - i18n_extract.py needs a literal as the
first argument, check_tr_coverage.py only audits tr() sites, and
check_specifiers.py never reads the source. The window said "New Box" in a build
that three other tools were reporting as fully translated.

The fix is at the use site (ImGui::Begin(tr(titles[k]))) rather than in the
array, because the array doubles as the switch's index order. tr() returns its
input unchanged for a key it does not know, so a missing row degrades to English
instead of to a blank title bar.

Note these are keys, not a format string, so tr() - not trFormat() - is right.
"""

NEW = [
    ("New Box", "Nuevo cubo", "Novo cubo", "Nouveau pavé", "Neuer Quader", "Nuovo cubo"),
    ("New Cylinder", "Nuevo cilindro", "Novo cilindro", "Nouveau cylindre", "Neuer Zylinder", "Nuovo cilindro"),
    ("New Sphere", "Nueva esfera", "Nova esfera", "Nouvelle sphère", "Neue Kugel", "Nuova sfera"),
    ("New Cone", "Nuevo cono", "Novo cone", "Nouveau cône", "Neuer Kegel", "Nuovo cono"),
    ("New Torus", "Nuevo toro", "Novo toro", "Nouveau tore", "Neuer Torus", "Nuovo toro"),
]
