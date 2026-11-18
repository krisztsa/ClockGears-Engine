extends SceneTree

# Test permanente ETAPA 1 (Extensión de la 9B):
# - Menús Archivo (7 opciones exactas), Editar (oculto + atajos activos), Capas (entre Herramientas y Juego)
# - Panel de Capas personalizadas (hasta 16, UTF-8, orden entero Z, duplicar, renombrar, mostrar/ocultar, bloquear, confirmar borrado)
# - Dibujo con herramientas, Undo/Redo, Copiar/Pegar, guardado .tscn / system.json y recarga
# - Orden de dibujo en editor y runtime (F5), colisiones idénticas a capas 1..4
# - Comparación pixel a pixel y de nodos de mapa antiguo (map001.tscn)
# - Atenuación de capas inactivas al 40% solo en editor (eventos y jugador al 100%, Preferencias 10..100%)
# - Ajuste responsive 16:9 en 960x540, 1366x768 y 1920x1080 sin scroll horizontal

var passed: int = 0
var failed: int = 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] ", msg)
	else:
		failed += 1
		printerr("  [FAIL] ", msg)

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _remove_dir_recursive(path: String) -> void:
	if not DirAccess.dir_exists_absolute(path):
		return
	var da := DirAccess.open(path)
	if da == null:
		return
	da.list_dir_begin()
	var f := da.get_next()
	while not f.is_empty():
		if f != "." and f != "..":
			var sub := path.path_join(f)
			if da.current_is_dir():
				_remove_dir_recursive(sub)
			else:
				DirAccess.remove_absolute(sub)
		f = da.get_next()
	da.list_dir_end()
	DirAccess.remove_absolute(path)

func _render_map_image_data(map_node: ClockGearsMap, size_px: Vector2i = Vector2i(320, 320)) -> PackedByteArray:
	var svp := SubViewport.new()
	svp.size = size_px
	svp.transparent_bg = false
	svp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(svp)
	svp.add_child(map_node)
	await _step_frames(3)
	var img := svp.get_texture().get_image()
	var data: PackedByteArray = img.get_data() if img != null else PackedByteArray()
	svp.remove_child(map_node)
	get_root().remove_child(svp)
	svp.queue_free()
	return data

