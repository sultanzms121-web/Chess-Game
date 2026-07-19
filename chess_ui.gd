extends CanvasLayer
class_name ChessUI

# ─── Palette ─────────────────────────────────────────────────────────────
const COL_BG           := Color(0.98, 0.97, 0.94, 0.92)
const COL_GOLD         := Color(0.78, 0.62, 0.20, 1.0)
const COL_GOLD_BRIGHT  := Color(0.89, 0.74, 0.32, 1.0)
const COL_INK          := Color(0.14, 0.13, 0.11, 1.0)
const COL_INK_SOFT     := Color(0.14, 0.13, 0.11, 0.65)
const COL_BLACK_ACCENT := Color(0.10, 0.10, 0.11, 1.0)
const COL_DANGER       := Color(0.62, 0.16, 0.14, 1.0)
const COL_LIGHT_OVERLAY:= Color(0.98, 0.98, 0.98, 0.85) # Replaced dark bg with a frosted glass look

var game_manager: GameManager
var board_root: Node3D

enum Mode { NONE, STANDARD, ADVANCED }
var current_mode: Mode = Mode.NONE
var selected_time_seconds: int = 300

var top_bar: PanelContainer
var turn_label: Label
var turn_dot: ColorRect
var check_label: Label

var white_timer_label: Label
var black_timer_label: Label
var white_time_left: float = 0.0
var black_time_left: float = 0.0
var timer_running: bool = false
var timer_container: HBoxContainer

var white_captured_tray: HBoxContainer
var black_captured_tray: HBoxContainer
var undo_button: Button

var mode_select_root: Control
var game_ui_root: Control
var time_picker_box: VBoxContainer
var time_btn_group: ButtonGroup
var game_mode_modal: PanelContainer
var opponent_modal: PanelContainer
var ai_difficulty_modal: PanelContainer
var settings_modal: PanelContainer
var help_modal: PanelContainer
var is_vs_ai: bool = false
var ai_difficulty: int = 1
var promotion_modal: PanelContainer
var promotion_title: Label
var intro_logo_container: ColorRect
var game_over_modal: PanelContainer
var game_over_title: Label
var game_over_subtitle: Label
var promotion_callback: Callable

var piece_sound: AudioStreamPlayer
var heartbeat_sound: AudioStreamPlayer
var tick_sound: AudioStreamPlayer
var intro_sound: AudioStreamPlayer
var is_heartbeat_playing: bool = false
var _last_tick_second: int = -1

var game_actions_container: HBoxContainer
var back_button: Button
var camera_button: Button
var capture_button: Button
var is_camera_mode: bool = false

signal camera_toggled(is_active: bool)

const PIECE_GLYPH := {
	"king": "♚", "queen": "♛", "rook": "♜",
	"bishop": "♝", "knight": "♞", "pawn": "♟"
}
const TIME_OPTIONS := [60, 180, 300, 600, 900]
const TIME_LABELS := ["1 min", "3 min", "5 min", "10 min", "15 min"]


# ════════════════════════════════════════════════════════════════════════
#  READY
# ════════════════════════════════════════════════════════════════════════
func _ready() -> void:
	layer = 10
	_setup_audio()
	_build_mode_select()
	_build_game_ui()
	_build_promotion_modal()
	_build_game_over_modal()
	_build_intro_logo()


# ════════════════════════════════════════════════════════════════════════
#  AUDIO
# ════════════════════════════════════════════════════════════════════════
func _setup_audio() -> void:
	piece_sound = AudioStreamPlayer.new()
	add_child(piece_sound)
	piece_sound.stream = load("res://audio/piece_place.wav")
	piece_sound.volume_db = 2.0

	heartbeat_sound = AudioStreamPlayer.new()
	add_child(heartbeat_sound)
	heartbeat_sound.stream = load("res://audio/heartbeat.wav")
	heartbeat_sound.volume_db = 6.0
	heartbeat_sound.pitch_scale = 0.8
	heartbeat_sound.finished.connect(_on_heartbeat_finished)

	tick_sound = AudioStreamPlayer.new()
	add_child(tick_sound)
	tick_sound.stream = load("res://audio/tick.mp3")
	tick_sound.volume_db = -2.0

	intro_sound = AudioStreamPlayer.new()
	add_child(intro_sound)
	if ResourceLoader.exists("res://audio/intro_theme.mp3"):
		var stream = load("res://audio/intro_theme.mp3")
		if "loop" in stream:
			stream.loop = true
		intro_sound.stream = stream
	intro_sound.volume_db = -5.0


