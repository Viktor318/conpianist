"""Comparison of the recognized score with the score-exact "guide" channels of the MIDI file (Yamaha XF).

Channels 15 (right hand) and 16 (left hand) hold the notes as written in the score, aligned to the grid,
so they can be compared with the measures note by note. The playing order (repeats, voltas,
D.C./D.S.) is computed from the score; where it does not match, it is looked up from the MIDI file.
"""
from fractions import Fraction as F

NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def pname(p):
    return f"{NAMES[p % 12]}{p // 12 - 1}"


def unroll(ms):
    """Playing order from the repeat signs and voltas."""
    vol, cur = {}, None
    for i, m in enumerate(ms):
        if isinstance(m.volta, int): cur = m.volta
        elif m.volta != 'cont': cur = None
        vol[i] = cur
    order, i, start, pas, guard = [], 0, 0, 1, 0
    while i < len(ms) and guard < 10 * len(ms) + 50:
        guard += 1
        m = ms[i]
        if m.left_bar == 'repeat-start' and i != start:
            start, pas = i, 1
        if vol[i] is not None and vol[i] != pas:
            i += 1
            continue
        order.append(i)
        if vol[i] is not None and vol[i] >= 2 and (i + 1 >= len(ms) or vol[i + 1] != vol[i]):
            start, pas = i + 1, 1
        if m.right_bar == 'repeat-end':
            if pas == 1:
                i, pas = start, 2
                continue
            start, pas = i + 1, 1
        i += 1
    return order


def measure_notes(m):
    out = []
    for st, vn, evs in m.voices:
        for e in evs:
            for h in e.heads:
                if not h.tie_stop and not e.grace and getattr(h, 'midi', None) is not None:
                    out.append((e.onset, h.midi, h.staff, h, e))
    return out


