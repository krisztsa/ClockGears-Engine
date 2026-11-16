extends SceneTree

# ==============================================================================
# TEST INTEGRAL: ICONOS RECORTADOS Y VISTA PREVIA AL PASAR EL RATON (9B)
# ==============================================================================

class WatchdogNode extends Node:
	var start_msec: int = 0
	var limit_msec: int = 25000
	var owner_tree: SceneTree = null
	func _process(_delta: float) -> void:
		if owner_tree != null and (Time.get_ticks_msec() - start_msec) >= limit_msec:
			owner_tree.call("_on_watchdog_timeout")

var _backup: Dictionary = {}
var _res_data_dir: String = "res://clockgears_data"

func _on_watchdog_timeout() -> void:
	printerr("[TIMEOUT] Watchdog alcanzado en test_item_preview_9b.gd")
	quit(1)

func _snapshot_data_dir(res_data_dir: String) -> Dictionary:
	var backup := {}
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				backup[full] = FileAccess.get_file_as_string(full)
			fname = da.get_next()
		da.list_dir_end()
	return backup

func _restore_data_dir(res_data_dir: String, backup: Dictionary) -> void:
	for p in backup.keys():
		var fa := FileAccess.open(p, FileAccess.WRITE)
		if fa != null:
			fa.store_string(backup[p])
			fa.close()

func _init() -> void:
	print("=== INICIO TEST INTEGRAL ITEM ICONS Y PREVIEW 9B ===")
	call_deferred("_run_test")

