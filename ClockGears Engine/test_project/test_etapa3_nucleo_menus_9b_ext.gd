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
	var fname := da.get_next()
	while fname != "":
		if fname != "." and fname != "..":
			var p := abs_dir.path_join(fname)
			if da.current_is_dir():
				_remove_dir_recursive_abs(p)
				DirAccess.remove_absolute(p)
			else:
				DirAccess.remove_absolute(p)
		fname = da.get_next()
	da.list_dir_end()
	DirAccess.remove_absolute(abs_dir)

func _make_solid_texture(w: int, h: int, col: Color) -> ImageTexture:
	var img := Image.create_empty(w, h, false, Image.FORMAT_RGBA8)
	img.fill(col)
	return ImageTexture.create_from_image(img)

func _find_first_button(node: Node) -> Button:
	if node == null:
		return null
	if node is Button and not (node is OptionButton):
		return node as Button
	for c in node.get_children():
		var found := _find_first_button(c)
		if found != null:
			return found
	return null

func _compare_images_exact(a: Image, b: Image) -> Dictionary:
	if a == null or b == null:
		return {"match": false, "diff_pixels": -1}
	if a.get_width() != b.get_width() or a.get_height() != b.get_height():
		return {"match": false, "diff_pixels": -2}
	var w := a.get_width()
	var h := a.get_height()
	var diff := 0
	for y in range(h):
		for x in range(w):
			var ca := a.get_pixel(x, y)
			var cb := b.get_pixel(x, y)
			if absf(ca.r - cb.r) > 0.01 or absf(ca.g - cb.g) > 0.01 or absf(ca.b - cb.b) > 0.01 or absf(ca.a - cb.a) > 0.01:
				diff += 1
	return {"match": diff == 0, "diff_pixels": diff}

