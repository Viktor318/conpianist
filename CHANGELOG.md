# Changelog

Ez a fájl a ConPianist változásait dokumentálja. Az 1.0–3.0 verziók az eredeti [hugbug/conpianist](https://github.com/hugbug/conpianist) projekt kiadásai (lásd az [eredeti release-eket](https://github.com/hugbug/conpianist/releases)); az ez utáni bejegyzések ennek a fork-nak ([Viktor318/conpianist](https://github.com/Viktor318/conpianist)) a saját, magáncélú fejlesztései.

## 4.3 (fork) – 2026. szeptember 26.

Fő újdonságok: lejátszás MIDI-eszközre (pl. loopMIDI → Cantabile) zongora nélkül is, élő játék több csatornán a virtuális billentyűzettel és a MIDI In 2-vel, a lejátszott hangok a virtuális billentyűzeten, indulás a bezáráskori állapotban.

### Új
- **Lejátszás MIDI-eszközre (pl. loopMIDI → Cantabile), zongora nélkül is.** A Kapcsolat beállításaiban két új, mindig állítható sor van: **MIDI Out** (a MIDI-eszköz, amelyen a lejátszás szól, ha a zongora nem érhető el) és **MIDI In 2** (másodlagos bemenet, pl. MIDI-billentyűzet). A bal panel Lejátszás részében új, harmadik lehetőség: **„MIDI-eszközön keresztül”**. Ha a zongora nem érhető el (USB-nél 3, hálózatnál 15 másodperc után), a program automatikusan erre vált, a MIDI Out-on játszik, és betölti az utolsó dalt; ha a zongora újra elérhető, visszavált (ha nem kézzel választottad). Ha nincs MIDI Out beállítva, erről egyszer üzenetet ad. A zongora mellett kézzel is választható. Ilyenkor a ConPianist saját lejátszója a MIDI Out-ra játszik; a Keverőben a hangerő, a pan és a zengetés szabványos MIDI-vezérlőkként (CC7, CC10, CC91) megy ki – a csatorna hangereje a dal saját hangerőváltozásait arányosan igazítja –, a Lejátszás panel hangereje minden csatornát arányosan állít, a „Hangszín módosítása” a General MIDI hangszíneket (a 10. csatornán a dobkészleteket) kínálja, és a csatornák a GM-hangszín nevét mutatják. A virtuális billentyűzet és a MIDI In 2 ilyenkor a MIDI Out-ra játszik. Ebben a módban a Hangszín fül, a zengetéstípus, a Stream Lights és a Segéd szürke; a Piano Room és a Hangerőegyensúly (a zongora saját hangja) használható marad. Ha a MIDI In 2 és a MIDI Out ugyanaz a port, a program nem küldi vissza a hangokat önmagának.
- **Lejátszott hangok a virtuális billentyűzeten USB-s lejátszásnál.** A ConPianist saját lejátszójával a jobb és bal kéz szólamának hangjai (a Keverő szólam-hozzárendelése szerint) automatikusan megjelennek a virtuális billentyűzeten, transzponálva, ahogy ténylegesen szólnak. A kíséret, a dobcsatorna és a némított szólamok hangjai nem jelennek meg; szünetnél, ugrásnál és megállításkor a kijelzés is törlődik. (Hálózati lejátszásnál ez nem lehetséges, mert a zongora nem küldi vissza a dal hangjait.)
- **Indulás a bezáráskori állapotban.** A program már nem mindig a hálózati lejátszással indul: a Lejátszás részben választott kimenetet (hálózat, USB, MIDI-eszköz) megjegyzi, és a következő indításkor azt használja. Ha az induláskor nem érhető el (pl. a zongora ki van kapcsolva), átmenetileg egy elérhető kimenetre vált, és amint a választott újra elérhető, visszavált rá. A hálózat elérhetőségét továbbra is ellenőrzi (ettől függ, melyik választógomb szürke). Bezáráskor (és dal betöltése után is) a teljes állapot a `LastState.conmem` fájlba kerül a beállításfájl mellé (Windowson: `%APPDATA%\ConPianist`): a Hangszín fül, a Piano Room, a Hangerőegyensúly, a Keverő, a Lejátszás panel, a betöltött dal és a dalban elért pozíció. Induláskor a program a zongora beállításait a csatlakozáskor, a dalt és a hozzá tartozó beállításokat a dal betöltésekor állítja vissza (ilyenkor a dal saját `.conmem` fájlja helyett). Hálózati lejátszásnál a dalt újra feltölti, ha a zongorán már nincs meg. Ha a zongora egy munkamenetben nem csatlakozott, a korábban mentett zongorabeállítások megmaradnak. A MIDI-eszközre mentett (General MIDI) és a zongorára mentett (Yamaha) hangszíneket a `.conmem` megkülönbözteti, és betöltéskor az aktuális lejátszóhoz alakítja; a General MIDI-re alakított Yamaha hangszín a zongorára visszaváltáskor pontosan az eredeti lesz.
- **Átméretezhető virtuális billentyűzet.** A billentyűzet feletti vonal egérrel húzva állítja a billentyűzet magasságát; a billentyűk arányosan szélesednek, és a magasság csak addig növelhető, amíg a teljes, 88 billentyűs sor elfér az ablakban (legfeljebb az ablak feléig). Dupla kattintás a vonalon visszaállítja az alapméretet. A beállított magasságot a program megjegyzi.

### Módosítva
- **Alapértelmezett mappa a dalokhoz:** a dalok, kották és `.conmem` fájlok megnyitása és mentése alapból a beállításfájl melletti `Demo Midi Songs` mappában indul (Windowson: `%APPDATA%\ConPianist\Demo Midi Songs`; ha nincs, a program létrehozza). A korábbi verziók által megjegyzett mappa egyszer erre áll át; utána a program ugyanúgy megjegyzi a legutóbb használt mappát, és ha az megszűnik, visszaáll a `Demo Midi Songs` mappára.
- **Kapcsolat újraindítása.** A Főmenü „Kapcsolat újraindítása” pontja a zongora kapcsolatán kívül a MIDI-eszközt (MIDI Out, MIDI In 2) is újranyitja, és újra ellenőrzi a hálózati elérhetőséget; így a kiszürkült lejátszási választógombok újraaktiválhatók (pl. ha egy sikertelen feltöltés után a hálózati lejátszás szürke maradt). Korábban csak a zongora MIDI-kapcsolatát nyitotta újra.
- **A lejátszó váltása nem változtatja meg a beállításokat.** Ha a lejátszás kimenetét váltod (hálózat, USB, MIDI-eszköz; kézzel vagy automatikusan), a dal ugyanúgy újratöltődik, de a Keverő (hangerő, pan, zengetés, csatornák ki/be, szólam-hozzárendelés, hangszínek) és a bal panel beállításai (szólamok, hangerő, tempó, transzponálás, A–B ismétlés, pozíció) megmaradnak; a `.conmem` ilyenkor nem töltődik be újra. A zongora és a MIDI-eszköz közötti váltásnál a hangszínek a legközelebbi megfelelőre váltanak (Yamaha ↔ General MIDI, a hangszín kategóriája és programszáma alapján); ha nincs megfelelő, zongora hangszín lesz. Dal betöltésekor továbbra is a dal és a hozzá tartozó `.conmem` beállításai érvényesek.
- **Élő játék több csatornán, MIDI In 2-vel.** A Keverő csatornamenüjének „Virtuális billentyűzet használata” pontja új nevet kapott: **„Élő játék ezen a csatornán”** (angolul „Live Play on This Channel”), és be-/kikapcsolható: egyszerre több csatorna is kiválasztható, ilyenkor minden leütött hang minden kiválasztott csatornán, rétegezve szól (legalább egy csatorna mindig kiválasztva marad, alapból az 1.). A virtuális billentyűzet és a MIDI In 2 (a pedállal, pitch benddel és a többi vezérlővel együtt) ezeken a csatornákon szól: hálózati és USB-s lejátszásnál a zongorán, MIDI-eszköz módban a MIDI Out-on. A MIDI In 2 hangjai látszanak a virtuális billentyűzeten. A Lejátszás panel transzponálása az élő játékra is vonatkozik, minden lejátszási módban (a 10., dobcsatornát kivéve); a lenyomott hang azon a hangon enged fel, amelyiken megszólalt, akkor is, ha közben változik a transzponálás. Ha játék közben változik a kiválasztás, a lenyomott hangok és a pedál ott engednek fel, ahol megszólaltak. A kiválasztott csatornák a `.conmem`-be is mentődnek (a régi, egycsatornás fájlok is betölthetők).
- A virtuális billentyűzet láthatósága nem része a regisztrációs memóriának: a `.conmem` fájlba már nem kerül be, a régebbi fájlokban lévő értéket a program figyelmen kívül hagyja. Így a billentyűzet a program újraindítása, dal betöltése és a lejátszó váltása után is úgy marad, ahogy utoljára beállították (a billentyűzet csatornája továbbra is a `.conmem` része).

### Javítva
- Az USB-kábel kihúzása és visszadugása után a zongora kapcsolata nem állt helyre (sem magától, sem a „Kapcsolat újraindítása” menüponttal), mert a program a régi, már nem működő MIDI-portot tartotta nyitva. Most újracsatlakozáskor a portok bezáródnak és újra megnyílnak, így a kapcsolat a kábel visszadugása után pár másodperc alatt magától helyreáll.

## 4.2 (fork) – 2026. szeptember 26.

Fő újdonság: zenedarab lejátszása USB-n keresztül, Wi-Fi nélkül, a ConPianist saját lejátszójával; a lejátszó a bal panelen választható.

### Új
- **Lejátszás USB-n keresztül, Wi-Fi nélkül.** A ConPianist saját lejátszója maga játssza le a MIDI-fájlt, és a hangokat pontos időzítéssel küldi a zongorának a MIDI-porton (USB) keresztül. Működik: lejátszás/szünet, pozíció és ütemszám, ugrás ütemenként, tempó, transzponálás, A–B ismétlés, kotta szinkron. A zenedarab elején lévő beállításokat (hangszínek, hangerő stb.) a program csak betöltéskor küldi el, így a Keverőben módosított hangszínek megmaradnak; a dal közepére ugráskor a közben változó beállításokat (pl. pedál) a program helyreállítja. A Stream Lights és a zongora Segéd módja ebben a módban nem működik, mert ezek a zongora saját lejátszójához kötődnek.
- **Választható lejátszó (Lejátszás rész a bal panelen).** A Szólam alatt két választógombbal dönthető el, ki játssza a zenedarabot: „Hálózaton keresztül” – a zongora saját lejátszója (a dal Wi-Fi-n töltődik fel, a Stream Lights és a Segéd is működik), vagy „USB-n keresztül” – a ConPianist saját lejátszója. Alapértelmezés minden indításkor a hálózati lejátszás, USB-kapcsolatnál is, ha a zongora elérhető hálózaton; ezt a program indításkor és a Kapcsolat beállításainak módosításakor ellenőrzi. A nem elérhető lehetőség szürke (hálózati kapcsolatnál az USB-s). Váltáskor a betöltött dal automatikusan újratöltődik a másik lejátszóba, az aktuális ütemnél, a `.conmem` beállításaival együtt. Ha a dal feltöltése a zongorára nem sikerül, a program üzenettel átvált USB-s lejátszásra, és a következő indításig abban marad. USB-s lejátszásnál a Stream Lights és a Segéd gombja szürke.
- **Az utolsó dal visszatöltése USB-s lejátszásnál.** A program megjegyzi az utoljára betöltött dalt; ha indításkor USB-s lejátszás lesz érvényben, automatikusan betölti (a `.conmem` beállításaival és a kottával együtt). Ha a dal a zongora kikapcsolt állapotában töltődött be, a zongora csatlakozásakor a program újratölti, hogy a hangszínek is beálljanak. Hálózati lejátszásnál ez eddig is így volt, mert ott a zongora őrzi a dalt.
- **Csatornák és szólamok ki-/bekapcsolása USB-s lejátszásnál.** A Keverő csatornakapcsolói, a Hangerőegyensúly ablak zenedarab-kapcsolója és a lejátszás jobb kéz / bal kéz / kíséret gombjai a saját lejátszóval is működnek; hogy melyik csatorna melyik szólamhoz tartozik, azt a Keverő szólam-hozzárendelése dönti el (alapból 1. csatorna jobb kéz, 2. csatorna bal kéz). Kikapcsoláskor a csatorna azonnal elhallgat, a tartott (pedálos) hangok is. Új dal betöltésekor minden csatorna és szólam bekapcsol, a dalhoz tartozó `.conmem` beállításai ezután érvényesülnek.
- **A zenedarab csatornáinak hangszíne módosítható a Keverőből.** A csatorna menüjében a „Hangszín módosítása” almenüben kategóriák szerint választható ki az új hangszín. A billentyűzet hangszíneit nem érinti; a módosítás a lejátszás végéig és a program újraindítása után is megmarad, a MIDI-fájl újbóli betöltése visszaállítja az eredetit. A program ugyanúgy állítja be a hangszínt, mint egy MIDI-fájl: szabványos bankváltó (CC0, CC32) és programváltó üzenettel az adott csatornán. A zongora saját vezérlőüzeneteit (VoicePreset, VoiceMidi) a zenedarab csatornáinál visszautasítja.
- **A zenedarab csatornáinak hangszíne a regisztrációs memóriába (`.conmem`) is mentődik.** A Főmenü „Zongoraállapot mentése” pontjával a módosított hangszínek dalonként elmenthetők; mivel a program a dal betöltésekor automatikusan betölti a mellette lévő `.conmem` fájlt, a saját hangszínek a MIDI-fájl újratöltése után is visszaállnak. A régebbi `.conmem` fájlok változatlanul használhatók.

### Javítva
- **Visszautasított kérések kezelése:** ha a zongora hibakóddal utasít vissza egy kérést, a program eddig a hibaválaszt érvényes értéknek vette (pl. üresre állította a csatorna hangszínének nevét). Most figyelmen kívül hagyja, és a naplóba írja.
- **Lefagyás MIDI-fájl feltöltésekor:** ha a zongora nem válaszolt, a program ablaka végleg lefagyott, mert a válaszra időkorlát nélkül várt. Most legfeljebb 3 másodpercig próbál csatlakozni, és legfeljebb 10 másodpercig vár válaszra, utána hibaüzenetet ír ki.
- **Hibás feltöltés sikeresnek jelezve:** a hálózati írás és olvasás hibakódját (`-1`) a program sikernek vette, ilyenkor a felirat örökre „Betöltés...” maradt.
- **Összeomlás olvashatatlan MIDI-fájlnál:** ha a kiválasztott fájlt nem lehetett megnyitni (pl. közben törölték), a program összeomlott; most hibaüzenetet ír ki.
- **Összeomlás hálózati csatlakozáskor:** a hálózati MIDI szabad portot kereső függvénye egy részleges siker után minden további próbálkozásnál hibázott, és a program üres objektumra hivatkozva összeomlott. A portkeresés javítva, sikertelenség esetén a program másodpercenként újrapróbálkozik.
- **Mesterhangolás elcsúszása:** a regisztrációs memóriából (`.conmem`) visszatöltött hangolás az értékek kb. 40%-ánál 0,1 Hz-cel eltért (pl. 442,3 Hz helyett 442,2 Hz), mert az érték kerekítés helyett csonkolódott.
- **Memóriahibák váratlan zongoraüzeneteknél:** a zongorától érkező csatorna- és szólamindexek ellenőrzése, mielőtt a program tömböket címez velük.
- Hosszú, ékezetes fájlnevek feltöltése: a név hossza most bájtban (UTF-8) számolva fér bele a 255 bájtos korlátba.
- Kisebb robusztussági javítások (nem inicializált mutató, hibás `MaxPan` konstans).
- A Keverőben a zengetéstípus legördülő listája nem lóg bele az elválasztó vonalba (a magyar „Zengetés” felirat hosszabb, mint az angol).
- A „Zongoraállapot mentése” rákérdez, mielőtt egy már létező fájlt felülírna.
- A regisztrációs memória betöltésekor a program már nem próbálja be-/kikapcsolni a zenedarab Master csatornáját, amit a zongora visszautasít.
- A kotta egérgörgővel és húzással csak a kotta tetejéig és aljáig görgethető; korábban a kottát a „végtelenbe” lehetett tolni. Ablakméret-változáskor a görgetési pozíció a kottán belül marad.
- A Keverő csatornamenüjének „Lejátszás a virtuális billentyűzeten” pontja új nevet kapott: „Virtuális billentyűzet használata” (angolul „Play on Virtual Keyboard” helyett „Use Virtual Keyboard”). A pont azt választja ki, hogy a virtuális billentyűzetre kattintva melyik csatornán szóljon a hang; a régi név azt sugallta, hogy a dal hangjai jelennek meg a billentyűzeten.

## 4.1 (fork) – 2026. szeptember

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
- A "MIDI-fájl betöltési hiba" jelenség (`Error loading midi`) nem szoftverhiba: a MIDI-fájl feltöltése a zongorára hálózaton keresztül történik (TCP, 10504-es port), USB-kapcsolattal ez a funkció nem használható — a zongorát Wi-Fi-re kell kötni, és a Connection képernyőn be kell állítani a helyes "Piano IP" címet.

---

## 4.0 – soha hivatalosan ki nem adva (fejlesztés: 2020. május–november)

Az eredeti fejlesztő a `v3.0` után a `.jucer`-ben átírta a verziószámot "4.0"-ra, és tovább dolgozott a `develop` branchen — de ez a munka soha nem lett formális GitHub release-ként kiadva vagy dokumentálva (2020. november 7-i utolsó commit után a fejlesztés láthatóan leállt). Emiatt mutatta az alkalmazás a 4.0-s verziószámot hivatalos changelog nélkül, egészen a fork 4.1-es verziójáig. Az alábbi lista a `v3.0` tag és a `develop` branch közti 35 commit alapján készült.

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
