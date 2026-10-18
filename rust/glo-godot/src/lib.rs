//! Godot binding: `GloNode` boots the firmware, owns the audio streams and feeds it accelerometer/buttons.

use std::path::PathBuf;

use glo_core::audio::{AudioEngine, AudioOptions, MicSource};
use glo_core::spectrum::Analyzer;
use godot::classes::{INode, Input, Node};
use godot::prelude::*;

struct GloExtension;

#[gdextension]
unsafe impl ExtensionLibrary for GloExtension {}

const STANDARD_GRAVITY: f32 = 9.81;

#[derive(GodotClass)]
#[class(base = Node)]
pub struct GloNode {
    engine: Option<AudioEngine>,
    /// Feed `Input.get_accelerometer()` to the firmware every frame.
    #[export]
    use_accelerometer: bool,
    /// Multiplies the accelerometer (per axis) before it reaches the firmware, to fix orientation.
    #[export]
    accel_sign: Vector3,
    /// Android: preferred microphone, "builtin", "usb" or "headset"; used when connected (applies at the next
    /// `start_audio`).
    #[export]
    mic_source: GString,
    underruns: u32,
    underrun_timer: f64,
    analyzer: Analyzer,
    base: Base<Node>,
}

#[godot_api]
impl INode for GloNode {
    fn init(base: Base<Node>) -> Self {
        Self {
            engine: None,
            use_accelerometer: true,
            accel_sign: Vector3::ONE,
            mic_source: "builtin".into(),
            underruns: 0,
            underrun_timer: 0.0,
            analyzer: Analyzer::default(),
            base,
        }
    }

    fn process(&mut self, delta: f64) {
        self.underrun_timer -= delta;
        if self.underrun_timer <= 0.0 {
            self.underrun_timer = 2.0;
            let n = glo_core::underruns();
            if n != self.underruns {
                eprintln!("glo: {} underrun frames in the last 2 s", n.wrapping_sub(self.underruns));
                self.underruns = n;
            }
        }
        if self.use_accelerometer && self.engine.is_some() {
            let a = Input::singleton().get_accelerometer();
            if a != Vector3::ZERO {
                let g = a * self.accel_sign / STANDARD_GRAVITY;
                glo_core::set_accel(g.x, g.y, g.z);
            }
        }
    }
}

#[godot_api]
impl GloNode {
    /// Boots the firmware. `data_dir` is an absolute, writable path (NVS, optional samples.bin/config_glo.txt).
    /// `samples` may be empty; otherwise it is the `_all_samples.bin` flash image.
    #[func]
    fn boot(&mut self, data_dir: GString, samples: PackedByteArray) -> bool {
        #[cfg(target_os = "android")]
        glo_core::logcat::redirect_stdio();
        let samples = (!samples.is_empty()).then(|| samples.as_slice());
        match glo_core::start(&PathBuf::from(data_dir.to_string()), samples) {
            Ok(()) => true,
            Err(e) => {
                godot_error!("glo: {e}");
                false
            }
        }
    }

    /// Opens the audio device. `buffer_frames` <= 0 lets the platform choose its burst size.
    #[func]
    fn start_audio(&mut self, use_mic: bool, buffer_frames: i32) -> bool {
        self.engine = None;
        let opts = AudioOptions {
            use_mic,
            mic_source: parse_mic_source(&self.mic_source.to_string()),
            buffer_frames: (buffer_frames > 0).then_some(buffer_frames as u32),
            ..Default::default()
        };
        match AudioEngine::start(opts) {
            Ok(engine) => {
                godot_print!("glo: audio {} Hz, mic {:?}", engine.sample_rate, engine.mic_name);
                self.engine = Some(engine);
                true
            }
            Err(e) => {
                godot_error!("glo: {e}");
                false
            }
        }
    }

    #[func]
    fn stop_audio(&mut self) {
        self.engine = None;
    }

    #[func]
    fn is_mic_active(&self) -> bool {
        self.engine.as_ref().is_some_and(|e| e.mic_active)
    }

    /// Connected microphone sources ("builtin", "usb", "headset"); empty when unknown (desktop).
    #[func]
    fn get_mic_sources(&self) -> PackedStringArray {
        glo_core::audio::mic_sources().unwrap_or_default().into_iter().map(|s| GString::from(mic_source_name(s))).collect()
    }

    /// Source of the current recording device ("builtin", "usb" or "headset").
    #[func]
    fn get_active_mic_source(&self) -> GString {
        mic_source_name(self.engine.as_ref().map_or(MicSource::Builtin, |e| e.mic_source)).into()
    }

