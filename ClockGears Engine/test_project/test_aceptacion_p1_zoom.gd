extends SceneTree

# Prueba de aceptación Punto 1: El zoom y ancho de los paneles no saltan al seleccionar.

var pass_count: int = 0
var fail_count: int = 0
var _finished: bool = false

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] " + msg)
	else:
		fail_count += 1
		printerr("  [FAIL] " + msg)

func _finish(code: int) -> void:
	if _finished:
		return
	_finished = true
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(code)

func _step_frames(n: int = 2) -> void:
	var c := mini(n, 600)
	for _i in range(c):
		await process_frame

func _click_at(win: Window, pos: Vector2) -> void:
	var ev_d := InputEventMouseButton.new()
	ev_d.button_index = MOUSE_BUTTON_LEFT
	ev_d.pressed = true
	ev_d.position = pos
	ev_d.global_position = pos
	win.push_input(ev_d)

	var ev_u := InputEventMouseButton.new()
	ev_u.button_index = MOUSE_BUTTON_LEFT
	ev_u.pressed = false
	ev_u.position = pos
	ev_u.global_position = pos
	win.push_input(ev_u)

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== ACEPTACIÓN PUNTO 1: ZOOM Y PANELES FIJOS AL SELECCIONAR ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var cap_dir := ProjectSettings.globalize_path("res://capturas_aceptacion")
	DirAccess.make_dir_recursive_absolute(cap_dir)

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("main_menu")
	await _step_frames(5)

	var ed := db.get_ui_designer_editor()
	_check(ed != null, "Editor de UI obtenido dentro de la Base de Datos")
	var h: Dictionary = ed.get_ui_handles()
	var left_col: Control = h.get("left_col_scroll", null) as Control
	var center_col: Control = h.get("wide_center_col", null) as Control
	var right_col: Control = h.get("wide_right_col", null) as Control
	var canvas_scroll: Control = h.get("canvas_scroll", null) as Control
	var canvas_stage: Control = h.get("canvas_stage", null) as Control

	# Asegurar zoom Ajustar activo
	ed.set_canvas_zoom_mode(0)
	await _step_frames(4)

	# Deseleccionar primero para capturar estado "antes" (sin propiedades de elemento abiertas)
	ed.select_element("")
	await _step_frames(3)
	var base_left_w: float = left_col.size.x
	var base_center_w: float = center_col.size.x
	var base_right_w: float = right_col.size.x
	var base_canvas_r: Rect2 = canvas_scroll.get_global_rect()
	var base_stage_r: Rect2 = canvas_stage.get_global_rect()
	var base_scale: Vector2 = canvas_stage.scale
	var base_zoom_mode: int = ed.get_canvas_zoom_mode()

	print("BASE METRICS: left=%.1f center=%.1f right=%.1f canvas_rect=%s stage_rect=%s scale=%s mode=%d" % [
		base_left_w, base_center_w, base_right_w, str(base_canvas_r), str(base_stage_r), str(base_scale), base_zoom_mode
	])

	var img_before: Image = db.get_texture().get_image()
	img_before.save_png(cap_dir.path_join("p1_antes.png"))

	# Recorrer todas las pantallas del capítulo Menús y hacer clics reales en elementos del árbol y del lienzo
	var ch_info: Dictionary = db.get_chapter_structure_info()
	var menu_keys: Array[String] = []
	for ch in ch_info.get("chapters", []):
		if str(ch.get("name", "")).find("Men") >= 0:
			for ckey in ch.get("category_keys", []):
				var skey := str(ckey)
				if skey != "window_styles":
					menu_keys.append(skey)
	_check(menu_keys.size() >= 6, "Encontradas todas las pestañas del capítulo Menús (%d: %s)" % [menu_keys.size(), str(menu_keys)])

	var total_clicks := 0
	var max_delta_px := 0.0
	var max_scale_delta := 0.0
	var captured_5 := false

	for cat_key in menu_keys:
		db.select_category_by_key(cat_key)
		await _step_frames(4)
		# Primero probar deseleccionado y luego seleccionar todos los elementos por tarjeta y por lienzo
		ed.select_element("")
		await _step_frames(2)

		var cards: Array = ed.get_design_cards_info()
		for cinfo in cards:
			var eid := str(cinfo.get("id", ""))
			var card_ctl: Control = cinfo.get("card_control", null) as Control
			if card_ctl != null and card_ctl.is_visible_in_tree():
				var cr := card_ctl.get_global_rect()
				_click_at(db, cr.position + Vector2(minf(cr.size.x * 0.4, 60.0), cr.size.y * 0.5))
				await _step_frames(2)
				total_clicks += 1

				var d_w := maxf(absf(left_col.size.x - base_left_w), maxf(absf(center_col.size.x - base_center_w), absf(right_col.size.x - base_right_w)))
				var cur_canvas_r := canvas_scroll.get_global_rect()
				var cur_stage_r := canvas_stage.get_global_rect()
				var d_cr := maxf(absf(cur_canvas_r.size.x - base_canvas_r.size.x), absf(cur_canvas_r.size.y - base_canvas_r.size.y))
				var d_sr := maxf(absf(cur_stage_r.size.x - base_stage_r.size.x), absf(cur_stage_r.size.y - base_stage_r.size.y))
				var d_sc := absf(canvas_stage.scale.x - base_scale.x)
				if maxf(d_w, maxf(d_cr, d_sr)) > 0.5 or d_sc > 0.001:
					print("DELTA DETECTED cat=%s elem=%s L=%.1f(base=%.1f) C=%.1f(base=%.1f) R=%.1f(base=%.1f) cr=%s sr=%s scale=%s" % [
						cat_key, eid, left_col.size.x, base_left_w, center_col.size.x, base_center_w, right_col.size.x, base_right_w, str(cur_canvas_r), str(cur_stage_r), str(canvas_stage.scale)
					])
				max_delta_px = maxf(max_delta_px, maxf(d_w, maxf(d_cr, d_sr)))
				max_scale_delta = maxf(max_scale_delta, d_sc)

				if not captured_5 and total_clicks >= 5:
					var img_5: Image = db.get_texture().get_image()
					img_5.save_png(cap_dir.path_join("p1_despues_5_selecciones.png"))
					captured_5 = true

			# Ahora clic real en el rectángulo del elemento dentro del lienzo
			var ov_r: Rect2 = ed.get_rendered_element_stage_rect(eid)
			if ov_r.size.x > 4.0 and ov_r.size.y > 4.0:
				var gl_pt: Vector2 = canvas_stage.get_global_transform() * (ov_r.position + ov_r.size * 0.5)
				if canvas_scroll.get_global_rect().has_point(gl_pt):
					_click_at(db, gl_pt)
					await _step_frames(2)
					total_clicks += 1
					var d_sc2 := absf(canvas_stage.scale.x - base_scale.x)
					var d_w2 := maxf(absf(left_col.size.x - base_left_w), maxf(absf(center_col.size.x - base_center_w), absf(right_col.size.x - base_right_w)))
					max_delta_px = maxf(max_delta_px, d_w2)
					max_scale_delta = maxf(max_scale_delta, d_sc2)

	if not captured_5:
		var img_fallback: Image = db.get_texture().get_image()
		img_fallback.save_png(cap_dir.path_join("p1_despues_5_selecciones.png"))

	# También probar zoom fijo 100 % y seleccionar elementos
	db.select_category_by_key("main_menu")
	ed.set_canvas_zoom_mode(1) # 100%
	await _step_frames(3)
	var zoom100_mode: int = ed.get_canvas_zoom_mode()
	var zoom100_scale := canvas_stage.scale
	var zoom100_stage_sz := canvas_stage.size
	var tcards := ed.get_design_cards_info()
	for tc in tcards:
		var cc := tc.get("card_control", null) as Control
		if cc != null and cc.is_visible_in_tree():
			_click_at(db, cc.get_global_rect().position + cc.get_global_rect().size * 0.4)
			await _step_frames(2)
	_check(ed.get_canvas_zoom_mode() == zoom100_mode and canvas_stage.scale.is_equal_approx(zoom100_scale) and canvas_stage.size.is_equal_approx(zoom100_stage_sz), "Zoom fijo 100%% no cambia al seleccionar (scale=%s, stage=%s)" % [str(zoom100_scale), str(zoom100_stage_sz)])

	_check(total_clicks >= 10, "Se ejecutaron selecciones reales en el árbol y lienzo (%d clics)" % total_clicks)
	_check(max_delta_px <= 0.5, "Ningún panel ni el rectángulo del lienzo cambiaron de tamaño ni 1 px (max_delta=%.2f)" % max_delta_px)
	_check(max_scale_delta <= 0.001, "El valor del zoom 'Ajustar' no saltó al seleccionar (max_scale_delta=%.5f)" % max_scale_delta)

	db.hide()
	db.queue_free()
	await _step_frames(2)
	_finish(0 if fail_count == 0 else 1)
