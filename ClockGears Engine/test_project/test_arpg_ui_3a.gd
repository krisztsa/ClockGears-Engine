extends SceneTree

var passed: int = 0
var failed: int = 0
var _watchdog_armed: bool = true

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

func assert_approx(a: float, b: float, eps: float, msg: String) -> void:
	if absf(a - b) <= eps:
		passed += 1
	else:
		failed += 1
		printerr("[FAIL] ", msg, " (got=", a, ", expected=", b, ")")

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

func find_label_exact(node: Node, txt: String) -> Label:
	for ch in node.get_children():
		if ch is Label and (ch as Label).text == txt:
			return ch as Label
		var sub := find_label_exact(ch, txt)
		if sub != null:
			return sub
	return null

func find_label_contains(node: Node, needle: String) -> Label:
	for ch in node.get_children():
		if ch is Label and needle in (ch as Label).text:
			return ch as Label
		var sub := find_label_contains(ch, needle)
		if sub != null:
			return sub
	return null

func find_button_exact(node: Node, txt: String) -> Button:
	for ch in node.get_children(true):
		if ch is Button and (ch as Button).text == txt:
			return ch as Button
		var sub := find_button_exact(ch, txt)
		if sub != null:
			return sub
	return null

func find_sibling_spinbox_after_label(lbl: Label) -> SpinBox:
	if lbl == null or lbl.get_parent() == null:
		return null
	var p := lbl.get_parent()
	var idx := lbl.get_index()
	if idx + 1 < p.get_child_count():
		var nxt := p.get_child(idx + 1)
		if nxt is SpinBox:
			return nxt as SpinBox
	return null

func find_sibling_option_after_label(lbl: Label) -> OptionButton:
	if lbl == null or lbl.get_parent() == null:
		return null
	var p := lbl.get_parent()
	var idx := lbl.get_index()
	if idx + 1 < p.get_child_count():
		var nxt := p.get_child(idx + 1)
		if nxt is OptionButton:
			return nxt as OptionButton
	return null

func check_no_horizontal_overflow(db: Node, ctx_label: String) -> void:
	var scrolls := find_controls_of_class(db, "ScrollContainer")
	for sc in scrolls:
		var scr := sc as ScrollContainer
		if not scr.is_visible_in_tree():
			continue
		var hbar := scr.get_h_scroll_bar()
		if hbar != null:
			assert_true(not hbar.visible, "%s: no horizontal scrollbar visible on ScrollContainer (%s, scroll_w=%s)" % [ctx_label, scr.name, str(scr.size.x)])
		if scr.get_child_count() > 0:
			var ch := scr.get_child(0)
			if ch is Control:
				var c := ch as Control
				var min_x := c.get_combined_minimum_size().x
				if scr.size.x > 10.0:
					assert_true(min_x <= scr.size.x + 2.0, "%s: content min width (%.1f) fits ScrollContainer width (%.1f)" % [ctx_label, min_x, scr.size.x])

func _init() -> void:
	call_deferred("_start_watchdog")
	call_deferred("_run_tests")

func _start_watchdog() -> void:
	await create_timer(45.0).timeout
	if _watchdog_armed:
		printerr("[TIMEOUT] test_arpg_ui_3a exceeded 45s watchdog!")
		quit(2)

