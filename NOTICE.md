# Notices

Flono, Copyright (C) 2026 Marco Mengelkoch, is free software under the GNU General Public
License v3.0 (see `LICENSE`). It comes with ABSOLUTELY NO WARRANTY. Source: https://github.com/marcomq/flono (firmware submodule:
https://github.com/h3o/Glo_v1, commit aad6c17).

Distributing a build (APK) means distributing the firmware and everything below: ship `LICENSE` and this
file with it and point recipients to the exact source revision.

## Components

| Component | Where | License |
|---|---|---|
| Flono port, bindings, app, sample generator, Copyright (C) 2026 Marco Mengelkoch | `port/`, `rust/`, `godot/`, `tools/` | GPL-3.0 |
| Glo / Gecho firmware v1.0, gechologic / phonicbloom (h3o) | `firmware/` | GPL-3.0 (upstream since v1.0.118; the v1.0 snapshot carries the ESP-IDF template's Apache-2.0 text) |
| Mutable Instruments Clouds and stmlib, Copyright Olivier Gillet | `firmware/main/mi/` | MIT |
| Dekrispator, Xavier Halgand; blepvco and minBLEP tables, Copyright Sean Bolton; resonant filter, Copyright Julian Schmidt | `firmware/main/dkr/` | GPL-2.0-or-later / per file header |
| KX123 driver headers, Copyright Rohm Semiconductor | `firmware/main/hw/kx123*` | Apache-2.0 |
| Godot Engine and its third-party components | APK runtime | MIT and others (`Engine.get_copyright_info()`) |
| godot-rust (gdext) | Rust dependency | MPL-2.0 |
| cpal | Rust dependency | Apache-2.0 |
| rtrb, hound, ndk, jni, libc and other crates | Rust dependencies | MIT and/or Apache-2.0 (see `cargo metadata`) |

All of the above are compatible with distribution under GPL-3.0.

## Sample image

The official Glo sample image (`_all_samples.bin`, gechologic.com) has no published license and must not be
redistributed. `tools/make_samples.py` synthesizes a replacement with the same layout; its output is
covered by this project's license.

## Disclaimer

Flono is an independent, unofficial project. It is not affiliated with, endorsed, sponsored or supported by
Phonicbloom, gechologic or the Glo project, and claims no trademark, brand or other rights to their names,
products or artwork. "Glo" is used only to identify the original instrument whose firmware Flono runs.
