## ConPianist 5.0

Fő újdonságok: OTS (One Touch Setting) a stílusokhoz – stílusonként 8 saját beállítás és a Yamaha hangszerek stílusfájljaiból beolvasott 4 gyári OTS; új felvételi módok a Felvétel ablakban (kíséret külön fájlba, felvétel a dal fölé, rétegenként); szintjelző a Keverőben; nyitva tartható Piano Room; az A–B jelek egérrel mozgathatók a kottában; sok hibajavítás és JUCE 9.0.4.

### Új – OTS (One Touch Setting)
- **Saját OTS:** stílusonként 8 beállítás a Fő, a Réteg és a Bal szólamhoz (hangszín, be/ki, hangerő, oktáv, tér, zengetés), külön, nyitva tartható **OTS ablakban** (felső sáv ikonja, a Kíséret ablak OTS gombja vagy az O billentyű). Behívás kattintással vagy Shift+F1–F8-cal; Tárolás, Törlés, Mentés és Elvetés. A módosított OTS piros keretet kap. A stílusok listájában citromsárga ikon jelzi, melyik stílusnak van saját OTS-e.
- **Gyári OTS:** a CVP-809, CVP-709, Genos, PSR-SX900, PSR-SX700, PSR-SX600, PSR-S970 és PSR-S950 stílusfájljaiból a program beolvassa a CSP-170 azonos nevű stílusainak 4 gyári OTS-ét, és a hangszíneket a CSP-170 hangszíneire fordítja (mind a 470 stílus megkapja). Főmenü → ESZKÖZÖK → **Gyári OTS beolvasása**; a stílusfájlok helye lent, a Telepítésnél.
- Az OTS ablak Gyári OTS része: 4 gomb (Ctrl+F1–F4), **OTS Link** (a Main A–D a gyári OTS 1–4-et hívja be, stílusváltáskor is), Másolás a saját gombokra. A gyári OTS is módosítható, menthető vagy elvethető.
- OTS-váltáskor a még szóló hangok elhallgatnak, így a hangszínváltás nem ad furcsa hangot; a kíséret tovább szól.

### Új – Felvétel ablak
- **Három felvételi mód:** Játék kíséret nélkül, Játék kísérettel és Játék dal/lejátszás fölé (USB-n és MIDI-eszközön). Kísérettel a játék és a kíséret **külön fájlba is** menthető, a teljes felvétel mellé.
- **Felvétel a dal fölé, rétegenként:** a saját lejátszó dala is bekerül a felvételbe, a Felvétel gomb nem állítja le a lejátszást, így újabb réteg vehető fel. A **Felvétel betöltése** egy korábbi felvételt tölt a lejátszóba.
- **Fájlnév, dátum szerinti almappák, időbélyeg:** a név szerkeszthető, a program megjegyzi; foglalt névnél sorszámot kap, semmit nem ír felül. Mentés után ugyanaz a felvétel más beállítással (például kvantálva) is elmenthető.
- Új elrendezés: egy Felvétel/Leállítás gomb, Mentés, Elvetés, Lejátszás; a felvétel hossza ütemben látszik.

### Új – Keverő, Piano Room, Kíséret ablak
- **Szintjelző a Keverő csatornáin:** kék sáv mutatja a megszólaló hangokat, szürke, ha a csatorna el van némítva (saját lejátszó és Élő játék).
- **A Piano Room nyitva maradhat** játék közben; a felső sáv új hangvilla ikonja nyitja meg.
- **Intro és Ending billentyűvel** (Shift+1–3, Ctrl+1–3). A Kíséret ablak gyorsbillentyűi a program **minden ablakában** működnek.
- Regisztrációs memória gyorsan: Shift+kattintás a memóriagombon ment, Ctrl+Shift+kattintás törli.

### Új – lejátszás és kotta
- **Lejátszás közben betöltött dal magától indul**, ahogy a zongora saját dalainál. A Dalválasztó dupla kattintásra bezárul.
- **Ha a dalhoz kotta tartozik, a dal a kotta utolsó ütemével ér véget**, akkor is, ha a MIDI-fájl tovább tart.
- **Az A–B jelek egérrel mozgathatók a kottában**; Ctrl+kattintás: A, Ctrl+Shift+kattintás: B.
- Szebb kotta: a tempójelzés, a crescendo és diminuendo villája, a duola/triola jele, két szólamnál a szünetek és az alsó szólam ujjrendje a helyére kerül.

