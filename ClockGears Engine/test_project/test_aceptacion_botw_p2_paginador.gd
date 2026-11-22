extends SceneTree

# Prueba de aceptación Punto 2 (BotW 2/2): Paginador multipágina (Q/E, L1/R1, clic en pestaña,
# + Página/duplicar/reordenar/borrar, iconos, transiciones, "Al entrar" y memoria foco/scroll).

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

func _ensure_visible_in_scroll(ctrl: Control) -> void:
	var p := ctrl.get_parent()
	while p != null:
		if p is ScrollContainer:
			(p as ScrollContainer).ensure_control_visible(ctrl)
		p = p.get_parent()

func _to_window_pos(ctrl: Control, local_pt: Vector2) -> Array:
	var win := ctrl.get_viewport() as Window
	var pos := ctrl.get_global_rect().position + local_pt
	while win != null and win.is_embedded() and win.get_parent() != null:
		pos += Vector2(win.position)
		var pwin := win.get_parent().get_viewport() as Window
		while pwin != null and pwin != root and not pwin.gui_embed_subwindows:
			pwin = (pwin.get_parent().get_viewport() as Window) if pwin.get_parent() != null else null
		if pwin == null or pwin == win:
			break
		win = pwin
	return [win, pos]

func _click_control_real(ctrl: Control) -> void:
	if ctrl == null:
		return
	_ensure_visible_in_scroll(ctrl)
	await _step_frames(1)
	var wp := _to_window_pos(ctrl, ctrl.size * 0.5)
	if wp[0] != null:
		await _click_at(wp[0], wp[1])

func _find_comp_card_button(grid: GridContainer, tid: String) -> Button:
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

