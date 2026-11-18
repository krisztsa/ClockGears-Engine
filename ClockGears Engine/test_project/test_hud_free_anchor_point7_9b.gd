extends SceneTree

# Punto 7: Modo "Libre (desanclado)" en el HUD.
# Verifica botón "Libre (desanclado)", arrastre en vista previa sin salirse de 960x540,
# edición manual por X/Y, persistencia al guardar y reabrir, y layout runtime 16:9.

var passed := 0
var failed := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  OK: " + msg)
	else:
		failed += 1
		printerr("  FAIL: " + msg)

func _step_frames(n: int = 3) -> void:
	for _i in range(n):
		await process_frame

func _snapshot_data_dir(res_data_dir: String) -> Dictionary:
	var backup := {}
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				backup[full] = FileAccess.get_file_as_string(full)
			fname = da.get_next()
		da.list_dir_end()
	return backup

func _restore_data_dir(res_data_dir: String, backup: Dictionary) -> void:
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				if not backup.has(full):
					DirAccess.remove_absolute(ProjectSettings.globalize_path(full))
			fname = da.get_next()
		da.list_dir_end()
	for full in backup.keys():
		var wf := FileAccess.open(full, FileAccess.WRITE)
		if wf:
			wf.store_string(backup[full])

func _init() -> void:
	call_deferred("_run_tests")