def compare(ms, midi, log, chans=None, tol=0, start=None, order=None):
    """Returns: (list of differences, statistics). Corrects an unambiguous octave difference (8va).
    chans: the MIDI channels compared; tol: time tolerance in ticks (for the played channels); start: the start of
    measure 1 of the score in the MIDI file, in quarters (if already known from another part)."""
    if chans is None:
        chans = (14, 15) if any(n.chan in (14, 15) for n in midi.notes) else tuple(sorted({n.chan for n in midi.notes}))
    tolq = F(tol, midi.ppq)
    notes = [(F(round(n.start / 5) * 5, midi.ppq), n.pitch, n.chan, F(round((n.end - n.start) / 5) * 5 + 5, midi.ppq))
             for n in midi.notes if n.chan in chans]
    notes.sort()
    if not notes:
        return [], {'error': 'nincs hang a MIDI-ben'}
    order = list(order) if order else unroll(ms)
    sn = {i: measure_notes(m) for i, m in enumerate(ms)}
    # compensate for the empty measures / pickup at the start of the MIDI file: the first notes must coincide
    ts = F(0)
    for k in order:
        if sn[k]:
            ts += min(o for o, *_ in sn[k]); break
        ts += ms[k].length
    t = notes[0][0] - ts if start is None else start
    t_start = t
    end = notes[-1][0]

    def window(t, L):
        return [n for n in notes if t - tolq <= n[0] < t + L - tolq]

    def snap(k, rel, p):
        if not tolq:
            return rel
        c = [o for o, pp, *_ in sn[k] if pp == p and abs(o - rel) <= tolq]
        return min(c, key=lambda o: abs(o - rel)) if c else rel

    def sim(k, t):
        S = {(o, p) for o, p, *_ in sn[k]}
        M = {(snap(k, q - t, p), p) for q, p, *_ in window(t, ms[k].length)}
        if not S and not M:
            return 1.0
        return len(S & M) / len(S | M)

    played, p, guard, jumps = [], 0, 0, []
    while t <= end and guard < 6 * len(ms) + 100:
        guard += 1
        cand = order[p] if p < len(order) else None
        d = sim(cand, t) if cand is not None else -1.0
        if d < 0.5:
            best = max(((sim(k, t), -abs(k - (cand if cand is not None else len(ms)))) + (k,) for k in range(len(ms)) if sn[k]),
                       default=None)
            if best and best[0] >= max(0.6, d + 0.25):
                k = best[2]
                jumps.append((cand, k))
                idx = [j for j in range(p, len(order)) if order[j] == k] or [j for j in range(len(order)) if order[j] == k]
                if idx:
                    p = idx[0]
                else:
                    order = order[:p] + list(range(k, len(ms))); 
                cand = order[p]
            elif cand is None:
                break
        played.append((cand, t))
        t += ms[cand].length
        p += 1
    t_end = t
    # grace notes: in the MIDI file they sound before/around the main note; their MIDI notes are paired separately
    claimed, grace_n, grace_ok = set(), 0, 0
    main = {}
    for k, t0 in played:
        for o, pch, *_ in sn[k]: main[(t0 + o, pch)] = True
    for k, t0 in played:
        for st, vn, evs in ms[k].voices:
            for e in evs:
                if not e.grace: continue
                for h in e.heads:
                    if getattr(h, 'midi', None) is None: continue
                    grace_n += 1
                    c = [i for i, n in enumerate(notes) if n[1] == h.midi and i not in claimed and
                         t0 + e.onset - F(1, 2) <= n[0] <= t0 + e.onset + F(1, 8) and not (tolq == 0 and (n[0], n[1]) in main)]
                    if c:
                        claimed.add(max(c, key=lambda i: notes[i][0])); grace_ok += 1
    if claimed:
        notes = [n for i, n in enumerate(notes) if i not in claimed]
    # differences + octave correction
    diffs, fixed, matched, total_s, total_m, durdiff, hand = [], 0, 0, 0, 0, [], 0
    shifted = 0
    fixed_at = set(); ott_conflict = set()
    seen = set()
    for k, t in played:
        S = {}
        for o, pch, st, h, e in sn[k]:
            S.setdefault((o, pch), []).append((st, h, e))
        M = {}
        for q, pch, ch, dur in window(t, ms[k].length):
            M.setdefault((snap(k, q - t, pch), pch), []).append((ch, dur))
        only_s = [x for x in S if x not in M]; only_m = [x for x in M if x not in S]
        arp = set()
        for (o, pch) in list(only_s):     # arpeggio: in the MIDI file the notes sound one after the other
            if any(e.arpeggio for _, _, e in S[(o, pch)]):
                hit = [x for x in only_m if x[1] == pch and 0 < x[0] - o <= F(1, 2)]
                if hit:
                    only_s.remove((o, pch)); only_m.remove(hit[0]); arp.add((o, pch))
        # a main note after grace notes: when the grace notes are played on the beat (an appoggiatura as a
        # sixteenth), the main note sounds later than it stands in the score, by a quarter of a beat for each
        # grace note at most; the score is right
        after = {}
        for st_, vn_, evs_ in ms[k].voices:
            g = 0
            for e_ in evs_:
                if e_.grace: g += 1
                else:
                    if g: after[id(e_)] = g
                    g = 0
        for (o, pch) in list(only_s):
            g = max((after.get(id(e), 0) for _, _, e in S[(o, pch)]), default=0)
            if g:
                hit = [x for x in only_m if x[1] == pch and 0 < x[0] - o <= min(F(1, 2), F(g, 4))]
                if hit:
                    only_s.remove((o, pch)); only_m.remove(min(hit)); shifted += 1
        for (o, pch) in list(only_s):
            for sh in (12, -12):
                if (o, pch + sh) in only_m:
                    hs = [h for st, h, e in S[(o, pch)]]
                    if any(getattr(h, 'ott_line', False) for h in hs):
                        # the score has an 8va/8vb line and the MIDI file plays it differently: not corrected, reported separately
                        ott_conflict.add(k + 1)
                    else:
                        for h in hs:
                            h.ott = getattr(h, 'ott', 0) + sh // 12
                        fixed += 1
                        fixed_at.add(k + 1)
                    only_s.remove((o, pch)); only_m.remove((o, pch + sh))
                    break
        total_s += len(S); total_m += len(M); matched += len(set(S) & set(M))
        for key in set(S) & set(M):
            st, h, e = S[key][0]; ch, dur = M[key][0]
            if len(chans) == 2 and (ch == chans[1]) != (st == 1): hand += 1
            if not tolq and not h.tie_start and dur != e.dur and (k, key) not in seen and not e.arpeggio and not e.fermata:
                durdiff.append((k, key[0], key[1], e.dur, dur))
        if (only_s or only_m) and (k, tuple(sorted(only_s)), tuple(sorted(only_m))) not in seen:
            seen.add((k, tuple(sorted(only_s)), tuple(sorted(only_m))))
            diffs.append((k, sorted(only_s), sorted(only_m)))
        seen.update((k, key) for key in set(S) & set(M))
    rest = len([n for n in notes if n[0] >= t_end])
    stats = dict(grace=(grace_ok, grace_n), ott_conflict=sorted(ott_conflict), fixed_at=sorted(fixed_at), start=t_start, chans=chans, played=len(played), order=[k for k, _ in played], matched=matched, score_notes=total_s, midi_notes=total_m,
                 fixed=fixed, durdiff=durdiff, hand=hand, shifted=shifted, leftover=rest, jumps=jumps, unplayed=sorted(set(range(len(ms))) - {k for k, _ in played}))
    return diffs, stats


