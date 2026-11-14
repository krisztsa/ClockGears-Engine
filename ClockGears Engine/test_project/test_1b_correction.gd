extends SceneTree

var passed: int = 0
var failed: int = 0

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

func make_test_png(path: String, w: int = 64, h: int = 32, col: Color = Color(0.2, 0.7, 0.9, 1.0)) -> void:
	var img := Image.create_empty(w, h, false, Image.FORMAT_RGBA8)
	img.fill(col)
	var abs_p := ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs_p.get_base_dir())
	img.save_png(abs_p)

func make_slot_dict(sheet_path: String, count: int, fps: float = 10.0, is_loop: bool = true) -> Dictionary:
	var frames := []
	var frames_data := []
	for i in range(count):
		frames.append(i)
		frames_data.append({
			"sheet": sheet_path,
			"x": (i % 4) * 32,
			"y": 0,
			"w": 32,
			"h": 32,
			"duration": 0.0
		})
	return {
		"spritesheet_path": sheet_path,
		"cols": 4,
		"rows": 1,
		"frame_w": 32,
		"frame_h": 32,
		"fps": fps,
		"loop": is_loop,
		"frames": frames,
		"frames_data": frames_data
	}

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

func find_controls_of_class(node: Node, cls_name: String) -> Array:
	var res := []
	for ch in node.get_children():
		if ch.is_class(cls_name):
			res.append(ch)
		res.append_array(find_controls_of_class(ch, cls_name))
	return res

func find_anim_track_option(db: Node) -> OptionButton:
	var opts := find_controls_of_class(db, "OptionButton")
	for o in opts:
		if o.has_meta("cg_anim_track_opt"):
			return o
	# Fallback heuristic: OptionButton whose sibling or nearby contains mover_abajo when Actor loaded
	for o in opts:
		for i in range(o.get_item_count()):
			var md = str(o.get_item_metadata(i))
			if md == "mover_abajo" or md.begins_with("mover_") or md.begins_with("atacar_"):
				return o
	return null

func find_label_with_text(node: Node, needle: String) -> Label:
	for ch in node.get_children():
		if ch is Label and needle in (ch as Label).text:
			return ch as Label
		var sub := find_label_with_text(ch, needle)
		if sub != null:
			return sub
	return null

func find_checkbox_with_text(node: Node, needle: String) -> CheckBox:
	for ch in node.get_children():
		if ch is CheckBox and needle in (ch as CheckBox).text:
			return ch as CheckBox
		var sub := find_checkbox_with_text(ch, needle)
		if sub != null:
			return sub
	return null

func get_anim_slot_keys(opt: OptionButton) -> PackedStringArray:
	var out := PackedStringArray()
	if opt == null:
		return out
	for i in range(opt.get_item_count()):
		var md = str(opt.get_item_metadata(i))
		out.append(md if not md.is_empty() else opt.get_item_text(i))
	return out

func push_mouse_button(btn: int, pressed: bool, pos: Vector2 = Vector2(200, 200)) -> void:
	var ev := InputEventMouseButton.new()
	ev.button_index = btn
	ev.pressed = pressed
	ev.position = pos
	ev.global_position = pos
	Input.parse_input_event(ev)

func push_key(keycode: int, pressed: bool) -> void:
	var ev := InputEventKey.new()
	ev.keycode = keycode
	ev.physical_keycode = keycode
	ev.pressed = pressed
	Input.parse_input_event(ev)

func wait_input_flush() -> void:
	await process_frame
	await process_frame

func _init() -> void:
	call_deferred("_run_tests")

