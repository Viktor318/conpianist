#!/usr/bin/env python3
"""midi_trim - removes the empty (note-less) whole measures at the start of a MIDI file.

Every event is kept (SysEx, controllers, program changes, tempo, meta and XF data): the setup messages of the
empty measure(s) move to time 0 in their order, every other event moves earlier by the length of the removed
measure(s). Non-MTrk chunks are copied unchanged. The Hungarian usage text shown by --help is in USAGE.
"""

USAGE = """midi_trim - a MIDI fajl elejen levo ures (hang nelkuli) teljes utemek eltavolitasa.

Minden esemeny megmarad (SysEx, vezerlok, hangszervaltas, tempo, meta- es XF-adatok): az ures
utem(ek)ben levo beallito uzenetek sorrendtartoan a 0. idopontra kerulnek, minden mas esemeny
az eltavolitott utem(ek) hosszaval elorebb jon. A nem MTrk blokkok valtozatlanul masolodnak.

Hasznalat:
    python midi_trim.py dal.mid                  -> dal_vagott.mid (automatikus: az osszes ures kezdo utem)
    python midi_trim.py *.mid -o kimeneti_mappa  -> azonos nevvel a mappaba (csak az erintett fajlok)
    python midi_trim.py dal.mid --bars 1         -> pontosan 1 utem
    python midi_trim.py *.mid --dry-run          -> csak kiirja, mit tenne
"""
import argparse
import glob
import os
import struct
import sys


def read_vlq(d, i):
    v = 0
    while True:
        b = d[i]; i += 1
        v = (v << 7) | (b & 0x7F)
        if b < 0x80:
            return v, i


def write_vlq(v):
    out = [v & 0x7F]
    v >>= 7
    while v:
        out.append((v & 0x7F) | 0x80); v >>= 7
    return bytes(reversed(out))


def parse_track(d):
    """-> [(abs_tick, raw event bytes with full status, kind)]"""
    ev, i, t, status = [], 0, 0, 0
    while i < len(d):
        dt, i = read_vlq(d, i); t += dt
        b = d[i]
        if b == 0xFF:
            j = i + 2; ln, j = read_vlq(d, j)
            ev.append((t, d[i:j + ln], 'meta%02X' % d[i + 1])); i = j + ln
        elif b in (0xF0, 0xF7):
            j = i + 1; ln, j = read_vlq(d, j)
            ev.append((t, d[i:j + ln], 'sysex')); i = j + ln
        else:
            if b & 0x80:
                status = b; i += 1
            n = 1 if status & 0xF0 in (0xC0, 0xD0) else 2
            data = d[i:i + n]; i += n
            kind = 'on' if status & 0xF0 == 0x90 and data[1] > 0 else 'chan'
            ev.append((t, bytes([status]) + data, kind))
    return ev


def build_track(ev):
    out, last = bytearray(), 0
    for t, raw, _ in ev:
        out += write_vlq(t - last) + raw; last = t
    return bytes(out)


