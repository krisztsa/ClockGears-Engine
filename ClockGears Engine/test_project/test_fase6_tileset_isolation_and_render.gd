extends SceneTree

# Test Automatizado: FASE 6 - Corrección de Problemas 1 y 2
# PROBLEMA 1: El editor dibuja los tiles pintados inmediatamente, orden correcto de capas y persistencia.
# PROBLEMA 2: Cada mapa tiene su propio TileSet independiente; Mapa con 0 ("Ninguno") queda vacío y sin heredar.

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
	print("=== INICIANDO TEST: INDEPENDENCIA DE TILESETS Y RENDERIZADO INMEDIATO ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton activo")
	dm.init_empty()

	DirAccess.make_dir_recursive_absolute("res://maps")
	DirAccess.make_dir_recursive_absolute("res://img/tilesets")

	# Crear dos texturas PNG de prueba de 64x64 (4 tiles 32x32 cada una) con colores distinguibles
	var img_a := Image.create_empty(64, 64, false, Image.FORMAT_RGBA8)
	img_a.fill(Color(0.2, 0.8, 0.2, 1.0)) # Verde (Pasto A)
	var path_a := "res://img/tilesets/test_forest_a.png"
	img_a.save_png(path_a)

	var img_b := Image.create_empty(64, 64, false, Image.FORMAT_RGBA8)
	img_b.fill(Color(0.8, 0.4, 0.1, 1.0)) # Naranja (Desierto B)
	var path_b := "res://img/tilesets/test_desert_b.png"
	img_b.save_png(path_b)

	# Configurar Base de Datos con 2 Tilesets distintos
	# Tileset 1: "Bosque" con test_forest_a.png en ranura A5
	var ts1_dict := {
		"id": 1,
		"name": "Bosque Exterior",
		"mode": 0,
		"slots": { "A5": path_a },
		"slot_collisions": {}
	}
	# Tileset 2: "Desierto" con test_desert_b.png en ranura A5
	var ts2_dict := {
		"id": 2,
		"name": "Desierto Cálido",
		"mode": 0,
		"slots": { "A5": path_b },
		"slot_collisions": {}
	}

	var ts_arr: Array = [ts1_dict, ts2_dict]
	dm.set_category_array(13, ts_arr) # CAT_TILESETS = 13

	# -------------------------------------------------------------------------
	# PRUEBA PROBLEMA 1: Visualización, renderizado y capas del mapa
	# -------------------------------------------------------------------------
	print("\n--- PRUEBA 1: Capas, Renderizado Inmediato y Persistencia ---")

	var map1 := ClockGearsMap.new()
	map1.name = "Map001"
	map1.set_map_name("Mapa Bosque")
	map1.set_tileset_id(1)
	map1.ensure_default_layers()
	map1.setup_tileset_from_database(1)

	var l_ground: TileMapLayer = map1.get_layer(0) # Suelo
	var l_deco: TileMapLayer = map1.get_layer(1)   # Decoracion
	var l_obj: TileMapLayer = map1.get_layer(2)    # Objetos
	var l_roof: TileMapLayer = map1.get_layer(3)   # Techo

	check(l_ground != null and l_deco != null and l_obj != null and l_roof != null, "Las 4 capas TileMapLayer existen y están vinculadas")
	check(l_ground.z_index == 0, "Orden de capas: Capa_Suelo está abajo (Z = 0)")
	check(l_deco.z_index == 1, "Orden de capas: Capa_Decoracion en Z = 1")
	check(l_obj.z_index == 2, "Orden de capas: Capa_Objetos en Z = 2")
	check(l_roof.z_index == 4, "Orden de capas: Capa_Techo está arriba (Z = 4)")

	check(l_ground.tile_set != null, "Capa_Suelo tiene TileSet asignado")
	check(l_ground.visible and l_deco.visible and l_obj.visible and l_roof.visible, "Todas las capas son visibles por defecto en el lienzo")

	# Pintar un tile con la herramienta (simulado por set_tile_at o set_cell)
	map1.set_tile_at(0, Vector2i(2, 3), Vector2i(0, 0))
	l_ground.update_internals()
	l_ground.queue_redraw()

	var used_cells: Array[Vector2i] = l_ground.get_used_cells()
	check(used_cells.has(Vector2i(2, 3)), "La celda (2, 3) está registrada en get_used_cells() tras pintar")
	check(map1.get_tile_at(0, Vector2i(2, 3)) == Vector2i(0, 0), "get_tile_at coincide exactamente con el tile pintado")

	# Verificar que el TileSet contiene la fuente y textura
	var ts_res: TileSet = l_ground.tile_set
	check(ts_res.has_source(0), "El TileSet contiene la fuente de atlas 0 (ranura A)")
	var atlas_src: TileSetAtlasSource = ts_res.get_source(0) as TileSetAtlasSource
	check(atlas_src != null and atlas_src.texture != null, "El AtlasSource tiene textura válida asignada")

	# Guardar el mapa a disco
	for i in range(map1.get_child_count()):
		map1.get_child(i).owner = map1
	var packed1 := PackedScene.new()
	packed1.pack(map1)
	ResourceSaver.save(packed1, "res://maps/map001.tscn")

	# Reabrir el mapa guardado desde disco y comprobar que sus tiles y TileSet se reconstituyen
	var reloaded_scene: PackedScene = ResourceLoader.load("res://maps/map001.tscn")
	check(reloaded_scene != null, "El archivo de escena res://maps/map001.tscn se carga correctamente")
	var map1_loaded: ClockGearsMap = reloaded_scene.instantiate() as ClockGearsMap
	check(map1_loaded != null, "La escena instanciada es un ClockGearsMap")
	map1_loaded.setup_tileset_from_database(map1_loaded.get_tileset_id())

	var ground_loaded: TileMapLayer = map1_loaded.get_layer(0)
	check(ground_loaded != null, "Capa_Suelo existe en el mapa recargado")
	check(ground_loaded.get_used_cells().has(Vector2i(2, 3)), "El tile pintado en (2, 3) persiste al reabrir el mapa")
	check(ground_loaded.tile_set != null, "El TileSet está asignado y listo en el editor sin necesidad de F5")

	# -------------------------------------------------------------------------
	# PRUEBA PROBLEMA 2: Independencia estricta de TileSet por mapa
	# -------------------------------------------------------------------------
	print("\n--- PRUEBA 2: Aislamiento estricto de TileSet por Mapa ---")

	# Crear Mapa 002 con tileset 0 ("(000: Ninguno)")
	var map2 := ClockGearsMap.new()
	map2.name = "Map002"
	map2.set_map_name("Mapa Vacio Sin TileSet")
	map2.set_tileset_id(0) # 0 = Ninguno
	map2.ensure_default_layers()
	map2.setup_tileset_from_database(0)

	var map2_ground: TileMapLayer = map2.get_layer(0)
	check(map2.get_tileset_id() == 0, "Mapa 002 tiene tileset_id = 0")
	check(map2_ground.tile_set == null, "Mapa 002 con tileset 0 NO tiene TileSet (tile_set es nulo)")

	# Comprobar que Mapa 002 NO heredó el tileset de Mapa 001
	check(map2_ground.tile_set != l_ground.tile_set, "Aislamiento: Mapa 002 no hereda el TileSet de Mapa 001")

	# Crear Mapa 003 con tileset 2 ("Desierto Cálido")
	var map3 := ClockGearsMap.new()
	map3.name = "Map003"
	map3.set_map_name("Mapa Desierto")
	map3.set_tileset_id(2)
	map3.ensure_default_layers()
	map3.setup_tileset_from_database(2)

	var map3_ground: TileMapLayer = map3.get_layer(0)
	check(map3.get_tileset_id() == 2, "Mapa 003 tiene tileset_id = 2")
	check(map3_ground.tile_set != null, "Mapa 003 tiene TileSet propio")
	check(map3_ground.tile_set != l_ground.tile_set, "TileSet de Mapa 003 es una instancia distinta a la de Mapa 001")

	var ts3_atlas: TileSetAtlasSource = map3_ground.tile_set.get_source(0) as TileSetAtlasSource
	check(ts3_atlas != null and (ts3_atlas.texture.resource_path.ends_with("test_desert_b.png") or ts3_atlas.texture.get_path().ends_with("test_desert_b.png")), "Mapa 003 usa la textura correspondiente al Tileset 2 (test_desert_b.png)")

	# Cambiar Mapa 002 a Tileset 2 y verificar que no altera a Mapa 001
	map2.setup_tileset_from_database(2)
	check(map2.get_tileset_id() == 2, "Mapa 002 actualizado a Tileset 2")
	check(map1.get_tileset_id() == 1, "Mapa 001 conserva inalterado su tileset_id = 1")
	check(l_ground.get_used_cells().has(Vector2i(2, 3)), "Los tiles pintados en Mapa 001 no fueron alterados por cambios en Mapa 002")

	# Limpiar memoria
	map1.free()
	map1_loaded.free()
	map2.free()
	map3.free()

	print("\n=======================================================")
	print("RESUMEN DE PRUEBAS:")
	print("  TOTAL PASADAS: ", total_passed)
	print("  TOTAL FALLIDAS: ", total_failed)
	print("=======================================================")

	if total_failed == 0:
		print(">> TODAS LAS PRUEBAS DE LA FASE 6 PASARON CON ÉXITO <<")
		quit(0)
	else:
		printerr(">> ALGUNAS PRUEBAS FALLARON <<")
		quit(1)
