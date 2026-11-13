"""Writing MusicXML 3.1 (score-partwise) from the recognized measures."""
import re
from fractions import Fraction as F
from math import lcm
from xml.sax.saxutils import escape

STEPS = 'CDEFGAB'
# width of the dynamic marks in Lomse, in tenths (measured on the rendered score)
DYN_WIDTH = {'p': 20, 'pp': 34, 'ppp': 48, 'mp': 38, 'mf': 38, 'f': 18, 'ff': 30, 'sf': 28}
CLEF_BASE = {'G': 30, 'F': 18}          # diatonic number of the bottom line (E4, G2)
SHARP_ORDER = 'FCGDAEB'
TYPES = {F(4): 'whole', F(2): 'half', F(1): 'quarter', F(1, 2): 'eighth', F(1, 4): '16th', F(1, 8): '32nd'}
ALTER = {'sharp': 1, 'flat': -1, 'natural': 0, 'dsharp': 2, 'dflat': -2}
ACC_NAMES = {'dsharp': 'double-sharp', 'dflat': 'flat-flat'}


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
    shown, hide = score.get('shown_parts', set()), ' print-object="no"'
    A(' <part-list>' + ''.join(f'<score-part id="P{k + 1}"><part-name{"" if nm in shown else hide}>{escape(nm)}</part-name></score-part>'
                              for k, (nm, _) in enumerate(parts)) + '</part-list>')
    # D.C. and Fine of any part: measure number -> instructions. They take effect at the end of the section: at the
    # first repeat sign, double or final barline from their measure on (a long text begins a measure earlier)
    jumps = {}
    for _, ms in parts:
        for n, m in enumerate(ms, 1):
            for d in m.directions:
                snd = _jump_sound(d[3]) if d[2] == 'words' else None
                if snd:
                    k = next((j for j in range(n, min(n + 2, len(ms)) + 1) if ms[j - 1].right_bar in ('repeat-end', 'double', 'final')), n)
                    jumps.setdefault(k, set()).add(snd)
    # A repeat sign with no forward repeat sign before it in its section: the repeat goes back to the start of the
    # section, the measure after the last repeat sign, double or final barline (the trio after the Fine). Written as
    # an implied forward repeat (a playback instruction, nothing is printed).
    implied, open_start, boundary = set(), True, None
    for n, m in enumerate(parts[0][1], 1):
        if m.left_bar == 'repeat-start': open_start = True
        if m.right_bar == 'repeat-end':
            if not open_start and boundary: implied.add(boundary + 1)
            open_start, boundary = False, n
        elif m.right_bar in ('double', 'final'):
            boundary = n
    for pk, (pname, ms) in enumerate(parts):
        A(f' <part id="P{pk + 1}">')
        clef = {0: 'G', 1: 'F'}
        late_clef = {}      # staff -> clef printed after the last note of the previous measure
        ott = {}
        fifths, time = 0, (4, 4, None)
        for n, m in enumerate(ms, 1):
            impl = ' implicit="yes"' if getattr(m, 'implicit', False) else ''
            A(f'  <measure number="{n}"{impl}>')
            if n > 1 and m.new_page:
                A('   <print new-page="yes"/>')
            elif n > 1 and m.new_system:
                A('   <print new-system="yes"/>')
            if pk == 0 and n in implied:
                A('   <sound forward-repeat="yes"/>')
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
                c = m.clefs.get(st) or late_clef.pop(st, None)
                if c and (n == 1 or c != clef[st]):
                    clef[st] = c
                    attrs.append(f'<clef number="{st + 1}"><sign>{c}</sign><line>{2 if c == "G" else 4}</line></clef>')
            if attrs:
                A('   <attributes>' + ''.join(attrs) + '</attributes>')
            if n == 1 and pk == 0 and (score.get('tempo_text') or score.get('bpm')):
                A('   <direction placement="above"><direction-type>')
                if score.get('tempo_text'):
                    dx = f' default-x="{score["tempo_dx"]}"' if score.get('tempo_dx') is not None else ''
                    # above the fingering and the clef; with a metronome mark below it, higher
                    dy = ' default-y="60"' if score.get('bpm') else ' default-y="40"'
                    A(f'    <words font-weight="bold"{dx}{dy}>{escape(score["tempo_text"])}</words>')
                if score.get('bpm'):
                    A('    </direction-type><direction-type>' if score.get('tempo_text') else '')
                    dot = '<beat-unit-dot/>' if score.get('beat_dot') else ''
                    dx = f' default-x="{score["tempo_dx"]}"' if score.get('tempo_dx') is not None else ''
                    A(f'    <metronome parentheses="yes"{dx} default-y="35"><beat-unit>{score.get("beat_unit", "quarter")}</beat-unit>{dot}'
                      f'<per-minute>{score["bpm"]}</per-minute></metronome>')
                A('   </direction-type><staff>1</staff>')
                if score.get('bpm'):
                    q = score['bpm'] * (1.5 if score.get('beat_dot') else 1)
                    A(f'    <sound tempo="{q:g}"/>')
                A('   </direction>')
            # directions (dynamics, text) before the nearest event
            dirs = list(m.directions)
            # words below the staff (between the staves) hang on the nearest note, shifted to their place in the PDF
            ex = [e.x for e in m.voices[0][2]] if m.voices else []
            for k, d in enumerate(dirs):
                if d[2] == 'words' and len(d) > 6:
                    a = min(ex, key=lambda x: abs(x - d[0])) if (d[4] == 'below' or _jump_sound(d[3])) and ex else d[0]
                    rx = round((d[0] - a) / d[6] * 10)
                    # a dynamic mark on the same note: Lomse draws it from the note to the right (in the PDF it is to the
                    # left of the note), so the text starts after it
                    dyn = [q for q in m.directions if q[2] == 'dynamics' and q[0] < d[0] and d[4] == 'below' and
                           next((x for x in ex if q[0] <= x + 6), None) == a]
                    if dyn:
                        rx = max(rx, max(DYN_WIDTH.get(q[3], 30) for q in dyn) + 16)
                    dirs[k] = (a,) + tuple(d[1:6]) + (rx,)
            dirs.sort(key=lambda d: (d[0], d[2]))
            alter_state = {}
            ka = key_alters(fifths)
            cur_clef = dict(clef)
            first_voice = True
            # the time of a horizontal position, from the notes of all the voices (for the ends of the hairpins)
            # (per onset the leftmost note; long rests stand in the middle of their time, so they are left out)
            by_on = {}
            for _, _, evs_ in m.voices:
                for e in evs_:
                    if not e.grace and e.onset is not None and not (e.rest in ('whole', 'half') or e.whole_measure):
                        by_on[e.onset] = min(by_on.get(e.onset, e.x), e.x)
            xt = []
            for on in sorted(by_on):
                if not xt or by_on[on] > xt[-1][0]:
                    xt.append((by_on[on], on))
            xt.append((max(m.x1, xt[-1][0] + 1) if xt else m.x1, m.length))

            def offset_of(d, now):
                if d[2] != 'wedge' or len(xt) < 2:
                    return 0
                x = d[0]
                if x <= xt[0][0]:
                    td = xt[0][1]
                else:
                    td = xt[-1][1]
                    for (xa, ta), (xb, tb) in zip(xt, xt[1:]):
                        if xa <= x <= xb:
                            td = ta + (tb - ta) * F(x - xa).limit_denominator(1000) / F(xb - xa).limit_denominator(1000) if xb > xa else ta
                            break
                # snapped to the nearest note onset: a time between the notes would get a column of its own in
                # Lomse, and the notes around it were drawn on top of each other
                td = min((p[1] for p in xt), key=lambda on: abs(on - td))
                return int(round((td - now) * div))
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
                        A(_direction(d, offset_of(d, t)))
                    t += e.dur
                    A(_event(e, vn, st, div, cur_clef, ka, alter_state))
                for d in pend:
                    A(_direction(d, offset_of(d, t)))
                first_voice = False
                if t > 0:
                    A(f'   <backup><duration>{int(t * div)}</duration></backup>')
            # a clef after the last note of the measure is written at the start of the next measure
            for d in sorted(d for d in dirs if d[2] == 'clef'):
                if d[3] != clef[d[1]]: late_clef[d[1]] = d[3]
                else: late_clef.pop(d[1], None)
            if not m.voices:
                for st in (0, 1):
                    A(f'   <note><rest measure="yes"/><duration>{int(m.length * div)}</duration><voice>{1 if st == 0 else 5}</voice>'
                      f'<staff>{st + 1}</staff></note>')
                    if st == 0: A(f'   <backup><duration>{int(m.length * div)}</duration></backup>')
            if o[-1].startswith('   <backup>'):
                o.pop()
            # D.C. and Fine also as playback instructions (the texts alone are not followed by the players):
            # once, in the first part, at the end of the measure (see jumps)
            if pk == 0:
                for snd in sorted(jumps.get(n, ())):
                    A(f'   <sound {snd}="yes"/>')
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


