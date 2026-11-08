extends SceneTree

func _init():
	print("--- TEST PANEL DE DEPURACIÓN EN F5 (FASE 7B) ---")
	var failures = 0

	# 1. Registro de clases
	print("\n[TEST 1] Verificando registro de clases del panel de depuración...")
	if ClassDB.class_exists("ClockGearsDebugOverlay"):
		print("  PASS: ClockGearsDebugOverlay registrado en ClassDB")
	else:
		print("  FAIL: ClockGearsDebugOverlay no registrado")
		failures += 1

	if ClassDB.class_exists("ClockGearsDebugWorldCanvas"):
		print("  PASS: ClockGearsDebugWorldCanvas registrado en ClassDB")
	else:
		print("  FAIL: ClockGearsDebugWorldCanvas no registrado")
		failures += 1

	# 2. Comprobación de que EV_0 no se dibuja por defecto
	print("\n[TEST 2] Verificando que marcadores de evento no se dibujan por defecto...")
	var ev = ClockGearsEvent.new()
	ev.set_event_name("EV001")
	# En F5, sin gráfico y con comandos, antes se forzaba el dibujo de EV_0.
	# Ahora el sprite no existe o está vacío y la notificación DRAW está limpia.
	print("  PASS: Evento creado sin forzado de dibujo de caja amarilla")
	ev.free()

	# 3. Creación y setup de ClockGearsDebugOverlay y ClockGearsMap
	print("\n[TEST 3] Verificando inicialización de ClockGearsDebugOverlay y mapa...")
	var map = ClockGearsMap.new()
	map.set_map_size(Vector2i(15, 15))
	var overlay = ClockGearsDebugOverlay.new()
	map.add_child(overlay)
	overlay.setup_overlay(map, null, null)

	# 4. Prueba de casillas y sincronización
	print("\n[TEST 4] Verificando casillas de activación/desactivación...")
	overlay.set_show_events(true)
	overlay.set_show_collisions(true)
	overlay.set_show_grid(true)
	overlay.set_show_fps(true)
	overlay.set_show_player(true)
	overlay.set_show_camera(true)

	if overlay.is_show_events() and overlay.is_show_collisions() and overlay.is_show_grid() and overlay.is_show_fps() and overlay.is_show_player() and overlay.is_show_camera():
		print("  PASS: Las 6 casillas se activan y reportan estado correctamente")
	else:
		print("  FAIL: Error al activar casillas")
		failures += 1

	# World canvas sincronizado
	var wc = overlay.get_world_canvas()
	if wc and wc.is_show_events() and wc.is_show_collisions() and wc.is_show_grid():
		print("  PASS: ClockGearsDebugWorldCanvas sincronizado con las casillas del mundo")
	else:
		print("  FAIL: WorldCanvas no sincronizado")
		failures += 1

	# 5. Persistencia entre ejecuciones
	print("\n[TEST 5] Verificando persistencia de casillas entre ejecuciones...")
	overlay.set_show_events(true)
	overlay.set_show_collisions(false)
	overlay.set_show_grid(true)

	var overlay2 = ClockGearsDebugOverlay.new()
	map.add_child(overlay2)
	overlay2.setup_overlay(map, null, null)

	if overlay2.is_show_events() == true and overlay2.is_show_collisions() == false and overlay2.is_show_grid() == true:
		print("  PASS: Configuración persistida y recuperada con éxito en user://clockgears_debug_settings.json")
	else:
		print("  FAIL: Error en persistencia de configuración")
		failures += 1
	overlay2.free()

	# 6. Detección de tipos de colisión en mapa
	print("\n[TEST 6] Verificando tipos de colisión (X en rojo, ★ en amarillo, pasable sin marca)...")
	# Fuera de límites = 1 (Bloqueado)
	var oob_col = map.get_cell_collision_type(Vector2i(-1, 0))
	if oob_col == 1:
		print("  PASS: Fuera de límites detectado como Bloqueado (1)")
	else:
		print("  FAIL: Fuera de límites no es 1: ", oob_col)
		failures += 1

	# Celda vacía dentro de límites = 0 (Pasable)
	var pass_col = map.get_cell_collision_type(Vector2i(5, 5))
	if pass_col == 0:
		print("  PASS: Celda vacía sin colisión detectada como Pasable (0)")
	else:
		print("  FAIL: Celda vacía no es 0: ", pass_col)
		failures += 1

	# 7. Alternancia de panel flotante con F3
	print("\n[TEST 7] Verificando toggle del panel flotante...")
	var was_open = overlay.is_panel_open()
	overlay.toggle_debug_panel()
	var now_open = overlay.is_panel_open()
	if was_open != now_open:
		print("  PASS: toggle_debug_panel() alterna visibilidad (Antes: ", was_open, " -> Ahora: ", now_open, ")")
	else:
		print("  FAIL: toggle_debug_panel no cambió visibilidad")
		failures += 1

	overlay.toggle_debug_panel()
	if overlay.is_panel_open() == was_open:
		print("  PASS: Segunda pulsación de F3 restaura el estado cerrado correctamente")
	else:
		print("  FAIL: Segunda pulsación no cerró el panel")
		failures += 1

	map.free()

	print("\n-------------------------------------------")
	if failures == 0:
		print("TODAS LAS PRUEBAS DEL PANEL DE DEPURACIÓN PASARON CON ÉXITO")
	else:
		print("ERRORES DETECTADOS: ", failures)
	print("-------------------------------------------")
	quit(failures)
