shader_type canvas_item;

// Whale RGB LED (smoothed), overall loudness and 8 bands (60 Hz .. 12 kHz).
uniform vec4 led : source_color = vec4(0.0);
uniform float rms = 0.0;
uniform float bands[8];

float band_at(float t) {
	float x = clamp(t, 0.0, 1.0) * 7.0;
	int i = int(floor(x));
	int j = min(i + 1, 7);
	return mix(bands[i], bands[j], fract(x));
}

void fragment() {
	vec2 res = 1.0 / SCREEN_PIXEL_SIZE;
	vec2 p = (UV - vec2(0.5, 0.45)) * vec2(res.x / res.y, 1.0);
	float r = length(p);
	float a = atan(p.y, p.x) / TAU + 0.5;

	float loud = clamp(rms * 9.0, 0.0, 2.0);
	float ring_r = 0.16 + 0.12 * loud;
	float spectrum = band_at(abs(a * 2.0 - 1.0)) * 20.0;
	float wobble = 0.02 * sin(a * TAU * 6.0 + TIME * 1.3) * (0.3 + 1.5 * loud);
	float d = abs(r - ring_r - wobble - 0.08 * spectrum);

	vec3 base = max(led.rgb, vec3(0.06, 0.09, 0.14));
	float width = 0.012 * (1.0 + 0.6 * loud);
	vec3 col = base * (width / (d + width));
	col += led.rgb * (0.25 + 0.3 * loud) * smoothstep(0.55, 0.0, r);
	col += base * 0.1 * loud * (0.5 + 0.5 * sin(r * 40.0 - TIME * 3.0));

	COLOR = vec4(col, 1.0);
}