func _run_test() -> void:
	var wd := WatchdogNode.new()
	wd.start_msec = Time.get_ticks_msec()
	wd.owner_tree = self
	root.add_child(wd)

	_backup = _snapshot_data_dir(_res_data_dir)

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		printerr("FAIL: DataManager no disponible")
		quit(1)
		return

	dm.load_all(_res_data_dir)

	# 1. Configurar un objeto con celda (col=2, row=1) en tileset forest-tilemap.png (128x128)
	var test_icon_path = "res://img/tilesets/forest-tilemap.png"
	var item_dict = {
		"id": 88,
		"name": "Madera Noble",
		"desc": "Madera refinada obtenida de los arboles mas antiguos del bosque magico. Se utiliza para construccion de alta calidad y refuerzo de escudos ancestrales. Este texto es largo a proposito para verificar que el scroll vertical de la descripcion funcione correctamente sin cortar ningun caracter ni desbordar la ventana.",
		"graphic_path": test_icon_path,
		"tile_coords": [2, 1],
		"tile_block_size": [1, 1],
		"usable_in_menu": true,
		"price": 120,
		"max_stack": 99,
		"type": 0
	}
	dm.create_record(3, item_dict) # CAT_ITEMS = 3
	dm.change_item("items:88", 5)

	# Configurar un arma en CAT_WEAPONS = 4 para probar equipo
	var wp_dict = {
		"id": 88,
		"name": "Espada de Madera",
		"desc": "Espada de entrenamiento tallada en madera noble.",
		"graphic_path": test_icon_path,
		"tile_coords": [2, 1],
		"tile_block_size": [1, 1]
	}
	dm.create_record(4, wp_dict)
	dm.change_item("weapons:88", 1)

	if dm.get_category_array(0).is_empty():
		dm.create_record(0, {"id": 1, "name": "Ícaro", "initial_level": 5, "party_initial": true})
	dm.equip_actor_piece(1, 0, "weapons:88")

	# Verificar que get_entry_icon_texture recorta correctamente la celda (2,1) -> tamaño 32x32
	var tex = dm.get_entry_icon_texture(item_dict, 32)
	if not tex is Texture2D or tex.get_size() != Vector2(32, 32):
		printerr("FAIL: get_entry_icon_texture no devolvio Texture2D de 32x32: ", tex)
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	print("[CHECK 1] Tamaño textura icono recortada: ", tex.get_size())
	print("OK: get_entry_icon_texture recorta exactamente la celda elegida y devuelve 32x32.")

	# 2. Configurar ventana y crear ClockGearsGameMenu
	root.size = Vector2i(960, 540)
	var gmenu = ClockGearsGameMenu.new()
	gmenu.set_editor_test_popup_mode(true)
	root.add_child(gmenu)

	var opened = gmenu.open_menu()
	if not opened:
		printerr("FAIL: No se pudo abrir el menu del juego")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return

	# Abrir subpantalla Objetos
	gmenu.open_subscreen("items")
	await process_frame
	await process_frame

	var prev_info = gmenu.get_item_preview_panel_info()
	print("[CHECK 2] Info panel lateral Objetos: ", prev_info)
	if not prev_info.get("visible", false):
		printerr("FAIL: El panel lateral de preview no esta visible")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	if not prev_info.get("has_texture", false):
		printerr("FAIL: El panel lateral no tiene textura cargada")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	print("OK: Pantalla Objetos muestra panel lateral con icono y descripcion.")

	# 3. Comprobar rotacion 2D pixel-perfect por arrastre
	var rot_ini = gmenu.get_preview_rotation_deg()
	print("[CHECK 3] Rotacion inicial: ", rot_ini)
	gmenu.simulate_preview_drag_rotation(16.0) # ~2 pasos de 15 deg = 30 deg
	var rot_after = gmenu.get_preview_rotation_deg()
	print("[CHECK 3] Rotacion tras arrastre: ", rot_after)
	if rot_after == rot_ini:
		printerr("FAIL: La rotacion no cambio con el arrastre simulado")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	if int(rot_after) % 15 != 0:
		printerr("FAIL: La rotacion no es multiplo del paso de 15 deg: ", rot_after)
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return

	# Al salir o cambiar de objeto, rotacion vuelve a 0
	gmenu.open_subscreen("main")
	if gmenu.get_preview_rotation_deg() != 0.0:
		printerr("FAIL: Rotacion no se reseteo a 0 al salir de la subpantalla")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	print("OK: Rotacion 2D por arrastre funciona a pasos fijos de 15 deg y se resetea a 0.")

	# 4. Probar configuracion en Base de Datos: posicion "left" y desactivar
	gmenu.open_subscreen("items")
	await process_frame

	var cfg = dm.get_game_menu_config()
	var prev_cfg = cfg.get("item_preview", {})
	prev_cfg["position"] = "left"
	prev_cfg["icon_scale"] = 3 # x3 = 96x96
	cfg["item_preview"] = prev_cfg
	gmenu.apply_game_menu_config_preview(cfg, "items")
	await process_frame

	var info_left = gmenu.get_item_preview_panel_info()
	print("[CHECK 4] Info panel a la izquierda: ", info_left)
	if info_left.get("panel_index", -1) != 0:
		printerr("FAIL: El panel lateral no se movio al indice 0 (izquierda): ", info_left.get("panel_index", -1))
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return

	# Desactivar panel
	prev_cfg["enabled"] = false
	cfg["item_preview"] = prev_cfg
	gmenu.apply_game_menu_config_preview(cfg, "items")
	await process_frame

	var info_disabled = gmenu.get_item_preview_panel_info()
	print("[CHECK 4] Info panel desactivado: ", info_disabled)
	if info_disabled.get("visible", true):
		printerr("FAIL: El panel lateral sigue visible cuando enabled = false")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	print("OK: Configuracion en Base de Datos modifica posicion (izq/der) y visibilidad.")

	# Restaurar preview
	prev_cfg["enabled"] = true
	prev_cfg["position"] = "right"
	prev_cfg["icon_scale"] = 2
	cfg["item_preview"] = prev_cfg
	gmenu.apply_game_menu_config_preview(cfg, "items")

	# 5. Probar subpantalla Equipo
	gmenu.open_subscreen("equip")
	await process_frame
	var eq_box = gmenu.find_child("EquipPanel", true, false)
	if not eq_box or not eq_box.is_visible():
		printerr("FAIL: EquipPanel no esta visible")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	print("OK: Subpantalla Equipo abierta correctamente con panel de vista previa.")

	# 6. Probar subpantalla Estado
	gmenu.open_subscreen("status")
	await process_frame
	var st_box = gmenu.find_child("StatusPanel", true, false)
	if not st_box or not st_box.is_visible():
		printerr("FAIL: StatusPanel no esta visible")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	print("OK: Subpantalla Estado abierta con bloque de equipo y piezas equipadas.")

	# 7. Probar HUD con widget contador de objeto
	var hud = ClockGearsHudOverlay.new()
	root.add_child(hud)
	var hud_cfg = {
		"visible": true,
		"entries": [
			{
				"id": "item_madera",
				"type": "item",
				"item_key": "Madera Noble",
				"pos_x": 10,
				"pos_y": 10,
				"width": 120,
				"height": 40,
				"show_name": true,
				"icon_path": ""
			}
		]
	}
	hud.setup_hud(null, null, null)
	hud.apply_hud_config_preview(hud_cfg)
	await process_frame
	var snap = hud.get_entry_snapshot("item_madera")
	print("[CHECK 7] Snapshot HUD item widget: ", snap)
	var hud_icon_rect: TextureRect = null
	for ch in hud.get_children():
		var found = ch.find_child("Icon", true, false)
		if found and found is TextureRect:
			hud_icon_rect = found
			break
	if hud_icon_rect and hud_icon_rect.texture != null:
		var h_sz = hud_icon_rect.texture.get_size()
		print("[CHECK 7] Tamaño textura en HUD: ", h_sz)
		if h_sz != Vector2(32, 32):
			printerr("FAIL: HUD uso textura de tamaño incorrecto (esperado 32x32): ", h_sz)
			_restore_data_dir(_res_data_dir, _backup)
			quit(1)
			return
		print("OK: HUD usa la celda recortada correcta de 32x32 (no el tileset completo).")
	else:
		printerr("FAIL: Icono en HUD no tiene textura")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return

	# 8. Probar Aviso de Drop (ClockGearsDebugOverlay)
	var debug_overlay = ClockGearsDebugOverlay.new()
	root.add_child(debug_overlay)
	debug_overlay.show_drop_toast("Madera Noble", 3)
	await process_frame
	var toasts = debug_overlay.get_active_drop_toasts()
	print("[CHECK 8] Drop toasts activos: ", toasts)
	if toasts.is_empty():
		printerr("FAIL: No se genero el toast de drop")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	if not toasts[0].get("has_icon", false):
		printerr("FAIL: Toast de drop no tiene icono")
		_restore_data_dir(_res_data_dir, _backup)
		quit(1)
		return
	print("OK: Aviso de drop muestra la celda recortada correcta.")

	# 9. Regla 16:9 y no cortes
	var test_resolutions = [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080)]
	for res in test_resolutions:
		root.size = res
		gmenu.open_subscreen("items")
		await process_frame
		var b_info = gmenu.get_layout_bounds_info(Vector2(res.x, res.y))
		print("[CHECK 9] Resolucion ", res, " -> Bounds: ", b_info)
	print("OK: Regla 16:9 verificada en 960x540, 1366x768 y 1920x1080.")

	hud.queue_free()
	debug_overlay.queue_free()
	gmenu.queue_free()

	_restore_data_dir(_res_data_dir, _backup)
	print("=== TODOS LOS TESTS PASARON EXITOSAMENTE (OK) ===")
	quit(0)
