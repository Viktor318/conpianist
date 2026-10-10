# Segédprogramok

## elso_utem_torlese.py – az üres első ütem törlése MIDI-fájlokból

A zongora saját dalaiból mentett Yamaha MIDI-fájlok első üteme egy hang nélküli
„beállító ütem”: GM/XG-reset, hangszínek, hangerő, zengetés, az XF-adatok (cím, szerző).
A zongora saját példánya enélkül kezdődik, ezért a fájl, a kotta és a zongora dala között
egy ütem eltérés lenne (hálózati lejátszásnál a kotta egy ütemmel lemaradna, lejátszási
mód váltásakor a dal rossz ütemre ugrana).

A szkript ezt az ütemet törli:
- a beállító adatok megmaradnak a fájl elején, változatlan sorrendben;
- a beállító ütem ideiglenes tempója és ütemmutatója csak akkor marad ki, ha a dalnak van
  saját tempója, illetve ütemmutatója a 2. ütem elején: ilyenkor a dal sajátja kerül a
  helyükre. Ha nincs, az ideiglenes megmarad a fájl elején, és a lista ezt jelzi;
- minden más (hangok, tempóváltások, pedál, akkordjelölések) pontosan egy ütemmel előrébb
  kerül.

Csak azokat a fájlokat alakítja át, amelyek első ütemében nincs hang, de vannak beállító
adatok (így írja a Yamaha). A már átalakított fájlokat, és amelyek nem ilyenek,
kihagyja, és megírja, miért. Az eredeti fájlokat nem változtatja meg: az újakat egy külön
mappába írja, ugyanazokkal az almappákkal és nevekkel. Átalakítás után visszaolvassa és
ellenőrzi az új fájlt.

### Használat (Windows)

1. Telepítsd a Pythont a [python.org](https://www.python.org/downloads/) oldalról (a
   telepítőben jelöld be: „Add python.exe to PATH”). Más nem kell hozzá.
2. Nyiss egy parancssort (Start menü → „cmd”), és lépj a repó `Tools` mappájába, például:

   ```
   cd C:\...\conpianist\Tools
   ```

3. Futtasd a bemeneti és a kimeneti mappával (szóközös útvonal idézőjelben):

   ```
   python elso_utem_torlese.py "C:\MIDI\eredeti" "C:\MIDI\atalakitott"
   ```

   Egyetlen fájl is megadható a bemeneti mappa helyett.
4. A végén a lista mutatja, melyik fájl készült el (ÁTALAKÍTVA; zárójelben, ha az ideiglenes
   tempó vagy ütemmutató megmaradt), melyik maradt ki és miért (KIHAGYVA), és melyiket nem
   lehetett beolvasni (HIBA).
5. Az átalakított fájlokat másold a `Songs` mappában a régiek helyére (előtte készíts
   róluk biztonsági másolatot a `Songs` mappán kívül), és a kottákból is töröld az üres
   első ütemet.

A dalfájlok nem kerülnek a repóba.
