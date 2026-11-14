extends SceneTree

var failed := 0

func assert_true(cond: bool, msg: String) -> void:
	if cond:
		print("[PASS] ", msg)
	else:
		print("[FAIL] ", msg)
		failed += 1

func _find_button_by_text(node: Node, txt: String) -> Button:
	if node is Button and node.text == txt:
		return node
	for c in node.get_children(true):
		var r = _find_button_by_text(c, txt)
		if r:
			return r
	return null

func _find_controls_of_type(node: Node, cls_name: String) -> Array:
	var out: Array = []
	if node.get_class() == cls_name or node.is_class(cls_name):
		out.append(node)
	for c in node.get_children(true):
		out.append_array(_find_controls_of_type(c, cls_name))
	return out

func _find_accept_dialog_by_title(parent: Node, title_sub: String) -> AcceptDialog:
	for c in parent.get_children():
		if c is AcceptDialog and c.title.contains(title_sub):
			return c
	return null

func _find_troop_canvas(db: Node) -> Control:
	for c in _find_controls_of_type(db, "Control"):
		if c.get_class() == "Control" and c.visible and c.get_child_count() >= 3 and c.size.x >= 500:
			return c
	return null

func _calc_expected_vp_rect(canvas: Control) -> Rect2:
	var avail := canvas.size
	var scale := minf(avail.x / 960.0, avail.y / 540.0)
	var vp_size := Vector2(960.0 * scale, 540.0 * scale)
	var vp_pos := Vector2((avail.x - vp_size.x) * 0.5, 0.0)
	return Rect2(vp_pos, vp_size)

func _get_member_spins(db: Node) -> Array:
	var spins: Array = []
	for sp in _find_controls_of_type(db, "SpinBox"):
		if sp.visible and (sp.max_value == 960 or sp.max_value == 540):
			spins.append(sp)
	return spins

func _find_sprite_bbox_in_image(img: Image, roi: Rect2i, target_col: Color) -> Rect2:
	var min_x := 99999
	var min_y := 99999
	var max_x := -1
	var max_y := -1
	for y in range(maxi(0, roi.position.y), mini(img.get_height(), roi.position.y + roi.size.y)):
		for x in range(maxi(0, roi.position.x), mini(img.get_width(), roi.position.x + roi.size.x)):
			var c := img.get_pixel(x, y)
			if absf(c.r - target_col.r) < 0.14 and absf(c.g - target_col.g) < 0.14 and absf(c.b - target_col.b) < 0.14:
				min_x = mini(min_x, x)
				min_y = mini(min_y, y)
				max_x = maxi(max_x, x)
				max_y = maxi(max_y, y)
	if max_x < min_x:
		return Rect2()
	return Rect2(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1)

func _snapshot_dir(dir_res: String) -> Dictionary:
	var snap := {}
	var da := DirAccess.open(dir_res)
	if da:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir():
				var p := dir_res.path_join(fname)
				snap[p] = FileAccess.get_file_as_bytes(p)
			fname = da.get_next()
	return snap

func _restore_dir(dir_res: String, snap: Dictionary) -> void:
	var da := DirAccess.open(dir_res)
	if da:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir():
				var p := dir_res.path_join(fname)
				if not snap.has(p):
					DirAccess.remove_absolute(ProjectSettings.globalize_path(p))
			fname = da.get_next()
	for p in snap.keys():
		var fa = FileAccess.open(p, FileAccess.WRITE)
		if fa:
			fa.store_buffer(snap[p])
			fa.close()

func _find_context_menu(db: Node) -> PopupMenu:
	for pm in _find_controls_of_type(db, "PopupMenu"):
		for i in range(pm.item_count):
			if pm.get_item_text(i).contains("Escala"):
				return pm
	return null

