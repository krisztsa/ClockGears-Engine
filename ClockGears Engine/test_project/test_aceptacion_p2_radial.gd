extends SceneTree

# Prueba de aceptación Punto 2: La rueda radial es un elemento real con botones hijo editables.

var pass_count: int = 0
var fail_count: int = 0
var _finished: bool = false

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] " + msg)
	else:
		fail_count += 1
		printerr("  [FAIL] " + msg)

func _finish(code: int) -> void:
	if _finished:
		return
	_finished = true
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(code)

func _step_frames(n: int = 2) -> void:
	var c := mini(n, 600)
	for _i in range(c):
		await process_frame

func _drag(win: Window, from_pos: Vector2, to_pos: Vector2) -> void:
	var ev_d := InputEventMouseButton.new()
	ev_d.button_index = MOUSE_BUTTON_LEFT
	ev_d.pressed = true
	ev_d.position = from_pos
	ev_d.global_position = from_pos
	win.push_input(ev_d)
	await _step_frames(1)

	for step in range(1, 5):
		var p := from_pos.lerp(to_pos, float(step) / 4.0)
		var ev_m := InputEventMouseMotion.new()
		ev_m.position = p
		ev_m.global_position = p
		ev_m.relative = (to_pos - from_pos) / 4.0
		ev_m.button_mask = MOUSE_BUTTON_MASK_LEFT
		win.push_input(ev_m)
		await _step_frames(1)

	var ev_u := InputEventMouseButton.new()
	ev_u.button_index = MOUSE_BUTTON_LEFT
	ev_u.pressed = false
	ev_u.position = to_pos
	ev_u.global_position = to_pos
	win.push_input(ev_u)
	await _step_frames(2)

func _click_at(win: Window, pos: Vector2, dbl: bool = false) -> void:
	var ev_d := InputEventMouseButton.new()
	ev_d.button_index = MOUSE_BUTTON_LEFT
	ev_d.pressed = true
	ev_d.double_click = dbl
	ev_d.position = pos
	ev_d.global_position = pos
	win.push_input(ev_d)

	var ev_u := InputEventMouseButton.new()
	ev_u.button_index = MOUSE_BUTTON_LEFT
	ev_u.pressed = false
	ev_u.position = pos
	ev_u.global_position = pos
	win.push_input(ev_u)
	await _step_frames(2)

