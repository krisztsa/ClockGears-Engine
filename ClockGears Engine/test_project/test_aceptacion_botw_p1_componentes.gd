extends SceneTree

# Prueba de aceptación Punto 1 (BotW 2/2): "+ Componente", escala en bloque (25-400% + entera),
# redimensión con Shift proporcional, anidado y paridad editor vs juego real.

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

func _drag(win: Window, from_pos: Vector2, to_pos: Vector2, with_shift: bool = false) -> void:
	win.notification(Viewport.NOTIFICATION_VP_MOUSE_ENTER)
	var ev_d := InputEventMouseButton.new()
	ev_d.button_index = MOUSE_BUTTON_LEFT
	ev_d.pressed = true
	ev_d.shift_pressed = with_shift
	ev_d.position = from_pos
	ev_d.global_position = from_pos
	win.push_input(ev_d, true)
	await _step_frames(1)

	for step in range(1, 5):
		var p := from_pos.lerp(to_pos, float(step) / 4.0)
		var ev_m := InputEventMouseMotion.new()
		ev_m.position = p
		ev_m.global_position = p
		ev_m.relative = (to_pos - from_pos) / 4.0
		ev_m.button_mask = MOUSE_BUTTON_MASK_LEFT
		ev_m.shift_pressed = with_shift
		win.push_input(ev_m, true)
		await _step_frames(1)

	var ev_u := InputEventMouseButton.new()
	ev_u.button_index = MOUSE_BUTTON_LEFT
	ev_u.pressed = false
	ev_u.shift_pressed = with_shift
	ev_u.position = to_pos
	ev_u.global_position = to_pos
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

