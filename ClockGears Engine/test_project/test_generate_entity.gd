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

func _make_sample_anim(sheet_path: String) -> Dictionary:
	return {
		"mover_abajo": {
			"cols": 4,
			"fps": 10.0,
			"frame_h": 32,
			"frame_w": 32,
			"frames": [0],
			"frames_data": [{
				"duration": 0.1,
				"h": 32,
				"sheet": sheet_path,
				"w": 32,
				"x": 0,
				"y": 0
			}],
			"loop": false,
			"rows": 1,
			"spritesheet_path": sheet_path
		}
	}

func _find_preview_texture(dlg: Node) -> Texture2D:
	var rects := find_controls_of_class(dlg, "TextureRect")
	for r in rects:
		var tr := r as TextureRect
		if tr.texture != null:
			return tr.texture
	return null

func _init() -> void:
	call_deferred("_start_watchdog")
	call_deferred("_run_tests")

func _start_watchdog() -> void:
	await create_timer(45.0).timeout
	if _watchdog_armed:
		printerr("[TIMEOUT] test_generate_entity exceeded 45s watchdog!")
		quit(2)

func _run_tests() -> void:
	var data_dir := "res://clockgears_data"
	var snap := backup_json_dir(data_dir)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	assert_true(dm != null, "ClockGearsDataManager singleton exists")

	var sprite_path := "res://tmp_import_src/tanda1a_test/char4.png"
	var sample_anim := _make_sample_anim(sprite_path)

	dm.init_empty()
	dm.set_records("npcs", [{
		"id": 1,
		"name": "Aldeano Test",
		"walk_speed_pct": 100,
		"run_speed_pct": 100,
		"animations": sample_anim
	}])
	dm.set_records("arpg_npcs", [{
		"id": 1,
		"name": "Aliado ARPG Test",
		"walk_speed_pct": 100,
		"run_speed_pct": 100,
		"attack_speed_pct": 100,
		"animations": sample_anim
	}])
	dm.set_records("enemies", [{
		"id": 1,
		"name": "Slime Turnos",
		"hp": 50,
		"battler_name": sprite_path,
		"graphic": sprite_path
	}])
	dm.set_records("troops", [{
		"id": 1,
		"name": "Patrulla Slime",
		"members": [{"enemy_id": 1, "x": 300, "y": 200}]
	}])
	dm.set_records("arpg_enemies", [{
		"id": 1,
		"name": "Goblin ARPG Test",
		"hp": 100,
		"walk_speed_pct": 100,
		"run_speed_pct": 100,
		"attack_speed_pct": 100,
		"animations": sample_anim
	}])

	# 1. Context menu ordering check
	var ctx := PopupMenu.new()
	var quick := PopupMenu.new()
	ctx.add_child(quick)
	ClockGearsGenerateEntityDialog.populate_event_context_menu(ctx, quick, false, true)

	var idx_create := -1
	var idx_quick := -1
	var idx_generate := -1
	for i in range(ctx.item_count):
		var t := ctx.get_item_text(i)
		if t == "Crear Nuevo Evento...":
			idx_create = i
		elif t == "Creación Rápida de Eventos":
			idx_quick = i
		elif t == "Generar entidad...":
			idx_generate = i

	assert_true(idx_create >= 0, "Context menu includes 'Crear Nuevo Evento...'")
	assert_true(idx_quick >= 0, "Context menu includes 'Creación Rápida de Eventos'")
	assert_true(idx_generate >= 0, "Context menu includes 'Generar entidad...'")
	assert_eq(idx_generate, idx_quick + 1, "'Generar entidad...' sits directly below 'Creación Rápida de Eventos'")

	ClockGearsGenerateEntityDialog.populate_event_context_menu(ctx, quick, true, false)
	var has_gen_on_existing := false
	for i in range(ctx.item_count):
		if ctx.get_item_text(i) == "Generar entidad...":
			has_gen_on_existing = true
	assert_true(not has_gen_on_existing, "'Generar entidad...' is hidden when tile already has an event")
	ctx.free()

	# 2. Dialog window UI, tabs, buttons, 1366x768 fit, preview, empty states, double click
	var dlg := ClassDB.instantiate("ClockGearsGenerateEntityDialog") as AcceptDialog
	assert_true(dlg != null, "ClockGearsGenerateEntityDialog instantiates")
	root.add_child(dlg)

	dlg.open_for_cell(5, 6)
	await process_frame

	assert_eq(dlg.get_target_cell(), Vector2i(5, 6), "Dialog records clicked tile cell (5, 6)")

	var tab_bars := find_controls_of_class(dlg, "TabBar")
	assert_eq(tab_bars.size(), 1, "Dialog has 1 top TabBar")
	if tab_bars.size() == 1:
		var tb := tab_bars[0] as TabBar
		var tab_titles: Array[String] = []
		for i in range(tb.tab_count):
			tab_titles.append(tb.get_tab_title(i))
		assert_eq(tab_titles, ["NPC", "ARPG NPC", "ENEMIGO", "ARPG ENEMIGO", "FLORA", "FAUNA"], "Tabs appear in exact required order")

	assert_eq(dlg.get_ok_button().text, "Generar", "Confirm button labeled 'Generar'")
	assert_eq(dlg.get_cancel_button().text, "Cancelar", "Cancel button labeled 'Cancelar'")

	var fit: Dictionary = dlg.check_layout_fits_window(Vector2i(1366, 768))
	assert_true(bool(fit.get("fits_horizontally", false)), "Dialog fits horizontally in 1366x768: " + str(fit))
	assert_true(bool(fit.get("fits_vertically", false)), "Dialog fits vertically in 1366x768: " + str(fit))

	for tab_idx in [0, 1, 2, 3]:
		dlg.select_tab(tab_idx)
		await process_frame
		assert_true(dlg.get_listed_count() >= 1, "Tab %d lists seeded record(s)" % tab_idx)
		dlg.select_list_index(0)
		await process_frame
		assert_true(not dlg.get_ok_button().disabled, "'Generar' enabled on populated tab %d" % tab_idx)
		var tex := _find_preview_texture(dlg)
		assert_true(tex != null, "Sprite preview renders texture on tab %d" % tab_idx)
		for sc in find_controls_of_class(dlg, "ScrollContainer"):
			var scr := sc as ScrollContainer
			if scr.is_visible_in_tree() and scr.get_h_scroll_bar() != null:
				assert_true(not scr.get_h_scroll_bar().visible, "No horizontal scrollbar on tab %d" % tab_idx)

	# Double click on list item emits entity_chosen -> triggers generation signal
	var chosen_Capture := []
	dlg.connect("entity_chosen", func(tab: int, rec_id: int, rec_name: String, cx: int, cy: int):
		chosen_Capture.append([tab, rec_id, rec_name, cx, cy])
	)
	dlg.select_tab(0)
	dlg.select_list_index(0)
	var lists := find_controls_of_class(dlg, "ItemList")
	assert_eq(lists.size(), 1, "Dialog has 1 left ItemList")
	if lists.size() == 1:
		(lists[0] as ItemList).emit_signal("item_activated", 0)
	await process_frame
	assert_eq(chosen_Capture.size(), 1, "Double clicking entry emits entity_chosen")
	if chosen_Capture.size() == 1:
		assert_eq(chosen_Capture[0], [0, 1, "Aldeano Test", 5, 6], "Double click payload matches selected NPC and cell")

	# FLORA & FAUNA must open empty with Generar disabled
	for empty_tab in [4, 5]:
		dlg.select_tab(empty_tab)
		await process_frame
		assert_eq(dlg.get_listed_count(), 0, "Tab %d (FLORA/FAUNA) opens empty" % empty_tab)
		assert_true(dlg.get_ok_button().disabled, "'Generar' disabled on empty tab %d" % empty_tab)

	# Empty populated category (NPC) also disables Generar
	var saved_npcs: Array = dm.get_records("npcs").duplicate(true)
	dm.set_records("npcs", [])
	dlg.select_tab(0)
	await process_frame
	assert_eq(dlg.get_listed_count(), 0, "Empty NPC category lists 0 entries")
	assert_true(dlg.get_ok_button().disabled, "'Generar' disabled when NPC list is empty")
	dm.set_records("npcs", saved_npcs)

	# 3. Map event creation & Undo/Redo checks
	var map := ClockGearsMap.new()
	map.ensure_default_layers()
	root.add_child(map)
	var events_parent: Node = map.get_node_or_null("Eventos")
	assert_true(events_parent != null, "ClockGearsMap has Eventos container")

	var ur := UndoRedo.new()

	# 3a. NPC (tab 0 -> behavior_type = 1)
	var ev_npc := ClockGearsGenerateEntityDialog.create_entity_event_on_map(map, 0, 1, 2, 3, 32, true, ur) as ClockGearsEvent
	assert_true(ev_npc != null, "NPC event created on map")
	if ev_npc != null:
		assert_eq(ev_npc.position, Vector2(64, 96), "NPC event placed at cell (2,3) * 32")
		assert_eq(ev_npc.behavior_type, 1, "NPC event uses behavior_type 1 (NPC)")
		assert_eq(ev_npc.behavior_ref_id, 1, "NPC event links behavior_ref_id 1")
		assert_true(ev_npc.get_parent() == events_parent, "NPC event added to Events container")
		ur.undo()
		assert_true(ev_npc.get_parent() == null, "Undo removes generated NPC event")
		ur.redo()
		assert_true(ev_npc.get_parent() == events_parent, "Redo restores generated NPC event")

	# 3b. ARPG NPC (tab 1 -> behavior_type = 2)
	var ev_anpc := ClockGearsGenerateEntityDialog.create_entity_event_on_map(map, 1, 1, 4, 3, 32, true, ur) as ClockGearsEvent
	assert_true(ev_anpc != null, "ARPG NPC event created on map")
	if ev_anpc != null:
		assert_eq(ev_anpc.position, Vector2(128, 96), "ARPG NPC placed at cell (4,3) * 32")
		assert_eq(ev_anpc.behavior_type, 2, "ARPG NPC uses behavior_type 2")
		assert_eq(ev_anpc.behavior_ref_id, 1, "ARPG NPC links behavior_ref_id 1")
		ur.undo()
		assert_true(ev_anpc.get_parent() == null, "Undo removes generated ARPG NPC event")
		ur.redo()
		assert_true(ev_anpc.get_parent() == events_parent, "Redo restores generated ARPG NPC event")

	# 3c. ARPG ENEMIGO (tab 3 -> behavior_type = 3)
	var ev_aen := ClockGearsGenerateEntityDialog.create_entity_event_on_map(map, 3, 1, 6, 3, 32, true, ur) as ClockGearsEvent
	assert_true(ev_aen != null, "ARPG ENEMIGO event created on map")
	if ev_aen != null:
		assert_eq(ev_aen.position, Vector2(192, 96), "ARPG ENEMIGO placed at cell (6,3) * 32")
		assert_eq(ev_aen.behavior_type, 3, "ARPG ENEMIGO uses behavior_type 3")
		assert_eq(ev_aen.behavior_ref_id, 1, "ARPG ENEMIGO links behavior_ref_id 1")
		ur.undo()
		assert_true(ev_aen.get_parent() == null, "Undo removes generated ARPG ENEMIGO event")
		ur.redo()
		assert_true(ev_aen.get_parent() == events_parent, "Redo restores generated ARPG ENEMIGO event")

	# 3d. ENEMIGO (Troop -> behavior_type = 0, Iniciar Batalla code=306, Player Touch trigger=1)
	var ev_troop := ClockGearsGenerateEntityDialog.create_entity_event_on_map(map, 2, 1, 8, 3, 32, true, ur) as ClockGearsEvent
	assert_true(ev_troop != null, "ENEMIGO (Troop) event created on map")
	if ev_troop != null:
		assert_eq(ev_troop.position, Vector2(256, 96), "Troop event placed at cell (8,3) * 32")
		assert_eq(ev_troop.behavior_type, 0, "Troop event uses Normal behavior_type 0")
		assert_eq(ev_troop.trigger_type, 1, "Troop event triggers on Player Touch (1)")
		assert_eq(ev_troop.commands.size(), 1, "Troop event has 1 battle command")
		if ev_troop.commands.size() == 1:
			var cmd: Dictionary = ev_troop.commands[0]
			assert_eq(cmd.get("type", ""), "battle", "Command type is 'battle'")
			assert_eq(int(cmd.get("code", 0)), 306, "Command code is 306 (Iniciar Batalla)")
			assert_eq(int(cmd.get("troop_id", 0)), 1, "Command targets troop_id 1")
		ur.undo()
		assert_true(ev_troop.get_parent() == null, "Undo removes generated Troop event")
		ur.redo()
		assert_true(ev_troop.get_parent() == events_parent, "Redo restores generated Troop event")

	# 3e. FLORA/FAUNA must not create an event
	var ev_flora = ClockGearsGenerateEntityDialog.create_entity_event_on_map(map, 4, 1, 9, 3, 32, true, ur)
	assert_true(ev_flora == null, "FLORA tab produces no map event")
	var ev_fauna = ClockGearsGenerateEntityDialog.create_entity_event_on_map(map, 5, 1, 9, 4, 32, true, ur)
	assert_true(ev_fauna == null, "FAUNA tab produces no map event")

	# 3f. Normal 'Crear Nuevo Evento...' blank event sanity check
	var ev_blank := ClockGearsEvent.new()
	ev_blank.position = Vector2(320, 96)
	events_parent.add_child(ev_blank)
	assert_eq(ev_blank.behavior_type, 0, "Blank event keeps default behavior_type 0")
	assert_eq(ev_blank.commands.size(), 0, "Blank event starts with empty commands")

	# Cleanup
	ur.free()
	map.queue_free()
	dlg.queue_free()
	await process_frame

	restore_json_dir(data_dir, snap)
	dm.load_all()

	print("[TEST_GENERATE_ENTITY] passed=%d failed=%d" % [passed, failed])
	_watchdog_armed = false
	quit(0 if failed == 0 else 1)