# ════════════════════════════════════════════════════════════════════════
#  MODE SELECT SCREEN
# ════════════════════════════════════════════════════════════════════════
func _build_mode_select() -> void:
	mode_select_root = Control.new()
	mode_select_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	mode_select_root.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(mode_select_root)

	var bg := ColorRect.new()
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
	
	var mat = ShaderMaterial.new()
	var shader = load("res://shaders/ocean_waves.gdshader")
	if shader:
		mat.shader = shader
	bg.material = mat
	mode_select_root.add_child(bg)

	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	center.mouse_filter = Control.MOUSE_FILTER_IGNORE
	mode_select_root.add_child(center)
	
	var settings_btn := Button.new()
	settings_btn.text = "⚙" # Unicode gear
	settings_btn.custom_minimum_size = Vector2(60, 60)
	var sb = StyleBoxEmpty.new()
	settings_btn.add_theme_stylebox_override("normal", sb)
	settings_btn.add_theme_stylebox_override("hover", sb)
	settings_btn.add_theme_stylebox_override("pressed", sb)
	settings_btn.add_theme_stylebox_override("focus", sb)
	settings_btn.add_theme_font_size_override("font_size", 42)
	settings_btn.add_theme_color_override("font_color", Color.WHITE)
	settings_btn.add_theme_color_override("font_hover_color", Color(0.8, 0.8, 0.8))
	settings_btn.add_theme_color_override("font_pressed_color", Color(0.6, 0.6, 0.6))
	settings_btn.pressed.connect(func(): _show_settings_modal())
	
	mode_select_root.add_child(settings_btn)
	settings_btn.set_anchors_and_offsets_preset(Control.PRESET_TOP_RIGHT, Control.PRESET_MODE_MINSIZE, 20)

	var card := VBoxContainer.new()
	card.add_theme_constant_override("separation", 40)
	center.add_child(card)

	var title := Label.new()
	title.text = "CHESS WORLD"
	title.add_theme_font_size_override("font_size", 64)
	title.add_theme_color_override("font_color", Color(0.95, 0.98, 1.0, 1.0))
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_color_override("font_shadow_color", Color(0.7, 0.8, 0.9, 0.5))
	title.add_theme_constant_override("shadow_offset_x", 3)
	title.add_theme_constant_override("shadow_offset_y", 3)
	title.add_theme_constant_override("shadow_outline_size", 6)
	card.add_child(title)

	var btn_vbox := VBoxContainer.new()
	btn_vbox.add_theme_constant_override("separation", 20)
	btn_vbox.custom_minimum_size = Vector2(300, 0)
	btn_vbox.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	card.add_child(btn_vbox)

	var start_btn := Button.new()
	start_btn.text = "Start Game"
	start_btn.custom_minimum_size = Vector2(300, 55)
	_style_metallic_button(start_btn)
	start_btn.pressed.connect(func(): 
		_show_opponent_modal()
	)
	btn_vbox.add_child(start_btn)
	
	var mode_btn := Button.new()
	mode_btn.text = "Game Mode"
	mode_btn.custom_minimum_size = Vector2(300, 55)
	_style_metallic_button(mode_btn)
	mode_btn.pressed.connect(func(): _show_game_mode_modal())
	btn_vbox.add_child(mode_btn)
	
	var exit_btn := Button.new()
	exit_btn.text = "Exit Game"
	exit_btn.custom_minimum_size = Vector2(300, 55)
	_style_metallic_button(exit_btn)
	exit_btn.pressed.connect(func(): get_tree().quit())
	btn_vbox.add_child(exit_btn)

	_build_game_mode_modal()
	_build_opponent_modal()
	_build_ai_difficulty_modal()
	_build_settings_modal()

func _build_opponent_modal() -> void:
	opponent_modal = PanelContainer.new()
	var cs := StyleBoxFlat.new()
	cs.bg_color = Color(0.04, 0.04, 0.04, 0.95)
	cs.set_corner_radius_all(15)
	cs.set_border_width_all(2)
	cs.border_color = Color(0.8, 0.9, 1.0, 0.8)
	cs.set_content_margin_all(24)
	opponent_modal.add_theme_stylebox_override("panel", cs)
	
	opponent_modal.set_anchors_preset(Control.PRESET_CENTER)
	opponent_modal.visible = false
	mode_select_root.add_child(opponent_modal)

	var v := VBoxContainer.new()
	v.add_theme_constant_override("separation", 24)
	opponent_modal.add_child(v)

	var title := Label.new()
	title.text = "Select Opponent"
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0, 1.0))
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(title)

	var btn_vbox := VBoxContainer.new()
	btn_vbox.add_theme_constant_override("separation", 20)
	btn_vbox.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	v.add_child(btn_vbox)

	var pvp_btn := Button.new()
	pvp_btn.text = "P vs P"
	pvp_btn.custom_minimum_size = Vector2(250, 55)
	_style_metallic_button(pvp_btn)
	pvp_btn.pressed.connect(func(): 
		is_vs_ai = false
		_hide_opponent_modal()
		_start_game_pressed()
	)
	btn_vbox.add_child(pvp_btn)

	var pvai_btn := Button.new()
	pvai_btn.text = "P vs AI"
	pvai_btn.custom_minimum_size = Vector2(250, 55)
	_style_metallic_button(pvai_btn)
	pvai_btn.pressed.connect(func(): 
		is_vs_ai = true
		_hide_opponent_modal()
		_show_ai_difficulty_modal()
	)
	btn_vbox.add_child(pvai_btn)

	var close_btn := Button.new()
	close_btn.text = "Close"
	close_btn.custom_minimum_size = Vector2(120, 40)
	close_btn.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	_style_metallic_button(close_btn)
	close_btn.pressed.connect(func(): _hide_opponent_modal())
	v.add_child(close_btn)

func _build_ai_difficulty_modal() -> void:
	ai_difficulty_modal = PanelContainer.new()
	var cs := StyleBoxFlat.new()
	cs.bg_color = Color(0.04, 0.04, 0.04, 0.95)
	cs.set_corner_radius_all(15)
	cs.set_border_width_all(2)
	cs.border_color = Color(0.8, 0.9, 1.0, 0.8)
	cs.set_content_margin_all(24)
	ai_difficulty_modal.add_theme_stylebox_override("panel", cs)
	
	ai_difficulty_modal.set_anchors_preset(Control.PRESET_CENTER)
	ai_difficulty_modal.visible = false
	mode_select_root.add_child(ai_difficulty_modal)

	var v := VBoxContainer.new()
	v.add_theme_constant_override("separation", 24)
	ai_difficulty_modal.add_child(v)

	var title := Label.new()
	title.text = "Select AI Difficulty"
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0, 1.0))
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(title)

	var btn_vbox := VBoxContainer.new()
	btn_vbox.add_theme_constant_override("separation", 20)
	btn_vbox.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	v.add_child(btn_vbox)
	
	var difficulties = ["Easy", "Medium", "Hard"]
	for i in range(difficulties.size()):
		var btn := Button.new()
		btn.text = difficulties[i]
		btn.custom_minimum_size = Vector2(250, 55)
		_style_metallic_button(btn)
		var cap_i := i
		btn.pressed.connect(func(): 
			ai_difficulty = cap_i
			_hide_ai_difficulty_modal()
			_start_game_pressed()
		)
		btn_vbox.add_child(btn)

	var back_btn := Button.new()
	back_btn.text = "Back"
	back_btn.custom_minimum_size = Vector2(120, 40)
	back_btn.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	_style_metallic_button(back_btn)
	back_btn.pressed.connect(func():
		_hide_ai_difficulty_modal()
		_show_opponent_modal()
	)
	v.add_child(back_btn)

