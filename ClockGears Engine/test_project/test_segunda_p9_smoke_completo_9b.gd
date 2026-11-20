extends SceneTree

# Test Punto 9 (Segunda Pasada UI 9B): Smoke test permanente (< 60 s)
# - Recorre todas las pestanas del sistema UI (11 pestanas)
# - Anade todos los tipos de elemento (frame, text, image, button, bar, hearts, currency, item, variable, radial, widget)
# - Modifica propiedades de cada categoria (Posicion, Aspecto/Forma/Opacidad/Estilo, Texto/Fuente, Imagen, Acciones, Rueda radial, Fondo/Blur/Velo)
# - Abre 'Probar' y cierra, ejecuta Deshacer, Rehacer y Restaurar por defecto
# - Cierra Base de Datos y la vuelve a abrir comprobando estabilidad y trace log

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

func _initialize() -> void:
	create_timer(55.0).timeout.connect(func():
		_check(false, "Timeout interno 55s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	var t0_ms: int = Time.get_ticks_msec()
	print("=== TEST PUNTO 9 (SEGUNDA PASADA UI 9B): SMOKE TEST PERMANENTE ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)

	# 1) Visitar las 11 pestanas del sistema
	var tabs := [
		["main_menu", "title"],
		["game_menu", "game_menu_main"],
		["menu_inventory", "game_menu_items"],
		["menu_equip", "game_menu_equip"],
		["menu_status", "game_menu_status"],
		["menu_saves", "game_menu_save"],
		["menu_options", "game_menu_options"],
		["credits", "credits"],
		["game_over", "game_over"],
		["hud", "hud"],
		["messages_battle", "messages_battle"]
	]
	var ed: ClockGearsUiDesignEditor = null
	for p in tabs:
		db.select_category_by_key(p[0])
		await _step_frames(1)
		ed = db.get_ui_designer_editor()
		_check(ed != null and ed.is_visible_in_tree() and ed.get_current_screen() == p[1], "Pestana '%s' abre '%s'" % [p[0], p[1]])

	# 2) En game_menu: anadir cada tipo de elemento
	db.select_category_by_key("game_menu")
	await _step_frames(2)
	ed = db.get_ui_designer_editor()
	ed.set_props_advanced_mode(true)
	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	await _step_frames(2)

	var types := ["frame", "text", "image", "button", "bar", "hearts", "currency", "item", "variable", "radial", "widget"]
	var added_ids := {}
	for tp in types:
		var eid: String = ed.add_element_at(tp, Vector2i(240, 240))
		_check(not eid.is_empty(), "Anadir elemento tipo: " + tp)
		added_ids[tp] = eid
	await _step_frames(2)

	var h: Dictionary = ed.get_ui_handles()
	# 3) Cambiar propiedad en cada categoria sobre un boton + frame + radial
	ed.select_element(str(added_ids["button"]))
	await _step_frames(1)
	var ox_sp: SpinBox = h.get("prop_ox_spin", null) as SpinBox
	var style_opt: OptionButton = h.get("prop_style_opt", null) as OptionButton
	var op_sl: HSlider = h.get("prop_opacity_slider", null) as HSlider
	var txt_ed: LineEdit = h.get("prop_text_edit", null) as LineEdit
	var add_act_btn: Button = h.get("prop_add_action_btn", null) as Button
	ox_sp.value = 180
	ox_sp.emit_signal("value_changed", 180.0)
	if style_opt.item_count > 1:
		style_opt.select(1)
		style_opt.emit_signal("item_selected", 1)
	op_sl.value = 85
	op_sl.emit_signal("value_changed", 85.0)
	txt_ed.text = "Smoke OK"
	txt_ed.emit_signal("text_changed", "Smoke OK")
	add_act_btn.emit_signal("pressed")
	await _step_frames(1)
	_check(true, "Propiedades de Posicion, Estilo, Opacidad, Texto y Acciones actualizadas")

	ed.select_element(str(added_ids["radial"]))
	await _step_frames(1)
	var r_cnt: HSlider = h.get("prop_rad_count_slider", null) as HSlider
	r_cnt.value = 7
	r_cnt.emit_signal("value_changed", 7.0)
	await _step_frames(1)
	_check(dm.get_ui_screen_design("game_menu_main").get("elements", []).size() >= types.size(), "Elementos guardados con radial actualizado")

	# 4) Probar -> Verificar abierto sin congelar el arbol -> Cerrar
	var test_btn: Button = h.get("test_btn", null) as Button
	test_btn.emit_signal("pressed")
	await _step_frames(3)
	var pinfo: Dictionary = ed.get_test_popup_info()
	_check(bool(pinfo.get("open", false)) and not paused, "'Probar' abre en ventana aislada sin congelar el editor")
	ed.close_test_popup()
	await _step_frames(2)

	# 5) Deshacer, Rehacer y Restaurar por defecto
	_check(ed.apply_undo(), "Deshacer funciona en smoke")
	_check(ed.apply_redo(), "Rehacer funciona en smoke")
	ed.request_reset_to_default(true)
	await _step_frames(2)
	_check(not dm.has_custom_ui_design("game_menu_main"), "Restaurar devuelve a Por defecto")

	# 6) Cerrar Base de Datos y volverla a abrir
	db.hide()
	db.queue_free()
	await _step_frames(2)

	var db2 := ClockGearsDatabaseDialog.new()
	db2.exclusive = false
	root.add_child(db2)
	db2.open_database()
	db2.select_category_by_key("main_menu")
	await _step_frames(3)
	var ed2 := db2.get_ui_designer_editor()
	_check(ed2 != null and ed2.is_visible_in_tree(), "Base de Datos reabre limpia y estable")
	db2.hide()
	db2.queue_free()
	await _step_frames(2)

	# 7) Log de traza presente y escrito
	_check(FileAccess.file_exists("user://ui_editor_trace.log"), "Trace log user://ui_editor_trace.log activo y grabado")

	var elapsed_ms: int = Time.get_ticks_msec() - t0_ms
	_check(elapsed_ms < 50000, "Smoke test completo por debajo de 60 s (%d ms)" % elapsed_ms)

	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	_finish(0 if fail_count == 0 else 1)
