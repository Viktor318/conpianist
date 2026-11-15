"""Shapes from a score page stored as a bitmap image (prototype).

The page image is scaled to PX pixels per staff space, the staff lines are removed, and noteheads,
stems, barlines, beams, slurs and the other symbols are located; the result is a list of Shape
objects like the ones read from a vector page, with the symbols already named (Shape.name).
"""
import base64
import json
import os
import numpy as np
import cv2
import pymupdf
from .extract import Shape

PX = 40
_BANK = None
P = pymupdf.Point
SLIDE = ('clef_g', 'clef_f', 'ped', 'ped_up', 'fermata', 'dyn_ppp', 'dyn_pp', 'dyn_mp', 'dyn_mf', 'dyn_ff', 'dyn_sf',
         'dyn_p', 'dyn_f', 'txt_cresc', 'txt_dim', 'txt_rit', 'time_common', 'coda', 'trill', 'rest_q', 'rest_8', 'rest_16',
         'flat', 'natural', 'sharp')
DIGITS_ = ('d1', 'd2', 'd3', 'd4', 'd5', 'd6', 'd8', 'd9')
NO_COMP = ('head_black', 'head_half', 'head_whole', 'parens', 'metro_q', 'dot') + SLIDE


def bank():
    """The symbol templates: name -> [(width, height in staff spaces, bitmap, number of samples, own)]. The
    templates cut from the image scores themselves (own) come first, and replace the others of the same size."""
    global _BANK
    if _BANK is None:
        with open(os.path.join(os.path.dirname(__file__), 'image_templates.json'), encoding='utf-8') as f:
            data = json.load(f)['templates']
        _BANK = {}
        for k, lst in data.items():
            items = [(t['w'], t['h'], np.unpackbits(np.frombuffer(base64.b64decode(t['bits']), np.uint8))[:t['rows'] * t['cols']]
                      .reshape(t['rows'], t['cols']).astype(bool), t['n'], t['own']) for t in lst]
            own = [v for v in items if v[4]]
            _BANK[k] = own + [v for v in items if not v[4] and not any(abs(v[0] - o[0]) < 0.2 and abs(v[1] - o[1]) < 0.25 for o in own)]
    return _BANK


def is_image_page(page):
    return any(i['width'] * i['height'] > 4e6 for i in page.get_image_info())


def _load(page):
    info = max(page.get_image_info(xrefs=True), key=lambda i: i['width'] * i['height'])
    pix = pymupdf.Pixmap(page.parent, info['xref'])
    a = np.frombuffer(pix.samples, np.uint8).reshape(pix.height, pix.width, pix.n)[:, :, 0]
    bw = (a < 128).astype(np.uint8)
    if bw.mean() > 0.5:
        bw = 1 - bw
    return bw, info['bbox']


def _staff_lines(bw):
    rows = bw.sum(1)
    ys = np.where(rows > 0.4 * bw.shape[1])[0]
    g = []
    for y in ys:
        if g and y - g[-1][-1] <= 2: g[-1].append(y)
        else: g.append([y])
    return [(a[0], a[-1]) for a in g]


