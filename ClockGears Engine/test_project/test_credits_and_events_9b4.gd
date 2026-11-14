extends SceneTree

var failures: Array[String] = []

func check(cond: bool, msg: String) -> void:
	if not cond:
		failures.append(msg)
		printerr("[FAIL] ", msg)

func _make_test_png(path: String, w: int = 64, h: int = 32) -> void:
	var img := Image.create(w, h, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.25, 0.65, 0.95, 1.0))
	for y in range(4, h - 4):
		for x in range(4, w - 4):
			img.set_pixel(x, y, Color(0.95, 0.85, 0.35, 1.0))
	DirAccess.make_dir_recursive_absolute(path.get_base_dir())
	img.save_png(path)

func _initialize() -> void:
	call_deferred("_run_all")

func _run_all() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var cat = Engine.get_singleton("ClockGearsSystemCatalog")
	check(dm != null and cat != null, "Singletons ClockGearsDataManager y ClockGearsSystemCatalog disponibles")
	if not dm or not cat:
		quit(1)
		return

	dm.init_empty()
	var tmp_root := OS.get_user_data_dir().path_join("test_9b4_tmp")
	var data_dir := tmp_root.path_join("clockgears_data")
	var saves_dir := tmp_root.path_join("saves")
	var img_path := tmp_root.path_join("logo_credits.png")
	DirAccess.make_dir_recursive_absolute(data_dir)
	DirAccess.make_dir_recursive_absolute(saves_dir)
	_make_test_png(img_path, 96, 48)
	dm.set_save_directory(saves_dir)

	# Crear actor/mapa mínimos para GameMenu + Save/Load
	var actor_id: int = dm.create_record(0, {"name": "Alden", "initial_level": 2})
	check(actor_id > 0, "Actor de prueba creado")
	var sys: Dictionary = dm.get_system_config()
	sys["starting_party"] = [actor_id]
	dm.set_system_config(sys)

	# --- 1. Regla: Sin líneas en créditos -> NO aparece "Créditos" en Pantalla de Título ---
	var title_empty := ClockGearsTitleScreen.new()
	title_empty.set_auto_quit_tree(false)
	root.add_child(title_empty)
	var vis_before: PackedStringArray = title_empty.get_visible_entry_ids()
	check(not vis_before.has("credits"), "Sin líneas de créditos, la entrada 'credits' NO aparece en el título (visibles=" + str(vis_before) + ")")

	# --- 2. Datos de Créditos: 2 secciones, rol+nombre, texto libre, imagen, espacio, estilos, fondo, música, velocidad y guardado JSON ---
	var credits_cfg := ClockGearsDataManager.get_default_credits_config()
	credits_cfg["background"] = {"type": "color", "color": "#101828ff", "image_path": "", "scale_mode": "fit", "filter": "nearest"}
	credits_cfg["bgm_path"] = "res://audio/credits_theme.ogg"
	credits_cfg["scroll_speed"] = 80.0
	credits_cfg["alignment"] = "center"
	credits_cfg["styles"] = {
		"section": {"font_size": 28, "color": "#ffd166ff"},
		"role": {"font_size": 14, "color": "#94d2bdff"},
		"name": {"font_size": 22, "color": "#ffffffff"},
		"text": {"font_size": 16, "color": "#e2e8f0ff"}
	}
	credits_cfg["items"] = [
		{"type": "section", "text": "Dirección y Diseño"},
		{"type": "role_name", "role": "Director", "name": "Elena Vance"},
		{"type": "spacer", "height": 24.0},
		{"type": "image", "image_path": img_path, "filter": "nearest", "max_width": 200.0, "max_height": 100.0},
		{"type": "section", "text": "Ingeniería y Arte"},
		{"type": "role_name", "role": "Motor C++", "name": "Marco Silva"},
		{"type": "text", "text": "Gracias especiales a toda la comunidad."}
	]
	dm.set_credits_config(credits_cfg)
	check(dm.has_credits_lines(), "has_credits_lines() devuelve true al haber líneas")

	check(dm.save_all(data_dir), "save_all() guarda credits.json sin errores")
	dm.init_empty()
	check(not dm.has_credits_lines(), "init_empty() limpia los créditos")
	check(dm.load_all(data_dir), "load_all() recarga credits.json")
	check(dm.has_credits_lines(), "Tras load_all(), has_credits_lines() vuelve a ser true")
	var loaded_cfg: Dictionary = dm.get_credits_config()
	check(Array(loaded_cfg.get("items", [])).size() == 7, "load_all() conserva los 7 elementos de créditos")
	check(String(loaded_cfg.get("bgm_path", "")) == "res://audio/credits_theme.ogg", "load_all() conserva bgm_path")
	check(abs(float(loaded_cfg.get("scroll_speed", 0.0)) - 80.0) < 0.01, "load_all() conserva scroll_speed=80")

	# Reconstruir título: ahora "credits" SÍ debe aparecer
	title_empty.rebuild_title_screen()
	var vis_after: PackedStringArray = title_empty.get_visible_entry_ids()
	check(vis_after.has("credits"), "Con líneas configuradas, 'credits' aparece en el título (visibles=" + str(vis_after) + ")")

	# --- 3. Ejecución desde el Título: scroll automático completo de 2 secciones, aceleración Enter/clic, Esc y regreso al título ---
	check(title_empty.trigger_entry("credits"), "trigger_entry('credits') abre los créditos desde el título")
	check(title_empty.is_credits_open(), "is_credits_open() es true tras pulsar Créditos")
	var title_cp: ClockGearsCreditsPlayer = title_empty.get_credits_player()
	check(title_cp != null and title_cp.is_playing(), "El reproductor de créditos del título está reproduciendo")

	var s0: Dictionary = title_cp.get_scroll_state()
	check(abs(float(s0.get("scroll_y", 0.0)) - 540.0) < 0.1, "Scroll comienza justo abajo de 540px (y=" + str(s0.get("scroll_y")) + ")")
	check(float(s0.get("content_height", 0.0)) > 150.0, "Contenido de 2 secciones tiene altura positiva (" + str(s0.get("content_height")) + ")")

	# Aceleración con Enter mantenido y con set_accelerated
	title_cp.handle_credits_key(KEY_ENTER, true)
	var s_acc: Dictionary = title_cp.get_scroll_state()
	check(bool(s_acc.get("accelerated", false)) and float(s_acc.get("effective_speed", 0.0)) > float(s_acc.get("scroll_speed", 0.0)), "Mantener Enter acelera el scroll")
	var y_before_acc: float = float(s_acc.get("scroll_y", 540.0))
	title_cp.step_credits(0.5)
	var y_after_acc: float = float(title_cp.get_scroll_state().get("scroll_y", 540.0))
	check(abs((y_before_acc - y_after_acc) - 80.0 * 4.0 * 0.5) < 0.5, "Desplazamiento acelerado exacto 4x en delta=0.5s")
	title_cp.handle_credits_key(KEY_ENTER, false)
	check(not bool(title_cp.get_scroll_state().get("accelerated", true)), "Soltar Enter restaura velocidad normal")

	# Verificar Esc sale y devuelve al título
	check(title_empty.handle_navigation_key(KEY_ESCAPE), "Esc durante créditos en el título cierra los créditos")
	check(not title_empty.is_credits_open() and title_cp.was_cancelled(), "Tras Esc, los créditos terminan como cancelados y vuelven al título")

	# Reabrir y dejar que la última línea salga completamente por arriba para terminar automáticamente
	title_empty.trigger_entry("credits")
	var steps := 0
	while title_cp.is_playing() and steps < 200:
		title_cp.step_credits(0.1)
		steps += 1
	check(not title_cp.is_playing(), "Las 2 secciones salen completamente por arriba y terminan automáticamente")
	check(title_cp.was_completed_naturally(), "was_completed_naturally() == true al llegar al final")
	check(not title_empty.is_credits_open(), "Al terminar naturalmente vuelve al título")

	# --- 4. Regla 16:9: 960x540, 1366x768, 1920x1080 ---
	for res in [Vector2(960, 540), Vector2(1366, 768), Vector2(1920, 1080)]:
		var lb: Dictionary = title_cp.get_layout_bounds_info(res)
		check(bool(lb.get("fits_within_viewport", false)), "Créditos caben en viewport " + str(res))

	title_empty.queue_free()

	# --- 5. Los 6 Comandos de Evento Nuevos + Regresión de eventos antiguos + Guardar/Cargar con bloqueo ---
	var map := ClockGearsMap.new()
	root.add_child(map)
	var interp := ClockGearsEventInterpreter.new()
	root.add_child(interp)
	interp.setup_interpreter(map, null, null)

	# 5a. Evento con Mostrar Créditos -> pausa intérprete -> termina -> ejecuta comando siguiente + viejo (switch + gold)
	var cmds_credits: Array = [
		{"code": 357, "type": "show_credits"},
		{"code": 121, "type": "switch", "name": "7", "val": true},
		{"code": 125, "type": "gold", "amount": 150}
	]
	interp.start_command_list(cmds_credits)
	check(interp.get_state() == ClockGearsEventInterpreter.STATE_WAITING_CREDITS, "Comando 'show_credits' pausa intérprete en STATE_WAITING_CREDITS")
	var rt_cp: ClockGearsCreditsPlayer = interp.get_credits_player()
	check(rt_cp != null and rt_cp.is_playing(), "Créditos del intérprete iniciados correctamente")
	var rt_steps := 0
	while rt_cp.is_playing() and rt_steps < 200:
		rt_cp.step_credits(0.15)
		rt_steps += 1
	check(not rt_cp.is_playing(), "Créditos en partida terminan completamente")
	check(dm.get_switch("7") == true and dm.get_gold() == 150, "Al terminar créditos se reanuda el evento y corren comandos antiguos (switch 7 + gold 150)")
	check(not interp.is_running(), "Intérprete regresa a STATE_IDLE tras completar la lista")

	# 5b. Abrir menú del juego (CG_CMD_OPEN_MENU = 351) y Abrir guardado (CG_CMD_OPEN_SAVE = 352)
	var gmenu: ClockGearsGameMenu = interp.get_game_menu()
	check(gmenu != null, "GameMenu en ClockGearsEventInterpreter existe")
	interp.start_command_list([
		{"code": 351, "type": "open_menu"},
		{"code": 352, "type": "open_save"}
	])
	check(interp.get_state() == ClockGearsEventInterpreter.STATE_WAITING_MENU and gmenu.is_menu_open(), "Comando 'open_menu' abre el menú del juego desde evento")
	check(gmenu.get_current_subscreen() == "main", "open_menu abre en subpantalla 'main'")
	gmenu.close_menu()
	check(gmenu.is_menu_open() and gmenu.get_current_subscreen() == "save", "Tras cerrar open_menu, avanza a 'open_save' directamente en subpantalla 'save'")
	# Guardar en ranura 1 directamente y luego cerrar
	check(gmenu.save_to_slot(1), "Guardar desde pantalla de guardado funciona")
	gmenu.handle_input_key(KEY_ESCAPE)
	check(not gmenu.is_menu_open() and not interp.is_running(), "Esc en 'open_save' directo cierra la ventana y finaliza el evento")

	# 5c. Bloquear / Permitir menú del juego + persistencia al guardar y cargar
	interp.start_command_list([{"code": 358, "type": "change_menu_access", "enabled": false}])
	check(dm.is_game_menu_disabled(), "Comando 'change_menu_access' enabled=false bloquea el menú")
	check(not gmenu.can_open_menu() and not gmenu.open_menu(), "Con menú bloqueado, can_open_menu() y open_menu() retornan false")
	check(dm.save_game_slot(2, {"map_path": "", "player_cell": Vector2i(3, 4)}), "Guardado en ranura 2 con menú bloqueado")
	dm.reset_runtime_state()
	check(not dm.is_game_menu_disabled(), "reset_runtime_state() restablece acceso al menú")
	check(dm.load_game_slot(2), "Cargar ranura 2 restaura estado runtime")
	check(dm.is_game_menu_disabled() and not gmenu.can_open_menu(), "Tras guardar y cargar partida, el bloqueo del menú se conserva")
	interp.start_command_list([{"code": 358, "type": "change_menu_access", "enabled": true}])
	check(not dm.is_game_menu_disabled() and gmenu.can_open_menu(), "Comando 'change_menu_access' enabled=true permite el menú de nuevo")

	# 5d. Cambiar estilo de ventana (CG_CMD_CHANGE_WINDOW_STYLE = 359) y actualización en vivo + persistencia
	var before_msg_style: String = dm.resolve_window_style_id("message")
	check(before_msg_style == "rounded", "Estilo inicial de 'message' es 'rounded' (got=" + before_msg_style + ")")
	interp.start_command_list([
		{"code": 359, "type": "change_window_style", "window_type_id": "message", "style_id": "rect"},
		{"code": 359, "type": "change_window_style", "window_type_id": "game_menu", "style_id": "rect"}
	])
	check(dm.resolve_window_style_id("message") == "rect", "change_window_style cambia 'message' a 'rect'")
	var msg_win: ClockGearsMessageWindow = interp.get_message_window()
	if msg_win:
		var msg_panels: Array = msg_win.find_children("*", "PanelContainer", true, false)
		check(not msg_panels.is_empty(), "MessageWindow tiene PanelContainer inspeccionable")
		if not msg_panels.is_empty():
			var insp: Dictionary = ClockGearsWindowStyleManager.inspect_control_style(msg_panels[0])
			check(String(insp.get("style_id", "")) == "rect", "MessageWindow aplica en vivo style_id='rect' (got=" + str(insp.get("style_id", "")) + ")")
	var cmd_panel = gmenu.find_child("MainCmdPanel", true, false)
	if cmd_panel:
		var g_insp: Dictionary = ClockGearsWindowStyleManager.inspect_control_style(cmd_panel)
		check(String(g_insp.get("style_id", "")) == "rect", "GameMenu MainCmdPanel aplica en vivo style_id='rect'")
	interp.start_command_list([
		{"code": 359, "type": "change_window_style", "window_type_id": "message", "style_id": "none"}
	])
	check(dm.resolve_window_style_id("message") == "none", "change_window_style cambia 'message' de nuevo a 'none'")

	# 5e. Ir a la pantalla de título (CG_CMD_RETURN_TO_TITLE = 354) con confirmación opcional
	interp.start_command_list([
		{"code": 354, "type": "return_to_title", "confirm": true},
		{"code": 122, "type": "variable", "name": "9", "val": 42}
	])
	check(interp.get_state() == ClockGearsEventInterpreter.STATE_WAITING_MENU and gmenu.is_menu_open(), "return_to_title con confirm=true abre diálogo de confirmación")
	check(gmenu.get_current_subscreen() == "to_title", "Subpantalla activa es 'to_title'")
	gmenu.handle_input_key(KEY_ESCAPE) # Cancelar regreso al título
	check(not gmenu.is_menu_open() and dm.get_variable("9") == 42, "Cancelar confirmación reanuda evento (variable 9 = 42)")

	# Sin confirmación
	var off_interp := ClockGearsEventInterpreter.new()
	var off_sig := {"seen": false}
	off_interp.connect("return_to_title_requested", func(): off_sig["seen"] = true)
	off_interp.start_command_list([{"code": 354, "type": "return_to_title", "confirm": false}])
	check(bool(off_sig["seen"]) and off_interp.was_return_to_title_requested(), "return_to_title sin confirmación emite señal y marca solicitud de regreso al título")
	off_interp.queue_free()

	if failures.is_empty():
		print("[PASS] Todas las pruebas de Tanda 4 (Créditos + 6 Comandos + Regresión) pasaron: OK")
		quit(0)
	else:
		printerr("[SUMMARY] Fallos: ", failures.size())
		quit(1)
