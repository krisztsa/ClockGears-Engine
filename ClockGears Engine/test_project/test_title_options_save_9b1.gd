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

func _remove_dir_recursive_abs(abs_dir: String) -> void:
	var da := DirAccess.open(abs_dir)
	if da == null:
		return
	da.list_dir_begin()
	var name := da.get_next()
	while name != "":
		if name != "." and name != "..":
			var p := abs_dir.path_join(name)
			if da.current_is_dir():
				_remove_dir_recursive_abs(p)
				DirAccess.remove_absolute(p)
			else:
				DirAccess.remove_absolute(p)
		name = da.get_next()
	da.list_dir_end()
	DirAccess.remove_absolute(abs_dir)

func _save_test_map(path: String, map_name: String, width: int = 16, height: int = 12) -> void:
	var m := ClockGearsMap.new()
	m.set_name(map_name)
	m.set_map_name(map_name)
	m.set_map_size(Vector2i(width, height))
	m.set_grid_resolution(32)
	m.set_combat_mode(1)
	m.ensure_default_layers()
	for i in range(m.get_child_count()):
		var ch := m.get_child(i)
		if ch:
			ch.set_owner(m)
	var ps := PackedScene.new()
	ps.pack(m)
	ResourceSaver.save(ps, path)
	m.free()

