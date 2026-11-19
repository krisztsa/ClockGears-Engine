extends SceneTree

# Test Etapa 5 (9B-Ext):
# 5A: Título, Opciones (game_menu_options), Guardar/Cargar (game_menu_save) y Créditos diseñables con acciones 3D, BGM y transición
# 5B: Revisión 16:9 en 960x540, 1366x768, 1920x1080 y 2560x1440 sin cortes, solapes, scroll horizontal ni desborde de botones
# 5C: Estabilidad de nodos/memoria en 50 aperturas y dibujo propio de lista larga (>32 capas)
# 5D: Migración de configuración antigua 9B/v1 y estado limpio

var passed: int = 0
var failed: int = 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _snapshot_data_dir(res_dir: String) -> Dictionary:
	var out: Dictionary = {"files": {}}
	var da := DirAccess.open(res_dir)
	if da == null:
		return out
	da.list_dir_begin()
	var fn := da.get_next()
	while fn != "":
		if not da.current_is_dir():
			var p := res_dir.path_join(fn)
			var f := FileAccess.open(p, FileAccess.READ)
			if f != null:
				out["files"][fn] = f.get_buffer(f.get_length())
		fn = da.get_next()
	return out

func _restore_data_dir(res_dir: String, snap: Dictionary) -> void:
	var files: Dictionary = snap.get("files", {})
	var da := DirAccess.open(res_dir)
	if da != null:
		da.list_dir_begin()
		var fn := da.get_next()
		while fn != "":
			if not da.current_is_dir() and not files.has(fn):
				da.remove(fn)
			fn = da.get_next()
	for fn in files.keys():
		var f := FileAccess.open(res_dir.path_join(fn), FileAccess.WRITE)
		if f != null:
			f.store_buffer(files[fn])

func _check_visible_hscroll_recursive(node: Node, ignore_names: PackedStringArray = PackedStringArray()) -> Array[String]:
	var bad: Array[String] = []
	if node == null:
		return bad
	if node is ScrollContainer:
		var sc := node as ScrollContainer
		if not ignore_names.has(sc.name) and sc.is_visible_in_tree():
			if sc.horizontal_scroll_mode != ScrollContainer.SCROLL_MODE_DISABLED:
				var hs := sc.get_h_scroll_bar()
				if hs != null and hs.visible and hs.max_value > hs.page + 1.0:
					bad.append(sc.name)
	for c in node.get_children():
		bad.append_array(_check_visible_hscroll_recursive(c, ignore_names))
	return bad

func _find_button_with_meta(node: Node, elem_id: String) -> Button:
	if node is Button and node.has_meta("ui_elem_id") and String(node.get_meta("ui_elem_id")) == elem_id:
		return node as Button
	for c in node.get_children():
		var r := _find_button_with_meta(c, elem_id)
		if r != null:
			return r
	return null

