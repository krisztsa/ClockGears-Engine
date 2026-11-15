extends SceneTree

var passed: int = 0
var failed: int = 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _step_frames(n: int = 3) -> void:
	for i in range(n):
		await process_frame

func _snapshot_data_dir(res_data_dir: String) -> Dictionary:
	var backup := {}
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				backup[full] = FileAccess.get_file_as_string(full)
			fname = da.get_next()
		da.list_dir_end()
	return backup

func _restore_data_dir(res_data_dir: String, backup: Dictionary) -> void:
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				if not backup.has(full):
					DirAccess.remove_absolute(ProjectSettings.globalize_path(full))
			fname = da.get_next()
		da.list_dir_end()
	for full in backup.keys():
		var wf := FileAccess.open(full, FileAccess.WRITE)
		if wf:
			wf.store_string(backup[full])

func _has_visible_hscroll(node: Node) -> bool:
	if node == null:
		return false
	if node is ScrollContainer:
		var sc := node as ScrollContainer
		if sc.is_visible_in_tree() and sc.horizontal_scroll_mode != ScrollContainer.SCROLL_MODE_DISABLED:
			var hb := sc.get_h_scroll_bar()
			if hb != null and hb.visible:
				return true
	for c in node.get_children():
		if _has_visible_hscroll(c):
			return true
	return false

func _find_clipped_control(node: Node, max_w: float) -> String:
	if node == null or node is SubViewport:
		return ""
	if node is Control:
		var ctrl := node as Control
		if ctrl.is_visible_in_tree():
			var gr := ctrl.get_global_rect()
			if gr.position.x < -1.0 or gr.end.x > max_w + 1.0:
				return "%s(x=%.1f..%.1f > %.1f)" % [ctrl.name, gr.position.x, gr.end.x, max_w]
	for c in node.get_children():
		var hit := _find_clipped_control(c, max_w)
		if not hit.is_empty():
			return hit
	return ""

func _select_option_by_meta(opt: OptionButton, target_meta: String) -> bool:
	if opt == null:
		return false
	for i in range(opt.get_item_count()):
		if String(opt.get_item_metadata(i)) == target_meta:
			opt.select(i)
			opt.emit_signal("item_selected", i)
			return true
	return false

func _drag_palette_to_canvas(pal_btn: Control, overlay: Control, canvas_pt: Vector2) -> bool:
	if pal_btn == null or overlay == null:
		return false
	var payload = pal_btn._get_drag_data(Vector2(8, 8)) if pal_btn.has_method("_get_drag_data") else pal_btn.get_drag_data(Vector2(8, 8))
	if not overlay.can_drop_data(canvas_pt, payload):
		return false
	overlay.drop_data(canvas_pt, payload)
	return true

func _push_mouse_to_overlay(win: Window, overlay: Control, ev: InputEventMouse, local_pt: Vector2) -> void:
	ev.position = overlay.get_global_rect().position + local_pt
	ev.global_position = ev.position
	win.push_input(ev)

func _mouse_drag(win: Window, overlay: Control, from_pt: Vector2, to_pt: Vector2) -> void:
	var mb_down := InputEventMouseButton.new()
	mb_down.button_index = MOUSE_BUTTON_LEFT
	mb_down.pressed = true
	_push_mouse_to_overlay(win, overlay, mb_down, from_pt)

	var mm := InputEventMouseMotion.new()
	mm.button_mask = MOUSE_BUTTON_MASK_LEFT
	mm.relative = to_pt - from_pt
	_push_mouse_to_overlay(win, overlay, mm, to_pt)

	var mb_up := InputEventMouseButton.new()
	mb_up.button_index = MOUSE_BUTTON_LEFT
	mb_up.pressed = false
	_push_mouse_to_overlay(win, overlay, mb_up, to_pt)

