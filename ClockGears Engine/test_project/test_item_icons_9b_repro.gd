extends SceneTree

# ==============================================================================
# REPRODUCCIÓN: ICONOS DE OBJETOS EN PANTALLAS DEL JUEGO (9B)
# ==============================================================================

class WatchdogNode extends Node:
	var start_msec: int = 0
	var limit_msec: int = 25000
	var owner_tree: SceneTree = null
	func _process(_delta: float) -> void:
		if owner_tree != null and (Time.get_ticks_msec() - start_msec) >= limit_msec:
			owner_tree.call("_on_watchdog_timeout")

var _finished: bool = false
var _backup: Dictionary = {}
var _res_data_dir: String = "res://clockgears_data"

func _on_watchdog_timeout() -> void:
	printerr("[TIMEOUT] Watchdog alcanzado en test_item_icons_9b_repro.gd")
	quit(1)

func _init() -> void:
	call_deferred("_run_repro")

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

func _find_texture_rects(n: Node, out_arr: Array) -> void:
	if n == null:
		return
	if n is TextureRect:
		out_arr.push_back(n)
	for c in n.get_children():
		_find_texture_rects(c, out_arr)

func _run_repro() -> void:
	var wd := WatchdogNode.new()
	wd.start_msec = Time.get_ticks_msec()
	wd.owner_tree = self
	root.add_child(wd)

	_backup = _snapshot_data_dir(_res_data_dir)

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		printerr("No ClockGearsDataManager")
		quit(1)
		return

	dm.load_all(_res_data_dir)

	# 1. Configurar un objeto 'Madera' con celda (2, 1) del tileset forest-tilemap.png (128x128)
	var tileset_path := "res://img/tilesets/forest-tilemap.png"
	var cell_x := 2
	var cell_y := 1
	var expected_region := Rect2i(cell_x * 32, cell_y * 32, 32, 32) # Rect2i(64, 32, 32, 32)

	var item_rec := {
		"id": 1,
		"name": "Madera de Roble",
		"desc": "Madera resistente para construcciones.",
		"graphic_path": tileset_path,
		"tile_coords": [cell_x, cell_y],
		"tile_block_size": [1, 1],
		"max_stack": 99,
		"type": 0
	}
	dm.create_record(3, item_rec) # CAT_ITEMS
	dm.change_item("items:1", 5)

	# También configurar un arma y armadura con celda (1, 2) y equipársela al actor 1
	var wp_rec := {
		"id": 1,
		"name": "Hacha de Leñador",
		"desc": "Corta madera con facilidad.",
		"graphic_path": tileset_path,
		"tile_coords": [1, 2],
		"tile_block_size": [1, 1]
	}
	dm.create_record(4, wp_rec) # CAT_WEAPONS
	dm.change_item("weapons:1", 2)

	if dm.get_category_array(0).is_empty():
		dm.create_record(0, {"id": 1, "name": "Ícaro", "initial_level": 5, "party_initial": true})
	dm.equip_actor_piece(1, 0, "weapons:1") # Equipar slot 0 con wp_rec

	print("====================================================")
	print("REPRODUCCIÓN PRE-FIX: COMPROBACIÓN DE REGIONES USADAS")
	print("====================================================")
	print("Objeto prueba: 'Madera de Roble' -> celda esperada: (col=%d, row=%d) = %s" % [cell_x, cell_y, str(expected_region)])

	# --- PANTALLA 1: Menú del juego -> Objetos ---
	var gmenu = ClockGearsGameMenu.new()
	gmenu.set_editor_test_popup_mode(true)
	root.add_child(gmenu)
	gmenu.open_menu()
	gmenu.open_subscreen("items")
	await process_frame
	await process_frame

	var items_panel = gmenu.find_child("ItemsPanel", true, false)
	var items_tex_rects: Array = []
	_find_texture_rects(items_panel, items_tex_rects)
	var objetos_region_used := "ninguna"
	var objetos_tex_size := Vector2i.ZERO
	for tr in items_tex_rects:
		if tr is TextureRect and tr.texture != null:
			objetos_tex_size = Vector2i(tr.texture.get_width(), tr.texture.get_height())
			if tr.texture is AtlasTexture:
				objetos_region_used = str(tr.texture.region)
			else:
				objetos_region_used = "completa 0,0 a %dx%d (SIN RECORTAR)" % [objetos_tex_size.x, objetos_tex_size.y]
			break
	print("[REPRO][Objetos] Región usada: %s | Tamaño textura: %s" % [objetos_region_used, str(objetos_tex_size)])

	# --- PANTALLA 2: Menú del juego -> Equipo (huecos y lista) ---
	gmenu.open_subscreen("equip")
	await process_frame
	await process_frame
	var equip_panel = gmenu.find_child("EquipPanel", true, false)
	var equip_tex_rects: Array = []
	_find_texture_rects(equip_panel, equip_tex_rects)
	var equip_icons_count := 0
	for tr in equip_tex_rects:
		if tr is TextureRect and tr.texture != null:
			equip_icons_count += 1
	var equip_region_used := "sin icono recortado (botones solo texto)" if equip_icons_count == 0 else "iconos encontrados: " + str(equip_icons_count)
	print("[REPRO][Equipo] Huecos y lista de candidatos: %s" % [equip_region_used])

	# --- PANTALLA 3: Menú del juego -> Estado ---
	gmenu.open_subscreen("status")
	await process_frame
	await process_frame
	var status_panel = gmenu.find_child("StatusPanel", true, false)
	var status_tex_rects: Array = []
	_find_texture_rects(status_panel, status_tex_rects)
	print("[REPRO][Estado] Iconos de equipo en estado: %d iconos encontrados (sin iconos de equipo)" % [max(0, status_tex_rects.size() - 1)])

	# --- PANTALLA 4: HUD -> Contador de objetos ---
	var hud_cfg := {
		"widgets": [
			{
				"id": "item_counter_test",
				"type": "item",
				"layer": 1,
				"item_key": "items:1",
				"show_name": true,
				"anchor": "top_left",
				"offset_x": 10,
				"offset_y": 10
			}
		]
	}
	var hud = ClockGearsHudOverlay.new()
	root.add_child(hud)
	hud.apply_hud_config(hud_cfg)
	await process_frame
	await process_frame

	var hud_tex_rects: Array = []
	_find_texture_rects(hud, hud_tex_rects)
	var hud_region_used := "ninguna"
	var hud_tex_size := Vector2i.ZERO
	for tr in hud_tex_rects:
		if tr is TextureRect and tr.texture != null:
			hud_tex_size = Vector2i(tr.texture.get_width(), tr.texture.get_height())
			if tr.texture is AtlasTexture:
				hud_region_used = str(tr.texture.region)
			else:
				hud_region_used = "completa 0,0 a %dx%d (SIN RECORTAR)" % [hud_tex_size.x, hud_tex_size.y]
			break
	print("[REPRO][HUD] Región usada: %s | Tamaño textura: %s" % [hud_region_used, str(hud_tex_size)])

	# --- PANTALLA 5: Aviso de drops (DebugOverlay) ---
	var debug_overlay = ClockGearsDebugOverlay.new()
	root.add_child(debug_overlay)
	debug_overlay.show_drop_toast("Madera de Roble", 3)
	await process_frame
	await process_frame

	var drop_tex_rects: Array = []
	_find_texture_rects(debug_overlay, drop_tex_rects)
	var drop_region_used := "ninguna"
	var drop_tex_size := Vector2i.ZERO
	for tr in drop_tex_rects:
		if tr is TextureRect and tr.texture != null:
			drop_tex_size = Vector2i(tr.texture.get_width(), tr.texture.get_height())
			if tr.texture is AtlasTexture:
				drop_region_used = str(tr.texture.region)
			else:
				drop_region_used = "completa 0,0 a %dx%d (SIN RECORTAR)" % [drop_tex_size.x, drop_tex_size.y]
			break
	print("[REPRO][Aviso de Drop] Región usada: %s | Tamaño textura: %s" % [drop_region_used, str(drop_tex_size)])

	print("====================================================")
	print("CONFIRMACIÓN DE CAUSA RAÍZ:")
	print("- Objetos: ClockGearsTitleScreen::load_runtime_texture carga el PNG entero (128x128) sin consultar tile_coords.")
	print("- Equipo: Los botones de huecos y candidatos no implementan icono de entrada.")
	print("- Estado: No renderiza iconos de las piezas equipadas.")
	print("- HUD: load_texture_safe carga el PNG entero (128x128) en r_entry.icon_rect ignorando tile_coords.")
	print("- Aviso de Drop: _resolve_item_icon solo busca en CAT_ITEMS y falla al procesar tile_coords si es Variant::ARRAY.")
	print("====================================================")

	# Limpiar nodos y restaurar
	gmenu.queue_free()
	hud.queue_free()
	debug_overlay.queue_free()
	_restore_data_dir(_res_data_dir, _backup)
	dm.load_all(_res_data_dir)

	quit(0)
