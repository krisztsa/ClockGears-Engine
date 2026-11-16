extends SceneTree

# Test para verificar el Selector Visual de Mapa para Transferir Jugador y el Preset Puerta (Punto 4)

func _init() -> void:
	print("=== INICIANDO TEST: TRANSFERIR JUGADOR VISUAL + PRESET PUERTA (PUNTO 4) ===")
	call_deferred("_run_checks")

func _run_checks() -> void:
	var root_node = root

	# -------------------------------------------------------------
	# 1. Test de ClockGearsMapCellSelector
	# -------------------------------------------------------------
	if not ClassDB.can_instantiate("ClockGearsMapCellSelector"):
		print("FAIL: ClockGearsMapCellSelector no instanciable desde ClassDB.")
		quit(1)
		return

	var selector = ClassDB.instantiate("ClockGearsMapCellSelector")
	root_node.add_child(selector)

	selector.call("populate_maps")
	var map_opt: OptionButton = selector.call("get_map_option_button")
	if map_opt == null or map_opt.get_item_count() == 0:
		print("FAIL: No se encontraron mapas en res://maps.")
		quit(1)
		return
	print("OK: Selector poblado con %d mapas del proyecto." % map_opt.get_item_count())

	# Fijar mapa y coordenadas de prueba
	selector.call("set_target_map", "res://maps/map001.tscn")
	selector.call("set_target_coords", 7, 9)
	selector.call("set_target_direction", "Derecha")

	var coords: Vector2i = selector.call("get_target_coords")
	if coords != Vector2i(7, 9):
		print("FAIL: Coordenadas esperadas (7, 9), obtenidas: ", coords)
		quit(1)
		return
	print("OK: Coordenadas de casilla destino verificadas: (7, 9)")

	var dir: String = selector.call("get_target_direction")
	if dir != "Derecha":
		print("FAIL: Direccion esperada 'Derecha', obtenida: ", dir)
		quit(1)
		return
	print("OK: Direccion seleccionada verificada: 'Derecha'")

	# Verificar que el ScrollContainer esta presente para desplazamiento sin zoom
	var scroll: ScrollContainer = selector.call("get_scroll_container")
	if scroll == null:
		print("FAIL: ScrollContainer de vista previa no encontrado.")
		quit(1)
		return
	print("OK: ScrollContainer de vista previa de mapa verificado (desplazamiento con rueda, sin zoom).")

	# Simular clic en casilla (3, 5) a traves de select_cell_at_pixel
	selector.call("select_cell_at_pixel", Vector2(3 * 32 + 10, 5 * 32 + 10))
	var coords_after_click: Vector2i = selector.call("get_target_coords")
	if coords_after_click != Vector2i(3, 5):
		print("FAIL: Tras clic en casilla esperabamos (3, 5), obtenido: ", coords_after_click)
		quit(1)
		return
	print("OK: Clic en casilla fija destino correctamente en (3, 5).")

	selector.queue_free()

	# -------------------------------------------------------------
	# 2. Test de integracion en ClockGearsEventEditorDialog
	# -------------------------------------------------------------
	var ev_dialog = ClassDB.instantiate("ClockGearsEventEditorDialog")
	root_node.add_child(ev_dialog)

	var dlg_selector = ev_dialog.call("get_teleport_selector")
	if dlg_selector == null:
		print("FAIL: ClockGearsMapCellSelector no integrado en ClockGearsEventEditorDialog.")
		quit(1)
		return
	print("OK: Selector visual de mapa integrado en el dialogo Transferir Jugador del Editor de Eventos.")

	ev_dialog.queue_free()

	# -------------------------------------------------------------
	# 3. Test de Preset 'Puerta' en Creacion Rapida
	# -------------------------------------------------------------
	# Crear mapa activo en memoria para recibir el evento rapido
	var test_map = ClockGearsMap.new()
	test_map.set_name("TestMap")
	root_node.add_child(test_map)

	# Asegurar que existe un sprite de prueba en res://img/characters/door_wood.png
	var test_img = Image.create(128, 32, false, Image.FORMAT_RGBA8)
	test_img.fill(Color(0.6, 0.4, 0.2, 1.0))
	test_img.save_png("res://img/characters/door_wood.png")

	# Probar creacion directa de evento puerta a traves de ClockGearsMapCellSelector
	var door_ev = ClockGearsMapCellSelector.create_quick_door_event(test_map, Vector2i(4, 6), "res://maps/map002.tscn", 12, 14, "Arriba", "res://img/characters/door_wood.png", 4)
	if door_ev == null:
		print("FAIL: No se pudo crear el evento con preset Puerta.")
		quit(1)
		return

	# Validar propiedades del evento Puerta
	if door_ev.get_trigger_type() != 1: # TRIGGER_PLAYER_TOUCH = 1
		print("FAIL: Disparador de puerta debe ser TRIGGER_PLAYER_TOUCH (1), obtenido: ", door_ev.get_trigger_type())
		quit(1)
		return
	print("OK: Puerta configurada para activarse al contacto con el jugador (TRIGGER_PLAYER_TOUCH).")

	if not door_ev.is_direction_fix():
		print("FAIL: Puerta debe tener dir_fix activado.")
		quit(1)
		return
	print("OK: Puerta configurada con direccion fija (dir_fix = true).")

	var pages: Array = door_ev.get_pages()
	if pages.is_empty():
		print("FAIL: Evento puerta sin paginas generadas.")
		quit(1)
		return

	var cmds: Array = pages[0].get("commands", [])
	print("OK: Comandos generados en la puerta: %d comandos." % cmds.size())

	# Verificar la secuencia de comandos generados:
	# 1. SE door
	# 2. Wait (apertura de puerta)
	# 3. Fade Oscurecer
	# 4. Teleport (map, x, y, dir)
	# 5. Fade Aclarar
	var has_se := false
	var has_wait := false
	var has_fade_out := false
	var has_teleport := false
	var has_fade_in := false
	var tele_cmd: Dictionary = {}

	for c in cmds:
		var ctype = c.get("type", "")
		if ctype == "se":
			has_se = true
		elif ctype == "wait":
			has_wait = true
		elif ctype == "fade" and c.get("mode", "") == "Oscurecer":
			has_fade_out = true
		elif ctype == "teleport":
			has_teleport = true
			tele_cmd = c
		elif ctype == "fade" and c.get("mode", "") == "Aclarar":
			has_fade_in = true

	if not has_se or not has_wait or not has_fade_out or not has_teleport or not has_fade_in:
		print("FAIL: Secuencia de comandos de puerta incompleta (se=%s, wait=%s, fade_out=%s, tele=%s, fade_in=%s)" % [has_se, has_wait, has_fade_out, has_teleport, has_fade_in])
		quit(1)
		return
	print("OK: Secuencia completa de comandos de puerta generada (SE -> Wait/Animacion -> Fade Out -> Teleport -> Fade In).")

	if tele_cmd.get("map", "") != "res://maps/map002.tscn" or tele_cmd.get("x", 0) != 12 or tele_cmd.get("y", 0) != 14 or tele_cmd.get("dir", "") != "Arriba":
		print("FAIL: Parametros de teleport en puerta incorrectos: ", tele_cmd)
		quit(1)
		return
	print("OK: Parametros de Transferir Jugador generados en la puerta verificados con exito: map='%s', x=%d, y=%d, dir='%s'." % [tele_cmd["map"], tele_cmd["x"], tele_cmd["y"], tele_cmd["dir"]])

	test_map.queue_free()

	print("ALL PASS: Transferir Jugador visual y preset Puerta verificados con exito.")
	quit(0)
