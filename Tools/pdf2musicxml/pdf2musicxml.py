#!/usr/bin/env python3
"""pdf2musicxml - converts vector piano score PDFs (exported from a notation program or Illustrator) to MusicXML.

The Hungarian usage text shown by --help is in USAGE.
"""

USAGE = """pdf2musicxml - vektoros (kottaszerkesztobol / Illustratorbol exportalt) zongorakotta PDF -> MusicXML.

Hasznalat:
    python pdf2musicxml.py kotta.pdf                 -> kotta.musicxml + kotta.report.txt
    python pdf2musicxml.py *.pdf -o kimenet_mappa
    python pdf2musicxml.py kotta.pdf --title "Cim" --composer "Szerzo" --tempo "Allegretto" --bpm 104

Fuggoseg: pip install pymupdf
Opcionalis: Tesseract OCR (a cim, szerzo es tempo kiolvasasahoz; nelkule a fajlnevbol veszi a cimet).
"""
import argparse
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unicodedata
from fractions import Fraction as F

import pymupdf

from p2m import musicxml, compare
from p2m.midi import Midi
from p2m.musicxml import _jump_sound
from p2m.recognize import analyse_page, solve_measure


FULL_WORDS = {'dolce', 'legato', 'leggiero', 'cantabile', 'espressivo', 'sostenuto', 'tempo', 'fine', 'tranquillo',
              'grazioso', 'animato', 'agitato', 'marcato', 'staccato', 'simile', 'sempre', 'subito', 'morendo',
              'crescendo', 'diminuendo', 'ritardando', 'rallentando', 'accelerando', 'calando', 'smorzando'}


def ocr(page, rect, psm=7):
    exe = shutil.which('tesseract') or next((p for p in (
        r'C:\Program Files\Tesseract-OCR\tesseract.exe',
        r'C:\Program Files (x86)\Tesseract-OCR\tesseract.exe') if os.path.exists(p)), None)
    if not exe:
        return None
    with tempfile.TemporaryDirectory() as td:
        png = os.path.join(td, 'a.png')
        page.get_pixmap(clip=rect + (-6, -4, 6, 4), dpi=400).save(png)
        try:
            r = subprocess.run([exe, png, 'stdout', '--psm', str(psm), '-l', 'eng'],
                               capture_output=True, text=True, encoding='utf-8', timeout=60)
            return r.stdout.strip()
        except Exception:
            return None


def text_lines(shapes, y_from, y_to):
    """Filled shapes in the given band, grouped into lines -> bounding rectangles."""
    items = sorted([s for s in shapes if s.kind != 's' and y_from <= s.cy <= y_to and s.h < 40], key=lambda s: s.y0)
    lines = []
    for s in items:
        if lines and s.y0 < lines[-1].y1 - 1:
            lines[-1] |= pymupdf.Rect(s.x0, s.y0, s.x1, s.y1)
        else:
            lines.append(pymupdf.Rect(s.x0, s.y0, s.x1, s.y1))
    return [r for r in lines if r.width > 15]


def title_from_name(name):
    """'Classic09 Polonaise op.53 Héroïque' -> 'Polonaise op.53 Héroïque'"""
    name = re.sub(r'^[0-9a-f]{8}-', '', name)
    name = re.sub(r'^[A-Za-z]+\d+[\s_]+', '', name)
    return unicodedata.normalize('NFC', name).strip()


def find_midi(pdf, where, exts=('.mid', '.midi')):
    """The MIDI file of the PDF: the given file, or the one with the most similar name in the folder."""
    if os.path.isfile(where):
        return where
    def norm(s):
        # comparison independent of accents, spaces and punctuation (Héroïque = Heroique = start of H_ro_que), NFC and NFD alike
        s = re.sub(r'^[0-9a-f]{8}-', '', os.path.splitext(os.path.basename(s))[0])
        s = ''.join(c for c in unicodedata.normalize('NFKD', s) if not unicodedata.combining(c))
        return re.sub(r'[^a-z0-9]', '', s.lower())
    a = norm(pdf)
    best = None
    for f in [f for e in exts for f in glob.glob(os.path.join(where, '*' + e))]:
        b = norm(f)
        n = 0
        while n < min(len(a), len(b)) and a[n] == b[n]: n += 1
        if best is None or n > best[0]: best = (n, f)
    return best[1] if best and best[0] >= 6 else None


