//! Duplex audio: cpal output drives `glo_render`; the mic (if permitted) feeds it through a ring.

use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use cpal::{BufferSize, Stream, StreamConfig};

const MAX_FRAMES: usize = 4096;

#[derive(Clone, Copy, Debug)]
pub struct AudioOptions {
    pub sample_rate: u32,
    /// Device callback size in frames; `None` lets the platform choose (AAudio: one burst).
    pub buffer_frames: Option<u32>,
    pub use_mic: bool,
}

impl Default for AudioOptions {
    fn default() -> Self {
        Self { sample_rate: 48_000, buffer_frames: None, use_mic: true }
    }
}

pub struct AudioEngine {
    _output: Stream,
    _input: Option<Stream>,
    pub sample_rate: u32,
    pub mic_active: bool,
}

fn config(opts: &AudioOptions, channels: u16) -> StreamConfig {
    StreamConfig {
        channels,
        sample_rate: opts.sample_rate,
        buffer_size: opts.buffer_frames.map_or(BufferSize::Default, BufferSize::Fixed),
    }
}

impl AudioEngine {
    pub fn start(opts: AudioOptions) -> Result<Self, String> {
        let host = cpal::default_host();
        let out_dev = host.default_output_device().ok_or("no output device")?;
        let rate = opts.sample_rate;

        let (mut mic_tx, mut mic_rx) = rtrb::RingBuffer::<f32>::new(MAX_FRAMES * 4);
        let mut input = None;
        if opts.use_mic {
            if let Some(in_dev) = host.default_input_device() {
                let in_ch = in_dev.default_input_config().map(|c| c.channels()).unwrap_or(1).clamp(1, 2);
                let built = in_dev.build_input_stream(
                    config(&opts, in_ch),
                    move |data: &[f32], _: &cpal::InputCallbackInfo| {
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
                    Ok(s) => input = Some(s),
                    Err(e) => eprintln!("glo: microphone unavailable: {e}"),
                }
            }
        }
        let mic_active = input.is_some();

        let out_ch = 2u16;
        let mut scratch = vec![0.0f32; MAX_FRAMES * 2];
        let mut mic = vec![0.0f32; MAX_FRAMES * 2];
        let output = out_dev
            .build_output_stream(
                config(&opts, out_ch),
                move |data: &mut [f32], _: &cpal::OutputCallbackInfo| {
                    // Keep mic latency bounded: drop what piled up beyond two callbacks.
                    let excess = mic_rx.slots().saturating_sub(data.len() * 2) & !1;
                    for _ in 0..excess {
                        let _ = mic_rx.pop();
                    }
                    for chunk in data.chunks_mut(MAX_FRAMES * out_ch as usize) {
                        let frames = chunk.len() / out_ch as usize;
                        let out = &mut scratch[..frames * 2];
                        let input = if mic_active {
                            let m = &mut mic[..frames * 2];
                            for s in m.iter_mut() {
                                *s = mic_rx.pop().unwrap_or(0.0);
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

        Ok(Self { _output: output, _input: input, sample_rate: rate, mic_active })
    }
}
