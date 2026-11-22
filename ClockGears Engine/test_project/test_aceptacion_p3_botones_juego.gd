extends SceneTree

# Prueba de aceptación Punto 3: Botones añadidos con "+ Botón" aparecen y funcionan en TODAS las pantallas del juego real.

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

func _click_at(win: Window, pos: Vector2) -> void:
	win.notification(Viewport.NOTIFICATION_VP_MOUSE_ENTER)
	var ev_m := InputEventMouseMotion.new()
	ev_m.position = pos
	ev_m.global_position = pos
	win.push_input(ev_m, true)
	await _step_frames(1)

	var ev_d := InputEventMouseButton.new()
	ev_d.button_index = MOUSE_BUTTON_LEFT
	ev_d.pressed = true
	ev_d.position = pos
	ev_d.global_position = pos
	win.push_input(ev_d, true)
	await _step_frames(1)

	var ev_u := InputEventMouseButton.new()
	ev_u.button_index = MOUSE_BUTTON_LEFT
	ev_u.pressed = false
	ev_u.position = pos
	ev_u.global_position = pos
	win.push_input(ev_u, true)
	await _step_frames(2)

func _press_key(win: Window, kc: Key) -> void:
	var ev_d := InputEventKey.new()
	ev_d.keycode = kc
	ev_d.physical_keycode = kc
	ev_d.pressed = true
	win.push_input(ev_d, true)
	await _step_frames(1)

	var ev_u := InputEventKey.new()
	ev_u.keycode = kc
	ev_u.physical_keycode = kc
	ev_u.pressed = false
	win.push_input(ev_u, true)
	await _step_frames(2)

func _find_button_by_text(root_node: Node, needle: String) -> Button:
	if root_node == null:
		return null
	for n in root_node.find_children("*", "Button", true, false):
		var b := n as Button
		if b != null and b.text == needle:
			return b
	return null

func _click_control_real(ctrl: Control) -> void:
	if ctrl == null:
		return
	var win := ctrl.get_viewport() as Window
	var pos := ctrl.get_global_rect().get_center()
	while win != null and win.is_embedded() and win.get_parent() != null:
		pos += Vector2(win.position)
		var pwin := win.get_parent().get_viewport() as Window
		while pwin != null and pwin != root and not pwin.gui_embed_subwindows:
			pwin = (pwin.get_parent().get_viewport() as Window) if pwin.get_parent() != null else null
		if pwin == null or pwin == win:
			break
		win = pwin
	if win != null:
		await _click_at(win, pos)

