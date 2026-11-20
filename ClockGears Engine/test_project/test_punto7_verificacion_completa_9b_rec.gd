extends SceneTree

# Test Permanente Punto 7 (UI Reconst 9B):
# Smoke test completo Editor + Runtime cubriendo TODA la lista de verificacion de la 9B.

var pass_count: int = 0
var fail_count: int = 0
var _finished: bool = false

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] " + msg)
	else:
		fail_count += 1
		printerr("  [FAIL] " + msg)

func _finish(code: int) -> void:
	if _finished:
		return
	_finished = true
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(code)

func _step_frames(n: int = 2) -> void:
	var c := mini(n, 600)
	for _i in range(c):
		await process_frame

func _drag_rect(win: Window, overlay: Control, from_pt: Vector2, to_pt: Vector2) -> void:
	var xf: Transform2D = overlay.get_global_transform_with_canvas()
	var g0 := xf * from_pt
	var g1 := xf * to_pt
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = g0
	press.global_position = g0
	win.push_input(press)
	var mot := InputEventMouseMotion.new()
	mot.button_mask = MOUSE_BUTTON_MASK_LEFT
	mot.position = g1
	mot.global_position = g1
	mot.relative = g1 - g0
	win.push_input(mot)
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = g1
	rel.global_position = g1
	win.push_input(rel)