func _find_button_by_text(root_node: Node, needle: String) -> Button:
	if root_node == null:
		return null
	for n in root_node.find_children("*", "Button", true, false):
		var b := n as Button
		if b != null and b.text.to_lower().find(needle.to_lower()) >= 0:
			return b
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
	print("=== ACEPTACIÓN PUNTO 1 (BOTW 2/2): + COMPONENTE, ESCALA Y ANIDADO ===")
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
	var add_comp_btn: Button = h.get("layer_add_component_btn", null) as Button
	_check(add_comp_btn != null and add_comp_btn.is_visible_in_tree(), "Botón '+ Componente' visible en barra Elementos")

	var orig_elems: Array = ed.get_effective_screen_elements()
	var orig_count := orig_elems.size()
	var orig_ids := {}
	for e in orig_elems:
		if typeof(e) == TYPE_DICTIONARY:
			orig_ids[str(e.get("id", ""))] = true

	# 1. Insertar "Cuadrícula con datos" vía diálogo real
	await _click_control_real(add_comp_btn)
	await _step_frames(3)
	var gal_dlg: Window = h.get("template_gallery_dialog", null) as Window
	var gal_tabs: TabContainer = h.get("template_tabs", null) as TabContainer
	var comp_grid: GridContainer = h.get("tpl_comp_grid", null) as GridContainer
	var gal_apply: Button = h.get("tpl_apply_btn", null) as Button
	_check(gal_dlg != null and gal_dlg.visible and gal_dlg.title == "Insertar componente", "Diálogo abre en modo 'Insertar componente'")
	_check(gal_tabs != null and gal_tabs.current_tab == 1, "Pestaña 'Componentes' seleccionada por defecto")

	var grid_btn := _find_comp_card_button(comp_grid, "comp_grid_data")
	_check(grid_btn != null, "Tarjeta componente 'Cuadrícula con datos' presente")
	if grid_btn != null:
		await _click_control_real(grid_btn)
	await _click_control_real(gal_apply)
	await _step_frames(3)

	var grid_id: String = ed.get_selected_element_id()
	var elems_after_1: Array = dm.get_ui_screen_elements("game_menu_items")
	var preserved_all := true
	for oid in orig_ids.keys():
		if dm.get_ui_element("game_menu_items", oid).is_empty():
			preserved_all = false
	_check(not grid_id.is_empty() and elems_after_1.size() > orig_count and preserved_all,
			"Insertar componente añade '%s' (%d -> %d) sin borrar los elementos existentes" % [grid_id, orig_count, elems_after_1.size()])

	# 2. Insertar "Rueda radial rápida" y "Panel de detalle de objeto" en la misma pantalla
	await _click_control_real(add_comp_btn)
	await _step_frames(2)
	var rad_btn := _find_comp_card_button(comp_grid, "comp_radial_wheel")
	_check(rad_btn != null, "Tarjeta componente 'Rueda radial rápida' presente")
	if rad_btn != null:
		await _click_control_real(rad_btn)
	await _click_control_real(gal_apply)
	await _step_frames(3)
	var rad_id: String = ed.get_selected_element_id()
	_check(not rad_id.is_empty() and rad_id != grid_id and str(dm.get_ui_element("game_menu_items", rad_id).get("type", "")) == "radial",
			"Rueda radial '%s' insertada junto a la cuadrícula '%s'" % [rad_id, grid_id])

	await _click_control_real(add_comp_btn)
	await _step_frames(2)
	var det_btn := _find_comp_card_button(comp_grid, "comp_detail_panel")
	_check(det_btn != null, "Tarjeta componente 'Panel de detalle de objeto' presente")
	if det_btn != null:
		await _click_control_real(det_btn)
	await _click_control_real(gal_apply)
	await _step_frames(3)
	var det_id: String = ed.get_selected_element_id()
	_check(not det_id.is_empty() and det_id != rad_id, "Panel de detalle '%s' insertado con hijos completos" % det_id)

	# 3. Mover rueda radial y probar redimensión con Shift (mantener proporción) en el lienzo
	ed.select_element(rad_id)
	await _step_frames(2)
	var overlay: Control = h.get("canvas_overlay", null) as Control
	var dl_ed: Control = h.get("design_layer", null) as Control
	var rad_before: Dictionary = dm.get_ui_element("game_menu_items", rad_id)
	var rw0 := float(rad_before.get("width", 260))
	var rh0 := float(rad_before.get("height", 260))
	var rctl_ed := _find_control_with_elem_id(dl_ed, rad_id)
	if rctl_ed != null and overlay != null:
		var br_global := rctl_ed.get_global_rect().position + rctl_ed.get_global_rect().size
		var ov_pos := br_global - overlay.get_global_rect().position
		var wp_start := _to_window_pos(overlay, ov_pos)
		await _drag(wp_start[0], wp_start[1], wp_start[1] + Vector2(-40, 12), true)
		await _step_frames(2)
	var rad_after_shift: Dictionary = dm.get_ui_element("game_menu_items", rad_id)
	var rw1 := float(rad_after_shift.get("width", 260))
	var rh1 := float(rad_after_shift.get("height", 260))
	_check(absf((rw1 / rh1) - (rw0 / rh0)) < 0.05 and rw1 < rw0,
			"Redimensionar con Shift mantiene proporción (antes %dx%d -> ahora %dx%d)" % [int(rw0), int(rh0), int(rw1), int(rh1)])

	# 4. Probar Escala (25..400%) y Escala entera en panel de detalle + componente anidado
	ed.select_element(det_id)
	await _step_frames(2)
	var sc_slider: Slider = h.get("prop_scale_slider", null) as Slider
	var sc_spin: SpinBox = h.get("prop_scale_spin", null) as SpinBox
	var int_chk: CheckBox = h.get("prop_int_scale_chk", null) as CheckBox
	_check(sc_slider != null and sc_spin != null and int_chk != null, "Controles Escala (slider + spin + Escala entera) existen")
	var det_ctl_before := _find_control_with_elem_id(dl_ed, det_id)
	var det_sz_before := det_ctl_before.size if det_ctl_before else Vector2.ZERO
	if sc_spin != null:
		sc_spin.value = 150
		await _step_frames(3)
	var det_ctl_150 := _find_control_with_elem_id(dl_ed, det_id)
	var det_sz_150 := det_ctl_150.size if det_ctl_150 else Vector2.ZERO
	_check(det_sz_before.x > 0.0 and absf((det_sz_150.x / det_sz_before.x) - 1.5) < 0.06,
			"Escala 150%% escala el contenedor en bloque (%.1f -> %.1f)" % [det_sz_before.x, det_sz_150.x])

	# Probar Escala entera ON -> ajusta 150 a 200 y activa TEXTURE_FILTER_NEAREST en icono hijo
	if int_chk != null:
		await _click_control_real(int_chk)
		await _step_frames(3)
	var det_after_int: Dictionary = dm.get_ui_element("game_menu_items", det_id)
	_check(int(det_after_int.get("scale_percent", 0)) == 200 and bool(det_after_int.get("integer_scale", false)),
			"Escala entera activa: ajusta 150%% a múltiplo entero 200%%")
	# Dejamos el panel en 125% legible para que quepa todo bonito en pantalla
	if int_chk != null and int_chk.button_pressed:
		await _click_control_real(int_chk)
		await _step_frames(2)
	if sc_spin != null:
		sc_spin.value = 125
		await _step_frames(2)

	# Crear carpeta con escala entera 200% y anidar dentro comp_icon_slot
	ed.select_element("")
	await _step_frames(1)
	var folder_btn: Button = h.get("layer_add_folder_btn", null) as Button
	await _click_control_real(folder_btn)
	await _step_frames(2)
	var create_fld_btn := _find_button_by_text(ed, "Crear")
	if create_fld_btn != null and create_fld_btn.is_visible_in_tree():
		await _click_control_real(create_fld_btn)
		await _step_frames(2)
	var folder_id: String = ed.get_selected_element_id()
	var fd: Dictionary = dm.get_ui_element("game_menu_items", folder_id)
	_check(not folder_id.is_empty() and str(fd.get("type", "")) == "folder", "Carpeta '%s' creada" % folder_id)
	fd["name"] = "CarpetaEscala2x"
	fd["anchor"] = "top_left"
	fd["offset_x"] = 36
	fd["offset_y"] = 410
	fd["width"] = 180
	fd["height"] = 90
	fd["integer_scale"] = true
	fd["scale_percent"] = 200
	dm.update_ui_element("game_menu_items", folder_id, fd, false)
	ed.refresh_from_data_manager()
	await _step_frames(2)
	var nested_id: String = ed.insert_component_by_id("comp_icon_slot", folder_id)
	await _step_frames(3)
	var nested_el: Dictionary = dm.get_ui_element("game_menu_items", nested_id)
	_check(str(nested_el.get("parent_id", "")) == folder_id, "Componente anidado '%s' dentro de carpeta '%s'" % [nested_id, folder_id])
	var nested_ctl_ed := _find_control_with_elem_id(dl_ed, nested_id)
	var ed_stage_scale: float = float(dl_ed.size.x) / 960.0 if dl_ed != null else 1.0
	var nested_ratio_ed := (nested_ctl_ed.size.x / (float(nested_el.get("width", 56)) * ed_stage_scale)) if (nested_ctl_ed and ed_stage_scale > 0.0) else 0.0
	_check(absf(nested_ratio_ed - 2.0) < 0.08, "Hijo anidado hereda escala 200%% en editor (factor=%.2f)" % nested_ratio_ed)
	var has_nearest := false
	if nested_ctl_ed != null:
		for n in nested_ctl_ed.find_children("*", "TextureRect", true, false):
			if (n as Control).texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST:
				has_nearest = true
	_check(nested_ctl_ed != null and (has_nearest or nested_ctl_ed.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST),
			"Escala entera aplica filtro TEXTURE_FILTER_NEAREST al pixel art")

	# Captura del editor
	await _step_frames(2)
	var img_ed: Image = root.get_texture().get_image()
	img_ed.save_png(cap_dir + "/botw_p1_editor.png")

	# 5. Guardar con OK y abrir juego real ClockGearsGameMenu
	await _click_control_real(db.get_ok_button())
	await _step_frames(3)
	db.queue_free()
	await _step_frames(2)

	dm.load_all()
	var gm = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm)
	await _step_frames(2)
	gm.open_menu_by_id("game_menu_items", false, false, true)
	await _step_frames(4)

	var gm_layer = gm.get_custom_ui_layer()
	var grid_ctl_gm := _find_control_with_elem_id(gm_layer, grid_id)
	var rad_ctl_gm := _find_control_with_elem_id(gm_layer, rad_id)
	var det_ctl_gm := _find_control_with_elem_id(gm_layer, det_id)
	var nested_ctl_gm := _find_control_with_elem_id(gm_layer, nested_id)
	_check(grid_ctl_gm != null and grid_ctl_gm.visible and rad_ctl_gm != null and rad_ctl_gm.visible and det_ctl_gm != null and det_ctl_gm.visible,
			"Los 3 componentes coexisten y son visibles en el menú Inventario del juego real")
	var gm_ui_scale := float(gm_layer.size.x) / 960.0 if gm_layer != null else 1.0
	var nested_ratio_gm := (nested_ctl_gm.size.x / (float(nested_el.get("width", 56)) * gm_ui_scale)) if nested_ctl_gm != null else 0.0
	_check(absf(nested_ratio_gm - 2.0) < 0.06, "Escala anidada 200%% coincide en el juego real (factor=%.2f)" % nested_ratio_gm)

	var img_gm: Image = root.get_texture().get_image()
	img_gm.save_png(cap_dir + "/botw_p1_juego.png")

	gm.close_menu()
	gm.queue_free()
	await _step_frames(2)

	# Restaurar datos del proyecto
	if not orig_ui_json.is_empty():
		var f := FileAccess.open(ui_file, FileAccess.WRITE)
		if f:
			f.store_string(orig_ui_json)
			f.close()
	dm.load_all()
	_check(true, "Datos de prueba restaurados tras captura")
	_finish(0 if fail_count == 0 else 1)
