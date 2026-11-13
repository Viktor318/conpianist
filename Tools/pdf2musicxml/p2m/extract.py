"""Reading the visible vector shapes of a PDF page (PyMuPDF)."""
import pymupdf
from dataclasses import dataclass, field


@dataclass
class Shape:
    kind: str            # 'f' filled, 's' stroked, 'fs' both
    ops: str             # path operations: l=line, c=curve, r=rectangle, q=quad
    x0: float
    y0: float
    x1: float
    y1: float
    width: float         # line width (only 's')
    items: list = field(repr=False, default_factory=list)
    used: bool = False
    dashed: bool = False

    @property
    def w(self): return self.x1 - self.x0
    @property
    def h(self): return self.y1 - self.y0
    @property
    def cx(self): return (self.x0 + self.x1) / 2
    @property
    def cy(self): return (self.y0 + self.y1) / 2
    @property
    def sig(self):
        """Shape signature: the operation sequence + size with 0.1 pt precision."""
        return f"{self.kind}:{self.ops}:{self.w:.1f}x{self.h:.1f}"


def _ops(items):
    return ''.join('r' if i[0] in ('re', 'qu') else i[0] for i in items)


def visible_shapes(page):
    """Only the shapes actually visible: layer switched on + non-empty clip area."""
    doc = page.parent
    off = {v['name'] for v in doc.get_ocgs().values() if not v['on']}
    out, stack = [], []
    for d in page.get_drawings(extended=True):
        lvl = d['level']
        while stack and stack[-1][0] >= lvl:
            stack.pop()
        if d['type'] in ('clip', 'group'):
            stack.append((lvl, pymupdf.Rect(d.get('scissor') or d.get('rect'))))
            continue
        if d.get('layer') in off:
            continue
        clip = None
        for _, r in stack:
            clip = r if clip is None else (clip & r)
        r = d['rect']
        # (a PDF without any clip area, e.g. made by Finale 2012: everything is visible)
        if clip is not None and (clip.is_empty or not clip.intersects(r + (-.5, -.5, .5, .5))):
            continue
        dsh = d.get('dashes') or ''
        out.append(Shape(d['type'], _ops(d['items']), r.x0, r.y0, r.x1, r.y1,
                         d.get('width') or 0.0, d['items'], dashed=bool(dsh.strip().startswith('[') and dsh.strip()[1:].split(']')[0].strip())))
    return out
