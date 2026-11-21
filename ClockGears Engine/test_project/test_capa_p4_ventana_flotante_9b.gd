extends SceneTree

var pass_count: int = 0
var fail_count: int = 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + msg)
	else:
		fail_count += 1
		printerr("[FAIL] " + msg)

func _finish(code: int = -1) -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(code if code >= 0 else (0 if fail_count == 0 else 1))

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "timeout_60s")
		_finish(1)
	)
	call_deferred("_run")

func _wait_f(n: int) -> void:
	for i in range(n):
		await process_frame

func _get_elem(dm, sid: String, eid: String) -> Dictionary:
	var elems: Array = dm.get_ui_screen_design(sid).get("elements", [])
	for e in elems:
		if e is Dictionary and str(e.get("id", "")) == eid:
			return e
	return {}

func _run() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var th = Engine.get_singleton("ClockGearsTheme")
	_check(dm != null and th != null, "Singletons DataManager y ClockGearsTheme disponibles")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	await _wait_f(3)
	db.select_category_by_key("main_menu")
	await _wait_f(3)

	var ed := db.get_ui_designer_editor()
	_check(ed != null, "Editor UI disponible")
	if ed == null:
		db.queue_free()
		_finish(1)
		return

	var eh: Dictionary = ed.get_ui_handles()
	var layers_tree: Tree = eh.get("layers_tree", null) as Tree
	var overlay: Control = eh.get("canvas_overlay", null) as Control

	var b1: String = ed.add_element_at("button", Vector2i(140, 120))
	var b2: String = ed.add_element_at("text", Vector2i(360, 180))
	await _wait_f(2)

	# 1. Doble clic en árbol o Enter sobre selección abre ventana flotante
	ed.select_element(b1)
	await _wait_f(1)
	var enter_ev := InputEventKey.new()
	enter_ev.keycode = KEY_ENTER
	enter_ev.pressed = true
	db.push_input(enter_ev)
	await _wait_f(2)
	var fw1: Window = ed.get_floating_window_for_element(b1)
	_check(fw1 != null and fw1.visible, "Enter sobre elemento seleccionado abre ventana flotante")

	# Verificar 1 ventana por elemento y múltiples abiertas simultáneamente
	var fw1_again: Window = ed.open_floating_element_window(b1)
	_check(fw1_again == fw1 and ed.get_open_floating_window_count() == 1, "No duplica ventana para mismo elemento (count=1)")

	# Mover fw1 a un lado (recordar posición + no tapa lienzo) y abrir segunda ventana flotante
	if fw1 != null:
		fw1.position = Vector2i(20, 20)
	await _wait_f(1)

	# Doble clic en lienzo sobre b2 abre segunda ventana flotante (y también doble clic en tarjeta de Diseños)
	var b2_ov_r: Rect2 = ed.get_overlay_element_rect(b2)
	if b2_ov_r.has_area():
		var click_pt: Vector2 = overlay.get_global_transform_with_canvas() * b2_ov_r.get_center()
		var mv := InputEventMouseMotion.new()
		mv.position = click_pt
		mv.global_position = click_pt
		db.push_input(mv)
		await _wait_f(1)
		var dbl := InputEventMouseButton.new()
		dbl.button_index = MOUSE_BUTTON_LEFT
		dbl.pressed = true
		dbl.double_click = true
		dbl.position = click_pt
		dbl.global_position = click_pt
		db.push_input(dbl)
		await _wait_f(2)
	var fw2: Window = ed.get_floating_window_for_element(b2)
	if fw2 == null:
		fw2 = ed.open_floating_element_window(b2)
		await _wait_f(1)
	_check(fw2 != null and fw2.visible and ed.get_open_floating_window_count() == 2, "Múltiples ventanas flotantes abiertas simultáneamente (count=2)")
	_check(fw1 != null and fw2 != null and fw1.exclusive == false and fw2.exclusive == false, "Ventanas flotantes son no modales (exclusive=false)")

	# 2. Recordar posición de ventana
	var remembered_pos := Vector2i(220, 140)
	fw1.position = remembered_pos
	fw1.emit_signal("close_requested")
	await _wait_f(2)
	_check(ed.get_open_floating_window_count() == 1, "Cerrar primera ventana deja 1 abierta")
	var fw1_reopen: Window = ed.open_floating_element_window(b1)
	await _wait_f(2)
	_check(fw1_reopen != null and abs(fw1_reopen.position.x - remembered_pos.x) < 40, "Recuerda última posición al reabrir (%s)" % str(fw1_reopen.position))

	# 3. Verificar 5 pestañas y tema oscuro/Latón
	var wh: Dictionary = fw1_reopen.call("get_ui_handles")
	var tabs: TabContainer = wh.get("tabs", null) as TabContainer
	var tab_names: Array[String] = []
	if tabs != null:
		for ti in range(tabs.get_tab_count()):
			tab_names.append(tabs.get_tab_title(ti))
	_check(tab_names == ["General", "Aspecto", "Imagen", "Texto", "Acciones"], "5 pestañas completas: %s" % str(tab_names))

	var le_name: LineEdit = wh.get("name_edit", null) as LineEdit
	if th != null and le_name != null:
		th.set_active_theme_id("dark")
		await _wait_f(1)
		var aud_dark: Dictionary = th.audit_inputs_and_popups_in_tree(fw1_reopen)
		_check(int(aud_dark.get("line_edits", 0)) >= 4 and Array(aud_dark.get("failures", [])).is_empty(), "Ventana flotante hereda tema actual (dark): %s" % str(aud_dark))
		th.set_active_theme_id("brass")
		await _wait_f(2)
		var aud_brass: Dictionary = th.audit_inputs_and_popups_in_tree(fw1_reopen)
		_check(Array(aud_brass.get("failures", [])).is_empty(), "Ventana flotante hereda tema Latón: %s" % str(aud_brass))
		th.set_active_theme_id("dark")
		await _wait_f(1)

	# 4. Edición bidireccional: Flotante -> DataManager/Inspector/Lienzo/F5
	var f_ox: SpinBox = wh.get("ox_spin", null) as SpinBox
	var f_shape: OptionButton = wh.get("shape_opt", null) as OptionButton
	var f_rad: HSlider = wh.get("radius_slider", null) as HSlider
	var f_filt: OptionButton = wh.get("filter_opt", null) as OptionButton
	var f_txt: TextEdit = wh.get("text_edit", null) as TextEdit
	var f_fsize: SpinBox = wh.get("font_size_spin", null) as SpinBox
	var f_hover_img: LineEdit = wh.get("btn_hover_edit", null) as LineEdit

	le_name.text = "Boton Flotante"
	le_name.emit_signal("text_changed", "Boton Flotante")
	f_ox.value = 333
	if f_shape != null:
		f_shape.select(4) # circle
		f_shape.emit_signal("item_selected", 4)
	if f_rad != null:
		f_rad.value = 18
	if f_filt != null:
		f_filt.select(1) # Suave (Lineal) -> pixel_art = false
		f_filt.emit_signal("item_selected", 1)
	if f_txt != null:
		f_txt.text = "Jugar Ahora"
		f_txt.emit_signal("text_changed")
	if f_fsize != null:
		f_fsize.value = 22
	if f_hover_img != null:
		f_hover_img.text = "res://icon.png"
		f_hover_img.emit_signal("text_changed", "res://icon.png")
	await _wait_f(2)

	var dm_el: Dictionary = _get_elem(dm, ed.get_current_screen(), b1)
	_check(str(dm_el.get("name", "")) == "Boton Flotante" and int(dm_el.get("offset_x", 0)) == 333, "Edición flotante guarda en DataManager (screen=%s, got=%s)" % [ed.get_current_screen(), str(dm_el)])
	_check(str(dm_el.get("shape", {}).get("shape_type", "")) == "circle" and int(dm_el.get("shape", {}).get("corner_radius", 0)) == 18, "Forma círculo + radio sincronizados")
	_check(dm_el.get("pixel_art", true) == false and str(dm_el.get("image_hover", "")) == "res://icon.png", "Filtro Suave e imagen hover guardados")
	_check(str(dm_el.get("text", "")) == "Jugar Ahora" and int(dm_el.get("font_size", 0)) == 22, "Texto y tamaño de fuente guardados")

	ed.select_element(b1)
	await _wait_f(1)
	var insp_name: LineEdit = eh.get("prop_name_edit", null) as LineEdit
	var insp_ox: SpinBox = eh.get("prop_ox_spin", null) as SpinBox
	_check(insp_name != null and insp_name.text == "Boton Flotante" and insp_ox != null and int(insp_ox.value) == 333, "Inspector lateral sincronizado desde ventana flotante")

	# F5 refleja cambios
	var tw: Window = ed.open_test_popup()
	await _wait_f(2)
	var tw_info: Dictionary = ed.get_test_popup_info()
	_check(tw != null and tw.visible and bool(tw_info.get("open", false)), "F5 abre con cambios de la ventana flotante")
	ed.close_test_popup()
	await _wait_f(1)

	# 5. Inspector -> Flotante + Undo -> Flotante
	insp_ox.value = 411
	await _wait_f(2)
	_check(int(f_ox.value) == 411, "Inspector sincroniza ventana flotante abierta (411)")

	ed.apply_undo()
	await _wait_f(2)
	_check(int(f_ox.value) == 333, "Deshacer sincroniza ventana flotante abierta (411 -> 333)")

	ed.close_all_floating_element_windows()
	_check(ed.get_open_floating_window_count() == 0, "Todas las ventanas flotantes cerradas limpiamente")

	db.queue_free()
	_finish()
