extends SceneTree

var passed := 0
var failed := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		printerr("[FAIL] ", msg)

func _make_png(path: String, w: int, h: int, color_fn: Callable) -> String:
	var img := Image.create(w, h, false, Image.FORMAT_RGBA8)
	for y in range(h):
		for x in range(w):
			img.set_pixel(x, y, color_fn.call(x, y))
	var abs_p := ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs_p.get_base_dir())
	img.save_png(abs_p)
	return path

func _get_sprite_region_row(sprite: Sprite2D) -> int:
	if sprite == null:
		return -1
	if sprite.vframes > 1:
		return int(sprite.frame / max(1, sprite.hframes))
	return -1

func _press_keys(keys: Array, pressed: bool) -> void:
	for k in keys:
		var ev := InputEventKey.new()
		ev.keycode = k
		ev.physical_keycode = k
		ev.pressed = pressed
		Input.parse_input_event(ev)
	Input.flush_buffered_events()

func _initialize():
	_run_tests()

func _run_tests() -> void:
	var tmp_dir := "res://tmp_import_src/tanda1a_test"
	var sheet4_path := _make_png(tmp_dir + "/char4.png", 96, 128, func(x, y):
		var row := int(y / 32)
		var col := int(x / 32)
		return Color(0.15 + row * 0.2, 0.2 + col * 0.25, 0.85 - row * 0.15, 1.0)
	)
	var sheet8_path := _make_png(tmp_dir + "/char8.png", 96, 256, func(x, y):
		var row := int(y / 32)
		var col := int(x / 32)
		return Color(0.1 + row * 0.11, 0.8 - row * 0.08, 0.2 + col * 0.25, 1.0)
	)
	var slot_sheet_path := _make_png(tmp_dir + "/slot_strip.png", 128, 32, func(x, y):
		var col := int(x / 32)
		return Color(0.2 + col * 0.2, 0.9 - col * 0.15, 0.3, 1.0)
	)
	var tileset_png_path := _make_png(tmp_dir + "/tileset_a5.png", 64, 32, func(x, _y):
		return Color(0.2, 0.5, 0.2, 1.0) if x < 32 else Color(0.8, 0.2, 0.2, 1.0)
	)

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager disponible")
	if dm == null:
		quit(1)
		return

	dm.init_empty()
	var sys: Dictionary = dm.get_system_config()
	check(sys.has("movement_directions") and int(sys.get("movement_directions", -1)) == 4, "system_config inicia con movement_directions == 4")

	var ts_id: int = dm.create_record(13, {"name": "TS_Tanda1A"})
	var ts_rec: Dictionary = dm.get_record_by_id(13, ts_id)
	ts_rec["slots"] = {"A5": tileset_png_path}
	ts_rec["slot_collisions"] = {
		"A5": {
			"0,0": {"passage": 15, "mode": 0},
			"1,0": {"passage": 0, "mode": 1},
		}
	}
	dm.update_record(13, ts_id, ts_rec)

	var a_id: int = dm.create_record(0, {"name": "Heroe_4Row"})
	var a_rec: Dictionary = dm.get_record_by_id(0, a_id)
	a_rec["graphic_path"] = sheet4_path
	dm.update_record(0, a_id, a_rec)

	sys["party"] = [a_id]
	sys["start_x"] = 2
	sys["start_y"] = 2
	dm.set_system_config(sys)

	var map := ClockGearsMap.new()
	map.set_name("TestMap1A")
	map.set_map_size(Vector2i(15, 15))
	map.set_tileset_id(ts_id)
	map.ensure_default_layers()

	var ev_diag := ClockGearsEvent.new()
	ev_diag.set_event_name("EV_Diag")
	ev_diag.set_pages([{
		"trigger": 0, "priority": 1, "through": false, "dir_fix": false,
		"graphic_path": sheet8_path, "hframes": 3, "vframes": 8, "frame": 0,
		"move_type": 0, "speed": 3, "freq": 3,
		"commands": [{"type": "variable", "name": "interacted", "op": "=", "val": 100}]
	}])
	map.add_clockgears_event(ev_diag, Vector2(6 * 32, 6 * 32))

	var ev_horiz := ClockGearsEvent.new()
	ev_horiz.set_event_name("EV_Horiz")
	ev_horiz.set_pages([{
		"trigger": 0, "priority": 1, "through": false, "dir_fix": false,
		"graphic_path": sheet4_path, "hframes": 3, "vframes": 4, "frame": 0,
		"move_type": 0, "speed": 3, "freq": 3,
		"commands": [{"type": "variable", "name": "interacted", "op": "=", "val": 200}]
	}])
	map.add_clockgears_event(ev_horiz, Vector2(9 * 32, 8 * 32))

	var ev_vert := ClockGearsEvent.new()
	ev_vert.set_event_name("EV_Vert")
	ev_vert.set_pages([{
		"trigger": 0, "priority": 1, "through": false, "dir_fix": false,
		"graphic_path": "", "hframes": 1, "vframes": 1, "frame": 0,
		"move_type": 0, "speed": 3, "freq": 3,
		"commands": [{"type": "variable", "name": "interacted", "op": "=", "val": 300}]
	}])
	map.add_clockgears_event(ev_vert, Vector2(8 * 32, 9 * 32))

	var ev_vert_only := ClockGearsEvent.new()
	ev_vert_only.set_event_name("EV_VertOnly")
	ev_vert_only.set_pages([{
		"trigger": 0, "priority": 1, "through": false, "dir_fix": false,
		"graphic_path": "", "hframes": 1, "vframes": 1, "frame": 0,
		"move_type": 0, "speed": 3, "freq": 3,
		"commands": [{"type": "variable", "name": "interacted", "op": "=", "val": 400}]
	}])
	map.add_clockgears_event(ev_vert_only, Vector2(10 * 32, 11 * 32))

	get_root().add_child(map)
	map.init_runtime_session()
	var ground: TileMapLayer = map.get_layer(0)
	for y in range(15):
		for x in range(15):
			ground.set_cell(Vector2i(x, y), 0, Vector2i(0, 0))
	# Bloquear (4, 3) con X para probar bloqueo de cardinal contigua al ir en diagonal desde (3, 3) a (4, 4)
	ground.set_cell(Vector2i(4, 3), 0, Vector2i(1, 0))
	await physics_frame

	var player: ClockGearsPlayer = map.get_runtime_player()
	var interp: ClockGearsEventInterpreter = map.get_runtime_interpreter()

	# 1. REGRESIÓN MODO 4:
	check(player.is_allow_diagonal() == false, "Modo 4: player.is_allow_diagonal() es false por defecto")
	player.set_grid_cell_immediate(Vector2i(2, 2), 2)
	check(player.try_step(6) == true and player.get_grid_cell() == Vector2i(3, 2), "Modo 4: paso cardinal derecha funciona")
	while player.is_moving_step():
		await physics_frame

	# Dos teclas simultáneas en modo 4 -> NO diagonal
	_press_keys([KEY_DOWN, KEY_RIGHT], true)
	await physics_frame
	_press_keys([KEY_DOWN, KEY_RIGHT], false)
	check(player.get_grid_cell() != Vector2i(4, 3) and player.get_direction() == 2, "Modo 4: dos teclas simultáneas no mueven en diagonal (mantiene cardinal)")
	while player.is_moving_step():
		await physics_frame

	# 2. MODO 8: dos teclas simultáneas -> diagonal y velocidad real == cardinal
	player.set_allow_diagonal(true)
	player.set_grid_cell_immediate(Vector2i(1, 5), 2)

	player.try_step(6)
	var card_frames := 0
	var card_start := player.get_position()
	while player.is_moving_step() and card_frames < 120:
		card_frames += 1
		await physics_frame
	var card_dist := card_start.distance_to(player.get_position())
	var card_speed := card_dist / (maxf(1.0, float(card_frames)) / 60.0)

	var diag_start := player.get_position()
	var diag_frames := 0
	_press_keys([KEY_DOWN, KEY_RIGHT], true)
	await physics_frame
	var started_diag := (player.get_grid_cell() == Vector2i(3, 6) and player.get_direction() == 3)
	_press_keys([KEY_DOWN, KEY_RIGHT], false)
	while player.is_moving_step() and diag_frames < 120:
		diag_frames += 1
		await physics_frame
	var diag_dist := diag_start.distance_to(player.get_position())
	var diag_speed := diag_dist / (maxf(1.0, float(diag_frames)) / 60.0)
	var time_ratio := float(diag_frames) / maxf(1.0, float(card_frames))

	check(started_diag, "Modo 8: pulsar DOWN+RIGHT mueve 1 casilla diagonal (2,5)->(3,6) y mira dir=3")
	print("   Velocidad cardinal=%.2f px/s (%d frames), diagonal=%.2f px/s (%d frames), ratio=%.3f" % [card_speed, card_frames, diag_speed, diag_frames, time_ratio])
	check(absf(time_ratio - sqrt(2.0)) <= 0.10 and absf(diag_speed - card_speed) <= card_speed * 0.10, "Modo 8: velocidad normalizada diagonal tarda sqrt(2)x y mantiene misma velocidad real")

	# 3. BLOQUEO DE ESQUINA
	player.set_grid_cell_immediate(Vector2i(3, 3), 2)
	var blocked_diag := player.try_step(3)
	check(blocked_diag == false and player.get_grid_cell() == Vector2i(3, 3), "Modo 8: diagonal bloqueada por cardinal contigua X (4,3) no mueve ni desliza")
	check(player.get_direction() == 3, "Modo 8: diagonal bloqueada SÍ cambia la mirada a esa diagonal (dir=3)")

	# 4. HOJA DE 4 FILAS Y 8 FILAS
	var pspr: Sprite2D = player.get_child(0) as Sprite2D
	player.set_direction(7)
	var row7 := _get_sprite_region_row(pspr)
	player.set_direction(3)
	var row3 := _get_sprite_region_row(pspr)
	check(row7 == 1, "Hoja 4 filas: mirada Arriba-Izq (7) usa pose horizontal Izquierda (fila 1, obtuvo %d)" % row7)
	check(row3 == 2, "Hoja 4 filas: mirada Abajo-Der (3) usa pose horizontal Derecha (fila 2, obtuvo %d)" % row3)

	ev_diag.set_direction(9)
	var ev_spr: Sprite2D = ev_diag.get_child(0) as Sprite2D
	var ev_row9 := _get_sprite_region_row(ev_spr)
	check(ev_diag.get_direction() == 9 and ev_row9 == 7, "Evento hoja 8 filas: dir=9 (Arriba-Der) muestra fila diagonal 7 (obtuvo %d)" % ev_row9)

	# 5. CASILLA DE ENFRENTE Y FALLBACK EN INTERACCIÓN DIAGONAL
	check(player.has_method("get_front_cell"), "ClockGearsPlayer expone función única 'get_front_cell'")
	player.set_grid_cell_immediate(Vector2i(5, 5), 3)
	if player.has_method("get_front_cell"):
		check(player.get_front_cell() == Vector2i(6, 6), "get_front_cell() en (5,5) mirando dir 3 devuelve casilla diagonal (6,6)")
	dm.set_variable("interacted", 0)
	var act1 := player.try_activate_front_event()
	interp.step_frame()
	interp.step_frame()
	check(act1 and dm.get_variable("interacted") == 100, "Interacción diagonal activa evento en casilla diagonal (6,6)")

	player.set_grid_cell_immediate(Vector2i(8, 8), 3)
	player.unlock_action_key()
	dm.set_variable("interacted", 0)
	var act2 := player.try_activate_front_event()
	interp.step_frame()
	interp.step_frame()
	check(act2 and dm.get_variable("interacted") == 200, "Interacción diagonal sin evento en diagonal prueba cardinal horizontal primero (9,8)")

	player.set_grid_cell_immediate(Vector2i(10, 10), 1)
	player.unlock_action_key()
	dm.set_variable("interacted", 0)
	var act3 := player.try_activate_front_event()
	interp.step_frame()
	interp.step_frame()
	check(act3 and dm.get_variable("interacted") == 400, "Interacción diagonal sin evento en diagonal/horiz prueba cardinal vertical (10,11)")

	# 6. RANURAS, CORRER, MARCADOR "?" Y REGISTRO DE ESTADOS EXTENSIBLE
	check(ClassDB.class_exists("ClockGearsCharacterStateRegistry"), "Existe registro extensible ClockGearsCharacterStateRegistry")
	if ClassDB.class_exists("ClockGearsCharacterStateRegistry"):
		var reg = Engine.get_singleton("ClockGearsCharacterStateRegistry")
		check(reg != null and reg.has_state("quieto") and reg.has_state("caminar") and reg.has_state("correr"), "Estados quieto, caminar y correr están registrados")
		if reg != null:
			reg.register_state("nadar", "nadar", false, "caminar", false, true)
			check(reg.has_state("nadar") and reg.has_state("quieto") and reg.has_state("caminar") and reg.has_state("correr"), "Registrar nuevo estado ('nadar') funciona sin alterar los existentes")

	# Actor completamente sin gráfico ni ranuras -> debe mostrar marcador '?' y nunca null/cuadro blanco
	var empty_actor_id: int = dm.create_record(0, {"name": "Actor_SinGrafico", "graphic_path": ""})
	player.load_actor_data(empty_actor_id)
	player.set_grid_cell_immediate(Vector2i(1, 1), 2)
	check(pspr.texture != null and pspr.texture.get_width() == 32 and pspr.texture.get_height() == 32, "Actor sin gráfico muestra textura marcador '?' 32x32 (nunca vacía/blanca)")

	var slot_actor_id: int = dm.create_record(0, {
		"name": "Actor_Slots",
		"graphic_path": "",
		"directional_idle": true,
		"sprite_data": {
			"frame_w": 32, "frame_h": 32, "cols": 4, "rows": 1,
			"animations": {
				"mover_izquierda": {
					"name": "mover_izquierda", "spritesheet_path": slot_sheet_path,
					"fps": 8.0, "loop": true, "frames": [0, 1],
					"frames_data": [
						{"sheet_path": slot_sheet_path, "region": Rect2i(0, 0, 32, 32), "duration": 0.125},
						{"sheet_path": slot_sheet_path, "region": Rect2i(32, 0, 32, 32), "duration": 0.125}
					]
				},
				"quieto_abajo": {
					"name": "quieto_abajo", "spritesheet_path": slot_sheet_path,
					"fps": 4.0, "loop": true, "frames": [2, 3],
					"frames_data": [
						{"sheet_path": slot_sheet_path, "region": Rect2i(64, 0, 32, 32), "duration": 0.25},
						{"sheet_path": slot_sheet_path, "region": Rect2i(96, 0, 32, 32), "duration": 0.25}
					]
				},
				"mover_arriba_derecha": {
					"name": "mover_arriba_derecha", "spritesheet_path": slot_sheet_path,
					"fps": 10.0, "loop": true, "frames": [1]
				}
			}
		}
	})
	player.load_actor_data(slot_actor_id)
	player.set_grid_cell_immediate(Vector2i(1, 1), 7)
	check(player.has_method("get_active_visual_slot"), "ClockGearsPlayer expone get_active_visual_slot()")
	if player.has_method("get_active_visual_slot"):
		check(player.get_active_visual_slot() == "mover_izquierda" and player.is_using_freeze_frame0() == true,
			"Ranura diagonal quieto vacía usa frame 0 de mover_izquierda (obtuvo %s freeze=%s)" % [player.get_active_visual_slot(), str(player.is_using_freeze_frame0())])
		player.set_direction(2)
		check(player.get_active_visual_slot() == "quieto_abajo" and player.is_using_freeze_frame0() == false,
			"Idle direccional activado usa quieto_abajo en bucle (obtuvo %s)" % player.get_active_visual_slot())
		var slot_rec: Dictionary = dm.get_record_by_id(0, slot_actor_id)
		slot_rec["directional_idle"] = false
		dm.update_record(0, slot_actor_id, slot_rec)
		player.load_actor_data(slot_actor_id)
		player.set_grid_cell_immediate(Vector2i(1, 1), 7)
		player.try_step(7)
		check(player.get_active_visual_slot() == "mover_izquierda", "Caminar en diagonal vacía (7) usa ranura horizontal mover_izquierda")
		while player.is_moving_step():
			await physics_frame
		player.set_grid_cell_immediate(Vector2i(2, 2), 7)
		player.try_step(7, true)
		check(player.get_action_state() == "correr" and player.get_active_visual_slot() == "mover_izquierda", "Correr en diagonal sin ranura correr_* cae a mover_izquierda")
		while player.is_moving_step():
			await physics_frame

	# 7. DB NO DESTRUCTIVO Y GUARDAR / REABRIR
	var save_dir := tmp_dir + "/saved_project"
	sys["movement_directions"] = 8
	dm.set_system_config(sys)
	dm.save_all(save_dir)

	var db := ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	db.open_database()
	check(db.has_method("set_system_movement_directions") and db.has_method("get_system_movement_directions"), "DB expone set/get_system_movement_directions")
	if db.has_method("set_system_movement_directions"):
		db.select_category(15)
		db.set_system_movement_directions(8)
		db.select_category(0)
		db.set_actor_directional_idle(true)
		db.set_system_movement_directions(4)
		db.set_system_movement_directions(8)
		check(db.get_system_movement_directions() == 8 and db.is_actor_directional_idle() == true, "DB alterna 4<->8 e Idle direccional sin error")
		db.hide()
		db.queue_free()

	dm.init_empty()
	var loaded_ok: bool = bool(dm.load_all(save_dir))
	check(loaded_ok, "dm.load_all recarga proyecto guardado")
	check(int(dm.get_system_config().get("movement_directions", -1)) == 8, "Al reabrir se conserva movement_directions = 8")
	var rec_reopened: Dictionary = dm.get_record_by_id(0, slot_actor_id)
	var anims_reopened: Dictionary = rec_reopened.get("sprite_data", {}).get("animations", {})
	check(anims_reopened.has("mover_arriba_derecha") and anims_reopened["mover_arriba_derecha"].get("fps", 0) == 10.0 and anims_reopened.has("quieto_abajo"), "Al reabrir se conservan ranuras diagonales e idle tras cambiar 8->4->8")

	# Compatibilidad proyecto viejo sin clave movement_directions -> carga como 4
	var old_dir := tmp_dir + "/legacy_project"
	var abs_old_sys := ProjectSettings.globalize_path(old_dir + "/system.json")
	DirAccess.make_dir_recursive_absolute(abs_old_sys.get_base_dir())
	var f_old := FileAccess.open(abs_old_sys, FileAccess.WRITE)
	if f_old != null:
		f_old.store_string("{\"game_title\":\"Legacy\",\"currency\":\"G\",\"combat_mode\":0}")
		f_old.close()
	dm.init_empty()
	dm.load_all(old_dir)
	check(int(dm.get_system_config().get("movement_directions", -1)) == 4, "Proyecto existente sin movement_directions carga como 4")

	map.queue_free()
	await process_frame
	await process_frame
	print("==================================================")
	print("   TEST TANDA 1A RESULTADO: %d PASSED / %d FAILED" % [passed, failed])
	print("==================================================")
	quit(0 if failed == 0 else 1)
