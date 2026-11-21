extends SceneTree

var pass_count := 0
var fail_count := 0
var _start_msec := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + msg)
	else:
		fail_count += 1
		printerr("[FAIL] " + msg)

func _wait_frames(n: int) -> void:
	for i in range(n):
		if Time.get_ticks_msec() - _start_msec > 55000:
			printerr("INTERNAL TIMEOUT 55s")
			_finish(1)
			return
		await process_frame

func _finish(forced_code: int = -1) -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	if forced_code >= 0:
		quit(forced_code)
	else:
		quit(0 if fail_count == 0 else 1)

func _check_fit_unclipped(h: Dictionary) -> Dictionary:
	var cs: ScrollContainer = h.get("canvas_scroll", null) as ScrollContainer
	var holder: Control = h.get("canvas_fit_holder", null) as Control
	var st: Control = h.get("canvas_stage", null) as Control
	if cs == null or holder == null or st == null:
		return {"ok": false, "reason": "missing_node"}
	var drawn_w: float = st.size.x * st.scale.x
	var drawn_h: float = st.size.y * st.scale.y
	var end_x: float = st.position.x + drawn_w
	var end_y: float = st.position.y + drawn_h
	var ok_holder: bool = st.position.x >= -0.5 and st.position.y >= -0.5 and end_x <= holder.size.x + 0.5 and end_y <= holder.size.y + 0.5
	var ok_scroll: bool = drawn_w <= cs.size.x + 1.0 and drawn_h <= cs.size.y + 1.0
	return {
		"ok": ok_holder and ok_scroll and drawn_w > 100.0 and drawn_h > 50.0,
		"drawn": Vector2(drawn_w, drawn_h),
		"holder": holder.size,
		"scroll": cs.size,
		"pos": st.position
	}

func _initialize() -> void:
	_start_msec = Time.get_ticks_msec()
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s")
		_finish(1)
	)
	call_deferred("_run_test")