func _build_settings_modal() -> void:
	settings_modal = PanelContainer.new()
	var cs := StyleBoxFlat.new()
	cs.bg_color = Color(0.04, 0.04, 0.04, 0.95)
	cs.set_corner_radius_all(15)
	cs.set_border_width_all(2)
	cs.border_color = Color(0.8, 0.9, 1.0, 0.8)
	cs.set_content_margin_all(24)
	settings_modal.add_theme_stylebox_override("panel", cs)
	
	settings_modal.set_anchors_preset(Control.PRESET_CENTER)
	settings_modal.visible = false
	mode_select_root.add_child(settings_modal)

	var v := VBoxContainer.new()
	v.add_theme_constant_override("separation", 24)
	settings_modal.add_child(v)

	var title := Label.new()
	title.text = "Settings"
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0, 1.0))
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(title)

	var btn_vbox := VBoxContainer.new()
	btn_vbox.add_theme_constant_override("separation", 20)
	btn_vbox.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	v.add_child(btn_vbox)

	var resolutions = [
		{"text": "1920 x 1080", "mode": DisplayServer.WINDOW_MODE_WINDOWED, "size": Vector2i(1920, 1080)},
		{"text": "1280 x 720", "mode": DisplayServer.WINDOW_MODE_WINDOWED, "size": Vector2i(1280, 720)},
		{"text": "Fullscreen", "mode": DisplayServer.WINDOW_MODE_FULLSCREEN, "size": Vector2i(1920, 1080)}
	]
	
	for res in resolutions:
		var btn := Button.new()
		btn.text = res["text"]
		btn.custom_minimum_size = Vector2(250, 55)
		_style_metallic_button(btn)
		btn.pressed.connect(func():
			if res["mode"] == DisplayServer.WINDOW_MODE_FULLSCREEN:
				get_window().mode = Window.MODE_FULLSCREEN
			else:
				get_window().mode = Window.MODE_WINDOWED
				get_window().size = res["size"]
		)
		btn_vbox.add_child(btn)

	var close_btn := Button.new()
	close_btn.text = "Close"
	close_btn.custom_minimum_size = Vector2(120, 40)
	close_btn.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	_style_metallic_button(close_btn)
	close_btn.pressed.connect(func(): _hide_settings_modal())
	v.add_child(close_btn)

func _show_settings_modal() -> void:
	settings_modal.visible = true
	await get_tree().process_frame
	var vs := get_viewport().get_visible_rect().size
	settings_modal.position = (vs - settings_modal.size) / 2.0

func _hide_settings_modal() -> void:
	settings_modal.visible = false

func _build_game_mode_modal() -> void:
	game_mode_modal = PanelContainer.new()
	var cs := StyleBoxFlat.new()
	cs.bg_color = Color(0.04, 0.04, 0.04, 0.95)
	cs.set_corner_radius_all(15)
	cs.set_border_width_all(2)
	cs.border_color = Color(0.8, 0.9, 1.0, 0.8)
	cs.set_content_margin_all(24)
	game_mode_modal.add_theme_stylebox_override("panel", cs)
	
	game_mode_modal.set_anchors_preset(Control.PRESET_CENTER)
	game_mode_modal.visible = false
	mode_select_root.add_child(game_mode_modal)

	var v := VBoxContainer.new()
	v.add_theme_constant_override("separation", 24)
	game_mode_modal.add_child(v)

	var title := Label.new()
	title.text = "Select Game Mode"
	title.add_theme_font_size_override("font_size", 28)
	title.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0, 1.0))
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(title)

	var row := HBoxContainer.new()
	row.alignment = BoxContainer.ALIGNMENT_CENTER
	row.add_theme_constant_override("separation", 20)
	v.add_child(row)

	row.add_child(_mode_card_metallic(
		"Standard", "Standard",
		"Legal move highlights\nNo time limit\nGreat for learning",
		func(): 
			current_mode = Mode.STANDARD
			_hide_game_mode_modal()
	))
	row.add_child(_mode_card_metallic(
		"Advanced", "Advanced",
		"No move highlights\nPer-player countdown\nCompetitive play",
		func(): _show_time_picker()
	))

	time_picker_box = VBoxContainer.new()
	time_picker_box.add_theme_constant_override("separation", 14)
	time_picker_box.visible = false
	v.add_child(time_picker_box)

	var tp_lbl := Label.new()
	tp_lbl.text = "Select time per player:"
	tp_lbl.add_theme_font_size_override("font_size", 16)
	tp_lbl.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0, 1.0))
	tp_lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	time_picker_box.add_child(tp_lbl)

	var tp_row := HBoxContainer.new()
	tp_row.alignment = BoxContainer.ALIGNMENT_CENTER
	tp_row.add_theme_constant_override("separation", 10)
	time_picker_box.add_child(tp_row)

	time_btn_group = ButtonGroup.new()
	for i in range(TIME_OPTIONS.size()):
		var tb := Button.new()
		tb.text = TIME_LABELS[i]
		tb.custom_minimum_size = Vector2(75, 40)
		tb.toggle_mode = true
		tb.button_group = time_btn_group
		if TIME_OPTIONS[i] == selected_time_seconds:
			tb.button_pressed = true
		var cap_i := i
		tb.pressed.connect(func(): selected_time_seconds = TIME_OPTIONS[cap_i])
		_style_metallic_button(tb)
		tp_row.add_child(tb)

	var confirm_btn := Button.new()
	confirm_btn.text = "Confirm Advanced"
	confirm_btn.custom_minimum_size = Vector2(250, 45)
	_style_metallic_button(confirm_btn)
	confirm_btn.pressed.connect(func(): 
		current_mode = Mode.ADVANCED
		_hide_game_mode_modal()
	)
	time_picker_box.add_child(confirm_btn)
	
	var close_btn := Button.new()
	close_btn.text = "Close"
	close_btn.custom_minimum_size = Vector2(120, 40)
	close_btn.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	_style_metallic_button(close_btn)
	close_btn.pressed.connect(func(): _hide_game_mode_modal())
	v.add_child(close_btn)

