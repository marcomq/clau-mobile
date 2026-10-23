shader_type canvas_item;

// Whale RGB LED (smoothed), overall loudness, FFT spectrum (40 Hz .. 16 kHz, 0..1) and waveform (-1..1).
uniform vec4 led : source_color = vec4(0.0);
uniform float rms = 0.0;
uniform float spectrum[64];
uniform float wave[128];

const float PETALS = 26.0;

float wave_at(float u) {
	float x = clamp(u, 0.0, 1.0) * 127.0;
	int i = int(floor(x));
	return mix(wave[i], wave[min(i + 1, 127)], fract(x));
}

float wave_soft(float u) {
	float d = 1.5 / 127.0;
	return (wave_at(u - d) + 2.0 * wave_at(u) + wave_at(u + d)) * 0.25;
}

float spec_at(float x) {
	x = clamp(x, 0.0, 63.0);
	int i = int(floor(x));
	return mix(spectrum[i], spectrum[min(i + 1, 63)], fract(x));
}

float spec_band(float t) {
	float x = t * 63.0;
	return (spec_at(x - 1.0) + 2.0 * spec_at(x) + spec_at(x + 1.0)) * 0.25;
}

// Rotates the hue around the grey axis, so colours stay anchored to the LED's.
vec3 hue(vec3 c, float a) {
	const vec3 k = vec3(0.57735);
	return max(c * cos(a) + cross(k, c) * sin(a) + k * dot(k, c) * (1.0 - cos(a)), 0.0);
}

