//! Host core for clau-mobile: the unmodified Glo Whale firmware running on a fake ESP32,
//! plus a low-latency cpal audio engine that drives it.

#[cfg(target_os = "android")]
pub mod android;
pub mod audio;
pub mod ffi;
#[cfg(target_os = "android")]
pub mod logcat;
pub mod spectrum;

use std::ffi::CString;
use std::os::raw::{c_char, c_int};
use std::path::Path;

/// Default flash config (firmware `config_glo.txt` plus the Gecho Clouds patches), overridable by `<data_dir>/config_glo.txt`.
pub const DEFAULT_CONFIG: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/config_glo.txt"));

pub const BUTTON_PWR: i32 = 1;
pub const BUTTON_U2: i32 = 2;
pub const BUTTON_U3: i32 = 3;

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Levels {
    pub rms: [f32; 2],
    pub bands: [f32; 8],
}

/// Boots the firmware once per process. `samples` is the `_all_samples.bin` flash image, if available
/// (otherwise `<data_dir>/samples.bin` is tried); it is copied before this returns.
pub fn start(data_dir: &Path, samples: Option<&[u8]>) -> Result<(), String> {
    std::fs::create_dir_all(data_dir).map_err(|e| format!("data dir {}: {e}", data_dir.display()))?;
    let dir = CString::new(data_dir.to_string_lossy().as_bytes()).map_err(|e| e.to_string())?;
    // SAFETY: pointers are valid for the duration of the calls; glo_start copies the samples.
    let rc = unsafe {
        ffi::glo_set_config(DEFAULT_CONFIG.as_ptr().cast(), DEFAULT_CONFIG.len());
        if let Some(s) = samples {
            ffi::glo_set_samples(s.as_ptr(), s.len());
        }
        let rc = ffi::glo_start(dir.as_ptr());
        ffi::glo_set_samples(std::ptr::null(), 0);
        rc
    };
    if rc == 0 { Ok(()) } else { Err(format!("glo_start failed ({rc})")) }
}

/// Realtime-safe. `out` and `input` are interleaved stereo; `input` must match `out` in length.
pub fn render(out: &mut [f32], input: Option<&[f32]>, device_rate: u32) {
    let frames = out.len() / 2;
    let in_ptr = match input {
        Some(i) if i.len() >= frames * 2 => i.as_ptr(),
        _ => std::ptr::null(),
    };
    // SAFETY: out holds `frames` stereo frames, input is null or at least as long.
    unsafe { ffi::glo_render(out.as_mut_ptr(), in_ptr, frames as i32, device_rate as i32) }
}

/// Accelerometer in g; (0, 0, -1) means lying flat, face up.
pub fn set_accel(x: f32, y: f32, z: f32) {
    unsafe { ffi::glo_set_accel(x, y, z) }
}

pub fn set_button(idx: i32, down: bool) {
    unsafe { ffi::glo_set_button(idx, down as i32) }
}

pub fn set_ring_frames(frames: u32) {
    unsafe { ffi::glo_set_ring_frames(frames.min(i32::MAX as u32) as i32) }
}

pub fn set_master_gain(gain: f32) {
    unsafe { ffi::glo_set_master_gain(gain) }
}

pub fn led() -> [bool; 3] {
    let (mut r, mut g, mut b) = (0, 0, 0);
    unsafe { ffi::glo_get_led(&mut r, &mut g, &mut b) };
    [r != 0, g != 0, b != 0]
}

pub fn channel() -> i32 {
    unsafe { ffi::glo_get_channel() }
}

/// Index of the running channel in the config's `[channels]` list (differs from `channel()` after reordering).
pub fn channel_id() -> i32 {
    unsafe { ffi::glo_get_channel_id() }
}

pub fn engine_rate() -> u32 {
    unsafe { ffi::glo_get_engine_rate() as u32 }
}

pub fn is_powered() -> bool {
    unsafe { ffi::glo_is_powered() != 0 }
}

/// Engine frames the output had to fill with silence because the firmware fell behind.
pub fn underruns() -> u32 {
    unsafe { ffi::glo_get_underruns() }
}

/// Lowers the mic AGC max gain by `db` on top of the phone-mic offset (for hotter mics, e.g. USB).
pub fn set_mic_offset_db(db: i32) {
    unsafe { ffi::glo_set_mic_offset_db(db) }
}

/// The user's mic sensitivity trim in dB (raises or lowers the AGC max gain).
pub fn set_mic_trim_db(db: i32) {
    unsafe { ffi::glo_set_mic_trim_db(db) }
}

/// The firmware's mic AGC max gain in dB (6x U2 raises, 6x U3 lowers it), `None` while the AGC is off.
pub fn mic_agc_db() -> Option<i32> {
    let db = unsafe { ffi::glo_get_mic_agc_db() };
    (db >= 0).then_some(db)
}

/// Short presses typed so far and not yet decoded: `a` = U1, `b` = U2, `c` = U3.
pub fn button_sequence() -> String {
    let mut buf = [0 as c_char; 16];
    let n = unsafe { ffi::glo_get_button_sequence(buf.as_mut_ptr(), buf.len() as c_int) };
    buf[..n.max(0) as usize].iter().map(|&c| c as u8 as char).collect()
}

/// Decoded commands the running channel has not picked up yet: bit 0 = sequence, 1 = U2 tap, 2 = U3 tap.
pub fn pending_commands() -> u32 {
    unsafe { ffi::glo_get_pending_command() as u32 }
}

/// Output volume in dB, `None` when muted.
pub fn volume_db() -> Option<f32> {
    let db = unsafe { ffi::glo_get_volume_db() };
    (db > -1000.0).then_some(db)
}

/// Stores the extra-channel unlock (as the firmware's − + − + − + − + ▷ does) and restarts the firmware.
pub fn set_all_channels_unlocked(on: bool) {
    unsafe { ffi::glo_set_all_channels_unlocked(on as c_int) }
}

pub fn all_channels_unlocked() -> bool {
    unsafe { ffi::glo_get_all_channels_unlocked() != 0 }
}

pub fn levels() -> Levels {
    let mut l = Levels::default();
    unsafe { ffi::glo_get_levels(&mut l.rms[0], &mut l.rms[1], l.bands.as_mut_ptr()) };
    l
}
