extends SceneTree

# Punto 4 (9B-2): Fundido de pantalla Oscurecer y Aclarar en 960x540, 1366x768 y 1920x1080,
# por encima de mundo y HUD (capa 120), cubriendo todo el viewport, y sobreviviendo a Transferir Jugador.

var passed := 0
var failed := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  OK: " + msg)
	else:
		failed += 1
		printerr("  FAIL: " + msg)

func _step_frames(n: int = 1) -> void:
	for _i in range(n):
		await physics_frame

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 4 (9B-2): FUNDIDO DE PANTALLA Y TRANSFERENCIA ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.reset_runtime_state()

	for res in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080)]:
		root.content_scale_mode = Window.CONTENT_SCALE_MODE_CANVAS_ITEMS
		root.content_scale_aspect = Window.CONTENT_SCALE_ASPECT_EXPAND
		root.content_scale_size = res
		root.size = res
		var m := ClockGearsMap.new()
		m.set_map_size(Vector2i(8, 8))
		root.add_child(m)
		m.ensure_default_layers()
		await _step_frames(2)

		var interp := m.get_runtime_interpreter()
		var hud := interp.get_hud_overlay()
		var hud_layer: CanvasLayer = interp.get_hud_layer()
		var ui_layer: CanvasLayer = interp.get_ui_layer()

		# Oscurecer 20 frames -> Aclarar 20 frames
		interp.start_command_list([
			{"type": "fade", "mode": "Oscurecer", "duration": 20},
			{"type": "fade", "mode": "Aclarar", "duration": 20}
		])

		var st0: Dictionary = interp.get_fade_state()
		_check(int(st0["layer"]) > hud_layer.layer and int(st0["layer"]) > ui_layer.layer,
			"%dx%d capa fundido (%d) encima de HUD (%d) y UI (%d)" % [res.x, res.y, st0["layer"], hud_layer.layer, ui_layer.layer])
		_check(absf(float(st0["alpha"]) - 0.0) < 0.01, "%dx%d inicio Oscurecer alpha=0.0 (%.3f)" % [res.x, res.y, st0["alpha"]])

		await _step_frames(10)
		var st_mid_out: Dictionary = interp.get_fade_state()
		_check(bool(st_mid_out["visible"]) and float(st_mid_out["alpha"]) > 0.35 and float(st_mid_out["alpha"]) < 0.65,
			"%dx%d mitad Oscurecer progresivo visible=true alpha=%.3f" % [res.x, res.y, st_mid_out["alpha"]])
		var r_mid: Rect2 = st_mid_out["rect"]
		var vp_mid: Rect2 = st_mid_out["viewport_rect"]
		_check(r_mid.size.x >= vp_mid.size.x - 1.0 and r_mid.size.y >= vp_mid.size.y - 1.0,
			"%dx%d rect fundido %s cubre viewport %s" % [res.x, res.y, r_mid, vp_mid])

		await _step_frames(10)
		var st_end_out: Dictionary = interp.get_fade_state()
		_check(bool(st_end_out["visible"]) and absf(float(st_end_out["alpha"]) - 1.0) < 0.01,
			"%dx%d fin Oscurecer negro total alpha=%.3f" % [res.x, res.y, st_end_out["alpha"]])

		await _step_frames(10)
		var st_mid_in: Dictionary = interp.get_fade_state()
		_check(float(st_mid_in["alpha"]) > 0.35 and float(st_mid_in["alpha"]) < 0.65,
			"%dx%d mitad Aclarar progresivo alpha=%.3f" % [res.x, res.y, st_mid_in["alpha"]])

		await _step_frames(11)
		var st_end_in: Dictionary = interp.get_fade_state()
		_check(absf(float(st_end_in["alpha"]) - 0.0) < 0.01 and not interp.is_running(),
			"%dx%d fin Aclarar devuelto a alpha=%.3f" % [res.x, res.y, st_end_in["alpha"]])

		m.queue_free()
		dm.reset_runtime_state()
		await _step_frames(1)

	# Prueba de transferencia entre dos escenas .tscn con Oscurecer -> Transferir -> Aclarar
	var tmp_map_path := "res://_test_fade_map_dest_9b2.tscn"
	var m_dest := ClockGearsMap.new()
	m_dest.set_map_size(Vector2i(10, 10))
	var ps := PackedScene.new()
	ps.pack(m_dest)
	ResourceSaver.save(ps, tmp_map_path)
	m_dest.free()

	var m_src := ClockGearsMap.new()
	m_src.set_map_size(Vector2i(8, 8))
	root.add_child(m_src)
	current_scene = m_src
	m_src.ensure_default_layers()
	await _step_frames(2)

	var interp_src := m_src.get_runtime_interpreter()
	interp_src.start_command_list([
		{"type": "fade", "mode": "Oscurecer", "duration": 10},
		{"type": "teleport", "map": tmp_map_path, "x": 4, "y": 5, "dir": "Arriba"},
		{"type": "fade", "mode": "Aclarar", "duration": 10}
	])

	await _step_frames(10)
	# Justo tras completar Oscurecer y disparar change_scene_to_file, el fade persistente debe seguir a alpha=1.0 sin parpadeo
	var alpha_during_transfer: float = dm.get_runtime_fade_alpha()
	_check(alpha_during_transfer >= 0.99, "Negro sobrevive al cambio de mapa (alpha=%.3f >= 0.99)" % alpha_during_transfer)

	# Esperar a que cargue m_dest y ejecute Aclarar(10)
	await _step_frames(16)
	var alpha_after_transfer: float = dm.get_runtime_fade_alpha()
	_check(alpha_after_transfer <= 0.01, "Aclarar post-transferencia devuelve la pantalla a alpha=%.3f" % alpha_after_transfer)

	if current_scene:
		current_scene.queue_free()
	DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_map_path))
	dm.reset_runtime_state()
	await _step_frames(1)

	if failed == 0:
		print("ALL PASS: Fundido de pantalla Oscurecer/Aclarar y transferencia verificados (%d checks)." % passed)
		quit(0)
	else:
		print("FAIL: %d comprobaciones fallaron." % failed)
		quit(1)
