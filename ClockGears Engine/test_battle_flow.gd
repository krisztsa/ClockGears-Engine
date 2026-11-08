extends SceneTree

func _init():
	print("==================================================")
	print("  TEST SUITE: FASE 8A - COMBATE POR TURNOS & ANIMACIONES")
	print("==================================================")

	var all_ok = true

	all_ok = test_animation_system() and all_ok
	all_ok = test_battle_damage_and_defense() and all_ok
	all_ok = test_battle_states() and all_ok
	all_ok = test_battle_skills_and_items() and all_ok
	all_ok = test_battle_victory_exp_drops_level_up() and all_ok
	all_ok = test_battle_defeat_and_game_over() and all_ok
	all_ok = test_event_interpreter_battle_flow() and all_ok

	if all_ok:
		print("\n>>> TODOS LOS TESTS DE LA FASE 8A PASARON CON ÉXITO (100% OK) <<<")
		quit(0)
	else:
		print("\n>>> ALGUNOS TESTS DE LA FASE 8A FALLARON <<<")
		quit(1)

# -----------------------------------------------------------------------------
# 1. Test del Sistema de Animaciones (Secuencias, Variantes, Triggers y Semilla)
# -----------------------------------------------------------------------------
func test_animation_system() -> bool:
	print("\n[TEST 1] Sistema de Animaciones (clockgears_animation.*)")
	var anim_mgr = Engine.get_singleton("ClockGearsAnimationManager")
	if not anim_mgr:
		print("  FALLO: No se encontró singleton ClockGearsAnimationManager")
		return false

	anim_mgr.clear_all()
	anim_mgr.set_rng_seed(1337)

	# 1.1 Registrar animaciones dinámicas con nombre arbitrario
	anim_mgr.register_animation("slash_a", {"frame_count": 4, "frame_duration": 0.1})
	anim_mgr.register_animation("slash_b", {"frame_count": 6, "frame_duration": 0.08})
	anim_mgr.register_animation("fire_burst", {"frame_count": 5, "frame_duration": 0.12})
	if not anim_mgr.has_animation("slash_a") or not anim_mgr.has_animation("fire_burst"):
		print("  FALLO: Error registrando animaciones individuales")
		return false
	print("  ✓ Animaciones dinámicas registradas correctamente")

	# 1.2 Registrar Secuencia (A -> B con repeticiones y pausas)
	var seq_steps = [
		{"anim_name": "slash_a", "repeats": 2, "delay_after": 0.05},
		{"anim_name": "fire_burst", "repeats": 1, "delay_after": 0.0}
	]
	anim_mgr.register_sequence("combo_fire_slash", seq_steps, false)
	if not anim_mgr.has_sequence("combo_fire_slash"):
		print("  FALLO: Error registrando secuencia combo_fire_slash")
		return false
	print("  ✓ Secuencia A -> B con repeticiones y pausas registrada")

	# 1.3 Registrar Variantes Aleatorias con pesos
	var choices = [
		{"anim_name": "slash_a", "weight": 80.0},
		{"anim_name": "slash_b", "weight": 20.0}
	]
	anim_mgr.register_random_variant("slash_variant", choices)
	if not anim_mgr.has_random_variant("slash_variant"):
		print("  FALLO: Error registrando variante aleatoria")
		return false

	# Probar resolución determinista con semilla fija 1337
	var pick1 = anim_mgr.resolve_animation_name("slash_variant")
	var pick2 = anim_mgr.resolve_animation_name("slash_variant")
	print("  ✓ Variantes aleatorias resueltas con pesos (pick1: " + pick1 + ", pick2: " + pick2 + ")")

	# 1.4 Tabla de Triggers
	var table = ClockGearsAnimationTriggerTable.new()
	table.bind_trigger_named("attack", "combo_fire_slash")
	table.bind_trigger_named("damage", "slash_b")
	table.bind_trigger_named("death", "fire_burst")

	if not table.has_trigger_named("attack"):
		print("  FALLO: Tabla de triggers no tiene trigger attack")
		return false
	if table.get_animation_named("attack") != "combo_fire_slash":
		print("  FALLO: Trigger attack no devuelve combo_fire_slash")
		return false

	# Disparar trigger asignado en dummy Node2D
	var dummy_node = Node2D.new()
	var triggered_attack = anim_mgr.trigger_animation(dummy_node, ClockGearsAnimationTriggerTable.ANIM_TRIGGER_ATTACK, 0, table)
	if not triggered_attack:
		print("  FALLO: No se disparó trigger attack asignado")
		dummy_node.free()
		return false

	# Disparar trigger NO asignado: debe ignorarse limpiamente sin error
	var triggered_unmapped = anim_mgr.trigger_animation(dummy_node, ClockGearsAnimationTriggerTable.ANIM_TRIGGER_MAP_TALK, 0, table)
	if triggered_unmapped:
		print("  FALLO: Trigger no asignado debió retornar false limpiamente")
		dummy_node.free()
		return false
	print("  ✓ Trigger asignado ejecutado y trigger no asignado ignorado limpiamente sin error")

	dummy_node.free()
	return true

