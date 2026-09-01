# ConPianist

> Ez ennek a projektnek **[Viktor318](https://github.com/Viktor318/conpianist) általi saját fork-ja**, saját célú, magánhasználatú továbbfejlesztésre. Az eredeti projekt: [hugbug/conpianist](https://github.com/hugbug/conpianist). Az angol nyelvű eredeti README a [README.en.md](README.en.md) fájlban található.

**ConPianist**, vagyis **Connected Pianist**, egy alkalmazás a Yamaha CSP (Clavinova Smart Piano) sorozatú digitális zongorák vezérlésére. Alternatívája a Yamaha saját "Smart Pianist" alkalmazásának. A Smart Pianist-tel ellentétben, ami iOS-en és Androidon fut, a Connected Pianist asztali rendszerekre készült — macOS, Windows és Linux alá. iPad-en is működik.

## Funkciók

A program egyelőre nem teljes értékű helyettesítője a hivatalos alkalmazásnak. Ennek ellenére már most is tud:
- csatlakozni a zongorához hálózaton vagy kábelen keresztül;
- állapotvesztés nélkül csatlakozni/újracsatlakozni: a program indításkor beolvassa a zongora teljes állapotát, és megjeleníti a felületen;
- MIDI-fájlokat feltölteni a zongorára hálózaton keresztül (USB-kábelen keresztül egyelőre nem);
- a feltöltött MIDI-fájlok lejátszását vezérelni: indítás, szünet, pozíció;
- a "stream lights" (billentyű-kivilágítás) vezérlése: ki, be, lassú, gyors;
- a guide (vezetett gyakorlás) mód vezérlése: ki, be, mód kiválasztása;
- részek kiválasztása: kíséret, jobb kéz, bal kéz;
- kiválasztott szakasz lejátszása ismétlődő (loop) módban;
- hangerő, tempó, transzponálás beállítása;
- hangszínek kiválasztása (mind a hétszáznál is több) a fő, bal kezes és réteg (layer) hangokhoz;
- oktáveltolás és osztáspont (fő/bal) beállítása;
- keverő az összes klasszikus funkcióval: MIDI-csatornák ki/be kapcsolása, hangerő, pan, visszhang, visszhang-effekt;
- extra funkciók a keverőben: rész-kiválasztás MIDI-csatornánként, hangszín kiválasztása közvetlenül a MIDI-csatornákból;
- balansz beállítás a fő/bal/réteg/dal/mikrofon/auxin csatornákra: hangerő, pan, visszhang, visszhang-effekt;
- kották megjelenítése a lejátszási pozícióval szinkronban: a kottákat külön MusicXML-fájlban kell megadni (közvetlenül a MIDI-fájlból nem jeleníthető meg kotta);
- a regisztrációs memória (beállítások) MIDI-dalokhoz rendelése.

## Képernyőképek

![Lejátszás kottával](https://user-images.githubusercontent.com/3368402/79354560-d8c86b00-7f3c-11ea-8710-9c948982de3a.png)

![Hangszínválasztás](https://user-images.githubusercontent.com/3368402/79360975-f699ce00-7f44-11ea-8aa1-803be03533d7.png)

![Keverő](https://user-images.githubusercontent.com/3368402/79354913-4ffdff00-7f3d-11ea-8bf9-0ecda2180204.png)

## Köszönetnyilvánítás

A ConPianist forráskódja a következő könyvtárakat tartalmazza:
- [Lomse](https://github.com/lenmus/lomse) a kották megjelenítéséhez;
- [Arduino AppleMIDI Library](https://github.com/lathoub/Arduino-AppleMIDI-Library) a zongorával való hálózati kommunikációhoz.

## Erről a fork-ról

Ez a változat saját, magáncélú felhasználásra készül egy Yamaha CSP-170 zongorához, modern fejlesztői eszközökkel (Visual Studio, friss JUCE, vcpkg) újra buildelve és néhány hibajavítással kiegészítve. A részletekért lásd a [CHANGELOG.md](CHANGELOG.md) fájlt.
