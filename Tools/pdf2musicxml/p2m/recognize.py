"""Recognizing score elements from the vector shapes.

Steps per page:
  1. staff lines -> staves -> systems (two staves = piano)
  2. barlines -> measures
  3. identifying glyphs by the catalog (shape signature)
  4. notehead + stem + beam/flag + dot -> note/chord with duration
  5. rests, key signature, time signature, accidentals, fingering, dynamics
  6. voices and onsets within the measure
"""
import json
import os
from dataclasses import dataclass, field
from fractions import Fraction as F

from .extract import visible_shapes

CATALOG = json.load(open(os.path.join(os.path.dirname(__file__), 'catalog.json'), encoding='utf-8'))

DYN = ('pp', 'p', 'mp', 'mf', 'f', 'ff', 'ppp', 'sf')
DIGITS = {'d1': 1, 'd2': 2, 'd3': 3, 'd4': 4, 'd5': 5, 'd6': 6, 'd8': 8, 'd9': 9}


# ----------------------------------------------------------------- data model
@dataclass
class Glyph:
    name: str
    x0: float
    y0: float
    x1: float
    y1: float
    ops: str = ''
    used: bool = False
    src: object = None

    @property
    def w(self): return self.x1 - self.x0
    @property
    def h(self): return self.y1 - self.y0
    @property
    def cx(self): return (self.x0 + self.x1) / 2
    @property
    def cy(self): return (self.y0 + self.y1) / 2


@dataclass
class Staff:
    ys: list            # y coordinates of the 5 lines from top to bottom
    x0: float
    x1: float

    @property
    def sp(self): return (self.ys[4] - self.ys[0]) / 4
    @property
    def top(self): return self.ys[0]
    @property
    def bottom(self): return self.ys[4]

    def pos(self, y):
        """Step in half staff spaces from the bottom line (0 = bottom line, 8 = top line)."""
        return round((self.ys[4] - y) / (self.sp / 2))

    def dist(self, y):
        return 0 if self.top <= y <= self.bottom else min(abs(y - self.top), abs(y - self.bottom))


@dataclass
class Head:
    g: Glyph
    kind: str           # black / half / whole
    staff: int          # 0 = upper, 1 = lower
    pos: int
    accidental: str = None
    fingering: int = None
    fing_place: str = None
    dots: int = 0
    paren: bool = False
    grace: bool = False
    tie_start: bool = False
    tie_stop: bool = False


@dataclass
class Event:
    """Chord or rest."""
    x: float
    staff: int
    heads: list = field(default_factory=list)     # empty = rest
    rest: str = None
    dur: F = None
    stem: str = None    # up / down / None
    beams: int = 0
    dots: int = 0
    y: float = 0.0
    voice: int = 1
    onset: F = None
    beam_tags: dict = field(default_factory=dict)
    beam_ids: tuple = ()
    whole_measure: bool = False
    staccato: bool = False
    accent: bool = False
    fermata: bool = False
    grace: bool = False
    arpeggio: bool = False
    marcato: bool = False
    stacciss: bool = False
    tenuto: bool = False
    tuplet_start: bool = False
    tuplet_stop: bool = False
    tuplet: tuple = None
    slur_start: list = field(default_factory=list)
    slur_stop: list = field(default_factory=list)
    slur_place: dict = field(default_factory=dict)   # slur number -> 'above' / 'below' (side in the PDF)


@dataclass
class Measure:
    x0: float
    x1: float
    events: list = field(default_factory=list)
    clefs: dict = field(default_factory=dict)       # staff -> 'G'/'F' (at the start of the measure)
    key: int = None
    time: tuple = None                              # (beats, beat_type, symbol)
    directions: list = field(default_factory=list)  # (x, staff, type, value)
    left_bar: str = None
    right_bar: str = None
    length: F = None
    new_system: bool = False
    new_page: bool = False
    warnings: list = field(default_factory=list)
    volta: object = None


# --------------------------------------------------------- glyph identification
def _contours(shape):
    """Splitting a compound path into connected contours."""
    out, cur, last = [], [], None
    for it in shape.items:
        if it[0] in ('re', 'qu'):
            r = it[1] if it[0] == 're' else it[1].rect
            if cur: out.append(cur); cur = []
            out.append([('r', [(r.x0, r.y0), (r.x1, r.y1)])]); last = None
            continue
        pts = [(p.x, p.y) for p in it[1:]]
        if last is not None and (abs(pts[0][0] - last[0]) > 0.02 or abs(pts[0][1] - last[1]) > 0.02):
            out.append(cur); cur = []
        cur.append((it[0], pts)); last = pts[-1]
    if cur: out.append(cur)
    return out


def _bbox(conts):
    xs = [p[0] for c in conts for _, pts in c for p in pts]
    ys = [p[1] for c in conts for _, pts in c for p in pts]
    return min(xs), min(ys), max(xs), max(ys)


def _fix_six(g, conts):
    if g.name == 'd9' and len(conts) > 1:
        hole = min((_bbox([c]) for c in conts), key=lambda b: (b[2] - b[0]) * (b[3] - b[1]))
        if (hole[1] + hole[3]) / 2 > g.cy:
            g.name = 'd6'
    return g


def glyphs_from_shapes(shapes):
    """Filled shapes -> glyphs. Illustrator sometimes merges several symbols into one compound
    path, so identification is also tried contour by contour."""
    glyphs = []
    for s in shapes:
        if s.kind == 's':
            continue
        if s.ops in CATALOG:
            glyphs.append(_fix_six(Glyph(CATALOG[s.ops], s.x0, s.y0, s.x1, s.y1, s.ops, src=s), _contours(s) if CATALOG[s.ops] == 'd9' else [])); continue
        conts = _contours(s)
        if len(conts) == 1:
            glyphs.append(Glyph('?', s.x0, s.y0, s.x1, s.y1, s.ops, src=s)); continue
        i = 0
        while i < len(conts):
            for k in range(min(6, len(conts) - i), 0, -1):
                ops = ''.join(o for c in conts[i:i + k] for o, _ in c)
                if ops not in CATALOG:
                    continue
                if k > 1:   # symbol with several contours: the further contours are holes in the first
                    b = _bbox(conts[i:i + 1])
                    inner = all(b[0] - .1 <= (bb[0] + bb[2]) / 2 <= b[2] + .1 and
                                b[1] - .1 <= (bb[1] + bb[3]) / 2 <= b[3] + .1
                                for bb in (_bbox([c]) for c in conts[i + 1:i + k]))
                    if not inner:
                        continue
                glyphs.append(_fix_six(Glyph(CATALOG[ops], *_bbox(conts[i:i + k]), ops, src=s), conts[i:i + k])); i += k; break
            else:
                ops = ''.join(o for o, _ in conts[i])
                glyphs.append(Glyph('?', *_bbox(conts[i:i + 1]), ops, src=s)); i += 1
    return glyphs


TW, TH = 12, 16


def raster(page, g):
    """The bounding rectangle of the glyph as a TW x TH grey image (list of 0..255)."""
    import pymupdf
    r = pymupdf.Rect(g.x0, g.y0, g.x1, g.y1)
    if r.width < 0.5 or r.height < 0.5:
        return None
    pm = page.get_pixmap(clip=r, matrix=pymupdf.Matrix(TW * 2 / r.width, TH * 2 / r.height), colorspace=pymupdf.csGRAY)
    w, h, buf = pm.width, pm.height, pm.samples
    if w < 2 or h < 2:
        return None
    return [255 - buf[min(h - 1, int((j + 0.5) * h / TH)) * pm.stride + min(w - 1, int((i + 0.5) * w / TW))]
            for j in range(TH) for i in range(TW)]