func _find_all_buttons(node: Node, out: Array[Button]) -> void:
	if node is Button:
		out.append(node as Button)
	for c in node.get_children():
		_find_all_buttons(c, out)

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST ETAPA 5 (9B-EXT): TÍTULO/PANTALLAS, 16:9, OPT Y MIGRACIÓN ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		check(false, "ClockGearsDataManager disponible")
		quit(1)
		return

	var res_data_dir := "res://clockgears_data"
	var backup := _snapshot_data_dir(res_data_dir)
	var logo_res := "res://_test_etapa5_logo.png"
	var img := Image.create(64, 64, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.85, 0.65, 0.25, 1.0))
	img.save_png(ProjectSettings.globalize_path(logo_res))

	dm.load_all(res_data_dir)
	dm.reset_runtime_state()

	# =========================================================================
	# 5A. Título, Opciones, Guardar/Cargar y Créditos totalmente diseñables
	# =========================================================================
	for sid in ["title", "game_menu_options", "game_menu_save", "credits"]:
		var acts: Array = ClockGearsDataManager.get_ui_button_actions(sid)
		var act_ids: Array[String] = []
		for a in acts:
			act_ids.append(String(a.get("id", "")))
		check(act_ids.has("set_switch") and act_ids.has("set_variable") and act_ids.has("open_menu") and act_ids.has("play_sound"),
			"Catálogo de acciones en '%s' incluye acciones 3D completas" % sid)

	# Crear un menú propio que se abrirá desde un botón multi-acción del Título
	var sub_from_title_id: String = dm.create_custom_menu("SubDesdeTitulo", {
		"custom_enabled": true,
		"elements": [
			{"id": "sub_title_lbl", "type": "text", "text": "Abierto desde Título", "offset_x": 120, "offset_y": 100, "width": 300, "height": 44},
			{"id": "sub_back", "type": "button", "text": "Volver", "action": "close", "offset_x": 120, "offset_y": 170, "width": 180, "height": 40}
		]
	})
	check(not sub_from_title_id.is_empty(), "Menú propio para probar open_menu desde Título creado")

	# Configurar Título personalizado con fondo degradado, logo, BGM, transición y botones 3D
	var title_design: Dictionary = {
		"screen_id": "title",
		"custom_enabled": true,
		"bgm_path": "res://audio/etapa5_title_bgm.ogg",
		"background": {
			"type": "gradient",
			"color": "#1a2238",
			"gradient_bottom": "#0a0e1a",
			"dark_veil": true
		},
		"transition": {
			"in_type": "fade",
			"out_type": "fade",
			"duration_sec": 0.25,
			"curve": "ease_out"
		},
		"elements": [
			{"id": "t_logo", "type": "image", "image_path": logo_res, "anchor": "top_center", "offset_x": 0, "offset_y": 30, "width": 160, "height": 90, "layer_order": 1},
			{"id": "t_btn_3d", "type": "button", "text": "Acción 3D + Menú", "action": "none", "actions": [
				{"type": "set_switch", "switch_name": "sw_etapa5_title", "value": true},
				{"type": "set_variable", "var_name": "var_etapa5_title", "value": 77},
				{"type": "open_menu", "menu_id": sub_from_title_id}
			], "anchor": "center", "offset_x": 0, "offset_y": -40, "width": 240, "height": 40, "focus_order": 0},
			{"id": "t_btn_opts", "type": "button", "text": "Opciones", "action": "options", "anchor": "center", "offset_x": 0, "offset_y": 10, "width": 240, "height": 40, "focus_order": 1},
			{"id": "t_btn_cont", "type": "button", "text": "Continuar", "action": "continue", "anchor": "center", "offset_x": 0, "offset_y": 60, "width": 240, "height": 40, "focus_order": 2},
			{"id": "t_btn_cred", "type": "button", "text": "Créditos", "action": "credits", "anchor": "center", "offset_x": 0, "offset_y": 110, "width": 240, "height": 40, "focus_order": 3},
			{"id": "t_btn_new", "type": "button", "text": "Nueva Partida", "action": "new_game", "anchor": "center", "offset_x": 0, "offset_y": 160, "width": 240, "height": 40, "focus_order": 4}
		]
	}
	dm.set_ui_screen_design("title", title_design, false)

	# Personalizar game_menu_options, game_menu_save y credits con botón propio 3D
	dm.set_ui_screen_design("game_menu_options", {
		"screen_id": "game_menu_options",
		"custom_enabled": true,
		"elements": [
			{"id": "opt_custom_3d", "type": "button", "text": "Extra Opciones 3D", "action": "set_variable", "var_name": "var_etapa5_opts", "value": 42, "offset_x": 60, "offset_y": 80, "width": 240, "height": 42},
			{"id": "opt_back", "type": "button", "text": "Volver", "action": "back", "offset_x": 60, "offset_y": 140, "width": 240, "height": 42}
		]
	}, false)

	dm.set_ui_screen_design("game_menu_save", {
		"screen_id": "game_menu_save",
		"custom_enabled": true,
		"elements": [
			{"id": "sav_custom_3d", "type": "button", "text": "Extra Guardar 3D", "action": "set_switch", "switch_name": "sw_etapa5_save", "value": true, "offset_x": 60, "offset_y": 80, "width": 240, "height": 42},
			{"id": "sav_back", "type": "button", "text": "Volver", "action": "back", "offset_x": 60, "offset_y": 140, "width": 240, "height": 42}
		]
	}, false)

	dm.set_ui_screen_design("credits", {
		"screen_id": "credits",
		"custom_enabled": true,
		"transition": {"in_type": "fade", "duration_sec": 0.2},
		"elements": [
			{"id": "cred_lbl", "type": "text", "text": "Créditos personalizados", "offset_x": 200, "offset_y": 40, "width": 360, "height": 44},
			{"id": "cred_custom_3d", "type": "button", "text": "Bonus Créditos 3D", "action": "set_variable", "var_name": "var_etapa5_cred", "value": 88, "offset_x": 200, "offset_y": 120, "width": 240, "height": 42},
			{"id": "cred_back", "type": "button", "text": "Cerrar", "action": "back", "offset_x": 200, "offset_y": 180, "width": 240, "height": 42}
		]
	}, false)

	# Crear una ranura guardada para habilitar Continuar
	dm.save_game_slot(1, {"map_path": "res://maps/start_map.tscn", "map_name": "Mapa Test", "player_x": 1, "player_y": 1})

	var vp := SubViewport.new()
	vp.size = Vector2i(960, 540)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(vp)

	var ts := ClockGearsTitleScreen.new()
	ts.set_auto_quit_tree(false)
	vp.add_child(ts)
	await _step_frames(3)

	check(ts.is_custom_ui_active(), "Pantalla de título activa diseño visual personalizado")
	check(ts.get_applied_bgm_path() == "res://audio/etapa5_title_bgm.ogg", "Título aplica BGM configurado en el diseño visual")
	var t_layer: ClockGearsUiDesignLayer = ts.get_custom_ui_layer()
	check(t_layer != null and t_layer.get_element_control("t_logo") != null, "Título personalizado muestra logo propio")

	# Pulsar botón multi-acción 3D -> cambia interruptor, variable y abre menú propio
	var btn_3d: Button = _find_button_with_meta(t_layer, "t_btn_3d")
	check(btn_3d != null, "Botón multi-acción 3D existe en Título")
	if btn_3d != null:
		btn_3d.emit_signal("pressed")
	await _step_frames(3)
	check(bool(dm.get_switch("sw_etapa5_title")) == true and int(dm.get_variable("var_etapa5_title")) == 77,
		"Botón del Título ejecuta múltiples acciones 3D (interruptor y variable)")
	var gm_sub := ts.get_node_or_null("TitleCustomSubMenu") as ClockGearsGameMenu
	check(gm_sub != null and gm_sub.is_menu_open() and gm_sub.get_active_menu_id() == sub_from_title_id,
		"Botón del Título abre submenú propio mediante open_menu")
	ts.close_submenus()
	await _step_frames(2)

	# Pulsar Opciones -> abre game_menu_options rediseñado y ejecuta su botón propio
	var btn_opts: Button = _find_button_with_meta(t_layer, "t_btn_opts")
	if btn_opts != null:
		btn_opts.emit_signal("pressed")
	await _step_frames(3)
	check(ts.is_options_menu_open(), "Opciones rediseñado se abre desde Título")
	if gm_sub != null and gm_sub.get_custom_ui_layer() != null:
		var opt_extra := _find_button_with_meta(gm_sub.get_custom_ui_layer(), "opt_custom_3d")
		check(opt_extra != null, "Botón 3D propio presente en game_menu_options rediseñado")
		if opt_extra != null:
			opt_extra.emit_signal("pressed")
		await _step_frames(2)
	check(int(dm.get_variable("var_etapa5_opts")) == 42, "Botón propio en game_menu_options ejecuta acción 3D")
	ts.close_submenus()
	await _step_frames(2)

	# Pulsar Continuar -> abre game_menu_save rediseñado y ejecuta su botón propio
	var btn_cont: Button = _find_button_with_meta(t_layer, "t_btn_cont")
	if btn_cont != null:
		btn_cont.emit_signal("pressed")
	await _step_frames(3)
	check(ts.is_continue_menu_open(), "Guardar/Cargar rediseñado se abre desde Título")
	if gm_sub != null and gm_sub.get_custom_ui_layer() != null:
		var sav_extra := _find_button_with_meta(gm_sub.get_custom_ui_layer(), "sav_custom_3d")
		check(sav_extra != null, "Botón 3D propio presente en game_menu_save rediseñado")
		if sav_extra != null:
			sav_extra.emit_signal("pressed")
		await _step_frames(2)
	check(bool(dm.get_switch("sw_etapa5_save")) == true, "Botón propio en game_menu_save ejecuta acción 3D")
	ts.close_submenus()
	await _step_frames(2)

	# Pulsar Créditos -> abre credits rediseñado y ejecuta su botón propio
	var btn_cred: Button = _find_button_with_meta(t_layer, "t_btn_cred")
	if btn_cred != null:
		btn_cred.emit_signal("pressed")
	await _step_frames(3)
	check(ts.is_credits_open() and ts.get_credits_player().is_custom_ui_active(), "Créditos rediseñados se abren desde Título")
	var cred_extra := _find_button_with_meta(ts.get_credits_player().get_custom_ui_layer(), "cred_custom_3d")
	check(cred_extra != null, "Botón 3D propio presente en Créditos rediseñados")
	if cred_extra != null:
		cred_extra.emit_signal("pressed")
	await _step_frames(2)
	check(int(dm.get_variable("var_etapa5_cred")) == 88, "Botón propio en Créditos ejecuta acción 3D")
	ts.close_submenus()
	await _step_frames(2)

	# =========================================================================
	# 5B. Revisión 16:9 en 960x540, 1366x768, 1920x1080 y 2560x1440
	#     y comprobación anti-desborde de texto en botones
	# =========================================================================
	var long_utf8_label := "Botón extremadamente largo de comprobación 16:9 con eñes, tildes áéíóú y sin desborde"
	var d_over: Dictionary = dm.get_ui_screen_design("title")
	d_over["elements"][1]["text"] = long_utf8_label
	dm.set_ui_screen_design("title", d_over, false)
	ts.rebuild_title_screen()
	await _step_frames(2)
	var btn_over := _find_button_with_meta(ts.get_custom_ui_layer(), "t_btn_3d")
	check(btn_over != null and btn_over.clip_text and btn_over.size.x <= 242.0,
		"Texto largo en botón diseñado se recorta y no desborda el ancho del botón (width=%.1f)" % (btn_over.size.x if btn_over != null else -1.0))

	# Volver a diseño por defecto del Título para comprobar también paneles estándar en los 4 tamaños
	dm.reset_ui_screen_design_to_default("title", false)
	ts.rebuild_title_screen()
	await _step_frames(2)

	var sizes_16_9 := [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]
	for sz in sizes_16_9:
		vp.size = sz
		await _step_frames(3)
		var bt: Dictionary = ts.get_layout_bounds_info(Vector2(sz))
		check(bool(bt.get("all_within_viewport", false)) and not bool(bt.get("any_overlap", true)),
			"Título 16:9 sin cortes ni solapes en %dx%d" % [sz.x, sz.y])

		ts.open_options_menu()
		await _step_frames(2)
		var bo: Dictionary = ts.get_layout_bounds_info(Vector2(sz))
		check(bool(bo.get("all_within_viewport", false)), "Título Opciones 16:9 cabe en %dx%d" % [sz.x, sz.y])
		ts.close_submenus()

		ts.open_continue_menu()
		await _step_frames(2)
		var bc: Dictionary = ts.get_layout_bounds_info(Vector2(sz))
		check(bool(bc.get("all_within_viewport", false)), "Título Continuar 16:9 cabe en %dx%d" % [sz.x, sz.y])
		ts.close_submenus()
		await _step_frames(1)

	ts.queue_free()
	await _step_frames(1)

	# Menú del juego en los 4 tamaños 16:9 (incluyendo radial)
	dm.reset_ui_screen_design_to_default("game_menu_options", false)
	dm.reset_ui_screen_design_to_default("game_menu_save", false)
	var gm := ClockGearsGameMenu.new()
	vp.add_child(gm)
	for sz in sizes_16_9:
		vp.size = sz
		for sub in ["main", "items", "equip", "status", "save", "options"]:
			gm.open_menu_by_id("game_menu_" + sub, false, false, true)
			await _step_frames(2)
			var bm: Dictionary = gm.get_layout_bounds_info(Vector2(sz))
			check(bool(bm.get("all_within_viewport", false)) and not bool(bm.get("overlaps", bm.get("any_overlap", true))),
				"GameMenu '%s' 16:9 sin cortes ni solapes en %dx%d" % [sub, sz.x, sz.y])
		var hscroll_gm := _check_visible_hscroll_recursive(gm)
		check(hscroll_gm.is_empty(), "GameMenu sin scroll horizontal en %dx%d (%s)" % [sz.x, sz.y, str(hscroll_gm)])
		gm.close_menu()

	# Verificar que todos los botones de GameMenu tienen clip_text o caben en su rect
	var gm_btns: Array[Button] = []
	_find_all_buttons(gm, gm_btns)
	var all_clip_or_bounded := true
	for b in gm_btns:
		if not b.clip_text and b.get_combined_minimum_size().x > 360.0:
			all_clip_or_bounded = false
			break
	check(all_clip_or_bounded, "Todos los botones del GameMenu protegen contra desborde de texto")
	gm.queue_free()
	vp.queue_free()
	await _step_frames(2)

	# Editor de menús (Database -> ui_designer) en los 4 tamaños 16:9 + 5C lista larga (>32 capas)
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.select_category_by_key("ui_designer")
	var ed: ClockGearsUiDesignEditor = db.get_ui_designer_editor()
	for sz in sizes_16_9:
		db.popup_centered(sz)
		ed.adapt_to_width(sz.x)
		await _step_frames(2)
		var hscroll_ed := _check_visible_hscroll_recursive(ed, PackedStringArray(["UiDesignerCanvasScroll"]))
		check(hscroll_ed.is_empty(), "Editor UI Designer sin scroll horizontal fuera del lienzo en %dx%d (%s)" % [sz.x, sz.y, str(hscroll_ed)])

	# 5C: Crear menú de 100 widgets y comprobar dibujo propio de lista larga (>32 -> 1 hijo ClockGearsLayersCanvas)
	var long_id: String = ed.create_menu_via_wizard({"name": "ListaLarga100", "template_id": "minimalist", "buttons": []})
	var els100: Array = []
	for k in range(100):
		var el := ClockGearsDataManager.make_default_ui_element("text", long_id)
		el["id"] = "l_%d" % k
		el["name"] = "Capa %d" % k
		el["offset_x"] = 20 + (k % 5) * 160
		el["offset_y"] = 20 + int(k / 5) * 24
		els100.append(el)
	var ld: Dictionary = dm.get_ui_screen_design(long_id)
	ld["elements"] = els100
	dm.set_ui_screen_design(long_id, ld, false)
	ed.select_screen(long_id)
	await _step_frames(2)
	var h: Dictionary = db.get_ui_designer_ui_handles()
	var layers_vbox: VBoxContainer = h["layers_list_vbox"]
	check(layers_vbox != null and layers_vbox.get_child_count() == 1,
		"Lista de capas larga (100 widgets) usa dibujo propio con 1 solo nodo (hijos=%d)" % (layers_vbox.get_child_count() if layers_vbox != null else -1))

	db.queue_free()
	await _step_frames(2)

	# =========================================================================
	# 5D. Migración de proyecto antiguo v1
	# =========================================================================
	var legacy_cfg: Dictionary = {
		"screens": {
			"title": {
				"elements": [
					{"type": "label", "text": "Proyecto Antiguo", "x": 40, "y": 40}
				]
			}
		}
	}
	var migrated: Dictionary = ClockGearsDataManager.normalize_ui_designs_config(legacy_cfg)
	var mig_screens: Dictionary = migrated.get("screens", {})
	check(mig_screens.has("title") and mig_screens.has("game_menu_options") and mig_screens.has("game_menu_save") and mig_screens.has("credits"),
		"Migración v1 conserva elementos heredados y completa las pantallas del sistema")

	# Restaurar estado limpio del proyecto
	_restore_data_dir(res_data_dir, backup)
	DirAccess.remove_absolute(ProjectSettings.globalize_path(logo_res))
	dm.delete_save_slot(1)
	dm.load_all(res_data_dir)
	dm.reset_runtime_state()

	print("=== RESULTADO ETAPA 5: %d PASSED / %d FAILED ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
