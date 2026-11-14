extends SceneTree

var passed: int = 0
var failed: int = 0
var drop_signals: Array = []

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func step_frames(frames: int) -> void:
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

func make_test_map(dm, w: int = 20, h: int = 16, map_id: String = "test_map_3c1") -> ClockGearsMap:
	var png_path := "res://tmp_t3c1_tileset.png"
	if not FileAccess.file_exists(png_path):
		var timg := Image.create(64, 32, false, Image.FORMAT_RGBA8)
		timg.fill(Color(0.25, 0.55, 0.30, 1.0))
		timg.save_png(ProjectSettings.globalize_path(png_path))

	var ts_rec := {
		"id": 1,
		"name": "TestTS",
		"slots": {"A5": png_path},
		"slot_collisions": {
			"A5": {
				"0,0": {"passage": 15, "layer": 0, "mode": 0},
				"1,0": {"passage": 0, "layer": 0, "mode": 1},
			}
		}
	}
	dm.set_category_array(13, [ts_rec]) # CAT_TILESETS

	var map := ClockGearsMap.new()
	map.set_map_size(Vector2i(w, h))
	map.set_grid_resolution(32)
	map.set_tileset_id(1)
	map.set_map_state_id(map_id)
	map.ensure_default_layers()

	var ts := TileSet.new()
	ts.set_tile_size(Vector2i(32, 32))
	var img := Image.create(64, 32, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.25, 0.55, 0.30, 1.0))
	var tex := ImageTexture.create_from_image(img)
	var atlas := TileSetAtlasSource.new()
	atlas.set_texture(tex)
	atlas.set_texture_region_size(Vector2i(32, 32))
	atlas.create_tile(Vector2i(0, 0))
	atlas.create_tile(Vector2i(1, 0))
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

func _make_attack_anim_sheet(hitbox_mult: float = 100.0, kb: float = 0.0, hitstop: int = 0) -> Dictionary:
	var hb_circle := {
		"type": 0,
		"offset_x": 24.0,
		"offset_y": 0.0,
		"radius": 24.0
	}
	var frames := []
	for _i in range(3):
		frames.append({
			"duration": 0.08,
			"hitbox": {
				"shapes": [hb_circle],
				"damage_mult_pct": hitbox_mult,
				"knockback": kb,
				"hitstop_ms": hitstop,
			}
		})
	return {
		"animations": {
			"idle": {"loop": true, "frames": [{"duration": 0.15}]},
			"atacar_derecha": {"loop": false, "frames": frames},
			"atacar_abajo": {"loop": false, "frames": frames},
		}
	}

func _make_enemy_sprite_data(hurt_mult_pct: float = 100.0, with_morir: bool = false) -> Dictionary:
	var d := {
		"hurtbox": {
			"enabled": true,
			"damage_mult_pct": hurt_mult_pct,
			"shape": {"type": 1, "size_x": 32.0, "size_y": 32.0, "offset_x": 0.0, "offset_y": 0.0}
		},
		"animations": {
			"idle": {"loop": true, "frames": [{"duration": 0.15}]},
			"golpeado": {"loop": false, "frames": [{"duration": 0.10}]},
		}
	}
	if with_morir:
		d["animations"]["morir"] = {"loop": false, "frames": [{"duration": 0.05}, {"duration": 0.05}]}
	return d

func _on_drop_obtained(item_name: String, amount: int) -> void:
	drop_signals.append({"name": item_name, "amount": amount})

func _init() -> void:
	var wd := create_timer(60.0)
	wd.timeout.connect(func():
		printerr("[TIMEOUT] Test Tanda 3C-1 supero 60s")
		quit(1)
	)
	call_deferred("_run_tests")

