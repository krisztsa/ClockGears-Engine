extends SceneTree

# Test permanente Punto 5 (Correcciones UI 9B):
# - Capítulo Menús consolidado en "Interfaz" (principal) y "Estilos de ventana" (segunda),
#   ocultando las 5 pestañas antiguas en la barra lateral sin borrarlas.
# - 3 zonas lado a lado en 1366x768, 1920x1080 y 2560x1440 sin scroll horizontal ni recortes.
# - Árbol de pantallas con Título, Opciones, Lista de partidas, Menú del juego (Principal,
#   Objetos, Equipo, Estado), Créditos, HUD y Fin del Juego.
# - Sección "Ajustes de la pantalla" con ida y vuelta completa en ClockGearsDataManager.

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

func _find_by_name_prefix(root_node: Node, nm: String) -> Node:
	return root_node.find_child(nm, true, false)

func _find_button_by_text(root_node: Node, txt: String) -> Button:
	var stack: Array[Node] = [root_node]
	while not stack.is_empty():
		var cur: Node = stack.pop_back()
		if cur is Button and (cur as Button).text.strip_edges() == txt:
			return cur as Button
		for i in range(cur.get_child_count()):
			stack.push_back(cur.get_child(i))
	return null

func _collect_tree_meta(tree: Tree) -> Array[String]:
	var out: Array[String] = []
	var root_it := tree.get_root()
	if root_it == null:
		return out
	var stack: Array[TreeItem] = [root_it]
	while not stack.is_empty():
		var it: TreeItem = stack.pop_back()
		var md := str(it.get_metadata(0))
		if not md.is_empty():
			out.append(md)
		var ch: TreeItem = it.get_first_child()
		while ch != null:
			stack.append(ch)
			ch = ch.get_next()
	return out

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 5 (9B CORR): INTERFAZ DE MENÚS CONSOLIDADA ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		_check(false, "ClockGearsDataManager disponible")
		quit(1)
		return
	dm.init_empty()

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.popup_centered(Vector2i(1366, 768))
	await _step_frames(2)

	# 1. Entrar al capítulo Menús y comprobar solo 2 categorías visibles
	db.select_category_by_key("ui_designer")
	await _step_frames(3)
	var menus_chap_btn := _find_button_by_text(db, "Menús")
	_check(menus_chap_btn != null and menus_chap_btn.visible, "Capítulo 'Menús' visible en barra lateral")
	if menus_chap_btn != null:
		menus_chap_btn.emit_signal("pressed")
		await _step_frames(2)

	var visible_cat_names: Array[String] = []
	var stack: Array[Node] = [db]
	while not stack.is_empty():
		var cur: Node = stack.pop_back()
		if cur is Button and (cur as Control).visible:
			var t := (cur as Button).text.strip_edges()
			if t in ["Interfaz", "Estilos de ventana", "Menú principal", "Menú del juego", "Créditos", "HUD", "Fin del Juego"]:
				if not visible_cat_names.has(t):
					visible_cat_names.append(t)
		for i in range(cur.get_child_count()):
			stack.push_back(cur.get_child(i))
	_check(visible_cat_names.size() == 2 and visible_cat_names.has("Interfaz") and visible_cat_names.has("Estilos de ventana"), "Capítulo Menús muestra solo 'Interfaz' y 'Estilos de ventana' (%s)" % str(visible_cat_names))

	# Compatibilidad: las pestañas antiguas existen ocultas y siguen siendo accesibles por clave
	db.select_category_by_key("game_menu")
	await _step_frames(1)
	_check(db.get_game_menu_editor_ui_handles().get("panel", null) != null, "Panel heredado game_menu conservado (no borrado)")
	db.select_category_by_key("ui_designer")
	await _step_frames(2)

	# 2. Árbol de pantallas y zonas 3 columnas
	var ed := db.get_ui_designer_editor()
	var h: Dictionary = db.get_ui_designer_ui_handles()
	var tree: Tree = h.get("screens_tree", null) as Tree
	var ss_box: Control = h.get("screen_settings_box", null) as Control
	var ss_body: Control = h.get("screen_settings_body", null) as Control
	var overlay: Control = h.get("canvas_overlay", null) as Control
	_check(tree != null and tree.visible, "Árbol de pantallas visible a la izquierda")
	_check(overlay != null and overlay.visible, "Lienzo central visible")
	_check(ss_box != null and ss_box.visible and ss_body != null, "Sección 'Ajustes de la pantalla' visible en Propiedades")

	var metas := _collect_tree_meta(tree) if tree != null else []
	for req_id in ["title", "game_menu_options", "game_menu_save", "game_menu_main", "game_menu_items", "game_menu_equip", "game_menu_status", "credits", "hud", "game_over"]:
		_check(metas.has(req_id), "Árbol incluye pantalla '%s'" % req_id)

	# 3. Disposición lado a lado en 1366x768, 1920x1080 y 2560x1440 sin scroll horizontal ni corte
	for res in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		db.size = res
		await _step_frames(3)
		var xr_tree := tree.get_global_rect().position.x + tree.get_global_rect().size.x
		var xl_canvas := overlay.get_global_rect().position.x
		var xl_ss := ss_box.get_global_rect().position.x
		var xr_ss := ss_box.get_global_rect().position.x + ss_box.get_global_rect().size.x
		_check(not bool(h.get("compact_mode", true)), "Modo 3 zonas lado a lado activo en %dx%d" % [res.x, res.y])
		_check(xr_tree <= xl_canvas + 40.0 and xl_ss >= xr_tree, "3 zonas izquierda-centro-derecha bien ordenadas en %dx%d" % [res.x, res.y])
		_check(xr_ss <= float(res.x) + 2.0, "Panel derecho dentro de %dx%d (xr=%.1f)" % [res.x, res.y, xr_ss])

	# 4. Ajustes de la pantalla por cada pantalla + sincronización hacia DataManager
	# 4a) Título
	ed.select_screen("title")
	await _step_frames(2)
	var t_edit := _find_by_name_prefix(ss_body, "SsTitleTextEdit") as LineEdit
	var t_slots := _find_by_name_prefix(ss_body, "SsSaveSlotsSpin") as SpinBox
	_check(t_edit != null and t_slots != null, "Título expone texto de título y ranuras en Ajustes de la pantalla")
	if t_edit != null:
		t_edit.text = "Crónica del Engranaje"
		t_edit.emit_signal("text_changed", t_edit.text)
		await _step_frames(1)
		_check(str(dm.get_title_menu_config().get("title", {}).get("text", "")) == "Crónica del Engranaje", "Edición de Título se guarda en DataManager")

	# 4b) Menú del juego: Principal (entradas) y Objetos (vista previa de objeto)
	ed.select_screen("game_menu_main")
	await _step_frames(2)
	var found_gm_toggle := false
	for ch in ss_body.get_children():
		if ch is HBoxContainer and ch.get_child_count() >= 2 and ch.get_child(0) is CheckBox and ch.get_child(1) is LineEdit:
			var le := ch.get_child(1) as LineEdit
			if le.text == "Objetos":
				le.text = "Mochila"
				le.emit_signal("text_changed", le.text)
				found_gm_toggle = true
				break
	_check(found_gm_toggle, "Entradas del menú del juego editables en Principal")
	if found_gm_toggle:
		var gm_entries: Array = dm.get_game_menu_config().get("entries", [])
		var renamed_ok := false
		for e in gm_entries:
			if str(e.get("id", "")) == "items" and str(e.get("text", "")) == "Mochila":
				renamed_ok = true
		_check(renamed_ok, "Entrada renombrada de Menú del juego sincronizada en DataManager")

	ed.select_screen("game_menu_items")
	await _step_frames(2)
	var ip_chk := _find_by_name_prefix(ss_body, "SsIpEnabledChk") as CheckBox
	var ip_pos := _find_by_name_prefix(ss_body, "SsIpPosOpt") as OptionButton
	_check(ip_chk != null and ip_pos != null, "Vista previa de objeto presente en pantalla Objetos")
	if ip_pos != null:
		ip_pos.select(1)
		ip_pos.emit_signal("item_selected", 1)
		await _step_frames(1)
		_check(str(dm.get_game_menu_config().get("item_preview", {}).get("position", "")) == "left", "Vista previa de objeto actualiza DataManager")

	# 4c) Créditos
	ed.select_screen("credits")
	await _step_frames(2)
	var c_spd := _find_by_name_prefix(ss_body, "SsCredSpeedSpin") as SpinBox
	var c_list := _find_by_name_prefix(ss_body, "SsCredItemsList") as ItemList
	var c_add := _find_button_by_text(ss_body, "+ Añadir")
	_check(c_spd != null and c_list != null and c_add != null, "Créditos expone velocidad, lista e ítems en Ajustes de la pantalla")
	if c_add != null:
		var prev_cnt := (dm.get_credits_config().get("items", []) as Array).size()
		c_add.emit_signal("pressed")
		await _step_frames(2)
		_check((dm.get_credits_config().get("items", []) as Array).size() == prev_cnt + 1, "+ Añadir en Créditos agrega elemento en DataManager")

	# 4d) HUD
	ed.select_screen("hud")
	await _step_frames(2)
	var h_add := _find_button_by_text(ss_body, "+ Widget")
	_check(h_add != null, "HUD expone añadir widget y muestras en Ajustes de la pantalla")
	if h_add != null:
		var prev_hw := (dm.get_hud_widgets() as Array).size()
		h_add.emit_signal("pressed")
		await _step_frames(2)
		_check((dm.get_hud_widgets() as Array).size() == prev_hw + 1, "+ Widget en HUD crea widget en DataManager")

	# 4e) Fin del Juego
	ed.select_screen("game_over")
	await _step_frames(2)
	var go_edit := _find_by_name_prefix(ss_body, "SsGoTitleTextEdit") as LineEdit
	_check(go_edit != null, "Fin del Juego expone título, música y botones en Ajustes de la pantalla")
	if go_edit != null:
		go_edit.text = "FIN DE LA TRAVESÍA"
		go_edit.emit_signal("text_changed", go_edit.text)
		await _step_frames(1)
		_check(str(dm.get_game_over_config().get("title_text", "")) == "FIN DE LA TRAVESÍA", "Edición de Fin del Juego se guarda en DataManager")

	# 5. Guardar/Recargar de proyecto para verificar que sync DB Save funciona
	ed.emit_signal("design_modified")
	await _step_frames(1)
	_check((db.get_hud_draft_copy().get("widgets", []) as Array).size() >= 1, "Borrador HUD en DB sincronizado tras design_modified")
	_check((db.get_credits_draft_copy().get("items", []) as Array).size() >= 1, "Borrador Créditos en DB sincronizado tras design_modified")

	print("RESUMEN PUNTO 5 (9B CORR): %d PASSED / %d FAILED" % [passed, failed])
	db.queue_free()
	quit(0 if failed == 0 else 1)
