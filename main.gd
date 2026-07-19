extends ChessBoard

# ─── Board geometry — MUST match chess_constants.h exactly ────────────
const PLAYABLE_AREA := 0.49866
const SQUARE_SIZE   := PLAYABLE_AREA / 8.0
const BOARD_HALF    := PLAYABLE_AREA / 2.0
const CELL_HALF     := SQUARE_SIZE  / 2.0
const BOARD_SURFACE_Y := 0.000206

var game_manager: GameManager
var ui: ChessUI
var hover_mesh: MeshInstance3D
var check_mesh: MeshInstance3D
var bg_lights_node: Node3D

@export_enum("standard", "advanced") var game_mode: String = "advanced"
var is_vs_ai: bool = false
var ai_difficulty: int = 1

var is_free_camera: bool = false
var cam_original_pos: Vector3
var cam_original_rot: Vector3
var orbit_yaw: float = 0.0
var orbit_pitch: float = 40.0
var orbit_radius: float = 0.6

# STEP 1 FIX: Instantiates the GameManager BEFORE children nodes run their _ready()
func _enter_tree() -> void:
	game_manager = GameManager.new()
	game_manager.name = "GameManager"
	add_child(game_manager)

var camera_pivot: Node3D

func _ready() -> void:
	_setup_lighting()
	_setup_camera_pivot()
	# STEP 2 FIX: Automatically builds all invisible click colliders for your manual pieces
	_auto_setup_piece_collisions()
	
	var advanced_board = get_node_or_null("new_board")
	if advanced_board:
		_disable_picking(advanced_board)
	
	_apply_board_mode()
	_setup_ui()
	_connect_signals()
	_setup_hover_mesh()
	_setup_check_mesh()

func _disable_picking(node: Node) -> void:
	if node is CollisionObject3D:
		node.input_ray_pickable = false
	for child in node.get_children():
		_disable_picking(child)

func set_game_mode(mode: String) -> void:
	game_mode = mode
	_apply_board_mode()

func _apply_board_mode() -> void:
	var advanced_board = get_node_or_null("new_board")
	if game_mode == "advanced":
		if advanced_board:
			advanced_board.show()
		for child in get_children():
			if _is_standard_board_mesh(child, advanced_board):
				child.hide()
	else:
		if advanced_board:
			advanced_board.hide()
		var showed_any_standard := false
		for child in get_children():
			if _is_standard_board_mesh(child, advanced_board):
				child.show()
				showed_any_standard = true
		if not showed_any_standard and advanced_board:
			# Fallback: if there is no separate standard board mesh, keep the advanced board visible.
			advanced_board.show()

func _is_standard_board_mesh(node: Node, advanced_board: Node) -> bool:
	if node == advanced_board or node is Camera3D or node is Light3D or node is WorldEnvironment or node == bg_lights_node or node == check_mesh: return false
	if node.name == "Area3D" or node.name == "ChessUI" or node.name == "GameManager" or node.name == "CameraPivot" or node == hover_mesh: return false
	if node is ChessPiece: return false
	return true

# Iterates through scene elements to create click bounding-boxes instantly
func _auto_setup_piece_collisions() -> void:
	for child in get_children():
		if child is ChessPiece:
			if child.has_node("Area3D"): continue
			
			var area := Area3D.new()
			area.name = "Area3D"
			child.add_child(area)

			var col_shape := CollisionShape3D.new()
			var capsule := CapsuleShape3D.new()

			if child is ChessKing:
				capsule.radius = 0.022
				capsule.height = 0.095
				col_shape.position.y = 0.045
			elif child is ChessQueen:
				capsule.radius = 0.021
				capsule.height = 0.085
				col_shape.position.y = 0.040
			elif child is ChessBishop:
				capsule.radius = 0.018
				capsule.height = 0.070
				col_shape.position.y = 0.033
			elif child is ChessKnight:
				capsule.radius = 0.018
				capsule.height = 0.060
				col_shape.position.y = 0.028
			elif child is ChessRook:
				capsule.radius = 0.020
				capsule.height = 0.055
				col_shape.position.y = 0.025
			elif child is ChessPawn:
				capsule.radius = 0.015
				capsule.height = 0.050
				col_shape.position.y = 0.023

			col_shape.shape = capsule
			area.add_child(col_shape)
			area.input_event.connect(child.on_click)

