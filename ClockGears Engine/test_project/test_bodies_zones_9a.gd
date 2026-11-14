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

func step_frames(frames: int, dt: float = 1.0 / 60.0) -> void:
	for _i in range(frames):
		await physics_frame

func _cleanup_dir_recursive(dir_res: String) -> void:
	var d := DirAccess.open(dir_res)
	if d == null:
		return
	d.list_dir_begin()
	var fn := d.get_next()
	while fn != "":
		if fn != "." and fn != "..":
			var sub := dir_res.path_join(fn)
			if d.current_is_dir():
				_cleanup_dir_recursive(sub)
			DirAccess.remove_absolute(ProjectSettings.globalize_path(sub))
		fn = d.get_next()
	d.list_dir_end()
	DirAccess.remove_absolute(ProjectSettings.globalize_path(dir_res))

func make_test_map(dm: ClockGearsDataManager, w: int = 20, h: int = 16) -> ClockGearsMap:
	var png_path := "res://tmp_t3b_tileset.png"
	if not FileAccess.file_exists(png_path):
		var timg := Image.create(96, 32, false, Image.FORMAT_RGBA8)
		timg.fill(Color(0.25, 0.55, 0.30, 1.0))
		timg.save_png(ProjectSettings.globalize_path(png_path))

	# Tileset con tile (0,0) pasable, (1,0) bloqueado X, (2,0) solo derecha/izquierda
	var ts_rec := {
		"id": 1,
		"name": "TestTS",
		"slots": {
			"A5": png_path
		},
		"slot_collisions": {
			"A5": {
				"0,0": {"passage": 15, "layer": 0, "mode": 0},
				"1,0": {"passage": 0, "layer": 0, "mode": 1},
				"2,0": {"passage": 6, "layer": 0, "mode": 0}
			}
		}
	}
	dm.set_category_array(13, [ts_rec]) # CAT_TILESETS = 13

	var map := ClockGearsMap.new()
	map.set_map_size(Vector2i(w, h))
	map.set_grid_resolution(32)
	map.set_tileset_id(1)
	map.ensure_default_layers()

	var ts := TileSet.new()
	ts.set_tile_size(Vector2i(32, 32))
	var img := Image.create(96, 32, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.25, 0.55, 0.30, 1.0))
	var tex := ImageTexture.create_from_image(img)
	var atlas := TileSetAtlasSource.new()
	atlas.set_texture(tex)
	atlas.set_texture_region_size(Vector2i(32, 32))
	for ax in range(3):
		atlas.create_tile(Vector2i(ax, 0))
	ts.add_source(atlas, 0)

	for ly in range(4):
		var layer: TileMapLayer = map.get_layer(ly)
		if layer:
			layer.set_tile_set(ts)

	var ground: TileMapLayer = map.get_layer(0)
	for y in range(h):
		for x in range(w):
			ground.set_cell(Vector2i(x, y), 0, Vector2i(0, 0))

	map.invalidate_passability_cache()
	return map

func _init() -> void:
	var wd := create_timer(60.0)
	wd.timeout.connect(func():
		printerr("[TIMEOUT] test_bodies_zones_9a excedio 60s.")
		quit(2)
	)
	call_deferred("_run_all")