def _match(img, t, thr):
    r = cv2.matchTemplate(img, t, cv2.TM_CCOEFF_NORMED)
    out = []
    h, w = t.shape
    while True:
        _, v, _, loc = cv2.minMaxLoc(r)
        if v < thr: break
        x, y = loc
        out.append((v, x, y))
        cv2.rectangle(r, (x - w // 2, y - h // 2), (x + w // 2, y + h // 2), -1, -1)
    return out


def image_shapes(page, debug=None):
    B = bank()
    bw0, bbox = _load(page)
    L0 = _staff_lines(bw0)
    if len(L0) < 5:
        return []
    mid = [(a + b) / 2 for a, b in L0]
    sp0 = float(np.median([mid[i + 1] - mid[i] for i in range(len(mid) - 1) if mid[i + 1] - mid[i] < 3 * (mid[1] - mid[0])]))
    k = PX / sp0
    bw = (cv2.resize(bw0 * 255, None, fx=k, fy=k, interpolation=cv2.INTER_AREA) > 100).astype(np.uint8)
    H, W = bw.shape
    sx = (bbox[2] - bbox[0]) / W; sy = (bbox[3] - bbox[1]) / H
    X = lambda x: bbox[0] + x * sx
    Y = lambda y: bbox[1] + y * sy
    shapes = []

    def add(kind, ops, x0, y0, x1, y1, width=0.0, items=None, name=None):
        s = Shape(kind, ops, X(x0), Y(y0), X(x1), Y(y1), width, items or [])
        s.name = name
        shapes.append(s)
        return s
    lines = _staff_lines(bw)
    # a row of ledger lines under many notes (or another long line) next to a staff looks like a sixth line:
    # of the lines one space apart, the five densest neighbours are the staff
    rowd0 = bw.sum(1)
    groups = []
    for ln in lines:
        if groups and ln[0] - groups[-1][-1][1] < 1.5 * PX: groups[-1].append(ln)
        else: groups.append([ln])
    lines = []
    for g in groups:
        if len(g) > 5:
            dens = [max(rowd0[a:b + 1]) for a, b in g]
            k = max(range(len(g) - 4), key=lambda i: sum(dens[i:i + 5]))
            g = g[k:k + 5]
        if len(g) == 5:
            lines += g
    # the five lines of a staff are evenly spaced: a line merged with a beam or another long mark is put back
    tmed_ = int(np.median([b - a + 1 for a, b in lines]))
    # a horizontal beam lying on a line makes it look thick: the line itself is the densest rows of the group
    rowd = bw.sum(1)
    for i, (a, b) in enumerate(lines):
        if b - a + 1 > tmed_ + 2:
            dense = [y for y in range(a, b + 1) if rowd[y] >= 0.8 * rowd[a:b + 1].max()]
            if dense and dense[-1] - dense[0] + 1 <= tmed_ + 2:
                lines[i] = (dense[0], dense[-1])
    for i in range(0, len(lines) - 4, 5):
        c = np.array([(a + b) / 2 for a, b in lines[i:i + 5]])
        d_ = np.median(np.diff(c)); c0 = np.median(c - np.arange(5) * d_)
        for j in range(5):
            cf = c0 + j * d_
            if abs(c[j] - cf) > 1.0 or lines[i + j][1] - lines[i + j][0] + 1 > tmed_ + 1:
                a_ = int(round(cf - (tmed_ - 1) / 2)); lines[i + j] = (a_, a_ + tmed_ - 1)
    # --- staff lines: emitted as strokes, removed from the image where nothing touches them
    nb = bw.copy()
    ext = []
    for a, b in lines:
        cols = np.where(bw[(a + b) // 2] > 0)[0]
        runs = [r for r in np.split(cols, np.where(np.diff(cols) > 3)[0] + 1) if len(r) > 3 * PX]
        ext.append((runs[0][0], runs[-1][-1] + 1) if runs else (cols[0], cols[-1] + 1))
    for li, (a, b) in enumerate(lines):
        grp = ext[li - li % 5:li - li % 5 + 5]       # the five lines of a staff share their extent
        xa, xb = float(np.median([e[0] for e in grp])), float(np.median([e[1] for e in grp]))
        yc = (a + b + 1) / 2
        add('s', 'l', xa, yc, xb, yc, 0.4)
        free = (bw[max(0, a - 2)] == 0) & (bw[min(H - 1, b + 2)] == 0)
        nb[max(0, a - 1):b + 2, free] = 0
        free = (bw[max(0, a - 3)] == 0) & (bw[min(H - 1, b + 3)] == 0)     # a line a little thicker or shifted
        nb[max(0, a - 2):b + 3, free] = 0
    # thin leftovers of a line (where its rows were a little off)
    sl = nb & (1 - cv2.morphologyEx(nb, cv2.MORPH_OPEN, np.ones((4, 1), np.uint8)))
    sl = cv2.morphologyEx(sl, cv2.MORPH_OPEN, np.ones((1, PX), np.uint8))
    band = np.zeros_like(nb)
    for a, b in lines:
        band[max(0, a - 6):b + 7] = 1
    nb[(sl & band) > 0] = 0
    work = nb.copy()
    f32 = lambda m: m.astype(np.float32)
    # --- black noteheads: where a slightly smaller head fits
    heads = []
    t = B['head_black'][0][2].astype(np.uint8)
    hh, hw = 46, 55

    def fit(tm, it, name, hw, hh):
        ker = cv2.erode(tm, np.ones((3, 3), np.uint8), iterations=it)
        er = cv2.erode(work, ker)
        n, lab, st, cen = cv2.connectedComponentsWithStats(er, connectivity=8)
        for i in range(1, n):
            cx, cy = cen[i]
            if st[i, cv2.CC_STAT_WIDTH] > 0.9 * hw or st[i, cv2.CC_STAT_HEIGHT] > 0.9 * hh:
                continue        # a large blob, not a head
            heads.append(dict(cx=cx + 0.5, cy=cy + 0.5, w=hw, h=hh, name=name))
    fit(t, 6, 'head_black', hw, hh)
    # --- half and whole noteheads
    for nm, thr in (('head_half', 0.62), ('head_whole', 0.62)):
        tm = B[nm][0][2].astype(np.uint8)
        for v, x, y in _match(f32(nb), f32(tm), thr):
            heads.append(dict(cx=x + tm.shape[1] / 2, cy=y + tm.shape[0] / 2, w=tm.shape[1], h=tm.shape[0], name=nm))
    # --- thin vertical lines: stems and barlines
    longh = cv2.morphologyEx(nb, cv2.MORPH_OPEN, np.ones((1, 10), np.uint8))
    thin = nb & (1 - longh)
    vert = cv2.morphologyEx(thin, cv2.MORPH_OPEN, np.ones((int(0.6 * PX), 1), np.uint8))
    n, lab, st, cen = cv2.connectedComponentsWithStats(vert, connectivity=8)
    stems = []
    for i in range(1, n):
        x0, y0, w, h = st[i, :4]
        xc = x0 + w // 2
        ya, yb = y0, y0 + h - 1
        col = lambda y: nb[y, max(0, xc - 1):xc + 2].any()
        while ya > 0 and col(ya - 1): ya -= 1
        while yb < H - 1 and col(yb + 1): yb += 1
        stems.append([x0 + w / 2, ya, yb + 1, w])
    stems.sort()
    merged = []
    for s in stems:
        # a piece joins the line it continues: the same place (a stem may be cut where a staff line was taken out, and
        # the two edges of a thick line are found a few pixels apart: a barline read twice became a double barline).
        # The lines of the other systems at the same x come between them in the list, so the last few are looked at.
        hit = None
        for m_ in reversed(merged[-12:]):
            if s[0] - m_[0] >= 0.2 * PX: break
            if s[1] <= m_[2] + 6 and s[2] >= m_[1] - 6:
                hit = m_; break
        if hit is not None:
            # the pieces of a stem cut by ledger lines may be a pixel or two off: the stem covers them all
            xl = min(hit[0] - hit[3] / 2, s[0] - s[3] / 2); xr = max(hit[0] + hit[3] / 2, s[0] + s[3] / 2)
            hit[0] = (xl + xr) / 2; hit[3] = xr - xl
            hit[1] = min(hit[1], s[1]); hit[2] = max(hit[2], s[2])
        else:
            merged.append(s)
    stems = merged
    sys_y = [(lines[i][0], lines[i + 9][1]) for i in range(0, len(lines) - 9, 10)]
    staff_y = [(lines[i][0], lines[i + 4][1]) for i in range(0, len(lines) - 4, 5)]
    used_stems = []
    for x, ya, yb, w in stems:
        bar = any(ya <= a + 4 and yb >= b - 4 for a, b in sys_y) or any(abs(ya - a) <= 4 and abs(yb - b) <= 5 for a, b in staff_y)
        near = any(ya - 0.6 * PX <= h['cy'] <= yb + 0.6 * PX and
                   min(abs(x - (h['cx'] - h['w'] / 2)), abs(x - (h['cx'] + h['w'] / 2))) < 0.2 * PX for h in heads) and yb - ya > 1.4 * PX
        if os.environ.get('P2M_TRACE', '').startswith('stem:'):
            q = list(map(int, os.environ['P2M_TRACE'][5:].split(',')))
            if q[0] <= x <= q[2] and ya <= q[3] and yb >= q[1]:
                hd = [(round(h['cx'] - h['w'] / 2), round(h['cx'] + h['w'] / 2), round(h['cy'])) for h in heads if abs(h['cx'] - x) < 60 and ya - 40 <= h['cy'] <= yb + 40]
                print('STEMT', round(x, 1), ya, yb, w, 'bar', bar, 'near', near, hd)
        # a stem that happens to run from the top line to the bottom line of its staff is not a barline
        if bar and near and not any(ya <= a + 4 and yb >= b - 4 for a, b in sys_y):
            bar = False
        if bar or near:
            add('s', 'l', x, ya, x, yb, 0.5)
            used_stems.append((x, ya, yb, bar))
            xa, xb = int(x - w / 2 - 2), int(x + w / 2 + 3)
            band = thin[ya:yb, xa:xb]
            work[ya:yb, xa:xb][band > 0] = 0
            if bar:
                work[ya:yb, xa:xb] = 0
    # thick barlines
    thick_x = []
    tall = cv2.morphologyEx(nb, cv2.MORPH_OPEN, np.ones((int(3.9 * PX), 1), np.uint8))
    n, lab, st, cen = cv2.connectedComponentsWithStats(tall, connectivity=8)
    for i in range(1, n):
        x0, y0, w, h = st[i, :4]
        if 10 <= w <= 0.8 * PX and st[i, cv2.CC_STAT_AREA] > 0.8 * w * h:
            add('s', 'l', x0 + w / 2, y0, x0 + w / 2, y0 + h, 2.5)
            thick_x.append(x0 + w / 2)
            work[y0:y0 + h, x0 - 1:x0 + w + 1] = 0
    # --- erase the heads
    for h in heads:
        tm = B[h['name']][0][2].astype(np.uint8)
        add('f', '@', h['cx'] - h['w'] / 2, h['cy'] - h['h'] / 2, h['cx'] + h['w'] / 2, h['cy'] + h['h'] / 2, name=h['name'])
        cv2.ellipse(work, (int(h['cx']), int(h['cy'])), (int(h['w'] / 2 + 3), int(h['h'] / 2 + 3)), 0, 0, 360, 0, -1)
    thin_h = work & (1 - cv2.morphologyEx(work, cv2.MORPH_OPEN, np.ones((13, 1), np.uint8)))
    for h in heads:
        a, b = min(staff_y, key=lambda s: min(abs(h['cy'] - s[0]), abs(h['cy'] - s[1])))
        if a - 0.7 * PX <= h['cy'] <= b + 0.7 * PX: continue
        ys_ = np.arange(a + 1.5, h['cy'] - 0.3 * PX, -PX)[1:] if h['cy'] < a else np.arange(b - 1.5, h['cy'] + 0.3 * PX, PX)[1:]
        for yl in ys_:
            ya_, yb_ = int(yl - 6), int(yl + 7); xa_, xb_ = int(h['cx'] - 1.15 * PX), int(h['cx'] + 1.15 * PX)
            sub = work[ya_:yb_, xa_:xb_]
            sub[thin_h[ya_:yb_, xa_:xb_] > 0] = 0
    if debug == 'work':
        return work
    # --- symbols found by sliding their template (several parts, or damaged by the staff line removal)
    claimed = np.zeros_like(work)
    cands = []
    for nm in B:
        for wsp, hsp, tm, cnt, own in B[nm][:4]:
            if not (nm in SLIDE or (nm in DIGITS_ and hsp > 1.5 and own)): continue
            tm = tm.astype(np.uint8)
            if tm.shape[0] < 12 or tm.shape[1] < 8: continue
            for v, x, y in _match(f32(work), f32(tm), 0.70 if own else 0.66):
                cands.append((v + 0.12 * np.log(tm.size), nm, x, y, tm))
    cands.sort(key=lambda c: -c[0])
    for v, nm, x, y, tm in cands:
        h, w = tm.shape
        if claimed[y:y + h, x:x + w].mean() > 0.15: continue
        claimed[y:y + h, x:x + w] = 1
        add('f', '@', x, y, x + w, y + h, name=nm)
        dm = cv2.dilate(cv2.copyMakeBorder(tm, 3, 3, 3, 3, cv2.BORDER_CONSTANT), np.ones((7, 7), np.uint8))
        ya, xa = max(0, y - 3), max(0, x - 3)
        sub = work[ya:y + h + 3, xa:x + w + 3]
        dm = dm[ya - (y - 3):ya - (y - 3) + sub.shape[0], xa - (x - 3):xa - (x - 3) + sub.shape[1]]
        sub[dm > 0] = 0
    used_set = {(x, ya, yb) for x, ya, yb, bar in used_stems}
    # --- grace notes: small heads, found after the larger symbols are gone
    gt = cv2.resize(B['head_black'][0][2].astype(np.uint8), (28, 24), interpolation=cv2.INTER_NEAREST)
    nobeam = work & (1 - cv2.morphologyEx(work, cv2.MORPH_OPEN, np.ones((1, int(1.6 * PX)), np.uint8)))
    er = cv2.erode(nobeam, cv2.erode(gt, np.ones((3, 3), np.uint8), iterations=2))
    n, lab, st, cen = cv2.connectedComponentsWithStats(er, connectivity=8)
    for i in range(1, n):
        if st[i, cv2.CC_STAT_WIDTH] > 16 or st[i, cv2.CC_STAT_HEIGHT] > 14: continue
        cx, cy = cen[i][0] + 0.5, cen[i][1] + 0.5
        if not any(a - 5 * PX < cy < b + 5 * PX for a, b in staff_y): continue
        gs = [s_ for s_ in stems if tuple(s_[:3]) not in used_set and s_[1] - 0.5 * PX <= cy <= s_[2] + 0.5 * PX and
              min(abs(s_[0] - (cx - 14)), abs(s_[0] - (cx + 14))) < 0.2 * PX and 0.9 * PX < s_[2] - s_[1] < 4 * PX]
        if not gs: continue
        add('f', '@', cx - 14, cy - 12, cx + 14, cy + 12, name='head_black')
        for x, ya, yb, w in gs:
            if True:
                add('s', 'l', x, ya, x, yb, 0.5)
                xa, xb = int(x - w / 2 - 2), int(x + w / 2 + 3)
                work[ya:yb, xa:xb][thin[ya:yb, xa:xb] > 0] = 0
        cv2.ellipse(work, (int(cx), int(cy)), (17, 15), 0, 0, 360, 0, -1)
    # --- flags at the stem tips: found by their template there, so that a tie or slur touching the flag does not hide it
    for fx, fa, fb, fbar in used_stems:
        if os.environ.get('P2M_TRACE') == 'flag' and 640 < fx < 700: print('STEM', fx, fa, fb, fbar)
        if fbar: continue
        best = None
        for nm, tip in (('flag_down', fa), ('flag_up', fb)):
            for wsp, hsp, tm, cnt, own in B.get(nm, [])[:2]:
                if not own: continue
                tm = tm.astype(np.uint8); th, tw = tm.shape; nblack = max(1, int(tm.sum()))
                nwhite = max(1, int(tm.size - tm.sum()))
                for dy in range(-6, 25, 2):         # (the flag may begin a little inside the stem)
                    y0 = int(tip + dy) if nm == 'flag_down' else int(tip - dy - th)
                    for dx in range(-8, 5, 2):
                        x0 = int(fx + dx)
                        if y0 < 0 or x0 < 0 or y0 + th > H or x0 + tw > W: continue
                        reg = work[y0:y0 + th, x0:x0 + tw]
                        rec = int((reg & tm).sum()) / nblack
                        if rec < 0.80: continue
                        wh = int(((1 - reg) & (1 - tm)).sum()) / nwhite
                        if wh < 0.55: continue
                        if best is None or rec + wh > best[0]:
                            best = (rec + wh, nm, x0, y0, tm)
        if best:
            _, nm, x0, y0, tm = best
            th, tw = tm.shape
            add('f', '@', x0, y0, x0 + tw, y0 + th, name=nm)
            work[y0:y0 + th, x0:x0 + tw][cv2.dilate(tm, np.ones((5, 5), np.uint8)) > 0] = 0
    # --- connected components of the rest
    n, lab, st, cen = cv2.connectedComponentsWithStats(work, connectivity=8)
    tips = [(x, y) for x, ya, yb, bar in used_stems if not bar for y in (ya, yb)]
    for i in range(1, n):
        x0, y0, w, h, area = st[i]
        if area < 40: continue
        m = (lab[y0:y0 + h, x0:x0 + w] == i).astype(np.uint8)
        wsp, hsp = w / PX, h / PX
        colsum = m.sum(0)
        fillr = area / (w * h)
        # dots
        if 0.2 < wsp < 0.6 and 0.2 < hsp < 0.6 and abs(w - h) <= 4 and fillr > 0.6:
            grow = 0
            if any(abs(x0 + w / 2 - bx) < 1.3 * PX for bx in thick_x) and any(a < y0 + h / 2 < b for a, b in staff_y):
                grow = max(0, (0.46 * PX - w) / 2)      # repeat dots are told by their size
            add('f', '@', x0 - grow, y0 - grow, x0 + w + grow, y0 + h + grow, name='dot'); continue
        # ledger line stubs and other small specks
        if h <= 7 and w < 1.2 * PX and fillr > 0.6 and not (0.8 * PX < w):
            continue
        tmed = float(np.median(colsum))
        para = area / max(1.0, tmed * w)
        if ',' in os.environ.get('P2M_TRACE', '') and ':' not in os.environ.get('P2M_TRACE', ''):
            tx0, ty0, tx1, ty1 = map(int, os.environ['P2M_TRACE'].split(','))
            if tx0 <= x0 <= tx1 and ty0 <= y0 <= ty1:
                print('TRACE', x0, y0, w, h, area, 'tmed', tmed, 'para', round(para, 2), 'colmax', colsum.max(), 'fill', round(fillr, 2),
                      [(round(sx_), sa, sb) for sx_, sa, sb, bar in used_stems if abs(sx_ - x0) < 60 or abs(sx_ - (x0 + w)) < 60 and sa - 60 < y0 < sb + 60])
        # beams: parallelograms that stems run into
        stub = 0.4 * PX < w <= 1.4 * PX and any(not bar and (abs(sx_ - x0) < 5 or abs(sx_ - (x0 + w)) < 5) and sa - 3 <= y0 and y0 + h <= sb + 3 for sx_, sa, sb, bar in used_stems)
        if (w > 0.8 * PX or stub) and 0.28 * PX <= tmed <= 0.7 * PX and 0.85 < para < 1.15 and colsum.max() < 1.5 * tmed + 3:
            touch = stub or any(x0 - 4 <= tx <= x0 + w + 4 and y0 - 6 <= ty <= y0 + h + 6 for tx, ty in tips)
            # the inner beams are crossed by the stems that end at the outer one
            touch = touch or sum(1 for sx_, sa, sb, bar in used_stems if not bar and x0 - 4 <= sx_ <= x0 + w + 4 and
                                 sa - 6 <= y0 and y0 + h <= sb + 6) >= 2
            if stub:      # widened so that it counts as a beam; away from its stem
                sxs = [sx_ for sx_, sa, sb, bar in used_stems if abs(sx_ - x0) < 5 or abs(sx_ - (x0 + w)) < 5]
                ext = max(0, int(0.86 * PX - w))
                if abs(sxs[0] - x0) < 5: w += ext
                else: x0 -= ext; w += ext
                m = cv2.resize(m, (w, h), interpolation=cv2.INTER_NEAREST)
            if touch or h > 0.75 * PX:
                ysl = np.nonzero(m[:, 1])[0]; ysr = np.nonzero(m[:, -2])[0]
                lt, lb, rt, rb = y0 + ysl[0], y0 + ysl[-1] + 1, y0 + ysr[0], y0 + ysr[-1] + 1
                pts = [P(X(x0), Y(lt)), P(X(x0 + w), Y(rt)), P(X(x0 + w), Y(rb)), P(X(x0), Y(lb))]
                add('f', 'lll', x0, y0, x0 + w, y0 + h, items=[('l', pts[0], pts[1]), ('l', pts[1], pts[2]), ('l', pts[2], pts[3])]); continue
            if fillr > 0.88:
                add('f', 'r', x0, y0, x0 + w, y0 + h); continue
        if fillr > 0.85 and 0.8 * PX < w < 1.6 * PX and h < 0.22 * PX:
            add('f', 'r', x0, y0, x0 + w, y0 + h); continue
        # line art: slurs, ties, hairpins, voltas
        dt = cv2.distanceTransform(cv2.copyMakeBorder(m, 1, 1, 1, 1, cv2.BORDER_CONSTANT), cv2.DIST_L2, 3)
        thick = 2 * dt.max()
        if w > 0.9 * PX and thick <= 0.38 * PX and fillr < 0.5 or (w > 2 * PX and thick <= 0.38 * PX):
            runs = np.array([np.count_nonzero(np.diff(np.concatenate(([0], m[:, c], [0]))) == 1) for c in range(w)])
            ys_l = np.nonzero(m[:, :3].any(1))[0]; ys_r = np.nonzero(m[:, -3:].any(1))[0]
            if h < 2 * PX and w > 2 * PX and min(ys_l[-1] - ys_l[0], ys_r[-1] - ys_r[0]) < 9 and max(ys_l[-1] - ys_l[0], ys_r[-1] - ys_r[0]) > 0.35 * PX and (runs >= 2).mean() > 0.3:        # hairpin
                open_left = (ys_l[-1] - ys_l[0]) > (ys_r[-1] - ys_r[0])
                if open_left:
                    vx, vy = x0 + w, y0 + ys_r.mean(); a_, b_ = (x0, y0 + ys_l[0]), (x0, y0 + ys_l[-1])
                else:
                    vx, vy = x0, y0 + ys_l.mean(); a_, b_ = (x0 + w, y0 + ys_r[0]), (x0 + w, y0 + ys_r[-1])
                add('s', 'll', x0, y0, x0 + w, y0 + h, 0.5, items=[('l', P(X(a_[0]), Y(a_[1])), P(X(vx), Y(vy))), ('l', P(X(vx), Y(vy)), P(X(b_[0]), Y(b_[1])))])
                continue
            rowsum = m.sum(1)
            if rowsum.max() > 0.9 * w and w > 3 * PX:           # volta bracket: a long horizontal line with hooks
                yl = y0 + int(np.argmax(rowsum))
                add('s', 'l', x0, yl + 2, x0 + w, yl + 2, 0.5)
                for c0 in (0, w - 6):
                    ysv = np.nonzero(m[:, c0:c0 + 6].any(1))[0]
                    if ysv[-1] - ysv[0] > 0.6 * PX:
                        xx = x0 + c0 + 3
                        add('s', 'l', xx, y0 + ysv[0], xx, y0 + ysv[-1], 0.5)
                continue
            if (runs <= 1).mean() > 0.9 and h < 9 * PX:         # slur or tie
                Lp = P(X(x0), Y(y0 + ys_l.mean())); Rp = P(X(x0 + w), Y(y0 + ys_r.mean()))
                ycm = (np.arange(h)[:, None] * m).sum() / area
                s = add('f', 'cc', x0, y0, x0 + w, y0 + h, items=[('c', Lp, Lp, Rp, Rp)])
                # the vertical centre of the shape tells the side of the bulge: keep it in the box
                continue
        if h > 6 * PX and wsp < 1.5:
            add('f', 'clc', x0, y0, x0 + w, y0 + h); continue      # brace
        # the other symbols: by size and overlap with the bank
        best = None
        for nm, lst in B.items():
            if nm in NO_COMP: continue
            for tw, th_, tm, cnt, own in lst:
                if abs(tw - wsp) > 0.1 + 0.08 * tw or abs(th_ - hsp) > 0.1 + 0.08 * th_: continue
                r = cv2.resize(tm.astype(np.uint8), (w, h), interpolation=cv2.INTER_NEAREST)
                iou = (r & m).sum() / max(1, (r | m).sum())
                if best is None or iou > best[0]: best = (iou, nm)
        if best and best[0] > 0.62:
            add('f', '@', x0, y0, x0 + w, y0 + h, name=best[1])
        else:
            add('f', '?', x0, y0, x0 + w, y0 + h)
    if debug:
        vis = cv2.cvtColor(255 - bw * 255, cv2.COLOR_GRAY2BGR)
        col = {'s': (0, 160, 0)}
        for s in shapes:
            a = (int((s.x0 - bbox[0]) / sx), int((s.y0 - bbox[1]) / sy)); b = (int((s.x1 - bbox[0]) / sx), int((s.y1 - bbox[1]) / sy))
            c = (0, 160, 0) if s.kind == 's' else (0, 0, 255) if s.name else (255, 0, 0) if s.ops == '?' else (200, 0, 200)
            if s.kind == 's' and abs(a[1] - b[1]) < 2 and b[0] - a[0] > 20 * PX: continue
            cv2.rectangle(vis, a, b, c, 2)
            if s.name and not s.name.startswith('head'):
                cv2.putText(vis, s.name, (a[0], a[1] - 3), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)
            elif s.ops not in ('@', 'l', '?'):
                cv2.putText(vis, s.ops, (a[0], a[1] - 3), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 0, 200), 2)
        cv2.imwrite(debug, vis)
        cv2.imwrite(debug.replace('.png', '_work.png'), 255 - work * 255)
    return shapes
