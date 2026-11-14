extends SceneTree

var passed: int = 0
var failed: int = 0
var _done: bool = false
var _snap: Dictionary = {}
var _data_dir: String = "res://clockgears_data"

func assert_true(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
	else:
		failed += 1
		printerr("[FAIL] ", msg)

func assert_eq(a, b, msg: String) -> void:
	if a == b:
		passed += 1
	else:
		failed += 1
		printerr("[FAIL] ", msg, " (got=", a, ", expected=", b, ")")

func assert_near(a: float, b: float, eps: float, msg: String) -> void:
	if absf(a - b) <= eps:
		passed += 1
	else:
		failed += 1
		printerr("[FAIL] ", msg, " (got=", a, ", expected=", b, ", diff=", absf(a - b), ")")

func backup_json_dir(dir_res: String) -> Dictionary:
	var snap := {}
	var d := DirAccess.open(dir_res)
	if d == null:
		return snap
	d.list_dir_begin()
	var fn := d.get_next()
	while fn != "":
		if not d.current_is_dir() and fn.ends_with(".json"):
			var fa := FileAccess.open(dir_res.path_join(fn), FileAccess.READ)
			if fa:
				snap[fn] = fa.get_as_text()
				fa.close()
		fn = d.get_next()
	d.list_dir_end()
	return snap

func restore_json_dir(dir_res: String, snap: Dictionary) -> void:
	if snap.is_empty():
		return
	var d := DirAccess.open(dir_res)
	if d:
		d.list_dir_begin()
		var fn := d.get_next()
		while fn != "":
			if not d.current_is_dir() and fn.ends_with(".json") and not snap.has(fn):
				d.remove(fn)
			fn = d.get_next()
		d.list_dir_end()
	for fn in snap.keys():
		var fa := FileAccess.open(dir_res.path_join(fn), FileAccess.WRITE)
		if fa:
			fa.store_string(snap[fn])
			fa.close()

func make_test_png(path: String, w: int, h: int, col: Color) -> void:
	var img := Image.create_empty(w, h, false, Image.FORMAT_RGBA8)
	img.fill(col)
	var abs_p := ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs_p.get_base_dir())
	img.save_png(abs_p)

func cleanup_temp_files() -> void:
	restore_json_dir(_data_dir, _snap)
	var abs_tmp := ProjectSettings.globalize_path("res://tmp_troop_hp_free_test")
	if DirAccess.dir_exists_absolute(abs_tmp):
		for f in DirAccess.get_files_at("res://tmp_troop_hp_free_test"):
			DirAccess.remove_absolute(abs_tmp.path_join(f))
		DirAccess.remove_absolute(abs_tmp)

func finish_and_quit() -> void:
	if _done:
		return
	_done = true
	cleanup_temp_files()
	print("[TEST_TROOP_HP_FREE_FIXED] passed=%d failed=%d" % [passed, failed])
	quit(0 if failed == 0 else 1)

func _watchdog_loop() -> void:
	var t0: int = Time.get_ticks_msec()
	while not _done:
		if Time.get_ticks_msec() - t0 >= 45000:
			failed += 1
			printerr("[FAIL] Watchdog 45s timeout!")
			finish_and_quit()
			return
		await process_frame

func find_controls_of_class(node: Node, cls_name: String) -> Array:
	var res := []
	for ch in node.get_children():
		if ch.is_class(cls_name):
			res.append(ch)
		res.append_array(find_controls_of_class(ch, cls_name))
	return res

func find_popup_with_item_text(root_node: Node, needle: String) -> PopupMenu:
	for p in find_controls_of_class(root_node, "PopupMenu"):
		var pm := p as PopupMenu
		for i in range(pm.get_item_count()):
			if needle in pm.get_item_text(i):
				return pm
	return null

func find_popup_item_idx(pm: PopupMenu, needle: String) -> int:
	if pm == null:
		return -1
	for i in range(pm.get_item_count()):
		if pm.get_item_text(i) == needle or needle in pm.get_item_text(i):
			return i
	return -1

func find_troop_canvas(db: Node) -> Control:
	for c in find_controls_of_class(db, "Control"):
		var ctrl := c as Control
		if ctrl and ctrl.get_class() == "Control" and ctrl.is_visible_in_tree() and ctrl.get_child_count() >= 3 and ctrl.size.x >= 300.0:
			return ctrl
	return null

