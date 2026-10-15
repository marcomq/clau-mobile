extends Control
## Glo shell: three on-screen buttons act as the Whale buttons, the shader shows LED + sound.

const RECORD_AUDIO := "android.permission.RECORD_AUDIO"
## On-screen buttons left to right.
const ZONE_BUTTONS := [2, 1, 3]
const ZONE_LABELS := ["U2", "U1  ⏻", "U3"]
const SAMPLES_PATHS := ["res://data/samples.bin", "user://samples.bin"]

@onready var glo: GloNode = $Glo
@onready var visual: ColorRect = $Visual
@onready var buttons: Control = $Buttons
@onready var status: Label = $Status

var _touch_zone := {}  # touch index -> button
var _zone_down := [0, 0, 0]
var _led := Color.BLACK
var _rms := 0.0
var _bands := PackedFloat32Array()
var _key_tilt := Vector3(0, 0, -1)


func _ready() -> void:
	_bands.resize(8)
	buttons.draw.connect(_draw_buttons)
	if not glo.boot(ProjectSettings.globalize_path("user://"), _load_samples()):
		status.text = "firmware failed to start"
		return
	if OS.get_name() == "Android":
		get_tree().on_request_permissions_result.connect(_on_permission_result)
		if OS.request_permission(RECORD_AUDIO):
			glo.start_audio(true, 0)
		else:
			glo.start_audio(false, 0)
	else:
		glo.use_accelerometer = false
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
		glo.start_audio(OS.get_name() != "Android" or RECORD_AUDIO in OS.get_granted_permissions(), 0)


func _button_radius() -> float:
	return minf(size.x * 0.12, 90.0)


func _button_center(i: int) -> Vector2:
	return Vector2(size.x * (0.2 + 0.3 * i), size.y - 96.0 - _button_radius())


## Generous hit area so the buttons are easy to find by feel; -1 outside all of them.
func _button_at(pos: Vector2) -> int:
	for i in 3:
		if pos.distance_to(_button_center(i)) < _button_radius() * 1.35:
			return i
	return -1


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


func _set_zone(zone: int, delta: int) -> void:
	var was_down: bool = _zone_down[zone] > 0
	_zone_down[zone] = maxi(_zone_down[zone] + delta, 0)
	var is_down: bool = _zone_down[zone] > 0
	if was_down != is_down:
		glo.set_button(ZONE_BUTTONS[zone], is_down)


func _input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		if event.pressed:
			var zone := _button_at(event.position)
			if zone < 0:
				return
			_touch_zone[event.index] = zone
			_set_zone(zone, 1)
		elif _touch_zone.has(event.index):
			_set_zone(_touch_zone[event.index], -1)
			_touch_zone.erase(event.index)
	elif event is InputEventKey and not event.echo:
		var keys := {KEY_1: 1, KEY_2: 2, KEY_3: 3}
		if keys.has(event.keycode):
			glo.set_button(keys[event.keycode], event.pressed)
		_key_tilt_input(event)


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
	buttons.queue_redraw()
	status.text = "channel %d  ·  %d Hz%s" % [glo.get_channel() + 1, glo.get_engine_rate(), "" if glo.is_powered() else "  ·  off (hold middle)"]
