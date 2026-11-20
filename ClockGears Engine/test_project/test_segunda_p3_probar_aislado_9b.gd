extends SceneTree

# Test Punto 3 (Segunda Pasada UI 9B): 'Probar' aislado sin romper/pausar la Base de Datos
# - Abrir Probar desde boton real en multiples pestanas incluyendo Mensajes y batalla (con DB exclusive = true)
# - SceneTree NUNCA queda pausado ni durante ni despues de Probar
# - Cierre por Esc, boton 'Cerrar' y close_requested
# - Base de Datos sigue totalmente responsiva (cambiar pestana, editar, OK, cerrar y reabrir)

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
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 3 (SEGUNDA PASADA UI 9B): PROBAR AISLADO ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = true
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)

	var tabs := ["main_menu", "game_menu", "menu_equip", "credits", "hud", "game_over", "messages_battle"]
	for i in range(tabs.size()):
		var ckey: String = tabs[i]
		db.select_category_by_key(ckey)
		await _step_frames(2)
		var ed := db.get_ui_designer_editor()
		var h: Dictionary = ed.get_ui_handles()
		var tbtn: Button = h.get("test_btn", null) as Button
		_check(tbtn != null, "[%s] Boton Probar presente" % ckey)
		tbtn.emit_signal("pressed")
		await _step_frames(3)

		var tinfo: Dictionary = ed.get_test_popup_info()
		var tw: Window = tinfo.get("window", null) as Window
		_check(tw != null and bool(tinfo.get("open", false)), "[%s] Probar abre ventana aislada" % ckey)
		_check(not paused, "[%s] SceneTree NO pausado durante Probar" % ckey)

		if tw != null:
			var close_b := tw.find_child("UiTestCloseButton", true, false) as Button
			_check(close_b != null and close_b.text == "Cerrar", "[%s] Boton 'Cerrar' visible dentro de Probar" % ckey)
			if ckey == "messages_battle":
				var msg_layer := tw.find_child("TestUiIsolatedLayer", true, false) as ClockGearsUiDesignLayer
				_check(msg_layer != null and msg_layer.is_visible_in_tree(), "[messages_battle] Probar renderiza su propia capa de Mensajes y batalla")

			# Probar tres metodos de cierre reales: Esc, boton Cerrar, close_requested
			var mode := i % 3
			if mode == 0:
				var esc := InputEventKey.new()
				esc.keycode = KEY_ESCAPE
				esc.pressed = true
				tw.push_input(esc)
			elif mode == 1 and close_b != null:
				close_b.emit_signal("pressed")
			else:
				tw.emit_signal("close_requested")
			await _step_frames(3)

		var tafter: Dictionary = ed.get_test_popup_info()
		_check(not bool(tafter.get("open", false)), "[%s] Ventana Probar cerrada limpiamente" % ckey)
		_check(not paused, "[%s] SceneTree sigue activo (no pausado) tras cerrar Probar" % ckey)

	# Verificar DB totalmente responsiva despues de Probar Mensajes y batalla: cambiar pestana, editar, OK, reabrir
	db.select_category_by_key("actors")
	await _step_frames(2)
	db.select_category_by_key("messages_battle")
	await _step_frames(2)
	var ed2 := db.get_ui_designer_editor()
	_check(ed2.get_current_screen() == "messages_battle", "DB cambia de pestana y vuelve a messages_battle tras Probar")
	var added_id: String = ed2.quick_add_button_element()
	_check(not added_id.is_empty(), "DB permite editar/anadir elemento tras Probar (%s)" % added_id)

	var ok_b := db.get_ok_button()
	if ok_b != null:
		ok_b.emit_signal("pressed")
		await _step_frames(2)
	_check(not paused and not db.visible, "Boton OK de DB cierra la ventana normalmente sin bloqueo")

	# Reabrir DB y volver a abrir otra pestana y Probar
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)
	_check(db.visible, "DB vuelve a abrir correctamente tras cerrar OK")
	db.select_category_by_key("menu_equip")
	await _step_frames(2)
	var ed3 := db.get_ui_designer_editor()
	_check(ed3 != null and ed3.get_current_screen() == "game_menu_equip", "DB reabierta responde a cambio de pestana")

	dm.reset_ui_screen_design_to_default("messages_battle", false)
	db.queue_free()
	await _step_frames(2)
	_finish(0 if fail_count == 0 else 1)
