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

func _backup_file(path: String) -> PackedByteArray:
	if FileAccess.file_exists(path):
		return FileAccess.get_file_as_bytes(path)
	return PackedByteArray()

func _restore_file(path: String, data: PackedByteArray, existed: bool) -> void:
	if existed:
		var fa = FileAccess.open(path, FileAccess.WRITE)
		if fa:
			fa.store_buffer(data)
			fa.close()
	elif FileAccess.file_exists(path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(path))

func _find_accept_dialog_by_title(parent: Node, title_sub: String) -> AcceptDialog:
	for c in parent.get_children():
		if c is AcceptDialog and c.title.contains(title_sub):
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

func _initialize() -> void:
	print("=== VERIFY TROOPS & BATTLEBACK START ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.load_database()

	var data_snap := _snapshot_dir("res://clockgears_data")

	# Seed controlled data for all 4 requirements
	dm.set_category_array(7, [
		{"id": 1, "name": "Cañón musgoso", "level": 3, "exp": 150, "gold": 25, "hp": 40, "mp": 10, "atk": 12, "def": 5, "agi": 5},
		{"id": 2, "name": "Nombre de enemigo extremadamente largo que no cabe", "level": 12, "exp": 10, "gold": 5, "hp": 40, "mp": 10, "atk": 10, "def": 5, "agi": 5}
	])
	dm.set_category_array(0, [
		{"id": 1, "name": "Héroe", "initial_level": 1, "exp": 0, "hp": 250, "mp": 50, "atk": 60, "def": 10, "agi": 25}
	])
	dm.set_category_array(4, [ # CAT_ITEMS
		{"id": 1, "name": "Poción", "hp_recover": 50, "consumable": true}
	])
	var sys: Dictionary = dm.get_system_config()
	sys["party"] = [1]
	dm.set_system_config(sys)
	dm.set_category_array(8, [
		{
			"id": 1,
			"name": "Tropa Alpha",
			"notes": "Nota inicial",
			"battleback1": "",
			"battleback2": "",
			"members": [
				{"enemy_id": 1, "x": 480.0, "y": 280.0, "hidden": false},
				{"enemy_id": 2, "x": 300.0, "y": 260.0, "hidden": false}
			]
		},
		{
			"id": 2,
			"name": "Tropa Legacy",
			"notes": "",
			"battleback1": "",
			"battleback2": "",
			"members": [{"enemy_id": 1, "x": 480.0, "y": 280.0, "hidden": false}]
		}
	])
	dm.save_database()

	var db = ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	await process_frame
	db.load_from_disk()
	db.open_database()
	db.select_category(8) # CAT_TROOPS
	await process_frame

	# ---------------------------------------------------------
	# CHECK 4a: Detalles de Tropas (Nombre y Notas visibles y editables)
	# ---------------------------------------------------------
	var line_edits = _find_controls_of_type(db, "LineEdit")
	var visible_grid_edits: Array = []
	for le in line_edits:
		if le.get_parent() is GridContainer and le.is_visible_in_tree():
			visible_grid_edits.append(le)
	assert_true(visible_grid_edits.size() >= 2, "REQ4a CAT_TROOPS shows visible Nombre & Notas fields (count=%d)" % visible_grid_edits.size())
	var notes_le: LineEdit = null
	for le in visible_grid_edits:
		if le.text == "Nota inicial":
			notes_le = le
			break
	assert_true(notes_le != null, "REQ4a Notes LineEdit loaded 'Nota inicial'")
	if notes_le:
		notes_le.text = "Nota editada"
		notes_le.emit_signal("text_changed", "Nota editada")
		await process_frame
		var saved_troops: Array = dm.get_category_array(8)
		assert_true(String(saved_troops[0].get("notes", "")) == "Nota editada", "REQ4a Notes edit persisted in troop record")

	# ---------------------------------------------------------
	# CHECK 4b: Etiqueta del enemigo en vista previa conserva 'Nv N'
	# ---------------------------------------------------------
	var short_lbl: String = db.get_formatted_troop_enemy_label(0, 0, 260.0)
	var long_lbl: String = db.get_formatted_troop_enemy_label(0, 1, 110.0)
	assert_true(short_lbl == "Cañón musgoso Nv 3", "REQ4b Short enemy label intact: '%s'" % short_lbl)
	assert_true(long_lbl.ends_with(" Nv 12") and long_lbl.contains("…"), "REQ4b Long enemy label truncates name only and keeps 'Nv 12': '%s'" % long_lbl)

	# ---------------------------------------------------------
	# CHECK 1: Fondo en blanco al cargarlo + azul oscuro sin fondo
	# ---------------------------------------------------------
	var tmp_src := ProjectSettings.globalize_path("res://tmp_import_src/repro_battleback_test.png")
	DirAccess.make_dir_recursive_absolute(tmp_src.get_base_dir())
	var img := Image.create_empty(64, 64, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.1, 0.7, 0.2, 1.0))
	img.save_png(tmp_src)

	var floor_src := ProjectSettings.globalize_path("res://tmp_import_src/repro_floor_test.png")
	var fimg := Image.create_empty(320, 100, false, Image.FORMAT_RGBA8)
	fimg.fill(Color(0.3, 0.2, 0.5, 1.0))
	fimg.save_png(floor_src)

	var target_res := "res://img/battlebacks/repro_battleback_test.png"
	var target_global := ProjectSettings.globalize_path(target_res)
	var floor_res := "res://img/battlebacks/repro_floor_test.png"
	var floor_global := ProjectSettings.globalize_path(floor_res)
	for p in [target_global, target_global + ".import", floor_global, floor_global + ".import"]:
		if FileAccess.file_exists(p):
			DirAccess.remove_absolute(p)

	var btn_bg = _find_button_by_text(db, "Cambiar fondo")
	assert_true(btn_bg != null, "REQ1 'Cambiar fondo' button exists")
	btn_bg.emit_signal("pressed")
	await process_frame
	var bg_dlg = _find_accept_dialog_by_title(db, "Cambiar fondo")
	assert_true(bg_dlg != null and bg_dlg.visible, "REQ1 Cambiar fondo dialog opens")
	var sel_btns: Array = []
	for b in _find_controls_of_type(bg_dlg, "Button"):
		if b.text == "Seleccionar...":
			sel_btns.append(b)
	assert_true(sel_btns.size() >= 2, "REQ1 Fondo & Suelo Seleccionar buttons exist")
	sel_btns[0].emit_signal("pressed")
	await process_frame
	var fdlg_bg = null
	for c in bg_dlg.get_children():
		if c is FileDialog:
			fdlg_bg = c
			break
	assert_true(fdlg_bg != null, "REQ1 FileDialog parented under Cambiar fondo dialog (no exclusive child error)")
	if fdlg_bg:
		fdlg_bg.emit_signal("file_selected", tmp_src)
		if fdlg_bg.visible:
			fdlg_bg.hide()
	await process_frame
	var troops_now: Array = dm.get_category_array(8)
	assert_true(String(troops_now[0].get("battleback1", "")) == target_res, "REQ1 battleback1 path saved immediately")

	# Also pick floor via slot 2
	sel_btns[1].emit_signal("pressed")
	await process_frame
	if fdlg_bg:
		fdlg_bg.emit_signal("file_selected", floor_src)
		if fdlg_bg.visible:
			fdlg_bg.hide()
	await process_frame
	bg_dlg.hide()

	# Switch to legacy troop 1 and back to troop 0
	var rec_lists = _find_controls_of_type(db, "ItemList")
	assert_true(rec_lists.size() >= 1, "REQ1 record_list found")
	var rec_list: ItemList = rec_lists[0]
	rec_list.select(1)
	rec_list.emit_signal("item_selected", 1)
	await process_frame
	rec_list.select(0)
	rec_list.emit_signal("item_selected", 0)
	await process_frame

	# Check legacy troop defaults (without battleback1_cfg / battleback2_cfg)
	var leg_bg_cfg: Dictionary = db.get_troop_battleback_cfg(1, 1)
	var leg_fl_cfg: Dictionary = db.get_troop_battleback_cfg(1, 2)
	assert_true(int(leg_bg_cfg.get("mode", -1)) == 0 and int(leg_bg_cfg.get("anchor", -1)) == 0, "REQ2 Legacy troop bg defaults to Cover + Center")
	assert_true(int(leg_fl_cfg.get("anchor", -1)) == 1, "REQ2 Legacy troop floor defaults to Bottom anchor")

	# Verify dark blue background on ClockGearsBattleScene when clearing backgrounds
	var clear_btns: Array = []
	for b in _find_controls_of_type(bg_dlg, "Button"):
		if b.text == "Quitar":
			clear_btns.append(b)
	var probe_scene := ClockGearsBattleScene.new()
	get_root().add_child(probe_scene)
	probe_scene.setup_battle(2, true, true, "", null, true) # Troop 2 has empty bg
	await process_frame
	var dark_bg_rect: ColorRect = null
	for c in probe_scene.get_children():
		if c is ColorRect:
			dark_bg_rect = c
			break
	assert_true(dark_bg_rect != null and dark_bg_rect.color.b > dark_bg_rect.color.r and dark_bg_rect.color.r < 0.15, "REQ1 Battle background without image is dark blue, never white")
	probe_scene.queue_free()

	# ---------------------------------------------------------
	# CHECK 2: Botón 'Ajustar' (Fondo y Suelo)
	# ---------------------------------------------------------
	var btn_adj = _find_button_by_text(db, "Ajustar")
	assert_true(btn_adj != null, "REQ2 'Ajustar' button exists")
	btn_adj.emit_signal("pressed")
	await process_frame
	var adj_menu: PopupMenu = null
	for c in db.get_children():
		if c is PopupMenu and c.item_count == 2 and c.get_item_text(0) == "Fondo" and c.get_item_text(1) == "Suelo":
			adj_menu = c
			break
	assert_true(adj_menu != null, "REQ2 Ajustar PopupMenu has 'Fondo' and 'Suelo'")
	adj_menu.emit_signal("id_pressed", 1)
	await process_frame

	var adj_bg_dlg = _find_accept_dialog_by_title(db, "Ajustar Fondo")
	assert_true(adj_bg_dlg != null and adj_bg_dlg.visible, "REQ2 Ajustar Fondo dialog opens")
	assert_true(_find_controls_of_type(adj_bg_dlg, "SpinBox").is_empty(), "REQ2 Ajustar Fondo has NO numeric SpinBoxes")
	for mtxt in ["Cubrir", "Ajustar", "Estirar", "Mosaico"]:
		assert_true(_find_button_by_text(adj_bg_dlg, mtxt) != null, "REQ2 Mode button '%s' exists" % mtxt)
	var chk_int: CheckBox = null
	for cb in _find_controls_of_type(adj_bg_dlg, "CheckBox"):
		if cb.text.contains("Escala entera"):
			chk_int = cb
			break
	assert_true(chk_int != null, "REQ2 CheckBox 'Escala entera (pixel art)' exists")

	# Test Cancel discards changes
	_find_button_by_text(adj_bg_dlg, "Ajustar").emit_signal("pressed")
	var kev := InputEventKey.new()
	kev.pressed = true
	kev.keycode = KEY_RIGHT
	kev.shift_pressed = true
	adj_bg_dlg.emit_signal("window_input", kev)
	var btn_cancel = _find_button_by_text(adj_bg_dlg, "Cancelar")
	assert_true(btn_cancel != null, "REQ2 Ajustar Cancelar button exists")
	if btn_cancel:
		btn_cancel.emit_signal("pressed")
	adj_bg_dlg.hide()
	await process_frame
	var cfg_after_cancel: Dictionary = db.get_troop_battleback_cfg(0, 1)
	assert_true(float(cfg_after_cancel.get("offset_x", -999.0)) == 0.0 and int(cfg_after_cancel.get("mode", -1)) == 0, "REQ2 Cancelar discards Ajustar changes")

	# Test Confirm saves mode, integer_scale, wheel, arrows, Centrar, Restablecer
	adj_menu.emit_signal("id_pressed", 1)
	await process_frame
	chk_int.set_pressed(true)
	chk_int.emit_signal("toggled", true)
	_find_button_by_text(adj_bg_dlg, "Estirar").emit_signal("pressed")
	# Arrow key 1px and Shift+Arrow 8px -> total +9
	var kev1 := InputEventKey.new()
	kev1.pressed = true
	kev1.keycode = KEY_RIGHT
	adj_bg_dlg.emit_signal("window_input", kev1)
	adj_bg_dlg.emit_signal("window_input", kev)
	# Wheel up inside canvas
	var adj_canvas: Control = null
	for ctrl in _find_controls_of_type(adj_bg_dlg, "Control"):
		if ctrl.focus_mode == Control.FOCUS_ALL and not (ctrl is Button) and not (ctrl is CheckBox):
			adj_canvas = ctrl
			break
	assert_true(adj_canvas != null, "REQ2 Ajustar canvas found")
	if adj_canvas:
		var wev := InputEventMouseButton.new()
		wev.button_index = MOUSE_BUTTON_WHEEL_UP
		wev.pressed = true
		adj_canvas.emit_signal("gui_input", wev)
	var btn_ok = _find_button_by_text(adj_bg_dlg, "Aceptar")
	assert_true(btn_ok != null, "REQ2 Ajustar Aceptar button exists")
	if btn_ok:
		btn_ok.emit_signal("pressed")
	adj_bg_dlg.emit_signal("confirmed")
	adj_bg_dlg.hide()
	await process_frame
	var saved_bg_cfg: Dictionary = db.get_troop_battleback_cfg(0, 1)
	assert_true(int(saved_bg_cfg.get("mode", -1)) == 2, "REQ2 Saved mode Estirar (2)")
	assert_true(bool(saved_bg_cfg.get("integer_scale", false)) == true, "REQ2 Saved Escala entera ON")
	assert_true(is_equal_approx(float(saved_bg_cfg.get("scale", 0.0)), round(float(saved_bg_cfg.get("scale", 0.0)))), "REQ2 Integer scale produces whole multiplier")
	assert_true(is_equal_approx(float(saved_bg_cfg.get("offset_x", 0.0)), 9.0), "REQ2 Arrow (1px) + Shift+Arrow (8px) = 9px offset saved")

	# Test Centrar & Restablecer on Suelo dialog
	adj_menu.emit_signal("id_pressed", 2)
	await process_frame
	var adj_fl_dlg = _find_accept_dialog_by_title(db, "Ajustar Suelo")
	assert_true(adj_fl_dlg != null and adj_fl_dlg.visible, "REQ2 Ajustar Suelo dialog opens independently")
	adj_fl_dlg.emit_signal("window_input", kev)
	_find_button_by_text(adj_fl_dlg, "Centrar").emit_signal("pressed")
	_find_button_by_text(adj_fl_dlg, "Restablecer").emit_signal("pressed")
	adj_fl_dlg.emit_signal("confirmed")
	adj_fl_dlg.hide()
	await process_frame
	var saved_fl_cfg: Dictionary = db.get_troop_battleback_cfg(0, 2)
	assert_true(float(saved_fl_cfg.get("offset_x", -1.0)) == 0.0 and float(saved_fl_cfg.get("offset_y", -1.0)) == 0.0 and int(saved_fl_cfg.get("anchor", -1)) == 1, "REQ2 Restablecer Suelo resets offset (0,0) and bottom anchor")

	# Test shared battleback renderer in BattleScene matches saved offset/cfg
	var bscene_render := ClockGearsBattleScene.new()
	get_root().add_child(bscene_render)
	bscene_render.setup_battle(1, true, true, "", null, true)
	await process_frame
	await process_frame
	var rect_bg: Rect2 = bscene_render.get_drawn_battleback_rect(1)
	var rect_fl: Rect2 = bscene_render.get_drawn_battleback_rect(2)
	var expected_centered_x := (bscene_render.get_viewport().get_visible_rect().size.x - rect_bg.size.x) * 0.5
	var vp_scale_x := bscene_render.get_viewport().get_visible_rect().size.x / 960.0
	assert_true(rect_bg.size.x > 0 and abs(rect_bg.position.x - (expected_centered_x + 9.0 * vp_scale_x)) < 0.5, "REQ2 Shared renderer applied saved offset_x (+9) in battle scene: %s" % str(rect_bg))
	assert_true(rect_fl.size.x > 0, "REQ2 Shared renderer drew floor in battle scene: %s" % str(rect_fl))
	bscene_render.queue_free()

	# Clear test bg via 'Quitar' buttons so we confirm clear works too
	clear_btns[0].emit_signal("pressed")
	await process_frame
	assert_true(String(dm.get_category_array(8)[0].get("battleback1", "x")) == "", "REQ1 Quitar clears battleback1 cleanly")

	# ---------------------------------------------------------
	# CHECK 3: Prueba de Batalla termina, Reintentar, Derrota, Esc, sin mutar DM
	# ---------------------------------------------------------
	dm.set_gold(100)
	dm.change_item("Poción", 3)
	var gold_before: int = dm.get_gold()
	var actor_exp_before: int = int(dm.get_category_array(0)[0].get("exp", 0))

	var btn_test = _find_button_by_text(db, "Prueba de batalla")
	assert_true(btn_test != null, "REQ3 'Prueba de batalla' button exists")
	btn_test.emit_signal("pressed")
	await process_frame

	var test_dlg = _find_accept_dialog_by_title(db, "Prueba de Batalla")
	assert_true(test_dlg != null and test_dlg.visible, "REQ3 Prueba de Batalla dialog opens")
	var bscene: ClockGearsBattleScene = null
	for c in test_dlg.get_children():
		if c is ClockGearsBattleScene:
			bscene = c
			break
	assert_true(bscene != null, "REQ3 ClockGearsBattleScene instantiated inside Battle Test")
	assert_true(bscene.get_signal_connection_list("battle_finished").size() >= 1, "REQ3 battle_finished signal connected")
	var ctrl = bscene.get_controller()
	assert_true(ctrl.get_test_mode() == true, "REQ3 Controller is running in disposable test_mode")

	# Consume item in test mode and kill all enemies
	ctrl.action_item("Poción", 0)
	for en in ctrl.get_enemy_units():
		en.set_hp(0)
	ctrl.check_battle_end()
	await process_frame
	await process_frame

	var btn_cont = _find_button_by_text(test_dlg, "Continuar")
	var btn_retry = _find_button_by_text(test_dlg, "Reintentar")
	assert_true(btn_cont != null and btn_cont.is_visible_in_tree(), "REQ3 Victory summary shows visible 'Continuar' button")
	assert_true(btn_retry != null and btn_retry.is_visible_in_tree(), "REQ3 Victory summary shows visible 'Reintentar' button")

	var found_vic_title := false
	var found_rewards := false
	for l in _find_controls_of_type(test_dlg, "Label"):
		if l.text == "¡Victoria!":
			found_vic_title = true
		if l.text.contains("EXP obtenida: 160") and l.text.contains("Oro obtenido: 30"):
			found_rewards = true
	assert_true(found_vic_title and found_rewards, "REQ3 Victory summary displays '¡Victoria!', EXP (160) & Gold (30)")

	# Non-mutation checks
	assert_true(dm.get_gold() == gold_before, "REQ3 Gold NOT mutated in DataManager (%d == %d)" % [dm.get_gold(), gold_before])
	assert_true(dm.has_item("Poción", 3) and not dm.has_item("Poción", 4), "REQ3 Inventory NOT mutated by Battle Test")
	assert_true(int(dm.get_category_array(0)[0].get("exp", -1)) == actor_exp_before, "REQ3 Persistent Actor EXP NOT mutated by Battle Test")

	# Retry same troop with fresh snapshot
	btn_retry.emit_signal("pressed")
	await process_frame
	var bscene2: ClockGearsBattleScene = null
	for c in test_dlg.get_children():
		if c is ClockGearsBattleScene:
			bscene2 = c
			break
	assert_true(bscene2 != null and not btn_cont.is_visible_in_tree(), "REQ3 Reintentar restarts same troop and hides summary")
	var ctrl2 = bscene2.get_controller()
	assert_true(ctrl2.get_enemy_units()[0].get_hp() == 40, "REQ3 Reintentar restored initial enemy HP (40)")

	# Defeat flow check
	for pu in ctrl2.get_party_units():
		pu.set_hp(0)
	ctrl2.check_battle_end()
	await process_frame
	await process_frame
	var found_defeat := false
	for l in _find_controls_of_type(test_dlg, "Label"):
		if l.text == "Derrota":
			found_defeat = true
			break
	var btn_exit = _find_button_by_text(test_dlg, "Salir")
	assert_true(found_defeat and btn_exit != null and btn_exit.is_visible_in_tree() and btn_retry.is_visible_in_tree(), "REQ3 Defeat shows 'Derrota' with 'Reintentar' and 'Salir'")

	# Esc closes Battle Test anytime
	var esc_ev := InputEventKey.new()
	esc_ev.pressed = true
	esc_ev.keycode = KEY_ESCAPE
	test_dlg.emit_signal("window_input", esc_ev)
	await process_frame
	assert_true(not test_dlg.visible, "REQ3 Esc closes Battle Test dialog anytime")

	# Re-open, win, press any key closes summary cleanly
	btn_test.emit_signal("pressed")
	await process_frame
	var bscene3: ClockGearsBattleScene = null
	for c in test_dlg.get_children():
		if c is ClockGearsBattleScene:
			bscene3 = c
			break
	var ctrl3 = bscene3.get_controller()
	for en in ctrl3.get_enemy_units():
		en.set_hp(0)
	ctrl3.check_battle_end()
	await process_frame
	await process_frame
	var any_key := InputEventKey.new()
	any_key.pressed = true
	any_key.keycode = KEY_SPACE
	test_dlg.emit_signal("window_input", any_key)
	await process_frame
	assert_true(not test_dlg.visible, "REQ3 Key press closes Victory summary cleanly")

	# Cleanup all test artifacts and restore JSONs
	for p in [target_global, target_global + ".import", floor_global, floor_global + ".import", tmp_src, floor_src]:
		if FileAccess.file_exists(p):
			DirAccess.remove_absolute(p)
	var tmp_dir := tmp_src.get_base_dir()
	if DirAccess.dir_exists_absolute(tmp_dir):
		DirAccess.remove_absolute(tmp_dir)
	_restore_dir("res://clockgears_data", data_snap)
	dm.reset_runtime_state()
	dm.load_database()

	print("=== VERIFY RESULT: failed=%d ===" % failed)
	quit(1 if failed > 0 else 0)
