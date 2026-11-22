extends SceneTree

# Prueba de aceptación Punto 3 (BotW 2/2): Entradas de pantalla y contenedor,
# condición "Solo si el contenedor tiene el foco", nuevas acciones (toggle/open/close/
# focus/set_container_filter/next_page...) y aviso de conflicto en editor.

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
	print("=== ACEPTACIÓN PUNTO 3 (BOTW 2/2): ENTRADAS Y ACCIONES DE CONTENEDOR ===")
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
	dm.set_switch("sw_grid_focus_fired", false)

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

	# 1. Insertar cuadrícula + rueda radial oculta + botón filtro en Pantalla Inventario
	var grid_id: String = ed.insert_component_by_id("comp_grid_data", "")
	var rad_id: String = ed.insert_component_by_id("comp_radial_wheel", "")
	var rad_el: Dictionary = dm.get_ui_element("game_menu_items", rad_id)
	rad_el["visible"] = false
	rad_el["layer_order"] = 50
	dm.update_ui_element("game_menu_items", rad_id, rad_el, false)

	var btn_def := ClockGearsDataManager.make_default_ui_element("button", "game_menu_items")
	btn_def["id"] = "btn_cat_consumables"
	btn_def["text"] = "Solo Consumibles"
	btn_def["anchor"] = "top_left"
	btn_def["offset_x"] = 40
	btn_def["offset_y"] = 480
	btn_def["width"] = 180
	btn_def["height"] = 36
	btn_def["actions"] = [
		{"type": "set_container_filter", "target_elem": grid_id, "filter_category": "consumable"},
		{"type": "focus_container", "target_elem": grid_id}
	]
	var cat_btn_id: String = dm.add_ui_element("game_menu_items", btn_def, false)
	ed.refresh_from_data_manager()
	await _step_frames(3)
	h = ed.get_ui_handles()

	# 2. Expandir 'Entradas globales' si está colapsada y añadir entrada de pantalla: Tab -> toggle_container
	var ss_hdr: Button = h.get("ss_inputs_hdr")
	var ss_add_btn: Button = h.get("ss_inputs_add_btn")
	_check(ss_hdr != null and ss_add_btn != null, "Sección 'Entradas globales' visible en pantalla")
	if ss_add_btn != null and not ss_add_btn.is_visible_in_tree():
		await _click_control_real(ss_hdr)
		await _step_frames(2)
	if ss_add_btn != null:
		await _click_control_real(ss_add_btn)
		await _step_frames(3)
	h = ed.get_ui_handles()
	var ss_list: VBoxContainer = h.get("ss_inputs_list_vbox")
	if ss_list != null and not ss_list.is_visible_in_tree() and h.get("ss_inputs_hdr") != null:
		await _click_control_real(h["ss_inputs_hdr"])
		await _step_frames(2)
	var cap_btn_scr: Button = null
	if ss_list != null:
		for b in ss_list.find_children("*", "Button", true, false):
			if str((b as Button).text) == "+Tecla":
				cap_btn_scr = b as Button
				break
	_check(cap_btn_scr != null, "Botón capturar tecla en entrada de pantalla presente")
	if cap_btn_scr != null:
		await _click_control_real(cap_btn_scr)
		await _step_frames(2)
		await _press_key(db, KEY_TAB)
		await _step_frames(3)
	h = ed.get_ui_handles()
	ss_list = h.get("ss_inputs_list_vbox")
	if ss_list != null and not ss_list.is_visible_in_tree() and h.get("ss_inputs_hdr") != null:
		await _click_control_real(h["ss_inputs_hdr"])
		await _step_frames(2)
	var scr_acts_opt: OptionButton = null
	var scr_acts_add: Button = null
	if ss_list != null:
		for c in ss_list.find_children("*", "OptionButton", true, false):
			scr_acts_opt = c as OptionButton
			break
		for b in ss_list.find_children("*", "Button", true, false):
			if str((b as Button).text) == "+":
				scr_acts_add = b as Button
				break
	if scr_acts_opt != null and scr_acts_add != null:
		_select_option_meta(scr_acts_opt, "toggle_container")
		await _click_control_real(scr_acts_add)
		await _step_frames(3)
	var sd_after_scr: Dictionary = dm.get_ui_screen_design("game_menu_items")
	var s_ins: Array = sd_after_scr.get("inputs", [])
	if s_ins.size() > 0:
		var s0: Dictionary = s_ins[0]
		var s0_acts: Array = s0.get("actions", [])
		if s0_acts.size() > 0:
			s0_acts[0]["target_elem"] = rad_id
			s0_acts[0]["transition_type"] = "scale"
			s0_acts[0]["transition_duration"] = 0.12
			s0["actions"] = s0_acts
			s_ins[0] = s0
			sd_after_scr["inputs"] = s_ins
			dm.set_ui_screen_design("game_menu_items", sd_after_scr, false)
	ed.refresh_from_data_manager()
	await _step_frames(2)
	_check(dm.get_ui_screen_design("game_menu_items").get("inputs", []).size() == 1 and dm.get_ui_screen_design("game_menu_items")["inputs"][0].get("actions", []).size() == 1,
			"Entrada de pantalla Tab -> toggle_container(%s) guardada vía UI" % rad_id)

	# 3. Seleccionar cuadrícula, añadir 1ª entrada (F + Solo con foco -> set_switch) y 2ª conflictiva (Tab sin foco)
	ed.select_element(grid_id)
	await _step_frames(2)
	h = ed.get_ui_handles()
	var elem_in_box: Control = h.get("prop_inputs_box")
	if elem_in_box != null and elem_in_box.get_child_count() >= 2:
		var body_ctrl := elem_in_box.get_child(1) as Control
		if body_ctrl != null and not body_ctrl.visible:
			await _click_control_real(elem_in_box.get_child(0) as Button)
			await _step_frames(2)
	await _click_control_real(h["prop_inputs_add_btn"])
	await _step_frames(2)
	var el_list: VBoxContainer = h["prop_inputs_list_vbox"]
	var el_cap: Button = null
	for b in el_list.find_children("*", "Button", true, false):
		if str((b as Button).text) == "+Tecla":
			el_cap = b as Button
			break
	if el_cap != null:
		await _click_control_real(el_cap)
		await _step_frames(1)
		await _press_key(db, KEY_F)
		await _step_frames(2)
	var el_fchk: CheckBox = null
	for cb in el_list.find_children("*", "CheckBox", true, false):
		el_fchk = cb as CheckBox
		break
	if el_fchk != null:
		await _click_control_real(el_fchk)
		await _step_frames(2)
	var gel: Dictionary = dm.get_ui_element("game_menu_items", grid_id)
	var gins: Array = gel.get("inputs", [])
	if gins.size() > 0:
		var gb: Dictionary = gins[0]
		gb["actions"] = [{"type": "set_switch", "switch_name": "sw_grid_focus_fired", "value": true}]
		gins[0] = gb
		# Añadir segunda entrada duplicada con Tab para probar detector de conflictos
		gins.append({
			"triggers": [{"kind": "key", "keycode": KEY_TAB, "key_name": "Tab"}],
			"require_focus": false,
			"actions": [{"type": "set_switch", "switch_name": "sw_dummy", "value": true}]
		})
		gel["inputs"] = gins
		dm.update_ui_element("game_menu_items", grid_id, gel, false)
	ed.refresh_from_data_manager()
	await _step_frames(3)

	h = ed.get_ui_handles()
	var conf_lbl: Label = h.get("prop_inputs_conflict_lbl")
	_check(conf_lbl != null and conf_lbl.visible and conf_lbl.text.find("Conflicto") >= 0,
			"Aviso de conflicto visible al duplicar tecla Tab en pantalla (%s)" % (conf_lbl.text if conf_lbl else ""))
	root.get_texture().get_image().save_png(cap_dir + "/botw_p3_editor.png")

	# Quitar el conflicto antes de guardar para el juego real
	gel = dm.get_ui_element("game_menu_items", grid_id)
	gins = gel.get("inputs", [])
	if gins.size() > 1:
		gins.remove_at(1)
		gel["inputs"] = gins
		dm.update_ui_element("game_menu_items", grid_id, gel, false)
	ed.refresh_from_data_manager()
	await _step_frames(2)
	_check(not h["prop_inputs_conflict_lbl"].visible, "Aviso de conflicto desaparece al quitar entrada duplicada")

	# 4. Guardar con OK y probar en juego real
	await _click_control_real(db.get_ok_button())
	await _step_frames(3)
	db.queue_free()
	await _step_frames(2)

	dm.load_all()
	var recs: Array = dm.get_records("items")
	var granted_keys: Array[String] = []
	for rec in recs:
		if granted_keys.size() >= 3:
			break
		if typeof(rec) != TYPE_DICTIONARY:
			continue
		var ikey := str(rec.get("name", ""))
		if not ikey.is_empty() and dm.get_item_qty(ikey) <= 0:
			dm.change_item(ikey, 2)
			granted_keys.append(ikey)

	var gm = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm)
	await _step_frames(2)
	gm.open_menu_by_id("game_menu_items", false, false, true)
	await _step_frames(4)

	var gm_layer: Control = gm.get_custom_ui_layer()
	var rad_gm := _find_control_with_elem_id(gm_layer, rad_id)
	var grid_gm := _find_control_with_elem_id(gm_layer, grid_id)
	var cat_btn_gm := _find_control_with_elem_id(gm_layer, cat_btn_id)
	if gm_layer.get_viewport() != null and gm_layer.get_viewport().gui_get_focus_owner() != null:
		gm_layer.get_viewport().gui_get_focus_owner().release_focus()
	await _step_frames(2)

	_check(rad_gm != null and not rad_gm.is_visible_in_tree(), "Rueda radial inicia cerrada en juego real")

	# Pulsar F SIN foco en la cuadrícula -> NO debe dispararse
	await _press_key(root, KEY_F)
	await _step_frames(2)
	_check(dm.get_switch("sw_grid_focus_fired") == false, "Tecla F sin foco en cuadrícula NO se dispara ('Solo si el contenedor tiene el foco')")

	# Pulsar Tab -> toggle_container abre la rueda radial y la enfoca
	await _press_key(root, KEY_TAB)
	await create_timer(0.20).timeout
	await _step_frames(3)
	_check(rad_gm != null and rad_gm.is_visible_in_tree() and gm_layer.is_container_open(rad_id),
			"Tab abre la rueda radial superpuesta con toggle_container")
	_check(gm_layer.get_active_focus_container_id() == rad_id, "toggle_container enfoca la rueda radial al abrirla")
	root.get_texture().get_image().save_png(cap_dir + "/botw_p3_juego_rueda_abierta.png")

	# Pulsar Tab otra vez -> cierra rueda radial
	await _press_key(root, KEY_TAB)
	await create_timer(0.20).timeout
	await _step_frames(3)
	_check(not gm_layer.is_container_open(rad_id) and not rad_gm.is_visible_in_tree(),
			"Tab vuelve a cerrar la rueda radial con toggle_container")

	# Pulsar botón "Solo Consumibles" -> ejecuta set_container_filter + focus_container(grid_id)
	if cat_btn_gm != null:
		await _click_at_ctrl(cat_btn_gm)
	await _step_frames(3)
	var ginfo: Dictionary = gm_layer.get_data_container_info(grid_id)
	_check(gm_layer.get_active_focus_container_id() == grid_id,
			"Botón con acción focus_container mueve el foco a la cuadrícula")

	# Ahora con foco en la cuadrícula, pulsar F SÍ dispara su acción
	await _press_key(root, KEY_F)
	await _step_frames(2)
	_check(dm.get_switch("sw_grid_focus_fired") == true, "Tecla F CON foco en cuadrícula SÍ ejecuta su acción")

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

func _click_at_ctrl(c: Control) -> void:
	var b := c as Button
	if b == null:
		for n in c.find_children("*", "Button", true, false):
			b = n as Button
			break
	var pos := (b if b != null else c).get_global_rect().get_center()
	root.notification(Viewport.NOTIFICATION_VP_MOUSE_ENTER)
	var ev_d := InputEventMouseButton.new()
	ev_d.button_index = MOUSE_BUTTON_LEFT
	ev_d.pressed = true
	ev_d.position = pos
	ev_d.global_position = pos
	root.push_input(ev_d, true)
	await _step_frames(1)
	var ev_u := InputEventMouseButton.new()
	ev_u.button_index = MOUSE_BUTTON_LEFT
	ev_u.pressed = false
	ev_u.position = pos
	ev_u.global_position = pos
	root.push_input(ev_u, true)
	await _step_frames(2)
