"""Writing MusicXML 3.1 (score-partwise) from the recognized measures."""
from fractions import Fraction as F
from math import lcm
from xml.sax.saxutils import escape

STEPS = 'CDEFGAB'
CLEF_BASE = {'G': 30, 'F': 18}          # diatonic number of the bottom line (E4, G2)
SHARP_ORDER = 'FCGDAEB'
TYPES = {F(4): 'whole', F(2): 'half', F(1): 'quarter', F(1, 2): 'eighth', F(1, 4): '16th', F(1, 8): '32nd'}
ALTER = {'sharp': 1, 'flat': -1, 'natural': 0}


def key_alters(fifths):
    if fifths >= 0:
        return {s: 1 for s in SHARP_ORDER[:fifths]}
    return {s: -1 for s in SHARP_ORDER[::-1][:-fifths]}


SEMI = [0, 2, 4, 5, 7, 9, 11]


def assign_pitches(ms):
    """Pitch of every head (clef, key signature, accidentals within the measure, ties), in time order."""
    clef = {0: 'G', 1: 'F'}
    fifths, tie_alter = 0, {}
    for m in ms:
        if m.key is not None: fifths = m.key
        for st, c in m.clefs.items(): clef[st] = c
        ka, state = key_alters(fifths), {}
        cd = sorted(d for d in m.directions if d[2] == 'clef')
        evs = sorted([e for _, _, evs in m.voices for e in evs if e.heads], key=lambda e: (e.onset, e.x))
        cur = dict(clef)
        nxt = {}
        for e in evs:
            for d in cd:
                if d[0] < e.x: cur[d[1]] = d[3]
            for h in e.heads:
                dn = CLEF_BASE[cur[h.staff]] + h.pos
                key = (h.staff, dn)
                step = STEPS[dn % 7]
                if h.accidental:
                    state[key] = ALTER[h.accidental]
                if key in state: alter = state[key]
                elif h.tie_stop and key in tie_alter: alter = tie_alter[key]
                else: alter = ka.get(step, 0)
                h.step, h.octave, h.alter = step, dn // 7, alter
                h.ott = getattr(h, 'ott', 0)
                h.midi = 12 * (dn // 7 + 1) + SEMI[dn % 7] + alter + 12 * h.ott
                if h.tie_start: nxt[key] = alter
        tie_alter = nxt
        for d in cd: clef[d[1]] = d[3]


def write(score, path):
    """score: dict(title, composer, tempo_text, bpm, beat_unit, beat_dot, rights, measures=[Measure])"""
    parts = score.get('parts') or [('Piano', score['measures'])]
    dens = [1]
    for m in [m for _, pm in parts for m in pm]:
        for _, _, evs in m.voices:
            for e in evs:
                dens += [e.dur.denominator, e.onset.denominator]
        dens.append(m.length.denominator)
    div = lcm(*dens)
    o = []
    A = o.append
    A('<?xml version="1.0" encoding="UTF-8"?>')
    A('<!DOCTYPE score-partwise PUBLIC "-//Recordare//DTD MusicXML 3.1 Partwise//EN" '
      '"http://www.musicxml.org/dtds/partwise.dtd">')
    A('<score-partwise version="3.1">')
    if score.get('title'):
        A(f' <work><work-title>{escape(score["title"])}</work-title></work>')
    A(' <identification>')
    if score.get('composer'):
        A(f'  <creator type="composer">{escape(score["composer"])}</creator>')
    if score.get('rights'):
        A(f'  <rights>{escape(score["rights"])}</rights>')
    A('  <encoding><software>pdf2musicxml (vektoros PDF kiolvaso)</software></encoding>')
    A(' </identification>')
    A(' <part-list>' + ''.join(f'<score-part id="P{k + 1}"><part-name>{escape(nm)}</part-name></score-part>'
                              for k, (nm, _) in enumerate(parts)) + '</part-list>')
    for pk, (pname, ms) in enumerate(parts):
        A(f' <part id="P{pk + 1}">')
        clef = {0: 'G', 1: 'F'}
        ott = {}
        fifths, time = 0, (4, 4, None)
        for n, m in enumerate(ms, 1):
            impl = ' implicit="yes"' if getattr(m, 'implicit', False) else ''
            A(f'  <measure number="{n}"{impl}>')
            if n > 1 and m.new_page:
                A('   <print new-page="yes"/>')
            elif n > 1 and m.new_system:
                A('   <print new-system="yes"/>')
            vs = getattr(m, 'ending_start', None)
            if m.left_bar == 'repeat-start' or vs:
                A('   <barline location="left">' + ('<bar-style>heavy-light</bar-style>' if m.left_bar else '') +
                  (f'<ending number="{vs}" type="start">{vs}.</ending>' if vs else '') +
                  ('<repeat direction="forward"/>' if m.left_bar else '') + '</barline>')
            attrs = []
            if n == 1:
                attrs.append(f'<divisions>{div}</divisions>')
            if m.key is not None and (n == 1 or m.key != fifths):
                fifths = m.key
                attrs.append(f'<key><fifths>{fifths}</fifths></key>')
            elif n == 1:
                attrs.append('<key><fifths>0</fifths></key>')
            if m.time and (n == 1 or m.time != time):
                time = m.time
                sym = ' symbol="common"' if time[2] else ''
                attrs.append(f'<time{sym}><beats>{time[0]}</beats><beat-type>{time[1]}</beat-type></time>')
            if n == 1:
                attrs.append('<staves>2</staves>')
            for st in (0, 1):
                c = m.clefs.get(st)
                if c and (n == 1 or c != clef[st]):
                    clef[st] = c
                    attrs.append(f'<clef number="{st + 1}"><sign>{c}</sign><line>{2 if c == "G" else 4}</line></clef>')
            if attrs:
                A('   <attributes>' + ''.join(attrs) + '</attributes>')
            if n == 1 and pk == 0 and (score.get('tempo_text') or score.get('bpm')):
                A('   <direction placement="above"><direction-type>')
                if score.get('tempo_text'):
                    dx = f' default-x="{score["tempo_dx"]}"' if score.get('tempo_dx') is not None else ''
                    dy = ' default-y="40"' if score.get('bpm') else ''
                    A(f'    <words font-weight="bold"{dx}{dy}>{escape(score["tempo_text"])}</words>')
                if score.get('bpm'):
                    A('    </direction-type><direction-type>' if score.get('tempo_text') else '')
                    dot = '<beat-unit-dot/>' if score.get('beat_dot') else ''
                    dx = f' default-x="{score["tempo_dx"]}"' if score.get('tempo_dx') is not None else ''
                    A(f'    <metronome parentheses="yes"{dx}><beat-unit>{score.get("beat_unit", "quarter")}</beat-unit>{dot}'
                      f'<per-minute>{score["bpm"]}</per-minute></metronome>')
                A('   </direction-type><staff>1</staff>')
                if score.get('bpm'):
                    q = score['bpm'] * (1.5 if score.get('beat_dot') else 1)
                    A(f'    <sound tempo="{q:g}"/>')
                A('   </direction>')
            # directions (dynamics, text) before the nearest event
            dirs = sorted(m.directions, key=lambda d: (d[0], d[2]))
            alter_state = {}
            ka = key_alters(fifths)
            cur_clef = dict(clef)
            first_voice = True
            for st, vn, evs in m.voices:
                t = F(0)
                pend = [d for d in dirs if d[2] in ('dynamics', 'words', 'wedge', 'pedal', 'coda')] if first_voice else []
                for e in evs:
                    for d in [d for d in dirs if d[2] == 'clef' and d[1] == st and d[0] < e.x and cur_clef[st] != d[3]]:
                        cur_clef[st] = d[3]; clef[st] = d[3]
                        A(f'   <attributes><clef number="{st + 1}"><sign>{d[3]}</sign>'
                          f'<line>{2 if d[3] == "G" else 4}</line></clef></attributes>')
                    if e.onset > t:
                        A(f'   <forward><duration>{int((e.onset - t) * div)}</duration><voice>{vn}</voice>'
                          f'<staff>{st + 1}</staff></forward>')
                        t = e.onset
                    if e.heads:
                        want = e.heads[0].ott
                        if want != ott.get(st, 0):
                            if ott.get(st, 0):
                                A(f'   <direction><direction-type><octave-shift type="stop" size="8" number="{st + 1}"/></direction-type><staff>{st + 1}</staff></direction>')
                            if want:
                                A(f'   <direction placement="{"above" if want > 0 else "below"}"><direction-type><octave-shift type="{"down" if want > 0 else "up"}" size="8" number="{st + 1}"/></direction-type><staff>{st + 1}</staff></direction>')
                            ott[st] = want
                    while pend and pend[0][0] <= e.x + 6:
                        d = pend.pop(0)
                        A(_direction(d))
                    t += e.dur
                    A(_event(e, vn, st, div, cur_clef, ka, alter_state))
                for d in pend:
                    A(_direction(d))
                first_voice = False
                if t > 0:
                    A(f'   <backup><duration>{int(t * div)}</duration></backup>')
            if not m.voices:
                for st in (0, 1):
                    A(f'   <note><rest measure="yes"/><duration>{int(m.length * div)}</duration><voice>{1 if st == 0 else 5}</voice>'
                      f'<staff>{st + 1}</staff></note>')
                    if st == 0: A(f'   <backup><duration>{int(m.length * div)}</duration></backup>')
            if o[-1].startswith('   <backup>'):
                o.pop()
            ve = getattr(m, 'ending_stop', None)
            end = f'<ending number="{ve[0]}" type="{ve[1]}"/>' if ve else ''
            if ve and m.right_bar is None:
                A(f'   <barline location="right">{end}</barline>')
            if m.right_bar == 'final':
                A('   <barline location="right"><bar-style>light-heavy</bar-style>' + end + '</barline>')
            elif m.right_bar == 'double':
                A('   <barline location="right"><bar-style>light-light</bar-style>' + end + '</barline>')
            elif m.right_bar == 'repeat-end':
                A('   <barline location="right"><bar-style>light-heavy</bar-style>' + end + '<repeat direction="backward"/></barline>')
            A('  </measure>')
        A(' </part>')
    A('</score-partwise>')
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(x for x in o if x) + '\n')