### Javítva (a fontosabbak)
- Lefagyás, ha a jobb és a bal kéz ugyanarra a csatornára van állítva; ritka összeomlás újracsatlakozáskor; összeomlás A–B ismétlést tartalmazó memória betöltésekor dal nélkül.
- Fölösleges újracsatlakozás lassan betöltődő hangszín után.
- Zongoradal betöltése után a kíséret egyes részei hangosabbak voltak.
- A Kíséret és a Felvétel ablak tempója nem állítja át a betöltött, de nem szóló dal tempóját.
- MIDI-eszközös lejátszásnál a billentyűzet részei és a kíséret beállításai is a zongorához jutnak, és a Hangszín fül is használható.
- Két memória gyors egymás utáni előhívásakor a második érvényes.
- Hálózati lejátszásnál a dal végén nem lassul le a tempó; kilépéskor a zongora leállítja a dalt, és az ablak azonnal eltűnik.
- A felvétel mentése nem törli a régi fájlt, ha az írás nem sikerül; a felvétel előtt beállított tempó bekerül a fájlba; a felvétel végén nincsenek lógó kísérethangok.
- A visszahallgatott felvétel nem lesz a következő indításkor a dal.
- A főablak a képernyőn nyílik meg, és megjegyzi a teljes méretet.
- A Tér legkisebb értéke −63 (a −64-et a zongora középre szólaltatja).
- Hibaüzenet, ha a regisztrációs memória vagy az OTS mentése nem sikerül.
- A gyári OTS beolvasása sérült stílusfájlnál nem omlik össze, és ha semmit nem olvasott be, a meglévőket megtartja.
- Gyorsabb Dalválasztó; a kotta csak egyszer olvasódik be; biztonságosabb szálkezelés.

A teljes lista a [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) fájlban van.

### Egyéb
- A program a JUCE 9.0.4-gyel készül; kilépéskor a kikapcsolt zongora MIDI-portja azonnal lezárul.
- Magyar kézikönyv (PDF) az 5.0-hoz később jön, az 5.1 fejlesztésével párhuzamosan.

### Ismert hibák
Doricóból exportált kottánál a le nem zárt pedálvonalak egymás alá torlódhatnak, a sor elején több kapcsos zárójel jelenhet meg, és a tempófeliratok egymásra csúszhatnak.

### Telepítés
Csomagold ki a ZIP-fájlt egy tetszőleges mappába, és indítsd el a `ConPianist.exe`-t. A `Resources` mappának és a `.dll` fájloknak az `.exe` mellett kell maradniuk. A gyári OTS-hez a Yamaha hangszerek stílusfájljai kellenek (ezek nincsenek a ZIP-ben): a `%APPDATA%\ConPianist\Styles` mappa modell szerinti almappáiba (`CVP-809`, `CVP-709`, `Genos`, `PSR-SX900`, `PSR-SX700`, `PSR-SX600`, `PSR-S970`, `PSR-S950`) kell tenni őket, majd a Főmenü → ESZKÖZÖK → Gyári OTS beolvasása pontot választani. A saját OTS-ek a `%APPDATA%\ConPianist\OTS` mappába kerülnek.

<details>
<summary>English</summary>

## ConPianist 5.0

Highlights: OTS (One Touch Setting) for the styles – 8 own setups per style and the 4 factory OTS read from the style files of Yamaha instruments; new recording modes in the Recording window (accompaniment into a separate file, recording over a song, layer by layer); a level meter in the mixer; Piano Room can stay open; the A–B marks can be dragged in the score; many fixes and JUCE 9.0.4.

### New – OTS (One Touch Setting)
- **Own OTS:** 8 setups per style for Main, Layer and Left (voice, on/off, volume, octave, pan, reverb), in a separate **OTS window** that can stay open (top bar icon, the OTS button of the Accompaniment window or the O key). Recall with a click or Shift+F1–F8; Store, Delete, Save and Discard. A modified OTS gets a red frame. In the style list a yellow icon marks the styles with own OTS.
- **Factory OTS:** from the style files of the CVP-809, CVP-709, Genos, PSR-SX900, PSR-SX700, PSR-SX600, PSR-S970 and PSR-S950, the program reads the 4 factory OTS of the styles of the CSP-170 with the same name, and maps the voices to the voices of the CSP-170 (all 470 styles get them). Main menu → TOOLS → **Import factory OTS**; where the style files go, see Installation below.
- The Factory OTS part of the OTS window: 4 buttons (Ctrl+F1–F4), **OTS Link** (Main A–D recall factory OTS 1–4, also when the style changes), Copy to the own buttons. A factory OTS can be modified, saved or discarded too.
- When the OTS changes, the notes still sounding are stopped, so the voice change does not sound odd; the accompaniment goes on.

