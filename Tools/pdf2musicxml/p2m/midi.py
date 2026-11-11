"""Minimal Standard MIDI File reader (no external package)."""
import struct
from dataclasses import dataclass


@dataclass
class MNote:
    start: int      # tick
    end: int
    pitch: int
    chan: int
    track: int
    vel: int


class Midi:
    def __init__(self, path):
        d = open(path, 'rb').read()
        assert d[:4] == b'MThd', 'nem MIDI fajl'
        hlen, self.fmt, ntr, self.ppq = struct.unpack('>IHHH', d[4:14])
        pos = 8 + hlen
        self.notes, self.tempos, self.timesigs, self.keysigs, self.pedal = [], [], [], [], []
        self.track_names, self.texts = {}, []
        for tr in range(ntr):
            while d[pos:pos + 4] != b'MTrk':
                pos += 8 + struct.unpack('>I', d[pos + 4:pos + 8])[0]
            ln = struct.unpack('>I', d[pos + 4:pos + 8])[0]
            self._track(d[pos + 8:pos + 8 + ln], tr)
            pos += 8 + ln
        self.notes.sort(key=lambda n: (n.start, n.pitch))

    def _track(self, d, tr):
        i, t, status, on = 0, 0, 0, {}

        def vlq():
            nonlocal i
            v = 0
            while True:
                b = d[i]; i += 1
                v = (v << 7) | (b & 0x7F)
                if b < 0x80:
                    return v
        while i < len(d):
            t += vlq()
            b = d[i]
            if b == 0xFF:
                typ = d[i + 1]; i += 2; ln = vlq(); data = d[i:i + ln]; i += ln
                if typ == 0x51: self.tempos.append((t, 60000000 / int.from_bytes(data, 'big')))
                elif typ == 0x58: self.timesigs.append((t, data[0], 2 ** data[1]))
                elif typ == 0x59: self.keysigs.append((t, struct.unpack('b', data[:1])[0]))
                elif typ == 0x03: self.track_names[tr] = data.decode('latin-1', 'replace')
                elif typ in (1, 5, 6): self.texts.append((t, typ, data.decode('latin-1', 'replace')))
                continue
            if b in (0xF0, 0xF7):
                i += 1; ln = vlq(); i += ln; continue
            if b & 0x80:
                status = b; i += 1
            hi, ch = status & 0xF0, status & 0x0F
            if hi in (0xC0, 0xD0):
                i += 1; continue
            a, v = d[i], d[i + 1]; i += 2
            if hi == 0x90 and v > 0:
                on.setdefault((ch, a), []).append((t, v))
            elif hi == 0x80 or (hi == 0x90 and v == 0):
                if on.get((ch, a)):
                    s, vel = on[(ch, a)].pop(0)
                    self.notes.append(MNote(s, t, a, ch, tr, vel))
            elif hi == 0xB0 and a == 64:
                self.pedal.append((t, ch, v))
