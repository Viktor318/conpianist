# ConPianist – fejlesztési terv

Ez a fájl a ConPianist jövőbeli fejlesztéseit gyűjti egy helyre. Új ötlet ide kerül be, verzióemeléskor pedig átnézzük és átrendezzük. Az elkészült változásokat a [CHANGELOG.md](../CHANGELOG.md) tartalmazza.

**Állapot:** 2026. október 6., a 4.7 kiadása után. A verziókra bontás javaslat, a sorrend verzióemeléskor változhat.

**Munkaigény:** K = kicsi (egy alkalom), Kö = közepes (néhány alkalom), N = nagy (több hét, vagy bizonytalan kimenetel).

## Alapelvek

- A program legyen teljes bárkinek, aki letölti: a hangszínek, a stílusok és a dalok **listája** a program része.
- MIDI- és PDF-fájl (dal, kotta) nem kerül a programba és a repóba, csak lista.
- Ami nem kerülhet fel a netre, az a program adatmappájában él (`%APPDATA%\ConPianist`, a Roaming mappában).
- A zongorán még ki nem próbált funkció előbb rövid tesztet kap, és csak utána kerül ütemezésre.
- **Kódátnézés minden minor verzió végén**, a kiadás és a verzióemelés előtt, amikor a funkciók elkészültek és tesztelve vannak: az előző kiadás óta változott fájlok, valamint a kényes közös részek (szálkezelés és zárolás, lejátszási módváltás, kapcsolat, beállítások mentése). Szempontok: hibák (holtpont, versenyhelyzet, felület csak a fő szálon, erőforrások, hibakezelés), szerkezet (ismétlődő vagy túl hosszú kód, elavult megjegyzések, kihasználatlan kód), teljesség (fordítások, CHANGELOG). Az eredmény egy magyar lista súlyosság szerint (javítandó / érdemes átírni / megjegyzés); a kiadásba kerülőkről Viktor dönt, a javítások külön commitba kerülnek. Refaktorálás csak a működés megváltoztatása nélkül, vagy utána teszteléssel.

## Új ötletek (még nincs besorolva)

Ide bármikor beírható egy új ötlet, egy sorban, akár félkészen is. A következő alkalommal kerül a megfelelő csomagba, a munkaigény és a függőségek megjelölésével.

- (üres)

## Folyamatban

**4.8 – Dalválasztó:** elkészült, tesztelés alatt.

### Javítandó hibák

- **A dal tempója nem marad meg újraindítás után** (2026. október 8.): **megoldva, tesztelve (október 8.).** Döntés: a zongora saját dala újraindítás után alapbeállításokkal, az elejéről indul (mintha a Dalválasztó Betöltés gombját nyomták volna meg); USB-n a MIDI-fájl melletti `.conmem` betöltődik; a saját MIDI-fájlok a bezáráskori állapotukkal töltődnek vissza.
- **Lefagy a program, ha üzenetcsere közben indítják a lejátszást** (2026. október 8.): a lejátszás gombját akkor megnyomva, amikor a program és a zongora még üzeneteket vált (például betöltés vagy csatlakozás után). Pontos lépések és a napló kell hozzá.
- **Lefagyás USB-s módban a Felvétel ablak tempójának állítgatása közben** (2026. október 8., 13:26): a napló a tempó (159) elküldése és a zongora visszaigazolása után megszakad, a program nem állt le szabályosan. Lehet, hogy ugyanaz a hiba, mint az előző. Újra előfordult (október 8., 14:55): hálózati és USB-s mód közötti váltás után USB-n a tempó gyors állítgatása közben, a zongorának küldött tempó visszaigazolása után. Közös pont: USB-s lejátszás, csatlakoztatott zongora, gyorsan egymás után küldött tempók. **Valószínű ok megtalálva, javítva, tesztelésre vár:** a saját lejátszó szála a hangok küldése közben (a saját zárát tartva) közvetlenül értesítette az ablakokat, egy ablak pedig ugyanekkor a lejátszó állapotát kérdezte le; a két szál kölcsönösen egymásra várt. Ha a javítás után is előfordul, a Visual Studio hívási verme kell (Break All, Parallel Stacks). Pontos lépések kellenek (melyik ablak, betöltött dal, szólt-e a lejátszás).

## Következő verziók (javaslat)

### 4.8 – Dalok, kotta és fájlkezelés

