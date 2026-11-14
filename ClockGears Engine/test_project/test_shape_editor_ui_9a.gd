extends SceneTree

var passed := 0
var failed := 0

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

func _find_tab_index(tc: TabContainer, title: String) -> int:
	if tc == null:
		return -1
	for i in range(tc.get_tab_count()):
		if tc.get_tab_title(i).find(title) != -1:
			return i
	return -1

func _find_slider_after_label(root: Node, label_needle: String) -> HSlider:
	for n in root.find_children("*", "Label", true, false):
		var l := n as Label
		if l.text.find(label_needle) != -1:
			var top := l.get_parent()
			if top != null and top.get_parent() != null:
				var pair := top.get_parent()
				for c in pair.get_children():
					if c is HSlider:
						return c as HSlider
	return null

func _find_spin_in_label_row(root: Node, label_needle: String, exact: bool = false) -> SpinBox:
	for n in root.find_children("*", "Label", true, false):
		var l := n as Label
		var matched := (l.text == label_needle) if exact else (l.text.find(label_needle) != -1)
		if matched:
			var top := l.get_parent()
			if top != null:
				var idx := l.get_index()
				for i in range(idx + 1, top.get_child_count()):
					if top.get_child(i) is SpinBox:
						return top.get_child(i) as SpinBox
	return null

func _initialize() -> void:
	call_deferred("_run_tests")

