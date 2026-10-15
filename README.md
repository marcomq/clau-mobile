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
godot/           app: 3 full-height touch zones = buttons, shader visuals from LED + spectrum
```

## How it works

- The firmware's audio loop calls `i2s_push_sample` per stereo sample at its own engine rate (50780 Hz by default).
  The shim puts samples into a small SPSC ring (default 512 frames ≈ 10 ms, `glo_set_ring_frames`) and blocks while it is full.
  The device audio callback (`glo_render`) drains it with linear resampling to the device rate. Mic input
  flows the other way into `i2s_pop_sample`.
- The codec registers are not emulated; `glo_render` applies the firmware's `codec_digital_volume`/`codec_analog_volume`
  (0.5 dB steps, mute values honoured) times a master gain.
- Buttons: left / middle / right touch zone = U2 / U1 (PWR) / U3. Accelerometer = `Input.get_accelerometer()` in g.
  RGB LED pins are read back for the visuals.
- Flash: `config_glo.txt` at 0x90000 (embedded; override with `<user data>/config_glo.txt`), the sample image at 0xa0000.
- NVS (settings, channel order) is stored as files under `<user data>/nvs/`.
- `esp_restart()` and power-off end all firmware threads at their next shim call. After that the
  supervisor boots `app_main` again (after power-off: when U1 is held). Firmware globals are not re-initialised on
  restart (unlike real hardware).
- The published v1.0 source never sets `glo_run = 1` (later releases do), which would leave only the fallback
  loop; the supervisor sets it before boot. No firmware file is modified.

## Sample image (not included)

Most Whale channels mix filtered background samples from flash (`_all_samples.bin`, ~7.3 MB, version 09 per
`config_glo.txt`). It is not part of the Glo_v1 repository. Without it those channels are near-silent. Provide it as either:

- `godot/data/samples.bin` (packed into the APK), or
- `samples.bin` in the app's user data dir, or
- `--samples FILE` for glo-desktop.

It can be read from a real Glo with `esptool.py read_flash 0xa0000 0x6f0000 samples.bin`.

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