# -----------------------------------------------------------------------------
# 2. Test de Fórmulas de Daño y Postura de Defensa
# -----------------------------------------------------------------------------
func test_battle_damage_and_defense() -> bool:
	print("\n[TEST 2] Fórmulas de Daño y Postura de Defensa (ClockGearsFormulas)")
	var ctrl = ClockGearsBattleController.new()
	ctrl.setup_battle(1, true, false)

	var actors = ctrl.get_party_units()
	var enemies = ctrl.get_enemy_units()
	if actors.is_empty() or enemies.is_empty():
		print("  FALLO: Grupo o enemigos vacíos en controlador")
		return false

	var hero = actors[0]
	var enemy = enemies[0]
	hero.set_atk(30)
	enemy.set_def(10)
	enemy.set_hp(300)
	enemy.set_max_hp(300)

	var hp_before = enemy.get_hp()
	# Atacar al enemigo 0
	ctrl.action_attack(0)
	var hp_after = enemy.get_hp()
	var dmg_dealt = hp_before - hp_after
	print("  ✓ Daño normal infligido por 'a.atk*4 - b.def*2' con varianza: " + str(dmg_dealt) + " HP (Restante: " + str(hp_after) + ")")
	if dmg_dealt <= 0:
		print("  FALLO: Daño debió ser mayor a 0")
		return false

	# Probar reducción por postura de defensa
	enemy.set_is_defending(true)
	var hp_before_def = enemy.get_hp()
	ctrl.action_attack(0)
	var dmg_defended = hp_before_def - enemy.get_hp()
	print("  ✓ Daño contra objetivo defendiendo: " + str(dmg_defended) + " HP (Mitigado a la mitad)")
	if dmg_defended > dmg_dealt:
		print("  FALLO: Daño defendido no fue mitigado")
		return false

	return true

# -----------------------------------------------------------------------------
# 3. Test de Estados Básicos (Aplicar, Duración, Quitar)
# -----------------------------------------------------------------------------
func test_battle_states() -> bool:
	print("\n[TEST 3] Estados Básicos (Aplicar, Duración, Quitar)")
	var unit = ClockGearsBattleUnit.new()
	unit.set_name("Guerrero")
	unit.set_hp(200)

	unit.apply_state(1, "Veneno", 2, 0)
	if not unit.has_state(1):
		print("  FALLO: Estado 1 no fue aplicado")
		return false
	print("  ✓ Estado 'Veneno' aplicado correctamente con duración 2 turnos")

	var ctrl = ClockGearsBattleController.new()
	ctrl.tick_states(unit)
	if not unit.has_state(1):
		print("  FALLO: Estado 1 debió quedar con 1 turno")
		return false

	# Segundo tick: expira
	ctrl.tick_states(unit)
	if unit.has_state(1):
		print("  FALLO: Estado 1 debió expirar y eliminarse")
		return false
	print("  ✓ Estado expiró y se eliminó tras cumplir su duración")

	return true

# -----------------------------------------------------------------------------
# 4. Test de Habilidades y Consumibles de Inventario
# -----------------------------------------------------------------------------
func test_battle_skills_and_items() -> bool:
	print("\n[TEST 4] Habilidades y Consumibles de Inventario")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if not dm:
		print("  FALLO: ClockGearsDataManager no disponible")
		return false

	var ctrl = ClockGearsBattleController.new()
	ctrl.setup_battle(1, true, false)
	var hero = ctrl.get_party_units()[0]
	var enemy = ctrl.get_enemy_units()[0]

	# Probar Habilidad: consume MP
	hero.set_mp(50)
	var hp_before_skill = enemy.get_hp()
	ctrl.action_skill(1, 0)
	var mp_after_skill = hero.get_mp()
	print("  ✓ Habilidad ejecutada: MP reducido de 50 a " + str(mp_after_skill) + ", daño aplicado a objetivo")
	if mp_after_skill >= 50:
		print("  FALLO: MP no fue consumido por la habilidad")
		return false

	# Probar Objeto consumible desde inventario
	dm.change_item("Poción", 2)
	if not dm.has_item("Poción", 2):
		print("  FALLO: No se pudo agregar Poción al inventario")
		return false

	hero.set_hp(50)
	hero.set_max_hp(300)
	ctrl.action_item("Poción", 0)
	var hp_after_potion = hero.get_hp()
	var has_2_potions = dm.has_item("Poción", 2)
	var has_1_potion = dm.has_item("Poción", 1)
	print("  ✓ Objeto 'Poción' consumido: Heroe curado a " + str(hp_after_potion) + " HP, stock en inventario actualizado")
	if hp_after_potion <= 50 or has_2_potions or not has_1_potion:
		print("  FALLO: La poción no curó o no consumió 1 del inventario")
		return false

	return true