func _mode_card_metallic(_title_text: String, icon_text: String, desc: String, cb: Callable) -> PanelContainer:
	var card := PanelContainer.new()
	var cs := StyleBoxFlat.new()
	cs.bg_color = Color(0.02, 0.02, 0.02, 0.9)
	cs.set_corner_radius_all(10)
	cs.set_border_width_all(1)
	cs.border_color = Color(0.7, 0.8, 0.9, 0.4)
	cs.set_content_margin_all(20)
	card.add_theme_stylebox_override("panel", cs)
	card.custom_minimum_size = Vector2(210, 0)

	var v := VBoxContainer.new()
	v.alignment = BoxContainer.ALIGNMENT_CENTER
	v.add_theme_constant_override("separation", 10)
	card.add_child(v)

	var ic := Label.new()
	ic.text = icon_text
	ic.add_theme_font_size_override("font_size", 22)
	ic.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0, 1.0))
	ic.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(ic)

	var d := Label.new()
	d.text = desc
	d.add_theme_font_size_override("font_size", 12)
	d.add_theme_color_override("font_color", Color(0.7, 0.75, 0.8, 1.0))
	d.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(d)

	var btn := Button.new()
	btn.text = "Select"
	btn.custom_minimum_size = Vector2(140, 40)
	_style_metallic_button(btn)
	btn.pressed.connect(cb)
	v.add_child(btn)
	return card

func _style_metallic_button(btn: Button) -> void:
	var normal := StyleBoxFlat.new()
	normal.bg_color = Color(0.03, 0.03, 0.04, 1.0)
	normal.set_corner_radius_all(10)
	normal.set_border_width_all(2)
	normal.border_color = Color(0.7, 0.8, 0.9, 0.6)
	
	var hover := StyleBoxFlat.new()
	hover.bg_color = Color(0.15, 0.2, 0.25, 1.0)
	hover.set_corner_radius_all(10)
	hover.set_border_width_all(2)
	hover.border_color = Color(0.95, 0.98, 1.0, 1.0)
	
	btn.add_theme_stylebox_override("normal", normal)
	btn.add_theme_stylebox_override("hover", hover)
	btn.add_theme_stylebox_override("pressed", hover)
	
	var dis := normal.duplicate()
	dis.border_color = Color(0.3, 0.3, 0.4, 0.3)
	btn.add_theme_stylebox_override("disabled", dis)
	
	btn.add_theme_color_override("font_color", Color(0.85, 0.9, 0.95, 1.0))
	btn.add_theme_color_override("font_hover_color", Color(1.0, 1.0, 1.0, 1.0))
	btn.add_theme_font_size_override("font_size", 18)

func _show_game_mode_modal() -> void:
	game_mode_modal.visible = true
	# Center it
	await get_tree().process_frame
	var vs := get_viewport().get_visible_rect().size
	game_mode_modal.position = (vs - game_mode_modal.size) / 2.0

func _hide_game_mode_modal() -> void:
	game_mode_modal.visible = false

func _show_time_picker() -> void:
	time_picker_box.visible = true

func _show_opponent_modal() -> void:
	opponent_modal.visible = true
	# Center it
	await get_tree().process_frame
	var vs := get_viewport().get_visible_rect().size
	opponent_modal.position = (vs - opponent_modal.size) / 2.0

func _hide_opponent_modal() -> void:
	opponent_modal.visible = false

func _show_ai_difficulty_modal() -> void:
	ai_difficulty_modal.visible = true
	# Center it
	await get_tree().process_frame
	var vs := get_viewport().get_visible_rect().size
	ai_difficulty_modal.position = (vs - ai_difficulty_modal.size) / 2.0

func _hide_ai_difficulty_modal() -> void:
	ai_difficulty_modal.visible = false

func _start_game_pressed() -> void:
	if intro_sound and intro_sound.playing:
		intro_sound.stop()
	if current_mode == Mode.NONE:
		current_mode = Mode.STANDARD
	_on_mode_selected(current_mode)

func _on_mode_selected(mode: Mode) -> void:
	current_mode = mode
	mode_select_root.visible = false
	mode_select_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	mode_select_root.process_mode = Node.PROCESS_MODE_DISABLED
	game_ui_root.visible = true
	if game_actions_container:
		game_actions_container.visible = true

	# ✨ THIS IS THE FIX: Tell main.gd to hide/show the correct board model
	if board_root and board_root.has_method("set_game_mode"):
		if "is_vs_ai" in board_root:
			board_root.is_vs_ai = is_vs_ai
		if "ai_difficulty" in board_root:
			board_root.ai_difficulty = ai_difficulty
		if mode == Mode.ADVANCED:
			board_root.set_game_mode("advanced")
		else:
			board_root.set_game_mode("standard")

	if game_manager:
		game_manager.set_highlights_enabled(mode == Mode.STANDARD)
		_connect_signals()

	_update_turn_indicator()
	_update_undo_button()

	if mode == Mode.ADVANCED:
		white_time_left = float(selected_time_seconds)
		black_time_left = float(selected_time_seconds)
		_last_tick_second = selected_time_seconds
		timer_container.visible = true
		_update_timer_labels()
		timer_running = true
	else:
		timer_container.visible = false


