## ConPianist 5.0.1

Fő újdonság: **hangfelvétel** a Felvétel ablakban – a zongora hangja a MIDI-felvétel mellé, WAV, FLAC, OGG vagy MP3 formátumban. Emellett a zongora dalainak eredeti MIDI-fájljai átalakítás nélkül használhatók, sok javítás a kottanézetben, és JUCE 9.0.5.

### Új – hangfelvétel
- **Hang felvétele:** a Felvétel ablak új kapcsolójával a program a MIDI-felvétel mellett a zongora hangját is felveszi (a CSP-170 USB-n hangkártyaként működik, ASIO-meghajtóval). A Mentés a MIDI-fájl mellé, azonos névvel menti, például „Felvétel 19-45-02.mid” és „Felvétel 19-45-02.flac”. A hang pontosan ott kezdődik, ahol a MIDI-fájl, így egy DAW-ban a kettő egymás alá tehető.
- **Formátum:** WAV, FLAC (veszteségmentes, kb. fele akkora), OGG (500 kb/s) vagy MP3 (320 kb/s; a LAME kódoló kell hozzá, lásd a Telepítést). **Normalizálás:** a leghangosabb pont −1 dBFS-re kerül.
- **Leállítás:** a felvétel a Leállítás gombra azonnal véget ér, a végi csendet a program levágja. **Felvétel folytatása a dal/kíséret végéig:** bekapcsolva a hang addig megy tovább, amíg a dal vagy a kíséret el nem hallgat.
- **Szintjelző** a kapcsoló mellett és a Hangkártya ablakban: zöld, sárga, és piros, ha a jel túl erős.
- **Hangkártya ablak** (Főmenü → ESZKÖZÖK → Hangkártya): itt választható ki a hangeszköz (eszköztípus, eszköz, bemenetek, mintavétel, pufferméret), alatta a szintjelző és a meghajtó által jelzett késés. Felvétel közben a beállítások nem változtathatók.
- Kilépéskor a program megvárja a hangfájl mentését, és rákérdez, ha a hang még nincs elmentve.

### Új – egyéb
- **Kézi módban is az első hangra vár a felvétel**, mint az automatikus módban (beszámolással nem változott). Az automatikus leállítás alapértéke 3 másodperc csend.
- **A zongora dalainak eredeti MIDI-fájljai átalakítás nélkül használhatók:** a saját lejátszó is átugorja a Yamaha-fájlok hang nélküli beállító ütemét és az üres kezdő ütemeket, így a dal, a kotta és a zongora ütemre egyezik.
- **pdf2musicxml segédprogram** (`Tools/pdf2musicxml`): a zongora dalaihoz tartozó kotta-PDF-ekből (vektoros és képként tárolt oldalakból is) MusicXML-kottát készít, és a dal MIDI-fájljával összeveti.
- A Dalválasztóban a Lesson mappa is kibontva jelenik meg.

### Javítva (a fontosabbak)
- Kottanézet: az ujjrend a kötőív alá és közvetlenül a hang mellé kerül, a triola száma kijjebb; megjelenik a szólamok közötti átkötés; vízszintes gerenda, ahol a nyomtatott kottában is az; nagyobb metronómjelzés; dőlt és félkövér szövegek; a kétoldali ismétlőjel két vastag vonallal; a kettős ütemvonal után álló kezdő ismétlőjel megjelenik.
- Kottanézet: a helyzetjelző vonal a helyén jár ütem közepén álló ismétlőjel után, D.C. al Fine után és előkével kezdődő daloknál.
- Felvétel: szüneteltetett dal folytatásakor a felvétel nem kezdődik másodpercekkel korábban, a számláló 0:00-ról indul.
- A teljes méretre nagyítva bezárt főablak a következő indításkor villanás nélkül jelenik meg, és a saját normál méretére áll vissza.

A teljes lista a [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) fájlban van.

### Egyéb
- A program a JUCE 9.0.5-tel készül, ASIO-támogatással.

### Ismert hibák
- Doricóból exportált kottánál a le nem zárt pedálvonalak egymás alá torlódhatnak, a sor elején több kapcsos zárójel jelenhet meg, és a tempófeliratok egymásra csúszhatnak.
- A hang és a MIDI egymáshoz igazítása a meghajtó által jelzett késésen alapul; ha a meghajtó pontatlan értéket jelez, a hang néhány ezredmásodperccel elcsúszhat.