func _run_all() -> void:
	print("=== TEST TANDA 3B (9A): CUERPOS, COLISIONES, MOVIMIENTO Y ZONAS ===")
	var dm = ClockGearsDataManager
	dm.init_empty()
	dm.reset_runtime_state()

	# ------------------------------------------------------------------
	# 1. MIGRACIÓN Y NORMALIZACIÓN DE DATOS
	# ------------------------------------------------------------------
	print("\n[1] Normalizacion de cuerpos, rutas, zonas y migracion ARPG...")
	# Registro antiguo sin detection_zones para verificar migracion desde vision_radius y attack_range
	var old_enemy_dict := {
		"id": 10,
		"name": "Goblin Viejo",
		"ai_type": "persigue_y_ataca",
		"vision_radius": 5.0,
		"attack_range": 1.5,
		"attack_interval": 0.8
	}
	var migrated_enemy := ClockGearsDataManager.get_normalized_record(20, old_enemy_dict) # CAT_ARPG_ENEMIES = 20
	var mig_zones: Array = migrated_enemy.get("detection_zones", [])
	check(mig_zones.size() == 2, "Migracion crea 2 zonas iniciales (Vision y Ataque)")
	check(mig_zones[0]["name"] == "Visión" and abs(float(mig_zones[0]["shape"]["radius"]) - 160.0) < 0.1 and mig_zones[0]["actions"][0]["type"] == "perseguir", "Zona 'Visión' migrada a radio 5*32=160px con accion perseguir")
	check(mig_zones[1]["name"] == "Ataque" and abs(float(mig_zones[1]["shape"]["radius"]) - 48.0) < 0.1 and mig_zones[1]["actions"][0]["type"] == "atacar", "Zona 'Ataque' migrada a radio 1.5*32=48px con accion atacar")

	# Guardado/carga round-trip en ruta temporal dentro de res:// sin ensuciar clockgears_data
	dm.set_category_array(20, [migrated_enemy])
	check(dm.save_all("res://tmp_cg_test_9a_t3b"), "save_all temporal exitoso")
	dm.init_empty()
	check(dm.load_all("res://tmp_cg_test_9a_t3b"), "load_all temporal exitoso")
	var loaded_e := dm.get_record_by_id(20, 10)
	check(loaded_e.get("detection_zones", []).size() == 2 and float(loaded_e.get("lose_sight_sec", 0.0)) == 4.0, "Round-trip JSON conserva zonas migradas y campos IA")

	var sys := dm.get_system_config()
	sys["combat_mode"] = 1
	sys["movement_directions"] = 8
	var actor_id: int = dm.create_record(0, { # CAT_ACTORS = 0
		"name": "Heroe",
		"body_shape": ClockGearsShapeGeometry.make_circle_shape(12.0, Vector2.ZERO),
		"body_through": false
	})
	sys["party"] = [actor_id]
	dm.set_system_config(sys)
	var actor_rec := dm.get_record_by_id(0, actor_id)
	check(actor_rec.has("body_shape") and actor_rec["body_shape"]["type"] == "circle", "Actor conserva body_shape circular")
	check(actor_rec.has("body_through") and actor_rec["body_through"] == false, "Actor normaliza body_through=false")

	# ------------------------------------------------------------------
	# 2. COLISIONES CON MUROS, ESQUINAS DIAGONALES, DESLIZAMIENTO Y CUERPOS
	# ------------------------------------------------------------------
	print("\n[2] Colisiones con tiles, diagonal modo 8, cuerpos, atravesable y sin empujones...")
	var map := make_test_map(dm, 20, 16)
	var ground: TileMapLayer = map.get_layer(0)
	# Pared horizontal en y=3 (x=3..7) y bloque diagonal en (10,3)
	for wx in range(3, 8):
		ground.set_cell(Vector2i(wx, 3), 0, Vector2i(1, 0))
	ground.set_cell(Vector2i(10, 3), 0, Vector2i(1, 0))
	map.invalidate_passability_cache()

	sys["player_start"] = {"x": 5, "y": 4}
	dm.set_system_config(sys)

	root.add_child(map)
	await step_frames(2)

	var player: ClockGearsPlayer = map.get_runtime_player()
	check(player != null, "RuntimePlayer instanciado")

	# Choque con muro arriba y deslizamiento diagonal
	player.set_grid_cell_immediate(Vector2i(5, 4), 8)
	var start_y := player.get_position().y
	var start_x := player.get_position().x
	# Mover en diagonal Arriba-Derecha contra pared superior en y=3: no debe subir a y=3, debe deslizar a la derecha
	player.move_with_collision(Vector2(10.0, -10.0))
	check(player.get_position().y >= start_y - 4.5, "Cuerpo circular no atraviesa pared superior X")
	check(player.get_position().x > start_x + 5.0, "Al chocar en diagonal contra pared desliza suavemente por X")

	# Regla diagonal modo 8: bloqueado (10,3), intentar pasar de (9,3) a (10,4)
	player.set_grid_cell_immediate(Vector2i(9, 3), 3)
	var can_diag_step := player.can_pass(Vector2i(9, 3), Vector2i(10, 4), 3)
	check(!can_diag_step, "Regla diagonal modo 8 impide cruzar esquina si (10,3) esta bloqueado")
	player.move_with_collision(Vector2(18.0, 18.0))
	check(player.get_grid_cell() != Vector2i(10, 4), "Movimiento con cuerpo tampoco roza esquina diagonal bloqueada")

	# Colision con formas rect, capsula y poligono + atravesable + sin empujones + legado intacto
	var npc_capsule_id: int = dm.create_record(19, { # CAT_NPCS = 19
		"name": "NPC Capsula",
		"npc_move_type": "quieto",
		"body_shape": ClockGearsShapeGeometry.make_capsule_shape(10.0, 26.0),
		"body_through": false
	})
	var ev_npc_cap := ClockGearsEvent.new()
	ev_npc_cap.set_event_id(1)
	ev_npc_cap.set_behavior_type(1)
	ev_npc_cap.set_behavior_ref_id(npc_capsule_id)
	map.add_clockgears_event(ev_npc_cap, Vector2(14 * 32, 6 * 32))
	ev_npc_cap.reset_spawn_origin()

	# Evento legado hoja estandar (behavior_type = 0)
	var ev_legacy := ClockGearsEvent.new()
	ev_legacy.set_event_id(2)
	ev_legacy.set_behavior_type(0)
	ev_legacy.set_priority_type(1)
	ev_legacy.set_through(false)
	map.add_clockgears_event(ev_legacy, Vector2(14 * 32, 8 * 32))
	await step_frames(2)

	check(ev_legacy.get_entity_body() == null and ev_legacy.get_zone_count() == 0, "Evento estandar legado (behavior_type=0) no crea cuerpo ni zonas nuevas")

	player.set_grid_cell_immediate(Vector2i(13, 6), 6)
	var npc_orig_pos := ev_npc_cap.get_position()
	for _k in range(10):
		player.move_with_collision(Vector2(6.0, 0.0))
	check(player.get_position().x < npc_orig_pos.x, "Cuerpo capsula de NPC bloquea al jugador")
	check(ev_npc_cap.get_position().distance_to(npc_orig_pos) < 0.01, "Cuerpo no es empujado por el jugador (sin empujones)")

	# Probar atravesable = true y forma poligono
	ev_npc_cap.set_body_through(true)
	for _k in range(10):
		player.move_with_collision(Vector2(6.0, 0.0))
	check(player.get_position().x > npc_orig_pos.x, "Con body_through=true el jugador atraviesa al NPC")

	ev_npc_cap.set_body_through(false)
	ev_npc_cap.set_body_shape(ClockGearsShapeGeometry.make_polygon_shape(PackedVector2Array([
		Vector2(-12, -12), Vector2(12, -12), Vector2(12, 12), Vector2(-12, 12)
	])))
	player.set_grid_cell_immediate(Vector2i(13, 6), 6)
	for _k in range(10):
		player.move_with_collision(Vector2(6.0, 0.0))
	check(player.get_position().x < npc_orig_pos.x, "Cuerpo poligono bloquea al jugador correctamente")
	ev_npc_cap.queue_free()
	ev_legacy.queue_free()
	await step_frames(2)

	# ------------------------------------------------------------------
	# 3. VAGAR, PATRULLA, A_DISTANCIA, SEGUIR Y PAUSA POR EVENTO
	# ------------------------------------------------------------------
	print("\n[3] Movimientos Vagar, Patrulla, A distancia, Seguir y Pausa por evento...")
	var wander_eid: int = dm.create_record(20, {
		"name": "Slime Vaga",
		"ai_type": "vagar",
		"wander_radius": 2.0,
		"walk_speed_pct": 140,
		"detection_zones": []
	})
	var ev_wander := ClockGearsEvent.new()
	ev_wander.set_event_id(10)
	ev_wander.set_behavior_type(3)
	ev_wander.set_behavior_ref_id(wander_eid)
	map.add_clockgears_event(ev_wander, Vector2(3 * 32, 10 * 32))
	ev_wander.reset_spawn_origin()

	var patrol_eid: int = dm.create_record(20, {
		"name": "Guardia Patrulla",
		"ai_type": "patrulla",
		"patrol_points": [{"x": 2.0, "y": 0.0}, {"x": 2.0, "y": 2.0}],
		"patrol_loop": false,
		"walk_speed_pct": 120,
		"detection_zones": []
	})
	var ev_patrol := ClockGearsEvent.new()
	ev_patrol.set_event_id(11)
	ev_patrol.set_behavior_type(3)
	ev_patrol.set_behavior_ref_id(patrol_eid)
	map.add_clockgears_event(ev_patrol, Vector2(10 * 32, 10 * 32))
	ev_patrol.reset_spawn_origin()

	# Mover jugador lejos
	player.set_grid_cell_immediate(Vector2i(1, 1), 2)
	await step_frames(90) # ~1.5s

	var w_dist: float = ev_wander.get_position().distance_to(ev_wander.get_spawn_origin())
	check(w_dist > 2.0 and w_dist <= 2.2 * 32.0, "Enemigo en Vagar se mueve y respeta radio desde origen (dist=" + str(snappedf(w_dist, 0.1)) + "px)")
	check(ev_patrol.get_position().distance_to(ev_patrol.get_spawn_origin()) > 20.0, "Enemigo en Patrulla recorre sus puntos relativos")

	ev_wander.queue_free()
	ev_patrol.queue_free()
	await step_frames(2)

	# Probar A distancia (min=2, max=4 tiles)
	var ranged_eid: int = dm.create_record(20, {
		"name": "Arquero",
		"ai_type": "a_distancia",
		"min_distance": 2.0,
		"max_distance": 4.0,
		"run_speed_pct": 150,
		"detection_zones": []
	})
	var ev_ranged := ClockGearsEvent.new()
	ev_ranged.set_event_id(12)
	ev_ranged.set_behavior_type(3)
	ev_ranged.set_behavior_ref_id(ranged_eid)
	map.add_clockgears_event(ev_ranged, Vector2(10 * 32, 8 * 32))
	ev_ranged.reset_spawn_origin()

	# Jugador muy cerca (1 tile = 32px < min 64px) -> debe alejarse
	player.set_grid_cell_immediate(Vector2i(9, 8), 6)
	await step_frames(40)
	var r_dist_after_near := ev_ranged.get_position().distance_to(player.get_position())
	check(r_dist_after_near >= 58.0, "Enemigo a_distancia retrocede si el jugador esta muy cerca (" + str(snappedf(r_dist_after_near, 0.1)) + "px)")
	ev_ranged.queue_free()
	await step_frames(2)

	# Probar NPC en Seguir + Pausa al ejecutar evento + Mirar al jugador
	var follow_npc_id: int = dm.create_record(21, { # CAT_ARPG_NPCS = 21
		"name": "Aliado",
		"npc_move_type": "seguir",
		"follow_distance": 1.5,
		"face_player_on_event": true,
		"event_pages": [{
			"trigger": 0, "priority": 1, "speed": 3, "through": false, "dir_fix": false,
			"commands": [{"type": "message", "text": "Hola!"}]
		}]
	})
	var ev_follow := ClockGearsEvent.new()
	ev_follow.set_event_id(13)
	ev_follow.set_behavior_type(2)
	ev_follow.set_behavior_ref_id(follow_npc_id)
	map.add_clockgears_event(ev_follow, Vector2(2 * 32, 12 * 32))
	ev_follow.reset_spawn_origin()

	player.set_grid_cell_immediate(Vector2i(8, 12), 4)
	var dist_before := ev_follow.get_position().distance_to(player.get_position())
	await step_frames(45)
	var dist_after := ev_follow.get_position().distance_to(player.get_position())
	check(dist_after < dist_before - 30.0, "NPC ARPG en modo 'seguir' se acerca al jugador")

	# Ejecutar su evento y comprobar pausa + mirar jugador
	var interp: ClockGearsEventInterpreter = map.get_runtime_interpreter()
	player.set_grid_cell_immediate(Vector2i(1, 12), 6)
	ev_follow.set_direction(2) # mirando abajo inicialmente
	interp.start_event(ev_follow)
	await step_frames(5)
	var pos_during_ev := ev_follow.get_position()
	check(ev_follow.is_paused_by_event(), "NPC se pausa mientras su evento esta ejecutandose")
	check(ev_follow.get_direction() == 4, "NPC gira mirando al jugador (izq=4) al iniciar evento")
	await step_frames(10)
	check(ev_follow.get_position().distance_to(pos_during_ev) < 0.01, "NPC detenido por evento no avanza")
	interp.advance_message_or_choice(0)
	await step_frames(2)
	interp.advance_message_or_choice(0)
	await step_frames(2)
	ev_follow.queue_free()
	await step_frames(2)

	# ------------------------------------------------------------------
	# 4. ZONAS DE DETECCIÓN (AREA2D ENTER/EXIT/WHILE_INSIDE + ACCIONES + A*)
	# ------------------------------------------------------------------
	print("\n[4] Zonas de deteccion (signals Area2D), acciones ordenadas, A* y retorno...")
	# Pared de piedra en x=7, y=5..10 dejando rodeo por arriba (y=4) para probar A*
	for oy in range(5, 11):
		ground.set_cell(Vector2i(7, oy), 0, Vector2i(1, 0))
	map.invalidate_passability_cache()

	var zone_enemy_id: int = dm.create_record(20, {
		"name": "Centinela Zona",
		"ai_type": "quieto",
		"avoid_obstacles": true,
		"path_recalc_ms": 100,
		"path_max_nodes": 256,
		"lose_sight_sec": 0.4,
		"walk_speed_pct": 100,
		"run_speed_pct": 150,
		"detection_zones": [
			{
				"name": "Alerta",
				"shape": ClockGearsShapeGeometry.make_circle_shape(96.0),
				"target_filter": "player",
				"trigger": "enter",
				"actions": [
					{"type": "activar_interruptor", "switch_name": "sw_enter", "value": true},
					{"type": "poner_variable", "var_name": "var_orden", "value": 10, "op": "="},
					{"type": "poner_variable", "var_name": "var_orden", "value": 7, "op": "+"},
					{"type": "perseguir"}
				]
			},
			{
				"name": "Salida",
				"shape": ClockGearsShapeGeometry.make_circle_shape(96.0),
				"target_filter": "player",
				"trigger": "exit",
				"actions": [
					{"type": "activar_interruptor", "switch_name": "sw_exit", "value": true}
				]
			},
			{
				"name": "Mele",
				"shape": ClockGearsShapeGeometry.make_circle_shape(80.0),
				"target_filter": "player",
				"trigger": "while_inside",
				"interval_sec": 0.2,
				"actions": [
					{"type": "poner_variable", "var_name": "var_inside_ticks", "value": 1, "op": "+"},
					{"type": "atacar"}
				]
			}
		]
	})

	var ev_sentinel := ClockGearsEvent.new()
	ev_sentinel.set_event_id(20)
	ev_sentinel.set_behavior_type(3)
	ev_sentinel.set_behavior_ref_id(zone_enemy_id)
	map.add_clockgears_event(ev_sentinel, Vector2(9 * 32, 7 * 32))
	ev_sentinel.reset_spawn_origin()

	# Colocar jugador fuera primero, luego entrar a la zona
	player.set_grid_cell_immediate(Vector2i(16, 7), 4)
	await step_frames(4)
	check(dm.get_switch("sw_enter") == false, "Antes de entrar en zona Area2D el interruptor sigue OFF")

	# Entrar en la zona (dentro de 80px de (9,7) -> en (11,7) son 64px)
	player.set_grid_cell_immediate(Vector2i(11, 7), 4)
	await step_frames(22) # ~0.36s -> enter + al menos 2 ticks de while_inside(0.2s)

	check(dm.get_switch("sw_enter") == true, "Area2D body_entered dispara accion activar_interruptor")
	check(dm.get_variable("var_orden") == 17, "Acciones ordenadas ponen variable (=10 luego +7 => 17)")
	check(ev_sentinel.get_ai_state_name() == "perseguir", "Accion perseguir cambia estado IA a perseguir")
	check(dm.get_variable("var_inside_ticks") >= 2, "Trigger while_inside se repite cada interval_sec (" + str(dm.get_variable("var_inside_ticks")) + ")")

	# Salir de la zona y mover al otro lado del muro (x=5, y=7) -> debe perder de vista o rodear
	# Primero comprobar exit dejando player lejos en (18, 2)
	player.set_grid_cell_immediate(Vector2i(18, 2), 6)
	await step_frames(6)
	check(dm.get_switch("sw_exit") == true, "Area2D body_exited dispara trigger exit")

	# Esperar lose_sight_sec (0.4s) para comprobar volver_puesto
	await step_frames(40)
	check(ev_sentinel.get_ai_state_name() == "volver_puesto" or ev_sentinel.get_position().distance_to(ev_sentinel.get_spawn_origin()) < 16.0, "Al perder de vista tras X s regresa hacia su puesto origen")

	# Prueba A*: poner enemigo en (9,7) persiguiendo jugador tras la pared en (5,7)
	ev_sentinel.set_position(Vector2(9 * 32, 7 * 32))
	ev_sentinel.reset_spawn_origin()
	player.set_grid_cell_immediate(Vector2i(5, 7), 6)
	ev_sentinel.execute_zone_actions([{"type": "perseguir"}], player)
	await step_frames(20)
	check(ev_sentinel.get_current_astar_path().size() > 2, "A* calcula ruta rodeando muro en x=7 (pasos=" + str(ev_sentinel.get_current_astar_path().size()) + ")")

	# Sin camino: encerrar completamente al jugador en (2, 2)
	for cx in range(1, 4):
		ground.set_cell(Vector2i(cx, 1), 0, Vector2i(1, 0))
		ground.set_cell(Vector2i(cx, 3), 0, Vector2i(1, 0))
	ground.set_cell(Vector2i(1, 2), 0, Vector2i(1, 0))
	ground.set_cell(Vector2i(3, 2), 0, Vector2i(1, 0))
	map.invalidate_passability_cache()
	player.set_grid_cell_immediate(Vector2i(2, 2), 2)
	ev_sentinel.execute_zone_actions([{"type": "perseguir"}], player)
	await step_frames(10)
	check(ev_sentinel.has_astar_no_path(), "Sin camino valido A* activa fallback sin atascarse")
	ev_sentinel.queue_free()
	await step_frames(2)

	# Limpiar recinto
	for cx in range(1, 4):
		ground.set_cell(Vector2i(cx, 1), 0, Vector2i(0, 0))
		ground.set_cell(Vector2i(cx, 3), 0, Vector2i(0, 0))
	ground.set_cell(Vector2i(1, 2), 0, Vector2i(0, 0))
	ground.set_cell(Vector2i(3, 2), 0, Vector2i(0, 0))
	map.invalidate_passability_cache()

	# ------------------------------------------------------------------
	# 5. DEPURACIÓN F3
	# ------------------------------------------------------------------
	print("\n[5] Opcion de depuracion F3 para cuerpos y zonas...")
	var dbg: ClockGearsDebugOverlay = map.get_runtime_debug_overlay()
	check(dbg != null, "ClockGearsDebugOverlay disponible en mapa runtime")
	if dbg:
		dbg.set_show_bodies_zones(true)
		await step_frames(2)
		var wc: ClockGearsDebugWorldCanvas = dbg.get_world_canvas()
		check(dbg.is_show_bodies_zones() and wc != null and wc.is_show_bodies_zones(), "Toggle F3 activa dibujado de cuerpos y zonas en WorldCanvas")
		dbg.set_show_bodies_zones(false)

	map.queue_free()
	await step_frames(2)

	# ------------------------------------------------------------------
	# 6. BENCHMARK DE RENDIMIENTO: 20 ENEMIGOS PERSIGUIENDO A LA VEZ (>60 FPS)
	# ------------------------------------------------------------------
	print("\n[6] Benchmark rendimiento: 20 enemigos persiguiendo con A* y zonas simultaneamente...")
	var perf_map := make_test_map(dm, 36, 26)
	var pg: TileMapLayer = perf_map.get_layer(0)
	for oy in range(6, 18):
		pg.set_cell(Vector2i(18, oy), 0, Vector2i(1, 0))
	perf_map.invalidate_passability_cache()

	var horde_eid: int = dm.create_record(20, {
		"name": "Horda ARPG",
		"ai_type": "persigue_y_ataca",
		"avoid_obstacles": true,
		"path_recalc_ms": 200,
		"path_max_nodes": 256,
		"vision_radius": 20.0,
		"attack_range": 1.0,
		"run_speed_pct": 120
	})

	sys["player_start"] = {"x": 8, "y": 12}
	dm.set_system_config(sys)
	root.add_child(perf_map)
	await step_frames(2)
	var p_perf: ClockGearsPlayer = perf_map.get_runtime_player()

	var enemies: Array = []
	for i in range(20):
		var ev := ClockGearsEvent.new()
		ev.set_event_id(100 + i)
		ev.set_behavior_type(3)
		ev.set_behavior_ref_id(horde_eid)
		var sx := 22 + (i % 5) * 2
		var sy := 5 + int(i / 5) * 3
		perf_map.add_clockgears_event(ev, Vector2(sx * 32, sy * 32))
		ev.reset_spawn_origin()
		enemies.append(ev)
	await step_frames(2)

	for ev in enemies:
		ev.execute_zone_actions([{"type": "perseguir"}], p_perf)

	var t0: int = Time.get_ticks_usec()
	const BENCH_FRAMES := 120
	await step_frames(BENCH_FRAMES)
	var elapsed_us: int = Time.get_ticks_usec() - t0
	var elapsed_sec: float = max(0.0001, float(elapsed_us) / 1000000.0)
	var sim_fps: float = float(BENCH_FRAMES) / elapsed_sec
	print("  [PERF] 20 enemigos x " + str(BENCH_FRAMES) + " frames = " + str(snappedf(elapsed_sec * 1000.0, 0.1)) + " ms totales -> " + str(snappedf(sim_fps, 0.1)) + " FPS")
	check(sim_fps >= 60.0, "20 enemigos persiguiendo con A* mantienen >= 60 FPS reales")

	perf_map.queue_free()
	await step_frames(2)

	_cleanup_dir_recursive("res://tmp_cg_test_9a_t3b")
	DirAccess.remove_absolute(ProjectSettings.globalize_path("res://tmp_t3b_tileset.png"))
	print("\n=== RESUMEN TANDA 3B: PASS=" + str(passed) + " | FAIL=" + str(failed) + " ===")
	quit(1 if failed > 0 else 0)