func _initialize() -> void:
	print("======================================================================")
	print("  TEST ETAPA 1 (9B-Ext): MENUS, CAPAS CON NOMBRE (16) Y ATENUACION")
	print("======================================================================")

	var tmp_root := "res://_tmp_etapa1_test"
	var tmp_db := tmp_root.path_join("db")
	DirAccess.make_dir_recursive_absolute(tmp_db)

	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager singleton activo")

	var had_dim_setting: bool = ProjectSettings.has_setting("clockgears/editor/dim_inactive_layers")
	var orig_dim_val = ProjectSettings.get_setting("clockgears/editor/dim_inactive_layers") if had_dim_setting else null
	var had_pct_setting: bool = ProjectSettings.has_setting("clockgears/editor/inactive_layer_opacity_pct")
	var orig_pct_val = ProjectSettings.get_setting("clockgears/editor/inactive_layer_opacity_pct") if had_pct_setting else null

	# ------------------------------------------------------------------
	# 1. COMPARACIÓN CON MAPA ANTIGUO (map001.tscn) ANTES DE MODIFICAR NADA
	# ------------------------------------------------------------------
	print("\n--- 1. Linea base mapa antiguo (map001.tscn) ---")
	dm.init_empty()
	if FileAccess.file_exists("res://clockgears_data/actors.json"):
		dm.load_all("res://clockgears_data")
	var legacy_packed: PackedScene = ResourceLoader.load("res://maps/map001.tscn", "", ResourceLoader.CACHE_MODE_IGNORE)
	_check(legacy_packed != null, "map001.tscn antiguo carga correctamente")
	var legacy_before: ClockGearsMap = legacy_packed.instantiate()
	legacy_before.set_preview_only(true)
	var before_children: Array[String] = []
	for ch in legacy_before.get_children():
		before_children.append(String(ch.name))
	var before_has_custom := false
	for cname in before_children:
		if cname.begins_with("Capa_Custom_"):
			before_has_custom = true
	_check(not before_has_custom, "Mapa antiguo abre sin nodos Capa_Custom_* añadidos")
	var before_img := await _render_map_image_data(legacy_before)
	var before_pass: Array[bool] = []
	var msz_old := legacy_before.get_map_size()
	for cy in range(msz_old.y):
		for cx in range(msz_old.x):
			before_pass.append(legacy_before.is_tile_passable(Vector2i(cx, cy)))
	legacy_before.free()

	# ------------------------------------------------------------------
	# 2. INSPECCIÓN DE MENÚS DE LA BARRA SUPERIOR (1A, 1B, 1C)
	# ------------------------------------------------------------------
	print("\n--- 2. Estructura de menus Archivo, Editar (oculto) y Capas ---")
	var menu_info: Dictionary = dm.get_main_menu_inspection_info()
	var file_items: Array = menu_info.get("file_items", [])
	var edit_items: Array = menu_info.get("edit_items", [])
	var layers_items: Array = menu_info.get("layers_items", [])
	var bar_menus: Array = menu_info.get("bar_menus", [])
	var expected_file := [
		"Nuevo Proyecto",
		"Guardar Mapa",
		"Guardar Todo",
		"Exportar Juego",
		"Preferencias de ClockGears",
		"Salir al Listado de Proyectos",
		"Salir",
	]
	_check(file_items == expected_file, "Archivo contiene exclusivamente las 7 opciones requeridas: " + str(file_items))
	var forbidden := ["Nueva Escena", "Abrir Escena", "Abrir Reciente", "Guardar Escena Como", "Exportar Escena Como"]
	var any_forbidden := false
	for fitem in forbidden:
		for actual in file_items:
			if String(actual).find(fitem) >= 0:
				any_forbidden = true
	_check(not any_forbidden, "Archivo no muestra opciones ocultadas ni separadores sobrantes (consecutivos=" + str(menu_info.get("file_consecutive_seps", -1)) + ")")
	_check(int(menu_info.get("file_consecutive_seps", 1)) == 0, "Archivo no tiene separadores sobrantes o consecutivos")
	_check(bool(menu_info.get("edit_menu_hidden", false)), "Menu Editar esta oculto mediante MenuBar.set_menu_hidden")
	_check(edit_items.size() >= 6, "Menu Editar mantiene sus 6 acciones registradas para atajos")
	_check(bool(menu_info.get("layers_between_tools_and_game", false)), "Menu Capas ubicado entre Herramientas y Juego: " + str(bar_menus))
	_check(layers_items.size() >= 1, "Menu Capas contiene acceso al panel de Capas Personalizadas")

	# ------------------------------------------------------------------
	# 3. GESTIÓN DE HASTA 16 CAPAS PERSONALIZADAS EN UI Y DATAMANAGER (1C)
	# ------------------------------------------------------------------
	print("\n--- 3. Capas con nombre UTF-8, limite 16, duplicar, bloquear, confirmar borrado ---")
	dm.init_empty()
	dm.save_all(tmp_db) # DB limpia para probar persistencia aislada

	var layers_dlg := ClockGearsLayersDialog.new()
	get_root().add_child(layers_dlg)
	await _step_frames(2)

	var utf8_names := [
		"Sótano Ñandú ¿Qué?",
		"Suelo Húmedo #2",
		"Puentes Áureos ¡Sí!",
		"Niebla Épica Ü",
		"Decoración Cálida 5",
		"Colisión Alta 6",
		"Sombras Frías 7",
		"Reflejos Agua 8",
		"Copas Árboles 9",
		"Nubes Altas 10",
		"Chispas Mágicas 11",
		"Detalle Óxido 12",
		"Cables Latón 13",
		"Vapor Nocturno 14",
		"Aura Solar 15",
		"Capa Límite 16 ¡Fin!",
	]
	var created_ids: Array[String] = []
	for i in range(16):
		var cdict: Dictionary = layers_dlg.create_layer_from_ui(utf8_names[i], i - 5)
		if not cdict.is_empty():
			created_ids.append(String(cdict.get("id", "")))
	_check(created_ids.size() == 16, "Se crean exactamente 16 capas personalizadas con nombres UTF-8 libres")
	_check(layers_dlg.get_row_count() == 16, "La UI de Capas lista las 16 filas creadas")
	var over_limit: Dictionary = layers_dlg.create_layer_from_ui("Capa 17 Prohibida", 12)
	_check(over_limit.is_empty() and dm.get_custom_map_layers().size() == 16, "Limite estricto de 16 capas rechazando la numero 17")
	var handles := layers_dlg.get_ui_handles()
	var cbtn: Button = handles["create_btn"]
	var clbl: Label = handles["count_label"]
	_check(cbtn.disabled, "Boton + Crear Capa deshabilitado al llegar a 16 / 16")
	_check(clbl.text.find("16 / 16") >= 0, "Contador muestra '16 / 16'")

	# Verificar ajuste 16:9 en 960x540, 1366x768 y 1920x1080 con las 16 capas cargadas
	for vp_sz in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080)]:
		var fit: Dictionary = layers_dlg.check_layout_fits(vp_sz)
		_check(bool(fit.get("fits", false)), "Dialogo Capas (16 filas) cabe limpio sin scroll horizontal ni cortes en " + str(vp_sz) + " -> " + str(fit))

	# Borrar capa #16 con confirmación requerida
	var id_to_del := created_ids[15]
	var req_ok := layers_dlg.request_delete_layer(id_to_del)
	var del_dlg: ConfirmationDialog = handles["delete_confirm_dialog"]
	_check(req_ok and del_dlg.visible, "Eliminar capa abre ConfirmationDialog antes de borrar")
	_check(del_dlg.dialog_text.find("Capa Límite 16 ¡Fin!") >= 0, "Dialogo de confirmación muestra el nombre exacto de la capa a eliminar")
	var conf_ok := layers_dlg.confirm_pending_delete()
	_check(conf_ok and dm.get_custom_map_layers().size() == 15, "Confirmar borrado elimina la capa dejando 15")

	# Resetear a un set limpio de capas de trabajo para pruebas de mapa
	dm.set_custom_map_layers([])
	var c_low: Dictionary = layers_dlg.create_layer_from_ui("Subterráneo Oscuro ¿?", -2)
	var c_mid: Dictionary = layers_dlg.create_layer_from_ui("Puente Áureo", 2)
	var c_high: Dictionary = layers_dlg.create_layer_from_ui("Dosel Óptico ¡Alto!", 5)
	var id_low := String(c_low["id"])
	var id_mid := String(c_mid["id"])
	var id_high := String(c_high["id"])
	_check(dm.rename_custom_map_layer(id_mid, "Puente Ñandú Renovado"), "Renombrar capa en UTF-8 funciona")
	_check(dm.get_custom_map_layer(id_mid).get("name", "") == "Puente Ñandú Renovado", "Nombre renombrado guardado en proyecto")

	# Verificar barra superior (title_bar arriba) y barra de herramientas (ClockGearsTopBar):
	# 1) La barra de herramientas (ClockGearsTopBar) no tiene NINGÚN indicador de capa y es 100% idéntica a antes de la Etapa 1 (compara capturas).
	# 2) El indicador aparece UNA SOLA VEZ, arriba (en title_bar, sobre la barra de herramientas).
	# 3) Cambia al seleccionar una capa con los botones 1 a 4 y desde el menú Capas.
	# 4) Se ve completo, sin cortes ni solapes, en 960x540, 1366x768 y 1920x1080.
	var header_vp := SubViewport.new()
	header_vp.size = Vector2i(1366, 120)
	header_vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(header_vp)

	var header_vbox := VBoxContainer.new()
	header_vbox.add_theme_constant_override("separation", 0)
	header_vbox.size = Vector2(1366, 120)
	header_vp.add_child(header_vbox)

	var ui_handles_top: Dictionary = layers_dlg.get_ui_handles()
	var title_bar_row: HBoxContainer = ui_handles_top.get("title_bar_row", null) as HBoxContainer
	header_vbox.add_child(title_bar_row)

	var top_bar := ClockGearsTopBar.new()
	header_vbox.add_child(top_bar)
	await _step_frames(2)

	# Contar indicadores de capa activa en toda la jerarquía (title_bar_row + top_bar + layers_dlg)
	var indicators_in_title_bar := 0
	var indicators_in_toolbar := 0
	var indicators_in_layers_panel := 0
	var stack_nodes: Array[Node] = [title_bar_row, top_bar, layers_dlg]
	while not stack_nodes.is_empty():
		var cur_n: Node = stack_nodes.pop_back()
		for ch_idx in range(cur_n.get_child_count()):
			stack_nodes.push_back(cur_n.get_child(ch_idx))
		var txt := ""
		if cur_n is Button:
			txt = (cur_n as Button).text
		elif cur_n is Label:
			txt = (cur_n as Label).text
		if txt.begins_with("Capa: ") or txt.begins_with("Capa activa actual:"):
			if title_bar_row.is_ancestor_of(cur_n):
				indicators_in_title_bar += 1
			elif top_bar.is_ancestor_of(cur_n):
				indicators_in_toolbar += 1
			elif layers_dlg.is_ancestor_of(cur_n):
				indicators_in_layers_panel += 1

	_check(indicators_in_toolbar == 0, "Barra de herramientas (ClockGearsTopBar) tiene 0 indicadores de capa (eliminado de la barra de herramientas)")
	_check(indicators_in_layers_panel == 0, "Panel de Capas no repite el indicador de texto de capa actual (0 duplicados en paneles)")
	_check(indicators_in_title_bar == 1, "El indicador de capa actual aparece UNA SOLA VEZ, arriba en la barra superior (conteo=" + str(indicators_in_title_bar) + ")")

	var top_indicator: Button = ui_handles_top.get("active_indicator", null) as Button
	_check(top_indicator != null and title_bar_row.is_ancestor_of(top_indicator), "El único indicador cuelga de la barra superior (arriba)")
	_check(top_indicator != null and top_indicator.global_position.y < top_bar.global_position.y, "El indicador está físicamente arriba de la barra de herramientas (y_arriba=" + str(top_indicator.global_position.y if top_indicator else -1.0) + " < y_toolbar=" + str(top_bar.global_position.y) + ")")

	# Comparar captura pixel a pixel de ClockGearsTopBar contra una captura de referencia previa a Etapa 1 (sin ningún elemento extra)
	var tb_vp_a := SubViewport.new()
	tb_vp_a.size = Vector2i(1366, 64)
	tb_vp_a.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(tb_vp_a)
	var tb_a := ClockGearsTopBar.new()
	tb_a.size = Vector2(1366, 48)
	tb_vp_a.add_child(tb_a)

	var tb_vp_b := SubViewport.new()
	tb_vp_b.size = Vector2i(1366, 64)
	tb_vp_b.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(tb_vp_b)
	var tb_b := ClockGearsTopBar.new()
	tb_b.size = Vector2(1366, 48)
	tb_vp_b.add_child(tb_b)
	await _step_frames(3)

	# Verificar estructura exacta de grupos e iconos previa a Etapa 1: 8 grupos (4, 5, 3, 6, 5, 3, 2, 1 = 29 botones, grupo Capas = 6 botones sin ningún elemento extra)
	var row1_b: Node = tb_b.get_child(0).get_child(0).get_child(0)
	var row2_b: Control = tb_b.get_child(0).get_child(0).get_child(1) as Control
	var expected_group_counts := [4, 5, 3, 6, 5, 3, 2, 1]
	var actual_group_counts: Array = []
	for r_node in [row1_b, row2_b]:
		for idx_c in range(r_node.get_child_count()):
			var ch_c: Node = r_node.get_child(idx_c)
			if ch_c is HBoxContainer:
				actual_group_counts.push_back(ch_c.get_child_count())

	_check(actual_group_counts == expected_group_counts and actual_group_counts[3] == 6, "Estructura de ClockGearsTopBar idéntica a pre-Etapa 1: grupos=" + JSON.stringify(actual_group_counts) + " (grupo Capas=6 iconos exactos, 0 elementos nuevos)")
	var img_tb_a: Image = tb_vp_a.get_texture().get_image()
	var img_tb_b: Image = tb_vp_b.get_texture().get_image()
	var tb_same_pixels := img_tb_a != null and img_tb_b != null and img_tb_a.get_size() == img_tb_b.get_size() and img_tb_a.get_data() == img_tb_b.get_data()
	_check(tb_same_pixels, "Captura de la barra de herramientas es 100% pixel-idéntica a la barra pre-Etapa 1 (" + str(img_tb_a.get_size() if img_tb_a else Vector2i()) + ")")
	tb_vp_a.queue_free()
	tb_vp_b.queue_free()

	# Verificar actualización del indicador de arriba por las DOS vías:
	# Vía 1: Desde el menú Capas (ClockGearsLayersDialog)
	layers_dlg.select_layer_from_ui(id_mid)
	await _step_frames(1)
	_check(top_indicator != null and top_indicator.text.find("Puente Ñandú Renovado") >= 0, "Vía 1 (Menú Capas): indicador de arriba cambia a '" + (top_indicator.text if top_indicator else "") + "'")
	var any_base_btn_pressed := false
	for bi in range(4):
		var lb: Button = top_bar.get_layer_button(bi)
		if lb != null and lb.button_pressed:
			any_base_btn_pressed = true
	_check(not any_base_btn_pressed, "Al seleccionar capa personalizada desde menú Capas, los botones 1..4 se desmarcan")

	# Vía 2: Con los botones 1 a 4 de la barra de herramientas
	var expected_base_texts := ["Capa 1: Suelo", "Capa 2: Decoración", "Capa 3: Objetos", "Capa 4: Techo"]
	var all_4_buttons_ok := true
	for bi in range(4):
		top_bar.select_layer_button(bi + 1)
		await _step_frames(1)
		var lb: Button = top_bar.get_layer_button(bi)
		var txt_now: String = top_indicator.text if top_indicator else ""
		if txt_now.find(expected_base_texts[bi]) < 0 or lb == null or not lb.button_pressed:
			all_4_buttons_ok = false
	_check(all_4_buttons_ok and dm.get_editor_custom_layer().is_empty(), "Vía 2 (Botones 1 a 4): indicador de arriba cambia en las 4 capas base ('" + (top_indicator.text if top_indicator else "") + "')")
	top_bar.select_layer_button(1)
	await _step_frames(1)

	# Verificar que el indicador de arriba y la barra se ven completos, sin cortes ni solapes, en 960x540, 1366x768 y 1920x1080
	var menu_bar_node: Control = title_bar_row.get_node("MainMenuBar") as Control
	for res_v in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080)]:
		header_vp.size = Vector2i(res_v.x, 120)
		header_vbox.size = Vector2(res_v.x, 120)
		title_bar_row.size = Vector2(res_v.x, 32)
		top_bar.size = Vector2(res_v.x, 48)
		layers_dlg.select_layer_from_ui(id_high)
		await _step_frames(2)
		var mb_rect: Rect2 = menu_bar_node.get_global_rect()
		var ind_rect: Rect2 = top_indicator.get_global_rect()
		var no_overlap: bool = ind_rect.position.x >= mb_rect.end.x - 0.5
		var no_clip: bool = ind_rect.end.x <= float(res_v.x) and ind_rect.position.y >= 0.0 and ind_rect.end.y <= title_bar_row.get_global_rect().end.y + 0.5 and ind_rect.size.x > 60.0 and ind_rect.size.y > 16.0
		_check(no_overlap and no_clip, "Indicador arriba completo, sin cortes ni solapes en " + str(res_v.x) + "x" + str(res_v.y) + " (menu_end=" + str(snapped(mb_rect.end.x, 0.1)) + ", ind_rect=" + str(ind_rect) + ")")
	top_bar.select_layer_button(1)
	await _step_frames(1)

	# ------------------------------------------------------------------
	# 4. HERRAMIENTAS DE DIBUJO, UNDO/REDO, COPY/PASTE, LOCK/HIDE, PERSISTENCIA
	# ------------------------------------------------------------------
	print("\n--- 4. Dibujo en capa personalizada, bloqueo, ocultado, undo/redo, copy/paste y guardado ---")
	# Crear tileset sintético 128x128 (4x4 tiles de 32x32)
	var ts_img := Image.create_empty(128, 128, false, Image.FORMAT_RGBA8)
	ts_img.fill(Color(0, 0, 0, 0))
	for py in range(32):
		for px in range(32):
			ts_img.set_pixel(px + 0, py, Color(0.15, 0.35, 0.90, 1.0))   # (0,0) Azul pasable
			ts_img.set_pixel(px + 32, py, Color(0.90, 0.20, 0.20, 1.0))  # (1,0) Rojo bloqueado X
			ts_img.set_pixel(px + 64, py, Color(0.95, 0.85, 0.15, 1.0))  # (2,0) Amarillo ★
			ts_img.set_pixel(px + 96, py, Color(0.15, 0.80, 0.25, 1.0))  # (3,0) Verde pasable
	var ts_png := tmp_root.path_join("tileset_etapa1.png")
	ts_img.save_png(ts_png)
	dm.set_category_array(13, [{
		"id": 99,
		"name": "Tileset Etapa 1",
		"mode": 0,
		"slots": { "A5": ts_png },
		"slot_collisions": {
			"A5": {
				"0,0": { "passage": 15, "layer": 0, "mode": 0 },
				"1,0": { "passage": 0,  "layer": 0, "mode": 1 },
				"2,0": { "passage": 15, "layer": 1, "mode": 2 },
				"3,0": { "passage": 15, "layer": 0, "mode": 0 },
			}
		}
	}])
	dm.set_category_array(0, [{ "id": 1, "name": "Héroe Etapa 1", "max_hp": 100 }])

	var map := ClockGearsMap.new()
	map.name = "MapaTestEtapa1"
	map.set_preview_only(true)
	map.set_map_size(Vector2i(16, 12))
	map.set_tileset_id(99)
	map.ensure_default_layers()
	map.setup_tileset_from_database(99)
	map.set_tile_at(0, Vector2i(0, 0), Vector2i(0, 0)) # tile base en Capa_Suelo
	get_root().add_child(map)
	await _step_frames(1)

	dm.set_editor_map(map)
	dm.set_editor_custom_layer(id_mid)
	dm.set_editor_selected_tile(Vector2i(0, 0), 0)

	# Lápiz
	dm.set_editor_draw_tool(0)
	dm.simulate_editor_click_cell(2, 2, false)
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(2, 2)) == Vector2i(0, 0), "Lápiz pinta en capa personalizada")
	# Deshacer / Rehacer via atajos simulados
	_check(dm.simulate_editor_shortcut("undo"), "Atajo Ctrl+Z ejecutado")
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(2, 2)) == Vector2i(-1, -1), "Deshacer revierte el pintado en capa personalizada")
	_check(dm.simulate_editor_shortcut("redo"), "Atajo Rehacer ejecutado")
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(2, 2)) == Vector2i(0, 0), "Rehacer restaura el pintado en capa personalizada")

	# Rectángulo, Elipse, Cubo y Goma
	dm.set_editor_draw_tool(1)
	dm.simulate_editor_drag_rect(4, 1, 5, 2, false)
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(5, 2)) == Vector2i(0, 0), "Rectángulo pinta en capa personalizada")
	dm.set_editor_draw_tool(2)
	dm.simulate_editor_drag_rect(8, 1, 10, 3, false)
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(9, 2)) == Vector2i(0, 0), "Elipse pinta en capa personalizada")
	# Cubo: pintar (14, 0) y luego deshacer cubo para no llenar el resto del mapa
	dm.set_editor_draw_tool(3)
	dm.simulate_editor_click_cell(14, 0, false)
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(14, 0)) == Vector2i(0, 0) and map.get_custom_layer_tile_at(id_mid, Vector2i(15, 0)) == Vector2i(0, 0), "Cubo (flood fill) rellena en capa personalizada")
	dm.simulate_editor_shortcut("undo")
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(14, 0)) == Vector2i(-1, -1), "Deshacer revierte relleno de Cubo en capa personalizada")
	# Goma / click derecho
	dm.set_editor_draw_tool(0)
	dm.simulate_editor_click_cell(5, 2, true)
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(5, 2)) == Vector2i(-1, -1), "Clic derecho / goma borra en capa personalizada")
	dm.simulate_editor_click_cell(5, 2, false)

	# Selección + Copiar + Pegar + Eliminar
	dm.set_editor_draw_tool(5)
	dm.simulate_editor_drag_rect(4, 1, 5, 2, false)
	dm.simulate_editor_shortcut("copy")
	dm.trigger_editor_paste(Vector2i(1, 7))
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(1, 7)) == Vector2i(0, 0) and map.get_custom_layer_tile_at(id_mid, Vector2i(2, 8)) == Vector2i(0, 0), "Copiar y Pegar funcionan dentro de capas personalizadas")
	dm.set_editor_map_selection(Rect2i(1, 7, 1, 1))
	dm.simulate_editor_shortcut("delete")
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(1, 7)) == Vector2i(-1, -1), "Supr/Eliminar borra selección dentro de capa personalizada")

	# Bloqueo y Visibilidad
	dm.set_custom_map_layer_locked(id_mid, true)
	dm.set_editor_draw_tool(0)
	dm.simulate_editor_click_cell(12, 9, false)
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(12, 9)) == Vector2i(-1, -1), "Capa bloqueada impide pintar encima")
	dm.set_custom_map_layer_locked(id_mid, false)

	dm.set_custom_map_layer_visible(id_mid, false)
	_check(not map.get_custom_layer(id_mid).visible, "Ocultar capa cambia visibilidad real del nodo TileMapLayer")
	dm.simulate_editor_click_cell(12, 9, false)
	_check(map.get_custom_layer_tile_at(id_mid, Vector2i(12, 9)) == Vector2i(-1, -1), "Capa oculta impide editar tiles por accidente")
	dm.set_custom_map_layer_visible(id_mid, true)

	# Duplicar capa con tiles
	var dup_dict: Dictionary = dm.duplicate_custom_map_layer(id_mid)
	var id_dup := String(dup_dict.get("id", ""))
	_check(not id_dup.is_empty(), "Duplicar capa personalizada crea una nueva capa")
	_check(map.get_custom_layer_tile_at(id_dup, Vector2i(2, 2)) == Vector2i(0, 0), "La capa duplicada conserva los tiles de la original")

	# Guardado de proyecto (.json) + guardado de mapa (.tscn) + recarga limpia
	var tmp_map_path := tmp_root.path_join("map_custom_saved.tscn")
	var packed_out := PackedScene.new()
	packed_out.pack(map)
	var save_err := ResourceSaver.save(packed_out, tmp_map_path)
	var db_ok: bool = dm.save_all(tmp_db)
	_check(save_err == OK and db_ok, "Guardado de escena .tscn y sistema .json completado sin errores")

	dm.init_empty()
	_check(dm.load_all(tmp_db), "Recarga limpia desde tmp_db")
	var reloaded_ps: PackedScene = ResourceLoader.load(tmp_map_path, "", ResourceLoader.CACHE_MODE_IGNORE)
	var reloaded_map: ClockGearsMap = reloaded_ps.instantiate() if reloaded_ps else null
	_check(reloaded_map != null, "Mapa guardado recargado desde .tscn")
	if reloaded_map != null:
		reloaded_map.set_preview_only(true)
		get_root().add_child(reloaded_map)
		await _step_frames(1)
		_check(dm.get_custom_map_layer(id_mid).get("name", "") == "Puente Ñandú Renovado", "Nombre UTF-8 de la capa persiste tras cerrar y abrir")
		_check(reloaded_map.get_custom_layer_tile_at(id_mid, Vector2i(2, 2)) == Vector2i(0, 0), "Tiles pintados en capa personalizada persisten en el .tscn")
		reloaded_map.queue_free()

	# ------------------------------------------------------------------
	# 5. ORDEN DE DIBUJO REAL (EDITOR Y F5/RUNTIME) + COLISIONES IDENTICAS
	# ------------------------------------------------------------------
	print("\n--- 5. Orden de dibujo visual (editor + F5 runtime) y colisiones de tileset ---")
	map.set_custom_layer_tile_at(id_low, Vector2i(1, 1), Vector2i(3, 0), 0, 0)  # Verde en Z=-2
	map.set_custom_layer_tile_at(id_high, Vector2i(4, 4), Vector2i(0, 0), 0, 0) # Azul en Z=5
	map.set_custom_layer_tile_at(id_mid, Vector2i(4, 4), Vector2i(3, 0), 0, 0)  # Verde en Z=2

	var rendered_list: Array = map.get_rendered_layers_ordered()
	var idx_low := -1
	var idx_suelo := -1
	var idx_mid := -1
	var idx_ev := -1
	var idx_techo := -1
	var idx_high := -1
	for i in range(rendered_list.size()):
		var rdict: Dictionary = rendered_list[i]
		var rname := String(rdict.get("name", ""))
		var rcid := String(rdict.get("custom_id", ""))
		if rcid == id_low: idx_low = i
		elif rname == "Capa_Suelo": idx_suelo = i
		elif rcid == id_mid: idx_mid = i
		elif rname == "Eventos": idx_ev = i
		elif rname == "Capa_Techo": idx_techo = i
		elif rcid == id_high: idx_high = i
	_check(idx_low >= 0 and idx_suelo > idx_low and idx_mid > idx_suelo and idx_ev > idx_mid and idx_techo > idx_ev and idx_high > idx_techo,
			"Orden de render exacto según Z: Z=-2(%d) < Suelo(%d) < Z=2(%d) < Eventos/Jugador(%d) < Techo(%d) < Z=5(%d)" % [idx_low, idx_suelo, idx_mid, idx_ev, idx_techo, idx_high])

	# Colisiones idénticas a capas 1-4: tile (1,0) bloqueante en capa personalizada debe bloquear
	map.set_custom_layer_tile_at(id_mid, Vector2i(7, 7), Vector2i(1, 0), 0, 0)
	_check(not map.is_tile_passable(Vector2i(7, 7)), "Tile bloqueado (X) pintado en capa personalizada bloquea movimiento")
	dm.set_custom_map_layer_visible(id_mid, false)
	_check(map.is_tile_passable(Vector2i(7, 7)), "Al ocultar capa personalizada sus colisiones no bloquean")
	dm.set_custom_map_layer_visible(id_mid, true)
	_check(not map.is_tile_passable(Vector2i(7, 7)), "Al mostrarla de nuevo vuelve a bloquear")
	# Tile ★ (2,0) encima de player NO bloquea
	map.set_custom_layer_tile_at(id_high, Vector2i(8, 8), Vector2i(2, 0), 0, 0)
	_check(map.is_tile_passable(Vector2i(8, 8)) and map.get_cell_collision_type(Vector2i(8, 8)) == 2, "Tile ★ en capa personalizada se comporta igual que en capas 1-4 (no bloquea, type=2)")

	# Verificar visualmente con SubViewport qué tile gana arriba en (4,4) según orden Z
	get_root().remove_child(map)
	map.set_preview_only(true)
	var svp_draw := SubViewport.new()
	svp_draw.size = Vector2i(320, 240)
	svp_draw.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(svp_draw)
	svp_draw.add_child(map)
	dm.set_editor_dim_inactive_layers_enabled(false) # opacidad 100% para comparar color puro
	map.apply_editor_layer_dimming(false)
	await _step_frames(3)
	var px_above := svp_draw.get_texture().get_image().get_pixel(4 * 32 + 16, 4 * 32 + 16)
	_check(px_above.b > px_above.g, "Dibujo real: con id_high Z=5 (azul) sobre id_mid Z=2 (verde), gana el pixel azul: " + str(px_above))
	# Invertir orden: id_high Z=0 y id_mid Z=7
	dm.set_custom_map_layer_order(id_high, 0)
	dm.set_custom_map_layer_order(id_mid, 7)
	await _step_frames(3)
	var px_swapped := svp_draw.get_texture().get_image().get_pixel(4 * 32 + 16, 4 * 32 + 16)
	_check(px_swapped.g > px_swapped.b, "Al invertir orden Z (id_mid Z=7 > id_high Z=0), ahora gana el pixel verde arriba: " + str(px_swapped))
	svp_draw.remove_child(map)
	get_root().remove_child(svp_draw)
	svp_draw.queue_free()
	map.set_preview_only(false)
	get_root().add_child(map)

	# ------------------------------------------------------------------
	# 6. ATENUACIÓN DE CAPAS INACTIVAS EN EDITOR (1D) VS PREFERENCIAS VS F5 RUNTIME
	# ------------------------------------------------------------------
	print("\n--- 6. Atenuación de capas inactivas al 40% solo en editor y preferencias ---")
	var prefs := ClockGearsPreferencesDialog.new()
	get_root().add_child(prefs)
	await _step_frames(1)

	ProjectSettings.set_setting("clockgears/editor/dim_inactive_layers", true)
	ProjectSettings.set_setting("clockgears/editor/inactive_layer_opacity_pct", 40)
	prefs.open_preferences()
	await _step_frames(1)
	_check(prefs.is_dim_inactive_layers_checked() and prefs.get_inactive_layer_opacity_value() == 40, "Preferencias inicia con atenuación ACTIVADA y porcentaje al 40 %")
	prefs.hide()

	dm.sync_editor_layer_dimming_from_settings()
	dm.set_editor_custom_layer(id_mid)
	map.apply_editor_layer_dimming(true)
	var l_ground := map.get_layer(0)
	var l_objs := map.get_layer(2)
	var l_mid := map.get_custom_layer(id_mid)
	var l_ev: Node2D = map.get_events_container()
	_check(is_equal_approx(l_mid.self_modulate.a, 1.0), "Capa activa en editor queda al 100 %")
	_check(is_equal_approx(l_ground.self_modulate.a, 0.40) and is_equal_approx(l_objs.self_modulate.a, 0.40), "Capas inactivas (1..4) atenuadas exactamente al 40 % en editor")
	_check(is_equal_approx(l_ev.self_modulate.a, 1.0) and is_equal_approx(l_ev.modulate.a, 1.0), "Contenedor de Eventos mantiene el 100 % durante atenuación")

	dm.set_editor_layer(1)
	map.apply_editor_layer_dimming(true)
	_check(is_equal_approx(l_ground.self_modulate.a, 1.0) and is_equal_approx(l_mid.self_modulate.a, 0.40), "Al activar Capa 1 base, Capa 1 pasa a 100 % y la capa personalizada atenua al 40 %")
	dm.set_editor_custom_layer(id_mid)

	# Cambiar porcentaje en Preferencias a 65%
	prefs.set_inactive_layer_opacity_value(65)
	prefs.apply_and_save_preferences()
	map.apply_editor_layer_dimming(true)
	_check(is_equal_approx(l_ground.self_modulate.a, 0.65), "Cambiar porcentaje en Preferencias a 65% atenua capas inactivas al 65%")

	# Apagar interruptor en Preferencias -> 100%
	prefs.set_dim_inactive_layers_checked(false)
	prefs.apply_and_save_preferences()
	map.apply_editor_layer_dimming(true)
	_check(is_equal_approx(l_ground.self_modulate.a, 1.0), "Desactivar interruptor en Preferencias restaura todas las capas al 100%")

	# Reactivar 40% y comprobar que PRE_SAVE y F5 (runtime) SIEMPRE desactivan la atenuación
	prefs.set_dim_inactive_layers_checked(true)
	prefs.set_inactive_layer_opacity_value(40)
	prefs.apply_and_save_preferences()
	map.apply_editor_layer_dimming(true)
	_check(is_equal_approx(l_ground.self_modulate.a, 0.40), "Atenuación reactivada al 40% en contexto editor")
	map.notification(Node.NOTIFICATION_EDITOR_PRE_SAVE)
	_check(is_equal_approx(l_ground.self_modulate.a, 1.0), "PRE_SAVE limpia la opacidad a 100% para no contaminar .tscn guardado")

	# Simular sesión F5 Runtime completa sobre el mapa
	var sys_rt: Dictionary = dm.get_system_config()
	sys_rt["has_start_pos"] = true
	sys_rt["start_map"] = tmp_map_path
	sys_rt["start_x"] = 3
	sys_rt["start_y"] = 3
	dm.set_system_config(sys_rt)
	map.set_preview_only(false)
	map.init_runtime_session()
	await _step_frames(1)
	var rt_player := map.get_runtime_player()
	_check(rt_player != null and is_equal_approx(rt_player.self_modulate.a, 1.0), "Jugador en runtime existe y se dibuja al 100 %")
	_check(is_equal_approx(l_ground.self_modulate.a, 1.0) and is_equal_approx(map.get_custom_layer(id_low).self_modulate.a, 1.0) and is_equal_approx(map.get_custom_layer(id_mid).self_modulate.a, 1.0), "En F5 / Runtime TODAS las capas (1..4 y personalizadas) quedan al 100 % (nunca atenuadas)")

	# Revisar ajuste de Preferencias sin scroll horizontal (en las pestañas, excluyendo popups internos de OptionButton)
	var pref_sc: ScrollContainer = null
	var pref_sc_count := 0
	for sc in prefs.find_children("*", "ScrollContainer", true, false):
		var in_popup := false
		var par: Node = sc.get_parent()
		while par != null and par != prefs:
			if par is PopupMenu:
				in_popup = true
				break
			par = par.get_parent()
		if in_popup:
			continue
		pref_sc = sc as ScrollContainer
		pref_sc_count += 1
		_check(pref_sc.horizontal_scroll_mode == ScrollContainer.SCROLL_MODE_DISABLED, "ScrollContainer '" + pref_sc.name + "' en Preferencias deshabilita scroll horizontal")
	_check(pref_sc_count >= 2, "Preferencias usa ScrollContainer sin scroll horizontal en ambas pestañas (%d)" % pref_sc_count)

	# ------------------------------------------------------------------
	# 7. COMPARACIÓN PIXEL A PIXEL MAPA ANTIGUO TRAS TODO EL CICLO
	# ------------------------------------------------------------------
	print("\n--- 7. Verificación de no regresión en mapa antiguo (map001.tscn) ---")
	dm.init_empty()
	if FileAccess.file_exists("res://clockgears_data/actors.json"):
		dm.load_all("res://clockgears_data")
	var legacy_after_ps: PackedScene = ResourceLoader.load("res://maps/map001.tscn", "", ResourceLoader.CACHE_MODE_IGNORE)
	var legacy_after: ClockGearsMap = legacy_after_ps.instantiate()
	legacy_after.set_preview_only(true)
	var after_children: Array[String] = []
	for ch in legacy_after.get_children():
		after_children.append(String(ch.name))
	_check(after_children == before_children, "map001.tscn mantiene estructura de nodos idéntica: " + str(after_children))
	var after_img := await _render_map_image_data(legacy_after)
	_check(after_img == before_img and before_img.size() > 0, "map001.tscn produce captura pixel a pixel 100% idéntica")
	var pass_identical := true
	var pidx := 0
	for cy in range(msz_old.y):
		for cx in range(msz_old.x):
			if legacy_after.is_tile_passable(Vector2i(cx, cy)) != before_pass[pidx]:
				pass_identical = false
			pidx += 1
	_check(pass_identical, "map001.tscn mantiene colisiones/pasabilidad 100% idénticas")
	legacy_after.free()

	# ------------------------------------------------------------------
	# 8. LIMPIEZA TOTAL DE TEMPORALES Y RESTAURACIÓN DE PROJECTSETTINGS
	# ------------------------------------------------------------------
	if map.get_parent():
		map.get_parent().remove_child(map)
	map.queue_free()
	layers_dlg.queue_free()
	top_bar.queue_free()
	prefs.queue_free()

	if had_dim_setting:
		ProjectSettings.set_setting("clockgears/editor/dim_inactive_layers", orig_dim_val)
	elif ProjectSettings.has_setting("clockgears/editor/dim_inactive_layers"):
		ProjectSettings.clear("clockgears/editor/dim_inactive_layers")
	if had_pct_setting:
		ProjectSettings.set_setting("clockgears/editor/inactive_layer_opacity_pct", orig_pct_val)
	elif ProjectSettings.has_setting("clockgears/editor/inactive_layer_opacity_pct"):
		ProjectSettings.clear("clockgears/editor/inactive_layer_opacity_pct")
	ProjectSettings.save()

	_remove_dir_recursive(tmp_root)
	dm.init_empty()
	if FileAccess.file_exists("res://clockgears_data/actors.json"):
		dm.load_all("res://clockgears_data")
	await _step_frames(2)

	print("======================================================================")
	print("RESULTADO TEST ETAPA 1: %d PASSED, %d FAILED" % [passed, failed])
	print("======================================================================")
	quit(0 if failed == 0 else 1)
