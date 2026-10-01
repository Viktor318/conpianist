## ConPianist 4.4

Fő újdonságok: élő játék a zongora saját hangján vagy a Keverő csatornáin (a dalban nem használt csatornákon is, csatornánkénti oktávval), hangszín átvétele a Hangszín fülről, rövidebb csatornamenü Alaphelyzet ponttal, stabilitási javítások.

### Új
- **Élő játék a zongora saját hangján.** A bal panel alján új **Élő játék** rész: „Zongora (Hangszín fül)” vagy „Keverő csatornái”. Az első esetben a virtuális billentyűzet és a MIDI In 2 pontosan úgy szól, mint a zongora saját billentyűzete (Fő, Réteg, Bal kéz az osztásponttal, Piano Room). Ehhez a zongorának USB-n kell csatlakoznia.
- **Élő játék a dalban nem használt Keverő-csatornákon.** A szürke csatornák is bekapcsolhatók élő játékra (ikonra kattintva vagy a menüből). Fehér keretet kapnak, saját hangszínnel, hangerővel, pannal és zengetéssel; a beállításaikat a program kikapcsolás után is megjegyzi, és a `.conmem`-be is menti.
- **Élő játék oktávja csatornánként** (−2…+2), a csatornamenüből; az érték a zongora ikon mellett látszik.
- **Hangszín átvétele a Hangszín fülről:** a Fő, a Réteg vagy a Bal kéz hangszíne, hangereje, panja, zengetése és oktávja egy kattintással átvehető egy Keverő-csatornára (MIDI-eszköz módban a legközelebbi General MIDI hangszínnel).
- **Alaphelyzet csatornánként** a csatornamenüben (megerősítő kérdéssel): dalcsatornán a dal saját értékei, élő játék csatornán az alapértékek állnak vissza.
- Rövidebb csatornamenü: a Szólam, az Élő játék oktávja, a Hangszín átvétele és a Hangszín átadása oldalra nyíló almenü.
- A 10. (dob) csatornán csak dobkészletek választhatók.
- A zongora ikon minden élő játékra kijelölt csatornán látszik, a virtuális billentyűzet nélkül is.
- A lenyomva tartott kitartó pedál megmarad, ha játék közben változik az élő játék vagy a lejátszás kimenete.
- A bal panel görgethető alacsony ablakban; új **A programról…** menüpont; MIDI-eszköz módban a felső sor a MIDI-eszköz nevét mutatja.

### Javítva
- A hálózati lejátszás magától visszajön, ha a zongorát a program indítása után kapcsolják be.
- Ritka összeomlások ablakok nyitásakor és zárásakor, a kapcsolat újraindításakor és a program bezárásakor (szálkezelés).
- Bezáráskor elhallgatnak az élő játék még szóló hangjai.

### Telepítés
Csomagold ki a ZIP-fájlt egy tetszőleges mappába, és indítsd el a `ConPianist.exe`-t. A `Resources` mappának és a `.dll` fájloknak az `.exe` mellett kell maradniuk. A részletes változáslista a [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) fájlban van.

<details>
<summary>English</summary>

## ConPianist 4.4

Highlights: Live Play on the piano's own sound or on mixer channels (also on channels not used in the song, with a per-channel octave), taking voices from the Voice tab, a shorter channel menu with a Reset item, stability fixes.

### New
- **Live Play on the piano's own sound.** A new **Live Play** section at the bottom of the left panel: "Piano (Voice tab)" or "Mixer channels". With the first one the virtual keyboard and MIDI In 2 sound exactly like the piano's own keys (Main, Layer, Left with the split point, Piano Room). The piano must be connected via USB.
- **Live Play on mixer channels not used in the song.** Grey channels can be switched on for Live Play (click the channel or use the menu). They get a white frame and their own voice, volume, pan and reverb; the settings are remembered after switching the channel off and are saved in `.conmem` files.
- **Per-channel Live Play octave** (−2…+2) in the channel menu; the value is shown beside the keyboard icon.
- **Take Voice from the Voice tab:** the voice, volume, pan, reverb and octave of Main, Layer or Left can be copied to a mixer channel with one click (the nearest General MIDI voice in MIDI device mode).
- **Reset Channel** in the channel menu (with a confirmation): a song channel returns to the song's own values, a Live Play channel to the defaults.
- Shorter channel menu: Part, Live Play Octave, Take Voice from and Use Voice for are submenus.
- Only drum kits can be chosen on channel 10 (drums).
- The keyboard icon is shown on every Live Play channel, also without the virtual keyboard.
- A held sustain pedal is kept when the Live Play or playback output changes while playing.
- The left panel scrolls in a low window; new **About ConPianist…** menu item; in MIDI device mode the status line shows the name of the MIDI device.

### Fixed
- Network playback comes back by itself when the piano is switched on after the program was started.
- Rare crashes when opening and closing windows, resetting the connection and closing the program (threading).
- Live Play notes still sounding are released when the program is closed.

### Installation
Unzip the file to any folder and start `ConPianist.exe`. The `Resources` folder and the `.dll` files must stay next to the `.exe`.

</details>
