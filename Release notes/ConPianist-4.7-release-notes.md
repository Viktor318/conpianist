## ConPianist 4.7

Fő újdonságok: regisztrációs memóriák és beépített stíluslista a Kíséret ablakban; kíséret keverő a kíséret nyolc szólamához; nyitva tartható Hangerőegyensúly ablak; a kíséret beállításai is megmaradnak a program bezárásakor; a felvett MIDI-fájlba bekerül a hangnem, az akkordok és a tempóváltások.

### Új – Kíséret ablak
- **Regisztrációs memóriák.** Nyolc számozott gomb tárolja a stílust, a tempót, a hangnemet, a billentyűzet három szólamát (Main, Layer, Left), a split pontot, az akkordfelismerést és a kíséret keverőjét. Mentés: Memória, majd egy szám; visszahívás: a szám vagy F1–F8. A memóriáknak saját név adható, a tartalmukat buboréksúgó mutatja.
- **A stíluslista a program része.** A CSP-170 mind a 470 stílusa választható, a `styles.csv` fájlra már nincs szükség. A név mellett a stílus típusa és ütemmutatója áll, például „Jazz Waltz Medium · Session · (3/4)”.
- **Ütem szűrő.** A hangnem melletti Ütem lista (2/4, 3/4, 4/4, 5/4, 6/4, 6/8, 9/8, 12/8) a kiválasztott ütemmutatóra szűri a stílusokat; ilyenkor a kategórialisták is csak a találatokat mutatják. A nyolcados stílusok (például „6-8 Modern”) a nevük szerinti ütemmutatóval jelennek meg, bár a zongora ezeket 4/4-ként tartja nyilván.
- **Akkordfelismerés és split pont.** A Full és Lower gomb azt állítja, hogy a zongora a teljes billentyűzeten vagy csak a split pont alatt ismeri fel az akkordot. A split pont nyilakkal léptethető, vagy a Tanulás gombbal egy billentyű leütésével adható meg. A „Fő hang lent” gomb azt kapcsolja, hogy a split pont alatt megszólal-e a fő hang.
- **Dúr vagy moll hangnem** az akkordok nevéhez; az akkord kijelzése külön sorba, nagyobb keretbe került.
- Az aktuális stílus neve nagyobb, narancssárga betűvel, keretben látszik.

### Új – ablakok
- **Kíséret keverő.** Új ablak a kíséret nyolc szólamához (Rhythm 1–2, Bass, Chord 1–2, Pad, Phrase 1–2): be- és kikapcsolás, hangerő, tér, zengetés, a szólam hangszínének nevével, és Master csík a teljes kísérethez. Dupla kattintás a stílus saját értékére állít vissza.
- **Hangerőegyensúly ablak.** Nyitva tartható, mint a Felvétel és a Kíséret ablak, és új Stílus csíkot kapott; a csík címe a teljes kíséretet kapcsolja ki és be.
- **Négy ikon a felső sávban:** Felvétel, Kíséret, Hangerőegyensúly, Kíséret keverő.
- A program megjegyzi az ablakok helyét.

### Új – felvétel
- A felvett MIDI-fájlba bekerül a **beállított hangnem** (előjegyzésként), a zongora által **felismert akkordok** és a felvétel közbeni **tempóváltások**.
- A sávok neve a szólam és a hangszín neve, például „Main: Pop Grand”; a kíséret sávjai a szólamok nevét kapják (Rhythm 1, Bass, …).
- A Felvétel ablakban új Mappa gomb nyitja meg a felvételek mappáját.

### Új – mentett állapot
- **A kíséret beállításai is megmaradnak.** A legutóbbi állapotba és a `.conmem` fájlba bekerül a stílus, a tempó, az akkordfelismerés, a split pont és a kíséret keverője; a `.conmem` a Kíséret ablak hangnemét is tartalmazza. A korábbi verzióval mentett fájlok ugyanúgy betölthetők.
- **A „Zongora visszaállítása alapállapotba” a kíséretet is visszaállítja.** Az alapállapot az, amit a Smart Pianist állít be az indításakor, egy kivétellel: a „Fő hang lent” ki van kapcsolva.

