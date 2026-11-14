extends SceneTree

var passed: int = 0
var failed: int = 0

func assert_true(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		printerr("[FAIL] ", msg)

func assert_eq(a, b, msg: String) -> void:
	if a == b:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		printerr("[FAIL] ", msg, " (got=", a, ", expected=", b, ")")

func backup_data_dir(dir_res: String) -> Dictionary:
	var snap := {}
	var d := DirAccess.open(dir_res)
	if d == null:
		return snap
	d.list_dir_begin()
	var fn := d.get_next()
	while fn != "":
		if not d.current_is_dir() and (fn.ends_with(".json") or fn.ends_with(".bak")):
			var fa := FileAccess.open(dir_res.path_join(fn), FileAccess.READ)
			if fa:
				snap[fn] = fa.get_as_text()
				fa.close()
		fn = d.get_next()
	d.list_dir_end()
	return snap

func restore_data_dir(dir_res: String, snap: Dictionary) -> void:
	var d := DirAccess.open(dir_res)
	if d:
		d.list_dir_begin()
		var fn := d.get_next()
		while fn != "":
			if not d.current_is_dir() and (fn.ends_with(".json") or fn.ends_with(".bak")) and not snap.has(fn):
				d.remove(fn)
			fn = d.get_next()
		d.list_dir_end()
	for fn in snap.keys():
		var fa := FileAccess.open(dir_res.path_join(fn), FileAccess.WRITE)
		if fa:
			fa.store_string(snap[fn])
			fa.close()

func make_test_png(path: String, w: int = 128, h: int = 32, col: Color = Color(0.3, 0.7, 0.9, 1.0)) -> void:
	var img := Image.create_empty(w, h, false, Image.FORMAT_RGBA8)
	img.fill(col)
	var abs_p := ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs_p.get_base_dir())
	img.save_png(abs_p)

func make_slot_dict(sheet_path: String, count: int, fps: float = 4.0, is_loop: bool = true) -> Dictionary:
	var frames := []
	var frames_data := []
	for i in range(count):
		frames.append(i)
		frames_data.append({
			"sheet": sheet_path,
			"x": (i % 4) * 32,
			"y": 0,
			"w": 32,
			"h": 32,
			"duration": 1.0 / fps
		})
	return {
		"spritesheet_path": sheet_path,
		"cols": 4,
		"rows": 1,
		"frame_w": 32,
		"frame_h": 32,
		"fps": fps,
		"loop": is_loop,
		"frames": frames,
		"frames_data": frames_data
	}

func _initialize() -> void:
	# Watchdog para garantizar que nunca se cuelgue en headless
	var wd := create_timer(45.0)
	wd.timeout.connect(func():
		printerr("[TIMEOUT] test_arpg_core_3a excedió 45s.")
		quit(2)
	)
	_run_tests()