func _run_tests() -> void:
	Input.set_use_accumulated_input(false)
	var data_dir := "res://clockgears_data"
	var snap := backup_json_dir(data_dir)
	var tmp_img := "res://tmp_1b_corr/sheet.png"
	var tmp_wpn := "res://tmp_1b_corr/wpn.png"
	make_test_png(tmp_img, 128, 32, Color(0.2, 0.8, 0.4, 1.0))
	make_test_png(tmp_wpn, 128, 32, Color(0.95, 0.3, 0.2, 1.0))

	var dm = Engine.get_singleton("ClockGearsDataManager")
	assert_true(dm != null, "ClockGearsDataManager singleton exists")

	# Prepare fresh data on disk so open_database() loads Kris cleanly
	dm.init_empty()
	var kris_anims := {
		"mover_abajo": make_slot_dict(tmp_img, 4, 8.0, true),
		"mover_arriba": make_slot_dict(tmp_img, 4, 8.0, true),
		"mover_izquierda": make_slot_dict(tmp_img, 4, 8.0, true),
		"mover_derecha": make_slot_dict(tmp_img, 4, 8.0, true),
		"atacar_arriba": make_slot_dict(tmp_img, 5, 12.0, false),
		"atacar_abajo": make_slot_dict(tmp_img, 4, 12.0, false),
		"atacar_arriba_derecha": make_slot_dict(tmp_img, 3, 12.0, false),
		"bloquear_abajo": make_slot_dict(tmp_img, 2, 8.0, true),
		"bloquear_derecha": make_slot_dict(tmp_img, 2, 8.0, true)
	}
	var kris := {
		"id": 1,
		"name": "Kris",
		"hp": 100,
		"mp": 20,
		"atk": 15,
		"def": 10,
		"directional_idle": false,
		"sprite_data": {
			"cols": 4,
			"rows": 1,
			"animations": kris_anims
		},
		"animations": kris_anims.duplicate(true)
	}
	dm.set_records("actors", [kris])
	dm.set_records("npcs", [])
	dm.set_records("arpg_npcs", [])
	dm.set_records("arpg_enemies", [])
	dm.set_records("arpg_weapons", [])
	dm.set_records("arpg_armors", [])
	dm.set_records("arpg_shields", [])
	dm.set_records("arpg_accessories", [])
	var sys_cfg: Dictionary = dm.get_system_config()
	sys_cfg["combat_mode"] = 1
	sys_cfg["movement_directions"] = 8
	sys_cfg["party"] = [1]
	# Remove new keys first to verify safe backward-compatible defaults
	sys_cfg.erase("arpg_move_while_attacking")
	sys_cfg.erase("arpg_attack_move_speed_pct")
	sys_cfg.erase("arpg_move_while_blocking")
	sys_cfg.erase("arpg_block_move_speed_pct")
	dm.set_system_config(sys_cfg)
	dm.save_all(data_dir)
	await process_frame

	# =========================================================================
	# SECTION 1: Empty / no-selection foreign data leak + SECTION 2: Slots/cat
	# =========================================================================
	var db = ClassDB.instantiate("ClockGearsDatabaseDialog")
	root.add_child(db)
	db.open_database()
	await process_frame

	db.select_category_by_key("actors")
	await process_frame
	var track_opt: OptionButton = find_anim_track_option(db)
	assert_true(track_opt != null, "1.1 Located animation track selector OptionButton")
	var actor_slots := get_anim_slot_keys(track_opt)
	assert_true("mover_abajo" in actor_slots and "atacar_arriba" in actor_slots and "bloquear_abajo" in actor_slots,
		"1.2 Actor ARPG shows mover_*, atacar_* and bloquear_*")

	var empty_cats := ["npcs", "arpg_npcs", "arpg_enemies", "arpg_weapons", "arpg_armors", "arpg_shields", "arpg_accessories"]
	for cat_key in empty_cats:
		db.select_category_by_key(cat_key)
		await process_frame
		var empty_lbl: Label = find_label_with_text(db, "Crea una entrada con +")
		assert_true(empty_lbl != null and empty_lbl.visible, "1.3 Empty category '%s' shows 'Crea una entrada con +'" % cat_key)
		var slot_cnt := track_opt.get_item_count() if track_opt != null else -1
		assert_eq(slot_cnt, 0, "1.4 Empty category '%s' clears all animation slots (no foreign Kris slots)" % cat_key)
		assert_true(track_opt == null or track_opt.disabled or not track_opt.is_visible_in_tree(),
			"1.5 Empty category '%s' disables/hides animation track editor" % cat_key)

	# Verify Name edit auto-create on empty arpg_enemies does NOT copy Kris animations
	db.select_category_by_key("actors")
	await process_frame
	db.select_category_by_key("arpg_enemies")
	await process_frame
	var name_lbl: Label = find_label_with_text(db, "Nombre:")
	var target_le: LineEdit = null
	if name_lbl and name_lbl.get_parent():
		for ch in name_lbl.get_parent().get_children():
			if ch is LineEdit and ch.get_index() > name_lbl.get_index():
				target_le = ch as LineEdit
				break
	assert_true(target_le != null, "1.6a Found Nombre LineEdit")
	if target_le:
		target_le.text = "SlimePrueba"
		target_le.emit_signal("text_changed", "SlimePrueba")
	await process_frame
	db.save_to_disk()
	await process_frame
	var ae_arr: Array = dm.get_records("arpg_enemies")
	assert_eq(ae_arr.size(), 1, "1.6b Typing Name auto-created 1 ARPG enemy")
	if ae_arr.size() > 0:
		var ae_anims: Dictionary = ae_arr[0].get("animations", {})
		assert_true(not ae_anims.has("atacar_arriba") and not ae_anims.has("mover_abajo"),
			"1.6c Auto-creating ARPG enemy after viewing Kris must NOT leak Kris animations (got keys=%s)" % str(ae_anims.keys()))

	# Section 2: Populated categories slot rules
	var wpn_anims := {
		"mover_abajo": make_slot_dict(tmp_wpn, 4, 8.0, true),
		"mover_derecha": make_slot_dict(tmp_wpn, 4, 8.0, true),
		"mover_arriba": make_slot_dict(tmp_wpn, 4, 8.0, true),
		"atacar_abajo": make_slot_dict(tmp_wpn, 4, 12.0, false),
		"atacar_arriba": make_slot_dict(tmp_wpn, 4, 12.0, false),
		"atacar_arriba_derecha": make_slot_dict(tmp_wpn, 3, 12.0, false),
		"bloquear_abajo": make_slot_dict(tmp_wpn, 2, 8.0, true),
		"bloquear_derecha": make_slot_dict(tmp_wpn, 2, 8.0, true)
	}
	var base_rec := {"id": 1, "name": "TestEntry", "directional_idle": false, "sprite_data": {"animations": {}}, "animations": {}}
	var npc_with_extra := base_rec.duplicate(true)
	npc_with_extra["sprite_data"] = {"animations": {"atacar_abajo": make_slot_dict(tmp_img, 3, 10.0, false)}}
	npc_with_extra["animations"] = {"atacar_abajo": make_slot_dict(tmp_img, 3, 10.0, false)}
	dm.set_records("npcs", [npc_with_extra])
	dm.set_records("arpg_npcs", [base_rec.duplicate(true)])
	dm.set_records("arpg_enemies", [base_rec.duplicate(true)])
	var wpn_rec := base_rec.duplicate(true)
	wpn_rec["id"] = 1
	wpn_rec["name"] = "EspadaTest"
	wpn_rec["draw_mode"] = "completo"
	wpn_rec["layer_order"] = 3
	wpn_rec["sprite_data"] = {"animations": wpn_anims}
	wpn_rec["animations"] = wpn_anims.duplicate(true)
	dm.set_records("arpg_weapons", [wpn_rec])
	dm.save_all(data_dir)
	db.open_database()
	await process_frame

	db.select_category_by_key("npcs")
	await process_frame
	var npc_slots := get_anim_slot_keys(track_opt)
	assert_true("mover_abajo" in npc_slots and "mover_arriba_derecha" in npc_slots, "2.1 NPC has 8-dir mover_*")
	assert_true(not ("atacar_abajo" in npc_slots) and not ("bloquear_abajo" in npc_slots),
		"2.2 NPC NEVER shows atacar_* or bloquear_* (got %s)" % str(npc_slots))
	# Verify hidden atacar_abajo on NPC was NOT deleted
	var saved_npc_anims: Dictionary = dm.get_records("npcs")[0].get("sprite_data", {}).get("animations", {})
	assert_true(saved_npc_anims.has("atacar_abajo"), "2.3 Misplaced atacar_abajo in NPC JSON hidden without deleting")

	db.select_category_by_key("arpg_npcs")
	await process_frame
	var anpc_slots := get_anim_slot_keys(track_opt)
	assert_true("atacar_abajo" in anpc_slots and not ("bloquear_abajo" in anpc_slots),
		"2.4 ARPG NPC shows atacar_* and NO bloquear_*")

	db.select_category_by_key("arpg_enemies")
	await process_frame
	var aen_slots := get_anim_slot_keys(track_opt)
	assert_true("atacar_abajo" in aen_slots and not ("bloquear_abajo" in aen_slots),
		"2.5 ARPG Enemy shows atacar_* and NO bloquear_*")

	db.select_category_by_key("arpg_weapons")
	await process_frame
	var awpn_slots := get_anim_slot_keys(track_opt)
	assert_true("atacar_abajo" in awpn_slots and "bloquear_abajo" in awpn_slots,
		"2.6 ARPG Weapon shows atacar_* and bloquear_* in ARPG mode")

	# Switch to Turn-Based (0) -> atacar_* and bloquear_* hidden without deleting Kris data
	var all_opts := find_controls_of_class(db, "OptionButton")
	var combat_opt: OptionButton = null
	for o in all_opts:
		if o.get_item_count() >= 2 and "Turnos" in o.get_item_text(0):
			combat_opt = o
			break
	assert_true(combat_opt != null, "2.7 Combat mode selector exists")
	if combat_opt:
		combat_opt.select(0)
		combat_opt.emit_signal("item_selected", 0)
		await process_frame
	db.select_category_by_key("actors")
	await process_frame
	var tb_actor_slots := get_anim_slot_keys(track_opt)
	assert_true("mover_abajo" in tb_actor_slots and not ("atacar_arriba" in tb_actor_slots) and not ("bloquear_abajo" in tb_actor_slots),
		"2.8 Classic Turn-Based hides atacar_* and bloquear_* on Actors")
	var k_saved: Dictionary = dm.get_records("actors")[0].get("sprite_data", {}).get("animations", {})
	assert_true(k_saved.has("atacar_arriba") and int(k_saved["atacar_arriba"].get("frames", []).size()) == 5,
		"2.9 Switching to Turn-Based preserves Kris atacar_arriba (5 frames)")

	# =========================================================================
	# SECTION 4: Database > Sistema ARPG settings
	# =========================================================================
	db.select_category_by_key("system")
	await process_frame
	var chk_move_atk: CheckBox = find_checkbox_with_text(db, "Moverse mientras atacas")
	var chk_move_blk: CheckBox = find_checkbox_with_text(db, "Moverse mientras bloqueas")
	assert_true(chk_move_atk != null and chk_move_blk != null, "4.1 Sistema has 'Moverse mientras atacas' and 'Moverse mientras bloqueas' checkboxes")
	if chk_move_atk and chk_move_blk:
		assert_true(not chk_move_atk.is_visible_in_tree() and not chk_move_blk.is_visible_in_tree(),
			"4.2 ARPG movement checkboxes hidden in Turn-Based mode")
		if combat_opt:
			combat_opt.select(1)
			combat_opt.emit_signal("item_selected", 1)
			await process_frame
		assert_true(chk_move_atk.is_visible_in_tree() and chk_move_blk.is_visible_in_tree(),
			"4.3 ARPG movement checkboxes visible when ARPG active")
		assert_eq(chk_move_atk.button_pressed, true, "4.4 Default 'Moverse mientras atacas' is true")
		assert_eq(chk_move_blk.button_pressed, false, "4.5 Default 'Moverse mientras bloqueas' is false")
		var lbl_spd_atk := find_label_with_text(db, "Velocidad al atacar (%)")
		var lbl_spd_blk := find_label_with_text(db, "Velocidad al bloquear (%)")
		assert_true(lbl_spd_atk != null and lbl_spd_atk.is_visible_in_tree(), "4.6 Attack speed % field visible when move_while_attacking=true")
		assert_true(lbl_spd_blk != null and not lbl_spd_blk.is_visible_in_tree(), "4.7 Block speed % field hidden when move_while_blocking=false")
		chk_move_blk.button_pressed = true
		chk_move_blk.emit_signal("toggled", true)
		await process_frame
		assert_true(lbl_spd_blk != null and lbl_spd_blk.is_visible_in_tree(), "4.8 Block speed % field becomes visible when checked")
		var fit_res: Dictionary = db.check_layout_fits_window(Vector2i(1364, 700))
		assert_true(bool(fit_res.get("fits_horizontally", false)), "4.9 Sistema layout fits 1364x700 without horizontal overflow: %s" % str(fit_res))

	db.queue_free()
	await process_frame

	# =========================================================================
	# SECTION 3: F5 / Runtime Controls (atacar LMB & bloquear RMB + UI blocks)
	# =========================================================================
	dm.set_records("actors", [kris])
	dm.set_records("arpg_weapons", [wpn_rec])
	dm.set_actor_arpg_equip(1, "weapon", 1)
	var rt_sys: Dictionary = dm.get_system_config()
	rt_sys["combat_mode"] = 1
	rt_sys["movement_directions"] = 8
	rt_sys["arpg_move_while_attacking"] = true
	rt_sys["arpg_attack_move_speed_pct"] = 100
	rt_sys["arpg_move_while_blocking"] = false
	rt_sys["arpg_block_move_speed_pct"] = 50
	dm.set_system_config(rt_sys)
	dm.save_all(data_dir)

	var map = ClassDB.instantiate("ClockGearsMap")
	map.set("map_width", 10)
	map.set("map_height", 10)
	root.add_child(map)
	await process_frame
	var player = map.get_node_or_null("RuntimePlayer")
	assert_true(player != null, "3.0 ClockGearsMap spawned RuntimePlayer")
	player.set_allow_diagonal(true)
	player.load_actor_data(1)
	player.set_grid_cell_immediate(Vector2i(4, 4), 2)
	await process_frame
	await physics_frame

	assert_true(InputMap.has_action("atacar") and InputMap.has_action("bloquear"),
		"3.1 Default ARPG input actions 'atacar' & 'bloquear' exist")
	assert_eq(player.get_action_state(), "quieto", "3.2 Player starts in quieto")

	# LMB Attack without WASD -> uses current facing (2)
	push_mouse_button(MOUSE_BUTTON_LEFT, true)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, false)
	await wait_input_flush()
	assert_eq(player.get_action_state(), "atacar", "3.3 LMB click starts 'atacar' in facing dir 2")
	assert_eq(player.get_direction(), 2, "3.4 Facing remains 2 when no WASD held")

	# Verify 1C weapon layer follows atacar
	var layers: Array = player.get_arpg_equip_layer_info()
	var wpn_visible: bool = false
	for l in layers:
		if str(l.get("slot", "")) == "weapon" and bool(l.get("visible", false)):
			wpn_visible = true
	assert_true(wpn_visible, "3.5 Equipped weapon layer visible during atacar (layers=%s)" % str(layers))

	# Pressing D + trying RMB block mid-attack must NOT change facing or interrupt attack
	push_key(KEY_D, true)
	push_mouse_button(MOUSE_BUTTON_RIGHT, true)
	await wait_input_flush()
	await physics_frame
	assert_eq(player.get_action_state(), "atacar", "3.6 Cannot block while attacking")
	assert_eq(player.get_direction(), 2, "3.7 Direction stays locked during attack")
	push_mouse_button(MOUSE_BUTTON_RIGHT, false)
	push_key(KEY_D, false)
	await wait_input_flush()

	# Finish attack animation naturally (~0.33s at 12fps for 4 frames; real headless delta can be tiny)
	for _i in range(250):
		if player.get_action_state() != "atacar":
			break
		await process_frame
	assert_eq(player.get_action_state(), "quieto", "3.8 Attack returns to 'quieto' once finished")

	# W + D held + LMB -> attacks diagonally (dir 9 = arriba_derecha)
	push_key(KEY_W, true)
	push_key(KEY_D, true)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, true)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, false)
	push_key(KEY_W, false)
	push_key(KEY_D, false)
	await wait_input_flush()
	assert_eq(player.get_action_state(), "atacar", "3.9 LMB with W+D starts 'atacar'")
	assert_eq(player.get_direction(), 9, "3.10 Attack direction resolved to 9 (arriba_derecha)")
	for _i in range(250):
		if player.get_action_state() != "atacar":
			break
		await process_frame

	# RMB held -> loops 'bloquear', facing locked, move_while_blocking=false stops movement
	player.set_grid_cell_immediate(Vector2i(4, 4), 2)
	push_mouse_button(MOUSE_BUTTON_RIGHT, true)
	await wait_input_flush()
	assert_eq(player.get_action_state(), "bloquear", "3.11 RMB held starts 'bloquear'")
	var cell_before_blk: Vector2i = player.get_grid_cell()
	push_key(KEY_D, true)
	push_mouse_button(MOUSE_BUTTON_LEFT, true)
	for _i in range(15):
		await physics_frame
	assert_eq(player.get_action_state(), "bloquear", "3.12 Cannot attack while blocking; stays 'bloquear'")
	assert_eq(player.get_direction(), 2, "3.13 Facing stays locked during block")
	assert_eq(player.get_grid_cell(), cell_before_blk, "3.14 move_while_blocking=false prevents movement during block")
	push_mouse_button(MOUSE_BUTTON_LEFT, false)
	push_key(KEY_D, false)
	push_mouse_button(MOUSE_BUTTON_RIGHT, false)
	await wait_input_flush()
	await physics_frame
	assert_eq(player.get_action_state(), "quieto", "3.15 Releasing RMB returns to 'quieto'")

	# Missing slot safe no-op (dir 4 izquierda has no atacar_izquierda or bloquear_izquierda in Kris)
	player.set_grid_cell_immediate(Vector2i(4, 4), 4)
	push_mouse_button(MOUSE_BUTTON_LEFT, true)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, false)
	await wait_input_flush()
	assert_eq(player.get_action_state(), "quieto", "3.15b Missing atacar_<dir> is a safe no-op")
	push_mouse_button(MOUSE_BUTTON_RIGHT, true)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_RIGHT, false)
	await wait_input_flush()
	assert_eq(player.get_action_state(), "quieto", "3.15c Missing bloquear_<dir> is a safe no-op")

	# F3 Debug Panel open blocks attack/block
	player.set_grid_cell_immediate(Vector2i(4, 4), 2)
	var dbg_ov = map.get_node_or_null("RuntimeDebugOverlay")
	assert_true(dbg_ov != null, "3.15d RuntimeDebugOverlay exists on map")
	if dbg_ov:
		dbg_ov.toggle_debug_panel()
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, true)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, false)
	await wait_input_flush()
	assert_eq(player.get_action_state(), "quieto", "3.15d2 F3 debug panel open blocks ARPG attack")
	if dbg_ov:
		dbg_ov.toggle_debug_panel()
	await wait_input_flush()

	# Message window open blocks attack/block
	var msg_win = ClassDB.instantiate("ClockGearsMessageWindow")
	root.add_child(msg_win)
	msg_win.show_message("Hola")
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, true)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, false)
	await wait_input_flush()
	assert_eq(player.get_action_state(), "quieto", "3.15e Message window open blocks ARPG attack")
	msg_win.queue_free()
	await wait_input_flush()

	# Click over UI Panel with MOUSE_FILTER_STOP (inside CanvasLayer HUD) must NOT trigger attack
	var ui_layer := CanvasLayer.new()
	ui_layer.layer = 10
	var ui_btn := Button.new()
	ui_btn.position = Vector2(150, 150)
	ui_btn.size = Vector2(120, 80)
	ui_btn.text = "HUD"
	ui_layer.add_child(ui_btn)
	root.add_child(ui_layer)
	await wait_input_flush()
	var mot := InputEventMouseMotion.new()
	mot.position = Vector2(200, 180)
	mot.global_position = Vector2(200, 180)
	Input.parse_input_event(mot)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, true, Vector2(200, 180))
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, false, Vector2(200, 180))
	await wait_input_flush()
	assert_eq(player.get_action_state(), "quieto", "3.16 Clicking over UI panel does NOT trigger attack")
	ui_layer.queue_free()
	await wait_input_flush()

	# Turn-Based mode -> LMB does NOT attack
	rt_sys["combat_mode"] = 0
	dm.set_system_config(rt_sys)
	push_mouse_button(MOUSE_BUTTON_LEFT, true)
	await wait_input_flush()
	push_mouse_button(MOUSE_BUTTON_LEFT, false)
	await wait_input_flush()
	assert_eq(player.get_action_state(), "quieto", "3.17 Turn-Based mode ignores ARPG attack click")

	# Movement speed settings during attack / block
	rt_sys["combat_mode"] = 1
	rt_sys["arpg_move_while_attacking"] = false
	dm.set_system_config(rt_sys)
	player.set_grid_cell_immediate(Vector2i(4, 4), 2)
	push_mouse_button(MOUSE_BUTTON_LEFT, true)
	await wait_input_flush()
	push_key(KEY_D, true)
	for _i in range(10):
		await physics_frame
	assert_eq(player.get_grid_cell(), Vector2i(4, 4), "4.10 move_while_attacking=false keeps player still during attack")
	push_key(KEY_D, false)
	push_mouse_button(MOUSE_BUTTON_LEFT, false)
	for _i in range(250):
		if player.get_action_state() != "atacar":
			break
		await process_frame

	rt_sys["arpg_move_while_blocking"] = true
	rt_sys["arpg_block_move_speed_pct"] = 50
	dm.set_system_config(rt_sys)
	player.set_grid_cell_immediate(Vector2i(4, 4), 2)
	push_mouse_button(MOUSE_BUTTON_RIGHT, true)
	await wait_input_flush()
	push_key(KEY_D, true)
	for _i in range(25):
		await physics_frame
	assert_eq(player.get_direction(), 2, "4.11 Facing remains locked when move_while_blocking=true")
	assert_true(player.get_grid_cell().x > 4 or player.is_moving_step(), "4.12 move_while_blocking=true allows moving while blocking")
	push_key(KEY_D, false)
	push_mouse_button(MOUSE_BUTTON_RIGHT, false)
	for _i in range(20):
		await physics_frame

	map.queue_free()
	await process_frame

	# Restore disk JSONs and remove temp images
	restore_json_dir(data_dir, snap)
	var abs_tmp := ProjectSettings.globalize_path("res://tmp_1b_corr")
	if DirAccess.dir_exists_absolute(abs_tmp):
		for f in DirAccess.get_files_at("res://tmp_1b_corr"):
			DirAccess.remove_absolute(abs_tmp.path_join(f))
		DirAccess.remove_absolute(abs_tmp)

	print("RESUMEN 1B CORRECTION -> PASS:%d FAIL:%d" % [passed, failed])
	quit(0 if failed == 0 else 1)
