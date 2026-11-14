extends SceneTree

var passed: int = 0
var failed: int = 0
var _done: bool = false
var _snap: Dictionary = {}
var _data_dir: String = "res://clockgears_data"

func log_evt(msg: String) -> void:
	print("[TEST] ", msg)

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

func cleanup_temp_files() -> void:
	restore_json_dir(_data_dir, _snap)
	var abs_tmp := ProjectSettings.globalize_path("res://tmp_troop_hp_test")
	if DirAccess.dir_exists_absolute(abs_tmp):
		for f in DirAccess.get_files_at("res://tmp_troop_hp_test"):
			DirAccess.remove_absolute(abs_tmp.path_join(f))
		DirAccess.remove_absolute(abs_tmp)

func finish_and_quit() -> void:
	if _done:
		return
	_done = true
	cleanup_temp_files()
	print("RESUMEN TROOP HP BAR -> PASS:%d FAIL:%d" % [passed, failed])
	quit(0 if failed == 0 else 1)

func _watchdog_loop() -> void:
	var t0: int = Time.get_ticks_msec()
	while not _done:
		if Time.get_ticks_msec() - t0 >= 60000:
			failed += 1
			printerr("[FAIL] Watchdog timeout 60s reached; aborting test cleanly")
			finish_and_quit()
			return
		await process_frame

func make_test_png(path: String, w: int, h: int, col: Color) -> void:
	var img := Image.create_empty(w, h, false, Image.FORMAT_RGBA8)
	img.fill(col)
	var abs_p := ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs_p.get_base_dir())
	img.save_png(abs_p)

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

func find_popup_item_idx(pm: PopupMenu, exact_or_sub: String) -> int:
	if pm == null:
		return -1
	for i in range(pm.get_item_count()):
		if pm.get_item_text(i) == exact_or_sub or exact_or_sub in pm.get_item_text(i):
			return i
	return -1

func find_troop_canvas(db: Node) -> Control:
	for c in find_controls_of_class(db, "Control"):
		var ctrl := c as Control
		if ctrl and ctrl.get_class() == "Control" and ctrl.is_visible_in_tree() and ctrl.get_child_count() >= 3 and ctrl.size.x >= 300.0:
			return ctrl
	return null

func _init() -> void:
	call_deferred("_watchdog_loop")
	call_deferred("_run_tests")

