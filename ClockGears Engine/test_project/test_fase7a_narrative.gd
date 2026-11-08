extends SceneTree

var passed_count = 0
var failed_count = 0

func assert_test(condition: bool, msg: String):
	if condition:
		passed_count += 1
		print("  [PASS] " + msg)
	else:
		failed_count += 1
		printerr("  [FAIL] " + msg)

func _init():
	print("====================================================")
	print("--- TEST FASE 7A: SISTEMA NARRATIVO Y MENSAJES ---")
	print("====================================================")

	# 1. Test ClockGearsTextParser - Códigos de Datos
	print("\n--- 1. PARSER: Códigos de Escape de Datos ---")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm != null:
		dm.set_variable("1", 42)
		dm.set_variable("2", 999)
	
	var res_v = ClockGearsTextParser.resolve_data_escapes("Tienes \\V[1] pociones y \\V[2] de oro.")
	assert_test(res_v == "Tienes 42 pociones y 999 de oro.", "Resolución de variables \\V[n] (" + res_v + ")")

	var res_g = ClockGearsTextParser.resolve_data_escapes("Moneda: \\G")
	assert_test(res_g.find("Oro") >= 0 or res_g.find("G") >= 0, "Resolución de moneda \\G (" + res_g + ")")

	# 2. Test ClockGearsTextParser - BBCode y Paleta de Colores
	print("\n--- 2. PARSER: BBCode, Colores y Formato ---")
	var col_hex = ClockGearsTextParser.get_palette_hex(1)
	assert_test(!col_hex.is_empty(), "Hex de color de paleta 1 obtenido: #" + col_hex)

	var text_c = "\\C[1]Texto Celeste\\C[0]"
	var bb_c = ClockGearsTextParser.to_bbcode(text_c)
	assert_test(bb_c.find("[color=#") >= 0 and bb_c.find("Texto Celeste") >= 0, "Conversión de \\C[1] a BBCode [color]: " + bb_c)

	var text_style = "\\BTexto Negrita\\B y \\ITCursiva\\IT"
	var bb_style = ClockGearsTextParser.to_bbcode(text_style)
	assert_test(bb_style.find("[b]Texto Negrita[/b]") >= 0, "Conversión de \\B a [b]: " + bb_style)
	assert_test(bb_style.find("[i]Cursiva[/i]") >= 0, "Conversión de \\IT a [i]: " + bb_style)

	var text_effects = "\\SHSombra\\SH y \\WVOnda\\WV y \\RBArcoiris\\RB"
	var bb_effects = ClockGearsTextParser.to_bbcode(text_effects)
	assert_test(bb_effects.find("[drop_shadow") >= 0 or bb_effects.find("Sombra") >= 0, "Soporte de \\SH (sombra)")
	assert_test(bb_effects.find("[wave") >= 0 or bb_effects.find("Onda") >= 0, "Soporte de \\WV (onda)")
	assert_test(bb_effects.find("[rainbow") >= 0 or bb_effects.find("Arcoiris") >= 0, "Soporte de \\RB (arcoíris)")

	# 3. Test ClockGearsTextParser - Parse Full (Tokens y Metadatos)
	print("\n--- 3. PARSER: Tokens de Ejecución y Metadatos (\\NM, \\E, \\$, \\!) ---")
	var full_text = "\\NM[Aria]\\E[feliz]¡Hola viajero!\\!\\.\\|\\$¿Necesitas algo?"
	var stripped = ClockGearsTextParser.strip_escapes(full_text)
	assert_test(stripped.find("\\NM") == -1 and stripped.find("\\E") == -1 and stripped.find("\\!") == -1, "strip_escapes elimina códigos de control: " + stripped)

	# 4. Test ClockGearsMessageWindow (Instanciación y API)
	print("\n--- 4. ClockGearsMessageWindow: Creación y Propiedades ---")
	var msg_win = ClockGearsMessageWindow.new()
	assert_test(msg_win != null, "ClockGearsMessageWindow instanciada con éxito")
	root.add_child(msg_win)

	# Modos Auto y Skip
	msg_win.set_auto_mode(true)
	assert_test(msg_win.get_auto_mode() == true, "Modo AUTO activado correctamente")
	msg_win.set_auto_mode(false)
	assert_test(msg_win.get_auto_mode() == false, "Modo AUTO desactivado")

	msg_win.set_skip_mode(true)
	assert_test(msg_win.get_skip_mode() == true, "Modo SKIP activado correctamente")
	msg_win.set_skip_mode(false)
	assert_test(msg_win.get_skip_mode() == false, "Modo SKIP desactivado")

	# Mostrar Mensaje
	print("\n--- 5. ClockGearsMessageWindow: Mostrar Mensaje y Estados ---")
	msg_win.show_message("\\NM[Guardia]¡Alto ahí!\\.", "Guardia", "", 0, 0)
	assert_test(msg_win.is_busy() == true, "La ventana de mensajes pasa a estado busy tras show_message")

	# Historial (Backlog)
	print("\n--- 6. ClockGearsMessageWindow: Historial de Diálogos (Backlog) ---")
	msg_win.add_history_entry("Guardia", "¡Alto ahí!")
	msg_win.add_history_entry("Jugador", "¿Quién anda ahí?")
	msg_win.open_history()
	assert_test(msg_win.is_history_open() == true, "Backlog abierto con éxito")
	msg_win.close_history()
	assert_test(msg_win.is_history_open() == false, "Backlog cerrado con éxito")

	# Elecciones (Hasta 6 opciones)
	print("\n--- 7. ClockGearsMessageWindow: Elecciones (6 opciones) ---")
	var choices = PackedStringArray(["Atacar", "Defender", "Habilidades", "Objetos", "Formación", "Huir"])
	msg_win.show_choices(choices, 1, 0)
	assert_test(msg_win.is_busy() == true, "Ventana de elecciones activa con 6 opciones")

	# Entrada de Número (1 a 8 dígitos)
	print("\n--- 8. ClockGearsMessageWindow: Entrada de Número ---")
	msg_win.show_number_input(1, 4)
	assert_test(msg_win.is_busy() == true, "Ventana de entrada de número (4 dígitos) activa")
	msg_win.close_window()
	assert_test(msg_win.is_busy() == false, "Ventana cerrada vuelve a estado idle (no busy)")

	# 9. Test Intérprete de Eventos y Runtime Integration
	print("\n--- 9. ClockGearsEventInterpreter: Integración de Mensajes ---")
	var interp = ClockGearsEventInterpreter.new()
	assert_test(interp != null, "ClockGearsEventInterpreter creado con éxito")
	root.add_child(interp)

	# Ejecutar comando de mensaje mediante el intérprete
	var cmd_msg = {
		"type": "message",
		"text": "\\NM[Elena]El viaje ha comenzado.\\!",
		"face": "",
		"frame": 0,
		"position": 0
	}
	interp.start_command_list([cmd_msg])
	assert_test(interp.is_running() == true, "El comando de mensaje inicia la ejecución del intérprete")
	assert_test(interp.is_blocking_player() == true, "El diálogo bloquea al jugador mientras se muestra")

	# Simular avance del mensaje por parte del jugador
	interp.advance_message_or_choice(0)
	assert_test(interp.is_running() == false, "Tras confirmar el mensaje, el intérprete finaliza")

	msg_win.queue_free()
	interp.queue_free()

	print("\n====================================================")
	print("RESULTADO TESTS FASE 7A:")
	print("  PASADOS: ", passed_count)
	print("  FALLADOS: ", failed_count)
	print("====================================================")

	if failed_count == 0:
		print(">> TODOS LOS TESTS DE LA FASE 7A PASARON EXITOSAMENTE <<")
		quit(0)
	else:
		printerr(">> FALLARON TESTS DE LA FASE 7A <<")
		quit(1)