### Tudnivaló
A Smart Pianist és a ConPianist egyszerre is csatlakozhat a zongorához, de ha a Smart Pianist fut, a zongora eldobhatja a ConPianistből betöltött dalt. Dallejátszáshoz érdemes a Smart Pianistet bezárni.

### Telepítés
Csomagold ki a ZIP-fájlt egy tetszőleges mappába, és indítsd el a `ConPianist.exe`-t. A `Resources` mappának és a `.dll` fájloknak az `.exe` mellett kell maradniuk. A részletes változáslista a [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) fájlban van.

<details>
<summary>English</summary>

## ConPianist 4.7

Highlights: registration memories and a built-in style list in the Accompaniment window; an accompaniment mixer for the eight parts of the accompaniment; a Balance window that can stay open; the accompaniment settings are kept when the program is closed; the key, the chords and the tempo changes are written into the recorded MIDI file.

### New – Accompaniment window
- **Registration memories.** Eight numbered buttons store the style, the tempo, the key, the three keyboard parts (Main, Layer, Left), the split point, the chord detection area and the accompaniment mixer. Save: Memory, then a number; recall: the number or F1–F8. A memory can be given a name; a tooltip shows what it holds.
- **The style list is part of the program.** All 470 styles of the CSP-170 can be chosen; the `styles.csv` file is no longer needed. The type and the time signature of a style are shown after its name, e.g. "Jazz Waltz Medium · Session · (3/4)".
- **Time signature filter.** The Meter list next to the key (2/4, 3/4, 4/4, 5/4, 6/4, 6/8, 9/8, 12/8) shows only the styles of the chosen time signature; the category lists show only the categories that have such styles. The styles in eighth-note time (e.g. "6-8 Modern") are shown with the time signature of their name, although the piano keeps them as 4/4.
- **Chord detection area and split point.** The Full and Lower buttons choose whether the piano detects the chord on the whole keyboard or below the split point. The split point is stepped with the arrows or given by pressing a key after the Learn button. The "Main voice below" button switches the main voice below the split point.
- **Major or minor key** for the chord names; the chord is shown in a row of its own, in a larger frame.
- The name of the current style is shown in a frame, in larger orange letters.

### New – windows
- **Accompaniment mixer.** A new window for the eight parts of the accompaniment (Rhythm 1–2, Bass, Chord 1–2, Pad, Phrase 1–2): on/off, volume, pan, reverb, with the name of the part's voice, and a Master strip for the whole accompaniment. A double click sets a control back to the style's own value.
- **Balance window.** It can stay open like the Recording and Accompaniment windows and has a new Style strip; the title of the strip switches the whole accompaniment off and on.
- **Four icons in the top bar:** Recording, Accompaniment, Balance, Accompaniment mixer.
- The program remembers the positions of its windows.

### New – recording
- The **key** (as a key signature), the **chords** detected by the piano and the **tempo changes** made while recording are written into the recorded MIDI file.
- The tracks are named with the part and its voice, e.g. "Main: Pop Grand"; the accompaniment tracks get the names of the parts (Rhythm 1, Bass, …).
- A new Folder button in the Recording window opens the folder of the recordings.

### New – saved state
- **The accompaniment settings are kept.** The last state and the `.conmem` file also hold the style, the tempo, the chord detection area, the split point and the accompaniment mixer; the `.conmem` file holds the key of the Accompaniment window too. Files saved with earlier versions load as before.
- **"Reset Piano to Default State" resets the accompaniment too.** The default state is the one Smart Pianist sets when it is started, with one exception: "Main voice below" is off.

### Note
Smart Pianist and ConPianist can be connected to the piano at the same time, but while Smart Pianist is running the piano may drop a song loaded from ConPianist. Close Smart Pianist when playing songs.

### Installation
Unzip the file to any folder and start `ConPianist.exe`. The `Resources` folder and the `.dll` files must stay next to the `.exe`.

</details>
