extends SceneTree

var passed: int = 0
var failed: int = 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		printerr("[FAIL] ", msg)

func _backup_data_dir(dir_res: String) -> Dictionary:
	var backup := {}
	var abs_dir := ProjectSettings.globalize_path(dir_res)
	var da := DirAccess.open(abs_dir)
	if da == null:
		return backup
	da.list_dir_begin()
	var fn := da.get_next()
	while fn != "":
		if not da.current_is_dir():
			var full := abs_dir.path_join(fn)
			backup[fn] = FileAccess.get_file_as_bytes(full)
		fn = da.get_next()
	da.list_dir_end()
	return backup

func _restore_data_dir(dir_res: String, backup: Dictionary) -> void:
	var abs_dir := ProjectSettings.globalize_path(dir_res)
	var da := DirAccess.open(abs_dir)
	if da != null:
		da.list_dir_begin()
		var fn := da.get_next()
		while fn != "":
			if not da.current_is_dir() and not backup.has(fn):
				da.remove(fn)
			fn = da.get_next()
		da.list_dir_end()
	for fn in backup.keys():
		var f := FileAccess.open(abs_dir.path_join(fn), FileAccess.WRITE)
		if f != null:
			f.store_buffer(backup[fn])
			f.close()

func _make_png(path: String, w: int, h: int, col: Color) -> String:
	var img := Image.create(w, h, false, Image.FORMAT_RGBA8)
	img.fill(col)
	var abs_p := ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs_p.get_base_dir())
	img.save_png(abs_p)
	return path

func _find_button_by_text(root: Node, needle: String, exact: bool = false) -> Button:
	for n in root.find_children("*", "Button", true, false):
		var b := n as Button
		if exact:
			if b.text == needle:
				return b
		elif b.text.find(needle) != -1:
			return b
	return null

func _find_checkbox_by_text(root: Node, needle: String) -> CheckBox:
	for n in root.find_children("*", "CheckBox", true, false):
		var c := n as CheckBox
		if c.text.find(needle) != -1:
			return c
	return null

func _find_option_containing(root: Node, item_text: String) -> OptionButton:
	for n in root.find_children("*", "OptionButton", true, false):
		var ob := n as OptionButton
		for i in range(ob.get_item_count()):
			if ob.get_item_text(i).find(item_text) != -1:
				return ob
	return null

func _select_option_item(ob: OptionButton, item_text: String) -> bool:
	if ob == null:
		return false
	for i in range(ob.get_item_count()):
		if ob.get_item_text(i).find(item_text) != -1:
			ob.select(i)
			ob.emit_signal("item_selected", i)
			return true
	return false

func _send_mouse_click(canvas: Control, pos: Vector2, button: int = MOUSE_BUTTON_LEFT) -> void:
	var press := InputEventMouseButton.new()
	press.button_index = button
	press.pressed = true
	press.position = pos
	canvas.call("handle_canvas_event", press)
	var release := InputEventMouseButton.new()
	release.button_index = button
	release.pressed = false
	release.position = pos
	canvas.call("handle_canvas_event", release)

func _send_mouse_drag(canvas: Control, from_pos: Vector2, to_pos: Vector2, button: int = MOUSE_BUTTON_LEFT) -> void:
	var press := InputEventMouseButton.new()
	press.button_index = button
	press.pressed = true
	press.position = from_pos
	canvas.call("handle_canvas_event", press)

	var motion := InputEventMouseMotion.new()
	motion.position = to_pos
	motion.relative = to_pos - from_pos
	motion.button_mask = MOUSE_BUTTON_MASK_LEFT if button == MOUSE_BUTTON_LEFT else 0
	canvas.call("handle_canvas_event", motion)

	var release := InputEventMouseButton.new()
	release.button_index = button
	release.pressed = false
	release.position = to_pos
	canvas.call("handle_canvas_event", release)

func _send_mouse_wheel(canvas: Control, pos: Vector2, dir_up: bool) -> void:
	var wb := InputEventMouseButton.new()
	wb.button_index = MOUSE_BUTTON_WHEEL_UP if dir_up else MOUSE_BUTTON_WHEEL_DOWN
	wb.pressed = true
	wb.position = pos
	canvas.call("handle_canvas_event", wb)

