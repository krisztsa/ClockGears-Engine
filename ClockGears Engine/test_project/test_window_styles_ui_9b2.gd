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

func _run_tests() -> void:
	print("=== TEST 9B TANDA 2 UI: EDITOR DE ESTILOS DE VENTANA ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton disponible")
	if dm == null:
		return

	var res_data_dir := "res://clockgears_data"
	var backup := _snapshot_data_dir(res_data_dir)
	var test_img_res := "res://_test_frame_9b2.png"
	var test_img_abs := ProjectSettings.globalize_path(test_img_res)

	# Crear textura PNG de 24x24 para probar Tipo Imagen + líneas de corte
	var img := Image.create(24, 24, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.15, 0.20, 0.32, 1.0))
	for x in range(24):
		img.set_pixel(x, 0, Color(0.95, 0.75, 0.25, 1.0))
		img.set_pixel(x, 23, Color(0.95, 0.75, 0.25, 1.0))
	for y in range(24):
		img.set_pixel(0, y, Color(0.95, 0.75, 0.25, 1.0))
		img.set_pixel(23, y, Color(0.95, 0.75, 0.25, 1.0))
	img.save_png(test_img_abs)

	dm.set_window_styles_config(dm.get_default_window_styles_config())
	dm.save_all(res_data_dir)

	await _step_frames(1)
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(3)

	# 1. Capítulo "Menús" contiene "Menú principal" y "Estilos de ventana"
	var chap_info: Dictionary = db.get_chapter_structure_info()
	var chaps: Array = chap_info.get("chapters", [])
	var menus_chap := {}
	for ch in chaps:
		if String(ch.get("name", "")) == "Menús":
			menus_chap = ch
			break
	check(not menus_chap.is_empty(), "Capítulo 'Menús' existe en Base de Datos")
	var menu_cats: Array = menus_chap.get("category_names", [])
	check(menu_cats.has("Menú principal") and menu_cats.has("Estilos de ventana"),
			"Capítulo 'Menús' incluye 'Menú principal' y 'Estilos de ventana'")

	db.select_category_by_key("window_styles")
	await _step_frames(3)
	var after_sel: Dictionary = db.get_chapter_structure_info()
	check(String(after_sel.get("current_category_key", "")) == "window_styles", "Categoría 'Estilos de ventana' queda activa")

	var h: Dictionary = db.get_window_styles_ui_handles()
	var rec_list := h.get("record_list") as ItemList
	var btn_add := h.get("btn_add") as Button
	var btn_remove := h.get("btn_remove") as Button
	var btn_copy := h.get("btn_copy") as Button
	var btn_paste := h.get("btn_paste") as Button
	var btn_max := h.get("btn_max") as Button
	var ws_panel := h.get("panel") as Control

	check(ws_panel != null and ws_panel.visible, "Panel de Estilos de ventana visible")
	check(rec_list != null and rec_list.item_count >= 3, "Lista izquierda muestra al menos los 3 estilos incluidos")
	check(btn_max != null and not btn_max.visible, "Botón 'Max' oculto en Estilos de ventana")

	# Built-in style ("Redondeado") cannot be deleted
	db.select_window_style(0)
	await _step_frames(2)
	check(btn_remove != null and btn_remove.disabled, "Botón '-' desactivado sobre estilo incluido")
	var count_before_rm: int = rec_list.item_count
	btn_remove.emit_signal("pressed")
	await _step_frames(2)
	check(rec_list.item_count == count_before_rm, "Pulsar '-' sobre estilo incluido no lo borra")

	# 2. Crear, copiar y borrar estilos
	btn_add.emit_signal("pressed")
	await _step_frames(2)
	check(rec_list.item_count == count_before_rm + 1, "Botón '+' crea un nuevo estilo")

	var name_edit := h.get("name_edit") as LineEdit
	name_edit.text = "Mi Estilo Dorado"
	name_edit.emit_signal("text_changed", "Mi Estilo Dorado")
	await _step_frames(2)

	var draft_after_add: Dictionary = db.get_window_styles_draft_copy()
	var styles_arr: Array = draft_after_add.get("styles", [])
	check(styles_arr.size() == count_before_rm + 1 and String(styles_arr.back().get("name", "")) == "Mi Estilo Dorado",
			"Nombre editado del nuevo estilo se refleja en el borrador")

	# Copy & Paste: copiar estilo incluido y pegar crea un duplicado editable; luego borrarlo
	db.select_window_style(0) # Redondeado (incluido)
	await _step_frames(1)
	btn_copy.emit_signal("pressed")
	await _step_frames(1)
	check(not btn_paste.disabled, "Copiar habilita botón Pegar")
	var before_paste_bi := rec_list.item_count
	btn_paste.emit_signal("pressed")
	await _step_frames(2)
	check(rec_list.item_count == before_paste_bi + 1, "Copiar estilo incluido y Pegar crea un nuevo duplicado editable")

	# Borrar el duplicado recién pegado, conservando "Mi Estilo Dorado"
	var count_before_del_custom := rec_list.item_count
	check(not btn_remove.disabled, "Botón '-' habilitado para estilo personalizado")
	btn_remove.emit_signal("pressed")
	await _step_frames(2)
	check(rec_list.item_count == count_before_del_custom - 1, "Botón '-' borra estilo personalizado")

	# Reselect "Mi Estilo Dorado"
	var custom_idx := rec_list.item_count - 1
	db.select_window_style(custom_idx)
	await _step_frames(2)

	# 3. Sliders actualizan vista previa en vivo al instante
	var win_s: Dictionary = h.get("window_surface", {})
	var bw_slider := win_s.get("border_width_slider") as HSlider
	var bw_spin := win_s.get("border_width_spin") as SpinBox
	var cr_slider := win_s.get("corner_radius_slider") as HSlider
	var cr_spin := win_s.get("corner_radius_spin") as SpinBox
	var op_slider := win_s.get("opacity_slider") as HSlider
	var op_spin := win_s.get("opacity_spin") as SpinBox
	var pad_slider := win_s.get("padding_slider") as HSlider
	var pad_spin := win_s.get("padding_spin") as SpinBox

	bw_slider.value = 6
	cr_slider.value = 18
	op_slider.value = 50
	pad_slider.value = 22
	await _step_frames(2)

	check(int(bw_spin.value) == 6 and int(cr_spin.value) == 18 and int(op_spin.value) == 50 and int(pad_spin.value) == 22,
			"Sliders de grosor, radio, opacidad y margen sincronizan SpinBoxes editables")

	var prev_info: Dictionary = db.get_window_style_preview_info()
	var win_prev: Dictionary = prev_info.get("window", {})
	check(int(win_prev.get("border_width", -1)) == 6, "Slider grosor actualiza StyleBox de vista previa al instante")
	check(int(win_prev.get("corner_radius", -1)) == 18, "Slider radio actualiza StyleBox de vista previa al instante")
	var fc_val: Variant = win_prev.get("fill_color", Color(0, 0, 0, 1))
	var prev_fill_col: Color = fc_val if fc_val is Color else Color.html(String(fc_val))
	check(absf(prev_fill_col.a - 0.50) < 0.04, "Slider opacidad actualiza alpha de vista previa al instante")
	var prev_pad: Dictionary = win_prev.get("padding", {})
	check(int(prev_pad.get("left", -1)) == 22, "Slider margen interno actualiza padding de vista previa al instante")

	# 4. Pestañas de botón (Normal, Resaltado, Pulsado, Desactivado) y escala de vista previa
	var btn_tabs := h.get("btn_state_tabs") as TabBar
	var btn_s: Dictionary = h.get("button_surface", {})
	var inherit_chk := btn_s.get("inherit_normal_chk") as CheckBox
	var b_bw_slider := btn_s.get("border_width_slider") as HSlider

	check(btn_tabs != null and btn_tabs.tab_count == 4, "Botones tienen 4 pestañas (Normal, Resaltado, Pulsado, Desactivado)")
	btn_tabs.current_tab = 1 # Resaltado
	btn_tabs.emit_signal("tab_changed", 1)
	await _step_frames(2)
	check(inherit_chk.visible, "Casilla 'Heredar del estado Normal' visible en Resaltado")
	inherit_chk.button_pressed = false
	inherit_chk.emit_signal("toggled", false)
	await _step_frames(2)
	b_bw_slider.value = 5
	await _step_frames(2)

	var p_btns: Array = db.get_window_style_preview_info().get("buttons", [])
	check(p_btns.size() == 4, "Vista previa muestra los 4 estados de botón simultáneamente")
	check(int(p_btns[1].get("border_width", -1)) == 5, "Cambio en pestaña Resaltado se refleja en botón Resaltado de vista previa")
	check(bool(p_btns[3].get("disabled", false)), "Botón Desactivado aparece en estado disabled en vista previa")

	# Probar selector de tamaño / escala en vista previa
	var size_opt := h.get("preview_size_opt") as OptionButton
	size_opt.select(2) # Full HD 1920x1080 (2x)
	size_opt.emit_signal("item_selected", 2)
	await _step_frames(2)
	var prev_2x: Dictionary = db.get_window_style_preview_info()
	check(is_equal_approx(float(prev_2x.get("window", {}).get("scale_factor", 1.0)), 2.0),
			"Selector de tamaño de vista previa cambia escala a 2x (1920x1080)")
	size_opt.select(0)
	size_opt.emit_signal("item_selected", 0)
	await _step_frames(1)

	# 5. Tipo Imagen: 4 márgenes de corte, líneas de corte dibujadas y persistencia
	var bt_opt := win_s.get("border_type_opt") as OptionButton
	var img_edit := win_s.get("image_path_edit") as LineEdit
	var cm_opt := win_s.get("center_mode_opt") as OptionButton
	var slice_sliders: Array = win_s.get("slice_sliders", [])
	var slice_prev := win_s.get("slice_preview") as ClockGearsNineSlicePreview

	bt_opt.select(3) # Imagen
	bt_opt.emit_signal("item_selected", 3)
	await _step_frames(2)
	img_edit.text = test_img_res
	img_edit.emit_signal("text_changed", test_img_res)
	cm_opt.select(1) # Repetir
	cm_opt.emit_signal("item_selected", 1)
	slice_sliders[0].value = 6
	slice_sliders[1].value = 5
	slice_sliders[2].value = 7
	slice_sliders[3].value = 4
	await _step_frames(3)

	var cut_info: Dictionary = slice_prev.get_cut_lines_info()
	var cut_margins: Dictionary = cut_info.get("margins", {})
	var cut_lines: Dictionary = cut_info.get("lines", {})
	check(bool(cut_info.get("has_texture", false)), "Vista de cortes 9-slice carga la textura importada")
	check(int(cut_margins.get("left", -1)) == 6 and int(cut_margins.get("top", -1)) == 5 and
			int(cut_margins.get("right", -1)) == 7 and int(cut_margins.get("bottom", -1)) == 4,
			"Líneas de corte reciben los 4 márgenes exactos (6, 5, 7, 4)")
	check(float(cut_lines.get("left_x", 0.0)) < float(cut_lines.get("right_x", 0.0)) and
			float(cut_lines.get("top_y", 0.0)) < float(cut_lines.get("bottom_y", 0.0)),
			"Coordenadas de líneas de corte horizontal/vertical están ordenadas sobre la imagen")

	var img_prev_win: Dictionary = db.get_window_style_preview_info().get("window", {})
	check(String(img_prev_win.get("border_type", "")) == "image" and String(img_prev_win.get("center_mode", "")) == "repeat",
			"Vista previa actualiza tipo Imagen + modo Repetir al instante")

	# 6. Asignación de estilo por defecto y por tipo de ventana
	h = db.get_window_styles_ui_handles()
	var custom_id := String(db.get_window_styles_draft_copy().get("styles", []).back().get("id", ""))
	var def_opt := h.get("default_style_opt") as OptionButton
	var assign_opts: Dictionary = h.get("assignment_opts", {})
	check(assign_opts.has("message") and assign_opts.has("title_buttons") and assign_opts.has("options"),
			"Sección Asignación muestra desplegables por cada tipo de ventana")

	def_opt.select(1) # Recto ("rect")
	def_opt.emit_signal("item_selected", 1)
	var msg_opt := assign_opts["message"] as OptionButton
	msg_opt.select(msg_opt.item_count - 1) # Custom style (1 + custom_idx)
	msg_opt.emit_signal("item_selected", msg_opt.item_count - 1)
	await _step_frames(2)

	# 7. Regla 16:9: 1366x768, 1920x1080 y 2560x1440 sin scroll horizontal ni cortes
	for res in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		db.adapt_to_window_size(res)
		await _step_frames(2)
		var lay: Dictionary = db.check_layout_fits_window(res)
		var tabs_rep: Dictionary = db.check_all_tabs_layout(res)
		var split := h.get("main_split") as BoxContainer
		var right_edge_ok := true
		if ws_panel != null and ws_panel.size.x > res.x:
			right_edge_ok = false
		check(bool(lay.get("fits_horizontally", false)) and bool(tabs_rep.get("all_ok", false)) and right_edge_ok and (split == null or not split.vertical),
				"Sin scroll horizontal ni cortes en %dx%d (layout lado a lado OK)" % [res.x, res.y])

	# 8. Guardar con OK, reabrir y verificar persistencia en disco
	db.get_ok_button().emit_signal("pressed")
	await _step_frames(3)
	db.queue_free()
	await _step_frames(2)

	var db2 := ClockGearsDatabaseDialog.new()
	root.add_child(db2)
	await _step_frames(2)
	db2.open_database()
	await _step_frames(3)
	db2.select_category_by_key("window_styles")
	await _step_frames(3)

	var reloaded: Dictionary = db2.get_window_styles_draft_copy()
	check(String(reloaded.get("default_style_id", "")) == "rect", "Estilo por defecto asignado persiste tras reabrir")
	check(String(reloaded.get("assignments", {}).get("message", "")) == custom_id, "Asignación por tipo de ventana ('message') persiste tras reabrir")

	var found_custom := {}
	for st in reloaded.get("styles", []):
		if String(st.get("id", "")) == custom_id:
			found_custom = st
			break
	check(not found_custom.is_empty() and String(found_custom.get("name", "")) == "Mi Estilo Dorado",
			"Estilo creado persiste tras guardar y reabrir")
	check(int(found_custom.get("nine_slice", {}).get("left", -1)) == 6 and int(found_custom.get("nine_slice", {}).get("bottom", -1)) == 4,
			"Márgenes de corte de imagen persisten tras guardar y reabrir")

	db2.queue_free()
	await _step_frames(2)

	# Cleanup & restore res://clockgears_data
	if FileAccess.file_exists(test_img_abs):
		DirAccess.remove_absolute(test_img_abs)
	_restore_data_dir(res_data_dir, backup)
	dm.load_all(res_data_dir)

	print("=== RESULTADO UI 9B TANDA 2: %d PASS, %d FAIL ===" % [passed, failed])

func _initialize() -> void:
	await _run_tests()
	quit(1 if failed > 0 else 0)
