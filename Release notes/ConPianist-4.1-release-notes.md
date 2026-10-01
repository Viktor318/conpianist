## ConPianist 4.1 (fork)

Saját célú továbbfejlesztés Yamaha CSP-170 zongorához, Visual Studio 2026 / JUCE 9.0.1 / vcpkg alapú modern build-környezettel.

### Új
- **Kétnyelvű (magyar/angol) kezelőfelület.** A nyelv a Főmenü → NYELV / LANGUAGE pontban váltható, a program újraindítása után lép életbe; első indításkor a Windows nyelvét követi. A fordítás a `Translations/translation_hu.txt` fájlban van, és a programba épül be. A szakkifejezések a CSP-170 magyar használati útmutatóját követik; a hangszínek és zengetéstípusok neve, a Stream Lights és a Piano Room angolul maradt.
- A korábban nem fordítható menü- és állapotszövegek fordíthatóvá tétele.

### Módosítva
- Verziószám: 4.1. A projektadatokban (`.exe` tulajdonságai) a készítő Viktor Oszkó; a szerzői jogi megjegyzés az eredeti szerzőt is megtartja.
- A Főmenü „Honlap” pontja a fork oldalára mutat (github.com/Viktor318/conpianist).
- A Windows-verzió saját alkalmazásikont kapott (`app-icon.svg`).

### Javítva
- **Build-hibák javítása modern eszközlánccal**: a projekt eredetileg egy ~2020-as JUCE 5.4.7-es környezetre volt beállítva; frissítve, hogy Visual Studio 2026-tal és egy friss JUCE-verzióval is lefordítható legyen.
  - FreeType hiányzó fejléceinek/könyvtárainak bekötése vcpkg-n keresztül (`ConPianist.jucer` VS2026 exportőr: `extraCompilerFlags`/`extraLinkerFlags`).
  - `/bigobj` fordítási kapcsoló hozzáadása (a Lomse egyik forrásfájlja túllépte az objektumfájl-formátum szekciólimitjét).
  - `Library/AppleMIDI/IPAddress.h`: hiányzó `<chrono>` include pótlása.
  - `Source/Scene/ConnectionComponent.cpp`: az újabb JUCE-verziókban megszűnt `MidiInput::getDevices()` lecserélve a jelenlegi `MidiInput::getAvailableDevices()` API-ra.
  - `Library/Lomse/src/render/lomse_font_freetype.cpp`: `char*`/`unsigned char*` típusütközés javítása (a friss FreeType fejlécek `FT_Byte*` típust várnak).
- **Hiányzó `Resources` mappa a build kimenetében**: a build korábban nem másolta be automatikusan a `Resources` mappát (betűtípusok, ikonok stb.) a lefordított `.exe` mellé, ami hibás/hiányos kotta-megjelenítést okozott (hiányzó hangfejek, violinkulcsok). Megoldás: automatikus post-build másolási lépés hozzáadva a `.jucer`-hez és a `.vcxproj`-hoz, ami a `Resources` mappát és a vcpkg FreeType DLL-jeit is átmásolja az `.exe` mellé.
- **Összeomlás dalszöveget (lyric) tartalmazó kották betöltésekor** (`vector subscript out of range`): a Lomse `LyricEngraver::create_shapes()` idő előtt ürítette ki a belső adatszerkezetét, mielőtt a hívó kód ki tudta volna olvasni belőle a létrehozott alakzatokat. Ez minden dalszöveget tartalmazó kottánál összeomlást (vagy a javítás első, ideiglenes változatában néma kihagyást) okozott volna — csak azért nem tűnt fel korábban, mert a gyakorló kották jellemzően nem tartalmaznak dalszöveget. Végleges javítás: a takarítást egy új `prepare_for_next_system()` metódusba mozgattuk, amit a hívó csak az adatok kiolvasása *után* hív meg (`lomse_engraver.h`, `lomse_lyric_engraver.h`/`.cpp`, `lomse_system_layouter.cpp`).

