extends RefCounted
## What each entry of config_glo.txt's [channels] list does, for the channel info overlay.
## Follows the Glo v1.00 owner's and extended manual, checked against the v1.0.118 firmware source.

const FILTERED := [
	"Tilt left: plucked arpeggio over the chords",
	"Tilt away / toward you: background quieter / louder",
	"Stand it up or turn it over: echo, shorter the further you go",
	"− 432 Hz tuning   + 440 Hz   ++ new chord progression",
	"−−− low-pass   +++ high-pass",
	"−−−− / ++++ less / more resonance",
	"Hold ▷: binaural beats (LED pink = available), hold again: next program, tap ▷: stop",
]
## Fire and Space run the "direct waves" filters: no arpeggio, and the filters follow the tilt, not the chords.
const FIRE := [
	"Tilt in any direction: the filter frequencies follow the tilt",
	"Tilt left: less reverb",
	"Stand it up or turn it over: echo, shorter the further you go",
	"−−− low-pass   +++ high-pass   −−−− / ++++ less / more resonance",
	"Hold ▷: binaural beats (LED pink = available), hold again: next program, tap ▷: stop",
]
const SPACE := [
	"Tilt in any direction: the filter frequencies follow the tilt",
	"Tilt away from you: louder noise",
	"Stand it up or turn it over: echo, shorter the further you go",
	"−−− low-pass   +++ high-pass   −−−− / ++++ less / more resonance",
	"Hold ▷: binaural beats (LED pink = available), hold again: next program, tap ▷: stop",
]
## What each +/− sequence does ('b' = +, 'c' = −), from the channels' firmware handlers.
const FILTERED_COMMANDS := {
	"c": "432 Hz tuning", "b": "440 Hz tuning", "bb": "new chord progression",
	"ccc": "low-pass filters", "bbb": "high-pass filters", "cccc": "less resonance", "bbbb": "more resonance",
}
const WATER := "Your voice and sounds ring through filters tuned to the chords, over a sampled background."

