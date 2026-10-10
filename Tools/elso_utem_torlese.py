# -*- coding: utf-8 -*-
"""Removes the setup measure (the first measure without notes) of Yamaha MIDI files.

The MIDI files saved from the songs of the piano begin with a measure without notes, in
which the song is set up (GM/XG reset, voices, volumes, reverb, the XF data); the piano's
own copy of the song starts without it. So that the measures of the MIDI file, of the score
and of the piano's song are the same, this measure is removed:
- the setup data of the measure stays at the beginning of the file, in its order;
- the time signature and the tempo of the setup measure are dropped (those at the beginning
  of the second measure take their place);
- everything else moves one measure earlier.

A file is changed only if its first measure has no notes and the time signature is set
again at the end of it (as Yamaha writes these files); other files, and files already
changed, are left out. The original files are not changed: the new ones are written into
the output folder, with the same subfolders and names.

Usage: see USAGE (Python 3, nothing else is needed).
"""
import os
import struct
import sys

USAGE = """Az üres első ütem (a Yamaha beállító üteme) törlése MIDI-fájlokból.

Használat:
    python elso_utem_torlese.py <bemeneti mappa vagy fájl> <kimeneti mappa>

A bemeneti mappa összes MIDI-fájlját (az almappákkal együtt) megnézi; az átalakított
fájlok a kimeneti mappába kerülnek, ugyanazokkal az almappákkal és nevekkel. Az eredeti
fájlok nem változnak. Csak azokat a fájlokat alakítja át, amelyek első ütemében nincs hang,
és utána új ütemmutató kezdődik (így írja a Yamaha); a többit, és a már átalakítottakat
kihagyja.
"""


# ---------------------------------------------------------------------------------------
# Reading and writing MIDI files; every event keeps its own bytes

def read_vlq(data, pos):
    value = 0
    while True:
        b = data[pos]
        pos += 1
        value = (value << 7) | (b & 0x7F)
        if not b & 0x80:
            return value, pos


def write_vlq(value):
    out = [value & 0x7F]
    value >>= 7
    while value:
        out.append((value & 0x7F) | 0x80)
        value >>= 7
    return bytes(reversed(out))


class Event:
    def __init__(self, tick, kind, status, data, meta_type=None):
        self.tick = tick
        self.kind = kind            # 'midi', 'meta' or 'sysex'
        self.status = status
        self.data = data
        self.meta_type = meta_type

    def is_note(self):
        return self.kind == 'midi' and (self.status & 0xF0) in (0x80, 0x90)

    def is_meta(self, meta_type):
        return self.kind == 'meta' and self.meta_type == meta_type

    def encode(self):
        if self.kind == 'midi':
            return bytes([self.status]) + self.data
        if self.kind == 'meta':
            return bytes([0xFF, self.meta_type]) + write_vlq(len(self.data)) + self.data
        return bytes([self.status]) + write_vlq(len(self.data)) + self.data


def parse_track(data):
    events, pos, tick, running = [], 0, 0, None
    while pos < len(data):
        delta, pos = read_vlq(data, pos)
        tick += delta
        b = data[pos]
        if b == 0xFF:
            meta_type = data[pos + 1]
            length, pos = read_vlq(data, pos + 2)
            events.append(Event(tick, 'meta', 0xFF, data[pos:pos + length], meta_type))
            pos += length
        elif b in (0xF0, 0xF7):
            length, pos = read_vlq(data, pos + 1)
            events.append(Event(tick, 'sysex', b, data[pos:pos + length]))
            pos += length
        else:
            if b & 0x80:
                running = b
                pos += 1
            if running is None:
                raise ValueError("sérült sáv (adat státuszbájt nélkül)")
            n = 1 if (running & 0xF0) in (0xC0, 0xD0) else 2
            events.append(Event(tick, 'midi', running, data[pos:pos + n]))
            pos += n
    return events


def read_midi(path):
    with open(path, 'rb') as f:
        data = f.read()
    if data[:4] != b'MThd':
        raise ValueError("nem MIDI-fájl")
    header_length = struct.unpack('>I', data[4:8])[0]
    header = data[8:8 + header_length]
    fmt, _, division = struct.unpack('>HHH', header[:6])
    if fmt not in (0, 1) or division & 0x8000:
        raise ValueError("nem támogatott MIDI-fájl (%d. formátum)" % fmt)
    chunks = []  # ('MTrk', events) or (chunk id, bytes), in the order of the file
    pos = 8 + header_length
    while pos + 8 <= len(data):
        chunk_id = data[pos:pos + 4]
        length = struct.unpack('>I', data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + length]
        if len(body) < length:
            raise ValueError("sérült fájl (váratlanul véget ér)")
        chunks.append(('MTrk', parse_track(body)) if chunk_id == b'MTrk' else (chunk_id, body))
        pos += 8 + length
    return header, division, chunks