def write_trimmed_midi(mid, md, measures, out_dir):
    """Cuts as many WHOLE measures from the start of the MIDI file as it starts later than the score.
    With a pickup the partial rest of the pickup measure stays (the first MIDI measure is whole).
    Barlines are counted by the time signature changes of the MIDI file (e.g. a 4/4 lead-in, then 3/4)."""
    from p2m.miditrim import trim
    L = ["", "=== MIDI vagas ==="]
    guide = [n for n in md.notes if n.chan in (14, 15)] or md.notes
    ts = sorted((t, a, b) for t, a, b in md.timesigs) or [(0, 4, 4)]
    if ts[0][0] > 0: ts.insert(0, (0, 4, 4))

    def bar_starts(limit):
        out, t, k = [], 0, 0
        while t <= limit:
            while k + 1 < len(ts) and ts[k + 1][0] <= t: k += 1
            out.append(t)
            t += md.ppq * 4 * ts[k][1] // ts[k][2]
        return out
    m0 = measures[0]
    ons = [e.onset for _, _, evs in m0.voices for e in evs if e.heads]
    out = os.path.join(out_dir, os.path.basename(mid))
    os.makedirs(out_dir, exist_ok=True)
    cut = 0
    if ons:
        offset = min(n.start for n in guide) - int(min(ons) * md.ppq)     # start of measure 1 of the score in the MIDI file
        if getattr(m0, 'implicit', False):
            bs = bar_starts(offset + 1)
            prev = max(b for b in bs if b <= offset)
            nxt = next((b for b in bar_starts(offset + 100000) if b > offset), offset)
            if offset in bs:
                # the pickup starts at a barline in the MIDI file too: the partial rest is in the preceding measure
                L.append(f"  felutes: {float(m0.length):g} negyed")
            else:
                L.append(f"  felutes: {float(m0.length):g} negyed, elotte {(nxt - offset - int(m0.length * md.ppq) + (offset - prev)) / md.ppq:g} negyed szunet marad")
            offset = prev
        bs = bar_starts(offset)
        if offset not in bs:
            L.append(f"  ! a kotta kezdete nem esik MIDI-utemvonalra (eltolas {offset} tick) - nem vagtam")
        else:
            cut = offset
    if any(n.start < cut for n in md.notes):
        L.append(f"  ! hang szol a levagando reszben - nem vagtam")
        cut = 0
    if cut:
        nbars = bar_starts(cut).index(cut)
        msg = trim(mid, out, cut_ticks=cut)
        L.append(f"  {nbars} ures utem levagva ({cut} tick); SysEx es minden mas uzenet megmaradt")
        if 'utemmutato elhagyva' in msg:
            L.append("  a levagott elouetem sajat utemmutatoja elmaradt (a 0. idoponton a darab utemmutatoja ervenyes)")
    else:
        shutil.copyfile(mid, out)
        L.append("  nincs plusz utem, a MIDI valtozatlan")
    return L


def join_wedges(measures):
    """A hairpin broken at the end of a system is drawn in two pieces, and the lines of one piece do not meet
    (crescendo: the second piece, diminuendo: the first one). The two pieces become one hairpin."""
    seq = [(i, d) for i, m in enumerate(measures) for d in sorted(m.directions, key=lambda d: d[0]) if d[2] == 'wedge']
    drop = []
    for k, (i, d) in enumerate(seq):
        if len(d) < 5 or d[3] == 'stop':
            continue
        if d[3] == 'crescendo' and k >= 2 and seq[k - 1][1][3] == 'stop' and seq[k - 2][1][3] == 'crescendo' and i - seq[k - 1][0] <= 1:
            drop += [seq[k - 1], seq[k]]
        elif (d[3] == 'diminuendo' and k + 2 < len(seq) and seq[k + 1][1][3] == 'stop' and seq[k + 2][1][3] == 'diminuendo'
              and seq[k + 2][0] - seq[k + 1][0] <= 1):
            drop += [seq[k + 1], seq[k + 2]]
    for i, d in drop:
        if d in measures[i].directions: measures[i].directions.remove(d)