def _direction(d):
    if d[2] == 'pedal':
        return (f'   <direction placement="below"><direction-type><pedal type="{d[3]}" sign="yes"/></direction-type><staff>2</staff></direction>')
    if d[2] == 'coda':
        return '   <direction placement="above"><direction-type><coda/></direction-type><staff>1</staff></direction>'
    if d[2] == 'words' and len(d) > 4:
        dy = f' default-y="{d[5]}"' if len(d) > 5 and d[5] is not None else ''
        return (f'   <direction placement="{d[4]}"><direction-type><words font-style="italic"{dy}>{escape(d[3])}</words></direction-type><staff>1</staff></direction>')
    if d[2] == 'wedge':
        return (f'   <direction placement="below"><direction-type><wedge type="{d[3]}"/></direction-type><staff>1</staff></direction>')
    if d[2] == 'dynamics':
        return (f'   <direction placement="below"><direction-type><dynamics><{d[3]}/></dynamics>'
                f'</direction-type><staff>1</staff></direction>')
    return (f'   <direction placement="below"><direction-type><words font-style="italic">{escape(d[3])}</words>'
            f'</direction-type><staff>1</staff></direction>')


def _tup(e):
    return ('<tuplet type="stop"/>' if e.tuplet_stop else '') + ('<tuplet type="start"/>' if e.tuplet_start else '')


