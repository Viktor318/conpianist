# pdf2musicxml

Vektoros zongorakotta-PDF → MusicXML konverter (Python, Windows/macOS/Linux) a ConPianist kottáihoz.
2026. november 11. óta a ConPianist repó része (`Tools/pdf2musicxml`, a v8-as csomagból), a fejlesztése itt folytatódik.
A parancsokat ebből a mappából kell futtatni (`cd Tools\pdf2musicxml`).

Fájlok: `pdf2musicxml.py`, `p2m/__init__.py`, `p2m/extract.py`, `p2m/recognize.py`,
`p2m/musicxml.py`, `p2m/midi.py`, `p2m/compare.py`, `p2m/miditrim.py`, `p2m/catalog.json`, `p2m/templates.json`.

A `.json` fájlt a PDF mellől magától megtalálja; más mappa: `--json MAPPA`, kikapcsolás: `--no-json`.
Önálló MIDI-vágás: `py -m p2m.miditrim *.mid -o kimeneti_mappa` (korábban `midi_trim.py`).

Nem optikai felismerés: a PDF-ben tárolt vektoros alakzatokat (kottafej, szár, gerenda, számjegy stb.)
olvassa ki pontos koordinátákkal, és ezekből építi fel a kottát.

## Telepítés (Windows)

    py -m pip install pymupdf

Opcionális: [Tesseract OCR](https://github.com/UB-Mannheim/tesseract/wiki) – a cím, a szerző és a tempójelzés
kiolvasásához (ezek a PDF-ben körvonalként vannak, nem szövegként). Ha nincs telepítve, a cím a fájlnévből jön,
a többi parancssorból adható meg.

## Használat

    py pdf2musicxml.py kotta.pdf
    py pdf2musicxml.py *.pdf -o kimenet
    py pdf2musicxml.py kotta.pdf --title "Cím" --composer "Szerző" --tempo "Allegretto" --bpm 104

Ellenőrzés MIDI fájllal (ajánlott, ha van):

    py pdf2musicxml.py *.pdf -o kimenet --midi MIDI_mappa

A `--midi` egy fájl vagy egy mappa lehet; mappánál név szerint párosítja a PDF-eket a `.mid` fájlokkal.
A program a Yamaha-dalfájlok kottahű „guide” csatornáit (15: jobb kéz, 16: bal kéz) veti össze hangról hangra
a felismert ütemekkel. Az elején lévő üres ütemeket és a felütést magától kiegyenlíti, az ismétléseket és a
D.C./D.S. ugrásokat követi. Az egyértelmű oktáveltérést (8va) kijavítja és jelzi a kottában; minden más
eltérést ütemszámmal felsorol a jelentés „MIDI összevetés” részében. A szerző nevét is a MIDI-ből veszi.

A kezdő tempójelzés helye: a szöveg és a metronómjelzés `default-x` értéket kap (a PDF-ből számolva:
a szöveg bal széle az első hanghoz/szünethez képest, tizedvonalközben), a szöveg pedig `default-y="40"` értéket,
ha metronómjelzés is van. A vízszintes hely felülírható: `--tempo-x -105`.

Szöveges utasítások (rit., a tempo stb.): a PDF szerint a sor fölé vagy alá kerülnek (`placement`); a sor fölöttiek
`default-y` értéket is kapnak (a szöveg alja a felső vonaltól, tizedvonalközben), hogy ne csússzanak az ujjrendre.
A fermata iránya mindig ki van írva: `type="upright"` vagy `type="inverted"`.

Fájlnevek: a kimenet neve a PDF nevével egyezik, ékezetekkel és írásjelekkel együtt
(`Bartók – Román népi táncok.pdf` → `Bartók – Román népi táncok.musicxml`). A MIDI-párosítás ékezet-,
szóköz- és írásjel-független, a macOS-en létrejött (szétbontott ékezetű) neveket is felismeri.

Cím és MIDI-előkészítés (a ConPianisthoz így ajánlott):

    py pdf2musicxml.py *.pdf -o kimenet --midi MIDI_mappa --midi-out kimenet

- A cím a fájlnévből jön, a `ClassicNN` előtag nélkül (`Classic09 Polonaise op.53 Héroïque.pdf` →
  „Polonaise op.53 Héroïque”). A PDF-ből olvasott cím: `--title-from-pdf`.
- `--midi-out`: a MIDI-ket ide írja, eredeti névvel. Ha a MIDI a kottához képest teljes üres ütem(ek)kel
  később kezdődik, azokat levágja; a SysEx és minden más üzenet megmarad (a levágott részből a 0. időpontra
  kerül). Felütésnél a felütés ütemének csonka szünete megmarad, a MIDI első üteme mindig teljes.
  A többi MIDI bájtra azonos másolat.

Minden PDF mellé két fájl készül:

- `kotta.musicxml` – az eredmény (MuseScore, Dorico, Lomse)
- `kotta.report.txt` – ellenőrző jelentés: ütemhossz-eltérések, párosítatlan jelek, fel nem használt alakzatok

**A jelentés a lényeg:** ha 0 figyelmeztetés és 0 fel nem használt jel van, akkor a PDF minden alakzata
bekerült a kottába, és minden ütem hossza stimmel. Ami a jelentésben szerepel, azt kézzel kell megnézni.

## Amit felismer

Kulcsok (soronként, kulcsváltással), előjegyzés, ütemmutató (számok és C), hangok és akkordok (szekund-akkordok is),
pontozás, gerendák, zászlók, szünetek (egésztől tizenhatodig), módosítójelek, két szólam soronként,
ujjrend (fent/lent, akkordra halmozva is), dinamika (ppp–ff, sf), crescendo-villák,
pedál (Ped. és feloldó csillag), átkötések és kötőívek (rendszerhatáron át is),
staccato, staccatissimo, tenuto, hangsúly, marcato, fermata, arpeggio, ismétlőjelek, 1./2. befejezés (volta),
duola és triola, felütés, coda-jel, sor- és oldaltörések.
OCR-rel (Tesseract kell hozzá): cím, szerző, tempó, szöveges utasítások (dolce, a tempo, poco rit., Fine, D.C. stb.).
Az OCR-rel olvasott szövegeket a jelentés külön felsorolja, ezeket érdemes átolvasni.

## Ismert hiányok

- a képként beágyazott kották (az Original Compositions nagy része): ezeket nem kezeli
- zárójeles hangok, zárójeles módosítójelek zárójele, zárójeles alternatív ujjrend, „(× 2)” megjegyzések
- ujjcsere jelölés („1-3”, ívvel összekötött ujjszámok)
- 8va vonalak: MIDI nélkül az oktávváltás nem érvényesül a hangmagasságban (MIDI-vel javítja)
- segno, D.S./D.C. csak szövegként, ugrásként nem
- előkék, 32-edek, triolán/duolán kívüli n-olák
- ütem közbeni előjegyzés-váltás (rendszer elején kezeli)
- más kiadó/más kottagrafika: a `p2m/catalog.json` alak-szignatúrái ehhez a sorozathoz készültek;
  új jelet a jelentés „ismeretlen alak” soraiból lehet felvenni

## Felépítés

- `p2m/extract.py` – látható vektoros alakzatok kiolvasása (rétegek, vágóterületek kezelése)
- `p2m/recognize.py` – kottasorok, ütemek, glifek, hangok, ritmus, szólamok, jelek
- `p2m/musicxml.py` – hangmagasság-számítás, MusicXML 3.1 író
- `p2m/midi.py`, `p2m/compare.py` – MIDI olvasó és összevetés (külső csomag nélkül)
- `p2m/catalog.json` – alak-szignatúra → jel neve
