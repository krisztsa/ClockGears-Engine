extends SceneTree

# Prueba de aceptación Punto 4 (BotW 2/2): Navegación y foco entre y dentro de contenedores
# (espacial con flechas/D-Pad, enlaces manuales izq/der/arr/ab, marco de foco y atenuación sin foco).

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

func _press_joy_btn(win: Window, jb: JoyButton) -> void:
	var ev_d := InputEventJoypadButton.new()
	ev_d.button_index = jb
	ev_d.pressed = true
	win.push_input(ev_d, true)
	await _step_frames(1)
	var ev_u := InputEventJoypadButton.new()
	ev_u.button_index = jb
	ev_u.pressed = false
	win.push_input(ev_u, true)
	await _step_frames(2)

func _select_option_meta(opt: OptionButton, meta_val: String) -> void:
	if opt == null:
		return
	for i in range(opt.item_count):
		if str(opt.get_item_metadata(i)) == meta_val:
			opt.select(i)
			opt.emit_signal("item_selected", i)
			return

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
	print("=== ACEPTACIÓN PUNTO 4 (BOTW 2/2): NAVEGACIÓN Y FOCO ENTRE CONTENEDORES ===")
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

	# Limpiar elementos de la pantalla inventario de prueba y crear 2 grids + 1 botón inferior
	var sd: Dictionary = dm.get_ui_screen_design("game_menu_items")
	sd["elements"] = []
	sd["custom_enabled"] = true
	dm.set_ui_screen_design("game_menu_items", sd, false)
	ed.refresh_from_data_manager()
	await _step_frames(2)

	var grid_l: String = ed.insert_component_by_id("comp_grid_data", "")
	var gl_el: Dictionary = dm.get_ui_element("game_menu_items", grid_l)
	gl_el["name"] = "Bolsa Izquierda"
	gl_el["anchor"] = "top_left"
	gl_el["offset_x"] = 30
	gl_el["offset_y"] = 70
	gl_el["width"] = 360
	gl_el["height"] = 280
	gl_el["layout_columns"] = 2
	gl_el["data_empty_count"] = 4
	gl_el["dim_unfocused"] = true
	gl_el["unfocused_opacity"] = 50
	dm.update_ui_element("game_menu_items", grid_l, gl_el, false)

	var grid_r: String = ed.insert_component_by_id("comp_grid_data", "")
	var gr_el: Dictionary = dm.get_ui_element("game_menu_items", grid_r)
	gr_el["name"] = "Equipo Derecha"
	gr_el["anchor"] = "top_left"
	gr_el["offset_x"] = 450
	gr_el["offset_y"] = 70
	gr_el["width"] = 360
	gr_el["height"] = 280
	gr_el["layout_columns"] = 2
	gr_el["data_empty_count"] = 4
	gr_el["focus_cursor_mode"] = "ring"
	dm.update_ui_element("game_menu_items", grid_r, gr_el, false)

	var btn_def := ClockGearsDataManager.make_default_ui_element("button", "game_menu_items")
	btn_def["id"] = "btn_salir_foco"
	btn_def["text"] = "Cerrar menú"
	btn_def["action"] = "close"
	btn_def["anchor"] = "top_left"
	btn_def["offset_x"] = 450
	btn_def["offset_y"] = 390
	btn_def["width"] = 200
	btn_def["height"] = 40
	var btn_b: String = dm.add_ui_element("game_menu_items", btn_def, false)

	ed.refresh_from_data_manager()
	await _step_frames(2)

	# Configurar en Propiedades de grid_r: atenuar sin foco y enlace manual Al ir abajo -> btn_b
	ed.select_element(grid_r)
	await _step_frames(3)
	h = ed.get_ui_handles()
	var fn_box: Control = h.get("prop_focus_nav_box")
	_check(fn_box != null and h.get("prop_focus_down_opt") != null and h.get("prop_dim_unfocused_chk") != null,
			"Sección 'Foco y navegación' presente en Propiedades del contenedor")
	if fn_box != null and not fn_box.is_visible_in_tree() and fn_box.get_parent() != null:
		for ch in fn_box.get_parent().get_children():
			if ch is Button:
				await _click_control_real(ch as Button)
				break
	_select_option_meta(h["prop_focus_down_opt"], btn_b)
	await _click_control_real(h["prop_dim_unfocused_chk"])
	h["prop_unfocused_opacity_spin"].value = 50
	await _step_frames(3)

	var saved_gr: Dictionary = dm.get_ui_element("game_menu_items", grid_r)
	_check(str(saved_gr.get("focus_next_down", "")) == btn_b and bool(saved_gr.get("dim_unfocused", false)),
			"Enlace manual abajo y atenuación sin foco guardados desde el editor")
	root.get_texture().get_image().save_png(cap_dir + "/botw_p4_editor.png")

	await _click_control_real(db.get_ok_button())
	await _step_frames(3)
	db.queue_free()
	await _step_frames(2)

	# Asegurar >= 2 objetos distintos en el inventario runtime para navegar dentro de columnas
	dm.load_all()
	var recs: Array = dm.get_records("items")
	var granted_keys: Array[String] = []
	for rec in recs:
		if granted_keys.size() >= 3:
			break
		if typeof(rec) != TYPE_DICTIONARY:
			continue
		var ikey := str(rec.get("name", ""))
		if not ikey.is_empty():
			if dm.get_item_qty(ikey) <= 0:
				dm.change_item(ikey, 2)
				granted_keys.append(ikey)

	var gm = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm)
	await _step_frames(2)
	gm.open_menu_by_id("game_menu_items", false, false, true)
	await _step_frames(4)

	var gm_layer: Control = gm.get_custom_ui_layer()
	_check(gm_layer.focus_container(grid_l), "Contenedor izquierdo enfocado inicialmente")
	await _step_frames(3)

	var ov_l: Dictionary = gm_layer.get_container_focus_overlay_state()
	var alphas_l: Dictionary = ov_l.get("container_alphas", {})
	_check(bool(ov_l.get("container_focus_enabled", false)) and gm_layer.get_active_focus_container_id() == grid_l,
			"Marco de foco visible alrededor del contenedor izquierdo")
	_check(float(alphas_l.get(grid_l, 1.0)) > 0.95 and float(alphas_l.get(grid_r, 1.0)) < 0.65,
			"Contenedor sin foco atenuado al 50%% (izq=%.2f, der=%.2f)" % [float(alphas_l.get(grid_l, 1.0)), float(alphas_l.get(grid_r, 1.0))])

	# Si grid_l tiene >=2 ranuras, Derecha mueve primero a columna 1 dentro de grid_l; si solo hay 1, salta ya o hacemos hasta 2 pasos
	var gl_ctrl := _find_control_with_elem_id(gm_layer, grid_l)
	var cells_l := gl_ctrl.find_children("DataCell_*", "Button", true, false)
	var has_two_focusable := (cells_l.size() >= 2 and (cells_l[1] as Button).focus_mode != Control.FOCUS_NONE)
	if has_two_focusable:
		await _press_key(root, KEY_RIGHT)
		await _step_frames(2)
		_check((cells_l[1] as Button).has_focus() and gm_layer.get_active_focus_container_id() == grid_l,
				"Flecha derecha navega dentro de las celdas del contenedor antes del borde")

	# Desde el borde derecho de grid_l -> salta espacialmente a grid_r!
	await _press_key(root, KEY_RIGHT)
	await _step_frames(3)
	_check(gm_layer.get_active_focus_container_id() == grid_r,
			"Al cruzar el borde derecho el foco salta espacialmente al contenedor derecho")
	var ov_r: Dictionary = gm_layer.get_container_focus_overlay_state()
	var alphas_r: Dictionary = ov_r.get("container_alphas", {})
	_check(float(alphas_r.get(grid_r, 1.0)) > 0.95 and float(alphas_r.get(grid_l, 1.0)) < 0.65,
			"Al cambiar de contenedor el foco ilumina derecha y atenúa izquierda (izq=%.2f, der=%.2f)" % [float(alphas_r.get(grid_l, 1.0)), float(alphas_r.get(grid_r, 1.0))])
	root.get_texture().get_image().save_png(cap_dir + "/botw_p4_juego.png")

	# D-Pad Izquierda en el borde izquierdo de grid_r -> vuelve a grid_l!
	await _press_joy_btn(root, JOY_BUTTON_DPAD_LEFT)
	await _step_frames(3)
	_check(gm_layer.get_active_focus_container_id() == grid_l,
			"D-Pad izquierda desde el borde izquierdo devuelve el foco al contenedor izquierdo")

	# Volver a grid_r y bajar por enlace manual hasta btn_b
	await _press_key(root, KEY_RIGHT)
	if has_two_focusable:
		await _press_key(root, KEY_RIGHT)
	await _step_frames(2)
	# Bajar hasta el borde inferior de grid_r (por enlace manual salta a btn_b)
	for _s in range(4):
		if gm_layer.get_active_focus_container_id() == btn_b:
			break
		await _press_key(root, KEY_DOWN)
	await _step_frames(2)
	var btn_ctrl := _find_control_with_elem_id(gm_layer, btn_b)
	_check(gm_layer.get_active_focus_container_id() == btn_b,
			"Enlace manual abajo desde el contenedor derecho enfoca el botón '%s'" % btn_b)

	# Subir desde el botón vuelve espacialmente al contenedor derecho
	await _press_key(root, KEY_UP)
	await _step_frames(2)
	_check(gm_layer.get_active_focus_container_id() == grid_r,
			"Flecha arriba desde el botón inferior devuelve el foco al contenedor derecho")

	for ks in granted_keys:
		dm.change_item(ks, -2)
	gm.close_menu()
	gm.queue_free()
	await _step_frames(2)

	if not orig_ui_json.is_empty():
		var f := FileAccess.open(ui_file, FileAccess.WRITE)
		if f:
			f.store_string(orig_ui_json)
			f.close()
	dm.load_all()
	_check(true, "Datos de prueba restaurados tras captura")
	_finish(0 if fail_count == 0 else 1)
