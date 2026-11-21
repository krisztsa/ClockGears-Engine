extends SceneTree

var pass_count: int = 0
var fail_count: int = 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + msg)
	else:
		fail_count += 1
		printerr("[FAIL] " + msg)

func _finish(code: int = -1) -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(code if code >= 0 else (0 if fail_count == 0 else 1))

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "timeout_60s")
		_finish(1)
	)
	call_deferred("_run")

func _wait_f(n: int) -> void:
	for i in range(n):
		await process_frame

func _run() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "DataManager singleton disponible")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	await _wait_f(3)
	db.select_category_by_key("title")
	await _wait_f(3)

	var ed := db.get_ui_designer_editor()
	_check(ed != null, "Editor UI disponible")
	if ed == null:
		db.queue_free()
		_finish(1)
		return

	var h: Dictionary = ed.get_ui_handles()
	var z_fit: Button = h.get("zoom_1_btn", null) as Button
	var z_50: Button = h.get("zoom_50_btn", null) as Button
	var z_100: Button = h.get("zoom_2_btn", null) as Button
	var z_200: Button = h.get("zoom_3_btn", null) as Button
	var z_300: Button = h.get("zoom_300_btn", null) as Button
	var cscroll: ScrollContainer = h.get("canvas_scroll", null) as ScrollContainer
	var overlay: Control = h.get("canvas_overlay", null) as Control
	var stage: Control = ed.get_canvas_stage()

	_check(z_fit != null and z_fit.text == "Ajustar", "Botón Ajustar existe")
	_check(z_50 != null and z_50.text == "50 %", "Botón 50 % existe")
	_check(z_100 != null and z_100.text == "100 %", "Botón 100 % existe")
	_check(z_200 != null and z_200.text == "200 %", "Botón 200 % existe")
	_check(z_300 != null and z_300.text == "300 %", "Botón 300 % existe")

	# Verificar tamaños exactos de cada preset + set_canvas_zoom(1) -> 100%
	z_50.emit_signal("pressed")
	await _wait_f(2)
	_check(ed.get_canvas_zoom_mode() == 3 and stage.size == Vector2(480, 270), "50 %% fija lienzo a 480x270 (sz=%s)" % str(stage.size))

	ed.set_canvas_zoom(1)
	await _wait_f(2)
	_check(ed.get_canvas_zoom_mode() == 1 and stage.size == Vector2(960, 540), "set_canvas_zoom(1) va a 100 %, NO a Ajustar")

	z_300.emit_signal("pressed")
	await _wait_f(2)
	_check(ed.get_canvas_zoom_mode() == 4 and stage.size == Vector2(2880, 1620), "300 %% fija lienzo a 2880x1620 (sz=%s)" % str(stage.size))

	z_200.emit_signal("pressed")
	await _wait_f(3)
	_check(ed.get_canvas_zoom_mode() == 2 and stage.size == Vector2(1920, 1080), "200 % fija lienzo a 1920x1080")

	# Fijar scroll manual y someter a 30+ selecciones, ediciones, movimientos, Simple/Avanzado y Undo/Redo -> 0 salto
	cscroll.scroll_horizontal = 140
	cscroll.scroll_vertical = 95
	await _wait_f(2)
	var lock_h: int = cscroll.scroll_horizontal
	var lock_v: int = cscroll.scroll_vertical
	var lock_sc: Vector2 = stage.scale
	var lock_sz: Vector2 = stage.size

	ed.select_screen("title")
	await _wait_f(2)
	z_200.emit_signal("pressed")
	await _wait_f(2)
	cscroll.scroll_horizontal = 140
	cscroll.scroll_vertical = 95
	await _wait_f(2)
	lock_h = cscroll.scroll_horizontal
	lock_v = cscroll.scroll_vertical
	lock_sc = stage.scale
	lock_sz = stage.size

	var created_ids: Array[String] = []
	for k in range(10):
		var nid: String = ed.add_element_at("text", Vector2i(60 + k * 25, 80 + k * 12))
		if not nid.is_empty():
			created_ids.append(nid)
	await _wait_f(2)

	var max_drift: float = 0.0
	for rep in range(32):
		var target: String = created_ids[rep % created_ids.size()] if not created_ids.is_empty() else ""
		ed.select_element(target)
		ed.set_props_advanced_mode((rep % 2) == 1)
		await _wait_f(1)
		max_drift = max(max_drift, abs(float(cscroll.scroll_horizontal - lock_h)))
		max_drift = max(max_drift, abs(float(cscroll.scroll_vertical - lock_v)))
		max_drift = max(max_drift, (stage.scale - lock_sc).length() * 100.0)
		max_drift = max(max_drift, (stage.size - lock_sz).length())
	_check(max_drift < 0.01 and ed.get_canvas_zoom_mode() == 2, "32 selecciones + Simple/Avanzado producen 0 px de salto (drift=%.3f)" % max_drift)

	# Editar propiedades en inspector + arrastrar + Undo/Redo -> 0 salto
	var ox_spin: SpinBox = h.get("prop_ox_spin", null) as SpinBox
	if ox_spin != null and not created_ids.is_empty():
		ed.select_element(created_ids[0])
		await _wait_f(1)
		ox_spin.value = ox_spin.value + 15
		var d0: Dictionary = {}
		var elems_t: Array = dm.get_ui_screen_design("title").get("elements", [])
		for e in elems_t:
			if e is Dictionary and str(e.get("id", "")) == created_ids[0]:
				d0 = e.duplicate(true)
				break
		if not d0.is_empty():
			d0["offset_y"] = int(d0.get("offset_y", 0)) + 12
			ed.update_dragged_element_fast(created_ids[0], d0)
		ed.apply_undo()
		ed.apply_redo()
		await _wait_f(2)
	var drift_after_edit: float = abs(float(cscroll.scroll_horizontal - lock_h)) + abs(float(cscroll.scroll_vertical - lock_v)) + (stage.scale - lock_sc).length() * 100.0 + (stage.size - lock_sz).length()
	_check(drift_after_edit < 0.01, "Editar propiedades, mover y Undo/Redo -> 0 px de salto (drift=%.3f)" % drift_after_edit)

	# Memoria por pestaña: hud -> 50%, title -> conserva 200% + scroll exacto
	ed.select_screen("hud")
	await _wait_f(2)
	z_50.emit_signal("pressed")
	await _wait_f(2)
	_check(ed.get_canvas_zoom_mode() == 3 and stage.size == Vector2(480, 270), "Pestaña HUD queda en 50 %")

	ed.select_screen("title")
	await _wait_f(3)
	_check(ed.get_canvas_zoom_mode() == 2 and stage.size == Vector2(1920, 1080), "Volver a Título recuerda zoom 200 %")
	_check(cscroll.scroll_horizontal == lock_h and cscroll.scroll_vertical == lock_v, "Volver a Título recuerda scroll exacto (%d,%d == %d,%d)" % [cscroll.scroll_horizontal, cscroll.scroll_vertical, lock_h, lock_v])

	ed.select_screen("hud")
	await _wait_f(3)
	_check(ed.get_canvas_zoom_mode() == 3 and stage.size == Vector2(480, 270), "Volver a HUD recuerda zoom 50 %")

	# Rueda del ratón desplaza scroll sin hacer zoom
	ed.select_screen("title")
	await _wait_f(2)
	cscroll.scroll_vertical = 20
	await _wait_f(1)
	var sv_before: int = cscroll.scroll_vertical
	var wh_pos: Vector2 = cscroll.get_global_rect().position + cscroll.get_global_rect().size * 0.5
	var mv := InputEventMouseMotion.new()
	mv.position = wh_pos
	mv.global_position = wh_pos
	db.push_input(mv)
	await _wait_f(1)
	var wh := InputEventMouseButton.new()
	wh.button_index = MOUSE_BUTTON_WHEEL_DOWN
	wh.pressed = true
	wh.factor = 1.0
	wh.position = wh_pos
	wh.global_position = wh_pos
	db.push_input(wh)
	await _wait_f(2)
	_check(ed.get_canvas_zoom_mode() == 2 and cscroll.scroll_vertical > sv_before, "Rueda hace scroll vertical sin cambiar zoom (%d -> %d)" % [sv_before, cscroll.scroll_vertical])

	# En Ajustar tampoco salta al seleccionar elementos; solo al pulsar Ajustar o redimensionar
	z_fit.emit_signal("pressed")
	await _wait_f(2)
	var fit_sc0: Vector2 = stage.scale
	for i in range(5):
		if i < created_ids.size():
			ed.select_element(created_ids[i])
		await _wait_f(1)
	_check((stage.scale - fit_sc0).length() < 0.001 and ed.get_canvas_zoom_mode() == 0, "En Ajustar, seleccionar elementos mantiene escala fija (delta=%.5f)" % (stage.scale - fit_sc0).length())

	db.queue_free()
	_finish()