def link_curves(systems, measures):
    """Curves: tie (same note) or slur; joining the ones that cross a system break (within one part)."""
    order, voice_of = {}, {}
    for i, m in enumerate(measures):
        for st, vn, evs in m.voices:
            for e in evs: order[id(e)] = (i, e.onset); voice_of[id(e)] = (st, vn)
    # the order in which the notes are written to the file: measure, voice, onset
    forder = {id(e): (i, k, e.onset) for i, m in enumerate(measures) for k, (_, _, evs) in enumerate(m.voices) for e in evs}

    def notes_between(ea, eb):
        # notes of the same voice between the two ends: then the curve is a slur, not a tie
        if id(ea) not in order or id(eb) not in order:
            return False
        oa, ob, va = order[id(ea)], order[id(eb)], voice_of[id(ea)]
        for i in range(oa[0], min(ob[0], len(measures) - 1) + 1):
            for st, vn, evs in measures[i].voices:
                if (st, vn) == va and any(e.heads and not e.grace and oa < order[id(e)] < ob for e in evs):
                    return True
        return False
    slurs, pending = [], []
    cur = {'sy': None, 'c': None}

    def prev_tail(b):
        i = systems.index(cur['sy']); c = cur['c']
        if i == 0 or not systems[i - 1].measures: return None
        if c['R'][0] - c['L'][0] > 2.5 * cur['sy'].sp or abs(c['y'] - b.g.cy) > 1.3 * cur['sy'].sp: return None
        for e in sorted(systems[i - 1].measures[-1].events, key=lambda e: -e.x):
            for h in e.heads:
                if h.staff == b.staff and h.pos == b.pos: return (h, e)
        return None
    for sy in systems:
        cur['sy'] = sy
        for c in sorted(getattr(sy, 'curves', []), key=lambda c: c['y']):
            cur['c'] = c
            if c['a'] is None and c['b'] is not None:
                same = [k for k in range(len(pending)) if pending[k]['a'].staff == c['b'].staff and pending[k]['a'].pos == c['b'].pos]
                if same:
                    p = pending.pop(same[0]); c = dict(c, a=p['a'], ea=p['ea'], stitched=True)
                elif pending and not prev_tail(c['b']):
                    k = min(range(len(pending)), key=lambda k: abs(pending[k]['a'].staff - c['b'].staff))
                    p = pending.pop(k); c = dict(c, a=p['a'], ea=p['ea'], stitched=True)
                else:
                    t_ = prev_tail(c['b'])
                    if not t_:
                        continue
                    c = dict(c, a=t_[0], ea=t_[1], stitched=True)
            elif c['b'] is None:
                if c['a'] is not None: pending.append(c)
                continue
            a, b = c['a'], c['b']
            art = lambda e: e.staccato or e.tenuto or e.stacciss
            edge = (c.get('stitched') or 'L' not in c or
                    (c['L'][0] - a.g.cx > 0.2 * a.g.w or c['R'][0] - b.g.cx < -0.2 * b.g.w))
            if (a.staff == b.staff and a.pos == b.pos and c['ea'] is not c['eb'] and not art(c['ea']) and not art(c['eb']) and edge
                    and not notes_between(c['ea'], c['eb']) and not c['ea'].grace):     # from a grace note: a slur
                a.tie_start = True; b.tie_stop = True
                a.tie_over = c.get('over')      # the side of the tie in the PDF
            elif c['ea'] is not c['eb']:
                slurs.append((c['ea'], c['eb'], c.get('over')))
        pending = [p for p in pending if p.get('age', 0) < 1]
        for p in pending: p['age'] = 1
    slurs = [s for s in slurs if id(s[0]) in order and id(s[1]) in order and order[id(s[0])] < order[id(s[1])]]
    slurs.sort(key=lambda s: forder[id(s[0])])
    active = {}
    for a, b, over in slurs:
        # a number is free again only when its slur has ended earlier in the file (or on this very note): a slur
        # of the other staff that ends at the same time is written later, and its number must not be reused yet
        for num in [k for k, v in active.items() if forder[id(v)] < forder[id(a)] or v is a]:
            del active[num]
        num = next(k for k in range(1, 9) if k not in active)
        active[num] = b
        a.slur_start.append(num); b.slur_stop.append(num)
        if over is not None:
            a.slur_place[num] = 'above' if over else 'below'


def implied_triplets(m, L):
    """Triplets written without their number (after the first ones, "simile"): if a voice of a staff (the notes
    with the stems in the same direction) is too long for the measure, and it fits exactly when some of its beamed
    groups of three, six or twelve equal notes are triplets, they are triplets."""
    for st in (0, 1):
        allv = [e for e in m.events if e.staff == st and not e.grace and e.dur]
        ups = [e for e in allv if e.heads and e.stem == 'up']; downs = [e for e in allv if e.heads and e.stem == 'down']
        # stems up and down at the same place: two voices; one after the other: one voice (with its rests)
        # (heads of two voices on the same pitch stand side by side: one head width apart)
        two = any(abs(a.x - b.x) < 1.3 * a.heads[0].g.w for a in ups for b in downs) or \
            any(e.heads and e.stem is None for e in allv)
        for evs in ([ups, downs] if two else [allv]):
            total = sum((e.dur for e in evs), F(0))
            if total <= L:
                continue
            groups = {}
            for e in evs:
                if e.heads and e.beam_ids and e.tuplet is None:
                    groups.setdefault(max(e.beam_ids), []).append(e)
            cand = [g for g in groups.values() if len(g) in (3, 6, 12) and len({e.dur for e in g}) == 1]
            # a triplet with a rest: a rest and two notes beamed together, of the same length, one after the other
            # (the rest before, between or after the notes); the beam holds these two notes only
            taken = {id(e) for g in cand for e in g}
            seq = sorted(evs, key=lambda e: e.x)
            k = 0
            while k + 3 <= len(seq):
                u = seq[k:k + 3]
                notes = [e for e in u if e.heads]
                whole_beams = all(len(groups.get(max(e.beam_ids), [])) == sum(1 for o in notes if o.beam_ids and max(o.beam_ids) == max(e.beam_ids))
                                  for e in notes if e.beam_ids)
                paired = len(notes) == 2 and all(e.beam_ids for e in notes) and max(notes[0].beam_ids) == max(notes[1].beam_ids)
                if (len({e.dur for e in u}) == 1 and paired and whole_beams and
                        all(e.tuplet is None and id(e) not in taken and not getattr(e, 'whole_measure', False) for e in u)):
                    cand.append(u); k += 3
                else:
                    k += 1
            cand.sort(key=lambda g: min(e.x for e in g))
            gain = [sum((e.dur for e in g), F(0)) / 3 for g in cand]
            excess = total - L
            if sum(gain, F(0)) == excess:
                chosen = cand
            else:
                continue
            for g in chosen:
                g.sort(key=lambda e: e.x)
                for k in range(0, len(g), 3):
                    for e in g[k:k + 3]:
                        e.dur = e.dur * F(2, 3); e.tuplet = (3, 2)
                    g[k].tuplet_start = True; g[k + 2].tuplet_stop = True