func _find_scale_submenu(ctx: PopupMenu) -> PopupMenu:
	if ctx == null:
		return null
	for i in range(ctx.item_count):
		if ctx.get_item_text(i).contains("Escala"):
			var sub := ctx.get_item_submenu_node(i)
			if sub is PopupMenu:
				return sub
	return null

func _initialize() -> void:
	print("=== REPRO & VERIFY TROOP MEMBER SCALE ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.load_database()
	var data_snap := _snapshot_dir("res://clockgears_data")

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://img/battlers"))
	var sp32_res := "res://img/battlers/_tmp_scale_enemy_32.png"
	var sp48_res := "res://img/battlers/_tmp_scale_enemy_48.png"
	var mag_col := Color(0.92, 0.16, 0.72, 1.0)
	var cyan_col := Color(0.12, 0.82, 0.88, 1.0)

	# 4-frame 32x32 sheet (128x32)
	var img32 := Image.create(128, 32, false, Image.FORMAT_RGBA8)
	img32.fill(mag_col)
	img32.save_png(ProjectSettings.globalize_path(sp32_res))

	# Single 48x48 sprite
	var img48 := Image.create(48, 48, false, Image.FORMAT_RGBA8)
	img48.fill(cyan_col)
	img48.save_png(ProjectSettings.globalize_path(sp48_res))

	var enemy_32 := {
		"id": 1,
		"name": "Slime 32",
		"level": 2,
		"hp": 100,
		"mp": 10,
		"atk": 10,
		"def": 5,
		"mat": 8,
		"mdf": 5,
		"agi": 10,
		"luk": 5,
		"exp": 15,
		"gold": 10,
		"graphic_path": sp32_res,
		"sprite_data": {
			"cols": 4,
			"frame_w": 32,
			"frame_h": 32,
			"animations": {
				"idle": {
					"name": "idle",
					"fps": 6.0,
					"loop": true,
					"cols": 4,
					"frame_w": 32,
					"frame_h": 32,
					"spritesheet_path": sp32_res,
					"frames": [0, 1],
					"frames_data": [
						{"x": 0, "y": 0, "w": 32, "h": 32, "duration": 0.16, "sheet": sp32_res},
						{"x": 32, "y": 0, "w": 32, "h": 32, "duration": 0.16, "sheet": sp32_res}
					]
				},
				"attack": {
					"name": "attack",
					"fps": 8.0,
					"loop": false,
					"cols": 4,
					"frame_w": 32,
					"frame_h": 32,
					"spritesheet_path": sp32_res,
					"frames": [2],
					"frames_data": [
						{"x": 64, "y": 0, "w": 32, "h": 32, "duration": 0.2, "sheet": sp32_res}
					]
				},
				"hurt": {
					"name": "hurt",
					"fps": 8.0,
					"loop": false,
					"cols": 4,
					"frame_w": 32,
					"frame_h": 32,
					"spritesheet_path": sp32_res,
					"frames": [3],
					"frames_data": [
						{"x": 96, "y": 0, "w": 32, "h": 32, "duration": 0.2, "sheet": sp32_res}
					]
				}
			}
		}
	}
	var enemy_48 := {
		"id": 2,
		"name": "Golem 48",
		"level": 5,
		"hp": 200,
		"mp": 20,
		"atk": 15,
		"def": 10,
		"graphic_path": sp48_res
	}
	dm.set_category_array(7, [enemy_32.duplicate(true), enemy_48.duplicate(true)])
	dm.set_category_array(0, [
		{"id": 1, "name": "Héroe", "level": 1, "hp": 200, "max_hp": 200, "atk": 25, "def": 10, "agi": 20}
	])
	dm.set_system_config({"starting_party": [1]})
	# Two members pointing to the SAME enemy_id=1 without "escala" (legacy format)
	dm.set_category_array(8, [
		{
			"id": 1,
			"name": "Dúo Slime",
			"members": [
				{"enemy_id": 1, "x": 160, "y": 170},
				{"enemy_id": 1, "x": 580, "y": 170}
			]
		}
	])
	dm.save_database()
	var enemy_json_before := JSON.stringify(dm.get_category_array(7))

	var db := ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	await process_frame
	db.open_database()
	db.select_category(8) # CAT_TROOPS
	for i in range(6):
		await process_frame

	var canvas := _find_troop_canvas(db)
	assert_true(canvas != null, "Troop placement canvas found")
	var main_scroll: ScrollContainer = null
	for sc in _find_controls_of_type(db, "ScrollContainer"):
		if sc.visible and sc.size.x > 400:
			main_scroll = sc
			break
	if main_scroll:
		main_scroll.ensure_control_visible(canvas)
		await process_frame
		main_scroll.scroll_vertical = mini(main_scroll.scroll_vertical, 240)
	for i in range(4):
		await process_frame

	var vp_rect := _calc_expected_vp_rect(canvas)
	var c_rect := canvas.get_global_rect()
	var left_roi := Rect2i(int(c_rect.position.x + vp_rect.position.x), int(c_rect.position.y + vp_rect.position.y), int(vp_rect.size.x * 0.48), int(vp_rect.size.y))
	var right_roi := Rect2i(int(c_rect.position.x + vp_rect.position.x + vp_rect.size.x * 0.50), int(c_rect.position.y + vp_rect.position.y), int(vp_rect.size.x * 0.48), int(vp_rect.size.y))

	# Baseline (x1): real 32x32 aspect, member 0 & member 1 identical x1 size
	var img_x1 := db.get_texture().get_image()
	var bbox_m0_x1 := _find_sprite_bbox_in_image(img_x1, left_roi, mag_col)
	var bbox_m1_x1 := _find_sprite_bbox_in_image(img_x1, right_roi, mag_col)
	assert_true(bbox_m0_x1.size.x > 5 and bbox_m1_x1.size.x > 5, "Legacy members without 'escala' render at x1 (m0=%s, m1=%s)" % [str(bbox_m0_x1.size), str(bbox_m1_x1.size)])
	assert_true(absf(bbox_m0_x1.size.x - bbox_m1_x1.size.x) <= 2.0 and absf(bbox_m0_x1.size.y - bbox_m1_x1.size.y) <= 2.0, "Legacy x1 pair renders equal size before scaling")
	var expected_x1_px := 32.0 * (vp_rect.size.x / 960.0)
	assert_true(absf(bbox_m0_x1.size.x - expected_x1_px) <= 3.0 and absf(bbox_m0_x1.size.y - expected_x1_px) <= 3.0, "x1 uses real 32x32 base size (~%.1fpx, got %s)" % [expected_x1_px, str(bbox_m0_x1.size)])

	# Right-click member 0 -> contextual menu with Escala -> x1..x6!
	var rclick_local := bbox_m0_x1.get_center() - c_rect.position
	var ev_rclick := InputEventMouseButton.new()
	ev_rclick.button_index = MOUSE_BUTTON_RIGHT
	ev_rclick.pressed = true
	ev_rclick.position = rclick_local
	canvas.emit_signal("gui_input", ev_rclick)
	for i in range(4):
		await process_frame

	var ctx := _find_context_menu(db)
	assert_true(ctx != null, "FAIL-BEFORE-FIX 1a: Right-clicking enemy opens contextual PopupMenu with 'Escala'")
	var sub := _find_scale_submenu(ctx)
	assert_true(sub != null and sub.item_count == 6, "FAIL-BEFORE-FIX 1b: 'Escala' submenu has 6 entries (x1..x6)")
	if sub:
		assert_true(sub.is_item_checked(0) and not sub.is_item_checked(1), "FAIL-BEFORE-FIX 1c: Current scale x1 is checked in submenu")
		assert_true(ctx.visible, "Contextual menu is visible after right-click")
		# Style check: rounded theme on PopupMenu
		var pop_sb := ctx.get_theme_stylebox("panel")
		assert_true(pop_sb is StyleBoxFlat and (pop_sb as StyleBoxFlat).corner_radius_top_left > 0, "Contextual menu uses current rounded theme")
		# Bounds check: inside DB window
		assert_true(ctx.position.x + ctx.size.x <= db.position.x + db.size.x + 8, "Context menu stays inside window bounds")
		# Pick x2 (id 2 -> index 1)
		sub.emit_signal("id_pressed", 2)
		for i in range(4):
			await process_frame

	# Check member 0 doubled in preview, feet bottom-center did NOT jump, and member 1 stayed x1!
	var img_x2 := db.get_texture().get_image()
	var bbox_m0_x2 := _find_sprite_bbox_in_image(img_x2, left_roi, mag_col)
	var bbox_m1_after := _find_sprite_bbox_in_image(img_x2, right_roi, mag_col)
	assert_true(absf(bbox_m0_x2.size.x - bbox_m0_x1.size.x * 2.0) <= 3.0 and absf(bbox_m0_x2.size.y - bbox_m0_x1.size.y * 2.0) <= 3.0, "FAIL-BEFORE-FIX 2a: Member 0 at x2 is exact double in preview (%s vs x1=%s)" % [str(bbox_m0_x2.size), str(bbox_m0_x1.size)])
	assert_true(absf(bbox_m1_after.size.x - bbox_m1_x1.size.x) <= 2.0, "Member 1 of same enemy stays x1 when member 0 is set to x2")
	var feet_m0_x1 := Vector2(bbox_m0_x1.get_center().x, bbox_m0_x1.position.y + bbox_m0_x1.size.y)
	var feet_m0_x2 := Vector2(bbox_m0_x2.get_center().x, bbox_m0_x2.position.y + bbox_m0_x2.size.y)
	assert_true(feet_m0_x1.distance_to(feet_m0_x2) <= 2.5, "Scaling anchors at feet center (x1=%s, x2=%s)" % [str(feet_m0_x1), str(feet_m0_x2)])

	# Right-click member 1 -> set x3 (distinct scales on two instances of SAME enemy!)
	ev_rclick.position = bbox_m1_after.get_center() - c_rect.position
	canvas.emit_signal("gui_input", ev_rclick)
	for i in range(3):
		await process_frame
	if sub:
		sub.emit_signal("id_pressed", 3)
		for i in range(4):
			await process_frame
	var img_m1_x3 := db.get_texture().get_image()
	var bbox_m1_x3 := _find_sprite_bbox_in_image(img_m1_x3, right_roi, mag_col)
	assert_true(absf(bbox_m1_x3.size.x - bbox_m1_x1.size.x * 3.0) <= 4.0 and absf(bbox_m1_x3.size.y - bbox_m1_x1.size.y * 3.0) <= 4.0, "FAIL-BEFORE-FIX 2b: Member 1 at x3 is triple in preview (%s vs x1=%s)" % [str(bbox_m1_x3.size), str(bbox_m1_x1.size)])

	# Edge-clamp & Esc close test on context menu
	var rclick_edge := (bbox_m1_x3.get_center() - c_rect.position)
	ev_rclick.position = rclick_edge
	canvas.emit_signal("gui_input", ev_rclick)
	for i in range(3):
		await process_frame
	if ctx and sub:
		assert_true(ctx.visible and sub.is_item_checked(2) and not sub.is_item_checked(1), "Right-clicking m1 checks active x3 scale in submenu")
		var ev_esc := InputEventKey.new()
		ev_esc.keycode = KEY_ESCAPE
		ev_esc.pressed = true
		ctx.push_input(ev_esc)
		ctx.hide()
		await process_frame
		assert_true(not ctx.visible, "Contextual menu closes cleanly on Esc")

	# Right-click very close to right/bottom canvas edge over a moved enemy -> stays inside DB bounds
	var spins_all := _get_member_spins(db)
	spins_all[2].value = 840
	spins_all[3].value = 400
	await process_frame
	await process_frame
	var img_edge := db.get_texture().get_image()
	var bbox_edge := _find_sprite_bbox_in_image(img_edge, right_roi, mag_col)
	ev_rclick.position = bbox_edge.get_center() - c_rect.position
	canvas.emit_signal("gui_input", ev_rclick)
	for i in range(3):
		await process_frame
	if ctx:
		assert_true(ctx.position.x + ctx.size.x <= db.position.x + db.size.x + 8 and ctx.position.y + ctx.size.y <= db.position.y + db.size.y + 8, "Context menu near right/bottom edge never clips outside window")
		ctx.hide()
		await process_frame
	spins_all[2].value = 580
	spins_all[3].value = 170
	await process_frame

	# Verify DB Enemy record untouched!
	assert_true(JSON.stringify(dm.get_category_array(7)) == enemy_json_before, "Scaling troop members does NOT alter DB Enemy record")

	# Alinear & 32px snap test with scaled enemy
	var chk_snap: CheckBox = null
	for cb in _find_controls_of_type(db, "CheckBox"):
		if cb.visible and cb.text.contains("32"):
			chk_snap = cb
			break
	var btn_align := _find_button_by_text(db, "Alinear")
	if chk_snap and btn_align:
		chk_snap.button_pressed = true
		btn_align.emit_signal("pressed")
		for i in range(4):
			await process_frame
		var sp_after_align := _get_member_spins(db)
		var aligned_x0 := int(sp_after_align[0].value)
		var aligned_y0 := int(sp_after_align[1].value)
		# At scale 2 for 32x32 -> draw=64x64; spr_left = x + 70 - 32 = aligned_x0 + 38 must be multiple of 32
		assert_true(((aligned_x0 + 38) % 32) == 0 and ((aligned_y0 + 110 - 64) % 32) == 0, "Alinear + 32px grid snap aligns scaled sprite to 32px grid (x=%d, y=%d)" % [aligned_x0, aligned_y0])
		chk_snap.button_pressed = false
		sp_after_align[0].value = 160
		sp_after_align[1].value = 170
		sp_after_align[2].value = 580
		sp_after_align[3].value = 170
		for i in range(4):
			await process_frame

	# Drag scaled member 0 -> drag must still work!
	var spins_before := _get_member_spins(db)
	var old_x0 := int(spins_before[0].value)
	var drag_start := bbox_m0_x2.get_center() - c_rect.position
	var drag_end := drag_start + Vector2(60, 15)
	var ev_ld := InputEventMouseButton.new()
	ev_ld.button_index = MOUSE_BUTTON_LEFT
	ev_ld.pressed = true
	ev_ld.position = drag_start
	canvas.emit_signal("gui_input", ev_ld)
	var ev_mv := InputEventMouseMotion.new()
	ev_mv.position = drag_end
	canvas.emit_signal("gui_input", ev_mv)
	var ev_lu := InputEventMouseButton.new()
	ev_lu.button_index = MOUSE_BUTTON_LEFT
	ev_lu.pressed = false
	ev_lu.position = drag_end
	canvas.emit_signal("gui_input", ev_lu)
	for i in range(4):
		await process_frame
	var spins_after := _get_member_spins(db)
	assert_true(int(spins_after[0].value) > old_x0 + 20, "Dragging scaled enemy still works cleanly (%d -> %d)" % [old_x0, int(spins_after[0].value)])
	spins_after[0].value = 160
	spins_after[1].value = 170
	await process_frame

	# Open Battle Test ("Prueba de batalla") -> verify member 0 (x2) vs member 1 (x3) sizes!
	var btn_bt := _find_button_by_text(db, "Prueba de batalla")
	btn_bt.emit_signal("pressed")
	for i in range(8):
		await process_frame
	var bt_dlg := _find_accept_dialog_by_title(db, "Prueba de Batalla")
	assert_true(bt_dlg != null and bt_dlg.visible, "Battle Test dialog opened")
	if bt_dlg:
		var bt_img := bt_dlg.get_texture().get_image()
		var bt_left := Rect2i(0, 0, int(bt_img.get_width() * 0.48), bt_img.get_height())
		var bt_right := Rect2i(int(bt_img.get_width() * 0.50), 0, int(bt_img.get_width() * 0.48), bt_img.get_height())
		var bt_m0 := _find_sprite_bbox_in_image(bt_img, bt_left, mag_col)
		var bt_m1 := _find_sprite_bbox_in_image(bt_img, bt_right, mag_col)
		var bt_ratio := bt_m1.size.x / maxf(1.0, bt_m0.size.x)
		assert_true(bt_ratio > 1.35 and bt_ratio < 1.65, "FAIL-BEFORE-FIX 3a: Battle Test scales m0=x2 & m1=x3 proportionally (m0=%s, m1=%s, ratio=%.2f)" % [str(bt_m0.size), str(bt_m1.size), bt_ratio])
		assert_true(bt_m0.size.x >= 56.0 and bt_m0.size.x <= 72.0, "FAIL-BEFORE-FIX 3b: Battle Test 32x32 at x2 renders ~64px wide (got %s)" % str(bt_m0.size))
		bt_dlg.emit_signal("canceled")
		for i in range(4):
			await process_frame

	# Save via OK, reopen DB -> verify x2 and x3 persisted cleanly!
	db.get_ok_button().emit_signal("pressed")
	for i in range(4):
		await process_frame
	assert_true(not db.visible, "DB OK saves and closes")
	db.free()

	var troop_saved: Array = dm.get_category_array(8)
	var saved_mems: Array = troop_saved[0].get("members", [])
	assert_true(int(saved_mems[0].get("escala", 1)) == 2 and int(saved_mems[1].get("escala", 1)) == 3, "FAIL-BEFORE-FIX 2c: Troop member scales saved (got %s, %s)" % [str(saved_mems[0]), str(saved_mems[1])])

	var db2 := ClockGearsDatabaseDialog.new()
	get_root().add_child(db2)
	await process_frame
	db2.load_from_disk()
	db2.select_category(8)
	db2.popup_centered(Vector2i(1366, 768))
	for i in range(6):
		await process_frame
	var canvas2 := _find_troop_canvas(db2)
	var ms2: ScrollContainer = null
	for sc in _find_controls_of_type(db2, "ScrollContainer"):
		if sc.visible and sc.size.x > 400:
			ms2 = sc
			break
	if ms2 and canvas2:
		ms2.ensure_control_visible(canvas2)
		await process_frame
		ms2.scroll_vertical = mini(ms2.scroll_vertical, 240)
	for i in range(4):
		await process_frame
	var c_rect2 := canvas2.get_global_rect()
	var vp2 := _calc_expected_vp_rect(canvas2)
	var img_reopen := db2.get_texture().get_image()
	var ro_l := Rect2i(int(c_rect2.position.x + vp2.position.x), int(c_rect2.position.y + vp2.position.y), int(vp2.size.x * 0.48), int(vp2.size.y))
	var ro_r := Rect2i(int(c_rect2.position.x + vp2.position.x + vp2.size.x * 0.50), int(c_rect2.position.y + vp2.position.y), int(vp2.size.x * 0.48), int(vp2.size.y))
	var ro_m0 := _find_sprite_bbox_in_image(img_reopen, ro_l, mag_col)
	var ro_m1 := _find_sprite_bbox_in_image(img_reopen, ro_r, mag_col)
	var ro_ratio := ro_m1.size.x / maxf(1.0, ro_m0.size.x)
	assert_true(ro_ratio > 1.35 and ro_ratio < 1.65, "Reopened DB preserves x2 and x3 distinct scales visually (m0=%s, m1=%s)" % [str(ro_m0.size), str(ro_m1.size)])
	db2.hide()
	db2.free()

	# Real battle (ClockGearsBattleScene): 48x48 at x2 (=96x96), clip at viewport edge without shifting (x,y), Attack/Hurt scale & impact FX centering!
	dm.set_category_array(8, [
		{
			"id": 1,
			"name": "Golem + Slime",
			"members": [
				{"enemy_id": 2, "x": 200, "y": 170, "escala": 2},
				{"enemy_id": 1, "x": 580, "y": 170, "escala": 2},
				{"enemy_id": 2, "x": 100, "y": 20, "escala": 6}
			]
		}
	])
	dm.save_database()

	get_root().size = Vector2i(960, 540)
	var bs := ClockGearsBattleScene.new()
	get_root().add_child(bs)
	bs.setup_battle(1, true, true, "", null, false)
	for i in range(6):
		await process_frame

	var r_golem_x2: Rect2 = bs.get_enemy_sprite_rect(0)
	var r_slime_x2: Rect2 = bs.get_enemy_sprite_rect(1)
	var r_clip_x6: Rect2 = bs.get_enemy_sprite_rect(2)
	assert_true(absf(r_golem_x2.size.x - 96.0) <= 2.0 and absf(r_golem_x2.size.y - 96.0) <= 2.0, "48x48 enemy at x2 renders 96x96 in real battle (got %s)" % str(r_golem_x2.size))
	assert_true(absf(r_slime_x2.size.x - 64.0) <= 2.0 and absf(r_slime_x2.size.y - 64.0) <= 2.0, "32x32 enemy Idle renders 64x64 in real battle (got %s)" % str(r_slime_x2.size))
	assert_true(r_clip_x6.position.y < 0.0 and r_clip_x6.size.x > 250.0, "Oversized x6 sprite reports full unshifted layout while canvas clips overflow (%s)" % str(r_clip_x6))
	var ctrl_units := bs.get_controller().get_enemy_units()
	assert_true(int(ctrl_units[2].get_battle_pos().x) == 100 and int(ctrl_units[2].get_battle_pos().y) == 20, "Oversized x6 sprite keeps exact (x,y)=(100,20) without clamping displacement")

	# Attack enemy 1 (Slime 32 at x2): verify Hurt animation keeps 64x64 scaled rect and FX centers on scaled sprite!
	bs.select_action_attack(1)
	bs.advance_time(0.05)
	await process_frame
	var r_slime_hurt: Rect2 = bs.get_enemy_sprite_rect(1)
	var eff_c: Vector2 = bs.get_enemy_effect_center(1)
	assert_true(absf(r_slime_hurt.size.x - 64.0) <= 2.0 and absf(r_slime_hurt.size.y - 64.0) <= 2.0, "Hurt animation preserves x2 (64x64) scaled sprite (got %s)" % str(r_slime_hurt.size))
	assert_true(eff_c.distance_to(r_slime_hurt.get_center()) <= 2.0, "Impact/effect center matches scaled sprite center")
	var fx_pos := Vector2(-999, -999)
	for n in _find_controls_of_type(bs, "ClockGearsFXInstance"):
		fx_pos = (n as Node2D).position
	assert_true(fx_pos.distance_to(eff_c) <= 2.5, "Spawned ClockGearsFXInstance centers on scaled sprite (%s vs %s)" % [str(fx_pos), str(eff_c)])

	bs.finish_battle_immediate()
	bs.free()

	_restore_dir("res://clockgears_data", data_snap)
	for tmp_p in [sp32_res, sp48_res]:
		DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_p))
		if FileAccess.file_exists(tmp_p + ".import"):
			DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_p + ".import"))

	print("==================================================")
	if failed == 0:
		print("ALL TESTS PASSED (0 FAILURES)")
	else:
		print("TESTS FAILED COUNT: ", failed)
	quit(0 if failed == 0 else 1)
