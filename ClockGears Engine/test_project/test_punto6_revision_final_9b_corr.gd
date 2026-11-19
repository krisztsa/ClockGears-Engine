extends SceneTree

# Test permanente Punto 6 (Correcciones UI 9B):
# - Revisión final en 1366x768, 1920x1080 y 2560x1440:
#   1. Barra superior completa (8 píldoras de menú y barra de herramientas) dentro de ventana, sin solapes.
#   2. Base de Datos al tamaño mínimo: ningún control visible de "Interfaz" ni de "Estilos de ventana"
#      se sale horizontalmente de su contenedor ni del diálogo.
#   3. Proyectos/menús existentes cargan sin pérdidas ni errores.

var passed: int = 0
var failed: int = 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _check_children_within_horizontal_bounds(container: Control, label: String, tol: float = 3.0) -> void:
	var crect := container.get_global_rect()
	var clipped := []
	var stack: Array[Node] = [container]
	while not stack.is_empty():
		var cur: Node = stack.pop_back()
		if cur is Control and cur != container:
			var c := cur as Control
			if not c.is_visible_in_tree():
				continue
			# Los ScrollContainer internos recortan/scrollan verticalmente, comprobar su ancho respecto al contenedor
			var r := c.get_global_rect()
			if r.size.x > 1.0 and r.size.y > 1.0:
				if r.position.x < crect.position.x - tol or (r.position.x + r.size.x) > (crect.position.x + crect.size.x + tol):
					# Ignorar popups / subventanas / tooltips / barras de scroll internas del Tree/ItemList si no tienen ancho fuera
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
	_check(clipped.is_empty(), "%s: sin controles fuera/con scroll horizontal (%d hallados%s)" % [label, clipped.size(), (": " + str(clipped.slice(0, 4))) if not clipped.is_empty() else ""])

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 6 (9B CORR): REVISIÓN FINAL 16:9 MULTI-RESOLUCIÓN ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		_check(false, "ClockGearsDataManager disponible")
		quit(1)
		return

	# 1. Proyectos / menús existentes abren intactos
	var loaded := bool(dm.load_all("res://clockgears_data"))
	_check(loaded, "Proyecto existente test_project/clockgears_data carga correctamente")
	_check(dm.get_window_styles().size() >= 1, "Estilos de ventana existentes conservados (%d)" % dm.get_window_styles().size())
	_check((dm.get_title_menu_config().get("entries", []) as Array).size() >= 4, "Configuración de Título existente conservada")

	# 2. Barra de menús y barra de herramientas caben completas en 1366x768, 1920x1080 y 2560x1440
	var vp := SubViewport.new()
	vp.size = Vector2i(1366, 220)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(vp)

	var bars_vbox := VBoxContainer.new()
	bars_vbox.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	vp.add_child(bars_vbox)

	var layers_dlg := ClockGearsLayersDialog.new()
	root.add_child(layers_dlg)
	await _step_frames(1)
	var bar_handles: Dictionary = layers_dlg.get_ui_handles()
	var title_row: HBoxContainer = bar_handles.get("title_bar_row", null) as HBoxContainer
	_check(title_row != null, "Barra superior de título y menús creada")
	if title_row != null:
		bars_vbox.add_child(title_row)
	var top_bar := ClockGearsTopBar.new()
	bars_vbox.add_child(top_bar)
	await _step_frames(2)

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.popup_centered(Vector2i(1000, 640))
	await _step_frames(3)
	var min_sz := db.get_min_size()
	if min_sz.x < 1000:
		min_sz = Vector2i(1000, 640)

	for res in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		vp.size = Vector2i(res.x, 220)
		root.size = res
		await _step_frames(2)
		if title_row != null:
			var mbar: MenuBar = title_row.get_node_or_null("MainMenuBar") as MenuBar
			var vis_menus := 0
			if mbar != null:
				for mi in range(mbar.get_menu_count()):
					if not mbar.is_menu_hidden(mi):
						vis_menus += 1
			_check(mbar != null and vis_menus == 8, "MainMenuBar muestra los 8 menús visibles en %dx%d" % [res.x, res.y])
			if mbar != null:
				var mrect := mbar.get_global_rect()
				_check(mrect.position.x + mrect.size.x <= float(res.x) + 1.0, "MainMenuBar cabe completo en %dx%d (xr=%.0f)" % [res.x, res.y, mrect.position.x + mrect.size.x])
		var trect := top_bar.get_global_rect()
		_check(trect.position.x + trect.size.x <= float(res.x) + 1.0, "ClockGearsTopBar cabe completa en %dx%d (xr=%.0f)" % [res.x, res.y, trect.position.x + trect.size.x])

		# Base de Datos al tamaño mínimo dentro de res
		db.size = min_sz
		await _step_frames(2)

		db.select_category_by_key("ui_designer")
		await _step_frames(3)
		var ed := db.get_ui_designer_editor()
		var h: Dictionary = db.get_ui_designer_ui_handles()
		var panel: Control = h.get("panel", null) as Control
		_check(panel != null, "Panel Interfaz accesible en %dx%d (DB min=%s)" % [res.x, res.y, str(min_sz)])
		if ed != null and panel != null:
			for sid in ["title", "game_menu_main", "game_menu_items", "credits", "hud", "game_over"]:
				ed.select_screen(sid)
				await _step_frames(2)
				_check_children_within_horizontal_bounds(panel, "%dx%d Interfaz[%s] (DB min)" % [res.x, res.y, sid])

		db.select_category_by_key("window_styles")
		await _step_frames(3)
		var ws_panel: Control = db.get_window_styles_ui_handles().get("panel", null) as Control
		_check(ws_panel != null and ws_panel.visible, "Estilos de ventana visible en %dx%d (DB min)" % [res.x, res.y])
		if ws_panel != null:
			_check_children_within_horizontal_bounds(ws_panel, "%dx%d Estilos de ventana (DB min)" % [res.x, res.y])

	print("RESUMEN PUNTO 6 (9B CORR): %d PASSED / %d FAILED" % [passed, failed])
	layers_dlg.queue_free()
	vp.queue_free()
	db.queue_free()
	quit(0 if failed == 0 else 1)