func _run_tests() -> void:
	print("=== TEST 9B TANDA 6B-1: EDITOR VISUAL DE INTERFAZ (LIENZO) ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton disponible")
	if dm == null:
		quit(1)
		return

	var res_data_dir := "res://clockgears_data"
	var backup := _snapshot_data_dir(res_data_dir)
	var prev_rt: Dictionary = dm.export_runtime_state()
	await _step_frames(1)

	# Imagen de prueba y variable/objeto base para enlazar
	var test_png := "user://_test_ui_6b1_badge.png"
	var img := Image.create(32, 32, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.28, 0.78, 0.96, 1.0))
	img.save_png(test_png)

	var items_arr: Array = dm.get_category_array(3)
	if items_arr.is_empty():
		items_arr.append({"id": 1, "name": "Poción Visual", "desc": "Cura"})
	dm.set_actor_runtime_hp(1, 65)
	dm.set_gold(350)
	dm.set_variable("llaves_sol", 7)

	# 1. Abrir Base de Datos -> Capítulo "Menús" -> "Diseño de interfaz"
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.popup_centered(Vector2i(1366, 768))
	await _step_frames(3)

	var chaps: Dictionary = db.get_chapter_structure_info()
	check(chaps.has("Menús") and Array(chaps["Menús"]).has("Diseño de interfaz"), "Capítulo 'Menús' incluye la categoría 'Diseño de interfaz'")

	db.select_category_by_key("ui_designer")
	await _step_frames(3)
	var editor = db.get_ui_designer_editor()
	var h: Dictionary = db.get_ui_designer_ui_handles()
	check(editor != null and editor.is_visible_in_tree() and not h.is_empty(), "select_category_by_key('ui_designer') abre ClockGearsUiDesignEditor")

	# 2. Responsive 1366x768 y 1920x1080
	var ui_panel: Control = h["panel"]
	var fit_1366: Dictionary = db.check_layout_fits_window(Vector2i(1366, 768))
	await _step_frames(2)
	check(bool(fit_1366.get("fits", false)), "Base de Datos reporta encaje en 1366x768 (%s)" % str(fit_1366))
	check(not _has_visible_hscroll(ui_panel), "Sin scroll horizontal visible en editor visual a 1366x768")
	var clip_1366 := _find_clipped_control(ui_panel, 1366.0)
	check(clip_1366.is_empty(), "Sin controles recortados a 1366x768 (%s)" % clip_1366)

	db.size = Vector2i(1920, 1080)
	db.adapt_to_window_size(Vector2i(1920, 1080))
	await _step_frames(3)
	var fit_1920: Dictionary = db.check_layout_fits_window(Vector2i(1920, 1080))
	check(bool(fit_1920.get("fits", false)), "Base de Datos reporta encaje en 1920x1080 (%s)" % str(fit_1920))
	check(not _has_visible_hscroll(ui_panel), "Sin scroll horizontal visible en editor visual a 1920x1080")

	# Volver a 1366x768 para seguir probando en tamaño estándar de portátil
	db.size = Vector2i(1366, 768)
	db.adapt_to_window_size(Vector2i(1366, 768))
	await _step_frames(2)

	# 3. Selector de las 7 pantallas
	var screen_opt: OptionButton = h["screen_opt"]
	var expected_screens := ["title", "game_menu_main", "game_menu_items", "game_menu_equip", "game_menu_status", "credits", "hud"]
	var listed_screens := []
	for i in range(screen_opt.get_item_count()):
		listed_screens.append(String(screen_opt.get_item_metadata(i)))
	var all_7 := true
	for sid in expected_screens:
		if not listed_screens.has(sid):
			all_7 = false
	check(all_7 and listed_screens.size() == 7, "Selector contiene las 7 pantallas requeridas (%s)" % str(listed_screens))

	# 4. Zoom con botones enteros y rueda del ratón bloqueada
	var overlay: Control = h["canvas_overlay"]
	var canvas_stage: Control = h["canvas_stage"]
	var zoom_1_btn: Button = h["zoom_1_btn"]
	var zoom_2_btn: Button = h["zoom_2_btn"]
	var zoom_3_btn: Button = h["zoom_3_btn"]

	check(editor.get_canvas_zoom() == 1 and canvas_stage.size == Vector2(960, 540), "Lienzo inicia en zoom 1x (960x540)")
	var wheel_up := InputEventMouseButton.new()
	wheel_up.button_index = MOUSE_BUTTON_WHEEL_UP
	wheel_up.pressed = true
	_push_mouse_to_overlay(db, overlay, wheel_up, Vector2(200, 200))
	var wheel_down := InputEventMouseButton.new()
	wheel_down.button_index = MOUSE_BUTTON_WHEEL_DOWN
	wheel_down.pressed = true
	_push_mouse_to_overlay(db, overlay, wheel_down, Vector2(200, 200))
	await _step_frames(1)
	check(editor.get_canvas_zoom() == 1 and canvas_stage.size == Vector2(960, 540), "La rueda del ratón NO cambia el zoom del lienzo")

	zoom_2_btn.emit_signal("pressed")
	await _step_frames(1)
	check(editor.get_canvas_zoom() == 2 and canvas_stage.custom_minimum_size == Vector2(1920, 1080), "Botón 2x ajusta lienzo a múltiplo entero 1920x1080")
	zoom_3_btn.emit_signal("pressed")
	await _step_frames(1)
	check(editor.get_canvas_zoom() == 3 and canvas_stage.custom_minimum_size == Vector2(2880, 1620), "Botón 3x ajusta lienzo a múltiplo entero 2880x1620")
	zoom_1_btn.emit_signal("pressed")
	await _step_frames(1)
	check(editor.get_canvas_zoom() == 1 and canvas_stage.custom_minimum_size == Vector2(960, 540), "Botón 1x devuelve el lienzo a 960x540")

	# 5. Paleta lateral completa (9 elementos) y arrastrar al lienzo en Título
	var pal: Dictionary = h["palette_btns"]
	var expected_types := ["image", "text", "button", "frame", "bar", "hearts", "currency", "item", "variable"]
	var pal_ok := pal.size() == 9
	for et in expected_types:
		if not pal.has(et):
			pal_ok = false
	check(pal_ok, "Paleta contiene los 9 elementos requeridos (%s)" % str(pal.keys()))

	_select_option_by_meta(screen_opt, "title")
	await _step_frames(2)
	check(editor.get_current_screen() == "title", "Pantalla activa cambiada a Título")

	var dropped_text := _drag_palette_to_canvas(pal["text"], overlay, Vector2(120, 80))
	await _step_frames(1)
	var text_id: String = editor.get_selected_element_id()
	check(dropped_text and not text_id.is_empty(), "Arrastrar 'Texto' de la paleta al lienzo crea elemento '%s'" % text_id)

	var design_layer = h["design_layer"]
	var text_ctrl: Control = design_layer.get_element_control(text_id)
	check(text_ctrl != null and design_layer.is_visible_in_tree(), "Elemento creado se renderiza en el ClockGearsUiDesignLayer del lienzo")

	var dropped_img := _drag_palette_to_canvas(pal["image"], overlay, Vector2(400, 100))
	await _step_frames(1)
	var img_id: String = editor.get_selected_element_id()
	var dropped_btn := _drag_palette_to_canvas(pal["button"], overlay, Vector2(140, 260))
	await _step_frames(1)
	var btn_id: String = editor.get_selected_element_id()
	check(dropped_img and dropped_btn and dm.get_ui_screen_elements("title").size() >= 3, "Arrastrados imagen, texto y botón al Título")

	# 6. Seleccionar con clic, mover arrastrando (píxeles enteros) y redimensionar con tiradores
	var img_ctrl: Control = design_layer.get_element_control(img_id)
	var img_center := img_ctrl.position + img_ctrl.size * 0.5
	var mb_click := InputEventMouseButton.new()
	mb_click.button_index = MOUSE_BUTTON_LEFT
	mb_click.pressed = true
	_push_mouse_to_overlay(db, overlay, mb_click, img_center)
	var mb_rel := InputEventMouseButton.new()
	mb_rel.button_index = MOUSE_BUTTON_LEFT
	mb_rel.pressed = false
	_push_mouse_to_overlay(db, overlay, mb_rel, img_center)
	await _step_frames(1)
	check(editor.get_selected_element_id() == img_id, "Clic en el lienzo selecciona el elemento '%s'" % img_id)

	var before_move: Dictionary = dm.get_ui_element("title", img_id)
	var ox0 := int(before_move.get("offset_x", 0))
	var oy0 := int(before_move.get("offset_y", 0))
	_mouse_drag(db, overlay, img_center, img_center + Vector2(37.4, 21.8))
	await _step_frames(2)
	var after_move: Dictionary = dm.get_ui_element("title", img_id)
	check(int(after_move.get("offset_x", 0)) == ox0 + 37 and int(after_move.get("offset_y", 0)) == oy0 + 22,
		"Arrastrar elemento lo mueve en píxeles enteros exactos (%d,%d -> %d,%d)" % [ox0, oy0, int(after_move.get("offset_x", 0)), int(after_move.get("offset_y", 0))])

	# Redimensionar con tirador inferior derecho (HANDLE_BR = 8)
	var br_rect: Rect2 = overlay.get_handle_widget_rect(8)
	var br_center := br_rect.position + br_rect.size * 0.5
	var w0 := int(after_move.get("width", 0))
	var h0 := int(after_move.get("height", 0))
	_mouse_drag(db, overlay, br_center, br_center + Vector2(30, 18))
	await _step_frames(2)
	var after_resize: Dictionary = dm.get_ui_element("title", img_id)
	check(int(after_resize.get("width", 0)) == w0 + 30 and int(after_resize.get("height", 0)) == h0 + 18,
		"Arrastrar tirador esquina redimensiona en píxeles enteros (%dx%d -> %dx%d)" % [w0, h0, int(after_resize.get("width", 0)), int(after_resize.get("height", 0))])

	# 7. Propiedades en vivo: texto UTF-8, imagen importada, estilo ventana, ancla 3x3, acción botón y orden foco
	editor.select_element(text_id)
	await _step_frames(1)
	var prop_text_edit: LineEdit = h["prop_text_edit"]
	var utf8_phrase := "¡Hola! á é í ó ú ñ ¿Qué tal?"
	prop_text_edit.text = utf8_phrase
	prop_text_edit.emit_signal("text_changed", utf8_phrase)
	await _step_frames(1)
	check(design_layer.get_element_displayed_text(text_id) == utf8_phrase, "Cambio de texto UTF-8 se ve al instante en ClockGearsUiDesignLayer")

	var anchor_btns: Array = h["anchor_btns"]
	check(anchor_btns.size() == 9, "Selector de anclaje 3x3 tiene 9 botones")
	anchor_btns[4].emit_signal("pressed") # center
	await _step_frames(1)
	check(String(dm.get_ui_element("title", text_id).get("anchor", "")) == "center", "Botón central del 3x3 asigna ancla 'center'")

	var prop_ox: SpinBox = h["prop_ox_spin"]
	var prop_oy: SpinBox = h["prop_oy_spin"]
	prop_ox.value = -10
	prop_ox.emit_signal("value_changed", -10.0)
	prop_oy.value = 12
	prop_oy.emit_signal("value_changed", 12.0)
	await _step_frames(1)
	var te_data: Dictionary = dm.get_ui_element("title", text_id)
	check(int(te_data.get("offset_x", 0)) == -10 and int(te_data.get("offset_y", 0)) == 12, "SpinBox X/Y actualiza desplazamiento")

	# Importar imagen para el elemento imagen
	editor.select_element(img_id)
	await _step_frames(1)
	var imported_res: String = db.import_menu_asset_for_ui_designer(ProjectSettings.globalize_path(test_png))
	await _step_frames(2)
	check(not imported_res.is_empty() and String(dm.get_ui_element("title", img_id).get("image_path", "")) == imported_res,
		"Imagen importada asignada al elemento del lienzo (%s)" % imported_res)

	# Configurar botón: acción del catálogo ("new_game"), estilo y orden de foco
	editor.select_element(btn_id)
	await _step_frames(1)
	var prop_action_opt: OptionButton = h["prop_action_opt"]
	var prop_style_opt: OptionButton = h["prop_style_opt"]
	var prop_focus_spin: SpinBox = h["prop_focus_spin"]
	var act_ok := _select_option_by_meta(prop_action_opt, "new_game")
	prop_focus_spin.value = 3
	prop_focus_spin.emit_signal("value_changed", 3.0)
	prop_text_edit.text = "Empezar ¡Ya!"
	prop_text_edit.emit_signal("text_changed", "Empezar ¡Ya!")
	if prop_style_opt.get_item_count() > 1:
		prop_style_opt.select(1)
		prop_style_opt.emit_signal("item_selected", 1)
	await _step_frames(1)
	var btn_data: Dictionary = dm.get_ui_element("title", btn_id)
	check(act_ok and String(btn_data.get("action", "")) == "new_game" and int(btn_data.get("focus_order", 0)) == 3,
		"Botón guarda acción del catálogo ('new_game') y focus_order=3")

	# 8. Lista de capas: ojo de visibilidad, subir/bajar capa y eliminar + Deshacer/Rehacer
	var layer_up_btn: Button = h["layer_up_btn"]
	var layer_down_btn: Button = h["layer_down_btn"]
	var layer_del_btn: Button = h["layer_del_btn"]
	var l0 := int(dm.get_ui_element("title", btn_id).get("layer_order", 0))
	layer_up_btn.emit_signal("pressed")
	await _step_frames(1)
	var l1 := int(dm.get_ui_element("title", btn_id).get("layer_order", 0))
	layer_down_btn.emit_signal("pressed")
	await _step_frames(1)
	check(l1 == l0 + 1 and int(dm.get_ui_element("title", btn_id).get("layer_order", 0)) == l0,
		"Botones Subir/Bajar capa reordenan layer_order en vivo")

	# Ojo de visibilidad en layers_list_vbox
	var layers_vbox: VBoxContainer = h["layers_list_vbox"]
	check(layers_vbox.get_child_count() >= 3, "Lista de capas lista los 3 elementos del Título")
	editor.toggle_element_visibility(text_id)
	await _step_frames(1)
	var tc_hidden: Control = design_layer.get_element_control(text_id)
	check(not bool(dm.get_ui_element("title", text_id).get("visible", true)) and (tc_hidden == null or not tc_hidden.visible),
		"Ojo de visibilidad oculta el elemento en el lienzo al instante")
	editor.toggle_element_visibility(text_id)
	await _step_frames(1)

	# Eliminar + Deshacer / Rehacer
	var count_before_del: int = dm.get_ui_screen_elements("title").size()
	var frame_id: String = editor.add_element_at("frame", Vector2i(500, 300))
	await _step_frames(1)
	check(dm.get_ui_screen_elements("title").size() == count_before_del + 1, "Elemento 'Marco' temporal añadido")
	editor.select_element(frame_id)
	layer_del_btn.emit_signal("pressed")
	await _step_frames(1)
	check(dm.get_ui_screen_elements("title").size() == count_before_del, "Botón eliminar quita elemento seleccionado")

	var undo_btn: Button = h["undo_btn"]
	var redo_btn: Button = h["redo_btn"]
	undo_btn.emit_signal("pressed")
	await _step_frames(1)
	check(dm.get_ui_screen_elements("title").size() == count_before_del + 1, "Botón Deshacer (pila 6A) restaura el elemento borrado")
	redo_btn.emit_signal("pressed")
	await _step_frames(1)
	check(dm.get_ui_screen_elements("title").size() == count_before_del, "Botón Rehacer (pila 6A) vuelve a aplicar el borrado")

	# 9. Editar HUD desde la paleta (barra, corazones, monedas, objeto, variable + enlace)
	_select_option_by_meta(screen_opt, "hud")
	await _step_frames(2)
	check(editor.get_current_screen() == "hud", "Pantalla activa cambiada a HUD")
	var hud_bar_ok := _drag_palette_to_canvas(pal["bar"], overlay, Vector2(40, 40))
	var hud_hearts_ok := _drag_palette_to_canvas(pal["hearts"], overlay, Vector2(40, 96))
	var hud_cur_ok := _drag_palette_to_canvas(pal["currency"], overlay, Vector2(720, 36))
	var hud_item_ok := _drag_palette_to_canvas(pal["item"], overlay, Vector2(720, 84))
	var hud_var_ok := _drag_palette_to_canvas(pal["variable"], overlay, Vector2(720, 132))
	await _step_frames(2)
	check(hud_bar_ok and hud_hearts_ok and hud_cur_ok and hud_item_ok and hud_var_ok and dm.get_ui_screen_elements("hud").size() >= 5,
		"Widgets HUD arrastrados desde paleta aparecen en el lienzo")

	var hud_var_id: String = editor.get_selected_element_id()
	var prop_var_edit: LineEdit = h["prop_var_edit"]
	var prop_binding_opt: OptionButton = h["prop_binding_opt"]
	_select_option_by_meta(prop_binding_opt, "variable")
	prop_var_edit.text = "llaves_sol"
	prop_var_edit.emit_signal("text_changed", "llaves_sol")
	await _step_frames(1)
	check(String(design_layer.get_element_displayed_text(hud_var_id)).find("7") >= 0,
		"Enlace a variable 'llaves_sol'=7 se resuelve en vivo dentro del lienzo HUD")

	# 10. Botón Probar en Título y Esc cierra
	_select_option_by_meta(screen_opt, "title")
	await _step_frames(2)
	var test_btn: Button = h["test_btn"]
	test_btn.emit_signal("pressed")
	await _step_frames(3)
	var tp_info: Dictionary = editor.get_test_popup_info()
	var t_win: Window = tp_info.get("window", null)
	var t_title = tp_info.get("title", null)
	check(bool(tp_info.get("open", false)) and t_title != null and t_title.is_custom_ui_active(),
		"Botón Probar abre pantalla real con el mismo diseño custom del lienzo")
	var esc_ev := InputEventKey.new()
	esc_ev.keycode = KEY_ESCAPE
	esc_ev.pressed = true
	t_win.emit_signal("window_input", esc_ev)
	await _step_frames(2)
	check(not bool(editor.get_test_popup_info().get("open", false)), "Esc cierra la ventana de Probar de forma segura")

	# 11. Guardar y reabrir conserva diseño y texto UTF-8
	dm.save_to_disk()
	dm.init_empty()
	dm.load_from_disk()
	db.select_category_by_key("ui_designer")
	_select_option_by_meta(screen_opt, "title")
	await _step_frames(2)
	check(dm.has_custom_ui_design("title") and design_layer.get_element_displayed_text(text_id) == utf8_phrase,
		"Guardar y reabrir conserva el diseño del Título y texto UTF-8 íntegro")

	# 12. Restablecer a diseño por defecto (con diálogo de confirmación)
	var reset_btn: Button = h["reset_btn"]
	var reset_dlg: ConfirmationDialog = h["reset_confirm_dialog"]
	reset_btn.emit_signal("pressed")
	await _step_frames(1)
	check(reset_dlg != null and reset_dlg.visible, "Restablecer abre diálogo de confirmación")
	reset_dlg.emit_signal("confirmed")
	await _step_frames(2)
	check(not dm.has_custom_ui_design("title"), "Confirmar Restablecer vuelve Título al diseño por defecto")

	# Limpieza
	if not imported_res.is_empty() and FileAccess.file_exists(imported_res):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(imported_res))
	db.queue_free()
	dm.import_runtime_state(prev_rt)
	_restore_data_dir(res_data_dir, backup)
	dm.load_all(res_data_dir)
	await _step_frames(1)

	print("=== RESULTADO 9B TANDA 6B-1: %d PASS, %d FAIL ===" % [passed, failed])
	quit(0 if failed == 0 else 1)

func _init() -> void:
	call_deferred("_run_tests")
