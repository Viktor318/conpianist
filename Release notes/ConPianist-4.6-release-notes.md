## ConPianist 4.6

Fő újdonságok: Kíséret ablak a zongora kíséretének (stílus) vezérléséhez; a zongora XG, GM2 és GS hangszínei pontos névvel, a Keverő menüjéből választhatók; a kísérettel készült felvétel a kíséret hangszíneivel együtt mentődik.

### Új
- **Kíséret ablak.** A főmenü új **Kíséret…** pontja külön, nyitva tartható ablakot nyit a zongora kíséretének vezérléséhez: indítás és leállítás, Sync Start, a szakaszok (Intro 1–3, Main A–D, Fill In, Break, Ending 1–3) az éppen szóló és a következő szakasz jelzésével, a kíséret hangereje, a felismert akkord és a stílus neve. Az Intro 2–3 és az Ending 2–3 a Smart Pianist alkalmazásból nem érhető el.
- **Fill In és Auto Fill:** a Fill In egy ütem kitöltést játszik ugyanabban a Main szakaszban; az Auto Fill bekapcsolva a Main szakasz váltásakor játszik kitöltést az új szakasz előtt.
- **Stílusválasztó:** három, szabadon böngészhető lista (főkategória, alkategória, stílus), a kiválasztott stílust az Alkalmaz gomb tölti be. A listák a program adatmappájában lévő `styles.csv` fájlból töltődnek, amely nem része a programnak; nélküle a listák üresek, a többi vezérlő működik.
- **Akkord a darab hangneme szerint:** megadható a darab (dúr) hangneme, és az akkordok neve a hangnem keresztjeivel vagy béivel jelenik meg.
- **Tempó:** Tap Tempo és Reset (a stílus alapértelmezett tempója); a főablak és a Kíséret ablak tempója minden lejátszási módban együtt változik.
- **Gyorsbillentyűk** a Kíséret ablakban (szóköz, 1–4, F, A, B, T, R, Enter); a listát a Gyorsbillentyűk súgója gomb mutatja.
- **XG, GM2 és GS hangszínek:** a Keverő csatornamenüjének **Hangszín módosítása** almenüjében új XG, GM2 és GS csoport, kategóriánként (480 XG, 256 GM2, 226 GS hang). A Keverőn, a mentett állapotban és a felvételben a hang saját neve jelenik meg.
- A **Hangszín átadása** almenü kiírja, melyik panel hang kerül a billentyűzet szólamára, ha a csatornán XG, GM2 vagy GS hang szól.
- A **Felvétel** és a **Kíséret** ablak kis méretre állítható; a felvétel, a visszahallgatás és a kíséret közben megy tovább. A program megjegyzi a két ablak helyét, és újraindítás után is ott nyitja meg őket.

### Javítva
- A kísérettel készült felvétel a kíséret hangszíneivel szól: eddig a kíséretsávokból többnyire hiányoztak a hangszínek, így lejátszva minden szólam zongorahangon szólt.
- Bekapcsolt Sync Start mellett a zongora hálózaton nem töltötte be a dalt, USB-s lejátszásnál pedig a dal hangjai elindították a kíséretet. A program a betöltés és a lejátszás idejére kikapcsolja a Sync Startot, utána visszakapcsolja.
- Ha egy másik program vagy egy dal GS módba kapcsolta a zongorát, a Keverőben a hangszín neve helyén számok jelentek meg, és ez a következő indításkor is visszatért.

### Tudnivaló
A kíséretet a zongora játssza, ezért a Kíséret ablakhoz csatlakoztatott zongora kell. A 10. (dob) csatornán a GM2 és GS dobkészletek nem választhatók, mert ott a zongora a saját dobkészletét szólaltatja meg helyettük.

### Telepítés
Csomagold ki a ZIP-fájlt egy tetszőleges mappába, és indítsd el a `ConPianist.exe`-t. A `Resources` mappának és a `.dll` fájloknak az `.exe` mellett kell maradniuk. A részletes változáslista a [CHANGELOG.md](https://github.com/Viktor318/conpianist/blob/develop/CHANGELOG.md) fájlban van.

<details>
<summary>English</summary>

## ConPianist 4.6

Highlights: an Accompaniment window to control the piano's accompaniment (style); the XG, GM2 and GS voices of the piano with their exact names, chosen from the mixer menu; a recording made with the accompaniment is saved with the voices of the accompaniment.

### New
- **Accompaniment window.** The new **Accompaniment…** item of the main menu opens a separate window that can stay open, to control the piano's accompaniment: start and stop, Sync Start, the sections (Intro 1–3, Main A–D, Fill In, Break, Ending 1–3) with the current and the next section marked, accompaniment volume, the recognised chord and the name of the style. Intro 2–3 and Ending 2–3 are not available from the Smart Pianist app.
- **Fill In and Auto Fill:** Fill In plays a one-measure fill and stays in the same Main section; with Auto Fill on, a fill is played before the new section when the Main section is changed.
- **Style selector:** three freely browsable lists (category, subcategory, style); the chosen style is loaded with the Apply button. The lists are read from the `styles.csv` file in the program's data folder, which is not part of the program; without it the lists are empty and the other controls work.
- **Chords in the key of the piece:** the (major) key of the piece can be given, and the chord names are spelled with the sharps or flats of that key.
- **Tempo:** Tap Tempo and Reset (the default tempo of the style); the tempo of the main window and of the Accompaniment window change together in every playback mode.
- **Keyboard shortcuts** in the Accompaniment window (space, 1–4, F, A, B, T, R, Enter); the Keyboard shortcuts button lists them.
- **XG, GM2 and GS voices:** new XG, GM2 and GS groups in the **Change Voice** submenu of the mixer channel menu, by category (480 XG, 256 GM2, 226 GS voices). The mixer, the saved state and the recording show the voice's own name.
- The **Use Voice for** submenu shows which panel voice goes to the keyboard part when the channel has an XG, GM2 or GS voice.
- The **Recording** and **Accompaniment** windows can be minimised; recording, listening back and the accompaniment go on meanwhile. The program remembers the position of both windows and opens them there after a restart too.

### Fixed
- A recording made with the accompaniment plays with the voices of the accompaniment: the voices were mostly missing from the accompaniment tracks, so every part played with a piano voice.
- With Sync Start on, the piano did not load a song over the network, and with USB playback the notes of the song started the accompaniment. The program switches Sync Start off while a song is loaded and played, and on again afterwards.
- When another program or a song had switched the piano to GS mode, the mixer showed numbers instead of the voice name, and this came back at the next start.

### Note
The accompaniment is played by the piano, so the Accompaniment window needs a connected piano. On channel 10 (drums) the GM2 and GS drum kits cannot be chosen, because the piano plays its own drum kit instead.

### Installation
Unzip the file to any folder and start `ConPianist.exe`. The `Resources` folder and the `.dll` files must stay next to the `.exe`.

</details>
