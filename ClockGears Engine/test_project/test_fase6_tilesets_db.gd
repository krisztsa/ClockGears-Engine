extends SceneTree

func _init():
	print("======================================================================")
	print("   TEST FASE 6: BASE DE DATOS > TILESETS & MAP RUNTIME (PROBLEMAS A/B/C/3)")
	print("======================================================================")
	var failures: int = 0

	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.init_empty()

	var db = ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.select_category(12) # CAT_TILESETS

	# ------------------------------------------------------------------
	# 1. PROBLEMA A: Ajuste de los 3 paneles (820x480 y 1440x860)
	#    y persistencia del tamaño de la ventana.
	# ------------------------------------------------------------------
	print("\n--- 1. Verificando PROBLEMA A (Layout en 820x480 y 1440x860 + Persistencia tamaño) ---")
	var small_fit: Dictionary = db.check_layout_fits_window(Vector2i(820, 480))
	print("[Layout 820x480] ", small_fit)
	if not small_fit.get("fits_horizontally", false):
		print("FALLO: Los 3 paneles desbordan en ventana pequeña 820x480.")
		failures += 1
	if not small_fit.get("help_wraps", false) or not small_fit.get("picker_has_scrollbars", false):
		print("FALLO: Falta salto de línea en ayuda o scrollbars en el visor.")
		failures += 1

	var large_fit: Dictionary = db.check_layout_fits_window(Vector2i(1440, 860))
	print("[Layout 1440x860] ", large_fit)
	if not large_fit.get("fits_horizontally", false):
		print("FALLO: Los 3 paneles desbordan en ventana grande 1440x860.")
		failures += 1

	db.set_remembered_window_size(Vector2i(1280, 760))
	var rem_size: Vector2i = db.get_remembered_window_size()
	print("[Window Size Persistence] Guardado=(1280, 760) -> Leído=", rem_size)
	if rem_size != Vector2i(1280, 760):
		print("FALLO: La ventana de Base de Datos no recordó su tamaño.")
		failures += 1

	# ------------------------------------------------------------------
	# 2. PROBLEMA B: Botones '+' y '-' de la lista + Importación
	#    con error claro en archivo inválido y pasos en consola.
	# ------------------------------------------------------------------
	print("\n--- 2. Verificando PROBLEMA B (Botones +/- e importador con validación y consola) ---")
	if db.get_tileset_count() != 0:
		print("FALLO: Regla Cero violada, había tilesets iniciales.")
		failures += 1

	var id1: int = db.add_tileset_record("Tileset Temporal")
	var id2: int = db.add_tileset_record("Mazmorra Clockwork")
	print("[Lista Tilesets] Creados IDs: ", id1, ", ", id2, " | Total=", db.get_tileset_count())
	if db.get_tileset_count() != 2:
		print("FALLO: El botón '+' no creó los tilesets.")
		failures += 1

	db.remove_tileset_record(0)
	print("[Lista Tilesets] Eliminado índice 0 | Total restante=", db.get_tileset_count())
	if db.get_tileset_count() != 1:
		print("FALLO: El botón '-' no eliminó el tileset.")
		failures += 1

	# Probar archivo inválido (debe fallar limpiamente e imprimir causa)
	var bad_path = "res://archivo_inexistente_corrupto.png"
	var bad_ok: bool = db.import_tileset_slot("A5", bad_path)
	print("[Importación inválida] resultado=", bad_ok, " (esperado: false)")
	if bad_ok:
		print("FALLO: Se aceptó un archivo inexistente.")
		failures += 1

	# ------------------------------------------------------------------
	# 3. PROBLEMA C: 9 ranuras (A1-A5, B, C, D, E), carga en A5 y B,
	#    marcado O/X/★ por arrastre (drag-painting), guardado y recarga.
	# ------------------------------------------------------------------
	print("\n--- 3. Verificando PROBLEMA C (Carga en A5 y B + Drag-Painting O/X/★ + Persistencia) ---")
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://tmp_import_src"))

	# Crear PNG de prueba para A5 (128x64 -> 4x2 = 8 tiles de 32x32)
	var img_a5 = Image.create(128, 64, false, Image.FORMAT_RGBA8)
	img_a5.fill(Color(0.20, 0.55, 0.35, 1.0))
	var src_a5 = ProjectSettings.globalize_path("res://tmp_import_src/suelo_a5_test.png")
	img_a5.save_png(src_a5)

	# Crear PNG de prueba para B (96x96 -> 3x3 = 9 tiles de 32x32)
	var img_b = Image.create(96, 96, false, Image.FORMAT_RGBA8)
	img_b.fill(Color(0.75, 0.45, 0.20, 1.0))
	var src_b = ProjectSettings.globalize_path("res://tmp_import_src/objetos_b_test.png")
	img_b.save_png(src_b)

	var ok_a5: bool = db.import_tileset_slot("A5", src_a5)
	var ok_b: bool = db.import_tileset_slot("B", src_b)
	print("[Carga A5 y B] ok_a5=", ok_a5, " (", db.get_tileset_slot_path(0, "A5"), ") | ok_b=", ok_b, " (", db.get_tileset_slot_path(0, "B"), ")")
	if not ok_a5 or not ok_b:
		print("FALLO: No se pudieron importar las hojas en A5 y B.")
		failures += 1

	# Marcar por ARRASTRE (drag-painting) en A5:
	# Arrastrar sobre fila superior: tiles (0,0) a (1,0) marcados como X (bloqueado, passage=0)
	print("[Drag-Painting A5] Marcando celdas (0,0) a (1,0) con [X / Bloqueado]...")
	db.drag_paint_tiles_meta("A5", Vector2i(0, 0), Vector2i(1, 0), {"passage": 0, "layer": 0, "mode": 1})

	# Arrastrar sobre fila superior: tiles (2,0) a (3,0) marcados como O (suelo pasable, passage=15)
	print("[Drag-Painting A5] Marcando celdas (2,0) a (3,0) con [O / Pasable]...")
	db.drag_paint_tiles_meta("A5", Vector2i(2, 0), Vector2i(3, 0), {"passage": 15, "layer": 0, "mode": 0})

	# Marcar por ARRASTRE (drag-painting) en B:
	# Arrastrar sobre tiles (0,1) a (1,1) marcados como ★ (layer=1: techo/pasa por detrás)
	print("[Drag-Painting B] Marcando celdas (0,1) a (1,1) con [★ / Pasa por detrás (layer=1)]...")
	db.drag_paint_tiles_meta("B", Vector2i(0, 1), Vector2i(1, 1), {"passage": 15, "layer": 1, "bush": true})

	# Guardar en disco, limpiar memoria y recargar desde disco
	db.save_to_disk()
	dm.init_empty()
	db.load_from_disk()
	db.select_category(12)
	print("[Persistencia Disco] Guardado y recargado desde res://clockgears_data/tilesets_data.json | Total tilesets=", db.get_tileset_count())
	if db.get_tileset_count() != 1:
		print("FALLO: Error al guardar o recargar tilesets_data.json.")
		failures += 1

	var path_a5_after: String = db.get_tileset_slot_path(0, "A5")
	var path_b_after: String = db.get_tileset_slot_path(0, "B")
	var meta_a5_00: Dictionary = db.get_tileset_slot_tile_meta("A5", Vector2i(0, 0))
	var meta_a5_10: Dictionary = db.get_tileset_slot_tile_meta("A5", Vector2i(1, 0))
	var meta_a5_20: Dictionary = db.get_tileset_slot_tile_meta("A5", Vector2i(2, 0))
	var meta_a5_30: Dictionary = db.get_tileset_slot_tile_meta("A5", Vector2i(3, 0))
	var meta_b_01: Dictionary = db.get_tileset_slot_tile_meta("B", Vector2i(0, 1))
	var meta_b_11: Dictionary = db.get_tileset_slot_tile_meta("B", Vector2i(1, 1))

	print("[Verificación Drag-Painting Post-Recarga]")
	print("  A5 (0,0) [esperado X/passage=0]: ", meta_a5_00)
	print("  A5 (1,0) [esperado X/passage=0]: ", meta_a5_10)
	print("  A5 (2,0) [esperado O/passage=15]: ", meta_a5_20)
	print("  A5 (3,0) [esperado O/passage=15]: ", meta_a5_30)
	print("  B  (0,1) [esperado ★/layer=1]: ", meta_b_01)
	print("  B  (1,1) [esperado ★/layer=1]: ", meta_b_11)

	if int(meta_a5_00.get("passage", -1)) != 0 or int(meta_a5_10.get("passage", -1)) != 0:
		print("FALLO: Drag-painting de X en A5 no persistió en ambas celdas.")
		failures += 1
	if int(meta_a5_20.get("passage", -1)) != 15 or int(meta_a5_30.get("passage", -1)) != 15:
		print("FALLO: Drag-painting de O en A5 no persistió en ambas celdas.")
		failures += 1
	if int(meta_b_01.get("layer", -1)) != 1 or int(meta_b_11.get("layer", -1)) != 1:
		print("FALLO: Drag-painting de ★ en B no persistió en ambas celdas.")
		failures += 1

	# ------------------------------------------------------------------
	# 4. PROBLEMA 3: Asignar Tileset al Mapa, Pintar Tiles y Probar
	#    Runtime Collision (X bloquea, O pasa, ★ pasa por detrás).
	# ------------------------------------------------------------------
	print("\n--- 4. Verificando PROBLEMA 3 (Asignar Tileset a Mapa, Pintado y Runtime Collision) ---")
	var map = ClockGearsMap.new()
	map.set_name("MapaTestFase6")
	map.set_map_size(Vector2i(25, 20))
	map.set_tileset_id(id2) # Mazmorra Clockwork (ID 2 o correspondiente)
	root.add_child(map)
	map.ensure_default_layers()

	# Importar textura en el mapa para inicializar TileSet atlas
	var map_ts_ok = map.import_tileset_png(src_a5)
	print("[Mapa import_tileset_png] ok=", map_ts_ok, " | Map Tileset ID=", map.get_tileset_id())
	if not map_ts_ok:
		print("FALLO: El mapa no pudo importar el tileset atlas.")
		failures += 1

	# Pintar tiles en el mapa:
	# Celda (1, 1) -> Tile de A5 coords (0,0) [X / Bloqueado]
	# Celda (1, 2) -> Tile de A5 coords (2,0) [O / Pasable]
	# Celda (1, 3) -> Tile de B coords (0,1) [★ / Pasa por detrás]
	print("[Pintado de Tiles en Mapa]")
	map.set_tile_at(0, Vector2i(1, 1), Vector2i(0, 0)) # Capa suelo -> X
	map.set_tile_at(0, Vector2i(1, 2), Vector2i(2, 0)) # Capa suelo -> O
	map.set_tile_at(0, Vector2i(1, 3), Vector2i(0, 1)) # Capa suelo solicitada -> se desvía a LAYER_OVERHEAD (★)
	print("  Celda (1,1) pintada con atlas coords (0,0) [X]")
	print("  Celda (1,2) pintada con atlas coords (2,0) [O]")
	print("  Celda (1,3) pintada con atlas coords (0,1) [★]")

	# Verificar que el tile ★ fue colocado en LAYER_OVERHEAD (capa 3 con z_index 4)
	var overhead_layer = map.get_layer(3) # LAYER_OVERHEAD
	var overhead_coords = overhead_layer.get_cell_atlas_coords(Vector2i(1, 3))
	print("[Capa Superior ★] Coordenadas en LAYER_OVERHEAD celda (1,3)=", overhead_coords, " (z_index=", overhead_layer.get_z_index(), ")")
	if overhead_coords != Vector2i(0, 1):
		print("FALLO: El tile ★ no se colocó automáticamente en LAYER_OVERHEAD.")
		failures += 1
	if overhead_layer.get_z_index() <= 3:
		print("FALLO: LAYER_OVERHEAD z_index debe ser > 3 para dibujarse sobre el personaje.")
		failures += 1

	# Comprobar pasabilidad a nivel de mapa
	var pass_x = map.is_tile_passable(Vector2i(1, 1))
	var pass_o = map.is_tile_passable(Vector2i(1, 2))
	var pass_star = map.is_tile_passable(Vector2i(1, 3))
	print("[Pasabilidad Mapa] Celda (1,1) X bloqueado=", not pass_x, " | Celda (1,2) O pasable=", pass_o, " | Celda (1,3) ★ pasable=", pass_star)
	if pass_x:
		print("FALLO: Celda (1,1) con X no bloquea en el mapa.")
		failures += 1
	if not pass_o:
		print("FALLO: Celda (1,2) con O bloquea indebidamente en el mapa.")
		failures += 1
	if not pass_star:
		print("FALLO: Celda (1,3) con ★ bloquea indebidamente en el mapa.")
		failures += 1

	# Comprobar en Runtime con ClockGearsPlayer
	var player = ClockGearsPlayer.new()
	player.set_name("PlayerTest")
	player.set_z_index(3) # z_index estándar del jugador
	map.add_child(player)
	player.setup_on_map(map, null, null)

	var can_to_x = player.can_pass(Vector2i(1, 0), Vector2i(1, 1), 2)
	var can_to_o = player.can_pass(Vector2i(1, 0), Vector2i(1, 2), 2)
	var can_to_star = player.can_pass(Vector2i(1, 0), Vector2i(1, 3), 2)
	print("[Runtime Collision Player]")
	print("  Jugador en (1,0) intentando mover a (1,1) [X]: permitido=", can_to_x, " (esperado: false [BLOQUEADO])")
	print("  Jugador en (1,0) intentando mover a (1,2) [O]: permitido=", can_to_o, " (esperado: true [PASABLE])")
	print("  Jugador en (1,0) intentando mover a (1,3) [★]: permitido=", can_to_star, " (esperado: true [PASA POR DETRÁS])")

	if can_to_x:
		print("FALLO: El jugador pudo atravesar el tile X en runtime.")
		failures += 1
	if not can_to_o:
		print("FALLO: El jugador no pudo pasar por el tile O en runtime.")
		failures += 1
	if not can_to_star:
		print("FALLO: El jugador no pudo pasar por el tile ★ en runtime.")
		failures += 1

	# Test solicitado: pintar con el lápiz una celda, leerla con get_tile_at y comprobar que coincide;
	# guardar, reabrir y comprobar que sigue.
	print("[Test Persistencia Pintado Lápiz en Mapa]")
	map.set_tile_at(0, Vector2i(4, 4), Vector2i(2, 0))
	var tile_pencil_read = map.get_tile_at(0, Vector2i(4, 4))
	print("  Tile pintado con lápiz en Capa 0, Celda (4,4): leído = ", tile_pencil_read, " (esperado: (2, 0))")
	if tile_pencil_read != Vector2i(2, 0):
		print("FALLO: get_tile_at(0, (4,4)) no coincide con el tile pintado.")
		failures += 1

	DirAccess.make_dir_recursive_absolute("res://maps")
	for child in map.get_children():
		child.set_owner(map)

	var packed_pencil = PackedScene.new()
	var pack_res = packed_pencil.pack(map)
	if pack_res != OK:
		print("FALLO: packed_pencil.pack falló con código: ", pack_res)
		failures += 1
	else:
		var save_res = ResourceSaver.save(packed_pencil, "res://maps/map_pencil_test.tscn")
		print("  Escena guardada en res://maps/map_pencil_test.tscn (res=", save_res, ")")
		if save_res != OK:
			print("FALLO: Error al guardar res://maps/map_pencil_test.tscn: ", save_res)
			failures += 1
		else:
			var reloaded_packed = ResourceLoader.load("res://maps/map_pencil_test.tscn", "PackedScene", ResourceLoader.CACHE_MODE_IGNORE)
			if reloaded_packed == null:
				print("FALLO: No se pudo recargar res://maps/map_pencil_test.tscn")
				failures += 1
			else:
				var reloaded_map = reloaded_packed.instantiate()
				var reloaded_tile = reloaded_map.get_tile_at(0, Vector2i(4, 4))
				print("  Tile recargado desde disco en Capa 0, Celda (4,4): ", reloaded_tile, " (esperado: (2, 0))")
				if reloaded_tile != Vector2i(2, 0):
					print("FALLO: El tile persistido no coincide tras recargar la escena de disco.")
					failures += 1
				reloaded_map.queue_free()

	# Limpieza
	player.queue_free()
	map.queue_free()
	db.queue_free()

	print("======================================================================")
	if failures == 0:
		print("RESULTADO FINAL: TODOS LOS TESTS DE TILESETS DB Y RUNTIME PASARON (0 FALLOS)")
		quit(0)
	else:
		print("RESULTADO FINAL: FALLARON ", failures, " COMPROBACIONES")
		quit(1)
