//! Compiles the Glo firmware (v1.0.118 Whale) together with the fake-ESP32 shim in `port/`.

use std::path::{Path, PathBuf};

const FW_DIRS: &[&str] = &[
    "", "hw", "dsp", "dkr", "v1", "mi/clouds", "mi/clouds/drivers", "mi/clouds/dsp",
    "mi/clouds/dsp/pvoc", "mi/stmlib/dsp", "mi/stmlib/utils",
];
const FW_INCLUDE_DIRS: &[&str] = &[
    "", "hw", "dsp", "dkr", "v1", "mi", "mi/clouds", "mi/clouds/drivers", "mi/clouds/dsp",
    "mi/clouds/dsp/fx", "mi/clouds/dsp/pvoc", "mi/stmlib", "mi/stmlib/dsp", "mi/stmlib/fft",
    "mi/stmlib/utils",
];
/// Replaced by `port/src/kx123_port.cpp`.
const FW_EXCLUDE: &[&str] = &["hw/kx123.cpp"];
/// Upstream Whale build bugs, fixed in a copy under OUT_DIR: (file, exact text, replacement).
const FW_PATCHES: &[(&str, &str, &str)] = &[
    ("dkr/soundGen.c", "DKR_FX_OPTIONS_CHANGE; event_channel_options = 0;\n", "DKR_FX_OPTIONS_CHANGE; event_channel_options = 0; }\n"),
    ("FilteredChannels.cpp", "\t#ifdef BOARD_GECHO\n\n\tfloat reverb_volume", "\tfloat reverb_volume"),
    ("FilteredChannels.cpp", "\n\tif(persistent_settings.SAMPLING_RATE != SAMPLE_RATE_DEFAULT)", "\n\t#ifdef BOARD_GECHO\n\tif(persistent_settings.SAMPLING_RATE != SAMPLE_RATE_DEFAULT)"),
    // The Whale loads the channel map once at boot, so freeing its strings per channel is a use-after-free.
    ("main.cpp", "\t\tfree(channels_map[channel_loop_remapped].name);\n", "\t\t#ifndef BOARD_WHALE\n\t\tfree(channels_map[channel_loop_remapped].name);\n"),
    ("main.cpp", "str_param = NULL;\n\t\t}\n", "str_param = NULL;\n\t\t}\n\t\t#endif\n"),
    ("glo_config.c", "\tstrcpy(buf,formula);\n", "\tsnprintf(buf, sizeof(buf), \"%s\", formula);\n"),
    // An empty or comment-only line read line_buffer[-1]: SIGBUS when the buffer starts a page.
    ("glo_config.c", "while (line_buffer[line_length - 1] == '\\t') {", "while (line_length > 0 && line_buffer[line_length - 1] == '\\t') {"),
    ("glo_config.c", "while (line_buffer[line_length - 1] == ' ') {", "while (line_length > 0 && line_buffer[line_length - 1] == ' ') {"),
    // The caller's map has MAIN_CHANNELS+1 entries and the parser never checked; a longer [channels] list overflowed the stack.
    ("glo_config.c", "\t\t\t\t\tchannels_found++;\n", "\t\t\t\t\tif(++channels_found > MAIN_CHANNELS) done = 1;\n"),
    // The Whale has no IR sensors; the Gecho default (IRS) would leave the accelerometer off.
    ("glo_config.c", "    use_acc_or_ir_sensors = persistent_settings.PARAMS_SENSORS;\n", "    #ifdef BOARD_GECHO\n    use_acc_or_ir_sensors = persistent_settings.PARAMS_SENSORS;\n    #endif\n"),
    // Only the Gecho branch stops the tempo task; on the Whale it would pile up and steal mic samples.
    ("main.cpp", "#define LISTEN_FOR_TEMPO_AT_IDLE\n", "#ifndef BOARD_WHALE\n#define LISTEN_FOR_TEMPO_AT_IDLE\n#endif\n"),
    // stmlib's Interpolate() reads table[i + 1], one past the end at index 1.0: repeat each table's last value.
    ("mi/clouds/resources.cc", "   1.000000000e+00,\n};\nconst float lut_xfade_in[] = {", "   1.000000000e+00,  1.000000000e+00,\n};\nconst float lut_xfade_in[] = {"),
    ("mi/clouds/resources.cc", "   7.071067812e-01,\n};\nconst float lut_xfade_out[] = {", "   7.071067812e-01,  7.071067812e-01,\n};\nconst float lut_xfade_out[] = {"),
    ("mi/clouds/resources.cc", "   4.329780281e-17,\n};\n", "   4.329780281e-17,  4.329780281e-17,\n};\n"),
    // The flanger's filter gain (x1.94) and high-pass leave int16 range on loud mic input.
    ("dsp/FilterFlanger.cpp", "sample_i16 = sample_lpf[0] * (1 + (1-ADC_LPF_ALPHA) * 1);", "sample_i16 = glo_sat16(sample_lpf[0] * (1 + (1-ADC_LPF_ALPHA) * 1));"),
    ("dsp/FilterFlanger.cpp", "sample_i16 = sample_lpf[1] * (1 + (1-ADC_LPF_ALPHA) * 1);", "sample_i16 = glo_sat16(sample_lpf[1] * (1 + (1-ADC_LPF_ALPHA) * 1));"),
    ("dsp/FilterFlanger.cpp", "\t\t\tsample_i16 -= sample_hpf[1];", "\t\t\tsample_i16 = glo_sat16(sample_i16 - sample_hpf[1]);"),
    ("dsp/FilterFlanger.cpp", "\t\t\tsample_i16 -= sample_hpf[3];", "\t\t\tsample_i16 = glo_sat16(sample_i16 - sample_hpf[3]);"),
];
/// Like FW_PATCHES, but every occurrence. Loud mic input wrapped around in these float-to-int16 casts: a crack.
const FW_PATCHES_ALL: &[(&str, &str, &str)] = &[
    ("ChannelsDef.cpp", "(int16_t)(sample_mix * SAMPLE_VOLUME)", "glo_sat16(sample_mix * SAMPLE_VOLUME)"),
    ("dsp/FilterFlanger.cpp", "(int16_t)(sample_mix * SAMPLE_VOLUME)", "glo_sat16(sample_mix * SAMPLE_VOLUME)"),
    ("dsp/SamplerLooper.cpp", "(int16_t)(sample_mix * SAMPLE_VOLUME)", "glo_sat16(sample_mix * SAMPLE_VOLUME)"),
    ("hw/signals.c", "sample = (int16_t)echo_mix_f;", "sample = glo_sat16(echo_mix_f);"),
    // The resonant filters ring far beyond int16 on loud input.
    ("filtered_channel_loop.inc", "(int16_t)(sample_mix * SAMPLE_VOLUME)", "glo_sat16(sample_mix * SAMPLE_VOLUME)"),
    ("filtered_channel_loop.inc", "sample_i16 += (int16_t)(sample_f[0]*MAIN_VOLUME*SAMPLE_VOLUME);", "sample_i16 = glo_sat16(sample_i16 + sample_f[0]*MAIN_VOLUME*SAMPLE_VOLUME);"),
    ("filtered_channel_loop.inc", "sample_i16 += (int16_t)(sample_f[1]*MAIN_VOLUME*SAMPLE_VOLUME);", "sample_i16 = glo_sat16(sample_i16 + sample_f[1]*MAIN_VOLUME*SAMPLE_VOLUME);"),
];
/// Included by FilteredChannels.cpp, whose patched copy finds them next to it before the original.
const FW_PATCHED_INCLUDES: &[&str] = &["filtered_channel_loop.inc"];
const SHIM_C: &[&str] = &["shim_rtos.c", "shim_gpio.c", "shim_i2s.c", "shim_sys.c", "shim_storage.c", "glo_core_api.c"];

