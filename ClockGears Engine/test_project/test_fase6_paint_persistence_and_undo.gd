extends SceneTree

# Test Automatizado: FASE 6 - Persistencia de Pintado de Tiles y Deshacer/Rehacer (Undo/Redo)
# PROBLEMA 1:
# - Pintar celda individual y rectángulo en el mapa.
# - Comprobar con get_tile_at que están ahí.
# - Guardar escena .tscn a disco, recargar y volver a comprobar.
# - Verificar Deshacer (Undo) y Rehacer (Redo) de trazo completo.
# - Verificar Goma (borrado de celda).

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
	print("======================================================================")
	print("   TEST FASE 6: PERSISTENCIA DE PINTADO DE TILES Y UNDO/REDO")
	print("======================================================================")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton activo")
	dm.init_empty()

	DirAccess.make_dir_recursive_absolute("res://maps")
	DirAccess.make_dir_recursive_absolute("res://img/tilesets")

	# 1. Crear textura PNG de prueba para el tileset (128x128 = 4x4 tiles de 32x32)
	var img := Image.create_empty(128, 128, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.2, 0.7, 0.3, 1.0)) # Verde base
	var tex_path := "res://img/tilesets/test_palette_sheet.png"
	img.save_png(tex_path)

	# Registrar Tileset en la Base de Datos
	var ts_dict := {
		"id": 1,
		"name": "Tileset Terreno Test",
		"mode": 0,
		"slots": { "A5": tex_path },
		"slot_collisions": {}
	}
	dm.set_category_array(13, [ts_dict]) # CAT_TILESETS = 13

	# 2. Instanciar ClockGearsMap y configurar capas
	var map := ClockGearsMap.new()
	map.name = "MapPaintTest"
	map.set_map_name("Mapa Prueba Pintado")
	map.set_tileset_id(1)
	map.ensure_default_layers()
	map.setup_tileset_from_database(1)

	var layer_ground: TileMapLayer = map.get_layer(0)
	check(layer_ground != null, "Capa_Suelo existe en el mapa")
	check(layer_ground.tile_set != null, "Capa_Suelo tiene TileSet asignado")

	# -------------------------------------------------------------------------
	# PASO 1: Pintar celda individual (2, 3) con tile (0, 0)
	# -------------------------------------------------------------------------
	print("\n--- PASO 1: Pintado de celda individual (2, 3) ---")
	map.set_tile_at(0, Vector2i(2, 3), Vector2i(0, 0))
	layer_ground.update_internals()
	layer_ground.queue_redraw()

	var tile_cell: Vector2i = map.get_tile_at(0, Vector2i(2, 3))
	check(tile_cell == Vector2i(0, 0), "get_tile_at(0, (2, 3)) devuelve (0, 0)")
	check(layer_ground.get_used_cells().has(Vector2i(2, 3)), "get_used_cells() contiene la celda (2, 3)")

	# -------------------------------------------------------------------------
	# PASO 2: Pintar un rectángulo desde (4, 4) hasta (6, 6) con tile (1, 0)
	# -------------------------------------------------------------------------
	print("\n--- PASO 2: Pintado de Rectángulo (4, 4) a (6, 6) ---")
	for ry in range(4, 7):
		for rx in range(4, 7):
			map.set_tile_at(0, Vector2i(rx, ry), Vector2i(1, 0))
	layer_ground.update_internals()
	layer_ground.queue_redraw()

	var rect_ok: bool = true
	for ry in range(4, 7):
		for rx in range(4, 7):
			var t: Vector2i = map.get_tile_at(0, Vector2i(rx, ry))
			if t != Vector2i(1, 0):
				rect_ok = false
				printerr("Fallo en celda del rect: (", rx, ", ", ry, ") got: ", t)
	check(rect_ok, "Todas las celdas del rectángulo (4..6, 4..6) tienen el tile (1, 0)")

	# -------------------------------------------------------------------------
	# PASO 3: Guardar el mapa en disco y recargarlo (Persistencia)
	# -------------------------------------------------------------------------
	print("\n--- PASO 3: Guardar en disco y recargar mapa (Persistencia) ---")
	var save_path := "res://maps/test_paint_persistence.tscn"
	for i in range(map.get_child_count()):
		map.get_child(i).owner = map
	var packed_scene := PackedScene.new()
	var pack_err = packed_scene.pack(map)
	check(pack_err == OK, "Escena de mapa empaquetada correctamente")
	var save_err = ResourceSaver.save(packed_scene, save_path)
	check(save_err == OK, "Escena guardada exitosamente en " + save_path)

	# Recargar desde disco
	var reloaded_scene: PackedScene = ResourceLoader.load(save_path)
	check(reloaded_scene != null, "Escena recargada desde disco con éxito")
	var reloaded_map: ClockGearsMap = reloaded_scene.instantiate() as ClockGearsMap
	check(reloaded_map != null, "Instancia recargada es ClockGearsMap válido")
	reloaded_map.setup_tileset_from_database(reloaded_map.get_tileset_id())

	var reloaded_ground: TileMapLayer = reloaded_map.get_layer(0)
	check(reloaded_ground != null, "Capa_Suelo presente en mapa recargado")
	check(reloaded_ground.tile_set != null, "TileSet preservado tras recargar")

	var reloaded_cell: Vector2i = reloaded_map.get_tile_at(0, Vector2i(2, 3))
	check(reloaded_cell == Vector2i(0, 0), "Celda individual (2, 3) persiste tras recargar con tile (0, 0)")

	var reloaded_rect_ok: bool = true
	for ry in range(4, 7):
		for rx in range(4, 7):
			if reloaded_map.get_tile_at(0, Vector2i(rx, ry)) != Vector2i(1, 0):
				reloaded_rect_ok = false
	check(reloaded_rect_ok, "Rectángulo (4..6, 4..6) persiste tras recargar con todos sus tiles intactos")

	# -------------------------------------------------------------------------
	# PASO 4: Deshacer / Rehacer (Undo / Redo) de un trazo de pintado
	# -------------------------------------------------------------------------
	print("\n--- PASO 4: Deshacer (Undo) y Rehacer (Redo) de trazo ---")
	var ur := UndoRedo.new()
	var stroke_cells: Array[Vector2i] = []
	for sy in range(10, 13):
		for sx in range(10, 13):
			stroke_cells.push_back(Vector2i(sx, sy))

	# Registrar acción de pintado de un bloque 3x3 en (10..12, 10..12) con tile (2, 0)
	ur.create_action("Pintar Trazo Rectángulo")
	for c in stroke_cells:
		ur.add_do_method(Callable(layer_ground, "set_cell").bind(c, 0, Vector2i(2, 0), 0))
		ur.add_undo_method(Callable(layer_ground, "set_cell").bind(c, -1, Vector2i(-1, -1), 0))
	ur.add_do_method(Callable(layer_ground, "update_internals"))
	ur.add_undo_method(Callable(layer_ground, "update_internals"))
	ur.commit_action()

	var painted_before_undo: bool = true
	for c in stroke_cells:
		if map.get_tile_at(0, c) != Vector2i(2, 0):
			painted_before_undo = false
	check(painted_before_undo, "Trazo aplicado correctamente antes de Undo (todas las celdas en (2, 0))")

	# Ejecutar UNDO
	ur.undo()
	var undone_ok: bool = true
	for c in stroke_cells:
		if map.get_tile_at(0, c) != Vector2i(-1, -1):
			undone_ok = false
	check(undone_ok, "Undo revierte todo el trazo de una sola acción (celdas vacías)")

	# Ejecutar REDO
	ur.redo()
	var redone_ok: bool = true
	for c in stroke_cells:
		if map.get_tile_at(0, c) != Vector2i(2, 0):
			redone_ok = false
	check(redone_ok, "Redo restaura todo el trazo de una sola acción (todas las celdas en (2, 0))")

	# -------------------------------------------------------------------------
	# PASO 5: Goma de Borrar (Eraser)
	# -------------------------------------------------------------------------
	print("\n--- PASO 5: Goma de Borrar (Eraser) ---")
	layer_ground.set_cell(Vector2i(2, 3), -1, Vector2i(-1, -1), 0)
	layer_ground.update_internals()
	check(map.get_tile_at(0, Vector2i(2, 3)) == Vector2i(-1, -1), "Goma de borrar elimina el tile en (2, 3) dejando celda vacía")

	print("\n======================================================================")
	print("RESULTADO FINAL: ", total_passed, " PASSED | ", total_failed, " FAILED")
	print("======================================================================")

	if total_failed == 0:
		print("TEST FASE 6 (PINTADO, PERSISTENCIA Y UNDO/REDO) COMPLETADO CON ÉXITO")
	else:
		printerr("TEST FASE 6 CONTIENE ERRORES")

	quit(0 if total_failed == 0 else 1)