func _run_tests() -> void:
	var data_dir := "res://clockgears_data"
	var snap := backup_data_dir(data_dir)
	var tmp_dir := "res://tmp_arpg_3a"
	var sheet_path := tmp_dir.path_join("hero_atk.png")
	make_test_png(sheet_path, 128, 32)

	var dm = ClockGearsDataManager
	dm.init_empty()

	# =========================================================================
	# 1. Enemigos ARPG independientes de Enemigos por turnos + Persistencia
	# =========================================================================
	# Enemigo por turnos distinto (debe permanecer aislado)
	var turn_id: int = dm.create_record(7, { # CAT_ENEMIES = 7
		"name": "Slime Turnos",
		"hp": 999,
		"mp": 55,
		"agi": 44,
		"atk": 77
	})
	assert_true(turn_id >= 1, "1.1 Creado enemigo por turnos #%d" % turn_id)

	var ae1_id: int = dm.create_record(20, { # CAT_ARPG_ENEMIES = 20
		"name": "Lobo ARPG"
	})
	var ae2_id: int = dm.create_record(20, {
		"name": "Golem ARPG",
		"level": 5,
		"exp": 80,
		"gold": 40,
		"hp": 450,
		"mp": 99, # Debe eliminarse al normalizar
		"agi": 88, # Debe eliminarse al normalizar
		"atk": 38,
		"def": 25,
		"mat": 8,
		"mdf": 18,
		"luk": 9,
		"ai_type": "a distancia",
		"vision_radius": 9.0,
		"attack_range": 4.5,
		"attack_interval": 2.2,
		"knockback_resist": 75,
		"walk_speed_pct": 80,
		"run_speed_pct": 120,
		"attack_speed_pct": 150,
		"drops": [
			{"kind": 1, "item_id": 2, "prob": 50},
			{"kind": 0, "item_id": 0, "prob": 100}
		],
		"traits": [
			{"code": 11, "data_id": 0, "element_key": "fisico", "value": 50.0},
			{"code": 11, "data_id": 2, "element_key": "fuego", "value": 200.0}
		]
	})

	var ae1: Dictionary = dm.get_record_by_id(20, ae1_id)
	var ae2: Dictionary = dm.get_record_by_id(20, ae2_id)

	assert_true(not ae1.has("mp") and not ae1.has("agi"), "1.2 Enemigo ARPG #1 no tiene MP ni AGI")
	assert_true(not ae2.has("mp") and not ae2.has("agi"), "1.3 Enemigo ARPG #2 purga MP y AGI")
	assert_eq(int(ae1.get("level", -1)), 1, "1.4 Default ARPG Enemy level=1")
	assert_eq(int(ae1.get("exp", -1)), 10, "1.5 Default ARPG Enemy exp=10")
	assert_eq(int(ae1.get("gold", -1)), 5, "1.6 Default ARPG Enemy gold=5")
	assert_eq(int(ae1.get("hp", -1)), 100, "1.7 Default ARPG Enemy hp=100")
	assert_eq(str(ae1.get("ai_type", "")), "persigue_y_ataca", "1.8 Default ai_type='persigue_y_ataca'")
	assert_eq(int(ae1.get("walk_speed_pct", 0)), 100, "1.9 Default walk_speed_pct=100")
	assert_eq(int(ae1.get("run_speed_pct", 0)), 100, "1.10 Default run_speed_pct=100")
	assert_eq(int(ae1.get("attack_speed_pct", 0)), 100, "1.11 Default attack_speed_pct=100")
	assert_eq((ae1.get("drops", []) as Array).size(), 3, "1.12 Enemigo ARPG #1 tiene 3 drops por defecto")

	# Verificar fisico + 10 elementos en traits
	var traits1: Array = ae1.get("traits", [])
	var elem_keys_1 := {}
	for tr in traits1:
		if tr is Dictionary and int(tr.get("code", 0)) == 11:
			elem_keys_1[str(tr.get("element_key", ""))] = float(tr.get("value", -1.0))
	assert_true(elem_keys_1.has("fisico") and elem_keys_1.size() >= 11,
		"1.13 Enemigo ARPG #1 incluye fisico + 10 elementos en traits (count=%d)" % elem_keys_1.size())
	assert_eq(elem_keys_1.get("fisico", -1.0), 100.0, "1.14 Tasa fisico por defecto = 100.0")
	assert_eq(elem_keys_1.get("fuego", -1.0), 100.0, "1.15 Tasa fuego por defecto = 100.0")

	# Verificar valores personalizados de #2 y aislamiento vs #1
	assert_eq(str(ae2.get("ai_type", "")), "a_distancia", "1.16 Enemigo ARPG #2 normaliza ai_type 'a_distancia'")
	var traits2: Array = ae2.get("traits", [])
	var elem_keys_2 := {}
	for tr in traits2:
		if tr is Dictionary and int(tr.get("code", 0)) == 11:
			elem_keys_2[str(tr.get("element_key", ""))] = float(tr.get("value", -1.0))
	assert_eq(elem_keys_2.get("fisico", -1.0), 50.0, "1.17 Enemigo ARPG #2 conserva fisico=50.0")
	assert_eq(elem_keys_2.get("fuego", -1.0), 200.0, "1.18 Enemigo ARPG #2 conserva fuego=200.0")

	# Mutar drops/traits de ae1 en copia local + update_record NO debe afectar ae2 ni turnos
	ae1["atk"] = 33
	ae1["drops"][0]["item_id"] = 9
	ae1["drops"][0]["prob"] = 25
	dm.update_record(20, ae1_id, ae1)
	dm.save_all(data_dir)
	dm.init_empty()
	dm.load_all(data_dir)

	var r_ae1: Dictionary = dm.get_record_by_id(20, ae1_id)
	var r_ae2: Dictionary = dm.get_record_by_id(20, ae2_id)
	var r_turn: Dictionary = dm.get_record_by_id(7, turn_id)
	assert_eq(int(r_ae1.get("atk", 0)), 33, "1.19 Persiste atk=33 tras guardar y recargar")
	assert_eq(int(r_ae1["drops"][0].get("item_id", 0)), 9, "1.20 Persiste drop #1 de Enemigo ARPG #1")
	assert_eq(int(r_ae2["drops"][0].get("item_id", 0)), 2, "1.21 Drop #1 de Enemigo ARPG #2 intacto (sin sangrado)")
	assert_eq(int(r_ae2.get("knockback_resist", -1)), 75, "1.22 knockback_resist=75 persistido en #2")
	assert_eq(int(r_turn.get("mp", -1)), 55, "1.23 Enemigo por turnos mantiene MP=55 sin vínculo con ARPG")
	assert_eq(int(r_turn.get("agi", -1)), 44, "1.24 Enemigo por turnos mantiene AGI=44 sin vínculo con ARPG")

	# =========================================================================
	# 2. Migración de entradas existentes (sin perder nombre, animaciones ni evento)
	# =========================================================================
	var legacy_anim := make_slot_dict(sheet_path, 4, 4.0, true)
	var legacy_enemy := {
		"id": 1,
		"name": "Viejo Esqueleto ARPG",
		"desc": "Legacy desc",
		"graphic_path": sheet_path,
		"directional_idle": true,
		"sprite_data": {"animations": {"mover_abajo": legacy_anim, "atacar_abajo": legacy_anim}},
		"animations": {"mover_abajo": legacy_anim, "atacar_abajo": legacy_anim},
		"event_pages": [{"trigger": 0, "commands": [{"type": "message", "text": "Hola"}]}]
	}
	var legacy_actor := {
		"id": 1,
		"name": "Kris",
		"hp": 320,
		"mp": 80,
		"agi": 22,
		"sprite_data": {"animations": {"atacar_abajo": make_slot_dict(sheet_path, 4, 4.0, false)}}
	}
	var legacy_npc := {
		"id": 1,
		"name": "Aldeano Viejo",
		"sprite_data": {"animations": {"mover_abajo": legacy_anim}},
		"event_pages": [{"trigger": 0, "commands": [{"type": "message", "text": "NPC ok"}]}]
	}
	var legacy_arpg_npc := {
		"id": 1,
		"name": "Guardia ARPG",
		"sprite_data": {"animations": {"atacar_abajo": legacy_anim}},
		"event_pages": [{"trigger": 1, "commands": []}]
	}

	var write_raw := func(fn: String, payload) -> void:
		var fa := FileAccess.open(data_dir.path_join(fn), FileAccess.WRITE)
		fa.store_string(JSON.stringify(payload, "\t"))
		fa.close()

	write_raw.call("arpg_enemies.json", [legacy_enemy])
	write_raw.call("actors.json", [legacy_actor])
	write_raw.call("npcs.json", [legacy_npc])
	write_raw.call("arpg_npcs.json", [legacy_arpg_npc])
	write_raw.call("database.json", {"engine": "ClockGears Engine", "schema_version": 2})

	dm.init_empty()
	dm.load_all(data_dir)

	var mig_enemy: Dictionary = dm.get_record_by_id(20, 1)
	var mig_actor: Dictionary = dm.get_record_by_id(0, 1)
	var mig_npc: Dictionary = dm.get_record_by_id(19, 1)
	var mig_arpg_npc: Dictionary = dm.get_record_by_id(21, 1)

	assert_eq(str(mig_enemy.get("name", "")), "Viejo Esqueleto ARPG", "2.1 Migración conserva nombre de enemigo ARPG")
	assert_true(mig_enemy.get("sprite_data", {}).get("animations", {}).has("mover_abajo"), "2.2 Migración conserva animaciones de enemigo ARPG")
	assert_eq((mig_enemy.get("event_pages", []) as Array).size(), 1, "2.3 Migración conserva event_pages de enemigo ARPG")
	assert_eq(int(mig_enemy.get("hp", 0)), 100, "2.4 Migración rellena stats default en enemigo ARPG")
	assert_eq(int(mig_enemy.get("walk_speed_pct", 0)), 100, "2.5 Migración asigna velocidades 100% en enemigo ARPG")
	assert_eq(int(mig_actor.get("mp", 0)), 80, "2.6 Migración Actor conserva MP de turnos")
	assert_eq(int(mig_actor.get("walk_speed_pct", 0)), 100, "2.7 Actor migra walk=100%")
	assert_eq(int(mig_actor.get("run_speed_pct", 0)), 100, "2.8 Actor migra run=100%")
	assert_eq(int(mig_actor.get("attack_speed_pct", 0)), 100, "2.9 Actor migra attack=100%")
	assert_eq(int(mig_npc.get("walk_speed_pct", 0)), 100, "2.10 NPC migra walk=100%")
	assert_eq(int(mig_npc.get("run_speed_pct", 0)), 100, "2.11 NPC migra run=100%")
	assert_true(not mig_npc.has("attack_speed_pct"), "2.12 NPC normal NO añade attack_speed_pct")
	assert_eq(int(mig_arpg_npc.get("attack_speed_pct", 0)), 100, "2.13 NPC ARPG migra attack=100%")

	# =========================================================================
	# 3. Database Dialog: aislamiento en creación / selección / edición (sin MP/AGI en ARPG Enemy)
	# =========================================================================
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await process_frame
	db.open_database()
	await process_frame

	# Poner modo ARPG (1) para mostrar capítulo Action RPG
	var s_cfg: Dictionary = dm.get_system_config()
	s_cfg["combat_mode"] = 1
	dm.set_system_config(s_cfg)
	db.load_from_data_manager() if db.has_method("load_from_data_manager") else null
	db.select_category_by_key("arpg_enemies")
	await process_frame
	# Añadir un segundo enemigo desde botón '+' si existe o simular creación
	var add_btn: Button = null
	for b in db.find_children("*", "Button", true, false):
		if b is Button and b.text == "+":
			add_btn = b
			break
	if add_btn:
		add_btn.emit_signal("pressed")
		await process_frame
	# Aplicar/Guardar
	for b in db.find_children("*", "Button", true, false):
		if b is Button and b.text == "Aplicar":
			b.emit_signal("pressed")
			await process_frame
			break
	var db_ae_list: Array = dm.get_records("arpg_enemies")
	assert_true(db_ae_list.size() >= 2, "3.1 Database añadió Enemigo ARPG independiente (total=%d)" % db_ae_list.size())
	if db_ae_list.size() >= 2:
		var second_rec: Dictionary = db_ae_list[1]
		assert_true(not second_rec.has("mp") and not second_rec.has("agi"), "3.2 Nuevo Enemigo ARPG en UI no contiene MP ni AGI")
		assert_eq(int(second_rec.get("hp", 0)), 100, "3.3 Nuevo Enemigo ARPG en UI conserva hp default 100")

	db.queue_free()
	await process_frame

	# =========================================================================
	# 4. Aplicación de velocidades en Runtime (Caminar, Correr, Atacar)
	# =========================================================================
	# Actor con animacion atacar_abajo de 4 frames a 4 FPS => 1.00s exactos al 100%
	var atk_actor := {
		"id": 1,
		"name": "VelTest",
		"walk_speed_pct": 100,
		"run_speed_pct": 100,
		"attack_speed_pct": 100,
		"sprite_data": {
			"animations": {
				"quieto_abajo": make_slot_dict(sheet_path, 1, 4.0, true),
				"mover_abajo": make_slot_dict(sheet_path, 4, 4.0, true),
				"atacar_abajo": make_slot_dict(sheet_path, 4, 4.0, false)
			}
		}
	}
	dm.set_records("actors", [atk_actor])
	var sys := dm.get_system_config()
	sys["combat_mode"] = 1 # ARPG
	dm.set_system_config(sys)

	var map = ClassDB.instantiate("ClockGearsMap")
	map.set("map_width", 20)
	map.set("map_height", 10)
	root.add_child(map)
	await process_frame

	var player = map.get_node_or_null("RuntimePlayer")
	assert_true(player != null, "4.0 Map crea RuntimePlayer")
	player.load_actor_data(1)
	player.set_grid_cell_immediate(Vector2i(2, 2), 2)

	assert_eq(player.get_walk_speed(), 128.0, "4.1 Base walk_speed 100% = 128.0 px/s")
	assert_eq(player.get_dash_speed(), 256.0, "4.2 Base dash_speed 100% = 256.0 px/s")

	var measure_attack_duration := func(actor_data: Dictionary) -> float:
		dm.set_records("actors", [actor_data])
		player.load_actor_data(1)
		player.set_grid_cell_immediate(Vector2i(2, 2), 2)
		player.set_action_state("atacar")
		var elapsed := 0.0
		var step := 0.02
		for _i in range(200):
			if player.get_action_state() != "atacar":
				break
			player.debug_advance_animation(step)
			elapsed += step
		return elapsed

	var t_100: float = measure_attack_duration.call(atk_actor)
	var atk_200 := atk_actor.duplicate(true)
	atk_200["attack_speed_pct"] = 200
	var t_200: float = measure_attack_duration.call(atk_200)
	var atk_50 := atk_actor.duplicate(true)
	atk_50["attack_speed_pct"] = 50
	var t_50: float = measure_attack_duration.call(atk_50)

	print("[INFO] Attack cycle times -> 100%%=%.3fs | 200%%=%.3fs | 50%%=%.3fs" % [t_100, t_200, t_50])
	assert_true(abs(t_100 - 1.0) <= 0.05, "4.3 Atacar 100%% dura ~1.00s (got %.3f)" % t_100)
	assert_true(abs(t_200 - 0.5) <= 0.05, "4.4 Atacar 200%% es doble de rápido ~0.50s (got %.3f)" % t_200)
	assert_true(abs(t_50 - 2.0) <= 0.08, "4.5 Atacar 50%% es mitad de rápido ~2.00s (got %.3f)" % t_50)

	# En modo por turnos (combat_mode = 0) NO aplica velocidad de ataque, sin borrar attack_speed_pct
	sys["combat_mode"] = 0
	dm.set_system_config(sys)
	var t_tb: float = measure_attack_duration.call(atk_200)
	assert_true(abs(t_tb - 1.0) <= 0.05, "4.6 En turnos attack_speed_pct=200 NO altera animación (got %.3f)" % t_tb)
	assert_eq(int(dm.get_record_by_id(0, 1).get("attack_speed_pct", 0)), 200, "4.7 En turnos no se borra attack_speed_pct")

	# Velocidad caminar y correr en pasos reales
	sys["combat_mode"] = 1
	dm.set_system_config(sys)
	var spd_mod := atk_actor.duplicate(true)
	spd_mod["walk_speed_pct"] = 50
	spd_mod["run_speed_pct"] = 200
	dm.set_records("actors", [spd_mod])
	player.load_actor_data(1)
	assert_eq(player.get_walk_speed(), 64.0, "4.8 walk_speed_pct=50 -> 64.0 px/s")
	assert_eq(player.get_dash_speed(), 512.0, "4.9 run_speed_pct=200 -> 512.0 px/s")

	# Medir frames reales de movimiento (try_step + physics_frame)
	var measure_step_frames := func(is_dash: bool) -> int:
		player.set_grid_cell_immediate(Vector2i(2, 2), 6)
		var started: bool = player.try_step(6, is_dash)
		var ticks := 0
		if not started:
			return -1
		for _i in range(120):
			ticks += 1
			await physics_frame
			if not player.is_moving_step():
				break
		return ticks

	dm.set_records("actors", [atk_actor]) # 100% walk (128 px/s), 100% run (256 px/s)
	player.load_actor_data(1)
	var walk_100_ticks: int = await measure_step_frames.call(false)
	var run_100_ticks: int = await measure_step_frames.call(true)

	dm.set_records("actors", [spd_mod]) # 50% walk (64 px/s), 200% run (512 px/s)
	player.load_actor_data(1)
	var walk_50_ticks: int = await measure_step_frames.call(false)
	var run_200_ticks: int = await measure_step_frames.call(true)

	print("[INFO] Step ticks (60Hz) -> walk100=%d walk50=%d | run100=%d run200=%d" % [walk_100_ticks, walk_50_ticks, run_100_ticks, run_200_ticks])
	assert_true(walk_50_ticks > walk_100_ticks and walk_50_ticks >= 25, "4.10 Caminar 50%% tarda ~doble de ticks que 100%% (%d vs %d)" % [walk_50_ticks, walk_100_ticks])
	assert_true(run_200_ticks > 0 and run_200_ticks < run_100_ticks, "4.11 Correr 200%% tarda ~mitad de ticks que 100%% (%d vs %d)" % [run_200_ticks, run_100_ticks])

	# Verificar que ClockGearsEvent vinculado a NPC/Enemigo ARPG escala move_speed por walk_speed_pct
	var slow_arpg_en := {
		"id": 1,
		"name": "Tortuga ARPG",
		"walk_speed_pct": 50,
		"event_pages": [{"trigger": 0, "speed": 3, "commands": []}]
	}
	dm.set_records("arpg_enemies", [slow_arpg_en])
	var ev = ClassDB.instantiate("ClockGearsEvent")
	root.add_child(ev)
	ev.set_behavior_type(3) # CAT_ARPG_ENEMIES
	ev.set_behavior_ref_id(1)
	assert_eq(ev.get_move_speed(), 60.0, "4.12 ClockGearsEvent Enemigo ARPG aplica walk_speed_pct=50% (120 * 0.5 = 60)")
	ev.queue_free()

	map.queue_free()
	await process_frame

	# Limpieza
	restore_data_dir(data_dir, snap)
	var abs_tmp := ProjectSettings.globalize_path(tmp_dir)
	if DirAccess.dir_exists_absolute(abs_tmp):
		for f in DirAccess.get_files_at(tmp_dir):
			DirAccess.remove_absolute(abs_tmp.path_join(f))
		DirAccess.remove_absolute(abs_tmp)

	print("RESUMEN TEST 3A -> PASS:%d FAIL:%d" % [passed, failed])
	quit(0 if failed == 0 else 1)
