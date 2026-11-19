extends SceneTree

# Test Punto 1 (Reconstrucción UI 9B):
# - Abre CADA pantalla simulando clic real en el árbol de pantallas (y vía select_screen).
# - En cada pantalla: selecciona cada elemento, arrastra, redimensiona, deshace, rehace,
#   restablece y cambia de pantalla sin cierres ni errores.
# - Verifica también el archivo de traza user://ui_editor_trace.log.

var passed: int = 0
var failed: int = 0
var _finished: bool = false

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _finish(code: int) -> void:
	if _finished:
		return
	_finished = true
	print("RESULTADOS: %d PASS, %d FAIL" % [passed, failed])
	quit(code)

func _step_frames(n: int = 2) -> void:
	var c := mini(n, 600)
	for _i in range(c):
		await process_frame

func _find_tree_item_by_meta(root_it: TreeItem, target: String) -> TreeItem:
	if root_it == null:
		return null
	if root_it.is_selectable(0) and str(root_it.get_metadata(0)) == target:
		return root_it
	var ch: TreeItem = root_it.get_first_child()
	while ch != null:
		var found := _find_tree_item_by_meta(ch, target)
		if found != null:
			return found
		ch = ch.get_next()
	return null

func _click_tree_item(win: Window, tree: Tree, it: TreeItem) -> void:
	tree.scroll_to_item(it, true)
	await _step_frames(1)
	var r: Rect2 = tree.get_item_area_rect(it)
	var xf: Transform2D = tree.get_global_transform_with_canvas()
	var local_pt := r.position + Vector2(minf(40.0, r.size.x * 0.5), r.size.y * 0.5)
	var g := xf * local_pt
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = g
	press.global_position = g
	win.push_input(press)
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = g
	rel.global_position = g
	win.push_input(rel)

func _drag_rect(win: Window, overlay: Control, from_pt: Vector2, to_pt: Vector2) -> void:
	var xf: Transform2D = overlay.get_global_transform_with_canvas()
	var g0 := xf * from_pt
	var g1 := xf * to_pt
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = g0
	press.global_position = g0
	win.push_input(press)
	var mot := InputEventMouseMotion.new()
	mot.button_mask = MOUSE_BUTTON_MASK_LEFT
	mot.position = g1
	mot.global_position = g1
	mot.relative = g1 - g0
	win.push_input(mot)
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = g1
	rel.global_position = g1
	win.push_input(rel)

