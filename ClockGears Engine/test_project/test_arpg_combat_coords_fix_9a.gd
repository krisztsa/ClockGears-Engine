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

func _make_real_map(dm, w: int = 30, h: int = 22, map_id: String = "fix9a_map") -> ClockGearsMap:
	var png_path := "user://fix9a_tileset.png"
	if not FileAccess.file_exists(png_path):
		var timg := Image.create(64, 32, false, Image.FORMAT_RGBA8)
		timg.fill(Color(0.25, 0.55, 0.30, 1.0))
		timg.save_png(ProjectSettings.globalize_path(png_path))

	var ts_rec := {
		"id": 1,
		"name": "Fix9ATS",
		"slots": {"A5": png_path},
		"slot_collisions": {
			"A5": {
				"0,0": {"passage": 15, "layer": 0, "mode": 0},
				"1,0": {"passage": 0, "layer": 0, "mode": 1},
			}
		}
	}
	dm.set_category_array(13, [ts_rec])

	var m := ClockGearsMap.new()
	m.set_map_size(Vector2i(w, h))
	m.set_grid_resolution(32)
	m.set_tileset_id(1)
	m.set_map_state_id(map_id)
	m.set_combat_mode(1)
	m.ensure_default_layers()

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
		var layer := m.get_layer(ly) as TileMapLayer
		if layer:
			layer.set_tile_set(ts)

	var ground := m.get_layer(0) as TileMapLayer
	for y in range(h):
		for x in range(w):
			ground.set_cell(Vector2i(x, y), 0, Vector2i(0, 0))

	m.invalidate_passability_cache()
	return m