func _run_tests() -> void:
	print("=== TEST ETAPA 3 (9B-EXT): NÚCLEO DE MENÚS ILIMITADOS ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager disponible")
	if dm == null:
		return

	var tmp_root := "user://test_etapa3_9b_ext"
	var data_dir := tmp_root.path_join("data")
	var mig_dir := tmp_root.path_join("mig_v1_data")
	var saves_dir := tmp_root.path_join("saves")
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(tmp_root))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(data_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(mig_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(saves_dir))

	dm.init_empty()
	dm.set_save_directory(saves_dir)

	# =========================================================================
	# 1. MIGRACIÓN AUTOMÁTICA V1 -> V2 + BACKUP + IDENTIDAD VISUAL
	# =========================================================================
	print("-- 1. Migración v1 -> v2 + backup + comparación visual --")
	var v1_ui_raw := FileAccess.get_file_as_string("res://clockgears_data/ui_designs.json")
	check(v1_ui_raw.find("\"schema_version\"") >= 0, "Proyecto base contiene ui_designs.json legítimo")

	# Copiar archivos legacy a mig_dir primero y cargarlos para capturar el render v1 con todos los datos reales
	var src_da := DirAccess.open("res://clockgears_data")
	if src_da:
		src_da.list_dir_begin()
		var fn := src_da.get_next()
		while fn != "":
			if not src_da.current_is_dir() and fn.ends_with(".json"):
				var content := FileAccess.get_file_as_string("res://clockgears_data".path_join(fn))
				var wf := FileAccess.open(mig_dir.path_join(fn), FileAccess.WRITE)
				if wf:
					wf.store_string(content)
			fn = src_da.get_next()
		src_da.list_dir_end()

	# Capturar apariencia pre-migración de Título y Menú del Juego con el proyecto real cargado (modo legacy)
	dm.load_all(mig_dir)
	var vp_v1 := SubViewport.new()
	vp_v1.size = Vector2i(960, 540)
	vp_v1.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(vp_v1)

	var bg_v1 := ColorRect.new()
	bg_v1.size = Vector2(960, 540)
	bg_v1.color = Color(0.05, 0.06, 0.09, 1.0)
	vp_v1.add_child(bg_v1)

	var gm_v1 := ClockGearsGameMenu.new()
	vp_v1.add_child(gm_v1)
	gm_v1.set_preview_mode(true)
	gm_v1.open_subscreen("main")
	await _step_frames(3)
	var img_gm_v1: Image = vp_v1.get_texture().get_image()
	vp_v1.queue_free()
	await _step_frames(1)

	# Asegurar schema_version = 1 en mig_dir/ui_designs.json (eliminando bak previo si hubiera)
	if FileAccess.file_exists(mig_dir.path_join("ui_designs.json.pre_menu_v2.bak")):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(mig_dir.path_join("ui_designs.json.pre_menu_v2.bak")))
	var v1_dict := {
		"schema_version": 1,
		"screens": {
			"title": {"custom_enabled": false, "elements": []},
			"game_menu_main": {"custom_enabled": false, "elements": []},
			"credits": {"custom_enabled": false, "elements": []},
			"hud": {"custom_enabled": false, "elements": []}
		}
	}
	var wf_v1 := FileAccess.open(mig_dir.path_join("ui_designs.json"), FileAccess.WRITE)
	wf_v1.store_string(JSON.stringify(v1_dict, "\t"))
	wf_v1.close()

	dm.clear_last_migrated_files()
	dm.load_all(mig_dir)

	var bak_path := mig_dir.path_join("ui_designs.json.pre_menu_v2.bak")
	check(FileAccess.file_exists(bak_path), "Migración creó respaldo automático .pre_menu_v2.bak antes de sobrescribir")
	var migrated_files: PackedStringArray = dm.get_last_migrated_files()
	check(migrated_files.size() >= 1 and migrated_files[0].find("ui_designs.json") >= 0, "dm.get_last_migrated_files() reporta ui_designs.json")
	var ui_v2_saved: Variant = JSON.parse_string(FileAccess.get_file_as_string(mig_dir.path_join("ui_designs.json")))
	check(ui_v2_saved is Dictionary and int(ui_v2_saved.get("schema_version", 0)) == 2, "ui_designs.json migrado a schema_version == 2")

	var vp_v2 := SubViewport.new()
	vp_v2.size = Vector2i(960, 540)
	vp_v2.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(vp_v2)
	var bg_v2 := ColorRect.new()
	bg_v2.size = Vector2(960, 540)
	bg_v2.color = Color(0.05, 0.06, 0.09, 1.0)
	vp_v2.add_child(bg_v2)
	var gm_v2 := ClockGearsGameMenu.new()
	vp_v2.add_child(gm_v2)
	gm_v2.set_preview_mode(true)
	gm_v2.open_subscreen("main")
	await _step_frames(3)
	var img_gm_v2: Image = vp_v2.get_texture().get_image()
	vp_v2.queue_free()
	await _step_frames(1)

	var cmp_gm := _compare_images_exact(img_gm_v1, img_gm_v2)
	check(cmp_gm["match"], "Menú del Juego migrado v1->v2 se renderiza 100%% idéntico al anterior (diff=%d)" % int(cmp_gm["diff_pixels"]))

	# =========================================================================
	# 2. 3A: MENÚS ILIMITADOS Y PLANTILLAS DE SISTEMA
	# =========================================================================
	print("-- 2. 3A: Creación, duplicado, orden, borrado y plantillas de sistema --")
	dm.init_empty()
	var sys_ids: PackedStringArray = dm.get_system_menu_ids()
	check(sys_ids.has("game_menu_main") and sys_ids.has("title") and sys_ids.has("credits") and sys_ids.has("hud"), "Lista de menús del sistema incluye Título/Menú/Créditos/HUD")
	check(not dm.delete_custom_menu("game_menu_main"), "No se permite borrar menús del sistema")

	var m1_id: String = dm.create_custom_menu("Menú de Habilidades")
	var m2_id: String = dm.create_custom_menu("Diario de Misiones Ñ")
	var m3_id: String = dm.create_custom_menu("Anillo Rápido ÁÉÍÓÚ")
	check(not m1_id.is_empty() and not m2_id.is_empty() and not m3_id.is_empty(), "Se crean 3 menús propios con acentos y ñ")

	var m_dup_id: String = dm.duplicate_menu(m2_id, "Diario Copia")
	check(not m_dup_id.is_empty() and m_dup_id != m2_id, "Duplicar menú genera nuevo ID único")
	check(dm.rename_menu(m_dup_id, "Diario Renombrado ¿¡"), "Renombrar menú soporta UTF-8")
	check(dm.move_custom_menu(m3_id, -1), "Reordenar menú personalizado con move_custom_menu funciona")
	check(dm.delete_custom_menu(m_dup_id), "Borrar menú personalizado funciona")

	# Restaurar plantilla original de sistema
	dm.set_ui_screen_design("game_menu_main", {"custom_enabled": true, "elements": [{"id": "tmp_btn", "type": "button", "label": "Temp"}]})
	check(dm.restore_system_menu_template("game_menu_main", false), "Restaurar plantilla original de game_menu_main")
	check(not dm.has_custom_ui_design("game_menu_main"), "game_menu_main vuelve a modo por defecto tras restaurar plantilla")

	# Persistencia save_all / load_all en data_dir
	dm.save_all(data_dir)
	dm.init_empty()
	dm.load_all(data_dir)
	var loaded_meta: Dictionary = dm.get_menu_metadata(m2_id)
	check(String(loaded_meta.get("name", "")) == "Diario de Misiones Ñ", "Nombre UTF-8 del menú propio persiste al guardar y recargar")

	# =========================================================================
	# 3. 3B, 3C, 3D: ENTRADAS, CONTEXTOS, PAUSA, CONFLICTO, COMANDO EVENTO Y MULTI-ACCIÓN
	# =========================================================================
	print("-- 3. 3B/3C/3D: Triggers, contextos, bloqueo, conflictos, abrir menú y multi-acción --")
	# Registrar acción reasignable en InputMap
	if not InputMap.has_action("test_cg_menu_act"):
		InputMap.add_action("test_cg_menu_act")
	InputMap.action_erase_events("test_cg_menu_act")
	var ev_t := InputEventKey.new()
	ev_t.keycode = KEY_T
	InputMap.action_add_event("test_cg_menu_act", ev_t)

	# Configurar m1_id: tecla J, solo mapa, pausa al abrir, botón multi-acción
	var red_tex_path := tmp_root.path_join("red_dot.png")
	var red_img := Image.create_empty(16, 16, false, Image.FORMAT_RGBA8)
	red_img.fill(Color(1, 0, 0, 1))
	red_img.save_png(ProjectSettings.globalize_path(red_tex_path))

	var ce_id := 1
	dm.set_category_array(14, [ # CAT_COMMON_EVENTS = 14
		{
			"id": ce_id,
			"name": "CE_Test_Menu",
			"commands": [
				{"type": "switch", "name": "ce_ok", "val": true},
				{"type": "variable", "name": "ce_num", "op": "=", "val": 77}
			]
		}
	])

	dm.set_switch("btn_enabled_sw", false)
	dm.set_switch("btn_vis_sw", true)

	var d1 := {
		"id": m1_id,
		"name": "Menú de Habilidades",
		"is_system": false,
		"custom_enabled": true,
		"pause_on_open": true,
		"trigger_context": "map",
		"respect_menu_access": true,
		"triggers": [
			{"type": "key", "key": "J"}
		],
		"sounds": {
			"open": "res://sfx_open.wav",
			"close": "res://sfx_close.wav",
			"move": "res://sfx_move.wav",
			"accept": "res://sfx_accept.wav",
			"cancel": "res://sfx_cancel.wav"
		},
		"cursor": {"type": "pill", "color": "#f5be54", "animation": "pulse", "size": 16},
		"elements": [
			{
				"id": "btn_chain",
				"type": "button",
				"label": "Cadena",
				"anchor": "top_left",
				"offset_x": 40,
				"offset_y": 40,
				"width": 160,
				"height": 42,
				"focus_order": 0,
				"focus_next_right": "btn_cond",
				"actions": [
					{"type": "set_switch", "switch": "sw_chain", "value": true},
					{"type": "set_variable", "variable": "var_chain", "op": "+", "value": 50},
					{"type": "play_sound", "sound": "res://sfx_step.wav"},
					{"type": "hud_visibility", "scope": "all", "visible": false},
					{"type": "common_event", "common_event_id": 1},
					{"type": "open_menu", "target": m2_id}
				]
			},
			{
				"id": "btn_cond",
				"type": "button",
				"label": "Condicional",
				"anchor": "top_left",
				"offset_x": 240,
				"offset_y": 40,
				"width": 160,
				"height": 42,
				"focus_order": 1,
				"visibility_mode": "switch",
				"visibility_switch": "btn_vis_sw",
				"enabled_mode": "switch",
				"enabled_switch": "btn_enabled_sw",
				"actions": [{"type": "close"}]
			}
		]
	}
	dm.set_ui_screen_design(m1_id, d1, false)

	var d2 := {
		"id": m2_id,
		"name": "Diario de Misiones Ñ",
		"is_system": false,
		"custom_enabled": true,
		"pause_on_open": false,
		"trigger_context": "map_and_battle",
		"respect_menu_access": true,
		"sounds": {
			"cancel": "res://sfx_cancel.wav"
		},
		"triggers": [
			{"type": "joy_button", "button": JOY_BUTTON_Y},
			{"type": "mouse_button", "button": MOUSE_BUTTON_RIGHT},
			{"type": "action", "action": "test_cg_menu_act"}
		],
		"elements": [
			{"id": "btn_back", "type": "button", "label": "Volver", "anchor": "top_left", "offset_x": 40, "offset_y": 40, "width": 140, "height": 40, "actions": [{"type": "back"}]}
		]
	}
	dm.set_ui_screen_design(m2_id, d2, false)

	# Comprobar detección de conflictos en mismo contexto / contexto solapado
	var d_conflict: Dictionary = dm.get_ui_screen_design(m3_id)
	d_conflict["triggers"] = [{"type": "key", "key": "J"}]
	d_conflict["trigger_context"] = "map_and_battle"
	dm.set_ui_screen_design(m3_id, d_conflict, false)
	var conflicts: Array = dm.get_menu_trigger_conflicts()
	var found_j_conflict := false
	for cf in conflicts:
		if cf is Dictionary and (String(cf.get("signature", "")) == "key:%d" % KEY_J or String(cf.get("trigger_label", "")).find("J") >= 0):
			found_j_conflict = true
	check(found_j_conflict, "get_menu_trigger_conflicts detecta tecla J repetida entre menús con contexto solapado")

	# Restaurar d3 limpio para prueba radial en punto 4
	d_conflict["triggers"] = [{"type": "key", "key": "K"}]
	d_conflict["trigger_context"] = "any"
	dm.set_ui_screen_design(m3_id, d_conflict, false)
	var conflicts_after: Array = dm.get_menu_trigger_conflicts()
	var j_still := false
	for cf in conflicts_after:
		if cf is Dictionary and String(cf.get("signature", "")) == "key:%d" % KEY_J:
			j_still = true
	check(not j_still, "Al quitar tecla duplicada desaparece el conflicto de J")

	# Montar ClockGearsEventInterpreter en el árbol para probar runtime y F5
	var interp := ClockGearsEventInterpreter.new()
	get_root().add_child(interp)
	await _step_frames(2)
	var gm: ClockGearsGameMenu = interp.get_game_menu()
	check(gm != null, "Intérprete crea ClockGearsGameMenu en runtime")

	# Verificar estado disabled por interruptor en m1_id antes de activar interruptor
	gm.open_menu_by_id(m1_id, false, false, false)
	await _step_frames(2)
	check(gm.is_menu_open() and gm.get_active_menu_id() == m1_id, "open_menu_by_id abre menú propio")
	check(gm.is_tree_paused_by_menu() and paused, "pause_on_open == true pausa el SceneTree")

	var layer: ClockGearsUiDesignLayer = gm.get_custom_ui_layer()
	var cond_elem_root: Control = layer.get_element_control("btn_cond")
	var b_cond: Button = _find_first_button(cond_elem_root)
	check(b_cond != null and b_cond.disabled, "Botón con enabled_mode='switch' empieza deshabilitado")
	dm.set_switch("btn_enabled_sw", true)
	layer.refresh_live_values()
	check(b_cond != null and not b_cond.disabled, "Activar interruptor habilita automáticamente el botón")
	dm.set_switch("btn_vis_sw", false)
	layer.refresh_live_values()
	check(cond_elem_root != null and not cond_elem_root.visible, "Desactivar interruptor de visibilidad oculta el botón")

	# Activar botón multi-acción
	check(gm.trigger_custom_element_by_id("btn_chain"), "Botón multi-acción ejecuta toda la secuencia")
	await _step_frames(1)
	check(dm.get_switch("sw_chain"), "Acción 1 set_switch ejecutada")
	check(dm.get_variable("var_chain") == 50, "Acción 2 set_variable ejecutada (=50)")
	check(not dm.is_hud_runtime_visible(), "Acción 4 hud_visibility oculta el HUD")
	check(dm.get_switch("ce_ok") and dm.get_variable("ce_num") == 77, "Acción 5 common_event ejecuta comando de evento común")
	check(gm.get_active_menu_id() == m2_id and gm.get_menu_history_depth() == 1, "Acción 6 open_menu navega a m2 con historial profundidad 1")
	check(not gm.is_tree_paused_by_menu() and not paused, "Cambiar a menú con pause_on_open=false quita la pausa del SceneTree")

	# Volver al menú anterior y cerrar
	gm.handle_input_key(KEY_ESCAPE)
	await _step_frames(1)
	check(gm.get_active_menu_id() == m1_id and gm.get_menu_history_depth() == 0, "Esc/Back regresa al menú anterior (m1) desde pila de historial")
	check(gm.get_last_played_menu_sound() == "res://sfx_cancel.wav", "Sonido cancel registrado al volver")
	gm.close_menu()
	await _step_frames(1)
	check(not gm.is_menu_open() and not paused, "Cerrar menú despausa el SceneTree correctamente")

	# Triggers: teclado, mando, ratón, acción reasignable, Bloquear Menú, Mensaje abierto
	var ev_j := InputEventKey.new()
	ev_j.keycode = KEY_J
	ev_j.pressed = true
	check(gm.handle_trigger_input_event(ev_j, "map") and gm.get_active_menu_id() == m1_id, "Tecla J abre menú 1 en contexto map")
	gm.close_menu()

	check(not gm.handle_trigger_input_event(ev_j, "battle"), "Tecla J NO abre menú 1 si contexto es 'battle' y el menú es solo mapa")

	var ev_joy := InputEventJoypadButton.new()
	ev_joy.button_index = JOY_BUTTON_Y
	ev_joy.pressed = true
	check(gm.handle_trigger_input_event(ev_joy, "battle") and gm.get_active_menu_id() == m2_id, "Botón de mando Y abre menú 2 en contexto map_and_battle")
	gm.close_menu()

	var ev_mb := InputEventMouseButton.new()
	ev_mb.button_index = MOUSE_BUTTON_RIGHT
	ev_mb.pressed = true
	check(gm.handle_trigger_input_event(ev_mb, "map") and gm.get_active_menu_id() == m2_id, "Botón de ratón derecho abre menú 2")
	gm.close_menu()

	# Reasignar acción InputMap de T -> U
	InputMap.action_erase_events("test_cg_menu_act")
	var ev_u := InputEventKey.new()
	ev_u.keycode = KEY_U
	ev_u.pressed = true
	InputMap.action_add_event("test_cg_menu_act", ev_u)
	var press_t := InputEventKey.new()
	press_t.keycode = KEY_T
	press_t.pressed = true
	var press_u := InputEventKey.new()
	press_u.keycode = KEY_U
	press_u.pressed = true
	check(not gm.handle_trigger_input_event(press_t, "map"), "Tras reasignar acción de InputMap, tecla antigua T ya no dispara menú 2")
	check(gm.handle_trigger_input_event(press_u, "map") and gm.get_active_menu_id() == m2_id, "Tras reasignar acción de InputMap, tecla nueva U sí abre menú 2")
	gm.close_menu()

	# Bloquear / Permitir Menú
	dm.set_game_menu_disabled(true)
	check(not gm.handle_trigger_input_event(ev_j, "map"), "Con menú bloqueado no se abre menú por entrada del jugador")
	dm.set_game_menu_disabled(false)

	# Ventana de mensajes abierta impide abrir cualquier menú (incluso contexto 'any')
	var mw: ClockGearsMessageWindow = interp.get_message_window()
	mw.show_message("Diálogo en curso", "NPC")
	var ev_k := InputEventKey.new()
	ev_k.keycode = KEY_K
	ev_k.pressed = true
	check(not gm.handle_trigger_input_event(ev_k, "other"), "Con ventana de mensajes abierta NO se abre menú ni en contexto 'any'")
	mw.hide()
	mw.force_finish() if mw.has_method("force_finish") else null
	# Limpiar estado visible/busy de message window
	if mw.has_method("skip_or_confirm"):
		mw.skip_or_confirm()
	await _step_frames(1)

	# 3C: Comando de evento "open_menu" (nuevo menu_id y legado sin menu_id)
	interp.start_command_list([{"type": "open_menu", "code": 351}])
	await _step_frames(2)
	check(gm.is_menu_open() and gm.get_active_menu_id() == "game_menu_main", "Comando open_menu antiguo (sin menu_id) abre game_menu_main igual que antes")
	gm.close_menu()
	await _step_frames(1)

	interp.start_command_list([{"type": "open_menu", "code": 351, "menu_id": m1_id}])
	await _step_frames(2)
	check(gm.is_menu_open() and gm.get_active_menu_id() == m1_id, "Comando open_menu con menu_id abre el menú personalizado elegido")
	gm.close_menu()
	await _step_frames(1)
	interp.queue_free()

	# Verificar selector en diálogo de editor (3C)
	var ed_dlg := ClockGearsEventEditorDialog.new()
	get_root().add_child(ed_dlg)
	var handles: Dictionary = ed_dlg.get_menu_command_ui_handles()
	var om_opt: OptionButton = handles.get("open_menu_selector_opt", null)
	check(om_opt != null, "Editor de eventos expone open_menu_selector_opt en dlg_open_menu")
	var dummy_ev := ClockGearsEvent.new()
	dummy_ev.set_pages([{"conditions": {}, "trigger": 0, "commands": []}])
	ed_dlg.edit_event(dummy_ev)
	var cmd_sel: Node = handles.get("command_selector", null)
	if cmd_sel != null:
		cmd_sel.emit_signal("command_selected", 351) # CMD_OPEN_MENU
	var found_m1_in_opt := -1
	if om_opt:
		for i in range(om_opt.get_item_count()):
			if String(om_opt.get_item_metadata(i)) == m1_id:
				found_m1_in_opt = i
				break
		check(found_m1_in_opt >= 0, "Selector Abrir Menú lista el menú personalizado creado")
		if found_m1_in_opt >= 0:
			om_opt.select(found_m1_in_opt)
	var dlg_om: ConfirmationDialog = handles.get("dlg_open_menu", null)
	if dlg_om:
		dlg_om.emit_signal("confirmed")
	var working_cmds: Array = ed_dlg.get_working_pages()[0]["commands"]
	check(not working_cmds.is_empty() and String(working_cmds[0].get("menu_id", "")) == m1_id, "Confirmar dlg_open_menu guarda menu_id seleccionado en el comando")
	check(ed_dlg.format_command_preview(working_cmds[0]).find("Menú de Habilidades") >= 0, "Vista previa del comando muestra el nombre del menú elegido")
	dummy_ev.queue_free()
	ed_dlg.queue_free()

	# =========================================================================
	# 4. 3E: PRIORIDAD MENÚ RADIAL PROPIO (8 RANURAS, 0 NODOS POR RANURA, _draw())
	# =========================================================================
	print("-- 4. 3E: Menú radial propio de 8 ranuras (0 nodos por ranura) --")
	var radial_slots := []
	for s in range(8):
		radial_slots.append({
			"id": "slot_%d" % s,
			"title": "Habilidad %d" % (s + 1),
			"description": "Descripción de la ranura radial %d" % (s + 1),
			"icon": red_tex_path,
			"pixel_art": true,
			"actions": [{"type": "set_variable", "variable": "radial_picked", "op": "=", "value": s + 10}]
		})

	var d_radial := {
		"id": m3_id,
		"name": "Anillo Rápido ÁÉÍÓÚ",
		"is_system": false,
		"custom_enabled": true,
		"layout": {
			"mode": "radial",
			"radial_radius": 140,
			"radial_start_angle_deg": -90.0,
			"radial_sweep_angle_deg": 360.0,
			"radial_clockwise": true,
			"radial_slot_size": 52,
			"radial_selected_scale": 1.35,
			"radial_show_center_info": true,
			"radial_slots": radial_slots
		},
		"elements": []
	}
	dm.set_ui_screen_design(m3_id, d_radial, false)

	var vp_rad := SubViewport.new()
	vp_rad.size = Vector2i(960, 540)
	vp_rad.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(vp_rad)
	var gm_rad := ClockGearsGameMenu.new()
	vp_rad.add_child(gm_rad)
	check(gm_rad.open_menu_by_id(m3_id, false, false, true), "Menú radial propio abre correctamente")
	await _step_frames(3)

	var rad_layer: ClockGearsUiDesignLayer = gm_rad.get_custom_ui_layer()
	check(rad_layer.is_radial_mode(), "ClockGearsUiDesignLayer activa modo radial")
	var rad_canvas: ClockGearsRadialCanvas = rad_layer.get_radial_canvas()
	check(rad_canvas != null, "ClockGearsRadialCanvas existe")
	check(rad_canvas.get_child_count() == 0, "Menú radial usa _draw() propio con 0 nodos hijos por ranura")
	check(rad_canvas.get_slot_count() == 8, "Menú radial carga exactamente 8 ranuras")
	var cinfo0: Dictionary = rad_canvas.get_center_displayed_info()
	check(String(cinfo0.get("title", "")) == "Habilidad 1" and String(cinfo0.get("description", "")).find("ranura radial 1") >= 0, "Centro del anillo muestra título y descripción de ranura activa")

	# Teclado derecha / izquierda y vector direccional de mando (ángulo derecho 0 rad -> ranura 2 en reloj desde arriba)
	gm_rad.handle_input_key(KEY_RIGHT)
	await _step_frames(12)
	check(rad_canvas.get_selected_index() == 1, "Tecla Derecha avanza selección radial a ranura 1")
	check(rad_canvas.get_slot_visual_scale(1) > rad_canvas.get_slot_visual_scale(0), "Ranura activa 1 se amplía respecto a las demás")

	rad_canvas.select_by_angle_vector(Vector2(1, 0)) # Este = ranura 2 en 8 slots desde -90° sentido horario
	await _step_frames(2)
	check(rad_canvas.get_selected_index() == 2, "Vector de stick/dirección selecciona la ranura derecha (índice 2)")

	# Posición de ratón por ángulo sobre ranura 4 (Sur) y clic real vía SubViewport.push_input sobre ranura 5 (Suroeste)
	var pos4: Vector2 = rad_canvas.get_slot_local_pos(4)
	rad_canvas.select_by_local_point(pos4)
	check(rad_canvas.get_selected_index() == 4, "Posición de ratón sobre ranura 4 la selecciona por ángulo")
	var pos5_vp: Vector2 = rad_canvas.get_global_transform_with_canvas() * rad_canvas.get_slot_local_pos(5)
	var mb_l := InputEventMouseButton.new()
	mb_l.button_index = MOUSE_BUTTON_LEFT
	mb_l.pressed = true
	mb_l.position = pos5_vp
	mb_l.global_position = pos5_vp
	vp_rad.push_input(mb_l)
	await _step_frames(2)
	check(rad_canvas.get_selected_index() == 5, "Clic real en ranura 5 selecciona ranura 5 por ángulo")
	check(dm.get_variable("radial_picked") == 15, "Clic en ranura 5 ejecuta su acción (var radial_picked == 15)")

	vp_rad.queue_free()
	await _step_frames(1)

	# =========================================================================
	# 5. 3E OTRAS DISTRIBUCIONES (grid, tabs, carousel) + 3F FORMAS CON RECORTE REAL
	# =========================================================================
	print("-- 5. 3E/3F: Grid/Tabs/Carousel y Recorte REAL de formas en SubViewport --")
	var vp_shape := SubViewport.new()
	vp_shape.size = Vector2i(960, 540)
	vp_shape.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(vp_shape)
	var shape_bg := ColorRect.new()
	shape_bg.size = Vector2(960, 540)
	shape_bg.color = Color(0.0, 0.0, 1.0, 1.0) # Fondo azul puro para comprobar recorte fuera de la forma
	vp_shape.add_child(shape_bg)

	var red_full_path := tmp_root.path_join("solid_red_160.png")
	var rf := Image.create_empty(160, 160, false, Image.FORMAT_RGBA8)
	rf.fill(Color(1.0, 0.0, 0.0, 1.0))
	rf.save_png(ProjectSettings.globalize_path(red_full_path))

	var ui_shapes := ClockGearsUiDesignLayer.new()
	ui_shapes.size = Vector2(960, 540)
	vp_shape.add_child(ui_shapes)
	var design_shapes := {
		"id": "custom_shapes_test",
		"custom_enabled": true,
		"elements": [
			{
				"id": "img_circle",
				"type": "image",
				"anchor": "top_left",
				"offset_x": 100,
				"offset_y": 100,
				"width": 160,
				"height": 160,
				"image_path": red_full_path,
				"scale_mode": "stretch",
				"pixel_art": false,
				"shape": {"type": "circle", "border_width": 0, "shadow": false, "glow": false}
			},
			{
				"id": "img_pill",
				"type": "image",
				"anchor": "top_left",
				"offset_x": 360,
				"offset_y": 100,
				"width": 200,
				"height": 60,
				"image_path": red_full_path,
				"scale_mode": "stretch",
				"pixel_art": false,
				"shape": {"type": "pill", "border_width": 0, "shadow": false, "glow": false}
			}
		]
	}
	ui_shapes.apply_screen_design("custom_shapes_test", design_shapes)
	await _step_frames(3)
	var shape_img: Image = vp_shape.get_texture().get_image()
	# En círculo inscrito en [100..260, 100..260], esquina (104,104) está FUERA del círculo (dist al centro (180,180) ≈ 107 > 80) -> debe ser azul; centro (180,180) -> rojo!
	var c_out: Color = shape_img.get_pixel(104, 104)
	var c_in: Color = shape_img.get_pixel(180, 180)
	check(c_out.b > 0.7 and c_out.r < 0.3, "Forma 'circle' recorta realmente la esquina exterior (pixel queda azul=%s)" % str(c_out))
	check(c_in.r > 0.7 and c_in.b < 0.3, "Forma 'circle' conserva contenido interior (pixel centro rojo=%s)" % str(c_in))

	# En píldora [360..560, 100..160] (radio 30), esquina (362, 102) dist al centro esquina (390, 130) ≈ 39.6 > 30 -> fuera -> azul; centro (460,130) -> rojo!
	var p_out: Color = shape_img.get_pixel(362, 102)
	var p_in: Color = shape_img.get_pixel(460, 130)
	check(p_out.b > 0.7 and p_out.r < 0.3, "Forma 'pill' recorta realmente la esquina exterior (pixel=%s)" % str(p_out))
	check(p_in.r > 0.7 and p_in.b < 0.3, "Forma 'pill' conserva contenido interior (pixel=%s)" % str(p_in))

	# Probar contenedor grid + paginación + tabs + carrusel
	var grid_elems := []
	for i in range(6):
		grid_elems.append({"id": "gbtn_%d" % i, "type": "button", "label": "G%d" % i, "width": 100, "height": 36, "tab_index": i % 2})
	ui_shapes.apply_screen_design("grid_test", {
		"id": "grid_test",
		"custom_enabled": true,
		"layout": {"mode": "grid", "columns": 2, "rows": 2, "spacing_x": 12, "spacing_y": 12, "pagination": true, "container_width": 320, "container_height": 200},
		"elements": grid_elems
	})
	await _step_frames(1)
	var gstate0: Dictionary = ui_shapes.get_layout_state_info()
	check(String(gstate0.get("mode", "")) == "grid" and int(gstate0.get("current_page", 0)) == 0, "Contenedor Grid con paginación arranca en página 0")
	ui_shapes.apply_screen_design("tabs_test", {
		"id": "tabs_test",
		"custom_enabled": true,
		"layout": {"mode": "tabs", "tab_labels": PackedStringArray(["Tab A", "Tab B"]), "width": 320, "height": 200},
		"elements": grid_elems
	})
	await _step_frames(1)
	check(ui_shapes.get_element_control("gbtn_0").visible and not ui_shapes.get_element_control("gbtn_1").visible, "Contenedor Tabs filtra elementos de la pestaña activa")
	ui_shapes.apply_screen_design("carousel_test", {
		"id": "carousel_test",
		"custom_enabled": true,
		"layout": {"mode": "carousel", "container_width": 320, "container_height": 200},
		"elements": grid_elems
	})
	var cstate: Dictionary = ui_shapes.get_layout_state_info()
	check(String(cstate.get("mode", "")) == "carousel", "Contenedor Carrusel activo correctamente")

	vp_shape.queue_free()
	await _step_frames(1)

	# Limpieza de InputMap & archivos temporales de prueba
	if InputMap.has_action("test_cg_menu_act"):
		InputMap.erase_action("test_cg_menu_act")
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(tmp_root))
	dm.init_empty()

func _initialize() -> void:
	call_deferred("_start")

func _start() -> void:
	await _run_tests()
	print("RESULTADOS_ETAPA3: %d PASSED, %d FAILED" % [passed, failed])
	quit(0 if failed == 0 else 1)
