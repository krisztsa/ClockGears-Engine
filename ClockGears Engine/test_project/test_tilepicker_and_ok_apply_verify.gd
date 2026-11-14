extends SceneTree

var failed := 0

func assert_true(cond: bool, msg: String) -> void:
	if cond:
		print("[PASS] ", msg)
	else:
		print("[FAIL] ", msg)
		failed += 1

func _find_controls_of_type(node: Node, cls_name: String) -> Array:
	var out: Array = []
	if node.get_class() == cls_name or node.is_class(cls_name):
		out.append(node)
	for c in node.get_children(true):
		out.append_array(_find_controls_of_type(c, cls_name))
	return out

func _find_button_by_text(node: Node, txt: String) -> Button:
	for c in _find_controls_of_type(node, "Button"):
		if c.text == txt:
			return c
	return null

func _find_tile_picker_dialog(db: Node) -> ConfirmationDialog:
	for c in db.get_children(true):
		if c.get_class() == "ClockGearsTilePickerDialog":
			return c
	return null

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

func _as_vec2i(v) -> Vector2i:
	if v is Vector2i:
		return v
	if v is Array and v.size() >= 2:
		return Vector2i(int(v[0]), int(v[1]))
	if v is Dictionary:
		return Vector2i(int(v.get("x", 0)), int(v.get("y", 0)))
	return Vector2i(-999, -999)

func _tex_has_color(tex: Texture2D, target: Color) -> bool:
	if tex == null:
		return false
	var img := tex.get_image()
	if img == null or img.is_empty():
		return false
	for y in range(img.get_height()):
		for x in range(img.get_width()):
			var c := img.get_pixel(x, y)
			if c.a > 0.5 and absf(c.r - target.r) < 0.12 and absf(c.g - target.g) < 0.12 and absf(c.b - target.b) < 0.12:
				return true
	return false