func _run_tests() -> void:
	var data_dir := "res://clockgears_data"
	var snap := backup_json_dir(data_dir)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	assert_true(dm != null, "ClockGearsDataManager singleton exists")

	dm.init_empty()
	dm.set_records("actors", [{
		"id": 1, "name": "Heroe", "hp": 120, "mp": 30, "atk": 15, "def": 10,
		"walk_speed_pct": 100, "run_speed_pct": 100, "attack_speed_pct": 100
	}])
	dm.set_records("npcs", [{
		"id": 1, "name": "Aldeano", "walk_speed_pct": 100, "run_speed_pct": 100
	}])
	dm.set_records("arpg_npcs", [{
		"id": 1, "name": "Aliado ARPG", "walk_speed_pct": 100, "run_speed_pct": 100, "attack_speed_pct": 100
	}])
	dm.set_records("arpg_enemies", [{
		"id": 1, "name": "Goblin ARPG", "desc": "Inicial"
	}])
	var sys_cfg: Dictionary = dm.get_system_config()
	sys_cfg["combat_mode"] = 1 # ARPG activo
	sys_cfg["party"] = [1]
	dm.set_system_config(sys_cfg)
	dm.save_all(data_dir)
	await process_frame

	var db = ClassDB.instantiate("ClockGearsDatabaseDialog")
	root.add_child(db)
	db.size = Vector2i(1366, 768)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await process_frame
	await process_frame

	# =========================================================================
	# 1. ENEMIGOS ARPG: Pestañas General, Combate, Animaciones, Evento
	# =========================================================================
	db.select_category_by_key("arpg_enemies")
	await process_frame
	await process_frame

	var tab_containers := find_controls_of_class(db, "TabContainer")
	assert_true(tab_containers.size() >= 1, "TabContainer present in DB")
	var tabs := tab_containers[0] as TabContainer
	assert_true(tabs.tabs_visible, "arpg_enemies shows tabs bar")
	assert_eq(tabs.get_tab_count(), 4, "arpg_enemies has 4 tabs")
	assert_true(not tabs.is_tab_hidden(0) and tabs.get_tab_title(0) == "General", "Tab 0 is General and visible")
	assert_true(not tabs.is_tab_hidden(1) and tabs.get_tab_title(1) == "Combate", "Tab 1 is Combate and visible")
	assert_true(not tabs.is_tab_hidden(2) and tabs.get_tab_title(2) == "Animaciones", "Tab 2 is Animaciones and visible")
	assert_true(not tabs.is_tab_hidden(3) and tabs.get_tab_title(3) == "Evento", "Tab 3 is Evento and visible")

	# General tab controls on arpg_enemies
	tabs.current_tab = 0
	await process_frame
	await process_frame

	var lbl_lvl := find_label_exact(db, "Nivel:")
	var lbl_exp := find_label_exact(db, "EXP:")
	var lbl_gold := find_label_exact(db, "Oro:")
	assert_true(lbl_lvl != null and lbl_lvl.is_visible_in_tree(), "arpg_enemies shows Nivel")
	assert_true(lbl_exp != null and lbl_exp.is_visible_in_tree(), "arpg_enemies shows EXP")
	assert_true(lbl_gold != null and lbl_gold.is_visible_in_tree(), "arpg_enemies shows Oro")

	var lbl_hp := find_label_exact(db, "HP:")
	var lbl_mp := find_label_exact(db, "MP:")
	var lbl_atk := find_label_exact(db, "ATK:")
	var lbl_def := find_label_exact(db, "DEF:")
	var lbl_mat := find_label_exact(db, "MAT:")
	var lbl_mdf := find_label_exact(db, "MDF:")
	var lbl_agi := find_label_exact(db, "AGI:")
	var lbl_luk := find_label_exact(db, "LUK:")
	assert_true(lbl_hp != null and lbl_hp.is_visible_in_tree(), "arpg_enemies shows HP")
	assert_true(lbl_atk != null and lbl_atk.is_visible_in_tree(), "arpg_enemies shows ATK")
	assert_true(lbl_def != null and lbl_def.is_visible_in_tree(), "arpg_enemies shows DEF")
	assert_true(lbl_mat != null and lbl_mat.is_visible_in_tree(), "arpg_enemies shows MAT")
	assert_true(lbl_mdf != null and lbl_mdf.is_visible_in_tree(), "arpg_enemies shows MDF")
	assert_true(lbl_luk != null and lbl_luk.is_visible_in_tree(), "arpg_enemies shows LUK")
	assert_true(lbl_mp == null or not lbl_mp.is_visible_in_tree(), "arpg_enemies hides MP")
	assert_true(lbl_agi == null or not lbl_agi.is_visible_in_tree(), "arpg_enemies hides AGI")

	var lbl_loot := find_label_exact(db, "Botín del enemigo")
	assert_true(lbl_loot != null and lbl_loot.is_visible_in_tree(), "arpg_enemies shows Botin del enemigo")
	var lbl_drop1 := find_label_exact(db, "Drop 1:")
	var lbl_drop3 := find_label_exact(db, "Drop 3:")
	assert_true(lbl_drop1 != null and lbl_drop1.is_visible_in_tree(), "arpg_enemies shows Drop 1")
	assert_true(lbl_drop3 != null and lbl_drop3.is_visible_in_tree(), "arpg_enemies shows Drop 3")

	var lbl_speeds := find_label_exact(db, "Velocidades")
	var lbl_walk := find_label_exact(db, "Caminar:")
	var lbl_run := find_label_exact(db, "Correr:")
	var lbl_atk_spd := find_label_exact(db, "Velocidad al atacar:")
	var lbl_spd_help := find_label_contains(db, "velocidad base del proyecto")
	assert_true(lbl_speeds != null and lbl_speeds.is_visible_in_tree(), "arpg_enemies shows Velocidades header")
	assert_true(lbl_walk != null and lbl_walk.is_visible_in_tree(), "arpg_enemies shows Caminar")
	assert_true(lbl_run != null and lbl_run.is_visible_in_tree(), "arpg_enemies shows Correr")
	assert_true(lbl_atk_spd != null and lbl_atk_spd.is_visible_in_tree(), "arpg_enemies shows Velocidad al atacar")
	assert_true(lbl_spd_help != null and lbl_spd_help.is_visible_in_tree(), "arpg_enemies shows short helper line under Velocidades")

	var lbl_traits := find_label_contains(db, "Rasgos (Traits)")
	assert_true(lbl_traits == null or not lbl_traits.is_visible_in_tree(), "arpg_enemies General does NOT show Traits box")
	var fit_gen: Dictionary = db.check_layout_fits_window(Vector2i(1366, 768))
	assert_true(bool(fit_gen.get("fits_horizontally", false)), "1366x768 fit check passes on arpg_enemies General")
	check_no_horizontal_overflow(db, "arpg_enemies General 1366x768")

	# Edit General fields of ARPG enemy
	var line_edits := find_controls_of_class(db, "LineEdit")
	var name_edit: LineEdit = null
	for le in line_edits:
		var l := le as LineEdit
		if l.is_visible_in_tree() and l.text == "Goblin ARPG":
			name_edit = l
			break
	assert_true(name_edit != null, "Located Name LineEdit on arpg_enemies")
	if name_edit != null:
		name_edit.text = "Slime ARPG"
		name_edit.emit_signal("text_changed", "Slime ARPG")

	find_sibling_spinbox_after_label(lbl_lvl).value = 7
	find_sibling_spinbox_after_label(lbl_exp).value = 42
	find_sibling_spinbox_after_label(lbl_gold).value = 19
	find_sibling_spinbox_after_label(lbl_hp).value = 320
	find_sibling_spinbox_after_label(lbl_atk).value = 28
	find_sibling_spinbox_after_label(lbl_def).value = 14
	find_sibling_spinbox_after_label(lbl_mat).value = 6
	find_sibling_spinbox_after_label(lbl_mdf).value = 9
	find_sibling_spinbox_after_label(lbl_luk).value = 11
	find_sibling_spinbox_after_label(lbl_walk).value = 130
	find_sibling_spinbox_after_label(lbl_run).value = 160
	find_sibling_spinbox_after_label(lbl_atk_spd).value = 115

	# =========================================================================
	# 2. ENEMIGOS ARPG: Pestaña Combate (Físico + 10 elementos + IA)
	# =========================================================================
	tabs.current_tab = 1
	await process_frame
	await process_frame

	assert_true(lbl_traits == null or not lbl_traits.is_visible_in_tree(), "arpg_enemies Combate hides Traits box")
	var lbl_elem_hdr := find_label_exact(db, "Debilidades y resistencias")
	assert_true(lbl_elem_hdr != null and lbl_elem_hdr.is_visible_in_tree(), "Combate shows Debilidades y resistencias")

	var expected_elems := ["Físico:", "Agua:", "Fuego:", "Hielo:", "Electricidad:", "Viento:", "Luz:", "Oscuridad:", "Astral:", "Tierra:", "Naturaleza:"]
	for en in expected_elems:
		var elbl := find_label_exact(db, en)
		assert_true(elbl != null and elbl.is_visible_in_tree(), "Combate shows element row '%s'" % en)

	var lbl_phys := find_label_exact(db, "Físico:")
	var phys_opt := find_sibling_option_after_label(lbl_phys)
	assert_true(phys_opt != null, "Físico row has OptionButton")
	var phys_spin: SpinBox = null
	if phys_opt != null and phys_opt.get_parent() != null and phys_opt.get_index() + 1 < phys_opt.get_parent().get_child_count():
		phys_spin = phys_opt.get_parent().get_child(phys_opt.get_index() + 1) as SpinBox
	assert_true(phys_spin != null, "Físico row has SpinBox")
	if phys_spin != null:
		phys_spin.value = 50

	var lbl_fire := find_label_exact(db, "Fuego:")
	var fire_opt := find_sibling_option_after_label(lbl_fire)
	var fire_spin: SpinBox = null
	if fire_opt != null and fire_opt.get_parent() != null and fire_opt.get_index() + 1 < fire_opt.get_parent().get_child_count():
		fire_spin = fire_opt.get_parent().get_child(fire_opt.get_index() + 1) as SpinBox
	if fire_spin != null:
		fire_spin.value = 200

	var lbl_ai_hdr := find_label_exact(db, "IA")
	var lbl_ai_type := find_label_exact(db, "Tipo de IA:")
	var lbl_ai_vis := find_label_exact(db, "Radio de visión:")
	var lbl_ai_rng := find_label_exact(db, "Alcance:")
	var lbl_ai_int := find_label_exact(db, "Tiempo entre ataques:")
	var lbl_ai_kb := find_label_exact(db, "Resistencia al retroceso:")
	assert_true(lbl_ai_hdr != null and lbl_ai_hdr.is_visible_in_tree(), "Combate shows IA header")
	assert_true(lbl_ai_type != null and lbl_ai_type.is_visible_in_tree(), "Combate shows Tipo de IA")
	assert_true(lbl_ai_vis != null and lbl_ai_vis.is_visible_in_tree(), "Combate shows Radio de vision")
	assert_true(lbl_ai_rng != null and lbl_ai_rng.is_visible_in_tree(), "Combate shows Alcance")
	assert_true(lbl_ai_int != null and lbl_ai_int.is_visible_in_tree(), "Combate shows Tiempo entre ataques")
	assert_true(lbl_ai_kb != null and lbl_ai_kb.is_visible_in_tree(), "Combate shows Resistencia al retroceso")

	var ai_opt := find_sibling_option_after_label(lbl_ai_type)
	assert_true(ai_opt != null and ai_opt.get_item_count() == 5, "Tipo de IA has 5 items")
	if ai_opt != null:
		ai_opt.select(4) # a_distancia
		ai_opt.emit_signal("item_selected", 4)
	find_sibling_spinbox_after_label(lbl_ai_vis).value = 9.5
	find_sibling_spinbox_after_label(lbl_ai_rng).value = 4.5
	find_sibling_spinbox_after_label(lbl_ai_int).value = 2.2
	find_sibling_spinbox_after_label(lbl_ai_kb).value = 35

	var fit_cmb: Dictionary = db.check_layout_fits_window(Vector2i(1366, 768))
	assert_true(bool(fit_cmb.get("fits_horizontally", false)), "1366x768 fit check passes on arpg_enemies Combate")
	check_no_horizontal_overflow(db, "arpg_enemies Combate 1366x768")

	tabs.current_tab = 2
	await process_frame
	check_no_horizontal_overflow(db, "arpg_enemies Animaciones 1366x768")
	tabs.current_tab = 3
	await process_frame
	check_no_horizontal_overflow(db, "arpg_enemies Evento 1366x768")

	# =========================================================================
	# 3. VELOCIDADES en Actores, NPC, NPC ARPG y Toggle ARPG <-> Turnos
	# =========================================================================
	# 3A: Actores with ARPG active
	db.select_category_by_key("actors")
	await process_frame
	await process_frame
	assert_eq(tabs.current_tab, 0, "Switching to Actors resets tab to General")
	assert_true(lbl_speeds.is_visible_in_tree(), "Actors shows Velocidades")
	assert_true(lbl_atk_spd.is_visible_in_tree(), "Actors (ARPG active) shows Velocidad al atacar")
	check_no_horizontal_overflow(db, "Actors General 1366x768")
	find_sibling_spinbox_after_label(lbl_walk).value = 125
	find_sibling_spinbox_after_label(lbl_run).value = 140
	find_sibling_spinbox_after_label(lbl_atk_spd).value = 135

	# 3B: NPC normal (no attack speed)
	db.select_category_by_key("npcs")
	await process_frame
	await process_frame
	assert_true(lbl_speeds.is_visible_in_tree(), "NPC shows Velocidades")
	assert_true(not lbl_atk_spd.is_visible_in_tree(), "NPC hides Velocidad al atacar")
	check_no_horizontal_overflow(db, "NPC General 1366x768")
	find_sibling_spinbox_after_label(lbl_walk).value = 85
	find_sibling_spinbox_after_label(lbl_run).value = 110

	# 3C: NPC ARPG (has attack speed)
	db.select_category_by_key("arpg_npcs")
	await process_frame
	await process_frame
	assert_true(lbl_speeds.is_visible_in_tree(), "NPC ARPG shows Velocidades")
	assert_true(lbl_atk_spd.is_visible_in_tree(), "NPC ARPG shows Velocidad al atacar")
	check_no_horizontal_overflow(db, "NPC ARPG General 1366x768")
	find_sibling_spinbox_after_label(lbl_walk).value = 95
	find_sibling_spinbox_after_label(lbl_run).value = 130
	find_sibling_spinbox_after_label(lbl_atk_spd).value = 120

	# 3D: Switch System to Turn-Based (0), verify Actor hides Velocidad al atacar without losing 135
	db.select_category_by_key("system")
	await process_frame
	var opts := find_controls_of_class(db, "OptionButton")
	var combat_mode_opt: OptionButton = null
	for o in opts:
		var ob := o as OptionButton
		if ob.is_visible_in_tree() and ob.get_item_count() == 2:
			if "Turnos" in ob.get_item_text(0) or "Action RPG" in ob.get_item_text(1):
				combat_mode_opt = ob
				break
	assert_true(combat_mode_opt != null, "Located combat mode OptionButton in System")
	if combat_mode_opt != null:
		combat_mode_opt.select(0)
		combat_mode_opt.emit_signal("item_selected", 0)
	await process_frame

	db.select_category_by_key("actors")
	await process_frame
	assert_true(lbl_speeds.is_visible_in_tree(), "Actors (Turn-Based) still shows Velocidades")
	assert_true(not lbl_atk_spd.is_visible_in_tree(), "Actors (Turn-Based) hides Velocidad al atacar")
	find_sibling_spinbox_after_label(lbl_walk).value = 128

	# =========================================================================
	# 4. APLICAR & OK PERSISTENCE TO DISK
	# =========================================================================
	var btn_apply := find_button_exact(db, "Aplicar")
	assert_true(btn_apply != null, "Located Aplicar button")
	if btn_apply != null:
		btn_apply.emit_signal("pressed")
	await process_frame

	var btn_ok := find_button_exact(db, "OK")
	assert_true(btn_ok != null, "Located OK button")
	if btn_ok != null:
		btn_ok.emit_signal("pressed")
	await process_frame
	db.queue_free()
	await process_frame

	dm.init_empty()
	dm.load_all(data_dir)
	var arpg_enemies: Array = dm.get_records("arpg_enemies")
	assert_eq(arpg_enemies.size(), 1, "arpg_enemies saved 1 record")
	var ae: Dictionary = arpg_enemies[0]
	assert_eq(ae.get("name", ""), "Slime ARPG", "arpg_enemy name saved")
	assert_eq(int(ae.get("level", 0)), 7, "arpg_enemy level saved")
	assert_eq(int(ae.get("exp", 0)), 42, "arpg_enemy exp saved")
	assert_eq(int(ae.get("gold", 0)), 19, "arpg_enemy gold saved")
	assert_eq(int(ae.get("hp", 0)), 320, "arpg_enemy hp saved")
	assert_eq(int(ae.get("atk", 0)), 28, "arpg_enemy atk saved")
	assert_eq(int(ae.get("def", 0)), 14, "arpg_enemy def saved")
	assert_eq(int(ae.get("mat", 0)), 6, "arpg_enemy mat saved")
	assert_eq(int(ae.get("mdf", 0)), 9, "arpg_enemy mdf saved")
	assert_eq(int(ae.get("luk", 0)), 11, "arpg_enemy luk saved")
	assert_true(not ae.has("mp") and not ae.has("agi"), "arpg_enemy does NOT persist mp or agi")
	assert_eq(int(ae.get("walk_speed_pct", 0)), 130, "arpg_enemy walk_speed_pct saved")
	assert_eq(int(ae.get("run_speed_pct", 0)), 160, "arpg_enemy run_speed_pct saved")
	assert_eq(int(ae.get("attack_speed_pct", 0)), 115, "arpg_enemy attack_speed_pct saved")
	assert_eq(ae.get("ai_type", ""), "a_distancia", "arpg_enemy ai_type saved")
	assert_approx(float(ae.get("vision_radius", 0.0)), 9.5, 0.01, "arpg_enemy vision_radius saved")
	assert_approx(float(ae.get("attack_range", 0.0)), 4.5, 0.01, "arpg_enemy attack_range saved")
	assert_approx(float(ae.get("attack_interval", 0.0)), 2.2, 0.01, "arpg_enemy attack_interval saved")
	assert_eq(int(ae.get("knockback_resist", 0)), 35, "arpg_enemy knockback_resist saved")

	var traits: Array = ae.get("traits", [])
	var phys_val := -1.0
	var fire_val := -1.0
	for t in traits:
		if t is Dictionary and int(t.get("code", 0)) == 11:
			var k := str(t.get("element_key", "")).to_lower()
			if k == "fisico":
				phys_val = float(t.get("value", -1.0))
			elif k == "fuego":
				fire_val = float(t.get("value", -1.0))
	assert_approx(phys_val, 50.0, 0.01, "arpg_enemy Fisico element rate saved (50%)")
	assert_approx(fire_val, 200.0, 0.01, "arpg_enemy Fuego element rate saved (200%)")

	var actors: Array = dm.get_records("actors")
	assert_eq(int(actors[0].get("walk_speed_pct", 0)), 128, "Actor walk_speed_pct saved")
	assert_eq(int(actors[0].get("run_speed_pct", 0)), 140, "Actor run_speed_pct saved")
	assert_eq(int(actors[0].get("attack_speed_pct", 0)), 135, "Actor attack_speed_pct preserved when edited in Turn-Based mode")

	var npcs: Array = dm.get_records("npcs")
	assert_eq(int(npcs[0].get("walk_speed_pct", 0)), 85, "NPC walk_speed_pct saved")
	assert_eq(int(npcs[0].get("run_speed_pct", 0)), 110, "NPC run_speed_pct saved")
	assert_true(not npcs[0].has("attack_speed_pct"), "NPC does NOT store attack_speed_pct")

	var arpg_npcs: Array = dm.get_records("arpg_npcs")
	assert_eq(int(arpg_npcs[0].get("walk_speed_pct", 0)), 95, "ARPG NPC walk_speed_pct saved")
	assert_eq(int(arpg_npcs[0].get("run_speed_pct", 0)), 130, "ARPG NPC run_speed_pct saved")
	assert_eq(int(arpg_npcs[0].get("attack_speed_pct", 0)), 120, "ARPG NPC attack_speed_pct saved")

	restore_json_dir(data_dir, snap)
	dm.init_empty()
	dm.load_all(data_dir)

	print("[TEST_ARPG_UI_3A] passed=", passed, " failed=", failed)
	_watchdog_armed = false
	quit(0 if failed == 0 else 1)
