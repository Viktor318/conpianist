"""Reading the music symbols of a PDF made by Finale: they are characters of a music font
(Maestro layout: Kousaku, Maestro) and of a dynamics font (Chaconne), so the character code
tells what the symbol is. The exact outline box of each character comes from the embedded font."""
import io
import logging
from .recognize import Glyph

MUSIC_FONTS = ('Kousaku', 'Maestro')
DYN_FONTS = ('Chaconne',)

# Maestro layout: character -> symbol name used by the recognizer
MAESTRO = {
    'œ': 'head_black', '˙': 'head_half', 'w': 'head_whole',
    '&': 'clef_g', '?': 'clef_f', 'c': 'time_common', 'C': 'time_cut',
    '#': 'sharp', 'b': 'flat', 'n': 'natural',
    'Œ': 'rest_q', '‰': 'rest_8', '≈': 'rest_16', 'Ó': 'rest_half', '∑': 'rest_whole',
    '.': 'dot', 'j': 'flag_up', 'J': 'flag_down', 'r': 'flag_up2', 'R': 'flag_down2',
    '>': 'accent', 'U': 'fermata', 'u': 'fermata', '^': 'marcato', 'v': 'marcato', 'g': 'arpeggio',
    '⋲': 'rest_16', '®': 'rest_32', '°': 'ped', '*': 'ped_up',
    'd': 'stacciss', '´': 'stacciss', '`': 'trill', '‹': 'dsharp', '∫': 'dflat',
    '-': 'tenuto', '<': 'tenuto', '¯': 'tenuto', '˘': 'accent', 'ﬂ': 'accent',
    # known symbols the recognizer does not use yet
    '~': 'trill_line', '√': 'ottava', '(': 'paren_l', ')': 'paren_r', 'Ä': 'paren_l', 'Ö': 'paren_r',
    'T': 'turn', 'm': 'mordent', '¿': 'head_x', 'O': 'swell', 'K': 'flag_x', 'Ô': 'flag_x',
}
# characters PyMuPDF gives another Unicode value for than the Mac Roman one: their codes in the font
CODES = {'⋲': 0xc5}
DYNAMICS = {'p': 'p', 'f': 'f', 'P': 'mp', 'F': 'mf', 'S': 'sf', 'Z': 'sfz', 'π': 'pp', 'ƒ': 'ff', '∏': 'ppp', 'Í': 'fp'}


def is_finale_page(page):
    return any(any(n in f[3] for n in MUSIC_FONTS) for f in page.get_fonts())


def _boxes(doc, page):
    """(font base name, character code) -> outline box in em units (x0, y0, x1, y1; y upwards)."""
    from fontTools.ttLib import TTFont
    out = {}
    for f in page.get_fonts():
        base = f[3].split('+')[-1]
        if not any(n in base for n in MUSIC_FONTS + DYN_FONTS):
            continue
        try:
            buf = doc.extract_font(f[0])[3]
            # (fontTools warns about harmless flaws of the embedded font subsets, e.g. "2 extra bytes in post.stringData")
            logging.getLogger('fontTools').setLevel(logging.ERROR)
            tt = TTFont(io.BytesIO(buf), fontNumber=0)
            upm = tt['head'].unitsPerEm; glyf = tt['glyf']
            for table in tt['cmap'].tables:
                for code, gname in table.cmap.items():
                    g = glyf[gname]
                    if getattr(g, 'numberOfContours', 0) == 0:
                        continue
                    out.setdefault((base, code), (g.xMin / upm, g.yMin / upm, g.xMax / upm, g.yMax / upm))
        except Exception:
            continue
    return out


def _code(ch):
    if ch in CODES:
        return CODES[ch]
    try:
        return ch.encode('mac_roman')[0]
    except Exception:
        return ord(ch)


def glyphs(page, log=None):
    """The music symbols of the page as glyphs with their names; unknown characters are counted."""
    doc = page.parent
    boxes = _boxes(doc, page)
    out, unknown, dyn = [], {}, []
    for t in page.get_texttrace():
        base = t['font'].split('+')[-1]
        size = t['size']
        music = any(n in base for n in MUSIC_FONTS)
        if not music and not any(n in base for n in DYN_FONTS):
            # the number of a tuplet: a small digit in a text font
            if 'Helvetica' in base and size < 9.5:
                for u, gid, origin, bbox in t['chars']:
                    if chr(u) in '236':
                        out.append(Glyph('tuplet' + chr(u), bbox[0], origin[1] - 0.72 * size, bbox[2], origin[1], 'font'))
            continue
        for u, gid, origin, bbox in t['chars']:
            ch = chr(u)
            if ch.isspace():
                continue
            b = boxes.get((base, _code(ch))) or boxes.get((base, u))
            if b is None:
                x0, y0, x1, y1 = bbox
            else:
                x0, x1 = origin[0] + b[0] * size, origin[0] + b[2] * size
                y0, y1 = origin[1] - b[3] * size, origin[1] - b[1] * size
            if not music:
                dyn.append((x0, y0, x1, y1, ch)); continue
            if ch.isdigit():
                name = 'd' + ch
            else:
                name = MAESTRO.get(ch)
            if name is None:
                unknown[ch] = unknown.get(ch, 0) + 1
                name = '?'
            g = Glyph(name, x0, y0, x1, y1, 'font'); g.char = ch; g.size = size
            out.append(g)
    # dynamics: neighbouring letters make one mark (p + p = pp)
    dyn.sort(key=lambda d: (round(d[1] / 3), d[0]))
    i = 0
    while i < len(dyn):
        grp = [dyn[i]]; i += 1
        while i < len(dyn) and abs(dyn[i][1] - grp[-1][1]) < 2 and dyn[i][0] - grp[-1][2] < 3:
            grp.append(dyn[i]); i += 1
        txt = ''.join(DYNAMICS.get(d[4], '?') for d in grp)
        name = 'dyn_' + txt if '?' not in txt else '?'
        if '?' in txt:
            for d in grp: unknown['dyn:' + d[4]] = unknown.get('dyn:' + d[4], 0) + 1
        g = Glyph(name, min(d[0] for d in grp), min(d[1] for d in grp), max(d[2] for d in grp), max(d[3] for d in grp), 'font')
        out.append(g)
    # the same symbol printed twice at the same place counts once (not the noteheads: two voices on one note)
    seen, uniq = set(), []
    for g in out:
        key = (g.name, round(g.x0, 1), round(g.y0, 1))
        if not g.name.startswith('head') and key in seen:
            continue
        seen.add(key); uniq.append(g)
    out = uniq
    if log and unknown:
        log('  ! ismeretlen karakterek: ' + ', '.join(f"{k!r} (U+{ord(k[-1]):04X}) x{v}" for k, v in sorted(unknown.items())))
    return out
