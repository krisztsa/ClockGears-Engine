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

func _rect_err(a: Rect2, b: Rect2) -> float:
	var dx: float = absf(a.position.x - b.position.x)
	var dy: float = absf(a.position.y - b.position.y)
	var dw: float = absf(a.size.x - b.size.x)
	var dh: float = absf(a.size.y - b.size.y)
	return maxf(maxf(dx, dy), maxf(dw, dh))

func _check_handles_on_rect(ed, r: Rect2) -> float:
	var max_e := 0.0
	var expected := [
		Vector2(r.position.x, r.position.y),
		Vector2(r.position.x + r.size.x * 0.5, r.position.y),
		Vector2(r.position.x + r.size.x, r.position.y),
		Vector2(r.position.x, r.position.y + r.size.y * 0.5),
		Vector2(r.position.x + r.size.x, r.position.y + r.size.y * 0.5),
		Vector2(r.position.x, r.position.y + r.size.y),
		Vector2(r.position.x + r.size.x * 0.5, r.position.y + r.size.y),
		Vector2(r.position.x + r.size.x, r.position.y + r.size.y)
	]
	for idx in range(8):
		var hr: Rect2 = ed.get_overlay_handle_rect(idx + 1)
		var hc := hr.position + hr.size * 0.5
		max_e = maxf(max_e, hc.distance_to(expected[idx]))
	return max_e

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

	# 1. Prueba específica en 'Partidas' (game_menu_save): ajuste + zoom 100%, hijos con offset local
	ed.select_screen("game_menu_save")
	await _wait_frames(3)
	for zm in [0, 1]:
		ed.set_canvas_zoom_mode(zm)
		await _wait_frames(2)
		var elems_save: Array = ed.get_effective_screen_elements()
		var max_err_save := 0.0
		var max_hdl_err_save := 0.0
		for e in elems_save:
			if typeof(e) != TYPE_DICTIONARY:
				continue
			var eid := str(e.get("id", ""))
			if str(e.get("type", "")) == "folder":
				continue
			ed.select_element(eid)
			await _wait_frames(1)
			var ov_r: Rect2 = ed.get_overlay_element_rect(eid)
			var sel_r: Rect2 = ed.get_overlay_selected_rect()
			var rnd_r: Rect2 = ed.get_rendered_element_stage_rect(eid)
			max_err_save = maxf(max_err_save, maxf(_rect_err(ov_r, rnd_r), _rect_err(sel_r, rnd_r)))
			max_hdl_err_save = maxf(max_hdl_err_save, _check_handles_on_rect(ed, rnd_r))
		_check(max_err_save <= 1.0 and max_hdl_err_save <= 1.0, "Partidas (zoom=%d): selección y 8 tiradores coinciden con el widget real (err=%.2fpx, hdl=%.2fpx)" % [zm, max_err_save, max_hdl_err_save])

	# Verificar explícitamente que sub_heading en Partidas no está en (16,12) sin padre (debe incluir save_panel_frame (24,20) => x>=39, y>=31)
	ed.set_canvas_zoom_mode(1)
	await _wait_frames(2)
	var sub_r: Rect2 = ed.get_overlay_element_rect("sub_heading")
	_check(sub_r.position.x >= 38.0 and sub_r.position.y >= 30.0, "Partidas sub_heading tiene coordenadas reales con padre (x=%.1f, y=%.1f)" % [sub_r.position.x, sub_r.position.y])

	# 2. Recorrer TODAS las pantallas (sistema + botones de menú) en Ajustar y 100%/200%
	var all_screens := [
		"title", "game_menu_main", "game_menu_items", "game_menu_skills",
		"game_menu_equip", "game_menu_status", "game_menu_save", "game_menu_options",
		"credits", "hud", "game_over"
	]
	var worst_err := 0.0
	var worst_hdl_err := 0.0
	var worst_desc := ""
	var total_checked := 0

	for sid in all_screens:
		ed.select_screen(sid)
		await _wait_frames(2)
		for zm in [0, 1, 2]:
			ed.set_canvas_zoom_mode(zm)
			await _wait_frames(2)
			var elems: Array = ed.get_effective_screen_elements()
			for e in elems:
				if typeof(e) != TYPE_DICTIONARY:
					continue
				var eid := str(e.get("id", ""))
				if str(e.get("type", "")) == "folder" or not bool(e.get("visible", true)):
					continue
				ed.select_element(eid)
				var ov_r: Rect2 = ed.get_overlay_element_rect(eid)
				var sel_r: Rect2 = ed.get_overlay_selected_rect()
				var rnd_r: Rect2 = ed.get_rendered_element_stage_rect(eid)
				if not rnd_r.has_area():
					continue
				var er1: float = _rect_err(ov_r, rnd_r)
				var er2: float = _rect_err(sel_r, rnd_r)
				var her: float = _check_handles_on_rect(ed, rnd_r)
				if maxf(er1, er2) > worst_err:
					worst_err = maxf(er1, er2)
					worst_desc = "%s/%s@z%d (ov=%s rnd=%s)" % [sid, eid, zm, str(ov_r), str(rnd_r)]
				worst_hdl_err = maxf(worst_hdl_err, her)
				total_checked += 1

	_check(total_checked >= 80 and worst_err <= 1.0 and worst_hdl_err <= 1.0,
		"Todos los %d elementos de todas las pantallas coinciden <=1px (peor err=%.2fpx [%s], hdl=%.2fpx)" % [total_checked, worst_err, worst_desc, worst_hdl_err])

	# 3. Elemento personalizado anidado 3 niveles (carpeta -> panel -> boton)
	ed.select_screen("game_menu_main")
	ed.set_canvas_zoom_mode(0)
	await _wait_frames(2)
	var f_id: String = ed.create_folder_element("Carpeta P2")
	var p_id: String = ed.add_child_element_of_type(f_id, "panel")
	var c_id: String = ed.add_child_element_of_type(p_id, "button")
	await _wait_frames(2)
	for eid_test in [p_id, c_id]:
		ed.select_element(eid_test)
		await _wait_frames(1)
		var sel_n: Rect2 = ed.get_overlay_selected_rect()
		var rnd_n: Rect2 = ed.get_rendered_element_stage_rect(eid_test)
		_check(_rect_err(sel_n, rnd_n) <= 1.0, "Elemento anidado %s selección coincide <=1px con widget real (err=%.2fpx)" % [eid_test, _rect_err(sel_n, rnd_n)])

	ed.request_reset_to_default(true)
	await _wait_frames(2)
	dlg.queue_free()
	await _wait_frames(1)
	_finish()
