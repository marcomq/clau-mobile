//! Desktop harness for glo-core.
//!
//! Offline:  glo-desktop --wav out.wav [--seconds 20] [--channel N] [--data DIR] [--samples FILE]
//! Realtime: glo-desktop [--data DIR] [--samples FILE] [--buffer FRAMES] [--ring FRAMES] [--no-mic]
//!           stdin commands: 1 / 2 / 3 (short press), 1L / 2L / 3L (long press), t X Y Z (tilt in g), q

use std::io::BufRead;
use std::path::PathBuf;
use std::thread::sleep;
use std::time::{Duration, Instant};

use glo_core::audio::{AudioEngine, AudioOptions};

struct Args {
    wav: Option<PathBuf>,
    seconds: f32,
    channel: u32,
    data: PathBuf,
    samples: Option<PathBuf>,
    buffer: Option<u32>,
    ring: Option<u32>,
    mic: bool,
}

fn parse_args() -> Result<Args, String> {
    let mut a = Args {
        wav: None,
        seconds: 20.0,
        channel: 0,
        data: PathBuf::from("glo-data"),
        samples: None,
        buffer: None,
        ring: None,
        mic: true,
    };
    let mut it = std::env::args().skip(1);
    while let Some(flag) = it.next() {
        let mut val = || it.next().ok_or(format!("{flag} needs a value"));
        match flag.as_str() {
            "--wav" => a.wav = Some(val()?.into()),
            "--seconds" => a.seconds = val()?.parse().map_err(|e| format!("--seconds: {e}"))?,
            "--channel" => a.channel = val()?.parse().map_err(|e| format!("--channel: {e}"))?,
            "--data" => a.data = val()?.into(),
            "--samples" => a.samples = Some(val()?.into()),
            "--buffer" => a.buffer = Some(val()?.parse().map_err(|e| format!("--buffer: {e}"))?),
            "--ring" => a.ring = Some(val()?.parse().map_err(|e| format!("--ring: {e}"))?),
            "--no-mic" => a.mic = false,
            other => return Err(format!("unknown argument {other}")),
        }
    }
    Ok(a)
}

fn press(idx: i32, hold: Duration) {
    glo_core::set_button(idx, true);
    sleep(hold);
    glo_core::set_button(idx, false);
}

fn status() -> String {
    let [r, g, b] = glo_core::led();
    let l = glo_core::levels();
    format!(
        "channel {} | engine {} Hz | LED {}{}{} | rms {:.3} {:.3}{}",
        glo_core::channel(),
        glo_core::engine_rate(),
        if r { 'R' } else { '-' },
        if g { 'G' } else { '-' },
        if b { 'B' } else { '-' },
        l.rms[0],
        l.rms[1],
        if glo_core::is_powered() { "" } else { " | OFF" }
    )
}

/// Renders in (roughly) real time: the firmware produces audio at its own pace on its own threads.
fn offline(a: &Args, wav: &PathBuf) -> Result<(), String> {
    const RATE: u32 = 48_000;
    const BLOCK: usize = 256;
    let spec = hound::WavSpec { channels: 2, sample_rate: RATE, bits_per_sample: 16, sample_format: hound::SampleFormat::Int };
    let mut w = hound::WavWriter::create(wav, spec).map_err(|e| e.to_string())?;
    let mut buf = vec![0.0f32; BLOCK * 2];
    let blocks = (a.seconds * RATE as f32) as usize / BLOCK;
    let block_time = Duration::from_secs_f64(BLOCK as f64 / RATE as f64);

    // Let the firmware boot and the PWR-release wait pass before switching channels.
    sleep(Duration::from_millis(500));
    for _ in 0..a.channel {
        press(glo_core::BUTTON_PWR, Duration::from_millis(100));
        sleep(Duration::from_millis(400));
    }

    let t0 = Instant::now();
    for i in 0..blocks {
        if let Some(wait) = (t0 + block_time * i as u32).checked_duration_since(Instant::now()) {
            sleep(wait);
        }
        glo_core::render(&mut buf, None, RATE);
        for s in &buf {
            w.write_sample((s.clamp(-1.0, 1.0) * 32767.0) as i16).map_err(|e| e.to_string())?;
        }
        if i % (RATE as usize / BLOCK) == 0 {
            eprintln!("[{:>3}s] {}", i * BLOCK / RATE as usize, status());
        }
    }
    w.finalize().map_err(|e| e.to_string())
}

fn realtime(a: &Args) -> Result<(), String> {
    let engine = AudioEngine::start(AudioOptions { buffer_frames: a.buffer, use_mic: a.mic, ..Default::default() })?;
    eprintln!("audio: {} Hz, mic {}", engine.sample_rate, if engine.mic_active { "on" } else { "off" });
    eprintln!("commands: 1/2/3 press, 1L/2L/3L long press, t X Y Z tilt, s status, q quit");
    for line in std::io::stdin().lock().lines() {
        let line = line.map_err(|e| e.to_string())?;
        let words: Vec<&str> = line.split_whitespace().collect();
        match words.as_slice() {
            ["q"] => break,
            ["s"] | [] => {}
            ["t", x, y, z] => match (x.parse(), y.parse(), z.parse()) {
                (Ok(x), Ok(y), Ok(z)) => glo_core::set_accel(x, y, z),
                _ => eprintln!("t X Y Z expects numbers"),
            },
            [b] => {
                let long = b.ends_with(['L', 'l']);
                match b.trim_end_matches(['L', 'l']).parse::<i32>() {
                    Ok(idx @ 1..=3) => press(idx, Duration::from_millis(if long { 2500 } else { 120 })),
                    _ => eprintln!("unknown command {b}"),
                }
            }
            _ => eprintln!("unknown command {line}"),
        }
        sleep(Duration::from_millis(50));
        eprintln!("{}", status());
    }
    Ok(())
}

fn main() {
    let result = parse_args().and_then(|a| {
        let samples = match &a.samples {
            Some(p) => Some(std::fs::read(p).map_err(|e| format!("{}: {e}", p.display()))?),
            None => None,
        };
        glo_core::start(&a.data, samples.as_deref())?;
        if let Some(r) = a.ring {
            glo_core::set_ring_frames(r);
        }
        match &a.wav {
            Some(wav) => offline(&a, wav),
            None => realtime(&a),
        }
    });
    if let Err(e) = result {
        eprintln!("error: {e}");
        std::process::exit(1);
    }
}