func _run_tests() -> void:
	print("=== TEST PUNTO 7: WIDGETS DEL HUD EN MODO LIBRE (DESANCLADO) ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager disponible")
	if dm == null:
		quit(1)
		return

	var res_data_dir := "res://clockgears_data"
	var backup := _snapshot_data_dir(res_data_dir)
	await _step_frames(1)

	# Widget inicial con anclaje top_left en (24, 20), tamaño 160x36
	var w0: Dictionary = ClockGearsDataManager.make_default_hud_widget("bar")
	w0["id"] = "barra_libre_test"
	w0["name"] = "BarraLibreTest"
	w0["anchor"] = "top_left"
	w0["offset_x"] = 24
	w0["offset_y"] = 20
	w0["width"] = 160
	w0["height"] = 36
	dm.set_hud_config({"schema_version": 1, "widgets": [w0]})
	dm.save_all(res_data_dir)

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(2)
	db.select_category_by_key("hud")
	await _step_frames(3)
	db.select_hud_widget(0)
	await _step_frames(2)

	var h: Dictionary = db.get_hud_editor_ui_handles()
	var free_btn: Button = h.get("free_anchor_btn", null)
	var ox_spin: SpinBox = h.get("offset_x_spin", null)
	var oy_spin: SpinBox = h.get("offset_y_spin", null)
	var drag_ov: Control = h.get("preview_drag_overlay", null)
	var live_prev = h.get("live_preview", null)

	_check(free_btn != null and ox_spin != null and oy_spin != null and drag_ov != null and live_prev != null, "Controles de modo libre y overlay de arrastre presentes")
	if free_btn == null or drag_ov == null or live_prev == null:
		_restore_data_dir(res_data_dir, backup)
		quit(1)
		return

	# 1. Pulsar "Libre (desanclado)" y verificar que conserva posición
	free_btn.emit_signal("pressed")
	await _step_frames(2)
	var draft: Dictionary = db.get_hud_draft_copy()
	var w_cur: Dictionary = draft["widgets"][0]
	_check(String(w_cur.get("anchor", "")) == "free", "Botón Libre cambia anchor a 'free'")
	_check(int(w_cur.get("offset_x", -1)) == 24 and int(w_cur.get("offset_y", -1)) == 20, "Al pasar a Libre conserva posición visible (24, 20)")

	# 2. Simular arrastre en vista previa
	var ov_sz := drag_ov.size
	if ov_sz.x < 10.0 or ov_sz.y < 10.0:
		drag_ov.size = Vector2(480, 270)
		ov_sz = drag_ov.size
	var scale_to_ov := Vector2(ov_sz.x / 960.0, ov_sz.y / 540.0)
	var click_pt := Vector2(24.0 + 40.0, 20.0 + 18.0) * scale_to_ov

	var mb_down := InputEventMouseButton.new()
	mb_down.button_index = MOUSE_BUTTON_LEFT
	mb_down.pressed = true
	mb_down.position = click_pt
	drag_ov.emit_signal("gui_input", mb_down)
	await _step_frames(1)

	var move_delta := Vector2(200.0, 150.0) * scale_to_ov
	var mm := InputEventMouseMotion.new()
	mm.position = click_pt + move_delta
	drag_ov.emit_signal("gui_input", mm)
	await _step_frames(1)

	var mb_up := InputEventMouseButton.new()
	mb_up.button_index = MOUSE_BUTTON_LEFT
	mb_up.pressed = false
	mb_up.position = click_pt + move_delta
	drag_ov.emit_signal("gui_input", mb_up)
	await _step_frames(1)

	draft = db.get_hud_draft_copy()
	w_cur = draft["widgets"][0]
	_check(absi(int(w_cur["offset_x"]) - 224) <= 2 and absi(int(w_cur["offset_y"]) - 170) <= 2,
		"Arrastre actualiza offset_x/y enteros (~224, ~170), obtenido: (%s, %s)" % [w_cur["offset_x"], w_cur["offset_y"]])
	_check(int(ox_spin.value) == int(w_cur["offset_x"]) and int(oy_spin.value) == int(w_cur["offset_y"]),
		"SpinBoxes X e Y reflejan la posición arrastrada")

	# 3. Arrastre extremo fuera de la pantalla se recorta dentro de [0, 960-w] x [0, 540-h]
	var click_pt2 := Vector2(float(w_cur["offset_x"]) + 20.0, float(w_cur["offset_y"]) + 10.0) * scale_to_ov
	var mb_down2 := InputEventMouseButton.new()
	mb_down2.button_index = MOUSE_BUTTON_LEFT
	mb_down2.pressed = true
	mb_down2.position = click_pt2
	drag_ov.emit_signal("gui_input", mb_down2)
	var mm_far := InputEventMouseMotion.new()
	mm_far.position = click_pt2 + Vector2(2000.0, 2000.0) * scale_to_ov
	drag_ov.emit_signal("gui_input", mm_far)
	var mb_up2 := InputEventMouseButton.new()
	mb_up2.button_index = MOUSE_BUTTON_LEFT
	mb_up2.pressed = false
	mb_up2.position = mm_far.position
	drag_ov.emit_signal("gui_input", mb_up2)
	await _step_frames(1)

	draft = db.get_hud_draft_copy()
	w_cur = draft["widgets"][0]
	_check(int(w_cur["offset_x"]) == 960 - 160 and int(w_cur["offset_y"]) == 540 - 36,
		"Arrastre más allá del borde inferior-derecho se recorta en (%d, %d), obtenido: (%s, %s)" % [960 - 160, 540 - 36, w_cur["offset_x"], w_cur["offset_y"]])

	# 4. Editar mediante campos X e Y
	ox_spin.value = 310
	ox_spin.emit_signal("value_changed", 310.0)
	oy_spin.value = 140
	oy_spin.emit_signal("value_changed", 140.0)
	await _step_frames(1)
	var st_prev: Dictionary = live_prev.get_widget_state("barra_libre_test")
	var br: Rect2 = st_prev.get("base_rect", Rect2())
	_check(absf(br.position.x - 310.0) < 0.5 and absf(br.position.y - 140.0) < 0.5,
		"Edición por X/Y coloca vista previa en (310, 140), obtenido: %s" % br.position)

	# 5. Guardar, cerrar y reabrir BD -> conserva anchor=free y (310, 140)
	db.save_to_disk()
	db.queue_free()
	await _step_frames(2)

	dm.load_all(res_data_dir)
	var db2 := ClockGearsDatabaseDialog.new()
	root.add_child(db2)
	await _step_frames(2)
	db2.open_database()
	await _step_frames(2)
	db2.select_category_by_key("hud")
	await _step_frames(2)
	db2.select_hud_widget(0)
	await _step_frames(1)

	var draft2: Dictionary = db2.get_hud_draft_copy()
	var w2: Dictionary = draft2["widgets"][0]
	_check(String(w2.get("anchor", "")) == "free" and int(w2.get("offset_x", -1)) == 310 and int(w2.get("offset_y", -1)) == 140,
		"Al guardar y reabrir conserva anchor='free' y posición (310, 140)")

	# 6. Runtime HUD en varias resoluciones 16:9 sin recortes ni salir de pantalla
	var game_hud := ClockGearsHudOverlay.new()
	root.add_child(game_hud)
	game_hud.apply_hud_config(dm.get_hud_config())
	for res in [Vector2(960, 540), Vector2(1366, 768), Vector2(1920, 1080)]:
		game_hud.set_viewport_size_override(res)
		var bounds: Dictionary = game_hud.get_layout_bounds_info()
		var clipped: Array = bounds.get("clipped_widgets", [])
		_check(clipped.is_empty(), "Runtime HUD sin widgets recortados en %dx%d" % [res.x, res.y])

	game_hud.queue_free()
	db2.queue_free()
	await _step_frames(1)
	_restore_data_dir(res_data_dir, backup)
	dm.load_all(res_data_dir)

	if failed == 0:
		print("ALL PASS: Modo libre (desanclado) de widgets del HUD verificado (%d checks)." % passed)
		quit(0)
	else:
		print("FAIL: %d comprobaciones fallaron." % failed)
		quit(1)
