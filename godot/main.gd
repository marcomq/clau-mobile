extends Control
## Glo shell: three on-screen buttons act as the Whale buttons, the shader shows LED + sound.

const RECORD_AUDIO := "android.permission.RECORD_AUDIO"
## On-screen buttons left to right.
const ZONE_BUTTONS := [2, 1, 3]
const ZONE_LABELS := ["U2", "U1  ⏻", "U3"]
const SETTINGS_PATH := "user://app.cfg"
const SAMPLES_PATHS := ["res://data/samples.bin", "user://samples.bin"]
## GPL-3.0 section 5(d): interactive programs show the Appropriate Legal Notices.
const ABOUT_TEXT := """glo-mobile — the Glo "Polyphonic Whale" firmware v1.0 (gechologic / phonicbloom) on a phone.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 3. It comes with ABSOLUTELY NO WARRANTY. License: https://www.gnu.org/licenses/gpl-3.0.html
Source code: https://github.com/marcomq/glo-mobile

Includes: Glo firmware (GPL-3.0), Mutable Instruments Clouds and stmlib by Olivier Gillet (MIT), Dekrispator by Xavier Halgand with code by Sean Bolton and Julian Schmidt (GPL), godot-rust (MPL-2.0), cpal (Apache-2.0) and other Rust crates (MIT / Apache-2.0). Full list: NOTICE.md in the source.

This app uses Godot Engine, available under the following license:

"""

@onready var glo: GloNode = $Glo
@onready var visual: ColorRect = $Visual
@onready var buttons: Control = $Buttons
@onready var status: Label = $Status
@onready var info: Label = $Info
@onready var level: Label = $Level
@onready var mic_source: Label = $MicSource
@onready var mic_down: Label = $MicDown
@onready var mic_up: Label = $MicUp
@onready var about: ColorRect = $About

var _touch_zone := {}  # touch index -> button
var _zone_down := [0, 0, 0]
var _led := Color.BLACK
var _rms := 0.0
var _bands := PackedFloat32Array()
var _key_tilt := Vector3(0, 0, -1)
var _headphones := true
var _headphone_check := 0.0
## Rotates the holding pose captured at start (or by tapping ◎) onto the Glo's rest pose, lying flat.
var _neutral := Quaternion.IDENTITY
var _calibrate := true
var _stick_touch := -1
var _stick := Vector2.ZERO
var _mic_agc_db := -1
var _usb_mic_connected := false
## Mic sensitivity trim in dB per source, on top of the firmware's AGC (6x U2 / 6x U3).
var _mic_trim := {"builtin": 0, "usb": 0}
var _notice := ""
var _notice_time := 0.0


func _ready() -> void:
	_bands.resize(8)
	buttons.draw.connect(_draw_buttons)
	$About/Text.text = ABOUT_TEXT + Engine.get_license_text()
	glo.use_accelerometer = false
	if not glo.boot(ProjectSettings.globalize_path("user://"), _load_samples()):
		status.text = "firmware failed to start"
		return
	if OS.get_name() == "Android":
		var cfg := ConfigFile.new()
		cfg.load(SETTINGS_PATH)
		glo.usb_mic = cfg.get_value("audio", "usb_mic", false)
		for k in _mic_trim:
			_mic_trim[k] = cfg.get_value("audio", "mic_trim_" + k, 0)
		mic_source.visible = true
		mic_down.visible = true
		mic_up.visible = true
		_usb_mic_connected = glo.has_usb_mic()
		get_tree().on_request_permissions_result.connect(_on_permission_result)
		if OS.request_permission(RECORD_AUDIO):
			glo.start_audio(true, 0)
		else:
			glo.start_audio(false, 0)
	else:
		glo.start_audio(true, 0)


func _load_samples() -> PackedByteArray:
	for path in SAMPLES_PATHS:
		if FileAccess.file_exists(path):
			return FileAccess.get_file_as_bytes(path)
	return PackedByteArray()


func _on_permission_result(permission: String, granted: bool) -> void:
	if permission == RECORD_AUDIO and granted:
		glo.start_audio(true, 0)


func _notification(what: int) -> void:
	# Release the audio device in the background; the firmware simply waits until audio resumes.
	if what == NOTIFICATION_APPLICATION_PAUSED:
		glo.stop_audio()
	elif what == NOTIFICATION_APPLICATION_RESUMED:
		_restart_audio()


func _restart_audio() -> void:
	glo.start_audio(OS.get_name() != "Android" or RECORD_AUDIO in OS.get_granted_permissions(), 0)