func _run_test() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "DataManager singleton disponible")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var dlg := ClockGearsDatabaseDialog.new()
	dlg.exclusive = false
	dlg.size = Vector2i(1366, 768)
	root.add_child(dlg)
	dlg.open_database()
	await _wait_frames(3)

	dlg.select_category_by_key("game_menu")
	await _wait_frames(3)
	var ed = dlg.get_ui_designer_editor()
	_check(ed != null, "Editor UI disponible")
	if ed == null:
		dlg.queue_free()
		_finish(1)
		return

	var h: Dictionary = ed.get_ui_handles()
	var cs: ScrollContainer = h.get("canvas_scroll", null) as ScrollContainer
	var st: Control = h.get("canvas_stage", null) as Control
	var right_col: Control = h.get("wide_right_col", null) as Control

	# 1. Prueba Partidas + otras pantallas en 3 resoluciones en modo Ajustar -> 0 recorte y 0 cambio en zoom/scroll al hacer 50 selecciones por pantalla
	var sizes := [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]
	var screens := ["game_menu_save", "game_menu_main", "game_menu_items", "game_menu_equip", "title"]

	for win_sz in sizes:
		dlg.size = win_sz
		await _wait_frames(3)
		for sid in screens:
			ed.select_screen(sid)
			ed.set_canvas_zoom_mode(0) # Ajustar
			ed.select_element("")
			await _wait_frames(3)

			var fit0 := _check_fit_unclipped(h)
			_check(bool(fit0.get("ok", false)), "%s @ %dx%d Ajustar entra completo sin recorte (drawn=%s holder=%s)" % [sid, win_sz.x, win_sz.y, str(fit0.get("drawn")), str(fit0.get("holder"))])

			var scale_before: Vector2 = st.scale
			var stage_sz_before: Vector2 = st.size
			var right_w_before: float = right_col.size.x
			var hsc_before: int = cs.scroll_horizontal
			var vsc_before: int = cs.scroll_vertical

			var elems: Array = ed.get_effective_screen_elements()
			var ids: Array = []
			for e in elems:
				if typeof(e) == TYPE_DICTIONARY and str(e.get("id", "")) != "":
					ids.append(str(e.get("id", "")))

			var drift := 0.0
			var clip_ok := true
			if not ids.is_empty():
				for k in range(50):
					var pick_id := str(ids[k % ids.size()])
					ed.select_element(pick_id)
					if k == 10:
						await _wait_frames(1)
						ed.set_props_advanced_mode(true)
					elif k == 20:
						await _wait_frames(1)
						ed.set_props_advanced_mode(false)
					elif k == 30:
						ed.open_floating_element_window(pick_id)
						await _wait_frames(1)
						ed.close_all_floating_element_windows()
				await _wait_frames(3)
				drift = maxf(absf(st.scale.x - scale_before.x), absf(right_col.size.x - right_w_before))
				drift = maxf(drift, st.size.distance_to(stage_sz_before))
				var fit_after := _check_fit_unclipped(h)
				clip_ok = bool(fit_after.get("ok", false))

			_check(int(ed.get_canvas_zoom_mode()) == 0 and drift < 0.01 and cs.scroll_horizontal == hsc_before and cs.scroll_vertical == vsc_before and clip_ok,
				"%s @ %dx%d: 50 selecciones + Simple/Avanzado + flotante => 0 deriva zoom/escala/panel/scroll y 0 recorte (drift=%.4f)" % [sid, win_sz.x, win_sz.y, drift])

	# 2. Zooms fijos (50%, 100%, 200%, 300%): solo cambian al pulsar botones de zoom, nunca al seleccionar ni editar
	dlg.size = Vector2i(1366, 768)
	ed.select_screen("game_menu_save")
	await _wait_frames(3)
	var zoom_btns := {
		3: h.get("zoom_50_btn", null) as Button,
		1: h.get("zoom_2_btn", null) as Button,
		2: h.get("zoom_3_btn", null) as Button,
		4: h.get("zoom_300_btn", null) as Button,
		0: h.get("zoom_1_btn", null) as Button
	}
	for zmode in [3, 1, 2, 4, 0]:
		var btn: Button = zoom_btns[zmode]
		_check(btn != null, "Botón zoom modo %d presente" % zmode)
		if btn != null:
			btn.emit_signal("pressed")
			await _wait_frames(2)
		var mode_now: int = ed.get_canvas_zoom_mode()
		var scale_z: Vector2 = st.scale
		var sz_z: Vector2 = st.size
		if zmode == 2 or zmode == 4:
			cs.scroll_horizontal = 45
			cs.scroll_vertical = 30
		var hsc_z: int = cs.scroll_horizontal
		var vsc_z: int = cs.scroll_vertical
		for sel_id in ["save_panel_frame", "sub_heading", "save_slots", "btn_back", ""]:
			ed.select_element(sel_id)
		await _wait_frames(2)
		_check(int(ed.get_canvas_zoom_mode()) == zmode and mode_now == zmode and st.scale.distance_to(scale_z) < 0.001 and st.size.distance_to(sz_z) < 0.5 and cs.scroll_horizontal == hsc_z and cs.scroll_vertical == vsc_z,
			"Zoom fijo %d no se altera al seleccionar múltiples elementos en Partidas" % zmode)

	# 3. Al duplicar/mover/redimensionar elemento y deshacer en 200%, ni zoom ni scroll se resetean
	ed.set_canvas_zoom_mode(2)
	await _wait_frames(2)
	cs.scroll_horizontal = 60
	cs.scroll_vertical = 40
	ed.select_element("btn_back")
	ed.duplicate_selected_element()
	await _wait_frames(2)
	var el_back: Dictionary = ed.get_effective_element("btn_back")
	el_back["offset_x"] = int(el_back.get("offset_x", 16)) + 25
	ed.update_dragged_element_fast("btn_back", el_back)
	await _wait_frames(2)
	ed.apply_undo()
	await _wait_frames(2)
	_check(int(ed.get_canvas_zoom_mode()) == 2 and cs.scroll_horizontal == 60 and cs.scroll_vertical == 40,
		"Mover, duplicar y deshacer mantiene zoom 200% y desplazamiento exactos (60, 40)")

	ed.request_reset_to_default(true)
	await _wait_frames(2)
	dlg.queue_free()
	await _wait_frames(1)
	_finish()
