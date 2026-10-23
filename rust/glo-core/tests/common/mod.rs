//! Headless harness: boots the firmware once per test binary and renders it in real time on a pump thread.
#![allow(dead_code)]

use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::thread::{self, sleep};
use std::time::{Duration, Instant};

pub const RATE: u32 = 48_000;
const BLOCK: usize = 256;
const MIC_HZ: f32 = 330.0;
/// v1.0.118 Granular starts frozen: the first U1 starts sampling, the second switches channel.
const GRANULAR_ID: i32 = 5;

/// Output measured over a time window.
#[derive(Clone, Copy, Debug, Default)]
pub struct Window {
    pub rms: f32,
    pub peak: f32,
    pub non_finite: bool,
    pub blocks: u32,
    /// Engine frames the firmware failed to deliver in time.
    pub underruns: u32,
}

#[derive(Default)]
struct Acc {
    sum_sq: f64,
    samples: u64,
    peak: f32,
    non_finite: bool,
    blocks: u32,
}

pub struct Glo {
    acc: Arc<Mutex<Acc>>,
    stop: Arc<AtomicBool>,
    mic: Arc<AtomicBool>,
    pump: Option<thread::JoinHandle<()>>,
}

impl Glo {
    /// Boots with a fresh data dir named after the test binary and optional samples from `GLO_TEST_SAMPLES`.
    pub fn boot(name: &str) -> Glo {
        let dir = std::env::temp_dir().join(format!("flono-test-{name}-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        let samples = std::env::var_os("GLO_TEST_SAMPLES").map(|p| std::fs::read(PathBuf::from(p)).expect("GLO_TEST_SAMPLES"));
        glo_core::start(&dir, samples.as_deref()).expect("boot");

        let acc = Arc::new(Mutex::new(Acc::default()));
        let stop = Arc::new(AtomicBool::new(false));
        let mic = Arc::new(AtomicBool::new(false));
        let pump = {
            let (acc, stop, mic) = (acc.clone(), stop.clone(), mic.clone());
            thread::spawn(move || {
                let mut buf = vec![0.0f32; BLOCK * 2];
                let mut input = vec![0.0f32; BLOCK * 2];
                let mut phase = 0.0f32;
                let block_time = Duration::from_secs_f64(BLOCK as f64 / RATE as f64);
                let t0 = Instant::now();
                let mut i = 0u32;
                while !stop.load(Ordering::Relaxed) {
                    if let Some(wait) = (t0 + block_time * i).checked_duration_since(Instant::now()) {
                        sleep(wait);
                    }
                    i += 1;
                    let mic_on = mic.load(Ordering::Relaxed);
                    if mic_on {
                        for f in input.chunks_mut(2) {
                            phase = (phase + MIC_HZ / RATE as f32).fract();
                            f.fill(0.2 * (phase * std::f32::consts::TAU).sin());
                        }
                    }
                    glo_core::render(&mut buf, mic_on.then_some(input.as_slice()), RATE);
                    let mut a = acc.lock().unwrap();
                    for &s in &buf {
                        if !s.is_finite() {
                            a.non_finite = true;
                            continue;
                        }
                        a.sum_sq += (s as f64) * (s as f64);
                        a.peak = a.peak.max(s.abs());
                    }
                    a.samples += buf.len() as u64;
                    a.blocks += 1;
                }
            })
        };
        let glo = Glo { acc, stop, mic, pump: Some(pump) };
        glo.wait_until(Duration::from_secs(10), "firmware powered on", || glo_core::is_powered() && glo_core::engine_rate() > 0);
        sleep(Duration::from_secs(1));
        glo
    }

    /// Feeds a sine as microphone input.
    pub fn mic(&self, on: bool) {
        self.mic.store(on, Ordering::Relaxed);
    }

    /// Output over the next `d`.
    pub fn listen(&self, d: Duration) -> Window {
        let underruns = glo_core::underruns();
        *self.acc.lock().unwrap() = Acc::default();
        sleep(d);
        let a = self.acc.lock().unwrap();
        Window {
            underruns: glo_core::underruns().wrapping_sub(underruns),
            rms: if a.samples > 0 { (a.sum_sq / a.samples as f64).sqrt() as f32 } else { 0.0 },
            peak: a.peak,
            non_finite: a.non_finite,
            blocks: a.blocks,
        }
    }

    pub fn wait_until(&self, timeout: Duration, what: &str, mut cond: impl FnMut() -> bool) {
        let t0 = Instant::now();
        while !cond() {
            assert!(t0.elapsed() < timeout, "timed out waiting for {what}");
            sleep(Duration::from_millis(20));
        }
    }

    pub fn hold(&self, idx: i32, d: Duration) {
        glo_core::set_button(idx, true);
        sleep(d);
        glo_core::set_button(idx, false);
    }

    /// Short presses, `a` = U1, `b` = U2 (+), `c` = U3 (−), as the firmware records them.
    pub fn tap(&self, seq: &str) {
        for c in seq.chars() {
            let idx = match c {
                'a' => glo_core::BUTTON_PWR,
                'b' => glo_core::BUTTON_U2,
                'c' => glo_core::BUTTON_U3,
                _ => panic!("unknown key {c}"),
            };
            // A short press needs >5 ticks of the 10 ms button scan (hw/ui.c). Under host load a 100 ms
            // hold got fewer and the tap was lost ("timed out waiting for channel change", mostly at Fire).
            // Hold + gap must stay below BUTTONS_SEQUENCE_TIMEOUT_SHORT (500 ms) for sequences.
            self.hold(idx, Duration::from_millis(200));
            sleep(Duration::from_millis(150));
        }
    }

    /// Waits until the typed sequence is decoded (the firmware clears it after its timeout).
    pub fn wait_decoded(&self) {
        self.wait_until(Duration::from_secs(5), "sequence decoded", || glo_core::button_sequence().is_empty());
    }

    /// Output volume once the fade-in after a channel start has finished.
    pub fn settled_volume(&self) -> Option<f32> {
        let mut last = glo_core::volume_db();
        let mut stable = 0;
        self.wait_until(Duration::from_secs(10), "volume settled", || {
            let v = glo_core::volume_db();
            stable = if v == last { stable + 1 } else { 0 };
            last = v;
            stable >= 30
        });
        last
    }

    /// Taps U1 and waits for the next channel to start and the tap to leave the sequence buffer.
    pub fn next_channel(&self) -> i32 {
        let before = glo_core::channel();
        self.tap("a");
        if glo_core::channel_id() == GRANULAR_ID {
            self.wait_decoded();
            sleep(Duration::from_millis(500));
            if glo_core::channel() == before {
                self.tap("a");
            }
        }
        self.wait_until(Duration::from_secs(5), "channel change", || glo_core::channel() != before);
        self.wait_decoded();
        sleep(Duration::from_millis(300));
        glo_core::channel()
    }
}

impl Drop for Glo {
    fn drop(&mut self) {
        self.stop.store(true, Ordering::Relaxed);
        if let Some(p) = self.pump.take() {
            let _ = p.join();
        }
    }
}
