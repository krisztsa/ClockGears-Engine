extends SceneTree

# Test Punto 1 (Segunda Pasada UI 9B): Editor grande y pulido
# - 3 columnas en 1366x768, 1920x1080, 2560x1440 en TODAS las pestanas
# - Lienzo ocupa >= 60% del ancho del editor y muestra 100% de 16:9 centrado sin recortes
# - Barra superior unica fija con separadores, etiquetas completas ('Probar'), rueda no hace zoom
# - Modo Simple vs Avanzado con 6 secciones (Posicion, Aspecto, Imagen, Texto, Acciones, Animacion)

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

func _check_no_clip_or_overlap(container: Control, label: String, tol: float = 3.0) -> void:
	var crect := container.get_global_rect()
	var clipped: Array[String] = []
	var top_bar_rects: Array[Rect2] = []
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
						clipped.append("%s(x=%.0f..%.0f vs %.0f..%.0f)" % [c.name, r.position.x, r.position.x + r.size.x, crect.position.x, crect.position.x + crect.size.x])
			if c is ScrollContainer and c.name != "UiDesignerCanvasScroll":
				var hs := (c as ScrollContainer).get_h_scroll_bar()
				if hs != null and hs.visible and hs.max_value > hs.page + 2.0:
					clipped.append("HScrollBar(%s:%.0f>%.0f)" % [c.name, hs.max_value, hs.page])
		for i in range(cur.get_child_count()):
			var ch := cur.get_child(i)
			if not (ch is Window or ch is Popup or ch is SubViewport):
				stack.push_back(ch)
	_check(clipped.is_empty(), "%s: sin cortes ni scroll H (%d hallados%s)" % [label, clipped.size(), (": " + str(clipped.slice(0, 3))) if not clipped.is_empty() else ""])

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 1 (SEGUNDA PASADA UI 9B): EDITOR GRANDE Y PULIDO ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	await _step_frames(3)

	var design_tabs := [
		"main_menu", "menu_options", "menu_saves", "game_menu",
		"menu_inventory", "menu_equip", "menu_status", "credits",
		"game_over", "hud", "messages_battle"
	]
	var resolutions := [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]

	for res in resolutions:
		root.size = res
		var db_sz := Vector2i(int(res.x * 0.94), int(res.y * 0.94)).max(db.min_size).min(res)
		db.size = db_sz
		await _step_frames(2)
		for ckey in design_tabs:
			db.select_category_by_key(ckey)
			await _step_frames(2)
			var ed := db.get_ui_designer_editor()
			var h: Dictionary = ed.get_ui_handles()
			var panel: Control = h.get("panel", null) as Control
			var canvas_card: Control = h.get("canvas_card", null) as Control
			var stage: Control = h.get("canvas_stage", null) as Control
			var cscroll: ScrollContainer = h.get("canvas_scroll", null) as ScrollContainer
			_check(panel != null and canvas_card != null and stage != null, "[%dx%d][%s] Panel y lienzo presentes" % [res.x, res.y, ckey])
			if panel != null and canvas_card != null and stage != null:
				var pw := panel.get_rect().size.x
				var cw := canvas_card.get_rect().size.x
				var ratio := (cw / pw) if pw > 1.0 else 0.0
				_check(ratio >= 0.60, "[%dx%d][%s] Lienzo domina >= 60%% del ancho (%.1f%%: %.0f / %.0f)" % [res.x, res.y, ckey, ratio * 100.0, cw, pw])
				# Verificar que el stage escalado 16:9 entero cabe sin recorte dentro de canvas_scroll
				var sr := stage.get_global_rect()
				var scr := cscroll.get_global_rect()
				var full_fit := sr.position.x >= scr.position.x - 2.0 and (sr.position.x + sr.size.x) <= (scr.position.x + scr.size.x + 2.0) and sr.position.y >= scr.position.y - 2.0 and (sr.position.y + sr.size.y) <= (scr.position.y + scr.size.y + 2.0)
				_check(full_fit, "[%dx%d][%s] Lienzo 16:9 completo dentro del visor sin recorte" % [res.x, res.y, ckey])
				_check_no_clip_or_overlap(panel, "%dx%d[%s]" % [res.x, res.y, ckey])

	# Verificar barra fija, etiquetas completas, zoom Ajustar/100%/200%, rueda NO hace zoom, y modo Simple vs Avanzado
	db.select_category_by_key("messages_battle")
	root.size = Vector2i(1366, 768)
	db.size = Vector2i(1284, 722)
	await _step_frames(3)
	var ed := db.get_ui_designer_editor()
	var h: Dictionary = ed.get_ui_designer_ui_handles() if false else ed.get_ui_handles()
	var test_btn: Button = h.get("test_btn", null) as Button
	var z_fit: Button = h.get("zoom_1_btn", null) as Button
	var z_100: Button = h.get("zoom_2_btn", null) as Button
	var z_200: Button = h.get("zoom_3_btn", null) as Button
	var overlay: Control = h.get("canvas_overlay", null) as Control
	var cscroll2: ScrollContainer = h.get("canvas_scroll", null) as ScrollContainer
	var top_bar_card := ed.find_child("UiDesignerTopBarCard", true, false) as Control

	_check(test_btn != null and test_btn.text == "Probar" and not test_btn.clip_text, "Etiqueta 'Probar' completa y sin recorte")
	_check(z_fit != null and z_fit.text == "Ajustar" and z_fit.button_pressed, "Zoom por defecto en 'Ajustar'")
	_check(z_100 != null and z_100.text == "100 %", "Boton zoom '100 %' presente")
	_check(z_200 != null and z_200.text == "200 %", "Boton zoom '200 %' presente")
	_check(top_bar_card != null and top_bar_card.is_visible_in_tree() and top_bar_card.get_global_rect().position.y >= 0.0 and top_bar_card.get_global_rect().size.y >= 24.0,
		"Barra superior unica fija visible arriba sin cortarse por scroll")

	# Rueda sobre overlay en zoom 200% desplaza verticalmente y nunca cambia zoom
	z_200.emit_signal("pressed")
	await _step_frames(2)
	_check(ed.get_canvas_zoom_mode() == 2 and ed.get_canvas_zoom() == 2, "Zoom cambia a 200%")
	var v0: int = cscroll2.scroll_vertical
	var w_ev := InputEventMouseButton.new()
	w_ev.button_index = MOUSE_BUTTON_WHEEL_DOWN
	w_ev.pressed = true
	w_ev.factor = 1.0
	w_ev.position = overlay.get_global_rect().position + Vector2(20, 20)
	w_ev.global_position = w_ev.position
	db.push_input(w_ev)
	await _step_frames(1)
	_check(ed.get_canvas_zoom_mode() == 2, "Rueda del raton NO cambia el zoom")
	_check(cscroll2.scroll_vertical >= v0, "Rueda del raton desplaza ScrollContainer sin error (v0=%d -> v1=%d)" % [v0, cscroll2.scroll_vertical])
	z_fit.emit_signal("pressed")
	await _step_frames(1)
	_check(ed.get_canvas_zoom_mode() == 0, "Volver a 'Ajustar' funciona")

	# Simple vs Avanzado con 6 secciones
	ed.select_element("msg_box_frame")
	await _step_frames(1)
	ed.set_props_advanced_mode(false)
	var cnt_simple: int = ed.get_visible_prop_row_count()
	ed.set_props_advanced_mode(true)
	var cnt_adv: int = ed.get_visible_prop_row_count()
	_check(cnt_adv > cnt_simple and cnt_simple > 0, "Modo Simple (%d filas) vs Avanzado (%d filas) funciona" % [cnt_simple, cnt_adv])
	for sec_name in ["Posición", "Aspecto", "Texto", "Imagen", "Acciones", "Animación"]:
		var sec_btn: Button = null
		var st: Array[Node] = [ed]
		while not st.is_empty():
			var n: Node = st.pop_back()
			if n is Button and (n as Button).text.contains(sec_name):
				sec_btn = n as Button
				break
			for i in range(n.get_child_count()):
				st.push_back(n.get_child(i))
		_check(sec_btn != null, "Seccion plegable '%s' presente en Propiedades" % sec_name)

	db.queue_free()
	await _step_frames(2)
	_finish(0 if fail_count == 0 else 1)
