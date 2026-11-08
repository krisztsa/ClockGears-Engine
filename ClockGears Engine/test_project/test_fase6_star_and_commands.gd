extends SceneTree

func _initialize() -> void:
	print("=== TEST ESPECÍFICO: TILE CON ★, PASABILIDAD Y Z-INDEX ===")
	
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if not dm:
		print("FAIL: DataManager no disponible")
		quit(1)
		return

	dm.init_empty()
	var path_a := "res://img/tilesets/test_forest_a.png"

	# Tileset 1 con tile (0,0) en ranura A5 marcado con Estrella (layer=1, mode=2)
	# y tile (1,0) marcado con Bloqueo (X, mode=1)
	# y tile (0,1) marcado con Pasable (O, mode=0)
	var slot_cols = {
		"A5": {
			"0,0": { "layer": 1, "mode": 2 }, # ★ Estrella: pasa por detrás, z_index=5, pasable=true
			"1,0": { "layer": 0, "mode": 1 }, # X Bloqueo: pasable=false
			"0,1": { "layer": 0, "mode": 0 }  # O Pasable: pasable=true
		}
	}
	var ts1_dict := {
		"id": 1,
		"name": "Bosque Estrella Test",
		"mode": 0,
		"slots": { "A5": path_a },
		"slot_collisions": slot_cols
	}
	dm.set_category_array(13, [ts1_dict])

	var map := ClockGearsMap.new()
	map.name = "MapStarTest"
	map.set_tileset_id(1)
	map.ensure_default_layers()
	map.setup_tileset_from_database(1)

	# 1. Comprobar que el tile con ★ en (0,0) tiene z_index = 5 en TileData (sobre jugador z=3)
	var suelo = map.get_layer(0)
	assert(suelo != null, "Capa suelo debe existir")
	var ts = suelo.tile_set
	assert(ts != null, "TileSet debe estar asignado")
	var source = ts.get_source(0) as TileSetAtlasSource
	assert(source != null, "AtlasSource 0 debe existir")
	var td_star = source.get_tile_data(Vector2i(0, 0), 0)
	assert(td_star != null, "TileData (0,0) debe existir")
	print("TileData (0,0) con ★ -> z_index: ", td_star.get_z_index())
	assert(td_star.get_z_index() == 5, "TileData (0,0) con ★ DEBE tener z_index = 5")

	var td_normal = source.get_tile_data(Vector2i(0, 1), 0)
	print("TileData (0,1) con O -> z_index: ", td_normal.get_z_index())
	assert(td_normal.get_z_index() == 0, "TileData normal con O debe tener z_index = 0")

	# 2. Pintar en celda (5, 5) el tile estrella (0, 0)
	suelo.set_cell(Vector2i(5, 5), 0, Vector2i(0, 0))

	# Pintar en celda (6, 6) el tile bloqueante (1, 0)
	suelo.set_cell(Vector2i(6, 6), 0, Vector2i(1, 0))

	# Pintar en celda (7, 7) el tile de suelo pasable (0, 1)
	suelo.set_cell(Vector2i(7, 7), 0, Vector2i(0, 1))

	# 3. Comprobar pasabilidad en runtime:
	var pass_star = map.is_tile_passable(Vector2i(5, 5))
	print("Pasabilidad celda (5,5) con ★: ", pass_star)
	assert(pass_star == true, "Celda con tile ★ DEBE ser pasable (el personaje pasa por detrás)")

	var pass_block = map.is_tile_passable(Vector2i(6, 6))
	print("Pasabilidad celda (6,6) con X: ", pass_block)
	assert(pass_block == false, "Celda con tile X debe bloquear")

	var pass_normal = map.is_tile_passable(Vector2i(7, 7))
	print("Pasabilidad celda (7,7) con O: ", pass_normal)
	assert(pass_normal == true, "Celda con tile O debe ser pasable")

	print("\n>>> PRUEBAS DE PASABILIDAD Y RENDERIZADO DE ★ SUPERADAS CON ÉXITO <<<")
	quit(0)