func _toggle_usb_mic() -> void:
	glo.usb_mic = not glo.usb_mic
	var cfg := ConfigFile.new()
	cfg.load(SETTINGS_PATH)
	cfg.set_value("audio", "usb_mic", glo.usb_mic)
	cfg.save(SETTINGS_PATH)
	_restart_audio()
	_show_mic()


func _mic_key() -> String:
	return "usb" if glo.is_mic_usb() else "builtin"


func _change_mic_trim(delta_db: int) -> void:
	var k := _mic_key()
	_mic_trim[k] = clampi(_mic_trim[k] + delta_db, -24, 24)
	var cfg := ConfigFile.new()
	cfg.load(SETTINGS_PATH)
	cfg.set_value("audio", "mic_trim_" + k, _mic_trim[k])
	cfg.save(SETTINGS_PATH)
	_show_notice("mic sensitivity %+d dB" % _mic_trim[k])


func _show_mic() -> void:
	var mic_name: String = glo.get_mic_name()
	_show_notice("recording from " + (mic_name if mic_name != "" else "no mic"))


func _show_notice(text: String) -> void:
	_notice = text
	_notice_time = 3.0


func _button_radius() -> float:
	return minf(size.x * 0.12, 90.0)


func _button_center(i: int) -> Vector2:
	return Vector2(size.x * (0.2 + 0.3 * i), size.y - 96.0 - _button_radius())


func _stick_center() -> Vector2:
	return Vector2(size.x * 0.5, size.y * 0.45)


func _stick_radius() -> float:
	return minf(size.x * 0.4, size.y * 0.22)


## Joystick deflection as a rotation: half the radius tilts by 90°, the edge turns the Glo upside down
## (the firmware uses z up to +0.95). Screen right = +x, screen up = +y.
func _stick_rotation() -> Quaternion:
	if _stick == Vector2.ZERO:
		return Quaternion.IDENTITY
	var dir := _stick.normalized()
	return Quaternion(Vector3(-dir.y, -dir.x, 0), _stick.length() * PI)


func _move_stick(pos: Vector2) -> void:
	_stick = ((pos - _stick_center()) / _stick_radius()).limit_length(1.0)


## Each full-height third of the screen is a touch field; the circles show which one is held.
func _button_at(pos: Vector2) -> int:
	return clampi(int(pos.x / size.x * 3.0), 0, 2)


func _draw_buttons() -> void:
	var r := _button_radius()
	var font := ThemeDB.fallback_font
	for i in 3:
		var c := _button_center(i)
		var down: bool = _zone_down[i] > 0
		var tint := Color(0.55, 0.65, 0.8).lerp(_led, 0.6)
		buttons.draw_circle(c, r, Color(tint, 0.45 if down else 0.08))
		buttons.draw_arc(c, r, 0.0, TAU, 64, Color(tint, 0.9 if down else 0.45), 3.0, true)
		var label: String = ZONE_LABELS[i]
		var w := font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, 28).x
		buttons.draw_string(font, c + Vector2(-w * 0.5, 10.0), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 28, Color(1, 1, 1, 0.85))
	var held := _stick_touch >= 0
	buttons.draw_arc(_stick_center(), _stick_radius(), 0.0, TAU, 96, Color(1, 1, 1, 0.25 if held else 0.08), 2.0, true)
	if held:
		buttons.draw_circle(_stick_center() + _stick * _stick_radius(), r * 0.5, Color(1, 1, 1, 0.35))


func _set_zone(zone: int, delta: int) -> void:
	var was_down: bool = _zone_down[zone] > 0
	_zone_down[zone] = maxi(_zone_down[zone] + delta, 0)
	var is_down: bool = _zone_down[zone] > 0
	if was_down != is_down:
		glo.set_button(ZONE_BUTTONS[zone], is_down)


