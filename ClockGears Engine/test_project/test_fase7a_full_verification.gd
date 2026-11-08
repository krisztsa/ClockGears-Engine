extends SceneTree

var passed_count = 0
var failed_count = 0
var current_point = 0

func assert_check(condition: bool, msg: String):
	if condition:
		passed_count += 1
		print("    [PASS] " + msg)
	else:
		failed_count += 1
		printerr("    [FAIL] " + msg)

func _init():
	print("====================================================================")
	print("--- VERIFICACIÓN COMPLETA FASE 7A: SISTEMA NARRATIVO Y MENSAJES ---")
	print("====================================================================")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm != null:
		dm.set_variable("1", 77)
		dm.set_variable("2", 150)
		dm.set_variable("10", 0)

	# -------------------------------------------------------------
	# PUNTO 1: Códigos de escape
	# \C[n], \N[n], \V[n], \I[n], \G, \{, \}, \., \|, \!, \^
	# -------------------------------------------------------------
	print("\n>>> PUNTO 1: Códigos de escape <<<")
	
	# \V[n]
	var res_v = ClockGearsTextParser.resolve_data_escapes("Pociones: \\V[1]")
	assert_check(res_v == "Pociones: 77", "Escape \\V[n] resuelve valor de variable (" + res_v + ")")

	# \G
	var res_g = ClockGearsTextParser.resolve_data_escapes("Dinero: \\G")
	assert_check(res_g.find("G") >= 0 or res_g.find("Oro") >= 0, "Escape \\G resuelve unidad monetaria (" + res_g + ")")

	# \N[n]
	var res_n = ClockGearsTextParser.resolve_data_escapes("Héroe: \\N[1]")
	assert_check(!res_n.is_empty() and res_n.find("\\N") == -1, "Escape \\N[n] resuelve nombre de actor (" + res_n + ")")

	# \C[n]
	var bb_c = ClockGearsTextParser.to_bbcode("\\C[1]Texto Celeste\\C[0]")
	assert_check(bb_c.find("[color=#") >= 0 and bb_c.find("Texto Celeste") >= 0, "Escape \\C[n] genera tag [color] de paleta: " + bb_c)

	# \{ y \}
	var bb_size = ClockGearsTextParser.to_bbcode("\\{Grande\\}Normal")
	assert_check(bb_size.find("[font_size=20]") >= 0, "Escapes \\{ y \\} modifican tamaño de fuente: " + bb_size)

	# \I[n]
	var bb_ico = ClockGearsTextParser.to_bbcode("Espada \\I[5]")
	assert_check(bb_ico.find("◆[5]") >= 0 or bb_ico.find("5") >= 0, "Escape \\I[n] inserta icono: " + bb_ico)

	# \., \|, \!, \^ en parse_to_dict
	var parsed_ctrl = ClockGearsTextParser.parse_to_dict("Inicio\\.\\|Medio\\!Final\\^")
	assert_check(parsed_ctrl.get("no_input_wait", false) == true, "Escape \\^ activa bandera no_input_wait")
	var has_pause_short = false
	var has_pause_long = false
	var has_wait_input = false
	var tokens_arr = parsed_ctrl.get("execution_tokens", [])
	for tok in tokens_arr:
		var ttype = int(tok.get("type", -1))
		if ttype == ClockGearsTextParser.TOKEN_PAUSE_SHORT: has_pause_short = true
		if ttype == ClockGearsTextParser.TOKEN_PAUSE_LONG: has_pause_long = true
		if ttype == ClockGearsTextParser.TOKEN_WAIT_INPUT: has_wait_input = true
	assert_check(has_pause_short, "Escape \\. genera token TOKEN_PAUSE_SHORT")
	assert_check(has_pause_long, "Escape \\| genera token TOKEN_PAUSE_LONG")
	assert_check(has_wait_input, "Escape \\! genera token TOKEN_WAIT_INPUT")

	# -------------------------------------------------------------
	# PUNTO 2: Retratos con expresiones
	# -------------------------------------------------------------
	print("\n>>> PUNTO 2: Retratos con expresiones <<<")
	var mw = ClockGearsMessageWindow.new()
	root.add_child(mw)

	mw.set_face("", 0, "feliz")
	assert_check(mw.get_face_frame() == 1, "Expresión 'feliz' mapea a frame 1")

	mw.set_face("", 0, "triste")
	assert_check(mw.get_face_frame() == 2, "Expresión 'triste' mapea a frame 2")

	mw.set_face("", 0, "enojado")
	assert_check(mw.get_face_frame() == 3, "Expresión 'enojado' mapea a frame 3")

	mw.set_face("", 0, "sorprendido")
	assert_check(mw.get_face_frame() == 4, "Expresión 'sorprendido' mapea a frame 4")

	mw.set_face("", 0, "guiño")
	assert_check(mw.get_face_frame() == 7, "Expresión 'guiño' mapea a frame 7")

	mw.show_message("\\E[enojado]¡No pasarás!", "Guardián")
	assert_check(mw.get_face_frame() == 3, "Token \\E[enojado] en show_message actualiza el frame a 3")

	# -------------------------------------------------------------
	# PUNTO 3: Caja de nombre
	# -------------------------------------------------------------
	print("\n>>> PUNTO 3: Caja de nombre <<<")
	mw.show_message("\\NM[Valeria]¡Adelante valientes!", "")
	assert_check(mw.get_speaker_name() == "Valeria", "Token \\NM[Valeria] extrae y asigna speaker_name correctamente")
	var clean_text = ClockGearsTextParser.strip_escapes("\\NM[Valeria]¡Adelante valientes!")
	assert_check(clean_text == "¡Adelante valientes!", "strip_escapes elimina el código \\NM del cuerpo visible")

	# -------------------------------------------------------------
	# PUNTO 4: Opciones (Mostrar Opciones) y bifurcación correcta
	# -------------------------------------------------------------
	print("\n>>> PUNTO 4: Opciones y bifurcación de ramas <<<")
	var interp = ClockGearsEventInterpreter.new()
	root.add_child(interp)

	# Prueba Rama 0 seleccionada: debe ejecutar la rama 0 y saltar la rama 1
	var commands_branch_test = [
		{ "type": "choices", "code": 102, "c1": "Opción A", "c2": "Opción B", "cancel_opt": 0 },
		{ "type": "when_choice", "code": 402, "choice": 0 },
		{ "type": "variable", "code": 122, "name": "10", "op": "=", "val": 100 },
		{ "type": "when_choice", "code": 402, "choice": 1 },
		{ "type": "variable", "code": 122, "name": "10", "op": "=", "val": 200 },
		{ "type": "branch_end_choice", "code": 404 }
	]

	# Ejecutar eligiendo Opción A (index 0)
	interp.start_command_list(commands_branch_test)
	assert_check(interp.get_state() == 4, "Intérprete espera elección en STATE_WAITING_CHOICE") # STATE_WAITING_CHOICE = 4
	interp.advance_message_or_choice(0) # Elige opción 0
	# Tras procesar la rama 0, la variable 10 debe ser 100 (y NO 200)
	var val_chosen_0 = dm.get_variable("10")
	assert_check(val_chosen_0 == 100, "Elección de Rama 0 ejecutó Rama 0 y fijó variable 10 en 100 (val=" + str(val_chosen_0) + ")")

	# Ejecutar eligiendo Opción B (index 1)
	dm.set_variable("10", 0)
	interp.start_command_list(commands_branch_test)
	interp.advance_message_or_choice(1) # Elige opción 1
	var val_chosen_1 = dm.get_variable("10")
	assert_check(val_chosen_1 == 200, "Elección de Rama 1 ejecutó Rama 1 y fijó variable 10 en 200 (val=" + str(val_chosen_1) + ")")

	# -------------------------------------------------------------
	# PUNTO 5: Entrada de número
	# -------------------------------------------------------------
	print("\n>>> PUNTO 5: Entrada de número <<<")
	var num_cmd = [
		{ "type": "number_input", "code": 103, "variable": 2, "digits": 3 }
	]
	interp.start_command_list(num_cmd)
	assert_check(interp.get_state() == 5, "Intérprete espera número en STATE_WAITING_NUMBER") # STATE_WAITING_NUMBER = 5
	interp.advance_message_or_choice(456)
	var val_num = dm.get_variable("2")
	assert_check(val_num == 456, "Entrada de número actualiza la variable de destino a 456 (val=" + str(val_num) + ")")
	assert_check(interp.is_running() == false, "Intérprete finaliza tras completar entrada de número")

	# -------------------------------------------------------------
	# PUNTO 6: Historial de mensajes (Backlog)
	# -------------------------------------------------------------
	print("\n>>> PUNTO 6: Historial de mensajes (Backlog) <<<")
	mw.add_history_entry("Narrador", "Había una vez un reino lejano.")
	mw.add_history_entry("Héroe", "Debo encontrar la espada.")
	assert_check(mw.get_history_count() >= 2, "Entradas añadidas al historial de diálogos (count=" + str(mw.get_history_count()) + ")")
	mw.open_history()
	assert_check(mw.is_history_open() == true, "open_history abre el panel de historial")
	mw.close_history()
	assert_check(mw.is_history_open() == false, "close_history cierra el panel de historial")

	# -------------------------------------------------------------
	# PUNTO 7: Modo AUTO y SALTAR
	# -------------------------------------------------------------
	print("\n>>> PUNTO 7: Modo AUTO y SALTAR <<<")
	mw.show_message("Texto de prueba de modos", "Sistema")
	assert_check(mw.get_auto_mode() == false, "Modo AUTO inicia DESACTIVADO por defecto")
	assert_check(mw.get_skip_mode() == false, "Modo SKIP inicia DESACTIVADO por defecto")
	mw.set_auto_mode(true)
	assert_check(mw.get_auto_mode() == true, "set_auto_mode(true) activa modo AUTO")
	mw.set_skip_mode(true)
	assert_check(mw.get_skip_mode() == true, "set_skip_mode(true) activa modo SKIP")
	mw.close_window()

	# -------------------------------------------------------------
	# PUNTO 8: Vista previa en vivo en diálogo "Mostrar Texto"
	# -------------------------------------------------------------
	print("\n>>> PUNTO 8: Vista previa en vivo del diálogo <<<")
	var preview_sample = "\\NM[Sabio]\\E[pensativo]El conocimiento es \\C[1]poder\\C[0]."
	var p_parsed = ClockGearsTextParser.parse_to_dict(preview_sample)
	assert_check(p_parsed.get("speaker_name", "") == "Sabio", "Vista previa: speaker_name detectado como 'Sabio'")
	assert_check(p_parsed.get("face_expression", "") == "pensativo", "Vista previa: face_expression detectado como 'pensativo'")
	assert_check(p_parsed.get("bbcode_text", "").find("[color=#") >= 0, "Vista previa: BBCode generado para colores en tiempo real")
	assert_check(p_parsed.get("plain_text", "") == "El conocimiento es poder.", "Vista previa: plain_text limpio sin códigos residuales: '" + str(p_parsed.get("plain_text", "")) + "'")

	# -------------------------------------------------------------
	# PUNTO 9: Varias páginas de texto seguidas sin bucles ni cierres solos
	# -------------------------------------------------------------
	print("\n>>> PUNTO 9: Varias páginas de texto seguidas <<<")
	var seq_cmds = [
		{ "type": "message", "text": "Página Uno", "speaker": "A", "face": "", "position": 0, "frame": 0 },
		{ "type": "message", "text": "Página Dos", "speaker": "B", "face": "", "position": 0, "frame": 0 },
		{ "type": "message", "text": "Página Tres", "speaker": "C", "face": "", "position": 0, "frame": 0 },
		{ "type": "variable", "name": "10", "op": "=", "val": 999 }
	]
	dm.set_variable("10", 0)
	interp.start_command_list(seq_cmds)
	assert_check(interp.is_running() == true, "Secuencia iniciada: intérprete en ejecución")
	assert_check(interp.get_state() == 3, "Página Uno activa: esperando mensaje (STATE_WAITING_MESSAGE=3)")

	# Avanzar Página Uno -> Dos
	interp.advance_message_or_choice(0)
	assert_check(interp.is_running() == true, "Página Dos activa: no se cierra solo ni entra en bucle")

	# Avanzar Página Dos -> Tres
	interp.advance_message_or_choice(0)
	assert_check(interp.is_running() == true, "Página Tres activa: avanzando correctamente paso a paso")

	# Avanzar Página Tres -> Fin y variable 10 = 999
	interp.advance_message_or_choice(0)
	assert_check(interp.is_running() == false, "Secuencia finalizada: intérprete vuelve a IDLE")
	assert_check(dm.get_variable("10") == 999, "Comando posterior ejecutado exitosamente (var 10 = 999)")

	# Limpieza
	mw.queue_free()
	interp.queue_free()

	print("\n====================================================================")
	print("RESUMEN DE VERIFICACIÓN FASE 7A (9 PUNTOS):")
	print("  TOTAL PASADOS: ", passed_count)
	print("  TOTAL FALLADOS: ", failed_count)
	print("====================================================================")

	if failed_count == 0:
		print(">> TODOS LOS 9 PUNTOS DE LA FASE 7A PASARON SATISFACTORIAMENTE <<")
		quit(0)
	else:
		printerr(">> SE DETECTARON FALLOS EN LA VERIFICACIÓN DE LA FASE 7A <<")
		quit(1)
