extends SceneTree

func _init():
	print("====================================================")
	print("TEST VERIFICACION SISTEMA DE ELECCIONES (ClockGears)")
	print("====================================================")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		print("ERROR: ClockGearsDataManager no disponible")
		quit(1)
		return

	# -------------------------------------------------------------------------
	# 1. Creación e inserción de ramas con contenido y anidamiento
	# -------------------------------------------------------------------------
	print("\n[Paso 1] Creando evento con [Comer, Huir], Al cancelar: Opcion 2...")
	var event = ClockGearsEvent.new()
	event.event_name = "TestElecciones"
	event.event_id = 99

	# Comandos:
	# 0: choices [Comer, Huir], cancel_opt = 2 (Opción 2)
	# 1: when_choice 0 (Comer)
	# 2: message "Comiendo..."
	# 3: variable "VarComida = 10"
	# 4: choices [Manzana, Pera] (anidada), cancel_opt = 1
	# 5: when_choice 0 (Manzana)
	# 6: variable "VarFruta = 100"
	# 7: when_choice 1 (Pera)
	# 8: variable "VarFruta = 200"
	# 9: branch_end_choice
	# 10: when_choice 1 (Huir)
	# 11: message "Huyendo..."
	# 12: variable "VarHuida = 99"
	# 13: branch_end_choice
	# 14: message "Continuacion despues de elecciones"
	# 15: variable "VarFin = 777"

	var cmds = [
		{ "type": "choices", "code": 102, "c1": "Comer", "c2": "Huir", "cancel_opt": 2, "cancel_type": 2 },
		{ "type": "when_choice", "code": 402, "choice": 0, "text": "Comer" },
		{ "type": "message", "code": 101, "text": "Comiendo...", "pos": 0 },
		{ "type": "variable", "code": 104, "name": "VarComida", "op": "=", "val": 10 },
		# Elección anidada:
		{ "type": "choices", "code": 102, "c1": "Manzana", "c2": "Pera", "cancel_opt": 1, "cancel_type": 1 },
		{ "type": "when_choice", "code": 402, "choice": 0, "text": "Manzana" },
		{ "type": "variable", "code": 104, "name": "VarFruta", "op": "=", "val": 100 },
		{ "type": "when_choice", "code": 402, "choice": 1, "text": "Pera" },
		{ "type": "variable", "code": 104, "name": "VarFruta", "op": "=", "val": 200 },
		{ "type": "branch_end_choice", "code": 404 },
		# Rama Huir:
		{ "type": "when_choice", "code": 402, "choice": 1, "text": "Huir" },
		{ "type": "message", "code": 101, "text": "Huyendo...", "pos": 0 },
		{ "type": "variable", "code": 104, "name": "VarHuida", "op": "=", "val": 99 },
		{ "type": "branch_end_choice", "code": 404 },
		# Continuación:
		{ "type": "message", "code": 101, "text": "Continuacion despues de elecciones", "pos": 0 },
		{ "type": "variable", "code": 104, "name": "VarFin", "op": "=", "val": 777 }
	]

	event.set_commands(cmds)
	var page0 = { "commands": cmds }
	event.set_pages([page0])

	print("Comandos configurados: ", event.get_commands().size())
	assert(event.get_commands().size() == 16, "Deben haber 16 comandos en el evento")

	# -------------------------------------------------------------------------
	# 2. Persistencia: Guardar y Recargar (serialize_to_meta / deserialize)
	# -------------------------------------------------------------------------
	print("\n[Paso 2] Probando persistencia (guardar y recargar)...")
	event.serialize_to_meta()
	var reloaded_event = ClockGearsEvent.new()
	if event.has_meta("commands"):
		reloaded_event.set_meta("commands", event.get_meta("commands"))
	if event.has_meta("pages"):
		reloaded_event.set_meta("pages", event.get_meta("pages"))
	reloaded_event.deserialize_from_meta()

	var reloaded_cmds = reloaded_event.get_commands()
	assert(reloaded_cmds.size() == 16, "Los comandos recargados deben coincidir exactamente")
	assert(reloaded_cmds[0].get("cancel_opt") == 2, "cancel_opt debe mantenerse en 2")
	assert(reloaded_cmds[4].get("c1") == "Manzana", "Eleccion anidada debe mantenerse intacta")
	print("Persistencia OK: 16 comandos y propiedades anidadas preservadas intactas.")

	# -------------------------------------------------------------------------
	# 3. Ejecución Runtime: Rama Elegida + Elección Anidada (Comer -> Pera)
	# -------------------------------------------------------------------------
	print("\n[Paso 3] Ejecucion Runtime: Seleccion Comer (0) y luego Pera (1)...")
	dm.set_variable("VarComida", 0)
	dm.set_variable("VarFruta", 0)
	dm.set_variable("VarHuida", 0)
	dm.set_variable("VarFin", 0)

	var interp = ClockGearsEventInterpreter.new()
	root.add_child(interp)

	interp.start_event(reloaded_event)
	assert(interp.get_state() == 4, "El interprete debe estar esperando opcion (STATE_WAITING_CHOICE)")
	print("Interprete en espera de opcion exterior. Seleccionando Opcion 0 (Comer)...")
	interp.advance_message_or_choice(0)

	# Avanza el mensaje "Comiendo..." hasta llegar a la eleccion anidada
	while interp.is_running() and interp.get_state() != 4:
		interp.step_frame()
		interp.advance_message_or_choice(0)

	assert(interp.get_state() == 4, "El interprete debe estar esperando opcion anidada")
	print("Interprete en espera de opcion anidada. Seleccionando Opcion 1 (Pera)...")
	interp.advance_message_or_choice(1)

	while interp.is_running():
		interp.step_frame()
		interp.advance_message_or_choice(0)

	var val_comida = dm.get_variable("VarComida")
	var val_fruta = dm.get_variable("VarFruta")
	var val_huida = dm.get_variable("VarHuida")
	var val_fin = dm.get_variable("VarFin")

	print("Valores resultantes:")
	print("  VarComida: ", val_comida, " (Esperado: 10)")
	print("  VarFruta:  ", val_fruta, " (Esperado: 200)")
	print("  VarHuida:  ", val_huida, " (Esperado: 0)")
	print("  VarFin:    ", val_fin, " (Esperado: 777)")

	assert(val_comida == 10, "Rama Comer debio ejecutarse (VarComida == 10)")
	assert(val_fruta == 200, "Rama anidada Pera debio ejecutarse (VarFruta == 200)")
	assert(val_huida == 0, "Rama Huir NO debio ejecutarse (VarHuida == 0)")
	assert(val_fin == 777, "Debe continuar despues de Fin Elecciones (VarFin == 777)")
	print("Aislamiento de ramas y continuacion post-Fin Elecciones OK.")

	# -------------------------------------------------------------------------
	# 4. Verificación de Cancelar (Esc -> Opción 2: Huir)
	# -------------------------------------------------------------------------
	print("\n[Paso 4] Ejecucion Runtime: Cancelar con Esc activa Opcion 2 (Huir)...")
	dm.set_variable("VarComida", 0)
	dm.set_variable("VarFruta", 0)
	dm.set_variable("VarHuida", 0)
	dm.set_variable("VarFin", 0)

	var interp2 = ClockGearsEventInterpreter.new()
	root.add_child(interp2)
	interp2.start_event(reloaded_event)

	assert(interp2.get_state() == 4, "Interprete esperando opcion")
	var msg_win = interp2.get_message_window()
	assert(msg_win != null, "MessageWindow debe existir")

	print("Simulando pulsacion de tecla ESCAPE...")
	var ev_key = InputEventKey.new()
	ev_key.keycode = KEY_ESCAPE
	ev_key.pressed = true
	msg_win.unhandled_input(ev_key)

	while interp2.is_running():
		interp2.step_frame()
		if interp2.get_state() != 4:
			interp2.advance_message_or_choice(0)

	var val2_comida = dm.get_variable("VarComida")
	var val2_fruta = dm.get_variable("VarFruta")
	var val2_huida = dm.get_variable("VarHuida")
	var val2_fin = dm.get_variable("VarFin")

	print("Valores resultantes tras Esc:")
	print("  VarComida: ", val2_comida, " (Esperado: 0)")
	print("  VarFruta:  ", val2_fruta, " (Esperado: 0)")
	print("  VarHuida:  ", val2_huida, " (Esperado: 99)")
	print("  VarFin:    ", val2_fin, " (Esperado: 777)")

	assert(val2_huida == 99, "Esc debio activar Opcion 2 (Huir) y poner VarHuida == 99")
	assert(val2_comida == 0, "Rama Comer NO debio ejecutarse tras cancelar")
	assert(val2_fruta == 0, "Rama anidada NO debio ejecutarse tras cancelar")
	assert(val2_fin == 777, "Debe continuar despues de Fin Elecciones tras cancelar")
	print("Cancelar con Esc activa Opcion 2 y continua el evento OK.")

	# -------------------------------------------------------------------------
	# 5. Verificación de Cancelar con Rama Propia (when_cancel)
	# -------------------------------------------------------------------------
	print("\n[Paso 5] Probando Al Cancelar: Rama (when_cancel)...")
	dm.set_variable("VarCancel", 0)
	dm.set_variable("VarFin", 0)

	var cmds_branch_cancel = [
		{ "type": "choices", "code": 102, "c1": "Atacar", "c2": "Defender", "cancel_opt": -1 },
		{ "type": "when_choice", "code": 402, "choice": 0, "text": "Atacar" },
		{ "type": "variable", "code": 104, "name": "VarAtk", "op": "=", "val": 1 },
		{ "type": "when_choice", "code": 402, "choice": 1, "text": "Defender" },
		{ "type": "variable", "code": 104, "name": "VarDef", "op": "=", "val": 1 },
		{ "type": "when_cancel", "code": 403, "choice": -1 },
		{ "type": "variable", "code": 104, "name": "VarCancel", "op": "=", "val": 55 },
		{ "type": "branch_end_choice", "code": 404 },
		{ "type": "variable", "code": 104, "name": "VarFin", "op": "=", "val": 888 }
	]

	var ev_cancel = ClockGearsEvent.new()
	ev_cancel.set_commands(cmds_branch_cancel)

	var interp3 = ClockGearsEventInterpreter.new()
	root.add_child(interp3)
	interp3.start_event(ev_cancel)

	var msg_win3 = interp3.get_message_window()
	var ev_esc = InputEventKey.new()
	ev_esc.keycode = KEY_ESCAPE
	ev_esc.pressed = true
	msg_win3.unhandled_input(ev_esc)

	while interp3.is_running():
		interp3.step_frame()
		if interp3.get_state() != 4:
			interp3.advance_message_or_choice(0)

	assert(dm.get_variable("VarCancel") == 55, "Rama when_cancel debio ejecutarse (VarCancel == 55)")
	assert(dm.get_variable("VarFin") == 888, "Debe continuar despues de Fin Elecciones (VarFin == 888)")
	print("Rama when_cancel con Esc OK.")

	print("\n====================================================")
	print("TODAS LAS PRUEBAS DEL SISTEMA DE ELECCIONES PASARON!")
	print("====================================================")
	quit(0)