TPL_NAMES = ('dyn_p', 'dyn_pp', 'dyn_ppp', 'dyn_mp', 'dyn_mf', 'dyn_f', 'dyn_ff', 'dyn_sf', 'ped', 'ped_up', 'fermata',
             'trill', 'clef_g', 'clef_f', 'rest_q', 'rest_8', 'rest_16', 'sharp', 'flat', 'natural', 'arpeggio')
_TPL = None


def raster_n(page, g, n=20):
    import pymupdf
    r = pymupdf.Rect(g.x0, g.y0, g.x1, g.y1)
    if r.width < 0.5 or r.height < 0.5:
        return None
    pm = page.get_pixmap(clip=r, matrix=pymupdf.Matrix(n * 3 / r.width, n * 3 / r.height), colorspace=pymupdf.csGRAY)
    w, h, buf = pm.width, pm.height, pm.samples
    if w < 3 or h < 3:
        return None
    out = []
    for j in range(n):
        for i in range(n):
            ys = range(int(j * h / n), max(int(j * h / n) + 1, int((j + 1) * h / n)))
            xs = range(int(i * w / n), max(int(i * w / n) + 1, int((i + 1) * w / n)))
            v = sum(255 - buf[y * pm.stride + x] for y in ys for x in xs) / (len(ys) * len(xs))
            out.append(v)
    return out


def match_templates(page, glyphs, sp, log):
    """Variants of known symbols drawn differently (other size, other contours), by image similarity."""
    global _TPL
    if _TPL is None:
        fn = os.path.join(os.path.dirname(__file__), 'templates.json')
        _TPL = json.load(open(fn)) if os.path.exists(fn) else {}
    if not _TPL:
        return
    import math
    for g in glyphs:
        if g.name != '?' or not (0.45 * sp < g.h < 9 * sp) or g.w > 6 * sp or g.w < 0.25 * sp:
            continue
        # part of a text? (neighbour of the same height right next to it)
        if any(o is not g and o.name in ('?',) and abs(o.y1 - g.y1) < 0.35 * sp and o.h > 0.5 * g.h and
               -0.2 * sp < max(o.x0 - g.x1, g.x0 - o.x1) < 0.35 * sp for o in glyphs):
            continue
        asp = g.w / g.h
        cands = [(n, t) for n, t in _TPL.items() if abs(math.log(asp / t['asp'])) < 0.22 and 0.6 < (g.h / sp) / t['hsp'] < 1.6]
        if not cands:
            continue
        v = raster_n(page, g)
        if v is None:
            continue
        sc = sorted((sum(abs(a - b) for a, b in zip(v, t['img'])) / len(v), n) for n, t in cands)
        if sc[0][0] < 40 and (len(sc) == 1 or sc[1][0] - sc[0][0] > 6):
            g.name = sc[0][1]; g.raster = True


def match_digits(page, glyphs, sp, log):
    tpl = CATALOG.get('_templates')
    if not tpl:
        return
    for g in glyphs:
        if g.name != '?' or not (0.55 * sp < g.w < 1.05 * sp and 1.05 * sp < g.h < 1.45 * sp):
            continue
        # unknown symbol as horizontal neighbour = part of a text (e.g. tempo number), not fingering
        if any(o is not g and o.name in ('?', 'parens', 'paren_l', 'paren_r', 'metro_q') and abs(o.cy - g.cy) < 0.6 * sp and
               -0.2 * sp < max(o.x0 - g.x1, g.x0 - o.x1) < 1.0 * sp for o in glyphs):
            continue
        v = raster(page, g)
        if v is None:
            continue
        sc = sorted((sum(abs(a - b) for a, b in zip(v, t)) / len(v), n) for n, t in tpl.items())
        if sc[0][0] < 38 and sc[1][0] - sc[0][0] > 8:
            g.name = sc[0][1]; g.raster = True


# ------------------------------------------------------------ page analysis
def find_staves(shapes):
    raw = [s for s in shapes if s.kind == 's' and s.ops == 'l' and s.h < 0.05 and s.w > 40 and s.width < 0.7]
    # line pieces at the same height (e.g. a Coda set apart at the end of the line) joined into one line
    raw.sort(key=lambda s: (round(s.y0, 1), s.x0))
    lines = []
    for s in raw:
        if lines and abs(lines[-1].y0 - s.y0) < 0.05:
            L = lines[-1]
            if s.x0 - L.x1 < 60:
                L.x1 = max(L.x1, s.x1); L.parts.append(s); continue
        s.parts = [s]
        lines.append(s)
    lines = [l for l in lines if l.x1 - l.x0 > 150]
    lines.sort(key=lambda s: s.y0)
    staves, i = [], 0
    while i + 4 < len(lines):
        grp = lines[i:i + 5]
        gaps = [grp[j + 1].y0 - grp[j].y0 for j in range(4)]
        if max(gaps) - min(gaps) < 0.15 * gaps[0] and 3 < gaps[0] < 9 and \
                max(abs(g.x0 - grp[0].x0) for g in grp) < 1:
            staves.append(Staff([g.y0 for g in grp], grp[0].x0, grp[0].x1))
            for g in grp:
                for part in g.parts: part.used = True
            i += 5
        else:
            i += 1
    return staves


class System:
    def __init__(self, upper, lower):
        self.staves = [upper, lower]
        self.sp = upper.sp
        self.x0, self.x1 = upper.x0, upper.x1
        self.top, self.bottom = upper.top, lower.bottom
        self.measures = []

    def staff_of(self, y):
        d0, d1 = self.staves[0].dist(y), self.staves[1].dist(y)
        return 0 if d0 <= d1 else 1


