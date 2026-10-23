# Flono

Flono runs the [Glo "Polyphonic Whale"](https://github.com/h3o/Glo_v1) ESP32 firmware (v1.0, Whale board)
unmodified on Android (and macOS) inside a Godot app. The repository is flono-mobile, the internal names (`glo-core`,
`GloNode`, `glo_*`) keep the glo prefix, and the Android package id is `io.github.marcomq.flono`.

**How to play it: see the [manual](MANUAL.md)** (controls, tilt, button commands, settings, all channels,
differences from the Glo). Privacy: [PRIVACY.md](PRIVACY.md) (no data is collected).

> **Disclaimer:** Flono is an independent, unofficial project. It is not affiliated with, endorsed or supported by
> Phonicbloom, gechologic or the Glo project, and claims no trademark, brand or other rights to their names or
> products. "Glo" only names the original instrument whose open-source firmware Flono runs.

```
firmware/        Glo_v1 submodule (h3o/Glo_v1, v1.0 Whale source, commit aad6c17), compiled as-is
port/            fake ESP-IDF/FreeRTOS: headers + shim (I2S rings, GPIO, NVS-in-files, flash image, tasks)
  glo_core.h     C API: glo_start / glo_render / glo_set_button / glo_set_accel / glo_get_led / ...
rust/glo-core    builds firmware + shim (cc), safe API, cpal duplex audio (AAudio / CoreAudio)
rust/glo-desktop test harness: offline WAV render or realtime with stdin control
rust/glo-godot   GDExtension: GloNode
godot/           app: 3 on-screen buttons, shader visuals from LED + spectrum, ? = channel info, ⓘ = license notices
tools/           make_samples.py: synthesizes a free sample image
```

## Controls

| Glo         | Android / on-screen        | Godot app keys | glo-desktop (stdin)             |
|-------------|----------------------------|----------------|---------------------------------|
| U1 (▷ play) | middle button (large, orange) | `1`         | `1` (short), `1L` (long press)  |
| U2 (+)      | right button               | `2`            | `2`, `2L`                       |
| U3 (−)      | left button                | `3`            | `3`, `3L`                       |
| Tilt       | accelerometer           | arrow keys     | `t X Y Z` (in g)                |
| Tilt       | joystick on the ring (adds to the accelerometer) | drag with the mouse |         |
| Quit       |                         |                | `q`                             |

Tapping ? shows what the current channel does and how to control it; tapping ⓘ opens the license notices and the disclaimer. Holding the lock next to ◎ for 10 s unlocks all channels (stored as the firmware's `XTRA_CHNL` NVS key, applied by
the supervisor between firmware restarts); holding it 3 s locks the extra channels again. Tapping ◎ takes the current holding pose as the Glo lying flat (also done at start).

## Audio

Like the original Glo, it is meant to be played with headphones. On Android the status line shows a warning while
no headphones are connected (wired, USB, Bluetooth, hearing aid or line out; checked every 2 s). Bluetooth speakers
cannot be told apart from Bluetooth headphones and count as headphones.

On Android the app records from the phone's built-in microphone by default, so a headset with its own mic
(wired or USB) does not take over; if no built-in mic is listed, it falls back to the system default.
Tapping "mic: built-in" at the top cycles through the connected microphones: built-in, USB and headset (wired or
Bluetooth). The choice is saved in `user://app.cfg`, and the built-in mic is used while the chosen one is disconnected.
The − / + next to it trim the mic sensitivity in 3 dB steps, saved separately for each of them.
Playback follows Android's routing (choose the output in Android's media output switcher); a USB microphone's own
output does not count as headphones. Audio restarts when a microphone is plugged in or removed. A few vendors'
audio drivers may ignore this and still use the headset mic. Without the microphone permission the app runs without input.
Mic sensitivity works like on the Glo: 6 short presses on U2 raise the AGC max gain by 12 dB (6 dB on the Glo), 6 on U3 lower it
(default `AGC_MAX_GAIN 32` in `config_glo.txt`, not saved across restarts); the status line shows the new value.
Phone mics are hotter than the Glo's, so the app applies the firmware's value 13 dB lower (25 dB lower for a USB mic).
glo-desktop takes `--no-mic`; on macOS the system default input is used.