# ════════════════════════════════════════════════════════════════════════
#  GAME HUD
# ════════════════════════════════════════════════════════════════════════
func _build_game_ui() -> void:
	game_ui_root = Control.new()
	game_ui_root.set_anchors_preset(Control.PRESET_FULL_RECT)
	game_ui_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	game_ui_root.visible = false
	add_child(game_ui_root)

	var top := MarginContainer.new()
	top.set_anchors_preset(Control.PRESET_TOP_WIDE)
	top.add_theme_constant_override("margin_top", 16)
	top.add_theme_constant_override("margin_left", 20)
	top.add_theme_constant_override("margin_right", 20)
	top.mouse_filter = Control.MOUSE_FILTER_IGNORE
	game_ui_root.add_child(top)

	var top_v := VBoxContainer.new()
	top_v.mouse_filter = Control.MOUSE_FILTER_IGNORE
	top_v.add_theme_constant_override("separation", 6)
	top.add_child(top_v)

	_build_top_bar(top_v)
	_build_timer_bar(top_v)

	var bot := MarginContainer.new()
	bot.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	bot.add_theme_constant_override("margin_bottom", 16)
	bot.add_theme_constant_override("margin_left", 20)
	bot.add_theme_constant_override("margin_right", 20)
	bot.mouse_filter = Control.MOUSE_FILTER_IGNORE
	game_ui_root.add_child(bot)

	var tray_row := HBoxContainer.new()
	tray_row.mouse_filter = Control.MOUSE_FILTER_IGNORE
	bot.add_child(tray_row)

	var lp := PanelContainer.new()
	lp.add_theme_stylebox_override("panel", _panel_style(0.35))
	lp.mouse_filter = Control.MOUSE_FILTER_IGNORE
	white_captured_tray = HBoxContainer.new()
	white_captured_tray.add_theme_constant_override("separation", 15)
	lp.add_child(white_captured_tray)
	tray_row.add_child(lp)

	var sp := Control.new()
	sp.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	sp.mouse_filter = Control.MOUSE_FILTER_IGNORE
	tray_row.add_child(sp)

	var rp := PanelContainer.new()
	rp.add_theme_stylebox_override("panel", _panel_style(0.35))
	rp.mouse_filter = Control.MOUSE_FILTER_IGNORE
	black_captured_tray = HBoxContainer.new()
	black_captured_tray.add_theme_constant_override("separation", 15)
	rp.add_child(black_captured_tray)
	tray_row.add_child(rp)

	game_actions_container = HBoxContainer.new()
	game_actions_container.add_theme_constant_override("separation", 10)
	game_actions_container.set_anchors_preset(Control.PRESET_TOP_LEFT)
	game_actions_container.position = Vector2(20, 20)
	game_actions_container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	game_actions_container.visible = false
	add_child(game_actions_container)

	back_button = Button.new()
	back_button.text = "Back"
	back_button.custom_minimum_size = Vector2(80, 36)
	back_button.mouse_filter = Control.MOUSE_FILTER_STOP
	_style_metallic_button(back_button)
	back_button.pressed.connect(_on_back_pressed)
	game_actions_container.add_child(back_button)

	camera_button = Button.new()
	camera_button.text = "📷 Camera"
	camera_button.custom_minimum_size = Vector2(110, 36)
	camera_button.mouse_filter = Control.MOUSE_FILTER_STOP
	_style_metallic_button(camera_button)
	camera_button.pressed.connect(_on_camera_pressed)
	game_actions_container.add_child(camera_button)

	capture_button = Button.new()
	capture_button.text = "📸 Capture"
	capture_button.custom_minimum_size = Vector2(110, 36)
	capture_button.mouse_filter = Control.MOUSE_FILTER_STOP
	_style_metallic_button(capture_button)
	capture_button.pressed.connect(_on_capture_pressed)
	capture_button.visible = false
	game_actions_container.add_child(capture_button)

	undo_button = Button.new()
	undo_button.text = "Undo"
	undo_button.custom_minimum_size = Vector2(80, 36)
	undo_button.mouse_filter = Control.MOUSE_FILTER_STOP
	_style_metallic_button(undo_button)
	undo_button.pressed.connect(_on_undo_pressed)
	game_actions_container.add_child(undo_button)

func _on_back_pressed() -> void:
	timer_running = false
	if tick_sound: tick_sound.stop()
	if heartbeat_sound: heartbeat_sound.stop()
	get_tree().reload_current_scene()

func _on_camera_pressed() -> void:
	is_camera_mode = !is_camera_mode
	if is_camera_mode:
		camera_button.text = "Resume"
		camera_button.add_theme_color_override("font_color", Color(0.9, 0.7, 0.1, 1.0)) # highlight gold
		if capture_button: capture_button.visible = true
	else:
		camera_button.text = "📷 Camera"
		camera_button.add_theme_color_override("font_color", Color(0.85, 0.9, 0.95, 1.0)) # reset
		if capture_button: capture_button.visible = false
	camera_toggled.emit(is_camera_mode)

func _on_capture_pressed() -> void:
	# Hide UI briefly for a cleaner screenshot if desired, but we can just capture the viewport
	# as is, or we can hide the UI first. Let's capture immediately.
	await RenderingServer.frame_post_draw
	var image = get_viewport().get_texture().get_image()
	
	var dir = OS.get_system_dir(OS.SYSTEM_DIR_PICTURES)
	if dir == "":
		dir = "user://"
	
	# Generate a unique filename using timestamp
	var dict = Time.get_datetime_dict_from_system()
	var filename = "/chess_screenshot_%d%02d%02d_%02d%02d%02d.png" % [dict.year, dict.month, dict.day, dict.hour, dict.minute, dict.second]
	var path = dir + filename
	
	if OS.has_feature("web"):
		# Try to use JavaScriptBridge for HTML5 exports
		if ClassDB.class_exists("JavaScriptBridge"):
			var js_bridge = ClassDB.instantiate("JavaScriptBridge")
			if js_bridge:
				js_bridge.download_buffer(image.save_png_to_buffer(), "chess_screenshot.png", "image/png")
	else:
		image.save_png(path)
		print("Screenshot saved to ", path)
		# Optional visual feedback:
		capture_button.text = "Your image is saved to Picture"
		capture_button.custom_minimum_size.x = 280
		await get_tree().create_timer(3.0).timeout
		capture_button.text = "📸 Capture"
		capture_button.custom_minimum_size.x = 110