def analyse_page(page, log, jbars=None):
    shapes = visible_shapes(page)
    staves = find_staves(shapes)
    if len(staves) % 2:
        log(f"  ! paratlan szamu kottasor ({len(staves)}), az utolsot kihagyom")
    # systems: the vertical line along the left edge joins the staves (4 staves for piano duets),
    # within it the staff pairs are separate parts
    vlines = [x for x in shapes if x.kind == 's' and x.ops == 'l' and x.w < 0.05]
    groups, i = [], 0
    while i < len(staves):
        st = staves[i]
        span = [v for v in vlines if abs(v.x0 - st.x0) < 1.5 and abs(v.y0 - st.top) < 1.0]
        y1 = max((v.y1 for v in span), default=None)
        n = sum(1 for t in staves[i:] if y1 is not None and t.bottom <= y1 + 1.0) if y1 is not None else 0
        if n < 2 or n % 2:
            n = 2
        groups.append(staves[i:i + n]); i += n
    systems = []
    for gi, grp in enumerate(groups):
        for k in range(0, len(grp) - 1, 2):
            sy = System(grp[k], grp[k + 1]); sy.group = gi; sy.part = k // 2; sy.group_top = grp[0].top
            systems.append(sy)
    if jbars:
        W, H = page.rect.width, page.rect.height
        for sy in systems:
            sy.jbars = [dict(b, x0=b['x'] * W, x1=(b['x'] + b['w']) * W) for b in jbars
                        if b['y'] * H < sy.bottom - sy.sp and (b['y'] + b['h']) * H > sy.top + sy.sp]
    glyphs = glyphs_from_shapes(shapes)
    from . import finale as _fin
    if _fin.is_finale_page(page):
        # a PDF made by Finale: the symbols are characters of the music font, not outlines
        glyphs = [g for g in glyphs if g.name == '?'] + _fin.glyphs(page, log)
        for sy in systems: sy.finale = True
    elif staves:
        match_digits(page, glyphs, staves[0].sp, log)
        match_templates(page, glyphs, staves[0].sp, log)
    strokes = [s for s in shapes if s.kind == 's' and not s.used and not (s.w > 300 and s.h > 300)]
    fills = [s for s in shapes if s.kind != 's' and not (s.w > 300 and s.h > 300)]
    glyphs = [g for g in glyphs if not (g.w > 300 and g.h > 300)]
    peds = [g for g in glyphs if g.name in ('ped', 'ped_up')]
    if systems and getattr(systems[0], 'finale', False):
        # (scores made by Finale: their systems stand close to each other)
        # A stem belongs to the system of its notehead, a beam to the system of its stems: with notes far
        # below or above the staves their middle may be nearer to the neighbouring system.
        sp0 = systems[0].sp
        def sdist(y): return min(min(st.dist(y) for st in sy.staves) for sy in systems)
        vst = [s for s in strokes if s.ops == 'l' and s.w < 0.05 and s.h > 1.5 * sp0 and s.width < 0.75]
        for s in vst:
            s.zy = s.y0 if sdist(s.y0) <= sdist(s.y1) else s.y1
        for f in fills:
            if f.ops == 'lll' and f.w > 0.8 * sp0:
                zs = [s.zy for s in vst if f.x0 - 0.25 * sp0 <= s.x0 <= f.x1 + 0.25 * sp0 and
                      s.y0 - 0.3 * sp0 <= _beam_y(f, s.x0) <= s.y1 + 0.3 * sp0 and
                      min(abs(s.y0 - _beam_y(f, s.x0)), abs(s.y1 - _beam_y(f, s.x0))) < 2.5 * sp0]
                if zs:
                    f.zy = sorted(zs)[len(zs) // 2]
    for k, sy in enumerate(systems):
        lo = systems[k - 1].bottom if k else 0
        hi = systems[k + 1].top if k + 1 < len(systems) else page.rect.height
        ya = sy.top - min(0.62 * (sy.top - lo), 12 * sy.sp) if k else sy.top - 9 * sy.sp
        yb = sy.bottom + min(0.38 * (hi - sy.bottom), 9 * sy.sp)
        zone = lambda o: ya <= getattr(o, 'zy', o.cy) <= yb
        vzone = lambda o: zone(o) or (o.ops == 'l' and o.w < 0.05 and o.y0 < sy.top + 0.6 and o.y1 > sy.bottom - 0.6)
        # pedal marks always belong to the staves above them, however far below they were pushed by low notes
        # (with the small pieces next to them, e.g. the dot of "Ped.")
        gzone = lambda o: sy.bottom < o.cy < hi if any(p.x0 - sy.sp <= o.cx <= p.x1 + sy.sp and p.y0 - sy.sp <= o.cy <= p.y1 + sy.sp
                                                        for p in peds) else zone(o)
        _analyse_system(sy, [g for g in glyphs if gzone(g)], [s for s in strokes if vzone(s)],
                        [s for s in fills if gzone(s)], log)
    return systems, glyphs, shapes


def _analyse_system(sy, glyphs, strokes, fills, log):
    sp = sy.sp
    S = sy.staves
    # --- barlines: vertical lines spanning both staves
    bars = [s for s in strokes if s.ops == 'l' and s.w < 0.05 and
            s.y0 < sy.top + 0.6 and s.y1 > sy.bottom - 0.6 and s.y0 > sy.top - 30 * sp and s.h < 70 * sp]
    for s in strokes:   # marking the barline pieces of each staff
        if s.ops == 'l' and s.w < 0.05 and s.width > 0.7 and any(
                abs(s.y0 - st.top) < 0.6 and abs(s.y1 - st.bottom) < 0.6 for st in S):
            s.used = True
    for b in bars: b.used = True
    bars.sort(key=lambda s: s.x0)
    groups = []
    for b in bars:
        if groups and b.x0 - groups[-1][-1].x0 < 1.6 * sp:
            groups[-1].append(b)
        else:
            groups.append([b])
    rdots = [g for g in glyphs if g.name == 'dot' and g.w > 0.42 * sp and
             any(abs(g.cy - (st.ys[1] + st.ys[2]) / 2) < 0.2 * sp or abs(g.cy - (st.ys[2] + st.ys[3]) / 2) < 0.2 * sp
                 for st in S)]
    binfo = []
    for grp in groups:
        x = sum(b.x0 for b in grp) / len(grp)
        thick = [b for b in grp if b.width > 1.8]
        kind = 'regular'
        if thick and len(grp) > 1:
            left = [d for d in rdots if 0 < grp[0].x0 - d.cx < 1.8 * sp]
            right = [d for d in rdots if 0 < d.cx - grp[-1].x0 < 1.8 * sp]
            for d in left + right: d.used = True
            if left and right: kind = 'repeat-both'
            elif left: kind = 'repeat-end'
            elif right: kind = 'repeat-start'
            else: kind = 'final' if thick[0].x0 >= grp[0].x0 + 0.1 else 'heavy-light'
        elif len(grp) > 1:
            kind = 'double'
        binfo.append((grp[0].x0, grp[-1].x0, kind))
    if not binfo or binfo[0][0] - sy.x0 > 1.0 * sp:
        binfo.insert(0, (sy.x0, sy.x0, 'regular'))
    jb = sorted(getattr(sy, 'jbars', None) or [], key=lambda b: b['x0'])
    if jb:
        # measure boundaries from the json file (Yamaha score sync); the barline style from the PDF
        def kind_at(x):
            c = [g for g in binfo if g[0] - 1.6 * sp <= x <= g[1] + 1.6 * sp]
            return min(c, key=lambda g: min(abs(g[0] - x), abs(g[1] - x)))[2] if c else 'regular'
        real = []
        for k, b in enumerate(jb):
            if b['len'] <= 0:
                continue
            x0 = b['x0']
            if k > 0 and jb[k - 1]['len'] <= 0 and (k - 1 == 0 or jb[k - 2]['len'] > 0):
                x0 = jb[k - 1]['x0']           # the preceding zero-length part (clef/key signature) belongs to this measure
            m = Measure(x0, b['x1'])
            m.jlen, m.jid = b['len'], b.get('idx', b['id'])
            lk, rk = kind_at(b['x0']), kind_at(b['x1'])
            if lk in ('repeat-start', 'repeat-both'): m.left_bar = 'repeat-start'
            if rk in ('repeat-end', 'repeat-both'): m.right_bar = 'repeat-end'
            elif rk in ('final', 'double'): m.right_bar = rk
            real.append(m)
        if real and jb[-1]['len'] <= 0:
            real[-1].x1 = jb[-1]['x1']
            rk = kind_at(jb[-1]['x1'])
            if rk in ('repeat-end', 'repeat-both') and real[-1].right_bar is None: real[-1].right_bar = 'repeat-end'
            elif rk in ('final', 'double') and real[-1].right_bar is None: real[-1].right_bar = rk
        sy.measures.extend(real)
    else:
        for a, b in zip(binfo, binfo[1:]):
            m = Measure(a[1], b[0])
            if a[2] in ('repeat-start', 'repeat-both'): m.left_bar = 'repeat-start'
            if b[2] in ('repeat-end', 'repeat-both'): m.right_bar = 'repeat-end'
            elif b[2] in ('final', 'double'): m.right_bar = b[2]
            sy.measures.append(m)
    if not sy.measures:
        return
    sy.measures[0].new_system = True

    def measure_at(x):
        for m in sy.measures:
            if m.x0 - 0.3 * sp <= x < m.x1 - 0.1 * sp:
                return m
        return sy.measures[-1] if x >= sy.measures[-1].x0 else sy.measures[0]

    def near(v, ref, tol): return abs(v - ref) <= tol

    # --- noteheads
    heads = []
    for g in glyphs:
        if g.name in ('head_black', 'head_half', 'head_whole') and 0.9 * sp < g.h < 1.4 * sp:
            st = sy.staff_of(g.cy)
            heads.append(Head(g, g.name[5:], st, S[st].pos(g.cy))); g.used = True
        elif g.name == 'head_black' and 0.55 * sp < g.h <= 0.9 * sp and 0.6 * sp < g.w < 1.15 * sp:
            st = sy.staff_of(g.cy)
            if S[st].dist(g.cy) < 4.5 * sp:
                heads.append(Head(g, 'black', st, S[st].pos(g.cy), grace=True)); g.used = True
    # --- stems: thin vertical lines
    stems = [s for s in strokes if s.ops == 'l' and s.w < 0.05 and not s.used and s.width < 0.75 and s.h > 1.5 * sp]
    beams = [s for s in fills if (s.ops == 'lll' and s.w > 0.8 * sp) or
             (s.ops == 'r' and s.w > 1.6 * sp and 0.3 * sp < s.h < 0.62 * sp) or
             (s.ops == 'r' and s.w > 0.8 * sp and 0.3 * sp < s.h < 0.62 * sp and any(
                 s.x0 - 0.3 * sp <= t.x0 <= s.x1 + 0.3 * sp and t.y0 - 0.3 * sp <= s.cy <= t.y1 + 0.3 * sp for t in stems))]
    flags = [g for g in glyphs if g.name in ('flag_up', 'flag_down')]
    events = []
    by_stem = {}
    for h in heads:
        if h.kind == 'whole' or h.grace:
            continue
        best = None
        for i, s in enumerate(stems):
            if not (s.y0 - 0.6 * sp <= h.g.cy <= s.y1 + 0.6 * sp):
                continue
            d = min(abs(s.x0 - h.g.x0), abs(s.x0 - h.g.x1))
            if d < 0.22 * sp and (best is None or d < best[0]):
                best = (d, i)
        if best is None:
            ev = Event(h.g.x0, h.staff, [h]); events.append(ev)
            sy.measures[0].warnings.append(f"szar nelkuli fej x={h.g.x0:.0f}")
        else:
            by_stem.setdefault(best[1], []).append(h)
    for i, hs in by_stem.items():
        s = stems[i]; s.used = True
        hs.sort(key=lambda h: h.g.cy)
        up = (hs[0].g.cy - s.y0) > (s.y1 - hs[-1].g.cy)
        tip = s.y0 if up else s.y1
        # the column x of the chord: left edge of the heads on the "normal" side of the stem
        col = s.x0 - hs[0].g.w if up else s.x0
        ev = Event(col, hs[0].staff if up else hs[-1].staff, hs, stem='up' if up else 'down')
        ids = []
        for bi, b in enumerate(beams):
            if b.x0 - 0.25 * sp <= s.x0 <= b.x1 + 0.25 * sp and b.y0 - 0.3 * sp <= tip <= b.y1 + 0.3 * sp + (0 if b.ops == 'r' else 0):
                ids.append(bi)
            elif b.x0 - 0.25 * sp <= s.x0 <= b.x1 + 0.25 * sp and s.y0 - 0.2 * sp <= _beam_y(b, s.x0) <= s.y1 + 0.2 * sp \
                    and abs(_beam_y(b, s.x0) - tip) < 2.2 * sp:
                ids.append(bi)
        ev.beam_ids = tuple(ids); ev.beams = len(ids)
        if not ids:
            for f in flags:
                if abs(f.x0 - s.x0) < 0.3 * sp and (abs(f.y0 - tip) < 0.6 * sp or abs(f.y1 - tip) < 0.6 * sp):
                    ev.beams = 1; f.used = True
        ev.y = tip
        events.append(ev)
    # --- grace notes: small heads with a short stem (with or without a slash)
    gst = [s_ for s_ in strokes if s_.ops == 'l' and s_.w < 0.05 and not s_.used and s_.width < 0.75 and 0.9 * sp < s_.h < 4 * sp]
    gby = {}
    for h in [h for h in heads if h.grace]:
        best = None
        for i, s_ in enumerate(gst):
            if not (s_.y0 - 0.5 * sp <= h.g.cy <= s_.y1 + 0.5 * sp): continue
            d = min(abs(s_.x0 - h.g.x0), abs(s_.x0 - h.g.x1))
            if d < 0.25 * sp and (best is None or d < best[0]): best = (d, i)
        gby.setdefault(best[1] if best else ('n', id(h)), []).append(h)
    for key, hs in gby.items():
        hs.sort(key=lambda h: h.g.cy)
        ev = Event(min(h.g.x0 for h in hs), hs[0].staff, hs, grace=True)
        if not isinstance(key, tuple):
            s_ = gst[key]; s_.used = True
            up = (hs[0].g.cy - s_.y0) > (s_.y1 - hs[-1].g.cy)
            ev.stem = 'up' if up else 'down'; ev.y = s_.y0 if up else s_.y1
            ev.beams = 1
            for bi, b in enumerate(beams):
                if b.x0 - 0.25 * sp <= s_.x0 <= b.x1 + 0.25 * sp and abs(_beam_y(b, s_.x0) - ev.y) < 0.8 * sp:
                    ev.beams = max(ev.beams, 2 if any(o is not b and abs(o.x0 - b.x0) < 2 * sp and abs(o.cy - b.cy) < 1.2 * sp for o in beams) else 1)
            for f in flags:
                if abs(f.x0 - s_.x0) < 0.4 * sp and min(abs(f.y0 - ev.y), abs(f.y1 - ev.y)) < 0.8 * sp: f.used = True
            for t in strokes:      # slash
                if t.ops == 'l' and not t.used and 0.3 * sp < t.w < 2.5 * sp and 0.3 * sp < t.h < 2.5 * sp and \
                        abs(t.cx - s_.x0) < 1.5 * sp and s_.y0 - sp < t.cy < s_.y1 + sp:
                    t.used = True
        events.append(ev)
    for b in beams: b.used = True
    # whole notes: those in the same column form one chord
    wh = sorted([h for h in heads if h.kind == 'whole'], key=lambda h: (h.staff, h.g.x0))
    for h in wh:
        for ev in events:
            if ev.heads and ev.heads[0].kind == 'whole' and ev.staff == h.staff and abs(ev.x - h.g.x0) < 0.8 * sp:
                ev.heads.append(h); break
        else:
            events.append(Event(h.g.x0, h.staff, [h]))
    # --- dots (augmentation dot / staccato)
    for g in glyphs:
        if g.name != 'dot' or g.used or g.w > 0.6 * sp:
            continue
        for ev in events:
            if not ev.heads: continue
            xr = max(h.g.x1 for h in ev.heads)
            hit = [h for h in ev.heads if abs(g.cy - h.g.cy) < 0.65 * sp and
                   (0.05 * sp < g.cx - h.g.x1 < 1.5 * sp or 0.05 * sp < g.cx - xr < 1.5 * sp)]
            if hit:
                min(hit, key=lambda h: abs(g.cy - h.g.cy)).dots = 1; g.used = True; break
    for ev in events:
        ev.dots = max((h.dots for h in ev.heads), default=0)
    for g in glyphs:
        if g.name == 'dot' and not g.used and g.w <= 0.6 * sp:
            for ev in events:
                hs = ev.heads
                if any(abs(g.cx - h.g.cx) < 0.45 * sp and 0.5 * sp < abs(g.cy - h.g.cy) < 2 * sp for h in hs) or (
                        hs and ev.stem and abs(g.cx - hs[0].g.cx) < 0.9 * sp and abs(g.cy - ev.y) < 1.4 * sp):
                    ev.staccato = True; g.used = True; break
    # --- rests
    for g in glyphs:
        if g.name in ('rest_q', 'rest_8', 'rest_16', 'rest_32', 'rest_half', 'rest_whole'):
            st = sy.staff_of(g.cy)
            ev = Event(g.x0, st, rest={'rest_q': 'quarter', 'rest_8': 'eighth', 'rest_16': '16th', 'rest_32': '32nd',
                                       'rest_half': 'half', 'rest_whole': 'whole'}[g.name], y=g.cy)
            for d in glyphs:
                if d.name == 'dot' and not d.used and 0 < d.cx - g.x1 < 1.2 * sp and abs(d.cy - g.cy) < 1.2 * sp:
                    ev.dots = 1; d.used = True
            events.append(ev); g.used = True
    for s in fills:
        if s.ops == 'r' and 0.9 * sp < s.w < 1.6 * sp and 0.35 * sp < s.h < 0.7 * sp and not s.used:
            st = sy.staff_of(s.cy)
            if S[st].dist(s.cy) > 0: continue
            line_above = min(S[st].ys, key=lambda y: abs(y - s.y0))
            kind = 'whole' if abs(line_above - s.y0) < 0.15 * sp else 'half'
            events.append(Event(s.x0, st, rest=kind, y=s.cy)); s.used = True
    # --- durations
    base = {'whole': F(4), 'half': F(2), 'quarter': F(1), 'eighth': F(1, 2), '16th': F(1, 4), '32nd': F(1, 8)}
    for ev in events:
        if ev.rest:
            d = base[ev.rest]
        else:
            k = ev.heads[0].kind
            d = F(4) if k == 'whole' else F(2) if k == 'half' else F(1, 2 ** ev.beams)
        if ev.dots: d = d * F(3, 2)
        if ev.grace: d = F(0)
        ev.dur = d
    # --- beam groups (beam begin/continue/end)
    groups = {}
    for ev in events:
        if ev.beam_ids:
            main = max(ev.beam_ids, key=lambda bi: beams[bi].w)
            groups.setdefault(main, []).append(ev)
    for grp in groups.values():
        grp.sort(key=lambda e: e.x)
        n = len(grp)
        for i, ev in enumerate(grp):
            for lvl in range(1, ev.beams + 1):
                prev = i > 0 and grp[i - 1].beams >= lvl
                nxt = i < n - 1 and grp[i + 1].beams >= lvl
                ev.beam_tags[lvl] = ('continue' if prev and nxt else 'begin' if nxt else 'end' if prev
                                     else ('backward hook' if i == n - 1 or (i > 0 and grp[i - 1].dots) else 'forward hook'))
    # --- clefs, time signature, accidentals
    for g in glyphs:
        if g.name in ('clef_g', 'clef_f'):
            m = measure_at(g.cx); st = sy.staff_of(g.cy)
            if g.x0 - m.x0 < 3 * sp:
                m.clefs[st] = g.name[-1].upper()
            else:
                m.directions.append((g.x0, st, 'clef', g.name[-1].upper()))
            g.used = True
    tdig = sorted([g for g in glyphs if g.name in DIGITS and 1.7 * sp < g.h < 2.4 * sp and
                   S[sy.staff_of(g.cy)].dist(g.cy) == 0 and sy.staff_of(g.cy) == 0], key=lambda g: g.cy)
    if len(tdig) >= 2:
        m = measure_at(tdig[0].cx)
        m.time = (DIGITS[tdig[0].name], DIGITS[tdig[1].name], None)
    for g in glyphs:
        if g.name in DIGITS and 1.7 * sp < g.h < 2.4 * sp and S[sy.staff_of(g.cy)].dist(g.cy) == 0:
            g.used = True
        if g.name == 'time_common':
            measure_at(g.cx).time = (4, 4, 'common'); g.used = True
    accs = [g for g in glyphs if g.name in ('sharp', 'flat', 'natural', 'dsharp', 'dflat')]
    for m in sy.measures:
        runs_ = {}
        for st in (0, 1):
            cl = [g for g in glyphs if g.name in ('clef_g', 'clef_f') and sy.staff_of(g.cy) == st and 0 <= g.x0 - m.x0 < 3 * sp]
            x = cl[0].x1 if cl else m.x0
            run = []
            for g in sorted([g for g in accs if sy.staff_of(g.cy) == st and m.x0 <= g.cx < m.x1], key=lambda g: g.x0):
                ay = g.cy if g.name != 'flat' else g.y1 - 0.5 * sp
                note_right = any(h.staff == st and -0.3 * sp < h.g.x0 - g.x1 < 1.5 * sp and abs(h.g.cy - ay) < 0.3 * sp for h in heads)
                if g.x0 - x < 1.6 * sp and not note_right:
                    run.append(g); x = g.x1
                else:
                    break
            if getattr(sy, 'finale', False):
                runs_[st] = run
            elif run:
                for g in run: g.used = True
                if st == 0:
                    real = [g for g in run if g.name != 'natural']
                    m.key = (len(real) if real[0].name == 'sharp' else -len(real)) if real else 0
        # (scores made by Finale) a key signature stands in both staves; the accidentals of a chord right after
        # the barline do not
        if runs_ and runs_[0] and len(runs_[0]) == len(runs_[1]):
            for g in runs_[0] + runs_[1]: g.used = True
            real = [g for g in runs_[0] if g.name != 'natural']
            m.key = (len(real) if real[0].name == 'sharp' else -len(real)) if real else 0
    for g in sorted(accs, key=lambda g: -g.x0):
        if g.used: continue
        st = sy.staff_of(g.cy)
        # the "body" of the symbol: the middle of the sharp/natural, the lower bowl of the flat
        ay = g.cy if g.name != 'flat' else g.y1 - 0.5 * sp
        cand = [h for h in heads if h.staff == st and -0.3 * sp < h.g.x0 - g.x1 < 3.2 * sp and abs(h.g.cy - ay) < 0.3 * sp
                and h.accidental is None]
        if cand:
            min(cand, key=lambda h: h.g.x0).accidental = g.name; g.used = True
    for g in accs:
        if not g.used:
            m = measure_at(g.cx)
            xs = [e.x for e in events if m.x0 <= e.x < m.x1]
            if g.x1 < min(xs or [m.x1]) or (m is sy.measures[-1] and g.x0 > max(xs or [m.x0]) + 2 * sp): g.used = True
    # --- fingering: small digits
    owner_f = {id(h): ev for ev in events for h in ev.heads}
    fd = [g for g in glyphs if g.name in DIGITS and not g.used and 0.9 * sp < g.h < 1.5 * sp]
    fd.sort(key=lambda g: (round(g.cx / (0.7 * sp)), g.cy))
    cols = []
    for g in fd:
        for c in cols:
            if abs(c[-1].cx - g.cx) < 0.6 * sp and 0 < g.y0 - c[-1].y1 < 0.9 * sp:
                c.append(g); break
        else:
            cols.append([g])
    for c in cols:
        cx = sum(g.cx for g in c) / len(c); cy = (c[0].y0 + c[-1].y1) / 2
        cand = [h for h in heads if abs(h.g.cx - cx) < 1.3 * sp] or [h for h in heads if abs(h.g.cx - cx) < 2.8 * sp and abs(h.g.cy - cy) < 6 * sp]
        if not cand:
            sy.measures[0].warnings.append(f"ujjrend-szam parositatlan x={cx:.0f} y={cy:.0f}"); continue
        if cy < S[0].top: st = 0
        elif cy > S[1].bottom: st = 1
        else: st = min(cand, key=lambda h: abs(h.g.cy - cy)).staff
        col = sorted([h for h in cand if h.staff == st], key=lambda h: h.g.cy)
        if not col:
            col = sorted(cand, key=lambda h: h.g.cy)
        dxmin = min(abs(h.g.cx - cx) for h in col)
        col = [h for h in col if abs(h.g.cx - cx) < dxmin + 0.75 * sp]
        evs_ = {id(owner_f[id(h)]) for h in col if id(h) in owner_f}
        col = sorted([h for h in heads if h.staff == col[0].staff and (h in col or id(owner_f.get(id(h))) in evs_)], key=lambda h: h.g.cy)
        above = cy < col[0].g.cy if len(col) == 1 else abs(cy - col[0].g.cy) < abs(cy - col[-1].g.cy)
        tgt = col[:len(c)] if above else col[-len(c):]
        for g, h in zip(c if above else c[-len(tgt):], tgt):
            h.fingering = DIGITS[g.name]; g.used = True
            h.fing_place = 'above' if cy < h.g.cy else 'below'
    # --- dynamics and text marks
    for g in glyphs:
        if g.name.startswith('dyn_'):
            measure_at(g.x0).directions.append((g.x0, 0, 'dynamics', g.name[4:])); g.used = True
        elif g.name.startswith('txt_'):
            above = g.cy < S[0].top
            ws = 1 if g.cy > S[1].top else 0
            measure_at(g.x0).directions.append((g.x0, ws, 'words', {'cresc': 'cresc.', 'dim': 'dim.', 'rit': 'rit.'}[g.name[4:]],
                                                'above' if above else 'below',
                                                round((S[ws].top - g.y1) / sp * 10), sp))
            g.used = True
        elif g.name in ('ped', 'ped_up'):
            measure_at(g.x0).directions.append((g.x0, 1, 'pedal', 'start' if g.name == 'ped' else 'stop')); g.used = True
        elif g.name == 'coda':
            measure_at(g.x0).directions.append((g.x0, 0, 'coda', '')); g.used = True
        elif g.name in ('accent', 'fermata', 'stacciss') or (g.ops == 'llllllll' and 1.1 * sp < g.w < 1.7 * sp and abs(g.w - g.h) < 0.3 * sp):
            if g.name == '?': g.name = 'marcato'
            cand = [e for e in events if e.heads or g.name == 'fermata']
            if cand:
                e = min(cand, key=lambda e: abs(e.x + 0.6 * sp - g.cx) + 0.25 * min([abs(h.g.cy - g.cy) for h in e.heads] + [abs(e.y - g.cy)]))
                val = True
                if g.name == 'fermata':
                    val = 'upright'
                    try:
                        cs = _contours(g.src) if g.src is not None else []
                        boxes = [b for b in (_bbox([c]) for c in cs) if g.x0 - .5 <= b[0] and b[2] <= g.x1 + .5 and g.y0 - .5 <= b[1] and b[3] <= g.y1 + .5]
                        dot = min(boxes, key=lambda b: (b[2] - b[0]) * (b[3] - b[1])) if len(boxes) > 1 else None
                        if dot is not None and (dot[1] + dot[3]) / 2 < g.cy: val = 'inverted'
                    except Exception:
                        pass
                setattr(e, g.name, val); g.used = True

    # --- tenuto: short horizontal line above/below the head
    for s_ in fills:
        if s_.ops == 'r' and 0.8 * sp < s_.w < 1.6 * sp and s_.h < 0.22 * sp and not s_.used:
            cand = [e for e in events if e.heads and abs(e.x + 0.6 * sp - s_.cx) < 0.8 * sp and
                    min([abs(h.g.cy - s_.cy) for h in e.heads] + [abs(e.y - s_.cy)]) < 2.2 * sp]
            if cand:
                min(cand, key=lambda e: abs(e.x + 0.6 * sp - s_.cx)).tenuto = True; s_.used = True
    # --- arpeggio
    for g in glyphs:
        if g.name == 'arpeggio':
            for ev in events:
                if ev.heads and 0 < ev.x - g.x1 < 2.2 * sp and any(g.y0 - sp < h.g.cy < g.y1 + sp for h in ev.heads):
                    ev.arpeggio = True; g.used = True
    arp = [g for g in glyphs if g.name == 'arpeggio' and g.used]
    for g in glyphs:
        if g.name == 'arpeggio' and not g.used and any(abs(a.x0 - g.x0) < 0.5 * sp for a in arp): g.used = True
    # --- tuplets: duplet (2 notes instead of 3), triplet (3 notes instead of 2)
    for g in glyphs:
        if g.name in ('tuplet2', 'tuplet3'):
            n = int(g.name[-1]); ratio = (2, 3) if n == 2 else (3, 2)
            hooks = [s_ for s_ in strokes if s_.ops == 'll' and not s_.used and abs(s_.cy - g.cy) < 1.5 * sp and abs(s_.cx - g.cx) < 7 * sp
                     and s_.w < 5 * sp]
            cand = [e for e in events if (e.heads or e.rest) and abs(e.x + 0.6 * sp - g.cx) < 8 * sp and e.tuplet is None]
            if not cand:
                continue
            def vd(e):
                ys = [h.g.cy for h in e.heads] + ([e.y] if e.stem or e.rest else [])
                return min(abs(y - g.cy) for y in ys)
            near = min(cand, key=lambda e: abs(e.x + 0.6 * sp - g.cx) + 0.6 * vd(e))
            same = [e for e in cand if e.staff == near.staff and (e.stem == near.stem or e.rest or near.rest)]
            grp = [e for e in same if near.beam_ids and set(e.beam_ids) & set(near.beam_ids)]
            if len(grp) != n:
                grp = sorted(same, key=lambda e: abs(e.x + 0.6 * sp - g.cx))[:n]
            grp.sort(key=lambda e: e.x)
            for e in grp:
                e.dur = e.dur * F(ratio[1], ratio[0]); e.tuplet = ratio
            grp[0].tuplet_start = True; grp[-1].tuplet_stop = True
            for s_ in hooks: s_.used = True
            g.used = True
    # --- slurs and ties (lens-shaped filled curves)
    owner = {id(h): ev for ev in events for h in ev.heads}
    first_x = min([e.x for e in events] or [sy.x0]); last_x = max([e.x for e in events] or [sy.x1])
    sy.curves = []
    for s_ in fills:
        if s_.ops not in ('cc', 'clcl', 'clc') or s_.h > 9 * sp or s_.w < 0.9 * sp or (s_.w < 1.5 * s_.h and s_.h > 5 * sp) or s_.used:
            continue
        pts = [(p.x, p.y) for it in s_.items for p in (it[1], it[-1])]
        L = min(pts); R = max(pts)

        def pick(tip, left):
            best = None
            for h in heads:
                dx = h.g.cx - tip[0]
                if abs(dx) > 1.9 * sp or (left and dx > 1.0 * sp) or (not left and dx < -1.0 * sp):
                    continue
                dy = min(abs(h.g.cy - tip[1]), abs(owner[id(h)].y - tip[1]) if owner[id(h)].stem else 1e9)
                if dy > 2.6 * sp:
                    continue
                c = abs(dx) + 0.5 * abs(h.g.cy - tip[1]) + 0.3 * dy
                if best is None or c < best[0]:
                    best = (c, h)
            return best[1] if best else None
        a, b = pick(L, True), pick(R, False)
        if a is not None and b is not None and a is b:
            # stub of a curve crossing the system break: it is on one side of the head
            if R[0] <= a.g.cx + 0.3 * sp and L[0] < a.g.x0: a = None
            elif L[0] >= a.g.cx - 0.3 * sp and R[0] > a.g.x1: b = None
            else: continue
        if a is None and not (L[0] < first_x - 0.5 * sp):
            sy.measures[0].warnings.append(f"iv kezdete parositatlan x={L[0]:.0f} y={L[1]:.0f}"); continue
        if b is None and not (R[0] > last_x + 0.5 * sp):
            sy.measures[0].warnings.append(f"iv vege parositatlan x={R[0]:.0f} y={R[1]:.0f}"); continue
        s_.used = True
        # the curve bulges upwards if its middle is above its ends
        over = s_.cy < (L[1] + R[1]) / 2
        sy.curves.append({'a': a, 'b': b, 'ea': owner.get(id(a)), 'eb': owner.get(id(b)), 'y': s_.cy, 'L': L, 'R': R,
                          'over': over})
    # --- volta (1st / 2nd ending): horizontal line above the upper staff + large number
    vd = [g for g in glyphs if g.name in DIGITS and not g.used and 1.5 * sp <= g.h < 2.3 * sp and g.cy < S[0].top]
    segs = [s_ for s_ in strokes if not s_.used and not s_.dashed and s_.w > 3 * sp and s_.y0 < S[0].top - 1.2 * sp and
            ((s_.ops == 'l' and s_.h < 0.05) or (s_.ops in ('ll', 'lll') and s_.h < 4 * sp and _has_vertical(s_)))]
    for s_ in segs:
        num = [g for g in vd if s_.x0 - sp < g.x0 < s_.x0 + 4 * sp and 0 < g.cy - s_.y0 < 3 * sp]
        n = DIGITS[num[0].name] if num else 'cont'
        for g in num:
            g.used = True
            for d in glyphs:
                if d.name == 'dot' and 0 < d.cx - g.x1 < sp and abs(d.y1 - g.y1) < 0.4 * sp: d.used = True
        for m in sy.measures:
            ov = min(m.x1, s_.x1) - max(m.x0, s_.x0)
            if ov > 0.5 * (m.x1 - m.x0):
                m.volta = n if (n != 'cont' and m.volta is None and not any(mm.volta == n for mm in sy.measures)) else (m.volta or 'cont')
        s_.used = True
    for s_ in strokes:   # the vertical hook of the volta
        if s_.ops == 'l' and s_.w < 0.05 and not s_.used and s_.y1 < S[0].top + 0.5 * sp and any(abs(s_.y0 - v.y0) < sp for v in segs):
            s_.used = True
    # --- 8va / 8vb dashed line: the notes of its staff sound an octave higher/lower
    for s_ in strokes:
        if not (s_.dashed and s_.ops == 'l' and s_.h < 0.05 and s_.w > 2 * sp) or s_.used:
            continue
        st = 0 if abs(s_.y0 - S[0].top) < abs(s_.y0 - S[1].bottom) and s_.y0 < S[1].top else 1
        if s_.y0 < S[st].top:
            sh = 1
        elif s_.y0 > S[st].bottom:
            sh = -1
        else:
            continue
        x_from = s_.x0 - 5 * sp
        start = s_.x0 - 0.8 * sp
        for g in glyphs:      # the "(8va)" text at the start of the line: valid from here
            if x_from <= g.cx <= s_.x0 + sp and abs(g.cy - s_.y0) < 2.2 * sp and (not g.used or g.name in DIGITS or g.name == 'parens'):
                g.used = True
                start = min(start, g.x0 - 0.6 * sp)
        for t in strokes:     # closing hook
            if t is not s_ and t.ops == 'l' and t.w < 0.05 and abs(t.x0 - s_.x1) < 0.6 * sp and min(abs(t.y0 - s_.y0), abs(t.y1 - s_.y0)) < 0.3 * sp:
                t.used = True
        for h in heads:
            if h.staff == st and start <= h.g.cx <= s_.x1 + 0.5 * sp:
                h.ott = sh; h.ott_line = True
        s_.used = True
    # --- crescendo / decrescendo hairpin: two lines meeting at one point
    hp = [s_ for s_ in strokes if s_.ops == 'l' and not s_.used and s_.w > 2 * sp and 0.02 < s_.h < 1.3 * sp]
    hp.sort(key=lambda s_: (round(s_.x0), s_.y0))
    for s_ in strokes:
        if s_.ops == 'll' and not s_.used and s_.w > 2 * sp and s_.h < 2 * sp and not _has_vertical(s_):
            pts = [(p.x, p.y) for it in s_.items for p in it[1:]]
            vertex = pts[1]
            kind = 'crescendo' if abs(vertex[0] - s_.x0) < abs(vertex[0] - s_.x1) else 'diminuendo'
            measure_at(s_.x0).directions.append((s_.x0, 0, 'wedge', kind))
            measure_at(s_.x1 - 0.5 * sp).directions.append((s_.x1, 0, 'wedge', 'stop'))
            s_.used = True
    done = set()
    for i, a in enumerate(hp):
        if i in done: continue
        for j in range(i + 1, len(hp)):
            b = hp[j]
            if j in done or abs(a.x0 - b.x0) > 0.5 * sp or abs(a.x1 - b.x1) > 0.5 * sp or abs(a.cy - b.cy) > 1.5 * sp:
                continue
            pa = [(p.x, p.y) for p in a.items[0][1:]]; pb = [(p.x, p.y) for p in b.items[0][1:]]
            la, ra = min(pa), max(pa); lb, rb = min(pb), max(pb)
            kind = 'crescendo' if abs(la[1] - lb[1]) < abs(ra[1] - rb[1]) else 'diminuendo'
            # lines that do not meet: the piece of a hairpin broken at the end of the system (joined in join_wedges)
            opened = ('open',) if min(abs(la[1] - lb[1]), abs(ra[1] - rb[1])) > 0.15 * sp else ()
            measure_at(a.x0).directions.append((a.x0, 0, 'wedge', kind) + opened)
            measure_at(a.x1 - 0.5 * sp).directions.append((a.x1, 0, 'wedge', 'stop') + opened)
            a.used = b.used = True; done |= {i, j}; break
    # --- assigning events to measures
    for ev in events:
        measure_at(ev.x + 0.3 * sp).events.append(ev)
    for m in sy.measures:
        m.events.sort(key=lambda e: (e.staff, e.x))
    # at the start of a system the part between the clef/key signature and a repeat sign (or other barline) is not a measure of its own
    if not jb and len(sy.measures) > 1 and not sy.measures[0].events and sy.measures[0].x1 - sy.measures[0].x0 < 9 * sp:
        m0, m1 = sy.measures[0], sy.measures[1]
        for st, c in m0.clefs.items(): m1.clefs.setdefault(st, c)
        if m1.key is None: m1.key = m0.key
        if m1.time is None: m1.time = m0.time
        m1.directions = m0.directions + m1.directions
        m1.warnings = m0.warnings + m1.warnings
        if m0.volta is not None and m1.volta is None: m1.volta = m0.volta
        m1.new_system = True
        m1.x0 = m0.x0
        sy.measures.pop(0)
    sy.measure_at = measure_at
    absorb = [g for g in glyphs if g.used and (g.name in ('ped', 'ped_up', 'coda') or g.name.startswith(('dyn_', 'txt_')))]
    for g in glyphs:
        if not g.used and any(u.x0 - 1 <= g.cx <= u.x1 + 2.5 and u.y0 - 1 <= g.cy <= u.y1 + 1.5 for u in absorb):
            g.used = True
    for g in glyphs:   # accidental in parentheses: the parentheses belong to the symbol
        if g.name in ('paren_l', 'paren_r') and any(a.used and abs(a.cy - g.cy) < sp and abs(a.cx - g.cx) < 1.5 * sp for a in accs):
            g.used = True
    rest_ = sorted([g for g in glyphs if not g.used and not (g.src is not None and g.src.used) and g.h < 3.2 * sp
                    and not (g.ops == 'clc' and g.h > 6 * sp)], key=lambda g: g.x0)
    clusters = []
    for g in rest_:
        for c in clusters:
            if g.x0 - c['x1'] < 1.5 * sp and g.x1 > c['x0'] - 0.5 * sp and g.y0 < c['y1'] + 0.2 * sp and g.y1 > c['y0'] - 0.2 * sp:
                c['g'].append(g); c['x1'] = max(c['x1'], g.x1); c['y0'] = min(c['y0'], g.y0); c['y1'] = max(c['y1'], g.y1); break
        else:
            clusters.append({'x0': g.x0, 'x1': g.x1, 'y0': g.y0, 'y1': g.y1, 'g': [g]})
    sy.text_clusters = [c for c in clusters if len(c['g']) >= 3 and c['x1'] - c['x0'] > 1.5 * sp]
    for s_ in strokes:   # ledger lines
        if s_.ops == 'l' and s_.h < 0.05 and s_.w < 3.2 * sp and not s_.used and any(
                abs(h.g.cx - s_.cx) < 1.6 * sp and abs(h.g.cy - s_.y0) < 4.5 * sp for h in heads) or any(
                e.rest in ('whole', 'half') and abs(e.x - s_.x0) < 1.2 * sp for e in events):
            s_.used = True
    sy.unused_strokes = [s_ for s_ in strokes if not s_.used and sy.x0 - 2 * sp <= s_.cx <= sy.x1 + sp]
    for g in glyphs:   # braces
        if g.ops == 'clc' and g.h > 6 * sp: g.used = True
    sy.unknown = [g for g in glyphs if not g.used and not (g.src is not None and g.src.used)]


def _has_vertical(s):
    return any(abs(it[1].x - it[2].x) < 0.1 and abs(it[1].y - it[2].y) > 1 for it in s.items if it[0] == 'l')


def _beam_y(b, x):
    if b.ops == 'r':
        return b.cy
    pts = [(p.x, p.y) for it in b.items for p in it[1:]]
    left = [p for p in pts if abs(p[0] - b.x0) < 0.3]
    right = [p for p in pts if abs(p[0] - b.x1) < 0.3]
    if not left or not right:
        return b.cy
    yl = sum(p[1] for p in left) / len(left); yr = sum(p[1] for p in right) / len(right)
    t = min(1, max(0, (x - b.x0) / max(b.w, 1e-6)))
    return yl + (yr - yl) * t


# ------------------------------------------------- voices, time within the measure
def solve_measure(m, length, sp, is_first):
    """Splitting into voices and onsets. length: the measure length in quarters."""
    voices = []     # (staff, [events])
    for st in (0, 1):
        evs = sorted([e for e in m.events if e.staff == st and not e.grace], key=lambda e: e.x)
        if not evs:
            continue
        for e in evs:
            if e.rest == 'whole' and len(evs) == 1:
                e.dur = length; e.whole_measure = True
        tot = sum(e.dur for e in evs)
        ups = [e for e in evs if e.stem == 'up']; downs = [e for e in evs if e.stem == 'down']
        cols = {round(e.x / (0.5 * sp)) for e in evs}
        overlap = len(cols) < len(evs) and any(
            abs(a.x - b.x) < 0.6 * sp for a in ups for b in downs)
        if tot == length and not overlap:
            voices.append((st, evs)); continue
        if not overlap and tot < length:
            if not is_first:
                m.warnings.append(f"{'felso' if st == 0 else 'also'} sor rovidebb az utemnel: {float(tot)} (vart: {float(length)})")
            voices.append((st, evs)); continue
        free = [e for e in evs if e.stem is None]
        v1, v2 = list(ups), list(downs)
        best = None
        for mask in range(1 << min(len(free), 10)):
            a = v1 + [e for i, e in enumerate(free) if mask >> i & 1]
            b = v2 + [e for i, e in enumerate(free) if not mask >> i & 1]
            sa, sb = sum(e.dur for e in a), sum(e.dur for e in b)
            # the rest is in the vertical band of its own voice
            cost = (sa != length) + (sb != length) + (sa > length) * 5 + (sb > length) * 5
            ya = [e.heads[0].g.cy for e in v1] or [0]; yb = [e.heads[-1].g.cy for e in v2] or [1e9]
            for e in free:
                in_a = e in a
                if e.rest and in_a and e.y > sum(yb) / len(yb): cost += 0.4
                if e.rest and not in_a and e.y < sum(ya) / len(ya): cost += 0.4
            if best is None or cost < best[0]:
                best = (cost, a, b)
        a, b = sorted(best[1], key=lambda e: e.x), sorted(best[2], key=lambda e: e.x)
        if a: voices.append((st, a))
        if b: voices.append((st, b))
    # time map from the complete voices
    xmap = []
    for st, evs in voices:
        if sum(e.dur for e in evs) == length:
            t = F(0)
            for e in evs:
                e.onset = t; xmap.append((e.x, t, e)); t += e.dur
    for st, evs in voices:
        if evs[0].onset is not None:
            continue
        t = F(0)
        for e in evs:
            hit = [o for x, o, _ in xmap if abs(x - e.x) < 0.9 * sp]
            if not hit and e.heads:
                # a head pushed to the right of the chord of the other voice (a second or a third apart) sounds with the chord
                hit = [o for x, o, c in xmap if 0 < e.x - x < 1.7 * sp and o >= t and c.staff == e.staff and
                       any(abs(h.pos - k.pos) <= 2 for h in e.heads for k in c.heads)]
            if hit and min(hit) >= t:
                t = min(h for h in hit if h >= t)
            e.onset = t; xmap.append((e.x, t, e)); t += e.dur
        if t > length:
            m.warnings.append(f"szolam tullog az utemen ({float(t)} > {float(length)})")
    # grace notes: before the next note, into its voice
    for g in sorted([e for e in m.events if e.grace], key=lambda e: e.x):
        cand = [(evs, i) for st, evs in voices if st == g.staff for i, e in enumerate(evs) if not e.grace and e.heads and e.x > g.x - 0.2 * sp]
        if not cand:
            cand = [(evs, len(evs) - 1) for st, evs in voices if st == g.staff and evs]
        if not cand:
            m.warnings.append(f"eloke parositatlan x={g.x:.0f}"); continue
        evs, i = min(cand, key=lambda c: abs(c[0][c[1]].x - g.x))
        g.onset = evs[i].onset
        evs.insert(i, g)
    full = any(sum(e.dur for e in evs) == length for _, evs in voices)
    end = max([e.onset + e.dur for _, evs in voices for e in evs] or [F(0)])
    m.length = length
    if voices and not full:
        if end < length and is_first:
            m.length = end; m.implicit = True
        else:
            m.warnings.append(f"utemhossz elter: {float(end)} (vart: {float(length)})")
    # voice numbers: upper staff 1-2, lower staff 5-6
    cnt = {0: 0, 1: 0}
    m.voices = []
    for st, evs in voices:
        cnt[st] += 1
        vn = (1 if st == 0 else 5) + cnt[st] - 1
        for e in evs: e.voice = vn
        m.voices.append((st, vn, evs))
