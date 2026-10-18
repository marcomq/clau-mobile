//! Duplex audio: cpal output drives `glo_render`; the mic (if permitted) feeds it through a ring.

use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::Arc;

use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use cpal::{BufferSize, Stream, StreamConfig};

const MAX_FRAMES: usize = 4096;
/// USB mics hiss far less than phone mics, so the AGC would lift room noise to full level.
const USB_MIC_OFFSET_DB: i32 = 15;

/// Which microphone to record from (Android). Unavailable sources fall back to the built-in mic.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum MicSource {
    #[default]
    Builtin,
    Usb,
    /// Wired or Bluetooth headset mic (USB headsets count as `Usb`).
    Headset,
}

#[derive(Clone, Copy, Debug)]
pub struct AudioOptions {
    pub sample_rate: u32,
    /// Device callback size in frames; `None` lets the platform choose (AAudio: one burst).
    pub buffer_frames: Option<u32>,
    pub use_mic: bool,
    /// Android: the preferred microphone, used when it is connected.
    pub mic_source: MicSource,
}

impl Default for AudioOptions {
    fn default() -> Self {
        Self { sample_rate: 48_000, buffer_frames: None, use_mic: true, mic_source: MicSource::Builtin }
    }
}

pub struct AudioEngine {
    _output: Stream,
    _input: Option<Stream>,
    pub sample_rate: u32,
    pub mic_active: bool,
    pub mic_name: Option<String>,
    pub mic_source: MicSource,
}

fn config(opts: &AudioOptions, channels: u16) -> StreamConfig {
    StreamConfig {
        channels,
        sample_rate: opts.sample_rate,
        buffer_size: opts.buffer_frames.map_or(BufferSize::Default, BufferSize::Fixed),
    }
}

/// Android devices with their descriptions; empty when enumeration is unavailable (see `android.rs`).
#[cfg(target_os = "android")]
fn android_devices(host: &cpal::Host, input: bool) -> Vec<(cpal::Device, cpal::DeviceDescription)> {
    if !crate::android::init_context() {
        return Vec::new();
    }
    let devs = if input {
        host.input_devices().map(|d| d.collect::<Vec<_>>())
    } else {
        host.output_devices().map(|d| d.collect::<Vec<_>>())
    };
    devs.unwrap_or_default().into_iter().filter_map(|d| d.description().ok().map(|desc| (d, desc))).collect()
}

#[cfg(target_os = "android")]
fn is_headphones(desc: &cpal::DeviceDescription) -> bool {
    use cpal::{DeviceType, InterfaceType};
    matches!(desc.device_type(), DeviceType::Headphones | DeviceType::Headset | DeviceType::HearingAid)
        || desc.device_type() != DeviceType::Speaker
            && matches!(desc.interface_type(), InterfaceType::Bluetooth | InterfaceType::Usb | InterfaceType::Line)
}

/// The output side of a USB microphone (e.g. its monitor jack), which is not headphones.
/// USB headsets and headphone adapters report as headsets and are not affected.
#[cfg(target_os = "android")]
fn is_usb_mic_output(out: &cpal::DeviceDescription, inputs: &[(cpal::Device, cpal::DeviceDescription)]) -> bool {
    use cpal::{DeviceType, InterfaceType};
    out.interface_type() == InterfaceType::Usb
        && out.device_type() != DeviceType::Headset
        && inputs.iter().any(|(_, i)| i.interface_type() == InterfaceType::Usb && i.name() == out.name())
}

/// Bluetooth mics are skipped: Android only routes them in call mode (SCO), which also degrades the output.
fn mic_source_of(desc: &cpal::DeviceDescription) -> Option<MicSource> {
    use cpal::{DeviceType, InterfaceType};
    if desc.interface_type() == InterfaceType::Bluetooth {
        None
    } else if desc.interface_type() == InterfaceType::Usb {
        Some(MicSource::Usb)
    } else if desc.device_type() == DeviceType::Headset {
        Some(MicSource::Headset)
    } else if desc.device_type() == DeviceType::Microphone {
        Some(MicSource::Builtin)
    } else {
        None
    }
}

