extends SceneTree

# Test Punto 2 (Reconstrucción UI 9B):
# - Verifica que arrastrar o editar cualquier pantalla "Por defecto" crea el diseño
#   personalizado conservando TODOS los elementos (marcos, paneles, listas, vista previa,
#   botones Usar/Tirar, huecos, parámetros, ranuras, opciones) en sus mismas posiciones.
# - Verifica Undo y Restablecer a por defecto.
# - Verifica en tiempo de juego (ClockGearsGameMenu) que los diseños personalizados montan
#   los controles interactivos reales dentro de cada widget y ejecutan Usar/Tirar/Volver.

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

func _find_by_class(node: Node, cls_name: String) -> Node:
	if node == null:
		return null
	if node.get_class() == cls_name:
		return node
	for i in range(node.get_child_count()):
		var hit := _find_by_class(node.get_child(i), cls_name)
		if hit != null:
			return hit
	return null

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

func _find_btn_exact(node: Node, text: String) -> Button:
	if node == null:
		return null
	if node is Button:
		var b := node as Button
		if b.text.strip_edges() == text:
			return b
	for i in range(node.get_child_count()):
		var f := _find_btn_exact(node.get_child(i), text)
		if f != null:
			return f
	return null

func _elem_rect(el: Dictionary, sc: float) -> Rect2:
	var ax := 0.0
	var ay := 0.0
	var anc := str(el.get("anchor", "top_left"))
	if anc.ends_with("center") or anc == "center":
		ax = 0.5
	elif anc.ends_with("right"):
		ax = 1.0
	if anc.begins_with("center") or anc == "center":
		ay = 0.5
	elif anc.begins_with("bottom"):
		ay = 1.0
	var sw := 960.0 * sc
	var sh := 540.0 * sc
	var w := float(el.get("width", 120)) * sc
	var h := float(el.get("height", 36)) * sc
	var ox := float(el.get("offset_x", 0)) * sc
	var oy := float(el.get("offset_y", 0)) * sc
	var x := ox
	if absf(ax - 0.5) < 0.01:
		x = sw * 0.5 - w * 0.5 + ox
	elif ax > 0.9:
		x = sw - w + ox
	var y := oy
	if absf(ay - 0.5) < 0.01:
		y = sh * 0.5 - h * 0.5 + oy
	elif ay > 0.9:
		y = sh - h + oy
	return Rect2(x, y, w, h)

func _drag_rect(win: Window, ctrl: Control, local_from: Vector2, local_to: Vector2) -> void:
	var xf: Transform2D = ctrl.get_global_transform_with_canvas()
	var g0 := xf * local_from
	var g1 := xf * local_to
	var p := InputEventMouseButton.new()
	p.button_index = MOUSE_BUTTON_LEFT
	p.pressed = true
	p.position = g0
	p.global_position = g0
	win.push_input(p)

	for s in range(1, 5):
		var cur := g0.lerp(g1, float(s) / 4.0)
		var m := InputEventMouseMotion.new()
		m.button_mask = MOUSE_BUTTON_MASK_LEFT
		m.position = cur
		m.global_position = cur
		m.relative = (g1 - g0) / 4.0
		win.push_input(m)

	var r := InputEventMouseButton.new()
	r.button_index = MOUSE_BUTTON_LEFT
	r.pressed = false
	r.position = g1
	r.global_position = g1
	win.push_input(r)