func _select_option_meta(opt: OptionButton, meta_val: String) -> void:
	if opt == null:
		return
	for i in range(opt.item_count):
		if str(opt.get_item_metadata(i)) == meta_val:
			opt.select(i)
			opt.emit_signal("item_selected", i)
			return

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== ACEPTACIÓN PUNTO 2 (BOTW 2/2): PAGINADOR MULTIPÁGINA ===")
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
	dm.set_switch("sw_entered_escudos", false)

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

	# 1. Insertar "Paginador (Q/E)" vía botón "+ Componente"
	await _click_control_real(h["layer_add_component_btn"])
	await _step_frames(3)
	var comp_grid: GridContainer = h["tpl_comp_grid"]
	var pager_card := _find_comp_card_button(comp_grid, "comp_pager")
	_check(pager_card != null, "Tarjeta 'Paginador (Q/E)' presente en Componentes")
	if pager_card != null:
		await _click_control_real(pager_card)
	await _click_control_real(h["tpl_apply_btn"])
	await _step_frames(3)

	var pager_id: String = ed.get_selected_element_id()
	var pager_el: Dictionary = dm.get_ui_element("game_menu_items", pager_id)
	_check(not pager_id.is_empty() and str(pager_el.get("type", "")) == "pager", "Paginador '%s' insertado correctamente" % pager_id)
	var page_opt: OptionButton = h["prop_pager_page_opt"]
	_check(page_opt != null and page_opt.item_count == 2, "Paginador inicia con 2 páginas (%d)" % (page_opt.item_count if page_opt else -1))
	_check(str(h["prop_pager_prev_lbl"].text).find("Q") >= 0 and str(h["prop_pager_next_lbl"].text).find("E") >= 0,
			"Entradas por defecto Q/L1 y E/R1 (%s | %s)" % [h["prop_pager_prev_lbl"].text, h["prop_pager_next_lbl"].text])

	# 2. + Página, Duplicar, Reordenar, Borrar y Renombrar páginas
	await _click_control_real(h["prop_pager_add_page_btn"])
	await _step_frames(2)
	_check(page_opt.item_count == 3, "+ Página crea la tercera página (%d)" % page_opt.item_count)

	await _click_control_real(h["prop_pager_dup_page_btn"])
	await _step_frames(2)
	_check(page_opt.item_count == 4, "Duplicar página duplica a 4 páginas (%d)" % page_opt.item_count)

	await _click_control_real(h["prop_pager_up_page_btn"])
	await _step_frames(2)
	var moved_pid: String = str(page_opt.get_item_metadata(2))
	_check(page_opt.selected == 2 and not moved_pid.is_empty(), "Subir página mueve la página activa al índice 2")

	await _click_control_real(h["prop_pager_del_page_btn"])
	await _step_frames(2)
	_check(page_opt.item_count == 3, "Borrar página reduce a 3 páginas (%d)" % page_opt.item_count)

	# Renombrar páginas: 0="Armas", 1="Escudos", 2="Consumibles"
	var pnames := ["Armas", "Escudos", "Consumibles"]
	var pids: Array[String] = []
	for pi in range(3):
		ed.set_editor_pager_page(pager_id, pi)
		await _step_frames(2)
		var name_ed: LineEdit = h["prop_pager_page_name_edit"]
		name_ed.text = pnames[pi]
		name_ed.emit_signal("text_changed", pnames[pi])
		await _step_frames(1)
		pids.append(str(page_opt.get_item_metadata(pi)))

	# Añadir acción "Al entrar en la página" a Escudos (set_switch sw_entered_escudos = true)
	ed.set_editor_pager_page(pager_id, 1)
	await _step_frames(2)
	_select_option_meta(h["prop_pager_enter_add_act_opt"], "set_switch")
	await _click_control_real(h["prop_pager_enter_add_act_btn"])
	await _step_frames(2)
	var p1_el: Dictionary = dm.get_ui_element("game_menu_items", pids[1])
	var p1_acts: Array = p1_el.get("enter_actions", [])
	if p1_acts.size() > 0:
		var st: Dictionary = p1_acts[0]
		st["switch_name"] = "sw_entered_escudos"
		st["value"] = true
		p1_acts[0] = st
		p1_el["enter_actions"] = p1_acts
		dm.update_ui_element("game_menu_items", pids[1], p1_el, false)
	ed.refresh_from_data_manager()
	await _step_frames(2)
	_check(dm.get_ui_element("game_menu_items", pids[1]).get("enter_actions", []).size() == 1,
			"Acción 'Al entrar en la página' guardada en Escudos")

	# Insertar cuadrícula dentro de Armas (pág 0) y rueda radial dentro de Escudos (pág 1)
	var grid_elem_id: String = ed.insert_component_by_id("comp_grid_data", pids[0])
	var rad_elem_id: String = ed.insert_component_by_id("comp_radial_wheel", pids[1])
	await _step_frames(3)
	_check(str(dm.get_ui_element("game_menu_items", grid_elem_id).get("parent_id", "")) == pids[0],
			"Cuadrícula '%s' anidada dentro de página Armas '%s'" % [grid_elem_id, pids[0]])
	_check(str(dm.get_ui_element("game_menu_items", rad_elem_id).get("parent_id", "")) == pids[1],
			"Rueda radial '%s' anidada dentro de página Escudos '%s'" % [rad_elem_id, pids[1]])

	# Probar modos de indicador (dots, arrows, tabs) y transición fade/slide
	ed.select_element(pager_id)
	await _step_frames(2)
	_select_option_meta(h["prop_pager_ind_mode_opt"], "dots")
	await _step_frames(2)
	var dl_ed: Control = h["design_layer"]
	var ind_dot0 := dl_ed.find_child("PagerInd_0", true, false) as Button
	var ind_dot1 := dl_ed.find_child("PagerInd_1", true, false) as Button
	_check(ind_dot0 != null and ind_dot1 != null and ind_dot1.text == "●" and ind_dot0.text == "○",
			"Indicador modo 'Puntos' muestra círculos (activo=●, inactivo=○)")
	_select_option_meta(h["prop_pager_ind_mode_opt"], "tabs")
	await _step_frames(2)
	_select_option_meta(h["prop_pager_trans_type_opt"], "slide")
	h["prop_pager_trans_dur_spin"].value = 0.12
	ed.set_editor_pager_page(pager_id, 0)
	await _step_frames(2)

	# Clic en pestaña 1 ("Escudos") directamente sobre el lienzo
	var tab1_ed := dl_ed.find_child("PagerInd_1", true, false) as Button
	_check(tab1_ed != null and tab1_ed.text == "Escudos", "Pestaña indicador 1 muestra 'Escudos'")
	if tab1_ed != null:
		await _click_control_real(tab1_ed)
		await _step_frames(3)
	_check(dl_ed.get_pager_active_page(pager_id) == 1, "Clic en pestaña 'Escudos' del lienzo cambia la vista a Escudos")
	var rad_ctrl_ed := _find_control_with_elem_id(dl_ed, rad_elem_id)
	var grid_ctrl_ed := _find_control_with_elem_id(dl_ed, grid_elem_id)
	_check(rad_ctrl_ed != null and rad_ctrl_ed.is_visible_in_tree() and (grid_ctrl_ed == null or not grid_ctrl_ed.is_visible_in_tree()),
			"Solo los hijos de la página activa ('Escudos') son visibles en el editor")

	root.get_texture().get_image().save_png(cap_dir + "/botw_p2_editor.png")

	# 3. Guardar con OK y probar en juego real ClockGearsGameMenu
	await _click_control_real(db.get_ok_button())
	await _step_frames(3)
	db.queue_free()
	await _step_frames(2)

	dm.load_all()
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
	var gm = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm)
	await _step_frames(2)
	gm.open_menu_by_id("game_menu_items", false, false, true)
	await _step_frames(4)

	var gm_layer: Control = gm.get_custom_ui_layer()
	_check(gm_layer.get_pager_active_page(pager_id) == 0, "Juego real inicia el Paginador en la página 0 ('Armas')")
	var grid_gm := _find_control_with_elem_id(gm_layer, grid_elem_id)
	var rad_gm := _find_control_with_elem_id(gm_layer, rad_elem_id)
	_check(grid_gm != null and grid_gm.is_visible_in_tree() and (rad_gm == null or not rad_gm.is_visible_in_tree()),
			"Juego real pág 0: cuadrícula visible y rueda pág 1 oculta")

	# Enfocar una casilla dentro de Armas (memoria foco) y pulsar E para pasar a Escudos
	var cell0 := grid_gm.find_child("DataCell_0", true, false) as Button if grid_gm != null else null
	if cell0 != null and cell0.focus_mode != Control.FOCUS_NONE:
		cell0.grab_focus()
		await _step_frames(2)
	var had_focus := (cell0 != null and cell0.has_focus())

	await _press_key(root, KEY_E)
	await create_timer(0.22).timeout
	await _step_frames(4)
	_check(gm_layer.get_pager_active_page(pager_id) == 1, "Tecla E avanza el Paginador a página 1 ('Escudos')")
	_check(dm.get_switch("sw_entered_escudos") == true, "Al entrar en 'Escudos' ejecutó su acción (sw_entered_escudos=ON)")
	_check(rad_gm != null and rad_gm.is_visible_in_tree() and (grid_gm == null or not grid_gm.is_visible_in_tree()),
			"Rueda radial dentro de 'Escudos' visible y cuadrícula de 'Armas' oculta en juego real")
	root.get_texture().get_image().save_png(cap_dir + "/botw_p2_juego_pag2.png")

	# Probar gamepad L1 -> vuelve a 0 (Armas) y restaura foco en DataCell_0 de Armas!
	await _press_joy_btn(root, JOY_BUTTON_LEFT_SHOULDER)
	await create_timer(0.22).timeout
	await _step_frames(4)
	_check(gm_layer.get_pager_active_page(pager_id) == 0, "Botón mando L1 retrocede a página 0 ('Armas')")
	_check(had_focus and cell0 != null and cell0.has_focus(), "Paginador recuerda foco por página al volver a 'Armas'")

	# Probar wrap_around con Q desde pág 0 -> pág 2 ('Consumibles')
	await _press_key(root, KEY_Q)
	await create_timer(0.22).timeout
	await _step_frames(4)
	_check(gm_layer.get_pager_active_page(pager_id) == 2, "Tecla Q en página 0 da la vuelta a página 2 ('Consumibles')")

	# Probar clic directo en pestaña 'Escudos' en el juego real
	var tab_gm1 := gm_layer.find_child("PagerInd_1", true, false) as Button
	if tab_gm1 != null:
		await _click_at(root, tab_gm1.get_global_rect().get_center())
		await create_timer(0.22).timeout
		await _step_frames(4)
	_check(gm_layer.get_pager_active_page(pager_id) == 1, "Clic en indicador pestaña del juego real cambia a 'Escudos'")

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
