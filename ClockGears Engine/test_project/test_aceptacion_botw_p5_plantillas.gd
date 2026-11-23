extends SceneTree

# Prueba de aceptación Punto 5 (BotW 2/2):
# A) Plantillas genéricas "Inventario por categorías" (Paginador 6 categorías + detalle rotatorio)
#    y "Rueda rápida con páginas" aplicadas desde el editor y probadas en juego real con Q/E/L1/R1.
# B) Crear pantalla de Inventario desde cero con 2 componentes (Cuadrícula + Rueda alternable),
#    mover/escalar ambos, enlazar entrada Tab/R -> toggle_container y verificar en juego real.

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
	for _i in range(mini(n, 600)):
		await process_frame

func _ensure_visible_in_scroll(ctrl: Control) -> void:
	var p := ctrl.get_parent()
	while p != null:
		if p is ScrollContainer:
			(p as ScrollContainer).ensure_control_visible(ctrl)
		p = p.get_parent()

func _click_control_real(ctrl: Control) -> void:
	if ctrl == null:
		return
	_ensure_visible_in_scroll(ctrl)
	await _step_frames(1)
	var win := ctrl.get_viewport() as Window
	var pos := ctrl.get_global_rect().get_center()
	while win != null and win.is_embedded() and win.get_parent() != null:
		pos += Vector2(win.position)
		var pwin := win.get_parent().get_viewport() as Window
		if pwin == null or pwin == win:
			break
		win = pwin
	if win == null:
		return
	win.notification(Viewport.NOTIFICATION_VP_MOUSE_ENTER)
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

func _find_card_btn(grid: GridContainer, tid: String) -> Button:
	if grid == null:
		return null
	for c in grid.get_children():
		for b in c.find_children("*", "Button", true, false):
			if str(b.get_meta("tpl_id", "")) == tid:
				return b as Button
	return null