fn sources(fw: &Path, exts: &[&str]) -> Vec<PathBuf> {
    let mut out = Vec::new();
    for dir in FW_DIRS {
        let Ok(entries) = std::fs::read_dir(fw.join(dir)) else { continue };
        for e in entries.flatten() {
            let p = e.path();
            let rel = p.strip_prefix(fw).unwrap().to_string_lossy().replace('\\', "/");
            let ext_ok = p.extension().and_then(|x| x.to_str()).is_some_and(|x| exts.contains(&x));
            if p.is_file() && ext_ok && !FW_EXCLUDE.contains(&rel.as_str()) {
                out.push(patched(&p, &rel));
            }
        }
    }
    out.sort();
    out
}

fn patched(src: &Path, rel: &str) -> PathBuf {
    let patches: Vec<_> = FW_PATCHES.iter().filter(|p| p.0 == rel).collect();
    let patches_all: Vec<_> = FW_PATCHES_ALL.iter().filter(|p| p.0 == rel).collect();
    if patches.is_empty() && patches_all.is_empty() {
        return src.to_path_buf();
    }
    let mut text = std::fs::read_to_string(src).unwrap();
    for (_, from, to) in patches {
        assert_eq!(text.matches(from).count(), 1, "patch for {rel} does not apply: {from:?}");
        text = text.replacen(from, to, 1);
    }
    for (_, from, to) in patches_all {
        assert!(text.contains(from), "patch for {rel} does not apply: {from:?}");
        text = text.replace(from, to);
    }
    let out = PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("patched").join(rel);
    std::fs::create_dir_all(out.parent().unwrap()).unwrap();
    std::fs::write(&out, text).unwrap();
    out
}

