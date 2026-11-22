extends SceneTree

var pass_count := 0
var fail_count := 0
var t0 := Time.get_ticks_msec()

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + msg)
	else:
		fail_count += 1
		printerr("[FAIL] " + msg)

func pump(n: int = 3) -> void:
	for _i in n:
		if Time.get_ticks_msec() - t0 > 55000:
			printerr("[TIMEOUT] Exceeded 55s internal safety budget!")
			quit(1)
			return
		await process_frame

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var dm_owned := false
	if dm == null:
		dm = ClassDB.instantiate("ClockGearsDataManager")
		dm_owned = true
	check(dm != null, "DataManager available")
	if dm and dm.has_method("load_project_data"):
		dm.load_project_data("res://clockgears_data")

	var db = ClassDB.instantiate("ClockGearsDatabaseDialog")
	check(db != null, "DatabaseDialog instantiated")
	if db == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	get_root().add_child(db)
	db.popup_centered(Vector2i(1366, 768))
	db.size = Vector2i(1366, 768)
	await pump(4)

	# --- PUNTO 1: DISTRIBUCIÓN POR DEFECTO ---
	var tabs := ["main_menu", "game_menu", "hud", "credits", "game_over", "messages_battle"]
	var ed = db.get_ui_designer_editor()
	check(ed != null, "UI Designer panel created")
	var h: Dictionary = db.get_ui_designer_ui_handles()

	for tab_key in tabs:
		db.select_category_by_key(tab_key)
		db.adapt_to_window_size(Vector2i(1366, 768))
		await pump(4)
		var left_col: Control = h.get("left_col_scroll")
		var center_col: Control = h.get("wide_center_col")
		var right_col: Control = h.get("wide_right_col")
		var cscroll: Control = h.get("canvas_scroll")
		var cstage: Control = h.get("canvas_stage")
		check(left_col != null and center_col != null and right_col != null, "Tab '%s' has 3 columns" % tab_key)
		if left_col and center_col and right_col:
			var lw := left_col.size.x
			var cw := center_col.size.x
			var rw := right_col.size.x
			check(lw >= 255.0 and lw <= 325.0, "Tab '%s' 1366 left width in [260..320] (got %.1f)" % [tab_key, lw])
			check(rw >= 435.0 and rw <= 565.0, "Tab '%s' 1366 right width in [440..560] (got %.1f)" % [tab_key, rw])
			check(cw > 450.0, "Tab '%s' 1366 center width > 450 (got %.1f)" % [tab_key, cw])
		if cscroll and cstage:
			var sr := cscroll.get_global_rect()
			var cr := Rect2(cstage.get_global_position(), cstage.size * cstage.scale)
			check(cr.position.x >= sr.position.x - 1.0 and cr.end.x <= sr.end.x + 1.0 and cr.position.y >= sr.position.y - 1.0 and cr.end.y <= sr.end.y + 1.0,
				"Tab '%s' canvas Ajustar fits inside canvas_scroll (stage=%s, scroll=%s)" % [tab_key, str(cr), str(sr)])

	# Test 1920x1080 and 2560x1440 proportions
	db.select_category_by_key("main_menu")
	for res in [Vector2i(960, 540), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		db.size = res
		db.adapt_to_window_size(res)
		await pump(5)
		var left_col: Control = h.get("left_col_scroll")
		var center_col: Control = h.get("wide_center_col")
		var right_col: Control = h.get("wide_right_col")
		var cscroll: Control = h.get("canvas_scroll")
		var cstage: Control = h.get("canvas_stage")
		var exp_l := clampf(float(res.x) * 0.21, 260.0, 320.0)
		var exp_r := clampf(float(res.x) * 0.34, 440.0, 560.0)
		check(absf(left_col.size.x - exp_l) <= 35.0, "%s left ~%.0f (got %.1f)" % [str(res), exp_l, left_col.size.x])
		check(absf(right_col.size.x - exp_r) <= 35.0, "%s right ~%.0f (got %.1f)" % [str(res), exp_r, right_col.size.x])
		check(center_col.size.x >= 180.0, "%s center >= 180 (got %.1f)" % [str(res), center_col.size.x])
		var sr := cscroll.get_global_rect()
		var cr := Rect2(cstage.get_global_position(), cstage.size * cstage.scale)
		check(cr.position.x >= sr.position.x - 1.0 and cr.end.x <= sr.end.x + 1.0 and cr.end.y <= sr.end.y + 1.0,
			"%s canvas Ajustar unclipped (stage=%s, scroll=%s)" % [str(res), str(cr), str(sr)])

	# Return to 1366x768 for splitter drag/remember/double-click reset + properties check
	db.size = Vector2i(1366, 768)
	db.adapt_to_window_size(Vector2i(1366, 768))
	await pump(4)

	var whbox: HSplitContainer = h.get("wide_hbox")
	var wrsplit: HSplitContainer = h.get("wide_right_split")
	var left_col: Control = h.get("left_col_scroll")
	var right_col: Control = h.get("wide_right_col")
	var init_lw := left_col.size.x
	var init_rw := right_col.size.x

	# Simulate user dragging right splitter wider via dragger signal & check persistence across tab switch
	wrsplit.set_split_offset(wrsplit.get_split_offset() - 60)
	wrsplit.emit_signal("dragged", wrsplit.get_split_offset())
	await pump(4)
	var dragged_rw := right_col.size.x
	check(dragged_rw > init_rw + 30.0, "Splitter drag widens right col (%.1f -> %.1f)" % [init_rw, dragged_rw])

	db.select_category_by_key("hud")
	db.adapt_to_window_size(Vector2i(1366, 768))
	await pump(4)
	check(absf(right_col.size.x - dragged_rw) < 10.0, "Dragged splitter width remembered across tabs (%.1f)" % right_col.size.x)

	# Double-click on right splitter dragger resets to default
	var dr1: Control = wrsplit.get_drag_area_control()
	check(dr1 != null, "Right splitter dragger control present")
	if dr1:
		var ev := InputEventMouseButton.new()
		ev.button_index = MOUSE_BUTTON_LEFT
		ev.pressed = true
		ev.double_click = true
		ev.position = dr1.size * 0.5
		dr1.emit_signal("gui_input", ev)
	await pump(4)
	check(absf(right_col.size.x - init_rw) <= 20.0, "Double-click on divider resets width (back to ~%.1f, got %.1f)" % [init_rw, right_col.size.x])

	# Select/add a temporary element to verify Properties 32px rows, full labels & large fields
	var tmp_id: String = ed.add_element_at("button", Vector2i(400, 260))
	await pump(4)
	check(not tmp_id.is_empty(), "Created temp element for properties check")
	var pbox: Control = h.get("props_fields_box")
	var found_ancho := false
	var found_posx := false
	var found_posy := false
	var small_rows := 0
	if pbox:
		var stack := [pbox]
		while not stack.is_empty():
			var node: Node = stack.pop_back()
			if node is HBoxContainer and (node as Control).visible:
				if (node as Control).size.y > 1.0 and (node as Control).size.y < 31.0:
					small_rows += 1
			if node is Label and (node as Control).visible:
				var lb := node as Label
				var txt := lb.text
				if txt.begins_with("Ancho"):
					found_ancho = true
					check(not lb.clip_text and lb.size.x >= 50.0, "Label '%s' not clipped (w=%.1f)" % [txt, lb.size.x])
				elif txt.begins_with("Pos X"):
					found_posx = true
				elif txt.begins_with("Pos Y"):
					found_posy = true
			for ch in node.get_children():
				stack.append(ch)
	check(found_ancho and found_posx and found_posy, "Full property labels present (Ancho, Pos X, Pos Y)")
	check(small_rows == 0, "All visible property HBox rows >= 31px (small_rows=%d)" % small_rows)

	var ox_sp: Control = h.get("prop_ox_spin")
	var w_sp: Control = h.get("prop_w_spin")
	if ox_sp and w_sp:
		check(ox_sp.size.x >= 120.0 and w_sp.size.x >= 120.0, "Property SpinBox fields are large (ox=%.1f, w=%.1f)" % [ox_sp.size.x, w_sp.size.x])

	# --- PUNTO 2: BOTONES Y ETIQUETAS SIEMPRE VISIBLES + AUDITORÍA ---
	var btn_wiz: Button = h.get("new_menu_wizard_btn")
	var btn_sub: Button = h.get("add_submenu_btn")
	var btn_bar: Button = h.get("add_button_bar_btn")
	check(btn_wiz != null and btn_sub != null and btn_bar != null, "Top bar 3 action buttons present")
	if btn_wiz and btn_sub and btn_bar:
		check(btn_wiz.icon != null and btn_sub.icon != null and btn_bar.icon != null, "Top bar 3 action buttons have icons")
		check(btn_wiz.icon != btn_sub.icon and btn_sub.icon != btn_bar.icon and btn_wiz.icon != btn_bar.icon, "Top bar 3 action buttons have distinct icons")
		check(btn_wiz.text != btn_sub.text and btn_sub.text != btn_bar.text, "Top bar 3 action buttons have distinct labels")
		check(not btn_wiz.tooltip_text.is_empty() and not btn_sub.tooltip_text.is_empty() and not btn_bar.tooltip_text.is_empty(), "Top bar 3 action buttons have descriptive tooltips")

	for zk in ["zoom_1_btn", "zoom_2_btn", "zoom_3_btn", "zoom_50_btn", "zoom_300_btn"]:
		var zb: Button = h.get(zk)
		check(zb != null and not zb.clip_text and zb.size.x + 1.5 >= zb.get_combined_minimum_size().x, "Zoom button '%s' fully visible (w=%.1f)" % [zb.text if zb else zk, zb.size.x if zb else 0.0])

	# Verify Imagen buttons ("Elegir..." & "X") + Estado labels on selected button
	var btn_elegir_count := 0
	var btn_x_count := 0
	var state_lbl_hits := 0
	if pbox:
		var stack2 := [pbox]
		while not stack2.is_empty():
			var node: Node = stack2.pop_back()
			if node is Button and (node as Control).visible:
				var bt := node as Button
				if bt.text.begins_with("Elegir"):
					btn_elegir_count += 1
					check(bt.size.x >= 72.0 and not bt.clip_text, "'Elegir...' button full width (w=%.1f)" % bt.size.x)
				elif bt.text == "X":
					btn_x_count += 1
					check(bt.size.x >= 30.0 and not bt.clip_text, "'X' clear button full width (w=%.1f)" % bt.size.x)
			elif node is Label and (node as Control).visible:
				var lb2 := node as Label
				if lb2.text in ["Estado normal:", "Estado encima:", "Estado pulsado:", "Estado desact.:"]:
					state_lbl_hits += 1
					check(not lb2.clip_text and lb2.size.x >= 100.0, "State label '%s' unclipped (w=%.1f)" % [lb2.text, lb2.size.x])
			for ch in node.get_children():
				stack2.append(ch)
	check(btn_elegir_count >= 5, "Found Ruta de imagen + 4 button state 'Elegir...' buttons (got %d)" % btn_elegir_count)
	check(btn_x_count >= 5, "Found Ruta de imagen + 4 button state 'X' buttons (got %d)" % btn_x_count)
	check(state_lbl_hits == 4, "All 4 button state labels present and unclipped (got %d)" % state_lbl_hits)

	# Floating window title check (no "[button]")
	ed.open_floating_element_window(tmp_id)
	await pump(3)
	var fwin = ed.get_floating_window_for_element(tmp_id)
	check(fwin != null, "Floating window opened")
	if fwin:
		check(not fwin.title.contains("[button]") and fwin.title.contains("Botón"), "Floating window title uses Spanish type (title='%s')" % fwin.title)
	ed.close_all_floating_element_windows()
	await pump(2)

	# --- PUNTO 3: AJUSTAR MANUALMENTE EN LA IMAGEN ---
	var img_id: String = ed.add_element_at("image", Vector2i(300, 180))
	await pump(2)
	var img_edit: LineEdit = h.get("prop_image_edit")
	var img_scale_opt: OptionButton = h.get("prop_image_scale_opt")
	var img_man_btn: Button = h.get("prop_image_adjust_manual_btn")
	check(img_man_btn != null and img_scale_opt != null, "Ajustar manualmente button and Escalado option present")

	# Assign real PNG texture from test_project
	img_edit.text = "res://img/characters/door_wood.png"
	img_edit.emit_signal("text_changed", img_edit.text)
	await pump(2)
	check(not img_man_btn.disabled, "Ajustar manualmente enabled when image assigned")

	# Test 9-slice disables Ajustar manualmente with explanatory tooltip
	var ns_idx := -1
	var fit_idx := -1
	var man_idx := -1
	for i in img_scale_opt.item_count:
		var md := str(img_scale_opt.get_item_metadata(i))
		if md == "nine_slice": ns_idx = i
		elif md == "fit": fit_idx = i
		elif md == "manual": man_idx = i
	check(ns_idx >= 0 and man_idx >= 0, "Escalado includes 9-slice and Manual options")
	img_scale_opt.select(ns_idx)
	img_scale_opt.emit_signal("item_selected", ns_idx)
	await pump(2)
	check(img_man_btn.disabled and img_man_btn.tooltip_text.to_lower().contains("9-slice"), "9-slice disables Ajustar manualmente with tooltip ('%s')" % img_man_btn.tooltip_text)

	img_scale_opt.select(fit_idx)
	img_scale_opt.emit_signal("item_selected", fit_idx)
	await pump(2)
	check(not img_man_btn.disabled, "Switching back to fit re-enables Ajustar manualmente")

	# Open Floating Window and verify it also has Ajustar manualmente
	ed.open_floating_element_window(img_id)
	await pump(2)
	var fw2 = ed.get_floating_window_for_element(img_id)
	if fw2:
		var fwh: Dictionary = fw2.get_ui_handles()
		var fw_man_btn: Button = fwh.get("adjust_manual_btn")
		check(fw_man_btn != null and not fw_man_btn.disabled, "Floating window has enabled Ajustar manualmente button")
	ed.close_all_floating_element_windows()
	await pump(1)

	# Open Adjust dialog, adjust position & scale in %, confirm, verify Undo/Redo & F5 runtime proportionality
	ed.open_manual_image_adjust_for_selected()
	await pump(2)
	h = db.get_ui_designer_ui_handles()
	var adj_dlg = h.get("image_adjust_dialog")
	check(adj_dlg != null and adj_dlg.visible, "ImageAdjustDialog opens cleanly")
	if adj_dlg:
		adj_dlg.set_values({
			"manual_offset_x_pct": 25.0,
			"manual_offset_y_pct": -15.0,
			"manual_scale_x_pct": 150.0,
			"manual_scale_y_pct": 150.0,
			"manual_keep_aspect": true,
			"manual_integer_scale": false
		})
		await pump(1)
		adj_dlg.confirm_adjust()
		await pump(3)

	var eff_img: Dictionary = ed.get_effective_element(img_id)
	check(str(eff_img.get("scale_mode", "")) == "manual", "Confirm switches scale_mode to 'manual'")
	check(absf(float(eff_img.get("manual_offset_x_pct", 0.0)) - 25.0) < 0.01, "Saved manual_offset_x_pct = 25%%")
	check(absf(float(eff_img.get("manual_offset_y_pct", 0.0)) + 15.0) < 0.01, "Saved manual_offset_y_pct = -15%%")
	check(absf(float(eff_img.get("manual_scale_x_pct", 0.0)) - 150.0) < 0.01, "Saved manual_scale_x_pct = 150%%")
	check(str(img_scale_opt.get_item_metadata(img_scale_opt.selected)) == "manual", "Properties Escalado reflects Manual")

	# Verify DataManager normalization preserves manual fields
	var norm_el: Dictionary = dm.normalize_ui_element(eff_img, 1, "title")
	check(str(norm_el.get("scale_mode", "")) == "manual" and absf(float(norm_el.get("manual_offset_x_pct", 0.0)) - 25.0) < 0.01, "DataManager normalize_ui_element preserves manual fields")

	# Verify runtime proportionality at 1920x1080 (2x of 960x540 base)
	var d_layer = h.get("design_layer")
	var rt_layer := ClassDB.instantiate("ClockGearsUiDesignLayer") as Control
	get_root().add_child(rt_layer)
	rt_layer.size = Vector2(1920, 1080)
	rt_layer.set_viewport_size_override(Vector2(1920, 1080))
	var active_d: Dictionary = dm.get_ui_screen_design(ed.get_current_screen())
	rt_layer.apply_screen_design(ed.get_current_screen(), active_d)
	await pump(2)
	var ed_ctrl: Control = d_layer.get_element_control(img_id)
	var rt_ctrl: Control = rt_layer.get_element_control(img_id)
	check(ed_ctrl != null and rt_ctrl != null, "Manual image element exists in both Editor and 1920x1080 runtime F5 layer")
	if ed_ctrl and rt_ctrl:
		var ed_tr := ed_ctrl.find_child("Image", true, false) as Control
		var rt_tr := rt_ctrl.find_child("Image", true, false) as Control
		check(ed_tr != null and rt_tr != null, "Image TextureRect built cleanly")
		if ed_tr and rt_tr:
			var ed_ratio := ed_tr.position / Vector2(maxf(1.0, ed_ctrl.size.x), maxf(1.0, ed_ctrl.size.y))
			var rt_ratio := rt_tr.position / Vector2(maxf(1.0, rt_ctrl.size.x), maxf(1.0, rt_ctrl.size.y))
			check(ed_ratio.distance_to(rt_ratio) < 0.03, "Relative pos invariant across 960x540 vs 1920x1080 (%s vs %s)" % [str(ed_ratio), str(rt_ratio)])
	rt_layer.queue_free()

	# Test Undo / Redo for manual adjustment
	ed.apply_undo()
	await pump(2)
	var undone_el: Dictionary = ed.get_effective_element(img_id)
	check(str(undone_el.get("scale_mode", "")) != "manual", "Undo restores previous scale_mode (%s)" % str(undone_el.get("scale_mode", "")))
	ed.apply_redo()
	await pump(2)
	var redone_el: Dictionary = ed.get_effective_element(img_id)
	check(str(redone_el.get("scale_mode", "")) == "manual", "Redo restores manual scale_mode")

	# Test Button shared manual image host across states via Floating Window + Adjust dialog
	ed.select_element(tmp_id)
	var fw3 = ed.open_floating_element_window(tmp_id)
	await pump(2)
	if fw3:
		var fwh3: Dictionary = fw3.get_ui_handles()
		var bn: LineEdit = fwh3.get("btn_normal_edit")
		var bh: LineEdit = fwh3.get("btn_hover_edit")
		bn.text = "res://img/characters/door_wood.png"
		bn.emit_signal("text_changed", bn.text)
		bh.text = "res://test_tileset_64x64.png"
		bh.emit_signal("text_changed", bh.text)
		await pump(2)
	ed.close_all_floating_element_windows()
	ed.open_manual_image_adjust_for_selected()
	await pump(2)
	if adj_dlg:
		adj_dlg.set_values({
			"manual_offset_x_pct": 10.0,
			"manual_offset_y_pct": 0.0,
			"manual_scale_x_pct": 120.0,
			"manual_scale_y_pct": 120.0,
			"manual_keep_aspect": true,
			"manual_integer_scale": false
		})
		adj_dlg.confirm_adjust()
		await pump(2)
	var btn_ctrl: Control = d_layer.get_element_control(tmp_id)
	var btn_mhost := btn_ctrl.find_child("ManualButtonImg", true, false) if btn_ctrl else null
	check(btn_mhost != null, "Button renders shared manual image across states via ManualButtonImg host")

	# Test Screen Background Manual Image Adjust (switched to main_menu which has background)
	db.select_category_by_key("main_menu")
	await pump(4)
	var h_bg: Dictionary = ed.get_ui_handles()
	var bg_ed: LineEdit = h_bg.get("menu_bg_img_edit")
	var bg_sc_opt: OptionButton = h_bg.get("menu_bg_scale_opt")
	var bg_man_btn: Button = h_bg.get("menu_bg_adjust_manual_btn")
	check(bg_man_btn != null and bg_sc_opt != null, "Screen background has Escalado + Ajustar manualmente button")
	if bg_ed and bg_man_btn:
		bg_ed.text = "res://img/characters/door_wood.png"
		bg_ed.emit_signal("text_changed", bg_ed.text)
		await pump(2)
		check(not bg_man_btn.disabled, "Screen bg Ajustar manualmente enabled when bg image assigned")
		ed.open_manual_image_adjust_for_screen_bg()
		await pump(2)
		if adj_dlg and adj_dlg.visible:
			adj_dlg.set_values({
				"manual_offset_x_pct": -12.0,
				"manual_offset_y_pct": 8.0,
				"manual_scale_x_pct": 110.0,
				"manual_scale_y_pct": 110.0,
				"manual_keep_aspect": true,
				"manual_integer_scale": false
			})
			adj_dlg.confirm_adjust()
			await pump(2)
		var d_scr: Dictionary = dm.get_ui_screen_design("title")
		var d_bg: Dictionary = d_scr.get("background", {})
		check(str(d_bg.get("scale_mode", "")) == "manual" and is_equal_approx(float(d_bg.get("manual_offset_x_pct", 0.0)), -12.0), "Screen bg manual adjustment persisted")

	# Cleanup Punto 3 temp elements
	db.select_category_by_key("hud")
	await pump(3)
	ed.select_element(img_id)
	ed.delete_selected_element()
	await pump(1)

	# --- PUNTO 4: BOTONES + BOTÓN / + CARPETA, VISTA PREVIA FANTASMA Y AGRUPACIÓN ---
	db.select_category_by_key("main_menu")
	db.adapt_to_window_size(Vector2i(1366, 768))
	await pump(4)
	var h4: Dictionary = ed.get_ui_handles()
	var add_btn_l: Button = h4.get("layer_add_button_btn")
	var add_fld_l: Button = h4.get("layer_add_folder_btn")
	check(add_btn_l != null and add_fld_l != null, "Panel Disenos has + Boton and + Carpeta buttons")
	if add_btn_l and add_fld_l:
		check(add_btn_l.visible and not add_btn_l.clip_text and add_btn_l.text.contains("Botón") and add_btn_l.size.y >= 31.0, "+ Boton visible, unclipped and >=32px (w=%.1f, h=%.1f)" % [add_btn_l.size.x, add_btn_l.size.y])
		check(add_fld_l.visible and not add_fld_l.clip_text and add_fld_l.text.contains("Carpeta") and add_fld_l.size.y >= 31.0, "+ Carpeta visible, unclipped and >=32px (w=%.1f, h=%.1f)" % [add_fld_l.size.x, add_fld_l.size.y])
		check(absf(add_btn_l.global_position.y - add_fld_l.global_position.y) <= 4.0, "+ Boton and + Carpeta sit side-by-side at 1366x768")
	for ik in ["layer_ungroup_btn", "layer_up_btn", "layer_down_btn", "layer_del_btn"]:
		var ib: Button = h4.get(ik)
		check(ib != null and ib.visible and not ib.tooltip_text.is_empty() and ib.icon != null, "Layer icon button '%s' visible with icon and tooltip" % ik)

	var elems_before: Array = ed.get_effective_screen_elements()
	var cnt_before := elems_before.size()
	var cw: Window = ed.open_create_button_window()
	await pump(2)
	check(cw != null and cw.visible and cw.is_creation_mode(), "+ Boton opens FloatingWindow in creation mode")
	check(cw.title == "Crear botón", "Creation window title is 'Crear botón' (got '%s')" % cw.title)
	check(ed.has_ghost_preview_element(), "Live ghost preview shown on canvas during creation")
	var cwh: Dictionary = cw.get_ui_handles()
	var qshape: OptionButton = cwh.get("quick_shape_opt")
	var cname: LineEdit = cwh.get("name_edit")
	if qshape:
		qshape.select(2) # Redondo
		qshape.emit_signal("item_selected", 2)
		await pump(1)
	if cname:
		cname.text = "Btn Ghost Test"
		cname.emit_signal("text_changed", cname.text)
		await pump(1)
	var ghost_d: Dictionary = ed.get_ghost_preview_element()
	var ghost_sh: Dictionary = ghost_d.get("shape", {})
	check(str(ghost_sh.get("shape_type", "")) == "circle" and int(ghost_d.get("width", 0)) == 96, "Quick shape 'Redondo' updates live ghost preview to circle 96x96")
	check(ed.get_effective_screen_elements().size() == cnt_before, "Ghost editing does not mutate DataManager before Confirm")

	cw.cancel_creation()
	await pump(2)
	check(not ed.has_ghost_preview_element() and not cw.visible, "Cancel creation hides window and clears ghost")
	check(ed.get_effective_screen_elements().size() == cnt_before, "Cancel leaves element count unchanged")

	var undo_cnt_before: int = dm.get_ui_design_undo_count(ed.get_current_screen())
	cw = ed.open_create_button_window()
	await pump(2)
	cwh = cw.get_ui_handles()
	qshape = cwh.get("quick_shape_opt")
	cname = cwh.get("name_edit")
	qshape.select(1) # Cuadrado
	qshape.emit_signal("item_selected", 1)
	cname.text = "Btn Cuadrado Nuevo"
	cname.emit_signal("text_changed", cname.text)
	await pump(1)
	cw.confirm_creation()
	await pump(2)
	var created_id: String = ed.get_selected_element_id()
	var created_el: Dictionary = ed.get_effective_element(created_id)
	check(not ed.has_ghost_preview_element() and str(created_el.get("name", "")) == "Btn Cuadrado Nuevo" and str(created_el.get("shape", {}).get("shape_type", "")) == "square", "Confirm creates button with configured shape and selects it")
	check(dm.get_ui_design_undo_count(ed.get_current_screen()) == undo_cnt_before + 1, "Button creation takes strictly 1 Undo step")
	ed.apply_undo()
	await pump(2)
	check(ed.get_effective_screen_elements().size() == cnt_before, "1 Undo step reverts created button")

	# Test + Carpeta inline prompt + group multiple selected elements preserving visual canvas rects
	var b1_id: String = ed.add_element_at("button", Vector2i(140, 120))
	var b2_id: String = ed.add_element_at("button", Vector2i(360, 250))
	await pump(2)
	var cur_els_pre: Array = ed.get_effective_screen_elements()
	var stg_base := Rect2(0, 0, 960, 540)
	var b1_pos_pre: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(b1_id, cur_els_pre, stg_base, 1.0)
	var b2_pos_pre: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(b2_id, cur_els_pre, stg_base, 1.0)
	ed.select_elements(PackedStringArray([b1_id, b2_id]), b1_id)
	await pump(1)

	ed.begin_inline_folder_create()
	await pump(1)
	var fbox: Control = h4.get("folder_prompt_box")
	var fedit: LineEdit = h4.get("folder_name_edit")
	check(fbox != null and fbox.visible and fedit != null and fedit.text == "Carpeta", "+ Carpeta shows inline prompt defaulting to 'Carpeta'")
	var undo_before_grp: int = dm.get_ui_design_undo_count(ed.get_current_screen())
	var grp_id: String = ed.confirm_inline_folder_create("Carpeta UI")
	await pump(2)
	check(not grp_id.is_empty() and not fbox.visible, "Inline folder confirm creates folder and hides prompt")
	var b1_after: Dictionary = ed.get_effective_element(b1_id)
	var b2_after: Dictionary = ed.get_effective_element(b2_id)
	check(str(b1_after.get("parent_id", "")) == grp_id and str(b2_after.get("parent_id", "")) == grp_id, "Selected elements moved inside created folder")
	var cur_els_post: Array = ed.get_effective_screen_elements()
	var b1_pos_post: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(b1_id, cur_els_post, stg_base, 1.0)
	var b2_pos_post: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(b2_id, cur_els_post, stg_base, 1.0)
	check(b1_pos_pre.position.distance_to(b1_pos_post.position) < 1.5 and b2_pos_pre.position.distance_to(b2_pos_post.position) < 1.5, "Grouping into folder preserves canvas visual positions (%s vs %s)" % [str(b1_pos_pre.position), str(b1_pos_post.position)])
	check(dm.get_ui_design_undo_count(ed.get_current_screen()) == undo_before_grp + 1, "Folder + grouping takes strictly 1 Undo step")

	# Creating button inside selected folder sets parent_id
	ed.select_element(grp_id)
	await pump(1)
	var cw_in: Window = ed.open_create_button_window()
	await pump(1)
	cw_in.confirm_creation()
	await pump(2)
	var child_btn_id: String = ed.get_selected_element_id()
	check(str(ed.get_effective_element(child_btn_id).get("parent_id", "")) == grp_id, "Creating button with folder selected nests button in folder")
	ed.apply_undo() # undo child_btn
	ed.apply_undo() # undo folder group (both grouped children & folder undone in 1 step!)
	await pump(2)
	check(str(ed.get_effective_element(b1_id).get("parent_id", "")) == "", "1 Undo step ungroups folder and restores root parenting")

	# Quick check 16:9 resolutions & Dark / Brass themes for Disenos + Boton/Carpeta buttons
	var ct_inst = Engine.get_singleton("ClockGearsTheme") if Engine.has_singleton("ClockGearsTheme") else null
	var orig_tid: String = str(ct_inst.get_active_theme_id()) if ct_inst else "default"
	var tids: Array = Array(ct_inst.get_available_theme_ids()) if ct_inst else ["default"]
	for tid in tids:
		if ct_inst:
			ct_inst.set_active_theme_id(str(tid))
		for r4 in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
			db.adapt_to_window_size(r4)
			await pump(2)
			check(add_btn_l.size.x + 1.5 >= add_btn_l.get_combined_minimum_size().x and add_fld_l.size.x + 1.5 >= add_fld_l.get_combined_minimum_size().x, "Theme '%s' %s Disenos + Boton / + Carpeta unclipped" % [str(tid), str(r4)])
			check(not add_btn_l.get_global_rect().intersects(add_fld_l.get_global_rect()), "Theme '%s' %s + Boton and + Carpeta do not overlap" % [str(tid), str(r4)])
	if ct_inst:
		ct_inst.set_active_theme_id(orig_tid)

	# Cleanup temp elements cleanly without saving to test_project disk
	ed.apply_undo()
	await pump(2)
	db.queue_free()
	await pump(2)
	if dm_owned and dm and is_instance_valid(dm):
		dm.free()

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