| Tétel | Leírás | Munka | Megjegyzés |
|---|---|---|---|
| Akkordok a kottában | **Kész** (a következő verzióban): a MusicXML akkordjelölései (`<harmony>`) a kotta felett jelennek meg; a program a Lomse-nak átadás előtt szöveges jelöléssé alakítja őket. Az egy hangra írt, eltolt (`offset`) akkordjelek a helyükre kerülnek (október 10.). | – | Előrehozva (október 8.). Később: az aktuális akkord kijelzése lejátszás közben (például a Kíséret ablak akkordkijelzőjében). |
| Dallista a programban | **Kész** (a következő verzióban): a zongora 403 beépített dala (címmel, szerzővel, kategóriával) a programba épült. | – | A 12 japán változat kimaradt. A Demo Songs 7 dala hangfájl (mp3), ezért az mp3-lejátszásig kimarad. |
| A zongora dalai mappánként | **Kész** (a következő verzióban): a program induláskor létrehozza az adatmappa `Songs` mappájában a Smart Pianist mappaszerkezetét (Score › 50 Popular, PDF Score › 50 Classics, Lesson, Bonus Songs, Music Library, User Songs). A dalfájlok nem a program részei, és soha nem kerülnek a repóba. | – | A kotta (MusicXML) és a MIDI-fájl a dal mappájába kerül, a dal címével, rövid nevével (például Pop01) vagy mindkettővel elnevezve. |
| Dalválasztó | **Kész** (a következő verzióban): külön ablak a felső sáv új ikonjával és a bal panel dalnevére kattintva. Hálózati lejátszásnál a zongora a saját dalát tölti be (fájl nélkül, az 50 Popular dalai is); USB-n és MIDI-eszközön csak a MIDI-fájllal rendelkező dal tölthető be, a többi szürke. Keresés címre és szerzőre, kottaikon, ha van kotta. | – | Az ablak betöltés után nyitva marad, átméretezhető és kis méretre tehető. |
| Saját dalok mappái | **Kész, tesztelésre vár** (október 9.): a mappák létrejönnek, a dalválasztó a bennük lévő MIDI-fájlokat mutatja (almappákkal együtt), a Felvétel ablak a User Songs › Recorded Songs mappába ment (a Mappa gomb is azt nyitja meg). | K | Az mp3-fájlok egyelőre kimaradnak. |
| Alapértelmezett mappa | **Kész, tesztelésre vár** (október 9.): a fájlműveletek alapértelmezett mappája az adatmappa `Songs` mappája; ide tér vissza a program, ha a megjegyzett mappa megszűnt. | – | |
| Mappa fájlműveletenként | **Kész, tesztelésre vár** (október 9.): két csoport, mindkettő a saját utoljára használt mappáját jegyzi meg (újraindítás után is): dalok és kották megnyitása (alapból a `Songs` mappa); zongoraállapot mentése és betöltése (alapból az adatmappa, `%APPDATA%\ConPianist`). A felvételek mindig a Recorded Songs mappába kerülnek. | – | |
| Metronóm a stílus ütemével | **Elvetve / kész más formában** (október 9.): a zongora stílusváltáskor maga állítja a metronóm tempóját és ütemmutatóját a stílus saját (zárójeles) ütemére, és így is szól; a program nem küld ütemet. A Kíséret ablak és az Ütem szűrő újra a zongora szerinti ütemmutatót mutatja (2/4, 3/4, 4/4, 5/4, 6/4). Felvétel közben az ütemmutató-váltás bekerül a fájlba, a legközelebbi ütemvonalra igazítva, csak ha változott. | – | Kipróbálva (október 9.): a Smart Pianistból betöltött stílusoknál is minden érték automatikusan beállt. |
| Rövid útmutató | 10–15 oldalas magyar PDF útmutató képekkel, Markdown forrásból. | Kö | A képernyőképeket Viktor készíti lista alapján. |
| Lomse frissítése | **Kész** (a következő verzióban): a kottamegjelenítő könyvtár 0.27.0-ról 0.30.0-ra frissült. Megjelennek a pedáljelek, a szövegek a kotta saját betűméretével; a tömörített MusicXML és az ismétlőjel-követés külön tétel. | – | A 0.30.0 utáni, kiadatlan javítások is átvéve. A Lomse forrásában öt saját javítás van (lásd CHANGELOG). Ismert szépséghibák Dorico-kottáknál: torlódó, le nem zárt pedálvonalak; többszörös kapcsos zárójel; egymásra csúszó tempófeliratok (javításuk a 4.9-ben). |
| Ugrás a kottában | **Kész:** dupla kattintásra a lejátszás a kattintott ütem elejére ugrik. | – | Ismétlésnél lásd az Ismétlőjelek kezelése tételt. |
| Ismétlőjelek kezelése | **Kész, tesztelésre vár** (október 10., az 5.0-ból előrehozva): a kotta jelzővonala és az A–B jelek a lejátszás sorrendjét követik (ismétlőjel, volta, D.C., D.S., Fine, Coda); dupla kattintásra ismétlődő ütemnél az aktuális körben, különben a legközelebbi előfordulásra ugrik. | – | Ha a dal hossza szerint az ismétlések nincsenek kibontva, marad a közvetlen megfeleltetés. Az Air On the G String (ismétlés és volta) kipróbálva; a D.C.-s, D.S.-es vagy Codás kotta tesztje a 4.9-be került. |
| Kotta megnyitása a dalával | **Kész:** a kottamegnyitó ablakban választott kotta mellől a program az azonos nevű MIDI-fájlt is betölti. | – | |
| Tömörített MusicXML (.mxl) megnyitása | **Kész:** a kottamegnyitó ablak és a MIDI betöltésekor futó kottakeresés az `.mxl` kiterjesztést is felismeri (a Dorico alapból ilyet exportál). | – | A program maga csomagolja ki a fájlt, külső könyvtár nem kell hozzá. |
| Ujjrend megjelenítése | Az ujjrend megjelenítése a kottában, MusicXML-ből. | K–Kö | **Elkészült, tesztelésre vár** (október 9.): a Lomse megjeleníti, csak a program mellé a régi Bravura betűkészlet került (abban nincsenek ujjrend-jelek). A Projucer-projekt utómásolási lépése javítva. Ujjrendes Dorico-kottával is ki kell próbálni. |
| Védelem hibás kottafájl ellen | **Elkészült, tesztelésre vár** (október 9., az 5.0-ból előrehozva): egy hibás MusicXML nem fagyasztja le és nem omlasztja össze a programot. Betöltés előtt XML-ellenőrzés, utána a kotta próbamegjelenítése egy ablak nélküli programpéldányban (`--check-score`); ha az nem végez (Release 5 mp, Debug 15 mp) vagy összeomlik, a kotta nem töltődik be, ablak jelzi, és az ok a naplóba kerül. A hibátlan kották a `CheckedScores.txt`-be kerülnek (útvonal, méret, dátum, programverzió), így csak egyszer ellenőrződnek. | – | Közben kiderült, hogy a nagy kották lassú betöltését (Sound of Silence, kb. 20 mp) a kotta szövegének előfeldolgozása okozta (a szólamszám-javítás és az akkordjelek átalakítása a szöveg hosszával négyzetesen lassult); javítva, a teljes ellenőrzés most kb. 0,3 mp. |
| Kódátnézés | Az első teljes kódátnézés az Alapelvekben leírt módon, a 4.8 kiadása előtt (a 4.7 óta változott fájlok és a kényes közös részek). | Kö | Az október 8-i holtponthiba (a saját lejátszó és az ablakok értesítése) indította. A szálkezelés október 8-án már át lett nézve (a valódi hibák javítva); hátravan az alacsony kockázatúak: a PianoController destruktora állítsa le a saját lejátszót; az ablakok jelentkezzenek le az értesítésekről (RemoveListener); a saját lejátszó a konstruktorban jöjjön létre; a GuiHelper::CallAsync SafePointer-je a fő szálon készüljön; az RTP-kapcsolat a bejövő üzeneteket a saját zárja elengedése után adja tovább; a LiveRecorder mentése ne tartsa a zárat a fájlírás alatt. |

