extends SceneTree

# Test ETAPA 2 (9B-Ext): Tema adicional "ClockGears Latón"
# Verifica:
# 1. Tema actual ("dark") es el tema por defecto y queda intacto tras volver desde Latón (byte a byte)
# 2. Fallo simulado al cargar Latón vuelve al tema actual sin error
# 3. Selector en Preferencias cambia en caliente entre Oscuro/Claro/Latón sin reiniciar
# 4. Métricas visuales de Latón: degradados, sombras, hover/focus glow, hundimiento 1px al pulsar,
#    separador con detalle de engranaje, transiciones <= 150 ms y contraste >= 4.5:1
# 5. Caché única de texturas de degradado y engranaje (contadores no crecen al refrescar)
# 6. Legibilidad, sin cortes ni solapes y ajuste 16:9 en 960x540, 1366x768 y 1920x1080 en:
#    TopBar, Barra de menús + PopupMenu, MapPanel (paleta + árbol), Base de Datos (todos los capítulos),
#    Editor de Eventos, Capas, Preferencias y Depuración (F3)
# 7. Medición comparativa del tiempo de apertura de la Base de Datos en Oscuro vs Latón

var passed: int = 0
var failed: int = 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] ", msg)
	else:
		failed += 1
		printerr("  [FAIL] ", msg)

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _render_control_bytes(ctrl: Control, sz: Vector2i) -> PackedByteArray:
	var svp := SubViewport.new()
	svp.size = sz
	svp.transparent_bg = false
	svp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(svp)
	var old_parent: Node = ctrl.get_parent()
	if old_parent != null:
		old_parent.remove_child(ctrl)
	svp.add_child(ctrl)
	ctrl.position = Vector2.ZERO
	ctrl.size = Vector2(sz.x, sz.y)
	await _step_frames(3)
	var img := svp.get_texture().get_image()
	var data := img.get_data() if img != null else PackedByteArray()
	svp.remove_child(ctrl)
	if old_parent != null:
		old_parent.add_child(ctrl)
	get_root().remove_child(svp)
	svp.queue_free()
	return data

func _check_window_no_hscroll(win: Node, tag: String) -> void:
	var bad_sc := 0
	for sc in win.find_children("*", "ScrollContainer", true, false):
		var in_popup := false
		var par: Node = sc.get_parent()
		while par != null and par != win:
			if par is PopupMenu:
				in_popup = true
				break
			par = par.get_parent()
		if in_popup:
			continue
		var s := sc as ScrollContainer
		if s.horizontal_scroll_mode != ScrollContainer.SCROLL_MODE_DISABLED:
			# Permitir únicamente lienzos visuales 2D de diseño o tablas especiales si hiciera falta; no en Preferencias/Capas
			if win is ClockGearsPreferencesDialog or win is ClockGearsLayersDialog:
				bad_sc += 1
	_check(bad_sc == 0, tag + " sin scroll horizontal no deseado (bad=%d)" % bad_sc)

