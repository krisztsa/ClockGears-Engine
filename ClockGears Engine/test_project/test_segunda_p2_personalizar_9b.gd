extends SceneTree

# Test Punto 2 (Segunda Pasada UI 9B): Personalizar sin cambiar ni duplicar nada
# - Imagen por defecto vs recien personalizada sin mover elementos -> identica pixel a pixel
# - En Equipo: mover SOLO 'Volver' (arrastre real en lienzo) -> ningun otro panel cambia ni desaparece
# - Verificacion en F5 (ClockGearsGameMenu) en Equipo personalizado: todos los paneles visibles y en posicion
# - Probar Deshacer y 'Restaurar plantilla original'

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

func _drag(win: Window, overlay: Control, from_pt: Vector2, to_pt: Vector2) -> void:
	var xf: Transform2D = overlay.get_global_transform_with_canvas()
	var g0 := xf * from_pt
	var g1 := xf * to_pt
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = g0
	press.global_position = g0
	win.push_input(press)
	var mot := InputEventMouseMotion.new()
	mot.button_mask = MOUSE_BUTTON_MASK_LEFT
	mot.position = g1
	mot.global_position = g1
	mot.relative = g1 - g0
	win.push_input(mot)
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = g1
	rel.global_position = g1
	win.push_input(rel)

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 2 (SEGUNDA PASADA UI 9B): PERSONALIZAR SIN CAMBIAR NI DUPLICAR ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = ed.get_ui_handles()
	var stage: Control = h.get("canvas_stage", null) as Control

	var screens := [
		["title", "main_menu"],
		["game_menu_main", "game_menu"],
		["game_menu_items", "menu_inventory"],
		["game_menu_equip", "menu_equip"],
		["game_menu_status", "menu_status"],
		["game_menu_save", "menu_saves"],
		["game_menu_options", "menu_options"],
		["credits", "credits"],
		["game_over", "game_over"],
		["messages_battle", "messages_battle"]
	]

	for pair in screens:
		var sid: String = pair[0]
		var ckey: String = pair[1]
		dm.reset_ui_screen_design_to_default(sid, false)
		db.select_category_by_key(ckey)
		await _step_frames(3)
		var sr := stage.get_global_rect()
		var r_i := Rect2i(Vector2i(sr.position), Vector2i(sr.size))
		var img_def: Image = db.get_texture().get_image().get_region(r_i)

		var tpl: Dictionary = dm.get_default_ui_screen_template(sid).duplicate(true)
		tpl["custom_enabled"] = true
		dm.set_ui_screen_design(sid, tpl, false)
		db.select_category_by_key(ckey)
		await _step_frames(3)
		var img_cus: Image = db.get_texture().get_image().get_region(r_i)

		var m: Dictionary = img_def.compute_image_metrics(img_cus, false)
		var max_d := float(m.get("max", 999.0))
		var rmse := float(m.get("root_mean_squared", 999.0))
		_check(max_d <= 0.01 and rmse <= 0.01, "[%s] Por defecto vs recien personalizada identica pixel a pixel (max=%.3f, rmse=%.4f)" % [sid, max_d, rmse])
		dm.reset_ui_screen_design_to_default(sid, false)

	# Prueba especifica en Equipo: arrastrar SOLO 'Volver' (btn_back)
	db.select_category_by_key("menu_equip")
	await _step_frames(3)
	_check(not dm.has_custom_ui_design("game_menu_equip"), "Equipo parte en Por defecto")
	var def_elems: Array = Array(dm.get_default_ui_screen_template("game_menu_equip").get("elements", []))
	var back_before := Vector2.ZERO
	var other_before := {}
	for el in def_elems:
		if el is Dictionary:
			var eid: String = str(el.get("id", ""))
			var rect_vec := Vector4(float(el.get("offset_x", 0)), float(el.get("offset_y", 0)), float(el.get("width", 0)), float(el.get("height", 0)))
			if eid == "btn_back":
				back_before = Vector2(rect_vec.x, rect_vec.y)
			else:
				other_before[eid] = rect_vec

	var overlay: Control = h.get("canvas_overlay", null) as Control
	var pres: Vector2i = ed.get_preview_resolution()
	var sc: float = (float(pres.x) / 960.0) * float(ed.get_canvas_zoom())
	# Centro de btn_back (814, 30, 110, 34)
	var back_c := Vector2((back_before.x + 55.0) * sc, (back_before.y + 17.0) * sc)
	_drag(db, overlay, back_c, back_c + Vector2(0.0, 28.0) * sc)
	await _step_frames(3)

	_check(dm.has_custom_ui_design("game_menu_equip"), "Equipo activa diseno personalizado al arrastrar 'Volver'")
	var after_elems: Array = dm.get_ui_screen_elements("game_menu_equip")
	_check(after_elems.size() == def_elems.size(), "Equipo conserva todos sus %d elementos tras mover 'Volver'" % def_elems.size())
	var back_moved := false
	var unchanged_all := true
	for el in after_elems:
		if el is Dictionary:
			var eid: String = str(el.get("id", ""))
			var cur := Vector4(float(el.get("offset_x", 0)), float(el.get("offset_y", 0)), float(el.get("width", 0)), float(el.get("height", 0)))
			if eid == "btn_back":
				back_moved = absf(cur.y - back_before.y) >= 16.0
			elif other_before.has(eid):
				var prev: Vector4 = other_before[eid]
				if cur != prev:
					unchanged_all = false
	_check(back_moved, "Solo 'Volver' se desplazo en Equipo")
	_check(unchanged_all, "Todos los demas paneles y botones de Equipo mantienen posicion y tamano exactos")

	# F5 Runtime check en ClockGearsGameMenu para Equipo personalizado
	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	_check(gm.open_menu_by_id("game_menu_equip", false, false, false), "F5 abre Equipo personalizado correctamente")
	await _step_frames(3)
	var cl := gm.get_custom_ui_layer()
	_check(cl != null and cl.is_visible_in_tree(), "F5 renderiza capa personalizada visible")
	for wname in ["EquipSlotsWrap", "EquipCandidatesWrap", "EquipItemPreviewBox", "EquipDescBox", "EquipParamsWrap"]:
		var wn := gm.find_child(wname, true, false) as Control
		_check(wn != null and wn.is_visible_in_tree() and wn.get_rect().size.x > 100.0 and wn.get_rect().size.y > 50.0,
			"F5 panel Equipo '%s' visible y con tamano intacto" % wname)
	var has_huecos := gm.find_child("EquipSlotsWrap", true, false) != null
	_check(has_huecos, "F5 conserva columnas de Huecos/Inventario/Parametros completas")
	gm.close_menu()
	gm.queue_free()
	await _step_frames(2)

	# Deshacer y Restaurar plantilla original
	var undo_btn: Button = h.get("undo_btn", null) as Button
	var reset_btn: Button = h.get("reset_btn", null) as Button
	_check(undo_btn != null and reset_btn != null, "Botones Deshacer y Restaurar presentes")
	if undo_btn != null:
		undo_btn.emit_signal("pressed")
		await _step_frames(2)
		var back_after_undo := -1.0
		for el in dm.get_ui_screen_elements("game_menu_equip"):
			if el is Dictionary and str(el.get("id", "")) == "btn_back":
				back_after_undo = float(el.get("offset_y", -1))
		_check(not dm.has_custom_ui_design("game_menu_equip") or absf(back_after_undo - back_before.y) <= 1.0,
			"Deshacer revierte el arrastre de 'Volver'")

	dm.reset_ui_screen_design_to_default("game_menu_equip", false)
	_check(not dm.has_custom_ui_design("game_menu_equip"), "Restaurar plantilla original vuelve a Por defecto")

	db.queue_free()
	await _step_frames(2)
	_finish(0 if fail_count == 0 else 1)
