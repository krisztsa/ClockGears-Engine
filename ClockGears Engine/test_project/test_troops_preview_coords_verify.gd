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
		if sp.visible and sp.max_value == 960 or sp.max_value == 540:
			spins.append(sp)
	return spins

func _find_accept_dialog_by_title(parent: Node, title_sub: String) -> AcceptDialog:
	for c in parent.get_children():
		if c is AcceptDialog and c.title.contains(title_sub):
			return c
	return null

func _find_sprite_bbox_in_image(img: Image, roi: Rect2i, target_col: Color) -> Rect2:
	var min_x := 99999
	var min_y := 99999
	var max_x := -1
	var max_y := -1
	for y in range(maxi(0, roi.position.y), mini(img.get_height(), roi.position.y + roi.size.y)):
		for x in range(maxi(0, roi.position.x), mini(img.get_width(), roi.position.x + roi.size.x)):
			var c := img.get_pixel(x, y)
			if absf(c.r - target_col.r) < 0.12 and absf(c.g - target_col.g) < 0.12 and absf(c.b - target_col.b) < 0.12:
				min_x = mini(min_x, x)
				min_y = mini(min_y, y)
				max_x = maxi(max_x, x)
				max_y = maxi(max_y, y)
	if max_x < min_x:
		return Rect2()
	return Rect2(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1)

