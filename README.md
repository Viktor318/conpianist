# ConPianist

> Ez ennek a projektnek **[Viktor318](https://github.com/Viktor318/conpianist) általi saját fork-ja**, saját célú, magánhasználatú továbbfejlesztésre. Az eredeti projekt: [hugbug/conpianist](https://github.com/hugbug/conpianist). Az angol nyelvű eredeti README a [README.en.md](README.en.md) fájlban található.

**ConPianist**, vagyis **Connected Pianist**, egy alkalmazás a Yamaha CSP (Clavinova Smart Piano) sorozatú digitális zongorák vezérlésére. Alternatívája a Yamaha saját "Smart Pianist" alkalmazásának. A Smart Pianist-tel ellentétben, ami iOS-en és Androidon fut, a Connected Pianist asztali rendszerekre készült — macOS, Windows és Linux alá. iPad-en is működik.

## Letöltés

A lefordított, Windowson (64 bit) futtatható változat a [Releases](https://github.com/Viktor318/conpianist/releases) oldalon található. A ZIP-fájlt egy tetszőleges mappába kell kicsomagolni, és a `ConPianist.exe`-t elindítani. A `Resources` mappának és a `.dll` fájloknak az `.exe` mellett kell maradniuk.

## Funkciók

A program egyelőre nem teljes értékű helyettesítője a hivatalos alkalmazásnak. Ennek ellenére már most is tud:
- csatlakozni a zongorához hálózaton vagy kábelen keresztül;
- állapotvesztés nélkül csatlakozni/újracsatlakozni: a program indításkor beolvassa a zongora teljes állapotát, és megjeleníti a felületen;
- **ugyanabban az állapotban indulni, ahogy bezárták**: bezáráskor a program elmenti a lejátszási módot, a hangszíneket, a Piano Room és a hangerőegyensúly beállításait, a Keverőt, a Lejátszás panelt, a betöltött dalt és a dalban elért pozíciót, és a következő indításkor ezeket visszaállítja;
- MIDI-fájlokat feltölteni a zongorára hálózaton keresztül;
- **MIDI-fájlokat lejátszani USB-kábelen keresztül is, Wi-Fi nélkül**: ilyenkor a ConPianist saját lejátszója játssza a dalt (tempó, transzponálás, ismétlés, szólamok, kotta szinkron működik; Stream Lights és Segéd nem). A bal panel Lejátszás részében választható, hogy a zongora saját lejátszója (hálózaton keresztül) vagy a ConPianist lejátszója (USB-n keresztül) játsszon; a választást a program megjegyzi;
- **MIDI-fájlokat lejátszani más MIDI-eszközre is** (pl. loopMIDI-n keresztül szoftveres hangszerre, például a Cantabile-be): a Kapcsolat beállításaiban a MIDI Out és a MIDI In 2 állítható be; ha a zongora nem érhető el, a lejátszás automatikusan a MIDI-eszközre vált, a zongora mellett pedig a Lejátszás részben kézzel is választható. A Keverő ilyenkor szabványos MIDI-vezérlőket és General MIDI hangszíneket használ, a virtuális billentyűzet és a MIDI In 2 a Keverőben kiválasztott élő játék csatornákon (akár többön, rétegezve) szól, a beállított transzponálással;
- a feltöltött MIDI-fájlok lejátszását vezérelni: indítás, szünet, pozíció;
- a "stream lights" (billentyű-kivilágítás) vezérlése: ki, be, lassú, gyors;
- a segéd (vezetett gyakorlás) mód vezérlése: ki, be, mód kiválasztása;
- részek kiválasztása: kíséret, jobb kéz, bal kéz;
- kiválasztott szakasz lejátszása ismétlődő (loop) módban;
- hangerő, tempó, transzponálás beállítása;
- hangszínek kiválasztása (mind a hétszáznál is több) a fő, bal kezes és réteg (layer) hangokhoz;
- oktáveltolás és osztáspont (fő/bal) beállítása;
- keverő az összes klasszikus funkcióval: MIDI-csatornák ki/be kapcsolása, hangerő, pan, zengetés, zengetéstípus;
- extra funkciók a keverőben: rész-kiválasztás MIDI-csatornánként, hangszín kiválasztása közvetlenül a MIDI-csatornákból;
- **a zenedarab csatornáinak hangszínét módosítani a keverőben** (pl. a jobb és bal kéz szólamát más hangszínen hallgatni);
- hangerőegyensúly (balansz) beállítása a fő/bal/réteg/dal/mikrofon/aux in csatornákra: hangerő, pan, zengetés, zengetéstípus;
- **Piano Room**: a zongora hangzásának finomhangolása — fedél helyzete, fényesség, környezet (zengetés), billentés érzékenysége, hangolás, virtuális rezonanciamodellezés (VRM), tompító- és húrrezonancia, billentyűfelengedési hang;
- kották megjelenítése a lejátszási pozícióval szinkronban: a kottákat külön MusicXML-fájlban kell megadni (közvetlenül a MIDI-fájlból nem jeleníthető meg kotta);
- a regisztrációs memória (beállítások) MIDI-dalokhoz rendelése, **a zenedarab csatornáinak saját hangszíneivel együtt**;
- **kétnyelvű (magyar/angol) kezelőfelület** — a nyelv a Főmenü → NYELV / LANGUAGE pontban váltható.

## Képernyőképek

![Lejátszás kottával](https://user-images.githubusercontent.com/3368402/79354560-d8c86b00-7f3c-11ea-8710-9c948982de3a.png)

![Hangszínválasztás](https://user-images.githubusercontent.com/3368402/79360975-f699ce00-7f44-11ea-8aa1-803be03533d7.png)

![Keverő](https://user-images.githubusercontent.com/3368402/79354913-4ffdff00-7f3d-11ea-8bf9-0ecda2180204.png)

*(A képernyőképek az eredeti, angol nyelvű változatot mutatják.)*

## Köszönetnyilvánítás

A ConPianist forráskódja a következő könyvtárakat tartalmazza:
- [Lomse](https://github.com/lenmus/lomse) a kották megjelenítéséhez;
- [Arduino AppleMIDI Library](https://github.com/lathoub/Arduino-AppleMIDI-Library) a zongorával való hálózati kommunikációhoz.

## Erről a fork-ról

Ez a változat saját, magáncélú felhasználásra készül egy Yamaha CSP-170 zongorához, modern fejlesztői eszközökkel (Visual Studio 2026, friss JUCE, vcpkg) újra buildelve. Az eredeti programhoz képest a legfontosabb változások:
- kétnyelvű (magyar/angol) kezelőfelület, a szakkifejezések a CSP-170 magyar használati útmutatóját követik;
- a zenedarab csatornáinak hangszíne módosítható a keverőben, és a regisztrációs memóriába is mentődik;
- zenedarab lejátszása USB-n keresztül, Wi-Fi nélkül, a ConPianist saját lejátszójával; a lejátszó a bal panelen választható;
- lejátszás más MIDI-eszközre is (pl. loopMIDI → Cantabile), zongora nélkül, General MIDI hangszínekkel;
- a lejátszott hangok megjelennek a virtuális billentyűzeten, amely át is méretezhető;
- több összeomlás és lefagyás javítása (dalszöveget tartalmazó kották, MIDI-feltöltés, hálózati csatlakozás);
- apróbb kényelmi javítások, pl. rákérdezés létező fájl felülírása előtt.

A részletes változáslista a [CHANGELOG.md](CHANGELOG.md) fájlban található.

**Tervezett fejlesztés:** MIDI-fájl mentése a programban beállított hangzással (hangszín, hangerő, tempó, pan, zengetés), a kotta hangzásadatainak frissítésével; később kotta létrehozása MIDI-fájlból.