### 4.9 – Akkordmenet-lejátszó

A kíséret előre megírt akkordokkal és átmenetekkel szól, a zongora beépített stílusaival. Kipróbálandó előfeltétele nincs: a zongora elfogadja a második portra küldött akkordhangokat, küldi a stílus ütempozícióját, és a Keverő csatornáin játszott hangok nem zavarják a stílust (tesztelve).

| Tétel | Leírás | Munka | Megjegyzés |
|---|---|---|---|
| Tesztkör a 4.8 kottafunkcióihoz | A 4.8-ból áthozva (október 10.): az ismétléskövetés kipróbálása D.C.-s, D.S.-es vagy Codás kottával, és az eltolt akkordjelek (például a Let It Go 11. és 13. ütemében) megjelenése a programban. Ha minden rendben, az Ismétlőjelek kezelése tétel Kész lesz. | K | |
| Kotta megjelenítési hibái Dorico-exportnál | A 4.8-ból áthozva (október 10.): a le nem zárt pedálvonalak torlódása, a többszörös kapcsos zárójel és az egymásra csúszó tempófeliratok javítása. | Kö | A „folytatódik” típusú jelölések (pedál, oktávjel) kezelése hiányzik a Lomse-ból. A kotta és néhány képernyőkép kell hozzá. |
| Szerkesztőablak | Külön ablak: ütemrács szakaszokra bontva (egy ütem több akkordra osztható, az üres ütem az előzőt tartja); akkordpaletta a beállított hangnem fokaiból, szakaszpaletta (Intro, Main A–D, Ending), automatikus Fill In a szakaszváltás előtt, Break, ismétlésszám, kész dalszerkezet-sablonok, gyors beírás egy sorban. | N | A lehető legegyszerűbb szerkesztés, előre felajánlott elemekkel. Megvalósítás előtt vázlat készül. |
| Automatikus lejátszás | A program a zongora ütemszámlálóját követve küldi az akkordokat (a második porton) és a szakaszváltásokat. | Kö–N | Az akkord küldésének pillanatát hallás után kell behangolni. |
| Léptetett mód | A következő akkordra billentyűvel vagy pedállal lehet lépni. | K | Szabad tempójú játékhoz. |
| Végtelenített mód | A beírt akkordsort leállításig ismétli. | K | Eldöntendő: az egész menetet vagy kijelölt szakaszt is; Intro csak az első körben, Ending leállításkor (javaslat). |
| Élő játék a Keverő csatornáin | Lejátszás közben a külső billentyűzet csak a Keverő csatornáin szól, így a játék nem keveredik a küldött akkordokkal, és a teljes billentyűzet használható. | K | |
| Tárolás | A menet szövegfájlként a Roaming adatmappában (stílus, tempó, hangnem, akkordok). | K | Eldöntendő: önálló fájl, vagy a regisztrációs memóriához kapcsolódik. |