def _event(e, vn, st, div, clef, ka, alter_state):
    out = []
    dur = int(e.dur * div)
    base = e.dur / F(3, 2) if e.dots else e.dur
    if e.tuplet: base = base * F(e.tuplet[0], e.tuplet[1])
    tm = f'<time-modification><actual-notes>{e.tuplet[0]}</actual-notes><normal-notes>{e.tuplet[1]}</normal-notes></time-modification>' if e.tuplet else ''
    typ = TYPES.get(base)
    if e.grace: typ = '16th' if e.beams >= 2 else 'eighth'
    if e.rest:
        r = '<rest measure="yes"/>' if e.whole_measure else '<rest/>'
        s = f'   <note>{r}<duration>{dur}</duration><voice>{vn}</voice>'
        if not e.whole_measure and typ:
            s += f'<type>{typ}</type>' + '<dot/>' * e.dots
        s += tm + f'<staff>{st + 1}</staff>'
        nt = _tup(e) + (f'<fermata type="{e.fermata if isinstance(e.fermata, str) else "upright"}"/>' if e.fermata else '')
        return s + (f'<notations>{nt}</notations>' if nt else '') + '</note>'
    heads = sorted(e.heads, key=lambda h: h.g.cy, reverse=True)      # from bottom to top
    for i, h in enumerate(heads):
        step, octv, alter = h.step, h.octave + h.ott, h.alter
        s = '   <note>' + ('<grace slash="yes"/>' if e.grace else '') + ('<chord/>' if i else '')
        s += f'<pitch><step>{step}</step>' + (f'<alter>{alter}</alter>' if alter else '') + f'<octave>{octv}</octave></pitch>'
        s += '' if e.grace else f'<duration>{dur}</duration>'
        if h.tie_stop: s += '<tie type="stop"/>'
        if h.tie_start: s += '<tie type="start"/>'
        s += f'<voice>{vn}</voice>'
        if typ:
            s += f'<type>{typ}</type>' + '<dot/>' * e.dots
        if h.accidental:
            s += f'<accidental>{h.accidental}</accidental>'
        s += tm
        if e.stem:
            s += f'<stem>{e.stem}</stem>'
        s += f'<staff>{h.staff + 1}</staff>'
        if not i:
            for lvl, tag in sorted(e.beam_tags.items()):
                s += f'<beam number="{lvl}">{tag}</beam>'
        nt = ''
        if h.tie_stop: nt += '<tied type="stop"/>'
        if h.tie_start: nt += '<tied type="start"/>'
        if not i:
            nt += _tup(e)
            for num in e.slur_stop: nt += f'<slur type="stop" number="{num}"/>'
            for num in e.slur_start: nt += f'<slur type="start" number="{num}"/>'
        if h.fingering:
            nt += f'<technical><fingering placement="{h.fing_place}">{h.fingering}</fingering></technical>'
        art = ''
        if not i and e.staccato: art += '<staccato/>'
        if not i and e.accent: art += '<accent/>'
        if not i and e.marcato: art += '<strong-accent/>'
        if not i and e.stacciss: art += '<staccatissimo/>'
        if not i and e.tenuto: art += '<tenuto/>'
        if art: nt += f'<articulations>{art}</articulations>'
        if e.arpeggio: nt += '<arpeggiate/>'
        if not i and e.fermata: nt += f'<fermata type="{e.fermata if isinstance(e.fermata, str) else "upright"}"/>'
        if nt:
            s += f'<notations>{nt}</notations>'
        out.append(s + '</note>')
    return '\n'.join(out)