# ─── Board click handler ──────────────────────────────────────────────
func _on_glass_shield_clicked(_camera, event, click_pos, _normal, _shape_idx):
	if is_free_camera: return
	if is_vs_ai and game_manager.current_turn_color() == "black":
		return
		
	if event is InputEventMouseMotion:
		var local_pos = to_local(click_pos)
		var col := int(floor((local_pos.x + BOARD_HALF) / SQUARE_SIZE))
		var row := int(floor((local_pos.z + BOARD_HALF) / SQUARE_SIZE))
		if col >= 0 and col < 8 and row >= 0 and row < 8:
			hover_mesh.visible = true
			var pos_x := -BOARD_HALF + col * SQUARE_SIZE + CELL_HALF
			var pos_z := -BOARD_HALF + row * SQUARE_SIZE + CELL_HALF
			hover_mesh.position.x = pos_x
			hover_mesh.position.z = pos_z
		else:
			hover_mesh.visible = false

	var mouse_event = event as InputEventMouseButton
	if mouse_event and mouse_event.pressed and mouse_event.button_index == MOUSE_BUTTON_LEFT:
		var local_pos = to_local(click_pos)
		var col := int(floor((local_pos.x + BOARD_HALF) / SQUARE_SIZE))
		var row := int(floor((local_pos.z + BOARD_HALF) / SQUARE_SIZE))
		col = clampi(col, 0, 7)
		row = clampi(row, 0, 7)
		game_manager.handle_square_click(col, row)

# ─── UI setup ─────────────────────────────────────────────────────────
func _setup_ui() -> void:
	if ui: ui.queue_free()
	ui = ChessUI.new()
	ui.name = "ChessUI"
	ui.game_manager = game_manager
	ui.board_root = self
	ui.camera_toggled.connect(_on_camera_toggled)
	get_viewport().call_deferred("add_child", ui)

func _connect_signals() -> void:
	game_manager.promotion_needed.connect(_on_promotion_needed)
	game_manager.turn_changed.connect(_on_turn_changed)
	game_manager.check_started.connect(_on_check_started)
	game_manager.check_cleared.connect(_on_check_cleared)
	game_manager.undo_performed.connect(_on_undo_performed)

func _on_check_started(color: String) -> void:
	for child in get_children():
		if child is ChessKing and child.get_piece_color() == color:
			var col = child.get_col()
			var row = child.get_row()
			if col != -1 and row != -1:
				var pos_x = -BOARD_HALF + col * SQUARE_SIZE + CELL_HALF
				var pos_z = -BOARD_HALF + row * SQUARE_SIZE + CELL_HALF
				check_mesh.position.x = pos_x
				check_mesh.position.z = pos_z
				check_mesh.visible = true
			break

func _on_check_cleared() -> void:
	if check_mesh: check_mesh.visible = false

func _on_undo_performed() -> void:
	var current_color = game_manager.current_turn_color()
	if game_manager.is_in_check(current_color):
		_on_check_started(current_color)
	else:
		_on_check_cleared()

func _setup_camera_pivot() -> void:
	var camera = get_node_or_null("Camera3D")
	if not camera: return
	
	# Store the original transform relative to the board
	var orig_trans = camera.transform
	
	camera_pivot = Node3D.new()
	camera_pivot.name = "CameraPivot"
	add_child(camera_pivot)
	
	remove_child(camera)
	camera_pivot.add_child(camera)
	
	# Restore the local transform
	camera.transform = orig_trans
	
	# Rotate 180 degrees initially because the default camera position is on Black's side
	camera_pivot.rotation.y = PI

func _on_turn_changed(color: String) -> void:
	if not camera_pivot: return
	# White is at PI (rotated to -Z), Black is at 0 (original +Z)
	var target_y = PI if color == "white" else 0.0
	if is_vs_ai:
		target_y = PI # Always keep camera on White side if vs AI
		
	var tween = create_tween()
	tween.set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	tween.tween_property(camera_pivot, "rotation:y", target_y, 0.7)

	if is_vs_ai and color == "black" and not game_manager.is_game_over():
		# Add a small delay for AI to think
		get_tree().create_timer(0.5).timeout.connect(_make_ai_move)

