extends SceneTree

var passed: int = 0
var failed: int = 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _step_frames(n: int = 3) -> void:
	for i in range(n):
		await process_frame

func _make_test_png(user_path: String, col: Color) -> void:
	var img := Image.create(32, 32, false, Image.FORMAT_RGBA8)
	img.fill(col)
	img.save_png(user_path)

func _initialize() -> void:
	call_deferred("_run_tests")

func _run_tests() -> void:
	print("=== TEST 9B TANDA 6A: EDITOR VISUAL DE INTERFAZ (DATOS Y RUNTIME) ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton disponible")
	if dm == null:
		quit(1)
		return
	dm.init_empty()

	# Configurar datos reales para probar enlaces de datos y runtime
	var sys: Dictionary = dm.get_system_config()
	sys["game_title"] = "Crónica del Reloj — Edición Áurea"
	sys["currency"] = "Dracmas"
	dm.set_system_config(sys)

	var aid: int = dm.create_record(0, {"id": 1, "name": "Ícaro", "initial_level": 5, "party_initial": true}) # CAT_ACTORS = 0
	var iid: int = dm.create_record(3, {"id": 1, "name": "Poción Vital", "desc": "Cura PV"}) # CAT_ITEMS = 3

	var ikey: String = dm.get_record_inventory_key(3, iid)
	dm.reset_runtime_state()
	dm.set_actor_runtime_hp(aid, 75)
	dm.set_actor_runtime_mp(aid, 18)
	dm.set_gold(420)
	dm.set_runtime_clock_sec(3665.0) # 01:01:05
	dm.change_item(ikey, 4)
	dm.set_variable("llaves_sol", 9)

	var test_img_path := "user://test_ui_9b6a_logo.png"
	_make_test_png(test_img_path, Color(0.25, 0.72, 0.95, 1.0))

	# --- 1. Catálogos de pantallas, tipos, enlaces y acciones ---
	print("[1] Catálogos base del sistema visual")
	var screens: PackedStringArray = dm.get_ui_design_screen_ids()
	check(screens.has("title") and screens.has("game_menu_main") and screens.has("game_menu_items") and screens.has("game_menu_equip") and screens.has("game_menu_status") and screens.has("credits") and screens.has("hud"),
		"Están las 7 pantallas requeridas (Título, Menú principal/Objetos/Equipo/Estado, Créditos, HUD)")

	var etypes: PackedStringArray = dm.get_ui_element_types()
	for req_t in ["image", "text", "button", "frame", "bar", "hearts", "currency", "item", "variable"]:
		check(etypes.has(req_t), "Tipo de elemento soportado: " + req_t)

	var bindings: Array = dm.get_ui_data_bindings()
	var binding_ids: Array = []
	for b in bindings:
		binding_ids.append(String(b.get("id", "")))
	for req_b in ["project_title", "currency_name", "gold", "item_qty", "variable", "player_hp", "player_mp", "actor_name", "actor_level", "playtime"]:
		check(binding_ids.has(req_b), "Enlace de datos en catálogo: " + req_b)

	var title_actions: Array = dm.get_ui_button_actions("title")
	var ta_ids: Array = []
	for a in title_actions:
		ta_ids.append(String(a.get("id", "")))
	check(ta_ids.has("new_game") and ta_ids.has("continue") and ta_ids.has("options") and ta_ids.has("credits") and ta_ids.has("quit"),
		"Catálogo fijo de acciones para Título completo")

	# --- 2. Sin diseño personalizado: aspecto y comportamiento idéntico al defecto ---
	print("[2] Pantallas sin diseño personalizado mantienen aspecto por defecto")
	check(not dm.has_custom_ui_design("title"), "Título inicia sin diseño personalizado")
	check(not dm.has_custom_ui_design("game_menu_main"), "Menú principal inicia sin diseño personalizado")
	check(not dm.has_custom_ui_design("credits"), "Créditos inicia sin diseño personalizado")

	var title_screen := ClockGearsTitleScreen.new()
	title_screen.set_auto_quit_tree(false)
	root.add_child(title_screen)
	title_screen.set_size(Vector2(960, 540))
	await _step_frames(3)
	check(not title_screen.is_custom_ui_active(), "TitleScreen sin diseño custom usa vista por defecto")
	check(title_screen.get_title_text_displayed() == "Crónica del Reloj — Edición Áurea", "TitleScreen por defecto muestra título del proyecto UTF-8")
	check(title_screen.get_visible_entry_ids().has("new_game") and title_screen.get_visible_entry_ids().has("options"),
		"TitleScreen por defecto conserva sus botones estándar")

	# --- 3. Enlaces de datos en vivo y Textos UTF-8 ---
	print("[3] Enlaces de datos en vivo y UTF-8")
	check(dm.resolve_ui_data_binding("project_title") == "Crónica del Reloj — Edición Áurea", "Binding project_title")
	check(dm.resolve_ui_data_binding("currency_name") == "Dracmas", "Binding currency_name")
	check(dm.resolve_ui_data_binding("gold") == "420", "Binding gold")
	check(dm.resolve_ui_data_binding("item_qty", {"item_key": ikey}) == "4", "Binding item_qty")
	check(dm.resolve_ui_data_binding("variable", {"variable_name": "llaves_sol"}) == "9", "Binding variable")
	check(dm.resolve_ui_data_binding("actor_name") == "Ícaro", "Binding actor_name")
	check(int(dm.resolve_ui_data_binding("actor_level")) >= 1, "Binding actor_level")
	check(dm.resolve_ui_data_binding("player_hp").contains("/"), "Binding player_hp formato cur/max")
	check(dm.resolve_ui_data_binding("player_mp").contains("/"), "Binding player_mp formato cur/max")
	check(dm.resolve_ui_data_binding("playtime") == "01:01:05", "Binding playtime")

	# --- 4. Diseño personalizado del Título: imagen añadida, botón movido, orden de foco, ratón/teclado y UTF-8 ---
	print("[4] Título personalizado en runtime (botón movido, imagen, enlaces, teclado/ratón, UTF-8)")
	dm.clear_ui_design_history()

	dm.add_ui_element("title", {
		"id": "custom_bg_frame",
		"type": "frame",
		"anchor": "top_left",
		"offset_x": 20,
		"offset_y": 20,
		"width": 520,
		"height": 420,
		"layer_order": 0
	})
	dm.add_ui_element("title", {
		"id": "custom_logo",
		"type": "image",
		"image_path": test_img_path,
		"anchor": "top_left",
		"offset_x": 40,
		"offset_y": 36,
		"width": 128,
		"height": 96,
		"layer_order": 1
	})
	var utf8_caption := "¡Bienvenidos! á é í ó ú ñ ¿Qué deseas hacer?"
	dm.add_ui_element("title", {
		"id": "utf8_title_lbl",
		"type": "text",
		"text": utf8_caption,
		"anchor": "top_left",
		"offset_x": 40,
		"offset_y": 144,
		"width": 440,
		"height": 40,
		"layer_order": 2
	})
	dm.add_ui_element("title", {
		"id": "live_gold_lbl",
		"type": "text",
		"data_binding": "gold",
		"text": "Tesoro: {value}",
		"anchor": "top_right",
		"offset_x": -24,
		"offset_y": 24,
		"width": 180,
		"height": 34,
		"layer_order": 2
	})
	dm.add_ui_element("title", {
		"id": "item_counter_title",
		"type": "item",
		"item_key": ikey,
		"show_name": true,
		"anchor": "top_right",
		"offset_x": -24,
		"offset_y": 68,
		"width": 200,
		"height": 38,
		"layer_order": 2
	})
	dm.add_ui_element("title", {
		"id": "var_title_el",
		"type": "variable",
		"variable_name": "llaves_sol",
		"prefix": "Llaves: ",
		"anchor": "top_right",
		"offset_x": -24,
		"offset_y": 114,
		"width": 180,
		"height": 34,
		"layer_order": 2
	})
	# Botones con orden de foco invertido a propósito: primero 'options' (focus_order=1), luego 'new_game' (focus_order=2)
	dm.add_ui_element("title", {
		"id": "btn_start",
		"type": "button",
		"text": "Comenzar ¡Ya!",
		"action": "new_game",
		"focus_order": 2,
		"anchor": "bottom_left",
		"offset_x": 40,
		"offset_y": -100,
		"width": 200,
		"height": 42,
		"layer_order": 3
	})
	dm.add_ui_element("title", {
		"id": "btn_opts",
		"type": "button",
		"text": "Configuración",
		"action": "options",
		"focus_order": 1,
		"anchor": "bottom_left",
		"offset_x": 40,
		"offset_y": -44,
		"width": 200,
		"height": 42,
		"layer_order": 3
	})

	check(dm.has_custom_ui_design("title"), "Título reporta diseño personalizado activo")
	title_screen.rebuild_title_screen()
	await _step_frames(3)
	check(title_screen.is_custom_ui_active(), "TitleScreen activa renderizado personalizado")

	var layer: ClockGearsUiDesignLayer = title_screen.get_custom_ui_layer()
	check(layer != null and layer.get_element_count() == 8, "TitleScreen renderiza los 8 elementos personalizados")
	check(layer.get_element_displayed_text("utf8_title_lbl") == utf8_caption, "Texto UTF-8 con áéíóúñ¿¡ se muestra intacto en runtime")
	check(layer.get_element_displayed_text("live_gold_lbl") == "Tesoro: 420", "Enlace gold en vivo con plantilla {value}")
	check(layer.get_element_displayed_text("item_counter_title").contains("x4"), "Contador de objeto muestra cantidad real x4")
	check(layer.get_element_displayed_text("var_title_el") == "Llaves: 9", "Elemento variable muestra valor en vivo")

	# Cambio en vivo de objeto y variable actualiza automáticamente el runtime
	dm.change_item(ikey, 3)
	dm.set_variable("llaves_sol", 12)
	await _step_frames(2)
	check(layer.get_element_displayed_text("item_counter_title").contains("x7"), "Contador de objeto se actualiza en vivo a x7")
	check(layer.get_element_displayed_text("var_title_el") == "Llaves: 12", "Elemento variable se actualiza en vivo a 12")

	# Navegación de teclado según focus_order (1=options -> DOWN -> 2=new_game)
	var fo := layer.get_focus_order_actions()
	check(fo.size() == 2 and fo[0] == "options" and fo[1] == "new_game", "Orden de foco respeta focus_order configurable (options primero, new_game segundo)")

	var opts_btn := layer.get_element_control("btn_opts").get_node("ContentRoot/ActionButton") as Button
	check(opts_btn != null, "Botón personalizado accesible en árbol GUI para clic de ratón")
	opts_btn.grab_focus()
	await _step_frames(1)
	check(title_screen.handle_navigation_key(KEY_ENTER), "Enter activa botón con foco del diseño custom (options)")
	check(title_screen.is_options_menu_open(), "Acción 'options' abre el panel de Opciones desde diseño custom")
	title_screen.close_submenus()
	await _step_frames(2)
	check(not title_screen.is_options_menu_open() and layer.is_visible(), "Esc / cerrar submenú regresa al diseño custom del Título")

	# Clic de ratón (señal pressed) sobre btn_start
	var started_hits: Array = []
	title_screen.connect("new_game_started", func(mp): started_hits.append(mp))
	var start_btn := layer.get_element_control("btn_start").get_node("ContentRoot/ActionButton") as Button
	start_btn.emit_signal("pressed")
	await _step_frames(1)
	check(started_hits.size() == 1, "Clic en botón movido 'Comenzar ¡Ya!' ejecuta acción new_game")

	# --- 5. Regla 16:9 a 1366x768, 1920x1080 y 2560x1440 ---
	print("[5] Escalado limpio 16:9 sin cortes ni solapamiento de botones")
	for vp_sz in [Vector2(960, 540), Vector2(1366, 768), Vector2(1920, 1080), Vector2(2560, 1440)]:
		var bounds := title_screen.get_layout_bounds_info(vp_sz)
		check(bool(bounds.get("all_within_viewport", false)) and not bool(bounds.get("clips", true)) and not bool(bounds.get("any_overlap", true)),
			"Layout personalizado encaja limpio en %dx%d sin recortes ni solapamientos" % [int(vp_sz.x), int(vp_sz.y)])

	# --- 6. Unificación del HUD existente ---
	print("[6] Unificación del HUD existente con diseño de pantalla 'hud'")
	# (La prueba de 'new_game' en [4] reinició el estado runtime; restauramos objeto y variable vivos)
	dm.change_item(ikey, 7)
	dm.set_variable("llaves_sol", 12)
	var hud_node := ClockGearsHudOverlay.new()
	root.add_child(hud_node)
	hud_node.set_preview_mode(false)
	hud_node.set_size(Vector2(960, 540))
	await _step_frames(2)

	var wid: String = dm.create_hud_widget({
		"id": "hud_pot_counter",
		"type": "item",
		"item_key": ikey,
		"anchor": "bottom_left",
		"offset_x": 16,
		"offset_y": -16
	})
	var var_wid: String = dm.create_hud_widget({
		"id": "hud_var_text",
		"type": "text",
		"text_mode": "variable",
		"variable_name": "llaves_sol",
		"text": "Llaves: {val}",
		"anchor": "bottom_right",
		"offset_x": -16,
		"offset_y": -16
	})
	await _step_frames(2)
	var hud_design: Dictionary = dm.get_ui_screen_design("hud")
	check(Array(hud_design.get("elements", [])).size() == 2, "Los widgets del HUD aparecen directamente como elementos del diseño 'hud'")
	var hud_snap_item := hud_node.get_widget_state(wid)
	var hud_snap_var := hud_node.get_widget_state(var_wid)
	check(int(hud_snap_item.get("qty", -1)) == 7, "HUD muestra en vivo cantidad de objeto (7)")
	check(String(hud_snap_var.get("text", "")) == "Llaves: 12", "HUD muestra en vivo variable de texto ('Llaves: 12')")

	# --- 7. Menú del juego y Créditos con diseño personalizado ---
	print("[7] Menú del juego y Créditos con diseño custom y acción 'volver'")
	var gmenu := ClockGearsGameMenu.new()
	gmenu.set_editor_test_popup_mode(true)
	root.add_child(gmenu)
	dm.add_ui_element("game_menu_main", {
		"id": "gm_back",
		"type": "button",
		"text": "Volver",
		"action": "back",
		"anchor": "top_left",
		"offset_x": 30,
		"offset_y": 30,
		"width": 160,
		"height": 40
	})
	dm.add_ui_element("game_menu_main", {
		"id": "gm_to_items",
		"type": "button",
		"text": "Ver Objetos",
		"action": "items",
		"anchor": "top_left",
		"offset_x": 30,
		"offset_y": 80,
		"width": 160,
		"height": 40
	})
	gmenu.open_menu()
	await _step_frames(2)
	check(gmenu.is_menu_open() and gmenu.is_custom_ui_active(), "GameMenu activa su diseño personalizado al abrir")
	gmenu.get_custom_ui_layer().trigger_action("items")
	await _step_frames(2)
	check(gmenu.get_current_subscreen() == "items" and not gmenu.is_custom_ui_active(), "Subpantalla 'items' sin custom sigue mostrando su UI estándar")
	gmenu.open_subscreen("main")
	await _step_frames(1)
	gmenu.get_custom_ui_layer().trigger_action("back")
	await _step_frames(1)
	check(not gmenu.is_menu_open(), "Acción 'back' del botón personalizado en Principal cierra el menú del juego")

	dm.add_ui_element("credits", {
		"id": "cred_back",
		"type": "button",
		"text": "Volver",
		"action": "back",
		"anchor": "bottom_right",
		"offset_x": -20,
		"offset_y": -20,
		"width": 140,
		"height": 36
	})
	var cplayer := ClockGearsCreditsPlayer.new()
	root.add_child(cplayer)
	cplayer.start_credits({"items": [{"type": "text", "text": "Fin"}], "scroll_speed": 60})
	await _step_frames(2)
	check(cplayer.is_playing() and cplayer.is_custom_ui_active(), "CreditsPlayer activa overlay custom cuando existe")
	cplayer.get_custom_ui_layer().trigger_action("back")
	await _step_frames(1)
	check(not cplayer.is_playing() and cplayer.was_cancelled(), "Botón 'back' en Créditos cierra créditos")

	# --- 8. Restablecer a diseño por defecto ---
	print("[8] Restablecer a diseño por defecto revierte al diseño base")
	dm.reset_ui_screen_design_to_default("title")
	check(not dm.has_custom_ui_design("title"), "Tras Restablecer, Título ya no tiene diseño custom")
	title_screen.rebuild_title_screen()
	await _step_frames(2)
	check(not title_screen.is_custom_ui_active(), "TitleScreen revierte inmediatamente al aspecto por defecto")
	check(title_screen.get_visible_entry_ids().has("new_game"), "Botones por defecto visibles tras Restablecer")

	# --- 9. Deshacer y Rehacer hasta 100 pasos ---
	print("[9] Pila de Deshacer y Rehacer (hasta 100 pasos)")
	# Acabamos de resetear 'title' con record_undo = true, así que 1 undo debe restaurar el diseño custom
	check(dm.can_undo_ui_design("title"), "Tras Restablecer se puede deshacer para recuperar el diseño")
	check(dm.undo_ui_design("title"), "Undo restaura diseño previo al reset")
	check(dm.has_custom_ui_design("title") and dm.get_ui_screen_elements("title").size() == 8, "Elementos restaurados tras Undo")
	check(dm.redo_ui_design("title") and not dm.has_custom_ui_design("title"), "Redo vuelve a aplicar el reset")

	dm.clear_ui_design_history("game_menu_equip")
	dm.reset_ui_screen_design_to_default("game_menu_equip", false)
	for step_i in range(1, 106):
		dm.set_ui_screen_design("game_menu_equip", {
			"custom_enabled": true,
			"elements": [{
				"id": "step_lbl",
				"type": "text",
				"text": "Paso %d" % step_i
			}]
		}, true)
	check(dm.get_ui_design_undo_count("game_menu_equip") == 100, "Tope estricto de 100 pasos en la pila de Deshacer (105 mutaciones -> 100)")
	var undone := 0
	while dm.undo_ui_design("game_menu_equip"):
		undone += 1
	check(undone == 100, "Se pudieron deshacer exactamente 100 pasos")
	check(not dm.can_undo_ui_design("game_menu_equip"), "Paso 101 de Undo devuelve false")
	check(dm.get_ui_element("game_menu_equip", "step_lbl").get("text", "") == "Paso 5", "Al deshacer 100 pasos desde 105 queda exactamente en Paso 5")
	check(dm.get_ui_design_redo_count("game_menu_equip") == 100, "Los 100 pasos pasan a la pila de Rehacer")
	var redone := 0
	while dm.redo_ui_design("game_menu_equip"):
		redone += 1
	check(redone == 100, "Se pudieron rehacer los 100 pasos completos")
	check(dm.get_ui_element("game_menu_equip", "step_lbl").get("text", "") == "Paso 105", "Tras 100 Redo vuelve a Paso 105")

	# --- 10. Persistencia save_all / load_all y UTF-8 en disco ---
	print("[10] Persistencia en JSON (ui_designs.json) y lectura UTF-8")
	var save_dir := "user://test_ui_9b6a_proj"
	dm.undo_ui_design("title") # dejar 'title' con diseño custom y texto UTF-8 activo? Espera: tras Redo de title arriba quedó reseteado, con undo queda en Paso anterior = custom activo
	if not dm.has_custom_ui_design("title"):
		dm.undo_ui_design("title")
	check(dm.has_custom_ui_design("title"), "Precondición: diseño custom de Título activo con UTF-8")
	check(dm.save_all(save_dir), "save_all guarda database + ui_designs.json + hud.json")

	var raw_json := FileAccess.get_file_as_string(save_dir.path_join("ui_designs.json"))
	check(raw_json.contains("¡Bienvenidos! á é í ó ú ñ ¿Qué deseas hacer?"), "ui_designs.json conserva caracteres UTF-8 íntegros en disco")

	dm.init_empty()
	check(not dm.has_custom_ui_design("title"), "init_empty limpia diseño custom en memoria")
	check(dm.load_all(save_dir), "load_all recarga proyecto desde disco")
	check(dm.has_custom_ui_design("title"), "load_all restaura diseño personalizado de Título")
	check(dm.get_ui_element("title", "utf8_title_lbl").get("text", "") == utf8_caption, "Texto UTF-8 recargado exactamente tras load_all")
	check(dm.get_ui_screen_elements("hud").size() == 2, "Widgets del HUD unificados recargados tras load_all")

	# Limpieza de archivos temporales en user:// y nodos
	title_screen.queue_free()
	hud_node.queue_free()
	gmenu.queue_free()
	cplayer.queue_free()
	dm.init_empty()

	print("=== RESULTADOS 9B TANDA 6A: %d PASS, %d FAIL ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