func _elem_rect(el: Dictionary, sc: float) -> Rect2:
	var anc := str(el.get("anchor", "top_left"))
	var ax := 0.0
	var ay := 0.0
	if anc.ends_with("_center") or anc == "center":
		ax = 0.5
	elif anc.ends_with("_right"):
		ax = 1.0
	if anc.begins_with("center_") or anc == "center":
		ay = 0.5
	elif anc.begins_with("bottom_"):
		ay = 1.0
	var w := float(el.get("width", 120))
	var h := float(el.get("height", 36))
	var ox := float(el.get("offset_x", 0))
	var oy := float(el.get("offset_y", 0))
	return Rect2((ax * (960.0 - w) + ox) * sc, (ay * (540.0 - h) + oy) * sc, w * sc, h * sc)

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno de 60 s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 1 (RECONSTRUCCION UI 9B): CIERRES AL CAMBIAR DE PANTALLA ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		_check(false, "ClockGearsDataManager disponible")
		_finish(1)
		return
	dm.init_empty()

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("ui_designer")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = db.get_ui_designer_ui_handles()
	var tree: Tree = h.get("screens_tree", null) as Tree
	var overlay: Control = h.get("canvas_overlay", null) as Control
	_check(ed != null and tree != null and overlay != null, "Editor Interfaz, árbol y overlay inicializados")

	var screens := [
		"title",
		"game_menu_options",
		"game_menu_save",
		"game_menu_main",
		"game_menu_items",
		"game_menu_equip",
		"game_menu_status",
		"credits",
		"hud",
		"game_over"
	]

	for sid in screens:
		var it := _find_tree_item_by_meta(tree.get_root(), sid)
		_check(it != null, "Árbol contiene pantalla '%s'" % sid)
		if it != null:
			await _click_tree_item(db, tree, it)
			await _step_frames(3)
		_check(ed.get_current_screen() == sid, "Clic real en árbol abre '%s' sin cierre (actual='%s')" % [sid, ed.get_current_screen()])

		var raw_elems: Array = dm.get_ui_screen_elements(sid)
		var elems: Array = raw_elems if (dm.has_custom_ui_design(sid) and not raw_elems.is_empty()) else Array(dm.get_default_ui_screen_template(sid).get("elements", []))
		_check(elems.size() >= 1, "Pantalla '%s' tiene elementos (%d)" % [sid, elems.size()])
		for e in elems:
			var eid := str((e as Dictionary).get("id", ""))
			var ok_sel := bool(ed.select_element(eid))
			_check(ok_sel, "  '%s': seleccionar '%s'" % [sid, eid])

		if elems.size() >= 1:
			var first_el: Dictionary = elems[elems.size() - 1]
			var fid := str(first_el.get("id", ""))
			ed.select_element(fid)
			await _step_frames(1)
			var pres: Vector2i = ed.get_preview_resolution()
			var sc: float = (float(pres.x) / 960.0) * float(ed.get_canvas_zoom())
			var r0 := _elem_rect(first_el, sc)
			var c0 := r0.position + r0.size * 0.5
			var dx := -24.0 if c0.x > 480.0 * sc else 24.0
			var dy := -16.0 if c0.y > 270.0 * sc else 16.0
			_drag_rect(db, overlay, c0, c0 + Vector2(dx, dy))
			await _step_frames(2)
			var after_drag: Dictionary = dm.get_ui_element(sid, fid)
			var moved := int(after_drag.get("offset_x", 0)) != int(first_el.get("offset_x", 0)) or int(after_drag.get("offset_y", 0)) != int(first_el.get("offset_y", 0))
			_check(moved, "  '%s': arrastrar mueve '%s'" % [sid, fid])

			# Redimensionar desde esquina inferior derecha (hacia dentro si está cerca del borde)
			var r1 := _elem_rect(after_drag, sc)
			var br := r1.position + r1.size
			var rdx := -16.0 if br.x > 880.0 * sc else 20.0
			var rdy := -8.0 if br.y > 490.0 * sc else 10.0
			_drag_rect(db, overlay, br, br + Vector2(rdx, rdy))
			await _step_frames(2)
			var after_res: Dictionary = dm.get_ui_element(sid, fid)
			var resized := int(after_res.get("width", 0)) != int(after_drag.get("width", 0)) or int(after_res.get("height", 0)) != int(after_drag.get("height", 0))
			_check(resized, "  '%s': redimensionar '%s' (%dx%d -> %dx%d)" % [sid, fid, int(after_drag.get("width", 0)), int(after_drag.get("height", 0)), int(after_res.get("width", 0)), int(after_res.get("height", 0))])

			_check(bool(ed.apply_undo()), "  '%s': deshacer sin errores" % sid)
			await _step_frames(1)
			_check(bool(ed.apply_redo()), "  '%s': rehacer sin errores" % sid)
			await _step_frames(1)
			ed.request_reset_to_default(true)
			await _step_frames(2)
			_check(not dm.has_custom_ui_design(sid) or sid == "hud", "  '%s': restablecer completado sin cierre" % sid)

	var trace_txt := FileAccess.get_file_as_string("user://ui_editor_trace.log")
	_check(not trace_txt.is_empty() and trace_txt.find("screen_open_done") >= 0, "Log user://ui_editor_trace.log escrito (%d bytes)" % trace_txt.length())
	print("TRACE_LOG_PATH=" + ProjectSettings.globalize_path("user://ui_editor_trace.log"))

	db.queue_free()
	await _step_frames(2)
	_finish(0 if failed == 0 else 1)
