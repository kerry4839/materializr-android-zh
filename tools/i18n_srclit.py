"""Shared: turn a C string-literal fragment from source text into the real
string the compiler will produce.

The source may write a character as an escape -- "Export STL\xE2\x80\xA6" is an
ellipsis -- while the catalogue holds the real character. Without decoding, the
key never matches and the string silently stays English.
"""
import re

_ESC = {'n': '\n', 't': '\t', 'r': '\r', '0': '\0',
        '"': '"', "'": "'", '\\': '\\', 'a': '\a', 'b': '\b', 'f': '\f', 'v': '\v'}


def decode(src):
    """Decode one literal's SOURCE text (without surrounding quotes)."""
    out = bytearray()
    i = 0
    while i < len(src):
        c = src[i]
        if c != '\\':
            out.extend(c.encode('utf-8'))
            i += 1
            continue
        if i + 1 >= len(src):
            out.extend(b'\\')
            break
        n = src[i + 1]
        if n == 'x':
            m = re.match(r'\\x([0-9a-fA-F]{1,2})', src[i:])
            if m:
                out.append(int(m.group(1), 16))
                i += m.end()
                continue
        if n in _ESC:
            out.extend(_ESC[n].encode('utf-8'))
            i += 2
            continue
        out.extend(n.encode('utf-8'))
        i += 2
    return out.decode('utf-8', errors='replace')