def _jump_sound(text):
    """The playback instruction of a text: 'dacapo' (D.C., Da Capo, also with words around it, e.g.
    "Menuetto D.C. al Fine"), 'fine' (Fine alone), or None."""
    s = re.sub(r'\s+', ' ', text.lower()).strip()
    if re.search(r'(^|[^a-z])d\. ?c\.?($|[^a-z])', s) or 'da capo' in s:
        return 'dacapo'
    if s.strip(' .') == 'fine':
        return 'fine'
    return None


def _direction(d, offset=0):
    if d[2] == 'pedal':
        return (f'   <direction placement="below"><direction-type><pedal type="{d[3]}" sign="yes"/></direction-type><staff>2</staff></direction>')
    if d[2] == 'coda':
        return '   <direction placement="above"><direction-type><coda/></direction-type><staff>1</staff></direction>'
    if d[2] == 'words' and len(d) > 4:
        dy = f' default-y="{d[5]}"' if len(d) > 5 and d[5] is not None else ''
        if len(d) > 6 and d[6]: dy += f' relative-x="{d[6]}"'
        return (f'   <direction placement="{d[4]}"><direction-type><words font-style="italic"{dy}>{escape(d[3])}</words></direction-type><staff>{d[1] + 1}</staff></direction>')
    if d[2] == 'wedge':
        # the offset (in divisions) tells where the hairpin starts or ends within a long note
        off = f'<offset>{offset}</offset>' if offset else ''
        return (f'   <direction placement="below"><direction-type><wedge type="{d[3]}"/></direction-type>{off}<staff>1</staff></direction>')
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
            s += f'<accidental>{ACC_NAMES.get(h.accidental, h.accidental)}</accidental>'
        s += tm
        if e.stem:
            s += f'<stem>{e.stem}</stem>'
        s += f'<staff>{h.staff + 1}</staff>'
        if not i:
            for lvl, tag in sorted(e.beam_tags.items()):
                s += f'<beam number="{lvl}">{tag}</beam>'
        nt = ''
        if h.tie_stop: nt += '<tied type="stop"/>'
        if h.tie_start:
            ov = getattr(h, 'tie_over', None)
            nt += '<tied type="start"' + ('' if ov is None else ' orientation="over"' if ov else ' orientation="under"') + '/>'
        if not i:
            nt += _tup(e)
            for num in e.slur_stop: nt += f'<slur type="stop" number="{num}"/>'
            for num in e.slur_start:
                pl = f' placement="{e.slur_place[num]}"' if num in e.slur_place else ''
                nt += f'<slur type="start" number="{num}"{pl}/>'
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
