# pdf2musicxml

Vektoros zongorakotta-PDF → MusicXML konverter (Python, Windows/macOS/Linux) a ConPianist kottáihoz.
2026. november 11. óta a ConPianist repó része (`Tools/pdf2musicxml`, a v8-as csomagból), a fejlesztése itt folytatódik.
A parancsokat ebből a mappából kell futtatni (`cd Tools\pdf2musicxml`).

Fájlok: `pdf2musicxml.py`, `p2m/__init__.py`, `p2m/extract.py`, `p2m/recognize.py`, `p2m/finale.py`,
`p2m/musicxml.py`, `p2m/midi.py`, `p2m/compare.py`, `p2m/miditrim.py`, `p2m/catalog.json`, `p2m/templates.json`.

A `.json` fájlt a PDF mellől magától megtalálja; más mappa: `--json MAPPA`, kikapcsolás: `--no-json`.
Önálló MIDI-vágás: `py -m p2m.miditrim *.mid -o kimeneti_mappa` (korábban `midi_trim.py`).

Nem optikai felismerés: a PDF-ben tárolt vektoros alakzatokat (kottafej, szár, gerenda, számjegy stb.)
olvassa ki pontos koordinátákkal, és ezekből építi fel a kottát.

## Telepítés (Windows)

    py -m pip install pymupdf fonttools

A `fonttools` a Finale-ből készült PDF-ekhez kell (lásd lent), a többihez nem.

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
Ha a PDF mellett ott van az azonos nevű `.mid` fájl, azt `--midi` nélkül is megtalálja (ahogy a `.json` fájlt).
A program a Yamaha-dalfájlok kottahű „guide” csatornáit (15: jobb kéz, 16: bal kéz) veti össze hangról hangra
a felismert ütemekkel. Az elején lévő üres ütemeket és a felütést magától kiegyenlíti, az ismétléseket és a
D.C./D.S. ugrásokat követi. Az egyértelmű oktáveltérést (8va) kijavítja és jelzi a kottában; minden más
eltérést ütemszámmal felsorol a jelentés „MIDI összevetés” részében. A szerző nevét is a MIDI-ből veszi. Ha egy főhang előtt előke áll, és a MIDI-ben a főhang legfeljebb előkénként egy tizenhatoddal (összesen legfeljebb egy nyolcaddal) később szól, az nem eltérés: az előkét ütésre játsszák. Az ilyen hangok számát a jelentés külön sorban adja meg.

A kezdő tempójelzés helye: a szöveg és a metronómjelzés a kottasor bal szélénél kezdődik, a kulcs fölött.
A `default-x` értéke a kottasor bal széle az ütem első hangoszlopához képest, tizedvonalközben, a PDF-ből számolva
(így keskeny ablakban sem csúszik rá az első hangok ujjrendjére). A metronómjelzés `default-y="35"`, fölötte a
szöveg `default-y="60"` értéket kap; metronómjelzés nélkül a szöveg `default-y="40"`. A vízszintes hely
felülírható: `--tempo-x -105`.

Az átkötés oldala (`orientation`) is a PDF szerinti, így a kéthangos akkord két íve felül és alul fut.
A crescendo- és decrescendo-villák kezdete és vége `<offset>` értéket kap, ha nem hangra esik (például egy hosszú
hang közepén kezdődik). Az időpont a legközelebbi hangkezdethez igazodik: a hangok közé eső időpontnak a Lomse saját
oszlopot nyitna, és a körülötte lévő hangokat egymásra rajzolná.

Ívek: átkötés csak egy szólam két szomszédos, azonos magasságú hangja között lehet; ha közöttük más hang is van,
az ív kötőív. A kötőív oldala (a hangok fölött vagy alatt) a PDF szerint kerül a kottába (`placement`).
A kötőívek sorszáma (`number`) a fájl sorrendjéhez igazodik: egy sorszám csak akkor használható újra, ha az íve a
fájlban is korábban véget ért (különben a másik sor egyszerre végződő ívével ütközne, és a Lomse rossz hangokat kötne össze).

Szólamnév: csak akkor látszik, ha a PDF-ben is szerepel (négykezesnél „Piano I”, „Piano II”); különben
`print-object="no"` jelöli, így a Lomse nem írja ki.