func _input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		if event.pressed:
			if info.get_global_rect().grow(16.0).has_point(event.position):
				about.visible = not about.visible
				return
			if about.visible:
				return
			if level.get_global_rect().grow(16.0).has_point(event.position):
				_calibrate = true
				return
			if mic_down.visible and mic_down.get_global_rect().grow(8.0).has_point(event.position):
				_change_mic_trim(-3)
				return
			if mic_up.visible and mic_up.get_global_rect().grow(8.0).has_point(event.position):
				_change_mic_trim(3)
				return
			if mic_source.visible and mic_source.get_global_rect().grow(16.0).has_point(event.position):
				_toggle_usb_mic()
				return
			if _stick_touch < 0 and event.position.distance_to(_stick_center()) < _stick_radius():
				_stick_touch = event.index
				_move_stick(event.position)
				return
			var zone := _button_at(event.position)
			_touch_zone[event.index] = zone
			_set_zone(zone, 1)
		elif event.index == _stick_touch:
			_stick_touch = -1
			glo.set_accel(_key_tilt)
		elif _touch_zone.has(event.index):
			_set_zone(_touch_zone[event.index], -1)
			_touch_zone.erase(event.index)
	elif event is InputEventScreenDrag and event.index == _stick_touch:
		_move_stick(event.position)
	elif event is InputEventScreenDrag and about.visible:
		_scroll_about(-event.relative.y)
	elif event is InputEventMouseButton and about.visible and event.pressed:
		match event.button_index:
			MOUSE_BUTTON_WHEEL_UP: _scroll_about(-60.0)
			MOUSE_BUTTON_WHEEL_DOWN: _scroll_about(60.0)
	elif event is InputEventKey and not event.echo:
		var keys := {KEY_1: 1, KEY_2: 2, KEY_3: 3}
		if keys.has(event.keycode):
			glo.set_button(keys[event.keycode], event.pressed)
		_key_tilt_input(event)


func _scroll_about(dy: float) -> void:
	var bar: VScrollBar = $About/Text.get_v_scroll_bar()
	bar.value = clampf(bar.value + dy, 0.0, bar.max_value)


## Desktop: arrow keys tilt the virtual device.
func _key_tilt_input(event: InputEventKey) -> void:
	var tilt := 0.7 if event.pressed else 0.0
	match event.keycode:
		KEY_LEFT: _key_tilt.x = -tilt
		KEY_RIGHT: _key_tilt.x = tilt
		KEY_UP: _key_tilt.y = tilt
		KEY_DOWN: _key_tilt.y = -tilt
		_: return
	_key_tilt.z = -sqrt(maxf(1.0 - _key_tilt.x * _key_tilt.x - _key_tilt.y * _key_tilt.y, 0.0))
	glo.set_accel(_key_tilt)


func _process(delta: float) -> void:
	var levels: PackedFloat32Array = glo.get_levels()
	var k := 1.0 - exp(-delta * 12.0)
	_rms = lerpf(_rms, (levels[0] + levels[1]) * 0.5, k)
	for i in 8:
		_bands[i] = lerpf(_bands[i], levels[2 + i], k)
	_led = _led.lerp(glo.get_led(), 1.0 - exp(-delta * 30.0))

	var mat := visual.material as ShaderMaterial
	mat.set_shader_parameter("led", _led)
	mat.set_shader_parameter("rms", _rms)
	mat.set_shader_parameter("bands", _bands)
	_feed_tilt()
	buttons.queue_redraw()
	_headphone_check -= delta
	if _headphone_check <= 0.0:
		_headphone_check = 2.0
		_headphones = glo.has_headphones()
		if mic_source.visible and glo.has_usb_mic() != _usb_mic_connected:
			_usb_mic_connected = not _usb_mic_connected
			_restart_audio()
			_show_mic()
	status.text = "channel %d  ·  %d Hz%s" % [glo.get_channel() + 1, glo.get_engine_rate(), "" if glo.is_powered() else "  ·  off (hold middle)"]
	var agc_db: int = glo.get_mic_agc_db()
	if agc_db != _mic_agc_db:
		if _mic_agc_db >= 0 and agc_db >= 0:
			_show_notice("mic gain %d dB" % agc_db)
		_mic_agc_db = agc_db
	_notice_time -= delta
	if _notice_time > 0.0:
		status.text += "  ·  " + _notice
	var trim: int = _mic_trim[_mic_key()]
	glo.set_mic_trim_db(trim)
	mic_source.text = ("mic: USB" if glo.is_mic_usb() else "mic: built-in") + ("" if trim == 0 else "  %+d dB" % trim)
	if not _headphones:
		status.text = "no headphones connected: the Glo is meant to be played with headphones\n" + status.text
	status.modulate.a = 0.45 if _headphones else 0.9


## The joystick tilts on top of the accelerometer (or the arrow-key tilt on desktop).
func _feed_tilt() -> void:
	var g := Input.get_accelerometer() * glo.accel_sign / 9.81
	if g != Vector3.ZERO:
		if _calibrate:
			_calibrate = false
			_neutral = Quaternion(g.normalized(), Vector3(0, 0, -1))
			level.modulate.a = 1.0
		level.modulate.a = move_toward(level.modulate.a, 0.45, get_process_delta_time())
		g = _neutral * g
	elif _stick_touch >= 0:
		g = _key_tilt
	else:
		return
	if _stick_touch >= 0:
		g = _stick_rotation() * g
	glo.set_accel(g)
