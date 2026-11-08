extends SceneTree

func _init():
	print("====================================================")
	print("--- TEST VERIFICACION DIAGNOSTICA BUG 7A ---")
	print("====================================================")

	var root = get_root()
	var map = ClockGearsMap.new()
	map.set_map_size(Vector2i(10, 10))
	map.set_grid_resolution(32)
	root.add_child(map)

	var player = ClockGearsPlayer.new()
	player.set_grid_cell_immediate(Vector2i(2, 2), 2) # Mirando hacia abajo (dir 2)
	map.add_child(player)

	# Crear un evento tipo NPC justo enfrente del jugador (celda (2, 3))
	var event = ClockGearsEvent.new()
	event.set_event_id(10)
	event.set_event_name("NPC_Aldeano")
	event.set_position(Vector2(2 * 32, 3 * 32))
	event.set_trigger_type(0) # TRIGGER_ACTION_BUTTON
	event.set_priority_type(1) # Mismo nivel

	var cmd1 = {
		"type": "message",
		"code": 101,
		"text": "Primer mensaje.",
		"speaker": "Aldeano"
	}
	var cmd2 = {
		"type": "message",
		"code": 101,
		"text": "Segundo mensaje.",
		"speaker": "Aldeano"
	}
	event.set_commands([cmd1, cmd2])
	map.add_child(event)

	var interp = ClockGearsEventInterpreter.new()
	map.add_child(interp)
	interp.setup_interpreter(map, player, null)
	player.setup_on_map(map, null, interp)

	print("\n[Paso 1] Jugador pulsa tecla de accion frente al evento:")
	var activated = player.try_activate_front_event()
	print("  try_activate_front_event() -> ", activated)
	assert(activated == true, "El evento debe activarse con boton de accion frente al NPC")
	assert(interp.is_running() == true, "El interprete debe estar en ejecucion")

	print("\n[Paso 2] Confirmar primer mensaje (cmd 0 -> 1):")
	interp.advance_message_or_choice(0)
	assert(interp.is_running() == true, "Interprete sigue corriendo (segundo mensaje en cola)")

	print("\n[Paso 3] Confirmar segundo mensaje (cmd 1 -> 2, fin del evento):")
	interp.advance_message_or_choice(0)
	assert(interp.is_running() == false, "El evento ha terminado y el interprete pasa a IDLE")

	print("\n[Paso 4] Simular que el jugador mantiene presionada la tecla tras el cierre:")
	print("  player.is_action_key_locked() -> ", player.is_action_key_locked())
	assert(player.is_action_key_locked() == true, "La tecla de accion debe quedar bloqueada al cerrar")
	var reactivated_while_locked = player.try_activate_front_event()
	print("  try_activate_front_event() mientras la tecla esta bloqueada -> ", reactivated_while_locked)
	assert(reactivated_while_locked == false, "NO se debe permitir reactivar el evento mientras no se haya soltado la tecla")

	print("\n[Paso 5] Simular que el jugador suelta la tecla y vuelve a pulsar:")
	player.unlock_action_key()
	print("  player.is_action_key_locked() tras soltar tecla -> ", player.is_action_key_locked())
	assert(player.is_action_key_locked() == false, "La tecla debe quedar desbloqueada tras soltarla")
	var reactivated_after_release = player.try_activate_front_event()
	print("  try_activate_front_event() tras soltar y volver a pulsar -> ", reactivated_after_release)
	assert(reactivated_after_release == true, "Una nueva pulsacion deliberada tras soltar reactiva el evento correctamente")

	print("\n====================================================")
	print(">> TEST COMPLETADO SATISFACTORIAMENTE: SIN BUCLES <<")
	print("====================================================")
	quit(0)