func _initialize() -> void:
	create_timer(45.0).timeout.connect(func():
		printerr("[TIMEOUT] Test UI Cuerpo/Zonas/Movimiento abortado tras 45s")
		quit(2)
	)
	call_deferred("_run_tests")

func _run_tests() -> void:
	var backup := _backup_data_dir("res://clockgears_data")
	var tmp_dir := "res://tmp_import_src/bz_ui_9a_test"
	var sprite_png := _make_png(tmp_dir + "/enemy_idle.png", 32, 32, Color(0.85, 0.25, 0.25, 1.0))

	var dm = ClockGearsDataManager
	dm.init_empty()
	var sys: Dictionary = dm.get_system_config()
	sys["combat_mode"] = 1 # ARPG
	sys["movement_directions"] = 4
	sys["party"] = [1]
	dm.set_system_config(sys)

	dm.set_records("actors", [{
		"id": 1,
		"name": "HeroeBz",
		"hp": 100, "mp": 20, "atk": 10, "def": 6, "mat": 5, "mdf": 5, "agi": 8, "luk": 5
	}])
	dm.set_records("npcs", [{
		"id": 1,
		"name": "AldeanoBz"
	}])
	dm.set_records("arpg_npcs", [{
		"id": 1,
		"name": "AliadoArpgBz"
	}])
	dm.set_records("arpg_enemies", [{
		"id": 1,
		"name": "SlimeGuardia",
		"ai_type": "quieto",
		"sprite_data": {
			"animations": {
				"quieto": {
					"spritesheet_path": sprite_png,
					"cols": 1, "rows": 1, "fps": 6.0, "loop": true,
					"frames_data": [
						{"sheet_path": sprite_png, "region": Rect2i(0, 0, 32, 32), "duration": 0.16}
					]
				}
			}
		},
		"event_pages": [
			{"name": "Pagina 1", "commands": []},
			{"name": "Pagina 2", "commands": []}
		]
	}])
	dm.save_all("res://clockgears_data")

	# =========================================================================
	# 1. Abrir BD -> Enemigos ARPG -> Pestaña "Cuerpo y zonas"
	# =========================================================================
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await process_frame
	db.size = Vector2i(1366, 768)
	db.open_database()
	db.select_category_by_key("arpg_enemies")
	await process_frame

	check(db.select_main_tab_by_name("Cuerpo y zonas"), "Pestaña 'Cuerpo y zonas' visible y seleccionable en Enemigos ARPG")
	await process_frame

	var bz_ed: ClockGearsShapeEditor = db.get_body_zones_editor()
	check(bz_ed != null, "get_body_zones_editor() devuelve instancia compartida de ClockGearsShapeEditor")
	var bz_canvas: Control = bz_ed.get_shape_canvas()
	check(bz_canvas != null, "Lienzo de Cuerpo y zonas disponible")

	bz_ed.set_zoom(2)
	bz_ed.set_snap_enabled(false)
	await process_frame

	# Regla rueda raton NO hace zoom
	var z_before: int = bz_ed.get_zoom()
	_send_mouse_wheel(bz_canvas, bz_canvas.get_canvas_origin(), true)
	_send_mouse_wheel(bz_canvas, bz_canvas.get_canvas_origin(), false)
	check(bz_ed.get_zoom() == z_before, "Rueda del ratón NUNCA cambia el zoom en Cuerpo y zonas")

	# Cuerpo circular: arrastrar tirador para cambiar radio y arrastrar cuerpo para moverlo
	bz_ed.select_entity_item(-1) # Cuerpo
	await process_frame
	var body_init: Dictionary = bz_ed.get_body_shape_data()
	var r0: float = float(body_init.get("radius", 12.0))
	var off0 := Vector2(float(body_init.get("offset_x", 0.0)), float(body_init.get("offset_y", 0.0)))
	var origin: Vector2 = bz_canvas.get_canvas_origin()
	var z := float(bz_ed.get_zoom())
	var handle_scr: Vector2 = origin + (off0 + Vector2(r0, 0.0)) * z
	var handle_dst: Vector2 = origin + (off0 + Vector2(r0 + 8.0, 0.0)) * z
	_send_mouse_drag(bz_canvas, handle_scr, handle_dst)
	await process_frame
	var body_resized: Dictionary = bz_ed.get_body_shape_data()
	var r1: float = float(body_resized.get("radius", 0.0))
	check(r1 > r0 + 4.0, "Arrastrar tirador del Cuerpo circular aumenta su radio (%s -> %s)" % [r0, r1])

	var body_center_scr: Vector2 = origin + off0 * z
	_send_mouse_drag(bz_canvas, body_center_scr, body_center_scr + Vector2(6.0 * z, 4.0 * z))
	await process_frame
	var body_moved: Dictionary = bz_ed.get_body_shape_data()
	check(abs(float(body_moved.get("offset_x", 0.0)) - (off0.x + 6.0)) < 1.5, "Arrastrar Cuerpo sobre el lienzo desplaza offset_x")

	var chk_through := _find_checkbox_by_text(bz_ed, "Atravesable")
	check(chk_through != null and chk_through.is_visible_in_tree(), "Casilla 'Atravesable' visible en Cuerpo")
	chk_through.set_pressed(true)
	chk_through.emit_signal("toggled", true)
	await process_frame
	check(bz_ed.is_body_through(), "Activar 'Atravesable' actualiza body_through = true")

	# =========================================================================
	# 2. Dos zonas (circular + rectangular), drag, selec anidada y acciones
	# =========================================================================
	bz_ed.set_zoom(1)
	await process_frame
	origin = bz_canvas.get_canvas_origin()

	# Limpiar zonas por defecto con '-' si existen, para probar '-' y partir de 0
	while bz_ed.get_zones_data().size() > 0:
		bz_ed.select_entity_item(0)
		bz_ed.remove_selected_entity_zone()
	await process_frame
	check(bz_ed.get_zones_data().size() == 0, "Eliminar zonas previas deja 0 zonas iniciales")

	# Zona 0: circular
	var z0_idx: int = bz_ed.add_entity_zone({
		"name": "ZonaPerseguir",
		"shape": {"type": "circle", "radius": 80.0, "offset_x": 0.0, "offset_y": 0.0},
		"target_filter": "player",
		"trigger": "enter"
	})
	check(z0_idx == 0, "Zona circular añadida con índice 0")
	bz_ed.select_entity_item(0)
	await process_frame

	# Arrastrar tirador radial de Zona 0
	var z0_r0: float = float(bz_ed.get_zones_data()[0]["shape"].get("radius", 80.0))
	_send_mouse_drag(bz_canvas, origin + Vector2(z0_r0, 0.0), origin + Vector2(z0_r0 + 25.0, 0.0))
	await process_frame
	var z0_r1: float = float(bz_ed.get_zones_data()[0]["shape"].get("radius", 0.0))
	check(z0_r1 > z0_r0 + 15.0, "Arrastrar tirador de Zona circular aumenta su radio (%s -> %s)" % [z0_r0, z0_r1])

	# Clic dentro del Cuerpo (anidado dentro de Zona 0) selecciona el Cuerpo (-1), no la zona grande
	var moved_body_off := Vector2(float(bz_ed.get_body_shape_data().get("offset_x", 0.0)), float(bz_ed.get_body_shape_data().get("offset_y", 0.0)))
	_send_mouse_click(bz_canvas, origin + moved_body_off)
	await process_frame
	check(bz_ed.get_selected_bz_index() == -1, "Clic en Cuerpo anidado dentro de zona grande selecciona Cuerpo (-1)")

	# Volver a seleccionar Zona 0 y añadir acción 'perseguir' vía UI de propiedades de zona
	bz_ed.select_entity_item(0)
	await process_frame
	var bz_page: Control = bz_ed.get_parent() as Control
	var btn_act_add := _find_button_by_text(bz_page, "+", true)
	check(btn_act_add != null, "Botón '+' de acciones de zona encontrado")
	btn_act_add.emit_signal("pressed")
	await process_frame
	var act_type_opt := _find_option_containing(bz_page, "Perseguir")
	check(_select_option_item(act_type_opt, "Perseguir"), "Acción 'Perseguir' seleccionada en selector de Zona 0")
	await process_frame

	# Zona 1: rectangular (desplazada a la derecha para no solapar)
	var z1_idx: int = bz_ed.add_entity_zone({
		"name": "ZonaEvento",
		"shape": {"type": "rect", "size_x": 64.0, "size_y": 48.0, "offset_x": 70.0, "offset_y": -10.0},
		"target_filter": "player",
		"trigger": "enter"
	})
	check(z1_idx == 1, "Zona rectangular añadida con índice 1")
	bz_ed.select_entity_item(1)
	await process_frame

	# Arrastrar centro de Zona 1 para moverla (evitando zona de cuerpo para coger Zona 1)
	var z1_before: Dictionary = bz_ed.get_zones_data()[1]["shape"]
	var z1_x0: float = float(z1_before.get("offset_x", 70.0))
	var z1_y0: float = float(z1_before.get("offset_y", -10.0))
	var z1_w0: float = float(z1_before.get("size_x", 64.0))
	var z1_h0: float = float(z1_before.get("size_y", 48.0))
	# Redimensionar desde tirador SE (offset + (size_x/2, size_y/2))
	var z1_se := origin + Vector2(z1_x0 + z1_w0 * 0.5, z1_y0 + z1_h0 * 0.5)
	_send_mouse_drag(bz_canvas, z1_se, z1_se + Vector2(16.0, 12.0))
	await process_frame
	var z1_resized: Dictionary = bz_ed.get_zones_data()[1]["shape"]
	check(float(z1_resized.get("size_x", 0.0)) > z1_w0 + 10.0, "Arrastrar tirador de Zona rectangular cambia su ancho (%s -> %s)" % [z1_w0, z1_resized.get("size_x", 0.0)])

	var z1_pick := origin + Vector2(z1_x0, z1_y0)
	_send_mouse_drag(bz_canvas, z1_pick, z1_pick + Vector2(18.0, -12.0))
	await process_frame
	var z1_after: Dictionary = bz_ed.get_zones_data()[1]["shape"]
	check(float(z1_after.get("offset_x", 0.0)) > z1_x0 + 10.0, "Arrastrar Zona rectangular desplaza offset_x")

	# Añadir acción 'ejecutar_evento' en Zona 1
	btn_act_add.emit_signal("pressed")
	await process_frame
	check(_select_option_item(act_type_opt, "Ejecutar evento"), "Acción 'Ejecutar evento' seleccionada en Zona 1")
	await process_frame
	var event_page_opt := _find_option_containing(bz_page, "Página")
	check(event_page_opt != null and event_page_opt.is_visible_in_tree(), "Selector de página de evento visible para acción 'Ejecutar evento'")

	# Comprobar Actores solo muestra Cuerpo (sin botón '+ Zona')
	db.select_category_by_key("actors")
	await process_frame
	db.select_main_tab_by_name("Cuerpo y zonas")
	await process_frame
	var btn_add_zone_in_actor := _find_button_by_text(bz_ed, "+ Zona")
	check(btn_add_zone_in_actor != null and not btn_add_zone_in_actor.is_visible_in_tree(), "Categoría Actores oculta botones de zonas (solo cuerpo)")

	# Volver a Enemigos ARPG para Pestaña "Movimiento"
	db.select_category_by_key("arpg_enemies")
	await process_frame

	# =========================================================================
	# 3. Pestaña "Movimiento" -> Patrulla -> colocar y arrastrar puntos
	# =========================================================================
	check(db.select_main_tab_by_name("Movimiento"), "Pestaña 'Movimiento' visible y seleccionable en Enemigos ARPG")
	await process_frame

	var mov_ed: ClockGearsShapeEditor = db.get_movement_shape_editor()
	check(mov_ed != null and mov_ed.get_editor_mode() == 2, "get_movement_shape_editor() está en MODE_MOVEMENT (2)")
	var mov_canvas: Control = mov_ed.get_shape_canvas()
	var mov_page: Control = mov_ed.get_parent() as Control

	# Rueda no hace zoom en Movimiento tampoco
	var mz_before: int = mov_ed.get_zoom()
	_send_mouse_wheel(mov_canvas, mov_canvas.get_canvas_origin(), true)
	check(mov_ed.get_zoom() == mz_before, "Rueda del ratón tampoco hace zoom en pestaña Movimiento")

	var mov_type_opt := _find_option_containing(mov_page, "Patrulla")
	check(_select_option_item(mov_type_opt, "Patrulla"), "Modo de movimiento 'Patrulla' seleccionado")
	await process_frame

	var m_origin: Vector2 = mov_canvas.get_canvas_origin()
	var mz := float(mov_ed.get_zoom())
	# Clic en (2, 0) tiles y luego en (3, -2) tiles para colocar puntos sobre el lienzo
	_send_mouse_click(mov_canvas, m_origin + Vector2(2.0 * 32.0 * mz, 0.0))
	await process_frame
	_send_mouse_click(mov_canvas, m_origin + Vector2(3.0 * 32.0 * mz, -2.0 * 32.0 * mz))
	await process_frame
	var pts_added: Array = mov_ed.get_patrol_points_data()
	check(pts_added.size() == 2, "Clic sobre lienzo coloca 2 puntos de patrulla (size=%d)" % pts_added.size())

	# Arrastrar el segundo punto de patrulla (índice 1) a (5, 1)
	var p1 := Vector2(float(pts_added[1].get("x", 0.0)), float(pts_added[1].get("y", 0.0)))
	var p1_scr: Vector2 = m_origin + p1 * 32.0 * mz
	var p1_dst: Vector2 = m_origin + Vector2(5.0, 1.0) * 32.0 * mz
	_send_mouse_drag(mov_canvas, p1_scr, p1_dst)
	await process_frame
	var pts_dragged: Array = mov_ed.get_patrol_points_data()
	var p1_after_x: float = float(pts_dragged[1].get("x", 0.0)) if pts_dragged.size() > 1 else 0.0
	var p1_after_y: float = float(pts_dragged[1].get("y", 0.0)) if pts_dragged.size() > 1 else 0.0
	check(abs(p1_after_x - 5.0) < 0.6 and abs(p1_after_y - 1.0) < 0.6, "Arrastrar punto de patrulla en lienzo mueve sus coordenadas (%s, %s)" % [p1_after_x, p1_after_y])

	var patrol_loop_opt := _find_option_containing(mov_page, "Bucle cerrado")
	check(_select_option_item(patrol_loop_opt, "Bucle cerrado"), "Selector 'Ida y vuelta / Bucle cerrado' permite 'Bucle cerrado'")
	await process_frame

	# =========================================================================
	# 4. Layout 16:9 a 1366x768 y 1920x1080 en categorías relevantes
	# =========================================================================
	for cat_key in ["arpg_enemies", "arpg_npcs", "npcs", "actors"]:
		db.select_category_by_key(cat_key)
		await process_frame
		for res_sz in [Vector2i(1366, 768), Vector2i(1920, 1080)]:
			db.size = res_sz
			await process_frame
			await process_frame
			var report: Dictionary = db.check_all_tabs_layout(res_sz)
			await process_frame
			var ok := bool(report.get("all_ok", false))
			if not ok:
				print("[LAYOUT DIAGNOSTIC %s %dx%d] " % [cat_key, res_sz.x, res_sz.y], report)
			check(ok, "Todas las pestañas visibles de '%s' sin clipping ni scroll horizontal a %dx%d" % [cat_key, res_sz.x, res_sz.y])

	# =========================================================================
	# 5. Guardar, cerrar BD, recargar y verificar persistencia total
	# =========================================================================
	db.save_to_disk()
	db.hide()
	db.queue_free()
	await process_frame

	dm.init_empty()
	var db2 := ClockGearsDatabaseDialog.new()
	root.add_child(db2)
	await process_frame
	db2.load_from_disk()
	db2.open_database()
	db2.select_category_by_key("arpg_enemies")
	await process_frame

	var recs_loaded: Array = dm.get_records("arpg_enemies")
	var e_rec: Dictionary = recs_loaded[0] if recs_loaded.size() > 0 else {}
	var p_body: Dictionary = e_rec.get("body_shape", {})
	var p_zones: Array = e_rec.get("detection_zones", [])
	var p_pts: Array = e_rec.get("patrol_points", [])

	check(abs(float(p_body.get("radius", 0.0)) - r1) < 1.0 and bool(e_rec.get("body_through", false)),
		"Persistencia Cuerpo circular (radius=%s) y Atravesable tras cerrar/reabrir BD" % p_body.get("radius", 0.0))
	check(p_zones.size() == 2, "Persistencia de las 2 zonas de detección tras reabrir BD (count=%d)" % p_zones.size())
	if p_zones.size() >= 2:
		var z0_acts: Array = p_zones[0].get("actions", [])
		var z1_acts: Array = p_zones[1].get("actions", [])
		var has_chase: bool = z0_acts.size() >= 1 and String(z0_acts[0].get("type", "")) == "perseguir"
		var has_ev: bool = z1_acts.size() >= 1 and String(z1_acts[0].get("type", "")) == "ejecutar_evento"
		check(has_chase and String(p_zones[0].get("shape", {}).get("type", "")) == "circle", "Zona 0 circular con acción 'perseguir' persiste")
		check(has_ev and String(p_zones[1].get("shape", {}).get("type", "")) == "rect", "Zona 1 rectangular con acción 'ejecutar_evento' persiste")
	check(String(e_rec.get("ai_type", "")) == "patrulla" and bool(e_rec.get("patrol_loop", false)) and p_pts.size() == 2,
		"Modo 'patrulla', 'Bucle cerrado' y 2 puntos de patrulla persisten tras reabrir BD")

	# Verificar que Tab Combate de Enemigo ARPG también refleja 'patrulla' sincronizado
	db2.select_main_tab_by_name("Combate")
	await process_frame
	var comb_page: Control = db2.get_main_editor_tabs().get_current_tab_control()
	var comb_ai_opt := _find_option_containing(comb_page, "Patrulla")
	check(comb_ai_opt != null and String(comb_ai_opt.get_selected_metadata()) == "patrulla", "Tipo IA en pestaña Combate sincronizado con 'patrulla'")

	db2.queue_free()
	await process_frame

	# =========================================================================
	# 6. En juego: F3 checkbox "Mostrar cuerpos y zonas" activa overlay visual
	# =========================================================================
	var dbg := ClockGearsDebugOverlay.new()
	root.add_child(dbg)
	await process_frame
	var f3_chk := _find_checkbox_by_text(dbg, "Mostrar cuerpos y zonas")
	check(f3_chk != null and f3_chk.text == "Mostrar cuerpos y zonas", "Panel F3 contiene casilla exacta 'Mostrar cuerpos y zonas'")
	if f3_chk != null:
		f3_chk.set_pressed(true)
		f3_chk.emit_signal("toggled", true)
		await process_frame
		var wc := dbg.get_world_canvas()
		check(dbg.is_show_bodies_zones() and wc != null and wc.is_show_bodies_zones(), "Marcar 'Mostrar cuerpos y zonas' en F3 activa dibujado en WorldCanvas")
	dbg.queue_free()
	await process_frame

	# =========================================================================
	# Limpieza
	# =========================================================================
	_restore_data_dir("res://clockgears_data", backup)
	dm.load_all("res://clockgears_data")
	var abs_tmp := ProjectSettings.globalize_path(tmp_dir)
	if FileAccess.file_exists(abs_tmp.path_join("enemy_idle.png")):
		DirAccess.remove_absolute(abs_tmp.path_join("enemy_idle.png"))
	DirAccess.remove_absolute(abs_tmp)

	print("=== TEST BODIES ZONES UI 9A SUMMARY: passed=%d failed=%d ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