func _get_piece_value(piece_type: String) -> int:
	match piece_type:
		"queen": return 900
		"rook": return 500
		"bishop": return 330
		"knight": return 320
		"pawn": return 100
	return 0

func _get_piece_at(col: int, row: int) -> ChessPiece:
	for child in get_children():
		if child is ChessPiece and child.get_col() == col and child.get_row() == row and abs(child.position.x) < 0.28:
			return child
	return null

func _make_ai_move() -> void:
	if game_manager.is_game_over() or game_manager.current_turn_color() != "black":
		return

	var black_pieces = []
	for child in get_children():
		if child is ChessPiece and child.get_piece_color() == "black" and child.get_col() != -1 and abs(child.position.x) < 0.28:
			black_pieces.append(child)
			
	var moves = []
	for piece in black_pieces:
		var p_col = piece.get_col()
		var p_row = piece.get_row()
		for c in range(8):
			for r in range(8):
				if p_col == c and p_row == r: continue
				
				var score = 0.0
				
				if ai_difficulty == 0:
					# Easy: Purely random
					score = randf()
				else:
					# Medium and Hard: Prioritize captures
					var target_piece = _get_piece_at(c, r)
					var is_capture = target_piece != null and target_piece.get_piece_color() == "white"
					if is_capture:
						score += _get_piece_value(target_piece.get_type_name())
					
					if ai_difficulty >= 2:
						# Hard: Positional heuristics
						var p_type = piece.get_type_name()
						var center_dist = abs(c - 3.5) + abs(r - 3.5)
						if p_type == "knight":
							score += (7.0 - center_dist) * 5.0
						elif p_type == "pawn":
							score += (6.0 - r) * 10.0 # Pawns start at 6, move towards 0
						elif p_type == "bishop":
							score += (7.0 - center_dist) * 3.0
							
						if p_type == "queen" and r > 4:
							score -= 20.0
						if p_type == "king" and r > 5:
							score += 10.0 
						
						score += randf() * 10.0 # Add minor noise to vary play
					elif ai_difficulty == 1:
						# Medium: Add random noise so it doesn't always play the exact same non-captures
						score += randf() * 50.0
				
				moves.append({
					"piece": piece,
					"col": c,
					"row": r,
					"score": score
				})

	moves.sort_custom(func(a, b): return a.score > b.score)

	var prev_highlights = game_manager.get_highlights_enabled()
	game_manager.set_highlights_enabled(false)

	for move in moves:
		var piece = move.piece
		game_manager.deselect()
		game_manager.handle_square_click(piece.get_col(), piece.get_row()) # Select
		game_manager.handle_square_click(move.col, move.row) # Move
		
		if game_manager.current_turn_color() == "white":
			game_manager.set_highlights_enabled(prev_highlights)
			return # Successfully moved and turn ended
			
	game_manager.set_highlights_enabled(prev_highlights)

func _on_promotion_needed(col: int, row: int, color: String) -> void:
	if is_vs_ai and color == "black":
		_finish_promotion(col, row, color, "queen")
		return
		
	if ui:
		ui.show_promotion_picker(color, func(piece_type: String):
			_finish_promotion(col, row, color, piece_type)
		)

func _finish_promotion(col: int, row: int, color: String, piece_type: String) -> void:
	for child in get_children():
		if child is ChessPiece and child.get_col() == col and child.get_row() == row:
			# Fix: Do NOT queue_free() the pawn. GameManager relies on the node existing 
			# to be able to restore it if the player hits Undo. Park it off-board.
			child.visible = false
			child.process_mode = Node.PROCESS_MODE_DISABLED
			child.position.y = -2.0
			break
			
	var piece_node: ChessPiece
	match piece_type:
		"queen":  piece_node = ChessQueen.new()
		"rook":   piece_node = ChessRook.new()
		"bishop": piece_node = ChessBishop.new()
		"knight": piece_node = ChessKnight.new()

	piece_node.piece_color = color
	piece_node.name = "%s_%s_%d_%d" % [color, piece_type, col, row]
	var pos_x := -BOARD_HALF + col * SQUARE_SIZE + CELL_HALF
	var pos_z := -BOARD_HALF + row * SQUARE_SIZE + CELL_HALF
	piece_node.position = Vector3(pos_x, BOARD_SURFACE_Y, pos_z)
	add_child(piece_node)

	var MODELS := {
		"white_queen": "res://models/white_queen.glb", "white_rook": "res://models/white_rook.glb",
		"white_bishop": "res://models/white_bishop.glb", "white_knight": "res://models/white_knight.glb",
		"black_queen": "res://models/black_queen.glb", "black_rook": "res://models/black_rook.glb",
		"black_bishop": "res://models/black_bishop.glb", "black_knight": "res://models/black_knight.glb"
	}
	var model_scene: PackedScene = load(MODELS["%s_%s" % [color, piece_type]])
	var model_instance = model_scene.instantiate()
	model_instance.scale = Vector3(0.8, 0.8, 0.8)
	piece_node.add_child(model_instance)
	_attach_single_collision(piece_node, piece_type)
	
	await get_tree().process_frame
	game_manager.complete_promotion(col, row, piece_node)