### 4.10 – MIDI áthangszerelő

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
| Kotta transzponálása | A kotta a dallal együtt transzponálódik (csak megjelenítés). | Kö | |
| Részletes kézikönyv | Teljes magyar kézikönyv az 5.0-hoz. | N | |

## Később (elfogadott, nincs ütemezve)

- **Lejátszás és gyakorlás:** Segéd mód és billentyűfények a saját lejátszóval; tempóemelés ismétléses gyakorláshoz; setlist; legutóbbi dalok; további gyorsbillentyűk.
- **Saját arranger**, az akkordmenet-lejátszó (4.9) folytatásaként, két lépésben: (1) az akkordmenet-szerkesztő a program saját lejátszójával, külső `.sty` fájlokhoz; (2) élő akkordfelismerés a split pont alatt. Csak akkor, ha minden más kész.
- **Protokoll feltérképezése** a Smart Pianist többi funkciójának eléréséhez.
- **Más rendszerek:** iOS-változat (iPad), utána Android.
- **Angol útmutató**, ha lesz rá igény.
- **mp3-lejátszás** az Imported Songs hangfájljaihoz és a Demo Songs hét dalához (egyelőre nem kell).
- **A Bonus Songs MIDI-fájljainak megszerzése** (legutolsó lépésként, ha minden más kész; csak saját használatra, sehova nem kerül fel): a zongora a Smart Pianistból megnyitott Bonus Songs dalt `EXTERNAL:/AppSong.mid` néven kapja (kipróbálva, október 9.), útvonallal nem tölthető be, és a zongorából nincs ismert letöltő parancs. Lehetséges utak: (1) a Smart Pianist feltöltésének rögzítése (Wireshark, a Smart Pianist Android-emulátorban a PC-n), a MIDI-fájl kivágása a feltöltésből egy kis segédprogrammal; (2) a Wiresharkkal a dal internetes letöltési címe is kiderülhet (ha a letöltés titkosított HTTPS, akkor csak a kiszolgáló neve látszik). Előtte meg kell nézni a Smart Pianist felhasználási feltételeit.

