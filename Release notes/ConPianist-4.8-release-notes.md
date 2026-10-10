## ConPianist 4.8

Fő újdonságok: Dalválasztó a zongora beépített dalaihoz és a saját dalokhoz; Sebesség csúszka USB-s és MIDI-eszközös lejátszáshoz; a kotta követi az ismétléseket, dupla kattintással ugorhatsz benne, és megjelennek az akkordjelölések és az ujjrend; a kíséret részei (Dob, Basszus, Egyéb) külön kapcsolhatók; frissebb kottamegjelenítő és védelem hibás kottafájl ellen.

### Új – Dalválasztó
- **Új ablak** a felső sáv új ikonjával (lista hangjeggyel) vagy a bal panel dalnevére kattintva. Bal oldalon a mappák a Smart Pianist szerkezetében (Score › 50 Popular, PDF Score › 50 Classics, Lesson, Bonus Songs, Music Library, User Songs), jobb oldalon a dalok sorszámmal, címmel, szerzővel és kottaikonnal.
- **Hálózati lejátszásnál** a zongora a saját dalát tölti be, fájl nélkül. **USB-n és MIDI-eszközön** az a beépített dal tölthető be, amelynek MIDI-fájlja ott van a mappájában; a többi szürke.
- **Keresés** minden mappában, a címben és a szerzőben. Betöltés dupla kattintással (a lejátszás is elindul), Enterrel vagy a Betöltés gombbal. Az ablak nyitva maradhat, átméretezhető, a helyét megjegyzi.
- A beépített dal kottája a dal mappájából töltődik be (a fájl neve a dal címe, a rövid neve, például `Pop01`, vagy a kettő együtt). A bal panel a dal címét mutatja.
- A program induláskor létrehozza a mappákat a `%APPDATA%\ConPianist\Songs` mappában. Újraindítás után a legutóbb betöltött beépített dal újra betöltődik.
- Lejátszási mód váltásakor a beépített dal megmarad, ugyanannál az ütemnél.

### Új – lejátszás
- **Sebesség** a bal panelen, a Tempó alatt, USB-s és MIDI-eszközös lejátszáshoz: a dal saját tempójához képest gyorsít vagy lassít (10–200%), a dal tempóváltásaival együtt. A Tempó mindig a dal adott helyén érvényes tempót mutatja. Dupla kattintás: 100%.

### Új – kotta
- **A kotta követi az ismétléseket** (ismétlőjel, volta, D.C., D.S., Fine, Coda): a jelzővonal és a görgetés a lejátszás sorrendjében halad, az A–B jelek is a helyükre kerülnek.
- **Dupla kattintás egy ütemre:** a lejátszás oda ugrik; ismétlődő ütemnél az éppen játszott körben marad.
- **Akkordjelölések** a hangjegysor fölött, a kottaszerkesztő írásmódjával; az egy hangra írt, később következő akkordjelek nem csúsznak egymásra.
- **Ujjrend** a hangok fölött vagy alatt, a kottában megadott oldalon.
- **Tömörített MusicXML (`.mxl`)** is megnyitható, és a kotta megnyitása a mellette lévő, azonos nevű MIDI-fájlt is betölti.
- **Védelem hibás kottafájl ellen:** a program betöltés előtt egy külön példányban kipróbálja a kottát; ha az nem olvasható, lefagy vagy összeomlik, a kotta nem töltődik be, és egy ablak megmondja, miért.
- **Frissebb kottamegjelenítő (Lomse 0.30):** pedáljelek, szebb kottakép; a kotta szövegei a kottában megadott betűmérettel jelennek meg.

### Új – Kíséret és Felvétel ablak
- **A kíséret részei:** Dob, Basszus és Egyéb gomb (gyorsbillentyű: D, B, E). A Break gyorsbillentyűje G lett.
- A felvételbe bekerül a felvétel közbeni ütemmutató-váltás is (például más ütemű stílus választásakor).
- A felvételek a `Songs\User Songs\Recorded Songs` mappába kerülnek. A dalok és a zongoraállapot (`.conmem`) külön mappát jegyez meg.
- Dupla kattintás az alapértékre több helyen; nincsenek összeérő gombok; a feliratok után nincs kettőspont.

### Javítva
- Lefagyás USB-s és MIDI-eszközös lejátszás közben (például a tempó gyors állításakor vagy lejátszási mód váltásakor).
- Hálózatra váltáskor a dal az elejéről indult, ha a zongora még töltötte.
- Nem töltődtek be a Yamaha XF-adatokat tartalmazó MIDI-fájlok (például a zongora Lesson dalai).
- Összeomlás egyes Dorico- és Sibelius-kották betöltésekor; nagy kották lassú betöltése.
- A kotta nem jelent meg rendesen, ha a program ékezetes nevű mappából indult.
- A program elfelejtette a zongora MIDI-portját, ha a Kapcsolat beállításait kikapcsolt zongoránál nyitották meg.

### Tudnivaló – a zongora dalaiból mentett MIDI-fájlok
A Yamaha MIDI-fájlok első üteme egy hang nélküli beállító ütem, a zongora saját példánya enélkül kezdődik. Ha egy beépített dalhoz MIDI-fájlt és kottát teszel a mappájába, ezt az ütemet mindkettőből törölni kell, különben hálózaton a kotta egy ütemmel lemarad. A MIDI-fájlokhoz a repó `Tools` mappájában segédprogram van (`elso_utem_torlese.py`, leírás: `Tools/README.md`); a kottából a kottaszerkesztőben kell törölni az első ütemet.

