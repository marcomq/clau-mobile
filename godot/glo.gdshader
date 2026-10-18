shader_type canvas_item;

// Whale RGB LED (smoothed), overall loudness, FFT spectrum (40 Hz .. 16 kHz, 0..1) and waveform (-1..1).
uniform vec4 led : source_color = vec4(0.0);
uniform float rms = 0.0;
uniform float spectrum[64];
uniform float wave[128];

float wave_at(float u) {
	float x = clamp(u, 0.0, 1.0) * 127.0;
	int i = int(floor(x));
	return mix(wave[i], wave[min(i + 1, 127)], fract(x));
}

// Rotates the hue around the grey axis, so colours stay anchored to the LED's.
vec3 hue(vec3 c, float a) {
	const vec3 k = vec3(0.57735);
	return max(c * cos(a) + cross(k, c) * sin(a) + k * dot(k, c) * (1.0 - cos(a)), 0.0);
}

float wave_line(vec2 p, float R, float flip, float w) {
	float u = p.x / (2.0 * R) + 0.5;
	float amp = 0.75 * R * flip;
	float y = -wave_at(u) * amp;
	float slope = (wave_at(u + 0.5 / 127.0) - wave_at(u - 0.5 / 127.0)) * 127.0 * amp / (2.0 * R);
	return w / (abs(p.y - y) / sqrt(1.0 + slope * slope) + w);
}

void fragment() {
	vec2 res = 1.0 / SCREEN_PIXEL_SIZE;
	float px = 1.0 / res.y;
	vec2 p = (UV - vec2(0.5, 0.45)) * vec2(res.x / res.y, 1.0);
	float r = length(p);
	// 0 at the bottom (lows) .. 1 at the top (highs), mirrored left/right.
	float t = abs(atan(p.x, p.y)) / PI;

	float loud = clamp(rms * 9.0, 0.0, 2.0);
	float ring_r = 0.15 + 0.04 * loud;
	vec3 base = max(led.rgb, vec3(0.06, 0.09, 0.14));
	float drift = sin(TIME * 0.12) * 0.25;
	float outside = smoothstep(ring_r, ring_r + 0.12, r);

	// Kaleidoscope: eight mirrored, slowly swirling segments behind everything.
	float seg = TAU / 8.0;
	float a = atan(p.y, p.x) + r * (2.0 + 0.8 * loud) * sin(TIME * 0.08) + TIME * 0.025;
	float m = abs(mod(a, seg) - 0.5 * seg);
	vec2 q = r * vec2(cos(m), sin(m));
	float kal = sin(q.x * 26.0 - TIME * 0.45 + 2.0 * sin(q.y * 31.0 + TIME * 0.25))
		* sin(r * 18.0 - TIME * (0.5 + 0.3 * loud));
	vec3 col = hue(base, drift + 0.3 * kal) * (0.05 + 0.1 * loud) * (0.65 + 0.35 * kal) * outside;

	vec3 ring_c = hue(base, drift + 0.2 * sin(t * PI * 3.0 + TIME * 0.3));
	float w = 0.006 * (1.0 + 0.6 * loud);
	col += 0.8 * ring_c * (w / (abs(r - ring_r) + w));

	float x = t * 64.0;
	float s = spectrum[min(int(x), 63)];
	float len = 0.012 + 0.2 * s;
	float along = r - ring_r - 0.014;
	float across = abs(fract(x) - 0.5) * PI * r / 64.0;
	float half_w = 0.3 * PI * r / 64.0;
	float bar = (1.0 - smoothstep(half_w - px, half_w + px, across))
		* smoothstep(-px, px, along) * (1.0 - smoothstep(len - px, len + px, along));
	float tip = clamp(along / max(len, 1e-3), 0.0, 1.0);
	vec3 bar_c = hue(base, drift + 0.6 * (t - 0.5) + 0.4 * tip * s);
	col += mix(bar_c, vec3(1.0), 0.1 + 0.25 * tip * s) * bar * (0.35 + 1.1 * s);
	col += bar_c * 0.3 * s * exp(-abs(along - 0.5 * len) * 16.0) * step(0.0, along);

	float R = ring_r - 0.025;
	float inside = smoothstep(R, R - 0.03, r) * (0.5 + 0.4 * loud);
	float ww = 0.003 * (1.0 + 0.5 * loud);
	col += mix(hue(base, drift + 0.3), vec3(1.0), 0.3) * wave_line(p, R, 1.0, ww) * inside;
	col += hue(base, drift - 0.3) * 0.6 * wave_line(p, R, -1.0, ww) * inside;

	col += led.rgb * (0.18 + 0.22 * loud) * smoothstep(0.55, 0.0, r);
	float spiral = sin(r * 40.0 - TIME * 1.2 + 6.0 * atan(p.y, p.x));
	col += base * 0.06 * loud * (0.5 + 0.5 * mix(sin(r * 40.0 - TIME * 1.2), spiral, outside));

	COLOR = vec4(col, 1.0);
}