func _select_meta(opt: OptionButton, target: String) -> void:
	if opt == null:
		return
	for i in range(opt.get_item_count()):
		if str(opt.get_item_metadata(i)) == target:
			opt.select(i)
			opt.emit_signal("item_selected", i)
			return

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== ACEPTACIÓN PUNTO 2: RUEDA RADIAL COMO ELEMENTO REAL ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var had_gm: bool = dm.has_custom_ui_design("game_menu_main")
	var orig_gm: Dictionary = dm.get_ui_screen_design("game_menu_main")
	var had_inv: bool = dm.has_custom_ui_design("game_menu_items")
	var orig_inv: Dictionary = dm.get_ui_screen_design("game_menu_items")
	var cap_dir := ProjectSettings.globalize_path("res://capturas_aceptacion")
	DirAccess.make_dir_recursive_absolute(cap_dir)

	# 1) Migración con copia de seguridad de una pantalla antigua layout.mode=="radial" sin elementos
	var mig_dir := ProjectSettings.globalize_path("user://test_p2_radial_mig")
	DirAccess.make_dir_recursive_absolute(mig_dir)
	var fdb := FileAccess.open(mig_dir.path_join("database.json"), FileAccess.WRITE)
	fdb.store_string("{\"schema_version\": 3}")
	fdb.close()
	var legacy_rad := {
		"schema_version": 5,
		"screens": {
			"game_menu_main": {
				"screen_id": "game_menu_main",
				"custom_enabled": true,
				"layout": {
					"mode": "radial",
					"radial_center_x": 480,
					"radial_center_y": 270,
					"radial_radius": 150,
					"radial_slots": [
						{"id": "s1", "title": "Objetos", "action": "items"},
						{"id": "s2", "title": "Opciones", "action": "options"},
						{"id": "s3", "title": "Cerrar", "action": "close"}
					]
				},
				"elements": []
			}
		}
	}
	var ui_path := mig_dir.path_join("ui_designs.json")
	var bak_path := ui_path + ".pre_radial_elements.bak"
	if FileAccess.file_exists(bak_path):
		DirAccess.remove_absolute(bak_path)
	var fa := FileAccess.open(ui_path, FileAccess.WRITE)
	fa.store_string(JSON.stringify(legacy_rad, "\t"))
	fa.close()

	var mig_dm = ClassDB.instantiate("ClockGearsDataManager")
	mig_dm.load_all(mig_dir)
	_check(FileAccess.file_exists(bak_path), "Migración radial crea copia de seguridad .pre_radial_elements.bak")
	var mig_rep: Dictionary = mig_dm.get_last_ui_migration_report()
	var mrads: Array = mig_rep.get("migrated_radials", [])
	_check(mrads.size() == 1 and int(mrads[0].get("child_buttons", 0)) == 3, "Migración convierte pantalla radial en elemento con 3 hijos botón (%s)" % str(mrads))
	var mig_els: Array = mig_dm.get_ui_screen_elements("game_menu_main")
	var has_rad_el := false
	var child_cnt := 0
	for me in mig_els:
		if str(me.get("type", "")) == "radial":
			has_rad_el = true
		elif str(me.get("type", "")) == "button" and str(me.get("parent_id", "")) == "game_menu_main_radial_wheel":
			child_cnt += 1
	_check(has_rad_el and child_cnt == 3 and str(mig_dm.get_ui_screen_design("game_menu_main").get("layout", {}).get("mode", "")) == "free", "Pantalla migrada pasa a modo free con elemento 'radial' y 3 botones hijo en árbol")
	mig_dm.free()

	# 2) Ventana real del editor: Menú del juego -> plantilla Anillo radial -> seleccionar, arrastrar, redimensionar, cambiar ranura
	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("game_menu")
	await _step_frames(4)

	dm.apply_menu_template("game_menu_main", "radial_ring", false, false)
	var ed := db.get_ui_designer_editor()
	ed.refresh_from_data_manager()
	await _step_frames(4)

	var cards: Array = ed.get_design_cards_info()
	var rad_id := ""
	var kid_ids: Array[String] = []
	for c in cards:
		if str(c.get("type", "")) == "radial":
			rad_id = str(c.get("id", ""))
		elif str(c.get("type", "")) == "button" and not rad_id.is_empty() and str(c.get("parent_id", "")) == rad_id:
			kid_ids.append(str(c.get("id", "")))
	_check(not rad_id.is_empty() and kid_ids.size() == 8, "En Menú del juego la rueda '%s' es elemento real con 8 botones hijo (%d)" % [rad_id, kid_ids.size()])

	# Seleccionar rueda con clic real en tarjeta de Elementos
	var h: Dictionary = ed.get_ui_handles()
	var canvas_stage: Control = h.get("canvas_stage", null) as Control
	var rad_card: Control = ed.get_design_card_control(rad_id)
	_check(rad_card != null and rad_card.is_visible_in_tree(), "Tarjeta de rueda radial visible en lista Elementos")
	await _click_at(db, rad_card.get_global_rect().position + Vector2(50, rad_card.size.y * 0.5))
	_check(ed.get_selected_element_id() == rad_id, "Clic real en árbol selecciona la rueda radial")

	var rad_before: Dictionary = dm.get_ui_element("game_menu_main", rad_id)
	var ox0 := int(rad_before.get("offset_x", 0))
	var oy0 := int(rad_before.get("offset_y", 0))
	var w0 := int(rad_before.get("width", 0))

	# Arrastrar rueda radial en el lienzo
	var st_r: Rect2 = ed.get_rendered_element_stage_rect(rad_id)
	var drag_start: Vector2 = canvas_stage.get_global_transform() * (st_r.position + st_r.size * 0.5)
	await _drag(db, drag_start, drag_start + Vector2(45, -25))
	var rad_moved: Dictionary = dm.get_ui_element("game_menu_main", rad_id)
	var ox1 := int(rad_moved.get("offset_x", 0))
	var oy1 := int(rad_moved.get("offset_y", 0))
	_check(abs(ox1 - ox0) >= 20 and abs(oy1 - oy0) >= 10, "Arrastre real en lienzo mueve la rueda (%d,%d -> %d,%d)" % [ox0, oy0, ox1, oy1])

	# Redimensionar con tirador inferior derecho (HANDLE_BR = 8)
	var h_br: Rect2 = ed.get_overlay_handle_rect(8)
	var h_br_gl: Vector2 = ed.get_canvas_overlay().get_global_transform() * (h_br.position + h_br.size * 0.5)
	await _drag(db, h_br_gl, h_br_gl + Vector2(-40, -40))
	var rad_resized: Dictionary = dm.get_ui_element("game_menu_main", rad_id)
	var w1 := int(rad_resized.get("width", 0))
	_check(w1 < w0 - 20, "Arrastrar tirador BR redimensiona la rueda (%d -> %d)" % [w0, w1])

	# Doble clic real en botón hijo slot_index == 2 para abrir ventana flotante (8 pestañas) y cambiar forma a 'rectangle'
	var kid2_id := kid_ids[2]
	for k in kid_ids:
		var kd: Dictionary = dm.get_ui_element("game_menu_main", k)
		if int(kd.get("slot_index", -1)) == 2 or k.ends_with("_slot_2") or k.ends_with("_3"):
			if int(kd.get("slot_index", -1)) == 2:
				kid2_id = k
				break
	var kid2_card: Control = ed.get_design_card_control(kid2_id)
	await _click_at(db, kid2_card.get_global_rect().position + Vector2(50, kid2_card.size.y * 0.5), true)
	var sfw = ed.open_floating_element_window(kid2_id)
	await _step_frames(2)
	var sfh: Dictionary = sfw.get_ui_handles()
	var s_tabs: TabContainer = sfh.get("tabs", null) as TabContainer
	_check(sfw != null and s_tabs != null and s_tabs.get_tab_count() == 8, "Ventana flotante de botón hijo tiene las 8 pestañas completas")
	_select_meta(sfh["rad_slot_shape_opt"], "rectangle")
	await _step_frames(3)

	var editor_rc = ed.get_canvas_design_layer().find_child("ElemRadialCanvas", true, false)
	_check(editor_rc != null and str(editor_rc.get_radial_slot_visual_config(2).get("shape", "")) == "rectangle", "Lienzo del editor refleja ranura 2 en forma 'rectangle'")
	db.get_texture().get_image().save_png(cap_dir.path_join("p2_editor_radial.png"))

	# Comprobar en el juego real (ClockGearsGameMenu)
	ed.close_all_floating_element_windows()
	db.hide()
	await _step_frames(2)

	var gm = ClassDB.instantiate("ClockGearsGameMenu")
	root.add_child(gm)
	gm.open_menu_by_id("game_menu_main", false, false, true)
	await _step_frames(4)
	var gm_layer = gm.get_custom_ui_layer()
	var gm_rc = gm_layer.find_child("ElemRadialCanvas", true, false)
	_check(gm_rc != null, "El juego real renderiza la rueda radial Elemento")
	if gm_rc != null:
		var gs2: Dictionary = gm_rc.get_radial_slot_visual_config(2)
		var sel_info: Dictionary = gm_rc.get_selected_slot_info()
		_check(str(gs2.get("shape", "")) == "rectangle", "Juego real coincide con lienzo: ranura 2 es 'rectangle'")
		_check(str(sel_info.get("title", "")).length() > 4, "Título en rueda NO se trunca a 4 caracteres: '%s'" % str(sel_info.get("title", "")))
	root.get_texture().get_image().save_png(cap_dir.path_join("p2_juego_radial.png"))
	gm.queue_free()
	await _step_frames(2)

	# 3) Inventario con rueda radial + datos de prueba (8 y 0 vacías numeradas)
	db.show()
	db.select_category_by_key("menu_inventory")
	await _step_frames(3)
	dm.apply_menu_template("game_menu_items", "radial_ring", false, false)
	dm.set_editor_test_data_count(8)
	ed.refresh_from_data_manager()
	await _step_frames(4)

	var inv_rc = ed.get_canvas_design_layer().find_child("ElemRadialCanvas", true, false)
	_check(inv_rc != null and inv_rc.get_slot_count() == 8, "En Inventario con Datos de prueba=8 la rueda muestra 8 ranuras de objetos (%d)" % (inv_rc.get_slot_count() if inv_rc != null else -1))
	if inv_rc != null:
		_check(not str(inv_rc.get_radial_slot_visual_config(0).get("title", "")).is_empty(), "Primera ranura muestra nombre real de objeto de prueba: '%s'" % str(inv_rc.get_radial_slot_visual_config(0).get("title", "")))
	db.get_texture().get_image().save_png(cap_dir.path_join("p2_inventario_radial_datos.png"))

	# Probar 0 datos -> ranuras vacías numeradas
	dm.set_editor_test_data_count(0)
	ed.refresh_from_data_manager()
	await _step_frames(3)
	var inv_rc0 = ed.get_canvas_design_layer().find_child("ElemRadialCanvas", true, false)
	_check(inv_rc0 != null and inv_rc0.get_slot_count() == 8 and str(inv_rc0.get_radial_slot_visual_config(0).get("title", "")).begins_with("Vac"), "Con 0 datos de prueba la rueda muestra ranuras vacías numeradas (%s)" % (str(inv_rc0.get_radial_slot_visual_config(0).get("title", "")) if inv_rc0 != null else ""))

	# Restaurar estado original
	dm.set_editor_test_data_count(-1)
	if had_gm:
		dm.set_ui_screen_design("game_menu_main", orig_gm, false)
	else:
		dm.reset_ui_screen_design_to_default("game_menu_main", false)
	if had_inv:
		dm.set_ui_screen_design("game_menu_items", orig_inv, false)
	else:
		dm.reset_ui_screen_design_to_default("game_menu_items", false)

	db.hide()
	db.queue_free()
	await _step_frames(2)
	_finish(0 if fail_count == 0 else 1)
