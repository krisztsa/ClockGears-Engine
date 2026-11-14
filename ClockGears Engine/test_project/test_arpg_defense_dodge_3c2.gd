extends SceneTree

var passed: int = 0
var failed: int = 0
var death_signals: int = 0
var restart_signals: int = 0

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

func _ensure_1px_png(path: String) -> void:
	if FileAccess.file_exists(path):
		return
	var img := Image.create(32, 32, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.8, 0.8, 0.9, 1.0))
	img.save_png(ProjectSettings.globalize_path(path))

func make_test_map(dm, w: int = 20, h: int = 16, map_id: String = "test_map_3c2", walls: Array = []) -> ClockGearsMap:
	var png_path := "res://tmp_t3c2_tileset.png"
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
	map.set_combat_mode(1) # ARPG
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
	for wc in walls:
		ground.set_cell(wc, 0, Vector2i(1, 0))

	map.invalidate_passability_cache()
	return map

func _make_actor_sprite_data(with_dodge_slot: bool = true) -> Dictionary:
	var sp := "res://tmp_t3c2_sprite.png"
	_ensure_1px_png(sp)
	var anims := {
		"idle": {"loop": true, "spritesheet": sp, "fps": 8, "frames": [{"duration": 0.15}]},
		"mover_abajo": {"loop": true, "spritesheet": sp, "fps": 8, "frames": [{"duration": 0.12}]},
		"mover_arriba": {"loop": true, "spritesheet": sp, "fps": 8, "frames": [{"duration": 0.12}]},
		"mover_izquierda": {"loop": true, "spritesheet": sp, "fps": 8, "frames": [{"duration": 0.12}]},
		"mover_derecha": {"loop": true, "spritesheet": sp, "fps": 8, "frames": [{"duration": 0.12}]},
		"atacar_derecha": {"loop": false, "spritesheet": sp, "fps": 10, "frames": [{"duration": 0.20}, {"duration": 0.20}]},
		"atacar_abajo": {"loop": false, "spritesheet": sp, "fps": 10, "frames": [{"duration": 0.20}, {"duration": 0.20}]},
		"bloquear_derecha": {"loop": true, "spritesheet": sp, "fps": 8, "frames": [{"duration": 0.15}]},
		"golpeado": {"loop": false, "spritesheet": sp, "fps": 10, "frames": [{"duration": 0.12}]},
		"morir": {"loop": false, "spritesheet": sp, "fps": 10, "frames": [{"duration": 0.06}, {"duration": 0.06}]},
	}
	if with_dodge_slot:
		anims["esquivar_derecha"] = {"loop": false, "spritesheet": sp, "fps": 10, "frames": [{"duration": 0.12}, {"duration": 0.12}]}
		anims["esquivar_izquierda"] = {"loop": false, "spritesheet": sp, "fps": 10, "frames": [{"duration": 0.12}, {"duration": 0.12}]}
	return {
		"hurtbox": {
			"enabled": true,
			"damage_mult_pct": 100.0,
			"shape": {"type": 1, "size_x": 28.0, "size_y": 28.0, "offset_x": 0.0, "offset_y": 0.0}
		},
		"animations": anims
	}

func _make_enemy_attack_sprite_data(hb_mult: float = 100.0, kb: float = 0.0, elem_id: int = 0) -> Dictionary:
	var sp := "res://tmp_t3c2_sprite.png"
	_ensure_1px_png(sp)
	var hb_circle := {
		"type": 0,
		"offset_x": 0.0,
		"offset_y": 0.0,
		"radius": 64.0
	}
	var frames := []
	for _i in range(3):
		frames.append({
			"duration": 0.06,
			"hitbox": {
				"shapes": [hb_circle],
				"damage_mult_pct": hb_mult,
				"knockback": kb,
				"hitstop_ms": 0,
				"element_id": elem_id,
			}
		})
	return {
		"animations": {
			"idle": {"loop": true, "spritesheet": sp, "fps": 8, "frames": [{"duration": 0.15}]},
			"atacar_abajo": {"loop": false, "spritesheet": sp, "fps": 12, "frames": frames},
			"atacar_izquierda": {"loop": false, "spritesheet": sp, "fps": 12, "frames": frames},
			"atacar_derecha": {"loop": false, "spritesheet": sp, "fps": 12, "frames": frames},
			"atacar_arriba": {"loop": false, "spritesheet": sp, "fps": 12, "frames": frames},
		}
	}

