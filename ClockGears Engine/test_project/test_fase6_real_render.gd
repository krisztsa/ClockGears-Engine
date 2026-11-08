extends SceneTree

# Test Automatizado: FASE 6 - Render Real de Viewport Texture (Lectura de Pixeles en GPU)
# Requerimiento 4:
# - SubViewport con renderizado activo (no dummy, UPDATE_ALWAYS)
# - Pinta un tile en una celda concreta
# - Fuerza el renderizado de al menos 2 frames
# - Lee con get_texture().get_image() el pixel del centro de la celda pintada
# - Comprueba que el pixel NO es el color de fondo y que coincide con el tile (con tolerancia)

var total_passed: int = 0
var total_failed: int = 0
var step: int = 0
var map: ClockGearsMap
var vp: SubViewport
var bg_color: Color
var tile_color: Color = Color(0.15, 0.85, 0.25, 1.0) # Verde distintivo

func check(condition: bool, msg: String) -> void:
	if condition:
		total_passed += 1
		print("  [PASS] ", msg)
	else:
		total_failed += 1
		printerr("  [FAIL] ", msg)

func _initialize() -> void:
	print("======================================================================")
	print("   TEST FASE 6: RENDER REAL DE VIEWPORT - LECTURA DE PIXELES GPU")
	print("======================================================================")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton activo")
	dm.init_empty()

	DirAccess.make_dir_recursive_absolute("res://maps")
	DirAccess.make_dir_recursive_absolute("res://img/tilesets")

	# 1. Crear imagen de textura con color uniforme de prueba
	var img := Image.create_empty(64, 64, false, Image.FORMAT_RGBA8)
	img.fill(tile_color)
	var tex_path := "res://img/tilesets/test_real_render_sheet.png"
	img.save_png(tex_path)
	check(FileAccess.file_exists(tex_path), "Textura de prueba guardada en " + tex_path)

	# 2. Registrar en Base de Datos de Tilesets
	var ts_dict := {
		"id": 1,
		"name": "Tileset Render Real",
		"mode": 0,
		"slots": { "A5": tex_path },
		"slot_collisions": {}
	}
	dm.set_category_array(13, [ts_dict])

	# 3. SubViewport dedicado con renderizado activo (no dummy)
	vp = SubViewport.new()
	vp.size = Vector2i(256, 256)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	vp.render_target_clear_mode = SubViewport.CLEAR_MODE_ALWAYS
	get_root().add_child(vp)

	# 4. Crear mapa y capas
	map = ClockGearsMap.new()
	map.name = "Map001"
	map.set_map_name("Mapa Render Test")
	map.set_tileset_id(1)
	vp.add_child(map)
	map.ensure_default_layers()
	map.setup_tileset_from_database(1)

	var layer_ground: TileMapLayer = map.get_layer(0)
	check(layer_ground != null, "Capa_Suelo inicializada")
	check(layer_ground.tile_set != null, "Capa_Suelo tiene TileSet asignado")

	print("Configuración inicial completada. Iniciando ciclo de renderizado en GPU...")

func _process(delta: float) -> bool:
	step += 1

	if step == 2:
		# Pintar celda concreta (1, 1)
		print("\n>>> PINTANDO TILE EN CELDA (1, 1) CON TILE (0, 0) <<<")
		var layer_ground: TileMapLayer = map.get_layer(0)
		layer_ground.set_cell(Vector2i(1, 1), 0, Vector2i(0, 0))
		layer_ground.update_internals()
		layer_ground.queue_redraw()

		check(layer_ground.get_cell_source_id(Vector2i(1, 1)) == 0, "Capa_Suelo registró tile en celda (1, 1)")

	elif step == 6: # Forzar al menos 2-4 frames de renderizado activo
		print("\n>>> VERIFICANDO LECTURA DE PIXELES TRAS RENDERIZADO GPU (FRAME %d) <<<" % step)
		var tex = vp.get_texture()
		check(tex != null, "ViewportTexture es válida tras varios frames de renderizado")
		if tex:
			var img = tex.get_image()
			check(img != null && !img.is_empty(), "Imagen obtenida de ViewportTexture vía GPU")
			if img:
				# Centro de la celda pintada (1, 1): x = 1*32 + 16 = 48, y = 1*32 + 16 = 48
				var p_cell = img.get_pixel(48, 48)
				# Celda vacía (5, 5): x = 5*32 + 16 = 176, y = 5*32 + 16 = 176
				var p_empty = img.get_pixel(176, 176)
				bg_color = p_empty

				print("  Pixel leído en centro de celda pintada (1, 1) [48, 48]: ", p_cell)
				print("  Pixel leído en celda vacía / fondo (5, 5) [176, 176]:    ", p_empty)
				print("  Color esperado del tile:                                 ", tile_color)

				check(p_cell != bg_color, "El pixel en el centro de la celda NO es el color de fondo")
				var diff_r = abs(p_cell.r - tile_color.r)
				var diff_g = abs(p_cell.g - tile_color.g)
				var diff_b = abs(p_cell.b - tile_color.b)
				var match_ok = (diff_r < 0.05 && diff_g < 0.05 && diff_b < 0.05)
				check(match_ok, "El pixel coincide con el color del tile (Tolerancia < 0.05, Dif: R=%.3f, G=%.3f, B=%.3f)" % [diff_r, diff_g, diff_b])
				check(p_cell.g > 0.7 && p_cell.r < 0.3, "Canal verde predominante (> 0.7) correspondiente al sprite pintado")

		print("\n======================================================================")
		print("   RESUMEN FINAL TEST RENDER REAL: %d PASSED / %d FAILED" % [total_passed, total_failed])
		print("======================================================================")
		check(total_failed == 0, "Todos los checks de render real pasaron satisfactoriamente")
		quit(0 if total_failed == 0 else 1)
		return true

	return false