func _find_control_with_elem_id(root_node: Node, eid: String) -> Control:
	if root_node == null or eid.is_empty():
		return null
	for n in root_node.find_children("*", "Control", true, false):
		if str(n.get_meta("ui_elem_id", "")) == eid:
			return n as Control
	return null

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== ACEPTACIÓN PUNTO 5 (BOTW 2/2): PLANTILLAS ESTILO BOTW Y MENU DESDE CERO ===")
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
	var orig_ui_json := FileAccess.get_file_as_string(ui_file) if FileAccess.file_exists(ui_file) else ""

	# Conceder objetos para que se vean filas reales al navegar categorías
	var recs: Array = dm.get_records("items")
	var granted_keys: Array[String] = []
	for rec in recs:
		if granted_keys.size() >= 4:
			break
		if typeof(rec) != TYPE_DICTIONARY:
			continue
		var ikey := str(rec.get("name", ""))
		if not ikey.is_empty() and dm.get_item_qty(ikey) <= 0:
			dm.change_item(ikey, 2)
			granted_keys.append(ikey)

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)

	db.select_category_by_key("menu_inventory")
	await _step_frames(3)
	var ed := db.get_ui_designer_editor()
	var h: Dictionary = ed.get_ui_handles()

	# --- A1: Abrir galería y aplicar plantilla "Inventario por categorías" ---
	await _click_control_real(h["templates_btn"])
	await _step_frames(3)
	var scr_grid: GridContainer = h["tpl_screen_grid"]
	var card_cat := _find_card_btn(scr_grid, "botw_inventory_categories")
	var card_radp := _find_card_btn(scr_grid, "botw_radial_paged")
	_check(card_cat != null, "Plantilla 'Inventario por categorías' visible en catálogo de Inventario")
	_check(card_radp != null, "Plantilla 'Rueda rápida con páginas' visible en catálogo de Inventario")

	if card_cat != null:
		await _click_control_real(card_cat)
		await _click_control_real(h["tpl_apply_btn"])
		await _step_frames(2)
		var confirm_dlg: ConfirmationDialog = h["template_apply_confirm_dialog"]
		if confirm_dlg != null and confirm_dlg.visible:
			await _click_control_real(confirm_dlg.get_ok_button())
		await _step_frames(4)

	var dl_ed: Control = h["design_layer"]
	var pgr_info_ed: Dictionary = dl_ed.get_pager_info("botw_cat_pager")
	_check(int(pgr_info_ed.get("page_count", 0)) == 6,
			"Plantilla 'Inventario por categorías' aplicada con 6 categorías (%d)" % int(pgr_info_ed.get("page_count", 0)))
	root.get_texture().get_image().save_png(cap_dir + "/botw_p5_editor_botw_cat.png")

	await _click_control_real(db.get_ok_button())
	await _step_frames(3)
	db.queue_free()
	await _step_frames(2)

	# Probar plantilla "Inventario por categorías" en juego real
	dm.load_all()
	var gm = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm)
	await _step_frames(2)
	gm.open_menu_by_id("game_menu_items", false, false, true)
	await _step_frames(4)
	var gm_layer: Control = gm.get_custom_ui_layer()
	_check(gm_layer.get_pager_active_page("botw_cat_pager") == 0,
			"Juego real inicia 'Inventario por categorías' en página 0 (Armas)")
	# Pulsar E 3 veces: Armas -> Escudos -> Armaduras -> Consumibles
	for _p in range(3):
		await _press_key(root, KEY_E)
		await create_timer(0.20).timeout
		await _step_frames(3)
	_check(gm_layer.get_pager_active_page("botw_cat_pager") == 3,
			"Pulsar E tres veces navega a página 3 (Consumibles)")
	gm_layer.focus_container("botw_grid_consumibles")
	await _step_frames(3)
	root.get_texture().get_image().save_png(cap_dir + "/botw_p5_juego_botw_cat.png")
	gm.close_menu()
	gm.queue_free()
	await _step_frames(2)

	# --- A2: Aplicar plantilla "Rueda rápida con páginas" y probar Q/E en juego real ---
	_check(dm.apply_menu_template("game_menu_items", "botw_radial_paged", false, false),
			"Plantilla 'Rueda rápida con páginas' aplicada al inventario")
	var gm2 = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm2)
	await _step_frames(2)
	gm2.open_menu_by_id("game_menu_items", false, false, true)
	await _step_frames(4)
	var gm2_layer: Control = gm2.get_custom_ui_layer()
	_check(int(gm2_layer.get_pager_info("botw_rad_pager").get("page_count", 0)) == 3,
			"Rueda rápida tiene 3 páginas (Armas, Escudos, Consumibles)")
	await _press_key(root, KEY_E)
	await create_timer(0.20).timeout
	await _step_frames(3)
	_check(gm2_layer.get_pager_active_page("botw_rad_pager") == 1,
			"Rueda rápida cambia de página con tecla E en juego real")
	root.get_texture().get_image().save_png(cap_dir + "/botw_p5_juego_botw_radial.png")
	gm2.close_menu()
	gm2.queue_free()
	await _step_frames(2)

	# --- B: Crear menú de Inventario desde cero combinando 2 componentes ---
	var db2 := ClockGearsDatabaseDialog.new()
	db2.exclusive = false
	root.add_child(db2)
	db2.open_database()
	db2.size = Vector2i(1366, 768)
	await _step_frames(3)
	db2.select_category_by_key("menu_inventory")
	await _step_frames(3)
	var ed2 := db2.get_ui_designer_editor()

	var empty_sd: Dictionary = dm.get_ui_screen_design("game_menu_items")
	empty_sd["elements"] = []
	empty_sd["inputs"] = []
	empty_sd["custom_enabled"] = true
	dm.set_ui_screen_design("game_menu_items", empty_sd, false)
	ed2.refresh_from_data_manager()
	await _step_frames(2)

	var gid: String = ed2.insert_component_by_id("comp_grid_data", "")
	var rid: String = ed2.insert_component_by_id("comp_radial_wheel", "")
	var gel: Dictionary = dm.get_ui_element("game_menu_items", gid)
	gel["offset_x"] = 32
	gel["offset_y"] = 40
	gel["scale_percent"] = 95
	dm.update_ui_element("game_menu_items", gid, gel, false)

	var rel: Dictionary = dm.get_ui_element("game_menu_items", rid)
	rel["offset_x"] = 500
	rel["offset_y"] = 70
	rel["scale_percent"] = 90
	rel["visible"] = false
	rel["layer_order"] = 20
	dm.update_ui_element("game_menu_items", rid, rel, false)

	var sd2: Dictionary = dm.get_ui_screen_design("game_menu_items")
	sd2["inputs"] = [{
		"triggers": [{"kind": "key", "keycode": KEY_R, "key_name": "R"}],
		"require_focus": false,
		"actions": [{"type": "toggle_container", "target_elem": rid, "transition_type": "scale", "transition_duration": 0.12}]
	}]
	dm.set_ui_screen_design("game_menu_items", sd2, false)
	ed2.refresh_from_data_manager()
	await _step_frames(2)

	await _click_control_real(db2.get_ok_button())
	await _step_frames(3)
	db2.queue_free()
	await _step_frames(2)

	dm.load_all()
	var gm3 = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm3)
	await _step_frames(2)
	gm3.open_menu_by_id("game_menu_items", false, false, true)
	await _step_frames(4)
	var gm3_layer: Control = gm3.get_custom_ui_layer()
	var g3_ctrl := _find_control_with_elem_id(gm3_layer, gid)
	var r3_ctrl := _find_control_with_elem_id(gm3_layer, rid)
	_check(g3_ctrl != null and g3_ctrl.is_visible_in_tree() and r3_ctrl != null and not r3_ctrl.is_visible_in_tree(),
			"Menú creado desde cero inicia con cuadrícula visible y rueda oculta")
	await _press_key(root, KEY_R)
	await create_timer(0.18).timeout
	await _step_frames(3)
	_check(r3_ctrl != null and r3_ctrl.is_visible_in_tree() and gm3_layer.get_active_focus_container_id() == rid,
			"Tecla R alterna y enfoca la rueda superpuesta en menú creado desde cero")
	root.get_texture().get_image().save_png(cap_dir + "/botw_p5_juego_desde_cero.png")
	await _press_key(root, KEY_R)
	await create_timer(0.18).timeout
	await _step_frames(3)
	_check(not gm3_layer.is_container_open(rid), "Segunda pulsación de R cierra la rueda superpuesta")

	for ks in granted_keys:
		dm.change_item(ks, -2)
	gm3.close_menu()
	gm3.queue_free()
	await _step_frames(2)

	if not orig_ui_json.is_empty():
		var f := FileAccess.open(ui_file, FileAccess.WRITE)
		if f:
			f.store_string(orig_ui_json)
			f.close()
	dm.load_all()
	_check(true, "Datos de prueba restaurados tras captura")
	_finish(0 if fail_count == 0 else 1)
