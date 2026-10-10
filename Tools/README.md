# Segédprogramok

## elso_utem_torlese.py – az üres első ütem törlése MIDI-fájlokból

A zongora saját dalaiból mentett Yamaha MIDI-fájlok első üteme egy hang nélküli
„beállító ütem”: GM/XG-reset, hangszínek, hangerő, zengetés, az XF-adatok (cím, szerző).
A zongora saját példánya enélkül kezdődik, ezért a fájl, a kotta és a zongora dala között
egy ütem eltérés lenne (hálózati lejátszásnál a kotta egy ütemmel lemaradna, lejátszási
mód váltásakor a dal rossz ütemre ugrana).

A szkript ezt az ütemet törli:
- a beállító adatok megmaradnak a fájl elején, változatlan sorrendben;
- a beállító ütem ideiglenes tempója és ütemmutatója kimarad, helyükre a dal saját, a 2.
  ütem elején álló tempója és ütemmutatója kerül;
- minden más (hangok, tempóváltások, pedál, akkordjelölések) pontosan egy ütemmel előrébb
  kerül.

Csak azokat a fájlokat alakítja át, amelyek első ütemében nincs hang, de vannak beállító
adatok, és a 2. ütem elején saját tempó és ütemmutató áll (így írja a Yamaha). A már átalakított fájlokat, és amelyek nem ilyenek,
kihagyja, és megírja, miért. Az eredeti fájlokat nem változtatja meg: az újakat egy külön
mappába írja, ugyanazokkal az almappákkal és nevekkel. Átalakítás után visszaolvassa és
ellenőrzi az új fájlt.

### Használat (Windows)

1. Telepítsd a Pythont a [python.org](https://www.python.org/downloads/) oldalról (a
   telepítőben jelöld be: „Add python.exe to PATH”). Más nem kell hozzá.
2. Nyiss egy parancssort (Start menü → „cmd”), és lépj abba a mappába, ahol a MIDI-fájlok
   vannak, például:

   ```
   cd "C:\MIDI\Classics"
   ```

3. Futtasd a szkriptet a repó `Tools` mappájából, paraméterek nélkül:

   ```
   python "C:\...\conpianist\Tools\elso_utem_torlese.py"
   ```

   Az átalakított fájlok a mappán belül a `Converted` mappába kerülnek (`.\Converted`),
   pontosan az eredeti nevükkel, az almappákkal együtt. A `Converted` mappát magát a szkript
   nem nézi át, így többször is futtatható. Más mappa is megadható:

   ```
   python elso_utem_torlese.py "C:\MIDI\eredeti"                         (→ C:\MIDI\eredeti\Converted)
   python elso_utem_torlese.py "C:\MIDI\eredeti" "C:\MIDI\atalakitott"   (saját kimeneti mappa)
   python elso_utem_torlese.py "C:\MIDI\eredeti\dal.mid"                (egyetlen fájl)
   ```

4. A szkript minden fájlról egy sort ír:
   - `MIDI-fájl sikeresen átalakítva, üres kezdő ütem törölve.`
   - `Nem találtam üres ütemet a fájl elején, MIDI-fájl kihagyva.` (ilyen a már átalakított
     fájl is);
   - `Az első ütem üres, de nincsenek benne a Yamaha beállító adatai (hangszínek, hangerő,
     GM/XG-reset), MIDI-fájl kihagyva.` (például egy szándékosan üresen hagyott ütem);
   - `Az első ütem üres, de a második ütem elején nincs saját tempó (vagy ütemmutató, vagy
     egyik sem) (nem a Yamaha beállító üteme), MIDI-fájl kihagyva.`
   - `A fájlban nincs hang, MIDI-fájl kihagyva.`
   - `A konvertálás nem sikerült, hibás a MIDI-fájl (...)`, zárójelben az okkal.

   A végén összesítés áll.
5. Az átalakított fájlokat másold a `Songs` mappában a régiek helyére (előtte készíts
   róluk biztonsági másolatot a `Songs` mappán kívül), és a kottákból is töröld az üres
   első ütemet. Ha a szkriptet közvetlenül a `Songs` valamelyik mappájában futtatod, a
   cserék után töröld a `Converted` mappát, különben a Dalválasztó a saját mappáknál
   (például Music Library) a benne lévő fájlokat is mutatja.

A dalfájlok nem kerülnek a repóba.