## Kipróbálandó (előbb teszt kell)

| Tétel | Mit kell megtudni |
|---|---|
| Beépített dal betöltése útvonallal | **Kipróbálva, működik** (2026. október): a zongora az útvonal alapján betölti és lejátssza a saját dalát, a pozíciót is küldi, így a kotta követi. A dalválasztó erre épül. |
| OTS (One Touch Setting) | Működik-e a CSP-170-en, és elérhető-e a programból (PSR-rel tesztelve). |
| Metronóm ütemei | **Kipróbálva** (október 9.): a zongora elfogadja a 2/4–6/4, 3/8, 6/8, 9/8 és 12/8 ütemet, a csengő a helyén szól. Stílusváltáskor a zongora a metronómot a stílus saját (zárójeles) ütemére állítja, a tempóval együtt. |
| Tempó a módváltáskor a zongora paneljén állított tempóval | Hálózatról USB-re váltáskor a program csak a programban állított tempót viszi át arányosan; a zongora paneljén állított tempót nem. A zongora ütem eleji tempójának mindenkori összevetése a fájléval kipróbálva (október 8.) rosszabb eredményt adott (a dal érezhetően gyorsult vagy lassult), ezért visszavonva. Ki kell deríteni, miért tér el a zongora jelzett tempója a fájlétól ugyanannál az ütemnél (ütemszámozás, a jelzés késése). |
| Sebesség hálózati módban | A Sebesség csúszka hálózati lejátszásnál is működhetne, ha a dalhoz van MIDI-fájl (feltöltött saját fájl, vagy beépített dal a mappájában lévő MIDI-fájllal): a program a fájlból tudja a dal tempóját az adott helyen, a szorzót a zongora tárolja. Előbb meg kell nézni, hogy a zongora belső példánya és a fájl tempói ugyanott ugyanazok-e (például a Canon D dur bevezető ütemét a zongora átugorja): ugyanannál az ütemnél USB-n és hálózaton ugyanazt a tempót mutatja-e. Addig hálózaton a Sebesség szürke. |
| Tempóváltás a felvett fájlban | A 4.7-ben kiadva, a visszajelzés még hiányzik. |
| Cím és zeneszerző a kotta tetején | A Lomse a MusicXML-be írt címet és zeneszerzőt (`<credit>`, `<work-title>`) nem mutatja (kipróbálva, október 9.). Az akkordjelölésekhez hasonlóan a program betöltéskor kiolvashatná és a kotta tetejére írhatná őket. Próbaképpen meg kell nézni, hogyan nézne ki, és csak utána eldönteni, hogy bekerüljön-e. |

## Elvetve

- **A zongora pop dalainak kiolvasása MIDI-fájlként:** a zongora a saját lejátszójának hangjait nem küldi ki USB-n, sem a Smart Pianistből, sem a ConPianistből indított lejátszásnál (kipróbálva 2026. október 6-án). Csak a dal adatai érkeznek meg: útvonal, hossz, pozíció, a csatornák hangszíne és keverőállása.
- **Külső stílusfájl (`.sty`) betöltése a zongorába:** a zongora minden nem beépített stílusútvonalat elutasít (kipróbálva).
- **Oldalanként befotózott kották** a népszerű beépített dalokhoz.
- **PDF-kotta megjelenítése a programban** (október 10.): a PDF-ben nincs ütem-információ, így a kotta pontos követése csak sok munkával (kottasorok és ütemvonalak felismerése, kézi javítás) lenne megoldható. Helyette a PDF-kották kottafelismerővel (például Audiveris, a MuseScore PDF-importja) MusicXML-lé alakítva, MuseScore-ban javítva kerülnek a programba.
- **Kottaszerkesztés és kotta generálása MIDI-ből a programban:** a kotta a felvett MIDI-fájlból Doricóval készül. Az elkészült kottákat (MusicXML) a programban Doricóból exportálva kell tesztelni.

## Nyitott kérdések

1. **Mappa fájlműveletenként:** mely műveletek tartoznak össze? Javaslat: dal megnyitása; `.conmem` megnyitása és mentése (egy pár); felvétel mentése; MIDI mentése szerkesztés után.
2. **Akkordmenet-lejátszó:** a szerkesztőablak pontos elrendezése (vázlat alapján), és hogy egy dal önálló fájl legyen-e, vagy a regisztrációs memóriához kapcsolódjon.