func _run_tests() -> void:
	print("=== Test Suite 9A Tanda 3C-1: Ataque Jugador, Dano, Retroceso, Drops y Respawn ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	var fm = Engine.get_singleton("ClockGearsFormulas")
	check(dm != null and fm != null, "Singletons DataManager y Formulas disponibles")
	if dm == null or fm == null:
		quit(1)
		return

	# Backup clockgears_data
	var has_orig := DirAccess.dir_exists_absolute(ProjectSettings.globalize_path("res://clockgears_data"))
	if has_orig:
		var bs := FileAccess.open("res://tmp_bak_3c1_marker", FileAccess.WRITE)
		if bs: bs.close()
	var backup_dir := "res://tmp_clockgears_data_bak_3c1"
	dm.save_all(backup_dir)

	dm.init_empty()
	dm.reset_runtime_state()
	ClockGearsDebugOverlay.clear_battle_log()

	# Poblar BD limpia
	dm.set_category_array(1, [{ # CAT_CLASSES
		"id": 1, "name": "Espadachin",
		"exp_base": 30, "exp_extra": 20, "exp_acc_a": 30, "exp_acc_b": 30
	}])
	dm.set_category_array(3, [{ # CAT_ITEMS
		"id": 1, "name": "Pocion Verde"
	}])
	dm.set_category_array(22, [ # CAT_ARPG_WEAPONS
		{"id": 1, "name": "Espada Hierro", "atk": 10},
		{"id": 2, "name": "Espada Ignis", "atk": 10, "element_id": 2, "element_key": "fuego"},
	])
	dm.set_category_array(7, [ # CAT_ENEMIES (para comprobar Batalla)
		{"id": 1, "name": "Limo Turnos", "hp": 50, "atk": 5, "def": 2}
	])
	dm.set_category_array(8, [ # CAT_TROOPS
		{"id": 1, "name": "Tropa Prueba", "members": [{"enemy_id": 1, "x": 400, "y": 240}]}
	])

	var actor_rec := {
		"id": 1,
		"name": "HeroeARPG",
		"class_id": 1,
		"level": 1,
		"hp": 120,
		"atk": 15,
		"def": 5,
		"luk": 0,
		"walk_speed_pct": 100,
		"attack_speed_pct": 100,
		"sprite_data": _make_attack_anim_sheet(100.0, 0.0, 0),
		"arpg_equip_slots": [
			{"slot": "arma", "category": "arpg_weapons", "piece_id": 1},
			{"slot": "armadura", "category": "arpg_armors", "piece_id": 0},
		]
	}
	dm.set_category_array(0, [actor_rec]) # CAT_ACTORS

	var enemy_base := {
		"id": 1, "name": "Goblin Base",
		"hp": 500, "atk": 8, "def": 5, "exp": 20, "gold": 10,
		"ai_type": "quieto", "knockback_resist": 0,
		"detection_zones": [],
		"sprite_data": _make_enemy_sprite_data(100.0, false)
	}
	var enemy_weak_200 := {
		"id": 2, "name": "Goblin PuntoDebil",
		"hp": 500, "atk": 8, "def": 5, "exp": 20, "gold": 10,
		"ai_type": "quieto", "knockback_resist": 0,
		"detection_zones": [],
		"sprite_data": _make_enemy_sprite_data(200.0, false)
	}
	var enemy_fire_weak := {
		"id": 3, "name": "Goblin DebilFuego",
		"hp": 500, "atk": 8, "def": 5, "exp": 20, "gold": 10,
		"ai_type": "quieto", "knockback_resist": 0,
		"detection_zones": [],
		"traits": [{"code": 11, "data_id": 2, "element_key": "fuego", "value": 200.0}],
		"sprite_data": _make_enemy_sprite_data(100.0, false)
	}
	var enemy_kb := {
		"id": 4, "name": "Goblin Empuje",
		"hp": 500, "atk": 8, "def": 5, "exp": 20, "gold": 10,
		"ai_type": "vagar", "wander_radius": 3.0, "knockback_resist": 10,
		"detection_zones": [],
		"sprite_data": _make_enemy_sprite_data(100.0, false)
	}
	var enemy_reward := {
		"id": 5, "name": "Goblin Botin",
		"hp": 10, "atk": 8, "def": 0, "exp": 200, "gold": 75,
		"ai_type": "quieto", "knockback_resist": 0,
		"drops": [{"kind": 3, "item_id": 1, "prob": 100}],
		"detection_zones": [],
		"sprite_data": _make_enemy_sprite_data(100.0, true)
	}
	dm.set_category_array(20, [enemy_base, enemy_weak_200, enemy_fire_weak, enemy_kb, enemy_reward]) # CAT_ARPG_ENEMIES
	dm.reset_runtime_state()
	dm.set_actor_arpg_equip(1, "arma", 1)

	var sys: Dictionary = dm.get_system_config()
	sys["combat_mode"] = 1
	sys["party"] = [1]
	sys["start_x"] = 4
	sys["start_y"] = 4
	sys["start_dir"] = 6
	sys["has_start_pos"] = true
	dm.set_system_config(sys)

	var drop_cb := Callable(self, "_on_drop_obtained")
	dm.connect("drop_obtained", drop_cb)

	# ---------------------------------------------------------
	# 1. UN GOLPE DAÑA SOLO 1 VEZ POR SWING Y EXACTO CON FORMULA
	# ---------------------------------------------------------
	print("--- 1. 1 golpe por swing y paridad formula ---")
	var map1 := make_test_map(dm, 20, 16, "map_swing")
	map1.set_combat_mode(1)
	var ev_cont: Node2D = map1.get_node_or_null("Eventos")
	if ev_cont == null:
		ev_cont = Node2D.new()
		ev_cont.set_name("Eventos")
		map1.add_child(ev_cont)

	var e1 := ClockGearsEvent.new()
	e1.set_event_id(1)
	e1.set_event_name("Goblin_Swing")
	e1.set_behavior_type(3)
	e1.set_behavior_ref_id(1)
	e1.set_position(Vector2(5 * 32, 4 * 32))
	ev_cont.add_child(e1)

	root.add_child(map1)
	await step_frames(2)

	var pl: ClockGearsPlayer = map1.get_runtime_player()
	check(pl != null, "RuntimePlayer creado en mapa ARPG")

	var dmg_events := []
	e1.connect("arpg_damaged", func(d: int, _c: bool): dmg_events.append(d))

	# Calcular dano exacto esperado usando exactamente la misma secuencia RNG
	fm.set_rng_seed(424242)
	var tb_crit_c: float = fm.calculate_critical_chance(0.0, 0.0)
	var expected_tb_dmg: int = int(fm.calculate_damage(
		"max(a.atk * 4 - b.def * 2, 0)",
		{"atk": 25, "def": 5, "mat": 10, "mdf": 10, "luk": 0},
		{"atk": 8, "def": 5},
		{"is_critical": tb_crit_c > 0.0, "critical_multiplier": 2.0, "variance": 20.0}
	))

	fm.set_rng_seed(424242)
	pl.set_grid_cell_immediate(Vector2i(4, 4), 6)
	pl.set_action_state("atacar")
	for _f in range(18): # recorrer todos los frames del swing (3 * 0.08s = 0.24s)
		pl.debug_advance_animation(0.02)
	check(dmg_events.size() == 1, "Un swing de 3 frames solo golpea 1 vez (golpes=%d)" % dmg_events.size())
	check(dmg_events.size() >= 1 and dmg_events[0] == expected_tb_dmg, "Dano ARPG coincide con formula por turnos (%s == %s)" % [str(dmg_events[0] if dmg_events.size() > 0 else -1), str(expected_tb_dmg)])

	# Segundo swing vuelve a golpear 1 vez
	pl.set_action_state("atacar")
	for _f in range(18):
		pl.debug_advance_animation(0.02)
	check(dmg_events.size() == 2, "Segundo swing golpea una segunda vez (golpes=%d)" % dmg_events.size())

	# ---------------------------------------------------------
	# 2. ELEMENTO DE ARMA ARPG Y HURTBOX x200 DUPLICA DANO
	# ---------------------------------------------------------
	print("--- 2. Elemento del arma y hurtbox x200 ---")
	var e_weak := ClockGearsEvent.new()
	e_weak.set_event_id(2)
	e_weak.set_event_name("Goblin_Weak200")
	e_weak.set_behavior_type(3)
	e_weak.set_behavior_ref_id(2)
	e_weak.set_position(Vector2(5 * 32, 4 * 32))
	ev_cont.add_child(e_weak)
	e_weak.update_active_page()

	e1.set_position(Vector2(15 * 32, 15 * 32)) # apartar e1 del rango
	fm.set_rng_seed(77777)
	pl.set_action_state("atacar")
	for _f in range(18):
		pl.debug_advance_animation(0.02)
	var dmg_100 := 500 - e1.get_arpg_hp() # usar delta con ref 1 con misma semilla primero
	# Mejor comparar directo dmg nuevo contra e1 recolocado y e_weak con misma semilla:
	e1.set_position(Vector2(5 * 32, 4 * 32))
	e_weak.set_position(Vector2(15 * 32, 15 * 32))
	var hp_before_100 := e1.get_arpg_hp()
	fm.set_rng_seed(88888)
	pl.set_action_state("atacar")
	for _f in range(18):
		pl.debug_advance_animation(0.02)
	var d100 := hp_before_100 - e1.get_arpg_hp()

	e1.set_position(Vector2(15 * 32, 15 * 32))
	e_weak.set_position(Vector2(5 * 32, 4 * 32))
	var hp_before_200 := e_weak.get_arpg_hp()
	fm.set_rng_seed(88888)
	pl.set_action_state("atacar")
	for _f in range(18):
		pl.debug_advance_animation(0.02)
	var d200 := hp_before_200 - e_weak.get_arpg_hp()
	check(d100 > 0 and d200 == d100 * 2, "Hurtbox con multiplicador 200%% duplica el dano (%d -> %d)" % [d100, d200])

	# Arma con elemento fuego contra enemigo debil 200% a fuego
	e_weak.set_position(Vector2(15 * 32, 15 * 32))
	var e_fire := ClockGearsEvent.new()
	e_fire.set_event_id(3)
	e_fire.set_event_name("Goblin_Fire")
	e_fire.set_behavior_type(3)
	e_fire.set_behavior_ref_id(3)
	e_fire.set_position(Vector2(5 * 32, 4 * 32))
	ev_cont.add_child(e_fire)
	e_fire.update_active_page()
	dm.set_actor_arpg_equip(1, "arma", 2) # Espada Ignis (fuego, mismo atk 10)
	fm.set_rng_seed(88888)
	pl.set_action_state("atacar")
	for _f in range(18):
		pl.debug_advance_animation(0.02)
	var d_fire := 500 - e_fire.get_arpg_hp()
	check(d_fire == d100 * 2, "Arma ARPG de fuego duplica dano vs debilidad 200%% (%d vs fisico %d)" % [d_fire, d100])
	dm.set_actor_arpg_equip(1, "arma", 1)

	# ---------------------------------------------------------
	# 3. RETROCESO (MURO, RESISTENCIA, GOLPEADO) E HITSTOP CONDICIONAL
	# ---------------------------------------------------------
	print("--- 3. Retroceso y pausa de impacto ---")
	# Limpiar e_fire fuera de rango
	e_fire.set_position(Vector2(16 * 32, 15 * 32))
	var e_kb := ClockGearsEvent.new()
	e_kb.set_event_id(4)
	e_kb.set_event_name("Goblin_KB")
	e_kb.set_behavior_type(3)
	e_kb.set_behavior_ref_id(4)
	e_kb.set_position(Vector2(5 * 32, 4 * 32))
	ev_cont.add_child(e_kb)
	e_kb.update_active_page()

	# Hit con kb=0 y hitstop=0 => hitstop queda en 0
	check(pl.get_arpg_hitstop_remaining() == 0.0 and e_kb.get_arpg_hitstop_remaining() == 0.0, "Sin hitstop cuando hitbox.hitstop_ms == 0")

	# Actualizar actor con hitbox kb=42, hitstop_ms=100 (resist enemigo = 10 -> retroceso efectivo 32px)
	actor_rec["sprite_data"] = _make_attack_anim_sheet(100.0, 42.0, 100)
	dm.update_record(0, 1, actor_rec)
	pl.reload_actor_arpg_data()
	pl.set_grid_cell_immediate(Vector2i(4, 4), 6)

	var kb_start_x := e_kb.get_position().x
	pl.set_action_state("atacar")
	check(pl.get_arpg_hitstop_remaining() > 0.0 and e_kb.get_arpg_hitstop_remaining() > 0.0, "Hitstop activo cuando hitstop_ms > 0")
	check(e_kb.get_action_state() == "golpeado", "Enemigo reproduce golpeado cuando retroceso efectivo > 0 (%s)" % e_kb.get_action_state())
	var kb_guard := 0
	while (e_kb.get_arpg_hitstop_remaining() > 0.0 or e_kb.get_arpg_knockback_remaining() > 0.0) and kb_guard < 40:
		await step_frames(1)
		kb_guard += 1
	var kb_moved_x := e_kb.get_position().x - kb_start_x
	check(kb_moved_x >= 28.0 and kb_moved_x <= 36.0, "Retroceso desplaza ~(42-10)=32px en espacio libre (dx=%.1f, frames=%d)" % [kb_moved_x, kb_guard])

	# Ahora colocar un muro en celda (7, 4) y el enemigo en (6, 4)
	var ground_l: TileMapLayer = map1.get_layer(0)
	ground_l.set_cell(Vector2i(7, 4), 0, Vector2i(1, 0)) # tile bloqueado X
	map1.invalidate_passability_cache()
	e_kb.set_position(Vector2(6 * 32, 4 * 32))
	pl.set_grid_cell_immediate(Vector2i(5, 4), 6)
	pl.set_action_state("atacar")
	var wall_guard := 0
	while (e_kb.get_arpg_hitstop_remaining() > 0.0 or e_kb.get_arpg_knockback_remaining() > 0.0) and wall_guard < 40:
		await step_frames(1)
		wall_guard += 1
	check(e_kb.get_position().x <= 6 * 32 + 1.0, "Retroceso se frena contra muro no pasable (x=%.1f <= 193)" % e_kb.get_position().x)

	# ---------------------------------------------------------
	# 4. MUERTE, COLISION, EXP/NIVEL, ORO Y DROPS
	# ---------------------------------------------------------
	print("--- 4. Muerte, EXP, Nivel, Oro y Drops ---")
	e_kb.set_position(Vector2(16 * 32, 15 * 32))
	ground_l.set_cell(Vector2i(7, 4), 0, Vector2i(0, 0))
	map1.invalidate_passability_cache()

	actor_rec["sprite_data"] = _make_attack_anim_sheet(100.0, 0.0, 0)
	dm.update_record(0, 1, actor_rec)
	pl.reload_actor_arpg_data()

	while pl.get_action_state() != "quieto":
		await step_frames(1)

	var e_drop := ClockGearsEvent.new()
	e_drop.set_event_id(5)
	e_drop.set_event_name("Goblin_Drop")
	e_drop.set_behavior_type(3)
	e_drop.set_behavior_ref_id(5)
	e_drop.set_position(Vector2(5 * 32, 4 * 32))
	ev_cont.add_child(e_drop)
	e_drop.update_active_page()

	drop_signals.clear()
	pl.set_grid_cell_immediate(Vector2i(4, 4), 6)
	pl.set_action_state("atacar")
	# Justo tras golpe mortal: Dying=true, colision desactivada
	check(e_drop.is_arpg_dying(), "Enemigo con hp<=0 entra en estado Dying ('morir')")
	check(e_drop.is_body_through(), "Colision de cuerpo desactivada inmediatamente al morir")
	check(dm.get_gold() == 75, "Oro sumado al grupo (+75 -> %d)" % dm.get_gold())
	check(dm.get_actor_runtime_exp(1) == 200, "EXP concedida al actor (%d)" % dm.get_actor_runtime_exp(1))
	check(dm.get_actor_runtime_level(1) > 1, "Actor sube de nivel con 200 EXP (Nivel=%d)" % dm.get_actor_runtime_level(1))
	check(dm.has_item("Pocion Verde", 1), "Drop 100% anadido al inventario")
	check(drop_signals.size() == 1 and drop_signals[0]["name"] == "Pocion Verde", "Senal drop_obtained emitida con Pocion Verde")

	# Esperar fin de animacion morir (2 * 0.05 = 0.10s -> ~10 frames)
	await step_frames(12)
	check(e_drop.is_arpg_dead() and not e_drop.is_visible(), "Enemigo desaparece al terminar la animacion morir")

	# ---------------------------------------------------------
	# 5. DEPURACION F3 (HP JUGADOR + NUMEROS + LOG)
	# ---------------------------------------------------------
	print("--- 5. Depuracion F3 ---")
	var dbg: ClockGearsDebugOverlay = map1.get_runtime_debug_overlay()
	check(dbg != null, "DebugOverlay activo en mapa")
	if dbg:
		var hp_info := dbg.get_player_hp_debug_info()
		check(int(hp_info.get("max_hp", 0)) > 120 and int(hp_info.get("level", 1)) > 1, "F3 reporta HP/Nivel actualizados tras subir de nivel (%s)" % str(hp_info))
		check(dbg.get_recent_damage_numbers().size() >= 1, "F3 registra numeros de dano recientes (%d)" % dbg.get_recent_damage_numbers().size())
		check(ClockGearsDebugOverlay.get_battle_log_text().find("[ARPG]") != -1, "Log de batalla contiene entrada por golpe [ARPG]")

	root.remove_child(map1)
	map1.queue_free()
	await step_frames(2)

	# ---------------------------------------------------------
	# 6. REAPARICION POR MAPA Y GUARDADO / CARGA
	# ---------------------------------------------------------
	print("--- 6. Respawn por mapa + Save/Load ---")
	var test_respawn_map := func(r_mode: int, r_mins: float, ev_id: int) -> ClockGearsMap:
		var m := make_test_map(dm, 16, 12, "respawn_map_" + str(ev_id))
		m.set_combat_mode(1)
		m.set_respawn_mode(r_mode)
		m.set_respawn_minutes(r_mins)
		var ec: Node2D = m.get_node_or_null("Eventos")
		if ec == null:
			ec = Node2D.new()
			ec.set_name("Eventos")
			m.add_child(ec)
		var ev := ClockGearsEvent.new()
		ev.set_event_id(ev_id)
		ev.set_event_name("EnResp_" + str(ev_id))
		ev.set_behavior_type(3)
		ev.set_behavior_ref_id(5) # hp 10
		ev.set_position(Vector2(5 * 32, 4 * 32))
		ec.add_child(ev)
		return m

	# 6A) RESPAWN_NEVER (1)
	dm.reset_runtime_state()
	var m_never: ClockGearsMap = test_respawn_map.call(1, 5.0, 101)
	root.add_child(m_never)
	await step_frames(2)
	var e101: ClockGearsEvent = m_never.get_node("Eventos").get_child(0)
	e101.take_arpg_hit(20, 0.0, Vector2.RIGHT, 0, false)
	await step_frames(10)
	var save_never: Dictionary = dm.export_runtime_state()
	root.remove_child(m_never)
	m_never.queue_free()
	await step_frames(1)

	dm.reset_runtime_state()
	dm.import_runtime_state(save_never)
	var m_never_2: ClockGearsMap = test_respawn_map.call(1, 5.0, 101)
	root.add_child(m_never_2)
	await step_frames(2)
	var e101_b: ClockGearsEvent = m_never_2.get_node("Eventos").get_child(0)
	check(e101_b.is_arpg_dead() and not e101_b.is_visible(), "RESPAWN_NEVER: enemigo permanece muerto tras guardar, cargar y volver a entrar")
	root.remove_child(m_never_2)
	m_never_2.queue_free()
	await step_frames(1)

	# 6B) RESPAWN_ON_REENTER (0)
	dm.reset_runtime_state()
	var m_reenter: ClockGearsMap = test_respawn_map.call(0, 5.0, 102)
	root.add_child(m_reenter)
	await step_frames(2)
	var e102: ClockGearsEvent = m_reenter.get_node("Eventos").get_child(0)
	e102.take_arpg_hit(20, 0.0, Vector2.RIGHT, 0, false)
	await step_frames(10)
	var save_reenter: Dictionary = dm.export_runtime_state()
	root.remove_child(m_reenter)
	m_reenter.queue_free()
	await step_frames(1)

	dm.reset_runtime_state()
	dm.import_runtime_state(save_reenter)
	var m_reenter_2: ClockGearsMap = test_respawn_map.call(0, 5.0, 102)
	root.add_child(m_reenter_2)
	await step_frames(2)
	var e102_b: ClockGearsEvent = m_reenter_2.get_node("Eventos").get_child(0)
	check(e102_b.is_arpg_alive() and e102_b.get_arpg_hp() == 10, "RESPAWN_ON_REENTER: enemigo reaparece vivo con HP completo al volver a entrar")
	root.remove_child(m_reenter_2)
	m_reenter_2.queue_free()
	await step_frames(1)

	# 6C) RESPAWN_AFTER_MINUTES (2) con 2.0 minutos
	dm.reset_runtime_state()
	var m_timed: ClockGearsMap = test_respawn_map.call(2, 2.0, 103)
	root.add_child(m_timed)
	await step_frames(2)
	var e103: ClockGearsEvent = m_timed.get_node("Eventos").get_child(0)
	e103.take_arpg_hit(20, 0.0, Vector2.RIGHT, 0, false)
	await step_frames(10)
	var save_timed: Dictionary = dm.export_runtime_state()
	root.remove_child(m_timed)
	m_timed.queue_free()
	await step_frames(1)

	dm.reset_runtime_state()
	dm.import_runtime_state(save_timed)
	dm.advance_runtime_clock(60.0) # 1 min (< 2 min)
	var m_timed_early: ClockGearsMap = test_respawn_map.call(2, 2.0, 103)
	root.add_child(m_timed_early)
	await step_frames(2)
	var e103_early: ClockGearsEvent = m_timed_early.get_node("Eventos").get_child(0)
	check(e103_early.is_arpg_dead(), "RESPAWN_AFTER_MINUTES: sigue muerto antes de pasar X minutos (60s < 120s)")
	dm.advance_runtime_clock(70.0) # total 130s (>= 120s)
	m_timed_early.refresh_enemy_respawns()
	check(e103_early.is_arpg_alive() and e103_early.get_arpg_hp() == 10, "RESPAWN_AFTER_MINUTES: reaparece al cumplirse X minutos (130s >= 120s)")
	root.remove_child(m_timed_early)
	m_timed_early.queue_free()
	await step_frames(1)

	# ---------------------------------------------------------
	# 7. MODO DE COMBATE POR MAPA + COMANDO "INICIAR BATALLA"
	# ---------------------------------------------------------
	print("--- 7. Modo de combate por mapa e Iniciar Batalla ---")
	sys = dm.get_system_config()
	sys["combat_mode"] = 1 # proyecto = ARPG
	dm.set_system_config(sys)
	var m_turn := make_test_map(dm, 16, 12, "map_turn_override")
	m_turn.set_combat_mode(0) # mapa fuerza Por Turnos
	root.add_child(m_turn)
	await step_frames(2)
	var pl_turn := m_turn.get_runtime_player()
	check(not m_turn.is_arpg_effective() and not pl_turn.is_arpg_active_here(), "Mapa en Por Turnos sobrescribe proyecto ARPG")
	root.remove_child(m_turn)
	m_turn.queue_free()
	await step_frames(1)

	sys["combat_mode"] = 0 # proyecto = Por Turnos
	dm.set_system_config(sys)
	var m_arpg := make_test_map(dm, 16, 12, "map_arpg_override")
	m_arpg.set_combat_mode(1) # mapa fuerza ARPG
	root.add_child(m_arpg)
	await step_frames(2)
	var pl_arpg := m_arpg.get_runtime_player()
	check(m_arpg.is_arpg_effective() and pl_arpg.is_arpg_active_here(), "Mapa en ARPG sobrescribe proyecto Por Turnos")

	# Ejecutar comando "battle" (Iniciar Batalla) en mapa ARPG -> debe abrir batalla por turnos
	var interp := m_arpg.get_runtime_interpreter()
	var battle_ev := ClockGearsEvent.new()
	battle_ev.set_event_id(200)
	battle_ev.set_event_name("BossEv")
	battle_ev.set_commands([{"type": "battle", "troop": "Tropa Prueba", "can_escape": true}])
	m_arpg.add_child(battle_ev)
	interp.start_event(battle_ev)
	await step_frames(4)
	var found_battle_scene := false
	for ch in interp.get_children():
		if ch is ClockGearsBattleScene:
			found_battle_scene = true
			ch.queue_free()
	check(found_battle_scene, "Comando 'Iniciar Batalla' abre combate por turnos incluso en mapa ARPG")
	root.remove_child(m_arpg)
	m_arpg.queue_free()
	await step_frames(2)

	# Restaurar estado original
	dm.disconnect("drop_obtained", drop_cb)
	if has_orig:
		dm.load_all(backup_dir)
		dm.save_all("res://clockgears_data")
		DirAccess.remove_absolute(ProjectSettings.globalize_path("res://tmp_bak_3c1_marker"))
	else:
		_cleanup_dir_recursive("res://clockgears_data")
	_cleanup_dir_recursive(backup_dir)
	for tmpf in ["res://tmp_t3c1_tileset.png"]:
		if FileAccess.file_exists(tmpf):
			DirAccess.remove_absolute(ProjectSettings.globalize_path(tmpf))

	print("=== RESULTADO TANDA 3C-1: PASS=%d FAIL=%d ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