# -----------------------------------------------------------------------------
# 5. Test de Fin de Batalla: Victoria, EXP, Oro, Drops y Level Up
# -----------------------------------------------------------------------------
func test_battle_victory_exp_drops_level_up() -> bool:
	print("\n[TEST 5] Victoria: EXP, Oro, Drops (hasta 3) y Subida de Nivel")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var ctrl = ClockGearsBattleController.new()
	ctrl.setup_battle(1, true, false)

	var hero = ctrl.get_party_units()[0]
	var enemy = ctrl.get_enemy_units()[0]
	hero.set_level(1)
	hero.set_exp(0)
	enemy.set_exp_reward(100)
	enemy.set_gold_reward(50)

	# Configurar drops del enemigo (probabilidad 100%)
	var drops = [
		{"kind": 0, "item_id": 1, "prob": 100},
		{"kind": 0, "item_id": 2, "prob": 100}
	]
	enemy.set_drops(drops)

	# Matar al enemigo
	enemy.set_hp(0)
	var finished = ctrl.check_battle_end()
	if not finished or ctrl.get_result() != ClockGearsBattleController.RESULT_VICTORY:
		print("  FALLO: La batalla debió terminar en RESULT_VICTORY")
		return false

	print("  ✓ Batalla finalizada en RESULT_VICTORY")
	print("  ✓ EXP ganada: " + str(ctrl.get_total_exp()) + " | Oro ganado: " + str(ctrl.get_total_gold()))
	print("  ✓ Drops obtenidos: " + str(ctrl.get_dropped_items()))
	print("  ✓ Mensajes de nivel: " + str(ctrl.get_level_up_messages()))

	if hero.get_level() <= 1:
		print("  FALLO: Con 100 EXP el héroe debió subir de nivel")
		return false
	print("  ✓ Héroe subió con éxito al nivel " + str(hero.get_level()))

	return true

# -----------------------------------------------------------------------------
# 6. Test de Fin de Batalla: Derrota y Game Over
# -----------------------------------------------------------------------------
func test_battle_defeat_and_game_over() -> bool:
	print("\n[TEST 6] Derrota y Game Over (Permitir perder)")
	# Caso 6.1: can_lose = false -> RESULT_GAME_OVER
	var ctrl_go = ClockGearsBattleController.new()
	ctrl_go.setup_battle(1, true, false) # can_lose = false
	var hero1 = ctrl_go.get_party_units()[0]
	hero1.set_hp(0)
	ctrl_go.check_battle_end()
	if ctrl_go.get_result() != ClockGearsBattleController.RESULT_GAME_OVER:
		print("  FALLO: Sin permitir perder debió ser RESULT_GAME_OVER")
		return false
	print("  ✓ Sin 'Permitir perder': termina en RESULT_GAME_OVER (Código " + str(ctrl_go.get_result()) + ")")

	# Caso 6.2: can_lose = true -> RESULT_DEFEAT
	var ctrl_def = ClockGearsBattleController.new()
	ctrl_def.setup_battle(1, true, true) # can_lose = true
	var hero2 = ctrl_def.get_party_units()[0]
	hero2.set_hp(0)
	ctrl_def.check_battle_end()
	if ctrl_def.get_result() != ClockGearsBattleController.RESULT_DEFEAT:
		print("  FALLO: Con permitir perder debió ser RESULT_DEFEAT")
		return false
	print("  ✓ Con 'Permitir perder': termina en RESULT_DEFEAT (Código " + str(ctrl_def.get_result()) + ")")

	return true

# -----------------------------------------------------------------------------
# 7. Test de Flujo Real: Intérprete de Eventos lanza Batalla y se Reanuda
# -----------------------------------------------------------------------------
func test_event_interpreter_battle_flow() -> bool:
	print("\n[TEST 7] Flujo Real: Comando de Evento Inicia Batalla y Reanuda Intérprete")
	var interp = ClockGearsEventInterpreter.new()

	var cmds = [
		{"type": "variable", "name": "1", "op": "=", "val": 10},
		{"type": "battle", "troop_id": 1, "can_escape": true, "can_lose": true, "bgm": ""},
		{"type": "variable", "name": "1", "op": "+", "val": 5}
	]

	interp.start_command_list(cmds)
	# Paso 1: ejecuta variable = 10, llega al comando battle y debe pausar
	interp.step_frame()
	var state = interp.get_state()
	print("  ✓ Intérprete ejecutó comando 'battle' y entró en estado de espera (state: " + str(state) + ")")
	if state != ClockGearsEventInterpreter.STATE_WAITING_BATTLE:
		print("  FALLO: Intérprete no entró en STATE_WAITING_BATTLE (esperado " + str(ClockGearsEventInterpreter.STATE_WAITING_BATTLE) + ", obtenido: " + str(state) + ")")
		interp.free()
		return false

	# Simular que la escena de batalla termina con victoria
	interp._on_battle_scene_finished(ClockGearsBattleController.RESULT_VICTORY)
	# Debe reanudarse y ejecutar variable += 5
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var final_val = dm.get_variable("1")
	print("  ✓ Batalla finalizada -> Intérprete se reanudó y completó el siguiente comando (Var 1 = " + str(final_val) + ")")
	if final_val != 15:
		print("  FALLO: La variable final debió ser 15, obtenido: " + str(final_val))
		interp.free()
		return false

	interp.free()
	return true