func open_db_troops(troop_index: int) -> Dictionary:
	var db = ClassDB.instantiate("ClockGearsDatabaseDialog")
	root.add_child(db)
	db.size = Vector2i(1364, 700)
	db.open_database()
	await process_frame
	db.select_category_by_key("troops")
	await process_frame
	var rec_list := find_controls_of_class(db, "ItemList")[0] as ItemList
	if rec_list and rec_list.get_item_count() > troop_index:
		rec_list.select(troop_index)
		rec_list.emit_signal("item_selected", troop_index)
	await process_frame
	await process_frame
	return {"db": db, "canvas": find_troop_canvas(db)}

func right_click_member(canvas: Control, spr_rect: Rect2) -> void:
	var rmb := InputEventMouseButton.new()
	rmb.button_index = MOUSE_BUTTON_RIGHT
	rmb.pressed = true
	rmb.position = spr_rect.get_center()
	canvas.gui_input.emit(rmb)

func _init() -> void:
	call_deferred("_watchdog_loop")
	call_deferred("_run_tests")

func _run_tests() -> void:
	_snap = backup_json_dir(_data_dir)
	var tmp_png := "res://tmp_troop_hp_free_test/enemy_64.png"
	make_test_png(tmp_png, 64, 64, Color(0.85, 0.25, 0.2, 1.0))

	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.init_empty()
	dm.set_records("actors", [{"id": 1, "name": "Heroe", "hp": 100, "mp": 20, "atk": 15, "def": 10}])
	dm.set_records("enemies", [{"id": 1, "name": "Golem", "level": 5, "hp": 200, "mp": 0, "atk": 18, "def": 12, "graphic_path": tmp_png}])
	var sys_cfg: Dictionary = dm.get_system_config()
	sys_cfg["party"] = [1]
	sys_cfg["combat_mode"] = 0
	dm.set_system_config(sys_cfg)

	# Tropa 1: legado en modo libre (offset antiguo) + modo desconocido
	var legacy_troop := {
		"id": 1,
		"name": "TropaLegacyLibre",
		"members": [
			{"enemy_id": 1, "x": 380, "y": 250, "escala": 1, "hp_bar_mode": "libre", "hp_bar_offset_x": 120, "hp_bar_offset_y": -110},
			{"enemy_id": 1, "x": 620, "y": 250, "escala": 1, "hp_bar_mode": "modo_desconocido"}
		]
	}
	# Tropa 2: para probar transiciones, mover enemigo, escalar, restablecer
	var work_troop := {
		"id": 2,
		"name": "TropaTrabajo",
		"members": [
			{"enemy_id": 1, "x": 300, "y": 260, "escala": 1, "hp_bar_mode": "abajo"},
			{"enemy_id": 1, "x": 580, "y": 260, "escala": 1, "hp_bar_mode": "arriba"}
		]
	}
	dm.set_records("troops", [legacy_troop, work_troop])
	dm.save_all(_data_dir)
	dm.load_all(_data_dir)
	await process_frame

	# 1. Migracion de tropa antigua
	var troops_loaded: Array = dm.get_records("troops")
	var lm0: Dictionary = troops_loaded[0]["members"][0]
	var lm1: Dictionary = troops_loaded[0]["members"][1]
	assert_eq(str(lm0.get("hp_bar_mode", "")), "libre", "Migrated legacy member keeps 'libre'")
	assert_eq(int(lm0.get("hp_bar_x", -1)), 500, "Migrated legacy offset_x=120 from x=380 -> hp_bar_x=500")
	assert_eq(int(lm0.get("hp_bar_y", -1)), 140, "Migrated legacy offset_y=-110 from y=250 -> hp_bar_y=140")
	assert_true(not lm0.has("hp_bar_offset_x") and not lm0.has("hp_bar_offset_y"), "Legacy offset keys removed (single source of truth)")
	assert_eq(str(lm1.get("hp_bar_mode", "")), "abajo", "Unknown mode falls back to 'abajo' safely")

	# 1b. Comprobar visualmente que la tropa antigua migrada se ve exactamente donde (380+120, 250-110) = (500, 140)
	root.size = Vector2i(960, 540)
	var bs_leg = ClassDB.instantiate("ClockGearsBattleScene")
	root.add_child(bs_leg)
	bs_leg.setup_battle(1, true, false, "", null, true)
	await process_frame
	await process_frame
	var leg_vp: Rect2 = bs_leg.get_battle_field_rect()
	var leg_grp0: Rect2 = bs_leg.get_enemy_hud_group_rect(0)
	assert_near((leg_grp0.get_center().x - leg_vp.position.x) / leg_vp.size.x, 500.0 / 960.0, 0.01, "Migrated legacy libre group X matches pre-migration visual spot (500/960)")
	assert_near((leg_grp0.position.y - leg_vp.position.y) / leg_vp.size.y, 143.0 / 540.0, 0.01, "Migrated legacy libre group Y matches pre-migration visual spot (143/540)")
	bs_leg.finish_battle_immediate()
	bs_leg.queue_free()
	await process_frame

	# 2. En Tropa 2: cambiar miembro 0 (Abajo) y miembro 1 (Arriba) a "Mover libremente" sin saltos
	var ctx2: Dictionary = await open_db_troops(1)
	var db = ctx2["db"]
	var canvas: Control = ctx2["canvas"]
	var vp: Rect2 = db.get_troop_preview_viewport_rect()
	var hp_sub: PopupMenu = find_popup_with_item_text(db, "Mover libremente")
	var scale_sub: PopupMenu = find_popup_with_item_text(db, "x3")
	var libre_idx := find_popup_item_idx(hp_sub, "Mover libremente")

	# M0 Abajo -> Libre
	var l0_before: Dictionary = db.get_troop_preview_enemy_layout(0)
	var rect0_abajo: Rect2 = l0_before["hud_group_rect"]
	right_click_member(canvas, l0_before["sprite_rect"])
	await process_frame
	hp_sub.emit_signal("id_pressed", hp_sub.get_item_id(libre_idx))
	await process_frame
	var l0_after: Dictionary = db.get_troop_preview_enemy_layout(0)
	var rect0_libre: Rect2 = l0_after["hud_group_rect"]
	assert_near(rect0_libre.get_center().x, rect0_abajo.get_center().x, 1.5, "Abajo->Libre starts at same X (no jump)")
	assert_near(rect0_libre.position.y, rect0_abajo.position.y, 1.5, "Abajo->Libre starts at same Y (no jump)")

	# M1 Arriba -> Libre
	var l1_before: Dictionary = db.get_troop_preview_enemy_layout(1)
	var rect1_arriba: Rect2 = l1_before["hud_group_rect"]
	right_click_member(canvas, l1_before["sprite_rect"])
	await process_frame
	hp_sub.emit_signal("id_pressed", hp_sub.get_item_id(libre_idx))
	await process_frame
	var l1_after: Dictionary = db.get_troop_preview_enemy_layout(1)
	var rect1_libre: Rect2 = l1_after["hud_group_rect"]
	assert_near(rect1_libre.get_center().x, rect1_arriba.get_center().x, 1.5, "Arriba->Libre starts at same X (no jump)")
	assert_near(rect1_libre.position.y, rect1_arriba.position.y, 1.5, "Arriba->Libre starts at same Y (no jump)")

	# Regresar M1 a Arriba para comprobar despues que Arriba sigue al enemigo
	var arriba_idx := find_popup_item_idx(hp_sub, "Arriba")
	right_click_member(canvas, l1_after["sprite_rect"])
	await process_frame
	hp_sub.emit_signal("id_pressed", hp_sub.get_item_id(arriba_idx))
	await process_frame

	# Arrastrar grupo libre de M0 a un punto lejano en pantalla (ej. zona superior derecha)
	var grp_c0: Vector2 = l0_after["hud_group_rect"].get_center()
	var target_screen := vp.position + Vector2(vp.size.x * 0.78, vp.size.y * 0.22)
	var ev_d := InputEventMouseButton.new()
	ev_d.button_index = MOUSE_BUTTON_LEFT
	ev_d.pressed = true
	ev_d.position = grp_c0
	canvas.gui_input.emit(ev_d)
	await process_frame
	var ev_m := InputEventMouseMotion.new()
	ev_m.position = target_screen
	canvas.gui_input.emit(ev_m)
	await process_frame
	var ev_u := InputEventMouseButton.new()
	ev_u.button_index = MOUSE_BUTTON_LEFT
	ev_u.pressed = false
	ev_u.position = target_screen
	canvas.gui_input.emit(ev_u)
	await process_frame

	var l0_fixed: Dictionary = db.get_troop_preview_enemy_layout(0)
	var grp0_fixed: Rect2 = l0_fixed["hud_group_rect"]
	var hp0_fixed: Rect2 = l0_fixed["hp_rect"]

	# 3. REPRODUCCION / VERIFICACION: Mover enemigo M0 y luego cambiar su escala
	var spr0_c: Vector2 = (l0_fixed["sprite_rect"] as Rect2).get_center()
	var mv_dn := InputEventMouseButton.new()
	mv_dn.button_index = MOUSE_BUTTON_LEFT
	mv_dn.pressed = true
	mv_dn.position = spr0_c
	canvas.gui_input.emit(mv_dn)
	await process_frame
	var mv_mo := InputEventMouseMotion.new()
	mv_mo.position = spr0_c + Vector2(90.0, 50.0)
	canvas.gui_input.emit(mv_mo)
	await process_frame
	var mv_up := InputEventMouseButton.new()
	mv_up.button_index = MOUSE_BUTTON_LEFT
	mv_up.pressed = false
	mv_up.position = mv_mo.position
	canvas.gui_input.emit(mv_up)
	await process_frame

	var l0_after_move: Dictionary = db.get_troop_preview_enemy_layout(0)
	var grp0_after_move: Rect2 = l0_after_move["hud_group_rect"]
	assert_true((l0_after_move["sprite_rect"] as Rect2).position.x > (l0_fixed["sprite_rect"] as Rect2).position.x + 20.0, "Enemy sprite moved right")
	assert_near(grp0_after_move.position.x, grp0_fixed.position.x, 0.5, "Libre HUD X stays fixed when enemy moves")
	assert_near(grp0_after_move.position.y, grp0_fixed.position.y, 0.5, "Libre HUD Y stays fixed when enemy moves")
	assert_near((l0_after_move["hp_rect"] as Rect2).position.x, hp0_fixed.position.x, 0.5, "Libre hp_rect X stays fixed when enemy moves")

	# Cambiar escala de M0 a x4
	right_click_member(canvas, l0_after_move["sprite_rect"])
	await process_frame
	scale_sub.emit_signal("id_pressed", 4)
	await process_frame
	var l0_after_scale: Dictionary = db.get_troop_preview_enemy_layout(0)
	var grp0_after_scale: Rect2 = l0_after_scale["hud_group_rect"]
	assert_true((l0_after_scale["sprite_rect"] as Rect2).size.x > (l0_after_move["sprite_rect"] as Rect2).size.x * 2.0, "Enemy sprite scaled x4")
	assert_near(grp0_after_scale.position.x, grp0_fixed.position.x, 0.5, "Libre HUD X stays fixed when enemy scale changes")
	assert_near(grp0_after_scale.position.y, grp0_fixed.position.y, 0.5, "Libre HUD Y stays fixed when enemy scale changes")
	assert_near(grp0_after_scale.size.x, grp0_fixed.size.x, 0.5, "Libre HUD width stays fixed when enemy scale changes")

	# Comprobar que Arriba (M1) SI sigue al enemigo al escalar
	var l1_before_scale: Dictionary = db.get_troop_preview_enemy_layout(1)
	right_click_member(canvas, l1_before_scale["sprite_rect"])
	await process_frame
	scale_sub.emit_signal("id_pressed", 2)
	await process_frame
	var l1_after_scale: Dictionary = db.get_troop_preview_enemy_layout(1)
	assert_true((l1_after_scale["hud_group_rect"] as Rect2).position.y < (l1_before_scale["hud_group_rect"] as Rect2).position.y, "Arriba mode still follows scaled sprite upward")

	# Guardar, cerrar y reabrir
	db.save_to_disk()
	await process_frame
	var saved_m0: Dictionary = dm.get_records("troops")[1]["members"][0]
	var saved_x: int = int(saved_m0.get("hp_bar_x", -1))
	var saved_y: int = int(saved_m0.get("hp_bar_y", -1))
	assert_true(saved_m0.has("hp_bar_x") and saved_m0.has("hp_bar_y"), "Saved member stores hp_bar_x and hp_bar_y integers")
	assert_true(not saved_m0.has("hp_bar_offset_x") and not saved_m0.has("hp_bar_offset_y"), "Saved member has no duplicate offset fields")
	assert_true(saved_x >= 0 and saved_x <= 960 and saved_y >= 0 and saved_y <= 540, "Saved hp_bar_x/y inside 960x540 bounds")
	db.queue_free()
	await process_frame

	dm.load_all(_data_dir)
	await process_frame
	var reopened_m0: Dictionary = dm.get_records("troops")[1]["members"][0]
	assert_eq(int(reopened_m0.get("hp_bar_x", -999)), saved_x, "Reopened troop preserves hp_bar_x")
	assert_eq(int(reopened_m0.get("hp_bar_y", -999)), saved_y, "Reopened troop preserves hp_bar_y")

	# Abrir de nuevo vista previa para comparar con BattleScene
	var ctx_cmp: Dictionary = await open_db_troops(1)
	var db_cmp = ctx_cmp["db"]
	var vp_cmp: Rect2 = db_cmp.get_troop_preview_viewport_rect()
	var prev_grp0: Rect2 = db_cmp.get_troop_preview_enemy_layout(0)["hud_group_rect"]
	db_cmp.queue_free()
	await process_frame

	root.size = Vector2i(960, 540)
	var bs = ClassDB.instantiate("ClockGearsBattleScene")
	root.add_child(bs)
	bs.setup_battle(2, true, false, "", null, true)
	await process_frame
	await process_frame
	var bs_vp: Rect2 = bs.get_battle_field_rect()
	var bs_grp0: Rect2 = bs.get_enemy_hud_group_rect(0)
	assert_near((bs_grp0.get_center().x - bs_vp.position.x) / bs_vp.size.x, (prev_grp0.get_center().x - vp_cmp.position.x) / vp_cmp.size.x, 0.01, "BattleScene libre X matches preview")
	assert_near((bs_grp0.position.y - bs_vp.position.y) / bs_vp.size.y, (prev_grp0.position.y - vp_cmp.position.y) / vp_cmp.size.y, 0.01, "BattleScene libre Y matches preview")
	bs.finish_battle_immediate()
	bs.queue_free()
	await process_frame

	# Verificar Restablecer borra posicion guardada y vuelve a Abajo
	var ctx_rst: Dictionary = await open_db_troops(1)
	var db_rst = ctx_rst["db"]
	var canvas_rst: Control = ctx_rst["canvas"]
	var hp_sub_rst: PopupMenu = find_popup_with_item_text(db_rst, "Restablecer")
	var rst_idx := find_popup_item_idx(hp_sub_rst, "Restablecer")
	var l0_rst: Dictionary = db_rst.get_troop_preview_enemy_layout(0)
	right_click_member(canvas_rst, l0_rst["sprite_rect"])
	await process_frame
	hp_sub_rst.emit_signal("id_pressed", hp_sub_rst.get_item_id(rst_idx))
	await process_frame
	db_rst.save_to_disk()
	await process_frame
	var m0_rst: Dictionary = dm.get_records("troops")[1]["members"][0]
	assert_eq(str(m0_rst.get("hp_bar_mode", "")), "abajo", "Restablecer restores 'abajo'")
	assert_true(not m0_rst.has("hp_bar_x") and not m0_rst.has("hp_bar_y"), "Restablecer clears saved hp_bar_x / hp_bar_y")

	# Comprobar que Abajo (M0 tras Restablecer) sigue al enemigo al moverlo
	var l0_abajo_1: Dictionary = db_rst.get_troop_preview_enemy_layout(0)
	var c_abajo: Vector2 = (l0_abajo_1["sprite_rect"] as Rect2).get_center()
	var ab_dn := InputEventMouseButton.new()
	ab_dn.button_index = MOUSE_BUTTON_LEFT
	ab_dn.pressed = true
	ab_dn.position = c_abajo
	canvas_rst.gui_input.emit(ab_dn)
	await process_frame
	var ab_mo := InputEventMouseMotion.new()
	ab_mo.position = c_abajo + Vector2(-60.0, 35.0)
	canvas_rst.gui_input.emit(ab_mo)
	await process_frame
	var ab_up := InputEventMouseButton.new()
	ab_up.button_index = MOUSE_BUTTON_LEFT
	ab_up.pressed = false
	ab_up.position = ab_mo.position
	canvas_rst.gui_input.emit(ab_up)
	await process_frame
	var l0_abajo_2: Dictionary = db_rst.get_troop_preview_enemy_layout(0)
	assert_true((l0_abajo_2["hud_group_rect"] as Rect2).position.x < (l0_abajo_1["hud_group_rect"] as Rect2).position.x - 10.0, "Abajo mode still follows enemy X movement")
	assert_true((l0_abajo_2["hud_group_rect"] as Rect2).position.y > (l0_abajo_1["hud_group_rect"] as Rect2).position.y + 10.0, "Abajo mode still follows enemy Y movement")

	db_rst.queue_free()
	await process_frame

	finish_and_quit()