/// v1.0.118's Clouds halts without its per-mode patch blocks, which only `config_gecho.txt` has.
/// The Infinite Looper (Gecho channel 24, Whale v2 channel 8) becomes the 25th, unlockable channel.
fn whale_config(fw_data: &Path, out: &Path) {
    let mut glo = std::fs::read_to_string(fw_data.join("config_glo.txt")).unwrap();
    let gecho = std::fs::read_to_string(fw_data.join("config_gecho.txt")).unwrap();
    let last = "binaural=1\t\t//Notjustmoreidlechatter, voiceover\n";
    assert_eq!(glo.matches(last).count(), 1, "config_glo.txt: {last:?} must occur exactly once");
    glo = glo.replace(last, &format!("{last}infinite_looper:settings=1\t\t\t\t//looper with warping and reversible delay\n"));
    let block = |text: &str, start: &str, end: &str| -> (usize, usize) {
        let s = text.find(start).unwrap_or_else(|| panic!("{start} missing"));
        (s, s + text[s..].find(end).unwrap_or_else(|| panic!("{end} missing after {start}")))
    };
    let (gs, ge) = block(&glo, "[clouds_patches]\n", "[something_else]");
    let (cs, ce) = block(&gecho, "[clouds_patches_granular]", "[something_else]");
    std::fs::write(out, [&glo[..gs], &gecho[cs..ce], &glo[ge..]].concat()).unwrap();
}

fn firmware_build(root: &Path, fw: &Path) -> cc::Build {
    let mut b = cc::Build::new();
    b.include(root.join("port/include"));
    for d in FW_INCLUDE_DIRS {
        b.include(fw.join(d));
    }
    // Searched last, so a case-insensitive file system never picks an alias over the real header.
    b.include(root.join("port/case"));
    b.define("GLO_PORT", None)
        .define("BOARD_WHALE", None)
        .define("ESP_PLATFORM", None)
        .flag("-include")
        .flag(root.join("port/include/glo_port_prelude.h").to_str().unwrap())
        .flag("-w")
        .flag("-fno-strict-aliasing")
        .flag("-fwrapv")
        .flag("-fcommon")
        .opt_level(2)
        .warnings(false);
    b
}

fn main() {
    let manifest = PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap());
    let root = manifest.join("../..").canonicalize().unwrap();
    let fw = root.join("firmware/main");
    assert!(fw.join("main.cpp").exists(), "firmware submodule missing: run `git submodule update --init`");
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();
    let target_env = std::env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default();
    assert!(target_env != "msvc", "the firmware shim needs pthreads and C11 atomics: build with x86_64-pc-windows-gnu (MinGW)");

    let out_dir = PathBuf::from(std::env::var("OUT_DIR").unwrap());
    whale_config(&fw.join("../data"), &out_dir.join("config_glo.txt"));

    let c_objs = firmware_build(&root, &fw)
        .std("gnu11")
        .files(sources(&fw, &["c"]))
        .file(root.join("port/src/agc_port.c"))
        .file(root.join("port/src/gecho_stubs.c"))
        .compile_intermediates();
    let shim_objs = cc::Build::new()
        .include(root.join("port/include"))
        .include(root.join("port"))
        .std("gnu11")
        .opt_level(2)
        .files(SHIM_C.iter().map(|f| root.join("port/src").join(f)))
        .compile_intermediates();

    for inc in FW_PATCHED_INCLUDES {
        patched(&fw.join(inc), inc);
    }

    // One archive: firmware and shim reference each other in both directions.
    let mut cpp = firmware_build(&root, &fw);
    cpp.cpp(true)
        .std("gnu++14")
        .flag("-fms-extensions")
        .files(sources(&fw, &["cpp", "cc"]))
        .file(root.join("port/src/kx123_port.cpp"))
        .objects(c_objs)
        .objects(shim_objs);
    if target_os == "android" {
        cpp.cpp_link_stdlib("c++_static");
        println!("cargo:rustc-link-lib=c++abi");
    }
    if target_os == "windows" {
        println!("cargo:rustc-link-lib=pthread");
    }
    cpp.compile("glo_firmware");

    println!("cargo:rerun-if-changed={}", root.join("port").display());
    println!("cargo:rerun-if-changed={}", fw.display());
    println!("cargo:rerun-if-changed={}", fw.join("../data").display());
}
