extends SceneTree

# Test Punto 5 (Reconstrucción UI 9B):
# - Base de Datos con tamaño mínimo estable (960x600) sin contenidos internos que la ensanchen
#   más allá de la resolución de pantalla en 1366x768, 1920x1080 y 2560x1440.
# - Especial énfasis en 'game_menu_items' (Objetos) con elemento seleccionado y ajustes de vista previa.
# - Pestaña "Menús" y panel de Propiedades nunca se cortan ni requieren scroll horizontal.
# - Lienzo y vistas previas mantienen relación 16:9 completa sin recorte.

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

func _find_btn_exact(node: Node, text: String) -> Button:
	if node == null:
		return null
	if node is Button and (node as Button).text.strip_edges() == text:
		return node as Button
	for i in range(node.get_child_count()):
		var hit := _find_btn_exact(node.get_child(i), text)
		if hit != null:
			return hit
	return null

func _check_within_bounds(container: Control, label: String, tol: float = 3.0) -> void:
	var crect := container.get_global_rect()
	var clipped := []
	var stack: Array[Node] = [container]
	while not stack.is_empty():
		var cur: Node = stack.pop_back()
		if cur is Control and cur != container:
			var c := cur as Control
			if not c.is_visible_in_tree():
				continue
			var r := c.get_global_rect()
			if r.size.x > 1.0 and r.size.y > 1.0:
				if r.position.x < crect.position.x - tol or (r.position.x + r.size.x) > (crect.position.x + crect.size.x + tol):
					if not (c is HScrollBar or c is VScrollBar):
						clipped.append("%s(%.0f..%.0f vs %.0f..%.0f)" % [c.name, r.position.x, r.position.x + r.size.x, crect.position.x, crect.position.x + crect.size.x])
			if c is ScrollContainer:
				var hs := (c as ScrollContainer).get_h_scroll_bar()
				if hs != null and hs.visible and hs.max_value > hs.page + 2.0:
					clipped.append("HScrollBar(%s:%.0f>%.0f)" % [c.name, hs.max_value, hs.page])
		for i in range(cur.get_child_count()):
			var ch := cur.get_child(i)
			if not (ch is Window or ch is Popup or ch is SubViewport):
				stack.push_back(ch)
	_check(clipped.is_empty(), "%s: sin cortes ni scroll horizontal (%d hallados%s)" % [label, clipped.size(), (": " + str(clipped.slice(0, 4))) if not clipped.is_empty() else ""])

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60 s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 5 (RECONSTRUCCION UI 9B): VENTANA ESTABLE Y 16:9 ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.init_empty()

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.open_database()
	db.select_category_by_key("ui_designer")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = db.get_ui_designer_ui_handles()
	var panel: Control = h.get("panel", null) as Control
	var stage: Control = h.get("canvas_stage", null) as Control
	var menus_btn := _find_btn_exact(db, "Menús")

	_check(db.min_size.x <= 1000 and db.min_size.y <= 640, "Tamaño mínimo DB razonable (%s)" % str(db.min_size))

	for res in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		root.size = res
		db.size = Vector2i(mini(res.x, int(res.x * 0.92)), mini(res.y, int(res.y * 0.92))).max(db.min_size)
		await _step_frames(3)
		var cmin := db.get_contents_minimum_size()
		_check(cmin.x <= float(res.x) + 1.0 and db.size.x <= res.x, "%dx%d: contenido no ensancha DB (%s <= %d)" % [res.x, res.y, str(cmin), res.x])
		if menus_btn != null:
			var mr := menus_btn.get_global_rect()
			_check(mr.position.x + mr.size.x <= float(db.size.x) + 1.0, "%dx%d: pestaña 'Menús' completa dentro de DB (xr=%.0f <= %d)" % [res.x, res.y, mr.position.x + mr.size.x, db.size.x])

		for sid in ["title", "game_menu_main", "game_menu_items", "game_menu_equip", "game_menu_status", "game_menu_save", "game_menu_options", "credits", "hud", "game_over"]:
			ed.select_screen(sid)
			await _step_frames(2)
			# Seleccionar elemento botón o primer elemento para desplegar TODAS las propiedades en panel derecho
			var elems: Array = dm.get_ui_screen_elements(sid) if dm.has_custom_ui_design(sid) else Array(dm.get_default_ui_screen_template(sid).get("elements", []))
			if not elems.is_empty():
				var target_id := str((elems[elems.size() - 1] as Dictionary).get("id", ""))
				ed.select_element(target_id)
				await _step_frames(1)
			var cmin_s := db.get_contents_minimum_size()
			_check(cmin_s.x <= float(res.x) + 1.0, "%dx%d [%s]: DB no desborda ancho (min_x=%.0f)" % [res.x, res.y, sid, cmin_s.x])
			if panel != null:
				_check_within_bounds(panel, "%dx%d [%s]" % [res.x, res.y, sid])
			if stage != null and stage.size.x > 10.0 and stage.size.y > 10.0:
				var ar := stage.size.x / stage.size.y
				_check(absf(ar - (16.0 / 9.0)) <= 0.05, "%dx%d [%s]: lienzo mantiene 16:9 (ar=%.3f)" % [res.x, res.y, sid, ar])

	# Probar también DB al mínimo exacto (960x600) sobre Objetos
	db.size = db.min_size
	ed.select_screen("game_menu_items")
	await _step_frames(3)
	_check(db.get_contents_minimum_size().x <= float(db.min_size.x) + 2.0, "DB mínimo (%s) admite pantalla 'Objetos' sin ensancharse" % str(db.min_size))
	if panel != null:
		_check_within_bounds(panel, "DB min [%s] Objetos" % str(db.min_size))

	db.queue_free()
	await _step_frames(2)
	_finish(0 if failed == 0 else 1)
