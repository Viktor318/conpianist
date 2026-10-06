# ConPianist – fejlesztési terv

Ez a fájl a ConPianist jövőbeli fejlesztéseit gyűjti egy helyre. Új ötlet ide kerül be, verzióemeléskor pedig átnézzük és átrendezzük. Az elkészült változásokat a [CHANGELOG.md](../CHANGELOG.md) tartalmazza.

**Állapot:** 2026. október 6., a 4.7 kiadása után. A verziókra bontás javaslat, a sorrend verzióemeléskor változhat.

**Munkaigény:** K = kicsi (egy alkalom), Kö = közepes (néhány alkalom), N = nagy (több hét, vagy bizonytalan kimenetel).

## Alapelvek

- A program legyen teljes bárkinek, aki letölti: a hangszínek, a stílusok és a dalok **listája** a program része.
- MIDI- és PDF-fájl (dal, kotta) nem kerül a programba és a repóba, csak lista.
- Ami nem kerülhet fel a netre, az a program adatmappájában él (`%APPDATA%\ConPianist`, a Roaming mappában).
- A zongorán még ki nem próbált funkció előbb rövid tesztet kap, és csak utána kerül ütemezésre.

## Új ötletek (még nincs besorolva)

Ide bármikor beírható egy új ötlet, egy sorban, akár félkészen is. A következő alkalommal kerül a megfelelő csomagba, a munkaigény és a függőségek megjelölésével.

- (üres)

## Folyamatban

Jelenleg nincs megkezdett fejlesztés.

## Következő verziók (javaslat)

### 4.8 – Dalok és fájlkezelés