    /// Mic sensitivity trim in dB on top of the firmware's AGC setting.
    #[func]
    fn set_mic_trim_db(&self, db: i32) {
        glo_core::set_mic_trim_db(db);
    }

    /// Name of the recording device, empty without mic.
    #[func]
    fn get_mic_name(&self) -> GString {
        self.engine.as_ref().and_then(|e| e.mic_name.as_deref()).unwrap_or("").into()
    }

    /// False only when headphones are known to be missing (Android); true when connected or unknown.
    #[func]
    fn has_headphones(&self) -> bool {
        glo_core::audio::headphones_connected().unwrap_or(true)
    }

    /// idx 1 = U1 (power / next channel), 2 = U2, 3 = U3.
    #[func]
    fn set_button(&self, idx: i32, down: bool) {
        glo_core::set_button(idx, down);
    }

    #[func]
    fn set_accel(&self, g: Vector3) {
        glo_core::set_accel(g.x, g.y, g.z);
    }

    #[func]
    fn set_ring_frames(&self, frames: i32) {
        glo_core::set_ring_frames(frames.max(0) as u32);
    }

    #[func]
    fn set_master_gain(&self, gain: f32) {
        glo_core::set_master_gain(gain);
    }

    #[func]
    fn get_led(&self) -> Color {
        let [r, g, b] = glo_core::led();
        Color::from_rgb(r as u8 as f32, g as u8 as f32, b as u8 as f32)
    }

    #[func]
    fn get_channel(&self) -> i32 {
        glo_core::channel()
    }

    /// Index of the running channel in the config's `[channels]` list.
    #[func]
    fn get_channel_id(&self) -> i32 {
        glo_core::channel_id()
    }

    #[func]
    fn get_engine_rate(&self) -> i32 {
        glo_core::engine_rate() as i32
    }

    #[func]
    fn is_powered(&self) -> bool {
        glo_core::is_powered()
    }

    /// The firmware's mic AGC max gain in dB (6x U2 / 6x U3), -1 while the AGC is off.
    #[func]
    fn get_mic_agc_db(&self) -> i32 {
        glo_core::mic_agc_db().unwrap_or(-1)
    }

    /// Short presses typed so far and not yet decoded: `a` = U1, `b` = U2, `c` = U3.
    #[func]
    fn get_button_sequence(&self) -> GString {
        GString::from(glo_core::button_sequence().as_str())
    }

    /// Decoded commands the running channel has not picked up yet: bit 0 = sequence, 1 = U2 tap, 2 = U3 tap.
    #[func]
    fn get_pending_commands(&self) -> i32 {
        glo_core::pending_commands() as i32
    }

    /// Output volume in dB, -1000 when muted.
    #[func]
    fn get_volume_db(&self) -> f32 {
        glo_core::volume_db().unwrap_or(-1000.0)
    }

    /// Stores the extra-channel unlock and restarts the firmware.
    #[func]
    fn set_all_channels_unlocked(&self, on: bool) {
        glo_core::set_all_channels_unlocked(on);
    }

    #[func]
    fn are_all_channels_unlocked(&self) -> bool {
        glo_core::all_channels_unlocked()
    }

    /// [rms_l, rms_r, band0 .. band7], bands from 60 Hz to 12 kHz.
    #[func]
    fn get_levels(&self) -> PackedFloat32Array {
        let l = glo_core::levels();
        let mut v = l.rms.to_vec();
        v.extend_from_slice(&l.bands);
        PackedFloat32Array::from(v.as_slice())
    }

    /// [spectrum (`bins` levels 0..1, 40 Hz .. 16 kHz, log-spaced), wave (`points` samples -1..1, about 10 ms)].
    #[func]
    fn analyze(&mut self, bins: u32, points: u32) -> PackedFloat32Array {
        let mut v = vec![0.0; (bins + points) as usize];
        let (spectrum, wave) = v.split_at_mut(bins as usize);
        self.analyzer.analyze(spectrum, wave);
        PackedFloat32Array::from(v.as_slice())
    }
}

fn mic_source_name(s: MicSource) -> &'static str {
    match s {
        MicSource::Builtin => "builtin",
        MicSource::Usb => "usb",
        MicSource::Headset => "headset",
    }
}

fn parse_mic_source(s: &str) -> MicSource {
    match s {
        "usb" => MicSource::Usb,
        "headset" => MicSource::Headset,
        _ => MicSource::Builtin,
    }
}