func _run_tests() -> void:
	print("=== TEST 9B TANDA 1: MENÚ PRINCIPAL, OPCIONES Y GUARDADO ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	var am = Engine.get_singleton("ClockGearsAudioManager")
	check(dm != null, "ClockGearsDataManager singleton disponible")
	check(am != null, "ClockGearsAudioManager singleton disponible")
	if dm == null or am == null:
		return

	# Aislar rutas de prueba en user:// para no ensuciar res://clockgears_data ni saves reales
	var data_dir := "user://test_9b1_data"
	var saves_dir := "user://test_9b1_saves"
	var options_path := "user://test_9b1_options.json"
	var maps_dir := "user://test_9b1_maps"
	var start_map_path := maps_dir.path_join("map_inicio.tscn")
	var second_map_path := maps_dir.path_join("map_bosque.tscn")
	var title_scene_path := "user://test_9b1_title.tscn"
	var bg_img_path := "user://test_9b1_bg.png"

	_remove_dir_recursive_abs(ProjectSettings.globalize_path(data_dir))
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(saves_dir))
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(maps_dir))
	for fdel in [options_path, options_path + ".bak", options_path + ".tmp", title_scene_path, bg_img_path]:
		if FileAccess.file_exists(fdel):
			DirAccess.remove_absolute(ProjectSettings.globalize_path(fdel))

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(data_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(saves_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(maps_dir))

	var img := Image.create(64, 32, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.2, 0.4, 0.7, 1.0))
	img.save_png(ProjectSettings.globalize_path(bg_img_path))

	dm.init_empty()
	dm.set_save_directory(saves_dir)
	dm.set_player_options_path(options_path)

	# Crear 2 actores y 2 mapas reales
	dm.set_category_array(0, [
		{"id": 1, "name": "Kael", "class_id": 1, "hp": 150, "mp": 40, "atk": 22, "def": 10},
		{"id": 2, "name": "Lyra", "class_id": 1, "hp": 120, "mp": 60, "atk": 18, "def": 12}
	])
	dm.set_category_array(1, [{"id": 1, "name": "Aventurero", "exp_base": 30, "exp_extra": 20, "exp_acc_a": 30, "exp_acc_b": 30}])
	_save_test_map(start_map_path, "AldeaInicio")
	_save_test_map(second_map_path, "BosqueOscuro")

	var sys: Dictionary = dm.get_system_config()
	sys["game_title"] = "Crónicas de ClockGears"
	sys["start_map"] = start_map_path
	sys["start_x"] = 4
	sys["start_y"] = 5
	sys["start_dir"] = 6
	sys["party"] = [1]
	dm.set_system_config(sys)
	dm.save_all(data_dir)

	# 1. Arranque en pantalla de título + Continuar oculto sin guardados + Créditos oculto
	check(ClockGearsTitleScreen.ensure_title_screen_scene(title_scene_path), "Escena de pantalla de título generada correctamente")
	var packed_title := ResourceLoader.load(title_scene_path) as PackedScene
	check(packed_title != null, "title_screen.tscn carga como PackedScene")

	var title_node := packed_title.instantiate() as ClockGearsTitleScreen
	check(title_node != null, "Instancia raíz es ClockGearsTitleScreen")
	title_node.set_auto_quit_tree(false)
	get_root().add_child(title_node)
	set_current_scene(title_node)
	await process_frame

	var ids0: PackedStringArray = title_node.get_visible_entry_ids()
	check(Array(ids0) == ["new_game", "options", "quit"], "Sin guardados ni créditos: visibles [new_game, options, quit] (obtenido: %s)" % [str(ids0)])
	check(not ids0.has("continue"), "Continuar NO aparece cuando no hay partidas guardadas")
	check(not ids0.has("credits"), "Créditos NO aparece cuando credits_configured es false")
	check(title_node.get_title_text_displayed() == "Crónicas de ClockGears", "Título visible hereda game_title del Sistema")
	check(dm.get_applied_window_title() == "Crónicas de ClockGears", "Título de ventana aplicado desde game_title")

	# 2. Nueva partida abre el mapa inicial en la posición inicial de Sistema
	check(title_node.trigger_new_game(), "trigger_new_game devuelve true y solicita transición")
	await process_frame
	await process_frame

	var cur_after_new := get_current_scene()
	check(cur_after_new is ClockGearsMap, "Nueva partida cargó escena ClockGearsMap")
	if cur_after_new is ClockGearsMap:
		var m_new := cur_after_new as ClockGearsMap
		check(m_new.get_scene_file_path() == start_map_path, "Nueva partida abre el Mapa Inicial de Sistema")
		var pl_new = m_new.get_runtime_player()
		check(pl_new != null and pl_new.get_grid_cell() == Vector2i(4, 5) and pl_new.get_direction() == 6,
			"Jugador inicia en posición inicial exacta de Sistema (4, 5, dir 6)")

	# 3. Guardado completo en ranura N + Continuar visible + Restauración completa al cargar
	dm.set_switch("puerta_abierta", true)
	dm.set_variable("llaves", 7)
	dm.change_item("Poción", 3)
	dm.set_gold(420)
	dm.set_actor_runtime_hp(1, 88)
	dm.set_runtime_clock_sec(3725.0) # 01:02:05
	dm.mark_arpg_enemy_defeated("BosqueOscuro", 9)
	sys = dm.get_system_config()
	sys["party"] = [1, 2]
	dm.set_system_config(sys)

	var save_ctx := {
		"map_path": second_map_path,
		"map_name": "Bosque Oscuro",
		"player_x": 9,
		"player_y": 7,
		"player_px_x": 304.0,
		"player_px_y": 236.0,
		"player_dir": 8,
		"party": [1, 2],
	}
	check(dm.get_save_slot_count() == 10, "Por defecto hay 10 ranuras de guardado")
	check(dm.save_game_slot(3, save_ctx), "Guardar en ranura 3 exitoso")
	check(dm.has_any_save(), "has_any_save() es true tras guardar")

	var slot_info: Dictionary = dm.get_save_slot_info(3)
	check(bool(slot_info.get("exists", false)), "Ranura 3 reporta exists = true")
	check(String(slot_info.get("map_path", "")) == second_map_path, "Ranura guarda map_path")
	check(not String(slot_info.get("saved_at", "")).is_empty(), "Ranura guarda fecha/hora (saved_at)")
	check(String(slot_info.get("playtime_formatted", "")) == "01:02:05", "Ranura formatea tiempo jugado (01:02:05)")

	# Volver al título con otro estado contaminado para verificar que cargar restaura limpio
	dm.reset_runtime_state()
	dm.set_switch("puerta_abierta", false)
	dm.set_variable("llaves", 0)
	dm.set_gold(0)
	sys = dm.get_system_config()
	sys["party"] = [1]
	dm.set_system_config(sys)

	change_scene_to_file(title_scene_path)
	await process_frame
	await process_frame
	var title2 := get_current_scene() as ClockGearsTitleScreen
	check(title2 != null, "Regreso a ClockGearsTitleScreen tras guardar")
	if title2:
		title2.set_auto_quit_tree(false)
		var ids1: PackedStringArray = title2.get_visible_entry_ids()
		check(ids1.has("continue"), "Continuar SÍ aparece cuando existe al menos 1 partida guardada")
		title2.open_continue_menu()
		check(title2.is_continue_menu_open(), "Menú de Continuar abre lista de espacios de guardado")
		check(title2.get_displayed_save_slot_texts().size() == 10, "Se muestran los 10 espacios de guardado configurados")
		check(title2.select_save_slot(3), "Seleccionar ranura 3 carga e inicia escena guardada")

	await process_frame
	await process_frame
	var cur_loaded := get_current_scene()
	check(cur_loaded is ClockGearsMap and (cur_loaded as ClockGearsMap).get_scene_file_path() == second_map_path,
		"Cargar partida abre el mapa guardado (%s)" % second_map_path)
	if cur_loaded is ClockGearsMap:
		var pl_l = (cur_loaded as ClockGearsMap).get_runtime_player()
		check(pl_l != null and pl_l.get_position().is_equal_approx(Vector2(304.0, 236.0)) and pl_l.get_direction() == 8,
			"Cargar partida restaura posición y dirección exactas")
	check(dm.get_switch("puerta_abierta") == true, "Cargar partida restaura interruptores")
	check(dm.get_variable("llaves") == 7, "Cargar partida restaura variables")
	check(dm.get_gold() == 420, "Cargar partida restaura dinero")
	check(dm.get_item_qty("Poción") == 3, "Cargar partida restaura inventario")
	check(dm.get_actor_runtime_hp(1) == 88, "Cargar partida restaura estado del grupo (HP)")
	check(dm.is_arpg_enemy_defeated("BosqueOscuro", 9), "Cargar partida conserva defeated_enemies")
	check(abs(dm.get_runtime_clock_sec() - 3725.0) < 5.0, "Cargar partida restaura tiempo jugado")
	var restored_party: Array = dm.get_system_config().get("party", [])
	check(restored_party == [1, 2], "Cargar partida restaura composición del grupo (%s)" % [str(restored_party)])

	# 4. Personalización del menú principal (desactivar, renombrar, reordenar, fondo, filtro, música, ranuras)
	var custom_tm := {
		"save_slots": 12,
		"credits_configured": false,
		"bgm_path": "res://audio/title_test_nonexistent_silent.ogg",
		"background": {
			"type": "image",
			"color": "#223344",
			"image_path": bg_img_path,
			"scale_mode": "rellenar",
			"filter": "lineal"
		},
		"title": {
			"type": "text",
			"text": "Título Personalizado",
			"image_path": "",
			"filter": "nearest"
		},
		"entries": [
			{"id": "options", "text": "Ajustes", "enabled": true, "order": 0},
			{"id": "continue", "text": "Seguir Aventura", "enabled": true, "order": 1},
			{"id": "new_game", "text": "Empezar", "enabled": true, "order": 2},
			{"id": "quit", "text": "Cerrar", "enabled": false, "order": 3},
			{"id": "credits", "text": "Equipo", "enabled": true, "order": 4}
		]
	}
	dm.set_title_menu_config(custom_tm)
	check(dm.save_all(data_dir), "save_all guarda title_menu.json junto con la BD")

	dm.init_empty()
	dm.set_save_directory(saves_dir)
	dm.set_player_options_path(options_path)
	check(dm.load_all(data_dir), "load_all recarga proyecto con title_menu personalizado")
	check(dm.get_save_slot_count() == 12, "Número configurable de ranuras (12) persiste")

	var tm_loaded: Dictionary = dm.get_title_menu_config()
	check(tm_loaded["background"]["scale_mode"] == "fill", "Modo de escala 'rellenar' normalizado a 'fill' y persistido")
	check(tm_loaded["background"]["filter"] == "linear", "Filtro 'lineal' normalizado a 'linear' y persistido")

	change_scene_to_file(title_scene_path)
	await process_frame
	await process_frame
	var title3 := get_current_scene() as ClockGearsTitleScreen
	check(title3 != null, "Arranque con menú personalizado")
	if title3:
		title3.set_auto_quit_tree(false)
		var ids2: PackedStringArray = title3.get_visible_entry_ids()
		var texts2: PackedStringArray = title3.get_visible_entry_texts()
		check(Array(ids2) == ["options", "continue", "new_game"],
			"Entradas reordenadas y con Salir desactivada + Créditos ocultos: %s" % [str(ids2)])
		check(Array(texts2) == ["Ajustes", "Seguir Aventura", "Empezar"],
			"Entradas renombradas reflejadas al arrancar: %s" % [str(texts2)])
		check(title3.is_background_image_active(), "Imagen de fondo personalizada cargada y activa")
		check(title3.get_background_scale_mode_applied() == "fill", "Stretch mode Rellenar aplicado al fondo")
		check(title3.get_background_filter_applied() == "linear", "Filtro Lineal aplicado al fondo")
		check(title3.get_title_text_displayed() == "Título Personalizado", "Título personalizado visible")
		check(title3.get_applied_bgm_path() == "res://audio/title_test_nonexistent_silent.ogg", "BGM de título reproducida")

		# 5. Opciones en archivo separado, persisten entre sesiones y se aplican al juego
		title3.open_options_menu()
		check(title3.is_options_menu_open(), "Submenú Opciones se abre")
		title3.apply_options_values(0.35, 0.55, 0.70, true)

	check(FileAccess.file_exists(options_path), "Opciones guardadas en su archivo separado (%s)" % options_path)
	var save_txt := FileAccess.get_file_as_string(dm.get_save_slot_path(3))
	check(not save_txt.contains("music_volume") and not save_txt.contains("brightness"),
		"Archivo de partida guardada NO mezcla opciones del jugador")
	var opt_txt := FileAccess.get_file_as_string(options_path)
	check(opt_txt.contains("music_volume") and not opt_txt.contains("puerta_abierta"),
		"Archivo de opciones NO mezcla datos de partida guardada")

	# Simular reinicio de sesión limpio y comprobar persistencia y aplicación de opciones
	dm.init_empty()
	dm.set_save_directory(saves_dir)
	dm.set_player_options_path(options_path)
	dm.load_all(data_dir)
	dm.ensure_player_options_loaded()
	dm.apply_player_options(get_root())
	var app_opts: Dictionary = dm.get_applied_player_options_state()
	check(abs(float(app_opts.get("music_volume", 0.0)) - 0.35) < 0.001, "Opción music_volume (0.35) persiste tras reiniciar")
	check(abs(float(app_opts.get("sfx_volume", 0.0)) - 0.55) < 0.001, "Opción sfx_volume (0.55) persiste tras reiniciar")
	check(abs(float(app_opts.get("brightness", 0.0)) - 0.70) < 0.001, "Opción brightness (0.70) persiste tras reiniciar")
	check(bool(app_opts.get("fullscreen", false)) == true, "Opción fullscreen (true) persiste tras reiniciar")
	check(abs(am.get_bus_volume(1) - 0.35) < 0.001, "ClockGearsAudioManager BUS_BGM aplicado (0.35)")
	check(abs(am.get_bus_volume(4) - 0.55) < 0.001, "ClockGearsAudioManager BUS_SE aplicado (0.55)")
	var bright_layer := get_root().get_node_or_null("ClockGearsBrightnessOverlay")
	var bright_rect := bright_layer.get_node_or_null("BrightnessRect") as ColorRect if bright_layer else null
	check(bright_rect != null and bright_rect.visible and abs(bright_rect.color.a - 0.30) < 0.02,
		"Capa global de brillo aplicada al viewport")

	# 6. Unidad Monetaria: fuente única de verdad y reflejada en textos visibles
	dm.set_currency_unit("Créditos")
	var sys_after_cur: Dictionary = dm.get_system_config()
	var vocab_after_cur: Dictionary = dm.get_vocabulary_config()
	check(sys_after_cur.get("currency", "") == "Créditos", "system_config['currency'] almacena Unidad Monetaria")
	check(not sys_after_cur.has("currency_unit") and not sys_after_cur.has("currency_name"),
		"Sin claves duplicadas currency_unit/currency_name en system_config")
	check(not vocab_after_cur.has("gold"), "Sin campo duplicado 'gold' en vocabulary_config")
	check(dm.format_text("Tienes \\G aquí") == "Tienes Créditos aquí", "DataManager.format_text(\\G) usa Unidad Monetaria")
	check(ClockGearsTextParser.to_bbcode("Precio: 50 \\G") == "Precio: 50 Créditos", "TextParser.to_bbcode(\\G) usa Unidad Monetaria")

	var mw := ClockGearsMessageWindow.new()
	get_root().add_child(mw)
	dm.set_gold(250)
	mw.show_message("\\$Tienes dinero suficiente (\\G)", "", "", 0, 0)
	var found_cur_in_mw := false
	for ch in mw.find_children("*", "Label", true, false):
		var lbl := ch as Label
		if lbl and lbl.text.contains("250 Créditos"):
			found_cur_in_mw = true
			break
	check(found_cur_in_mw, "MessageWindow panel de dinero ($) muestra '250 Créditos' sin 'Oro'/'G' hardcodeado")
	mw.queue_free()

	# 7. Migración limpia de proyecto existente SIN title_menu.json ni schema moderno
	var legacy_dir := "user://test_9b1_legacy"
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(legacy_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(legacy_dir))
	FileAccess.open(legacy_dir.path_join("database.json"), FileAccess.WRITE).store_string("{\"schema_version\":2}")
	FileAccess.open(legacy_dir.path_join("actors.json"), FileAccess.WRITE).store_string("[{\"id\":7,\"name\":\"HéroeAntiguo\",\"hp\":200}]")
	FileAccess.open(legacy_dir.path_join("system.json"), FileAccess.WRITE).store_string("{\"game_title\":\"Juego Antiguo\",\"currency_unit\":\"Rupias\",\"start_map\":\"res://maps/m1.tscn\"}")
	FileAccess.open(legacy_dir.path_join("vocabulary.json"), FileAccess.WRITE).store_string("{\"hp\":\"PV\",\"gold\":\"MonedaVieja\"}")

	dm.init_empty()
	dm.set_save_directory(saves_dir)
	dm.set_player_options_path(options_path)
	check(dm.load_all(legacy_dir), "Proyecto antiguo SIN title_menu.json carga sin error")
	var legacy_actors: Array = dm.get_category_array(0)
	check(legacy_actors.size() == 1 and String(legacy_actors[0].get("name", "")) == "HéroeAntiguo",
		"Proyecto antiguo conserva sus actores intactos")
	check(dm.get_currency_unit() == "Rupias", "Proyecto antiguo migra currency_unit legado a fuente única")
	check(dm.get_save_slot_count() == 10, "Proyecto antiguo recibe save_slots=10 por defecto")
	check(dm.get_title_menu_config().get("entries", []).size() == 5, "Proyecto antiguo recibe entradas por defecto del menú de título")

	# Limpieza completa
	if get_current_scene() != null and get_current_scene() != get_root():
		var cs := get_current_scene()
		set_current_scene(null)
		if is_instance_valid(cs):
			cs.free()
	if bright_layer:
		bright_layer.queue_free()
	dm.set_save_directory("")
	dm.set_player_options_path("")
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(data_dir))
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(saves_dir))
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(maps_dir))
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(legacy_dir))
	for fdel2 in [options_path, options_path + ".bak", options_path + ".tmp", title_scene_path, bg_img_path]:
		if FileAccess.file_exists(fdel2):
			DirAccess.remove_absolute(ProjectSettings.globalize_path(fdel2))

func _init() -> void:
	call_deferred("_start")

func _start() -> void:
	await _run_tests()
	print("RESULTADO 9B-1: PASSED=%d FAILED=%d" % [passed, failed])
	quit(1 if failed > 0 else 0)
