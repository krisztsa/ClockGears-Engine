extends SceneTree

# Test Automatizado: FASE 6 - Simulación Real de Ratón en Editor (Lápiz, Rect, Elipse, Cubo, Goma), Undo/Redo y Base de Datos Sistema

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
	print("   TEST FASE 6: EVENTOS DE RATON EN EDITOR, UNDO/REDO Y SISTEMA")
	print("======================================================================")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton activo")
	dm.init_empty()

	DirAccess.make_dir_recursive_absolute("res://maps")
	DirAccess.make_dir_recursive_absolute("res://img/tilesets")

	# 1. Crear o asegurar imagen de tileset (128x128 = 4x4 tiles de 32x32)
	var img := Image.create_empty(128, 128, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.25, 0.65, 0.35, 1.0)) # Verde pasto
	# Marcar algunas cuadrículas para diferenciar
	for y in range(32):
		for x in range(32):
			img.set_pixel(x, y, Color(0.18, 0.55, 0.28, 1.0))
	var tex_path := "res://img/tilesets/forest-tilemap.png"
	img.save_png(tex_path)
	check(FileAccess.file_exists(tex_path), "Archivo de textura forest-tilemap.png creado en disco")

	# Registrar Tileset #1 en Base de Datos
	var ts_dict := {
		"id": 1,
		"name": "Bosque y Terreno Principal",
		"mode": 0,
		"slots": { "A5": tex_path },
		"slot_collisions": {}
	}
	dm.set_category_array(13, [ts_dict]) # CAT_TILESETS = 13
	dm.save_all("res://clockgears_data")

	# Configurar Sistema
	var sys := {
		"game_title": "Mi RPG ClockGears",
		"currency": "Oro",
		"combat_mode": 0,
		"start_map": "res://maps/map001.tscn",
		"has_start_pos": true,
		"start_x": 5,
		"start_y": 7,
		"start_dir": 2,
		"party": [1]
	}
	dm.set_system_config(sys)
	dm.save_all("res://clockgears_data")

	# 2. Instanciar ClockGearsMap y registrarlo en el árbol de escena
	var root := get_root()
	var map := ClockGearsMap.new()
	map.name = "Map001"
	map.set_map_name("Aldea Inicial")
	map.set_tileset_id(1)
	root.add_child(map)
	map.ensure_default_layers()
	map.setup_tileset_from_database(1)
	dm.set_editor_map(map)

	var layer_ground: TileMapLayer = map.get_layer(0)
	check(layer_ground != null, "Capa_Suelo existe en el mapa")
	check(layer_ground.tile_set != null, "Capa_Suelo tiene TileSet asignado")

	# Configurar editor activo en Capa 1 (Capa_Suelo)
	dm.set_editor_layer(1)

	# -------------------------------------------------------------------------
	# PRUEBA 1: Lápiz (draw_tool = 0) mediante Clic Real de Ratón
	# -------------------------------------------------------------------------
	print("\n>>> PRUEBA 1: Pintar con LAPIZ (draw_tool = 0) en celda (2, 2) <<<")
	dm.set_editor_draw_tool(0)
	dm.simulate_editor_click_cell(2, 2, false)

	var tile_pencil: Vector2i = map.get_tile_at(0, Vector2i(2, 2))
	print("  Resultado en mapa tras clic: celda (2, 2) = ", tile_pencil)
	check(tile_pencil != Vector2i(-1, -1), "El LAPIZ mediante clic real de raton guardo el tile en la Capa_Suelo")

	# -------------------------------------------------------------------------
	# PRUEBA 2: Deshacer (Undo / Ctrl+Z) y Rehacer (Redo / Ctrl+Y)
	# -------------------------------------------------------------------------
	print("\n>>> PRUEBA 2: Deshacer (Undo) y Rehacer (Redo) del Lápiz <<<")
	dm.trigger_editor_undo()
	var tile_undone: Vector2i = map.get_tile_at(0, Vector2i(2, 2))
	print("  Resultado tras trigger_editor_undo(): celda (2, 2) = ", tile_undone)
	check(tile_undone == Vector2i(-1, -1), "Deshacer (Undo) restauro la celda (2, 2) a vacia (-1, -1)")

	dm.trigger_editor_redo()
	var tile_redone: Vector2i = map.get_tile_at(0, Vector2i(2, 2))
	print("  Resultado tras trigger_editor_redo(): celda (2, 2) = ", tile_redone)
	check(tile_redone != Vector2i(-1, -1), "Rehacer (Redo) restauro el tile pintado en la celda (2, 2)")

	# -------------------------------------------------------------------------
	# PRUEBA 3: Rectángulo (draw_tool = 1) mediante Arrastre de Ratón
	# -------------------------------------------------------------------------
	print("\n>>> PRUEBA 3: Pintar con RECTANGULO (draw_tool = 1) de (4, 4) a (6, 6) <<<")
	dm.set_editor_draw_tool(1)
	dm.simulate_editor_drag_rect(4, 4, 6, 6, false)

	var all_rect_ok := true
	for rx in range(4, 7):
		for ry in range(4, 7):
			var rtile: Vector2i = map.get_tile_at(0, Vector2i(rx, ry))
			if rtile == Vector2i(-1, -1):
				all_rect_ok = false
	print("  Resultado rectangulo (4..6, 4..6): todas celdas pintadas = ", all_rect_ok)
	check(all_rect_ok, "El RECTANGULO mediante arrastre real de raton pinto y guardo las 9 celdas")

	# -------------------------------------------------------------------------
	# PRUEBA 4: Elipse (draw_tool = 2) mediante Arrastre de Ratón
	# -------------------------------------------------------------------------
	print("\n>>> PRUEBA 4: Pintar con ELIPSE (draw_tool = 2) de (8, 8) a (11, 11) <<<")
	dm.set_editor_draw_tool(2)
	dm.simulate_editor_drag_rect(8, 8, 11, 11, false)

	var ellipse_center: Vector2i = map.get_tile_at(0, Vector2i(9, 9))
	print("  Resultado elipse en centro (9, 9) = ", ellipse_center)
	check(ellipse_center != Vector2i(-1, -1), "La ELIPSE mediante arrastre real de raton pinto y guardo celdas")

	# -------------------------------------------------------------------------
	# PRUEBA 5: Cubo de Relleno (draw_tool = 3) mediante Clic de Ratón
	# -------------------------------------------------------------------------
	print("\n>>> PRUEBA 5: Pintar con CUBO (draw_tool = 3) en celda (5, 5) <<<")
	dm.set_editor_draw_tool(3)
	dm.simulate_editor_click_cell(5, 5, false)

	var bucket_tile: Vector2i = map.get_tile_at(0, Vector2i(5, 5))
	print("  Resultado cubo en celda (5, 5) = ", bucket_tile)
	check(bucket_tile != Vector2i(-1, -1), "El CUBO DE RELLENO mediante clic real de raton ejecuto el llenado y guardo")

	# -------------------------------------------------------------------------
	# PRUEBA 6: Goma de Borrar (draw_tool = 7) mediante Clic de Ratón
	# -------------------------------------------------------------------------
	print("\n>>> PRUEBA 6: Borrar con GOMA (draw_tool = 7) en celda (2, 2) <<<")
	dm.set_editor_draw_tool(7)
	dm.simulate_editor_click_cell(2, 2, false)

	var erased_tile: Vector2i = map.get_tile_at(0, Vector2i(2, 2))
	print("  Resultado goma en celda (2, 2) = ", erased_tile)
	check(erased_tile == Vector2i(-1, -1), "La GOMA mediante clic real de raton borro el tile (-1, -1)")

	# -------------------------------------------------------------------------
	# PRUEBA 7: Persistencia en Disco (.tscn)
	# -------------------------------------------------------------------------
	print("\n>>> PRUEBA 7: Persistencia del mapa pintado al guardar y recargar <<<")
	var packed_scene := PackedScene.new()
	layer_ground.owner = map
	for c in map.get_children():
		c.owner = map
	var pack_err := packed_scene.pack(map)
	check(pack_err == OK, "Escena empaquetada con exito")

	var save_path := "res://maps/map001.tscn"
	var save_err := ResourceSaver.save(packed_scene, save_path)
	check(save_err == OK, "Escena guardada en disco: " + save_path)

	var loaded_scene: PackedScene = ResourceLoader.load(save_path, "", ResourceLoader.CACHE_MODE_IGNORE)
	check(loaded_scene != null, "Escena recargada desde disco")
	var reloaded_map: ClockGearsMap = loaded_scene.instantiate() as ClockGearsMap
	check(reloaded_map != null, "Escena instanciada como ClockGearsMap")

	var reloaded_rect_cell: Vector2i = reloaded_map.get_tile_at(0, Vector2i(4, 4))
	print("  Celda (4, 4) tras recarga de disco = ", reloaded_rect_cell)
	check(reloaded_rect_cell != Vector2i(-1, -1), "Los tiles pintados persisten intactos al guardar y recargar de disco")

	# -------------------------------------------------------------------------
	# PRUEBA 8: Base de Datos > Sistema (Problema 2)
	# -------------------------------------------------------------------------
	print("\n>>> PRUEBA 8: Verificación Base de Datos > Sistema <<<")
	var loaded_sys: Dictionary = dm.get_system_config()
	check(loaded_sys.get("game_title", "") == "Mi RPG ClockGears", "Titulo de juego persistido en Sistema")
	check(loaded_sys.get("currency", "") == "Oro", "Unidad monetaria persistida en Sistema")
	check(loaded_sys.get("start_map", "") == "res://maps/map001.tscn", "Mapa inicial asignado en Sistema")
	check(int(loaded_sys.get("start_x", 0)) == 5, "Posición inicial X sincronizada en Sistema (5)")
	check(int(loaded_sys.get("start_y", 0)) == 7, "Posición inicial Y sincronizada en Sistema (7)")

	# Probar sincronización al cambiar posición en mapa
	map.set_player_start_position(Vector2i(9, 12), 4)
	var updated_sys: Dictionary = dm.get_system_config()
	print("  Posición inicial actualizada desde mapa: X=", updated_sys.get("start_x"), ", Y=", updated_sys.get("start_y"))
	check(int(updated_sys.get("start_x", 0)) == 9 and int(updated_sys.get("start_y", 0)) == 12, "Posición inicial del jugador se sincronizo bidireccionalmente con Sistema")

	reloaded_map.queue_free()
	map.queue_free()

	print("\n======================================================================")
	print("   RESUMEN FINAL: ", total_passed, " PASSED / ", total_failed, " FAILED")
	print("======================================================================")
	if total_failed == 0:
		print(">>> TODOS LOS TESTS DE FASE 6 PASARON CON EXITO <<<")
		quit(0)
	else:
		printerr(">>> HUBO FALLOS EN FASE 6 <<<")
		quit(1)
