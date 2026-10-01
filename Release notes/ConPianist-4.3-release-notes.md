## ConPianist 4.3 (fork)

Fő újdonságok: lejátszás MIDI-eszközre zongora nélkül is, élő játék több csatornán, indulás a bezáráskori állapotban.

### Új
- **Lejátszás MIDI-eszközre (pl. loopMIDI → Cantabile), zongora nélkül is.** A Kapcsolat beállításaiban új **MIDI Out** és **MIDI In 2** sor, a Lejátszás részben új, harmadik lehetőség: „MIDI-eszközön keresztül”. Ha a zongora nem érhető el, a program automatikusan erre vált, és ha újra elérhető, visszavált. A Keverő ilyenkor szabványos MIDI-vezérlőket (CC7, CC10, CC91) és General MIDI hangszíneket használ.
- **Lejátszott hangok a virtuális billentyűzeten** USB-s és MIDI-eszközös lejátszásnál (a jobb és bal kéz szólama, transzponálva).
- **Átméretezhető virtuális billentyűzet**: a billentyűzet feletti vonal húzásával; dupla kattintás visszaállítja az alapméretet.
- **Indulás a bezáráskori állapotban**: a program megjegyzi a választott lejátszási kimenetet, és bezáráskor a teljes állapotot (hangszínek, Piano Room, hangerőegyensúly, Keverő, Lejátszás panel, dal, pozíció) a `LastState.conmem` fájlba menti, amit induláskor visszaállít.
- **Ugrás a dal elejére és végére**: a tekerőgombokat 1 másodpercig nyomva.

### Módosítva
- **Élő játék több csatornán**: a Keverő „Élő játék ezen a csatornán” menüpontjával több csatorna is kiválasztható (rétegezve). A virtuális billentyűzet és a MIDI In 2 ezeken szól, a beállított transzponálással.
- **A lejátszó váltása megtartja a beállításokat** (Keverő, bal panel). A zongora és a MIDI-eszköz között a hangszínek a Yamaha ↔ General MIDI megfelelőjükre váltanak.
- A **„Kapcsolat újraindítása”** menüpont a MIDI-eszközt is újranyitja, és újra ellenőrzi a hálózat elérhetőségét.
- **Alapértelmezett mappa** a dalokhoz, kottákhoz és `.conmem` fájlokhoz: `%APPDATA%\ConPianist\Demo Midi Songs`.
- A virtuális billentyűzet láthatósága már nem része a `.conmem` fájlnak.

### Javítva
- Az USB-kábel kihúzása és visszadugása után a kapcsolat most magától helyreáll.
- Hálózati lejátszásnál a dal végén a Keverő és a bal panel beállításai már nem állnak vissza a dal saját beállításaira.

### Telepítés
Csomagold ki a ZIP-et egy tetszőleges mappába, és indítsd el a `ConPianist.exe`-t. A mellékelt `Demo Midi Songs` mappa tartalmát érdemes a `%APPDATA%\ConPianist\Demo Midi Songs` mappába másolni.

A részletes változáslista a [CHANGELOG.md](CHANGELOG.md) fájlban található.

---

<details>
<summary><b>English</b></summary>

## ConPianist 4.3 (fork)

Highlights: playback on a MIDI device even without the piano, Live Play on several channels, starting in the state the program was closed in.

### New
- **Playback on a MIDI device (e.g. loopMIDI → Cantabile), even without the piano.** New **MIDI Out** and **MIDI In 2** settings in the Connection Settings, and a third option in the Playback section: "Via MIDI Device". If the piano is not available, the program switches to it automatically, and switches back when the piano is available again. The Mixer then uses standard MIDI controllers (CC7, CC10, CC91) and General MIDI voices.
- **Played notes on the virtual keyboard** with USB and MIDI device playback (right- and left-hand parts, transposed).
- **Resizable virtual keyboard**: drag the line above the keyboard; double-click restores the default size.
- **Start in the state the program was closed in**: the chosen playback output is remembered, and on exit the whole state (voices, Piano Room, balance, Mixer, Playback panel, song, position) is saved to `LastState.conmem` and restored at the next start.
- **Jump to the beginning / end of the song**: hold the rewind / forward button for 1 second.

### Changed
- **Live Play on several channels**: several channels can be selected (layered) with "Live Play on This Channel" in the Mixer. The virtual keyboard and MIDI In 2 play on them, with the transposition applied.
- **Switching the player keeps the settings** (Mixer, left panel). Between the piano and a MIDI device, voices are mapped to their Yamaha ↔ General MIDI equivalents.
- **Reset Connection** also reopens the MIDI device and checks the network again.
- **Default folder** for songs, scores and `.conmem` files: `%APPDATA%\ConPianist\Demo Midi Songs`.
- The visibility of the virtual keyboard is no longer stored in `.conmem` files.

### Fixed
- After unplugging and plugging in the USB cable, the connection now recovers by itself.
- With network playback, the Mixer and left-panel settings are no longer reset to the song's own settings when the song ends.

### Installation
Unzip the archive to any folder and start `ConPianist.exe`. It is worth copying the contents of the included `Demo Midi Songs` folder to `%APPDATA%\ConPianist\Demo Midi Songs`.

The detailed list of changes (in Hungarian) is in [CHANGELOG.md](CHANGELOG.md).

</details>