# ════════════════════════════════════════════════════════════════════════
#  STYLE HELPERS
# ════════════════════════════════════════════════════════════════════════
func _panel_style(border_alpha: float = 0.55) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = COL_BG
	sb.set_corner_radius_all(14)
	sb.set_border_width_all(1)
	sb.border_color = Color(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, border_alpha)
	sb.set_content_margin_all(14)
	sb.shadow_color = Color(0, 0, 0, 0.15)
	sb.shadow_size = 10
	return sb


func _gold_button_style(hover: bool = false) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = COL_GOLD_BRIGHT if hover else COL_GOLD
	sb.set_corner_radius_all(10)
	sb.set_content_margin_all(10)
	return sb


func _style_button(btn: Button) -> void:
	btn.add_theme_stylebox_override("normal", _gold_button_style(false))
	btn.add_theme_stylebox_override("hover", _gold_button_style(true))
	btn.add_theme_stylebox_override("pressed", _gold_button_style(true))
	var dis := _gold_button_style(false)
	dis.bg_color = Color(COL_GOLD.r, COL_GOLD.g, COL_GOLD.b, 0.25)
	btn.add_theme_stylebox_override("disabled", dis)
	btn.add_theme_color_override("font_color", Color.WHITE)
	btn.add_theme_color_override("font_hover_color", Color.WHITE)
	btn.add_theme_color_override("font_disabled_color", Color(1, 1, 1, 0.5))


# ════════════════════════════════════════════════════════════════════════
#  TOP BAR & TIMER BAR
# ════════════════════════════════════════════════════════════════════════
func _build_top_bar(parent: VBoxContainer) -> void:
	top_bar = PanelContainer.new()
	top_bar.add_theme_stylebox_override("panel", _panel_style())
	top_bar.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	top_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	top_bar.visible = false
	parent.add_child(top_bar)

	var h := HBoxContainer.new()
	h.add_theme_constant_override("separation", 10)
	top_bar.add_child(h)

	turn_dot = ColorRect.new()
	turn_dot.custom_minimum_size = Vector2(14, 14)
	turn_dot.color = Color.WHITE
	turn_dot.visible = false
	h.add_child(turn_dot)

	turn_label = Label.new()
	turn_label.text = "White to move"
	turn_label.add_theme_font_size_override("font_size", 20)
	turn_label.add_theme_color_override("font_color", COL_INK)
	turn_label.visible = false
	h.add_child(turn_label)

	check_label = Label.new()
	check_label.text = ""
	check_label.add_theme_font_size_override("font_size", 16)
	check_label.add_theme_color_override("font_color", COL_DANGER)
	check_label.visible = false
	h.add_child(check_label)


func _build_timer_bar(parent: VBoxContainer) -> void:
	timer_container = HBoxContainer.new()
	timer_container.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	timer_container.add_theme_constant_override("separation", 40)
	timer_container.mouse_filter = Control.MOUSE_FILTER_IGNORE
	timer_container.visible = false
	parent.add_child(timer_container)

	var wp := PanelContainer.new()
	wp.add_theme_stylebox_override("panel", StyleBoxEmpty.new())
	timer_container.add_child(wp)
	var wh := HBoxContainer.new()
	wh.add_theme_constant_override("separation", 6)
	wp.add_child(wh)
	var wd := Label.new()
	wd.text = "⏱"
	wd.add_theme_font_size_override("font_size", 20)
	wd.add_theme_color_override("font_color", Color.WHITE)
	wh.add_child(wd)
	white_timer_label = Label.new()
	white_timer_label.text = "5:00"
	white_timer_label.add_theme_font_size_override("font_size", 22)
	white_timer_label.add_theme_color_override("font_color", Color.WHITE)
	white_timer_label.custom_minimum_size = Vector2(70, 0)
	wh.add_child(white_timer_label)

	var bp := PanelContainer.new()
	bp.add_theme_stylebox_override("panel", StyleBoxEmpty.new())
	timer_container.add_child(bp)
	var bh := HBoxContainer.new()
	bh.add_theme_constant_override("separation", 6)
	bp.add_child(bh)
	var bd := Label.new()
	bd.text = "⏱"
	bd.add_theme_font_size_override("font_size", 20)
	bd.add_theme_color_override("font_color", Color.WHITE)
	bh.add_child(bd)
	black_timer_label = Label.new()
	black_timer_label.text = "5:00"
	black_timer_label.add_theme_font_size_override("font_size", 22)
	black_timer_label.add_theme_color_override("font_color", Color.WHITE)
	black_timer_label.custom_minimum_size = Vector2(70, 0)
	bh.add_child(black_timer_label)


# ════════════════════════════════════════════════════════════════════════
#  TIMER LOGIC
# ════════════════════════════════════════════════════════════════════════
func _process(delta: float) -> void:
	if current_mode != Mode.ADVANCED or not timer_running or not game_manager:
		return
	if game_manager.is_game_over():
		timer_running = false
		return

	var cur_sec: int
	if game_manager.is_white_turn():
		white_time_left = max(0.0, white_time_left - delta)
		if white_time_left == 0.0:
			timer_running = false
			_update_timer_labels()
			_trigger_timeout("White")
			return
		cur_sec = int(white_time_left)
	else:
		black_time_left = max(0.0, black_time_left - delta)
		if black_time_left == 0.0:
			timer_running = false
			_update_timer_labels()
			_trigger_timeout("Black")
			return
		cur_sec = int(black_time_left)

	if cur_sec != _last_tick_second:
		_last_tick_second = cur_sec
		if tick_sound and tick_sound.stream:
			tick_sound.play()

	_update_timer_labels()


