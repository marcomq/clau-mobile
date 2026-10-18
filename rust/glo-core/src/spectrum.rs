//! Display analysis of the output: a log-spaced FFT spectrum and a triggered waveform.

use std::f32::consts::PI;

const FFT_LEN: usize = 2048;
const SCOPE_LEN: usize = 2048;
const F_LO: f32 = 40.0;
const F_HI: f32 = 16000.0;
const DB_FLOOR: f32 = -66.0;
const DB_CEIL: f32 = -6.0;
/// Samples the waveform spans (about 10 ms at 48 kHz).
const WAVE_SPAN: usize = 512;

pub struct Analyzer {
    scope: Vec<f32>,
    window: Vec<f32>,
    re: Vec<f32>,
    im: Vec<f32>,
}

impl Default for Analyzer {
    fn default() -> Self {
        Self {
            scope: vec![0.0; SCOPE_LEN],
            window: (0..FFT_LEN).map(|i| 0.5 - 0.5 * (2.0 * PI * i as f32 / FFT_LEN as f32).cos()).collect(),
            re: vec![0.0; FFT_LEN],
            im: vec![0.0; FFT_LEN],
        }
    }
}

impl Analyzer {
    /// Fills `spectrum` (40 Hz .. 16 kHz, log-spaced) with levels in 0..1 and `wave` with the waveform in -1..1.
    pub fn analyze(&mut self, spectrum: &mut [f32], wave: &mut [f32]) {
        let rate = unsafe { crate::ffi::glo_get_scope(self.scope.as_mut_ptr(), SCOPE_LEN as i32) };
        self.spectrum(rate.max(1) as f32, spectrum);
        triggered_wave(&self.scope, wave);
    }

    fn spectrum(&mut self, rate: f32, out: &mut [f32]) {
        let tail = &self.scope[SCOPE_LEN - FFT_LEN..];
        for (i, (&s, &w)) in tail.iter().zip(&self.window).enumerate() {
            self.re[i] = s * w;
            self.im[i] = 0.0;
        }
        fft(&mut self.re, &mut self.im);
        // Hann window sums to N/2: a full-scale sine peaks at 0 dB.
        let norm = 4.0 / FFT_LEN as f32;
        let bin_hz = rate / FFT_LEN as f32;
        let mag = |b: usize| self.re[b].hypot(self.im[b]);
        let n = out.len();
        for (k, o) in out.iter_mut().enumerate() {
            let b0 = F_LO * (F_HI / F_LO).powf(k as f32 / n as f32) / bin_hz;
            let b1 = F_LO * (F_HI / F_LO).powf((k + 1) as f32 / n as f32) / bin_hz;
            // Bands narrower than a bin interpolate at their centre, else neighbours would share a bin (steps).
            let peak = if b1 - b0 < 1.0 {
                let x = (0.5 * (b0 + b1)).clamp(1.0, (FFT_LEN / 2 - 2) as f32);
                let (i, fr) = (x as usize, x.fract());
                mag(i) * (1.0 - fr) + mag(i + 1) * fr
            } else {
                let lo = (b0.ceil() as usize).clamp(1, FFT_LEN / 2 - 1);
                (lo..=(b1.floor() as usize).clamp(lo, FFT_LEN / 2 - 1)).map(mag).fold(0.0, f32::max)
            } * norm;
            let db = 20.0 * peak.max(1e-9).log10();
            *o = ((db - DB_FLOOR) / (DB_CEIL - DB_FLOOR)).clamp(0.0, 1.0);
        }
    }
}

/// Starts the waveform at a rising zero crossing so periodic sounds stand still.
fn triggered_wave(scope: &[f32], out: &mut [f32]) {
    let latest = scope.len() - WAVE_SPAN;
    let start = (latest.saturating_sub(WAVE_SPAN)..latest)
        .rev()
        .find(|&i| scope[i] <= 0.0 && scope[i + 1] > 0.0)
        .unwrap_or(latest);
    let n = out.len();
    for (j, o) in out.iter_mut().enumerate() {
        *o = scope[start + j * WAVE_SPAN / n].clamp(-1.0, 1.0);
    }
}

/// In-place iterative radix-2 FFT; `re.len()` must be a power of two.
fn fft(re: &mut [f32], im: &mut [f32]) {
    let n = re.len();
    let mut j = 0;
    for i in 1..n {
        let mut bit = n >> 1;
        while j & bit != 0 {
            j ^= bit;
            bit >>= 1;
        }
        j |= bit;
        if i < j {
            re.swap(i, j);
            im.swap(i, j);
        }
    }
    let mut len = 2;
    while len <= n {
        let ang = -2.0 * PI / len as f32;
        for start in (0..n).step_by(len) {
            for k in 0..len / 2 {
                let (s, c) = (ang * k as f32).sin_cos();
                let (a, b) = (start + k, start + k + len / 2);
                let tr = re[b] * c - im[b] * s;
                let ti = re[b] * s + im[b] * c;
                re[b] = re[a] - tr;
                im[b] = im[a] - ti;
                re[a] += tr;
                im[a] += ti;
            }
        }
        len <<= 1;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sine(hz: f32, rate: f32, amp: f32) -> Vec<f32> {
        (0..SCOPE_LEN).map(|i| amp * (2.0 * PI * hz * i as f32 / rate).sin()).collect()
    }

    #[test]
    fn sine_peaks_in_its_band() {
        let mut a = Analyzer { scope: sine(1000.0, 48000.0, 0.5), ..Analyzer::default() };
        let mut out = [0.0; 64];
        a.spectrum(48000.0, &mut out);
        let peak = (0..64).max_by(|&x, &y| out[x].total_cmp(&out[y])).unwrap();
        let band = (64.0 * (1000.0f32 / F_LO).ln() / (F_HI / F_LO).ln()) as usize;
        assert!(peak.abs_diff(band) <= 1, "peak {peak}, expected {band}");
        assert!(out[peak] > 0.85, "level {}", out[peak]);
        assert!(out[5] < 0.2 && out[60] < 0.2);
    }

    #[test]
    fn low_bands_have_no_steps() {
        let mut a = Analyzer { scope: sine(100.0, 48000.0, 0.5), ..Analyzer::default() };
        let mut out = [0.0; 64];
        a.spectrum(48000.0, &mut out);
        assert!(out[6..=9].windows(2).all(|w| w[1] > w[0]), "{:?}", &out[..12]);
    }

    #[test]
    fn wave_starts_at_rising_zero_crossing() {
        let mut out = [0.0; 128];
        triggered_wave(&sine(440.0, 48000.0, 0.5), &mut out);
        assert!(out[0].abs() < 0.05 && out[1] > out[0]);
    }
}
