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

func _cleanup_dir(abs_path: String) -> void:
	var da := DirAccess.open(abs_path)
	if da == null:
		return
	da.list_dir_begin()
	var fn := da.get_next()
	while fn != "":
		if fn != "." and fn != "..":
			var sub := abs_path.path_join(fn)
			if da.current_is_dir():
				_cleanup_dir(sub)
			else:
				DirAccess.remove_absolute(sub)
		fn = da.get_next()
	da.list_dir_end()
	DirAccess.remove_absolute(abs_path)

func _initialize() -> void:
	# Watchdog hard-exit para que el proceso headless nunca quede colgado.
	create_timer(45.0).timeout.connect(func():
		printerr("[TIMEOUT] Test abortado tras 45s")
		quit(2)
	)
	_run_tests()
	print("[SUMMARY] passed=", passed, " failed=", failed)
	quit(0 if failed == 0 else 1)

func _run_tests() -> void:
	var reg := ClockGearsCharacterStateRegistry
	var geom := ClockGearsShapeGeometry
	var dm := ClockGearsDataManager

	# =========================================================================
	# 1. Ranuras por categoria (4 y 8 direcciones + ocultacion por turnos)
	# =========================================================================
	var act_arpg4 := Array(reg.get_category_slot_ids_by_key("actors", 4, false, 1))
	check(act_arpg4.has("mover_abajo") and act_arpg4.has("correr_abajo"), "Actors ARPG tiene mover y correr")
	check(act_arpg4.has("esquivar_abajo") and act_arpg4.has("atacar_abajo") and act_arpg4.has("bloquear_abajo"), "Actors ARPG tiene esquivar, atacar, bloquear")
	check(act_arpg4.has("golpeado") and act_arpg4.has("morir"), "Actors ARPG tiene golpeado y morir sin direccion")
	check(not act_arpg4.has("esquivar_arriba_derecha"), "Actors 4 dirs no incluye diagonales")

	var act_arpg8 := Array(reg.get_category_slot_ids_by_key("actors", 8, true, 1))
	check(act_arpg8.has("esquivar_arriba_derecha") and act_arpg8.has("correr_abajo_izquierda") and act_arpg8.has("quieto_arriba_derecha"), "Actors 8 dirs + dir_idle incluye diagonales")

	var npc_arpg4 := Array(reg.get_category_slot_ids_by_key("npcs", 4, false, 1))
	check(npc_arpg4.has("correr_derecha") and not npc_arpg4.has("esquivar_derecha") and not npc_arpg4.has("golpeado") and not npc_arpg4.has("morir"), "NPC tiene correr y no esquivar/golpeado/morir")

	var arpg_npc4 := Array(reg.get_category_slot_ids_by_key("arpg_npcs", 4, false, 1))
	check(arpg_npc4.has("correr_arriba") and arpg_npc4.has("atacar_arriba") and arpg_npc4.has("golpeado") and arpg_npc4.has("morir") and not arpg_npc4.has("esquivar_arriba"), "NPC ARPG tiene correr, atacar, golpeado, morir sin esquivar")

	var arpg_en4 := Array(reg.get_category_slot_ids_by_key("arpg_enemies", 4, false, 1))
	check(arpg_en4.has("correr_izquierda") and arpg_en4.has("atacar_izquierda") and arpg_en4.has("golpeado") and arpg_en4.has("morir"), "Enemigos ARPG tienen correr, atacar, golpeado y morir")

	var turn_act4 := Array(reg.get_category_slot_ids_by_key("actors", 4, false, 0))
	check(turn_act4.has("mover_abajo") and turn_act4.has("correr_abajo"), "Modo turnos mantiene mover y correr")
	check(not turn_act4.has("atacar_abajo") and not turn_act4.has("bloquear_abajo") and not turn_act4.has("esquivar_abajo") and not turn_act4.has("golpeado") and not turn_act4.has("morir"), "Modo turnos oculta atacar, bloquear, esquivar, golpeado y morir")
	check(reg.is_actor_fixed_directional_slot("esquivar_abajo") and reg.is_actor_fixed_directional_slot("golpeado") and reg.is_actor_fixed_directional_slot("morir"), "Slots fijos incluyen esquivar, golpeado y morir para no borrarse al ocultar")

	# =========================================================================
	# 2. Fallbacks de estados y extensibilidad del registro (sin volteo auto)
	# =========================================================================
	var sheet_min := {
		"frame_w": 32,
		"frame_h": 32,
		"animations": {
			"mover_abajo": {
				"name": "mover_abajo",
				"fps": 8.0,
				"loop": true,
				"frames_data": [{ "sheet": "res://fake.png", "x": 0, "y": 0, "w": 32, "h": 32 }]
			},
			"quieto_abajo": {
				"name": "quieto_abajo",
				"fps": 6.0,
				"loop": true,
				"frames_data": [{ "sheet": "res://fake.png", "x": 32, "y": 0, "w": 32, "h": 32 }]
			}
		}
	}
	var res_run := reg.resolve_state_animation_dict(sheet_min, false, "correr", 2)
	check(res_run.get("found", false) and res_run.get("resolved_slot", "") == "mover_abajo", "correr sin ranura cae a mover")
	var res_dodge := reg.resolve_state_animation_dict(sheet_min, false, "esquivar", 2)
	check(res_dodge.get("found", false) and res_dodge.get("resolved_slot", "") == "mover_abajo", "esquivar sin ranura cae a mover")
	var res_hit_nodir := reg.resolve_state_animation_dict(sheet_min, false, "golpeado", 2)
	check(res_hit_nodir.get("found", false) and res_hit_nodir.get("resolved_slot", "") == "mover_abajo" and res_hit_nodir.get("freeze_frame0", false), "golpeado sin ranura cae a idle (frame 0 mover cuando idle dir=false)")
	var res_die_diridle := reg.resolve_state_animation_dict(sheet_min, true, "morir", 2)
	check(res_die_diridle.get("found", false) and res_die_diridle.get("resolved_slot", "") == "quieto_abajo", "morir sin ranura cae a quieto_<dir> cuando idle dir=true")

	# Sin volteo automatico cuando falta derecha pero existe izquierda
	var res_missing_right := reg.resolve_state_animation_dict(sheet_min, false, "caminar", 6)
	check(not res_missing_right.get("found", false), "No voltea ni inventa sprites cuando falta una direccion cardinal")

	# Registro extensible de estado futuro (ej. saltar)
	reg.register_state("saltar", "saltar", false, "caminar", false, false, true)
	check(reg.has_state("saltar") and reg.resolve_slot_for_state("saltar", 6) == "saltar_derecha", "Estado extensible registrado correctamente")
	var res_jump_fb := reg.resolve_state_animation_dict(sheet_min, false, "saltar", 2)
	check(res_jump_fb.get("found", false) and res_jump_fb.get("resolved_slot", "") == "mover_abajo", "Estado extensible sin ranura usa fallback registrado")
	check(reg.resolve_slot_for_state("golpeado", 6) == "golpeado" and reg.resolve_slot_for_state("morir", 4) == "morir", "golpeado y morir son no direccionales")

	# =========================================================================
	# 3. Formas (circulo, rectangulo, capsula, poligono concavo/convexo) con Shape2D
	# =========================================================================
	var c_shape := geom.make_circle_shape(10.0, Vector2(0, 0))
	var r_shape := geom.make_rect_shape(Vector2(20, 10), Vector2(12, 0), 0.0)
	check(geom.shapes_overlap_world(c_shape, Vector2(100, 100), 2, r_shape, Vector2(105, 100), 2), "Circulo y rectangulo solapados en mundo")
	check(not geom.shapes_overlap_world(c_shape, Vector2(100, 100), 2, r_shape, Vector2(200, 100), 2), "Circulo y rectangulo separados en mundo")

	var cap_shape := geom.make_capsule_shape(5.0, 20.0, Vector2(0, 0), 90.0)
	check(geom.point_in_shape_world(Vector2(108, 100), cap_shape, Vector2(100, 100), 2), "Punto dentro de capsula rotada 90 deg")
	check(not geom.point_in_shape_world(Vector2(100, 108), cap_shape, Vector2(100, 100), 2), "Punto fuera del eje corto de capsula rotada 90 deg")

	# Poligono concavo en L
	var l_poly := geom.make_polygon_shape(PackedVector2Array([
		Vector2(0, 0), Vector2(20, 0), Vector2(20, 6), Vector2(6, 6), Vector2(6, 20), Vector2(0, 20)
	]))
	check(geom.point_in_shape_world(Vector2(103, 103), l_poly, Vector2(100, 100), 2), "Punto dentro de brazo solido del poligono concavo")
	check(not geom.point_in_shape_world(Vector2(115, 115), l_poly, Vector2(100, 100), 2), "Punto dentro de muesca concava queda fuera")

	# Transformacion mundo por direccion desde orientacion base (authored_dir=2 -> entity_dir=6)
	var dir_hb := geom.make_circle_shape(6.0, Vector2(0, 16))
	check(geom.point_in_shape_world(Vector2(116, 100), dir_hb, Vector2(100, 100), 6, 2), "Posicion mundo rota hitbox de abajo (0,16) hacia derecha (+16,0)")
	check(not geom.point_in_shape_world(Vector2(100, 116), dir_hb, Vector2(100, 100), 6, 2), "Al mirar a derecha ya no impacta abajo")

	# =========================================================================
	# 4. Hitboxes solo en atacar_*, Hurtbox por entidad/frame, Anclaje Simple, Copia y Swing
	# =========================================================================
	var raw_rec := {
		"id": 99,
		"name": "HeroeTest",
		"sprite_data": {
			"frame_w": 40,
			"frame_h": 48,
			"hurtbox": {
				"enabled": true,
				"damage_mult_pct": 100.0,
				"shape": geom.make_rect_shape(Vector2(24, 36), Vector2(0, -4))
			},
			"animations": {
				"mover_abajo": {
					"name": "mover_abajo",
					"fps": 8.0,
					"loop": true,
					"frames_data": [{
						"sheet": "res://hero.png", "x": 0, "y": 0, "w": 40, "h": 48,
						"hitbox": {
							"damage_mult_pct": 200.0,
							"knockback": 50.0,
							"hitstop_ms": 80,
							"shapes": [geom.make_circle_shape(12.0, Vector2(0, 16))]
						}
					}]
				},
				"atacar_abajo": {
					"name": "atacar_abajo",
					"fps": 10.0,
					"loop": false,
					"frames_data": [{
						"sheet": "res://hero_atk.png", "x": 0, "y": 48, "w": 40, "h": 48,
						"hitbox": {
							"damage_mult_pct": 150.0,
							"knockback": 24.0,
							"hitstop_ms": 60,
							"shapes": [geom.make_rect_shape(Vector2(20, 10), Vector2(0, 20), 0.0)]
						},
						"hurtbox_override": {
							"enabled": true,
							"damage_mult_pct": 125.0,
							"shape": geom.make_circle_shape(14.0, Vector2(0, 2))
						},
						"weapon_anchor": {
							"enabled": true, "x": 6.0, "y": 14.0, "rotation": 90.0
						}
					}]
				},
				"atacar_derecha": {
					"name": "atacar_derecha",
					"fps": 10.0,
					"loop": false,
					"frames_data": [{
						"sheet": "res://hero_atk_r.png", "x": 40, "y": 48, "w": 40, "h": 48
					}]
				},
				"atacar_izquierda": {
					"name": "atacar_izquierda",
					"fps": 10.0,
					"loop": false,
					"frames_data": [{
						"sheet": "res://hero_atk_l.png", "x": 80, "y": 48, "w": 40, "h": 48
					}]
				},
				"esquivar_abajo": {
					"name": "esquivar_abajo",
					"fps": 10.0,
					"loop": false,
					"frames_data": [{ "sheet": "res://hero.png", "x": 0, "y": 96, "w": 40, "h": 48 }]
				},
				"golpeado": {
					"name": "golpeado",
					"fps": 8.0,
					"loop": false,
					"frames_data": [{ "sheet": "res://hero.png", "x": 0, "y": 144, "w": 40, "h": 48 }]
				},
				"morir": {
					"name": "morir",
					"fps": 6.0,
					"loop": false,
					"frames_data": [{ "sheet": "res://hero.png", "x": 0, "y": 192, "w": 40, "h": 48 }]
				}
			}
		}
	}

	var norm_rec := dm.get_normalized_record(0, raw_rec) # CAT_ACTORS = 0
	var sp_norm: Dictionary = norm_rec["sprite_data"]
	var move_hb := geom.get_frame_hitbox_data(sp_norm, "mover_abajo", 0)
	check(Array(move_hb.get("shapes", [])).is_empty(), "Hitbox fuera de atacar_* es descartada automaticamente")

	var atk_hb := geom.get_frame_hitbox_data(sp_norm, "atacar_abajo", 0)
	check(Array(atk_hb.get("shapes", [])).size() == 1 and int(atk_hb.get("hitstop_ms", 0)) == 60 and is_equal_approx(float(atk_hb.get("knockback", 0.0)), 24.0), "Hitbox en atacar_* conserva formas, daño%, knockback e hitstop_ms")

	var eff_hb_move := geom.get_effective_hurtbox_for_frame(sp_norm, "mover_abajo", 0)
	var eff_hb_atk := geom.get_effective_hurtbox_for_frame(sp_norm, "atacar_abajo", 0)
	check(eff_hb_move.get("shape", {}).get("type", "") == "rect" and is_equal_approx(float(eff_hb_move.get("damage_mult_pct", 0.0)), 100.0), "Hurtbox base de entidad aplica en frames sin override")
	check(eff_hb_atk.get("shape", {}).get("type", "") == "circle" and is_equal_approx(float(eff_hb_atk.get("damage_mult_pct", 0.0)), 125.0), "Override por frame sustituye hurtbox base")

	# Copia de formas entre direcciones (abajo -> derecha por rotacion; derecha -> izquierda por espejo) sin tocar sprite
	var copied_sp := geom.copy_slot_shapes_between_directions(sp_norm, "atacar", 2, 6, "rotate", true, false)
	copied_sp = geom.copy_slot_shapes_between_directions(copied_sp, "atacar", 6, 4, "auto", true, false)
	var hb_right := geom.get_frame_hitbox_data(copied_sp, "atacar_derecha", 0)
	var s_r: Dictionary = hb_right["shapes"][0]
	check(is_equal_approx(float(s_r.get("offset_x", 0.0)), 20.0) and is_equal_approx(float(s_r.get("offset_y", 0.0)), 0.0), "Copiar atacar_abajo -> atacar_derecha rota offset (0,20) a (20,0)")
	var hb_left := geom.get_frame_hitbox_data(copied_sp, "atacar_izquierda", 0)
	var s_l: Dictionary = hb_left["shapes"][0]
	check(is_equal_approx(float(s_l.get("offset_x", 0.0)), -20.0), "Copiar atacar_derecha -> atacar_izquierda refleja forma en X a (-20,0)")
	var left_frame: Dictionary = copied_sp["animations"]["atacar_izquierda"]["frames_data"][0]
	check(left_frame.get("sheet", "") == "res://hero_atk_l.png" and int(left_frame.get("x", -1)) == 80, "Copiar formas NO modifica sheet ni region del sprite destino")

	# Comprobar impacto hitbox vs hurtbox con Check + SwingTracker (1 golpe por swing)
	check(geom.check_attack_hits_hurtbox(hb_right["shapes"], Vector2(100, 100), 6, eff_hb_move, Vector2(120, 100), 2), "Hitbox derecha impacta hurtbox del objetivo en mundo")
	var tracker := ClockGearsAttackSwingTracker.new()
	var swing_1 := tracker.start_new_swing()
	check(tracker.register_hit(swing_1, 501) == true, "Swing 1 registra primer golpe al objetivo 501")
	check(tracker.register_hit(swing_1, 501) == false, "Swing 1 bloquea segundo golpe al mismo objetivo 501")
	check(tracker.register_hit(swing_1, 502) == true, "Swing 1 si puede golpear a distinto objetivo 502")
	var swing_2 := tracker.start_new_swing()
	check(tracker.register_hit(swing_2, 501) == true, "Nuevo swing vuelve a poder golpear al objetivo 501")

	# Anclaje de arma solo en modo Simple
	var sword_simple := {
		"draw_mode": "simple",
		"simple_images": {
			"atacar": { "abajo": { "path": "res://sword.png", "offset_x": 1, "offset_y": 2, "behind_body": false } }
		}
	}
	var layer_simple := reg.resolve_arpg_piece_with_body_anchor(sword_simple, 22, "atacar", 2, 0, false, 4, sp_norm)
	check(layer_simple.get("visible", false) and layer_simple.get("has_anchor", false) and is_equal_approx(float(layer_simple.get("rotation", 0.0)), 90.0) and int(layer_simple.get("offset_x", 0)) == 7 and int(layer_simple.get("offset_y", 0)) == 16, "Modo Simple combina offset de pieza + anclaje y rotacion del frame")
	var sword_full := {
		"draw_mode": "completo",
		"sprite_data": sp_norm
	}
	var layer_full := reg.resolve_arpg_piece_with_body_anchor(sword_full, 22, "atacar", 2, 0, false, 4, sp_norm)
	check(not layer_full.get("has_anchor", true) and is_equal_approx(float(layer_full.get("rotation", 99.0)), 0.0), "Modo Completo ignora anclaje de arma Simple")

	# =========================================================================
	# 5. Migracion de entrada antigua + Turnos oculta sin borrar + Maniqui no exporta
	# =========================================================================
	var tmp_dir := "res://tmp_cg_test_9a_data"
	var abs_tmp := ProjectSettings.globalize_path(tmp_dir)
	DirAccess.make_dir_recursive_absolute(abs_tmp)

	dm.init_empty()
	check(dm.get_mannequin_size() == 32, "Maniqui por defecto es 32x32")
	dm.set_mannequin_size(64)
	check(dm.get_mannequin_size() == 64, "Maniqui acepta 64x64")
	dm.set_mannequin_size(128)
	check(dm.get_mannequin_size() == 32, "Maniqui invalido normaliza a 32x32")
	dm.set_mannequin_size(48)

	var sys_cfg := dm.get_system_config()
	sys_cfg["combat_mode"] = 0 # Modo turnos activo: ranuras ARPG ocultas en UI pero datos NO borrados
	dm.set_system_config(sys_cfg)

	var exported_sys := dm.get_exported_system_config()
	check(not exported_sys.has("mannequin_size") and int(dm.get_mannequin_size()) == 48, "Maniqui se excluye de la config exportada mientras el proyecto conserva 48")

	# Entrada antigua sin hurtbox ni hitbox/weapon_anchor en frame, tamaño 48x64
	var old_actor := {
		"id": 1,
		"name": "KrisOld",
		"sprite_data": {
			"frame_w": 48,
			"frame_h": 64,
			"animations": {
				"mover_abajo": {
					"name": "mover_abajo",
					"fps": 8.0,
					"loop": true,
					"frames_data": [{ "sheet": "res://old.png", "x": 0, "y": 0, "w": 48, "h": 64 }]
				},
				"atacar_abajo": {
					"name": "atacar_abajo",
					"fps": 10.0,
					"loop": false,
					"frames_data": [{ "sheet": "res://old_atk.png", "x": 0, "y": 0, "w": 48, "h": 64 }]
				},
				"esquivar_abajo": {
					"name": "esquivar_abajo",
					"fps": 12.0,
					"loop": false,
					"frames_data": [{ "sheet": "res://old_dodge.png", "x": 0, "y": 0, "w": 48, "h": 64 }]
				},
				"golpeado": {
					"name": "golpeado",
					"fps": 8.0,
					"loop": false,
					"frames_data": [{ "sheet": "res://old_hit.png", "x": 0, "y": 0, "w": 48, "h": 64 }]
				},
				"morir": {
					"name": "morir",
					"fps": 6.0,
					"loop": false,
					"frames_data": [{ "sheet": "res://old_die.png", "x": 0, "y": 0, "w": 48, "h": 64 }]
				}
			}
		}
	}
	dm.set_records("actors", [old_actor])
	check(dm.save_all(tmp_dir), "save_all guarda actores migrados y sistema en disco")

	dm.init_empty()
	check(dm.load_all(tmp_dir), "load_all recarga proyecto guardado")
	check(dm.get_mannequin_size() == 48, "Maniqui 48 persiste tras save_all/load_all")
	var loaded_actors := dm.get_records("actors")
	check(loaded_actors.size() == 1, "Actor migrado persiste en carga")
	if loaded_actors.size() >= 1:
		var la: Dictionary = loaded_actors[0]
		var la_sp: Dictionary = la.get("sprite_data", {})
		var la_hb: Dictionary = la_sp.get("hurtbox", {})
		var la_shape: Dictionary = la_hb.get("shape", {})
		check(is_equal_approx(float(la_shape.get("size_x", 0.0)), 48.0) and is_equal_approx(float(la_shape.get("size_y", 0.0)), 64.0), "Entrada antigua migra hurtbox por defecto al tamano del frame (48x64)")
		var la_anims: Dictionary = la_sp.get("animations", {})
		check(la_anims.has("mover_abajo") and la_anims.has("atacar_abajo") and la_anims.has("esquivar_abajo") and la_anims.has("golpeado") and la_anims.has("morir"), "Modo turnos no borra ranuras ARPG almacenadas tras migrar y guardar")
		var old_atk_hb := geom.get_frame_hitbox_data(la_sp, "atacar_abajo", 0)
		check(Array(old_atk_hb.get("shapes", [])).is_empty(), "Entrada antigua migra con hitboxes vacias sin perder frames")
	dm.init_empty()
	_cleanup_dir(abs_tmp)