func _update_timer_labels() -> void:
	if white_timer_label:
		white_timer_label.text = _fmt(white_time_left)
		if white_time_left < 30.0:
			white_timer_label.add_theme_color_override("font_color", Color(0.8, 0.1, 0.1))
		else:
			white_timer_label.add_theme_color_override("font_color", Color.WHITE)
	if black_timer_label:
		black_timer_label.text = _fmt(black_time_left)
		if black_time_left < 30.0:
			black_timer_label.add_theme_color_override("font_color", Color(0.8, 0.1, 0.1))
		else:
			black_timer_label.add_theme_color_override("font_color", Color.WHITE)


func _fmt(s: float) -> String:
	var t := int(s)
	return "%d:%02d" % [t / 60, t % 60]


func _trigger_timeout(loser: String) -> void:
	if tick_sound:
		tick_sound.stop()
	if heartbeat_sound:
		heartbeat_sound.stop()
	is_heartbeat_playing = false
	
	var winner := "Black" if loser == "White" else "White"
	game_over_title.text = "Time's Up!"
	game_over_subtitle.text = loser + " ran out of time. " + winner + " wins!"
	game_over_modal.visible = true
	await get_tree().process_frame
	var vs := get_viewport().get_visible_rect().size
	game_over_modal.position = (vs - game_over_modal.size) / 2.0


# ════════════════════════════════════════════════════════════════════════
#  PROMOTION MODAL
# ════════════════════════════════════════════════════════════════════════
func _build_promotion_modal() -> void:
	promotion_modal = PanelContainer.new()
	var cs := StyleBoxFlat.new()
	cs.bg_color = Color(0.04, 0.04, 0.04, 0.95)
	cs.set_corner_radius_all(15)
	cs.set_border_width_all(2)
	cs.border_color = Color(0.8, 0.9, 1.0, 0.8)
	cs.set_content_margin_all(24)
	promotion_modal.add_theme_stylebox_override("panel", cs)
	promotion_modal.set_anchors_preset(Control.PRESET_CENTER)
	promotion_modal.visible = false
	add_child(promotion_modal)

	var v := VBoxContainer.new()
	v.add_theme_constant_override("separation", 12)
	promotion_modal.add_child(v)

	promotion_title = Label.new()
	promotion_title.text = "Choose Promotion"
	promotion_title.add_theme_font_size_override("font_size", 18)
	promotion_title.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0, 1.0))
	promotion_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(promotion_title)

	var h := HBoxContainer.new()
	h.add_theme_constant_override("separation", 8)
	v.add_child(h)

	for pt in ["queen", "rook", "bishop", "knight"]:
		var btn := Button.new()
		btn.text = PIECE_GLYPH[pt] + "\n" + pt.capitalize()
		btn.custom_minimum_size = Vector2(72, 72)
		btn.add_theme_font_size_override("font_size", 22)
		_style_metallic_button(btn)
		btn.pressed.connect(_on_promotion_choice.bind(pt))
		h.add_child(btn)


func show_promotion_picker(color: String, callback: Callable) -> void:
	promotion_callback = callback
	promotion_title.text = "Choose Promotion (" + color.capitalize() + ")"
	promotion_modal.visible = true
	await get_tree().process_frame
	var vs := get_viewport().get_visible_rect().size
	promotion_modal.position = (vs - promotion_modal.size) / 2.0


func _on_promotion_choice(pt: String) -> void:
	promotion_modal.visible = false
	if promotion_callback.is_valid():
		promotion_callback.call(pt)


# ════════════════════════════════════════════════════════════════════════
#  GAME OVER MODAL
# ════════════════════════════════════════════════════════════════════════
func _build_game_over_modal() -> void:
	game_over_modal = PanelContainer.new()
	var cs := StyleBoxFlat.new()
	cs.bg_color = Color(0.04, 0.04, 0.04, 0.95)
	cs.set_corner_radius_all(15)
	cs.set_border_width_all(2)
	cs.border_color = Color(0.8, 0.9, 1.0, 0.8)
	cs.set_content_margin_all(24)
	game_over_modal.add_theme_stylebox_override("panel", cs)
	
	game_over_modal.set_anchors_preset(Control.PRESET_CENTER)
	game_over_modal.visible = false
	add_child(game_over_modal)

	var v := VBoxContainer.new()
	v.add_theme_constant_override("separation", 20)
	game_over_modal.add_child(v)

	game_over_title = Label.new()
	game_over_title.text = "Checkmate"
	game_over_title.add_theme_font_size_override("font_size", 28)
	game_over_title.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0, 1.0))
	game_over_title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(game_over_title)

	game_over_subtitle = Label.new()
	game_over_subtitle.text = ""
	game_over_subtitle.add_theme_font_size_override("font_size", 16)
	game_over_subtitle.add_theme_color_override("font_color", Color(0.7, 0.75, 0.8, 1.0))
	game_over_subtitle.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	v.add_child(game_over_subtitle)

	var nb := Button.new()
	nb.text = "New Game"
	nb.custom_minimum_size = Vector2(200, 50)
	_style_metallic_button(nb)
	nb.pressed.connect(_on_new_game_pressed)
	v.add_child(nb)


func _on_new_game_pressed() -> void:
	var tree = get_tree()
	if tree:
		tree.reload_current_scene()
	queue_free()


# ════════════════════════════════════════════════════════════════════════
#  SIGNAL CONNECTIONS & HANDLERS
# ════════════════════════════════════════════════════════════════════════
func _connect_signals() -> void:
	game_manager.turn_changed.connect(_on_turn_changed)
	game_manager.piece_captured.connect(_on_piece_captured)
	game_manager.check_started.connect(_on_check_started)
	game_manager.check_cleared.connect(_on_check_cleared)
	game_manager.game_over.connect(_on_game_over)
	game_manager.undo_performed.connect(_on_undo_performed)
	game_manager.move_made.connect(_on_move_made)


func _on_turn_changed(_c: String) -> void:
	_update_turn_indicator()
	_update_undo_button()


func _on_move_made(_n: String) -> void:
	_update_undo_button()
	if piece_sound and piece_sound.stream:
		piece_sound.play()