Szöveges utasítások (rit., a tempo, dolce, cresc. stb.): a PDF szerint a sor fölé vagy alá kerülnek (`placement`), és
mind `default-y` értéket kap (a szöveg alja a sor felső vonalától, tizedvonalközben), mert a Lomse a szöveget csak
ebből teszi a sorok közé. A sor alatti szövegek a legközelebbi hanghoz kötődnek, a PDF szerinti eltolással
(`relative-x`); az alsó sor alattiak (Fine, D.C. al Fine) az alsó sorhoz igazodnak.
Ha ugyanarra a hangra dinamikai jel is kerül (p dolce), a szöveg a jel után kezdődik. A nem rövidített szavak
(dolce, legato, cantabile stb.) után az OCR által olvasott pont lekerül.
A fermata iránya mindig ki van írva: `type="upright"` vagy `type="inverted"`.

Kulcsváltás az ütem utolsó hangja után: a következő ütem elejére kerül, a Lomse az ütemvonal elé rajzolja.

Sorvégen megtört villa: ha a villa két vonala nem ér össze, az a megtört villa egyik darabja; a két darabból egy villa
lesz.

Visszaugrások: a D.C. és a Fine felirat mellé lejátszási utasítás is kerül (`<sound dacapo="yes"/>`, `<sound fine="yes"/>`),
a szakasz végére (az első ismétlőjelhez, kettős vagy záró ütemvonalhoz), mert a kottamegjelenítő a feliratot magát nem követi.
Ha egy ismétlőjelhez a szakaszában nincs kiírt kezdő ismétlőjel, a szakasz eleje (az előző ismétlőjel, kettős vagy záró
ütemvonal utáni ütem) `<sound forward-repeat="yes"/>` utasítást kap; ebből a kottában semmi nem látszik.
A D.C. és a Fine felirata ahhoz az ütemhez tartozik, ahol a szöveg véget ér. A D.S. és a Coda még nincs meg (a segno
jelet a script nem ismeri fel).

Pedáljelek: mindig a fölöttük lévő sorhoz tartoznak, akkor is, ha a mély hangok miatt lejjebb kerültek.

Két szólam egy sorban: az akkord mellé jobbra tolt (szekund vagy terc távolságra lévő) hang az akkorddal együtt szól.

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

## Finale-ből készült PDF-ek (Lesson)

A zongora Lesson dalainak kottái (Beyer, Burgmüller, Czerny 100, Czerny 30, Hanon; 303 kotta) Finale-ből
készültek: a kottajelek bennük nem körvonalak, hanem egy kottabetűkészlet (Kousaku, a Maestro kiosztásával)
karakterei, a dinamikai jelek a Chaconne betűkészleté. A szkript ezt magától észreveszi, és ilyenkor a jeleket
a karakterkódból olvassa ki (`p2m/finale.py`), nem alakmintákból; a jelek pontos méretét a PDF-be ágyazott
betűkészletből veszi. A vonalak (kottasor, szár, gerenda, ütemvonal, ív) itt is rajzolt elemek.

Karakterből felismert jelek: kottafejek, kulcsok, szünetek (egésztől 32-edig), módosítójelek (kettős kereszt
és kettős bé is), zászlók, pontok, ujjrend, ütemmutató, Ped. és feloldás, hangsúly, marcato, tenuto,
staccatissimo, fermata, trilla, triolaszám; dinamika: p, pp, ppp, mp, mf, f, ff, sf, sfz, fp.
A szám nélküli triolákat (az első ütem után „simile”) szólamonként, hatos gerendacsoportban is felismeri.
Az ismeretlen karaktereket a jelentés felsorolja.

Állapot (2026. november 13.): a 303 kottából 171-ben minden hang egyezik a MIDI-vel,
a hangok 90,5%-a egyezik összesen. Még nincs bekötve: a szöveges feliratok (a PDF-ben sima szövegként
megvannak, OCR nem kell hozzájuk), a szögletes vonallal jelölt pedál, a 8va karakter (a szaggatott vonalat
kezeli), trillavonal, mordent, paránytrilla, zárójelek. Ismert hibák: két szólam egy sorban (a ritmus
szétesik), átkötések egy része, hosszú ívek, ismétlések sorrendje néhány kottában.

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
- `p2m/finale.py` – Finale-ből készült PDF: a kottabetűkészlet karakterei → jelek
- `p2m/musicxml.py` – hangmagasság-számítás, MusicXML 3.1 író
- `p2m/midi.py`, `p2m/compare.py` – MIDI olvasó és összevetés (külső csomag nélkül)
- `p2m/catalog.json` – alak-szignatúra → jel neve