def trim(path, out, bars=None, dry=False, cut_ticks=None):
    d = open(path, 'rb').read()
    if d[:4] != b'MThd':
        return f"{os.path.basename(path)}: nem MIDI fajl"
    hlen, fmt, ntr, div = struct.unpack('>IHHH', d[4:14])
    if div & 0x8000:
        return f"{os.path.basename(path)}: SMPTE idoalap, kihagyva"
    chunks, pos = [], 8 + hlen
    while pos + 8 <= len(d):
        cid = d[pos:pos + 4]; ln = struct.unpack('>I', d[pos + 4:pos + 8])[0]
        chunks.append((cid, d[pos + 8:pos + 8 + ln])); pos += 8 + ln
    tail = d[pos:]
    tracks = [parse_track(c) if cid == b'MTrk' else None for cid, c in chunks]
    allev = [e for tr in tracks if tr for e in tr]
    ons = [t for t, raw, k in allev if k == 'on']
    if not ons:
        return f"{os.path.basename(path)}: nincs benne hang"
    ts = sorted((t, raw) for t, raw, k in allev if k == 'meta58')
    if cut_ticks is not None:
        name = os.path.basename(path)
        if cut_ticks <= 0:
            return f"{name}: nincs mit levagni"
        if any(t < cut_ticks for t in ons):
            return f"{name}: hang szol a levagando reszben, kihagyva"
        # of the time signatures in the cut part only the last one (valid at the cut) is kept
        moved_ts = [(t, id(raw)) for tr in tracks if tr for t, raw, k in tr if k == 'meta58' and t <= cut_ticks]
        keep = max(moved_ts)[1] if moved_ts else None
        dropped = len(moved_ts) - (1 if moved_ts else 0)
        if dry:
            return f"[proba] {name}: {cut_ticks} tick levagva"
        res = bytearray(d[:8 + hlen])
        for (cid, c), tr in zip(chunks, tracks):
            if tr is not None:
                c = build_track([(max(0, t - cut_ticks), raw, k) for t, raw, k in tr
                                 if not (k == 'meta58' and t <= cut_ticks and id(raw) != keep)])
            res += cid + struct.pack('>I', len(c)) + c
        res += tail
        os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
        open(out, 'wb').write(res)
        return f"{name}: {cut_ticks} tick levagva" + (f", {dropped} felulirt utemmutato elhagyva" if dropped else '')
    num, den = (ts[0][1][3], 2 ** ts[0][1][4]) if ts and ts[0][0] == 0 else (4, 4)
    bar = div * 4 * num // den
    first = min(ons)
    empty = first // bar
    # notes played by hand may fall a few ticks before the barline: if the first note sounds just before a barline, that measure still counts as empty
    if (first % bar) > bar - div // 8:
        empty += 1
    if any(t < empty * bar for t, raw in ts[1:]):
        return f"{os.path.basename(path)}: utemmutato-valtas az ures reszben, kihagyva"
    n = empty if bars is None else bars
    if bars is not None and any(t < bars * bar for t, raw, k in allev if k == 'on'):
        return f"{os.path.basename(path)}: hang szol a levagando reszben, kihagyva"
    name = os.path.basename(path)
    if n <= 0:
        return f"{name}: nincs ures kezdo utem ({num}/{den}, elso hang: {first} tick)"
    cut = n * bar
    moved = sum(1 for t, raw, k in allev if t < cut)
    msg = f"{name}: {n} ures utem ({num}/{den}, {cut} tick) eltavolitva; elso hang {first} -> {max(0, first - cut)} tick; a 0. idopontra tolt beallito esemenyek: {moved}"
    if dry:
        return "[proba] " + msg
    res = bytearray(d[:8 + hlen])
    for (cid, c), tr in zip(chunks, tracks):
        if tr is not None:
            c = build_track([(max(0, t - cut), raw, k) for t, raw, k in tr])
        res += cid + struct.pack('>I', len(c)) + c
    res += tail
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    open(out, 'wb').write(res)
    return msg


def main():
    # on Windows, with redirected output or when started from another program, the code page lacks some accented letters (o, u):
    # do not stop when printing
    for st in (sys.stdout, sys.stderr):
        try:
            st.reconfigure(errors='replace')
        except Exception:
            pass
    ap = argparse.ArgumentParser(description=USAGE, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('midi', nargs='+')
    ap.add_argument('-o', '--out', help='kimeneti mappa (azonos fajlnevvel); nelkule a fajl melle _vagott nevvel')
    ap.add_argument('--bars', type=int, help='ennyi utemet vagjon le (alapertelmezes: az osszes ures kezdo utemet)')
    ap.add_argument('--dry-run', action='store_true')
    a = ap.parse_args()
    for p in [f for x in a.midi for f in (glob.glob(x) or [x])]:
        out = os.path.join(a.out, os.path.basename(p)) if a.out else os.path.splitext(p)[0] + '_vagott.mid'
        print(trim(p, out, a.bars, a.dry_run))


if __name__ == '__main__':
    main()
