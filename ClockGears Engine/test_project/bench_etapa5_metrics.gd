extends SceneTree

func _calc_stats(samples: Array[float]) -> Dictionary:
	if samples.is_empty():
		return {"mean": 0.0, "p95": 0.0, "max": 0.0}
	var sorted := samples.duplicate()
	sorted.sort()
	var sum := 0.0
	for v in sorted:
		sum += v
	var mean := sum / float(sorted.size())
	var idx95 := mini(sorted.size() - 1, int(floor(0.95 * float(sorted.size() - 1))))
	return {
		"mean": mean,
		"p95": sorted[idx95],
		"max": sorted[sorted.size() - 1],
	}

func _step_frames(n: int = 2) -> void:
	for i in range(n):
		await process_frame

func _initialize() -> void:
	call_deferred("_run_bench")

func _run_bench() -> void:
	print("=== BENCHMARK ETAPA 5C ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.init_empty()

	# 1. Abrir el editor de menús (Database -> ui_designer)
	var samples_open_ed: Array[float] = []
	for i in range(5):
		var db := ClockGearsDatabaseDialog.new()
		var t0 := Time.get_ticks_usec()
		root.add_child(db)
		db.popup_centered(Vector2i(1366, 768))
		db.select_category_by_key("ui_designer")
		await _step_frames(1)
		var dt := float(Time.get_ticks_usec() - t0) / 1000.0
		samples_open_ed.append(dt)
		db.queue_free()
		await _step_frames(1)
	var s1 := _calc_stats(samples_open_ed)
	print("M1 Abrir editor menus (ms): mean=%.2f p95=%.2f max=%.2f nodes=%d obj=%d mem=%.2fMB" % [
		s1["mean"], s1["p95"], s1["max"],
		int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)),
		int(Performance.get_monitor(Performance.OBJECT_COUNT)),
		float(OS.get_static_memory_usage()) / (1024.0 * 1024.0)
	])

	# Preparar BD + editor para M2
	var db2 := ClockGearsDatabaseDialog.new()
	root.add_child(db2)
	db2.popup_centered(Vector2i(1366, 768))
	db2.select_category_by_key("ui_designer")
	await _step_frames(2)
	var ed: ClockGearsUiDesignEditor = db2.get_ui_designer_editor()

	# Crear dos menús de 100 widgets cada uno
	var m100_a: String = ed.create_menu_via_wizard({"name": "Bench 100 A", "template_id": "minimalist", "buttons": []})
	var m100_b: String = ed.create_menu_via_wizard({"name": "Bench 100 B", "template_id": "minimalist", "buttons": []})
	for m_id in [m100_a, m100_b]:
		var els: Array = []
		for k in range(100):
			var el := ClockGearsDataManager.make_default_ui_element("text", m_id)
			el["id"] = "w_%d" % k
			el["name"] = "Widget %d" % k
			el["offset_x"] = (k % 10) * 90 + 16
			el["offset_y"] = int(k / 10) * 48 + 16
			els.append(el)
		var d: Dictionary = dm.get_ui_screen_design(m_id)
		d["elements"] = els
		d["custom_enabled"] = true
		dm.set_ui_screen_design(m_id, d, false)

	var samples_switch_100: Array[float] = []
	var node_before_switch := int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT))
	for i in range(10):
		var tgt := m100_a if (i % 2 == 0) else m100_b
		var t0 := Time.get_ticks_usec()
		ed.select_screen(tgt)
		var dt := float(Time.get_ticks_usec() - t0) / 1000.0
		samples_switch_100.append(dt)
		await _step_frames(1)
	var s2 := _calc_stats(samples_switch_100)
	print("M2 Cambiar menu 100 widgets (ms): mean=%.2f p95=%.2f max=%.2f nodes=%d (+%d) obj=%d mem=%.2fMB" % [
		s2["mean"], s2["p95"], s2["max"],
		int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)),
		int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)) - node_before_switch,
		int(Performance.get_monitor(Performance.OBJECT_COUNT)),
		float(OS.get_static_memory_usage()) / (1024.0 * 1024.0)
	])
	db2.queue_free()
	await _step_frames(2)

	# 3. Abrir un menú radial de 12 ranuras en el juego
	var rad_id: String = dm.create_custom_menu("Radial 12", {})
	var rd: Dictionary = dm.get_ui_screen_design(rad_id)
	var slots: Array = []
	for k in range(12):
		slots.append({"id": "r_%d" % k, "title": "Slot %d" % k, "description": "Desc", "action": "close"})
	rd["layout"] = {"mode": "radial", "radial_center_x": 480, "radial_center_y": 270, "radial_radius": 160, "radial_slots": slots}
	dm.set_ui_screen_design(rad_id, rd, false)

	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	await _step_frames(1)
	var samples_rad: Array[float] = []
	for i in range(10):
		var t0 := Time.get_ticks_usec()
		gm.open_menu_by_id(rad_id, false, false, true)
		var dt := float(Time.get_ticks_usec() - t0) / 1000.0
		samples_rad.append(dt)
		gm.close_menu()
		await _step_frames(1)
	var s3 := _calc_stats(samples_rad)
	print("M3 Menu radial 12 ranuras (ms): mean=%.2f p95=%.2f max=%.2f nodes=%d obj=%d mem=%.2fMB" % [
		s3["mean"], s3["p95"], s3["max"],
		int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)),
		int(Performance.get_monitor(Performance.OBJECT_COUNT)),
		float(OS.get_static_memory_usage()) / (1024.0 * 1024.0)
	])

	# 4. Abrir y cerrar el menú del juego 50 veces
	var n0 := int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT))
	var o0 := int(Performance.get_monitor(Performance.OBJECT_COUNT))
	var m0 := float(OS.get_static_memory_usage()) / (1024.0 * 1024.0)
	var samples_toggle: Array[float] = []
	for i in range(50):
		var t0 := Time.get_ticks_usec()
		gm.open_menu_by_id("game_menu_main", false, false, true)
		gm.close_menu()
		var dt := float(Time.get_ticks_usec() - t0) / 1000.0
		samples_toggle.append(dt)
	await _step_frames(3)
	var n1 := int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT))
	var o1 := int(Performance.get_monitor(Performance.OBJECT_COUNT))
	var m1 := float(OS.get_static_memory_usage()) / (1024.0 * 1024.0)
	var s4 := _calc_stats(samples_toggle)
	print("M4 Abrir/cerrar game_menu x50 (ms): mean=%.2f p95=%.2f max=%.2f node_delta=%d obj_delta=%d mem_delta=%.2fMB" % [
		s4["mean"], s4["p95"], s4["max"],
		n1 - n0, o1 - o0, m1 - m0
	])

	gm.queue_free()
	await _step_frames(2)
	dm.load_all()
	quit(0)
