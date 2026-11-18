extends SceneTree

# Punto 3 (9B-2): 200 conversaciones seguidas con el mismo NPC usando Enter.
# Verifica que el 100% de las veces Enter abre el NPC, acelera/cierra los mensajes y no se traba.

func _step_frames(n: int = 1) -> void:
	for _i in range(n):
		await physics_frame
		await process_frame

func _send_enter_tap(hold_frames: int = 1, wait_after_up_frames: int = 1) -> void:
	var ev_down := InputEventKey.new()
	ev_down.keycode = KEY_ENTER
	ev_down.physical_keycode = KEY_ENTER
	ev_down.pressed = true
	ev_down.echo = false
	Input.parse_input_event(ev_down)
	await _step_frames(hold_frames)

	var ev_up := InputEventKey.new()
	ev_up.keycode = KEY_ENTER
	ev_up.physical_keycode = KEY_ENTER
	ev_up.pressed = false
	Input.parse_input_event(ev_up)
	if wait_after_up_frames > 0:
		await _step_frames(wait_after_up_frames)

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 3 (9B-2): 200 CONVERSACIONES SEGUIDAS CON ENTER ===")
	var m := ClockGearsMap.new()
	m.set_map_size(Vector2i(8, 8))
	root.add_child(m)
	m.ensure_default_layers()
	await _step_frames(2)

	var npc := ClockGearsEvent.new()
	npc.set_name("NPC_Enter200")
	npc.set_behavior_type(0)
	npc.set_trigger_type(0)
	npc.position = Vector2(2 * 32 + 16, 1 * 32 + 16)
	npc.set_commands([
		{"type": "message", "text": "Hola viajero, este es un mensaje de prueba suficientemente largo.", "pos": 2, "bg": 0},
		{"type": "message", "text": "Segundo mensaje de la conversacion.", "pos": 2, "bg": 0}
	])
	m.add_child(npc)

	var player := m.get_runtime_player()
	var interp := m.get_runtime_interpreter()
	player.setup_on_map(m, m.get_runtime_followers(), interp)
	player.set_grid_cell_immediate(Vector2i(2, 2), 8)
	await _step_frames(2)

	var mw := interp.get_message_window()
	var failures := 0

	for conv in range(1, 201):
		# 1. Hablar con el NPC con Enter
		await _send_enter_tap(1, 1)
		if not interp.is_running() or not mw.is_busy():
			failures += 1
			print("FAIL conv=%d: No abrio el evento con Enter (interp_state=%d)" % [conv, interp.get_state()])
			break

		# 2. Para cada mensaje: como recien abre en STATE_TYPING(1), 1er Enter completa(->STATE_WAITING=2), 2do Enter cierra.
		# Variamos rapidez entre pulsaciones (0 o 1 frame tras soltar).
		var expected_steps := 0
		while interp.is_running() and expected_steps < 6:
			expected_steps += 1
			var s_before: int = mw.get_message_state()
			var i_before: int = interp.get_state()
			await _send_enter_tap(1, 0 if (conv % 2 == 0) else 1)
			var s_after: int = mw.get_message_state()
			var i_after: int = interp.get_state()
			if s_before == s_after and i_before == i_after:
				failures += 1
				print("FAIL conv=%d step=%d: Enter no tuvo efecto (state=%d)" % [conv, expected_steps, s_before])
				break

		if interp.is_running() or mw.is_busy():
			failures += 1
			print("FAIL conv=%d: La conversacion no cerro (steps=%d, mw_state=%d)" % [conv, expected_steps, mw.get_message_state()])
			break

		# Esperar 1 frame antes de volver a hablar
		await _step_frames(1)

	# Prueba extra: mensaje con pausa intermedia \! debe detenerse antes de cerrar
	var npc_pause := ClockGearsEvent.new()
	npc_pause.set_name("NPC_Pause")
	npc_pause.set_trigger_type(0)
	npc_pause.position = Vector2(3 * 32 + 16, 2 * 32 + 16)
	npc_pause.set_commands([
		{"type": "message", "text": "Parte uno\\!Parte dos", "pos": 2, "bg": 0}
	])
	m.add_child(npc_pause)
	player.set_grid_cell_immediate(Vector2i(2, 2), 6)
	await _step_frames(2)

	await _send_enter_tap(1, 1) # abre parte 1
	await _send_enter_tap(1, 1) # completa hasta \!
	if not mw.is_busy():
		failures += 1
		print("FAIL pausa intermedia: se cerro antes de mostrar la parte 2")
	await _send_enter_tap(1, 1) # reanuda parte 2 y la completa
	await _send_enter_tap(1, 1) # cierra tras parte 2
	if interp.is_running() or mw.is_busy():
		failures += 1
		print("FAIL pausa intermedia: no cerro tras completar parte 2 (state=%d)" % mw.get_message_state())
	else:
		print("OK: Pausa intermedia \\! funciona correctamente.")

	m.queue_free()
	if failures == 0:
		print("ALL PASS: 200/200 conversaciones consecutivas con Enter completadas sin ningun fallo.")
		quit(0)
	else:
		print("FAIL: Hubo %d fallos." % failures)
		quit(1)
