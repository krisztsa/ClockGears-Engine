extends SceneTree

func _init():
	print("=== INICIO TEST FASE 8A: TROPAS, EDITOR ANIMACIONES Y COMBATE REAL ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if not dm:
		push_error("ClockGearsDataManager no encontrado")
		quit(1)
		return

	# 1. Configurar Actor
	var actors = [
		{
			"id": 1,
			"name": "Kris",
			"hp": 100, "max_hp": 100, "mp": 20, "max_mp": 20,
			"atk": 25, "def": 10, "mat": 10, "mdf": 10, "agi": 20, "luk": 10,
			"level": 1,
			"graphic_path": "res://img/characters/kris.png"
		}
	]
	dm.set_category_array(0, actors) # CAT_ACTORS = 0

	# 2. Configurar Enemigo con animación idle de 4 frames
	var anim_idle = {
		"name": "idle",
		"spritesheet_path": "res://img/battlers/cañón_musgoso_down.png",
		"frame_w": 32,
		"frame_h": 32,
		"cols": 4,
		"rows": 1,
		"fps": 6.0,
		"loop": true,
		"frames": [0, 1, 2, 3]
	}
	var sprite_data = {
		"frame_w": 32,
		"frame_h": 32,
		"cols": 4,
		"rows": 1,
		"margin": 0,
		"spacing": 0,
		"animations": {
			"idle": anim_idle
		}
	}
	var enemies = [
		{
			"id": 1,
			"name": "Cañón Musgoso",
			"hp": 60, "max_hp": 60, "mp": 0, "max_mp": 0,
			"atk": 12, "def": 8, "mat": 5, "mdf": 5, "agi": 5, "luk": 5,
			"exp": 30, "gold": 20,
			"graphic_path": "res://img/battlers/cañón_musgoso_down.png",
			"sprite_data": sprite_data,
			"animations": sprite_data["animations"]
		}
	]
	dm.set_category_array(7, enemies) # CAT_ENEMIES = 7

	# 3. Configurar Tropa con 2 miembros (repetir el mismo enemigo en posiciones distintas)
	var troops = [
		{
			"id": 1,
			"name": "Doble Cañón Musgoso",
			"members": [
				{ "enemy_id": 1, "x": 200, "y": 140 },
				{ "enemy_id": 1, "x": 360, "y": 140 }
			]
		}
	]
	dm.set_category_array(8, troops) # CAT_TROOPS = 8

	# 4. Guardar en disco y recargar
	dm.save_all("res://clockgears_data")
	print("[DB] Guardado en res://clockgears_data")

	dm.load_all("res://clockgears_data")
	var loaded_troops = dm.get_category_array(8)
	assert(loaded_troops.size() >= 1, "Tropa no recargada")
	var t0 = loaded_troops[0]
	var loaded_members = t0.get("members", [])
	print("[DB] Tropa recargada: '%s', miembros: %d" % [t0.get("name"), loaded_members.size()])
	assert(loaded_members.size() == 2, "La tropa debe tener exactamente 2 miembros")

	# 5. Instanciar Escena de Batalla Real
	var scene_root = get_root()
	var bs = ClassDB.instantiate("ClockGearsBattleScene")
	assert(bs != null, "ClockGearsBattleScene no instanciado")
	scene_root.add_child(bs)

	bs.setup_battle(1, true, false, "")
	var ctrl = bs.get_controller()
	assert(ctrl != null, "ClockGearsBattleController no instanciado")

	var order = ctrl.get_turn_order()
	var enemy_units = ctrl.get_enemy_units()
	print("[Batalla] Unidades enemigas instanciadas: %d" % enemy_units.size())
	assert(enemy_units.size() == 2, "Deben existir 2 unidades enemigas")

	var e1 = enemy_units[0]
	var e2 = enemy_units[1]
	print("[Batalla] Enemigo 1: '%s' (HP: %d/%d, Pos: %s)" % [e1.get_name(), e1.get_hp(), e1.get_max_hp(), e1.get_battle_pos()])
	print("[Batalla] Enemigo 2: '%s' (HP: %d/%d, Pos: %s)" % [e2.get_name(), e2.get_hp(), e2.get_max_hp(), e2.get_battle_pos()])
	assert(e1.get_name() != e2.get_name(), "Los nombres deben distinguirse (ej: A y B)")

	print("[Batalla] Orden de turnos (%d participantes):" % order.size())
	for u in order:
		print("  - %s (Actor: %s, AGI: %d)" % [u.get_name(), u.get_is_actor(), u.get_action_speed()])
	assert(order.size() == 3, "El orden de turnos debe incluir a Kris y a los 2 enemigos")

	# 6. Ejecutar Ataque del Jugador a Enemigo 1
	var active = ctrl.get_active_unit()
	print("[Batalla] Turno activo inicial: %s" % active.get_name())
	assert(active.get_is_actor() == true, "Kris (AGI 20) debe tener el primer turno")

	print("[Batalla] Jugador ejecuta comando 'Atacar' contra Enemigo 0...")
	var atk_ok = bs.select_action_attack(0)
	assert(atk_ok == true, "select_action_attack debe tener éxito")

	var history = ctrl.get_log_history()
	assert(history.size() >= 1, "Debe existir al menos 1 entrada en el log de golpes")
	var kris_log = history[0]
	print("[F3 Debug Log (Kris)] " + kris_log)
	assert(kris_log.contains("Kris (ATK: 25)"), "El log debe registrar el ATK real de Kris")
	assert(kris_log.contains("DEF: 8"), "El log debe registrar el DEF real del enemigo")
	assert(kris_log.contains("Fórmula:"), "El log debe mostrar la fórmula")
	assert(kris_log.contains("Daño final:"), "El log debe mostrar el daño final")

	if history.size() >= 2:
		var enemy_log = history[1]
		print("[F3 Debug Log (Enemigo B)] " + enemy_log)
		assert(enemy_log.contains("ATK: 12"), "El log debe registrar el ATK real del enemigo")
		assert(enemy_log.contains("DEF: 10"), "El log debe registrar el DEF real de Kris")

	# Verificar que el combate no se cerró prematuramente (Enemigo 2 sigue vivo)
	print("[Batalla] Enemigo 1 HP tras golpe: %d (Muerto: %s)" % [e1.get_hp(), e1.get_is_dead()])
	print("[Batalla] Enemigo 2 HP: %d (Muerto: %s)" % [e2.get_hp(), e2.get_is_dead()])
	assert(bs.is_battle_finished() == false, "El combate NO debe cerrarse porque el Enemigo 2 sigue vivo")

	# Siguiente turno: si es de un enemigo, ejecutamos su turno
	var next_unit = ctrl.get_active_unit()
	if next_unit != null:
		print("[Batalla] Siguiente unidad activa: %s" % next_unit.get_name())
		if not next_unit.get_is_actor():
			print("[Batalla] Enemigo '%s' ejecuta su turno..." % next_unit.get_name())
			ctrl.execute_enemy_turn()
			print("[F3 Debug Log] " + ctrl.get_last_hit_log())

	# Avanzar hasta que Kris vuelva a actuar
	while ctrl.get_active_unit() != null and not ctrl.get_active_unit().get_is_actor() and not ctrl.is_finished():
		ctrl.execute_enemy_turn()

	if not ctrl.is_finished() and ctrl.get_active_unit() != null and ctrl.get_active_unit().get_is_actor():
		print("[Batalla] Kris ataca al Enemigo 2 (índice 1)...")
		bs.select_action_attack(1)
		print("[F3 Debug Log] " + ctrl.get_last_hit_log())

	while not ctrl.is_finished() and not ctrl.get_active_unit().get_is_actor():
		ctrl.execute_enemy_turn()
	if not ctrl.is_finished() and ctrl.get_active_unit() != null and ctrl.get_active_unit().get_is_actor():
		bs.select_action_attack(1)
		print("[F3 Debug Log] " + ctrl.get_last_hit_log())

	print("[Batalla] Combate terminado: %s (Resultado: %d)" % [ctrl.is_finished(), ctrl.get_result()])
	assert(ctrl.is_finished() == true, "La batalla debe terminar tras derrotar a ambos enemigos")
	assert(ctrl.get_result() == 1, "El resultado debe ser Victoria (1)")
	print("[Batalla] EXP ganada: %d, Oro ganado: %d" % [ctrl.get_total_exp(), ctrl.get_total_gold()])
	assert(ctrl.get_total_exp() == 60, "EXP total debe ser 60 (30 x 2 enemigos)")
	assert(ctrl.get_total_gold() == 40, "Oro total debe ser 40 (20 x 2 enemigos)")

	bs.finish_battle_immediate()
	bs.queue_free()
	print("=== TODOS LOS TESTS DE LA FASE 8A PASARON CORRECTAMENTE ===")
	quit(0)
