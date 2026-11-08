extends SceneTree

# Test automatizado de los 3 fallos bloqueantes de la Fase 6:
# 1. Importación de PNG de tileset (32x32), validación de archivo inválido, corte en tiles y dibujo en el mapa.
# 2. Validación de arranque del juego (F5 / Jugar) e impresión de cada condición en consola.
# 3. Herramienta "Posición inicial del jugador" (único punto de inicio sincronizado con Sistema) y comando "Transferir jugador".

var total_passed: int = 0
var total_failed: int = 0

func check(condition: bool, msg: String) -> void:
	if condition:
		total_passed += 1
		print("  [PASS] ", msg)
	else:
		total_failed += 1
		printerr("  [FAIL] ", msg)

func _initialize() -> void:
	print("=== INICIANDO TEST FASE 6: CORRECCION DE 3 FALLOS BLOQUEANTES ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton activo")

	# Limpiar estado previo para probar Regla Cero desde cero
	dm.init_empty()
	var clean_sys: Dictionary = dm.get_system_config()
	clean_sys["has_start_pos"] = false
	clean_sys["start_map"] = ""
	clean_sys["start_x"] = -1
	clean_sys["start_y"] = -1
	clean_sys["party"] = []
	dm.set_system_config(clean_sys)

	# Crear mapa de prueba
	DirAccess.make_dir_recursive_absolute("res://maps")
	var map := ClockGearsMap.new()
	map.name = "Map001"
	map.set_map_name("Aldea Inicial")
	map.set_map_size(Vector2i(20, 15))
	map.ensure_default_layers()

	# Guardar el mapa en res://maps/map001.tscn para que exista en el proyecto
	for i in range(map.get_child_count()):
		map.get_child(i).owner = map
	var packed := PackedScene.new()
	packed.pack(map)
	ResourceSaver.save(packed, "res://maps/map001.tscn")
	map.scene_file_path = "res://maps/map001.tscn"

	# -------------------------------------------------------------------------
	# PROBLEMA 1: Importar tileset PNG, validar inválido, cortar 32x32 y pintar
	# -------------------------------------------------------------------------
	print("\n--- TEST PROBLEMA 1: Importación de Tileset PNG 32x32 y Dibujo en Mapa ---")
	var invalid_ok: bool = map.import_tileset_png("res://ruta_inexistente_invalida.png")
	check(not invalid_ok, "Archivo PNG inexistente/inválido es rechazado con mensaje claro")

	# Crear un PNG válido de 64x64 (4 tiles de 32x32)
	var img := Image.create_empty(64, 64, false, Image.FORMAT_RGBA8)
	for y in range(64):
		for x in range(64):
			if x < 32 and y < 32:
				img.set_pixel(x, y, Color(0.2, 0.7, 0.3, 1.0)) # Tile (0,0) Hierba
			elif x >= 32 and y < 32:
				img.set_pixel(x, y, Color(0.5, 0.5, 0.55, 1.0)) # Tile (1,0) Piedra
			elif x < 32 and y >= 32:
				img.set_pixel(x, y, Color(0.2, 0.4, 0.85, 1.0)) # Tile (0,1) Agua
			else:
				img.set_pixel(x, y, Color(0.75, 0.6, 0.35, 1.0)) # Tile (1,1) Tierra
	var tmp_png_path := "res://test_tileset_64x64.png"
	var save_err := img.save_png(tmp_png_path)
	check(save_err == OK, "PNG de prueba 64x64 generado en disco")

	var import_ok: bool = map.import_tileset_png(tmp_png_path)
	check(import_ok, "import_tileset_png importa correctamente el PNG de 64x64")
	check(FileAccess.file_exists("res://img/tilesets/test_tileset_64x64.png"), "El tileset importado se guarda dentro del proyecto en res://img/tilesets/")

	var ground_layer: TileMapLayer = map.get_node_or_null("Capa_Suelo") as TileMapLayer
	check(ground_layer != null and ground_layer.tile_set != null, "Capa_Suelo tiene TileSet asignado tras importar")
	if ground_layer and ground_layer.tile_set:
		check(ground_layer.tile_set.tile_size == Vector2i(32, 32), "Tamaño de tile configurado a 32x32 por defecto")
		var atlas: TileSetAtlasSource = ground_layer.tile_set.get_source(0) as TileSetAtlasSource
		check(atlas != null, "TileSetAtlasSource n.º 0 creado en el TileSet")
		if atlas:
			check(atlas.has_tile(Vector2i(0, 0)) and atlas.has_tile(Vector2i(1, 0)) and atlas.has_tile(Vector2i(0, 1)) and atlas.has_tile(Vector2i(1, 1)),
				"PNG de 64x64 cortado automáticamente en los 4 tiles de 32x32: (0,0), (1,0), (0,1), (1,1)")

	# Pintar tiles en el mapa y comprobar lectura
	map.set_tile_at(0, Vector2i(3, 4), Vector2i(1, 0))
	map.set_tile_at(0, Vector2i(4, 4), Vector2i(0, 1))
	check(map.get_tile_at(0, Vector2i(3, 4)) == Vector2i(1, 0), "get_tile_at(0, (3,4)) devuelve el tile pintado (1,0)")
	check(map.get_tile_at(0, Vector2i(4, 4)) == Vector2i(0, 1), "get_tile_at(0, (4,4)) devuelve el tile pintado (0,1)")

	# -------------------------------------------------------------------------
	# PROBLEMA 2 y 3: Punto de inicio único, validación de F5 y arranque runtime
	# -------------------------------------------------------------------------
	print("\n--- TEST PROBLEMA 2 y 3: Posición Inicial del Jugador y Arranque Runtime ---")
	# Intentar arrancar runtime sin actor ni posición inicial -> Regla Cero debe bloquear e informar en consola
	map.init_runtime_session()
	check(map.get_runtime_player() == null, "Sin actor ni punto de inicio, el runtime no inventa jugador y avisa en consola")

	# Crear un actor válido en la Base de Datos
	var aid = dm.create_record(0, {"name": "Crono"}) # CAT_ACTORS = 0
	check(aid == 1, "Actor 'Crono' (ID 1) creado en la Base de Datos")

	# Intentar arrancar runtime CON actor pero SIN posición inicial -> debe seguir avisando que falta posición inicial
	map.init_runtime_session()
	check(map.get_runtime_player() == null, "Con actor pero sin posición inicial colocada, F5/runtime avisa qué falta y no arranca")

	# Colocar primera posición inicial en (2, 3) mirando a la Derecha (dir=2)
	map.set_player_start_position(Vector2i(2, 3), 2)
	var sys1: Dictionary = dm.get_system_config()
	check(bool(sys1.get("has_start_pos", false)) == true, "Marcador de posición inicial activa has_start_pos en Sistema")
	check(int(sys1.get("start_x", -1)) == 2 and int(sys1.get("start_y", -1)) == 3, "Posición inicial (2, 3) guardada en Sistema")

	# Colocar una segunda posición inicial en (6, 7) mirando a la Izquierda (dir=4): debe mover la anterior (solo un punto de inicio)
	map.set_player_start_position(Vector2i(6, 7), 4)
	var sys2: Dictionary = dm.get_system_config()
	check(int(sys2.get("start_x", -1)) == 6 and int(sys2.get("start_y", -1)) == 7 and int(sys2.get("start_dir", -1)) == 4,
		"Al colocar otro punto de inicio en (6, 7), se mueve el anterior y Sistema lee (6, 7, dir=4)")
	check(String(sys2.get("start_map", "")) == "res://maps/map001.tscn", "Sistema lee el mapa inicial 'res://maps/map001.tscn'")

	# Ahora con Actor + Mapa + Posición de inicio válidos, arrancar el runtime
	root.add_child(map)
	map.init_runtime_session()
	var player: ClockGearsPlayer = map.get_runtime_player()
	check(player != null, "Con actor, mapa y posición de inicio válidos, el runtime arranca y crea ClockGearsPlayer")
	if player:
		check(player.get_grid_cell() == Vector2i(6, 7), "El jugador aparece exactamente en la casilla de inicio (6, 7)")
		check(player.position == Vector2(6 * 32, 7 * 32), "Posición pixel-perfect del jugador en (192, 224)")

		# Mover al personaje en rejilla de 32x32 hacia la Derecha (DIR_RIGHT = 6) -> (7, 7)
		var stepped: bool = player.try_step(6)
		check(stepped, "El personaje se mueve en rejilla de 32x32 hacia la Derecha (7, 7)")
		check(player.get_grid_cell() == Vector2i(7, 7), "Celda en rejilla del jugador actualizada a (7, 7)")

	# Probar el comando de evento "Transferir jugador" (teleport) en el intérprete de eventos
	var interp: ClockGearsEventInterpreter = map.get_runtime_interpreter()
	check(interp != null, "ClockGearsEventInterpreter activo en runtime")
	if interp and player:
		var ev := ClockGearsEvent.new()
		ev.set_event_name("EV_Portal")
		ev.position = Vector2(7 * 32, 7 * 32)
		var cmd_transfer := {
			"type": "teleport",
			"map": "", # mismo mapa para prueba inmediata de coordenadas y dirección
			"x": 12,
			"y": 9,
			"dir": "Arriba"
		}
		ev.set_commands([cmd_transfer])
		map.add_clockgears_event(ev, Vector2(7 * 32, 7 * 32))
		interp.start_event(ev)
		interp.step_frame()
		check(player.get_grid_cell() == Vector2i(12, 9), "Comando 'Transferir jugador' mueve al jugador a la celda destino (12, 9)")
		check(player.get_direction() == 8, "Comando 'Transferir jugador' actualiza la dirección del jugador a Arriba (DIR_UP=8)")

	map.free()
	print("\n==========================================================")
	print("RESULTADO TEST FASE 6 BLOQUEANTES: ", total_passed, " PASS / ", total_failed, " FAIL")
	print("==========================================================")
	quit(0 if total_failed == 0 else 1)
