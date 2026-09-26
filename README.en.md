# ConPianist

> **This is a personal fork** by [Viktor318](https://github.com/Viktor318/conpianist) of the original project [hugbug/conpianist](https://github.com/hugbug/conpianist), maintained for a Yamaha CSP-170. The main documentation of this fork is in Hungarian ([README.md](README.md)); the text below is the original English README.
>
> **New in this fork** (see [CHANGELOG.md](CHANGELOG.md), in Hungarian):
> - bilingual user interface (English/Hungarian), switchable in the main menu (LANGUAGE);
> - the voices of the song's MIDI channels can be changed in the mixer, and they are saved in the registration memory (`.conmem`);
> - songs can also be played over USB without Wi-Fi, using ConPianist's own player; the player (the piano's own via the network, or ConPianist's via USB) can be chosen in the Playback section of the left panel;
> - songs can also be played to any other MIDI device without the piano (e.g. via loopMIDI to a software instrument such as Cantabile), with General MIDI voices and standard MIDI controllers in the mixer;
> - the played notes are shown on the virtual keyboard, which can be resized;
> - builds with current tools (Visual Studio 2026, recent JUCE, vcpkg); several crash and freeze fixes.
>
> A ready-to-run Windows (64-bit) build is available on the [Releases](https://github.com/Viktor318/conpianist/releases) page.

**ConPianist** or **Connected Pianist** is an app to control Yamaha digital pianos of CSP (Clavinova Smart Piano) series. This is an alternative to Yamaha's own app "Smart Pianist". Unlike Smart Pianist, which works on iOS and Android, Connected Pianist is designed for desktop systems - macOS, Windows and Linux. It works on iPad too though.

## Features

The program is not an adequate replacement for the official app yet. Nonetheless the program already can:
- connect to piano via network or cable;
- connect/reconnect without losing piano state: the program reads whole piano state on start and indicates it in the UI;
- upload midi-files to piano via network (but can't upload via USB cable);
- playback control of uploaded midi-files: start, pause, position;
- stream lights control: on, off, slow, fast;
- guide mode control: on, off, guide mode;
- select parts: backing, right hand, left hand;
- playback selected fragment in a loop;
- volume, tempo, transpose;
- select voices (all seven hundreds) for main, left and layer;
- octave shift and split point (main/left) setting;
- mixer with all classic functions: midi-channels on/off, volume, pan, reverb, reverb effect;
- extra functions in mixer: part selection for midi-channels, voice selection directly from midi-channels;
- balance for main/left/layer/song/mic/auxin: volume, pan, reverb, reverb effect;
- show scores with correct playback position: scores must be provided in a separate muscixml-file (can't show scores directly from midi-file);
- registration memory (settings) associated with midi-songs.

## Screenshots

![Playback with scores](https://user-images.githubusercontent.com/3368402/79354560-d8c86b00-7f3c-11ea-8710-9c948982de3a.png)

![Voice selection](https://user-images.githubusercontent.com/3368402/79360975-f699ce00-7f44-11ea-8aa1-803be03533d7.png)

![Mixer](https://user-images.githubusercontent.com/3368402/79354913-4ffdff00-7f3d-11ea-8bf9-0ecda2180204.png)

## Acknowledgements

The source code of ConPianist includes following libraries:
- [Lomse](https://github.com/lenmus/lomse) to display scores;
- [Arduino AppleMIDI Library](https://github.com/lathoub/Arduino-AppleMIDI-Library) to communicate with piano via network.
