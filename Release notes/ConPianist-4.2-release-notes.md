## ConPianist 4.2 (fork)

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

A részletes változáslista a [CHANGELOG.md](CHANGELOG.md) fájlban található.

---

<details>
<summary><b>English</b></summary>

## ConPianist 4.2 (fork)

Highlight: song playback over USB, without Wi-Fi, with ConPianist's own player; the player can be chosen on the left panel.

### New
- **Playback over USB, without Wi-Fi.** ConPianist's own player plays the MIDI file itself and sends the notes to the piano over the MIDI port (USB) with exact timing. Supported: play/pause, position and measure number, stepping by measure, tempo, transpose, A–B loop, score sync. The settings at the beginning of the song (voices, volumes etc.) are sent only when the song is loaded, so voices changed in the Mixer are kept; when jumping into the middle of the song, settings that change during the song (e.g. the pedal) are restored. Stream Lights and the piano's Guide mode do not work in this mode, because they belong to the piano's own player.
- **Selectable player (Playback section on the left panel).** Below Part, two radio buttons choose who plays the song: "Via Network" – the piano's own player (the song is uploaded over Wi-Fi; Stream Lights and Guide work), or "Via USB" – ConPianist's own player. The default at every start is network playback, also with a USB connection, if the piano can be reached over the network; this is checked at start and when the Connection Settings are changed. An unavailable option is greyed out (with a network connection, the USB option). When switching, the loaded song is reloaded into the other player automatically, at the current measure, with its `.conmem` settings. If uploading the song to the piano fails, the program shows a message, switches to USB playback, and stays there until the next start. With USB playback, the Stream Lights and Guide buttons are greyed out.
- **Last song reloaded with USB playback.** The program remembers the last loaded song; if USB playback is active at start, it loads it automatically (with its `.conmem` settings and score). If the song was loaded while the piano was switched off, it is reloaded when the piano connects, so that the voices are set too. With network playback this already worked, because the piano keeps the song.
- **Channels and parts on/off with USB playback.** The Mixer's channel switches, the song switch in the Balance window, and the right hand / left hand / backing buttons also work with the own player; which channel belongs to which part is decided by the Mixer's part assignment (by default channel 1 right hand, channel 2 left hand). A switched-off channel is silenced immediately, including held (pedalled) notes. When a new song is loaded, all channels and parts are switched on, then the song's `.conmem` settings apply.
- **Voices of the song's channels can be changed in the Mixer.** In the channel menu, the "Change Voice" submenu offers the new voice by category. The keyboard voices are not affected; the change is kept until the end of playback and after restarting the program, and loading the MIDI file again restores the original. The voice is set the same way as by a MIDI file: with standard bank select (CC0, CC32) and program change messages on the channel. (The piano rejects its own control messages – VoicePreset, VoiceMidi – for song channels.)
- **Voices of the song's channels are saved in the registration memory (`.conmem`).** With "Save Piano State" in the Main menu, the changed voices can be saved per song; since the program loads the `.conmem` file next to the song automatically, your own voices come back after reloading the MIDI file. Older `.conmem` files can still be used.

### Fixed
- **Rejected requests:** if the piano rejected a request with an error code, the program took the error answer as a valid value (e.g. set the channel's voice name to empty). Now it is ignored and logged.
- **Freeze when uploading a MIDI file:** if the piano did not answer, the program window froze for good, because it waited for the answer without a time limit. Now it tries to connect for at most 3 seconds and waits at most 10 seconds for an answer, then shows an error message.
- **Failed upload reported as successful:** the error code (`-1`) of network writes and reads was taken as success, and the label stayed "Loading..." forever.
- **Crash with an unreadable MIDI file:** if the selected file could not be opened (e.g. it was deleted in the meantime), the program crashed; now it shows an error message.
- **Crash when connecting over the network:** after a partial success, the network MIDI function looking for a free port failed at every further attempt, and the program crashed on an empty object. The port search is fixed; if it fails, the program retries every second.
- **Master tune drift:** the tuning restored from the registration memory (`.conmem`) was 0.1 Hz off for about 40% of the values (e.g. 442.2 Hz instead of 442.3 Hz), because the value was truncated instead of rounded.
- **Memory errors with unexpected piano messages:** channel and part indexes coming from the piano are checked before they are used to address arrays.
- Uploading long file names with accented letters: the length of the name is now counted in bytes (UTF-8), so it fits into the 255-byte limit.
- Smaller robustness fixes (uninitialised pointer, wrong `MaxPan` constant).
- In the Mixer, the reverb type drop-down list no longer overlaps the separator line (the Hungarian label is longer than the English one).
- "Save Piano State" asks before overwriting an existing file.
- When loading a registration memory, the program no longer tries to switch the song's Master channel on or off, which the piano rejects.
- The score can be scrolled with the mouse wheel and by dragging only between its top and bottom; before, it could be pushed "into infinity". When the window is resized, the scroll position stays within the score.
- The Mixer channel menu item "Play on Virtual Keyboard" was renamed to "Use Virtual Keyboard". It selects the channel that sounds when clicking the virtual keyboard; the old name suggested that the song's notes are shown on the keyboard.

The detailed list of changes (in Hungarian) is in [CHANGELOG.md](CHANGELOG.md).

</details>
