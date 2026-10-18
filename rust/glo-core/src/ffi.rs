//! Raw bindings to `port/glo_core.h`.

use std::os::raw::{c_char, c_float, c_int, c_uint};

extern "C" {
    pub fn glo_set_config(text: *const c_char, len: usize);
    pub fn glo_set_samples(data: *const u8, len: usize);
    pub fn glo_start(data_dir: *const c_char) -> c_int;
    pub fn glo_render(out_lr: *mut c_float, in_lr: *const c_float, frames: c_int, device_rate: c_int);
    pub fn glo_set_accel(x: c_float, y: c_float, z: c_float);
    pub fn glo_set_button(idx: c_int, down: c_int);
    pub fn glo_set_ring_frames(frames: c_int);
    pub fn glo_set_master_gain(gain: c_float);
    pub fn glo_get_led(r: *mut c_int, g: *mut c_int, b: *mut c_int);
    pub fn glo_get_channel() -> c_int;
    pub fn glo_get_channel_id() -> c_int;
    pub fn glo_get_engine_rate() -> c_int;
    pub fn glo_is_powered() -> c_int;
    pub fn glo_get_underruns() -> c_uint;
    pub fn glo_get_mic_agc_db() -> c_int;
    pub fn glo_set_mic_offset_db(db: c_int);
    pub fn glo_set_mic_trim_db(db: c_int);
    pub fn glo_get_levels(rms_l: *mut c_float, rms_r: *mut c_float, bands: *mut c_float);
    pub fn glo_get_scope(out: *mut c_float, n: c_int) -> c_int;
    pub fn glo_get_button_sequence(out: *mut c_char, max: c_int) -> c_int;
    pub fn glo_get_pending_command() -> c_int;
    pub fn glo_get_volume_db() -> c_float;
    pub fn glo_set_all_channels_unlocked(on: c_int);
    pub fn glo_get_all_channels_unlocked() -> c_int;
}