func _initialize() -> void:
	print("=== REPRO & VERIFY TROOPS PREVIEW / COORDS / ALINEAR ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.load_database()
	var data_snap := _snapshot_dir("res://clockgears_data")

	# Create distinctive test battleback (teal background) & 4-frame sprite (bright magenta)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://img/battlebacks"))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://img/battlers"))
	var bb_res := "res://img/battlebacks/_tmp_verify_bb.png"
	var sp_res := "res://img/battlers/_tmp_verify_enemy.png"
	var bb_color := Color(0.12, 0.55, 0.58, 1.0)
	var sp_color := Color(0.92, 0.18, 0.65, 1.0)

	var bb_img := Image.create(320, 180, false, Image.FORMAT_RGBA8)
	bb_img.fill(bb_color)
	bb_img.save_png(ProjectSettings.globalize_path(bb_res))

	var sp_img := Image.create(256, 64, false, Image.FORMAT_RGBA8)
	sp_img.fill(Color(0, 0, 0, 0))
	for py in range(8, 56):
		for px in range(8, 56):
			sp_img.set_pixel(px, py, sp_color)
	sp_img.save_png(ProjectSettings.globalize_path(sp_res))

	dm.set_category_array(0, [
		{"id": 1, "name": "Héroe", "initial_level": 1, "exp": 0, "hp": 250, "mp": 50, "atk": 60, "def": 10, "agi": 25}
	])
	dm.set_category_array(7, [
		{
			"id": 1,
			"name": "Cañón musgoso",
			"level": 1,
			"graphic_path": sp_res,
			"hp": 100, "mp": 10, "atk": 5, "def": 2, "agi": 5,
			"sprite_data": {
				"cols": 4, "rows": 1, "frame_w": 64, "frame_h": 64,
				"animations": {
					"Idle": { "name": "Idle", "fps": 6.0, "loop": true, "spritesheet_path": sp_res, "frames": [0] }
				}
			}
		},
		{ "id": 2, "name": "Sin Gráfico", "level": 2, "hp": 50, "mp": 0, "atk": 5, "def": 2, "agi": 5 }
	])
	var sys: Dictionary = dm.get_system_config()
	sys["party"] = [1]
	dm.set_system_config(sys)
	dm.set_category_array(8, [
		{
			"id": 1,
			"name": "Cañón musgoso",
			"battleback1": bb_res,
			"members": [ { "enemy_id": 1, "x": 395, "y": 178 } ]
		},
		{
			"id": 2,
			"name": "Sin Fondo",
			"battleback1": "",
			"members": [ { "enemy_id": 2, "x": 200, "y": 150 } ]
		},
		{
			"id": 3,
			"name": "Grupo Triple",
			"battleback1": "",
			"members": [
				{ "enemy_id": 1, "x": 120, "y": 120 },
				{ "enemy_id": 1, "x": 280, "y": 160 },
				{ "enemy_id": 1, "x": 440, "y": 120 }
			]
		}
	])
	dm.save_database()

	var db := ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	await process_frame
	db.load_from_disk()
	db.select_category(8) # CAT_TROOPS
	db.popup_centered(Vector2i(1366, 768))
	for i in range(8):
		await process_frame

	# 1. Check preview NOT white & battleback + enemy sprite rendered with real colors
	var canvas := _find_troop_canvas(db)
	assert_true(canvas != null, "Tropa 001 preview canvas found")
	var vp_rect := _calc_expected_vp_rect(canvas)
	var g_vp_pos := canvas.global_position + vp_rect.position
	# Ensure preview is scrolled into view so pixels can be sampled on db window
	var scrolls := _find_controls_of_type(db, "ScrollContainer")
	var main_scroll: ScrollContainer = null
	for sc in scrolls:
		if sc.visible and sc.size.x > 400:
			main_scroll = sc
			break
	if main_scroll:
		main_scroll.ensure_control_visible(canvas)
		await process_frame
		main_scroll.scroll_vertical = mini(main_scroll.scroll_vertical, 240)
	for i in range(4):
		await process_frame
	var db_img := db.get_texture().get_image()
	var c_rect := canvas.get_global_rect()
	var prev_roi := Rect2i(int(c_rect.position.x + vp_rect.position.x), int(c_rect.position.y + vp_rect.position.y), int(vp_rect.size.x), int(vp_rect.size.y))
	# Background sample near top-left of preview
	var bg_sample := db_img.get_pixel(prev_roi.position.x + 25, prev_roi.position.y + 25)
	assert_true(not (bg_sample.r > 0.95 and bg_sample.g > 0.95 and bg_sample.b > 0.95), "FAIL-BEFORE-FIX 1a: Preview background is NOT solid white (got %s)" % str(bg_sample))
	assert_true(absf(bg_sample.g - bb_color.g) < 0.15 and absf(bg_sample.b - bb_color.b) < 0.15, "FAIL-BEFORE-FIX 1b: Preview background matches battleback teal color (got %s)" % str(bg_sample))

	# Detect sprite bounding box in preview image
	var prev_bbox := _find_sprite_bbox_in_image(db_img, prev_roi, sp_color)
	assert_true(prev_bbox.size.x > 10.0 and prev_bbox.size.y > 10.0, "FAIL-BEFORE-FIX 1c: Preview enemy magenta sprite is visibly drawn (bbox=%s)" % str(prev_bbox))

	# 2. Check Battle Test sprite relative center & size match Preview!
	var btn_bt := _find_button_by_text(db, "Prueba de batalla")
	assert_true(btn_bt != null, "Prueba de batalla button found")
	btn_bt.emit_signal("pressed")
	for i in range(8):
		await process_frame
	var bt_dlg := _find_accept_dialog_by_title(db, "Prueba de Batalla")
	assert_true(bt_dlg != null and bt_dlg.visible, "Battle test window opened")
	var bt_img := bt_dlg.get_texture().get_image()
	var bt_size := bt_img.get_size()
	var bt_bbox := _find_sprite_bbox_in_image(bt_img, Rect2i(0, 0, bt_size.x, bt_size.y), sp_color)
	assert_true(bt_bbox.size.x > 10.0 and bt_bbox.size.y > 10.0, "Battle Test enemy magenta sprite found (bbox=%s)" % str(bt_bbox))

	var prev_rel_center := Vector2((prev_bbox.get_center().x - prev_roi.position.x) / vp_rect.size.x, (prev_bbox.get_center().y - prev_roi.position.y) / vp_rect.size.y)
	var bt_rel_center := Vector2(bt_bbox.get_center().x / float(bt_size.x), bt_bbox.get_center().y / float(bt_size.y))
	print("   Compare centers -> Preview rel=%s vs BattleTest rel=%s" % [str(prev_rel_center), str(bt_rel_center)])
	assert_true(prev_rel_center.distance_to(bt_rel_center) < 0.03, "FAIL-BEFORE-FIX 2a: Preview and Battle Test place enemy at SAME relative position (diff=%f)" % prev_rel_center.distance_to(bt_rel_center))

	var prev_rel_size := Vector2(prev_bbox.size.x / vp_rect.size.x, prev_bbox.size.y / vp_rect.size.y)
	var bt_rel_size := Vector2(bt_bbox.size.x / float(bt_size.x), bt_bbox.size.y / float(bt_size.y))
	print("   Compare sprite sizes -> Preview rel=%s vs BattleTest rel=%s" % [str(prev_rel_size), str(bt_rel_size)])
	assert_true(absf(prev_rel_size.x - bt_rel_size.x) < 0.025 and absf(prev_rel_size.y - bt_rel_size.y) < 0.025, "FAIL-BEFORE-FIX 2b: Preview and Battle Test sprite scaled size matches")

	bt_dlg.emit_signal("canceled")
	for i in range(4):
		await process_frame

	# 3. Check troop WITHOUT background -> dark blue (never white) + "?" marker (never white square)
	var rec_list: ItemList = _find_controls_of_type(db, "ItemList")[0]
	rec_list.select(1)
	rec_list.emit_signal("item_selected", 1)
	for i in range(4):
		await process_frame
	# Verify scroll reset to top when selecting troop!
	if main_scroll:
		assert_true(main_scroll.scroll_vertical == 0, "FAIL-BEFORE-FIX 5: Selecting troop resets col2 scroll_vertical to 0 (got %d)" % main_scroll.scroll_vertical)
		main_scroll.ensure_control_visible(canvas)
		await process_frame
		main_scroll.scroll_vertical = mini(main_scroll.scroll_vertical, 240)
	for i in range(4):
		await process_frame
	var img_nobg := db.get_texture().get_image()
	var c_rect2 := canvas.get_global_rect()
	var nobg_px := img_nobg.get_pixel(int(c_rect2.position.x + vp_rect.position.x + 25), int(c_rect2.position.y + vp_rect.position.y + 25))
	assert_true(nobg_px.r < 0.15 and nobg_px.b > nobg_px.r, "No-background preview is dark blue, never white (got %s)" % str(nobg_px))

	# 3b. Check bottom HUD strip is darkened vs top strip & placeholder "?" silhouette is NOT white square
	var hud_px := img_nobg.get_pixel(int(c_rect2.position.x + vp_rect.position.x + 25), int(c_rect2.position.y + vp_rect.position.y + vp_rect.size.y * 0.78))
	var top_lum := nobg_px.r + nobg_px.g + nobg_px.b
	var hud_lum := hud_px.r + hud_px.g + hud_px.b
	assert_true(hud_lum < top_lum, "Bottom HUD strip is darker than upper arena (top=%s hud=%s)" % [str(nobg_px), str(hud_px)])

	# Placeholder silhouette around (200, 150) has silhouette/outline/'?' pixels, NOT solid white box
	var ph_center_x := int(c_rect2.position.x + vp_rect.position.x + (270.0 / 960.0) * vp_rect.size.x)
	var ph_center_y := int(c_rect2.position.y + vp_rect.position.y + (205.0 / 540.0) * vp_rect.size.y)
	var ph_white_count := 0
	var ph_sil_count := 0
	for py in range(ph_center_y - 35, ph_center_y + 35):
		for px in range(ph_center_x - 35, ph_center_x + 35):
			var pc := img_nobg.get_pixel(px, py)
			if pc.r > 0.95 and pc.g > 0.95 and pc.b > 0.95:
				ph_white_count += 1
			elif absf(pc.r - nobg_px.r) + absf(pc.g - nobg_px.g) + absf(pc.b - nobg_px.b) > 0.12:
				ph_sil_count += 1
	assert_true(ph_white_count < 200 and ph_sil_count > 40, "No-graphic enemy draws '?' silhouette instead of solid white box (white=%d sil=%d)" % [ph_white_count, ph_sil_count])

	# 3c. Hover over bottom HUD strip sets warning tooltip, and dropping inside HUD shows non-blocking warning label
	var ev_hover := InputEventMouseMotion.new()
	ev_hover.position = vp_rect.position + Vector2(vp_rect.size.x * 0.5, vp_rect.size.y - 30.0)
	canvas.emit_signal("gui_input", ev_hover)
	assert_true(canvas.tooltip_text.contains("HUD"), "Hovering bottom HUD strip sets explanatory tooltip ('%s')" % canvas.tooltip_text)

	var spins_t2 := _get_member_spins(db)
	spins_t2[1].value = 390
	for i in range(3):
		await process_frame
	var hud_warn_found := false
	for lbl in _find_controls_of_type(db, "Label"):
		if lbl.visible and lbl.text.contains("HUD"):
			hud_warn_found = true
			break
	assert_true(hud_warn_found, "Placing enemy inside HUD strip allows placement and shows non-blocking warning")
	spins_t2[1].value = 150
	for i in range(2):
		await process_frame

	# 4. Back to Troop 001: test Dragging updates X/Y + matches Battle Test, then test Alinear restores reference (395, 178)
	rec_list.select(0)
	rec_list.emit_signal("item_selected", 0)
	for i in range(4):
		await process_frame
	if main_scroll:
		main_scroll.ensure_control_visible(canvas)
		await process_frame
		main_scroll.scroll_vertical = mini(main_scroll.scroll_vertical, 240)
	for i in range(4):
		await process_frame
	var c_rect3 := canvas.get_global_rect()
	var prev_roi3 := Rect2i(int(c_rect3.position.x + vp_rect.position.x), int(c_rect3.position.y + vp_rect.position.y), int(vp_rect.size.x), int(vp_rect.size.y))

	var spins := _get_member_spins(db)
	assert_true(spins.size() == 2, "Troop 001 has X and Y SpinBoxes")
	var sp_x: SpinBox = spins[0]
	var sp_y: SpinBox = spins[1]
	assert_true(int(sp_x.value) == 395 and int(sp_y.value) == 178, "Initial saved X=395 Y=178 intact")

	# Simulate mouse drag from current sprite center to a new target (~65% X, ~30% Y)
	var start_local := (prev_bbox.get_center() - c_rect3.position) if prev_bbox.size.x > 0 else (vp_rect.position + Vector2(vp_rect.size.x * 0.484, vp_rect.size.y * 0.431))
	var drop_local := vp_rect.position + Vector2(vp_rect.size.x * 0.65, vp_rect.size.y * 0.30)
	var ev_down := InputEventMouseButton.new()
	ev_down.button_index = MOUSE_BUTTON_LEFT
	ev_down.pressed = true
	ev_down.position = start_local
	canvas.emit_signal("gui_input", ev_down)
	var ev_move := InputEventMouseMotion.new()
	ev_move.position = drop_local
	canvas.emit_signal("gui_input", ev_move)
	var ev_up := InputEventMouseButton.new()
	ev_up.button_index = MOUSE_BUTTON_LEFT
	ev_up.pressed = false
	ev_up.position = drop_local
	canvas.emit_signal("gui_input", ev_up)
	for i in range(4):
		await process_frame

	spins = _get_member_spins(db)
	sp_x = spins[0]
	sp_y = spins[1]
	var dragged_x := int(sp_x.value)
	var dragged_y := int(sp_y.value)
	assert_true(abs(dragged_x - 395) > 30, "Mouse drag updated X SpinBox (%d -> %d)" % [395, dragged_x])

	# Verify dragged position matches Battle Test
	btn_bt.emit_signal("pressed")
	for i in range(8):
		await process_frame
	bt_dlg = _find_accept_dialog_by_title(db, "Prueba de Batalla")
	var bt_img2 := bt_dlg.get_texture().get_image()
	var bt_bbox2 := _find_sprite_bbox_in_image(bt_img2, Rect2i(0, 0, bt_img2.get_width(), bt_img2.get_height()), sp_color)
	var db_img2 := db.get_texture().get_image()
	var prev_bbox2 := _find_sprite_bbox_in_image(db_img2, prev_roi3, sp_color)
	var p2 := Vector2((prev_bbox2.get_center().x - prev_roi3.position.x) / vp_rect.size.x, (prev_bbox2.get_center().y - prev_roi3.position.y) / vp_rect.size.y)
	var b2 := Vector2(bt_bbox2.get_center().x / float(bt_img2.get_width()), bt_bbox2.get_center().y / float(bt_img2.get_height()))
	assert_true(p2.distance_to(b2) < 0.03, "FAIL-BEFORE-FIX 3: Dragged enemy lands in same relative spot in Battle Test (prev=%s bt=%s)" % [str(p2), str(b2)])
	bt_dlg.emit_signal("canceled")
	for i in range(4):
		await process_frame

	# 5. Click Alinear -> restores Cañón musgoso to reference X=395, Y=178!
	var btn_align := _find_button_by_text(db, "Alinear")
	assert_true(btn_align != null, "Alinear button found")
	btn_align.emit_signal("pressed")
	for i in range(4):
		await process_frame
	spins = _get_member_spins(db)
	sp_x = spins[0]
	sp_y = spins[1]
	assert_true(int(sp_x.value) == 395 and int(sp_y.value) == 178, "FAIL-BEFORE-FIX 4a: Alinear restores 1-enemy Troop 001 to EXACT reference (395, 178) (got %d,%d)" % [int(sp_x.value), int(sp_y.value)])

	# Also verify 1-enemy Alinear preserves exact (395, 178) even with 32px snap enabled
	var chk_snap: CheckBox = null
	for cb in _find_controls_of_type(db, "CheckBox"):
		if cb.text.contains("32"):
			chk_snap = cb
			break
	assert_true(chk_snap != null, "32px grid snap checkbox found")
	if chk_snap:
		chk_snap.button_pressed = true
		sp_x.value = 120
		sp_y.value = 90
		await process_frame
		btn_align.emit_signal("pressed")
		for i in range(3):
			await process_frame
		spins = _get_member_spins(db)
		sp_x = spins[0]
		sp_y = spins[1]
		assert_true(int(sp_x.value) == 395 and int(sp_y.value) == 178, "1-enemy Alinear keeps exact (395, 178) even with 32px snap ON (got %d,%d)" % [int(sp_x.value), int(sp_y.value)])
		chk_snap.button_pressed = false

	# Ensure drag STILL works after Alinear!
	var ev_down2 := InputEventMouseButton.new()
	ev_down2.button_index = MOUSE_BUTTON_LEFT
	ev_down2.pressed = true
	ev_down2.position = drop_local if prev_bbox2.size.x == 0 else (vp_rect.position + Vector2(vp_rect.size.x * 0.484, vp_rect.size.y * 0.431))
	canvas.emit_signal("gui_input", ev_down2)
	var ev_move2 := InputEventMouseMotion.new()
	ev_move2.position = vp_rect.position + Vector2(vp_rect.size.x * 0.25, vp_rect.size.y * 0.35)
	canvas.emit_signal("gui_input", ev_move2)
	var ev_up2 := InputEventMouseButton.new()
	ev_up2.button_index = MOUSE_BUTTON_LEFT
	ev_up2.pressed = false
	ev_up2.position = ev_move2.position
	canvas.emit_signal("gui_input", ev_up2)
	for i in range(3):
		await process_frame
	spins = _get_member_spins(db)
	sp_x = spins[0]
	sp_y = spins[1]
	assert_true(abs(int(sp_x.value) - 395) > 20, "Mouse drag continues working after Alinear (%d,%d)" % [int(sp_x.value), int(sp_y.value)])

	# 6. Test Alinear with 3 enemies -> preserves relative spacing, centers group bbox around alignment point!
	rec_list.select(2)
	rec_list.emit_signal("item_selected", 2)
	for i in range(4):
		await process_frame
	var s3 := _get_member_spins(db)
	assert_true(s3.size() == 6, "Grupo Triple has 6 X/Y SpinBoxes")
	var sep01_before := Vector2(s3[2].value - s3[0].value, s3[3].value - s3[1].value)
	var sep12_before := Vector2(s3[4].value - s3[2].value, s3[5].value - s3[3].value)
	btn_align.emit_signal("pressed")
	for i in range(4):
		await process_frame
	s3 = _get_member_spins(db)
	var sep01_after := Vector2(s3[2].value - s3[0].value, s3[3].value - s3[1].value)
	var sep12_after := Vector2(s3[4].value - s3[2].value, s3[5].value - s3[3].value)
	assert_true(sep01_before.distance_to(sep01_after) <= 2.0 and sep12_before.distance_to(sep12_after) <= 2.0, "FAIL-BEFORE-FIX 4b: Alinear 3 enemies preserves inter-member separation")
	var mid_x_after := (float(s3[0].value) + float(s3[4].value)) * 0.5
	assert_true(absf(mid_x_after - 395.0) <= 2.0, "FAIL-BEFORE-FIX 4c: Alinear 3 enemies centers symmetric block around reference X=395 (got mid_x=%f)" % mid_x_after)

	# 7. Save, close & reopen DB -> positions preserved
	db.save_to_disk()
	var saved_x0 := int(s3[0].value)
	var saved_y0 := int(s3[1].value)
	db.hide()
	db.free()
	var db2 := ClockGearsDatabaseDialog.new()
	get_root().add_child(db2)
	await process_frame
	db2.load_from_disk()
	db2.select_category(8)
	db2.popup_centered(Vector2i(1366, 768))
	for i in range(5):
		await process_frame
	var rl2: ItemList = _find_controls_of_type(db2, "ItemList")[0]
	rl2.select(2)
	rl2.emit_signal("item_selected", 2)
	for i in range(3):
		await process_frame
	var s3_reloaded := _get_member_spins(db2)
	assert_true(int(s3_reloaded[0].value) == saved_x0 and int(s3_reloaded[1].value) == saved_y0, "Save/close/reopen preserves aligned positions")
	db2.hide()
	db2.free()

	# Cleanup temp files and restore snapshot
	_restore_dir("res://clockgears_data", data_snap)
	DirAccess.remove_absolute(ProjectSettings.globalize_path(bb_res))
	DirAccess.remove_absolute(ProjectSettings.globalize_path(sp_res))
	if FileAccess.file_exists(bb_res + ".import"):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(bb_res + ".import"))
	if FileAccess.file_exists(sp_res + ".import"):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(sp_res + ".import"))

	print("==================================================")
	if failed == 0:
		print("ALL TESTS PASSED (0 FAILURES)")
	else:
		print("TESTS FAILED COUNT: ", failed)
	quit(0 if failed == 0 else 1)