func _spawn_enemy(map: ClockGearsMap, ev_id: int, cell: Vector2i, dir: int, enemy_id: int = 1) -> ClockGearsEvent:
	var ev := ClockGearsEvent.new()
	ev.set_event_id(ev_id)
	ev.set_event_name("Enemigo3C2_%d" % ev_id)
	ev.set_behavior_type(3) # BEHAVIOR_ARPG_ENEMY
	ev.set_behavior_ref_id(enemy_id)
	ev.set_direction(dir)
	ev.set_position(Vector2(cell) * 32.0)
	var container := map.get_node_or_null("Eventos")
	if container == null:
		container = map
	container.add_child(ev)
	return ev

func _send_action_input(action_name: String, pressed: bool) -> void:
	var ev := InputEventAction.new()
	ev.action = action_name
	ev.pressed = pressed
	get_root().push_input(ev)

func _on_player_died() -> void:
	death_signals += 1

func _on_map_restarted() -> void:
	restart_signals += 1

func _init() -> void:
	var wd := create_timer(60.0)
	wd.timeout.connect(func():
		printerr("[TIMEOUT] Test Tanda 3C-2 supero 60s")
		quit(1)
	)
	call_deferred("_run_tests")

func _run_tests() -> void:
	print("=== Test Suite 9A Tanda 3C-2: Dano al Jugador, Bloqueo, Invulnerabilidad, Esquivar y Muerte ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	var fm = Engine.get_singleton("ClockGearsFormulas")
	check(dm != null and fm != null, "Singletons DataManager y Formulas disponibles")
	if dm == null or fm == null:
		quit(1)
		return

	# ---------------------------------------------------------
	# 1. MIGRACION DE SYSTEM.JSON ANTIGUO Y DEFAULTS
	# ---------------------------------------------------------
	print("--- 1. Migracion de system.json antiguo y defaults ---")
	var mig_dir := "res://tmp_mig_3c2_data"
	dm.init_empty()
	dm.save_all(mig_dir)
	var old_sys := {
		"game_title": "Proyecto Viejo",
		"combat_mode": 1,
		"screen_width": 960,
		"screen_height": 540
	}
	var sf := FileAccess.open(mig_dir + "/system.json", FileAccess.WRITE)
	sf.store_string(JSON.stringify(old_sys, "\t"))
	sf.close()

	dm.init_empty()
	check(dm.load_all(mig_dir), "load_all() carga proyecto antiguo con system.json sin claves 3C-2")
	var migrated_sys: Dictionary = dm.get_system_config()
	check(is_equal_approx(float(migrated_sys.get("arpg_invuln_seconds", -1.0)), 0.5), "Migracion: arpg_invuln_seconds == 0.5 por defecto")
	check(int(migrated_sys.get("arpg_block_reduction_pct", -1)) == 50, "Migracion: arpg_block_reduction_pct == 50 por defecto")
	check(String(migrated_sys.get("arpg_dodge_type", "")) == "voltereta", "Migracion: arpg_dodge_type == 'voltereta' por defecto")
	check(String(migrated_sys.get("arpg_dodge_key", "")) == "Space", "Migracion: arpg_dodge_key == 'Space' por defecto")
	check(is_equal_approx(float(migrated_sys.get("arpg_dodge_duration", -1.0)), 0.25), "Migracion: arpg_dodge_duration == 0.25 por defecto")
	check(is_equal_approx(float(migrated_sys.get("arpg_dodge_distance", -1.0)), 96.0), "Migracion: arpg_dodge_distance == 96.0 por defecto")
	check(is_equal_approx(float(migrated_sys.get("arpg_dodge_cooldown", -1.0)), 0.4), "Migracion: arpg_dodge_cooldown == 0.4 por defecto")
	check(String(migrated_sys.get("game_title", "")) == "Proyecto Viejo", "Migracion conserva claves previas de system.json")
	_cleanup_dir_recursive(mig_dir)

	# ---------------------------------------------------------
	# Preparar BD limpia en memoria (sin mutar clockgears_data/)
	# ---------------------------------------------------------
	dm.init_empty()
	dm.reset_runtime_state()
	var sys: Dictionary = dm.get_system_config()
	sys["combat_mode"] = 1
	sys["party"] = [1]
	sys["start_x"] = 3
	sys["start_y"] = 3
	sys["start_dir"] = 6
	sys["has_start_pos"] = true
	dm.set_system_config(sys)

	dm.set_category_array(1, [{ # CAT_CLASSES
		"id": 1, "name": "Guerrero",
		"exp_base": 30, "exp_extra": 20, "exp_acc_a": 30, "exp_acc_b": 30
	}])
	dm.set_category_array(23, [ # CAT_ARPG_ARMORS
		{
			"id": 1,
			"name": "Capa Ignifuga",
			"def": 0,
			"traits": [{"code": 11, "data_id": 2, "element_key": "fuego", "value": 50.0}]
		}
	])
	dm.set_category_array(0, [{ # CAT_ACTORS
		"id": 1,
		"name": "Heroe3C2",
		"class_id": 1,
		"level": 1,
		"hp": 200,
		"atk": 15,
		"def": 10, # ATK enemy 15*4 - 10*2 = 40
		"luk": 0,
		"walk_speed_pct": 100,
		"run_speed_pct": 150,
		"attack_speed_pct": 100,
		"sprite_data": _make_actor_sprite_data(true),
		"arpg_equip_slots": [
			{"slot": "arma", "category": "arpg_weapons", "piece_id": 0},
			{"slot": "armadura", "category": "arpg_armors", "piece_id": 0},
		]
	}])
	dm.set_category_array(20, [ # CAT_ARPG_ENEMIES
		{
			"id": 1,
			"name": "Trasgo Rojo",
			"level": 1,
			"exp": 10,
			"gold": 5,
			"hp": 100,
			"atk": 15, # Formula: 15*4 - 10*2 = 40 base
			"def": 5,
			"mat": 5,
			"mdf": 5,
			"luk": 0,
			"ai_type": "quieto",
			"detection_zones": [],
			"attack_interval": 0.8,
			"attack_speed_pct": 100,
			"knockback_resistance": 0,
			"drops": [],
			"sprite_data": _make_enemy_attack_sprite_data(100.0, 24.0, 0)
		},
		{
			"id": 2,
			"name": "Trasgo Mortal",
			"level": 5,
			"exp": 20,
			"gold": 10,
			"hp": 100,
			"atk": 120, # Formula: 120*4 - 10*2 = 460 lethal
			"def": 5,
			"ai_type": "quieto",
			"detection_zones": [],
			"attack_interval": 0.5,
			"attack_speed_pct": 100,
			"knockback_resistance": 0,
			"drops": [],
			"sprite_data": _make_enemy_attack_sprite_data(100.0, 0.0, 0)
		},
		{
			"id": 3,
			"name": "Trasgo Igneo",
			"level": 1,
			"exp": 10,
			"gold": 5,
			"hp": 100,
			"atk": 15,
			"def": 5,
			"ai_type": "quieto",
			"detection_zones": [],
			"attack_interval": 0.8,
			"attack_speed_pct": 100,
			"knockback_resistance": 0,
			"element_id": 2,
			"element_key": "fuego",
			"drops": [],
			"sprite_data": _make_enemy_attack_sprite_data(200.0, 0.0, 2)
		},
		{
			"id": 4,
			"name": "Trasgo Empuje",
			"level": 1,
			"exp": 10,
			"gold": 5,
			"hp": 100,
			"atk": 15,
			"def": 5,
			"ai_type": "quieto",
			"detection_zones": [],
			"attack_interval": 0.8,
			"attack_speed_pct": 100,
			"knockback_resistance": 0,
			"drops": [],
			"sprite_data": _make_enemy_attack_sprite_data(100.0, 48.0, 0)
		}
	])

	# ---------------------------------------------------------
	# 2. ENEMIGO ATACA, FORMULA, RESISTENCIA ACTOR, HITBOX MULT Y RETROCESO
	# ---------------------------------------------------------
	print("--- 2. Enemigo ataca al jugador: formula, resistencia elemental, hitbox mult y retroceso ---")
	# Colocar pared a la izquierda (2, 5) para comprobar colision del retroceso
	var map1 := make_test_map(dm, 16, 12, "map_3c2_combat", [Vector2i(2, 5)])
	get_root().add_child(map1)
	await step_frames(2)

	var player: ClockGearsPlayer = map1.get_runtime_player()
	check(player != null and player.is_arpg_active_here(), "Jugador ARPG instanciado y activo en mapa")
	player.set_grid_cell_immediate(Vector2i(5, 5), 6) # mirando derecha (hacia enemigo)

	# Enemigo a la derecha del jugador (6, 5), hitbox 100% y knockback 24px
	var enemy1 := _spawn_enemy(map1, 10, Vector2i(6, 5), 4, 1)
	await step_frames(1)

	var hp_before: int = dm.get_actor_runtime_hp(1)
	var pos_before: Vector2 = player.get_position()
	enemy1.start_arpg_attack(player)
	await step_frames(16)
	var hp_after_hit1: int = dm.get_actor_runtime_hp(1)
	var dmg1: int = hp_before - hp_after_hit1
	# Base: 15*4 - 10*2 = 40 (±20% varianza -> [32, 48])
	check(dmg1 >= 30 and dmg1 <= 52, "Golpe enemigo aplica formula ATK/DEF y 1 solo golpe por swing (dmg=%d)" % dmg1)
	check(player.get_position().x < pos_before.x - 4.0, "Retroceso empuja al jugador lejos del enemigo (dx=%.1f)" % (player.get_position().x - pos_before.x))

	# Comprobar que retroceso contra pared no atraviesa tile bloqueado (2,5)
	await step_frames(38) # esperar invuln
	player.set_grid_cell_immediate(Vector2i(3, 5), 6)
	var enemy_wall := _spawn_enemy(map1, 11, Vector2i(4, 5), 4, 4)
	enemy_wall.start_arpg_attack(player)
	await step_frames(14)
	check(player.get_position().x >= 88.0, "Retroceso respeta colision con pared en (2,5): x=%.1f >= 88.0" % player.get_position().x)
	enemy_wall.queue_free()

	# Comprobar resistencia elemental del actor (equipo ARPG con fuego=50) + hitbox 200%
	await step_frames(38)
	dm.set_actor_arpg_equip(1, "armadura", 1)
	dm.set_actor_runtime_hp(1, 200)
	player.set_grid_cell_immediate(Vector2i(7, 5), 6)
	var enemy_fire := _spawn_enemy(map1, 12, Vector2i(8, 5), 4, 3)
	enemy_fire.start_arpg_attack(player)
	await step_frames(14)
	var fire_dmg: int = 200 - dm.get_actor_runtime_hp(1)
	# Base ~40 * 2.0 hitbox * 0.5 fuego = ~40 ([32, 52])
	check(fire_dmg >= 30 and fire_dmg <= 52, "Resistencia elemental actor (50%%) + hitbox 200%% combinan correctamente (dmg=%d)" % fire_dmg)
	dm.set_actor_arpg_equip(1, "armadura", 0)

	# ---------------------------------------------------------
	# 3. INVULNERABILIDAD (0.5s y desactivada con 0.0)
	# ---------------------------------------------------------
	print("--- 3. Invulnerabilidad tras dano (0.5s vs 0.0) ---")
	await step_frames(38)
	dm.set_actor_runtime_hp(1, 200)
	player.set_grid_cell_immediate(Vector2i(7, 5), 6)
	enemy1.set_position(Vector2(8 * 32, 5 * 32))
	enemy_fire.set_position(Vector2(8 * 32, 5 * 32))

	enemy1.start_arpg_attack(player)
	await step_frames(2)
	check(player.is_arpg_invulnerable(), "Tras recibir dano el jugador queda invulnerable (0.5s)")
	var hp_mid: int = dm.get_actor_runtime_hp(1)
	enemy_fire.start_arpg_attack(player) # segundo enemigo ataca durante i-frames
	await step_frames(14)
	check(dm.get_actor_runtime_hp(1) == hp_mid, "Segundo golpe durante invulnerabilidad es ignorado (0 dano)")

	await step_frames(34) # supera 0.5s restantes
	check(not player.is_arpg_invulnerable(), "Invulnerabilidad expira tras 0.5s")

	# Configurar invulnerabilidad = 0 -> desactivada
	sys = dm.get_system_config()
	sys["arpg_invuln_seconds"] = 0.0
	dm.set_system_config(sys)
	dm.set_actor_runtime_hp(1, 200)
	enemy1.start_arpg_attack(player)
	await step_frames(2)
	check(not player.is_arpg_invulnerable(), "Con arpg_invuln_seconds = 0 no se activa invulnerabilidad")
	var hp_after_zero_first: int = dm.get_actor_runtime_hp(1)
	enemy_fire.start_arpg_attack(player)
	await step_frames(14)
	check(dm.get_actor_runtime_hp(1) < hp_after_zero_first, "Con invulnerabilidad desactivada (0) dos enemigos consecutivos hacen dano inmediato")

	sys["arpg_invuln_seconds"] = 0.5
	dm.set_system_config(sys)
	await step_frames(36)

	# ---------------------------------------------------------
	# 4. BLOQUEO (clic derecho, reduccion frontal, espalda, mirada fija y exclusion)
	# ---------------------------------------------------------
	print("--- 4. Bloqueo: frontal 50%, espalda sin bloqueo, mirada fija, exclusion atacar/bloquear ---")
	dm.set_actor_runtime_hp(1, 200)
	player.set_grid_cell_immediate(Vector2i(7, 5), 6) # mirando derecha
	enemy1.set_position(Vector2(8 * 32, 5 * 32)) # frente al jugador
	enemy1.start_arpg_attack(player) # golpe sin bloqueo de control
	await step_frames(36)

	# Activar bloqueo manteniendo clic derecho ("bloquear")
	_send_action_input("bloquear", true)
	await step_frames(2)
	check(player.is_arpg_blocking() and player.get_action_state() == "bloquear", "Mantener clic derecho ('bloquear') entra en estado bloquear")

	# Mirada fija mientras bloquea
	player.set_direction(4)
	check(player.get_direction() == 6, "La mirada queda fija mientras se bloquea")

	# No se puede atacar y bloquear a la vez
	_send_action_input("atacar", true)
	await step_frames(1)
	check(player.get_action_state() == "bloquear", "No se puede atacar mientras se esta bloqueando")

	# Golpe frontal bloqueado (~50% de 40 -> ~20, max <= 25)
	dm.set_actor_runtime_hp(1, 200)
	enemy1.start_arpg_attack(player)
	await step_frames(14)
	var frontal_block_dmg: int = 200 - dm.get_actor_runtime_hp(1)
	check(frontal_block_dmg >= 14 and frontal_block_dmg <= 26, "Bloqueo frontal reduce el dano ~50%% (dmg=%d)" % frontal_block_dmg)

	# Golpe por la espalda NO se bloquea
	await step_frames(36)
	_send_action_input("bloquear", false)
	await step_frames(1)
	player.set_grid_cell_immediate(Vector2i(7, 5), 6) # mirando derecha (x=224)
	_send_action_input("bloquear", true)
	await step_frames(2)
	dm.set_actor_runtime_hp(1, 200)
	enemy1.set_position(Vector2(6 * 32, 5 * 32)) # detras del jugador que mira a derecha (192 < 224)
	enemy1.start_arpg_attack(player)
	await step_frames(14)
	var back_dmg: int = 200 - dm.get_actor_runtime_hp(1)
	check(back_dmg >= 30 and back_dmg <= 52, "Golpe por la espalda NO se bloquea aunque mantenga bloqueo (dmg=%d)" % back_dmg)

	# Soltar bloqueo -> termina bloquear
	_send_action_input("bloquear", false)
	await step_frames(2)
	check(not player.is_arpg_blocking() and player.get_action_state() != "bloquear", "Al soltar clic derecho termina el bloqueo")

	# Mientras ataca, no puede bloquear ni esquivar
	await step_frames(36)
	_send_action_input("atacar", true)
	await step_frames(1)
	check(player.get_action_state() == "atacar", "Ataque iniciado correctamente")
	_send_action_input("bloquear", true)
	await step_frames(1)
	check(player.get_action_state() == "atacar" and not player.is_arpg_blocking(), "No se puede bloquear en medio de un ataque")
	_send_action_input("bloquear", false)
	var dodge_mid_atk: bool = player.try_trigger_dodge(0)
	check(not dodge_mid_atk and not player.is_arpg_dodging(), "No se puede esquivar en medio de un ataque")
	await step_frames(30) # esperar fin de ataque

	# ---------------------------------------------------------
	# 5. ESQUIVAR: voltereta (invulnerable) vs dash (vulnerable), colision y cooldown
	# ---------------------------------------------------------
	print("--- 5. Esquivar: voltereta vs dash, colisiones, animacion y cooldown ---")
	dm.set_actor_runtime_hp(1, 200)
	player.set_grid_cell_immediate(Vector2i(6, 5), 6)
	enemy1.set_position(Vector2(7 * 32, 5 * 32))

	# Voltereta via tecla Space ("esquivar")
	_send_action_input("esquivar", true)
	await step_frames(1)
	check(player.is_arpg_dodging() and player.is_arpg_dodge_invulnerable(), "Voltereta con Space activa esquivar + invulnerabilidad")
	check(player.get_action_state() == "esquivar", "Esquivar reproduce ranura 'esquivar'")
	enemy1.start_arpg_attack(player)
	await step_frames(6)
	check(dm.get_actor_runtime_hp(1) == 200, "Voltereta otorga invulnerabilidad frente a ataque enemigo")
	await step_frames(18) # terminar voltereta (0.25s = 15f)
	check(not player.is_arpg_dodging(), "Voltereta termina tras duracion configurada")
	check(player.get_arpg_dodge_cooldown_remaining() > 0.05, "Cooldown activo tras esquivar")
	check(not player.try_trigger_dodge(0), "No se puede volver a esquivar durante el cooldown")

	# Esperar fin de cooldown (0.4s = 24f) y comprobar dash (sin invulnerabilidad)
	await step_frames(28)
	sys = dm.get_system_config()
	sys["arpg_dodge_type"] = "dash"
	sys["arpg_dodge_duration"] = 0.25
	sys["arpg_dodge_distance"] = 96.0
	dm.set_system_config(sys)

	player.set_grid_cell_immediate(Vector2i(6, 5), 6)
	enemy1.set_position(Vector2(7 * 32, 5 * 32))
	dm.set_actor_runtime_hp(1, 200)
	check(player.try_trigger_dodge(0), "Esquivar tipo 'dash' se activa al acabar el cooldown")
	check(player.is_arpg_dodging() and not player.is_arpg_dodge_invulnerable(), "Tipo 'dash' NO otorga invulnerabilidad")
	enemy1.start_arpg_attack(player)
	await step_frames(6)
	check(dm.get_actor_runtime_hp(1) < 200, "En tipo 'dash' el enemigo si puede danar al jugador")
	await step_frames(38)

	# Esquivar hacia pared en (2, 5) respeta colisiones + fallback sin ranura esquivar_<dir>
	var a_rec_fb: Dictionary = dm.get_record_by_id(0, 1)
	a_rec_fb["sprite_data"] = _make_actor_sprite_data(false)
	dm.update_record(0, 1, a_rec_fb)
	player.load_actor_data(1)
	await step_frames(2)

	player.set_grid_cell_immediate(Vector2i(3, 5), 4) # mirando hacia pared en (2,5)
	check(player.try_trigger_dodge(4), "Esquivar funciona sin errores usando fallback cuando falta ranura esquivar_<dir>")
	await step_frames(18)
	check(player.get_position().x >= 88.0, "Esquivar respeta colision con pared en (2,5): x=%.1f >= 88.0" % player.get_position().x)
	await step_frames(36)

	# ---------------------------------------------------------
	# 6. MUERTE DEL JUGADOR: morir, bloqueo de controles, senal y reinicio de mapa
	# ---------------------------------------------------------
	print("--- 6. Muerte del jugador: morir, bloqueo, senal jugador_muerto y reinicio ---")
	death_signals = 0
	restart_signals = 0
	player.connect("jugador_muerto", Callable(self, "_on_player_died"))
	map1.connect("map_restarted", Callable(self, "_on_map_restarted"))

	dm.set_actor_runtime_hp(1, 30)
	player.set_grid_cell_immediate(Vector2i(10, 5), 6) # lejos de start (3, 3)
	var killer := _spawn_enemy(map1, 20, Vector2i(11, 5), 4, 2)
	killer.start_arpg_attack(player)
	await step_frames(2)

	check(dm.get_actor_runtime_hp(1) == 0, "Golpe letal deja HP del jugador en 0")
	check(player.is_arpg_dying(), "Jugador entra en estado muriendo")
	check(player.get_action_state() == "morir", "Muerte reproduce animacion 'morir'")
	check(not player.is_arpg_action_allowed(), "Controles ARPG bloqueados al morir")
	check(not player.try_step(6, false), "Movimiento bloqueado al morir")
	check(not player.try_trigger_dodge(6), "Esquivar bloqueado al morir")
	check(death_signals == 1, "Senal 'jugador_muerto' emitida exactamente 1 vez")

	# Esperar fin de animacion 'morir' (2 frames x 0.06s = 0.12s ~ 8 frames) + reinicio
	await step_frames(18)
	check(restart_signals >= 1, "Mapa reiniciado tras la muerte del jugador")
	check(player.is_arpg_alive() and not player.is_arpg_dead() and not player.is_arpg_dying(), "Jugador vivo tras reinicio de mapa")
	check(player.get_grid_cell() == Vector2i(3, 3), "Jugador reaparece en posicion inicial del mapa (3,3), actual=%s" % str(player.get_grid_cell()))
	check(dm.get_actor_runtime_hp(1) == 200, "HP restaurado al maximo tras reinicio (%d)" % dm.get_actor_runtime_hp(1))

	get_root().remove_child(map1)
	map1.queue_free()
	await step_frames(2)

	for tmpf in ["res://tmp_t3c2_tileset.png", "res://tmp_t3c2_sprite.png"]:
		if FileAccess.file_exists(tmpf):
			DirAccess.remove_absolute(ProjectSettings.globalize_path(tmpf))

	print("=== RESULTADO TANDA 3C-2: PASS=%d FAIL=%d ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
