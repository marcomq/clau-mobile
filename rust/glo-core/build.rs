//! Compiles the unmodified Glo firmware (v1.0 Whale) together with the fake-ESP32 shim in `port/`.

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
                out.push(p);
            }
        }
    }
    out.sort();
    out
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

    let c_objs = firmware_build(&root, &fw)
        .std("gnu11")
        .files(sources(&fw, &["c"]))
        .file(root.join("port/src/agc_port.c"))
        .compile_intermediates();
    let shim_objs = cc::Build::new()
        .include(root.join("port/include"))
        .include(root.join("port"))
        .std("gnu11")
        .opt_level(2)
        .files(SHIM_C.iter().map(|f| root.join("port/src").join(f)))
        .compile_intermediates();

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
}