def finish_part(measures, systems, sp):
    """Measure lengths, voices, pickup, curves, voltas for one part."""
    time = (4, 4, None)
    for i, m in enumerate(measures):
        if m.time: time = m.time
        nominal = F(time[0] * 4, time[1])
        jl = getattr(m, 'jlen', None)
        if jl is not None:
            L = F(str(jl)).limit_denominator(96)
            if systems and getattr(systems[0], 'finale', False):
                implied_triplets(m, L)
            solve_measure(m, L, sp, False)
            if i == 0 and L < nominal:
                m.implicit = True
        else:
            solve_measure(m, nominal, sp, i == 0)
    if len(measures) > 1 and getattr(measures[0], 'implicit', False):
        last = measures[-1]
        end = max([e.onset + e.dur for _, _, evs in last.voices for e in evs] or [F(0)])
        if end + measures[0].length == last.length:
            last.length = end; last.warnings = [w for w in last.warnings if 'rovidebb' not in w and 'utemhossz' not in w]
    link_curves(systems, measures)
    join_wedges(measures)
    cur = None
    for i, m in enumerate(measures):
        v = m.volta
        if isinstance(v, int):
            m.ending_start = v; cur = v
        elif v != 'cont':
            cur = None
        if cur is not None:
            nxt = measures[i + 1].volta if i + 1 < len(measures) else None
            if nxt != 'cont':
                m.ending_stop = (cur, 'stop' if m.right_bar == 'repeat-end' else 'discontinue'); cur = None