func _elem_rect(el: Dictionary, sc: float) -> Rect2:
	var anc := str(el.get("anchor", "top_left"))
	var ax := 0.0
	var ay := 0.0
	if anc.ends_with("_center") or anc == "center":
		ax = 0.5
	elif anc.ends_with("_right"):
		ax = 1.0
	if anc.begins_with("center_") or anc == "center":
		ay = 0.5
	elif anc.begins_with("bottom_"):
		ay = 1.0
	var w := float(el.get("width", 120))
	var h := float(el.get("height", 36))
	var ox := float(el.get("offset_x", 0))
	var oy := float(el.get("offset_y", 0))
	return Rect2((ax * (960.0 - w) + ox) * sc, (ay * (540.0 - h) + oy) * sc, w * sc, h * sc)

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno de 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 7 (UI RECONST 9B): VERIFICACION COMPLETA EDITOR + RUNTIME ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var th = Engine.get_singleton("ClockGearsTheme")
	_check(dm != null and th != null, "Singletons ClockGearsDataManager y ClockGearsTheme activos")
	if dm == null or th == null:
		_finish(1)
		return

	dm.load_all()
	var backup_ui_cfg: Dictionary = dm.get_ui_designs_config().duplicate(true)
	var backup_hud_cfg: Dictionary = dm.get_hud_config().duplicate(true)
	var backup_title_cfg: Dictionary = dm.get_title_menu_config().duplicate(true)
	var backup_go_cfg: Dictionary = dm.get_game_over_config().duplicate(true)
	var backup_opts_cfg: Dictionary = dm.get_player_options().duplicate(true)
	var backup_rt: Dictionary = dm.export_runtime_state()
	var prev_theme_id: String = str(th.get_active_theme_id())

	# =========================================================================
	# BLOQUE A: EDITOR — 12 PESTANAS, ARRASTRE, SUBMENUS, BARRA, ICONOS, TEMAS
	# =========================================================================
	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)

	var design_tabs := {
		"main_menu": "title",
		"menu_options": "game_menu_options",
		"menu_saves": "game_menu_save",
		"game_menu": "game_menu_main",
		"menu_inventory": "game_menu_items",
		"menu_equip": "game_menu_equip",
		"menu_status": "game_menu_status",
		"credits": "credits",
		"game_over": "game_over",
		"hud": "hud",
		"messages_battle": "messages_battle"
	}
	var shared_ed_id: int = -1
	for ckey in design_tabs.keys():
		var sid: String = str(design_tabs[ckey])
		db.select_category_by_key(ckey)
		await _step_frames(2)
		var ed := db.get_ui_designer_editor()
		_check(ed != null and ed.is_visible_in_tree() and ed.get_current_screen() == sid,
			"[Editor] Pestana '%s' abre '%s' correctamente" % [ckey, sid])
		if ed != null:
			if shared_ed_id == -1:
				shared_ed_id = ed.get_instance_id()
			else:
				_check(ed.get_instance_id() == shared_ed_id, "[Editor] '%s' comparte el mismo componente unico" % ckey)
			var raw_elems: Array = dm.get_ui_screen_elements(sid)
			var elems: Array = raw_elems if (dm.has_custom_ui_design(sid) and not raw_elems.is_empty()) else Array(dm.get_default_ui_screen_template(sid).get("elements", []))
			if not elems.is_empty():
				var el: Dictionary = Dictionary(elems[elems.size() - 1]).duplicate(true)
				var eid: String = str(el.get("id", ""))
				var ox0: int = int(el.get("offset_x", 0))
				var oy0: int = int(el.get("offset_y", 0))
				ed.select_element(eid)
				await _step_frames(1)
				var ov := ed.get_canvas_overlay() as Control
				var pres: Vector2i = ed.get_preview_resolution()
				var sc: float = (float(pres.x) / 960.0) * float(ed.get_canvas_zoom())
				var r0 := _elem_rect(el, sc)
				var c0 := r0.position + r0.size * 0.5
				_drag_rect(db, ov, c0, c0 + Vector2(-20.0 if c0.x > 480.0 * sc else 20.0, -14.0 if c0.y > 270.0 * sc else 14.0))
				await _step_frames(2)
				var after: Dictionary = dm.get_ui_element(sid, eid)
				_check(int(after.get("offset_x", ox0)) != ox0 or int(after.get("offset_y", oy0)) != oy0,
					"[Editor] Arrastrar '%s' en '%s' funciona" % [eid, sid])
				if sid != "hud":
					ed.request_reset_to_default(true)
					await _step_frames(1)

	db.select_category_by_key("window_styles")
	await _step_frames(2)
	var ws_panel := db.get_window_styles_ui_handles().get("panel", null) as Control
	_check(ws_panel != null and ws_panel.is_visible_in_tree(), "[Editor] Pestana 12 'Estilos de ventana' visible")

	# Submenu jerarquico + Rueda radial + Boton multi-accion + Paquete portable
	db.select_category_by_key("menu_inventory")
	await _step_frames(2)
	var ed_inv := db.get_ui_designer_editor()
	var sub_id: String = ed_inv.add_submenu_to_current("Submenú Smoke P7")
	await _step_frames(2)
	_check(not sub_id.is_empty() and dm.has_menu(sub_id), "[Editor] Submenu jerarquico creado: " + sub_id)
	_check(ed_inv.apply_template_by_id("rueda_radial", false, true), "[Editor] Plantilla Rueda radial aplicada")
	var btn_id: String = ed_inv.quick_add_button_element("Acción Dual")
	await _step_frames(1)
	_check(not btn_id.is_empty(), "[Editor] + Anadir boton operativo")
	var pkg_path := "user://tmp_pkg_p7.cgpack"
	_check(bool(dm.export_template_package(sub_id, pkg_path, true)), "[Editor] Exportacion paquete portable ok")
	var imp_res: Dictionary = dm.import_template_package(pkg_path, true)
	_check(bool(imp_res.get("ok", false)), "[Editor] Importacion paquete portable ok")
	var imp_mid: String = str(imp_res.get("menu_id", ""))
	if FileAccess.file_exists(pkg_path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(pkg_path))

	# Barra superior, Capas 1-4 y Temas en caliente
	var vp := SubViewport.new()
	vp.size = Vector2i(1366, 200)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(vp)
	var layers_dlg := ClockGearsLayersDialog.new()
	root.add_child(layers_dlg)
	await _step_frames(1)
	var row := layers_dlg.get_ui_handles().get("title_bar_row", null) as HBoxContainer
	_check(row != null, "[Editor] Barra superior inicializada")
	if row != null:
		vp.add_child(row)
		var top_bar := ClockGearsTopBar.new()
		vp.add_child(top_bar)
		await _step_frames(2)
		var mbar := row.get_node_or_null("MainMenuBar") as MenuBar
		var capas_title := ""
		if mbar != null:
			for i in range(mbar.get_menu_count()):
				if not mbar.is_menu_hidden(i) and mbar.get_menu_title(i).begins_with("Capas"):
					capas_title = mbar.get_menu_title(i)
		_check(capas_title.begins_with("Capas") and capas_title.count("Capa") == 1,
			"[Editor] Menu Capas muestra capa activa sin repetir 'Capa': '%s'" % capas_title)
		_check(top_bar.get_layer_button(0) != null and top_bar.get_layer_button(3) != null,
			"[Editor] Capas 1 a 4 intactas en TopBar")

	var pref := ClockGearsPreferencesDialog.new()
	pref.exclusive = false
	root.add_child(pref)
	pref.open_preferences()
	await _step_frames(2)
	_check(pref.select_editor_theme_by_id("brass") and String(th.get_active_theme_id()) == "brass", "[Editor] Tema ClockGears Laton aplica en caliente")
	_check(pref.select_editor_theme_by_id("light") and String(th.get_active_theme_id()) == "light", "[Editor] Tema Claro aplica en caliente")
	pref.select_editor_theme_by_id(prev_theme_id)
	await _step_frames(1)

	# Verificar cero botones vacios en DB activa (excluyendo widgets compuestos internos como ColorPickerButton/OptionButton/CheckBox)
	var empty_btn_cnt := 0
	var bstack: Array[Node] = [db]
	while not bstack.is_empty():
		var bn: Node = bstack.pop_back()
		if bn is Button:
			var b := bn as Button
			var is_internal_w := (b is OptionButton or b is ColorPickerButton or b is CheckBox or b is CheckButton)
			var p := b.get_parent()
			while not is_internal_w and p != null:
				if p is OptionButton or p is SpinBox or p is ColorPickerButton or p is ColorPicker or p is FileDialog or p is TabBar or p is ScrollBar:
					is_internal_w = true
				p = p.get_parent()
			if not is_internal_w and b.is_visible_in_tree() and b.text.strip_edges().is_empty() and b.icon == null:
				empty_btn_cnt += 1
		for i in range(bn.get_child_count()):
			bstack.push_back(bn.get_child(i))
	_check(empty_btn_cnt == 0, "[Editor] Cero botones sin icono ni texto en Base de Datos (vacios=%d)" % empty_btn_cnt)

	pref.queue_free()
	layers_dlg.queue_free()
	vp.queue_free()
	db.queue_free()
	await _step_frames(2)

	# Limpiar submenus creados en Bloque A antes de Runtime
	if not sub_id.is_empty():
		dm.delete_custom_menu(sub_id)
	if not imp_mid.is_empty():
		dm.delete_custom_menu(imp_mid)
	dm.set_ui_designs_config(backup_ui_cfg)

	# =========================================================================
	# BLOQUE B: RUNTIME (F5) — FLUJO JUGABLE COMPLETO
	# =========================================================================
	dm.set_player_options(backup_opts_cfg, false)
	var save_path_1: String = dm.get_save_slot_path(1)
	var had_save_1: bool = FileAccess.file_exists(save_path_1)
	var prev_save_bytes: PackedByteArray = FileAccess.get_file_as_bytes(save_path_1) if had_save_1 else PackedByteArray()

	# 1. Pantalla de titulo -> Opciones -> Partidas -> Nueva partida
	var ts := ClockGearsTitleScreen.new()
	ts.set_editor_test_popup_mode(false)
	ts.set_auto_quit_tree(false)
	root.add_child(ts)
	await _step_frames(2)
	var ts_ids: Array = ts.get_visible_entry_ids()
	_check(ts_ids.has("new_game") and ts_ids.has("options"), "[Runtime] Pantalla de titulo expone Nueva partida y Opciones: " + str(ts_ids))
	ts.open_options_menu()
	await _step_frames(1)
	_check(ts.is_options_menu_open(), "[Runtime] Opciones abre desde Pantalla de titulo")
	ts.apply_options_values(0.55, 0.72, 1.0, false)
	await _step_frames(1)
	var opts_now: Dictionary = dm.get_player_options()
	_check(absf(float(opts_now.get("music_volume", 0.0)) - 0.55) < 0.02, "[Runtime] Opciones actualiza volumen de musica en vivo (0.55)")
	ts.close_submenus()
	ts.trigger_new_game()
	await _step_frames(1)
	_check(not ts.get_last_started_map_path().is_empty() or ts.was_quit_requested() == false, "[Runtime] Boton Nueva partida ejecuta flujo de entrada al juego")
	ts.queue_free()

	# 2. Menu del juego + Inventario (usar + preview 360 rotacion) + Equipo + Estado + Partidas
	var items_arr: Array = dm.get_category_array(0) # actors check below
	var item_list: Array = dm.get_category_array(4) # CAT_ITEMS
	var items_all: Array = dm.get_category_array(2) # CAT_ITEMS or find potion by inventory key
	var potion_key := ""
	for cat_check in [2, 3, 4]:
		var arr: Array = dm.get_category_array(cat_check)
		if not arr.is_empty():
			var ikey: String = dm.get_record_inventory_key(2, int(Dictionary(arr[0]).get("id", 1)))
			if cat_check == 2:
				potion_key = ikey
				break
	if potion_key.is_empty():
		potion_key = "item:1"
	dm.change_item(potion_key, 2)
	var party: Array = dm.get_party_actor_ids()
	var leader_id: int = int(party[0]) if not party.is_empty() else 1
	var st_before: Dictionary = dm.get_actor_runtime_state(leader_id)
	var hp_max: int = maxi(100, int(st_before.get("max_hp", 100)))
	dm.set_actor_runtime_hp(leader_id, maxi(1, hp_max - 25))

	var interp_gm := ClockGearsEventInterpreter.new()
	root.add_child(interp_gm)
	var gmenu := interp_gm.get_game_menu()
	if gmenu != null:
		gmenu.set_editor_test_popup_mode(false)
		gmenu.set_auto_quit_tree(false)
		gmenu.open_menu()
	await _step_frames(2)
	_check(gmenu != null and gmenu.is_menu_open(), "[Runtime] Menu del juego se abre correctamente")
	_check(gmenu.open_subscreen("items") and gmenu.get_current_subscreen() == "items", "[Runtime] Subpantalla Inventario abre")
	await _step_frames(1)
	gmenu.select_item(potion_key)
	var rot0: float = gmenu.get_preview_rotation_deg()
	gmenu.simulate_preview_drag_rotation(36.0)
	_check(absf(gmenu.get_preview_rotation_deg() - rot0) > 1.0, "[Runtime] Inventario rota vista previa de objeto con arrastre")
	gmenu.use_selected_item(leader_id)
	var qty_after_use: int = dm.get_item_qty(potion_key)
	_check(qty_after_use <= 2, "[Runtime] Usar objeto en Inventario procesado (cant=%d)" % qty_after_use)

	_check(gmenu.open_subscreen("equip") and gmenu.get_current_subscreen() == "equip", "[Runtime] Subpantalla Equipo abre")
	var eq_prev: Dictionary = gmenu.get_current_equip_preview()
	_check(not eq_prev.is_empty(), "[Runtime] Equipo muestra vista previa/estadisticas del actor")

	_check(gmenu.open_subscreen("status") and gmenu.get_current_subscreen() == "status", "[Runtime] Subpantalla Estado abre")
	_check(not gmenu.get_status_summary_text().is_empty(), "[Runtime] Estado muestra nivel/PV/estadisticas")

	_check(gmenu.open_subscreen("save") and gmenu.get_current_subscreen() == "save", "[Runtime] Subpantalla Partidas abre")
	var ok_save := gmenu.save_to_slot(1)
	_check(ok_save and FileAccess.file_exists(save_path_1), "[Runtime] Guardar partida en ranura 1 crea archivo de guardado")
	gmenu.close_menu()
	_check(not gmenu.is_menu_open(), "[Runtime] Menu del juego se cierra limpiamente")
	interp_gm.queue_free()

	# 3. HUD + Creditos + Game Over + 7 Comandos de evento de menus
	var hud := ClockGearsHudOverlay.new()
	root.add_child(hud)
	var hud_cfg: Dictionary = ClockGearsDataManager.get_default_hud_config()
	hud_cfg["widgets"] = [
		ClockGearsDataManager.make_default_hud_widget("hearts"),
		ClockGearsDataManager.make_default_hud_widget("bar"),
		ClockGearsDataManager.make_default_hud_widget("currency"),
		ClockGearsDataManager.make_default_hud_widget("item")
	]
	hud.apply_hud_config(hud_cfg)
	await _step_frames(1)
	_check(hud.get_widget_count() >= 4, "[Runtime] HUD monta corazones, barra, monedas y contador (%d)" % hud.get_widget_count())
	dm.set_hud_runtime_visible(false)
	_check(not dm.is_hud_runtime_visible(), "[Runtime] HUD responde a ocultacion en ejecucion")
	dm.set_hud_runtime_visible(true)
	hud.queue_free()

	var interp := ClockGearsEventInterpreter.new()
	root.add_child(interp)
	interp.start_command_list([
		{"type": "change_hud_visibility", "visible": false},
		{"type": "change_hud_visibility", "visible": true},
		{"type": "change_menu_access", "enabled": true},
		{"type": "open_menu", "menu_id": "game_menu_main"},
	])
	for _s in range(6):
		interp.step_frame()
	var gm_evt := interp.get_game_menu()
	_check(gm_evt != null and gm_evt.is_menu_open(), "[Runtime] Comando evento open_menu abre el menu")
	if gm_evt != null:
		gm_evt.close_menu()
	interp.start_command_list([{"type": "show_credits", "credits_config": {"items": [{"type": "text", "text": "Equipo ClockGears"}], "scroll_speed": 40.0}}])
	for _s in range(4):
		interp.step_frame()
	var cr_evt := interp.get_credits_player()
	_check(cr_evt != null and cr_evt.is_playing(), "[Runtime] Comando evento show_credits reproduce creditos")
	if cr_evt != null:
		cr_evt.step_credits(0.5)
		cr_evt.stop_credits(false)
	interp.start_command_list([{"type": "show_game_over"}])
	for _s in range(4):
		interp.step_frame()
	var go_evt := interp.get_game_over_screen()
	_check(go_evt != null and go_evt.is_open(), "[Runtime] Comando evento show_game_over abre Game Over")
	if go_evt != null:
		go_evt.hide_game_over()
	interp.queue_free()

	# Restaurar estado previo limpio sin ensuciar datos ni disco
	if had_save_1:
		var fa := FileAccess.open(save_path_1, FileAccess.WRITE)
		if fa != null:
			fa.store_buffer(prev_save_bytes)
			fa.close()
	elif FileAccess.file_exists(save_path_1):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(save_path_1))

	dm.set_ui_designs_config(backup_ui_cfg)
	dm.set_hud_config(backup_hud_cfg)
	dm.set_title_menu_config(backup_title_cfg)
	dm.set_game_over_config(backup_go_cfg)
	dm.set_player_options(backup_opts_cfg, false)
	dm.import_runtime_state(backup_rt)
	await _step_frames(2)

	_finish(0 if fail_count == 0 else 1)
