extends SceneTree

# Test Punto 4 (Segunda Pasada UI 9B): Crear botones con funciones
# - '+ Boton' en barra superior y en paleta crea boton centrado ya seleccionado
# - Inspector completo: texto, icono, 4 imagenes de estado, sonidos pasar/pulsar,
#   visibilidad y habilitado por interruptor, atajo teclado/mando, y lista ordenada
#   (+ Anadir accion, subir/bajar, quitar) con todas las acciones incl. play_music
# - Ejecutar en F5 boton con ('open_menu' submenu + 'play_sound') en orden y verificar

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

func _find_action_idx(opt: OptionButton, act_id: String) -> int:
	if opt == null:
		return -1
	for i in range(opt.item_count):
		if str(opt.get_item_metadata(i)) == act_id:
			return i
	return -1

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 4 (SEGUNDA PASADA UI 9B): BOTONES Y MULTI-ACCION ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	# Verificar catalogo de acciones incluye abrir menu/submenu, cerrar, volver, nueva partida, continuar, salir, titulo, evento comun, interruptor, variable, sonido, musica, HUD
	var all_acts: Array = ClockGearsDataManager.get_all_ui_button_action_types()
	var ids: Array[String] = []
	for ad in all_acts:
		if ad is Dictionary:
			ids.append(str(ad.get("id", "")))
	for req in ["open_menu", "open_system_screen", "back", "close", "new_game", "continue", "to_title", "quit", "common_event", "set_switch", "set_variable", "play_sound", "play_music", "hud_visibility"]:
		_check(ids.has(req), "Accion disponible en catalogo: " + req)

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("game_menu")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = ed.get_ui_handles()
	var bar_btn: Button = h.get("add_button_bar_btn", null) as Button
	var pal: Dictionary = h.get("palette_btns", {})
	var pal_btn: Button = pal.get("button", null) as Button
	_check(bar_btn != null and bar_btn.text == "+ Botón", "Barra superior tiene '+ Botón'")
	_check(pal_btn != null and pal_btn.text == "+ Botón", "Paleta tiene '+ Botón'")

	# Crear submenu personalizado destino
	var sub_id: String = ed.add_submenu_to_current("Submenú Secreto")
	await _step_frames(2)
	_check(not sub_id.is_empty() and dm.has_menu(sub_id), "Submenu creado: " + sub_id)

	# Volver a game_menu_main y pulsar '+ Boton' de barra
	ed.select_screen("game_menu_main")
	await _step_frames(2)
	var before_cnt: int = dm.get_ui_screen_elements("game_menu_main").size()
	if before_cnt == 0:
		before_cnt = Array(dm.get_default_ui_screen_template("game_menu_main").get("elements", [])).size()
	bar_btn.emit_signal("pressed")
	await _step_frames(2)

	var sel_id: String = ed.get_selected_element_id()
	var after_elems: Array = dm.get_ui_screen_elements("game_menu_main")
	_check(not sel_id.is_empty() and after_elems.size() == before_cnt + 1, "'+ Botón' crea y selecciona el nuevo boton (%s)" % sel_id)
	var new_el: Dictionary = dm.get_ui_element("game_menu_main", sel_id)
	var ox := int(new_el.get("offset_x", -1))
	var oy := int(new_el.get("offset_y", -1))
	var bw := int(new_el.get("width", 120))
	var bh := int(new_el.get("height", 36))
	_check(absf(ox + bw * 0.5 - 480.0) <= 2.0 and absf(oy + bh * 0.5 - 270.0) <= 2.0,
		"Boton nuevo aparece centrado en el lienzo (%d,%d,%dx%d)" % [ox, oy, bw, bh])

	# Configurar texto, sonidos, interruptores, atajo, 4 imagenes de estado
	var sound_hov: LineEdit = h.get("prop_sound_hover_edit", null) as LineEdit
	var sound_clk: LineEdit = h.get("prop_sound_click_edit", null) as LineEdit
	var vis_opt: OptionButton = h.get("prop_vis_mode_opt", null) as OptionButton
	var vis_sw: LineEdit = h.get("prop_vis_sw_edit", null) as LineEdit
	var en_opt: OptionButton = h.get("prop_en_mode_opt", null) as OptionButton
	var en_sw: LineEdit = h.get("prop_en_sw_edit", null) as LineEdit
	var add_act_opt: OptionButton = h.get("prop_add_action_opt", null) as OptionButton
	var add_act_btn: Button = h.get("prop_add_action_btn", null) as Button
	var multi_box: VBoxContainer = h.get("prop_multi_actions_vbox", null) as VBoxContainer
	var state_edits: Array = h.get("prop_btn_state_img_edits", [])
	_check(sound_hov != null and sound_clk != null and vis_opt != null and en_opt != null and state_edits.size() == 4,
		"Todos los controles avanzados de boton presentes en inspector")

	new_el["text"] = "Abrir Diario"
	dm.update_ui_element("game_menu_main", sel_id, new_el, false)
	ed.select_element(sel_id)
	await _step_frames(1)

	if sound_hov: sound_hov.text = "res://audio/hover_test.wav"; sound_hov.emit_signal("text_changed", sound_hov.text)
	if sound_clk: sound_clk.text = "res://audio/click_test.wav"; sound_clk.emit_signal("text_changed", sound_clk.text)
	if vis_sw and vis_opt:
		vis_sw.text = "sw_ver_diario"
		vis_opt.select(1) # Si ON
		vis_opt.emit_signal("item_selected", 1)
	if en_sw and en_opt:
		en_sw.text = "sw_hab_diario"
		en_opt.select(1) # Si ON
		en_opt.emit_signal("item_selected", 1)

	# Capturar atajo KEY_J via el boton del inspector
	var cap_btn: Button = h.get("prop_btn_shortcut_capture_btn", null) as Button
	if cap_btn:
		cap_btn.emit_signal("pressed")
		await _step_frames(1)
		var kev := InputEventKey.new()
		kev.keycode = KEY_J
		kev.pressed = true
		db.push_input(kev)
		await _step_frames(1)

	# Anadir primero 'play_sound', luego 'open_menu', y luego reordenar con boton '▲'
	var idx_ps := _find_action_idx(add_act_opt, "play_sound")
	var idx_om := _find_action_idx(add_act_opt, "open_menu")
	_check(idx_ps >= 0 and idx_om >= 0, "Opciones 'play_sound' y 'open_menu' presentes en '+ Añadir acción'")
	add_act_opt.select(idx_ps)
	add_act_btn.emit_signal("pressed")
	await _step_frames(1)
	add_act_opt.select(idx_om)
	add_act_btn.emit_signal("pressed")
	await _step_frames(2)

	# Reordenar paso 2 (open_menu) arriba -> queda [open_menu, play_sound]
	_check(multi_box != null and multi_box.get_child_count() == 2, "Lista ordenada muestra 2 acciones")
	if multi_box != null and multi_box.get_child_count() == 2:
		var row1: Node = multi_box.get_child(1)
		var up_btn: Button = null
		var st: Array[Node] = [row1]
		while not st.is_empty():
			var n: Node = st.pop_back()
			if n is Button and (n as Button).text == "▲":
				up_btn = n as Button
				break
			for c in range(n.get_child_count()):
				st.push_back(n.get_child(c))
		_check(up_btn != null, "Boton reordenar '▲' presente en fila de accion")
		if up_btn:
			up_btn.emit_signal("pressed")
			await _step_frames(2)

	# Asignar target submenu a paso 0 (open_menu) y sonido a paso 1 (play_sound) via controles UI
	if multi_box != null and multi_box.get_child_count() == 2:
		var m_opt: OptionButton = null
		var st0: Array[Node] = [multi_box.get_child(0)]
		while not st0.is_empty():
			var n: Node = st0.pop_back()
			if n is OptionButton:
				m_opt = n as OptionButton
				break
			for c in range(n.get_child_count()):
				st0.push_back(n.get_child(c))
		_check(m_opt != null, "Selector de menu destino presente en paso 'open_menu'")
		if m_opt:
			var mi := _find_action_idx(m_opt, sub_id)
			_check(mi >= 0, "Submenu personalizado aparece en el selector de 'open_menu'")
			if mi >= 0:
				m_opt.select(mi)
				m_opt.emit_signal("item_selected", mi)

		var se_edit: LineEdit = null
		var st1: Array[Node] = [multi_box.get_child(1)]
		while not st1.is_empty():
			var n: Node = st1.pop_back()
			if n is LineEdit:
				se_edit = n as LineEdit
				break
			for c in range(n.get_child_count()):
				st1.push_back(n.get_child(c))
		_check(se_edit != null, "Campo ruta de sonido presente en paso 'play_sound'")
		if se_edit:
			se_edit.text = "res://audio/diario_open.wav"
			se_edit.emit_signal("text_changed", se_edit.text)

	await _step_frames(2)
	var saved_el: Dictionary = dm.get_ui_element("game_menu_main", sel_id)
	var saved_acts: Array = saved_el.get("actions", [])
	_check(saved_acts.size() == 2 and str(Dictionary(saved_acts[0]).get("type", "")) == "open_menu" and str(Dictionary(saved_acts[1]).get("type", "")) == "play_sound",
		"Acciones guardadas en orden correcto [open_menu, play_sound]")

	# Ejecutar en F5 ClockGearsGameMenu: verificar interruptores y ejecucion secuencial
	dm.set_switch("sw_ver_diario", false)
	dm.set_switch("sw_hab_diario", false)
	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	gm.open_menu_by_id("game_menu_main", false, false, false)
	await _step_frames(3)
	var cl := gm.get_custom_ui_layer()
	var rt_root: Control = cl.get_element_control(sel_id)
	var rt_btn := rt_root.find_child("ActionButton", true, false) as Button if rt_root != null else null
	_check(rt_btn != null and not rt_root.is_visible_in_tree(), "F5 oculta boton cuando interruptor de visibilidad esta OFF")

	dm.set_switch("sw_ver_diario", true)
	cl.refresh_live_values()
	await _step_frames(2)
	_check(rt_btn.is_visible_in_tree() and rt_btn.disabled, "F5 muestra boton desactivado cuando interruptor habilitado esta OFF")

	dm.set_switch("sw_hab_diario", true)
	cl.refresh_live_values()
	await _step_frames(2)
	_check(not rt_btn.disabled, "F5 habilita boton al activar interruptor habilitado")

	# Pulsar boton real en F5
	rt_btn.emit_signal("pressed")
	await _step_frames(3)
	var exec_acts: Array = gm.get_last_executed_actions()
	_check(exec_acts.size() == 2 and str(exec_acts[0]) == "open_menu" and str(exec_acts[1]) == "play_sound",
		"F5 ejecuta ambas acciones en orden exacto (%s)" % str(exec_acts))
	_check(gm.get_active_menu_id() == sub_id, "F5 abrio el submenu personalizado (%s == %s)" % [gm.get_active_menu_id(), sub_id])
	_check(gm.get_last_played_menu_sound() == "res://audio/diario_open.wav", "F5 reprodujo el sonido configurado (%s)" % gm.get_last_played_menu_sound())

	# Probar paleta '+ Boton' tambien crea en el centro
	gm.close_menu()
	gm.queue_free()
	ed.select_screen("game_menu_main")
	await _step_frames(2)
	pal_btn.emit_signal("pressed")
	await _step_frames(2)
	var pal_sel: String = str(ed.get_selected_element_id())
	var pal_el: Dictionary = dm.get_ui_element("game_menu_main", pal_sel)
	_check(pal_sel != sel_id and absf(float(pal_el.get("offset_x", 0)) + float(pal_el.get("width", 120)) * 0.5 - 480.0) <= 2.0,
		"Paleta '+ Botón' tambien crea boton centrado ya seleccionado")

	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	db.queue_free()
	await _step_frames(2)
	_finish(0 if fail_count == 0 else 1)
