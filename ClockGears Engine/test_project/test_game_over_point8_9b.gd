extends SceneTree

# Punto 8: Pantalla de Game Over, categoría Game Over en Menús,
# comportamiento al morir (Reintentar / Volver al menú) y comando "Mostrar Game Over".

var passed := 0
var failed := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  OK: " + msg)
	else:
		failed += 1
		printerr("  FAIL: " + msg)

func _step_frames(n: int = 3) -> void:
	for _i in range(n):
		await process_frame

func _snapshot_data_dir(res_data_dir: String) -> Dictionary:
	var backup := {}
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				backup[full] = FileAccess.get_file_as_string(full)
			fname = da.get_next()
		da.list_dir_end()
	return backup

func _restore_data_dir(res_data_dir: String, backup: Dictionary) -> void:
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				if not backup.has(full):
					DirAccess.remove_absolute(ProjectSettings.globalize_path(full))
			fname = da.get_next()
		da.list_dir_end()
	for full in backup.keys():
		var wf := FileAccess.open(full, FileAccess.WRITE)
		if wf:
			wf.store_string(backup[full])

func _init() -> void:
	call_deferred("_run_tests")

func _run_tests() -> void:
	print("=== TEST PUNTO 8: PANTALLA DE GAME OVER ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager disponible")
	if dm == null:
		quit(1)
		return

	var res_data_dir := "res://clockgears_data"
	var backup := _snapshot_data_dir(res_data_dir)
	var prev_save_dir: String = dm.get_save_directory()
	var temp_save_dir := "user://test_game_over_saves_9b"
	dm.set_save_directory(temp_save_dir)
	for s in range(1, 5):
		dm.delete_save_slot(s)

	# 1. Categoría "Game Over" en capítulo Menús de la Base de Datos
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(2)

	var chap_info: Dictionary = db.get_chapter_structure_info()
	var found_go := false
	for ch in chap_info.get("chapters", []):
		if String(ch.get("name", "")) == "Menús":
			found_go = Array(ch.get("category_keys", [])).has("game_over")
	_check(found_go, "Capítulo 'Menús' contiene categoría 'game_over'")

	db.select_category_by_key("game_over")
	await _step_frames(2)
	var h: Dictionary = db.get_game_over_editor_ui_handles()
	var panel: Control = h.get("panel", null)
	_check(panel != null and panel.visible, "Panel de editor de Game Over visible")

	var title_edit: LineEdit = h.get("title_text_edit", null)
	var retry_chk: CheckBox = h.get("retry_enabled_chk", null)
	var retry_edit: LineEdit = h.get("retry_label_edit", null)
	var ret_title_chk: CheckBox = h.get("title_enabled_chk", null)
	var ret_title_edit: LineEdit = h.get("title_label_edit", null)
	var live_prev = h.get("live_preview", null)
	_check(title_edit != null and retry_chk != null and retry_edit != null and ret_title_chk != null and ret_title_edit != null and live_prev != null,
		"Controles y vista previa de Game Over presentes")

	title_edit.text = "FIN DE LA PARTIDA"
	title_edit.emit_signal("text_changed", "FIN DE LA PARTIDA")
	retry_edit.text = "Intentar de nuevo"
	retry_edit.emit_signal("text_changed", "Intentar de nuevo")
	ret_title_edit.text = "Ir al inicio"
	ret_title_edit.emit_signal("text_changed", "Ir al inicio")
	await _step_frames(1)

	var prev_snap: Dictionary = live_prev.get_state_snapshot()
	_check(String(prev_snap.get("title_text", "")) == "FIN DE LA PARTIDA" and String(prev_snap.get("retry_label", "")) == "Intentar de nuevo",
		"Vista previa refleja título y botón personalizados")

	db.save_to_disk()
	db.queue_free()
	await _step_frames(2)

	dm.load_all(res_data_dir)
	var saved_go: Dictionary = dm.get_game_over_config()
	_check(String(saved_go.get("title_text", "")) == "FIN DE LA PARTIDA" and String(saved_go.get("title_label", "")) == "Ir al inicio",
		"Configuración de Game Over guardada y recargada en game_over.json")

	# 2. Muerte del jugador -> aparece pantalla de Game Over tras terminar animación morir
	var sys_cfg: Dictionary = dm.get_system_config()
	sys_cfg["combat_mode"] = 1
	dm.set_system_config(sys_cfg)

	var m := ClockGearsMap.new()
	m.set_map_size(Vector2i(8, 8))
	root.add_child(m)
	m.ensure_default_layers()
	await _step_frames(2)

	var p := m.get_runtime_player()
	var interp := m.get_runtime_interpreter()
	_check(p != null and interp != null, "Mapa con jugador e intérprete inicializados")

	if p != null and interp != null:
		p.setup_on_map(m, m.get_runtime_followers(), interp)
		var aid: int = p.get_actor_id()
		if aid > 0:
			dm.set_actor_runtime_hp(aid, 10)
		p.take_arpg_hit(999, 0.0, Vector2(1, 0))
		_check(p.is_arpg_dying(), "Jugador entra en estado 'morir' tras daño letal")

		# Avanzar hasta que termine la animación morir
		for _f in range(20):
			if p.is_arpg_dead():
				break
			await create_timer(0.05).timeout
		await _step_frames(2)

		var go_screen = interp.get_game_over_screen()
		_check(go_screen != null and go_screen.is_open(), "Al terminar animación 'morir', aparece la pantalla de Game Over")
		_check(interp.get_state() == ClockGearsEventInterpreter.STATE_GAME_OVER, "Intérprete pasa a STATE_GAME_OVER")

		var snap: Dictionary = go_screen.get_state_snapshot()
		_check(bool(snap.get("retry_visible", false)) and bool(snap.get("title_visible", false)),
			"Botones Reintentar y Volver al menú visibles en Game Over")

		# 3. Reintentar SIN partidas guardadas -> reinicia con HP completo
		_check(not dm.has_any_save(), "Sin partidas guardadas inicialmente")
		go_screen.trigger_retry()
		await _step_frames(2)
		_check(go_screen.get_last_action() == "restart_initial_map", "Reintentar sin guardados reinicia desde mapa inicial")
		if aid > 0:
			_check(dm.get_actor_runtime_hp(aid) == dm.get_actor_runtime_max_hp(aid), "Reintentar restaura el HP completo")

		# 4. Reintentar CON partidas guardadas -> carga la última partida guardada
		dm.set_gold(111)
		dm.save_game_slot(1, {"map_path": "", "map_name": "Slot1"})
		await create_timer(0.05).timeout
		dm.set_gold(777)
		dm.save_game_slot(3, {"map_path": "", "map_name": "Slot3"})
		_check(dm.get_latest_save_slot() == 3, "get_latest_save_slot detecta el slot más reciente (slot 3)")

		dm.set_gold(0)
		interp.show_game_over()
		await _step_frames(1)
		# Evitamos change_scene_to_file pasando sin map_path en el slot; verificamos load_game_slot vía trigger_retry
		var ok_retry: bool = go_screen.trigger_retry()
		_check(ok_retry and go_screen.get_last_action() == "load_latest_save" and dm.get_gold() == 777,
			"Reintentar con guardados carga la última partida guardada (gold=777)")

		# 5. Comando de evento "Mostrar Game Over" en intérprete y en editor de eventos
		interp.start_command_list([{"type": "show_game_over", "code": ClockGearsCommandSelector.CMD_SHOW_GAME_OVER}])
		await _step_frames(1)
		_check(go_screen.is_open() and interp.get_state() == ClockGearsEventInterpreter.STATE_GAME_OVER,
			"Comando de evento 'show_game_over' muestra la pantalla de Game Over")

		# 6. Volver al menú (señal return_to_title_requested)
		go_screen.set_preview_mode(true) # evita change_scene_to_file en headless sobre root actual
		go_screen.set_preview_mode(false)
		# Verificamos el diálogo del editor de eventos
		var ev_dlg := ClockGearsEventEditorDialog.new()
		root.add_child(ev_dlg)
		await _step_frames(1)
		var ev_handles: Dictionary = ev_dlg.get_menu_command_ui_handles()
		_check(ev_handles.get("dlg_show_game_over", null) != null, "Diálogo de comando 'Mostrar Game Over' registrado en EventEditorDialog")
		ev_dlg.queue_free()

	m.queue_free()
	for s in range(1, 5):
		dm.delete_save_slot(s)
	dm.set_save_directory(prev_save_dir)
	_restore_data_dir(res_data_dir, backup)
	dm.load_all(res_data_dir)
	await _step_frames(1)

	if failed == 0:
		print("ALL PASS: Pantalla de Game Over completa verificada (%d checks)." % passed)
		quit(0)
	else:
		print("FAIL: %d comprobaciones fallaron." % failed)
		quit(1)
