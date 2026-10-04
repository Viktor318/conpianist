## ConPianist 4.5

Fő újdonságok: élő játék felvétele MIDI-fájlba külön ablakban (metronóm, beszámolás, kíséret, visszahallgatás, kvantálás); a zongora billentyűzetének külön transzponálása a Piano Roomban; a Doricóból exportált és az ékezetes nevű kották megjelenítése.

### Új
- **Élő játék felvétele.** A főmenü új **Felvétel…** pontja külön ablakot nyit, amely nyitva maradhat a program használata közben. Felveszi a virtuális billentyűzetet, a MIDI In 2-t, a zongora saját billentyűit és – ha szól – a zongora kíséretét (stílus) is. A felvétel a dalok mappájába, önálló MIDI-fájlba menthető, a hangszínekkel és a keverő beállításaival együtt.
- **Automatikus és kézi indítás:** automatikus módban a felvétel az első hangra indul, és a megadott csend után leáll; kézi módban gombbal.
- **Metronóm, csengő, beszámolás:** a tempó, az ütemmutató és a metronóm hangereje az ablakban állítható. Metronómmal vagy dal mellett a felvétel az ütem elejéhez igazodik, így a belőle készített kotta ütemei a helyükön vannak.
- **Visszahallgatás** mentés előtt is, megállítással és folytatással, idő- és ütemkijelzéssel, csúszkával a felvételben.
- **Kvantálás** mentéskor: negyed, nyolcad vagy tizenhatod felbontás, külön választható triolafelismeréssel; a hangok vége is igazítható, a hangok közti apró szünetek kitölthetők. A felvétel maga eredeti marad, a visszahallgatás az éppen beállított változatot játssza.
- Kilépéskor a program figyelmeztet, ha van el nem mentett felvétel.
- **A zongora billentyűzetének külön transzponálása:** a Piano Room ablakban új **Transzponálás** csúszka (−12…+12 félhang), amely csak a zongora saját billentyűit transzponálja, a dalt nem.
- A Piano Room Hangolás és Transzponálás csúszkája dupla kattintásra az alapértékre áll.

### Javítva
- A kotta akkor is betöltődik, ha a fájl nevében vagy a mappa útvonalában ékezetes betű van.
- A Doricóból exportált MusicXML-kotta megjelenik (eddig hibaüzenet, illetve sok kötőívnél hiba jött).

### Tudnivaló
Amíg a Smart Pianist alkalmazás is csatlakozik a zongorához, a zongora nem küldi USB-n a saját billentyűinek hangjait, így azok ilyenkor nem kerülnek a felvételbe.

### Telepítés
Csomagold ki a ZIP-fájlt egy tetszőleges mappába, és indítsd el a `ConPianist.exe`-t. A `Resources` mappának és a `.dll` fájloknak az `.exe` mellett kell maradniuk. A részletes változáslista a [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) fájlban van.

<details>
<summary>English</summary>

## ConPianist 4.5

Highlights: recording of Live Play into a MIDI file in a separate window (metronome, count-in, accompaniment, listening back, quantization); a separate transpose setting for the piano's keyboard in Piano Room; scores exported from Dorico and scores with accented file names are displayed.

### New
- **Recording of Live Play.** The new **Recording…** item of the main menu opens a separate window that can stay open while the program is used. It records the virtual keyboard, MIDI In 2, the piano's own keys and, when it plays, the piano's accompaniment (style). The recording is saved into the songs folder as a standalone MIDI file, with its voices and mixer settings.
- **Automatic and manual start:** in automatic mode the recording starts with the first note and stops after the given silence; in manual mode with the buttons.
- **Metronome, bell, count-in:** the tempo, the time signature and the metronome volume are set in the window. With the metronome or with a song the recording is aligned to the measures, so the measures of a score made from it are in place.
- **Listening back** before saving too, with stop and continue, time and measure display, and a slider to move in the recording.
- **Quantization** when saving: quarter, eighth or sixteenth note resolution, with separately chosen triplet recognition; the ends of the notes can be aligned too, and short gaps between notes can be filled. The recording itself is not changed; listening back plays the version as it is set.
- The program warns on exit if there is a recording that has not been saved.
- **Separate transpose for the piano's keyboard:** a new **Transpose** slider in the Piano Room window (−12…+12 semitones) that transposes only the piano's own keys, not the song.
- The Master Tune and Transpose sliders of Piano Room return to their default on double-click.

### Fixed
- A score is loaded even if its file name or folder path contains accented letters.
- MusicXML scores exported from Dorico are displayed (they gave an error message, or failed with many slurs).

### Note
While the Smart Pianist app is connected to the piano too, the piano does not send the notes of its own keys over USB, so they are not recorded then.

### Installation
Unzip the file to any folder and start `ConPianist.exe`. The `Resources` folder and the `.dll` files must stay next to the `.exe`.

</details>