func _run_tests() -> void:
	var backup := _backup_data_dir("res://clockgears_data")
	var tmp_dir := "res://tmp_import_src/shape_ui_9a_test"
	var actor_png := _make_png(tmp_dir + "/actor_atk.png", 64, 32, Color(0.2, 0.7, 0.9, 1.0))
	var sword_png := _make_png(tmp_dir + "/sword_atk.png", 64, 32, Color(0.9, 0.8, 0.2, 1.0))

	var dm = ClockGearsDataManager
	dm.init_empty()
	var sys: Dictionary = dm.get_system_config()
	sys["combat_mode"] = 1 # ARPG
	sys["movement_directions"] = 4
	sys["mannequin_size"] = 32
	sys["party"] = [1]
	dm.set_system_config(sys)

	dm.set_records("actors", [{
		"id": 1,
		"name": "HeroeFormas",
		"hp": 100, "mp": 20, "atk": 12, "def": 8, "mat": 5, "mdf": 6, "agi": 10, "luk": 7,
		"sprite_data": {
			"animations": {
				"atacar_abajo": {
					"spritesheet_path": actor_png,
					"cols": 2, "rows": 1, "fps": 8.0, "loop": false,
					"frames_data": [
						{"sheet_path": actor_png, "region": Rect2i(0, 0, 32, 32), "duration": 0.125},
						{"sheet_path": actor_png, "region": Rect2i(32, 0, 32, 32), "duration": 0.125}
					]
				},
				"atacar_derecha": {
					"spritesheet_path": actor_png,
					"cols": 2, "rows": 1, "fps": 8.0, "loop": false,
					"frames_data": [
						{"sheet_path": actor_png, "region": Rect2i(0, 0, 32, 32), "duration": 0.125},
						{"sheet_path": actor_png, "region": Rect2i(32, 0, 32, 32), "duration": 0.125}
					]
				},
				"mover_abajo": {
					"spritesheet_path": actor_png,
					"cols": 2, "rows": 1, "fps": 8.0, "loop": true,
					"frames_data": [
						{"sheet_path": actor_png, "region": Rect2i(0, 0, 32, 32), "duration": 0.125}
					]
				}
			}
		}
	}])
	dm.set_records("arpg_weapons", [{
		"id": 1,
		"name": "EspadaPrueba",
		"draw_mode": "completo",
		"layer_order": 1,
		"sprite_data": {
			"animations": {
				"atacar_abajo": {
					"spritesheet_path": sword_png,
					"cols": 2, "rows": 1, "fps": 8.0, "loop": false,
					"frames_data": [
						{"sheet_path": sword_png, "region": Rect2i(0, 0, 32, 32), "duration": 0.125},
						{"sheet_path": sword_png, "region": Rect2i(32, 0, 32, 32), "duration": 0.125}
					]
				}
			}
		}
	}])
	dm.set_records("enemies", [{"id": 1, "name": "SlimeTurnos"}])
	dm.save_all("res://clockgears_data")

	# ---------------------------------------------------------
	# 1. Abrir Base de Datos y comprobar submodos Frames / Formas
	# ---------------------------------------------------------
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await process_frame
	db.open_database()
	db.select_category_by_key("actors")
	await process_frame

	var main_tabs: TabContainer = null
	for n in db.find_children("*", "TabContainer", true, false):
		main_tabs = n as TabContainer
		break
	var anim_tab_idx := _find_tab_index(main_tabs, "Animaciones")
	if main_tabs != null and anim_tab_idx >= 0:
		main_tabs.current_tab = anim_tab_idx
	await process_frame

	var btn_frames := _find_button_by_text(db, "Frames", true)
	var btn_formas := _find_button_by_text(db, "Formas", true)
	check(btn_frames != null and btn_formas != null and btn_formas.is_visible_in_tree(), "Actor muestra toggle de ranura 'Frames' y 'Formas'")

	# Turn-based enemy stays in Frames mode only
	db.select_category_by_key("enemies")
	await process_frame
	check(btn_formas != null and not btn_formas.is_visible_in_tree(), "Enemigo por turnos NO muestra modo Formas (mantiene Frames intacto)")

	# Back to Actors -> atacar_abajo -> switch to Formas
	db.select_category_by_key("actors")
	await process_frame
	if main_tabs != null and anim_tab_idx >= 0:
		main_tabs.current_tab = anim_tab_idx
	db.select_anim_slot("atacar_abajo")
	await process_frame
	btn_formas.emit_signal("pressed")
	await process_frame
	check(db.get_anim_submode() == 1, "Cambio a modo Formas en atacar_abajo activo")

	var se: ClockGearsShapeEditor = db.get_shape_editor()
	check(se != null and se.visible, "ClockGearsShapeEditor visible en modo Formas")

	# Zoom buttons x1..x8 and wheel NEVER zooms
	var btn_x6 := _find_button_by_text(se, "x6", true)
	check(btn_x6 != null, "Boton de zoom entero x6 presente")
	if btn_x6 != null:
		btn_x6.emit_signal("pressed")
	check(se.get_zoom() == 6, "Zoom cambia a x6 al pulsar boton")

	var canvas: Control = se.get_shape_canvas()
	check(canvas != null, "Lienzo de formas accesible")
	var ev_wheel := InputEventMouseButton.new()
	ev_wheel.button_index = MOUSE_BUTTON_WHEEL_UP
	ev_wheel.pressed = true
	ev_wheel.position = Vector2(100, 100)
	canvas.handle_canvas_event(ev_wheel)
	check(se.get_zoom() == 6, "La rueda del raton NO hace zoom en el editor de formas")

	# Grid & Snap toggles
	var btn_grid := _find_button_by_text(se, "Rejilla 32x32", true)
	var btn_snap := _find_button_by_text(se, "Imán", true)
	check(btn_grid != null and btn_snap != null, "Botones Rejilla 32x32 e Imán presentes")
	btn_snap.emit_signal("toggled", true)
	check(se.is_snap_enabled(), "Imán activable")
	btn_snap.emit_signal("toggled", false)

	# ---------------------------------------------------------
	# 2. Crear hitbox circular, rectangular y hurtbox poligonal
	# ---------------------------------------------------------
	se.add_hitbox_shape({"type": "circle", "offset_x": 0.0, "offset_y": -10.0, "radius": 12.0})
	se.add_hitbox_shape({"type": "rect", "offset_x": 4.0, "offset_y": 16.0, "size_x": 20.0, "size_y": 12.0})

	# Poligonal hurtbox via mouse clicks on canvas after enabling per-frame override
	var chk_ov := _find_checkbox_by_text(se, "Anular hurtbox")
	check(chk_ov != null, "Checkbox 'Anular hurtbox en este frame' presente")
	chk_ov.button_pressed = true
	chk_ov.emit_signal("toggled", true)
	await process_frame

	se.set_edit_target(1) # Hurtbox
	se.set_tool(4) # TOOL_POLYGON
	var origin: Vector2 = canvas.call("get_canvas_origin")
	var pts := [Vector2(-10, -12), Vector2(10, -12), Vector2(12, 12), Vector2(-12, 12)]
	for p in pts:
		var ev_p := InputEventMouseButton.new()
		ev_p.button_index = MOUSE_BUTTON_LEFT
		ev_p.pressed = true
		ev_p.position = origin + p * float(se.get_zoom())
		canvas.handle_canvas_event(ev_p)
		var ev_r := InputEventMouseButton.new()
		ev_r.button_index = MOUSE_BUTTON_LEFT
		ev_r.pressed = false
		ev_r.position = ev_p.position
		canvas.handle_canvas_event(ev_r)
	var poly_ok := se.finish_polygon()
	check(poly_ok, "Hurtbox poligonal creada cerrando vertices en lienzo")

	# Drag/move selected shape using real mouse events
	se.select_shape(1, 0) # first hitbox (circle at 0, -10)
	var drag_from: Vector2 = origin + Vector2(0.0, -10.0) * float(se.get_zoom())
	var drag_to: Vector2 = origin + Vector2(6.0, -10.0) * float(se.get_zoom())
	var md := InputEventMouseButton.new()
	md.button_index = MOUSE_BUTTON_LEFT
	md.pressed = true
	md.position = drag_from
	canvas.handle_canvas_event(md)
	var mm := InputEventMouseMotion.new()
	mm.position = drag_to
	mm.button_mask = MOUSE_BUTTON_MASK_LEFT
	canvas.handle_canvas_event(mm)
	var mu := InputEventMouseButton.new()
	mu.button_index = MOUSE_BUTTON_LEFT
	mu.pressed = false
	mu.position = drag_to
	canvas.handle_canvas_event(mu)

	var hb_after_drag := se.get_hitbox_data()
	var shapes_arr: Array = hb_after_drag.get("shapes", [])
	check(shapes_arr.size() == 2 and is_equal_approx(float(shapes_arr[0].get("offset_x", 0.0)), 6.0),
		"Arrastrar hitbox circular en el lienzo mueve su offset_x a 6")

	# Resize circle radius via east handle drag (circle now at (6, -10), radius 12 -> handle at (18, -10) -> drag to (24, -10) => radius ~18)
	var h_from: Vector2 = origin + Vector2(18.0, -10.0) * float(se.get_zoom())
	var h_to: Vector2 = origin + Vector2(24.0, -10.0) * float(se.get_zoom())
	md.position = h_from
	canvas.handle_canvas_event(md)
	mm.position = h_to
	canvas.handle_canvas_event(mm)
	mu.position = h_to
	canvas.handle_canvas_event(mu)
	var r_after: float = float(se.get_hitbox_data().get("shapes", [])[0].get("radius", 0.0))
	check(is_equal_approx(r_after, 18.0), "Redimensionar tirador del circulo cambia su radio a 18")

	# Sliders / Spinboxes synced: Damage %, Knockback, Hitstop, Hurtbox mult
	var sl_dmg := _find_slider_after_label(se, "Daño (%)")
	var sl_kb := _find_slider_after_label(se, "Retroceso")
	var sl_hs := _find_slider_after_label(se, "Hitstop")
	var sl_hu := _find_slider_after_label(se, "Mult. daño recibido")
	check(sl_dmg != null and sl_kb != null and sl_hs != null and sl_hu != null, "Sliders sincronizados presentes")
	sl_dmg.value = 175.0
	sl_kb.value = 120.0
	sl_hs.value = 65.0
	sl_hu.value = 50.0

	# Weapon anchor
	var chk_anc := _find_checkbox_by_text(se, "Activar ancla de arma")
	check(chk_anc != null, "Checkbox 'Activar ancla de arma' presente")
	chk_anc.button_pressed = true
	chk_anc.emit_signal("toggled", true)
	var anc_x_spin := _find_spin_in_label_row(se, "X:", true)
	var anc_y_spin := _find_spin_in_label_row(se, "Y:", true)
	var rot_sl := _find_slider_after_label(se, "Rotación arma")
	if anc_x_spin != null: anc_x_spin.value = 12.0
	if anc_y_spin != null: anc_y_spin.value = -6.0
	if rot_sl != null: rot_sl.value = 45.0

	# Copy to next frame
	var copied_next := db.copy_shapes_to_next_frame()
	check(copied_next, "Copiar al siguiente frame devuelve true")
	db.select_shape_frame(1)
	await process_frame
	var f2_hb := se.get_hitbox_data()
	var f2_hurt := se.get_hurtbox_data()
	var f2_anc := se.get_weapon_anchor_data()
	check(Array(f2_hb.get("shapes", [])).size() == 2 and is_equal_approx(float(f2_hb.get("damage_mult_pct", 0.0)), 175.0),
		"Frame 2 hereda las 2 hitboxes y daño 175%")
	check(se.has_hurtbox_override_enabled() and String(f2_hurt.get("shape", {}).get("type", "")) == "polygon",
		"Frame 2 hereda hurtbox poligonal anulada")
	check(bool(f2_anc.get("enabled", false)) and is_equal_approx(float(f2_anc.get("rotation", 0.0)), 45.0),
		"Frame 2 hereda ancla de arma rotada 45°")

	# Copy from down direction to right direction (rotates/mirrors without touching sprite)
	db.select_anim_slot("atacar_derecha")
	await process_frame
	var sprite_before: String = str(dm.get_records("actors")[0]["sprite_data"]["animations"]["atacar_derecha"]["frames_data"][0]["sheet"])
	var copied_dir := db.copy_shapes_from_direction("abajo", "rotate")
	check(copied_dir, "Copiar desde direccion 'abajo' hacia 'atacar_derecha' funciona")
	var d_hb := se.get_hitbox_data()
	var d_shapes: Array = d_hb.get("shapes", [])
	check(d_shapes.size() == 2, "atacar_derecha recibe las 2 hitboxes rotadas")

	# OK / Save & close + Reopen DB
	var ok_btn := db.get_ok_button()
	ok_btn.emit_signal("pressed")
	await process_frame
	db.queue_free()
	await process_frame

	var db2 := ClockGearsDatabaseDialog.new()
	root.add_child(db2)
	await process_frame
	db2.open_database()
	db2.select_category_by_key("actors")
	await process_frame
	db2.select_anim_slot("atacar_abajo")
	await process_frame
	db2.set_anim_submode(1)
	db2.select_shape_frame(0)
	await process_frame
	var se2: ClockGearsShapeEditor = db2.get_shape_editor()
	var persisted_hb := se2.get_hitbox_data()
	var persisted_hurt := se2.get_hurtbox_data()
	var persisted_anc := se2.get_weapon_anchor_data()
	check(Array(persisted_hb.get("shapes", [])).size() == 2 and is_equal_approx(float(persisted_hb.get("knockback", 0.0)), 120.0) and int(persisted_hb.get("hitstop_ms", 0)) == 65,
		"Tras guardar/cerrar/reabrir DB, hitboxes + knockback + hitstop persisten")
	check(se2.has_hurtbox_override_enabled() and String(persisted_hurt.get("shape", {}).get("type", "")) == "polygon" and is_equal_approx(float(persisted_hurt.get("damage_mult_pct", 0.0)), 50.0),
		"Tras reabrir DB, hurtbox poligonal anulada y mult 50% persisten")
	check(bool(persisted_anc.get("enabled", false)) and is_equal_approx(float(persisted_anc.get("x", 0.0)), 12.0) and is_equal_approx(float(persisted_anc.get("y", 0.0)), -6.0) and is_equal_approx(float(persisted_anc.get("rotation", 0.0)), 45.0),
		"Tras reabrir DB, ancla de arma X=12, Y=-6, rot=45 persiste")
	var sprite_after: String = str(dm.get_records("actors")[0]["sprite_data"]["animations"]["atacar_derecha"]["frames_data"][0]["sheet"])
	check(sprite_before == sprite_after, "Copiar direccion nunca altera el sprite original")

	# ---------------------------------------------------------
	# 3. Sistema -> Tamaño del Maniquí + Maniquí en Equipo ARPG + No exportado
	# ---------------------------------------------------------
	db2.select_category_by_key("system")
	await process_frame
	db2.set_system_mannequin_size(64)
	check(db2.get_system_mannequin_size() == 64, "Tamaño del maniquí en Sistema actualizado a 64")

	db2.select_category_by_key("arpg_weapons")
	await process_frame
	db2.select_anim_slot("atacar_abajo")
	db2.set_anim_submode(1)
	await process_frame
	check(se2.is_mannequin_shown() and se2.get_mannequin_size() == 64,
		"Pieza de equipo ARPG en Formas muestra maniquí con tamaño 64x64 de Sistema")

	var exp_sys := dm.get_exported_system_config()
	check(not exp_sys.has("mannequin_size"), "El maniquí NUNCA se exporta en get_exported_system_config()")

	# Check Frames mode still intact
	db2.set_anim_submode(0)
	await process_frame
	var frames_add_btn := _find_button_by_text(db2, "Añadir frames desde spritesheet")
	check(frames_add_btn != null and frames_add_btn.visible, "Modo Frames sigue intacto y visible")

	# ---------------------------------------------------------
	# 4. Regla 16:9 en 1366x768 y 1920x1080 (sin scroll horizontal en Formas)
	# ---------------------------------------------------------
	db2.set_anim_submode(1)
	for res_sz in [Vector2i(1366, 768), Vector2i(1920, 1080)]:
		db2.size = res_sz
		await process_frame
		var fit_info: Dictionary = db2.check_layout_fits_window(res_sz)
		check(bool(fit_info.get("fits_horizontally", false)), "DB encaja horizontalmente a %dx%d" % [res_sz.x, res_sz.y])
		check(se2.size.x > 400 and se2.size.y > 240, "ShapeEditor tiene tamaño valido en %dx%d" % [res_sz.x, res_sz.y])
		var anim_sc: ScrollContainer = se2.get_parent().get_parent().get_parent().get_parent() as ScrollContainer
		if anim_sc != null:
			check(anim_sc.horizontal_scroll_mode == ScrollContainer.SCROLL_MODE_DISABLED, "Pestaña Animaciones/Formas sin scroll horizontal a %dx%d" % [res_sz.x, res_sz.y])

	db2.queue_free()
	await process_frame

	# Limpieza de respaldo de disco y archivos temporales
	_restore_data_dir("res://clockgears_data", backup)
	dm.load_all("res://clockgears_data")
	var abs_tmp := ProjectSettings.globalize_path(tmp_dir)
	for fn in ["actor_atk.png", "sword_atk.png"]:
		if FileAccess.file_exists(abs_tmp.path_join(fn)):
			DirAccess.remove_absolute(abs_tmp.path_join(fn))
	DirAccess.remove_absolute(abs_tmp)

	print("=== TEST SHAPE EDITOR 9A SUMMARY: passed=%d failed=%d ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