func _update_turn_indicator() -> void:
	if not game_manager:
		return
	var w := game_manager.is_white_turn()
	turn_label.text = "White to move" if w else "Black to move"
	turn_dot.color = Color.WHITE if w else COL_BLACK_ACCENT


func _on_heartbeat_finished() -> void:
	if is_heartbeat_playing and heartbeat_sound:
		heartbeat_sound.play()

func _on_check_started(c: String) -> void:
	check_label.text = c.capitalize() + " is in check"
	check_label.visible = true
	if heartbeat_sound and not is_heartbeat_playing:
		heartbeat_sound.play()
		is_heartbeat_playing = true


func _on_check_cleared() -> void:
	check_label.visible = false
	if is_heartbeat_playing:
		heartbeat_sound.stop()
		is_heartbeat_playing = false


func _on_piece_captured(piece_type: String, color: String) -> void:
	var g := Label.new()
	g.text = PIECE_GLYPH.get(piece_type, "?")
	g.add_theme_font_size_override("font_size", 18)
	g.add_theme_color_override("font_color", COL_GOLD)
	if color == "black":
		white_captured_tray.add_child(g)
	else:
		black_captured_tray.add_child(g)


func _on_game_over(reason: String) -> void:
	timer_running = false
	if tick_sound:
		tick_sound.stop()
	if is_heartbeat_playing:
		heartbeat_sound.stop()
		is_heartbeat_playing = false
	if reason.findn("stalemate") != -1:
		game_over_title.text = "Stalemate"
		game_over_subtitle.text = "Draw - no legal moves remain."
	else:
		game_over_title.text = "Checkmate!"
		game_over_subtitle.text = reason
	game_over_modal.visible = true
	await get_tree().process_frame
	var vs := get_viewport().get_visible_rect().size
	game_over_modal.position = (vs - game_over_modal.size) / 2.0


func _on_undo_pressed() -> void:
	if game_manager:
		game_manager.undo()


func _on_undo_performed() -> void:
	_rebuild_captured_trays()
	_update_turn_indicator()
	_update_undo_button()
	var in_check := game_manager.is_in_check(game_manager.current_turn_color())
	check_label.visible = in_check
	if in_check:
		check_label.text = game_manager.current_turn_color().capitalize() + " is in check"
		if not is_heartbeat_playing:
			heartbeat_sound.play()
			is_heartbeat_playing = true
	else:
		if is_heartbeat_playing:
			heartbeat_sound.stop()
			is_heartbeat_playing = false
	game_over_modal.visible = false


func _update_undo_button() -> void:
	if game_manager and undo_button:
		undo_button.disabled = not game_manager.can_undo()


func _rebuild_captured_trays() -> void:
	for c in white_captured_tray.get_children():
		c.queue_free()
	for c in black_captured_tray.get_children():
		c.queue_free()
	if not game_manager:
		return
	for e in game_manager.get_captured_pieces():
		_on_piece_captured(_glyph_key(e["type"]), e["color"])


func _glyph_key(t: String) -> String:
	match t:
		"K": return "king"
		"Q": return "queen"
		"R": return "rook"
		"B": return "bishop"
		"N": return "knight"
		"P": return "pawn"
		_: return "pawn"

# ════════════════════════════════════════════════════════════════════════
#  INTRO LOGO
# ════════════════════════════════════════════════════════════════════════
func _build_intro_logo() -> void:
	var tex := load("res://logo/logo.png")
	if tex:
		# 1. Solid black background that hides the 3D world and never fades!
		intro_logo_container = ColorRect.new()
		intro_logo_container.color = Color.BLACK
		intro_logo_container.set_anchors_preset(Control.PRESET_FULL_RECT)
		
		# 2. Container for the logo that WILL fade
		var logo_fader = CenterContainer.new()
		logo_fader.set_anchors_preset(Control.PRESET_FULL_RECT)
		logo_fader.modulate = Color(1, 1, 1, 0) # Start transparent
		
		var margin = MarginContainer.new()
		margin.add_theme_constant_override("margin_bottom", 120) # Pushes the logo up visually
		
		var tex_rect = TextureRect.new()
		tex_rect.texture = tex
		tex_rect.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tex_rect.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		tex_rect.custom_minimum_size = Vector2(800, 800)
		
		var mat = ShaderMaterial.new()
		var shader = load("res://shaders/logo_waves.gdshader")
		if shader:
			mat.shader = shader
		tex_rect.material = mat
		
		margin.add_child(tex_rect)
		logo_fader.add_child(margin)
		intro_logo_container.add_child(logo_fader)
		add_child(intro_logo_container)
		
		# Keep main menu hidden
		mode_select_root.visible = false
		
		tex_rect.pivot_offset = Vector2(400, 400)
		tex_rect.scale = Vector2(0.85, 0.85)
		
		# Create the sequence: Fade In -> Wait -> Fade Out -> Reveal Menu
		var tween = create_tween()
		tween.set_parallel(true)
		
		# Fade in logo and scale up smoothly
		tween.tween_property(logo_fader, "modulate", Color(1, 1, 1, 1), 1.5).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
		tween.tween_property(tex_rect, "scale", Vector2(1.0, 1.0), 3.5).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		
		# Wait
		tween.chain().tween_interval(1.5)
		
		# Fade out the logo itself to black, leaving intro_logo_container solid black behind it
		tween.chain().set_parallel(true)
		tween.tween_property(logo_fader, "modulate", Color(1, 1, 1, 0), 1.5).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
		tween.tween_property(tex_rect, "scale", Vector2(1.05, 1.05), 1.5).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		
		tween.chain().tween_callback(_on_intro_faded)
	else:
		mode_select_root.visible = true
		if intro_sound and intro_sound.stream:
			intro_sound.play()

func _on_intro_faded() -> void:
	# Show the main menu ONLY after the fade out is complete
	mode_select_root.visible = true
	if intro_sound and intro_sound.stream:
		intro_sound.play()
	if intro_logo_container:
		intro_logo_container.queue_free()
		intro_logo_container = null
