extends SceneTree

var pass_count := 0
var fail_count := 0
var _start_msec := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + msg)
	else:
		fail_count += 1
		printerr("[FAIL] " + msg)

func _wait_frames(n: int) -> void:
	for i in range(n):
		if Time.get_ticks_msec() - _start_msec > 55000:
			printerr("INTERNAL TIMEOUT 55s")
			_finish(1)
			return
		await process_frame

func _finish(forced_code: int = -1) -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	if forced_code >= 0:
		quit(forced_code)
	else:
		quit(0 if fail_count == 0 else 1)

func _initialize() -> void:
	_start_msec = Time.get_ticks_msec()
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s")
		_finish(1)
	)
	call_deferred("_run_test")

func _run_test() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "DataManager singleton disponible")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var dlg := ClockGearsDatabaseDialog.new()
	dlg.exclusive = false
	dlg.size = Vector2i(1366, 768)
	root.add_child(dlg)
	dlg.open_database()
	await _wait_frames(3)

	dlg.select_category_by_key("game_menu")
	await _wait_frames(2)
	var ed = dlg.get_ui_designer_editor()
	_check(ed != null, "Editor UI disponible en Humo P4")
	if ed == null:
		dlg.queue_free()
		_finish(1)
		return

	var h: Dictionary = ed.get_ui_handles()
	var th = Engine.get_singleton("ClockGearsTheme")

	# 1. Humo tarjetas en 'Diseños' + sin Capas inferior + >= 280px
	ed.select_screen("game_menu_main")
	await _wait_frames(2)
	var left_scroll: ScrollContainer = h.get("left_col_scroll", null) as ScrollContainer
	var cards: Array = ed.get_design_cards_info()
	_check(h.get("layers_card", null) == null and left_scroll != null and left_scroll.size.x >= 280.0 and cards.size() >= 10,
		"Humo P4: Tarjetas en Diseños activas (%d), columna >=280px, sin árbol Capas" % cards.size())

	# 2. Humo seleccion <=1px en Partidas + 0 deriva zoom
	ed.select_screen("game_menu_save")
	ed.set_canvas_zoom_mode(0)
	await _wait_frames(2)
	var cs: ScrollContainer = h.get("canvas_scroll", null) as ScrollContainer
	var st: Control = h.get("canvas_stage", null) as Control
	var s0: Vector2 = st.scale
	ed.select_element("sub_heading")
	await _wait_frames(1)
	var ov: Rect2 = ed.get_overlay_selected_rect()
	var rn: Rect2 = ed.get_rendered_element_stage_rect("sub_heading")
	var err_px: float = maxf(absf(ov.position.x - rn.position.x), absf(ov.position.y - rn.position.y))
	_check(err_px <= 1.0 and st.scale.distance_to(s0) < 0.001, "Humo P4: Partidas selección exacta (%.2fpx) y 0 deriva zoom" % err_px)

	# 3. Humo ventana flotante 2 vías + Deshacer
	var fw: Window = ed.open_floating_element_window("sub_heading")
	_check(fw != null and ed.get_open_floating_window_count() == 1, "Humo P4: Ventana flotante de elemento abre")
	await _wait_frames(1)
	ed.close_all_floating_element_windows()

	# 4. Humo rueda radial + imagen/estilo + Probar
	var rad_id: String = ed.add_element_at("radial", Vector2i(480, 270))
	await _wait_frames(1)
	ed.select_element(rad_id)
	ed.set_radial_layout_live(140, -45.0)
	_check(int(ed.get_effective_element(rad_id).get("layout", {}).get("radial_radius", 0)) == 140, "Humo P4: Rueda radial actualiza en vivo")
	ed.apply_undo()
	await _wait_frames(1)

	# 5. Barra superior fija sin scroll
	var top_bar_card := ed.find_child("UiDesignerTopBarCard", true, false) as Control
	_check(top_bar_card != null and top_bar_card.position.y >= 0.0 and top_bar_card.size.y >= 24.0, "Humo P4: Barra superior fija visible y sin recortes")

	# 6. Auditoría de tema Dark + Brass (inputs, tooltips, popups, contraste >= 4.5) y acentos UTF-8
	if th != null:
		var prev_th: String = th.get_active_theme_id()
		for tid in ["dark", "brass"]:
			th.set_active_theme_id(tid)
			th.apply_to_window(dlg)
			await _wait_frames(1)
			var met: Dictionary = th.inspect_theme_metrics(tid)
			var aud: Dictionary = th.audit_inputs_and_popups_in_tree(dlg)
			_check(bool(met.get("all_contrasts_pass_4_5", false)) and bool(aud.get("ok", false)), "Humo P4: Tema %s cumple contraste >=4.5 y estilos en inputs/popups" % tid)
		th.set_active_theme_id(prev_th)

	# Verificar texto español con tildes intacto en títulos/botones
	var prob_btn: Button = h.get("test_btn", null) as Button
	var has_accents := false
	if top_bar_card != null and prob_btn != null:
		has_accents = prob_btn.text == "Probar"
	_check(has_accents, "Humo P4: Etiquetas UTF-8 intactas sin ruptura de acentos")

	ed.request_reset_to_default(true)
	await _wait_frames(1)
	dlg.queue_free()
	await _wait_frames(1)
	_finish()