func _attach_single_collision(piece_node: ChessPiece, piece_type: String) -> void:
	var area := Area3D.new()
	area.name = "Area3D"
	piece_node.add_child(area)
	var col_shape := CollisionShape3D.new()
	var capsule := CapsuleShape3D.new()
	match piece_type:
		"queen": capsule.radius = 0.021; capsule.height = 0.085; col_shape.position.y = 0.040
		"rook": capsule.radius = 0.020; capsule.height = 0.055; col_shape.position.y = 0.025
		"bishop": capsule.radius = 0.018; capsule.height = 0.070; col_shape.position.y = 0.033
		"knight": capsule.radius = 0.018; capsule.height = 0.060; col_shape.position.y = 0.028
	col_shape.shape = capsule
	area.add_child(col_shape)
	area.input_event.connect(piece_node.on_click)

func _setup_lighting() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0, 0, 0)
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.8, 0.8, 0.85)
	env.ambient_light_energy = 0.15 # Balanced ambient light for the board
	
	env.tonemap_mode = Environment.TONE_MAPPER_ACES
	
	# Enable volumetric fog for light shafts
	env.volumetric_fog_enabled = true
	env.volumetric_fog_density = 0.02 # Very low intensity to prevent bleeding
	env.volumetric_fog_albedo = Color(0.95, 0.95, 1.0)
	env.volumetric_fog_emission = Color(0.0, 0.0, 0.0) # High contrast
	
	var we := WorldEnvironment.new()
	we.environment = env
	we.name = "GameWorldEnvironment"
	add_child(we)
	
	var fill_light := DirectionalLight3D.new()
	fill_light.rotation_degrees = Vector3(-45, 180, 0)
	fill_light.light_energy = 0.2 # Balanced fill light
	fill_light.light_color = Color(0.9, 0.9, 1.0)
	fill_light.shadow_enabled = false
	fill_light.name = "FillLight"
	add_child(fill_light)
	
	var main_light = get_node_or_null("DirectionalLight3D")
	if main_light:
		main_light.light_energy = 0.6 # Balanced main light for the board
		main_light.shadow_enabled = true
		main_light.shadow_bias = 0.02
		main_light.shadow_blur = 1.5
		main_light.light_volumetric_fog_energy = 0.0 # prevent main light from washing out shafts
	# Add Volumetric Spotlights node
	bg_lights_node = Node3D.new()
	add_child(bg_lights_node)
	
	# 5 highly separated spotlights with narrow beams
	var spot_positions = [
		Vector3(-1.2, 1.6, -1.5),
		Vector3(-0.6, 1.5, -1.5),
		Vector3(0.0, 1.4, -1.5),
		Vector3(0.6, 1.5, -1.5),
		Vector3(1.2, 1.6, -1.5)
	]
	
	for i in range(spot_positions.size()):
		var spot := SpotLight3D.new()
		spot.position = spot_positions[i]
		spot.rotation_degrees = Vector3(-90, 0, 0) # straight down
		spot.spot_range = 4.0
		spot.spot_angle = 6.0 # Very narrow so they stay completely separate
		spot.light_energy = 20.0 # Higher energy since the beam is much thinner
		spot.light_color = Color(0.92, 0.95, 1.0)
		spot.light_volumetric_fog_energy = 4.0 # Lower fog multiplier to prevent bleeding/blooming
		spot.shadow_enabled = true
		spot.shadow_blur = 1.0 # Less shadow blur to keep beams defined
		spot.name = "VolumetricSpot_" + str(i)
		bg_lights_node.add_child(spot)