def convert(pdf, out_dir, args):
    name = os.path.splitext(os.path.basename(pdf))[0]
    rep = []
    log = rep.append
    doc = pymupdf.open(pdf)
    unknown, texts = [], []
    part_measures, part_systems, part_names = {}, {}, {}
    meta = dict(title=args.title, composer=args.composer, tempo_text=args.tempo, bpm=args.bpm)
    sp = 5.0
    jfile = find_midi(pdf, args.json, ('.json',)) if args.json else None
    if jfile is None and os.path.exists(os.path.splitext(pdf)[0] + '.json'):
        jfile = os.path.splitext(pdf)[0] + '.json'
    jdata = None
    if jfile and not args.no_json:
        try:
            jdata = json.load(open(jfile, encoding='utf-8'))
        except Exception as ex:
            log(f"  ! a json nem olvashato: {ex}")
    for pi, page in enumerate(doc):
        jb = [dict(b, idx=i) for i, b in enumerate(jdata['bars']) if b['page'] == pi + 1 and b['w'] > 0] if jdata else None
        systems, glyphs, shapes = analyse_page(page, log, jb)
        if not systems:
            continue
        if pi == 0:
            top = min(sy.top for sy in systems) - 9 * systems[0].sp
            lines = text_lines(shapes, 0, top)
            got = [g for g in (ocr(page, r) for r in lines) if g]
            if got:
                if not meta['title']: meta['title'] = got[0]
                if len(got) > 2 and got[1].lstrip().startswith('('):
                    got = [got[0]] + got[2:]
                if not meta['composer'] and len(got) > 1: meta['composer'] = got[1]
            foot = text_lines(shapes, page.rect.height - 45, page.rect.height)
            if foot:
                meta['rights'] = (ocr(page, foot[0]) or '').replace('©', '(c)') or None
        groups = sorted({sy.group for sy in systems})
        if systems and getattr(systems[0], 'finale', False):
            finale_texts(page, pi, systems, meta, texts)
        for sy in systems:
            si = groups.index(sy.group)
            for c in getattr(sy, 'text_clusters', []):
                rect = pymupdf.Rect(c['x0'], c['y0'], c['x1'], c['y1'])
                big = max(g.h for g in c['g']) > 1.55 * sy.sp
                is_tempo = pi == 0 and si == 0 and sy.part == 0 and c['y1'] < sy.top and big and c['x0'] < sy.x0 + 14 * sy.sp
                tall = rect.height > 2.6 * sy.sp
                t = (ocr(page, rect, 6 if tall else 7) or '').strip()
                if tall and is_tempo: t = t.split('\n')[0]
                t = re.sub(r'\s+', ' ', t).strip(' _|~')
                if c['x1'] < sy.x0 + 0.5 * sy.sp:          # part name before the system (e.g. "Piano I")
                    t = re.sub(r'[^A-Za-z0-9 .\-]', '', t).strip()
                    m_ = re.match(r'(Piano|Pf|Klavier)\s*(I+|[12])', t, re.I)
                    if m_: t = 'Piano ' + {'1': 'I', '2': 'II'}.get(m_.group(2), m_.group(2).upper())
                    if t: part_names.setdefault(sy.part, t)
                    for g in c['g']: g.used = True
                elif is_tempo:
                    meta['tempo_left'] = (sy.x0, sy.sp)
                    log(f"tempo-sor OCR: {t!r}")
                    mm = re.search(r'[=:]\s*([0-9]{2,3})', t) or re.search(r'([0-9]{2,3})\s*\)?\s*$', t)
                    word = re.match(r'[A-Za-z][A-Za-z .]+', t)
                    if meta['bpm'] is None and mm: meta['bpm'] = int(mm.group(1))
                    if not meta['tempo_text'] and word: meta['tempo_text'] = re.sub(r'\s+[A-Za-z]$', '', word.group(0).strip())
                    for g in c['g']: g.used = True
                elif len(re.findall(r'[A-Za-z0-9]', t)) >= 2:
                    # full words are not abbreviations: a dot read after them is OCR noise
                    if t.endswith('.') and t[:-1].split()[-1].lower() in FULL_WORDS: t = t[:-1]
                    place = 'above' if (c['y0'] + c['y1']) / 2 < sy.staves[0].top else 'below'
                    # texts below the top line of the lower staff are placed relative to the lower staff
                    ws = 1 if (c['y0'] + c['y1']) / 2 > sy.staves[1].top else 0
                    # D.C., Fine: written at the end of their section, they belong to the measure where the text ends
                    # (a long one, e.g. "Menuetto D.C. al Fine", begins a measure earlier)
                    tm = sy.measure_at(c['x1'] - sy.sp) if _jump_sound(t) else sy.measure_at(c['x0'])
                    tm.directions.append((c['x0'], ws, 'words', t, place,
                                                                 round((sy.staves[ws].top - c['y1']) / sy.sp * 10), sy.sp))
                    texts.append((pi + 1, si + 1, t))
                    for g in c['g']: g.used = True
            sy.unknown = [g for g in getattr(sy, 'unknown', []) if not g.used]
        for sy in systems:
            sp = sy.sp
            si = groups.index(sy.group)
            if sy.measures and si == 0 and pi > 0:
                sy.measures[0].new_page = True
            for m in sy.measures:
                m.page, m.system = pi + 1, si + 1
            part_measures.setdefault(sy.part, []).extend(sy.measures)
            part_systems.setdefault(sy.part, []).append(sy)
            for g in getattr(sy, 'unknown', []):
                unknown.append((pi + 1, si + 1, g))
            for s_ in getattr(sy, 'unused_strokes', []):
                from p2m.recognize import Glyph
                unknown.append((pi + 1, si + 1, Glyph('vonal', s_.x0, s_.y0, s_.x1, s_.y1, s_.ops)))
    if not args.title_from_pdf and not args.title:
        meta['title'] = title_from_name(name)
    if not meta['title']:
        meta['title'] = title_from_name(name)
    pkeys = sorted(part_measures)
    nparts = len(pkeys)
    for k in pkeys:
        finish_part(part_measures[k], part_systems[k], sp)
    measures = part_measures[pkeys[0]] if pkeys else []
    counts = [len(part_measures[k]) for k in pkeys]
    if jdata:
        jreal = sum(1 for b in jdata['bars'] if b['len'] > 0 and b['w'] > 0)
        jhidden = sum(1 for b in jdata['bars'] if b['w'] <= 0)
        log(f"json: {os.path.basename(jfile)} - {jreal} utem (+{len(jdata['bars']) - jreal - jhidden} nulla hosszu"
            + (f", +{jhidden} a lapon nem szereplo eloutem" if jhidden else '') + f"), felutes elotti szunet: {jdata.get('auftakt')} negyed")
        for k in pkeys:
            if len(part_measures[k]) != jreal:
                log(f"  ! {part_names.get(k, k + 1)}: {len(part_measures[k])} utem, a json szerint {jreal}")
    if len(set(counts)) > 1:
        log(f"  ! a szolamok utemszama elter: {counts}")
    t0 = next((m.time for m in measures if m.time), (4, 4, None))
    if t0[1] == 8 and t0[0] % 3 == 0:
        meta['beat_dot'] = True
    if meta.get('tempo_left') and measures and measures[0].voices:
        # reference: the first column of the measure (a whole-measure rest stands in the middle, so not the first voice)
        xs = [e.x for _, _, evs in measures[0].voices for e in evs if e.onset == 0]
        if xs:
            x0, sp0 = meta['tempo_left']
            meta['tempo_dx'] = round((x0 - min(xs)) / sp0 * 10)
    if args.tempo_x is not None:
        meta['tempo_dx'] = args.tempo_x
    for k in pkeys:
        for m in part_measures[k]:
            if not hasattr(m, 'page'): m.page, m.system = 0, 0
        musicxml.assign_pitches(part_measures[k])
    # ---- MIDI comparison per part
    midi_lines, part_diffs, part_stats = [], {}, {}
    mid = find_midi(pdf, args.midi) if args.midi else None
    if not args.midi:
        # the MIDI file beside the PDF with the same name is found without --midi (like the .json file)
        for ext in ('.mid', '.midi'):
            if os.path.isfile(os.path.splitext(pdf)[0] + ext):
                mid = os.path.splitext(pdf)[0] + ext; break
    if args.midi and not mid:
        midi_lines = ["", "=== MIDI osszevetes ===", "  nem talaltam a PDF-hez tartozo MIDI fajlt"]
    tempo_lines = []
    if mid:
        md = Midi(mid)
        start = None
        for k in pkeys:
            ms = part_measures[k]
            if k == 0:
                chans, tol = None, 0
            else:
                chans, tol = (2 * k, 2 * k + 1), md.ppq // 8
            order = None
            if jdata and jdata.get('seqs'):
                idx = {getattr(m, 'jid', None): i for i, m in enumerate(ms)}
                order = [idx[b] for e in jdata['seqs'] for b in e if b in idx]
            diffs, stats = compare.compare(ms, md, log, chans=chans, tol=tol, start=start, order=order)
            if stats.get('fixed'):
                musicxml.assign_pitches(ms)
                diffs, st2 = compare.compare(ms, md, log, chans=chans, tol=tol, start=start, order=order)
                st2['fixed'] = stats['fixed']; st2['fixed_at'] = stats['fixed_at']; st2['ott_conflict'] = stats['ott_conflict']; stats = st2
            if k == 0:
                start = stats.get('start')
                set_tempos(meta, ms, md, stats, tempo_lines)
            part_diffs[k], part_stats[k] = diffs, stats
            L = compare.report(diffs, stats, ms)
            if nparts > 1:
                L[1] = f"=== MIDI osszevetes: {part_names.get(k, f'{k + 1}. szolam')} (csatornak: {', '.join(str(c + 1) for c in stats.get('chans', ()))}" + \
                       (", idotures" if tol else "") + ") ==="
            midi_lines += L
        midi_lines.insert(2, f"  MIDI: {os.path.basename(mid)}")
        xf = [t for _, _, t in md.texts if t.startswith('XFln:')]
        if xf and len(xf[0].split(':')) > 3:
            midi_lines.insert(3, f"  MIDI szerinti cim/szerzo: {xf[0].split(':')[2]} / {xf[0].split(':')[3]}")
            if not args.composer and xf[0].split(':')[3].isascii() and xf[0].split(':')[3].strip():
                meta['composer'] = xf[0].split(':')[3].strip()
        if args.midi_out:
            midi_lines += write_trimmed_midi(mid, md, measures, args.midi_out)
    if nparts > 1:
        meta['parts'] = [(part_names.get(k, f'Piano {"I" * (k + 1)}'), part_measures[k]) for k in pkeys]
    meta['shown_parts'] = set(part_names.values())
    meta['measures'] = measures
    out = os.path.join(out_dir or os.path.dirname(os.path.abspath(pdf)), name + '.musicxml')
    musicxml.write(meta, out)
    # ---- report
    nwarn = 0
    lines = [f"Forras: {pdf}", f"Kimenet: {out}",
             f"Cim: {meta['title']!r}  Szerzo: {meta.get('composer')!r}  Tempo: {meta.get('tempo_text')!r} = {meta.get('bpm')}",
             f"Utemek szama: {len(measures)}" + (f"  Szolamok: {', '.join(part_names.get(k, str(k + 1)) for k in pkeys)} ({counts})" if nparts > 1 else ''), ""]
    for k in pkeys:
        for i, m in enumerate(part_measures[k], 1):
            for w in m.warnings:
                nwarn += 1
                pn = f"{part_names.get(k, k + 1)}, " if nparts > 1 else ''
                lines.append(f"  ! {pn}{i}. utem (oldal {m.page}, rendszer {m.system}): {w}")
    kinds = {}
    for p, s, g in unknown:
        k = f"vonal ({g.ops}, {g.w:.1f}x{g.h:.1f})" if g.name == 'vonal' else g.name if g.name != '?' else f"ismeretlen alak ({len(g.ops)} muvelet, {g.w:.1f}x{g.h:.1f})"
        kinds.setdefault(k, []).append(f"o{p}/r{s} x={g.x0:.0f} y={g.y0:.0f}")
    lines.append("")
    lines.append(f"Fel nem hasznalt jelek: {len(unknown)}")
    for k, v in sorted(kinds.items(), key=lambda kv: -len(kv[1])):
        lines.append(f"  {len(v):3d} x {k}: " + '; '.join(v[:6]) + (' ...' if len(v) > 6 else ''))
    lines += midi_lines
    if tempo_lines:
        lines += ["", "=== Tempo ==="] + tempo_lines
    lines += ["", "OCR-rel olvasott szovegek (ellenorizd):"] + [f"  o{p}/r{s_}: {t}" for p, s_, t in texts] + [""] + rep
    with open(os.path.splitext(out)[0] + '.report.txt', 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    ndiff = sum(len(d) for d in part_diffs.values())
    nfix = sum(st.get('fixed', 0) for st in part_stats.values())
    extra = (f", MIDI-elteres {ndiff} utemben" + (f", 8va-javitas {nfix} hang" if nfix else '')) if mid else ''
    pinfo = f" ({nparts} szolam)" if nparts > 1 else ''
    print(f"{os.path.basename(pdf)}: {len(measures)} utem{pinfo}, {nwarn} figyelmeztetes, {len(unknown)} fel nem hasznalt jel{extra} -> {out}")
    return measures, unknown


TEMPO_WORD = re.compile(r'(?i)^(molto |poco |un poco |pi[uù] |meno |tempo di |mouvement )?'
                        r'(allegr|andant|adagio|largo|larghetto|lento|lent\b|moderato|presto|prestissimo|vivace|vivo|'
                        r'maestoso|grave|comodo|mosso|marcia|valse|menuett|langsam)')


def is_tempo_mark(text):
    """A new tempo written in the score (Allegro, Andante, Più lento, Tempo di Valse): it begins with a capital."""
    t = text.strip()
    if not t or not t[0].isupper() or re.search(r'D\.\s*[CS]\.|Fine|Coda', t):
        return False
    return bool(TEMPO_WORD.match(t)) and not re.match(r'(?i)tempo\s+(I\b|1\b|primo)', t)


def finale_texts(page, pi, systems, meta, texts):
    """The bold texts of a score made with Finale are plain text in the PDF: the tempo at the beginning of the piece,
    a new tempo later, and the titles of the sections (Thema, Var. 1)."""
    spans = []
    for b in page.get_text('dict')['blocks']:
        for ln in b.get('lines', []):
            for sp_ in ln['spans']:
                t = sp_['text'].strip()
                if 'Bold' in sp_['font'] and 'Ital' not in sp_['font'] and len(t) > 1 and not re.fullmatch(r'[\d. ]+', t):
                    spans.append((sp_['bbox'], t))
    groups = sorted({sy.group for sy in systems})
    for (x0, y0, x1, y1), t in sorted(spans, key=lambda s_: (s_[0][1], s_[0][0])):
        # the system below the text (of the first part, if there are several)
        below = [sy for sy in systems if sy.top > y0 and sy.top - y1 < 12 * sy.sp]
        if not below:
            continue
        sy = min(below, key=lambda sy: (sy.top, sy.part))
        si = groups.index(sy.group)
        first = pi == 0 and si == 0 and x0 < sy.x0 + 14 * sy.sp
        if first and not meta.get('tempo_text') and is_tempo_mark(t):
            meta['tempo_text'] = t
            meta['tempo_left'] = (sy.x0, sy.sp)
        else:
            sy.measure_at(x0).directions.append((x0, 0, 'words', t, 'above', round((sy.staves[0].top - y1) / sy.sp * 10), sy.sp, 'bold'))
        texts.append((pi + 1, si + 1, t))
    # D.C. al Fine, Fine (italic): written at the end of their section, they belong to the measure where the text ends
    for bl in page.get_text('dict')['blocks']:
        for ln in bl.get('lines', []):
            for sp_ in ln['spans']:
                t = re.sub(r'\s+', ' ', sp_['text']).strip()
                if 'Ital' not in sp_['font'] or not _jump_sound(t):
                    continue
                x0, y0, x1, y1 = sp_['bbox']
                cy = (y0 + y1) / 2
                sy = min(systems, key=lambda sy: 0 if sy.top <= cy <= sy.bottom else min(abs(cy - sy.top), abs(cy - sy.bottom)))
                ws = 1 if cy > sy.staves[1].top else 0
                place = 'above' if cy < sy.staves[0].top else 'below'
                sy.measure_at(x1 - sy.sp).directions.append((x0, ws, 'words', t, place, round((sy.staves[ws].top - y1) / sy.sp * 10), sy.sp))
                texts.append((pi + 1, groups.index(sy.group) + 1, t))


def midi_tempo(md, t0, span):
    """The tempo of the MIDI file from t0 for span (both in quarters), in quarters per minute: the median of the
    tempo values weighted by their time (half of the time the music is slower, half of the time faster). The
    recordings are played freely, a new tempo stands at almost every beat: the median is the value the others
    gather around, and a slow beginning or the end of a phrase does not move it as it moves the average."""
    ev = sorted(md.tempos)
    if not ev or span <= 0:
        return None
    a, b = float(t0) * md.ppq, float(t0 + span) * md.ppq
    cur = [v for t, v in ev if t <= a]
    cur = cur[-1] if cur else ev[0][1]
    seg, last = [], a
    for t, v in ev:
        if a < t < b:
            seg.append((t - last, cur)); cur = v; last = t
    seg.append((b - last, cur))
    seg = sorted((d, v) for d, v in seg if d > 0)
    seg.sort(key=lambda x: x[1])
    half, acc = sum(d for d, _ in seg) / 2, 0.0
    for d, v in seg:
        acc += d
        if acc >= half:
            return v
    return None


def beat_unit(time):
    """The note of the metronome mark for a time signature: the note of its bottom number (an eighth in 6/8, a
    quarter in 3/4 and 4/4, a half in 2/2). Returns (name, dotted, its length in quarters)."""
    bottom = time[1] if time else 4
    return {2: ('half', False, 2.0), 8: ('eighth', False, 0.5), 16: ('16th', False, 0.25)}.get(bottom, ('quarter', False, 1.0))


def set_tempos(meta, ms, md, stats, tempo_lines):
    """The tempo of the beginning of the piece and of the tempo marks in it. The tempo of the beginning is always
    written: the metronome mark of the score, otherwise the median of the first six measures of the MIDI file (see
    midi_tempo), for the note of the bottom number of the time signature. A new tempo mark gets the tempo of the MIDI file
    at its place; "a tempo" the tempo in force before it, "Tempo I" the tempo of the beginning."""
    order = stats.get('order') or []
    start = stats.get('start')
    if md is None or start is None or not order:
        return
    at, t = {}, start        # the first time a measure is played, in quarters
    times = []
    for k in order:
        at.setdefault(k, len(times)); times.append(t); t += ms[k].length

    def tempo_at(k, n=6):
        if k not in at:
            return None
        i = at[k]
        span = sum(ms[j].length for j in order[i:i + n])
        return midi_tempo(md, times[i], span)

    typical = None
    seg, evs = [], sorted(md.tempos)
    end = float(t) * md.ppq
    for (ta, va), (tb, _) in zip(evs, evs[1:] + [(end, 0)]):
        ta, tb = max(ta, float(start) * md.ppq), min(tb, end)
        if tb > ta: seg.append((tb - ta, va))
    if seg:
        seg.sort(key=lambda x: x[1]); half, acc = sum(d for d, _ in seg) / 2, 0
        for d, v in seg:
            acc += d
            if acc >= half:
                typical = v; break
    name, dot, quarters = beat_unit(ms[order[0]].time if ms[order[0]].time else ms[0].time)
    if meta.get('bpm'):
        base = meta['bpm'] * (1.5 if meta.get('beat_dot') else 1) * {'half': 2, 'eighth': 0.5, '16th': 0.25}.get(meta.get('beat_unit', 'quarter'), 1)
        tempo_lines.append(f"  kezdo tempo: {meta['bpm']} (a kottabol)")
    else:
        q = tempo_at(order[0])
        if q is None:
            return
        shown = max(1, round(q / quarters))
        meta['bpm'], meta['beat_unit'], meta['beat_dot'], meta['bpm_midi'] = shown, name, dot, True
        base = shown * quarters
        unit = {'half': 'fel', 'quarter': 'negyed', 'eighth': 'nyolcad', '16th': 'tizenhatod'}[name]
        tempo_lines.append(f"  kezdo tempo: {'pontozott ' if dot else ''}{unit} = {shown} (a MIDI-bol, az elso hat utem kozeperteke: negyed = {q:.1f}"
                           + (f"; a darab jellemzo tempoja: negyed = {typical:.0f}" if typical else '') + ")")
    first, cur = base, base
    for i, m in enumerate(ms):
        for j, d in sorted(enumerate(m.directions), key=lambda x: x[1][0]):
            if d[2] != 'words' or len(d) < 7:
                continue
            text, snd, style = d[3].strip(), None, (d[7] if len(d) > 7 else None)
            if re.match(r'(?i)a tempo\b', text):
                snd, why = cur, 'az elozo tempo'
            elif re.match(r'(?i)tempo\s+(I\b|1\b|primo)', text):
                snd, why = first, 'a kezdo tempo'; cur = first
            elif is_tempo_mark(text):
                q = tempo_at(i)
                if q is None:
                    continue
                snd, why = float(round(q)), 'a MIDI-bol'; cur = snd; style = 'bold'
            if snd is not None:
                m.directions[j] = tuple(d[:7]) + (style, snd)
                tempo_lines.append(f"  {i + 1}. utem, \"{text}\": negyed = {snd:g} ({why})")


def main():
    # on Windows, with redirected output or when started from another program, the code page lacks some accented letters (o, u):
    # do not stop when printing
    for st in (sys.stdout, sys.stderr):
        try:
            st.reconfigure(errors='replace')
        except Exception:
            pass
    ap = argparse.ArgumentParser(description=USAGE, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('pdf', nargs='+')
    ap.add_argument('-o', '--out', help='kimeneti mappa (alapertelmezes: a PDF mellett)')
    ap.add_argument('--title'); ap.add_argument('--composer'); ap.add_argument('--tempo')
    ap.add_argument('--bpm', type=int)
    ap.add_argument('--tempo-x', type=int, help='a kezdo tempojelzes default-x erteke tizedvonalkozben (alapertelmezes: a kottasor bal szele, '
                         'a PDF-bol szamolva)')
    ap.add_argument('--midi-out', help='ide irja a MIDI-ket (az elejukrol a kottahoz kepest plusz teljes utemeket levagva), eredeti nevvel')
    ap.add_argument('--title-from-pdf', action='store_true', help='a cimet a PDF-bol olvassa (OCR), ne a fajlnevbol')
    ap.add_argument('--json', help='a Yamaha kottaszinkron .json fajlok mappaja (alapertelmezes: a PDF mellett)')
    ap.add_argument('--no-json', action='store_true', help='a json-t ne hasznalja')
    ap.add_argument('--midi', help='MIDI fajl vagy mappa az ellenorzeshez (nev szerint parositja)')
    args = ap.parse_args()
    files = [f for p in args.pdf for f in (glob.glob(p) or [p])]
    if args.out: os.makedirs(args.out, exist_ok=True)
    for f in files:
        try:
            convert(f, args.out, args)
        except Exception as ex:
            import traceback
            print(f"{os.path.basename(f)}: HIBA - {type(ex).__name__}: {ex}")
            traceback.print_exc()


if __name__ == '__main__':
    main()
