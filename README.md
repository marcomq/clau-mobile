# glo-mobile

The [Glo "Polyphonic Whale"](https://github.com/shroomist/Glo_v1) ESP32 firmware (v1.0, Whale board), running
unmodified on Android (and macOS) inside a Godot app.

```
firmware/        Glo_v1 submodule, branch glo-port (v1.0 Whale source, aad6c17), compiled as-is
port/            fake ESP-IDF/FreeRTOS: headers + shim (I2S rings, GPIO, NVS-in-files, flash image, tasks)
  glo_core.h     C API: glo_start / glo_render / glo_set_button / glo_set_accel / glo_get_led / ...
rust/glo-core    builds firmware + shim (cc), safe API, cpal duplex audio (AAudio / CoreAudio)
rust/glo-desktop test harness: offline WAV render or realtime with stdin control
rust/glo-godot   GDExtension: GloNode
godot/           app: 3 on-screen buttons, shader visuals from LED + spectrum, ⓘ = license notices
tools/           make_samples.py: synthesizes a free sample image
```

## How it works

- The firmware's audio loop calls `i2s_push_sample` per stereo sample at its own engine rate (50780 Hz by default).
  The shim puts samples into a small SPSC ring (default 512 frames ≈ 10 ms, `glo_set_ring_frames`) and blocks while it is full.
  The device audio callback (`glo_render`) drains it with linear resampling to the device rate. Mic input
  flows the other way into `i2s_pop_sample`.
- The codec registers are not emulated; `glo_render` applies the firmware's `codec_digital_volume`/`codec_analog_volume`
  (0.5 dB steps, mute values honoured) times a master gain.
- Buttons: left / middle / right on-screen button = U2 / U1 (PWR) / U3 (keys 1 / 2 / 3 on desktop). Accelerometer = `Input.get_accelerometer()` in g.
  RGB LED pins are read back for the visuals.
- Flash: `config_glo.txt` at 0x90000 (embedded; override with `<user data>/config_glo.txt`), the sample image at 0xa0000.
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
uv run tools/make_samples.py            # writes godot/data/samples.bin
```

Use it as `godot/data/samples.bin` (packed into the APK), `samples.bin` in the app's user data dir, or `--samples FILE` for glo-desktop.

The official image (gechologic.com `v2/download/samples.bin.3c5017e…`, or read from a device with
`esptool.py read_flash 0xa0000 0x6f0000 samples.bin`) has no published license: fine for personal testing,
but do not put it in an APK you distribute. `godot/data/` packs whatever file is there.

## Build

Requirements: Rust (+ `aarch64-linux-android` target), `cargo-ndk`, Android NDK, Godot 4.7 with export templates.

```sh
git submodule update --init

# desktop harness
cd rust
cargo run --release -p glo-desktop -- --wav out.wav --seconds 20 --channel 2
cargo run --release -p glo-desktop            # realtime; type 1/2/3, 1L, "t 0.5 0 -0.8", q

# Godot on macOS
cargo build --release -p glo-godot && cp target/release/libglo_godot.dylib ../godot/bin/

# Android
export ANDROID_NDK_HOME=/opt/homebrew/share/android-commandlinetools/ndk/29.0.14206865
cargo ndk -t arm64-v8a --platform 26 build --release -p glo-godot
cp target/aarch64-linux-android/release/libglo_godot.so ../godot/bin/
cd ../godot && godot --headless --import && godot --headless --export-debug Android
adb install -r ../build/glo-mobile.apk
```

The first build of `godot-core` (generated Godot bindings) takes several minutes per target; later builds are incremental.
Firmware log output goes to logcat with the tag `glo`.

## License

GPL-3.0 (`LICENSE`), as is the upstream firmware. Third-party components and their licenses are listed in
`NOTICE.md`; the app shows the required notices behind the ⓘ button. When you distribute an APK, publish the exact
source revision it was built from (including the `firmware` submodule commit). Because of GPL-3.0, the Apple App
Store is not an option without permission from all copyright holders.
