/*
 *  This file is part of ConPianist. See <https://github.com/Viktor318/conpianist>.
 *
 *  Copyright (C) 2026 Viktor Oszkó <oszko.viktor@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FactoryOts.h"
#include "Presets.h"

// Voices of the style files that are not on the CSP-170 with the same number, and the
// panel voice of the CSP-170 used for them (0x00MMLLPP; generated from
// docs/Gyari_OTS_hangszinelemzes_pontos_CVP809_CVP709_Genos_SX900_SX700_SX600_S970_S950.xlsx, October 17, 2026:
// the same name, a similar name, or chosen by Viktor). This table is the one of the PSR-SX900,
// PSR-SX700, PSR-SX600 and PSR-S970.
static const std::pair<int, int> VoiceTable[] = {
	{0x000008, 0x007008}, // name: Celesta -> Celesta
	{0x00006f, 0x007044}, // decision: Shanai -> Oboe
	{0x00606f, 0x00706d}, // decision: Pungi -> Bagpipe
	{0x006851, 0x68053e}, // similar: RS SawLead1 -> Brassy Lead
	{0x006a51, 0x68053e}, // similar: RS SawLead2 -> Brassy Lead
	{0x006c51, 0x681051}, // similar: RS TechSaw -> Soft Saw
	{0x006d51, 0x681051}, // similar: RS DualSaw -> Soft Saw
	{0x007001, 0x6c1001}, // name: BrightPiano -> Bright Piano
	{0x007003, 0x680d03}, // similar: HonkyTonk -> Honky Tonk Upright
	{0x007009, 0x680009}, // name: Glockenspiel -> Glockenspiel
	{0x00700b, 0x68010b}, // name: Vibraphone -> Vibraphone
	{0x00700c, 0x68000c}, // name: Marimba -> Marimba
	{0x00700d, 0x68000d}, // name: Xylophone -> Xylophone
	{0x00703c, 0x08213c}, // similar: FrenchHorns -> Warm Horns
	{0x007043, 0x680043}, // name: BaritoneSax -> Baritone Sax
	{0x007050, 0x680050}, // similar: SquareLead -> Sub Lead
	{0x007051, 0x680151}, // similar: SawLead -> PWM Lead
	{0x007103, 0x6c1f00}, // similar: OctavePiano1 -> Stage Piano
	{0x007115, 0x007217}, // similar: TuttiAccordion -> Tango Accordion
	{0x007136, 0x007434}, // similar: Voices -> Gospel Voices
	{0x00715b, 0x007359}, // similar: RS WarmPad -> Neo Warm Pad
	{0x007169, 0x680069}, // decision: Oud -> Banjo
	{0x007203, 0x6c1f00}, // similar: OctavePiano2 -> Stage Piano
	{0x007250, 0x680151}, // similar: MiniLead -> PWM Lead
	{0x007300, 0x6c0000}, // decision: ConcertGrand -> CFX Grand
	{0x007350, 0x680f51}, // similar: VinylLead -> Fusion Lead
	{0x00773c, 0x68023c}, // similar: SymphonyBrass -> Symphony Horns 2
	{0x007858, 0x007458}, // decision: PanHeaven -> Time Travel
	{0x007958, 0x007458}, // decision: DreamHeaven -> Time Travel
	{0x007a00, 0x6c0000}, // decision: ConcertGrand -> CFX Grand
	{0x007a50, 0x007654}, // similar: PanLead -> Hipa Lead
	{0x007d30, 0x007c30}, // decision: Watariyat -> Dynamic Strings
	{0x007e10, 0x007510}, // name: ClassicJazz -> Classic Jazz
	{0x007f50, 0x680550}, // similar: RS DualSquare -> Soft Square
	{0x082028, 0x680835}, // similar: Ooh -> Boys Choir Ooh
	{0x08202b, 0x082131}, // decision: Seattle1stViolins -> Real Strings
	{0x082128, 0x680835}, // similar: OohVibrato -> Boys Choir Ooh
	{0x08212b, 0x007028}, // similar: Orchestral1stViolin -> Solo Violin
	{0x08212c, 0x007028}, // similar: Orchestral2ndViolin -> Solo Violin
	{0x08212e, 0x00702a}, // similar: OrchestralCello -> Cello
	{0x082201, 0x082200}, // decision: SpanishGuitar -> Nylon Guitar
	{0x082228, 0x680835}, // similar: BoysOoh -> Boys Choir Ooh
	{0x082301, 0x082100}, // name: FlamencoGuitar -> Flamenco Guitar
	{0x082336, 0x007434}, // similar: GospelWow-Hey SW -> Gospel Voices
	{0x082401, 0x082300}, // similar: SpanishElectro -> Electro Acoustic Guitar
	{0x082437, 0x083137}, // decision: Ba-Da-Yah -> Boys Choir Oh-Ah
	{0x082501, 0x082001}, // decision: SteelAcousticPick -> Steel Guitar
	{0x082504, 0x68051b}, // similar: BluesyFinger -> Slide Finger
	{0x082701, 0x007119}, // similar: 12StringPicked -> 12String Guitar
	{0x082803, 0x082903}, // name: VintageAmp -> Vintage Amp
	{0x082804, 0x082c03}, // similar: CountryPickDrive -> Country Pick
	{0x082806, 0x082603}, // similar: SolidJazzGuitar -> 70s Solid Guitar
	{0x082904, 0x082c03}, // similar: CountryPickVintage -> Country Pick
	{0x082938, 0x08213c}, // similar: WarmPopHorns -> Warm Horns
	{0x082b30, 0x680631}, // similar: SeattleStringsPad -> Real Strings p
	{0x082b38, 0x082438}, // decision: PopHornsMellow -> Smooth Brass
	{0x082c38, 0x00753e}, // decision: PopHornsBright -> Pop Brass
	{0x082e05, 0x082705}, // similar: 50sDriveWahWah -> Overdrive Wah
	{0x083005, 0x082e03}, // similar: 50sVintageDelay -> Ballad Delay
	{0x083105, 0x082a05}, // decision: 50sVintageWarm -> Pure Vintage
	{0x083405, 0x00751d}, // similar: OldiesGuitar -> Blues Guitar
	{0x083605, 0x082603}, // similar: 70sJazzCombo -> 70s Solid Guitar
	{0x083705, 0x007654}, // similar: HalfWahLead -> Hipa Lead
	{0x083805, 0x68041b}, // similar: WahWahGuitar -> Cool Wah Guitar
	{0x083903, 0x082001}, // similar: SixtiesGuitar -> Steel Guitar
	{0x0a0000, 0x007510}, // decision: JazzStandard -> Classic Jazz
	{0x0a0002, 0x0a000a}, // decision: EuroPercussion -> Euro Perc
	{0x0a0102, 0x0a010a}, // name: EuroBallad -> Ballad Euro
	{0x0a0200, 0x0a0408}, // name: OrganPops -> Organ Pops
	{0x0a0202, 0x0a020a}, // name: EuroReeds -> Euro Reeds
	{0x0a0302, 0x0a030a}, // decision: EuroEvenWarmth -> Even Warmth
	{0x0a0400, 0x007113}, // similar: GospelOrgan -> Chapel Organ
	{0x0a0402, 0x0a040a}, // name: EuroBright -> Bright Euro
	{0x0a0500, 0x0a0409}, // name: BalladOrgan -> Ballad Organ
	{0x0a0502, 0x0a050a}, // name: EuroPops -> Euro Pops
	{0x0a0600, 0x680115}, // similar: OrganClarinet -> Accordion Clarinet
	{0x0a0602, 0x0a060a}, // name: EuroFull -> Full Euro
	{0x0a0700, 0x0a0909}, // name: JazzSquabble -> Jazz Squabble
	{0x680002, 0x680202}, // name: MIDI Grand -> MIDI Grand
	{0x680003, 0x6c1c03}, // name: CocktailPiano -> Cocktail Piano
	{0x680013, 0x007a13}, // decision: ConcertOrganTutti -> Pipe Organ Tutti
	{0x680036, 0x680133}, // similar: 80sBreathyVoice -> 80s Pad
	{0x68003d, 0x08203c}, // similar: PopHorns -> Soft Horns
	{0x680044, 0x082044}, // name: OrchestralOboe -> Orchestral Oboe
	{0x680049, 0x082049}, // name: OrchestralFlute -> Orchestral Flute
	{0x680103, 0x6c1c03}, // name: CocktailPiano -> Cocktail Piano
	{0x680136, 0x680133}, // similar: 80sDreamyVox -> 80s Pad
	{0x680203, 0x6c1f00}, // similar: OctavePiano1 -> Stage Piano
	{0x68023d, 0x08223c}, // similar: MuteSwingHorns -> Muted Horns
	{0x680300, 0x6c1b03}, // name: AmbientPiano -> Ambient Piano
	{0x680303, 0x6c1f00}, // similar: OctavePiano2 -> Stage Piano
	{0x680339, 0x00763c}, // similar: OrchTrombones -> Soft Trombones
	{0x68033d, 0x006d3d}, // decision: PopHornsAccent -> Accent Brass
	{0x680400, 0x6c1002}, // name: RockPiano -> Rock Piano
	{0x680413, 0x007b13}, // decision: Diapason -> Pipe Organ Principal
	{0x680438, 0x006c3d}, // decision: OrchTrumpetsFast -> Brass f
	{0x680439, 0x00763c}, // similar: OrchTrombonesFast -> Soft Trombones
	{0x68043c, 0x68013c}, // decision: OrchHornsFast -> Symphony Horns 1
	{0x68043d, 0x082038}, // similar: PopHornsShake -> Brass Shake
	{0x680463, 0x680133}, // similar: 80sHarpvox -> 80s Pad
	{0x680500, 0x6c1101}, // name: PopGrand -> Pop Grand
	{0x680513, 0x680147}, // similar: Claribel&Flute -> Clarinet & Flutes
	{0x68053d, 0x006a3d}, // decision: PopHornsCresc -> Brass Dynamics
	{0x680554, 0x680154}, // decision: BriteDecay -> Detuned Vintage
	{0x68063d, 0x00783c}, // similar: BalladBrass -> Small Brass
	{0x680704, 0x007604}, // similar: SuitcaseSoft -> Suitcase EP
	{0x680713, 0x007113}, // decision: SoftReeds -> Chapel Organ
	{0x680804, 0x007404}, // name: VintageEP -> Vintage EP
	{0x680900, 0x6c1b03}, // name: AmbientPiano -> Ambient Piano
	{0x680904, 0x007604}, // similar: SuitcaseBallad -> Suitcase EP
	{0x680932, 0x082030}, // similar: UnisonStrings -> Studio Strings
	{0x68093e, 0x00743f}, // similar: 80sSynthBrassStab -> Fat Synth Brass
	{0x680a00, 0x6c1002}, // name: RockPiano -> Rock Piano
	{0x680a04, 0x007604}, // similar: SuitcasePhaser -> Suitcase EP
	{0x680a3e, 0x00743f}, // similar: 80sSynthBrassTack -> Fat Synth Brass
	{0x680a59, 0x00735a}, // similar: 80sSoftPad -> Big Octave Pad
	{0x680b00, 0x6c1101}, // name: PopGrand -> Pop Grand
	{0x680b04, 0x007604}, // similar: SuitcaseDrive -> Suitcase EP
	{0x680b19, 0x00701b}, // similar: ChickenPickGuitar -> Clean Guitar
	{0x680b3d, 0x00733f}, // similar: OrchHornsPad -> Ober Horns
	{0x680b59, 0x007359}, // similar: 80sWarmVCF Pad -> Neo Warm Pad
	{0x680c04, 0x007104}, // similar: SuitcaseTremolo -> Tremolo Suitcase EP
	{0x680d00, 0x6c0502}, // name: UprightPiano -> Upright Piano
	{0x680d04, 0x007604}, // similar: SuitcaseJazz -> Suitcase EP
	{0x680d33, 0x007c30}, // similar: 80sMiamiStrings -> Dynamic Strings
	{0x680e04, 0x007604}, // similar: SuitcaseWarmth -> Suitcase EP
	{0x680e32, 0x082131}, // similar: 80sDreamStrings -> Real Strings
	{0x680f04, 0x007404}, // similar: VintageEP Drive -> Vintage EP
	{0x680f32, 0x082030}, // similar: 80sPopStrings -> Studio Strings
	{0x680f34, 0x680834}, // similar: AccompAah -> Boys Choir Aah
	{0x680f35, 0x680835}, // similar: AccompOoh -> Boys Choir Ooh
	{0x680f5f, 0x00775f}, // similar: 80sSynthSlowSwell -> Analog Swell
	{0x681004, 0x007604}, // similar: SuitcaseHard -> Suitcase EP
	{0x681032, 0x680358}, // similar: 80sBalladPad -> Galaxy Pad
	{0x681250, 0x680550}, // similar: 80sBitSquareArp -> Soft Square
	{0x68135a, 0x68007e}, // decision: Crowds -> Applause 1
	{0x68145a, 0x68055a}, // decision: Gemini -> Bright Fat Saw
	{0x68155a, 0x68055a}, // decision: HandsUp! -> Bright Fat Saw
	{0x681812, 0x007213}, // similar: HomeOrganViolin -> Hymn Organ
	{0x68185a, 0x68055a}, // decision: PunchyChordz -> Bright Fat Saw
	{0x682211, 0x0a080a}, // name: EuroAccomp2 -> Euro Accomp 2
	{0x682311, 0x0a090a}, // name: EuroAccomp3 -> Euro Accomp 3
	{0x682951, 0x007651}, // similar: 80sArnoldLead -> Early Lead
};

// The voices of the other models that differ from VoiceTable (-1: none on the CSP-170). A
// voice of these models may differ from the voice of the same number of the PSR models.
static const std::pair<int, int> Cvp809VoiceTable[] = {
	{0x00000f, 0x00700f}, // name: Dulcimer -> Dulcimer
	{0x007a0c, 0x68012e}, // similar: Orchestral Marimba -> Orchestral Harp
	{0x007b00, 0x6c1001}, // name: Bright Piano -> Bright Piano
	{0x007b03, 0x6c0000}, // similar: CFX 1Octave -> CFX Grand
	{0x007c03, 0x6c0000}, // similar: CFX 2Octaves -> CFX Grand
	{0x082007, 0x083803}, // decision: 60s Shadow Lead -> Shadowed Guitar
	{0x082107, 0x082603}, // similar: 60s Ballad Guitar -> 70s Solid Guitar
	{0x082114, 0x007007}, // similar: Clavi SW -> Clavi Bright
	{0x082207, 0x082505}, // similar: 60s Vintage Blues -> Vintage Blues
	{0x082227, 0x680834}, // similar: Boys Haa -> Boys Choir Aah
	{0x082307, 0x082305}, // similar: 60s Rock Hero -> Rock Hero
	{0x08232b, 0x007b30}, // decision: Kino Strings Violins -> Movie Strings
	{0x08232d, 0x007c31}, // similar: Kino Strings Violas -> Disco Strings 2
	{0x08232e, 0x007b30}, // decision: Kino Strings Cellos -> Movie Strings
	{0x082407, 0x00751b}, // similar: 60s Vintage Clean -> 60s Clean
	{0x082504, 0x083703}, // similar: Bluesy Fingers -> Bluesy Night
	{0x082507, 0x68071b}, // similar: 60s Chicken Pick -> Slide Pick
	{0x082536, 0x007434}, // similar: Gospel Wow-Hey SW -> Gospel Voices
	{0x082607, 0x082005}, // similar: 60s Guitar Hero -> Guitar Hero
	{0x082631, 0x082930}, // similar: Horns & Strings -> Lush Strings
	{0x082714, 0x007604}, // similar: 70s Suitcase Warm 1 -> Suitcase EP
	{0x082807, 0x00751b}, // similar: 60s Blues Rock -> 60s Clean
	{0x082814, 0x007404}, // similar: 70s Vintage EP -> Vintage EP
	{0x082828, 0x680835}, // similar: Boys Doo -> Boys Choir Ooh
	{0x08290c, 0x007219}, // name: Mandolin -> Mandolin
	{0x082914, 0x007604}, // similar: 70s Suitcase Clean -> Suitcase EP
	{0x082937, 0x007434}, // decision: Latin Vocals -> Gospel Voices
	{0x082a0c, 0x007219}, // similar: Mandolin Mute Velocity -> Mandolin
	{0x082a14, 0x007104}, // similar: 70s Suitcase Tremolo -> Tremolo Suitcase EP
	{0x082a38, 0x00713a}, // similar: Bright Horn Section -> Baritone Horn
	{0x082b30, 0x680258}, // similar: Seattle String Pad -> Pearls Pad
	{0x082c07, 0x00731c}, // similar: 60s Vintage Echo -> Vintage Muted Echo
	{0x082c14, 0x007604}, // similar: 70s Suitcase Ballad -> Suitcase EP
	{0x082c31, 0x00712c}, // similar: Kino Strings Tremolo -> Tremolo Strings
	{0x082d01, 0x082000}, // similar: Resonator Guitar -> Concert Guitar
	{0x082d14, 0x007604}, // similar: 70s Suitcase Pan -> Suitcase EP
	{0x082d31, 0x007c31}, // similar: Kino Strings Vn Va Vc -> Disco Strings 2
	{0x082e01, 0x007718}, // similar: Resonator Mute Velocity -> Nylon Mute
	{0x082e07, 0x007819}, // similar: 60s Shadow Mute -> Steel Guitar Mute
	{0x082e14, 0x007604}, // similar: 70s Suitcase Amped -> Suitcase EP
	{0x082e31, 0x007c31}, // similar: Kino Strings Natural -> Disco Strings 2
	{0x082f01, 0x00741c}, // similar: D-Folk Guitar -> Funk Guitar
	{0x082f07, 0x007819}, // similar: 60s Country Mute -> Steel Guitar Mute
	{0x082f14, 0x007604}, // similar: 70s Suitcase Phase -> Suitcase EP
	{0x082f31, 0x007b30}, // decision: Kino Strings -> Movie Strings
	{0x083007, 0x082c03}, // similar: 60s Funk Pick -> Country Pick
	{0x083014, 0x007604}, // similar: 70s Suitcase Warm 2 -> Suitcase EP
	{0x083031, 0x007b30}, // decision: Kino Strings Vn Vc -> Movie Strings
	{0x083101, 0x082001}, // decision: Steel Acoustic Finger -> Steel Guitar
	{0x083114, 0x680e00}, // similar: 70s Vintage Drive EP -> 70s Ballad Piano
	{0x083131, 0x007b30}, // decision: Kino Strings 1Octave -> Movie Strings
	{0x083331, 0x007b30}, // decision: Kino Strings Warm -> Movie Strings
	{0x083431, 0x007b30}, // decision: Kino Strings Low -> Movie Strings
	{0x083531, 0x007c31}, // similar: Kino Strings 2Octaves -> Disco Strings 2
	{0x083601, 0x68071b}, // similar: Steel Thumb Pick -> Slide Pick
	{0x083631, 0x007b30}, // decision: Kino Strings Warm Vc -> Movie Strings
	{0x083731, 0x007c31}, // similar: Kino Strings Slow -> Disco Strings 2
	{0x083d03, 0x082001}, // similar: Pedal Steel Guitar 1 -> Steel Guitar
	{0x084030, 0x007128}, // similar: Jazz Violin -> Violin
	{0x084032, 0x00702a}, // decision: Classical Cello -> Cello
	{0x08403e, 0x007638}, // name: Flugelhorn -> Flugel Horn
	{0x084040, 0x00723b}, // similar: Mute Trumpet -> Muted Trumpet
	{0x084041, 0x007338}, // similar: Soft Trumpet -> Trumpet
	{0x084044, 0x007044}, // similar: Pop Oboe -> Oboe
	{0x08404a, 0x007349}, // name: Classical Flute -> Classical Flute
	{0x084050, 0x007542}, // name: Breathy Tenor Sax -> Breathy Tenor Sax
	{0x084051, 0x680043}, // similar: Funk Baritone Sax -> Baritone Sax
	{0x084053, 0x082242}, // similar: Funk Alto Sax -> Soft Alto Sax
	{0x084054, 0x007140}, // similar: Pop Soprano Sax -> Soprano Sax
	{0x08405c, 0x007247}, // similar: Ballad Clarinet -> Jazz Clarinet
	{0x084068, 0x007016}, // name: Harmonica -> Harmonica
	{0x084130, 0x007028}, // similar: Celtic Violin -> Solo Violin
	{0x084141, 0x007338}, // decision: Classic Trumpet -> Trumpet
	{0x084144, 0x007144}, // name: Classical Oboe -> Classical Oboe
	{0x084146, 0x007046}, // decision: Classical Bassoon -> Bassoon
	{0x084149, 0x007539}, // name: Trombone -> Trombone
	{0x08414a, 0x007249}, // name: Jazz Flute -> Jazz Flute
	{0x084150, 0x007f42}, // similar: Smooth Tenor Sax -> Pop Tenor Sax
	{0x084151, 0x680043}, // name: Baritone Sax -> Baritone Sax
	{0x084153, 0x082042}, // name: Alto Sax -> Alto Sax
	{0x084154, 0x007140}, // similar: Ballad Soprano Sax -> Soprano Sax
	{0x08415c, 0x007047}, // name: Clarinet -> Clarinet
	{0x084168, 0x007016}, // similar: Blues Harmonica -> Harmonica
	{0x084241, 0x007338}, // name: Trumpet -> Trumpet
	{0x084250, 0x007f42}, // similar: Tenor Sax -> Pop Tenor Sax
	{0x08425c, 0x680247}, // similar: Romance Clarinet -> German Clarinet
	{0x084341, 0x082540}, // name: Big Band Trumpet -> Big Band Trumpet
	{0x68003d, 0x00753e}, // decision: Pop Horns Dynamic -> Pop Brass
	{0x680229, 0x007c31}, // similar: Kino Strings Spic Va -> Disco Strings 2
	{0x680338, 0x006e3d}, // decision: Orchestral Trumpets -> Brass mf
	{0x680417, 0x007815}, // similar: Bajan Accordion -> Jazz Accordion
	{0x680513, 0x007349}, // similar: Claribel & Flute -> Classical Flute
	{0x680600, 0x6c1101}, // name: Pop Grand -> Pop Grand
	{0x68083d, 0x68023c}, // similar: Symphony Brass -> Symphony Horns 2
	{0x68093d, 0x00763c}, // similar: Trumpets & Trombones -> Soft Trombones
	{0x680a02, 0x007102}, // similar: CP80 Electric Grand -> CP80 Stage
	{0x680b02, 0x007102}, // similar: CP80 Production -> CP80 Stage
	{0x680b3c, 0x00713a}, // similar: Baritone Horn Long -> Baritone Horn
	{0x680c35, 0x007434}, // similar: Gospel Vocals Hmm -> Gospel Voices
	{0x680d19, 0x007819}, // similar: Steel Guitar Twin 2 -> Steel Guitar Mute
	{0x680d3d, 0x00763c}, // similar: Orch Horns & Trombones -> Soft Trombones
	{0x680f01, 0x680e01}, // similar: Stage Upright -> Saloon Upright
	{0x680f3d, 0x68023c}, // similar: Symphony Horns Fast -> Symphony Horns 2
	{0x681200, 0x6c0000}, // decision: Bösendorfer Grand -> CFX Grand
	{0x681201, 0x6c0601}, // name: Themic Grand -> Themic Grand
	{0x681303, 0x6c1b03}, // name: Ambient Piano -> Ambient Piano
	{0x681305, 0x6c0000}, // similar: CFX Ballad Mix 1 -> CFX Grand
	{0x681318, 0x007219}, // similar: Mandolin Tremolo -> Mandolin
	{0x681403, 0x6c1c03}, // similar: CFX Cocktail Piano -> Cocktail Piano
	{0x681430, 0x007c31}, // similar: Kino Strings Spic Sct -> Disco Strings 2
	{0x681605, 0x6c0000}, // similar: CFX Ballad Mix 2 -> CFX Grand
	{0x681805, 0x6c0000}, // decision: Atmosphere Grand -> CFX Grand
	{0x681900, 0x6c0000}, // decision: Warm Pad Grand -> CFX Grand
	{0x681915, 0x007715}, // similar: Super Musette -> French Musette Acc.
	{0x681950, 0x007851}, // similar: Wire Lead Millennial -> Wire Lead
	{0x68195a, 0x68055a}, // decision: Trancy Chordz -> Bright Fat Saw
	{0x681a50, 0x680f51}, // similar: Fusion Wire -> Fusion Lead
	{0x681b15, 0x007515}, // similar: Steirische Harmonika -> Steirische
	{0x681b50, 0x007c50}, // similar: Mini Classic Unison -> Soft Mini
	{0x681c15, 0x680015}, // similar: Jazz Cassotto -> Cassotto
	{0x681d50, 0x007c50}, // similar: Mini Classic Velocity+ -> Soft Mini
	{0x681e00, 0x6c0000}, // similar: CFX Pad Production -> CFX Grand
	{0x681e50, 0x007c50}, // similar: Mini Classic Soft -> Soft Mini
	{0x681f50, 0x007c50}, // similar: Mini Classic Open -> Soft Mini
	{0x682111, 0x0a070a}, // name: Euro Accomp 1 -> Euro Accomp 1
	{0x682150, 0x007c50}, // similar: Funky Velocity Mini -> Soft Mini
	{0x682200, 0x6c0000}, // decision: Studio Superstar Grand -> CFX Grand
	{0x682204, 0x007604}, // similar: Suitcase String Machine -> Suitcase EP
	{0x682300, 0x68055e}, // similar: Studio Pad Production -> Spectrum Pad
	{0x682350, 0x681051}, // similar: Mini Saw Velocity -> Soft Saw
	{0x682600, 0x6c0102}, // name: Studio Grand -> Studio Grand
	{0x682657, 0x007f51}, // similar: 80s Punchy Synth -> Punchy Hook
	{0x682900, 0x6c0000}, // decision: Studio Grand Pad -> CFX Grand
	{0x682c00, 0x6c0000}, // decision: Stage Grand -> CFX Grand
	{0x682d00, 0x6c0000}, // decision: Mellow Jazz Grand -> CFX Grand
	{0x6c1800, 0x6c0000}, // decision: Warm Grand Pad -> CFX Grand
	{0x6c1a02, 0x6c1902}, // similar: Piano & EP 2 -> Piano & EP
	{0x6c2102, 0x6c0000}, // similar: CFX DX Layer -> CFX Grand
};

static const std::pair<int, int> Cvp709VoiceTable[] = {
	{0x00000f, 0x00700f}, // name: Dulcimer -> Dulcimer
	{0x00745e, 0x68005b}, // similar: Hah Pad -> Breath Pad
	{0x007a08, 0x007008}, // similar: Concert Celesta -> Celesta
	{0x007a0c, 0x68012e}, // similar: Orchestral Marimba -> Orchestral Harp
	{0x007a34, 0x007234}, // similar: Choir -> Hah Choir
	{0x007a72, 0x007072}, // name: Steel Drums -> Steel Drums
	{0x007b00, 0x6c1001}, // name: Bright Piano -> Bright Piano
	{0x007b02, 0x007102}, // similar: CP80 Studio -> CP80 Stage
	{0x007b03, 0x6c1601}, // name: Grand Piano 1octave -> Grand Piano 1octave
	{0x007c00, 0x6c1200}, // name: Mellow Piano -> Mellow Piano
	{0x007c02, 0x6c1002}, // name: Rock Piano -> Rock Piano
	{0x007c03, 0x6c1701}, // name: Grand Piano 2octaves -> Grand Piano 2octaves
	{0x08202b, -1}, // none: Real 1st Violins -> none
	{0x082036, 0x007434}, // similar: Gospel Hmm -> Gospel Voices
	{0x082114, 0x007107}, // similar: Clavi -> Wah Clavi
	{0x082214, 0x007007}, // similar: Clavi Funk -> Clavi Bright
	{0x082227, 0x680835}, // similar: Haa Boys -> Boys Choir Ooh
	{0x082328, 0x680835}, // similar: Ooh Boys Vibrato -> Boys Choir Ooh
	{0x082337, 0x007c30}, // similar: ShooBeeDynamic -> Dynamic Strings
	{0x082501, 0x082300}, // similar: Steel Acoustic Guitar -> Electro Acoustic Guitar
	{0x082504, 0x083703}, // similar: Bluesy Fingers -> Bluesy Night
	{0x082528, 0x680835}, // similar: Ooh Girls Vibrato -> Boys Choir Ooh
	{0x082604, 0x00701b}, // similar: Clean Fingers -> Clean Guitar
	{0x082630, 0x082231}, // similar: Broadway Strings -> Ballad Strings
	{0x082631, 0x68002c}, // similar: Orchestral Layers Horns & Strings -> Real Tremolo Strings
	{0x082837, 0x680132}, // similar: Pop Vocal Ensemble -> Soft Ensemble
	{0x082937, 0x007434}, // decision: Latin Vocals -> Gospel Voices
	{0x082b30, 0x680258}, // similar: Real String Pad -> Pearls Pad
	{0x082b31, 0x082031}, // similar: Romance Strings -> Concert Strings
	{0x082d05, 0x082605}, // similar: Vintage Distortion Stage -> Stage Lead
	{0x084030, 0x007128}, // similar: Jazz Violin -> Violin
	{0x084040, 0x00723b}, // similar: Mute Trumpet -> Muted Trumpet
	{0x084041, 0x007338}, // similar: Jazz Trumpet -> Trumpet
	{0x08404a, 0x007349}, // name: Classical Flute -> Classical Flute
	{0x084050, 0x007542}, // name: Breathy Tenor Sax -> Breathy Tenor Sax
	{0x08405c, 0x007247}, // similar: Ballad Clarinet -> Jazz Clarinet
	{0x084068, 0x007016}, // name: Harmonica -> Harmonica
	{0x08406c, 0x00706d}, // decision: Woodwind World Irish Pipe Air -> Bagpipe
	{0x084130, 0x007028}, // similar: Celtic Violin -> Solo Violin
	{0x084141, 0x007338}, // decision: Classic Trumpet -> Trumpet
	{0x084149, 0x007539}, // name: Trombone -> Trombone
	{0x08414a, 0x007249}, // name: Jazz Flute -> Jazz Flute
	{0x084150, 0x007d42}, // name: Jazz Tenor Sax -> Jazz Tenor Sax
	{0x084154, 0x007140}, // decision: Ballad Soprano -> Soprano Sax
	{0x08415c, 0x007047}, // name: Clarinet -> Clarinet
	{0x084168, 0x007216}, // name: Blues Harp -> Blues Harp
	{0x084241, 0x007338}, // name: Trumpet -> Trumpet
	{0x084250, 0x007f42}, // similar: Tenor Sax -> Pop Tenor Sax
	{0x08425c, 0x680247}, // similar: Romance Clarinet -> German Clarinet
	{0x084341, 0x082540}, // name: Big Band Trumpet -> Big Band Trumpet
	{0x68003d, 0x00753e}, // decision: Pop Horns Dynamic -> Pop Brass
	{0x680213, 0x007313}, // decision: Fonds Et Anches -> Church Organ
	{0x68023d, 0x68023c}, // similar: Swing Horns Mute -> Symphony Horns 2
	{0x680313, 0x007013}, // decision: Organo Pleno -> Full Organ
	{0x680338, 0x006e3d}, // decision: Orchestral Trumpets -> Brass mf
	{0x680513, 0x007349}, // similar: Claribel 8' & Flute 4' -> Classical Flute
	{0x680600, 0x6c1101}, // name: Pop Grand -> Pop Grand
	{0x680a05, 0x007a05}, // decision: TX816 EP Bright -> DX7 EP
	{0x680b05, 0x007a05}, // decision: TX816 EP Mellow -> DX7 EP
	{0x680b3d, 0x00713a}, // similar: Orch Horn Pad -> Baritone Horn
	{0x680c35, 0x007434}, // similar: Gospel Hmm -> Gospel Voices
	{0x680d3d, 0x00763c}, // similar: Orch Horns & Trombones -> Soft Trombones
	{0x680f3d, 0x68023c}, // similar: Symphony Horns Fast -> Symphony Horns 2
	{0x681135, 0x680835}, // similar: Ooh -> Boys Choir Ooh
	{0x681201, 0x6c0601}, // name: Themic Grand -> Themic Grand
	{0x681300, 0x6c0102}, // name: Studio Grand -> Studio Grand
	{0x681303, 0x6c1b03}, // name: Ambient Piano -> Ambient Piano
	{0x681400, 0x6c1f00}, // name: Stage Piano -> Stage Piano
	{0x681403, 0x6c1c03}, // name: Cocktail Piano -> Cocktail Piano
	{0x68195a, 0x68055a}, // decision: Trancy Chordz -> Bright Fat Saw
	{0x6c1800, 0x6c0000}, // decision: Warm Grand Pad -> CFX Grand
};

static const std::pair<int, int> GenosVoiceTable[] = {
	{0x00000f, 0x00700f}, // name: Dulcimer -> Dulcimer
	{0x082007, 0x083803}, // decision: 60sShadowLead -> Shadowed Guitar
	{0x082107, 0x082603}, // similar: 60sBalladGuitar -> 70s Solid Guitar
	{0x082207, 0x082505}, // similar: 60sVintageBlues -> Vintage Blues
	{0x082307, 0x082305}, // similar: 60sRockHero -> Rock Hero
	{0x08232b, 0x007b30}, // decision: KinoStringsViolins -> Movie Strings
	{0x08232d, 0x007c31}, // similar: KinoStringsViolas -> Disco Strings 2
	{0x08232e, 0x007b30}, // decision: KinoStringsCellos -> Movie Strings
	{0x082336, -1}, // none: Wow-Hey-Hmm -> none
	{0x082407, 0x00751b}, // similar: 60sVintageClean -> 60s Clean
	{0x082507, 0x68071b}, // similar: 60sChickenPick -> Slide Pick
	{0x082607, 0x082005}, // similar: 60sGuitarHero -> Guitar Hero
	{0x082714, 0x007604}, // similar: 70sSuitcaseWarm1 -> Suitcase EP
	{0x082814, 0x007404}, // similar: 70sVintageEP -> Vintage EP
	{0x08290c, 0x007219}, // name: Mandolin -> Mandolin
	{0x082914, 0x007604}, // similar: 70sSuitcaseClean -> Suitcase EP
	{0x082937, 0x007434}, // decision: LatinVocals -> Gospel Voices
	{0x082a14, 0x007104}, // similar: 70sSuitcaseTremolo -> Tremolo Suitcase EP
	{0x082a38, 0x00713a}, // similar: BrightHornSection -> Baritone Horn
	{0x082c07, 0x00731c}, // similar: 60sVintageEcho -> Vintage Muted Echo
	{0x082c14, 0x007604}, // similar: 70sSuitcaseBallad -> Suitcase EP
	{0x082c31, 0x00712c}, // similar: KinoStringsTremolo -> Tremolo Strings
	{0x082d01, 0x082000}, // similar: ResonatorGuitar -> Concert Guitar
	{0x082d14, 0x007604}, // similar: 70sSuitcasePan -> Suitcase EP
	{0x082d31, 0x007c31}, // similar: KinoStringsVnVaVc -> Disco Strings 2
	{0x082e01, 0x007718}, // similar: ResonatorMuteVel -> Nylon Mute
	{0x082e07, 0x007819}, // similar: 60sShadowMute -> Steel Guitar Mute
	{0x082e14, 0x007604}, // similar: 70sSuitcaseAmped -> Suitcase EP
	{0x082e31, 0x007c31}, // similar: KinoStringsNatural -> Disco Strings 2
	{0x082f07, 0x007819}, // similar: 60sCountryMute -> Steel Guitar Mute
	{0x082f14, 0x007604}, // similar: 70sSuitcasePhase -> Suitcase EP
	{0x082f31, 0x007b30}, // decision: KinoStrings -> Movie Strings
	{0x083007, 0x082c03}, // similar: 60sFunkPick -> Country Pick
	{0x083031, 0x007b30}, // decision: KinoStringsVnVc -> Movie Strings
	{0x083101, 0x082001}, // decision: SteelAcousticFinger -> Steel Guitar
	{0x083114, 0x680e00}, // similar: 70sVintageDriveEP -> 70s Ballad Piano
	{0x083131, 0x007b30}, // decision: KinoStrings1Oct -> Movie Strings
	{0x083331, 0x007b30}, // decision: KinoStringsWarm -> Movie Strings
	{0x083431, 0x007b30}, // decision: KinoStringsLow -> Movie Strings
	{0x083531, 0x007c31}, // similar: KinoStrings2Oct -> Disco Strings 2
	{0x083601, 0x68071b}, // similar: SteelThumbPick -> Slide Pick
	{0x083631, 0x007b30}, // decision: KinoStringsWarmVc -> Movie Strings
	{0x083731, 0x007c31}, // similar: KinoStringsSlow -> Disco Strings 2
	{0x083801, 0x083403}, // similar: SteelAcoustAmp -> Pedal Steel Amp
	{0x083d03, 0x082001}, // similar: PedalSteelGuitar -> Steel Guitar
	{0x084030, 0x007128}, // similar: JazzViolin -> Violin
	{0x084032, 0x00702a}, // decision: ClassicalCello -> Cello
	{0x08403e, 0x007638}, // name: Flugelhorn -> Flugel Horn
	{0x084040, 0x00723b}, // similar: MuteTrumpet -> Muted Trumpet
	{0x084041, 0x007338}, // similar: SoftTrumpet -> Trumpet
	{0x084044, 0x007044}, // similar: PopOboe -> Oboe
	{0x08404a, 0x007349}, // name: ClassicalFlute -> Classical Flute
	{0x084050, 0x007542}, // name: BreathyTenorSax -> Breathy Tenor Sax
	{0x084053, 0x082242}, // similar: FunkAltoSax -> Soft Alto Sax
	{0x084054, 0x007140}, // similar: PopSopranoSax -> Soprano Sax
	{0x084068, 0x007016}, // name: Harmonica -> Harmonica
	{0x084130, 0x007028}, // similar: CelticViolin -> Solo Violin
	{0x084132, 0x00702a}, // similar: PopCello -> Cello
	{0x084141, 0x007338}, // decision: ClassicTrumpet -> Trumpet
	{0x084144, 0x007144}, // name: ClassicalOboe -> Classical Oboe
	{0x084146, 0x007046}, // decision: ClassicalBassoon -> Bassoon
	{0x084149, 0x007539}, // name: Trombone -> Trombone
	{0x08414a, 0x007249}, // name: JazzFlute -> Jazz Flute
	{0x084150, 0x007f42}, // similar: SmoothTenorSax -> Pop Tenor Sax
	{0x084151, 0x680043}, // name: BaritoneSax -> Baritone Sax
	{0x084153, 0x082042}, // name: AltoSax -> Alto Sax
	{0x084154, 0x007140}, // similar: BalladSopranoSax -> Soprano Sax
	{0x08415c, 0x007047}, // name: Clarinet -> Clarinet
	{0x084168, 0x007016}, // similar: BluesHarmonica -> Harmonica
	{0x084241, 0x007338}, // name: Trumpet -> Trumpet
	{0x084250, 0x007f42}, // similar: TenorSax -> Pop Tenor Sax
	{0x084252, 0x082352}, // similar: BigBandTenorSax -> Big Band Sax
	{0x084253, 0x082352}, // similar: BigBandAltoSax -> Big Band Sax
	{0x08425c, 0x680247}, // similar: RomanceClarinet -> German Clarinet
	{0x084341, 0x082540}, // name: BigBandTrumpet -> Big Band Trumpet
	{0x680103, 0x6c0000}, // similar: CFX CocktailGrand -> CFX Grand
	{0x680203, 0x6c0000}, // similar: CFX OneOctave -> CFX Grand
	{0x680229, 0x007c31}, // similar: KinoStringsSpicVa -> Disco Strings 2
	{0x680303, 0x6c0000}, // similar: CFX TwoOctaves -> CFX Grand
	{0x680417, 0x007815}, // similar: BajanAccordion -> Jazz Accordion
	{0x680900, 0x6c0000}, // similar: CFX AmbientGrand -> CFX Grand
	{0x680a00, 0x6c0000}, // similar: CFX StageGrand -> CFX Grand
	{0x680a02, 0x007102}, // similar: CP80 ElectricGrand -> CP80 Stage
	{0x680b02, 0x007102}, // similar: CP80 Production -> CP80 Stage
	{0x680b05, 0x6c1200}, // similar: TX Mellow -> Mellow Piano
	{0x680c19, 0x007819}, // similar: SteelGuitarTwin1 -> Steel Guitar Mute
	{0x680c35, 0x007434}, // similar: GospelHmm -> Gospel Voices
	{0x680d19, 0x007819}, // similar: SteelGuitarTwin2 -> Steel Guitar Mute
	{0x681305, 0x6c0000}, // similar: CFX BalladMix1 -> CFX Grand
	{0x681318, 0x007219}, // similar: MandolinTremolo -> Mandolin
	{0x681430, 0x007c31}, // similar: KinoStringsSpicSct -> Disco Strings 2
	{0x681500, 0x6c0000}, // similar: CFX ConcertGrand -> CFX Grand
	{0x681600, 0x6c0102}, // similar: C7 StudioGrand -> Studio Grand
	{0x681605, 0x6c0000}, // similar: CFX BalladMix2 -> CFX Grand
	{0x681900, 0x6c0000}, // similar: CFX WarmPad -> CFX Grand
	{0x681915, 0x007715}, // similar: SuperMusette -> French Musette Acc.
	{0x681a50, 0x680f51}, // similar: FusionWire -> Fusion Lead
	{0x681b15, 0x007515}, // similar: SteirischeHarmonika -> Steirische
	{0x681b50, 0x007c50}, // similar: MiniClassicUnison -> Soft Mini
	{0x681c15, 0x680015}, // similar: JazzCassotto -> Cassotto
	{0x681c50, 0x007c50}, // similar: MiniClassic -> Soft Mini
	{0x681d50, 0x007c50}, // similar: MiniClassicVelo+ -> Soft Mini
	{0x681e00, 0x6c0000}, // similar: CFX PadProduction -> CFX Grand
	{0x681e50, 0x007c50}, // similar: MiniClassicSoft -> Soft Mini
	{0x681f50, 0x007c50}, // similar: MiniClassicOpen -> Soft Mini
	{0x682150, 0x007c50}, // similar: FunkyVeloMini -> Soft Mini
	{0x682200, 0x6c1904}, // similar: C7 SuperstarGrand -> Superstar Grand
	{0x682350, 0x681051}, // similar: MiniSawVelo -> Soft Saw
};

static const std::pair<int, int> S950VoiceTable[] = {
	{0x000047, 0x007047}, // name: Clarinet -> Clarinet
	{0x007015, 0x007b15}, // similar: Musette -> Musette Bass
	{0x00703e, 0x00743f}, // similar: SynthBrass -> Fat Synth Brass
	{0x007241, 0x082042}, // name: AltoSax -> Alto Sax
	{0x007330, 0x082031}, // similar: ConcertoStrings -> Concert Strings
	{0x007430, 0x082830}, // similar: BowStrings -> Big Strings
	{0x007951, 0x680f51}, // similar: FunkyLead -> Fusion Lead
	{0x007d50, 0x007c50}, // similar: BrightMini -> Soft Mini
	{0x007e10, 0x0a0008}, // name: OrganFlutes -> Organ Flutes
	{0x007e51, 0x681051}, // similar: AttackSaw -> Soft Saw
};

namespace
{
	struct SourceVoiceTable
	{
		const char* source; // folder of the model (see Sources)
		const std::pair<int, int>* table;
		size_t size;
	};
}
static const SourceVoiceTable SourceVoiceTables[] = {
	{"CVP-809", Cvp809VoiceTable, std::size(Cvp809VoiceTable)},
	{"CVP-709", Cvp709VoiceTable, std::size(Cvp709VoiceTable)},
	{"Genos", GenosVoiceTable, std::size(GenosVoiceTable)},
	{"PSR-S950", S950VoiceTable, std::size(S950VoiceTable)},
};

// The style file of each style of the CSP-170 (generated from the same file): the one with
// the most OTS usable on the CSP-170, then with the most Main and Layer voices that are on
// it, then with the most Left voices that are on it, then the first one in the order of
// Sources. Easy 8Beat 1 and 2 have the Easy 8Beat of the CVP-809, 70s Pop Duo the 70s Pop
// Duo 1 (chosen by Viktor; no file of the same name).
// Paths with "/" in the folder of the model.
namespace
{
	struct StyleSource
	{
		const char* style;  // title of the style of the CSP-170
		const char* source; // folder of the model (see Sources)
		const char* file;   // style file in it
	};
}
static const StyleSource StyleSources[] = {
	{"Contemp Gtr Pop", "PSR-SX700", "Pop&Rock/ContempGtrPop.T547.prs"},
	{"Standard 8Beat", "CVP-709", "Pop&Rock/Pop/Standard 8Beat.T308.prs"},
	{"West Coast Beat", "CVP-809", "Pop&Rock/Pop/West Coast Beat.T308.prs"},
	{"Cool 8Beat", "PSR-SX700", "Dance/Cool8Beat.T549.prs"},
	{"80s American Pop", "CVP-809", "Pop&Rock/Pop/80s American Pop.T308.prs"},
	{"Live 8Beat", "PSR-S970", "Pop&Rock/Live8Beat.T160.prs"},
	{"80s Guitar Pop", "CVP-709", "Pop&Rock/Pop/80s Guitar Pop.T308.prs"},
	{"Classic 8Beat", "PSR-SX600", "Pop & Rock/Classic8Beat.T547.prs"},
	{"80s Pop", "CVP-709", "Pop&Rock/Pop/80s Pop.T308.prs"},
	{"Chart Piano Shuffle", "PSR-SX700", "Pop&Rock/ChartPianoShuffle.T547.prs"},
	{"Chart Rock Shuffle", "PSR-SX600", "Pop & Rock/ChartRockShuffle.T547.prs"},
	{"80s Boy Band", "PSR-SX700", "Dance/80sBoyBand.T549.prs"},
	{"Chart Guitar Pop", "CVP-709", "Pop&Rock/Pop/Chart Guitar Pop.T308.prs"},
	{"90s Guitar Pop", "PSR-SX700", "Pop&Rock/90sGuitarPop.T547.prs"},
	{"Easy Pop", "CVP-709", "Pop&Rock/Pop/Easy Pop.T308.prs"},
	{"Chart Pop", "PSR-SX600", "Dance & R&B/ChartPop.T551.prs"},
	{"Straight 8 Pop", "CVP-709", "Pop&Rock/Pop/Straight 8 Pop.T308.prs"},
	{"Scand Pop Shuffle", "PSR-S970", "Pop&Rock/ScandPopShuffle.T160.prs"},
	{"Uptempo 8Beat", "PSR-SX700", "Dance/Up-tempo8Beat.T549.prs"},
	{"Brit Pop", "PSR-SX600", "Pop & Rock/BritPop.T547.prs"},
	{"8Beat Modern", "CVP-809", "USA_Styles/8BeatModern.T160.sty"},
	{"Brit Pop Swing", "PSR-SX700", "Pop&Rock/BritPopSwing.T547.prs"},
	{"West Coast Pop", "PSR-SX700", "Pop&Rock/WestCoastPop.T547.prs"},
	{"8Beat Guitar Pop", "CVP-709", "Pop&Rock/Pop/8Beat Guitar Pop.T308.prs"},
	{"Classic 16Beat", "PSR-S950", "Pop&Rock/Classic16Beat.T160.prs"},
	{"8Beat", "CVP-709", "Pop&Rock/Pop/8Beat.T308.pst"},
	{"Standard Rock", "CVP-709", "Pop&Rock/Rock/Standard Rock.T310.prs"},
	{"80s Power Rock", "PSR-SX600", "Pop & Rock/80sPowerRock.T548.prs"},
	{"Contemp Rock", "PSR-S970", "Pop&Rock/ContempRock.T162.prs"},
	{"80s Pop Rock", "CVP-709", "Pop&Rock/Rock/80s Pop Rock.T310.prs"},
	{"Funk Pop Rock", "CVP-809", "USA_Styles/Funk Pop Rock.T310.sty"},
	{"80s Synth Rock", "PSR-SX700", "Pop&Rock/80sSynthRock.T548.prs"},
	{"Acoustic Rock", "PSR-SX700", "Pop&Rock/AcousticRock.T548.prs"},
	{"Brit Rock Pop", "CVP-709", "Pop&Rock/Rock/Brit Rock Pop.T310.prs"},
	{"Soft Rock", "PSR-S950", "Pop&Rock/SoftRock.T162.prs"},
	{"Slow Rock", "CVP-709", "Pop&Rock/Rock/Slow Rock.T310.pst"},
	{"70s Piano Hit", "CVP-809", "Pop&Rock/70's Pop&Rock/70s Piano Hit.T281.prs"},
	{"70s Piano Rock", "CVP-709", "Pop&Rock/70s Pop&Rock/70s Piano Rock.T281.prs"},
	{"70s Pop Duo", "CVP-709", "Pop&Rock/70s Pop&Rock/70s Pop Duo 1.T281.prs"},
	{"70s Glam Piano", "PSR-S970", "Ballad/70sGlamPiano.T160.prs"},
	{"70s Swing Piano", "CVP-709", "Pop&Rock/70s Pop&Rock/70s Swing Piano.T281.prs"},
	{"Glam Piano Rock", "CVP-709", "Pop&Rock/70s Pop&Rock/Glam Piano Rock.T281.prs"},
	{"70s 8Beat", "PSR-SX600", "Pop & Rock/70s8Beat.T547.prs"},
	{"70s Rock", "CVP-709", "Pop&Rock/70s Pop&Rock/70s Rock.T281.sst"},
	{"Vintage Guitar Pop", "PSR-SX900", "R&B/VintageGuitarPop.T550.prs"},
	{"60s Big Hit", "PSR-SX700", "R&B/60sBigHit.T550.prs"},
	{"60s Rising Pop", "PSR-SX700", "R&B/60sRisingPop.T550.prs"},
	{"60s Guitar Pop", "PSR-SX600", "Pop & Rock/60sGuitarPop.T547.prs"},
	{"60s Vintage Pop", "PSR-SX900", "R&B/60sVintagePop.T550.prs"},
	{"60s Chart Swing", "PSR-SX700", "R&B/60sChartSwing.T550.prs"},
	{"60s Piano Pop", "PSR-SX900", "R&B/60sPianoPop.T550.prs"},
	{"60s Rock & Roll", "CVP-709", "Pop&Rock/60sPop&Rock/60s Rock & Roll.T280.prs"},
	{"60s Pop Rock", "PSR-SX700", "R&B/60sPopRock.T550.prs"},
	{"Bubblegum Pop", "PSR-SX600", "Pop & Rock/BubblegumPop.T547.prs"},
	{"60s Vintage Rock", "PSR-SX600", "Pop & Rock/60sVintageRock.T548.prs"},
	{"60s 8Beat", "PSR-SX700", "R&B/60s8Beat.T550.prs"},
	{"Vintage R&R", "CVP-809", "Pop&Rock/50's Rock&Roll/Vintage R&R.T279.prs"},
	{"Oldies R&R", "PSR-SX600", "Dance & R&B/OldiesR&R.T552.prs"},
	{"Rock & Roll 1", "PSR-SX600", "Dance & R&B/Rock&Roll1.T552.sst"},
	{"Rock & Roll 2", "PSR-SX600", "Dance & R&B/Rock&Roll2.T552.prs"},
	{"Skiffle", "CVP-709", "Pop&Rock/50sRock&Roll/Skiffle.T279.prs"},
	{"Piano R&R", "CVP-709", "Pop&Rock/50sRock&Roll/Piano R&R.T279.pst"},
	{"Rock & Roll Shfl", "PSR-S970", "R&B/Rock&RollShfl.T161.sst"},
	{"Easy 8Beat 1", "CVP-809", "Pop&Rock/Pop&Rock Ballad/Easy 8Beat.T311.prs"},
	{"Easy 8Beat 2", "CVP-809", "Pop&Rock/Pop&Rock Ballad/Easy 8Beat.T311.prs"},
	{"6-8 Soul Ballad", "CVP-809", "Pop&Rock/Pop&Rock Ballad/6-8 Soul Ballad.T311.prs"},
	{"6-8 Ballad Rock", "PSR-SX700", "Pop&Rock/6-8BalladRock.T548.prs"},
	{"90s Piano Ballad", "CVP-809", "Pop&Rock/Pop&Rock Ballad/90s Piano Ballad.T311.prs"},
	{"Standard Piano Bld", "CVP-809", "Pop&Rock/Pop&Rock Ballad/Standard Piano Bld.T311.prs"},
	{"Smooth Ballad", "CVP-809", "Pop&Rock/Pop&Rock Ballad/Smooth Ballad.T311.prs"},
	{"70s Piano Pop Bld", "CVP-709", "Pop&Rock/Pop&Rock Ballad/70s Piano Pop Bld.T311.prs"},
	{"Mod 6-8 Piano Bld", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Mod 6-8 Piano Bld.T311.prs"},
	{"Orch Rock Ballad", "PSR-S970", "Pop&Rock/OrchRockBallad.T162.prs"},
	{"Pop Ballad", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Pop Ballad.T311.prs"},
	{"Pop Guitar Ballad", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Pop Guitar Ballad.T311.prs"},
	{"Epic Ballad", "PSR-S970", "Ballad/EpicBallad.T153.prs"},
	{"Modern Pop Ballad", "PSR-SX700", "Pop&Rock/ModernPopBallad.T547.prs"},
	{"70s Chart Ballad", "PSR-SX700", "Jazz/70sChartBallad.T556.prs"},
	{"80s Movie Ballad", "PSR-SX700", "Movie&Show/80sMovieBallad.T559.prs"},
	{"80s Synth Piano", "CVP-709", "Pop&Rock/Pop&Rock Ballad/80s Synth Piano.T311.prs"},
	{"80s Smooth Ballad", "PSR-SX700", "R&B/80sSmoothBallad.T552.prs"},
	{"90s Cool Ballad", "PSR-SX600", "Pop & Rock/90sCoolBallad.T547.prs"},
	{"80s Analog Ballad", "PSR-S970", "Ballad/80sAnalogBallad.T162.prs"},
	{"90s Rock Ballad", "PSR-SX600", "Pop & Rock/90sRockBallad.T548.prs"},
	{"80s EP Ballad", "PSR-SX600", "Pop & Rock/80sEP Ballad.T547.prs"},
	{"Power Ballad 1", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Power Ballad 1.T311.prs"},
	{"Power Ballad 2", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Power Ballad 2.T311.prs"},
	{"8Beat Ballad 1", "PSR-SX600", "Pop & Rock/8BeatBallad1.T547.prs"},
	{"8Beat Ballad 2", "PSR-S970", "Ballad/8BeatBallad2.T160.prs"},
	{"MOR 16Beat", "CVP-709", "Pop&Rock/Pop&Rock Ballad/MOR 16Beat.T311.prs"},
	{"Acoustic Gtr Ballad", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Acoustic Gtr Ballad.T311.prs"},
	{"Easy Acoustc Bld", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Easy Acoustc Bld.T311.prs"},
	{"Organ Ballad", "PSR-SX600", "Pop & Rock/OrganBallad.T547.prs"},
	{"Pop Piano Ballad", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Pop Piano Ballad.T311.prs"},
	{"EP Ballad", "PSR-SX600", "Pop & Rock/EP Ballad.T547.prs"},
	{"16Beat Pop", "CVP-709", "Pop&Rock/Pop&Rock Ballad/16Beat Pop.T311.prs"},
	{"Contemp Rock Bld", "PSR-S970", "Pop&Rock/ContempRockBld.T162.prs"},
	{"16Beat Ballad", "PSR-SX700", "Pop&Rock/16BeatBallad.T547.prs"},
	{"Modern 16Bt Ballad", "PSR-SX600", "Pop & Rock/Modern16BtBallad.T547.prs"},
	{"Contemp Pop Bld", "PSR-S970", "Ballad/ContempPopBld.T160.prs"},
	{"Acoustic Ballad", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Acoustic Ballad.T311.prs"},
	{"6-8 Modern", "PSR-SX600", "Pop & Rock/6-8Modern.T547.prs"},
	{"Love Song", "PSR-SX700", "Pop&Rock/LoveSong.T547.prs"},
	{"Pop Waltz", "PSR-SX700", "Dance/PopWaltz.T549.prs"},
	{"Slow Orch Ballad", "CVP-709", "Pop&Rock/Pop&Rock Ballad/Slow Orch Ballad.T311.prs"},
	{"Piano Ballad", "PSR-SX600", "Pop & Rock/PianoBallad.T547.prs"},
	{"8Beat Piano Ballad", "CVP-709", "Pop&Rock/Pop&Rock Ballad/8Beat Piano Ballad.T311.pst"},
	{"Worship Slow", "PSR-SX600", "Dance & R&B/WorshipSlow.T552.prs"},
	{"Worship 6-8", "PSR-SX700", "R&B/Worship6-8.T552.prs"},
	{"Worship Medium", "PSR-SX600", "Dance & R&B/WorshipMedium.T552.prs"},
	{"Worship Fast", "PSR-SX600", "Dance & R&B/WorshipFast.T552.sst"},
	{"Worship Power Bld", "CVP-709", "Pop&Rock/Worship/Worship Power Bld.T319.prs"},
	{"Funky Shuffle", "PSR-SX700", "R&B/FunkyShuffle.T552.prs"},
	{"Detroit Pop 1", "PSR-SX600", "Dance & R&B/DetroitPop1.T552.prs"},
	{"Detroit Pop 2", "PSR-SX600", "Dance & R&B/DetroitPop2.T552.prs"},
	{"70s Cool Shuffle", "CVP-809", "Pop&Rock/Soul/70s Cool Shuffle.T313.sst"},
	{"70s Chart Soul", "PSR-SX600", "Dance & R&B/70sChartSoul.T552.prs"},
	{"Motor City", "PSR-SX700", "R&B/MotorCity.T552.prs"},
	{"Frankly Soul", "PSR-SX700", "R&B/FranklySoul.T552.sst"},
	{"Live Soul Band", "PSR-SX700", "R&B/LiveSoulBand.T550.prs"},
	{"Kool Funk", "PSR-SX600", "Dance & R&B/KoolFunk.T552.sst"},
	{"Soul", "PSR-SX600", "Dance & R&B/Soul.T552.prs"},
	{"Kool Shuffle", "PSR-SX700", "R&B/KoolShuffle.T552.prs"},
	{"Soul Shuffle", "CVP-809", "Pop&Rock/Soul/Soul Shuffle.T313.prs"},
	{"Lovely Shuffle", "PSR-SX700", "R&B/LovelyShuffle.T552.prs"},
	{"Soul Beat", "CVP-709", "Pop&Rock/Soul/Soul Beat.T313.prs"},
	{"Modern Hip Hop", "PSR-S970", "Dance/ModernHipHop.T155.prs"},
	{"R&B Soul Ballad", "PSR-SX700", "R&B/R&B SoulBallad.T552.sst"},
	{"Euro Hip Hop", "PSR-S950", "Dance/EuroHipHop.T155.prs"},
	{"New R&B Ballad", "PSR-SX600", "Pop & Rock/NewR&B Ballad.T547.prs"},
	{"Soul R&B", "PSR-SX600", "Pop & Rock/SoulR&B.T547.prs"},
	{"Classic Hip Hop", "PSR-SX600", "Dance & R&B/ClassicHipHop.T551.prs"},
	{"Chart R&B", "PSR-S950", "Dance/ChartR&B.T161.prs"},
	{"New R&B", "PSR-SX600", "Dance & R&B/NewR&B.T551.prs"},
	{"Analog Ballad", "PSR-S970", "Ballad/AnalogBallad.T160.prs"},
	{"Global DJ's", "PSR-SX600", "Dance & R&B/GlobalDJs.T551.prs"},
	{"Clubdance 1", "PSR-SX700", "Dance/ClubDance1.T551.prs"},
	{"Clubdance 2", "PSR-S950", "Dance/Clubdance2.T152.prs"},
	{"Ibiza 2010", "PSR-S950", "Dance/Ibiza2010.T152.prs"},
	{"Club House", "PSR-S950", "Dance/ClubHouse.T152.prs"},
	{"Dream Dance", "PSR-S950", "Dance/DreamDance.T152.prs"},
	{"Electronica", "PSR-SX600", "Dance & R&B/Electronica.T551.prs"},
	{"Retro Pop", "CVP-709", "Pop&Rock/Dance/Retro Pop.T292.prs"},
	{"Funky House", "PSR-SX700", "Dance/FunkyHouse.T551.prs"},
	{"Dancefloor", "PSR-S950", "Dance/Dancefloor.T152.prs"},
	{"Funk Disco", "PSR-SX600", "Dance & R&B/FunkDisco.T551.prs"},
	{"70s Disco 1", "PSR-SX700", "Dance/70sDisco1.T549.prs"},
	{"70s Disco 2", "PSR-SX600", "Dance & R&B/70sDisco2.T551.prs"},
	{"80s Disco", "PSR-SX600", "Dance & R&B/80sDisco.T551.prs"},
	{"90s Disco", "PSR-SX700", "Dance/90sDisco.T551.prs"},
	{"Disco Philly", "PSR-S970", "Dance/DiscoPhilly.T161.prs"},
	{"70s Disco Funk", "PSR-SX700", "Dance/70sDiscoFunk.T549.prs"},
	{"Disco House", "PSR-SX600", "Dance & R&B/DiscoHouse.T551.prs"},
	{"Saturday Night", "PSR-SX600", "Entertainment/SaturdayNight.T611.prs"},
	{"Synth Pop", "PSR-SX700", "Dance/SynthPop.T549.prs"},
	{"Disco Fox Rock", "CVP-709", "Pop&Rock/Disco/Disco Fox Rock.T293.prs"},
	{"80s Synth Disco", "PSR-SX600", "Dance & R&B/80sSynthDisco.T551.prs"},
	{"Movie Disco", "PSR-S950", "Movie&Show/MovieDisco.T163.prs"},
	{"Disco Chocolate", "PSR-SX700", "Dance/DiscoChocolate.T549.prs"},
	{"Chillout 1", "PSR-SX700", "Dance/Chillout1.T551.prs"},
	{"Chillout 2", "PSR-SX700", "Dance/Chillout2.T551.prs"},
	{"Coudy Bay", "CVP-709", "Pop&Rock/Chill Out/Coudy Bay.T288.prs"},
	{"Night Walk", "PSR-SX700", "Dance/NightWalk.T551.prs"},
	{"Chillout Cafe", "PSR-SX700", "Dance/ChilloutCafe.T551.prs"},
	{"Play 4 Sofa", "PSR-SX700", "Dance/Play4Sofa.T551.prs"},
	{"Ethereal Synth", "CVP-709", "Pop&Rock/Chill Out/Ethereal Synth.T288.fps"},
	{"Angel Sun", "PSR-SX900", "Dance/AngelSun.T551.prs"},
	{"Celtic Trance", "PSR-S950", "Dance/CelticTrance.T152.prs"},
	{"6-8 Trance", "PSR-SX600", "Dance & R&B/6-8Trance.T551.prs"},
	{"Euro Trance", "PSR-SX700", "Dance/EuroTrance.T551.prs"},
	{"Easy Country Pop", "CVP-809", "County&Blues/Modern Country/Easy Country Pop.T305.prs"},
	{"70s Country Pop", "PSR-SX600", "Country & Ballroom/70sCountryPop.T554.prs"},
	{"Country Blues", "PSR-SX700", "R&B/CountryBlues.T552.sst"},
	{"70s Chart Country", "PSR-SX600", "Country & Ballroom/70sChartCountry.T554.prs"},
	{"Modern Pickin'", "PSR-SX700", "Pop&Rock/ModernPickin'.T554.prs"},
	{"New Country", "PSR-SX600", "Country & Ballroom/NewCountry.T554.prs"},
	{"Country Strum", "PSR-SX600", "Country & Ballroom/CountryStrum.T554.prs"},
	{"Modern Bluegrass", "PSR-SX600", "Country & Ballroom/ModernBluegrass.T554.prs"},
	{"Country Hits", "PSR-SX700", "Pop&Rock/CountryHits.T554.prs"},
	{"Country Brothers", "PSR-SX600", "Country & Ballroom/CountryBrothers.T554.prs"},
	{"Country 8Beat", "PSR-SX600", "Country & Ballroom/Country8Beat.T554.prs"},
	{"Modern Cntry Pop", "PSR-S970", "Country/ModernCntryPop.T151.prs"},
	{"Country Pop", "CVP-709", "Country&Blues/Modern Country/Country Pop.T305.prs"},
	{"Country Rock", "PSR-SX700", "Pop&Rock/CountryRock.T554.prs"},
	{"Hully Gully", "PSR-SX600", "World/HullyGully.T589.prs"},
	{"70s Country Swing", "CVP-809", "County&Blues/Trad Country/70s Country Swing.T315.prs"},
	{"Country Swing", "PSR-SX700", "Jazz/CountrySwing.T556.prs"},
	{"Pickin' Swing", "CVP-709", "Country&Blues/Trad Country/Pickin' Swing.T315.prs"},
	{"Easy Country", "PSR-SX600", "Country & Ballroom/EasyCountry.T554.prs"},
	{"Country Shuffle", "PSR-SX700", "Pop&Rock/CountryShuffle.T554.prs"},
	{"Bluegrass", "PSR-SX700", "World/Bluegrass.T589.prs"},
	{"Country Sing-along", "PSR-SX700", "Pop&Rock/CountrySingalong.T554.prs"},
	{"Hoedown", "PSR-S970", "Country/Hoedown.T151.prs"},
	{"Standard Swing", "CVP-709", "Country&Blues/Trad Country/Standard Swing.T315.prs"},
	{"Country 2-4", "PSR-SX700", "Pop&Rock/Country2-4.T554.prs"},
	{"Country Two Step", "PSR-SX600", "Country & Ballroom/CountryTwoStep.T554.prs"},
	{"Folk Pop", "PSR-SX700", "World/FolkPop.T589.prs"},
	{"Slow Country Pop", "CVP-809", "County&Blues/Country Ballad/Slow Country Pop.T290.prs"},
	{"Mod Country Bld 1", "PSR-S970", "Country/ModCountryBld1.T151.prs"},
	{"Mod Country Bld 2", "PSR-S970", "Country/ModCountryBld2.T151.sst"},
	{"Country Rock Bld", "PSR-S970", "Country/CountryRockBld.T151.prs"},
	{"Country Waltz", "PSR-SX700", "World/CountryWaltz.T589.prs"},
	{"Country Pop Ballad", "CVP-709", "Country&Blues/Country Ballad/Country Pop Ballad.T290.prs"},
	{"The Blues", "CVP-709", "Country&Blues/Blues/The Blues.T284.prs"},
	{"Modern Shuffle", "PSR-SX600", "Dance & R&B/ModernShuffle.T552.sst"},
	{"Slow Blues", "PSR-SX900", "R&B/SlowBlues.T552.sst"},
	{"Shuffle Blues", "PSR-SX600", "Dance & R&B/ShuffleBlues.T552.prs"},
	{"6-8 Rock", "CVP-709", "Country&Blues/Blues/6-8 Rock.T284.prs"},
	{"Blueberry Blues", "PSR-SX700", "R&B/BlueberryBlues.T550.prs"},
	{"6-8 Soul", "PSR-SX600", "Dance & R&B/6-8Soul.T552.prs"},
	{"Blues Rock", "PSR-SX700", "R&B/BluesRock.T552.sst"},
	{"Piano Blues 1", "CVP-709", "Country&Blues/Blues/Piano Blues 1.T284.pst"},
	{"Piano Blues 2", "CVP-709", "Country&Blues/Blues/Piano Blues 2.T284.pst"},
	{"Gospel Funk", "CVP-709", "Country&Blues/Gospel/Gospel Funk.T298.sst"},
	{"Gospel Brothers", "PSR-SX700", "R&B/GospelBrothers.T552.prs"},
	{"Gospel Sisters", "PSR-SX600", "Dance & R&B/GospelSisters.T552.prs"},
	{"Gospel Swing", "PSR-SX700", "R&B/GospelSwing.T552.sst"},
	{"Southern Gospel", "CVP-709", "Country&Blues/Gospel/Southern Gospel.T298.sst"},
	{"Cool Jazz Club", "CVP-809", "Standards&Jazz/Jazz Combo/Cool Jazz Club.T300.prs"},
	{"Jazz Guitar Club", "CVP-709", "Standards&Jazz/Jazz Combo/Jazz Guitar Club.T300.sst"},
	{"Slow Jazz Ballad", "CVP-809", "Standards&Jazz/Jazz Combo/Slow Jazz Ballad.T300.prs"},
	{"Instrumental Jazz", "CVP-709", "Standards&Jazz/Jazz Combo/Instrumental Jazz.T300.sst"},
	{"Swing Medium", "CVP-809", "Standards&Jazz/Jazz Combo/Swing Medium.T300.sst"},
	{"Fast Jazz", "PSR-SX700", "Jazz/FastJazz.T555.sst"},
	{"Trad Piano Jazz", "PSR-SX900", "Jazz/TradPianoJazz.T555.prs"},
	{"Acoustic Jazz", "PSR-SX600", "Latin & Jazz/AcousticJazz.T555.sst"},
	{"Trad Piano Ballad", "CVP-709", "Standards&Jazz/Jazz Combo/Trad Piano Ballad.T300.prs"},
	{"Cool Piano Jazz", "PSR-SX700", "Jazz/CoolPianoJazz.T555.sst"},
	{"French Jazz", "PSR-SX900", "Jazz/FrenchJazz.T555.prs"},
	{"Modern Jazz Bld 1", "CVP-709", "Standards&Jazz/Jazz Combo/Modern Jazz Bld 1.T300.prs"},
	{"Modern Jazz Bld 2", "CVP-709", "Standards&Jazz/Jazz Combo/Modern Jazz Bld 2.T300.prs"},
	{"Organ Combo", "PSR-S950", "Swing&Jazz/OrganCombo.T157.sst"},
	{"Piano Boogie", "PSR-SX900", "R&B/PianoBoogie.T550.sst"},
	{"Jazz Club", "PSR-S970", "Swing&Jazz/JazzClub.T157.prs"},
	{"Bebop", "CVP-709", "Standards&Jazz/Jazz Combo/Bebop.T300.sst"},
	{"Boogie Woogie", "CVP-709", "Standards&Jazz/Jazz Combo/Boogie Woogie.T300.pst"},
	{"Piano Swing 1", "CVP-709", "Standards&Jazz/Jazz Combo/Piano Swing 1.T300.pst"},
	{"Piano Swing 2", "CVP-709", "Standards&Jazz/Jazz Combo/Piano Swing 2.T300.pst"},
	{"Jazz Ballad", "CVP-709", "Standards&Jazz/Jazz Combo/Jazz Ballad.T300.pst"},
	{"2Beat Swing", "CVP-809", "Standards&Jazz/Jazz Combo/2Beat Swing.T300.pst"},
	{"Big Band Jazz", "PSR-SX900", "Jazz/BigBandJazz.T555.sst"},
	{"Orch Big Band 1", "PSR-S970", "Swing&Jazz/OrchBigBand1.T157.prs"},
	{"Orch Big Band 2", "PSR-S970", "Swing&Jazz/OrchBigBand2.T157.prs"},
	{"Modern Big Band", "PSR-SX700", "Jazz/ModernBigBand.T555.prs"},
	{"Mod Big Band Shfl", "PSR-S970", "Swing&Jazz/ModBigBandShfl.T157.sst"},
	{"Big Band Fast 1", "PSR-S950", "Swing&Jazz/BigBandFast1.T157.sst"},
	{"Big Band Fast 2", "PSR-SX600", "Latin & Jazz/BigBandFast2.T555.prs"},
	{"Mod Big Band Bld", "PSR-S950", "Swing&Jazz/ModBigBandBld.T157.prs"},
	{"Classic Big Band", "PSR-SX600", "Latin & Jazz/ClassicBigBand.T555.prs"},
	{"Afro Cuban", "PSR-SX700", "Jazz/AfroCuban.T555.prs"},
	{"Jump Jive", "PSR-SX700", "Jazz/JumpJive.T555.prs"},
	{"40s Big Band", "CVP-709", "Standards&Jazz/Big Band/40s Big Band.T283.sst"},
	{"Moonlight Ballad", "PSR-SX600", "Latin & Jazz/MoonlightBallad.T555.prs"},
	{"Easy Swing", "PSR-SX700", "Jazz/EasySwing.T556.prs"},
	{"Orchestra Swing 1", "PSR-S970", "Swing&Jazz/OrchestraSwing1.T153.prs"},
	{"Orchestra Swing 2", "PSR-SX600", "Latin & Jazz/OrchestraSwing2.T555.prs"},
	{"MOR Swing", "PSR-SX600", "Latin & Jazz/MOR Swing.T555.prs"},
	{"Easy Listening", "PSR-SX700", "Jazz/EasyListening.T556.prs"},
	{"Cool Jazz Ballad", "PSR-SX600", "Latin & Jazz/CoolJazzBallad.T555.prs"},
	{"Midnight Swing", "PSR-S970", "Swing&Jazz/MidnightSwing.T153.prs"},
	{"Organ Groove", "PSR-S950", "Swing&Jazz/OrganGroove.T157.prs"},
	{"Cool Jazz Waltz", "PSR-SX700", "Jazz/CoolJazzWaltz.T555.prs"},
	{"Easy Waltz", "CVP-809", "Standards&Jazz/Jazz Waltz/Easy Waltz.T301.prs"},
	{"Jazz Waltz Slow", "CVP-709", "Standards&Jazz/Jazz Waltz/Jazz Waltz Slow.T301.prs"},
	{"MOR Waltz", "CVP-709", "Standards&Jazz/Jazz Waltz/MOR Waltz.T301.prs"},
	{"Jazz Waltz Medium", "CVP-709", "Standards&Jazz/Jazz Waltz/Jazz Waltz Medium.T301.sst"},
	{"Five-Four", "CVP-709", "Standards&Jazz/Jazz Waltz/Five-Four.T301.prs"},
	{"Jazz Waltz", "CVP-809", "Standards&Jazz/Jazz Waltz/Jazz Waltz.T301.pst"},
	{"Jazz Waltz Fast 1", "CVP-709", "Standards&Jazz/Jazz Waltz/Jazz Waltz Fast 1.T301.prs"},
	{"Jazz Waltz Fast 2", "CVP-709", "Standards&Jazz/Jazz Waltz/Jazz Waltz Fast 2.T301.prs"},
	{"Ragtime 1", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Ragtime 1.T294.prs"},
	{"Ragtime 2", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Ragtime 2.T294.prs"},
	{"Piano Rag 1", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Piano Rag 1.T294.pst"},
	{"Piano Rag 2", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Piano Rag 2.T294.pst"},
	{"Dixieland 1", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Dixieland 1.T294.prs"},
	{"Dixieland 2", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Dixieland 2.T294.prs"},
	{"Stride 1", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Stride 1.T294.pst"},
	{"Stride 2", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Stride 2.T294.pst"},
	{"Charleston", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Charleston.T294.prs"},
	{"Saloon Piano", "CVP-709", "Standards&Jazz/Dixie&Trad Jazz/Saloon Piano.T294.prs"},
	{"Jazz Pop", "PSR-SX600", "Pop & Rock/JazzPop.T547.prs"},
	{"Fusion Shuffle", "PSR-SX900", "R&B/FusionShuffle.T552.prs"},
	{"Movie Soundtrack", "PSR-SX700", "Movie&Show/MovieSoundtrack.T559.prs"},
	{"Mod Broadway Bld", "CVP-709", "Entertainment/Movie &Show/Mod Broadway Bld.T306.prs"},
	{"Broadway Ballad", "PSR-SX700", "Movie&Show/BroadwayBallad.T559.prs"},
	{"On Broadway", "PSR-SX700", "Movie&Show/OnBroadway.T559.fps"},
	{"Movie Swing 1", "PSR-S970", "Movie&Show/MovieSwing1.T163.prs"},
	{"Movie Swing 2", "PSR-SX700", "Movie&Show/MovieSwing2.T559.sst"},
	{"Cartoon Ballad", "CVP-809", "Entertainment/Movie&Show/Cartoon Ballad.T306.prs"},
	{"Animation Ballad", "PSR-SX700", "Movie&Show/AnimationBallad.T559.prs"},
	{"Tap Dance Swing", "PSR-SX700", "Movie&Show/TapDanceSwing.T559.prs"},
	{"Sci-fi March", "PSR-SX700", "Movie&Show/Sci-FiMarch.T559.prs"},
	{"70s TV Theme", "PSR-SX700", "Movie&Show/70sTV Theme.T559.prs"},
	{"Blockbuster", "CVP-709", "Entertainment/Movie &Show/Blockbuster.T306.prs"},
	{"Big Screen Classic", "CVP-709", "Entertainment/Movie &Show/Big Screen Classic.T306.prs"},
	{"Secret Service", "PSR-SX700", "Movie&Show/SecretService.T559.prs"},
	{"Movie Ballad", "PSR-SX700", "Movie&Show/MovieBallad.T559.sst"},
	{"Wild West", "PSR-SX700", "Movie&Show/WildWest.T559.prs"},
	{"Showtune", "PSR-S970", "Movie&Show/Showtune.T163.prs"},
	{"Choir Soundtrack", "CVP-809", "USA_Styles/Choir Soundtrack.T306.sty"},
	{"Animation Fantasy", "PSR-SX900", "Movie&Show/AnimationFantasy.T559.prs"},
	{"Musical", "CVP-709", "Entertainment/Movie &Show/Musical.T306.pst"},
	{"Classic Piano Bld", "PSR-S970", "Movie&Show/ClassicPianoBld.T160.prs"},
	{"Easy Ballad", "PSR-S970", "Ballad/EasyBallad.T160.prs"},
	{"Romantic Ballad", "CVP-709", "Entertainment/Easy Listening/Romantic Ballad.T295.prs"},
	{"Guitar Serenade", "PSR-SX700", "Movie&Show/GuitarSerenade.T559.prs"},
	{"Romantic Waltz", "CVP-809", "Entertainment/Easy Listening/Romantic Waltz.T295.prs"},
	{"Piano Orch Ballad", "CVP-709", "Entertainment/Easy Listening/Piano Orch Ballad.T295.pst"},
	{"6-8 Slow Rock", "PSR-S970", "Ballad/6-8SlowRock.T161.prs"},
	{"6-8 Orchestral", "PSR-SX600", "Entertainment/6-8Orchestral.T611.prs"},
	{"12-8 Ballad", "PSR-S970", "Ballad/12-8Ballad.T160.prs"},
	{"Tijuana", "PSR-SX700", "Jazz/Tijuana.T556.prs"},
	{"French 50s", "PSR-SX600", "Entertainment/French50s.T611.prs"},
	{"Pub Piano", "PSR-SX900", "Entertainer/PubPiano.T553.prs"},
	{"Pop Classics", "PSR-S970", "Movie&Show/PopClassics.T160.prs"},
	{"Viennese Waltz", "PSR-SX700", "Movie&Show/VienneseWaltz.T560.prs"},
	{"Quickstep", "PSR-SX600", "Country & Ballroom/Quickstep.T558.prs"},
	{"English Waltz", "PSR-SX700", "Ballroom/EnglishWaltz.T558.prs"},
	{"Tango", "PSR-S970", "Ballroom/Tango.T406.prs"},
	{"Slow Waltz", "CVP-709", "Entertainment/Ballroom/Slow Waltz.T282.prs"},
	{"Pasodoble", "PSR-SX700", "Ballroom/Pasodoble.T558.prs"},
	{"Slowfox", "PSR-S950", "Ballroom/Slowfox.T153.prs"},
	{"Samba", "PSR-SX600", "Country & Ballroom/Samba.T558.prs"},
	{"Foxtrot", "PSR-SX700", "Ballroom/Foxtrot.T558.prs"},
	{"Rumba", "PSR-SX600", "Country & Ballroom/Rumba.T558.prs"},
	{"Cha Cha Cha", "PSR-SX600", "Country & Ballroom/ChaChaCha.T558.prs"},
	{"Standard Waltz", "CVP-709", "Entertainment/Ballroom/Standard Waltz.T282.prs"},
	{"Jive", "PSR-SX900", "Ballroom/Jive.T558.prs"},
	{"Standard Rumba", "CVP-709", "Entertainment/Ballroom/Standard Rumba.T282.prs"},
	{"Swingfox", "PSR-S970", "Ballroom/Swingfox.T406.prs"},
	{"Piano Rumba", "CVP-709", "Entertainment/Ballroom/Piano Rumba.T282.pst"},
	{"Beguine", "CVP-709", "Entertainment/Ballroom/Beguine.T282.prs"},
	{"Piano Cha Cha", "CVP-709", "Entertainment/Ballroom/Piano Cha Cha.T282.pst"},
	{"Twist", "PSR-SX700", "R&B/Twist.T550.prs"},
	{"Piano Beguine", "CVP-709", "Entertainment/Ballroom/Piano Beguine.T282.pst"},
	{"Swing Waltz", "CVP-709", "Entertainment/Ballroom/Swing Waltz.T282.prs"},
	{"9-8 Waltz", "CVP-709", "Entertainment/Ballroom/9-8 Waltz.T282.prs"},
	{"Euro Pop Organ", "PSR-SX600", "Entertainment/EuroPopOrgan.T611.prs"},
	{"Organ Bossa", "PSR-SX600", "Latin & Jazz/OrganBossa.T557.prs"},
	{"Organ Samba", "PSR-SX600", "Country & Ballroom/OrganSamba.T558.prs"},
	{"Organ Cha Cha", "PSR-SX600", "Country & Ballroom/OrganChaCha.T558.prs"},
	{"Organ Swing", "PSR-SX600", "Country & Ballroom/OrganSwing.T558.prs"},
	{"Organ Quickstep", "PSR-SX600", "Country & Ballroom/OrganQuickstep.T558.prs"},
	{"Organ Rumba", "PSR-SX600", "Country & Ballroom/OrganRumba.T558.prs"},
	{"Christmas Swing", "PSR-SX700", "Movie&Show/ChristmasSwing.T560.prs"},
	{"Christmas Ballad", "Genos", "Classic_Events/ChristmasBallad_G.T160.prs"},
	{"Christmas Shuffle", "PSR-SX600", "Entertainment/ChristmasShuffle.T611.prs"},
	{"Christmas Waltz", "PSR-SX700", "Movie&Show/ChristmasWaltz.T560.prs"},
	{"Schlager Fever", "PSR-SX700", "Entertainer/SchlagerFever.T553.prs"},
	{"Schlager Palace", "PSR-S970", "Entertainer/SchlagerPalace.T407.prs"},
	{"Party Arena", "PSR-S970", "Entertainer/PartyArena.T407.prs"},
	{"Mallorca Party", "PSR-S950", "Entertainer/MallorcaParty.T160.prs"},
	{"Apres Ski Hit", "CVP-709", "Entertainment/Schlager/Apres Ski Hit.T312.prs"},
	{"Alpen Schlager", "PSR-S970", "Entertainer/AlpenSchlager.T407.prs"},
	{"Disco Fox", "PSR-SX900", "Entertainer/DiscoFox.T553.prs"},
	{"Soft Schlager", "PSR-SX900", "Entertainer/SoftSchlager.T553.prs"},
	{"German Rock", "CVP-709", "Entertainment/Schlager/German Rock.T312.prs"},
	{"Schlager Fox", "PSR-SX900", "Entertainer/SchlagerFox.T553.prs"},
	{"Schlager Italia", "CVP-709", "Entertainment/Schlager/Schlager Italia.T312.prs"},
	{"Schlager Shuffle", "PSR-S970", "Entertainer/SchlagerShuffle.T407.prs"},
	{"70s French Hit", "CVP-709", "Entertainment/Schlager/70s French Hit.T312.prs"},
	{"Schlager Waltz", "PSR-SX900", "Entertainer/SchlagerWaltz.T553.prs"},
	{"Alp Ballad 1", "CVP-709", "Entertainment/Schlager/Alp Ballad 1.T312.prs"},
	{"Alp Ballad 2", "CVP-709", "Entertainment/Schlager/Alp Ballad 2.T312.prs"},
	{"Schlager Pop", "PSR-SX700", "Entertainer/SchlagerPop.T553.prs"},
	{"Schlager Beat", "PSR-SX700", "Entertainer/SchlagerBeat.T553.prs"},
	{"Schlager Samba", "PSR-SX600", "Entertainment/SchlagerSamba.T611.prs"},
	{"Schlager Alp", "PSR-SX600", "Entertainment/SchlagerAlp.T611.prs"},
	{"Schlager Polka", "CVP-709", "Entertainment/Schlager/Schlager Polka.T312.prs"},
	{"Schlager Rumba", "CVP-709", "Entertainment/Schlager/Schlager Rumba.T312.prs"},
	{"Schlager Rock", "CVP-709", "Entertainment/Schlager/Schlager Rock.T312.prs"},
	{"Schlager 6-8", "PSR-SX700", "Entertainer/Schlager6-8.T553.prs"},
	{"Scand Bugg", "PSR-SX700", "World/ScandBugg.T589.prs"},
	{"Scand Shuffle", "PSR-SX900", "World/ScandShuffle.T589.prs"},
	{"Scand Slow Rock", "PSR-SX700", "World/ScandSlowRock.T589.prs"},
	{"Party Polka", "PSR-SX700", "Entertainer/PartyPolka.T553.prs"},
	{"8Beat Adria", "PSR-SX700", "Jazz/8BeatAdria.T556.prs"},
	{"Polka Pop", "CVP-709", "Entertainment/Schlager/Polka Pop.T312.prs"},
	{"Scand Country 1", "PSR-S970", "Entertainer/ScandCountry1.T151.prs"},
	{"Scand Country 2", "PSR-SX600", "Entertainment/ScandCountry2.T611.prs"},
	{"Brazilian Bossa", "PSR-SX700", "Latin/BrazilianBossa.T557.prs"},
	{"Cool Bossa", "PSR-SX700", "Latin/CoolBossa.T557.prs"},
	{"Bossa Slow", "CVP-809", "Latin&World/Brazil/Bossa Slow.T285.prs"},
	{"Bossa Fast", "CVP-809", "Latin&World/Brazil/Bossa Fast.T285.prs"},
	{"Slow Bar Bossa", "CVP-709", "Latin&World/Brazil/Slow Bar Bossa.T285.prs"},
	{"Samba Fast", "CVP-809", "Latin&World/Brazil/Samba Fast.T285.prs"},
	{"Orchestral Bossa", "PSR-S970", "Latin/OrchestralBossa.T158.prs"},
	{"Brazilian Samba", "PSR-S970", "Latin/BrazilianSamba.T158.prs"},
	{"Bossa Nova", "PSR-SX600", "Latin & Jazz/BossaNova.T557.prs"},
	{"Piano Samba", "CVP-709", "Latin&World/Brazil/Piano Samba.T285.pst"},
	{"Piano Bossa", "CVP-709", "Latin&World/Brazil/Piano Bossa.T285.pst"},
	{"Rumba Bolero", "CVP-809", "Latin&World/Cuba/Rumba Bolero.T291.prs"},
	{"Salsa", "PSR-SX700", "Latin/Salsa.T557.sst"},
	{"Guajira", "PSR-SX700", "Latin/Guajira.T557.prs"},
	{"Danzon", "PSR-SX600", "Latin & Jazz/Danzon.T557.prs"},
	{"Guitar Rumba", "PSR-SX700", "Latin/GuitarRumba.T557.prs"},
	{"Cuban Son", "PSR-S970", "Latin/CubanSon.T158.sst"},
	{"Mariachi Waltz", "PSR-SX700", "World/MariachiWaltz.T589.prs"},
	{"Norteno", "PSR-SX600", "World/Norteno.T589.prs"},
	{"Bolero Lento", "PSR-SX600", "Latin & Jazz/BoleroLento.T557.prs"},
	{"Sheriff Reggae", "PSR-SX600", "Latin & Jazz/SheriffReggae.T557.prs"},
	{"Merengue", "PSR-SX600", "Latin & Jazz/Merengue.T557.prs"},
	{"Happy Reggae", "PSR-S970", "Latin/HappyReggae.T158.prs"},
	{"Cumbia", "PSR-SX600", "Latin & Jazz/Cumbia.T557.sst"},
	{"Bomba", "PSR-SX600", "Latin & Jazz/Bomba.T557.sst"},
	{"Bachata", "PSR-SX700", "Latin/Bachata.T557.prs"},
	{"Zouk", "PSR-SX600", "World/Zouk.T589.prs"},
	{"Rumba Island", "PSR-SX600", "Latin & Jazz/RumbaIsland.T557.prs"},
	{"Flamenco", "PSR-SX700", "World/Flamenco.T589.prs"},
	{"Sirtaki", "PSR-SX900", "World/Sirtaki.T589.prs"},
	{"Pop Flamenco", "PSR-SX600", "World/PopFlamenco.T589.prs"},
	{"Tarantella", "PSR-SX700", "World/Tarantella.T589.prs"},
	{"Spanish Paso", "PSR-S970", "World/SpanishPaso.S751.prs"},
	{"Italian Mazurka", "CVP-709", "Latin&World/European/Italian Mazurka.T296.prs"},
	{"Rumba Flamenco", "PSR-SX700", "Latin/RumbaFlamenco.T557.prs"},
	{"Latin Party Pop", "PSR-SX700", "Latin/LatinPartyPop.T557.prs"},
	{"Pop Bossa", "PSR-SX700", "Latin/PopBossa.T557.prs"},
	{"Pop Latin", "PSR-SX700", "Latin/PopLatin.T557.prs"},
	{"Pop Latin Ballad", "PSR-SX600", "Latin & Jazz/PopLatinBallad.T557.prs"},
	{"Latin Disco", "PSR-SX600", "Latin & Jazz/LatinDisco.T557.prs"},
	{"Rock Cha Cha", "CVP-809", "USA_Styles/Rock Cha-Cha.T302.sty"},
	{"Jig", "CVP-709", "Latin&World/Celtic/Jig.T287.prs"},
	{"Celtic Dance", "PSR-SX900", "World/CelticDance.T589.prs"},
	{"Reel", "PSR-SX600", "World/Reel.T589.prs"},
	{"Irish Dance", "PSR-SX700", "World/IrishDance.T589.prs"},
	{"Irish Hymn 1", "PSR-S970", "World/IrishHymn1.S942.prs"},
	{"Irish Hymn 2", "PSR-S970", "World/IrishHymn2.S942.prs"},
	{"Turkish Euro", "Genos", "DJ/TurkishEuro_G.T151.prs"},
	{"J-Pop Hit 1", "CVP-709", "Latin&World/World/J-Pop Hit 1.T318.prs"},
	{"J-Pop Hit 2", "CVP-709", "Latin&World/World/J-Pop Hit 2.T318.prs"},
	{"Oriental Pop", "PSR-SX600", "World/OrientalPop.T589.prs"},
	{"Hawaiian", "PSR-S950", "World/Hawaiian.T164.prs"},
	{"Choral Symphony", "CVP-709", "Latin&World/Classic/Choral Symphony.T289.prs"},
	{"Ethereal Hymn", "CVP-709", "Latin&World/Classic/Ethereal Hymn.T289.fps"},
	{"Romantic Ballet", "PSR-S970", "Movie&Show/RomanticBallet.T150.prs"},
	{"Ethereal Voices", "PSR-SX700", "Movie&Show/EtherealVoices.T559.fps"},
	{"Green Fantasia", "PSR-SX700", "Movie&Show/GreenFantasia.T560.prs"},
	{"Baroque Air", "PSR-SX700", "Movie&Show/BaroqueAir.T560.sst"},
	{"String Adagio", "CVP-709", "Latin&World/Classic/String Adagio.T289.prs"},
	{"Moonlight 6-8", "PSR-SX700", "Movie&Show/Moonlight6-8.T560.prs"},
	{"Nocturne", "CVP-809", "Latin&World/Classic/Nocturne.T289.pst"},
	{"Learning 2-4", "CVP-709", "Latin&World/Classic/Learning 2-4.T289.prs"},
	{"Orchestral March", "PSR-SX700", "Movie&Show/OrchestralMarch.T560.prs"},
	{"Orchestral Bolero", "CVP-709", "Latin&World/Classic/Orchestral Bolero.T289.prs"},
	{"Traditional 1", "CVP-709", "Latin&World/Classic/Traditional 1.T289.pst"},
	{"Traditional 2", "CVP-809", "Latin&World/Classic/Traditional 2.T289.pst"},
	{"Traditional 3", "CVP-709", "Latin&World/Classic/Traditional 3.T289.pst"},
	{"Arpeggio 1", "CVP-709", "Latin&World/Classic/Arpeggio 1.T289.pst"},
	{"Arpeggio 2", "CVP-709", "Latin&World/Classic/Arpeggio 2.T289.pst"},
	{"Arpeggio 3", "CVP-709", "Latin&World/Classic/Arpeggio 3.T289.pst"},
	{"Highland Waltz", "PSR-SX600", "World/HighlandWaltz.T589.prs"},
	{"Scand Waltz", "PSR-SX700", "World/ScandWaltz.T589.prs"},
	{"French Waltz", "PSR-SX700", "World/FrenchWaltz.T589.prs"},
	{"Ober Walzer", "PSR-SX600", "World/OberWalzer.T589.prs"},
	{"Italian Waltz", "PSR-SX700", "World/ItalianWaltz.T589.prs"},
	{"German Waltz", "PSR-SX900", "World/GermanWaltz.T589.prs"},
	{"French Musette", "PSR-SX700", "World/FrenchMusette.T589.prs"},
	{"Bohemian Waltz", "PSR-SX700", "World/BohemianWaltz.T589.prs"},
	{"Waltz", "CVP-709", "Latin&World/Trad Waltz/Waltz.T316.pst"},
	{"Piano Slow Waltz", "CVP-709", "Latin&World/Trad Waltz/Piano Slow Waltz.T316.pst"},
	{"US March", "PSR-S970", "World/USMarch.S745.prs"},
	{"Ober Polka", "PSR-SX600", "World/OberPolka.T589.prs"},
	{"German March", "PSR-SX700", "World/GermanMarch.T589.prs"},
	{"Italian Polka", "CVP-709", "Latin&World/March&Polka/Italian Polka.T303.prs"},
	{"6-8 March", "PSR-S970", "World/6-8March.S745.prs"},
	{"Zither Polka", "CVP-709", "Latin&World/March&Polka/Zither Polka.T303.prs"},
	{"Piano March 1", "CVP-709", "Latin&World/March&Polka/Piano March 1.T303.pst"},
	{"Piano March 2", "CVP-809", "Latin&World/March&Polka/Piano March 2.T303.pst"},
	{"4 Stroke", "CVP-709", "Latin&World/March&Polka/4 Stroke.T303.pst"},
	{"6-8 Piano March", "CVP-709", "Latin&World/March&Polka/6-8 Piano March.T303.pst"},
};

static const char* const StyleExtensions[] = {"prs", "sst", "pst", "pcs", "sty", "fps", "scp"};

const StringArray& FactoryOts::Sources()
{
	static const StringArray sources{"CVP-809", "CVP-709", "Genos", "PSR-SX900", "PSR-SX700", "PSR-SX600", "PSR-S970",
		"PSR-S950"};
	return sources;
}

String FactoryOts::NameKey(const String& name)
{
	const String lower = name.toLowerCase().replace("&", "and");
	String key;
	for (const juce_wchar c : lower)
	{
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
		{
			key << String::charToString(c);
		}
	}
	return key;
}

String FactoryOts::CspVoice(int yamahaVoice, const String& source)
{
	const auto find = [](int number)
		{
			for (const Voice& voice : Presets::Voices())
			{
				if (voice.num == number)
				{
					return voice.path;
				}
			}
			return String();
		};
	const String same = find(yamahaVoice);
	if (same.isNotEmpty())
	{
		return same;
	}
	// not with the same number: from the tables (a voice of another model may differ)
	for (const SourceVoiceTable& tables : SourceVoiceTables)
	{
		if (source != tables.source)
		{
			continue;
		}
		for (size_t i = 0; i < tables.size; i++)
		{
			if (tables.table[i].first == yamahaVoice)
			{
				return tables.table[i].second < 0 ? String() : find(tables.table[i].second);
			}
		}
	}
	for (const auto& entry : VoiceTable)
	{
		if (entry.first == yamahaVoice)
		{
			return find(entry.second);
		}
	}
	return {};
}

//==============================================================================
// Reading the style files

namespace
{
	// A chunk of a Standard MIDI File or of the Yamaha chunks after it.
	struct Chunk
	{
		String tag;
		const uint8* data;
		int size;
	};

	std::vector<Chunk> ReadChunks(const uint8* data, int size)
	{
		std::vector<Chunk> chunks;
		int pos = 0;
		while (pos + 8 <= size)
		{
			const String tag = String::createStringFromData(data + pos, 4);
			const int length = (int)(((uint32)data[pos + 4] << 24) | ((uint32)data[pos + 5] << 16) |
				((uint32)data[pos + 6] << 8) | (uint32)data[pos + 7]);
			// (compared without adding to pos: a damaged length near 2 GB would overflow)
			if (length < 0 || length > size - pos - 8)
			{
				break;
			}
			chunks.push_back({tag, data + pos + 8, length});
			pos += 8 + length;
		}
		return chunks;
	}

	bool ReadVlq(const uint8* data, int size, int& pos, int& value)
	{
		value = 0;
		for (int i = 0; i < 4 && pos < size; i++)
		{
			const uint8 c = data[pos++];
			value = (value << 7) | (c & 0x7f);
			if ((c & 0x80) == 0)
			{
				return true;
			}
		}
		return false;
	}

	// The values of a MIDI channel in an OTS track (-1: not set).
	struct ChannelValues
	{
		int msb = -1, lsb = -1, program = -1, volume = -1;
		int on = -1, octave = -1, partVolume = -1;
		int pan = -1, reverb = -1, chorus = -1; // Control Change 10, 91, 93
		int Voice() const { return program < 0 ? -1 : (jmax(0, msb) << 16) | (jmax(0, lsb) << 8) | program; }
	};

	// The channels of one OTS (one track of the OTSc chunk).
	void ReadOtsTrack(const uint8* data, int size, ChannelValues channels[16])
	{
		int pos = 0;
		uint8 status = 0;
		while (pos < size)
		{
			int delta;
			if (!ReadVlq(data, size, pos, delta) || pos >= size)
			{
				return;
			}
			const uint8 byte = data[pos];
			if (byte == 0xff)
			{
				// meta event
				pos += 2;
				int length;
				if (pos > size || !ReadVlq(data, size, pos, length))
				{
					return;
				}
				pos += length;
				continue;
			}
			if (byte == 0xf0 || byte == 0xf7)
			{
				pos++;
				int length;
				if (!ReadVlq(data, size, pos, length) || pos + length > size)
				{
					return;
				}
				// F0 43 73 01 50 08 cc pp vv F7: a value of part pp of channel cc
				const uint8* sysex = data + pos;
				if (byte == 0xf0 && length >= 8 && sysex[0] == 0x43 && sysex[1] == 0x73 &&
					sysex[2] == 0x01 && sysex[3] == 0x50 && sysex[4] == 0x08 && sysex[5] < 16)
				{
					ChannelValues& ch = channels[sysex[5]];
					const int value = sysex[7];
					switch (sysex[6])
					{
						case 0x00: ch.on = value; break;
						case 0x03: ch.octave = value; break;
						case 0x04: ch.partVolume = value; break;
						default: break;
					}
				}
				pos += length;
				continue;
			}
			if (byte & 0x80)
			{
				status = byte;
				pos++;
			}
			if ((status & 0x80) == 0)
			{
				return; // running status without a status
			}
			const int type = status & 0xf0;
			const int length = (type == 0xc0 || type == 0xd0) ? 1 : 2;
			if (pos + length > size)
			{
				return;
			}
			ChannelValues& ch = channels[status & 0x0f];
			if (type == 0xb0)
			{
				const int controller = data[pos], value = data[pos + 1];
				if (controller == 0) ch.msb = value;
				else if (controller == 32) ch.lsb = value;
				else if (controller == 7) ch.volume = value;
				else if (controller == 10) ch.pan = value;
				else if (controller == 91) ch.reverb = value;
				else if (controller == 93) ch.chorus = value;
			}
			else if (type == 0xc0)
			{
				ch.program = data[pos];
			}
			pos += length;
		}
	}

	String VoiceNumber(int voice)
	{
		return String(voice >> 16) + "/" + String((voice >> 8) & 0x7f) + "/" + String((voice & 0x7f) + 1);
	}
}

bool FactoryOts::ReadStyleFile(const File& file, OneTouchSettings::Setting* settings, int& skipped, StringArray& log,
	const String& source, String* error)
{
	// a style file is well under 1 MB: a huge file of the same name is not read into memory
	static const int64 MaxStyleFileSize = 32 * 1024 * 1024;
	if (file.getSize() > MaxStyleFileSize)
	{
		if (error != nullptr) *error = "the file is too large for a style file";
		return false;
	}
	MemoryBlock block;
	if (!file.loadFileAsData(block))
	{
		if (error != nullptr) *error = "the file cannot be read";
		return false;
	}
	const std::vector<Chunk> chunks = ReadChunks((const uint8*)block.getData(), (int)block.getSize());
	const auto ots = std::find_if(chunks.begin(), chunks.end(), [](const Chunk& c) { return c.tag == "OTSc"; });
	if (ots == chunks.end())
	{
		return false;
	}
	const std::vector<Chunk> tracks = ReadChunks(ots->data, ots->size);
	int index = 0;
	for (const Chunk& track : tracks)
	{
		if (track.tag != "MTrk" || index >= OneTouchSettings::NumFactorySettings)
		{
			continue;
		}
		ChannelValues channels[16];
		ReadOtsTrack(track.data, track.size, channels);
		OneTouchSettings::Setting& setting = settings[index];
		setting = OneTouchSettings::Setting();

		// Main: Right 1 (channel 1), Layer: Right 2 (channel 2), Left: Left (channel 4)
		static const int Channels[OneTouchSettings::NumParts] = {0, 1, 3};
		String problem;
		for (int i = 0; i < OneTouchSettings::NumParts; i++)
		{
			const ChannelValues& ch = channels[Channels[i]];
			OneTouchSettings::Part& part = setting.parts[i];
			const int voice = ch.Voice();
			part.voice = voice < 0 ? String() : CspVoice(voice, source);
			part.active = ch.on > 0 || (i == 0 && ch.on < 0); // not set: Main on, the others off
			part.activeSet = i != 2;  // the on / off state of the Left part is not taken
			// the volume of the Mixer: Control Change 7 (else the Yamaha message, see VolumeFromPartMessage)
			const int first = VolumeFromPartMessage ? ch.partVolume : ch.volume;
			const int volume = first >= 0 ? first : (VolumeFromPartMessage ? ch.volume : ch.partVolume);
			part.volume = jlimit(PianoController::MinVolume, PianoController::MaxVolume,
				volume >= 0 ? volume : PianoController::DefaultVolume);
			part.octave = jlimit(PianoController::MinOctave, PianoController::MaxOctave,
				ch.octave >= 0 ? ch.octave - 0x40 : PianoController::DefaultOctave);
			// pan (64: centre), reverb and chorus, if the file has them
			part.effectsSet = ch.pan >= 0 && ch.reverb >= 0;
			part.pan = jlimit(PianoController::MinPan, PianoController::MaxPan,
				ch.pan >= 0 ? ch.pan - PianoController::PanBase : PianoController::DefaultPan);
			part.reverb = jlimit(PianoController::MinReverb, PianoController::MaxReverb,
				ch.reverb >= 0 ? ch.reverb : PianoController::DefaultReverb);
			part.chorusSet = ch.chorus >= 0;
			part.chorus = jlimit(0, 127, ch.chorus >= 0 ? ch.chorus : 0); // kept, not used
			if (part.voice.isEmpty())
			{
				// not changed by the setting
				part.valuesSet = false;
				part.effectsSet = false;
				part.chorusSet = false;
				const bool needed = i == 0 || (i == 1 && part.active);
				if (needed)
				{
					problem << (problem.isEmpty() ? "" : ", ") << OneTouchSettings::PartNames[i]
						<< " " << (voice < 0 ? String("-") : VoiceNumber(voice));
				}
			}
		}
		if (problem.isEmpty())
		{
			setting.used = true;
		}
		else
		{
			setting = OneTouchSettings::Setting();
			skipped++;
			log.add("OTS " + String(index + 1) + ": no voice on the CSP-170 for " + problem);
		}
		index++;
	}
	return index > 0;
}

FactoryOts::Result FactoryOts::Import(const File& stylesFolder, const File& factoryFolder,
	const std::function<void(double)>& progress, const std::function<bool()>& shouldExit)
{
	const auto exiting = [&shouldExit]() { return shouldExit != nullptr && shouldExit(); };
	Result result;
	const auto report = [&progress](double done)
		{
			if (progress)
			{
				progress(jlimit(0.0, 1.0, done));
			}
		};
	report(0.0);

	// the style files by their names (the first one of a name; without the "_G" of the Genos)
	std::vector<std::map<String, File>> files((size_t)Sources().size());
	for (int source = 0; source < Sources().size(); source++)
	{
		const File folder = stylesFolder.getChildFile(Sources()[source]);
		if (!folder.isDirectory())
		{
			continue;
		}
		result.sourcesFound = true;
		// (a link that points back to a folder above is not followed in circles)
		Array<File> found = folder.findChildFiles(File::findFiles, true, "*", File::FollowSymlinks::noCycles);
		std::sort(found.begin(), found.end(), [](const File& a, const File& b)
			{
				return a.getFullPathName().compareIgnoreCase(b.getFullPathName()) < 0;
			});
		for (const File& file : found)
		{
			const String extension = file.getFileExtension().substring(1).toLowerCase();
			if (std::none_of(std::begin(StyleExtensions), std::end(StyleExtensions),
				[&extension](const char* e) { return extension == e; }))
			{
				continue;
			}
			// <name>.T<number>.<extension> (some files of the PSR-S970: <name>.S<number>.<extension>)
			const String name = file.getFileNameWithoutExtension();
			const String number = name.fromLastOccurrenceOf(".", false, false);
			if (!name.containsChar('.') || !(number.startsWithChar('T') || number.startsWithChar('S')) ||
				!number.substring(1).containsOnly("0123456789") || number.length() < 2)
			{
				continue;
			}
			String base = name.upToLastOccurrenceOf(".", false, false);
			if (base.endsWith("_G"))
			{
				base = base.dropLastCharacters(2);
			}
			const String key = NameKey(base);
			if (key.isNotEmpty() && files[(size_t)source].count(key) == 0)
			{
				files[(size_t)source][key] = file;
			}
		}
	}
	if (!result.sourcesFound)
	{
		return result;
	}

	struct Item
	{
		String style;
		OneTouchSettings::Setting settings[OneTouchSettings::NumFactorySettings];
		int used = 0; // the settings taken
	};
	std::vector<Item> items;
	std::map<String, const StyleSource*> sourceOf;
	for (const StyleSource& entry : StyleSources)
	{
		sourceOf[NameKey(entry.style)] = &entry;
	}
	const int styleCount = (int)Presets::Styles().size();
	int styleIndex = 0;
	for (const Style& style : Presets::Styles())
	{
		if (exiting())
		{
			result.cancelled = true;
			return result; // nothing has been changed yet
		}
		report(0.9 * styleIndex++ / jmax(1, styleCount));
		const String key = NameKey(style.title);
		File file;
		int fileSource = -1;
		// the chosen style file, if it is there
		const auto chosen = sourceOf.find(key);
		if (chosen != sourceOf.end())
		{
			const File f = stylesFolder.getChildFile(chosen->second->source)
				.getChildFile(String(chosen->second->file).replaceCharacter('/', File::getSeparatorChar()));
			if (f.existsAsFile())
			{
				file = f;
				fileSource = Sources().indexOf(chosen->second->source);
			}
		}
		// else the first one of the same name
		for (int source = 0; file == File() && source < (int)files.size(); source++)
		{
			const auto it = files[(size_t)source].find(key);
			if (it != files[(size_t)source].end())
			{
				file = it->second;
				fileSource = source;
			}
		}
		if (file == File())
		{
			result.missing++;
			continue;
		}
		Item item;
		item.style = style.path;
		StringArray log;
		String error;
		if (!ReadStyleFile(file, item.settings, result.skipped, log, Sources()[fileSource], &error))
		{
			result.details.add(style.title + " (" + file.getRelativePathFrom(stylesFolder) + "): " +
				(error.isNotEmpty() ? error : String("no OTS in the file")));
			continue;
		}
		for (const String& line : log)
		{
			result.details.add(style.title + " (" + file.getRelativePathFrom(stylesFolder) + "): " + line);
		}
		int used = 0;
		for (const OneTouchSettings::Setting& setting : item.settings)
		{
			used += setting.used ? 1 : 0;
		}
		if (used > 0)
		{
			item.used = used;
			items.push_back(item);
		}
	}
	if (items.empty())
	{
		// nothing read (e.g. an empty folder of a model): the factory settings stay as they are
		result.details.add("No factory OTS were read: the previous ones are kept");
		return result;
	}

	// The new files are written into a folder of their own (next to the OTS folder: its own
	// settings are looked for in every folder of the OTS folder), which then takes the place
	// of the old factory folder. Only the styles really written are counted.
	report(0.9);
	const File otsFolder = factoryFolder.getParentDirectory();
	const File newFolder = otsFolder.getSiblingFile(otsFolder.getFileName() + "-" + factoryFolder.getFileName() + ".new");
	const File oldFolder = otsFolder.getSiblingFile(otsFolder.getFileName() + "-" + factoryFolder.getFileName() + ".old");
	newFolder.deleteRecursively();
	oldFolder.deleteRecursively();
	int written = 0;
	for (const Item& item : items)
	{
		if (exiting())
		{
			newFolder.deleteRecursively();
			result.cancelled = true;
			return result;
		}
		const File file = OneTouchSettings::FileIn(newFolder, item.style);
		if (OneTouchSettings::WriteFile(file, item.style, item.settings, OneTouchSettings::NumFactorySettings))
		{
			result.styles++;
			result.settings += item.used;
		}
		else
		{
			result.failed++;
			result.details.add(item.style + ": cannot write " + file.getFullPathName());
		}
		report(0.9 + 0.1 * ++written / jmax(1, (int)items.size()));
	}
	if (result.styles == 0)
	{
		newFolder.deleteRecursively();
		result.details.add("No factory OTS could be written: the previous ones are kept");
		return result;
	}

	// The new folder in the place of the old one. If a folder cannot be renamed (e.g. a file
	// of it is open in another program), the new files are copied over the old folder.
	bool replaced = false;
	if (!factoryFolder.exists() || factoryFolder.moveFileTo(oldFolder))
	{
		replaced = newFolder.moveFileTo(factoryFolder);
		if (!replaced && oldFolder.isDirectory())
		{
			oldFolder.moveFileTo(factoryFolder); // the old one back
		}
	}
	if (!replaced)
	{
		result.details.add("The factory folder could not be renamed: the files are copied");
		if (!factoryFolder.deleteRecursively())
		{
			result.details.add("Not every old file could be deleted from " + factoryFolder.getFullPathName());
		}
		replaced = newFolder.copyDirectoryTo(factoryFolder);
		if (!replaced)
		{
			result.details.add("Cannot copy the new files into " + factoryFolder.getFullPathName());
		}
	}
	newFolder.deleteRecursively();
	oldFolder.deleteRecursively();
	result.replaced = replaced;
	report(1.0);
	return result;
}