### Megjegyzés
- A "MIDI-fájl betöltési hiba" jelenség (`Error loading midi`) nem szoftverhiba: a MIDI-fájl feltöltése a zongorára hálózaton keresztül történik (TCP, 10504-es port), USB-kapcsolattal ez a funkció nem használható — a zongorát Wi-Fi-re kell kötni, és a Connection képernyőn be kell állítani a helyes "Piano IP" címet. A 4.2 óta a dalok USB-n, Wi-Fi nélkül is lejátszhatók.

A részletes változáslista a [CHANGELOG.md](CHANGELOG.md) fájlban található.

---

<details>
<summary><b>English</b></summary>

## ConPianist 4.1 (fork)

A personal continuation for the Yamaha CSP-170, with a modern build environment based on Visual Studio 2026 / JUCE 9.0.1 / vcpkg.

### New
- **Bilingual (Hungarian/English) user interface.** The language can be changed in Main menu → NYELV / LANGUAGE and takes effect after restarting the program; at the first start it follows the Windows language. The translation is in `Translations/translation_hu.txt` and is built into the program. The terms follow the Hungarian owner's manual of the CSP-170; the names of voices and reverb types, Stream Lights and Piano Room stay in English.
- Menu and status texts that could not be translated before are now translatable.

### Changed
- Version number: 4.1. In the project data (properties of the `.exe`), the author is Viktor Oszkó; the copyright notice keeps the original author as well.
- The "Homepage" item in the Main menu points to the fork (github.com/Viktor318/conpianist).
- The Windows version got its own application icon (`app-icon.svg`).

### Fixed
- **Build errors with a modern toolchain:** the project was originally set up for a ~2020 JUCE 5.4.7 environment; it was updated so that it builds with Visual Studio 2026 and a current JUCE version.
  - Missing FreeType headers/libraries linked through vcpkg (`ConPianist.jucer`, VS2026 exporter: `extraCompilerFlags`/`extraLinkerFlags`).
  - `/bigobj` compiler flag added (a Lomse source file exceeded the section limit of the object file format).
  - `Library/AppleMIDI/IPAddress.h`: missing `<chrono>` include added.
  - `Source/Scene/ConnectionComponent.cpp`: `MidiInput::getDevices()`, removed in newer JUCE versions, replaced with the current `MidiInput::getAvailableDevices()` API.
  - `Library/Lomse/src/render/lomse_font_freetype.cpp`: `char*`/`unsigned char*` type mismatch fixed (current FreeType headers expect `FT_Byte*`).
- **Missing `Resources` folder in the build output:** the build did not copy the `Resources` folder (fonts, icons etc.) next to the compiled `.exe`, which caused incomplete score rendering (missing note heads, clefs). Fixed with an automatic post-build step in the `.jucer` and the `.vcxproj` that copies the `Resources` folder and the vcpkg FreeType DLLs next to the `.exe`.
- **Crash when loading scores with lyrics** (`vector subscript out of range`): Lomse's `LyricEngraver::create_shapes()` cleared its internal data too early, before the calling code could read the created shapes. This would have crashed on every score with lyrics (or, in the first temporary version of the fix, silently skipped them) – it only went unnoticed because practice scores usually have no lyrics. Final fix: the cleanup was moved to a new `prepare_for_next_system()` method, which the caller calls only *after* reading the data (`lomse_engraver.h`, `lomse_lyric_engraver.h`/`.cpp`, `lomse_system_layouter.cpp`).

### Note
- The "Error loading midi" message is not a software bug: MIDI files are uploaded to the piano over the network (TCP, port 10504), which does not work with a USB connection alone – the piano must be connected to Wi-Fi, and the correct "Piano IP" must be set in the Connection Settings. Since 4.2, songs can also be played over USB without Wi-Fi.

The detailed list of changes (in Hungarian) is in [CHANGELOG.md](CHANGELOG.md).

</details>
