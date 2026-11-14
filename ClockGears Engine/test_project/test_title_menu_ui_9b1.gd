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

func _remove_dir_recursive_abs(abs_dir: String) -> void:
	var da := DirAccess.open(abs_dir)
	if da == null:
		return
	da.list_dir_begin()
	var name := da.get_next()
	while name != "":
		if name != "." and name != "..":
			var p := abs_dir.path_join(name)
			if da.current_is_dir():
				_remove_dir_recursive_abs(p)
				DirAccess.remove_absolute(p)
			else:
				DirAccess.remove_absolute(p)
		name = da.get_next()
	da.list_dir_end()
	DirAccess.remove_absolute(abs_dir)

func _save_test_map(path: String, map_name: String) -> void:
	var m := ClockGearsMap.new()
	m.set_name(map_name)
	m.set_map_name(map_name)
	m.set_map_size(Vector2i(12, 10))
	m.set_grid_resolution(32)
	m.ensure_default_layers()
	for i in range(m.get_child_count()):
		var ch := m.get_child(i)
		if ch:
			ch.set_owner(m)
	var ps := PackedScene.new()
	ps.pack(m)
	ResourceSaver.save(ps, path)
	m.free()

func _run_tests() -> void:
	print("=== TEST 9B TANDA 1 UI: TÍTULO, OPCIONES, CONTINUAR Y EDITOR DB ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	var th = Engine.get_singleton("ClockGearsTheme")
	check(dm != null, "ClockGearsDataManager singleton disponible")
	check(th != null, "ClockGearsTheme singleton disponible")
	if dm == null:
		return

	# Snapshot all files in res://clockgears_data so test restores clean repository state
	var res_data_dir := "res://clockgears_data"
	var backup_files := {}
	var da_snap := DirAccess.open(res_data_dir)
	if da_snap != null:
		da_snap.list_dir_begin()
		var sf := da_snap.get_next()
		while sf != "":
			if sf != "." and sf != ".." and not da_snap.current_is_dir():
				backup_files[sf] = FileAccess.get_file_as_string(res_data_dir.path_join(sf))
			sf = da_snap.get_next()
		da_snap.list_dir_end()

	var test_saves_dir := "user://test_9b1_ui_saves"
	var test_options_path := "user://test_9b1_ui_options.json"
	var test_maps_dir := "user://test_9b1_ui_maps"
	var test_img_path := "user://test_9b1_ui_bg.png"
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(test_saves_dir))
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(test_maps_dir))
	for fp in [test_options_path, test_options_path + ".bak", test_options_path + ".tmp", test_img_path]:
		if FileAccess.file_exists(fp):
			DirAccess.remove_absolute(ProjectSettings.globalize_path(fp))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(test_saves_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(test_maps_dir))

	var img := Image.create(64, 32, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.25, 0.45, 0.75, 1.0))
	img.save_png(ProjectSettings.globalize_path(test_img_path))

	var start_map_path := test_maps_dir.path_join("inicio.tscn")
	_save_test_map(start_map_path, "MapaInicio")

	dm.init_empty()
	dm.set_category_array(0, [{"id": 1, "name": "Kael", "class_id": 1, "hp": 100, "mp": 20, "atk": 15, "def": 8}])
	dm.set_save_directory(test_saves_dir)
	dm.set_player_options_path(test_options_path)
	var sys: Dictionary = dm.get_system_config()
	sys["game_title"] = "Proyecto UI 9B"
	sys["start_map"] = start_map_path
	sys["start_x"] = 2
	sys["start_y"] = 3
	dm.set_system_config(sys)

	var title_cfg: Dictionary = ClockGearsDataManager.get_default_title_menu_config()
	title_cfg["save_slots"] = 5
	dm.set_title_menu_config(title_cfg)

	# =========================================================================
	# 1. Pantalla de Título en 960x540 y resoluciones 16:9 / fullscreen
	# =========================================================================
	var vp := SubViewport.new()
	vp.size = Vector2i(960, 540)
	vp.disable_3d = true
	root.add_child(vp)

	var ts := ClockGearsTitleScreen.new()
	ts.set_auto_quit_tree(false)
	vp.add_child(ts)
	await _step_frames(3)

	var visible_no_save: PackedStringArray = ts.get_visible_entry_ids()
	check(not visible_no_save.has("continue"), "Continuar NO aparece cuando no hay partidas guardadas")
	check(visible_no_save == PackedStringArray(["new_game", "options", "quit"]), "Orden por defecto sin Continuar ni Créditos: new_game, options, quit")

	# Crear una partida en ranura 2 para probar Continuar
	dm.reset_runtime_state()
	dm.set_runtime_clock_sec(125.0) # 00:02:05
	var save_ctx := {
		"map_path": start_map_path,
		"map_name": "MapaInicio",
		"player_x": 3,
		"player_y": 4,
		"player_px_x": 96.0,
		"player_px_y": 128.0,
		"player_dir": 2,
	}
	check(dm.save_game_slot(2, save_ctx), "Partida de prueba guardada en ranura 2")
	ts.rebuild_title_screen()
	await _step_frames(3)

	var visible_with_save: PackedStringArray = ts.get_visible_entry_ids()
	check(visible_with_save.has("continue"), "Continuar aparece automáticamente al haber partida guardada")

	for sz in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080)]:
		vp.size = sz
		await _step_frames(3)
		var b: Dictionary = ts.get_layout_bounds_info(Vector2(sz))
		check(bool(b.get("all_within_viewport", false)), "Título y botones caben completos sin recortes en %dx%d" % [sz.x, sz.y])
		check(not bool(b.get("any_overlap", true)), "Sin solapes en título/botones en %dx%d" % [sz.x, sz.y])
		check(int(b.get("button_corner_radius", 0)) >= 8, "Botones redondeados (>=8px) aplicados en %dx%d" % [sz.x, sz.y])

	vp.size = Vector2i(960, 540)
	await _step_frames(2)

	# =========================================================================
	# 2. Ratón, teclado (Flechas / Enter / Esc), Opciones en vivo y Continuar
	# =========================================================================
	# Abrir Opciones con click real (pressed signal) sobre botón Entry_options
	var opt_btn := ts.find_child("Entry_options", true, false) as Button
	check(opt_btn != null, "Botón Entry_options existe en pantalla de título")
	if opt_btn != null:
		opt_btn.emit_signal("pressed")
	await _step_frames(2)
	check(ts.is_options_menu_open(), "Opciones se abre al pulsar su botón")

	var opt_bounds: Dictionary = ts.get_layout_bounds_info(Vector2(960, 540))
	check(bool(opt_bounds.get("all_within_viewport", false)), "Panel Opciones cabe completo en 960x540")

	# Verificar sliders y valores visibles en Opciones + cambios en vivo
	var sliders := ts.find_children("", "HSlider", true, false)
	check(sliders.size() == 3, "Opciones tiene exactamente 3 sliders (música, efectos, brillo)")
	var fs_chk := ts.find_children("", "CheckBox", true, false)
	check(fs_chk.size() == 1, "Opciones tiene casilla Pantalla completa")
	if sliders.size() == 3:
		(sliders[0] as HSlider).value = 42.0
		(sliders[1] as HSlider).value = 35.0
		(sliders[2] as HSlider).value = 120.0
	await _step_frames(2)
	var lbls := ts.find_children("", "Label", true, false)
	var lbl_texts := []
	for l in lbls:
		if l is Label:
			lbl_texts.append((l as Label).text)
	check(lbl_texts.has("42%") and lbl_texts.has("35%") and lbl_texts.has("120%"), "Opciones muestra valores numéricos actualizados en vivo (42%, 35%, 120%)")
	var saved_opts: Dictionary = dm.get_player_options()
	check(is_equal_approx(float(saved_opts.get("music_volume", -1.0)), 0.42), "Slider de música guarda y aplica en vivo")
	check(is_equal_approx(float(saved_opts.get("sfx_volume", -1.0)), 0.35), "Slider de efectos guarda y aplica en vivo")
	check(is_equal_approx(float(saved_opts.get("brightness", -1.0)), 1.20), "Slider de brillo guarda y aplica en vivo")

	# Esc vuelve del menú Opciones al menú principal
	check(ts.handle_navigation_key(KEY_ESCAPE), "Tecla Esc manejada desde Opciones")
	await _step_frames(2)
	check(not ts.is_options_menu_open(), "Esc cierra Opciones y vuelve al título")

	# Navegación con flechas y Enter hasta Continuar (entry 1: new_game=0, continue=1)
	var nb0 := ts.find_child("Entry_new_game", true, false) as Button
	if nb0:
		nb0.grab_focus()
	await _step_frames(1)
	check(ts.handle_navigation_key(KEY_DOWN), "Tecla Down mueve el foco al siguiente botón")
	await _step_frames(1)
	check(ts.handle_navigation_key(KEY_ENTER), "Tecla Enter activa el botón enfocado (Continuar)")
	await _step_frames(2)
	check(ts.is_continue_menu_open(), "Continuar se abrió por navegación con teclado (Down + Enter)")

	var cont_bounds: Dictionary = ts.get_layout_bounds_info(Vector2(960, 540))
	check(bool(cont_bounds.get("all_within_viewport", false)), "Panel Continuar cabe completo en 960x540")

	var slots_info: PackedStringArray = ts.get_displayed_save_slot_texts()
	check(slots_info.size() == 5, "Continuar muestra 1 fila por espacio (5 configurados)")
	check(slots_info[1].contains("MapaInicio") and slots_info[1].contains("00:02:05"), "Ranura ocupada muestra mapa, fecha y tiempo jugado")

	var slot_btns := ts.find_child("ContinuePanel", true, false).find_children("", "Button", true, false)
	var empty_slot_btn: Button = null
	var filled_slot_btn: Button = null
	for b in slot_btns:
		if b is Button:
			var bt := (b as Button).text
			if bt.contains("Ranura 01"):
				empty_slot_btn = b
			elif bt.contains("Ranura 02"):
				filled_slot_btn = b
	check(empty_slot_btn != null and empty_slot_btn.disabled and empty_slot_btn.modulate.a < 0.8, "Espacio vacío se ve apagado y deshabilitado (no elegible)")
	check(filled_slot_btn != null and not filled_slot_btn.disabled, "Espacio guardado está habilitado para elegir")

	var loaded_sig := []
	ts.save_loaded.connect(func(slot: int, mpath: String): loaded_sig.append([slot, mpath]))
	if filled_slot_btn != null:
		filled_slot_btn.emit_signal("pressed")
	await _step_frames(2)
	check(loaded_sig.size() == 1 and int(loaded_sig[0][0]) == 2, "Elegir ranura ocupada carga la partida y emite save_loaded")

	check(ts.handle_navigation_key(KEY_ESCAPE), "Esc cierra panel Continuar")
	await _step_frames(2)
	check(not ts.is_continue_menu_open(), "Panel Continuar cerrado con Esc")

	vp.queue_free()
	await _step_frames(2)

	# =========================================================================
	# 3. Editor Base de Datos: Capítulo "Menús" > "Menú principal" + Live Preview + Probar
	# =========================================================================
	dm.save_all(res_data_dir)
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.open_database()
	await _step_frames(3)

	var chap_info: Dictionary = db.get_chapter_structure_info()
	var chaps: Array = chap_info.get("chapters", [])
	var menus_chap := {}
	for ch in chaps:
		if String(ch.get("name", "")) == "Menús":
			menus_chap = ch
			break
	check(not menus_chap.is_empty(), "Base de Datos contiene el nuevo capítulo 'Menús'")
	check(menus_chap.get("category_names", []) == ["Menú principal"], "Capítulo 'Menús' contiene únicamente la categoría 'Menú principal'")

	db.select_category_by_key("main_menu")
	await _step_frames(3)
	var after_sel: Dictionary = db.get_chapter_structure_info()
	check(String(after_sel.get("current_category_key", "")) == "main_menu", "Categoría 'Menú principal' queda activa")

	var layout_1366: Dictionary = db.check_layout_fits_window(Vector2i(1366, 768))
	var tabs_1366: Dictionary = db.check_all_tabs_layout(Vector2i(1366, 768))
	check(bool(tabs_1366.get("all_ok", false)), "Editor de Menú principal cabe en 1366x768 sin cortes ni scroll horizontal")
	check(layout_1366.has("window_size"), "check_layout_fits_window reporta layout para Menú principal")

	# Cambiar fondo, texto de entradas, título, música y orden -> verificar vista previa en vivo
	db.set_title_menu_background("color", "#2b3a55", "fill", "linear")
	db.set_title_menu_entry_state("new_game", true, "Empezar Aventura")
	db.move_title_menu_entry("options", -1) # sube antes de credits/continue o según orden
	db.move_title_menu_entry("options", -1)
	db.set_title_menu_title_config("text", "Leyendas Editadas", "nearest")
	db.set_title_menu_bgm("res://audio/bgm/custom_title.ogg")
	await _step_frames(3)

	var live_prev: ClockGearsTitleScreen = db.get_title_menu_live_preview()
	check(live_prev != null, "Vista previa en vivo incrustada disponible en 'Menú principal'")
	if live_prev != null:
		check(live_prev.get_background_color_applied().to_lower() == "2b3a55", "Vista previa actualiza color de fondo al instante")
		check(live_prev.get_title_text_displayed() == "Leyendas Editadas", "Vista previa actualiza título al instante")
		check(live_prev.get_visible_entry_texts().has("Empezar Aventura"), "Vista previa actualiza texto renombrado al instante")
		var lp_ids: PackedStringArray = live_prev.get_visible_entry_ids()
		check(lp_ids.find("options") >= 0 and lp_ids.find("options") < lp_ids.find("quit"), "Vista previa refleja reordenamiento de entradas al instante")
		check(live_prev.get_applied_bgm_path() == "res://audio/bgm/custom_title.ogg", "Vista previa actualiza BGM configurada")

	# Botón Probar
	var test_win: Window = db.open_title_menu_test_popup()
	await _step_frames(3)
	check(test_win != null and test_win.visible, "Botón 'Probar' abre ventana interactiva del menú principal")
	if test_win != null:
		var test_ts := test_win.find_children("", "ClockGearsTitleScreen", true, false)
		check(test_ts.size() == 1, "Ventana 'Probar' contiene ClockGearsTitleScreen")
		if test_ts.size() == 1:
			var tts := test_ts[0] as ClockGearsTitleScreen
			check(tts.get_title_text_displayed() == "Leyendas Editadas", "Ventana 'Probar' muestra los cambios en curso")
			check(tts.handle_navigation_key(KEY_ESCAPE), "Esc dentro de 'Probar' cierra la prueba de forma segura")
			await _step_frames(2)
			check(not test_win.visible, "Ventana 'Probar' se cerró al pulsar Esc")

	# Pulsar OK para guardar y cerrar; luego reabrir nueva instancia y verificar persistencia
	db.get_ok_button().emit_signal("pressed")
	await _step_frames(3)
	check(not db.visible, "OK guarda y cierra Base de Datos")
	db.queue_free()
	await _step_frames(3)

	var db2 := ClockGearsDatabaseDialog.new()
	root.add_child(db2)
	db2.open_database()
	await _step_frames(3)
	var draft2: Dictionary = db2.get_title_menu_draft_copy()
	var bg2: Dictionary = draft2.get("background", {})
	var t2: Dictionary = draft2.get("title", {})
	check(String(bg2.get("color", "")).to_lower() == "#2b3a55", "Color de fondo persiste tras OK y reapertura")
	check(String(t2.get("text", "")) == "Leyendas Editadas", "Texto de título persiste tras OK y reapertura")
	check(String(draft2.get("bgm_path", "")) == "res://audio/bgm/custom_title.ogg", "Música de título persiste tras OK y reapertura")

	var live_prev2: ClockGearsTitleScreen = db2.get_title_menu_live_preview()
	check(live_prev2 != null and live_prev2.get_visible_entry_texts().has("Empezar Aventura"), "Texto renombrado de entrada persiste al reabrir")

	db2.hide()
	db2.queue_free()
	await _step_frames(2)

	# Cleanup temporary files & restore backed up project data
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(test_saves_dir))
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(test_maps_dir))
	for fp in [test_options_path, test_options_path + ".bak", test_options_path + ".tmp", test_img_path]:
		if FileAccess.file_exists(fp):
			DirAccess.remove_absolute(ProjectSettings.globalize_path(fp))
	var da_rest := DirAccess.open(res_data_dir)
	if da_rest != null:
		da_rest.list_dir_begin()
		var rf := da_rest.get_next()
		while rf != "":
			if rf != "." and rf != ".." and not da_rest.current_is_dir():
				var fpath := res_data_dir.path_join(rf)
				if not backup_files.has(rf):
					DirAccess.remove_absolute(ProjectSettings.globalize_path(fpath))
			rf = da_rest.get_next()
		da_rest.list_dir_end()
	for fname in backup_files.keys():
		var fpath := res_data_dir.path_join(fname)
		var fa := FileAccess.open(fpath, FileAccess.WRITE)
		if fa:
			fa.store_string(backup_files[fname])
			fa.close()

func _init() -> void:
	call_deferred("_run_all")

func _run_all() -> void:
	await _run_tests()
	print("RESULTADO: %d passed, %d failed" % [passed, failed])
	quit(0 if failed == 0 else 1)