def report(diffs, stats, ms):
    L = ["", "=== MIDI osszevetes ==="]
    if 'error' in stats:
        return L + ["  " + stats['error']]
    L.append(f"  lejatszott utemek: {stats['played']} (a kottaban {len(ms)} utem), egyezo hangok: {stats['matched']} / "
             f"kotta {stats['score_notes']} / MIDI {stats['midi_notes']}")
    if stats['fixed']:
        L.append(f"  oktav-elteres (8va/8vb) MIDI alapjan javitva: {stats['fixed']} hang"
                 + (f" (utemek: {', '.join(map(str, stats.get('fixed_at', [])))})" if stats.get('fixed_at') else ''))
    if stats.get('grace', (0, 0))[1]:
        L.append(f"  elokek: {stats['grace'][1]} a kottaban, ebbol {stats['grace'][0]} megvan a MIDI-ben")
    if stats.get('ott_conflict'):
        L.append("  a kotta 8va/8vb vonala alatt a MIDI oktavnyi elteressel jatszik (a kotta a PDF szerint marad): utemek "
                 + ', '.join(map(str, stats['ott_conflict'])))
    if stats['jumps']:
        L.append("  a kottabol nem kovetkezo ugrasok (D.C./D.S./coda vagy rosszul olvasott ismetles): " +
                 ', '.join(f"{(a + 1) if a is not None else 'veg'}->{b + 1}" for a, b in stats['jumps']))
    if stats['unplayed']:
        L.append("  a MIDI-ben nem szereplo utemek: " + ', '.join(str(k + 1) for k in stats['unplayed']))
    if stats['leftover']:
        L.append(f"  a MIDI vegen parositatlan hangok: {stats['leftover']}")
    if stats.get('shifted'):
        L.append(f"  eloke utan kesobb megszolalo fohangok (az eloke utesre szol; a kotta jo): {stats['shifted']}")
    if stats['hand']:
        L.append(f"  mas kezben/sorban levo hangok (csak tajekoztatas): {stats['hand']}")
    if not diffs:
        L.append("  hangmagassag es kezdet: nincs elteres")
    for k, s, m in diffs:
        a = ', '.join(f"{pname(p)}@{float(o):g}" for o, p in s)
        b = ', '.join(f"{pname(p)}@{float(o):g}" for o, p in m)
        L.append(f"  ! {k + 1}. utem (o{ms[k].page}/r{ms[k].system}): " + (f"csak a kottaban: {a}" if a else "") +
                 ("; " if a and b else "") + (f"csak a MIDI-ben: {b}" if b else ""))
    if stats['durdiff']:
        L.append(f"  eltero hanghossz: {len(stats['durdiff'])}")
        for k, o, p, ds, dm in stats['durdiff'][:40]:
            L.append(f"  ~ {k + 1}. utem: {pname(p)}@{float(o):g} kotta {float(ds):g}, MIDI {float(dm):g} negyed")
    return L