func _initialize() -> void:
	print("=== REPRO & VERIFY TILEPICKER + ICON + OK/APLICAR ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.load_database()
	var data_snap := _snapshot_dir("res://clockgears_data")

	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://img/tilesets"))
	var ts_res := "res://img/tilesets/_tmp_forest_tilemap.png"
	var long_ts_res := "res://img/tilesets/_tmp_very_long_forest_tileset_spritesheet_2026_final_edition.png"
	var single_res := "res://img/tilesets/_tmp_single_wide_icon.png"
	var red_col := Color(0.85, 0.15, 0.15, 1.0)
	var green_col := Color(0.15, 0.70, 0.25, 1.0)
	var brown_col := Color(0.55, 0.32, 0.14, 1.0)
	var blue_col := Color(0.15, 0.35, 0.85, 1.0)

	# 4x2 cells (128x64): [0,0]=RED, [2,0]=BROWN (tronco), rest=GREEN
	var ts_img := Image.create(128, 64, false, Image.FORMAT_RGBA8)
	for cy in range(2):
		for cx in range(4):
			var col := green_col
			if cx == 0 and cy == 0:
				col = red_col
			elif cx == 2 and cy == 0:
				col = brown_col
			for py in range(cy * 32, (cy + 1) * 32):
				for px in range(cx * 32, (cx + 1) * 32):
					ts_img.set_pixel(px, py, col)
	ts_img.save_png(ProjectSettings.globalize_path(ts_res))
	ts_img.save_png(ProjectSettings.globalize_path(long_ts_res))

	# 64x32 single wide non-tile PNG (BLUE) to test FileDialog fallback & aspect-fit centering
	var wide_img := Image.create(64, 32, false, Image.FORMAT_RGBA8)
	wide_img.fill(blue_col)
	wide_img.save_png(ProjectSettings.globalize_path(single_res))

	var seed_items: Array = [
		{ "id": 1, "name": "Madera", "price": 10, "graphic_path": "" },
		{ "id": 2, "name": "Roca", "price": 5, "graphic_path": "" },
	]
	for k in range(3, 18):
		seed_items.append({ "id": k, "name": "Item_%02d" % k, "price": k, "graphic_path": "" })
	dm.set_category_array(3, seed_items)
	dm.set_category_array(13, [
		{ "id": 1, "name": "Bosque", "slots": { "B": ts_res } }
	])
	dm.save_database()

	var db := ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	await process_frame
	db.open_database()
	db.select_category(3) # CAT_ITEMS
	for i in range(6):
		await process_frame

	var btn_apply := _find_button_by_text(db, "Aplicar")
	var btn_ok := db.get_ok_button()
	assert_true(btn_apply != null, "FAIL-BEFORE-FIX 4a: Database dialog has 'Aplicar' button")
	assert_true(btn_ok != null and btn_ok.text == "OK", "Database dialog has 'OK' button")
	if btn_apply and btn_ok:
		assert_true(btn_apply.disabled, "FAIL-BEFORE-FIX 4b: 'Aplicar' is disabled when there are no pending changes")
		assert_true(absf(btn_apply.size.x - btn_ok.size.x) <= 2.0 and absf(btn_apply.size.y - btn_ok.size.y) <= 2.0, "OK & Aplicar have identical size (%s vs %s)" % [str(btn_ok.size), str(btn_apply.size)])
		var sb_ok := btn_ok.get_theme_stylebox("normal")
		var sb_app := btn_apply.get_theme_stylebox("normal")
		assert_true(sb_ok is StyleBoxFlat and (sb_ok as StyleBoxFlat).corner_radius_top_left > 0, "OK button uses rounded theme")
		assert_true(sb_app is StyleBoxFlat and (sb_app as StyleBoxFlat).corner_radius_top_left > 0, "Aplicar button uses rounded theme")

	# Section 1 check: empty-graphic item 'Roca' gets fallback category icon (non-null, not pure white)
	var rec_list: ItemList = _find_controls_of_type(db, "ItemList")[0]
	assert_true(rec_list.get_fixed_icon_size().x >= 32 and rec_list.get_fixed_icon_size().x == rec_list.get_fixed_icon_size().y, "Uniform square fixed_icon_size on record_list (%s)" % str(rec_list.get_fixed_icon_size()))
	assert_true(rec_list.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST, "record_list uses TEXTURE_FILTER_NEAREST")
	var roca_icon: Texture2D = rec_list.get_item_icon(1)
	assert_true(roca_icon != null and not _tex_has_color(roca_icon, red_col), "Empty-graphic item uses fallback category icon")

	# Test Esc / X / child modal protection on TilePickerDialog
	var btn_tp := _find_button_by_text(db, "TilePicker...")
	assert_true(btn_tp != null, "TilePicker... button found")
	btn_tp.emit_signal("pressed")
	for i in range(4):
		await process_frame

	var tp_dlg := _find_tile_picker_dialog(db)
	assert_true(tp_dlg != null and tp_dlg.visible, "TilePickerDialog opened")
	var tp = _find_controls_of_type(tp_dlg, "ClockGearsTilePicker")[0]

	# X (close_requested) on TilePicker closes ONLY TilePicker
	tp_dlg.emit_signal("close_requested")
	for i in range(4):
		await process_frame
	assert_true(not tp_dlg.visible and db.visible, "X on TilePicker closes only TilePicker and keeps DB open")

	# Re-open and test Cancel/Esc closes ONLY TilePicker
	btn_tp.emit_signal("pressed")
	for i in range(4):
		await process_frame
	# Simulate pressing ui_close_dialog on DB while child modal tp_dlg is open -> DB stays open!
	var close_ev := InputEventAction.new()
	close_ev.action = "ui_close_dialog"
	close_ev.pressed = true
	db.push_input(close_ev)
	await process_frame
	assert_true(db.visible, "ui_close_dialog does NOT close DB while child TilePicker is open")

	# Load the 128x64 tileset texture into picker and click cell [2,0]
	var tex_loaded := ImageTexture.create_from_image(ts_img)
	if tp.has_method("set_atlas_texture"):
		tp.call("set_atlas_texture", tex_loaded, ts_res)
	# If set_atlas_texture isn't exposed to GDScript, check what atlas is loaded or click canvas
	var canvas = _find_controls_of_type(tp, "ClockGearsTilePickerCanvas")[0]
	if tp.get_method_list().filter(func(m): return m["name"] == "set_atlas_texture").size() == 0:
		# Simulate internal FileDialog file_selected inside TilePicker
		var fds := _find_controls_of_type(tp, "FileDialog")
		if fds.size() > 0:
			fds[0].emit_signal("file_selected", ProjectSettings.globalize_path(ts_res))
			for i in range(3):
				await process_frame

	# Click cell [2,0] on TilePickerCanvas (x = 2*32+16 = 80, y = 0*32+16 = 16 at zoom 1x)
	var click_pos: Vector2 = canvas.get_global_rect().position + Vector2(80, 16)
	var motion := InputEventMouseMotion.new()
	motion.position = click_pos
	motion.global_position = click_pos
	tp_dlg.push_input(motion)
	var click_down := InputEventMouseButton.new()
	click_down.button_index = MOUSE_BUTTON_LEFT
	click_down.pressed = true
	click_down.position = click_pos
	click_down.global_position = click_pos
	tp_dlg.push_input(click_down)
	var click_up := InputEventMouseButton.new()
	click_up.button_index = MOUSE_BUTTON_LEFT
	click_up.pressed = false
	click_up.position = click_pos
	click_up.global_position = click_pos
	tp_dlg.push_input(click_up)
	await process_frame

	# First test Cancel -> discards and keeps DB open
	tp_dlg.get_cancel_button().emit_signal("pressed")
	for i in range(4):
		await process_frame
	assert_true(db.visible, "Cancel on TilePicker keeps DatabaseDialog open")
	assert_true(String(dm.get_category_array(3)[0].get("graphic_path", "")) == "", "Cancel on TilePicker does NOT modify Madera graphic")

	# Re-open TilePicker, select [2,0] and press OK via real mouse click on OK button!
	btn_tp.emit_signal("pressed")
	for i in range(4):
		await process_frame
	var fds2 := _find_controls_of_type(tp, "FileDialog")
	if fds2.size() > 0:
		fds2[0].emit_signal("file_selected", ProjectSettings.globalize_path(ts_res))
		for i in range(3):
			await process_frame
	click_pos = canvas.get_global_rect().position + Vector2(80, 16)
	motion.position = click_pos
	motion.global_position = click_pos
	click_down.position = click_pos
	click_down.global_position = click_pos
	click_up.position = click_pos
	click_up.global_position = click_pos
	tp_dlg.push_input(motion)
	tp_dlg.push_input(click_down)
	tp_dlg.push_input(click_up)
	await process_frame

	# Press OK on TilePickerDialog
	tp_dlg.get_ok_button().emit_signal("pressed")
	for i in range(6):
		await process_frame

	print("DEBUG after OK click: db.visible=", db.visible, " tp_dlg.visible=", tp_dlg.visible, " dm_item=", dm.get_category_array(3)[0])

	assert_true(db.visible, "FAIL-BEFORE-FIX 3a: Pressing OK in TilePicker MUST keep DatabaseDialog open")
	assert_true(not tp_dlg.visible, "TilePickerDialog closed after OK")

	# Check graphic preview shows ONLY [2,0] BROWN cell (32x32), NOT full tileset (RED/GREEN)
	var preview_tex: Texture2D = null
	for tr in _find_controls_of_type(db, "TextureRect"):
		if tr.visible and tr.custom_minimum_size.x >= 80:
			preview_tex = tr.texture
			break
	assert_true(_tex_has_color(preview_tex, brown_col), "FAIL-BEFORE-FIX 3b: Graphic preview shows chosen BROWN tile [2,0]")
	assert_true(not _tex_has_color(preview_tex, red_col) and not _tex_has_color(preview_tex, green_col), "FAIL-BEFORE-FIX 3c: Graphic preview shows ONLY tile [2,0], NOT full tileset")

	# Check list item icon shows BROWN [2,0], NOT generic category icon or RED/GREEN
	var row_icon: Texture2D = rec_list.get_item_icon(0)
	assert_true(_tex_has_color(row_icon, brown_col) and not _tex_has_color(row_icon, red_col), "FAIL-BEFORE-FIX 1: ItemList row icon shows cropped BROWN tile [2,0]")

	# Section 2 check: label under graphic_preview wraps up to 2 lines & has full tooltip
	var preview_ctrl: Control = null
	for tr in _find_controls_of_type(db, "TextureRect"):
		if tr.visible and tr.custom_minimum_size.x >= 80:
			preview_ctrl = tr
			break
	var graphic_box: Node = preview_ctrl.get_parent().get_parent().get_parent() if preview_ctrl else null
	var path_lbl: Label = null
	if graphic_box:
		for ch in graphic_box.get_children():
			if ch is Label:
				path_lbl = ch
				break
	assert_true(path_lbl != null, "Graphic path label located under graphic_frame")
	if path_lbl:
		assert_true(path_lbl.autowrap_mode != TextServer.AUTOWRAP_OFF and path_lbl.max_lines_visible == 2, "Graphic path label wraps up to 2 lines cleanly")
		assert_true(path_lbl.tooltip_text.contains("_tmp_forest_tilemap.png") and path_lbl.tooltip_text.contains("[2,0]"), "Tooltip preserves full path + [2,0] (%s)" % path_lbl.tooltip_text)

	# Click Aplicar -> saves and keeps DB open on same item/scroll; Aplicar becomes disabled
	var vscroll := rec_list.get_v_scroll_bar()
	var prev_scroll := vscroll.value if vscroll else 0.0
	if btn_apply:
		assert_true(not btn_apply.disabled, "FAIL-BEFORE-FIX 4c: 'Aplicar' is enabled after modifying tile")
		btn_apply.emit_signal("pressed")
		for i in range(4):
			await process_frame
		assert_true(db.visible, "Aplicar keeps DatabaseDialog open")
		assert_true(btn_apply.disabled, "Aplicar disables again right after saving")
		assert_true(rec_list.get_selected_items().size() > 0 and rec_list.get_selected_items()[0] == 0, "Aplicar preserves selected item")
		if vscroll:
			assert_true(absf(vscroll.value - prev_scroll) <= 1.0, "Aplicar preserves list scroll")

	# Check saved JSON on disk has file AND cell [2,0]
	var dm_items: Array = dm.get_category_array(3)
	assert_true(String(dm_items[0].get("graphic_path", "")).contains("_tmp_forest_tilemap"), "Saved graphic_path points to tileset file")
	var saved_tc = dm_items[0].get("tile_coords", null)
	assert_true(saved_tc != null and _as_vec2i(saved_tc) == Vector2i(2, 0), "FAIL-BEFORE-FIX 3d: Saved record stores tile_coords [2,0] (got %s)" % str(saved_tc))

	# Re-open TilePicker -> verify [2,0] is already pre-selected!
	btn_tp.emit_signal("pressed")
	for i in range(4):
		await process_frame
	if tp.has_method("get_selected_tiles_array"):
		var sel_arr: Array = tp.get_selected_tiles_array()
		assert_true(sel_arr.size() > 0 and _as_vec2i(sel_arr[0].get("atlas_coords", Vector2i(-1, -1))) == Vector2i(2, 0), "FAIL-BEFORE-FIX 3e: Reopening TilePicker pre-selects saved tile [2,0] (got %s)" % str(sel_arr))
	if tp_dlg.visible:
		tp_dlg.get_cancel_button().emit_signal("pressed")
	for i in range(3):
		await process_frame

	# Test very long tileset filename -> middle ellipsis & full tooltip, still shows BROWN [2,0] on item 0
	btn_tp.emit_signal("pressed")
	for i in range(4):
		await process_frame
	var fds_long := _find_controls_of_type(tp, "FileDialog")
	if fds_long.size() > 0:
		fds_long[0].emit_signal("file_selected", ProjectSettings.globalize_path(long_ts_res))
		for i in range(3):
			await process_frame
	if tp.has_method("set_selected_tile"):
		tp.call("set_selected_tile", Vector2i(2, 0), Vector2i(1, 1))
	tp_dlg.get_ok_button().emit_signal("pressed")
	for i in range(4):
		await process_frame
	if path_lbl:
		assert_true(path_lbl.tooltip_text.contains("_tmp_very_long_forest_tileset_spritesheet_2026_final_edition.png"), "Tooltip has full very-long tileset path (got '%s')" % path_lbl.tooltip_text)
		assert_true(path_lbl.text.contains("…") or path_lbl.get_line_count() <= 2, "Very-long path ellipsizes/wraps within 2 lines (text='%s')" % path_lbl.text)

	# Test Item 1 via single FileDialog graphic ("Cambiar gráfico...") -> updates row icon to BLUE wide PNG
	rec_list.select(1)
	rec_list.emit_signal("item_selected", 1)
	await process_frame
	var btn_chg := _find_button_by_text(db, "Cambiar gráfico...")
	assert_true(btn_chg != null, "Cambiar gráfico... button found")
	if btn_chg:
		btn_chg.emit_signal("pressed")
		for i in range(2):
			await process_frame
	var db_fds := _find_controls_of_type(db, "FileDialog")
	var chosen_fd: FileDialog = null
	for fd in db_fds:
		if fd.get_parent() == db and fd.visible:
			chosen_fd = fd
			break
	if chosen_fd == null and db_fds.size() > 0:
		chosen_fd = db_fds[db_fds.size() - 1]
	if chosen_fd:
		chosen_fd.hide()
		chosen_fd.emit_signal("file_selected", ProjectSettings.globalize_path(single_res))
	for i in range(4):
		await process_frame
	var item1_icon: Texture2D = rec_list.get_item_icon(1)
	assert_true(_tex_has_color(item1_icon, blue_col) and not _tex_has_color(item1_icon, brown_col), "Cambiar gráfico (FileDialog) updates row icon to BLUE wide PNG immediately")
	# Also verify Item 1 preview updated cleanly and can Copiar/Pegar
	var btn_copy := _find_button_by_text(db, "Copiar")
	var btn_paste := _find_button_by_text(db, "Pegar")
	if btn_copy and btn_paste:
		btn_copy.emit_signal("pressed")
		rec_list.select(2)
		rec_list.emit_signal("item_selected", 2)
		btn_paste.emit_signal("pressed")
		for i in range(3):
			await process_frame
		assert_true(_tex_has_color(rec_list.get_item_icon(2), blue_col), "Pegar record updates destination row icon immediately")
	# Select back item 0 (Madera) before OK
	rec_list.select(0)
	rec_list.emit_signal("item_selected", 0)
	await process_frame

	# Edit name and press Database OK -> saves and closes DB
	var name_le: LineEdit = null
	for le in _find_controls_of_type(db, "LineEdit"):
		if le.visible and le.placeholder_text.contains("Nombre"):
			name_le = le
			break
	if name_le:
		name_le.text = "Madera Noble"
		name_le.emit_signal("text_changed", "Madera Noble")
		await process_frame
	if btn_ok:
		btn_ok.emit_signal("pressed")
		for i in range(4):
			await process_frame
	assert_true(not db.visible, "Pressing Database OK closes DatabaseDialog")
	db.free()

	# Reopen fresh DatabaseDialog -> tile [2,0] and "Madera Noble" preserved!
	var db2 := ClockGearsDatabaseDialog.new()
	get_root().add_child(db2)
	await process_frame
	db2.load_from_disk()
	db2.select_category(3)
	db2.popup_centered(Vector2i(1280, 720))
	for i in range(6):
		await process_frame
	var rl2: ItemList = _find_controls_of_type(db2, "ItemList")[0]
	assert_true(rl2.get_item_text(0).contains("Madera Noble"), "Saved name persisted across reopen")
	assert_true(_tex_has_color(rl2.get_item_icon(0), brown_col) and not _tex_has_color(rl2.get_item_icon(0), red_col), "FAIL-BEFORE-FIX 3f: Reopened DB row icon still shows BROWN tile [2,0]")
	var preview_tex2: Texture2D = null
	for tr in _find_controls_of_type(db2, "TextureRect"):
		if tr.visible and tr.custom_minimum_size.x >= 80:
			preview_tex2 = tr.texture
			break
	assert_true(_tex_has_color(preview_tex2, brown_col) and not _tex_has_color(preview_tex2, red_col), "FAIL-BEFORE-FIX 3g: Reopened DB graphic preview still shows ONLY BROWN tile [2,0]")

	db2.hide()
	db2.free()

	_restore_dir("res://clockgears_data", data_snap)
	for tmp_res in [ts_res, long_ts_res, single_res]:
		DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_res))
		if FileAccess.file_exists(tmp_res + ".import"):
			DirAccess.remove_absolute(ProjectSettings.globalize_path(tmp_res + ".import"))

	print("==================================================")
	if failed == 0:
		print("ALL TESTS PASSED (0 FAILURES)")
	else:
		print("TESTS FAILED COUNT: ", failed)
	quit(0 if failed == 0 else 1)
