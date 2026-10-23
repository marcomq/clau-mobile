# Flono manual

Flono runs the original firmware of Glo, "the Polyphonic Whale" (v1.0.118), on your phone. The phone's screen
replaces Glo's three buttons, the phone's tilt replaces Glo's accelerometer, and its microphone replaces Glo's
microphones. Everything you hear is made by the Glo firmware; Flono only fixes a few memory bugs in it and adds the
Infinite looper to the channel list. This manual follows the official
[Glo owner's manual and extended manual v1.00](https://phonicbloom.com/glo/manual/) and adds what is different in
v1.0.118 and on a phone.

Flono is an independent project, not affiliated with or endorsed by Phonicbloom, gechologic or the Glo project
(see [Disclaimer](#disclaimer)).

- [Before you start](#before-you-start)
- [The screen](#the-screen)
- [Basic controls](#basic-controls)
- [Tilt](#tilt)
- [Button commands](#button-commands)
- [Settings menu](#settings-menu)
- [Binaural beats](#binaural-beats)
- [Channels](#channels)
- [Differences from the Glo](#differences-from-the-glo)
- [Troubleshooting](#troubleshooting)

## Before you start

- **Use headphones.** Flono listens to the microphone and plays back what it hears, transformed. A loudspeaker
  feeds back into the microphone, and binaural beats only work with one ear per channel. The status line warns
  while no headphones are connected.
- **Allow the microphone** when Android asks. Without it, Flono still runs but cannot react to your voice.
- **Sound to play with:** sing, hum, whistle, clap, tap or scratch the phone near its microphone. Most channels
  turn whatever they hear into chords.
- **Background samples:** most channels play a background (sea, forest, creek, drums, voice). They come from a
  sample image; without one these channels are almost silent. See the README's
  [Sample image](README.md#sample-image) section.

## The screen

```
 ◎  🔒             mic: built-in   ?   ⓘ
                    −           +

               ( ring joystick )

    ( − )          (  ▷  )         ( + )
  hold: quieter  tap: next channel  hold: louder
              channel 1 Earth
```

| Element | What it does |
|---|---|
| **▷** (large, middle, orange) | Glo's play / power button. Tap: next channel. Hold: channel options (e.g. binaural beats). |
| **−** (left) and **+** (right) | Glo's minus and plus buttons. Hold: volume. Tap: channel-specific commands. |
| Each third of the screen | is one touch field: you don't have to hit the circle exactly. Several fingers can press several buttons at once (needed for combinations like **+** and **−** together). |
| Ring in the middle | Joystick: drag inside it to tilt the virtual Glo, on top of the phone's own tilt. Half-way to the edge tilts by 90°, the edge turns it upside down. Let go to return. |
| Visuals | Colour = Glo's LED (it signals modes, see each channel), shape = the sound. |
| **◎** (top left) | Calibrate: your current way of holding the phone becomes "Glo lying flat". Also done at start. |
| **Lock** (next to ◎) | Hold 10 s to unlock all 25 channels; hold 3 s to lock the extra ones again. A ring shows the progress, the status line counts down. Flono restarts afterwards. Open and orange = unlocked. |
| **?** | What the current channel does and how to control it. Tap anywhere to close. |
| **ⓘ** | About, licenses and the disclaimer. |
| `mic: built-in` / `mic: USB` (Android) | Tap to switch to a USB microphone when one is connected. **−** / **+** next to it trim the mic sensitivity in 3 dB steps. |
| Status line | Channel number and name, short notices (mic gain, mic sensitivity). |

On a computer: keys `1` = ▷, `2` = +, `3` = −, arrow keys tilt, the mouse drags the joystick.

## Basic controls

| Action | How |
|---|---|
| Next channel | Tap **▷**. After the last channel it starts again at the first. |
| Volume | Hold **+** or **−**. The volume changes after half a second and is saved. |
| Channel options | Hold **▷** for 1 s (the printed manual says 2 s; the firmware reacts after 1 s). What it does depends on the channel. |
| Move a channel to the front | While the channel plays, hold **▷** and **+** together for 2 s. A melodic beep confirms; next time it is the first channel. Repeat with other channels to build your own order. |
| Calibrate tilt | Tap **◎** while holding the phone the way you want to play it. |
| Power on again | Hold **▷** (only needed after Flono switched itself off, see [Differences](#differences-from-the-glo)). |

The 12 main channels are available right away; 13 more need to be [unlocked](#unlock-all-channels).

## Tilt

Glo is played by tilting it. In Flono you can tilt the phone or drag the ring joystick; both add up.
"Lying flat" is however you held the phone at start or when you last tapped **◎**.

In the four nature channels and their variants:

| Tilt | Effect |
|---|---|
| Left | A plucked arpeggio plays the chord notes; the further, the louder. (Not in 4 Fire and 9 Space, see below.) |
| Away from you / toward you | Background (sea, forest, creek, wind) quieter / louder |
| Stand it up, or turn it over | Echo; the further past upright, the shorter the echo |

The other channels use tilt differently; tap **?** or see [Channels](#channels).

## Button commands

Short taps on **+** and **−** are collected into sequences: tap quickly (less than about 1 s between taps). A
sequence is carried out after a short pause, so a single tap also reacts with a small delay. The phone beeps
for each tap (this can be switched off).

The status line shows the taps while you type (`+ + …`). Once the sequence is complete, it shows what happened:
what the command did (e.g. "440 Hz tuning", "pitch glides up", "next patch"), "ignored" when the channel
takes it but does nothing, "no effect in this channel" or "not a command". Taps on **▷** count too: tap
**▷** and then **+ +** within about a second and the firmware reads it as one sequence ("not a command"). Holding **+** / **−** shows the new volume.

### Everywhere

| Sequence | Effect | Saved? |
|---|---|---|
| **+ − + − + −** | Key beeps on / off | yes |
| **+** ×5 | Output volume +6 dB (up to 0 dB) | yes |
| **−** ×5 | Output volume −6 dB (down to −50 dB) | yes |
| **+** ×6 | Microphone gain limit (AGC) higher. Flono: +12 dB per step (Glo: 6 dB), shown in the status line. Careful, feedback. | no |
| **−** ×6 | Microphone gain limit lower | no |
| **▷ + + + − − − ▷** | Factory reset: forgets volumes, tuning, tempo, channel order and unlocking; then restarts. Do it shortly after start. | |
| **− + − + − + − + ▷** | Unlock all channels (see below) | yes |

### Unlock all channels

Hold the lock next to **◎** for 10 seconds, or key in the Glo's own sequence **− + − + − + − + ▷** (four times
minus-plus, then play; a voice message confirms). Flono restarts with all 25 channels and remembers it. To hide
the extra channels again, hold the lock for 3 seconds; the factory reset also locks them. Locking or unlocking
resets the channel order.

### In the nature channels (1 Earth, 2 Wind, 3 Water, 4 Fire, 9 Space, 13 and 17–24 Water variants)

| Sequence | Effect | Saved? |
|---|---|---|
| **−** | Tune to 432 Hz | yes |
| **+** | Tune to 440 Hz | yes |
| **+ +** | New random chord progression | no |
| **− − −** | Low-pass filters | no |
| **+ + +** | High-pass filters | no |
| **− − − −** / **+ + + +** | Less / more resonance (repeatable) | no |

## Settings menu

Hold **+** and **−** together for 1 s (two fingers, left and right). One beep: you are at option A. Tap **▷** to
go to the next option (two, three, four beeps). Tapping **▷** after option D, or holding **▷**, leaves the menu.
While an option is selected, holding **+** and **−** for 1 s resets it to its default. Settings are saved.

| Option | **−** | **+** |
|---|---|---|
| A. Equalizer | bass +3 dB (cycles 3, 6, 9, −9, −6, −3, 0 dB) | treble, same | 
| B. Tempo | −5 BPM | +5 BPM (10 to 330, default 120) |
| C. Fine tuning | −10 cents | +10 cents (±200 cents around 440 / 432 Hz) |
| D. Fixed effects | arpeggio level 25 %, 50 %, 75 %, 100 %, off | next fixed echo length, or echo off |

Option D replaces tilt control of echo and arpeggio, so you don't need to hold the phone at an angle.
**Flono:** the equalizer (A) has no audible effect, because the Glo's audio chip is not emulated.

## Binaural beats

A binaural beat is heard when each ear gets a slightly different frequency: the brain perceives the difference
as a slow pulse. **Headphones are required.**

In channels with binaural beats (the four nature channels and some variants) the LED glows pink when the
channel starts. Hold **▷** to start the channel's program: the LED pulses in the program's colour. Hold **▷**
again to move to the next program; tap **▷** to stop.

| Program | Range | LED | Meant for | Default in |
|---|---|---|---|---|
| Alpha | 7.5–12.5 Hz | amber | relaxation | 1 Earth |
| Beta | 12.5–30 Hz | blue | concentration | 4 Fire, 9 Space, 13 Water (creek) |
| Delta | 0.5–4 Hz | green | deep sleep | 3 Water |
| Theta | 4–7 Hz | turquoise | meditation, creativity | 2 Wind |

The official manual recommends 15–30 minutes in a relaxed position with closed eyes, and no more than 30 minutes
the first time. Lying the Glo on its side (headphone socket up) maximises the arpeggio and the binaural effect;
in Flono that is tilting fully left.

## Channels

Default order. The status line and the **?** overlay show the channel name; after you
[moved channels to the front](#basic-controls) the numbers shift.

### Main channels

1. **Earth**: forest and birds; your sounds ring through resonant low-pass filters tuned to "Air on the G String". Binaural: alpha.
2. **Wind**: wind generated from filtered noise, with a calm chord progression. Binaural: theta.
3. **Water**: sea waves; high-pass filters, chords from "Rachel's Song". Binaural: delta.
4. **Fire**: a trip to space: noise and your sounds through filters whose frequencies follow the tilt, with
   reverb. Tilt left: less reverb. Stand it up or turn it over: echo, as in the nature channels (no arpeggio).
   The nature-channel commands work here too; low-/high-pass and resonance are the audible ones, because the
   filters follow the tilt instead of the chords. Binaural: beta.
5. **Reverb**: a decaying reverb whose buffer slowly changes size, heard as a pitch gliding down or up.
   - Tilt away / toward you: low-pass filter (away = darker; useful to leave only humming or drumming).
   - Stand it up or turn it over: echo, shorter the further you go (as in the nature channels).
   - **+** pitch glides up, **−** pitch glides down, hold **▷** stops the glide.
6. **Granular sampler**: records tiny grains of sound and plays them back as a chord.
   - It starts frozen (LED green). Tap **▷** to start recording (LED red); hold **▷** to freeze the current
     grains again. Tapping **▷** while recording moves to the next channel.
   - Tilt left / right: minor / major chord and wider stereo. Tilt away / toward you: more / fewer voices.
   - **−** detunes the chord (repeatable), **+** undoes it, **+ +** plays a song instead of a static chord.
     Tap faster than elsewhere: a sequence ends after half a second.
7. **DCO "Seasick Jonah"**: 32 oscillators in stereo plus a delay line that records your sounds.
   - Tilt left / right selects a parameter, shown by the LED: frequency (white), spread (cyan), phase (pink),
     envelope (green), resonance (red). Tilt away / toward you changes its value; the scale wraps around near
     flat, so you can jump from maximum to minimum.
   - Hold **▷**: select parameters with **+** / **−** instead (tilt then only changes the value).
   - **+ +** / **− −** slower / faster delay line (pitch-bends the recording), **+ + +** next waveform (sine,
     square, triangle, off), **− − −** delay line on / off, **+ −** randomize everything, **− +** parameter drift.
   - Upside down erases the delay faster; on its left side recording stops and the loop plays forever.
8. **Drum kit**: three drums and a cymbal. Drum in the air in four directions (left, right, away, toward you;
   two at once works too): a drum sounds when the phone tilts more than about 20° that way, and again only
   after it came back to the middle. The Glo is held upright for this; in Flono the way you hold the phone (◎)
   already counts as upright. LED: blue kick, red snare, green cymbal, pink hi-hat.
   Hold **▷**: echo on / off, cycling through timings.
9. **Space**: like Fire, without reverb; tilt away from you for louder noise. Binaural: beta.
10. **Clouds**: Émilie Gillet's Mutable Instruments Clouds, a granular texture processor for your sounds.
    **+** / **−** next / previous patch, **+ +** / **− −** raise / lower the hard limiter, hold **▷** freeze
    on / off (LED blue = frozen, cyan = live); while frozen, a tap on **▷** unfreezes instead of changing the
    channel. Tap faster than elsewhere: a sequence ends after half a second. Too much feedback or reverb can run away: lower the volume first.
    (**+ + +** / **− − −** enter / leave a parameter editing mode that reports over the Glo's USB serial port; not useful in Flono.)
11. **Dekrispator**: Xavier Halgand's synth with an internal sequencer; your sounds mix in before its effects.
    **+** / **−** next / previous patch, **+ +** next sequence (then random ones), **− −** last random patch,
    **+ + +** random patch, **− − −** random effects (both saved). Hold **▷** cycles echo / envelope / filters
    (LED red / green / blue, mixed colours for combinations, white = all).
12. **Song of Wind and Ice**: a passive soundscape of evolving wind; the microphone is not used.
    Hold **▷**: isochronic tones (colours as for binaural beats). **+** / **−** tone louder / quieter,
    **+ +** / **− −** longer / shorter tones, hold **+** / **−** wind volume.

### Unlocked channels

13. **Water (creek)**: creek background, chords from "Ghost in the Shell". Binaural: beta.
14. **Bytebeat**: chiptune-like music from math formulas. **+** next song, **−** stereo panning (4 options with
    and 4 without echo), **− −** slower (1/2 … 1/128), **+ +** faster again.
15. **Chopper**: cuts your voice into pieces and spreads them left and right; everything is controlled by tilt.
16. **Pass-through**: the microphone straight to the headphones, e.g. for listening closely or field recording.
17. **Water (sea)**, low-pass filters, "Rachel's Song".
18. **Water (creek)**, low-pass filters, "Ghost in the Shell".
19. **Water (sea)**, high-pass filters, a newer chord progression.
20. **Water (creek)**, high-pass filters, "Freedom of Creation".
21.–23. **Water (sea 1, sea 1+2, sea 2)**: chord progression from Paul Lansky's "Notjustmoreidlechatter".
24. **Voiceover**: high-pass filters over a voiceover background, "Notjustmoreidlechatter".
25. **Infinite looper**: your sounds on an endless tape loop, played back at a changeable speed (from the Gecho,
    channel 8 on the Whale v2). **+** / **−** tape faster / slower (pitch up / down), **+ +** / **− −** the loop
    fades slower / faster. Tilt has no effect: the Gecho controls it with IR sensors the Whale lacks.

Channels 13 and 17–24 work like the nature channels (tilt, commands, binaural beats where noted).

## Differences from the Glo

Flono runs the firmware with only bug fixes, but a phone is not a Glo:

| On the Glo | In Flono |
|---|---|
| **▷** and **−** together switch it off | Not available: on the Glo this is done by the power circuit, not the firmware. Close the app instead. |
| Switches itself off after 1 hour on the same channel (`AUTO_POWER_OFF_TIMEOUT`) | Same: the sound fades out and the status line shows "off". Hold **▷** to start again. |
| Hold **▷** 2 s for options, **▷** + **+** 5 s to move a channel (printed manual) | 1 s and 2 s: what the firmware actually does |
| **+** and **−** tapped together toggle the microphones | No effect (the audio chip is not emulated) |
| Equalizer (settings option A) | No effect (same reason) |
| 6× **+** / **−** change the mic gain limit by 6 dB | 12 dB, because phone microphones behave differently; the start value is also lower |
| 24 channels after unlocking | 25: the Infinite looper from the same firmware is added at the end, as on the Whale v2 |
| Line input, USB serial output | Not available |
| Light sensor (Glo v2 / v3) | Not part of the v1.0.118 firmware |
| Voice messages and backgrounds from the official sample image | Synthesized replacements unless you supply the official image yourself (see README) |
| Restart / factory reset re-initialises everything | The firmware restarts, but values it keeps in memory (e.g. the mic gain limit) last until the app is closed |

## Troubleshooting

- **Silence or only a soft hiss:** check the volume (hold **+**), headphones, and that a sample image is
  installed; in Song of Wind and Ice the mic is not used anyway.
- **Feedback or howling:** use headphones; lower the mic gain limit (6× **−**) or the mic sensitivity (**−** next
  to `mic:` at the top).
- **Doesn't react to my voice:** grant the microphone permission (Android settings → Apps → Flono → Permissions),
  check `mic:` at the top, raise the mic gain limit (6× **+**).
- **Tilt feels wrong:** hold the phone the way you want to play it and tap **◎**.
- **Something is stuck:** factory reset with **▷ + + + − − − ▷** right after start, or close and reopen the app.

## Disclaimer

Flono is an independent, unofficial project. It is not affiliated with, endorsed, sponsored or supported by
Phonicbloom, gechologic or the Glo project, and it claims no trademark, brand or other rights to their names,
products or artwork. "Glo" and "Glo the Polyphonic Whale" are used only to identify the original instrument
whose openly published firmware Flono runs. For the real instrument, visit [phonicbloom.com](https://phonicbloom.com/).
