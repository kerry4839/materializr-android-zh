#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Update i18n_catalogue.py with Chinese translations."""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))

from i18n_catalogue import CAT
from zh_translations import get_zh_translation

# Read the original file
with open('tools/i18n_catalogue.py', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the CAT list and update entries with Chinese translations
# We'll rebuild the CAT list with 7 arguments (adding Chinese)

new_cat_lines = []
for en, tr_dict in CAT:
    zh = get_zh_translation(en)
    # Escape for Python string
    en_escaped = en.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    es_escaped = tr_dict.get('es', '').replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    pt_escaped = tr_dict.get('pt', '').replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    fr_escaped = tr_dict.get('fr', '').replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    de_escaped = tr_dict.get('de', '').replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    it_escaped = tr_dict.get('it', '').replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    zh_escaped = zh.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
    
    new_cat_lines.append(f'    E("{en_escaped}", "{es_escaped}", "{pt_escaped}", "{fr_escaped}", "{de_escaped}", "{it_escaped}", "{zh_escaped}"),')

# Replace the CAT list in the file
# Find the start and end of CAT
start_marker = 'CAT = ['
end_marker = ']'

start_idx = content.find(start_marker)
if start_idx == -1:
    print("ERROR: CAT start marker not found")
    sys.exit(1)

# Find the matching closing bracket
bracket_count = 0
end_idx = start_idx
for i, ch in enumerate(content[start_idx:], start_idx):
    if ch == '[':
        bracket_count += 1
    elif ch == ']':
        bracket_count -= 1
        if bracket_count == 0:
            end_idx = i
            break

if bracket_count != 0:
    print("ERROR: Could not find matching bracket")
    sys.exit(1)

# Build new content
new_content = content[:start_idx + len(start_marker)] + '\n' + '\n'.join(new_cat_lines) + '\n' + content[end_idx:]

# Write back
with open('tools/i18n_catalogue.py', 'w', encoding='utf-8') as f:
    f.write(new_content)

print("Updated i18n_catalogue.py with Chinese translations")

# Now regenerate the header
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.exit(os.system('python3 tools/i18n_catalogue.py'))