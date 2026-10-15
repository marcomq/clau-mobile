# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy>=1.26"]
# ///
"""Synthesizes a free replacement for the Glo `_all_samples.bin` flash image.

The layout (offsets, lengths, drum split, voice menu timings) is read from the firmware config, so the
result drops in without config changes. Speech uses espeak-ng when installed, otherwise soft chimes.

    uv run tools/make_samples.py [--config firmware/data/config_glo.txt] [--out godot/data/samples.bin]
"""

import argparse
import io
import re
import shutil
import subprocess
import wave
from pathlib import Path

import numpy as np

BED_RATE = 50780  # Whale engine rate; background samples are read one value per engine frame
VOICE_RATE = 48000  # SAMPLE_RATE_VOICE_MENU
VOICE_V1_RATE = 22050
rng = np.random.default_rng(1977)


def section(cfg: str, name: str) -> list[str]:
    m = re.search(r"^\[" + re.escape(name) + r"\]\s*$(.*?)(?=^\[)", cfg, re.M | re.S)
    lines = [l.split("//")[0].strip() for l in m.group(1).splitlines()] if m else []
    return [l for l in lines if l]


def shaped_noise(n: int, lo: float, hi: float, tilt: float, rate: int) -> np.ndarray:
    """Circular (loop-seamless) noise, band-limited to lo..hi Hz with a 1/f^tilt slope."""
    f = np.fft.rfftfreq(n, 1 / rate)
    spec = rng.normal(size=f.size) + 1j * rng.normal(size=f.size)
    gain = np.where((f >= lo) & (f <= hi), 1.0, 0.0) / np.maximum(f, 1.0) ** tilt
    gain = np.convolve(gain, np.ones(64) / 64, mode="same")  # soften the band edges
    return norm(np.fft.irfft(spec * gain, n))


def loop_lfo(n: int, seconds: float, phase: float = 0.0) -> np.ndarray:
    """0..1 raised cosine with a whole number of cycles over the buffer, so it loops cleanly."""
    cycles = max(1, round(n / BED_RATE / seconds))
    return 0.5 - 0.5 * np.cos(2 * np.pi * (cycles * np.arange(n) / n + phase))


def add_wrapped(dst: np.ndarray, at: int, ev: np.ndarray) -> None:
    idx = (at + np.arange(ev.size)) % dst.size
    np.add.at(dst, idx, ev)


def norm(x: np.ndarray, rms: float = 1.0) -> np.ndarray:
    return x * (rms / max(np.sqrt(np.mean(x**2)), 1e-12))


def sea(n: int) -> np.ndarray:
    body = shaped_noise(n, 30, 1800, 0.6, BED_RATE)
    foam = shaped_noise(n, 1500, 9000, 0.3, BED_RATE)
    swell = 0.5 * loop_lfo(n, 9.0) + 0.5 * loop_lfo(n, 6.5, 0.3)
    return body * (0.25 + 0.75 * swell**1.5) + 0.6 * foam * swell**4


def forest(n: int) -> np.ndarray:
    out = 0.35 * shaped_noise(n, 150, 2500, 0.8, BED_RATE) * (0.5 + 0.5 * loop_lfo(n, 11.0))
    t_bird = 0
    while t_bird < n:
        f0, f1 = rng.uniform(2200, 4500), rng.uniform(1800, 6000)
        for _ in range(rng.integers(2, 7)):
            dur = rng.uniform(0.04, 0.12)
            t = np.arange(int(dur * BED_RATE)) / BED_RATE
            freq = f0 + (f1 - f0) * t / dur
            chirp = np.sin(2 * np.pi * np.cumsum(freq) / BED_RATE) * np.sin(np.pi * t / dur) ** 2
            add_wrapped(out, t_bird, rng.uniform(1.0, 3.0) * chirp)
            t_bird += int((dur + rng.uniform(0.03, 0.09)) * BED_RATE)
        t_bird += int(rng.uniform(0.4, 2.5) * BED_RATE)
    return out


def water(n: int) -> np.ndarray:
    out = 0.6 * shaped_noise(n, 250, 5000, 0.4, BED_RATE)
    for _ in range(int(70 * n / BED_RATE)):
        f0, tau = rng.uniform(350, 2600), rng.uniform(0.004, 0.02)
        t = np.arange(int(tau * 6 * BED_RATE)) / BED_RATE
        freq = f0 * (1 + 4 * t / (tau * 6))  # bubbles rise in pitch as they resonate
        bubble = np.sin(2 * np.pi * np.cumsum(freq) / BED_RATE) * np.exp(-t / tau)
        add_wrapped(out, int(rng.integers(0, n)), rng.uniform(0.5, 3.0) * bubble)
    return out