### New – Recording window
- **Three recording modes:** Playing without accompaniment, Playing with accompaniment and Playing over the song/playback (over USB and on a MIDI device). With accompaniment, the playing and the accompaniment can **also be saved into separate files**, next to the full recording.
- **Recording over a song, layer by layer:** the song of ConPianist's own player is recorded too, and the Record button does not stop the playback, so another layer can be recorded. **Load recording** loads an earlier recording into the player.
- **File name, subfolders by date, time stamp:** the name can be edited and is remembered; an existing name gets a number, nothing is overwritten. After saving, the same recording can be saved with other settings too (e.g. quantized).
- New layout: one Record/Stop button, Save, Discard, Play; the length of the recording is shown in measures.

### New – mixer, Piano Room, Accompaniment window
- **Level meter on the mixer channels:** a blue bar shows the notes played, grey when the channel is muted (own player and Live Play).
- **Piano Room can stay open** while playing; the new tuning fork icon of the top bar opens it.
- **Intro and Ending with keys** (Shift+1–3, Ctrl+1–3). The shortcuts of the Accompaniment window work in **every window** of the program.
- Registration memory quickly: Shift+click on a memory button saves, Ctrl+Shift+click deletes.

### New – playback and score
- **A song loaded during playback starts by itself**, as with the piano's own songs. The Song Selector closes on a double click.
- **If the song has a score, the song ends with the last measure of the score**, also if the MIDI file goes on.
- **The A–B marks can be dragged in the score**; Ctrl+click: A, Ctrl+Shift+click: B.
- Nicer score: the tempo mark, the hairpins of crescendo and diminuendo, the tuplet numbers, the rests with two voices and the fingerings of the lower voice are placed correctly.

### Fixed (the main ones)
- Freeze when the right and the left hand are set to the same channel; rare crash on reconnect; crash when loading a memory with an A–B loop without a song.
- Needless reconnect after a slowly loading voice.
- After loading a piano song some parts of the accompaniment were louder.
- The tempo of the Accompaniment and the Recording window no longer changes the tempo of a loaded song that is not playing.
- With playback on a MIDI device, the keyboard parts and the accompaniment settings go to the piano, and the Voice tab can be used.
- When two memories are recalled quickly one after the other, the second one is applied.
- With network playback the tempo no longer slows down at the end of the song; on exit the piano stops the song, and the window disappears at once.
- Saving a recording no longer deletes the old file if writing fails; the tempo set before the recording is written into the file; no hanging accompaniment notes at the end of a recording.
- A recording that was listened back no longer becomes the song at the next start.
- The main window opens on the screen and remembers that it was maximised.
- The lowest Pan value is −63 (the piano plays −64 in the centre).
- An error message if saving the registration memories or the OTS fails.
- Importing the factory OTS does not crash on a damaged style file, and keeps the existing ones if nothing could be read.
- A faster Song Selector; the score is read only once; safer threading.

The full list is in [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) (in Hungarian).

### Other
- The program is built with JUCE 9.0.4; on exit the MIDI port of a piano that was switched off closes at once.
- A user manual (PDF, in Hungarian) for 5.0 comes later, in parallel with the work on 5.1.

### Known issues
In scores exported from Dorico, pedal lines that are not closed may pile up, several braces may appear at the start of a system, and tempo texts may overlap.

### Installation
Unzip the file to any folder and start `ConPianist.exe`. The `Resources` folder and the `.dll` files must stay next to the `.exe`. The factory OTS need the style files of Yamaha instruments (not included): put them into the model subfolders (`CVP-809`, `CVP-709`, `Genos`, `PSR-SX900`, `PSR-SX700`, `PSR-SX600`, `PSR-S970`, `PSR-S950`) of `%APPDATA%\ConPianist\Styles`, then choose main menu → TOOLS → Import factory OTS. The own OTS are saved to `%APPDATA%\ConPianist\OTS`.

</details>