def write_midi(path, header, chunks):
    out = bytearray(b'MThd' + struct.pack('>I', len(header)) + header)
    for chunk_id, content in chunks:
        if chunk_id == 'MTrk':
            body, last = bytearray(), 0
            for event in content:
                body += write_vlq(event.tick - last) + event.encode()
                last = event.tick
            out += b'MTrk' + struct.pack('>I', len(body)) + body
        else:
            out += chunk_id + struct.pack('>I', len(content)) + content
    with open(path, 'wb') as f:
        f.write(out)


# ---------------------------------------------------------------------------------------
# The conversion

def first_measure_length(tracks, division):
    numerator, denominator = 4, 4
    for events in tracks:
        for event in events:
            if event.tick > 0:
                break
            if event.is_meta(0x58) and len(event.data) >= 2:
                numerator, denominator = event.data[0], 2 ** event.data[1]
    return division * 4 * numerator // denominator


def convert(source, target):
    """Returns None if the file was converted, otherwise the reason why it was left out."""
    header, division, chunks = read_midi(source)
    tracks = [content for chunk_id, content in chunks if chunk_id == 'MTrk']
    measure = first_measure_length(tracks, division)
    if not any(e.is_note() for events in tracks for e in events):
        return "nincs benne hang"
    if any(e.is_note() and e.tick < measure for events in tracks for e in events):
        return "az első ütemben van hang (már átalakított, vagy nem Yamaha-fájl)"
    if not any(e.is_meta(0x58) and e.tick == measure for events in tracks for e in events):
        return "az első ütem után nincs új ütemmutató (nem a Yamaha beállító üteme)"

    new_chunks = []
    for chunk_id, content in chunks:
        if chunk_id != 'MTrk':
            new_chunks.append((chunk_id, content))
            continue
        setup, rest = [], []
        for event in content:
            if event.tick < measure:
                if event.is_meta(0x51) or event.is_meta(0x58):
                    continue  # the tempo and the time signature of the setup measure
                setup.append(Event(0, event.kind, event.status, event.data, event.meta_type))
            else:
                rest.append(Event(event.tick - measure, event.kind, event.status, event.data, event.meta_type))
        new_chunks.append(('MTrk', setup + rest))

    os.makedirs(os.path.dirname(target) or '.', exist_ok=True)
    write_midi(target, header, new_chunks)

    # check: the new file is read again; its events must be those written, and its notes
    # those of the old file, one measure earlier
    _, _, check = read_midi(target)
    written = [[e.encode() for e in c] for i, c in new_chunks if i == 'MTrk']
    read_back = [[e.encode() for e in c] for i, c in check if i == 'MTrk']
    old_notes = sorted((e.tick - measure, e.encode()) for events in tracks for e in events if e.is_note())
    new_notes = sorted((e.tick, e.encode()) for i, c in check if i == 'MTrk' for e in c if e.is_note())
    if written != read_back or old_notes != new_notes:
        os.remove(target)
        raise ValueError("az ellenőrzés hibát talált, az új fájl nem készült el")
    return None


def midi_files(folder, skip):
    for root, dirs, files in os.walk(folder):
        if skip and os.path.abspath(root).startswith(skip):
            continue
        for name in sorted(files):
            if name.lower().endswith(('.mid', '.midi')):
                yield os.path.join(root, name)


def main():
    if len(sys.argv) != 3:
        print(USAGE)
        return 2
    source, output = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    if os.path.isfile(source):
        files, base = [source], os.path.dirname(source)
    elif os.path.isdir(source):
        if output == source:
            print("A kimeneti mappa nem lehet ugyanaz, mint a bemeneti.")
            return 2
        files, base = list(midi_files(source, output + os.sep)), source
    else:
        print("Nem található: " + source)
        return 2

    converted, skipped, failed = [], [], []
    for path in files:
        relative = os.path.relpath(path, base)
        target = os.path.join(output, relative)
        try:
            reason = convert(path, target)
        except (IndexError, struct.error):
            failed.append((relative, "sérült fájl (váratlanul véget ér)"))
            continue
        except Exception as error:  # a damaged file does not stop the others
            failed.append((relative, str(error)))
            continue
        (skipped.append((relative, reason)) if reason else converted.append(relative))

    for relative in converted:
        print("ÁTALAKÍTVA:  " + relative)
    for relative, reason in skipped:
        print("KIHAGYVA:    %s (%s)" % (relative, reason))
    for relative, reason in failed:
        print("HIBA:        %s (%s)" % (relative, reason))
    print("\nÖsszesen %d fájl: %d átalakítva, %d kihagyva, %d hibás. Az új fájlok helye: %s"
          % (len(files), len(converted), len(skipped), len(failed), output))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