Both need the Android device list, which cpal reads through JNI. A GDExtension gets no JavaVM handed to it, so
`glo-core/src/android.rs` looks it up via `JNI_GetCreatedJavaVMs` (libnativehelper, Android 12+). On older
Android versions there is no warning and the mic follows the system routing.

## How it works

- The firmware's audio loop calls `i2s_push_sample` per stereo sample at its own engine rate (50780 Hz by default).
  The shim puts samples into a small SPSC ring (default 512 frames ≈ 10 ms, `glo_set_ring_frames`) and blocks while it is full.
  The device audio callback (`glo_render`) drains it with linear resampling to the device rate. Mic input
  flows the other way into `i2s_pop_sample`.
- The codec registers are not emulated; `glo_render` applies the firmware's `codec_digital_volume`/`codec_analog_volume`
  (0.5 dB steps, mute values honoured) times a master gain.
- Buttons and tilt: see [Controls](#controls). Accelerometer = `Input.get_accelerometer()` in g.
  RGB LED pins are read back for the visuals.
- Flash: `config_glo.txt` at 0x90000 (embedded; override with `<user data>/config_glo.txt`), the sample image at 0x330000.
- NVS (settings, channel order) is stored as files under `<user data>/nvs/`.
- `esp_restart()` and power-off end all firmware threads at their next shim call. After that the
  supervisor boots `app_main` again (after power-off: when U1 is held). Firmware globals are not re-initialised on
  restart (unlike real hardware).
- The published v1.0 source never sets `glo_run = 1` (later releases do), which would leave only the fallback
  loop; the supervisor sets it before boot. No firmware file is modified.

## Sample image

Most Whale channels run their filters on background samples from flash (sea, forest, creek; the drum kit and
voice menu also live there). The layout is the `[samples_map]` in `config_glo.txt` (`_all_samples.bin` version 09, 7264532 bytes).
Without an image those channels are near-silent.

Generate a free replacement (synthesized, same layout; voice menu spoken if `espeak-ng` is installed, chimes otherwise):

```sh
brew install espeak-ng                   # optional, for the spoken voice menu
uv run tools/make_samples.py            # writes godot/data/samples.bin
uv run tools/make_samples.py --config firmware/data/config_glo.txt --out /tmp/samples.bin
```

`uv` installs the only dependency (numpy) from the script header. The script reads the `[samples_map]`, `DRUM_LENGTHx`,
`[voice_menu]` and `[voice_menu_v1]` sections of the config and writes 16-bit little-endian PCM at the same
offsets, so the result works without config changes. The output is deterministic (fixed random seed).

| Sample | Original                 | Synthesized as                                                            |
|--------|--------------------------|---------------------------------------------------------------------------|
| 1, 3   | sea (10 s, 20 s)         | band-limited noise with slow swells and surf hiss, loops seamlessly       |
| 4      | forest                   | soft rustling noise plus random bird chirps                               |
| 6      | water (creek)            | noise plus randomly placed rising bubbles                                 |
| 9      | drums                    | snare, hi-hat, kick, cymbal, split at the `DRUM_LENGTHx` boundaries       |
| 5      | voice menu (48 kHz)      | each `[voice_menu]` entry spoken by espeak-ng at its time offset, or a chime |
| 10     | voice menu v1 (22.05 kHz)| same for `[voice_menu_v1]`                                                |
| 2, 7, 8| combined samples         | not written: they are byte ranges spanning their neighbours               |

Background samples are generated at the engine rate (50780 Hz), because the firmware reads one value per engine frame.

Use it as `godot/data/samples.bin` (packed into the APK), `samples.bin` in the app's user data dir, or `--samples FILE` for glo-desktop.

The official image (gechologic.com `v2/download/samples.bin.3c5017e…`, or read from a device with
`esptool.py read_flash 0xa0000 0x6f0000 samples.bin`) has no published license: fine for personal testing,
but do not put it in an APK you distribute. `godot/data/` packs whatever file is there.

## Build

Requirements: Rust (+ `aarch64-linux-android` target), `cargo-ndk`, Android NDK, Godot 4.7 with export templates.

[`build.sh`](build.sh) wraps the usual steps. It also fetches the `firmware` submodule, uses the Homebrew NDK path below
if `ANDROID_NDK_HOME` is unset, and runs `$GODOT` (default `godot`):

```sh
./build.sh desktop    # Godot extension for the host OS → godot/bin/<macos|linux|windows>/
./build.sh android    # Android extension + APK → godot/bin/android/, build/flono.apk
./build.sh install    # android, then adb install
./build.sh log        # adb logcat -s glo
```

`./build.sh release` exports a release APK signed with the keystore in `GODOT_ANDROID_KEYSTORE_RELEASE_PATH`
(`_USER` = alias, `_PASSWORD`).

### Release (GitHub Actions)

Pushing a tag `v*` (e.g. `v0.1.0`) runs [`.github/workflows/release.yml`](.github/workflows/release.yml): it
synthesizes the free sample image, builds the signed APK and attaches `flono-<version>.apk`, `LICENSE` and
`NOTICE.md` to a GitHub release. Repository
secrets: `ANDROID_KEYSTORE_BASE64` (`base64 -i release.keystore`), `ANDROID_KEYSTORE_ALIAS`,
`ANDROID_KEYSTORE_PASSWORD`. Manual runs (workflow_dispatch) only upload the APK as a build artifact.

### Version

The single source of the version is `version` under `[workspace.package]` in [`rust/Cargo.toml`](rust/Cargo.toml).
Every Android build (`./build.sh android | release | aab`, locally and in CI) writes it into both presets in
`godot/export_presets.cfg` as version name, with version code major × 10000 + minor × 100 + patch (0.1.2 → 102);
don't edit the presets by hand. `./build.sh version` prints it. To release: bump the version in `rust/Cargo.toml`,
commit, tag `v<version>` and push the tag. CI fails if the tag doesn't match; untagged builds are named
`<version>-<commit>`.

Tested on macOS (desktop) and Android. Linux and Windows are set up but untested:

- Linux: needs a C/C++ toolchain and the ALSA headers (`libasound2-dev` / `alsa-lib-devel`) for cpal.
- Windows: the firmware shim uses pthreads and C11 atomics, so MSVC does not work; build with MinGW
  (`rustup target add x86_64-pc-windows-gnu`, MinGW-w64 gcc on `PATH`) and run `build.sh` from Git Bash or MSYS2.

The individual commands, e.g. for the desktop harness:

```sh
git submodule update --init

# desktop harness
cd rust
cargo run --release -p glo-desktop -- --wav out.wav --seconds 20 --channel 2
cargo run --release -p glo-desktop            # realtime; type 1/2/3, 1L, "t 0.5 0 -0.8", q

# Godot on macOS
cargo build --release -p glo-godot && cp target/release/libglo_godot.dylib ../godot/bin/macos/

# Android
export ANDROID_NDK_HOME=/opt/homebrew/share/android-commandlinetools/ndk/29.0.14206865
cargo ndk -t arm64-v8a --platform 26 build --release -p glo-godot
cp target/aarch64-linux-android/release/libglo_godot.so ../godot/bin/android/
cd ../godot && godot --headless --import && godot --headless --export-debug Android
adb install -r ../build/flono.apk
```

`adb install` needs USB debugging enabled on the phone (Settings → About phone → tap Build number 7×, then
Developer options → USB debugging). Alternatively copy `build/flono.apk` to the phone and open it there
(allow installing unknown apps).

The first build of `godot-core` (generated Godot bindings) takes several minutes per target; later builds are incremental.
Firmware log output goes to logcat with the tag `glo` (`./build.sh log`).

## License

GPL-3.0 (`LICENSE`), as is the upstream firmware. Third-party components and their licenses are listed in
`NOTICE.md`; the app shows the required notices behind the ⓘ button. When you distribute an APK, publish the exact
source revision it was built from (including the `firmware` submodule commit). Because of GPL-3.0, the Apple App
Store is not an option without permission from all copyright holders.