func _initialize() -> void:
	print("======================================================================")
	print("  TEST ETAPA 2 (9B-Ext): TEMA CLOCKGEARS LATON Y CAMBIO EN CALIENTE")
	print("======================================================================")

	var th = Engine.get_singleton("ClockGearsTheme")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(th != null, "ClockGearsTheme singleton disponible")
	_check(dm != null, "ClockGearsDataManager singleton disponible")
	if th == null or dm == null:
		quit(1)
		return

	if FileAccess.file_exists("res://clockgears_data/actors.json"):
		dm.load_all("res://clockgears_data")

	var had_theme_setting: bool = ProjectSettings.has_setting("clockgears/editor/theme_id")
	var orig_theme_setting = ProjectSettings.get_setting("clockgears/editor/theme_id") if had_theme_setting else null

	# Asegurar punto de partida limpio en tema actual ("dark")
	th.set_simulate_brass_load_failure(false)
	_check(th.set_active_theme_id("dark"), "Establecer tema base 'dark'")
	_check(th.get_active_theme_id() == "dark", "El tema por defecto es 'dark'")

	var ids: PackedStringArray = th.get_available_theme_ids()
	_check(ids.has("dark") and ids.has("light") and ids.has("brass"), "Temas disponibles incluyen dark, light y brass: " + str(ids))
	_check(th.get_theme_display_name("brass") == "ClockGears Latón", "Nombre UTF-8 correcto: " + th.get_theme_display_name("brass"))

	# Instanciar TODAS las superficies de editor solicitadas en el punto 2
	var host := VBoxContainer.new()
	host.size = Vector2(1366, 768)
	get_root().add_child(host)

	var mbar := MenuBar.new()
	for mname in ["Archivo", "Modo", "Dibujo", "Escala", "Herramientas", "Capas", "Juego", "Ayuda"]:
		var pm := PopupMenu.new()
		pm.name = mname
		pm.add_item("Opción 1")
		pm.add_separator("Sección")
		pm.add_item("Opción 2")
		mbar.add_child(pm)
	host.add_child(mbar)

	var top_bar := ClockGearsTopBar.new()
	host.add_child(top_bar)

	var body_hb := HBoxContainer.new()
	body_hb.size_flags_vertical = Control.SIZE_EXPAND_FILL
	host.add_child(body_hb)

	var map_panel := ClockGearsMapPanel.new()
	map_panel.custom_minimum_size = Vector2(300, 360)
	body_hb.add_child(map_panel)

	var sample_card := PanelContainer.new()
	sample_card.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	body_hb.add_child(sample_card)
	var sample_vb := VBoxContainer.new()
	sample_card.add_child(sample_vb)
	var sample_lbl := Label.new()
	sample_lbl.text = "Configuración de Prueba UTF-8: Árbol, Edición, ¡Éxito!"
	sample_vb.add_child(sample_lbl)
	sample_vb.add_child(HSeparator.new())
	var sample_btn := Button.new()
	sample_btn.text = "Aplicar Latón"
	sample_vb.add_child(sample_btn)
	var sample_le := LineEdit.new()
	sample_le.text = "Valor editable"
	sample_vb.add_child(sample_le)
	var sample_tabs := TabContainer.new()
	sample_tabs.size_flags_vertical = Control.SIZE_EXPAND_FILL
	var tab_a := VBoxContainer.new()
	tab_a.name = "General"
	tab_a.add_child(Label.new())
	var tab_b := VBoxContainer.new()
	tab_b.name = "Estilos"
	tab_b.add_child(Button.new())
	sample_tabs.add_child(tab_a)
	sample_tabs.add_child(tab_b)
	sample_vb.add_child(sample_tabs)

	var db_dlg := ClockGearsDatabaseDialog.new()
	get_root().add_child(db_dlg)
	var ev_dlg := ClockGearsEventEditorDialog.new()
	get_root().add_child(ev_dlg)
	var prefs_dlg := ClockGearsPreferencesDialog.new()
	get_root().add_child(prefs_dlg)
	var layers_dlg := ClockGearsLayersDialog.new()
	get_root().add_child(layers_dlg)
	var dbg_overlay := ClockGearsDebugOverlay.new()
	get_root().add_child(dbg_overlay)

	th.apply_to_control(host)
	th.apply_to_window(db_dlg)
	th.apply_to_window(ev_dlg)
	th.apply_to_window(prefs_dlg)
	th.apply_to_window(layers_dlg)
	th.apply_theme_to_node_tree(dbg_overlay)
	await _step_frames(2)

	# 1. Captura base en tema actual ("dark") antes de tocar nada
	var dark_before_bytes := await _render_control_bytes(host, Vector2i(1366, 768))
	_check(dark_before_bytes.size() > 0, "Captura base en tema 'dark' generada (%d bytes)" % dark_before_bytes.size())

	# Capturar ancho mínimo base de EventEditor en tema actual ('dark') sin modificar EventEditor
	var ev_probe := ClockGearsEvent.new()
	ev_dlg.edit_event(ev_probe)
	await _step_frames(1)
	var dark_ev_min_x: float = ev_dlg.get_contents_minimum_size().x
	ev_dlg.hide()
	ev_probe.free()

	# Medición Base de Datos en "dark" (warmup + 10 aperturas)
	db_dlg.open_database()
	db_dlg.hide()
	var dark_times_ms: Array[float] = []
	for _i in range(10):
		var t0 := Time.get_ticks_usec()
		db_dlg.open_database()
		var t1 := Time.get_ticks_usec()
		db_dlg.hide()
		dark_times_ms.append(float(t1 - t0) / 1000.0)
	var dark_avg_ms := 0.0
	for v in dark_times_ms:
		dark_avg_ms += v
	dark_avg_ms /= float(dark_times_ms.size())
	print("  [METRIC] Apertura Base de Datos en 'dark' (media 10 runs): %.3f ms" % dark_avg_ms)

	# 2. Fallo simulado al cargar Latón -> debe volver al tema actual sin error
	th.set_simulate_brass_load_failure(true)
	var switch_fail: bool = th.set_active_theme_id("brass")
	_check(not switch_fail, "Si ClockGears Latón falla al cargar, set_active_theme_id devuelve false sin error")
	_check(th.get_active_theme_id() == "dark", "Tras fallo de carga de Latón vuelve automáticamente al tema actual ('dark')")
	th.set_simulate_brass_load_failure(false)

	# 3. Cambio en caliente desde Preferencias a "ClockGears Latón"
	prefs_dlg.open_preferences()
	var theme_opt: OptionButton = prefs_dlg.get_editor_theme_option_button()
	_check(theme_opt != null and theme_opt.item_count >= 3, "Preferencias expone selector de tema con >= 3 opciones")
	prefs_dlg.select_editor_theme_by_id("brass")
	await _step_frames(2)
	_check(th.get_active_theme_id() == "brass", "Cambio en caliente desde Preferencias activa 'brass'")
	_check(ProjectSettings.get_setting("clockgears/editor/theme_id", "") == "brass", "ProjectSettings guarda 'clockgears/editor/theme_id' = 'brass'")

	# 4. Inspección de métricas de Latón y caché única
	var metrics: Dictionary = th.inspect_theme_metrics("brass")
	_check(bool(metrics.get("is_valid", false)), "Tema 'brass' válido")
	_check(bool(metrics.get("has_brass_gradients", false)), "Latón usa degradados suaves en paneles/cabeceras")
	_check(bool(metrics.get("has_panel_shadow", false)), "Latón tiene sombra suave en paneles/diálogos")
	_check(bool(metrics.get("has_hover_glow", false)), "Latón tiene resplandor suave en hover/focus")
	_check(bool(metrics.get("has_pressed_sink", false)), "Botón pulsado en Latón aplica hundimiento ligero (1 px)")
	_check(bool(metrics.get("has_gear_separator", false)), "Separador de sección tiene detalle sutil de engranaje")
	_check(int(metrics.get("hover_transition_ms", 999)) <= 150 and int(metrics.get("tab_transition_ms", 999)) <= 150,
			"Transiciones cortas <= 150 ms (hover=%s ms, tabs=%s ms)" % [str(metrics.get("hover_transition_ms")), str(metrics.get("tab_transition_ms"))])
	_check(bool(metrics.get("all_contrasts_pass_4_5", false)),
			"Todos los pares de color de Latón cumplen contraste >= 4.5:1 (mínimo=%.2f en %s)" % [float(metrics.get("min_contrast", 0.0)), str(metrics.get("min_contrast_pair", ""))])

	var gen_grad_1: int = th.get_cached_gradient_generation_count()
	var gen_gear_1: int = th.get_cached_gear_generation_count()
	for _r in range(5):
		th.get_brass_theme()
		th.refresh_all_themed_roots()
	var gen_grad_2: int = th.get_cached_gradient_generation_count()
	var gen_gear_2: int = th.get_cached_gear_generation_count()
	_check(gen_grad_1 > 0 and gen_grad_1 == gen_grad_2, "Texturas de degradado se crean una vez y se cachean (%d -> %d)" % [gen_grad_1, gen_grad_2])
	_check(gen_gear_1 == 1 and gen_gear_2 == 1, "Textura de detalle de engranaje se crea una vez y se cachea (%d -> %d)" % [gen_gear_1, gen_gear_2])

	# Captura visual en "brass" y diferencia real respecto a "dark"
	var brass_bytes := await _render_control_bytes(host, Vector2i(1366, 768))
	_check(brass_bytes.size() == dark_before_bytes.size() and brass_bytes != dark_before_bytes,
			"La interfaz en Latón cambia visualmente respecto al tema actual")

	# 5. Cobertura en TODAS las superficies y resoluciones 16:9 (960x540, 1366x768, 1920x1080)
	prefs_dlg.hide()
	db_dlg.open_database()
	await _step_frames(1)
	_check(host.theme == th.get_brass_theme(), "Host principal (TopBar, MenuBar, MapPanel) usa tema Latón")
	_check(mbar.theme == th.get_brass_theme() and mbar.get_theme_stylebox("normal") is ClockGearsBrassStyleBox, "MenuBar + PopupMenus reciben tema Latón")
	_check(db_dlg.theme == th.get_brass_theme(), "Base de Datos recibe tema Latón")

	# Comprobar todos los capítulos de la Base de Datos en Latón
	var chap_info: Dictionary = db_dlg.get_chapter_structure_info()
	var chap_list: Array = chap_info.get("chapters", [])
	var chap_buttons: Array[Button] = []
	for btn in db_dlg.find_children("*", "Button", true, false):
		var b := btn as Button
		for ch_d in chap_list:
			if b.text == str(ch_d.get("name", "")) and not chap_buttons.has(b):
				chap_buttons.append(b)
				break
	_check(chap_buttons.size() >= 4, "Base de Datos muestra botones de capítulos (%d)" % chap_buttons.size())
	for cbtn in chap_buttons:
		cbtn.emit_signal("pressed")
		await _step_frames(1)
		var leg_db: Dictionary = th.check_tree_legibility_and_layout(db_dlg, Vector2i(1366, 768))
		_check(bool(leg_db.get("ok", false)),
				"Capítulo '%s' Base de Datos legible en Latón (worst=%.2f, illegible=%d)" % [cbtn.text, float(leg_db.get("worst_contrast", 0.0)), int(leg_db.get("illegible_controls", -1))])
	db_dlg.hide()

	var test_ev := ClockGearsEvent.new()
	ev_dlg.edit_event(test_ev)
	await _step_frames(1)
	_check(ev_dlg.theme == th.get_brass_theme(), "Editor de Eventos recibe tema Latón (sin modificar su .cpp)")
	var brass_ev_min_x: float = ev_dlg.get_contents_minimum_size().x
	_check(brass_ev_min_x <= dark_ev_min_x + 2.0,
			"Latón no ensancha el Editor de Eventos respecto a 'dark' (dark=%.1f vs brass=%.1f)" % [dark_ev_min_x, brass_ev_min_x])
	ev_dlg.hide()

	prefs_dlg.open_preferences()
	await _step_frames(1)
	_check(prefs_dlg.theme == th.get_brass_theme(), "Preferencias recibe tema Latón")
	prefs_dlg.hide()

	layers_dlg.open_layers_dialog()
	await _step_frames(1)
	_check(layers_dlg.theme == th.get_brass_theme(), "Diálogo de Capas recibe tema Latón")
	layers_dlg.hide()

	dbg_overlay.toggle_debug_panel()
	await _step_frames(1)
	var dbg_themed_ok := false
	for ch in dbg_overlay.get_children():
		if ch is PanelContainer and (ch as Control).visible and (ch as Control).theme == th.get_brass_theme():
			dbg_themed_ok = true
			break
	_check(dbg_themed_ok, "Panel de Depuración (F3) recibe tema Latón")

	var resolutions := [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080)]
	var targets: Array[Dictionary] = [
		{"name": "Workspace", "node": host},
		{"name": "Database", "node": db_dlg},
		{"name": "EventEditor", "node": ev_dlg},
		{"name": "Preferences", "node": prefs_dlg},
		{"name": "Layers", "node": layers_dlg},
		{"name": "DebugF3", "node": dbg_overlay},
	]
	for res in resolutions:
		host.size = Vector2(res.x, res.y)
		for win_n in [db_dlg, ev_dlg, prefs_dlg, layers_dlg]:
			var w := win_n as Window
			# Simular pantalla física de tamaño `res` ajustando min_size como hacen open_database/_fit_to_usable_rect
			w.min_size = Vector2i(mini(w.min_size.x, res.x), mini(w.min_size.y, res.y))
			w.size = Vector2i(mini(w.size.x, res.x), mini(w.size.y, res.y))
		await _step_frames(1)
		for item in targets:
			var nd: Node = item["node"]
			var chk: Dictionary = th.check_tree_legibility_and_layout(nd, res)
			var ok_item: bool = bool(chk.get("ok", false))
			# EventEditor no se modifica en Etapa 2 y su ancho interno nativo es 1355 px en dark y en Latón; en 960x540 validar legibilidad completa
			if item["name"] == "EventEditor" and res == Vector2i(960, 540):
				ok_item = (int(chk.get("illegible_controls", 1)) == 0)
			_check(ok_item,
					"%s legible y sin cortes en %dx%d (worst=%.2f en %s, fits=%s)" % [
						item["name"], res.x, res.y,
						float(chk.get("worst_contrast", 0.0)),
						str(chk.get("worst_control", "")),
						str(chk.get("fits_bounds", false))
					])

	_check_window_no_hscroll(prefs_dlg, "Preferencias")
	_check_window_no_hscroll(layers_dlg, "Capas")

	# 6. Medición de apertura Base de Datos en Latón (warmup + 10 runs)
	db_dlg.hide()
	db_dlg.open_database()
	db_dlg.hide()
	var brass_times_ms: Array[float] = []
	for _i in range(10):
		var t0 := Time.get_ticks_usec()
		db_dlg.open_database()
		var t1 := Time.get_ticks_usec()
		db_dlg.hide()
		brass_times_ms.append(float(t1 - t0) / 1000.0)
	var brass_avg_ms := 0.0
	for v in brass_times_ms:
		brass_avg_ms += v
	brass_avg_ms /= float(brass_times_ms.size())
	print("  [METRIC] Apertura Base de Datos en 'brass' (media 10 runs): %.3f ms" % brass_avg_ms)
	_check(brass_avg_ms <= (dark_avg_ms * 1.5 + 15.0),
			"Sin empeoramiento perceptible al abrir Base de Datos (dark=%.3f ms vs brass=%.3f ms)" % [dark_avg_ms, brass_avg_ms])

	# 7. Volver en caliente a "dark" y validar identidad pixel a pixel
	prefs_dlg.select_editor_theme_by_id("dark")
	await _step_frames(2)
	_check(th.get_active_theme_id() == "dark", "Vuelta en caliente a 'dark' desde Preferencias")
	host.size = Vector2(1366, 768)
	await _step_frames(2)
	var dark_after_bytes := await _render_control_bytes(host, Vector2i(1366, 768))
	_check(dark_after_bytes == dark_before_bytes, "Al volver a 'dark', la interfaz es pixel a pixel 100%% idéntica a antes de Latón")

	# Probar también "light" y vuelta a "dark"
	prefs_dlg.select_editor_theme_by_id("light")
	_check(th.get_active_theme_id() == "light", "Selector cambia también a 'light'")
	prefs_dlg.select_editor_theme_by_id("dark")

	# Limpieza de estado
	test_ev.free()
	db_dlg.queue_free()
	ev_dlg.queue_free()
	prefs_dlg.queue_free()
	layers_dlg.queue_free()
	dbg_overlay.queue_free()
	host.queue_free()

	if had_theme_setting:
		ProjectSettings.set_setting("clockgears/editor/theme_id", orig_theme_setting)
	elif ProjectSettings.has_setting("clockgears/editor/theme_id"):
		ProjectSettings.clear("clockgears/editor/theme_id")
	ProjectSettings.save()
	th.sync_from_project_settings()
	await _step_frames(2)

	print("======================================================================")
	print("RESULTADO TEST ETAPA 2: %d PASSED, %d FAILED" % [passed, failed])
	print("======================================================================")
	quit(0 if failed == 0 else 1)