### Ismert hibák
Doricóból exportált kottánál a le nem zárt pedálvonalak egymás alá torlódhatnak, a sor elején több kapcsos zárójel jelenhet meg, és a tempófeliratok egymásra csúszhatnak.

### Telepítés
Csomagold ki a ZIP-fájlt egy tetszőleges mappába, és indítsd el a `ConPianist.exe`-t. A `Resources` mappának és a `.dll` fájloknak az `.exe` mellett kell maradniuk. A dalok új alapértelmezett mappája a `%APPDATA%\ConPianist\Songs`; a korábbi `%APPDATA%\ConPianist\Demo Midi Songs` mappát a program nem törli és nem másolja át. A ZIP-ben lévő `Demo Midi Songs` mappát érdemes a `%APPDATA%\ConPianist\Songs\Demo Midi Songs` mappába másolni. A részletes változáslista a [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) fájlban van.

<details>
<summary>English</summary>

## ConPianist 4.8

Highlights: a Song Selector for the piano's built-in songs and the user's own songs; a Speed control for playback over USB and on a MIDI device; the score follows repeats, a double click jumps in it, and chord symbols and fingerings are shown; the parts of the accompaniment (Rhythm, Bass, Others) can be switched separately; a newer score renderer and protection against faulty score files.

### New – Song Selector
- **A new window**, opened with the new top bar icon (a list with a note) or by clicking the song name in the left panel. On the left the folders in the structure of Smart Pianist (Score › 50 Popular, PDF Score › 50 Classics, Lesson, Bonus Songs, Music Library, User Songs), on the right the songs with number, title, composer and a score icon.
- **With network playback** the piano loads its own song, without a file. **Over USB and on a MIDI device** a built-in song can be loaded if its MIDI file is in its folder; the others are grey.
- **Search** in every folder, in the title and the composer. Load with a double click (playback starts too), Enter or the Load button. The window can stay open, can be resized and remembers its position.
- The score of a built-in song is loaded from the song's folder (named after the title, the short name such as `Pop01`, or both). The left panel shows the title of the song.
- The program creates the folders in `%APPDATA%\ConPianist\Songs` at start. After a restart the built-in song loaded last is loaded again.
- Switching the player keeps the built-in song, at the same measure.

### New – playback
- **Speed** in the left panel, under the Tempo, for playback over USB and on a MIDI device: faster or slower relative to the song's own tempo (10–200%), with the tempo changes of the song. The Tempo always shows the tempo at the current position of the song. Double click: 100%.

### New – score
- **The score follows repeats** (repeat signs, voltas, D.C., D.S., Fine, Coda): the position line and the scrolling follow the playback order, and the A–B marks are placed correctly.
- **Double click on a measure:** playback jumps there; for a repeated measure it stays in the current pass.
- **Chord symbols** above the staff, as the notation program writes them; several chord symbols over one note no longer overlap.
- **Fingerings** above or below the notes, on the side given in the score.
- **Compressed MusicXML (`.mxl`)** can be opened, and opening a score also loads the MIDI file with the same name next to it.
- **Protection against faulty score files:** before loading, the program tries the score in a separate instance; if it cannot be read, freezes or crashes, the score is not loaded and a window tells why.
- **Newer score renderer (Lomse 0.30):** pedal marks, nicer engraving; the texts of the score use the font size given in the score.

### New – Accompaniment and Recording windows
- **Parts of the accompaniment:** Rhythm, Bass and Others buttons (shortcuts: D, B, E). The shortcut of Break is now G.
- Time signature changes made while recording (e.g. choosing a style with another time signature) are written into the recording.
- Recordings are saved to `Songs\User Songs\Recorded Songs`. Songs and the piano state (`.conmem`) remember their own folders.
- Double click resets a control to its default in more places; no buttons touching each other; no colons after the labels.

### Fixed
- Freeze during playback over USB and on a MIDI device (e.g. when changing the tempo quickly or switching the player).
- Switching to network playback started the song from the beginning if the piano was still loading it.
- MIDI files with Yamaha XF data (e.g. the piano's Lesson songs) could not be loaded.
- Crash when loading some Dorico and Sibelius scores; slow loading of large scores.
- The score was not shown properly when the program was started from a folder with accented letters in its name.
- The program forgot the piano's MIDI port if the Connection settings were opened while the piano was switched off.

### Note – MIDI files saved from the piano's songs
The first measure of Yamaha MIDI files is a setup measure without notes; the piano's own copy starts without it. If you put a MIDI file and a score for a built-in song into its folder, remove this measure from both, otherwise the score lags one measure behind with network playback. The repository's `Tools` folder has a script for the MIDI files (`elso_utem_torlese.py`, see `Tools/README.md`, in Hungarian); remove the first measure from the score in the notation program.

### Known issues
In scores exported from Dorico, pedal lines that are not closed may pile up, several braces may appear at the start of a system, and tempo texts may overlap.

### Installation
Unzip the file to any folder and start `ConPianist.exe`. The `Resources` folder and the `.dll` files must stay next to the `.exe`. The new default folder of the songs is `%APPDATA%\ConPianist\Songs`; the former `%APPDATA%\ConPianist\Demo Midi Songs` folder is neither deleted nor copied. It is worth copying the included `Demo Midi Songs` folder to `%APPDATA%\ConPianist\Songs\Demo Midi Songs`.

</details>