float hash(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

float noise(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
		mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

// Light caustics on a water surface (after Dave Hoskins' tileable water caustic).
float caustic(vec2 uv, float time) {
	vec2 p = uv * TAU - 250.0;
	vec2 i = p;
	float c = 1.0;
	for (int n = 0; n < 4; n++) {
		float tt = time * (1.0 - 3.5 / float(n + 1));
		i = p + vec2(cos(tt - i.x) + sin(tt + i.y), sin(tt - i.y) + cos(tt + i.x));
		c += 1.0 / length(vec2(p.x / (sin(i.x + tt) * 200.0), p.y / (cos(i.y + tt) * 200.0)));
	}
	c = 1.17 - pow(c / 4.0, 1.4);
	return pow(abs(c), 8.0);
}

// How far the loudspeaker cone bulges at angle a: follows the waveform, like the old membrane lines.
float cone_bulge(float a, float amp) {
	float m = 0.5 - 0.5 * cos(a);
	float idle = 0.5 * cos(3.0 * a + TIME * 0.23) + 0.35 * cos(5.0 * a - TIME * 0.31)
		+ 0.8 * (noise(vec2(m * 6.0, TIME * 0.25)) - 0.5);
	float wob = 0.25 * (noise(vec2(m * 4.5, TIME * 0.3)) - 0.5);
	return amp * (wave_soft(0.05 + 0.4 * m) + wob) + 0.02 * idle;
}

// Cone height over x = r / R: dust cap, ribbed cone, rolled surround; it pumps with the bass.
float cone_h(float x, float bulge, float ribs, float pump) {
	bulge *= smoothstep(0.26, 0.45, x) * smoothstep(0.88, 0.68, x);
	float h;
	if (x < 0.28) {
		h = 0.1 + 0.15 * sqrt(1.0 - x * x / (0.28 * 0.28));
	} else if (x < 0.88) {
		float c = (x - 0.28) / 0.6;
		h = -0.25 * (1.0 - c) + ribs * sin((x - 0.6 * bulge) * TAU * 3.5) * sin(PI * c);
	} else {
		h = 0.09 * sin(PI * clamp((x - 0.88) / 0.12, 0.0, 1.0));
	}
	return h + bulge + pump * 0.12 * smoothstep(1.0, 0.88, x);
}

// One whorl of petals: x = mask, y = position along the petal (0 root .. 1 tip), z = rim, w = vein.
vec4 petal_layer(float psi, float r, float rr, float phase, float reach) {
	float step_a = TAU / PETALS;
	float bend = 0.5 * (r - rr) * sin(TIME * 0.3 + phase * 7.0 + r * 6.0);
	float a = psi + phase + bend;
	float idx = round(a / step_a);
	float centre = idx * step_a - phase;
	float s = spec_band(clamp(abs(centre) / PI, 0.0, 1.0));
	float lobe = pow(abs(cos(a * PETALS * 0.5)), 1.3);
	float len = (0.025 + 0.24 * s) * reach * (0.85 + 0.3 * noise(vec2(idx * 3.1, TIME * 0.15)));
	float edge = rr + 0.006 + len * lobe;
	float along = r - rr;
	float mask = smoothstep(edge + 0.004, edge - 0.004, r) * smoothstep(-0.006, 0.004, along);
	float tip = clamp(along / max(edge - rr, 1e-3), 0.0, 1.0);
	float x = fract(a / step_a + 0.5) - 0.5;
	float vein = exp(-x * x * 500.0) * tip * (1.0 - tip) * 4.0;
	float rim = exp(-abs(r - edge) * 160.0) * smoothstep(0.0, 0.01, along);
	return vec4(mask, tip, rim, vein);
}

void fragment() {
	vec2 res = 1.0 / SCREEN_PIXEL_SIZE;
	vec2 p = (UV - vec2(0.5, 0.45)) * vec2(res.x / res.y, 1.0);
	float r = length(p);
	// 0 at the bottom (lows) .. 1 at the top (highs), mirrored left/right.
	float psi = atan(p.x, p.y);
	float t = abs(psi) / PI;

	float loud = clamp(rms * 9.0, 0.0, 2.0);
	float ring_r = 0.12 + 0.03 * loud;
	vec3 base = max(led.rgb, vec3(0.06, 0.09, 0.14));
	float drift = sin(TIME * 0.12) * 0.25;
	vec2 dir = p / max(r, 1e-4);
	float rr = ring_r + 0.004 * sin(TIME * 0.4);

	// Water: drifting caustics, pushed outwards by slow ripples from the flower.
	float ripple = sin(r * 38.0 - TIME * 1.1);
	float ripple_amt = (0.3 + 0.7 * loud) * smoothstep(rr, rr + 0.12, r) * exp(-r * 2.5);
	vec2 wuv = p * 1.3 + dir * 0.012 * ripple * ripple_amt;
	float caus = r < rr - 0.004 ? 0.0 : caustic(wuv, TIME * 0.12 + 23.0);
	vec3 water_c = hue(base, drift + 0.25 * sin(p.x * 3.0 + p.y * 2.0 + TIME * 0.1));
	vec3 col = water_c * (0.045 + 0.2 * caus + 0.03 * loud);
	col += water_c * 0.05 * pow(0.5 + 0.5 * ripple, 3.0) * ripple_amt;

	// Flower: a back whorl peeking between the front petals, both led by the spectrum.
	float step_a = TAU / PETALS;
	vec4 back = petal_layer(psi, r, rr, 0.5 * step_a, 1.2);
	vec4 front = petal_layer(psi, r, rr, 0.0, 1.0);
	col += hue(base, drift) * 0.12 * exp(-max(r - rr, 0.0) * 14.0) * (0.4 + 0.6 * loud);

	vec3 back_c = hue(base, drift + 0.6 * (t - 0.5) - 0.15) * (0.25 + 0.35 * back.y);
	col = mix(col, back_c, back.x * 0.9);
	col += back_c * back.z * 0.6;

	vec3 front_c = hue(base, drift + 0.6 * (t - 0.5) + 0.25 * front.y);
	front_c *= 0.35 + 0.55 * front.y + 0.2 * front.w;
	col = mix(col, mix(front_c, vec3(1.0), 0.08 * front.y), front.x * 0.92);
	col += mix(front_c, vec3(1.0), 0.3) * front.z * 0.7;

	// Heart: a loudspeaker, lit from the top left; the shading alone shows its shape and how it moves.
	// Slopes come from finite differences: along the radius on the profile, around it on the bulge.
	if (r < rr + 0.004) {
		float heart = smoothstep(rr + 0.003, rr - 0.003, r);
		float bass = (spectrum[2] + spectrum[4] + spectrum[6] + spectrum[8]) * 0.25;
		float R = rr - 0.004;
		float x = r / R;
		float amp = 0.06 + 0.1 * loud;
		float ribs = 0.003 + 0.02 * min(loud, 1.0);
		float pump = clamp(1.4 * bass + 0.15 * loud, 0.0, 1.5);
		float a = mod(psi + TIME * 0.1 + PI, TAU) - PI;
		float da = 0.02;
		float b0 = cone_bulge(a, amp);
		float h0 = cone_h(x, b0, ribs, pump);
		float dr = (cone_h(x + 0.004, b0, ribs, pump) - h0) / 0.004;
		float dt = (cone_h(x, cone_bulge(a + da, amp), ribs, pump) - h0) / (da * max(x, 0.05));
		vec3 n = normalize(vec3(-(dr * dir + dt * vec2(dir.y, -dir.x)), 1.0));
		vec3 light = normalize(vec3(-0.45, -0.65, 0.6));
		float diff = max(dot(n, light), 0.0);
		float spec = pow(max(dot(n, normalize(light + vec3(0.0, 0.0, 1.0))), 0.0), 40.0);
		vec3 cone_c = hue(base, drift + 0.35);
		vec3 speaker = cone_c * (0.05 + 0.32 * diff) + mix(cone_c, vec3(1.0), 0.5) * 0.5 * spec;
		col = mix(col, speaker, heart);
	}
	col += hue(base, drift + 0.15) * 0.6 * exp(-abs(r - rr) * 220.0);

	col += led.rgb * (0.12 + 0.18 * loud) * exp(-r * r * 30.0);

	col *= mix(0.6, 1.0, smoothstep(0.95, 0.2, r));
	col = 1.0 - exp(-col * 1.15);
	col += (hash(FRAGCOORD.xy + fract(TIME) * 97.0) - 0.5) / 255.0;

	COLOR = vec4(col, 1.0);
}