| Tétel | Leírás | Munka | Megjegyzés |
|---|---|---|---|
| Dallista a programban | A zongora beépített dalainak listája a programba épül, a stíluslista mintájára: 50 Popular (Pop, Standard, Folk, Holiday & Events, Children's Music), 50 Classics (Arrangements, Duets, Original Compositions), Lesson (Beyer, Burgmüller, Czerny 100, Czerny 30, Hanon). | K | A lista megvan (403 dal a nemzetközi változatban; a 12 japán változat kimarad). A Demo Songs 7 dala hangfájl (mp3) az alkalmazás csomagjában, ezért az mp3-lejátszásig kimarad. A Bonus Songs az alkalmazásban havonta bővül. |
| A zongora dalai mappánként | A dalok ugyanolyan mappaszerkezetben jelennek meg, mint a Smart Pianistben; a fájlok helye az adatmappa, amelyben a program létrehozza a mappákat. A dalfájlok nem a program részei, és soha nem kerülnek a repóba. | Kö | Az 50 Classics és a Lesson fájljai (353 dal) megvannak; az 50 Popular dalai nem olvashatók ki a zongorából, ezek csak a zongora saját lejátszójával szólhatnak (lásd a kipróbálandó tételt), a mappáik üresek maradnak. |
| Dalválasztó | A beépített dalok kiválasztása és betöltése a programból. Az a dal, amelynek a fájlja nincs meg az adatmappában, szürkén látszik, és nem tölthető be. | Kö | A két előző tételre épül. |
| Saját dalok mappái | A Smart Pianist gyűjtőinek megfelelő mappák az adatmappában: Music Library (megvásárolt fájlok), Imported Songs (saját MIDI- és mp3-fájlok), Recorded Songs (saját felvételek), Bonus Songs (az alkalmazás havonta bővülő, PDF-kottás dalai; a fájlokat Viktor tölti le az alkalmazással és másolja be). A dalválasztó a mappákban lévő MIDI-fájlokat mutatja; ezekhez nincs beépített lista. A Felvétel ablak a Recorded Songs mappába ment. | K | Az mp3-fájlok egyelőre kimaradnak. |
| Alapértelmezett mappa | Induláskor minden fájlművelet alapértelmezett mappája az adatmappa, mert az biztosan létezik. | K | |
| Mappa fájlműveletenként | Minden megnyitás és mentés megjegyzi a saját utoljára használt mappáját; az összetartozó megnyitás–mentés pár egy mappát használ. | K | Most egyetlen közös mappa van. |
| Metronóm a stílus ütemével | Stílus betöltésekor a metronóm átveszi a stílus név szerinti ütemmutatóját; fordítva nem hat. | K | A Felvétel ablak ütemlistája már minden szükséges ütemet tartalmaz. Zongorás próba kell (9/8, 12/8). |
| Rövid útmutató | 10–15 oldalas magyar PDF útmutató képekkel, Markdown forrásból. | Kö | A képernyőképeket Viktor készíti lista alapján. |

### 4.9 – MIDI áthangszerelő

Egyetlen funkció, amely a korábban külön tervezett „betöltött MIDI-fájl szerkesztése” és a két MIDI-segédprogram helyébe lép. Csak a fájl **kezdeti** beállításairól szól: a hangjegyekhez nem nyúl.

| Tétel | Leírás | Munka | Megjegyzés |
|---|---|---|---|
| Kezdeti beállítások beolvasása | A Keverő a betöltött MIDI-fájl kezdeti beállításait mutatja csatornánként (hangszín, hangerő, tér, zengetés, kórus). | Kö | A Keverő nagyrészt már ezt mutatja; a kórus új. |
| Módosítás és pótlás | Minden kezdeti, nem hangjegy-adat kicserélhető, a hiányzók hozzátehetők: a csatornák beállításai, a tempó, az ütemmutató, a hangnem és a szöveges adatok (cím, szerző, szerzői jog, sávnevek). | Kö | A tempó, az ütem, a hangnem és a szövegek külön „Fájl adatai” ablakban szerkeszthetők. Szöveges adat minden, amit a MIDI-szabvány megenged; dalszöveg egyelőre nem kell. |
| Mentés MIDI-fájlba | A program a megadott beállításokkal menti a fájlt; a hangjegyek változatlanok. Új nevet ajánl fel (`…_new.mid`), de az eredeti is felülírható. Ha a fájl közepén is van hangszín-, hangerő- vagy tempóváltás, az megmarad: csak a kezdeti érték változik. | Kö | A felvevő fájlírója megvan. |
| Régi felvételek rendbetétele | Ugyanezzel a funkcióval pótolhatók a régi (akár a zongorával közvetlenül készült) felvételek hiányzó beállításai. | – | Nem külön munka, az előző három tétel használata. |
| Meglévő MusicXML frissítése | A MIDI mentésekor a hozzá tartozó MusicXML hangzásadatai is frissülnek. | Kö | Később is hozzátehető. |
| MIDI In 2 átengedő mód | A program a MIDI In 2-n érkező hangokat továbbadja a zongorának, az órajelet, a start és a stop üzenetet kiszűri, a szakaszváltást továbbítja (PSR kísérete a zongorán). | Kö | Független a többi tételtől, előbbre is hozható. |

### 5.0 – Kotta

| Tétel | Leírás | Munka | Megjegyzés |
|---|---|---|---|
| Lomse frissítése | A kottamegjelenítő könyvtár frissítése 0.27.0-ról (2020) a legújabb, 0.30.0 verzióra (2022). Amit hoz: ujjrend és pedáljelek megjelenítése, minden ütemvonalstílus, helyes volta-zárójelek sortörésnél, tömörített (.mxl) és UTF-16 kódolású MusicXML beolvasása, szebb kottakép (előkék, kötőívek, gerendák, térközök), új nézetek. | N | Kockázatos: a program saját módosításokkal használja (dalszöveges összeomlás, FreeType-típusjavítás, Dorico-kották javításai), ezeket egyenként ellenőrizni kell, és át kell vinni, ha az új verzió nem oldja meg. Megváltozott a rajzolópuffer átadása és a naplózó neve. A kottás tételek előtt kell megcsinálni, külön ágon, a tesztkották előtte–utána összevetésével. |
| Tömörített MusicXML (.mxl) megnyitása | A kottamegnyitó ablak és a MIDI betöltésekor futó kottakeresés az `.mxl` kiterjesztést is felismeri (a Dorico alapból ilyet exportál). | K | A Lomse frissítésére épül, azzal együtt készül. |
| Ismétlőjelek kezelése | A kotta kurzora követi az ismétléseket és a voltákat (MusicXML-ből). | Kö–N | |
| Ujjrend megjelenítése | Az ujjrend megjelenítése a kottában, MusicXML-ből. | Kö | A Lomse 0.29-től megjeleníti az ujjrendet; a frissítés után kell kipróbálni, mennyi munka marad. |
| PDF megjelenítő | PDF-kotta megjelenítése; a dal betöltésekor a program előbb MusicXML-t, utána PDF-et keres, menüből választható. | N | Új könyvtár kell hozzá; a pozíciókövetés PDF-nél nem lehetséges. |
| Kotta transzponálása | A kotta a dallal együtt transzponálódik (csak megjelenítés). | Kö | |
| Részletes kézikönyv | Teljes magyar kézikönyv az 5.0-hoz. | N | |

## Később (elfogadott, nincs ütemezve)

- **Lejátszás és gyakorlás:** Segéd mód és billentyűfények a saját lejátszóval; tempóemelés ismétléses gyakorláshoz; setlist; legutóbbi dalok; további gyorsbillentyűk.
- **Akkordmenet-lejátszó és saját arranger**, három lépésben: (1) előre beírt akkordmenet a zongora beépített stílusaival, szakaszváltásokkal, ütemenként több akkorddal; (2) ugyanez a program saját lejátszójával, külső `.sty` fájlokhoz; (3) élő akkordfelismerés a split pont alatt. Csak akkor, ha minden más kész. Az első lépés részletei: a kíséret előre megírt akkordokkal és átmenetekkel (Intro, Main A–D, Fill In, Break, Ending) szól; a menet külön ablakban szerkeszthető, a lehető legegyszerűbben, előre felajánlott elemekkel (akkordok, szakaszok, átmenetek). Elfogadott irány: ütemrács szakaszokra bontva (egy ütem több akkordra osztható, az üres ütem az előzőt tartja); akkordpaletta a beállított hangnem fokaiból, szakaszpaletta, automatikus Fill In a szakaszváltás előtt, ismétlésszám, kész dalszerkezet-sablonok, gyors beírás egy sorban; a menet szövegfájlként a Roaming adatmappában. Lejátszás: elsősorban automatikus mód (a zongora ütemszámlálóját követve, az akkordokat a második porton küldve), mellette léptetett mód; végtelenített mód is kell, amely a beírt akkordsort leállításig ismétli. A szerkesztőablakról megvalósítás előtt vázlat készül.
- **Protokoll feltérképezése** a Smart Pianist többi funkciójának eléréséhez.
- **Más rendszerek:** iOS-változat (iPad), utána Android.
- **Angol útmutató**, ha lesz rá igény.
- **mp3-lejátszás** az Imported Songs hangfájljaihoz és a Demo Songs hét dalához (egyelőre nem kell).

## Kipróbálandó (előbb teszt kell)

| Tétel | Mit kell megtudni |
|---|---|
| Beépített dal betöltése útvonallal | Be tudja-e tölteni a ConPianist a zongora beépített dalát (például 50 Popular) a zongorán az ismert útvonal alapján, Smart Pianist nélkül. Ha igen, ezek a dalok fájl nélkül is lejátszhatók a zongora saját lejátszójával. |
| OTS (One Touch Setting) | Működik-e a CSP-170-en, és elérhető-e a programból (PSR-rel tesztelve). |
| Metronóm ütemei | Elfogadja-e a zongora a 9/8-at és a 12/8-at, és hol az ütésszám felső határa. |
| Tempóváltás a felvett fájlban | A 4.7-ben kiadva, a visszajelzés még hiányzik. |

## Elvetve

- **A zongora pop dalainak kiolvasása MIDI-fájlként:** a zongora a saját lejátszójának hangjait nem küldi ki USB-n, sem a Smart Pianistből, sem a ConPianistből indított lejátszásnál (kipróbálva 2026. október 6-án). Csak a dal adatai érkeznek meg: útvonal, hossz, pozíció, a csatornák hangszíne és keverőállása.
- **Külső stílusfájl (`.sty`) betöltése a zongorába:** a zongora minden nem beépített stílusútvonalat elutasít (kipróbálva).
- **Oldalanként befotózott kották** a népszerű beépített dalokhoz.
- **Kottaszerkesztés és kotta generálása MIDI-ből a programban:** a kotta a felvett MIDI-fájlból Doricóval készül. Az elkészült kottákat (MusicXML) a programban Doricóból exportálva kell tesztelni.

## Nyitott kérdések

1. **Mappa fájlműveletenként:** mely műveletek tartoznak össze? Javaslat: dal megnyitása; `.conmem` megnyitása és mentése (egy pár); felvétel mentése; MIDI mentése szerkesztés után.
2. **PDF-kotta:** elég a megjelenítés lapozással, vagy kell hozzá automatikus lapozás a lejátszás közben?
3. **Akkordmenet-lejátszó:** a szerkesztőablak pontos elrendezése (vázlat alapján), és hogy egy dal önálló fájl legyen-e, vagy a regisztrációs memóriához kapcsolódjon.