/// On Android the default input follows routing, so a headset would take over the mic; pin the built-in one,
/// or the preferred `source` when it is connected.
#[cfg(target_os = "android")]
fn input_device(host: &cpal::Host, source: MicSource, _rate: u32) -> Option<cpal::Device> {
    let mut devs = android_devices(host, true);
    let find = |s| devs.iter().position(|(_, d)| mic_source_of(d) == Some(s));
    find(source).or_else(|| find(MicSource::Builtin)).map(|i| devs.swap_remove(i).0).or_else(|| host.default_input_device())
}

/// The default input, unless it cannot record at `rate` (e.g. a Bluetooth headset mic at 16 kHz on macOS); then
/// the first input that can.
#[cfg(not(target_os = "android"))]
fn input_device(host: &cpal::Host, _source: MicSource, rate: u32) -> Option<cpal::Device> {
    let takes_rate = |d: &cpal::Device| {
        d.supported_input_configs()
            .is_ok_and(|mut c| c.any(|r| (r.min_sample_rate()..=r.max_sample_rate()).contains(&rate)))
    };
    host.default_input_device()
        .filter(takes_rate)
        .or_else(|| host.input_devices().ok()?.find(takes_rate))
        .or_else(|| host.default_input_device())
}

/// Whether headphones (wired, USB, Bluetooth, hearing aid, line out) are connected. `None` when unknown:
/// on desktop, or when Android device enumeration is unavailable. A USB microphone's own output does not count.
#[cfg(target_os = "android")]
pub fn headphones_connected() -> Option<bool> {
    if !crate::android::init_context() {
        return None;
    }
    let host = cpal::default_host();
    let inputs = android_devices(&host, true);
    Some(android_devices(&host, false).iter().any(|(_, d)| is_headphones(d) && !is_usb_mic_output(d, &inputs)))
}

#[cfg(not(target_os = "android"))]
pub fn headphones_connected() -> Option<bool> {
    None
}

/// Bluetooth output takes data in large bursts: a fixed callback size then runs several callbacks back to back,
/// faster than the firmware refills its ring.
#[cfg(target_os = "android")]
fn bluetooth_output(host: &cpal::Host) -> bool {
    android_devices(host, false).iter().any(|(_, d)| d.interface_type() == cpal::InterfaceType::Bluetooth)
}

#[cfg(not(target_os = "android"))]
fn bluetooth_output(_host: &cpal::Host) -> bool {
    false
}

/// Connected microphone sources (Android), in `MicSource` order without duplicates. `None` when unknown.
#[cfg(target_os = "android")]
pub fn mic_sources() -> Option<Vec<MicSource>> {
    if !crate::android::init_context() {
        return None;
    }
    let inputs = android_devices(&cpal::default_host(), true);
    let found: Vec<_> = inputs.iter().filter_map(|(_, d)| mic_source_of(d)).collect();
    Some([MicSource::Builtin, MicSource::Usb, MicSource::Headset].into_iter().filter(|s| found.contains(s)).collect())
}

#[cfg(not(target_os = "android"))]
pub fn mic_sources() -> Option<Vec<MicSource>> {
    None
}

/// `preferred` when the output device supports it, else the device's default rate (e.g. 44.1 kHz Bluetooth
/// headphones on macOS); the firmware resamples to whatever rate it renders at.
fn output_rate(dev: &cpal::Device, preferred: u32) -> u32 {
    let supported = dev
        .supported_output_configs()
        .is_ok_and(|mut c| c.any(|r| (r.min_sample_rate()..=r.max_sample_rate()).contains(&preferred)));
    if supported {
        preferred
    } else {
        dev.default_output_config().map_or(preferred, |c| c.sample_rate())
    }
}