const CHANNELS := [
	{"name": "Earth", "about": "Forest and birds. Sing, hum or tap: the sound rings through resonant filters tuned to \"Air on the G String\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Wind", "about": "Wind made from filtered noise; your sounds ring through filters tuned to a calm chord progression.", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Water", "about": "Sea waves. " + WATER + " Chords from \"Rachel's Song\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Fire", "about": "Travel to space: noise and your sounds through filters that follow the tilt, with reverb.", "controls": FIRE, "commands": FILTERED_COMMANDS},
	{"name": "Reverb", "commands": {"b": "pitch glides up", "c": "pitch glides down"}, "about": "A decaying reverb whose buffer slowly changes size, heard as a pitch gliding down or up.", "controls": [
		"Tilt away / toward you: low-pass filter (away = darker)",
		"Stand it up or turn it over: echo, shorter the further you go",
		"+ pitch glides up   − pitch glides down",
		"Hold ▷: stop the glide and keep the reverb where it is",
	]},
	{"name": "Granular sampler", "commands": {"c": "more detune", "b": "less detune", "bb": "next song", "cc": "ignored"}, "about": "Records tiny grains of sound and plays them back as a chord at different pitches.", "controls": [
		"Starts frozen (LED green): tap ▷ to record (LED red), hold ▷ to freeze again",
		"Tap ▷ while recording: next channel",
		"Tilt left / right: minor / major chord, wider stereo",
		"Tilt away / toward you: more / fewer voices",
		"− detune the chord (repeat for more)   + undo detuning",
		"++ play a song instead of a static chord (tap quickly: sequences end after 0.5 s)",
	]},
	{"name": "DCO \"Seasick Jonah\"", "commands": {"b": "next parameter (after hold ▷)", "c": "previous parameter (after hold ▷)", "bb": "slower delay", "cc": "faster delay", "bbb": "next waveform", "ccc": "delay on / off", "bc": "randomize", "cb": "parameter drift"}, "about": "32 oscillators in stereo plus a delay line that records your sounds.", "controls": [
		"Tilt left / right: pick a parameter (LED: white frequency, cyan spread, pink phase, green envelope, red resonance)",
		"Tilt away / toward you: change its value",
		"Hold ▷: pick parameters with + / − instead of tilt",
		"++ / −− slower / faster delay   +++ next waveform   −−− delay on / off",
		"+− randomize   −+ parameter drift",
		"Upside down: erase the delay faster. On its left side: stop recording, loop forever",
	]},
	{"name": "Drum kit", "commands": {"b": "ignored", "c": "ignored"}, "about": "Three drums and a cymbal. Hold the phone as usual and drum in the air in four directions.", "controls": [
		"Tilt or swing left / right / away / toward you to hit a drum, then back to the middle",
		"LED: blue kick, red snare, green cymbal, pink hi-hat",
		"Hold ▷: echo on / off, cycles through timings",
	]},
	{"name": "Space", "about": "Like Fire without reverb: noise and your sounds through filters that follow the tilt.", "controls": SPACE, "commands": FILTERED_COMMANDS},
	{"name": "Clouds", "commands": {"b": "next patch", "c": "previous patch", "bb": "hard limiter up", "cc": "hard limiter down", "bbb": "parameter edit mode on", "ccc": "parameter edit mode off"}, "about": "Émilie Gillet's Mutable Instruments Clouds: a granular texture processor for your sounds.", "controls": [
		"+ next patch   − previous patch",
		"++ / −− raise / lower the hard limiter",
		"Hold ▷: freeze on / off (LED blue = frozen, cyan = live); tap ▷ while frozen: unfreeze",
		"Too much feedback or reverb can run away: lower the volume first",
	]},
	{"name": "Dekrispator", "commands": {"b": "next patch", "c": "previous patch", "bb": "next sequence", "cc": "last random patch", "bbb": "random patch", "ccc": "random effects"}, "about": "Xavier Halgand's synth with an internal sequencer; your sounds mix in before its effects.", "controls": [
		"+ next patch   − previous patch",
		"++ next sequence (then random ones)   −− last random patch",
		"+++ random patch   −−− random effects (both saved)",
		"Hold ▷: cycle echo / envelope / filters (LED red / green / blue, mixed for combinations)",
	]},
	{"name": "Song of Wind and Ice", "commands": {"b": "tone louder", "c": "tone quieter", "bb": "longer tones", "cc": "shorter tones", "bbb": "ignored", "ccc": "ignored"}, "about": "A passive soundscape of evolving wind, no microphone.", "controls": [
		"Hold ▷: isochronic tones (LED amber alpha, blue beta, green delta, turquoise theta)",
		"+ / − tone louder / quieter   ++ / −− longer / shorter tones",
		"Hold + or −: wind volume",
	]},
	{"name": "Water (creek)", "about": "Creek background. " + WATER + " Chords from \"Ghost in the Shell\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Bytebeat", "commands": {"b": "next song", "c": "stereo panning / echo", "bb": "faster", "cc": "slower"}, "about": "Chiptune-like music generated from math formulas.", "controls": [
		"+ next song",
		"− stereo panning, cycles 4 options with and 4 without echo",
		"−− slower (1/2 … 1/128)   ++ faster, back to 1/1",
	]},
	{"name": "Chopper", "about": "Cuts your voice into pieces and spreads them left and right.", "controls": [
		"Everything is controlled by tilting",
	]},
	{"name": "Pass-through", "about": "The microphone straight to the headphones, e.g. for field recording.", "controls": [
		"Use headphones: a speaker will feed back",
	]},
	{"name": "Water (sea)", "about": "Sea background, low-pass filters. " + WATER + " Chords from \"Rachel's Song\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Water (creek)", "about": "Creek background, low-pass filters. " + WATER + " Chords from \"Ghost in the Shell\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Water (sea)", "about": "Sea background, high-pass filters. " + WATER + " A newer chord progression.", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Water (creek)", "about": "Creek background, high-pass filters. " + WATER + " Chords from \"Freedom of Creation\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Water (sea 1)", "about": WATER + " Chords from Paul Lansky's \"Notjustmoreidlechatter\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Water (sea 1+2)", "about": WATER + " Chords from Paul Lansky's \"Notjustmoreidlechatter\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Water (sea 2)", "about": WATER + " Chords from Paul Lansky's \"Notjustmoreidlechatter\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Voiceover", "about": "High-pass filters over a voiceover background, chords from \"Notjustmoreidlechatter\".", "controls": FILTERED, "commands": FILTERED_COMMANDS},
	{"name": "Infinite looper", "commands": {"b": "faster tape", "c": "slower tape", "bb": "loop fades slower", "cc": "loop fades faster"}, "about": "Your sounds on an endless tape loop, played back at changing speeds.", "controls": [
		"+ / − tape faster / slower (pitch up / down)",
		"++ / −− the loop fades slower / faster",
		"Tilt has no effect in this channel",
	]},
]

const GENERAL := [
	"▷ tap: next channel   hold + or −: volume",
	"Tilt: tilt the phone or drag inside the ring",
	"+ / − commands: tap quickly; a single tap acts after a short pause",
	"Hold + and − for 1 s: settings menu (see the manual)",
]


## What the +/− sequence `seq` did in channel `id`, empty when unknown.
static func command(id: int, seq: String) -> String:
	if id < 0 or id >= CHANNELS.size():
		return ""
	return CHANNELS[id].get("commands", {}).get(seq, "")


## BBCode for the overlay. `id` is the [channels] index, `position` the channel number shown in the status line.
static func bbcode(id: int, position: int, font_size: int) -> String:
	var lines := PackedStringArray()
	if id >= 0 and id < CHANNELS.size():
		var ch: Dictionary = CHANNELS[id]
		lines.append("[font_size=%d][b]%d · %s[/b][/font_size]" % [roundi(font_size * 1.5), position, ch.name])
		lines.append(ch.about)
		lines.append("[ul]" + "\n".join(ch.controls) + "[/ul]")
	else:
		lines.append("[b]Channel %d[/b]" % position)
		lines.append("No description for this channel (custom config_glo.txt?).")
	lines.append("")
	for line in GENERAL:
		lines.append("[color=#ffffffaa]" + line + "[/color]")
	lines.append("")
	lines.append("[color=#ffffff77]Tap anywhere to close[/color]")
	return "\n".join(lines)