def drums(lengths: list[int]) -> np.ndarray:
    def env(n, tau):
        return np.exp(-np.arange(n) / (tau * BED_RATE))

    def hp(x):
        return np.diff(x, prepend=0.0)

    n1, n2, n3, n4 = (l // 2 for l in lengths)
    t3 = np.arange(n3) / BED_RATE
    snare = 0.7 * hp(rng.normal(size=n1)) * env(n1, 0.06) + np.sin(2 * np.pi * 190 * np.arange(n1) / BED_RATE) * env(n1, 0.04)
    hat = hp(hp(rng.normal(size=n2))) * env(n2, 0.025)
    kick = np.sin(2 * np.pi * np.cumsum(50 + 110 * np.exp(-t3 / 0.035)) / BED_RATE) * env(n3, 0.12)
    cymbal = hp(hp(rng.normal(size=n4))) * env(n4, 0.45)
    parts = [snare, hat, kick, cymbal]
    return np.concatenate([0.85 * p / np.max(np.abs(p)) for p in parts])


def speak(text: str, rate: int) -> np.ndarray | None:
    exe = shutil.which("espeak-ng") or shutil.which("espeak")
    if not exe:
        return None
    wav = subprocess.run([exe, "-v", "en-us", "-s", "150", "--stdout", text], capture_output=True, check=True).stdout
    with wave.open(io.BytesIO(wav)) as w:
        x = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(float)
        src = w.getframerate()
    return np.interp(np.arange(int(x.size * rate / src)) * src / rate, np.arange(x.size), x)


def chime(n: int, rate: int, k: int) -> np.ndarray:
    t = np.arange(min(n, int(0.9 * rate))) / rate
    notes = [523.25, 659.25, 783.99, 987.77]
    x = sum(np.sin(2 * np.pi * notes[(k + i) % 4] * t) * np.exp(-t / 0.25) * (t >= 0.12 * i) for i in range(2))
    return x


def voice_track(items: list[tuple[str, float]], total: float, n: int, rate: int) -> np.ndarray:
    out = np.zeros(n)
    bounds = [p for _, p in items] + [total]
    for k, (name, pos) in enumerate(items):
        text = name.replace("_", " ").replace("hz", " hertz").replace("bpm", "B P M")
        start, end = int(pos * rate), min(int(bounds[k + 1] * rate), n)
        clip = speak(text, rate)
        clip = chime(end - start, rate, k) if clip is None else clip
        clip = norm(clip[: end - start] * np.minimum(1.0, np.arange(clip[: end - start].size)[::-1] / 200))
        out[start : start + clip.size] = clip
    return out


def voice_items(cfg: str, name: str) -> tuple[list[tuple[str, float]], float]:
    items, total = [], 0.0
    for line in section(cfg, name):
        key, val = line.split()[:2]
        if key == "(total_length)":
            total = float(val)
        elif not key.startswith("("):
            items.append((key, float(val)))
    return items, total


def main() -> None:
    root = Path(__file__).resolve().parent.parent
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--config", type=Path, default=root / "firmware/data/config_glo.txt")
    ap.add_argument("--out", type=Path, default=root / "godot/data/samples.bin")
    a = ap.parse_args()
    cfg = a.config.read_text(errors="replace")

    smap = {int(k): tuple(map(int, v.split(","))) for k, v in (l.split(":") for l in section(cfg, "samples_map"))}
    gs = dict(re.findall(r"^(DRUM_LENGTH\d)\s+(\d+)", cfg, re.M))
    drum_lengths = [int(gs.get(f"DRUM_LENGTH{i}", d)) for i, d in enumerate((26880, 29060, 33036, 141116), 1)]
    menu, menu_total = voice_items(cfg, "voice_menu")
    menu_v1, menu_v1_total = voice_items(cfg, "voice_menu_v1")

    # sample index -> (generator(n), target RMS); indices 2, 7, 8 are combinations of their neighbours
    gens = {
        1: (sea, 2600),
        3: (sea, 4400),
        4: (forest, 4200),
        5: (lambda n: voice_track(menu, menu_total, n, VOICE_RATE), 4000),
        6: (water, 5000),
        9: (lambda n: drums(drum_lengths), None),
        10: (lambda n: voice_track(menu_v1, menu_v1_total, n, VOICE_V1_RATE), 3000),
    }
    size = max(off + ln for off, ln in smap.values())
    image = np.zeros(size // 2, dtype=np.int16)
    for idx, (gen, rms) in gens.items():
        off, ln = smap[idx]
        x = gen(ln // 2)[: ln // 2]
        x = norm(x, rms) if rms else x * 32767
        image[off // 2 : off // 2 + x.size] = np.clip(x, -32767, 32767).astype(np.int16)
        print(f"sample {idx:2d}: {ln:8d} bytes at {off}")
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_bytes(image.astype("<i2").tobytes())
    print(f"wrote {a.out} ({size} bytes){'' if shutil.which('espeak-ng') or shutil.which('espeak') else ' - no espeak-ng: voice menu uses chimes'}")


if __name__ == "__main__":
    main()