func _run_tests() -> void:
	_snap = backup_json_dir(_data_dir)
	var tmp_enemy_png := "res://tmp_troop_hp_test/enemy_32.png"
	make_test_png(tmp_enemy_png, 32, 32, Color(0.85, 0.25, 0.2, 1.0))

	var dm = Engine.get_singleton("ClockGearsDataManager")
	assert_true(dm != null, "ClockGearsDataManager singleton exists")
	if dm == null:
		finish_and_quit()
		return

	log_evt("Paso 1: Verificar migracion de tropa antigua y modo desconocido")
	dm.init_empty()
	var actor1 := {
		"id": 1,
		"name": "Heroe",
		"hp": 100,
		"mp": 20,
		"atk": 15,
		"def": 10
	}
	var enemy1 := {
		"id": 1,
		"name": "Slime",
		"level": 3,
		"hp": 120,
		"mp": 10,
		"atk": 12,
		"def": 5,
		"graphic_path": tmp_enemy_png
	}
	var legacy_troop := {
		"id": 1,
		"name": "TropaAntigua",
		"members": [
			{"enemy_id": 1, "x": 200, "y": 240, "escala": 1},
			{"enemy_id": 1, "x": 500, "y": 240, "escala": 2, "hp_bar_mode": "modo_inventado"}
		]
	}
	dm.set_records("actors", [actor1])
	dm.set_records("enemies", [enemy1])
	dm.set_records("troops", [legacy_troop])
	var sys_cfg: Dictionary = dm.get_system_config()
	sys_cfg["party"] = [1]
	sys_cfg["combat_mode"] = 0
	dm.set_system_config(sys_cfg)
	dm.save_all(_data_dir)
	dm.load_all(_data_dir)
	await process_frame

	var loaded_troops: Array = dm.get_records("troops")
	assert_eq(loaded_troops.size(), 1, "1.1 Legacy troop loaded")
	var lmems: Array = loaded_troops[0].get("members", [])
	assert_eq(lmems.size(), 2, "1.2 Legacy troop has 2 members")
	assert_eq(str(lmems[0].get("hp_bar_mode", "")), "abajo", "1.3 Legacy member migrates to hp_bar_mode='abajo'")
	assert_eq(int(lmems[0].get("hp_bar_offset_x", -999)), 0, "1.4 Legacy member migrates hp_bar_offset_x=0")
	assert_eq(int(lmems[0].get("hp_bar_offset_y", -999)), 0, "1.5 Legacy member migrates hp_bar_offset_y=0")
	assert_eq(str(lmems[1].get("hp_bar_mode", "")), "abajo", "1.6 Unknown mode migrates/falls back safely to 'abajo'")

	log_evt("Paso 2: Configurar tropa con 3 miembros (Arriba, Libre, Oculta) y abrir BD")
	var test_troop := {
		"id": 2,
		"name": "Tropa3Modos",
		"members": [
			{"enemy_id": 1, "x": 160, "y": 250, "escala": 1, "hp_bar_mode": "arriba", "hp_bar_offset_x": 0, "hp_bar_offset_y": 0},
			{"enemy_id": 1, "x": 420, "y": 250, "escala": 2, "hp_bar_mode": "libre", "hp_bar_offset_x": 35, "hp_bar_offset_y": -85},
			{"enemy_id": 1, "x": 680, "y": 250, "escala": 1, "hp_bar_mode": "oculta", "hp_bar_offset_x": 0, "hp_bar_offset_y": 0}
		]
	}
	dm.set_records("troops", [legacy_troop, test_troop])
	dm.save_all(_data_dir)
	dm.load_all(_data_dir)
	await process_frame

	var db = ClassDB.instantiate("ClockGearsDatabaseDialog")
	root.add_child(db)
	db.size = Vector2i(1364, 700)
	db.open_database()
	await process_frame
	db.select_category_by_key("troops")
	await process_frame

	var item_lists := find_controls_of_class(db, "ItemList")
	assert_true(item_lists.size() >= 1, "2.0 record_list found")
	var rec_list: ItemList = item_lists[0] as ItemList
	if rec_list.get_item_count() > 1:
		rec_list.select(1)
		rec_list.emit_signal("item_selected", 1)
	await process_frame
	await process_frame

	var ctx_menu: PopupMenu = find_popup_with_item_text(db, "Escala")
	assert_true(ctx_menu != null, "2.1 Context menu with 'Escala' exists")
	var hp_idx := find_popup_item_idx(ctx_menu, "Ajustar barra de vida")
	assert_true(hp_idx >= 0, "2.2 Context menu has 'Ajustar barra de vida' entry")
	var hp_sub: PopupMenu = find_popup_with_item_text(db, "Mover libremente")
	assert_true(hp_sub != null, "2.3 Submenu has 'Mover libremente'")
	assert_true(find_popup_item_idx(hp_sub, "Arriba") >= 0, "2.4 Submenu has 'Arriba'")
	assert_true(find_popup_item_idx(hp_sub, "Abajo") >= 0, "2.5 Submenu has 'Abajo'")
	assert_true(find_popup_item_idx(hp_sub, "Ocultar barra") >= 0, "2.6 Submenu has 'Ocultar barra'")
	assert_true(find_popup_item_idx(hp_sub, "Restablecer") >= 0, "2.7 Submenu has 'Restablecer'")

	assert_true(db.has_method("get_troop_preview_enemy_layout"), "2.8 DB exposes get_troop_preview_enemy_layout")
	var prev_layouts := []
	if db.has_method("get_troop_preview_enemy_layout"):
		for m_i in range(3):
			prev_layouts.append(db.get_troop_preview_enemy_layout(m_i))

	var prev_vp: Rect2 = db.get_troop_preview_viewport_rect() if db.has_method("get_troop_preview_viewport_rect") else Rect2(0, 0, 960, 540)
	assert_true(prev_vp.size.x > 10.0 and prev_vp.size.y > 10.0, "2.9 Preview viewport rect valid")

	if prev_layouts.size() == 3:
		var l_arr: Dictionary = prev_layouts[0]
		var spr0: Rect2 = l_arr.get("sprite_rect", Rect2())
		var hp0: Rect2 = l_arr.get("hp_rect", Rect2())
		var lbl0_y: float = float(l_arr.get("label_baseline_y", 0.0))
		assert_true(hp0.position.y + hp0.size.y <= spr0.position.y + 1.0,
			"2.10 Member 0 (Arriba): HP bar is ABOVE sprite (hp_bottom=%s <= spr_top=%s)" % [str(hp0.position.y + hp0.size.y), str(spr0.position.y)])
		assert_true(lbl0_y <= hp0.position.y + 1.0,
			"2.11 Member 0 (Arriba): Name+level stays ABOVE HP bar (lbl_y=%s <= hp_y=%s)" % [str(lbl0_y), str(hp0.position.y)])
		assert_near(hp0.get_center().x, spr0.get_center().x, 1.5,
			"2.12 Member 0 (Arriba): centered horizontally over sprite")

		var base_scale1_top: float = spr0.position.y
		var base_hp0_bottom: float = hp0.position.y + hp0.size.y
		var scale_sub: PopupMenu = find_popup_with_item_text(db, "x2")
		assert_true(scale_sub != null, "2.13 Escala submenu exists")
		if scale_sub:
			scale_sub.emit_signal("id_pressed", 2)
			await process_frame
			var l_arr_x2: Dictionary = db.get_troop_preview_enemy_layout(0)
			var spr0_x2: Rect2 = l_arr_x2.get("sprite_rect", Rect2())
			var hp0_x2: Rect2 = l_arr_x2.get("hp_rect", Rect2())
			assert_true(spr0_x2.size.y > spr0.size.y * 1.8, "2.14 Escala x2 enlarges sprite")
			assert_true(spr0_x2.position.y < base_scale1_top and (hp0_x2.position.y + hp0_x2.size.y) < base_hp0_bottom,
				"2.15 Arriba follows scaled sprite upward")
			assert_true(hp0_x2.position.y + hp0_x2.size.y <= spr0_x2.position.y + 1.0,
				"2.16 Arriba stays cleanly above scaled x2 sprite")
			scale_sub.emit_signal("id_pressed", 1)
			await process_frame
			prev_layouts[0] = db.get_troop_preview_enemy_layout(0)

		var l_hid: Dictionary = prev_layouts[2]
		assert_eq(bool(l_hid.get("hp_visible", true)), false, "2.17 Member 2 (Oculta) hides HP bar in preview")
		assert_true(float(l_hid.get("label_baseline_y", 0.0)) > 0.0, "2.18 Member 2 (Oculta) keeps name+level visible")

	log_evt("Paso 3: Guardar, cerrar y reabrir DB/DataManager")
	db.save_to_disk()
	db.queue_free()
	await process_frame

	dm.load_all(_data_dir)
	await process_frame
	var reopened_t2: Dictionary = dm.get_records("troops")[1]
	var rmems: Array = reopened_t2.get("members", [])
	assert_eq(str(rmems[0].get("hp_bar_mode", "")), "arriba", "3.1 Save/reopen preserves Member 0 'arriba'")
	assert_eq(str(rmems[1].get("hp_bar_mode", "")), "libre", "3.2 Save/reopen preserves Member 1 'libre'")
	assert_eq(int(rmems[1].get("hp_bar_offset_x", 0)), 35, "3.3 Save/reopen preserves Member 1 offset_x=35")
	assert_eq(int(rmems[1].get("hp_bar_offset_y", 0)), -85, "3.4 Save/reopen preserves Member 1 offset_y=-85")
	assert_eq(str(rmems[2].get("hp_bar_mode", "")), "oculta", "3.5 Save/reopen preserves Member 2 'oculta'")

	log_evt("Paso 4: Arrastre de enemigo y grupo libre en vista previa")
	var db2 = ClassDB.instantiate("ClockGearsDatabaseDialog")
	root.add_child(db2)
	db2.size = Vector2i(1364, 700)
	db2.open_database()
	await process_frame
	db2.select_category_by_key("troops")
	await process_frame
	var rec_list2: ItemList = find_controls_of_class(db2, "ItemList")[0] as ItemList
	if rec_list2.get_item_count() > 1:
		rec_list2.select(1)
		rec_list2.emit_signal("item_selected", 1)
	await process_frame
	await process_frame

	var vp2: Rect2 = db2.get_troop_preview_viewport_rect() if db2.has_method("get_troop_preview_viewport_rect") else Rect2(0, 0, 960, 540)
	var before_move_m1: Dictionary = db2.get_troop_preview_enemy_layout(1) if db2.has_method("get_troop_preview_enemy_layout") else {}
	var canvas2: Control = find_troop_canvas(db2)
	assert_true(canvas2 != null, "4.0 Found troop placement canvas in DB")

	if canvas2 and db2.has_method("get_troop_preview_enemy_layout") and not before_move_m1.is_empty():
		var spr1_c: Vector2 = (before_move_m1.get("sprite_rect", Rect2()) as Rect2).get_center()
		var grp1_before: Rect2 = before_move_m1.get("hud_group_rect", Rect2())
		var drag_delta := Vector2(50.0, 20.0)

		var ev_dn := InputEventMouseButton.new()
		ev_dn.button_index = MOUSE_BUTTON_LEFT
		ev_dn.pressed = true
		ev_dn.position = spr1_c
		canvas2.gui_input.emit(ev_dn)
		await process_frame

		var ev_mv := InputEventMouseMotion.new()
		ev_mv.position = spr1_c + drag_delta
		canvas2.gui_input.emit(ev_mv)
		await process_frame

		var ev_up := InputEventMouseButton.new()
		ev_up.button_index = MOUSE_BUTTON_LEFT
		ev_up.pressed = false
		ev_up.position = spr1_c + drag_delta
		canvas2.gui_input.emit(ev_up)
		await process_frame

		var after_move_m1: Dictionary = db2.get_troop_preview_enemy_layout(1)
		var spr1_after: Vector2 = (after_move_m1.get("sprite_rect", Rect2()) as Rect2).get_center()
		var grp1_after: Rect2 = after_move_m1.get("hud_group_rect", Rect2())
		var spr_shift: Vector2 = spr1_after - spr1_c
		var grp_shift: Vector2 = grp1_after.position - grp1_before.position
		assert_true(spr_shift.x > 10.0, "4.1 Dragging enemy sprite moves enemy (shift=%s)" % str(spr_shift))
		assert_near(grp_shift.x, spr_shift.x, 2.0, "4.2 Moving enemy drags its free HUD group along X")
		assert_near(grp_shift.y, spr_shift.y, 2.0, "4.3 Moving enemy drags its free HUD group along Y")

		var grp_c: Vector2 = grp1_after.get_center()
		var grp_ev_dn := InputEventMouseButton.new()
		grp_ev_dn.button_index = MOUSE_BUTTON_LEFT
		grp_ev_dn.pressed = true
		grp_ev_dn.position = grp_c
		canvas2.gui_input.emit(grp_ev_dn)
		await process_frame

		var grp_ev_mv := InputEventMouseMotion.new()
		grp_ev_mv.position = Vector2(vp2.position.x + vp2.size.x + 200.0, vp2.position.y - 120.0)
		canvas2.gui_input.emit(grp_ev_mv)
		await process_frame

		var grp_during: Dictionary = db2.get_troop_preview_enemy_layout(1)
		assert_true(bool(grp_during.get("hud_highlighted", false)), "4.4 Free HUD group is highlighted while dragging")

		var grp_ev_up := InputEventMouseButton.new()
		grp_ev_up.button_index = MOUSE_BUTTON_LEFT
		grp_ev_up.pressed = false
		grp_ev_up.position = grp_ev_mv.position
		canvas2.gui_input.emit(grp_ev_up)
		await process_frame

		var after_grp_drag: Dictionary = db2.get_troop_preview_enemy_layout(1)
		var grp_clamped: Rect2 = after_grp_drag.get("hud_group_rect", Rect2())
		assert_true(grp_clamped.position.x >= vp2.position.x - 0.5 and grp_clamped.position.x + grp_clamped.size.x <= vp2.position.x + vp2.size.x + 0.5,
			"4.5 Free HUD group stays inside 960 battle width (rect=%s, vp=%s)" % [str(grp_clamped), str(vp2)])
		assert_true(grp_clamped.position.y >= vp2.position.y - 0.5 and grp_clamped.position.y + grp_clamped.size.y <= vp2.position.y + vp2.size.y + 0.5,
			"4.6 Free HUD group stays inside 540 battle height (rect=%s, vp=%s)" % [str(grp_clamped), str(vp2)])

	db2.queue_free()
	await process_frame

	log_evt("Paso 5: Prueba de batalla vs Vista Previa y tropa antigua")
	dm.set_records("troops", [legacy_troop, test_troop])
	dm.save_all(_data_dir)
	dm.load_all(_data_dir)

	var db3 = ClassDB.instantiate("ClockGearsDatabaseDialog")
	root.add_child(db3)
	db3.size = Vector2i(1364, 700)
	db3.open_database()
	await process_frame
	db3.select_category_by_key("troops")
	await process_frame
	var rec_list3: ItemList = find_controls_of_class(db3, "ItemList")[0] as ItemList
	if rec_list3.get_item_count() > 1:
		rec_list3.select(1)
		rec_list3.emit_signal("item_selected", 1)
	await process_frame
	await process_frame
	var vp_prev3: Rect2 = db3.get_troop_preview_viewport_rect() if db3.has_method("get_troop_preview_viewport_rect") else Rect2(0, 0, 960, 540)
	var prev_exact := []
	if db3.has_method("get_troop_preview_enemy_layout"):
		for m_i in range(3):
			prev_exact.append(db3.get_troop_preview_enemy_layout(m_i))
	db3.queue_free()
	await process_frame

	# ClockGearsBattleScene hereda de CanvasLayer; toma tamaño del viewport raiz
	root.size = Vector2i(960, 540)
	var bs = ClassDB.instantiate("ClockGearsBattleScene")
	root.add_child(bs)
	bs.setup_battle(2, true, false, "", null, true)
	await process_frame
	await process_frame

	assert_eq(bs.is_enemy_hp_bar_visible(0), true, "5.1 Battle member 0 (arriba) HP bar visible")
	assert_eq(bs.is_enemy_hp_bar_visible(1), true, "5.2 Battle member 1 (libre) HP bar visible")
	assert_eq(bs.is_enemy_hp_bar_visible(2), false, "5.3 Battle member 2 (oculta) HP bar HIDDEN")
	assert_true("Slime" in bs.get_enemy_label_text(2) and "Nv" in bs.get_enemy_label_text(2),
		"5.4 Battle member 2 (oculta) keeps name and level text")
	assert_true(bs.has_method("get_enemy_hud_group_rect") and bs.has_method("get_enemy_hp_bar_rect"),
		"5.5 BattleScene exposes get_enemy_hud_group_rect and get_enemy_hp_bar_rect")

	if bs.has_method("get_enemy_hud_group_rect") and prev_exact.size() == 3:
		var bs_vp: Rect2 = bs.get_battle_field_rect() if bs.has_method("get_battle_field_rect") else Rect2(0, 0, 960, 540)
		for m_i in range(3):
			var p_grp: Rect2 = prev_exact[m_i].get("hud_group_rect", Rect2())
			var b_grp: Rect2 = bs.get_enemy_hud_group_rect(m_i)
			var p_nx := (p_grp.get_center().x - vp_prev3.position.x) / maxf(1.0, vp_prev3.size.x)
			var p_ny := (p_grp.get_center().y - vp_prev3.position.y) / maxf(1.0, vp_prev3.size.y)
			var b_nx := (b_grp.get_center().x - bs_vp.position.x) / maxf(1.0, bs_vp.size.x)
			var b_ny := (b_grp.get_center().y - bs_vp.position.y) / maxf(1.0, bs_vp.size.y)
			assert_near(b_nx, p_nx, 0.01, "5.6.%d Battle group center X matches preview normalized (b=%s vs p=%s)" % [m_i, str(b_nx), str(p_nx)])
			assert_near(b_ny, p_ny, 0.01, "5.7.%d Battle group center Y matches preview normalized (b=%s vs p=%s)" % [m_i, str(b_ny), str(p_ny)])

		bs.finish_battle_immediate()
		bs.queue_free()
		await process_frame

		var bs_leg = ClassDB.instantiate("ClockGearsBattleScene")
		root.add_child(bs_leg)
		bs_leg.setup_battle(1, true, false, "", null, true)
		await process_frame
		var leg_spr: Rect2 = bs_leg.get_enemy_sprite_rect(0)
		var leg_hp: Rect2 = bs_leg.get_enemy_hp_bar_rect(0)
		assert_true(leg_hp.position.y >= leg_spr.position.y + leg_spr.size.y,
			"5.8 Legacy troop renders HP bar below sprite as before")
		assert_eq(bs_leg.is_enemy_hp_bar_visible(1), true, "5.9 Unknown mode renders HP bar as Abajo without error")
		bs_leg.finish_battle_immediate()
		bs_leg.queue_free()
		await process_frame
	else:
		bs.finish_battle_immediate()
		bs.queue_free()
		await process_frame

	log_evt("Paso 6: Restablecer devuelve a Abajo y limpia desplazamiento")
	var db4 = ClassDB.instantiate("ClockGearsDatabaseDialog")
	root.add_child(db4)
	db4.size = Vector2i(1364, 700)
	db4.open_database()
	await process_frame
	db4.select_category_by_key("troops")
	await process_frame
	var rec_list4: ItemList = find_controls_of_class(db4, "ItemList")[0] as ItemList
	if rec_list4.get_item_count() > 1:
		rec_list4.select(1)
		rec_list4.emit_signal("item_selected", 1)
	await process_frame
	await process_frame

	if db4.has_method("get_troop_preview_enemy_layout"):
		var canvas4: Control = find_troop_canvas(db4)
		var l_m1: Dictionary = db4.get_troop_preview_enemy_layout(1)
		var rmb_ev := InputEventMouseButton.new()
		rmb_ev.button_index = MOUSE_BUTTON_RIGHT
		rmb_ev.pressed = true
		rmb_ev.position = (l_m1.get("sprite_rect", Rect2()) as Rect2).get_center()
		canvas4.gui_input.emit(rmb_ev)
		await process_frame

		var hp_sub4: PopupMenu = find_popup_with_item_text(db4, "Restablecer")
		var rst_idx := find_popup_item_idx(hp_sub4, "Restablecer")
		assert_true(hp_sub4 != null and rst_idx >= 0, "6.0 Found Restablecer in context submenu")
		if hp_sub4 and rst_idx >= 0:
			hp_sub4.emit_signal("id_pressed", hp_sub4.get_item_id(rst_idx))
			await process_frame
		db4.save_to_disk()
		await process_frame
		var after_rst: Dictionary = dm.get_records("troops")[1].get("members", [])[1]
		assert_eq(str(after_rst.get("hp_bar_mode", "")), "abajo", "6.1 Restablecer sets mode back to 'abajo'")
		assert_eq(int(after_rst.get("hp_bar_offset_x", -1)), 0, "6.2 Restablecer clears offset_x to 0")
		assert_eq(int(after_rst.get("hp_bar_offset_y", -1)), 0, "6.3 Restablecer clears offset_y to 0")

	db4.queue_free()
	await process_frame

	finish_and_quit()