impl AudioEngine {
    pub fn start(opts: AudioOptions) -> Result<Self, String> {
        // cpal panics when opening a stream without the context.
        #[cfg(target_os = "android")]
        if !crate::android::init_context() {
            return Err("no Android context".into());
        }
        let host = cpal::default_host();
        let out_dev = host.default_output_device().ok_or("no output device")?;
        let rate = output_rate(&out_dev, opts.sample_rate);
        let opts = AudioOptions { sample_rate: rate, ..opts };

        let (mut mic_tx, mut mic_rx) = rtrb::RingBuffer::<f32>::new(MAX_FRAMES * 4);
        let mic_chunk = Arc::new(AtomicUsize::new(0));
        let mic_chunk_in = mic_chunk.clone();
        let mic_fed = Arc::new(AtomicBool::new(false));
        let mic_fed_in = mic_fed.clone();
        let mut input = None;
        let mut mic_name = None;
        let mut mic_source = MicSource::Builtin;
        if opts.use_mic {
            if let Some(in_dev) = input_device(&host, opts.mic_source, rate) {
                let in_ch = in_dev.default_input_config().map(|c| c.channels()).unwrap_or(1).clamp(1, 2);
                let built = in_dev.build_input_stream(
                    config(&opts, in_ch),
                    move |data: &[f32], _: &cpal::InputCallbackInfo| {
                        mic_chunk_in.fetch_max(data.len() / in_ch as usize * 2, Ordering::Relaxed);
                        mic_fed_in.store(true, Ordering::Relaxed);
                        for frame in data.chunks(in_ch as usize) {
                            let l = frame[0];
                            let r = *frame.get(1).unwrap_or(&l);
                            if mic_tx.slots() >= 2 {
                                let _ = mic_tx.push(l);
                                let _ = mic_tx.push(r);
                            }
                        }
                    },
                    |e| eprintln!("glo: input stream error: {e}"),
                    None,
                );
                match built.and_then(|s| s.play().map(|_| s)) {
                    Ok(s) => {
                        input = Some(s);
                        let desc = in_dev.description().ok();
                        mic_source = desc.as_ref().and_then(mic_source_of).unwrap_or_default();
                        crate::set_mic_offset_db(if mic_source == MicSource::Usb { USB_MIC_OFFSET_DB } else { 0 });
                        mic_name = desc.map(|d| d.name().to_string());
                    }
                    Err(e) => eprintln!("glo: microphone unavailable: {e}"),
                }
            }
        }
        let mic_active = input.is_some();

        let out_ch = 2u16;
        let out_opts = if false /* DEBUG simulate cheap device */ && bluetooth_output(&host) { AudioOptions { buffer_frames: None, ..opts } } else { opts };
        let mut scratch = vec![0.0f32; MAX_FRAMES * 2];
        let mut mic = vec![0.0f32; MAX_FRAMES * 2];
        let mut mic_last = [0.0f32; 2];
        let mut out_max = 0usize;
        let mut out_run = 0usize;
        let output = out_dev
            .build_output_stream(
                config(&out_opts, out_ch),
                move |data: &mut [f32], _: &cpal::OutputCallbackInfo| {
                    // Keep mic latency bounded: drop what piled up beyond three of the largest bursts of either stream.
                    // An output burst is what it takes between two mic callbacks: one large callback (Bluetooth
                    // alternates e.g. 28 and 932 frames) or several small ones back to back (fixed size, mixer path).
                    if mic_fed.swap(false, Ordering::Relaxed) {
                        out_run = 0;
                    }
                    out_run += data.len();
                    out_max = out_max.max(out_run);
                    let keep = out_max.max(mic_chunk.load(Ordering::Relaxed)) * 3;
                    let excess = mic_rx.slots().saturating_sub(keep) & !1;
                    for _ in 0..excess {
                        let _ = mic_rx.pop();
                    }
                    // Short of a whole callback (output bursts larger than the mic's, e.g. Bluetooth): hold the mic
                    // once so a backlog builds up, instead of a gap at the end of every callback.
                    let starved = mic_rx.slots() < data.len();
                    for chunk in data.chunks_mut(MAX_FRAMES * out_ch as usize) {
                        let frames = chunk.len() / out_ch as usize;
                        let out = &mut scratch[..frames * 2];
                        let input = if mic_active {
                            let m = &mut mic[..frames * 2];
                            // A gap fades out instead of dropping to zero, which would click.
                            for (i, s) in m.iter_mut().enumerate() {
                                let fade = mic_last[i & 1] * 0.995;
                                *s = if starved { fade } else { mic_rx.pop().unwrap_or(fade) };
                                mic_last[i & 1] = *s;
                            }
                            Some(&m[..])
                        } else {
                            None
                        };
                        crate::render(out, input, rate);
                        chunk.copy_from_slice(out);
                    }
                },
                |e| eprintln!("glo: output stream error: {e}"),
                None,
            )
            .map_err(|e| format!("output stream: {e}"))?;
        output.play().map_err(|e| format!("output play: {e}"))?;

        Ok(Self { _output: output, _input: input, sample_rate: rate, mic_active, mic_name, mic_source })
    }
}
