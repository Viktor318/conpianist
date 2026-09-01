# Changelog

Ez a fájl a ConPianist változásait dokumentálja. Az 1.0–3.0 verziók az eredeti [hugbug/conpianist](https://github.com/hugbug/conpianist) projekt kiadásai (lásd az [eredeti release-eket](https://github.com/hugbug/conpianist/releases)); az ez utáni bejegyzések ennek a fork-nak ([Viktor318/conpianist](https://github.com/Viktor318/conpianist)) a saját, magáncélú fejlesztései.

## [Fork – folyamatban] – 2026

Saját célú továbbfejlesztés Yamaha CSP-170 zongorához, Visual Studio 2026 / JUCE 9.0.1 / vcpkg alapú modern build-környezettel.

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
- A "MIDI-fájl betöltési hiba" jelenség (`Error loading midi`) nem szoftverhiba: a MIDI-fájl feltöltése a zongorára hálózaton keresztül történik (TCP, 10504-es port), USB-kapcsolattal ez a funkció nem használható — a zongorát Wi-Fi-re kell kötni, és a Connection képernyőn be kell állítani a helyes "Piano IP" címet.

---

## 4.0 – soha hivatalosan ki nem adva (fejlesztés: 2020. május–november)

Az eredeti fejlesztő a `v3.0` után a `.jucer`-ben átírta a verziószámot "4.0"-ra, és tovább dolgozott a `develop` branchen — de ez a munka soha nem lett formális GitHub release-ként kiadva vagy dokumentálva (2020. november 7-i utolsó commit után a fejlesztés láthatóan leállt). Emiatt az alkalmazás jelenleg is "4.0"-t mutat, hivatalos changelog nélkül. Az alábbi lista a `v3.0` tag és a `develop` branch közti 35 commit alapján készült.

- **"Piano Room" panel** (új funkció): a zongora teremszimulációs beállításai egy önálló felületen — fényesség (brightness), Virtual Resonance Modeling (VRM), húr- és csillapítórezonancia, mesterhangolás (master tune), billentésgörbe (touch curve), fedélpozíció (lid position), key-off sampling —, ezekhez saját regisztrációs memóriával; a megnyitó gomb a hangválasztó panelbe került.
- **Alap Android-támogatás**: aszinkron dialógusok és menük, új `GuiHelper` modul, kotta-komponens és betűtípus-kezelés Android-on, build-jegyzetek minden platformra.
- Simább (smooth) csúszka-viselkedés.
- Forráskód-átszervezés (belső refaktorálás, funkcionális változás nélkül).
- FreeType statikus linkelése macOS-en (build-only változtatás).

## 3.0 – 2020. május 15.

- Gyorsabb hálózati csatlakozási idő és üzenet-visszaigazolási protokoll a kapcsolatkezelésben.
- Regisztrációs memória (beállítások) mentése és betöltése `.conmem` fájlokba.
- MIDI-dalokhoz párosított regisztrációs memória fájlok automatikus betöltése.
- Frissített Lomse kotta-nézet komponens.
- Automatikus kotta-méretezés kis ablakméret esetén.
- Jobban megkülönböztethető dialógusablakok.
- Opcionális naplózás (logging) lehetősége.

## 2.0 – 2020. április 15.

- Teljes értékű keverő bevezetése csatorna-vezérléssel és effekt-kezeléssel.
- Rész-kiválasztás és hangszínválasztás közvetlenül a keverő csatornáiból.
- Balansz-dialógus vezérlőkkel.
- Oktáveltolás beállítása.
- Kották szűrése rész szerint.
- Stream lights sebességének állítása.
- Guide mód kiválasztási lehetőségek.
- Zongoraállapot szinkronizálása újracsatlakozáskor.
- Alapszintű iOS/iPad támogatás.

## 1.0 – 2020. március 14.

- Első kiadás. Alapfunkciók:
  - hálózati zongoracsatlakozás (USB-n keresztül még nem támogatott);
  - MIDI-fájl feltöltés és lejátszásvezérlés;
  - stream lights és guide mód kezelése;
  - rész-kiválasztás;
  - hangszínválasztás mind a 700+ elérhető hangból;
  - hangerő/tempó/transzponálás állítása;
  - kotta megjelenítése külön MusicXML-fájlból, szinkronizált lejátszási pozícióval.
