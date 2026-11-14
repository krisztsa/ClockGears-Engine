extends SceneTree

var _passed := 0
var _failed := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		_passed += 1
	else:
		_failed += 1
		printerr("[FAIL] ", msg)

func _step_frames(n: int = 2) -> void:
	for i in range(n):
		await process_frame

func _step_physics(seconds: float) -> void:
	var dt := 1.0 / float(Engine.physics_ticks_per_second)
	var steps := int(ceil(seconds / dt))
	for i in range(steps):
		await physics_frame

func _run() -> void:
	print("=== TEST ARPG UI SETTINGS & HUD (9A TANDA 3C) ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager singleton exists")
	if dm == null:
		quit(1)
		return

	var orig_sys: Dictionary = dm.get_system_config().duplicate(true)
	var dbg_settings_path := "user://clockgears_debug_settings.json"
	var had_dbg_settings := FileAccess.file_exists(dbg_settings_path)
	var orig_dbg_settings := ""
	if had_dbg_settings:
		orig_dbg_settings = FileAccess.get_file_as_string(dbg_settings_path)
		DirAccess.remove_absolute(ProjectSettings.globalize_path(dbg_settings_path))

	# ------------------------------------------------------------------
	# 1. Propiedades del Mapa: Modo de combate, Reaparicion y Respawn Min
	# ------------------------------------------------------------------
	var dlg = ClockGearsMapPropertiesDialog.new()
	root.add_child(dlg)
	dlg.popup_for_new_map(99, 0)
	await _step_frames(2)

	_check(dlg.get_selected_combat_mode() == -1, "Mapa nuevo: Modo de combate por defecto es Heredar (-1)")
	_check(dlg.get_selected_respawn_mode() == 0, "Mapa nuevo: Reaparicion por defecto es Al volver (0)")
	_check(not dlg.is_respawn_minutes_active(), "Minutos inactivos en modo Al volver")
	_check(dlg.check_dialog_fits_size(Vector2i(1366, 768)), "MapPropertiesDialog cabe en 1366x768")
	_check(dlg.check_dialog_fits_size(Vector2i(1920, 1080)), "MapPropertiesDialog cabe en 1920x1080")

	dlg.set_arpg_map_options(0, 1, 5.0)
	_check(not dlg.is_respawn_minutes_active(), "Minutos inactivos en modo Nunca")
	dlg.set_arpg_map_options(1, 2, 12.5)
	_check(dlg.get_selected_combat_mode() == 1, "Modo de combate cambia a ARPG (1)")
	_check(dlg.get_selected_respawn_mode() == 2, "Reaparicion cambia a Tras X minutos (2)")
	_check(dlg.is_respawn_minutes_active(), "Minutos activos SOLO en Tras X minutos")
	_check(is_equal_approx(dlg.get_selected_respawn_minutes(), 12.5), "Minutos configurados en 12.5")

	# Persistencia de mapa en disco (.tscn) y reapertura en dialogo
	var test_map := ClockGearsMap.new()
	test_map.set_map_name("Mapa 3C UI")
	test_map.set_combat_mode(dlg.get_selected_combat_mode())
	test_map.set_respawn_mode(dlg.get_selected_respawn_mode())
	test_map.set_respawn_minutes(dlg.get_selected_respawn_minutes())
	var packed := PackedScene.new()
	packed.pack(test_map)
	var tmp_map_path := "user://tmp_map_props_3c.tscn"
	ResourceSaver.save(packed, tmp_map_path)
	test_map.free()

	var loaded_ps: PackedScene = ResourceLoader.load(tmp_map_path, "", ResourceLoader.CACHE_MODE_IGNORE)
	_check(loaded_ps != null, "Mapa guardado recargado desde disco")
	if loaded_ps != null:
		var reloaded_map := loaded_ps.instantiate() as ClockGearsMap
		_check(reloaded_map != null, "Instancia ClockGearsMap valida")
		if reloaded_map != null:
			dlg.set_arpg_map_options(reloaded_map.get_combat_mode(), reloaded_map.get_respawn_mode(), reloaded_map.get_respawn_minutes())
			dlg.popup_for_edit_map(99, "MAP099", "Mapa 3C UI", "(Ninguna)", 25, 20, 0)
			await _step_frames(1)
			_check(dlg.get_selected_combat_mode() == 1, "Persiste Modo ARPG al reabrir mapa")
			_check(dlg.get_selected_respawn_mode() == 2, "Persiste Reaparicion Tras X minutos al reabrir")
			_check(dlg.is_respawn_minutes_active(), "Minutos siguen activos al reabrir")
			_check(is_equal_approx(dlg.get_selected_respawn_minutes(), 12.5), "Minutos persisten (12.5) al reabrir")
			reloaded_map.free()
	if FileAccess.file_exists(tmp_map_path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_map_path))
	dlg.queue_free()
	await _step_frames(2)

	# ------------------------------------------------------------------
	# 2. Sistema (Combate en tiempo real) + Regla 16:9
	# ------------------------------------------------------------------
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.select_category_by_key("system")
	await _step_frames(2)

	db.set_system_combat_mode(0)
	await _step_frames(1)
	_check(not db.is_realtime_combat_section_visible(), "Sistema Turnos: 'Combate en tiempo real' oculta")

	db.set_system_combat_mode(1)
	await _step_frames(1)
	_check(db.is_realtime_combat_section_visible(), "Sistema ARPG: 'Combate en tiempo real' visible")

	var default_cfg := db.get_system_config_copy()
	_check(not bool(default_cfg.get("arpg_show_damage_numbers", true)), "Mostrar numeros de dano desactivado por defecto")

	db.set_system_realtime_value("arpg_invuln_seconds", 0.85)
	db.set_system_realtime_value("arpg_block_reduction_pct", 75)
	db.set_system_realtime_value("arpg_dodge_type", "dash")
	db.set_system_realtime_value("arpg_dodge_duration", 0.40)
	db.set_system_realtime_value("arpg_dodge_distance", 128.0)
	db.set_system_realtime_value("arpg_dodge_cooldown", 0.70)
	db.set_system_realtime_value("arpg_show_damage_numbers", true)
	await _step_frames(1)

	var updated_cfg: Dictionary = dm.get_system_config()
	_check(is_equal_approx(float(updated_cfg.get("arpg_invuln_seconds", 0.0)), 0.85), "Invuln 0.85 guardado")
	_check(int(updated_cfg.get("arpg_block_reduction_pct", 0)) == 75, "Reduccion bloqueo 75% guardada")
	_check(String(updated_cfg.get("arpg_dodge_type", "")) == "dash", "Tipo esquivar dash guardado")
	_check(is_equal_approx(float(updated_cfg.get("arpg_dodge_duration", 0.0)), 0.40), "Duracion esquivar 0.40 guardada")
	_check(is_equal_approx(float(updated_cfg.get("arpg_dodge_distance", 0.0)), 128.0), "Distancia esquivar 128 guardada")
	_check(is_equal_approx(float(updated_cfg.get("arpg_dodge_cooldown", 0.0)), 0.70), "Cooldown esquivar 0.70 guardado")
	_check(bool(updated_cfg.get("arpg_show_damage_numbers", false)), "Mostrar numeros de dano true guardado")

	# Verificar persistencia en disco + reapertura limpia de Base de Datos (sin dejar cambios sucios)
	var db_json_path := "res://clockgears_data/database.json"
	var had_db_json := FileAccess.file_exists(db_json_path)
	var orig_db_json := FileAccess.get_file_as_string(db_json_path) if had_db_json else ""
	db.save_to_disk()
	db.queue_free()
	await _step_frames(2)

	var db_reopen := ClockGearsDatabaseDialog.new()
	root.add_child(db_reopen)
	db_reopen.load_from_disk()
	db_reopen.select_category_by_key("system")
	await _step_frames(2)
	var reopened_cfg := db_reopen.get_system_config_copy()
	_check(db_reopen.is_realtime_combat_section_visible(), "Reapertura Database: 'Combate en tiempo real' sigue visible en ARPG")
	_check(is_equal_approx(float(reopened_cfg.get("arpg_invuln_seconds", 0.0)), 0.85), "Reapertura Database: invuln 0.85 persiste")
	_check(int(reopened_cfg.get("arpg_block_reduction_pct", 0)) == 75, "Reapertura Database: reduccion bloqueo 75% persiste")
	_check(String(reopened_cfg.get("arpg_dodge_type", "")) == "dash", "Reapertura Database: tipo dash persiste")
	_check(bool(reopened_cfg.get("arpg_show_damage_numbers", false)), "Reapertura Database: mostrar numeros true persiste")

	var lay_768 := db_reopen.check_layout_fits_window(Vector2i(1366, 768))
	var tabs_768 := db_reopen.check_all_tabs_layout(Vector2i(1366, 768))
	var lay_1080 := db_reopen.check_layout_fits_window(Vector2i(1920, 1080))
	var tabs_1080 := db_reopen.check_all_tabs_layout(Vector2i(1920, 1080))
	var lay_1440 := db_reopen.check_layout_fits_window(Vector2i(2560, 1440))
	_check(bool(lay_768.get("fits_horizontally", false)) and bool(tabs_768.get("all_ok", false)), "Database Sistema 1366x768 sin clip ni scroll horizontal")
	_check(bool(lay_1080.get("fits_horizontally", false)) and bool(tabs_1080.get("all_ok", false)), "Database Sistema 1920x1080 sin clip ni scroll horizontal")
	_check(bool(lay_1440.get("fits_horizontally", false)), "Database Sistema 2560x1440 cabe horizontalmente")

	db_reopen.queue_free()
	await _step_frames(2)

	# Restaurar database.json limpio
	if had_db_json:
		var fw_db := FileAccess.open(db_json_path, FileAccess.WRITE)
		if fw_db != null:
			fw_db.store_string(orig_db_json)
			fw_db.close()

	# ------------------------------------------------------------------
	# 3. F3 + Numeros de dano + Drop Toast + Enemigo Barra + Jugador Parpadeo
	# ------------------------------------------------------------------
	var sys_rt: Dictionary = orig_sys.duplicate(true)
	sys_rt["combat_mode"] = 1
	sys_rt["arpg_invuln_seconds"] = 0.5
	sys_rt["arpg_show_damage_numbers"] = false
	dm.set_system_config(sys_rt)

	var dbg := ClockGearsDebugOverlay.new()
	root.add_child(dbg)
	await _step_frames(2)

	_check(not dbg.is_show_bodies_zones(), "F3: Mostrar cuerpos y zonas apagado por defecto")
	_check(not dbg.is_show_player_hp(), "F3: Mostrar HP del jugador apagado por defecto")
	_check(not dbg.is_show_damage_numbers(), "F3: Mostrar numeros de dano apagado por defecto")

	# Con Sistema=false y F3=false -> no muestra numeros visuales
	dbg.clear_damage_numbers()
	dbg.push_damage_number(Vector2(100, 100), 25, false)
	_check(dbg.get_recent_damage_numbers().size() == 1, "Log reciente sigue registrando el golpe (compat 3C-1)")
	_check(dbg.get_visible_damage_numbers_count() == 0, "Sistema=false y F3=false: 0 numeros flotantes visibles")

	# Sistema=true -> muestra numeros visuales
	sys_rt["arpg_show_damage_numbers"] = true
	dm.set_system_config(sys_rt)
	dbg.push_damage_number(Vector2(100, 100), 30, true)
	_check(dbg.get_visible_damage_numbers_count() == 1, "Sistema=true: muestra numero flotante")

	# Sistema=false y F3=true -> muestra numeros visuales
	dbg.clear_damage_numbers()
	sys_rt["arpg_show_damage_numbers"] = false
	dm.set_system_config(sys_rt)
	dbg.set_show_damage_numbers(true)
	dbg.set_show_player_hp(true)
	dbg.push_damage_number(Vector2(100, 100), 18, false)
	_check(dbg.get_visible_damage_numbers_count() == 1, "F3=true (aunque Sistema=false): muestra numero flotante")

	# Persistencia de F3 tras destruir y reabrir DebugOverlay
	dbg.queue_free()
	await _step_frames(2)
	var dbg2 := ClockGearsDebugOverlay.new()
	root.add_child(dbg2)
	await _step_frames(2)
	_check(dbg2.is_show_damage_numbers(), "F3 Mostrar numeros de dano persiste")
	_check(dbg2.is_show_player_hp(), "F3 Mostrar HP del jugador persiste")

	# Drop Toast con "+1 Pocion" + icono y desaparicion automatica
	dbg2.clear_drop_toasts()
	dm.emit_signal("drop_obtained", "Poción", 1)
	await _step_frames(1)
	var toasts := dbg2.get_active_drop_toasts()
	_check(toasts.size() == 1, "Aviso flotante creado al obtener drop")
	if toasts.size() >= 1:
		var t0: Dictionary = toasts[0]
		_check(String(t0.get("text", "")) == "+1 Poción", "Texto de aviso coincide (+1 Poción)")
		_check(bool(t0.get("has_icon", false)), "Aviso incluye icono del objeto")

	# Esperar >2.5s reales de process para comprobar que el aviso desaparece solo
	var t0 := Time.get_ticks_msec()
	while (Time.get_ticks_msec() - t0) < 2800 and dbg2.get_active_drop_toasts().size() > 0:
		await process_frame
	_check(dbg2.get_active_drop_toasts().size() == 0, "Aviso flotante desaparece automaticamente tras segundos")

	# Escena mapa con Enemigo ARPG (barra de vida) y Jugador ARPG (parpadeo invuln)
	var map := ClockGearsMap.new()
	map.set_map_size(Vector2i(16, 16))
	map.set_combat_mode(1)
	root.add_child(map)
	await _step_frames(1)

	var orig_arpg_enemies: Array = dm.get_category_array(20).duplicate(true)
	dm.set_category_array(20, [{"id": 1, "name": "SlimeBar", "hp": 60, "atk": 10, "def": 0}])
	var sys_map: Dictionary = dm.get_system_config()
	sys_map["combat_mode"] = 1
	sys_map["arpg_invuln_seconds"] = 0.5
	dm.set_system_config(sys_map)

	var enemy := ClockGearsEvent.new()
	enemy.set_event_id(1)
	enemy.set_behavior_type(3)
	enemy.set_behavior_ref_id(1)
	enemy.set_position(Vector2(160, 160))
	map.add_child(enemy)
	await _step_frames(1)

	_check(not enemy.is_arpg_hp_bar_visible(), "Enemigo ARPG: barra oculta antes de recibir dano")
	var took_ev := enemy.take_arpg_hit(10, 0.0, Vector2.RIGHT, 0, false)
	_check(took_ev, "Enemigo ARPG recibe impacto")
	_check(enemy.is_arpg_hp_bar_visible() and enemy.get_arpg_hp_bar_remaining() > 2.5, "Enemigo ARPG: barra visible unos segundos tras golpe")
	await _step_physics(3.2)
	_check(not enemy.is_arpg_hp_bar_visible(), "Enemigo ARPG: barra se oculta tras agotar el tiempo")

	var player := ClockGearsPlayer.new()
	map.add_child(player)
	player.setup_on_map(map, null, null)
	player.load_actor_data(1)
	player.set_grid_cell_immediate(Vector2i(3, 3), 2)
	await _step_frames(1)

	_check(is_equal_approx(player.get_arpg_invuln_blink_alpha(), 1.0), "Jugador opacidad inicial 1.0 antes de invuln")
	var hit_pl := player.take_arpg_hit(1, 0.0, Vector2.LEFT, 0, false, "Slime")
	_check(hit_pl and player.is_arpg_invulnerable(), "Jugador entra en invulnerabilidad tras dano")
	var saw_dim := false
	var saw_bright := false
	for step_i in range(12):
		await _step_physics(0.06)
		var a := player.get_arpg_invuln_blink_alpha()
		if a < 0.5:
			saw_dim = true
		elif a > 0.9:
			saw_bright = true
	_check(saw_dim and saw_bright, "Jugador parpadea (alterna alfa) mientras es invulnerable")
	await _step_physics(0.6)
	_check(not player.is_arpg_invulnerable(), "Invulnerabilidad del jugador finaliza")
	_check(is_equal_approx(player.get_arpg_invuln_blink_alpha(), 1.0), "Opacidad vuelve a 1.0 al terminar invulnerabilidad")

	# Limpieza
	map.queue_free()
	dbg2.queue_free()
	dm.set_category_array(20, orig_arpg_enemies)
	dm.set_system_config(orig_sys)
	if had_dbg_settings:
		var fw := FileAccess.open(dbg_settings_path, FileAccess.WRITE)
		if fw != null:
			fw.store_string(orig_dbg_settings)
			fw.close()
	elif FileAccess.file_exists(dbg_settings_path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(dbg_settings_path))

	await _step_frames(2)
	print("RESULTADOS 3C UI: ", _passed, " PASSED, ", _failed, " FAILED")
	quit(0 if _failed == 0 else 1)

func _initialize() -> void:
	call_deferred("_run")