func _init() -> void:
	var wd := create_timer(45.0)
	wd.timeout.connect(func():
		printerr("[TIMEOUT] test_arpg_combat_coords_fix_9a excedio 45s.")
		quit(2)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== Test Suite 9A: ARPG Coordenadas, F3 Camara/Zoom, Hitboxes Defecto y Migracion ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var old_actors: Array = dm.get_category_array(0).duplicate(true)
	var old_ts: Array = dm.get_category_array(13).duplicate(true)
	var old_enemies: Array = dm.get_category_array(20).duplicate(true)
	var old_sys: Dictionary = dm.get_system_config().duplicate(true)

	# ---------------------------------------------------------
	# 1. Cuerpo por defecto nuevo vs conservacion + Migracion (d)/(e)
	# ---------------------------------------------------------
	print("--- 1. Cuerpo por defecto nuevo vs guardado + Migracion segura ---")
	var new_actor := ClockGearsDataManager.get_normalized_record(0, {"id": 1, "name": "NuevoActor"})
	var new_b: Dictionary = new_actor.get("body_shape", {})
	check(new_b.get("type", "") == "circle" and is_equal_approx(float(new_b.get("radius", 0.0)), 10.0) and is_equal_approx(float(new_b.get("offset_y", 0.0)), 6.0),
		"Entidad nueva usa cuerpo por defecto circular a los pies r=10 off=(0,6)")

	var saved_actor := ClockGearsDataManager.get_normalized_record(0, {
		"id": 2,
		"name": "ActorGuardado",
		"body_shape": {"type": "circle", "radius": 19.0, "offset_x": -1.0, "offset_y": 3.0}
	})
	var saved_b: Dictionary = saved_actor.get("body_shape", {})
	check(is_equal_approx(float(saved_b.get("radius", 0.0)), 19.0) and is_equal_approx(float(saved_b.get("offset_x", 0.0)), -1.0),
		"Cuerpo ya guardado por el creador no se sobrescribe")

	var old_unmigrated := ClockGearsDataManager.get_normalized_record(20, {
		"id": 1, "name": "OldSlime", "ai_type": "persigue_y_ataca",
		"vision_radius": 5.0, "attack_range": 1.5, "attack_interval": 0.9
	})
	var mz0: Array = old_unmigrated.get("detection_zones", [])
	check(mz0.size() == 2 and mz0[0]["name"] == "Visión" and mz0[1]["name"] == "Ataque" and mz0[1]["trigger"] == "while_inside" and is_equal_approx(float(mz0[1]["interval_sec"]), 0.9),
		"Migracion desde 0 crea 'Visión' (enter) y 'Ataque' (while_inside, interval=attack_interval)")

	var prev_migrated_unedited := ClockGearsDataManager.get_normalized_record(20, {
		"id": 2, "name": "MigSinEditar", "ai_type": "persigue_y_ataca",
		"vision_radius": 6.0, "attack_range": 1.0, "attack_interval": 1.2,
		"detection_zones": [
			{"name": "Ataque", "target_filter": "player", "trigger": "enter", "interval_sec": 0.5, "actions": [{"type": "atacar"}], "shape": {"type": "circle", "radius": 32.0, "offset_x": 0.0, "offset_y": 0.0}},
			{"name": "Zona 2", "target_filter": "player", "trigger": "enter", "interval_sec": 0.5, "actions": [{"type": "perseguir"}], "shape": {"type": "circle", "radius": 192.0, "offset_x": 0.0, "offset_y": 0.0}},
		]
	})
	var mz1: Array = prev_migrated_unedited.get("detection_zones", [])
	check(mz1[0]["trigger"] == "while_inside" and is_equal_approx(float(mz1[0]["interval_sec"]), 1.2) and mz1[1]["name"] == "Visión",
		"Entrada ya migrada sin editar actualiza 'Ataque' a while_inside=1.2 y renombra 'Zona 2' a 'Visión'")

	var creator_edited := ClockGearsDataManager.get_normalized_record(20, {
		"id": 3, "name": "CanonEditado", "ai_type": "persigue_y_ataca",
		"vision_radius": 6.0, "attack_range": 1.0, "attack_interval": 1.5,
		"detection_zones": [
			{"name": "Ataque", "target_filter": "player", "trigger": "enter", "interval_sec": 1.5, "actions": [{"type": "atacar"}], "shape": {"type": "circle", "radius": 43.0, "offset_x": 3.0, "offset_y": 16.0}},
			{"name": "Zona 2", "target_filter": "player", "trigger": "while_inside", "interval_sec": 0.5, "actions": [{"type": "perseguir"}], "shape": {"type": "circle", "radius": 106.0, "offset_x": 2.0, "offset_y": 11.0}},
		]
	})
	var mz2: Array = creator_edited.get("detection_zones", [])
	check(mz2[0]["trigger"] == "enter" and is_equal_approx(float(mz2[0]["shape"]["radius"]), 43.0) and mz2[1]["name"] == "Zona 2" and is_equal_approx(float(mz2[1]["shape"]["radius"]), 106.0),
		"Zonas editadas por el creador en 'Ataque' y 'Zona 2' quedan intactas")

	# ---------------------------------------------------------
	# 2. Configuracion runtime: Camara desplazada + Zoom 2x/3x + Resoluciones
	# ---------------------------------------------------------
	var p_sheet_path := ProjectSettings.globalize_path("user://fix9a_p32.png")
	var img_p := Image.create(32, 32, false, Image.FORMAT_RGBA8)
	img_p.fill(Color(0.2, 0.5, 1.0, 1.0))
	img_p.save_png(p_sheet_path)

	var e_sheet_path := ProjectSettings.globalize_path("user://fix9a_e64.png")
	var img_e := Image.create(64, 64, false, Image.FORMAT_RGBA8)
	img_e.fill(Color(0.8, 0.3, 0.2, 1.0))
	img_e.save_png(e_sheet_path)

	var actor_rec := {
		"id": 1,
		"name": "FixKris",
		"hp": 1000, "max_hp": 1000, "atk": 20, "def": 10,
		"sprite_data": {
			"frame_w": 32, "frame_h": 32, "cols": 1, "rows": 1,
			"hurtbox": {"enabled": true, "damage_mult_pct": 100.0, "shape": {"type": "rect", "size_x": 32.0, "size_y": 32.0, "offset_x": 0.0, "offset_y": 0.0}},
			"animations": {
				"idle": {"name": "idle", "spritesheet_path": p_sheet_path, "cols": 1, "rows": 1, "frame_w": 32, "frame_h": 32, "fps": 6.0, "loop": true, "frames": [0], "frames_data": [{"sheet": p_sheet_path, "x": 0, "y": 0, "w": 32, "h": 32}]},
				"atacar_derecha": {"name": "atacar_derecha", "spritesheet_path": p_sheet_path, "cols": 2, "rows": 1, "frame_w": 32, "frame_h": 32, "fps": 12.0, "loop": false, "frames": [0, 1], "frames_data": [
					{"sheet": p_sheet_path, "x": 0, "y": 0, "w": 32, "h": 32, "hitbox": {"damage_mult_pct": 100.0, "knockback": 0.0, "hitstop_ms": 0, "shapes": []}},
					{"sheet": p_sheet_path, "x": 0, "y": 0, "w": 32, "h": 32, "hitbox": {"damage_mult_pct": 100.0, "knockback": 0.0, "hitstop_ms": 0, "shapes": []}}
				]}
			}
		}
	}

	# Enemigo 64x64 SIN ranura atacar_<dir> propia (solo idle)
	var enemy_rec := {
		"id": 1,
		"name": "FixCanon64",
		"hp": 500, "atk": 5, "def": 0,
		"ai_type": "persigue_y_ataca",
		"attack_interval": 0.35,
		"body_shape": {"type": "circle", "radius": 14.0, "offset_x": 0.0, "offset_y": 16.0},
		"detection_zones": [
			{"name": "Ataque", "target_filter": "player", "trigger": "while_inside", "interval_sec": 0.35, "actions": [{"type": "atacar"}], "shape": {"type": "circle", "radius": 44.0, "offset_x": 0.0, "offset_y": 0.0}},
			{"name": "Visión", "target_filter": "player", "trigger": "enter", "interval_sec": 0.5, "actions": [{"type": "perseguir"}], "shape": {"type": "circle", "radius": 128.0, "offset_x": 0.0, "offset_y": 0.0}},
		],
		"sprite_data": {
			"frame_w": 64, "frame_h": 64, "cols": 1, "rows": 1,
			"hurtbox": {"enabled": true, "damage_mult_pct": 100.0, "shape": {"type": "rect", "size_x": 36.0, "size_y": 36.0, "offset_x": 0.0, "offset_y": 0.0}},
			"animations": {
				"idle": {"name": "idle", "spritesheet_path": e_sheet_path, "cols": 1, "rows": 1, "frame_w": 64, "frame_h": 64, "fps": 8.0, "loop": true, "frames": [0], "frames_data": [{"sheet": e_sheet_path, "x": 0, "y": 0, "w": 64, "h": 64}]}
			}
		}
	}

	dm.set_category_array(0, [actor_rec])
	dm.set_category_array(20, [enemy_rec])
	dm.reset_runtime_state()

	var sys: Dictionary = old_sys.duplicate(true)
	sys["combat_mode"] = 1
	sys["party"] = [1]
	sys["has_start_pos"] = true
	sys["start_x"] = 10
	sys["start_y"] = 8
	sys["start_dir"] = 6
	sys["camera_zoom"] = 2
	sys["camera_smooth"] = false
	sys["camera_clamp"] = true
	sys["camera_follow_player"] = true
	sys["arpg_invuln_seconds"] = 0.05
	sys["arpg_show_damage_numbers"] = true
	dm.set_system_config(sys)

	var test_configs := [
		{"res": Vector2i(960, 540), "zoom": 2},
		{"res": Vector2i(1366, 768), "zoom": 2},
		{"res": Vector2i(1920, 1080), "zoom": 3},
	]

	for cfg in test_configs:
		var res: Vector2i = cfg["res"]
		var zm: int = cfg["zoom"]
		sys["camera_zoom"] = zm
		dm.set_system_config(sys)
		dm.set_actor_runtime_hp(1, 200)

		var sv := SubViewport.new()
		sv.size = res
		sv.disable_3d = true
		if res != Vector2i(960, 540):
			sv.size_2d_override = Vector2i(960, 540)
			sv.size_2d_override_stretch = true
		root.add_child(sv)

		var m2 := _make_real_map(dm, 30, 22, "fix9a_%dx%d" % [res.x, zm])
		var ev_cont := m2.get_node_or_null("Eventos") as Node2D
		if ev_cont == null:
			ev_cont = Node2D.new()
			ev_cont.set_name("Eventos")
			m2.add_child(ev_cont)

		var ev2 := ClockGearsEvent.new()
		ev2.set_event_id(1)
		ev2.set_event_name("Canon64")
		ev2.position = Vector2(11 * 32, 8 * 32)
		ev2.set_behavior_type(3)
		ev2.set_behavior_ref_id(1)
		ev2.set_direction(4)
		ev_cont.add_child(ev2)

		sv.add_child(m2)
		await process_frame
		await physics_frame
		await process_frame

		var pl := m2.get_runtime_player()
		var cam := m2.get_runtime_camera()
		var dbg := m2.get_node_or_null("RuntimeDebugOverlay") as ClockGearsDebugOverlay
		check(cam != null and cam.position.length() > 100.0 and is_equal_approx(cam.zoom.x, float(zm)),
			"[%dx%d z=%d] Camara desplazada y con zoom %dx" % [res.x, res.y, zm, zm])

		# (a) Unificacion de origen: sprite 64x64 centro == body origin == zones_root == hurtbox/hitbox center
		var ev_sprite: Sprite2D = null
		for child in ev2.get_children():
			if child is Sprite2D:
				ev_sprite = child
				break
		check(ev_sprite != null, "[%dx%d z=%d] Evento tiene Sprite2D hijo" % [res.x, res.y, zm])
		var expected_ev_local := (ev_sprite.position + Vector2(32, 32)) if ev_sprite != null else Vector2.ZERO # (16, 0) en sprite 64x64 sobre tile 32x32
		var e_body_node := ev2.get_entity_body()
		var e_zones_node := ev2.get_node_or_null("RuntimeDetectionZones") as Node2D
		check(ev2.get_body_local_origin().distance_to(expected_ev_local) < 0.01 and ev2.get_arpg_world_center().distance_to(ev2.get_body_world_origin()) < 0.01,
			"[%dx%d z=%d] Origen unico sprite64 == body_origin == arpg_world_center (%s)" % [res.x, res.y, zm, str(ev2.get_body_local_origin())])
		check(e_body_node != null and e_body_node.position.distance_to(expected_ev_local) < 0.01 and e_zones_node != null and e_zones_node.position.distance_to(expected_ev_local) < 0.01,
			"[%dx%d z=%d] Physics body y zonas en origen del sprite (%s)" % [res.x, res.y, zm, str(expected_ev_local)])

		# (b) F3 conversor world_to_screen coincide 1:1 con canvas_transform (camara+zoom+viewport) y nuevo check Hitboxes/Hurtboxes
		if dbg:
			dbg.set_show_events(true)
			dbg.set_show_bodies_zones(true)
			dbg.set_show_hitboxes_hurtboxes(true)
			dbg.set_show_damage_numbers(true)
			check(dbg.is_show_hitboxes_hurtboxes(), "[%dx%d z=%d] F3 checkbox 'Mostrar hitboxes y hurtboxes' activo" % [res.x, res.y, zm])
			var wc := dbg.get_world_canvas()
			var cxf: Transform2D = m2.get_viewport().get_canvas_transform()
			var pt_world := ev2.get_body_world_origin()
			var s_wc: Vector2 = wc.world_to_screen(pt_world)
			var s_expected: Vector2 = cxf * pt_world
			check(s_wc.distance_to(s_expected) < 0.05,
				"[%dx%d z=%d] world_to_screen coincide con canvas_transform cam+zoom (%s vs %s)" % [res.x, res.y, zm, str(s_wc), str(s_expected)])

		# (c) Jugador con slot atacar_derecha SIN hitboxes dibujadas ataca y hace daño con hitbox por defecto + F3 dmg number
		var def_hb := ClockGearsShapeGeometry.make_default_attack_hitbox_shape(6, 32)
		check(def_hb.get("type", "") == "rect" and is_equal_approx(float(def_hb.get("size_x", 0.0)), 32.0) and is_equal_approx(float(def_hb.get("offset_x", 0.0)), 24.0),
			"[%dx%d] Hitbox por defecto derecha es rect 32x32 en offset (24,0)" % [res.x, res.y])

		if res == Vector2i(960, 540):
			var hp_before := ev2.get_arpg_hp()
			pl.set_action_state("atacar")
			for _i in range(20):
				await physics_frame
				await process_frame
			check(ev2.get_arpg_hp() < hp_before,
				"Jugador sin hitboxes en atacar_derecha golpea con hitbox por defecto (%d -> %d)" % [hp_before, ev2.get_arpg_hp()])
			check(dbg.get_recent_damage_numbers().size() > 0, "F3 registra numero de daño flotante tras golpe")

			# No sobrescribir hitboxes dibujadas: frame 0 vacio, frame 1 con hitbox dibujada muy lejana
			var a_custom := actor_rec.duplicate(true)
			a_custom["sprite_data"]["animations"]["atacar_derecha"]["frames_data"][0]["duration"] = 0.30
			a_custom["sprite_data"]["animations"]["atacar_derecha"]["frames_data"][0]["hitbox"]["shapes"] = []
			a_custom["sprite_data"]["animations"]["atacar_derecha"]["frames_data"][1]["duration"] = 0.20
			a_custom["sprite_data"]["animations"]["atacar_derecha"]["frames_data"][1]["hitbox"]["shapes"] = [{"type": "rect", "size_x": 16.0, "size_y": 16.0, "offset_x": 300.0, "offset_y": 0.0}]
			dm.set_category_array(0, [a_custom])
			pl.load_actor_data(1)
			var hp_no_ow := ev2.get_arpg_hp()
			pl.set_action_state("atacar")
			for _i in range(6):
				await physics_frame
			check(ev2.get_arpg_hp() == hp_no_ow,
				"Con hitbox dibujada en frame 1, frame 0 vacio no inyecta hitbox por defecto")

			# Esperar a que termine el swing antes de restaurar el actor
			for _w in range(35):
				await physics_frame

			# (c) Enemigo SIN ranura atacar_* propia ataca periodicamente respetando attack_interval
			dm.set_category_array(0, [actor_rec])
			pl.load_actor_data(1)
			ev2.position = Vector2(11 * 32, 8 * 32)
			var pl_hp_0 := pl.get_arpg_hp()
			for _f in range(45): # ~0.75s > interval 0.35s y invuln 0.5s
				await physics_frame
			var pl_hp_1 := pl.get_arpg_hp()
			check(pl_hp_1 < pl_hp_0, "Enemigo sin ranura atacar_* ataca con hitbox por defecto (HP %d -> %d)" % [pl_hp_0, pl_hp_1])
			for _f in range(45):
				await physics_frame
			var pl_hp_2 := pl.get_arpg_hp()
			check(pl_hp_2 < pl_hp_1, "Enemigo repite ataque respetando while_inside/attack_interval (HP %d -> %d)" % [pl_hp_1, pl_hp_2])

			# Colision en hueco 1 casilla (32 px): default (r=10, off=(0,6)), r=14 pasan; r=19 bloqueado
			var lg := m2.get_layer(0) as TileMapLayer
			lg.set_cell(Vector2i(15, 6), 0, Vector2i(1, 0))
			lg.set_cell(Vector2i(15, 7), 0, Vector2i(1, 0))
			lg.set_cell(Vector2i(15, 9), 0, Vector2i(1, 0))
			lg.set_cell(Vector2i(15, 10), 0, Vector2i(1, 0))
			m2.invalidate_passability_cache()

			pl.set_grid_cell_immediate(Vector2i(14, 8), 6)
			pl.set_body_shape({"type": "circle", "radius": 19.0, "offset_x": -1.0, "offset_y": 3.0})
			for _k in range(20):
				pl.move_with_collision(Vector2(4, 0))
			check(pl.position.x < 15 * 32, "Cuerpo r=19 queda bloqueado en hueco de 32px (x=%.1f)" % pl.position.x)

			pl.set_grid_cell_immediate(Vector2i(14, 8), 6)
			pl.set_body_shape({"type": "circle", "radius": 10.0, "offset_x": 0.0, "offset_y": 6.0})
			for _k in range(20):
				pl.move_with_collision(Vector2(4, 0))
			check(pl.position.x >= 15 * 32, "Cuerpo por defecto r=10 off=(0,6) cruza hueco de 32px (x=%.1f)" % pl.position.x)

		sv.queue_free()
		await process_frame

	dm.set_category_array(0, old_actors)
	dm.set_category_array(13, old_ts)
	dm.set_category_array(20, old_enemies)
	dm.set_system_config(old_sys)
	for fn in ["user://fix9a_tileset.png", "user://fix9a_p32.png", "user://fix9a_e64.png"]:
		var gp := ProjectSettings.globalize_path(fn)
		if FileAccess.file_exists(fn):
			DirAccess.remove_absolute(gp)

	print("RESUMEN test_arpg_combat_coords_fix_9a: %d PASS, %d FAIL" % [passed, failed])
	quit(0 if failed == 0 else 1)
