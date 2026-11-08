extends SceneTree

func _init():
	print("=== REPRODUCIR COMBATE CON INTERPRETE Y ESCENA REALES ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm:
		dm.load_all("res://clockgears_data")

	var interp = ClockGearsEventInterpreter.new()
	root.add_child(interp)

	var cmd = {
		"type": "battle",
		"troop_id": 1,
		"troop": "001: Tropa 001",
		"can_escape": true,
		"can_lose": true
	}

	interp.start_command_list([cmd])

	# Buscar la escena de batalla activa en interp
	var battle_scene = null
	for c in interp.get_children():
		if c is ClockGearsBattleScene:
			battle_scene = c
			break

	if not battle_scene:
		print("ERROR: No se instanció ClockGearsBattleScene en el intérprete!")
		quit(1)
		return

	var ctrl = battle_scene.get_controller()
	print("Batalla iniciada:")
	print("  Party count: ", ctrl.get_party_units().size())
	print("  Enemy count: ", ctrl.get_enemy_units().size())
	for e in ctrl.get_enemy_units():
		print("  Enemigo nombre: '", e.get_name(), "' HP: ", e.get_hp(), " Grafico: '", e.get_graphic_path(), "'")

	var turn_order = ctrl.get_turn_order()
	print("Orden de turnos inicial (size=", turn_order.size(), "):")
	for u in turn_order:
		print("    - ", u.get_name(), " (is_actor=", u.get_is_actor(), ", agi=", u.get_agi(), ")")

	# Simular clic en Atacar
	print("Ejecutando acción Atacar sobre el enemigo 0...")
	var success = battle_scene.select_action_attack(0)
	print("Resultado de select_action_attack: ", success)
	print("Estado del intérprete tras atacar: ", interp.get_state(), " (0=IDLE, 5=WAITING_BATTLE)")
	print("Escena de batalla sigue viva?: ", is_instance_valid(battle_scene) and not battle_scene.is_queued_for_deletion())

	if is_instance_valid(battle_scene) and not battle_scene.is_queued_for_deletion():
		print("El combate sigue activo en la escena de batalla.")
	else:
		print("FALLO REPRODUCIDO: La escena de batalla se cerró/destruyó inmediatamente al atacar!")

	quit(0)
