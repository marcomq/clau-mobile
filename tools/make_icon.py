# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy>=1.26", "pillow>=10"]
# ///
"""Renders the app icon from the same formula as godot/glo.gdshader (one frozen frame).

Writes the plain icon, the Android adaptive-icon layers (foreground with alpha, black background,
monochrome for themed icons) and a macOS icon (rounded tile with margin and shadow, as the Dock expects).

    uv run tools/make_icon.py [--out godot/icons] [--led 0.2,0.55,1.0]
"""

import argparse
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

SUPERSAMPLE = 4
# A calm frame: moderate loudness, spectrum falling towards the highs (band values as glo_get_levels reports them).
LOUD = 0.6
BANDS = np.array([0.030, 0.026, 0.022, 0.018, 0.014, 0.010, 0.007, 0.005])


def band_at(t):
    x = np.clip(t, 0.0, 1.0) * 7.0
    i = np.floor(x).astype(int)
    j = np.minimum(i + 1, 7)
    return BANDS[i] + (BANDS[j] - BANDS[i]) * (x - i)


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def render(size, led, zoom):
    """RGB in 0..1 over black. zoom > 1 shows the ring larger (the shader's screen height = 1 / zoom)."""
    n = size * SUPERSAMPLE
    c = (np.arange(n) + 0.5) / n - 0.5
    x, y = np.meshgrid(c, -c)
    x, y = x / zoom, y / zoom
    r = np.hypot(x, y)
    a = np.arctan2(y, x) / (2 * np.pi) + 0.5

    ring_r = 0.18 + 0.08 * LOUD
    spectrum = band_at(np.abs(a * 2.0 - 1.0)) * 20.0
    wobble = 0.02 * np.sin(a * 2 * np.pi * 6.0) * (0.3 + LOUD)
    d = np.abs(r - ring_r - wobble - 0.05 * spectrum)

    base = np.maximum(led, [0.06, 0.09, 0.14])
    col = base * (0.012 / (d + 0.012))[..., None]
    col += led * 0.35 * smoothstep(0.55, 0.0, r)[..., None]
    col += base * 0.05 * LOUD * (0.5 + 0.5 * np.sin(r * 40.0))[..., None]
    col = np.clip(col, 0.0, 1.0)
    return col.reshape(size, SUPERSAMPLE, size, SUPERSAMPLE, 3).mean(axis=(1, 3))


def to_image(col, alpha=False):
    if not alpha:
        return Image.fromarray((col * 255 + 0.5).astype(np.uint8), "RGB")
    # Un-premultiply so that compositing over black gives back the same pixels.
    a = col.max(axis=2, keepdims=True)
    rgb = np.where(a > 0, col / np.maximum(a, 1e-6), 0.0)
    rgba = np.concatenate([rgb, a], axis=2)
    return Image.fromarray((rgba * 255 + 0.5).astype(np.uint8), "RGBA")


def macos_icon(led):
    """Apple's icon grid: an 824 px squircle tile centred on a 1024 px canvas, with a soft drop shadow."""
    canvas, tile = 1024, 824
    n = tile * SUPERSAMPLE
    c = np.abs((np.arange(n) + 0.5) / n * 2.0 - 1.0)
    x, y = np.meshgrid(c, c)
    mask = (x**5 + y**5 <= 1.0).astype(float)
    mask = mask.reshape(tile, SUPERSAMPLE, tile, SUPERSAMPLE).mean(axis=(1, 3))
    alpha = Image.fromarray((mask * 255 + 0.5).astype(np.uint8), "L")

    off = (canvas - tile) // 2
    shadow_a = Image.new("L", (canvas, canvas), 0)
    shadow_a.paste(alpha.point(lambda v: v * 0.45), (off, off + 10))
    img = Image.new("RGBA", (canvas, canvas), (0, 0, 0, 0))
    img.putalpha(shadow_a.filter(ImageFilter.GaussianBlur(14)))
    face = to_image(render(tile, led, zoom=1.35)).convert("RGBA")
    face.putalpha(alpha)
    img.alpha_composite(face, (off, off))
    return img


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", type=Path, default=Path("godot/icons"))
    ap.add_argument("--led", default="0.2,0.55,1.0", help="LED colour r,g,b in 0..1")
    args = ap.parse_args()
    led = np.array([float(v) for v in args.led.split(",")])
    args.out.mkdir(parents=True, exist_ok=True)

    to_image(render(512, led, zoom=1.35)).save(args.out / "icon.png")
    # Adaptive icons: launchers crop to the inner 66 %, so the ring stays smaller there.
    to_image(render(432, led, zoom=1.0), alpha=True).save(args.out / "icon_foreground.png")
    Image.new("RGB", (432, 432), (0, 0, 0)).save(args.out / "icon_background.png")
    # Themed icons tint the alpha only: keep the ring line, drop the soft glow.
    ring = render(432, np.zeros(3), zoom=1.0).max(axis=2) / 0.14  # led off: only the ring's base colour remains
    mono = np.clip((ring - 0.35) / 0.4, 0.0, 1.0)
    Image.fromarray((np.dstack([np.ones_like(mono), np.ones_like(mono), np.ones_like(mono), mono]) * 255 + 0.5).astype(np.uint8), "RGBA").save(args.out / "icon_monochrome.png")
    macos_icon(led).save(args.out / "icon_macos.icns")
    print(f"wrote {args.out}/icon.png, icon_foreground.png, icon_background.png, icon_monochrome.png, icon_macos.icns")


if __name__ == "__main__":
    main()