func _collect_id_map(arr: Array) -> Dictionary:
	var d := {}
	for e in arr:
		if e is Dictionary:
			d[str((e as Dictionary).get("id", ""))] = e
	return d

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		printerr("TIMEOUT INTERNO 60s alcanzado")
		failed += 1
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.init_empty()
	await _step_frames(2)

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("ui_designer")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = db.get_ui_designer_ui_handles()
	var overlay: Control = h.get("canvas_overlay", null) as Control
	_check(ed != null and overlay != null, "ClockGearsUiDesignEditor y overlay inicializados")
	if ed == null or overlay == null:
		db.queue_free()
		await _step_frames(2)
		_finish(1)
		return

	var expected_ids := {
		"game_menu_main": ["menu_frame", "menu_heading", "menu_btn_close", "currency_time_frame", "menu_gold", "menu_playtime", "party_frame", "party_heading", "party_list"],
		"game_menu_items": ["items_panel_frame", "sub_heading", "items_target_selector", "btn_back", "items_list", "item_preview", "item_details", "btn_item_use", "btn_item_discard"],
		"game_menu_equip": ["equip_panel_frame", "sub_heading", "equip_target_selector", "btn_back", "equip_slots", "btn_equip_clear", "equip_candidates", "btn_equip_apply", "equip_item_preview", "equip_desc", "equip_params"],
		"game_menu_status": ["status_panel_frame", "sub_heading", "status_target_selector", "btn_back", "status_basic", "status_equips", "status_params"],
		"game_menu_save": ["save_panel_frame", "sub_heading", "save_slots", "btn_back"],
		"game_menu_options": ["options_panel_frame", "sub_heading", "options_controls", "btn_back"],
		"title": ["title_heading", "btn_new_game"],
		"credits": ["credits_heading", "credits_body", "btn_credits_back"],
		"game_over": ["game_over_title", "btn_game_over_retry", "btn_game_over_title"],
	}

	var screens := ["title", "game_menu_main", "game_menu_items", "game_menu_equip", "game_menu_status", "game_menu_save", "game_menu_options", "credits", "game_over"]
	for sid in screens:
		ed.select_screen(sid)
		await _step_frames(1)
		ed.request_reset_to_default(true)
		await _step_frames(2)
		_check(not dm.has_custom_ui_design(sid), "'%s' empieza Por defecto" % sid)

		var def_elems: Array = dm.get_default_ui_screen_template(sid).get("elements", [])
		var def_map := _collect_id_map(def_elems)
		for req_id in expected_ids[sid]:
			_check(def_map.has(req_id), "  '%s' plantilla incluye '%s'" % [sid, req_id])

		# Arrastrar el primer botón (o penúltimo elemento) sobre el lienzo
		var drag_el: Dictionary = def_elems[def_elems.size() - 1]
		for de in def_elems:
			var dd := de as Dictionary
			if str(dd.get("id", "")) == "btn_back" or str(dd.get("id", "")) == "btn_item_use" or str(dd.get("type", "")) == "button":
				drag_el = dd
				break
		var did := str(drag_el.get("id", ""))
		var pres: Vector2i = ed.get_preview_resolution()
		var sc: float = (float(pres.x) / 960.0) * float(ed.get_canvas_zoom())
		var r0 := _elem_rect(drag_el, sc)
		var c0 := r0.position + r0.size * 0.5
		var dx := -22.0 if c0.x > 480.0 * sc else 22.0
		var dy := -14.0 if c0.y > 270.0 * sc else 14.0
		_drag_rect(db, overlay, c0, c0 + Vector2(dx, dy))
		await _step_frames(2)

		_check(dm.has_custom_ui_design(sid), "  '%s': arrastrar activa diseño personalizado" % sid)
		var cust_elems: Array = dm.get_ui_screen_elements(sid)
		var cust_map := _collect_id_map(cust_elems)
		_check(cust_elems.size() == def_elems.size(), "  '%s': conserva TODOS los elementos (%d == %d)" % [sid, cust_elems.size(), def_elems.size()])

		var moved_ok := false
		var intact_ok := true
		for eid in def_map.keys():
			if not cust_map.has(eid):
				intact_ok = false
				break
			var b: Dictionary = def_map[eid]
			var a: Dictionary = cust_map[eid]
			if eid == did:
				moved_ok = int(a.get("offset_x", 0)) != int(b.get("offset_x", 0)) or int(a.get("offset_y", 0)) != int(b.get("offset_y", 0))
			else:
				if int(a.get("offset_x", 0)) != int(b.get("offset_x", 0)) or int(a.get("offset_y", 0)) != int(b.get("offset_y", 0)) or int(a.get("width", 0)) != int(b.get("width", 0)) or int(a.get("height", 0)) != int(b.get("height", 0)):
					intact_ok = false
		_check(moved_ok, "  '%s': elemento arrastrado '%s' se movió" % [sid, did])
		_check(intact_ok, "  '%s': demás elementos mantienen exactamente sus posiciones y tamaños" % sid)
		_check(bool(ed.apply_undo()), "  '%s': Deshacer tras arrastrar" % sid)
		await _step_frames(1)
		_check(bool(ed.apply_redo()), "  '%s': Rehacer tras arrastrar" % sid)
		await _step_frames(1)

	# Prueba runtime en ClockGearsGameMenu para verificar interactividad con diseño personalizado activo
	var gm := ClockGearsGameMenu.new()
	gm.set_preview_mode(false)
	root.add_child(gm)
	_check(gm.open_menu_by_id("game_menu_main", false, false, true), "Runtime ClockGearsGameMenu abre con diseño personalizado")
	await _step_frames(2)

	var runtime_subs := [
		["main", "party_list"],
		["items", "items_list"],
		["items", "item_preview"],
		["items", "item_details"],
		["equip", "equip_slots"],
		["equip", "equip_candidates"],
		["equip", "equip_preview"],
		["equip", "equip_params"],
		["status", "status_basic"],
		["status", "status_equips"],
		["status", "status_params"],
		["save", "save_slots"],
		["options", "options_controls"],
	]
	for pair in runtime_subs:
		var sub: String = pair[0]
		var wkind: String = pair[1]
		gm.open_subscreen(sub)
		await _step_frames(2)
		var layer = gm.get_custom_ui_layer()
		_check(layer != null and layer.is_visible(), "Runtime '%s': capa personalizada visible" % sub)
		var mount = layer.get_widget_mount(wkind) if layer != null else null
		_check(mount != null and mount.get_child_count() > 0, "Runtime '%s': widget '%s' montado y funcional" % [sub, wkind])

	# Probar retorno desde botón 'Volver' en subpantalla personalizada
	gm.open_subscreen("items")
	await _step_frames(1)
	_check(gm.trigger_custom_element_by_id("btn_back"), "Runtime 'items': botón personalizado 'Volver' dispara acción")
	await _step_frames(1)
	_check(gm.get_current_subscreen() == "main", "Runtime 'items': 'Volver' regresa a Principal (actual='%s')" % gm.get_current_subscreen())

	# Volver y restablecer a por defecto todas las pantallas
	gm.close_menu()
	gm.queue_free()
	for sid in screens:
		ed.select_screen(sid)
		ed.request_reset_to_default(true)
		await _step_frames(1)
		_check(not dm.has_custom_ui_design(sid), "Restablecido '%s' a Por defecto" % sid)

	db.queue_free()
	await _step_frames(2)
	_finish(0 if failed == 0 else 1)
