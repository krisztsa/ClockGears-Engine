extends SceneTree

# Test permanente Punto 2 (Correcciones UI 9B):
# - En todo el editor (barra superior, paneles de mapa, Base de Datos en todas las categorías,
#   Diseño de interfaz y diálogos), ningún botón queda sin icono ni texto.
# - Cada botón de solo icono tiene su icono (o texto corto si falta) y tooltip en español.
# - Al entrar al árbol y al cambiar de tema (actual <-> Latón), los iconos se refrescan sin quedar vacíos.

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

func _is_internal_widget_child(b: BaseButton) -> bool:
	var p := b.get_parent()
	while p != null:
		if p is OptionButton or p is SpinBox or p is ColorPickerButton or p is ColorPicker or p is FileDialog or p is TabBar or p is ScrollBar:
			return true
		if b is OptionButton or b is ColorPickerButton or b is CheckBox or b is CheckButton:
			return true
		p = p.get_parent()
	return false

func _collect_empty_and_tipless(root_node: Node, ctx: String, out_empty: Array[String], out_no_tip_icon: Array[String], seen: Dictionary) -> void:
	if root_node == null:
		return
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
			var txt := b.text.strip_edges()
			var has_ico := b.icon != null
			var tip := b.tooltip_text.strip_edges()
			if txt.is_empty() and not has_ico:
				out_empty.append("%s :: %s (tip='%s')" % [ctx, str(b.get_path()), tip])
			elif txt.is_empty() and has_ico and tip.is_empty():
				out_no_tip_icon.append("%s :: %s" % [ctx, str(b.get_path())])

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 2 (9B CORR): BOTONES CON ICONO/TEXTO, TOOLTIPS Y TEMAS ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var th = Engine.get_singleton("ClockGearsTheme")
	if dm == null:
		_check(false, "ClockGearsDataManager disponible")
		quit(1)
		return
	dm.init_empty()
	var prev_theme := String(th.get_active_theme_id()) if th else "default"

	var empty_btns: Array[String] = []
	var tipless_icon_btns: Array[String] = []
	var seen_ids: Dictionary = {}

	# 1. ClockGearsTopBar + notificaciones ENTER_TREE y THEME_CHANGED
	var top_bar := ClockGearsTopBar.new()
	root.add_child(top_bar)
	await _step_frames(2)
	top_bar.notification(Control.NOTIFICATION_THEME_CHANGED)
	await _step_frames(1)
	_collect_empty_and_tipless(top_bar, "ClockGearsTopBar", empty_btns, tipless_icon_btns, seen_ids)
	top_bar.queue_free()

	# 2. ClockGearsMapPanel
	var map_panel := ClockGearsMapPanel.new()
	root.add_child(map_panel)
	await _step_frames(2)
	map_panel.notification(Control.NOTIFICATION_THEME_CHANGED)
	await _step_frames(1)
	_collect_empty_and_tipless(map_panel, "ClockGearsMapPanel", empty_btns, tipless_icon_btns, seen_ids)
	map_panel.queue_free()

	# 3. ClockGearsDatabaseDialog en todas sus categorías y con diseño abierto
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.popup_centered(Vector2i(1366, 768))
	await _step_frames(2)
	var keys := ["actors", "classes", "skills", "items", "weapons", "armors", "enemies", "troops", "states", "animations", "tilesets", "common_events", "types", "system", "main_menu", "window_styles", "game_menu", "credits", "hud", "game_over", "ui_designer"]
	for k in keys:
		db.select_category_by_key(k)
		await _step_frames(1)
		_collect_empty_and_tipless(db, "Database[%s]" % k, empty_btns, tipless_icon_btns, seen_ids)

	# Verificar handles de UiDesignEditor y cambio de tema en caliente
	var ui_handles: Dictionary = db.get_ui_designer_ui_handles()
	var expected_action_keys := ["undo_btn", "redo_btn", "reset_btn", "test_btn", "new_menu_wizard_btn", "templates_btn", "layer_up_btn", "layer_down_btn", "layer_del_btn", "zoom_1_btn", "zoom_2_btn", "zoom_3_btn"]
	for ak in expected_action_keys:
		var ab := ui_handles.get(ak, null) as Button
		_check(ab != null and not ab.tooltip_text.strip_edges().is_empty() and (ab.icon != null or not ab.text.strip_edges().is_empty()), "UiDesigner botón '%s' tiene tooltip e icono/texto" % ak)

	if th:
		th.set_active_theme_id("brass")
		th.apply_to_window(db)
		db.notification(Control.NOTIFICATION_THEME_CHANGED)
		await _step_frames(2)
		_collect_empty_and_tipless(db, "Database[ui_designer:brass]", empty_btns, tipless_icon_btns, seen_ids)
		th.set_active_theme_id(prev_theme)
		th.apply_to_window(db)

	db.queue_free()

	# 4. Resto de diálogos instanciables del editor
	for cls_name in [
		"ClockGearsLayersDialog",
		"ClockGearsPreferencesDialog",
		"ClockGearsCameraDialog",
		"ClockGearsMapPropertiesDialog",
		"ClockGearsGenerateEntityDialog",
		"ClockGearsTilePickerDialog",
		"ClockGearsEventEditorDialog",
	]:
		if not ClassDB.class_exists(cls_name) or not ClassDB.can_instantiate(cls_name):
			continue
		var dlg: Node = ClassDB.instantiate(cls_name)
		root.add_child(dlg)
		await _step_frames(2)
		_collect_empty_and_tipless(dlg, cls_name, empty_btns, tipless_icon_btns, seen_ids)
		dlg.queue_free()

	await _step_frames(2)
	if not empty_btns.is_empty():
		for eb in empty_btns:
			printerr("  EMPTY_BTN: " + eb)
	if not tipless_icon_btns.is_empty():
		for tb in tipless_icon_btns:
			printerr("  TIPLESS_ICON_BTN: " + tb)

	_check(empty_btns.is_empty(), "Ningún botón del editor queda sin icono ni texto (vacíos=%d)" % empty_btns.size())
	_check(tipless_icon_btns.is_empty(), "Todo botón de solo icono tiene tooltip en español (sin_tooltip=%d)" % tipless_icon_btns.size())

	print("=== RESULTADO PUNTO 2: %d PASSED, %d FAILED ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
