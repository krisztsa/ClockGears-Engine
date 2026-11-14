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

func _run_tests() -> void:
	print("=== TEST 9B TANDA 4 UI: EDITOR DE CRÉDITOS, VISTA PREVIA Y DIÁLOGOS DE EVENTOS ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton disponible")
	if dm == null:
		return

	var res_data_dir := "res://clockgears_data"
	var backup := _snapshot_data_dir(res_data_dir)
	await _step_frames(1)

	# Imagen de prueba para créditos
	var img_res := "res://_test_credits_img_9b4.png"
	var img := Image.create(64, 32, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.3, 0.7, 0.95, 1.0))
	img.save_png(ProjectSettings.globalize_path(img_res))

	# 1. Base de Datos -> Capítulo Menús -> Categoría Créditos
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(2)

	var ch_info: Dictionary = db.get_chapter_structure_info()
	var menus_names: Array = []
	for ch in ch_info.get("chapters", []):
		if String(ch.get("name", "")) == "Menús":
			menus_names = ch.get("category_names", [])
	check(menus_names.has("Créditos"), "Capítulo 'Menús' incluye categoría 'Créditos': " + str(menus_names))

	db.select_category_by_key("credits")
	await _step_frames(2)

	var handles: Dictionary = db.get_credits_editor_ui_handles()
	var cred_panel: Control = handles.get("panel", null)
	check(cred_panel != null and cred_panel.visible, "Panel editor de Créditos visible")

	var live_prev: ClockGearsCreditsPlayer = handles.get("live_preview", null)
	check(live_prev != null, "Vista previa en vivo ClockGearsCreditsPlayer instanciada")

	# Crear líneas desde controles UI (+): Sección, Rol y nombre, Texto, Imagen, Espacio
	var add_type_opt: OptionButton = handles.get("add_type_opt", null)
	var add_btn: Button = handles.get("add_btn", null)
	var del_btn: Button = handles.get("del_btn", null)
	var up_btn: Button = handles.get("up_btn", null)
	var down_btn: Button = handles.get("down_btn", null)
	var dup_btn: Button = handles.get("dup_btn", null)
	var items_list: ItemList = handles.get("items_list", null)

	check(add_type_opt != null and add_btn != null and del_btn != null and up_btn != null and down_btn != null and dup_btn != null and items_list != null,
		"Botones +, -, subir, bajar, duplicar y lista de líneas presentes")

	# Limpiar items iniciales si los hubiera
	while items_list and items_list.get_item_count() > 0:
		items_list.select(0)
		items_list.emit_signal("item_selected", 0)
		del_btn.emit_signal("pressed")
		await _step_frames(1)

	# Añadir Sección
	_select_option_by_meta(add_type_opt, "section")
	add_btn.emit_signal("pressed")
	await _step_frames(1)
	var item_text_edit: LineEdit = handles.get("item_text_edit", null)
	if item_text_edit:
		item_text_edit.set_text("Dirección")
		item_text_edit.emit_signal("text_changed", "Dirección")
		await _step_frames(1)

	# Añadir Rol y nombre
	_select_option_by_meta(add_type_opt, "role_name")
	add_btn.emit_signal("pressed")
	await _step_frames(1)
	var role_edit: LineEdit = handles.get("item_role_edit", null)
	var name_edit: LineEdit = handles.get("item_name_edit", null)
	if role_edit:
		role_edit.set_text("Director")
		role_edit.emit_signal("text_changed", "Director")
	if name_edit:
		name_edit.set_text("Elena Vance")
		name_edit.emit_signal("text_changed", "Elena Vance")
	await _step_frames(1)

	# Añadir Texto
	_select_option_by_meta(add_type_opt, "text")
	add_btn.emit_signal("pressed")
	await _step_frames(1)
	var multi_edit: TextEdit = handles.get("item_multiline_edit", null)
	if multi_edit:
		multi_edit.set_text("Gracias por jugar")
		multi_edit.emit_signal("text_changed")
	await _step_frames(1)

	# Añadir Imagen y Espacio
	_select_option_by_meta(add_type_opt, "image")
	add_btn.emit_signal("pressed")
	await _step_frames(1)
	var img_edit: LineEdit = handles.get("item_img_edit", null)
	if img_edit:
		img_edit.set_text(img_res)
		img_edit.emit_signal("text_changed", img_res)
	await _step_frames(1)

	_select_option_by_meta(add_type_opt, "spacer")
	add_btn.emit_signal("pressed")
	await _step_frames(1)

	var snap1: Dictionary = live_prev.get_preview_snapshot()
	check(int(snap1.get("item_count", 0)) == 5, "Vista previa refleja las 5 líneas/elementos añadidos (got=" + str(snap1.get("item_count", 0)) + ")")

	# Probar duplicar elemento índice 2 ("Texto") y reordenar con Subir / Bajar
	items_list.select(2)
	items_list.emit_signal("item_selected", 2)
	await _step_frames(1)
	dup_btn.emit_signal("pressed")
	await _step_frames(1)
	var draft_after_dup: Dictionary = db.get_credits_draft_copy()
	var items_dup: Array = draft_after_dup.get("items", [])
	check(items_dup.size() == 6 and String(Dictionary(items_dup[3]).get("text", "")) == "Gracias por jugar",
		"Duplicar crea copia idéntica debajo del elemento seleccionado (size=" + str(items_dup.size()) + ")")

	# Cambiar texto de la copia duplicada y moverla hacia arriba y abajo
	if multi_edit:
		multi_edit.set_text("Versión 1.0")
		multi_edit.emit_signal("text_changed")
		await _step_frames(1)
	up_btn.emit_signal("pressed")
	await _step_frames(1)
	var items_up: Array = db.get_credits_draft_copy().get("items", [])
	check(String(Dictionary(items_up[2]).get("text", "")) == "Versión 1.0",
		"Botón ↑ sube la línea al índice anterior: " + str(Dictionary(items_up[2]).get("text", "")))
	down_btn.emit_signal("pressed")
	await _step_frames(1)
	var items_down: Array = db.get_credits_draft_copy().get("items", [])
	check(String(Dictionary(items_down[3]).get("text", "")) == "Versión 1.0",
		"Botón ↓ baja la línea al índice siguiente")

	# Cambiar velocidad con el slider, alineación y música/fondo -> vista previa refleja cada cambio
	var speed_slider: HSlider = handles.get("speed_slider", null)
	var speed_spin: SpinBox = handles.get("speed_spin", null)
	var align_opt: OptionButton = handles.get("align_opt", null)
	var bgm_edit: LineEdit = handles.get("bgm_edit", null)
	if speed_slider:
		speed_slider.set_value(120.0)
		speed_slider.emit_signal("value_changed", 120.0)
	if align_opt:
		align_opt.select(0) # Izquierda
		align_opt.emit_signal("item_selected", 0)
	if bgm_edit:
		bgm_edit.set_text("res://audio/ending.ogg")
		bgm_edit.emit_signal("text_changed", "res://audio/ending.ogg")
	await _step_frames(1)

	check(speed_spin != null and is_equal_approx(speed_spin.value, 120.0), "Slider de velocidad sincroniza SpinBox a 120 px/s")
	var snap2: Dictionary = live_prev.get_preview_snapshot()
	check(is_equal_approx(float(snap2.get("scroll_speed", 0.0)), 120.0), "Vista previa refleja velocidad=120.0")
	check(String(snap2.get("alignment", "")) == "left", "Vista previa refleja alineación 'left'")
	check(String(snap2.get("bgm_path", "")) == "res://audio/ending.ogg", "Vista previa refleja bgm_path configurado")

	# Botón "Probar" a velocidad real
	var test_btn: Button = handles.get("test_btn", null)
	check(test_btn != null, "Botón 'Probar' presente en Créditos")
	if test_btn:
		test_btn.emit_signal("pressed")
		await _step_frames(2)
	var test_win: Window = db.get_credits_test_popup()
	var test_player: ClockGearsCreditsPlayer = db.get_credits_test_player()
	check(test_win != null and test_win.visible, "Botón 'Probar' abre popup de créditos")
	check(test_player != null and test_player.is_playing(), "Reproductor en 'Probar' inicia reproducción activa")
	if test_player:
		var st0: Dictionary = test_player.get_scroll_state()
		var y0: float = float(st0.get("scroll_y", 540.0))
		test_player.step_credits(0.5)
		var st1: Dictionary = test_player.get_scroll_state()
		var y1: float = float(st1.get("scroll_y", 0.0))
		check(is_equal_approx(y0 - y1, 60.0), "Probar avanza exactamente scroll_speed * delta (120 * 0.5 = 60 px, delta_y=" + str(y0 - y1) + ")")
		test_player.handle_credits_key(KEY_ESCAPE, true)
		await _step_frames(1)
	check(test_win != null and not test_win.visible, "Esc cierra la ventana 'Probar' de créditos")

	# Regla 16:9 en 1366x768, 1920x1080 y 2560x1440 (sin cortes ni scroll horizontal)
	for win_sz in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		db.adapt_to_window_size(win_sz)
		await _step_frames(1)
		var lrep: Dictionary = db.check_all_tabs_layout(win_sz)
		check(bool(lrep.get("all_ok", false)), "Database 'Créditos' cabe en %dx%d sin scroll horizontal" % [win_sz.x, win_sz.y])

	# Persistir Database con OK
	db.get_ok_button().emit_signal("pressed")
	await _step_frames(2)
	dm.load_all(res_data_dir)
	check(dm.has_credits_lines(), "Créditos persisten en JSON tras pulsar OK")
	var saved_cred: Dictionary = dm.get_credits_config()
	check(is_equal_approx(float(saved_cred.get("scroll_speed", 0.0)), 120.0) and Array(saved_cred.get("items", [])).size() == 6,
		"Velocidad y las 6 líneas/elementos guardados correctamente en credits.json")

	db.queue_free()
	await _step_frames(2)

	# 2. Editor de Eventos: Catálogo y los 6 nuevos diálogos
	var ev_dlg := ClockGearsEventEditorDialog.new()
	root.add_child(ev_dlg)
	await _step_frames(2)

	var init_pages := [{"commands": []}]
	ev_dlg.edit_database_event("common_events", 1, "EventoMenú9B4", init_pages)
	await _step_frames(2)

	var ev_handles: Dictionary = ev_dlg.get_menu_command_ui_handles()
	var selector: ClockGearsCommandSelector = ev_handles.get("command_selector", null)
	check(selector != null, "ClockGearsCommandSelector accesible desde ClockGearsEventEditorDialog")

	# Verificar los 6 botones presentes en el catálogo para añadir comandos
	for needle in ["Mostrar Créditos", "Abrir Menú del Juego", "Bloquear / Permitir Menú", "Ir a la Pantalla de Título", "Abrir Guardado", "Cambiar Estilo de Ventana"]:
		check(_find_button_by_text(selector, needle) != null, "Catálogo incluye botón '%s'" % needle)

	# 2a. Mostrar Créditos
	ev_dlg.trigger_command_id(ClockGearsCommandSelector.CMD_SHOW_CREDITS)
	await _step_frames(1)
	var dlg_sc: ConfirmationDialog = ev_handles.get("dlg_show_credits", null)
	check(dlg_sc != null and dlg_sc.visible, "Diálogo 'Mostrar Créditos' visible al seleccionarlo")
	dlg_sc.get_ok_button().emit_signal("pressed")
	await _step_frames(1)

	# 2b. Abrir Menú del Juego
	ev_dlg.trigger_command_id(ClockGearsCommandSelector.CMD_OPEN_MENU)
	await _step_frames(1)
	var dlg_om: ConfirmationDialog = ev_handles.get("dlg_open_menu", null)
	check(dlg_om != null and dlg_om.visible, "Diálogo 'Abrir Menú del Juego' visible")
	dlg_om.get_ok_button().emit_signal("pressed")
	await _step_frames(1)

	# 2c. Bloquear / Permitir Menú (Bloquear)
	ev_dlg.trigger_command_id(ClockGearsCommandSelector.CMD_CHANGE_MENU_ACCESS)
	await _step_frames(1)
	var dlg_ma: ConfirmationDialog = ev_handles.get("dlg_change_menu_access", null)
	var ma_opt: OptionButton = ev_handles.get("menu_access_opt", null)
	check(dlg_ma != null and dlg_ma.visible and ma_opt != null, "Diálogo 'Bloquear / Permitir Menú' visible")
	if ma_opt:
		ma_opt.select(1) # Bloquear
	dlg_ma.get_ok_button().emit_signal("pressed")
	await _step_frames(1)

	# 2d. Ir a la Pantalla de Título (con confirmación)
	ev_dlg.trigger_command_id(ClockGearsCommandSelector.CMD_RETURN_TO_TITLE)
	await _step_frames(1)
	var dlg_rt: ConfirmationDialog = ev_handles.get("dlg_return_to_title", null)
	var rt_chk: CheckBox = ev_handles.get("return_title_confirm_check", null)
	check(dlg_rt != null and dlg_rt.visible and rt_chk != null, "Diálogo 'Ir a la Pantalla de Título' visible")
	if rt_chk:
		rt_chk.set_pressed(true)
	dlg_rt.get_ok_button().emit_signal("pressed")
	await _step_frames(1)

	# 2e. Abrir Guardado
	ev_dlg.trigger_command_id(ClockGearsCommandSelector.CMD_OPEN_SAVE)
	await _step_frames(1)
	var dlg_os: ConfirmationDialog = ev_handles.get("dlg_open_save", null)
	check(dlg_os != null and dlg_os.visible, "Diálogo 'Abrir Guardado' visible")
	dlg_os.get_ok_button().emit_signal("pressed")
	await _step_frames(1)

	# 2f. Cambiar Estilo de Ventana ("message" -> "rect")
	ev_dlg.trigger_command_id(ClockGearsCommandSelector.CMD_CHANGE_WINDOW_STYLE)
	await _step_frames(1)
	var dlg_cws: ConfirmationDialog = ev_handles.get("dlg_change_window_style", null)
	var cws_wtype: OptionButton = ev_handles.get("cws_window_type_opt", null)
	var cws_style: OptionButton = ev_handles.get("cws_style_opt", null)
	check(dlg_cws != null and dlg_cws.visible and cws_wtype != null and cws_style != null, "Diálogo 'Cambiar Estilo de Ventana' visible")
	_select_option_by_meta(cws_wtype, "message")
	_select_option_by_meta(cws_style, "rect")
	dlg_cws.get_ok_button().emit_signal("pressed")
	await _step_frames(1)

	# Verificar los 6 comandos guardados en working_pages y sus líneas en la lista (command_tree)
	var pages_out: Array = ev_dlg.get_working_pages()
	var cmds_out: Array = Dictionary(pages_out[0]).get("commands", [])
	check(cmds_out.size() == 6, "Se añadieron los 6 comandos nuevos a la página del evento (count=" + str(cmds_out.size()) + ")")
	if cmds_out.size() == 6:
		check(String(Dictionary(cmds_out[0]).get("type", "")) == "show_credits", "Comando 0 es 'show_credits'")
		check(String(Dictionary(cmds_out[1]).get("type", "")) == "open_menu", "Comando 1 es 'open_menu'")
		check(String(Dictionary(cmds_out[2]).get("type", "")) == "change_menu_access" and not bool(Dictionary(cmds_out[2]).get("enabled", true)), "Comando 2 es 'change_menu_access' enabled=false")
		check(String(Dictionary(cmds_out[3]).get("type", "")) == "return_to_title" and bool(Dictionary(cmds_out[3]).get("confirm", false)), "Comando 3 es 'return_to_title' confirm=true")
		check(String(Dictionary(cmds_out[4]).get("type", "")) == "open_save", "Comando 4 es 'open_save'")
		check(String(Dictionary(cmds_out[5]).get("type", "")) == "change_window_style" and String(Dictionary(cmds_out[5]).get("window_type_id", "")) == "message" and String(Dictionary(cmds_out[5]).get("style_id", "")) == "rect", "Comando 5 es 'change_window_style' message->rect")

	var cmd_tree: Tree = ev_handles.get("command_tree", null)
	var tree_lines: Array[String] = []
	if cmd_tree and cmd_tree.get_root():
		var it: TreeItem = cmd_tree.get_root().get_first_child()
		while it != null:
			tree_lines.append(it.get_text(0))
			it = it.get_next()
	check(tree_lines.size() >= 6, "Lista visual de comandos muestra las 6 líneas")
	var joined := "\n".join(tree_lines)
	check(joined.find("Mostrar Créditos") >= 0 and joined.find("Abrir Menú del Juego") >= 0 and joined.find("Bloquear / Permitir Menú: Bloquear") >= 0 and joined.find("Ir a la Pantalla de Título (con confirmación)") >= 0 and joined.find("Abrir Guardado") >= 0 and joined.find("Cambiar Estilo de Ventana:") >= 0,
		"Todas las líneas formateadas correctamente en la lista de comandos:\n" + joined)

	ev_dlg.queue_free()
	await _step_frames(2)

	var abs_img := ProjectSettings.globalize_path(img_res)
	if FileAccess.file_exists(abs_img):
		DirAccess.remove_absolute(abs_img)
	_restore_data_dir(res_data_dir, backup)
	dm.load_all(res_data_dir)

func _initialize() -> void:
	await _run_tests()
	print("RESULTADOS: %d PASS, %d FAIL" % [passed, failed])
	quit(0 if failed == 0 else 1)