func _configure_created_button_and_confirm(ed: Object, dm: Object, label_txt: String, sw_name: String) -> String:
	var cw: Window = ed.get_create_button_window()
	if cw == null or not cw.visible:
		printerr("DEBUG P3: cw null o invisible! cw=", cw, " vis=", cw.visible if cw else false)
		return ""
	await _step_frames(2)
	var ch: Dictionary = cw.get_ui_handles()
	var te: TextEdit = ch.get("text_edit", null) as TextEdit
	if te != null:
		te.set_text(label_txt)
		te.emit_signal("text_changed")
	var btn_confirm: Button = ch.get("create_confirm_btn", null) as Button
	if btn_confirm != null and btn_confirm.is_visible_in_tree():
		await _click_control_real(btn_confirm)
	else:
		cw.confirm_creation()
	await _step_frames(2)
	var new_id: String = ed.get_selected_element_id()
	var sid: String = ed.get_current_screen()
	if not new_id.is_empty():
		var el: Dictionary = dm.get_ui_element(sid, new_id)
		if not el.is_empty():
			el["action"] = "set_switch"
			el["actions"] = [{"type": "set_switch", "switch_name": sw_name, "value": true}]
			dm.update_ui_element(sid, new_id, el, false)
	return new_id

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== ACEPTACIÓN PUNTO 3: BOTONES AÑADIDOS VISIBLES Y ACTIVOS EN EL JUEGO REAL ===")
	DisplayServer.window_set_size(Vector2i(1366, 768))
	root.content_scale_size = Vector2i(1366, 768)
	root.size = Vector2i(1366, 768)
	await _step_frames(2)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var cap_dir := ProjectSettings.globalize_path("res://capturas_aceptacion")
	DirAccess.make_dir_recursive_absolute(cap_dir)
	var ui_file := ProjectSettings.globalize_path("res://clockgears_data/ui_designs.json")
	var hud_file := ProjectSettings.globalize_path("res://clockgears_data/hud.json")
	var orig_ui_json := FileAccess.get_file_as_string(ui_file) if FileAccess.file_exists(ui_file) else ""
	var orig_hud_json := FileAccess.get_file_as_string(hud_file) if FileAccess.file_exists(hud_file) else ""

	# 1. Abrir editor real, añadir un botón con "+ Botón" en las 12 categorías de Menús y pulsar OK
	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)

	var cat_to_sid := {
		"game_menu": "game_menu_main",
		"menu_inventory": "game_menu_items",
		"menu_equip": "game_menu_equip",
		"menu_status": "game_menu_status",
		"menu_saves": "game_menu_save",
		"menu_options": "game_menu_options",
		"main_menu": "title",
		"credits": "credits",
		"hud": "hud",
		"game_over": "game_over",
		"messages": "messages",
		"battle": "battle",
	}

	var ed := db.get_ui_designer_editor()
	for cat_key in cat_to_sid.keys():
		db.select_category_by_key(cat_key)
		await _step_frames(3)
		var h: Dictionary = ed.get_ui_handles()
		var add_btn: Button = h.get("layer_add_button_btn", null) as Button
		_check(add_btn != null and add_btn.is_visible_in_tree(), "Botón '+ Botón' visible en %s" % cat_key)
		if add_btn != null:
			await _click_control_real(add_btn)
		var nid := await _configure_created_button_and_confirm(ed, dm, "P3_" + str(cat_key), "sw_p3_" + str(cat_key))
		var created_el: Dictionary = dm.get_ui_element(cat_to_sid[cat_key], nid)
		_check(not nid.is_empty() and str(created_el.get("type", "")) == "button", "'+ Botón' crea botón '%s' en %s" % [nid, str(cat_key)])

	# Guardar con clic real en OK (persiste ui_designs.json y hud.json en disco)
	await _click_control_real(db.get_ok_button())
	await _step_frames(3)
	db.queue_free()
	await _step_frames(2)

	# Simular apertura limpia del juego real cargando desde disco
	dm.load_all()
	for k in cat_to_sid.keys():
		dm.set_switch("sw_p3_" + k, false)

	# 2. Probar las 6 pantallas de ClockGearsGameMenu en el juego real
	var gm = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm)
	await _step_frames(2)

	var gm_cats := ["game_menu", "menu_inventory", "menu_equip", "menu_status", "menu_saves", "menu_options"]
	for cat_key in gm_cats:
		var sid: String = cat_to_sid[cat_key]
		gm.open_menu_by_id(sid, false, false, true)
		await _step_frames(4)
		var layer: Control = gm.get_custom_ui_layer()
		var lbl: String = "P3_" + str(cat_key)
		var btn := _find_button_by_text(layer, lbl)
		var vp_rect := Rect2(Vector2.ZERO, Vector2(root.size))
		var vis_ok := btn != null and btn.is_visible_in_tree() and vp_rect.intersects(btn.get_global_rect())
		_check(vis_ok, "GameMenu (%s): '%s' visible en juego real (%s)" % [sid, lbl, str(btn.get_global_rect()) if btn != null else "null"])
		if cat_key == "game_menu":
			root.get_texture().get_image().save_png(cap_dir.path_join("p3_juego_menu_principal.png"))
		elif cat_key == "menu_inventory":
			root.get_texture().get_image().save_png(cap_dir.path_join("p3_juego_inventario.png"))
		if btn != null:
			# Probar clic de ratón real en el centro del botón
			await _click_at(root, btn.get_global_rect().get_center())
			_check(dm.get_switch("sw_p3_" + cat_key) == true, "GameMenu (%s): clic real ejecuta acción del botón añadido" % sid)
			dm.set_switch("sw_p3_" + cat_key, false)
			# Probar foco y Enter de teclado
			btn.grab_focus()
			await _step_frames(1)
			await _press_key(root, KEY_ENTER)
			_check(dm.get_switch("sw_p3_" + cat_key) == true, "GameMenu (%s): teclado (Enter con foco) ejecuta acción del botón" % sid)
	gm.close_menu()
	gm.queue_free()
	await _step_frames(2)

	# 3. TitleScreen
	var ts = ClassDB.instantiate("ClockGearsTitleScreen")
	root.add_child(ts)
	await _step_frames(4)
	var ts_btn := _find_button_by_text(ts.get_custom_ui_layer(), "P3_main_menu")
	_check(ts_btn != null and ts_btn.is_visible_in_tree(), "TitleScreen: 'P3_main_menu' visible en juego real")
	if ts_btn != null:
		await _click_at(root, ts_btn.get_global_rect().get_center())
		_check(dm.get_switch("sw_p3_main_menu") == true, "TitleScreen: clic real ejecuta acción del botón")
		dm.set_switch("sw_p3_main_menu", false)
		ts_btn.grab_focus()
		await _step_frames(1)
		await _press_key(root, KEY_ENTER)
		_check(dm.get_switch("sw_p3_main_menu") == true, "TitleScreen: teclado (Enter con foco) ejecuta acción del botón")
	ts.queue_free()
	await _step_frames(2)

	# 4. CreditsPlayer
	var cr = ClassDB.instantiate("ClockGearsCreditsPlayer")
	root.add_child(cr)
	cr.start_credits({})
	await _step_frames(4)
	var cr_btn := _find_button_by_text(cr.get_custom_ui_layer(), "P3_credits")
	_check(cr_btn != null and cr_btn.is_visible_in_tree(), "CreditsPlayer: 'P3_credits' visible en juego real")
	if cr_btn != null:
		await _click_at(root, cr_btn.get_global_rect().get_center())
		_check(dm.get_switch("sw_p3_credits") == true, "CreditsPlayer: clic real ejecuta acción del botón sin ser bloqueado")
	cr.queue_free()
	await _step_frames(2)

	# 5. HudOverlay
	var hud = ClassDB.instantiate("ClockGearsHudOverlay")
	root.add_child(hud)
	hud.set_preview_mode(false)
	hud.reload_from_data_manager()
	await _step_frames(4)
	var hud_btn := _find_button_by_text(hud, "P3_hud")
	_check(hud_btn != null and hud_btn.is_visible_in_tree(), "HUD: 'P3_hud' visible en juego real")
	root.get_texture().get_image().save_png(cap_dir.path_join("p3_juego_hud.png"))
	if hud_btn != null:
		await _click_at(root, hud_btn.get_global_rect().get_center())
		_check(dm.get_switch("sw_p3_hud") == true, "HUD: clic real ejecuta acción del botón")
	hud.queue_free()
	await _step_frames(2)

	# 6. GameOverScreen
	var go = ClassDB.instantiate("ClockGearsGameOverScreen")
	root.add_child(go)
	go.show_game_over({})
	await _step_frames(4)
	var go_btn := _find_button_by_text(go.get_custom_ui_layer(), "P3_game_over")
	_check(go_btn != null and go_btn.is_visible_in_tree(), "GameOver: 'P3_game_over' visible en juego real")
	root.get_texture().get_image().save_png(cap_dir.path_join("p3_juego_game_over.png"))
	if go_btn != null:
		await _click_at(root, go_btn.get_global_rect().get_center())
		_check(dm.get_switch("sw_p3_game_over") == true, "GameOver: clic real ejecuta acción del botón")
		dm.set_switch("sw_p3_game_over", false)
		go_btn.grab_focus()
		await _step_frames(1)
		await _press_key(root, KEY_ENTER)
		_check(dm.get_switch("sw_p3_game_over") == true, "GameOver: teclado (Enter con foco) ejecuta acción del botón")
	go.queue_free()
	await _step_frames(2)

	# 7. MessageWindow
	var mw = ClassDB.instantiate("ClockGearsMessageWindow")
	root.add_child(mw)
	mw.show_message("Hola desde P3 aceptación")
	await _step_frames(4)
	var mw_btn := _find_button_by_text(mw.get_custom_ui_layer(), "P3_messages")
	_check(mw_btn != null and mw_btn.is_visible_in_tree(), "Mensajes: 'P3_messages' visible en juego real")
	root.get_texture().get_image().save_png(cap_dir.path_join("p3_juego_mensajes.png"))
	if mw_btn != null:
		await _click_at(root, mw_btn.get_global_rect().get_center())
		_check(dm.get_switch("sw_p3_messages") == true, "Mensajes: clic real ejecuta acción del botón")
		dm.set_switch("sw_p3_messages", false)
		mw_btn.grab_focus()
		await _step_frames(1)
		await _press_key(root, KEY_ENTER)
		_check(dm.get_switch("sw_p3_messages") == true, "Mensajes: teclado (Enter con foco) ejecuta acción del botón")
	mw.close_window()
	mw.queue_free()
	await _step_frames(2)

	# 8. BattleScene
	var bt = ClassDB.instantiate("ClockGearsBattleScene")
	root.add_child(bt)
	bt.setup_battle(1, true, false, "", null, true)
	await _step_frames(4)
	var bt_btn := _find_button_by_text(bt.get_custom_ui_layer(), "P3_battle")
	_check(bt_btn != null and bt_btn.is_visible_in_tree(), "Batalla: 'P3_battle' visible en juego real")
	root.get_texture().get_image().save_png(cap_dir.path_join("p3_juego_batalla.png"))
	if bt_btn != null:
		await _click_at(root, bt_btn.get_global_rect().get_center())
		_check(dm.get_switch("sw_p3_battle") == true, "Batalla: clic real ejecuta acción del botón")
		dm.set_switch("sw_p3_battle", false)
		bt_btn.grab_focus()
		await _step_frames(1)
		await _press_key(root, KEY_ENTER)
		_check(dm.get_switch("sw_p3_battle") == true, "Batalla: teclado (Enter con foco) ejecuta acción del botón")
	bt.finish_battle_immediate()
	bt.queue_free()
	await _step_frames(2)

	# Restaurar archivos originales de datos en test_project
	if orig_ui_json.is_empty():
		if FileAccess.file_exists(ui_file):
			DirAccess.remove_absolute(ui_file)
	else:
		var fw := FileAccess.open(ui_file, FileAccess.WRITE)
		fw.store_string(orig_ui_json)
		fw.close()
	if orig_hud_json.is_empty():
		if FileAccess.file_exists(hud_file):
			DirAccess.remove_absolute(hud_file)
	else:
		var fh := FileAccess.open(hud_file, FileAccess.WRITE)
		fh.store_string(orig_hud_json)
		fh.close()
	dm.load_all()

	_finish(0 if fail_count == 0 else 1)
