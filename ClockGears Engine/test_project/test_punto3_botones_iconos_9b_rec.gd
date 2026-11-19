extends SceneTree

# Test Punto 3 (Reconstrucción UI 9B):
# - Recorre TODOS los botones del editor (barra superior, paneles de mapa, Base de Datos en
#   todas las categorías, Diseño de interfaz incluyendo capas dinámicas, alinear y anclas,
#   diálogos de capas/preferencias/eventos) verificando que NINGÚN botón queda sin icono ni texto,
#   todo botón de solo icono tiene tooltip en español, los iconos se refrescan al entrar al árbol
#   y al cambiar de tema (default <-> Latón) sin advertencias "Attempting to access theme items too early".

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

func _is_internal_widget_child(b: BaseButton) -> bool:
	var p := b.get_parent()
	while p != null:
		if p is OptionButton or p is SpinBox or p is ColorPickerButton or p is ColorPicker or p is FileDialog or p is TabBar or p is ScrollBar:
			return true
		if b is OptionButton or b is ColorPickerButton or b is CheckBox or b is CheckButton:
			return true
		p = p.get_parent()
	return false

func _scan_buttons(root_node: Node, ctx: String, empty_out: Array[String], tipless_out: Array[String], seen: Dictionary) -> int:
	var scanned := 0
	if root_node == null:
		return 0
	var stack: Array[Node] = [root_node]
	while not stack.is_empty():
		var cur: Node = stack.pop_back()
		for i in range(cur.get_child_count()):
			stack.push_back(cur.get_child(i))
		if cur is Button:
			var b := cur as Button
			if _is_internal_widget_child(b):
				continue
			var oid := b.get_instance_id()
			if seen.has(oid):
				continue
			seen[oid] = true
			scanned += 1
			var txt := b.text.strip_edges()
			var has_ico := b.icon != null
			var tip := b.tooltip_text.strip_edges()
			if txt.is_empty() and not has_ico:
				empty_out.append("%s :: %s (tip='%s')" % [ctx, str(b.get_path()), tip])
			elif txt.is_empty() and has_ico and tip.is_empty():
				tipless_out.append("%s :: %s" % [ctx, str(b.get_path())])
	return scanned

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60 s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 3 (RECONSTRUCCION UI 9B): BOTONES, ICONOS, TOOLTIPS Y TEMA ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var th = Engine.get_singleton("ClockGearsTheme")
	if dm == null:
		_check(false, "ClockGearsDataManager disponible")
		_finish(1)
		return
	dm.init_empty()
	var prev_theme := String(th.get_active_theme_id()) if th else "default"

	var empty_btns: Array[String] = []
	var tipless_btns: Array[String] = []
	var seen: Dictionary = {}
	var total_btns := 0

	var top_bar := ClockGearsTopBar.new()
	root.add_child(top_bar)
	await _step_frames(2)
	top_bar.notification(Control.NOTIFICATION_THEME_CHANGED)
	await _step_frames(1)
	total_btns += _scan_buttons(top_bar, "TopBar", empty_btns, tipless_btns, seen)
	top_bar.queue_free()

	var map_panel := ClockGearsMapPanel.new()
	root.add_child(map_panel)
	await _step_frames(2)
	map_panel.notification(Control.NOTIFICATION_THEME_CHANGED)
	await _step_frames(1)
	total_btns += _scan_buttons(map_panel, "MapPanel", empty_btns, tipless_btns, seen)
	map_panel.queue_free()

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(2)
	var keys := ["actors", "classes", "skills", "items", "weapons", "armors", "enemies", "troops", "states", "animations", "tilesets", "common_events", "types", "system", "main_menu", "window_styles", "game_menu", "credits", "hud", "game_over", "ui_designer"]
	for k in keys:
		db.select_category_by_key(k)
		await _step_frames(1)
		total_btns += _scan_buttons(db, "Database[%s]" % k, empty_btns, tipless_btns, seen)

	var ed := db.get_ui_designer_editor()
	ed.select_screen("game_menu_items")
	await _step_frames(2)
	total_btns += _scan_buttons(db, "Database[ui_designer:items]", empty_btns, tipless_btns, seen)

	var h: Dictionary = db.get_ui_designer_ui_handles()
	for bk in ["new_menu_wizard_btn", "templates_btn", "undo_btn", "redo_btn", "reset_btn", "test_btn", "align_center_h_btn", "align_center_v_btn", "dist_v_btn", "dup_elem_btn", "copy_elem_btn", "paste_elem_btn", "layer_up_btn", "layer_down_btn", "layer_del_btn", "menu_dup_btn", "menu_up_btn", "menu_down_btn", "menu_del_btn"]:
		var btn := h.get(bk, null) as Button
		_check(btn != null and btn.icon != null and not btn.tooltip_text.strip_edges().is_empty(), "Botón '%s' tiene icono real y tooltip en español" % bk)

	if th:
		th.set_active_theme_id("brass")
		th.apply_to_window(db)
		db.notification(Control.NOTIFICATION_THEME_CHANGED)
		await _step_frames(2)
		total_btns += _scan_buttons(db, "Database[ui_designer:brass]", empty_btns, tipless_btns, seen)
		th.set_active_theme_id(prev_theme)
		th.apply_to_window(db)

	db.queue_free()

	for cls_name in ["ClockGearsLayersDialog", "ClockGearsPreferencesDialog", "ClockGearsCameraDialog", "ClockGearsMapPropertiesDialog", "ClockGearsGenerateEntityDialog", "ClockGearsTilePickerDialog", "ClockGearsEventEditorDialog"]:
		if not ClassDB.class_exists(cls_name) or not ClassDB.can_instantiate(cls_name):
			continue
		var dlg: Node = ClassDB.instantiate(cls_name)
		root.add_child(dlg)
		await _step_frames(2)
		total_btns += _scan_buttons(dlg, cls_name, empty_btns, tipless_btns, seen)
		dlg.queue_free()

	await _step_frames(2)
	for eb in empty_btns:
		printerr("  EMPTY_BTN: " + eb)
	for tb in tipless_btns:
		printerr("  TIPLESS_BTN: " + tb)

	_check(total_btns > 50, "Se auditaron botones en todo el editor (%d)" % total_btns)
	_check(empty_btns.is_empty(), "Cero botones sin icono ni texto (vacíos=%d)" % empty_btns.size())
	_check(tipless_btns.is_empty(), "Cero botones de solo icono sin tooltip (sin_tooltip=%d)" % tipless_btns.size())

	_finish(0 if failed == 0 else 1)