func _setup_hover_mesh() -> void:
	hover_mesh = MeshInstance3D.new()
	var plane = PlaneMesh.new()
	plane.size = Vector2(SQUARE_SIZE, SQUARE_SIZE)
	hover_mesh.mesh = plane
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(1.0, 1.0, 0.0, 0.25) # Transparent yellow
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	hover_mesh.material_override = mat
	hover_mesh.position.y = BOARD_SURFACE_Y + 0.002
	hover_mesh.visible = false
	add_child(hover_mesh)
	
	var area = get_node_or_null("Area3D")
	if area:
		area.mouse_exited.connect(func(): hover_mesh.visible = false)

func _setup_check_mesh() -> void:
	check_mesh = MeshInstance3D.new()
	var plane = PlaneMesh.new()
	plane.size = Vector2(SQUARE_SIZE, SQUARE_SIZE)
	check_mesh.mesh = plane
	var mat = StandardMaterial3D.new()
	mat.albedo_color = Color(1.0, 0.0, 0.0, 0.5) # Semi-transparent red
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	check_mesh.material_override = mat
	check_mesh.position.y = BOARD_SURFACE_Y + 0.001
	check_mesh.visible = false
	add_child(check_mesh)

func _process(delta: float) -> void:
	if bg_lights_node:
		var cam = get_viewport().get_camera_3d()
		if cam:
			var cam_pos = cam.global_position
			bg_lights_node.rotation.y = atan2(cam_pos.x, cam_pos.z)

	for child in get_children():
		if child is ChessPiece:
			var inner_model: Node3D = null
			for sub in child.get_children():
				if sub is MeshInstance3D and sub.is_set_as_top_level():
					if is_equal_approx(sub.scale.x, 1.0):
						sub.global_position = Vector3(sub.global_position.x * 1.25, sub.global_position.y, sub.global_position.z * 1.25)
						sub.scale = Vector3(1.25, 1.0, 1.25)
				elif sub is Node3D and not sub is Area3D:
					inner_model = sub

			if inner_model:
				if abs(child.position.x) > 0.28:
					inner_model.position.y = 0.015
				else:
					inner_model.position.y = 0.0

func _on_camera_toggled(is_active: bool) -> void:
	is_free_camera = is_active
	var cam = get_viewport().get_camera_3d()
	if not cam: return
	
	if is_free_camera:
		cam_original_pos = cam.position
		cam_original_rot = cam.rotation
		get_viewport().physics_object_picking = false
		orbit_yaw = 0.0
		orbit_pitch = 40.0
		orbit_radius = 0.6
		_update_orbital_camera()
	else:
		cam.position = cam_original_pos
		cam.rotation = cam_original_rot
		get_viewport().physics_object_picking = true

func _update_orbital_camera() -> void:
	var cam = get_viewport().get_camera_3d()
	if not cam: return
	var pitch_rad = deg_to_rad(orbit_pitch)
	var yaw_rad = deg_to_rad(orbit_yaw)
	var cy = orbit_radius * sin(pitch_rad)
	var xz_len = orbit_radius * cos(pitch_rad)
	var cx = xz_len * sin(yaw_rad)
	var cz = xz_len * cos(yaw_rad)
	cam.position = Vector3(cx, cy, cz)
	cam.look_at(Vector3.ZERO, Vector3.UP)

func _unhandled_input(event: InputEvent) -> void:
	if not is_free_camera: return
	
	if event is InputEventMouseMotion:
		if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) or Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT) or Input.is_mouse_button_pressed(MOUSE_BUTTON_MIDDLE):
			orbit_yaw -= event.relative.x * 0.5
			orbit_pitch += event.relative.y * 0.5
			orbit_pitch = clamp(orbit_pitch, 5.0, 85.0)
			_update_orbital_camera()
	elif event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			orbit_radius = clamp(orbit_radius - 0.05, 0.2, 1.5)
			_update_orbital_camera()
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			orbit_radius = clamp(orbit_radius + 0.05, 0.2, 1.5)
			_update_orbital_camera()
