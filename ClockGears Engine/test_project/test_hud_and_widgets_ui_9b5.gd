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

func _find_button_by_text(node: Node, needle: String) -> Button:
	if node == null:
		return null
	if node is Button:
		var b := node as Button
		if not (b is OptionButton) and b.text.find(needle) >= 0:
			return b
	for c in node.get_children():
		var res := _find_button_by_text(c, needle)
		if res != null:
			return res
	return null

func _select_option_by_meta(opt: OptionButton, target_meta: String) -> bool:
	if opt == null:
		return false
	for i in range(opt.get_item_count()):
		if String(opt.get_item_metadata(i)) == target_meta:
			opt.select(i)
			opt.emit_signal("item_selected", i)
			return true
	return false

func _select_option_by_vec2i(opt: OptionButton, target_sz: Vector2i) -> bool:
	if opt == null:
		return false
	for i in range(opt.get_item_count()):
		var md = opt.get_item_metadata(i)
		if typeof(md) == TYPE_VECTOR2I and md == target_sz:
			opt.select(i)
			opt.emit_signal("item_selected", i)
			return true
	return false

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

func _run_tests() -> void:
	print("=== TEST 9B TANDA 5 UI: EDITOR DEL HUD, VISTA PREVIA 16:9 Y DIÁLOGO MOSTRAR/OCULTAR HUD ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton disponible")
	if dm == null:
		return

	var res_data_dir := "res://clockgears_data"
	var backup := _snapshot_data_dir(res_data_dir)
	var prev_rt: Dictionary = dm.export_runtime_state()
	await _step_frames(1)

	# Crear textura PNG de prueba y datos BD (objeto, variable, interruptor)
	var img_res := "res://_test_hud_badge_9b5.png"
	var img := Image.create(32, 32, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.95, 0.72, 0.22, 1.0))
	img.save_png(ProjectSettings.globalize_path(img_res))

	var items_arr: Array = dm.get_category_array(3) # CAT_ITEMS
	var had_potion := false
	for it in items_arr:
		if typeof(it) == TYPE_DICTIONARY and int(it.get("id", 0)) == 1:
			had_potion = true
			if String(it.get("name", "")).strip_edges().is_empty():
				it["name"] = "Poción HUD"
			break
	if not had_potion:
		items_arr.append({"id": 1, "name": "Poción HUD", "desc": "Cura HP"})
	dm.set_category_array(3, items_arr)
	var vars_dict: Dictionary = dm.get_variables_names()
	vars_dict["1"] = "PuntosMision"
	dm.set_variables_names(vars_dict)
	var sw_dict: Dictionary = dm.get_switches_names()
	sw_dict["1"] = "MostrarRadar"
	dm.set_switches_names(sw_dict)
	dm.set_hud_config({"schema_version": 1, "widgets": []})
	dm.save_all(res_data_dir)

	# 1. Base de Datos -> Capítulo Menús -> Categoría HUD
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(2)

	var chap_info: Dictionary = db.get_chapter_structure_info()
	var found_hud_in_menus := false
	for ch in chap_info.get("chapters", []):
		if String(ch.get("name", "")) == "Menús":
			var keys: Array = ch.get("category_keys", [])
			found_hud_in_menus = keys.has("hud")
	check(found_hud_in_menus, "Capítulo 'Menús' contiene categoría 'hud'")

	db.select_category_by_key("hud")
	await _step_frames(2)

	var h: Dictionary = db.get_hud_editor_ui_handles()
	var panel: Control = h.get("panel", null)
	check(panel != null and panel.visible, "Panel de editor del HUD visible al seleccionar categoría HUD")
	var add_type_opt: OptionButton = h.get("add_type_opt", null)
	var add_btn: Button = h.get("add_btn", null)
	var del_btn: Button = h.get("del_btn", null)
	var dup_btn: Button = h.get("dup_btn", null)
	var layer_up_btn: Button = h.get("layer_up_btn", null)
	var layer_down_btn: Button = h.get("layer_down_btn", null)
	var widgets_list: ItemList = h.get("widgets_list", null)
	var anchor_btns: Array = h.get("anchor_btns", [])
	var preview_vp: SubViewport = h.get("preview_viewport", null)
	var preview_bg: TextureRect = h.get("preview_map_bg", null)
	var live_preview = h.get("live_preview", null)
	check(add_type_opt != null and add_btn != null and dup_btn != null and layer_up_btn != null and layer_down_btn != null, "Botones de lista (+, -, Duplicar, Subir/Bajar capa) presentes")
	check(anchor_btns.size() == 9 and anchor_btns[0] != null and anchor_btns[8] != null, "Selector de anclaje 3x3 completo (9 puntos)")
	check(preview_vp != null and preview_bg != null and preview_bg.texture != null and live_preview != null, "Vista previa 16:9 sobre fondo de mapa lista")

	# 2. Crear los 6 tipos de widget desde la UI
	var types_order := ["bar", "hearts", "currency", "item", "text", "image"]
	for wtype in types_order:
		check(_select_option_by_meta(add_type_opt, wtype), "Selector de tipo elige '%s'" % wtype)
		add_btn.emit_signal("pressed")
		await _step_frames(1)

	var draft: Dictionary = db.get_hud_draft_copy()
	var widgets: Array = draft.get("widgets", [])
	check(widgets.size() == 6 and widgets_list.get_item_count() == 6, "6 widgets creados (uno de cada tipo)")

	# 3. Editar propiedades de barra: slider + spin, color, estilo ventana y anclaje 3x3
	db.select_hud_widget(0) # bar
	await _step_frames(1)
	var name_edit: LineEdit = h["name_edit"]
	name_edit.text = "BarraVidaHeroe"
	name_edit.emit_signal("text_changed", "BarraVidaHeroe")
	var w_slider: HSlider = h["width_slider"]
	var w_spin: SpinBox = h["width_spin"]
	w_slider.value = 260
	w_slider.emit_signal("value_changed", 260.0)
	check(int(w_spin.value) == 260, "Slider Ancho sincroniza SpinBox editable a 260 px")
	anchor_btns[2].emit_signal("pressed") # top_right
	var ox_spin: SpinBox = h["offset_x_spin"]
	ox_spin.value = -20
	ox_spin.emit_signal("value_changed", -20.0)
	var oy_spin: SpinBox = h["offset_y_spin"]
	oy_spin.value = 18
	oy_spin.emit_signal("value_changed", 18.0)
	check(_select_option_by_meta(h["window_style_opt"], "default"), "Estilo de ventana 'default' asignable a widget")
	var fill_col_btn: ColorPickerButton = h["bar_fill_col_btn"]
	fill_col_btn.color = Color(0.2, 0.85, 0.35, 1.0)
	fill_col_btn.emit_signal("color_changed", fill_col_btn.color)
	await _step_frames(1)

	draft = db.get_hud_draft_copy()
	var w_bar: Dictionary = draft["widgets"][0]
	check(String(w_bar.get("anchor", "")) == "top_right" and int(w_bar.get("offset_x", 0)) == -20, "Anclaje 3x3 top_right y offset X aplicados")
	check(String(w_bar.get("window_style_id", "")) == "default" and int(w_bar.get("width", 0)) == 260, "Estilo de ventana y ancho guardados en barra")

	# 4. Editar Corazones, Objeto (BD), Texto (Variable) e Imagen
	db.select_hud_widget(1) # hearts
	(h["hearts_hp_spin"] as SpinBox).value = 25
	(h["hearts_hp_spin"] as SpinBox).emit_signal("value_changed", 25.0)
	anchor_btns[0].emit_signal("pressed") # top_left
	await _step_frames(1)

	db.select_hud_widget(3) # item
	var item_opt: OptionButton = h["item_db_opt"]
	check(item_opt.get_item_count() >= 2, "Selector de objeto BD poblado con objetos del proyecto")
	item_opt.select(1)
	item_opt.emit_signal("item_selected", 1)
	(h["item_hide_zero_chk"] as CheckBox).button_pressed = true
	(h["item_hide_zero_chk"] as CheckBox).emit_signal("toggled", true)
	await _step_frames(1)

	db.select_hud_widget(4) # text
	check(_select_option_by_meta(h["text_mode_opt"], "variable"), "Widget texto cambia a modo Variable")
	var tvar_opt: OptionButton = h["text_var_opt"]
	check(tvar_opt.get_item_count() >= 2, "Selector de variable poblado desde BD")
	tvar_opt.select(1)
	tvar_opt.emit_signal("item_selected", 1)
	await _step_frames(1)

	db.select_hud_widget(5) # image
	db.set_hud_widget_field(5, "image_path", img_res)
	var op_slider: HSlider = h["img_opacity_slider"]
	var op_spin: SpinBox = h["img_opacity_spin"]
	op_slider.value = 80
	op_slider.emit_signal("value_changed", 80.0)
	check(int(op_spin.value) == 80, "Slider Opacidad sincroniza SpinBox a 80%")
	await _step_frames(1)

	# 5. Duplicar, Subir/Bajar capa y Borrar
	db.select_hud_widget(0)
	dup_btn.emit_signal("pressed")
	await _step_frames(1)
	check(db.get_hud_draft_copy()["widgets"].size() == 7, "Duplicar añade copia del widget (7)")
	var lo_before := int(db.get_hud_draft_copy()["widgets"][1].get("layer_order", 0))
	layer_up_btn.emit_signal("pressed")
	await _step_frames(1)
	var moved_idx: int = db.get_hud_editor_ui_handles()["selected_widget_idx"]
	var lo_after := int(db.get_hud_draft_copy()["widgets"][moved_idx].get("layer_order", 0))
	check(lo_after > lo_before, "Subir capa incrementa layer_order (%d -> %d)" % [lo_before, lo_after])
	del_btn.emit_signal("pressed")
	await _step_frames(1)
	check(db.get_hud_draft_copy()["widgets"].size() == 6, "Eliminar (-) quita widget duplicado (vuelve a 6)")

	# 6. Vista previa en vivo: datos de ejemplo (HP, monedas, cantidad objeto) y resoluciones 960x540 / 1366x768 / 1920x1080
	var hp_samp: SpinBox = h["sample_hp_spin"]
	var gold_samp: SpinBox = h["sample_gold_spin"]
	var qty_samp: SpinBox = h["sample_item_qty_spin"]
	hp_samp.value = 50
	hp_samp.emit_signal("value_changed", 50.0)
	gold_samp.value = 777
	gold_samp.emit_signal("value_changed", 777.0)
	qty_samp.value = 0
	qty_samp.emit_signal("value_changed", 0.0)
	await _step_frames(2)

	var st_bar: Dictionary = live_preview.get_widget_state("BarraVidaHeroe")
	var st_cur: Dictionary = live_preview.get_widget_state("Monedas")
	var st_item0: Dictionary = live_preview.get_widget_state("Objeto")
	check(bool(st_bar.get("found", false)) and int(st_bar.get("value", -1)) == 50, "Vista previa muestra HP de ejemplo = 50 en vivo")
	check(bool(st_cur.get("found", false)) and int(st_cur.get("gold", -1)) == 777, "Vista previa muestra monedas de ejemplo = 777 en vivo")
	check(bool(st_item0.get("found", false)) and not bool(st_item0.get("visible", true)), "Vista previa oculta contador de objeto cuando cantidad ejemplo = 0")

	qty_samp.value = 4
	qty_samp.emit_signal("value_changed", 4.0)
	await _step_frames(2)
	var st_item4: Dictionary = live_preview.get_widget_state("Objeto")
	check(bool(st_item4.get("visible", false)) and int(st_item4.get("qty", -1)) == 4, "Vista previa reactiva contador de objeto con x4 al cambiar cantidad")

	var res_opt: OptionButton = h["preview_res_opt"]
	for sz in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080)]:
		check(_select_option_by_vec2i(res_opt, sz), "Selector de resolución previa cambia a %dx%d" % [sz.x, sz.y])
		await _step_frames(2)
		var binfo: Dictionary = live_preview.get_layout_bounds_info()
		var clipped_arr: Array = binfo.get("clipped_widgets", [])
		var vp_sz_i := Vector2i(binfo.get("viewport_size", Vector2()))
		check(preview_vp.size == sz and vp_sz_i == sz and clipped_arr.is_empty(), "Vista previa 16:9 a %dx%d renderiza HUD sin cortes (scale=%.2f)" % [sz.x, sz.y, float(binfo.get("scale", 0.0))])

	# 7. Layout responsivo 1366x768 y 1920x1080 en Base de Datos (sin scroll horizontal)
	var main_split: BoxContainer = h["main_split"]
	for win_sz in [Vector2i(1366, 768), Vector2i(1920, 1080)]:
		db.size = win_sz
		db.adapt_to_window_size(win_sz)
		await _step_frames(2)
		var fit_info: Dictionary = db.check_layout_fits_window(win_sz)
		var clip_ctrl := _find_clipped_control(panel, float(win_sz.x))
		check(bool(fit_info.get("fits_horizontally", false)) and not main_split.vertical and not _has_visible_hscroll(panel) and clip_ctrl.is_empty(), "Categoría HUD cabe en %dx%d sin scroll horizontal ni recortes (%s)" % [win_sz.x, win_sz.y, clip_ctrl])

	# Volver vista previa a 960x540 para comparar rectángulos exactos contra HUD en juego
	check(_select_option_by_vec2i(res_opt, Vector2i(960, 540)), "Vista previa vuelve a 960x540 para paridad runtime")
	await _step_frames(2)
	var prev_states: Array = live_preview.get_all_widget_states()

	# Guardar BD y verificar que el HUD runtime en juego coincide con la configuración
	db.save_to_disk()
	await _step_frames(2)

	var saved_widgets: Array = dm.get_hud_widgets()
	check(saved_widgets.size() == 6, "Guardar BD persiste los 6 widgets en hud.json")

	# 8. Instanciar ClockGearsHudOverlay en juego y comparar con vista previa
	var game_vp := SubViewport.new()
	game_vp.size = Vector2i(960, 540)
	root.add_child(game_vp)
	var game_hud := ClockGearsHudOverlay.new()
	game_vp.add_child(game_hud)
	dm.set_actor_runtime_hp(1, 50)
	dm.set_gold(777)
	dm.change_item(String(saved_widgets[3].get("item_key", "")), 4 - dm.get_item_qty(String(saved_widgets[3].get("item_key", ""))))
	game_hud.apply_hud_config(dm.get_hud_config())
	await _step_frames(2)
	var rt_states: Array = game_hud.get_all_widget_states()
	var parity_ok := prev_states.size() == 6 and rt_states.size() == 6
	if parity_ok:
		for i in range(6):
			var ps: Dictionary = prev_states[i]
			var rs: Dictionary = rt_states[i]
			if String(ps.get("id", "")) != String(rs.get("id", "")) or Rect2(ps.get("screen_rect", Rect2())) != Rect2(rs.get("screen_rect", Rect2())) or String(ps.get("window_style_id", "")) != String(rs.get("window_style_id", "")):
				parity_ok = false
	var rt_bar: Dictionary = game_hud.get_widget_state("BarraVidaHeroe")
	check(parity_ok and String(rt_bar.get("window_style_id", "")) == "default" and int(rt_bar.get("value", -1)) == 50, "HUD en juego coincide exactamente (rects, estilo y valores) con la vista previa")
	game_vp.queue_free()
	db.hide()
	await _step_frames(1)

	# 9. Diálogo de comando 'Mostrar / Ocultar HUD' en ClockGearsEventEditorDialog
	var cat_sel := ClockGearsCommandSelector.new()
	root.add_child(cat_sel)
	await _step_frames(1)
	var hud_cmd_btn: Button = _find_button_by_text(cat_sel, "Mostrar / Ocultar HUD")
	check(hud_cmd_btn != null, "Catálogo de comandos contiene 'Mostrar / Ocultar HUD...'")
	cat_sel.queue_free()

	var ev := ClockGearsEvent.new()
	ev.pages = [{"commands": []}]
	root.add_child(ev)

	var ev_dlg := ClockGearsEventEditorDialog.new()
	root.add_child(ev_dlg)
	await _step_frames(1)
	ev_dlg.edit_event(ev)
	await _step_frames(2)

	ev_dlg.trigger_command_id(360) # CMD_CHANGE_HUD_VISIBILITY
	await _step_frames(2)
	var ev_h: Dictionary = ev_dlg.get_menu_command_ui_handles()
	var dlg_chv: ConfirmationDialog = ev_h.get("dlg_change_hud_visibility", null)
	check(dlg_chv != null and dlg_chv.visible, "Diálogo 'Mostrar / Ocultar HUD' se abre desde el editor de eventos")

	var scope_opt: OptionButton = ev_h["chv_scope_opt"]
	var w_opt: OptionButton = ev_h["chv_widget_opt"]
	var act_opt: OptionButton = ev_h["chv_action_opt"]
	scope_opt.select(1) # Widget específico
	scope_opt.emit_signal("item_selected", 1)
	check(w_opt.get_item_count() >= 7, "Selector de widget del diálogo lista los widgets del proyecto")
	w_opt.select(1)
	w_opt.emit_signal("item_selected", 1)
	act_opt.select(1) # Ocultar
	dlg_chv.emit_signal("confirmed")
	await _step_frames(2)

	var pages_out: Array = ev_dlg.get_working_pages()
	var cmds_out: Array = pages_out[0].get("commands", [])
	check(cmds_out.size() == 1 and String(cmds_out[0].get("type", "")) == "change_hud_visibility", "Comando 'change_hud_visibility' guardado en la lista")
	if not cmds_out.is_empty():
		var cdict: Dictionary = cmds_out[0]
		check(String(cdict.get("scope", "")) == "widget" and not bool(cdict.get("visible", true)) and String(cdict.get("widget_name", "")).length() > 0, "Comando guarda scope='widget', widget_name y visible=false")
		var preview_line: String = ev_dlg.format_command_preview(cdict)
		check(preview_line.find("Ocultar HUD") >= 0, "Formato en lista muestra '%s'" % preview_line)

	# Limpieza
	ev_dlg.queue_free()
	ev.queue_free()
	db.queue_free()
	if FileAccess.file_exists(img_res):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(img_res))
	dm.import_runtime_state(prev_rt)
	_restore_data_dir(res_data_dir, backup)
	dm.load_all(res_data_dir)
	await _step_frames(1)

	print("=== RESULTADO TANDA 5 UI: %d PASS, %d FAIL ===" % [passed, failed])
	quit(0 if failed == 0 else 1)

func _init() -> void:
	call_deferred("_run_tests")