### Telepítés
Csomagold ki a ZIP-fájlt egy tetszőleges mappába, és indítsd el a `ConPianist.exe`-t. A `Resources` mappának és a `.dll` fájloknak az `.exe` mellett kell maradniuk. A hangfelvételhez a zongora USB-meghajtója kell (Yamaha Steinberg USB Driver); a hangeszköz a Főmenü → ESZKÖZÖK → Hangkártya pontban választható ki. Az MP3-hoz a LAME kódoló kell (nincs a ZIP-ben): a `lame.exe` és a mellé tartozó `.dll` (például `libmpg123-0.dll`) a `ConPianist.exe` mellé vagy a `%APPDATA%\ConPianist\Lame` mappába teendő; nélküle az MP3 nem választható. A gyári OTS-hez szükséges stílusfájlok helye nem változott (lásd az 5.0 kiadási jegyzetét).

<details>
<summary>English</summary>

## ConPianist 5.0.1

Highlight: **audio recording** in the Recording window – the sound of the piano next to the MIDI recording, as WAV, FLAC, OGG or MP3. Also: the original MIDI files of the piano's songs can be used without conversion, many fixes in the score view, and JUCE 9.0.5.

### New – audio recording
- **Record audio:** with the new option of the Recording window, the program records the sound of the piano next to the MIDI recording (the CSP-170 works as an audio interface over USB, with the ASIO driver). Save writes it next to the MIDI file with the same name, e.g. "Recording 19-45-02.mid" and "Recording 19-45-02.flac". The audio starts exactly where the MIDI file starts, so the two can be lined up in a DAW.
- **Format:** WAV, FLAC (lossless, about half the size), OGG (500 kb/s) or MP3 (320 kb/s; needs the LAME encoder, see Installation). **Normalize:** the loudest point is raised to −1 dBFS.
- **Stop:** the recording ends at once with the Stop button; the silence at the end is cut off. **Continue recording until the song/accompaniment ends:** when on, the audio goes on until the song or the accompaniment has ended.
- **Level meter** beside the option and in the Audio Interface window: green, yellow, and red when the signal is too high.
- **Audio Interface window** (main menu → TOOLS → Audio Interface): choose the audio device (device type, device, inputs, sample rate, buffer size); below it the level meter and the latency reported by the driver. The settings cannot be changed during a recording.
- On exit the program waits until the audio file has been saved, and asks if the audio has not been saved yet.

### New – other
- **In manual mode the recording also waits for the first note**, as in automatic mode (unchanged with count-in). The default silence of the automatic stop is 3 seconds.
- **The original MIDI files of the piano's songs can be used without conversion:** ConPianist's own player also skips the silent setup measure and the empty leading measures of the Yamaha files, so the song, the score and the piano have the same measures.
- **pdf2musicxml script** (`Tools/pdf2musicxml`): makes MusicXML scores from the score PDFs of the piano's songs (vector and image pages) and checks them against the song's MIDI file.
- The Lesson folder of the Song Selector is shown expanded too.

### Fixed (the main ones)
- Score view: fingerings below the slur and right next to the note, tuplet numbers further out; ties between voices are shown; flat beams where the printed score has them; a larger metronome mark; italic and bold texts; a double-sided repeat sign with two thick lines; a start repeat after a double barline is shown.
- Score view: the position line stays in place after a repeat sign in the middle of a measure, after D.C. al Fine, and in songs starting with a grace note.
- Recording: when a paused song is continued, the recording no longer starts seconds earlier; the counter starts at 0:00.
- A main window closed maximised appears without flicker at the next start and returns to its own normal size.

The full list is in [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) (in Hungarian).

### Other
- The program is built with JUCE 9.0.5, with ASIO support.

### Known issues
- In scores exported from Dorico, pedal lines that are not closed may pile up, several braces may appear at the start of a system, and tempo texts may overlap.
- The audio is lined up with the MIDI using the latency the driver reports; if the driver reports an inaccurate value, the audio may be off by a few milliseconds.

### Installation
Unzip the file to any folder and start `ConPianist.exe`. The `Resources` folder and the `.dll` files must stay next to the `.exe`. Audio recording needs the piano's USB driver (Yamaha Steinberg USB Driver); the audio device is chosen in main menu → TOOLS → Audio Interface. MP3 needs the LAME encoder (not included): put `lame.exe` and its `.dll` (e.g. `libmpg123-0.dll`) next to `ConPianist.exe` or into `%APPDATA%\ConPianist\Lame`; without it MP3 cannot be chosen. The style files for the factory OTS go where they did before (see the 5.0 release notes).

</details>
