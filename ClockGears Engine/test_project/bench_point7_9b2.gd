extends SceneTree

# Benchmark Punto 7 (9B-2): Medición ANTES y DESPUÉS sin suposiciones.
# Ejecuta los 8 escenarios requeridos (50 repeticiones c/u cuando aplica),
# midiendo frío #1 vs caliente (media, p95, max), nodos, Controls, memoria y objetos sin liberar.

var results: Array[Dictionary] = []

func _step_process(n: int = 1) -> void:
	for _i in range(n):
		await process_frame

func _step_physics(n: int = 1) -> void:
	for _i in range(n):
		await physics_frame

func _count_controls(n: Node) -> int:
	if n == null:
		return 0
	var c := 1 if (n is Control) else 0
	for ch in n.get_children():
		c += _count_controls(ch)
	return c

func _calc_stats(samples: Array[float]) -> Dictionary:
	if samples.is_empty():
		return {"avg": 0.0, "p95": 0.0, "max": 0.0}
	var sorted := samples.duplicate()
	sorted.sort()
	var sum := 0.0
	for v in sorted:
		sum += v
	var idx95 := mini(sorted.size() - 1, int(ceil(float(sorted.size()) * 0.95)) - 1)
	if idx95 < 0:
		idx95 = 0
	return {
		"avg": sum / float(sorted.size()),
		"p95": sorted[idx95],
		"max": sorted[sorted.size() - 1],
	}

func _drain_and_stats() -> Dictionary:
	await _step_process(4)
	return {
		"nodes": int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)),
		"objs": int(Performance.get_monitor(Performance.OBJECT_COUNT)),
		"orphans": int(Performance.get_monitor(Performance.OBJECT_ORPHAN_NODE_COUNT)),
		"mem_mb": Performance.get_monitor(Performance.MEMORY_STATIC) / (1024.0 * 1024.0),
	}

func _record(name: String, cold_ms: float, op_samples: Array[float], frame_samples: Array[float], peak_nodes: int, peak_ctrls: int, before_st: Dictionary, after_st: Dictionary) -> void:
	var op_st := _calc_stats(op_samples)
	var fr_st := _calc_stats(frame_samples)
	var rec := {
		"name": name,
		"cold_ms": cold_ms,
		"op_avg": op_st["avg"],
		"op_p95": op_st["p95"],
		"op_max": op_st["max"],
		"fr_avg": fr_st["avg"],
		"fr_p95": fr_st["p95"],
		"fr_max": fr_st["max"],
		"peak_nodes": peak_nodes,
		"peak_ctrls": peak_ctrls,
		"mem_start": before_st["mem_mb"],
		"mem_end": after_st["mem_mb"],
		"mem_delta_kb": (after_st["mem_mb"] - before_st["mem_mb"]) * 1024.0,
		"unfreed_objs": after_st["objs"] - before_st["objs"],
		"unfreed_nodes": after_st["nodes"] - before_st["nodes"],
		"orphans_delta": after_st["orphans"] - before_st["orphans"],
	}
	results.append(rec)
	print("SCENARIO | %s | cold=%.2fms | op(avg/p95/max)=%.2f/%.2f/%.2f ms | frame(avg/p95/max)=%.2f/%.2f/%.2f ms | nodes=%d ctrls=%d | mem=%.2f->%.2fMB (d=%.1fKB) | leaked_objs=%d leaked_nodes=%d orphans=%d" % [
		name, cold_ms,
		op_st["avg"], op_st["p95"], op_st["max"],
		fr_st["avg"], fr_st["p95"], fr_st["max"],
		peak_nodes, peak_ctrls,
		before_st["mem_mb"], after_st["mem_mb"], rec["mem_delta_kb"],
		rec["unfreed_objs"], rec["unfreed_nodes"], rec["orphans_delta"],
	])

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== BENCHMARK PUNTO 7 (9B-2) ===")
	var start_from := 1
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--from="):
			start_from = int(a.trim_prefix("--from="))
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm != null:
		dm.load_all("res://clockgears_data")
	await _step_process(3)

	# ------------------------------------------------------------------
	# Escenario 1: Abrir/cerrar Base de Datos y recorrer capítulos/pestañas (50x)
	# ------------------------------------------------------------------
	var test_cats := ["actors", "classes", "skills", "items", "enemies", "troops", "tilesets", "system", "main_menu", "game_menu", "credits", "hud", "ui_designer", "game_over"]
	if start_from <= 1:
		var b1 := await _drain_and_stats()
		var s1_cold := 0.0
		var s1_ops: Array[float] = []
		var s1_frs: Array[float] = []
		var s1_peak_n := 0
		var s1_peak_c := 0
		for it in range(51):
			var t0 := Time.get_ticks_usec()
			var db := ClockGearsDatabaseDialog.new()
			root.add_child(db)
			db.open_database()
			for ck in test_cats:
				db.select_category_by_key(ck)
			var tf0 := Time.get_ticks_usec()
			await _step_process(1)
			var tf1 := Time.get_ticks_usec()
			s1_peak_n = maxi(s1_peak_n, int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)))
			s1_peak_c = maxi(s1_peak_c, _count_controls(db))
			db.hide()
			db.queue_free()
			await _step_process(1)
			var t1 := Time.get_ticks_usec()
			var dt_op := float(t1 - t0) / 1000.0
			var dt_fr := float(tf1 - tf0) / 1000.0
			if it == 0:
				s1_cold = dt_op
			else:
				s1_ops.append(dt_op)
				s1_frs.append(dt_fr)
		var a1 := await _drain_and_stats()
		_record("1_DB_50x_all_chapters", s1_cold, s1_ops, s1_frs, s1_peak_n, s1_peak_c, b1, a1)

	# ------------------------------------------------------------------
	# Escenario 2: Abrir Editor de Eventos con 200 comandos (50x)
	# ------------------------------------------------------------------
	if start_from <= 2:
		var b2 := await _drain_and_stats()
		var ev2 := ClockGearsEvent.new()
		root.add_child(ev2)
		var cmds200: Array = []
		for ci in range(200):
			var m := ci % 4
			if m == 0:
				cmds200.append({"type": "message", "params": {"text": "Mensaje de prueba %d con acento áéíóú." % ci, "position": 2}})
			elif m == 1:
				cmds200.append({"type": "control_switch", "params": {"switch_id": 1, "value": true}})
			elif m == 2:
				cmds200.append({"type": "wait", "params": {"frames": 15}})
			else:
				cmds200.append({"type": "fade", "params": {"fade_type": 0, "frames": 30}})
		ev2.set_pages([{"commands": cmds200, "trigger": 0}])

		var s2_cold := 0.0
		var s2_ops: Array[float] = []
		var s2_frs: Array[float] = []
		var s2_peak_n := 0
		var s2_peak_c := 0
		for it in range(51):
			var t0 := Time.get_ticks_usec()
			var dlg := ClockGearsEventEditorDialog.new()
			root.add_child(dlg)
			dlg.edit_event(ev2)
			var t1 := Time.get_ticks_usec()
			var tf0 := Time.get_ticks_usec()
			await _step_process(1)
			var tf1 := Time.get_ticks_usec()
			s2_peak_n = maxi(s2_peak_n, int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)))
			s2_peak_c = maxi(s2_peak_c, _count_controls(dlg))
			dlg.hide()
			dlg.queue_free()
			await _step_process(1)
			var dt_op := float(t1 - t0) / 1000.0
			var dt_fr := float(tf1 - tf0) / 1000.0
			if it == 0:
				s2_cold = dt_op
			else:
				s2_ops.append(dt_op)
				s2_frs.append(dt_fr)
		ev2.queue_free()
		var a2 := await _drain_and_stats()
		_record("2_EventEditor_200cmds_50x", s2_cold, s2_ops, s2_frs, s2_peak_n, s2_peak_c, b2, a2)

	# ------------------------------------------------------------------
	# Escenario 3: Abrir cada editor del capítulo Menús (50x)
	# ------------------------------------------------------------------
	if start_from <= 3:
		var b3 := await _drain_and_stats()
		var db3 := ClockGearsDatabaseDialog.new()
		root.add_child(db3)
		db3.open_database()
		await _step_process(2)
		var menu_cats := ["main_menu", "window_styles", "game_menu", "credits", "hud", "ui_designer", "game_over"]
		var s3_cold := 0.0
		var s3_ops: Array[float] = []
		var s3_frs: Array[float] = []
		var s3_peak_n := 0
		var s3_peak_c := 0
		for it in range(51):
			var t0 := Time.get_ticks_usec()
			for mc in menu_cats:
				db3.select_category_by_key(mc)
			var t1 := Time.get_ticks_usec()
			var tf0 := Time.get_ticks_usec()
			await _step_process(1)
			var tf1 := Time.get_ticks_usec()
			s3_peak_n = maxi(s3_peak_n, int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)))
			s3_peak_c = maxi(s3_peak_c, _count_controls(db3))
			var dt_op := float(t1 - t0) / 1000.0
			var dt_fr := float(tf1 - tf0) / 1000.0
			if it == 0:
				s3_cold = dt_op
			else:
				s3_ops.append(dt_op)
				s3_frs.append(dt_fr)
		db3.hide()
		db3.queue_free()
		var a3 := await _drain_and_stats()
		_record("3_Menus_7editors_50x", s3_cold, s3_ops, s3_frs, s3_peak_n, s3_peak_c, b3, a3)

	# ------------------------------------------------------------------
	# Escenario 4: Abrir/cerrar Menú del juego e Inventario en juego (50x)
	# ------------------------------------------------------------------
	if start_from <= 4:
		var b4 := await _drain_and_stats()
		var gm := ClockGearsGameMenu.new()
		root.add_child(gm)
		await _step_process(2)
		var s4_cold := 0.0
		var s4_ops: Array[float] = []
		var s4_frs: Array[float] = []
		var s4_peak_n := 0
		var s4_peak_c := 0
		for it in range(51):
			var t0 := Time.get_ticks_usec()
			gm.open_menu()
			gm.open_subscreen("items")
			gm.close_menu()
			var t1 := Time.get_ticks_usec()
			var tf0 := Time.get_ticks_usec()
			await _step_process(1)
			var tf1 := Time.get_ticks_usec()
			s4_peak_n = maxi(s4_peak_n, int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)))
			s4_peak_c = maxi(s4_peak_c, _count_controls(gm))
			var dt_op := float(t1 - t0) / 1000.0
			var dt_fr := float(tf1 - tf0) / 1000.0
			if it == 0:
				s4_cold = dt_op
			else:
				s4_ops.append(dt_op)
				s4_frs.append(dt_fr)
		gm.queue_free()
		var a4 := await _drain_and_stats()
		_record("4_InGame_Menu_Inventory_50x", s4_cold, s4_ops, s4_frs, s4_peak_n, s4_peak_c, b4, a4)

	# ------------------------------------------------------------------
	# Escenario 5: Cambiar de mapa (50x)
	# ------------------------------------------------------------------
	var b5 := await _drain_and_stats()
	var map_paths := ["res://maps/map001.tscn", "res://maps/map002.tscn"]
	var cur_map: Node = null
	var s5_cold := 0.0
	var s5_ops: Array[float] = []
	var s5_frs: Array[float] = []
	var s5_peak_n := 0
	var s5_peak_c := 0
	for it in range(51):
		var mpath: String = map_paths[it % 2]
		var t0 := Time.get_ticks_usec()
		if cur_map != null and is_instance_valid(cur_map):
			cur_map.queue_free()
		var ps := ResourceLoader.load(mpath, "", ResourceLoader.CACHE_MODE_REUSE) as PackedScene
		if ps != null:
			cur_map = ps.instantiate()
			root.add_child(cur_map)
		var t1 := Time.get_ticks_usec()
		var tf0 := Time.get_ticks_usec()
		await _step_process(1)
		var tf1 := Time.get_ticks_usec()
		s5_peak_n = maxi(s5_peak_n, int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)))
		s5_peak_c = maxi(s5_peak_c, _count_controls(root))
		var dt_op := float(t1 - t0) / 1000.0
		var dt_fr := float(tf1 - tf0) / 1000.0
		if it == 0:
			s5_cold = dt_op
		else:
			s5_ops.append(dt_op)
			s5_frs.append(dt_fr)
	if cur_map != null and is_instance_valid(cur_map):
		cur_map.queue_free()
	var a5 := await _drain_and_stats()
	_record("5_Map_Change_50x", s5_cold, s5_ops, s5_frs, s5_peak_n, s5_peak_c, b5, a5)

	# ------------------------------------------------------------------
	# Escenario 6: Título a partida y de vuelta al título (50x)
	# ------------------------------------------------------------------
	var b6 := await _drain_and_stats()
	var s6_cold := 0.0
	var s6_ops: Array[float] = []
	var s6_frs: Array[float] = []
	var s6_peak_n := 0
	var s6_peak_c := 0
	for it in range(51):
		var t0 := Time.get_ticks_usec()
		var ts := ClockGearsTitleScreen.new()
		root.add_child(ts)
		await _step_process(1)
		ts.queue_free()
		var ps := ResourceLoader.load("res://maps/map001.tscn") as PackedScene
		var mp: Node = ps.instantiate() if ps != null else ClockGearsMap.new()
		root.add_child(mp)
		var pl := ClockGearsPlayer.new()
		mp.add_child(pl)
		var tf0 := Time.get_ticks_usec()
		await _step_process(1)
		var tf1 := Time.get_ticks_usec()
		s6_peak_n = maxi(s6_peak_n, int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)))
		s6_peak_c = maxi(s6_peak_c, _count_controls(root))
		mp.queue_free()
		var ts2 := ClockGearsTitleScreen.new()
		root.add_child(ts2)
		await _step_process(1)
		ts2.queue_free()
		var t1 := Time.get_ticks_usec()
		var dt_op := float(t1 - t0) / 1000.0
		var dt_fr := float(tf1 - tf0) / 1000.0
		if it == 0:
			s6_cold = dt_op
		else:
			s6_ops.append(dt_op)
			s6_frs.append(dt_fr)
	var a6 := await _drain_and_stats()
	_record("6_Title_To_Game_To_Title_50x", s6_cold, s6_ops, s6_frs, s6_peak_n, s6_peak_c, b6, a6)

	# ------------------------------------------------------------------
	# Escenario 7: Combate ARPG con 20 enemigos persiguiendo (60 frames x warm)
	# ------------------------------------------------------------------
	var b7 := await _drain_and_stats()
	var m7 := ClockGearsMap.new()
	m7.set_map_size(Vector2i(24, 24))
	root.add_child(m7)
	m7.ensure_default_layers()
	var p7 := ClockGearsPlayer.new()
	p7.position = Vector2(384, 384)
	m7.add_child(p7)
	for ei in range(20):
		var en := ClockGearsEvent.new()
		en.set_event_id(ei + 1)
		en.set_event_name("Slime_%02d" % (ei + 1))
		en.set_behavior_type(3) # Enemy ARPG
		en.position = Vector2(128 + (ei % 5) * 96, 128 + int(ei / 5) * 96)
		en.apply_behavior_record_config({
			"id": ei + 1,
			"name": "Slime_%02d" % (ei + 1),
			"hp": 100,
			"atk": 10,
			"ai_type": "persigue_y_ataca",
			"sight_range": 20,
			"speed": 90.0,
		}, 90.0)
		m7.add_child(en)
	await _step_physics(2)
	var s7_cold := 0.0
	var s7_ops: Array[float] = []
	var s7_frs: Array[float] = []
	var s7_peak_n := int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT))
	var s7_peak_c := _count_controls(root)
	for it in range(61):
		var tf0 := Time.get_ticks_usec()
		await _step_physics(1)
		var tf1 := Time.get_ticks_usec()
		var dt_fr := float(tf1 - tf0) / 1000.0
		if it == 0:
			s7_cold = dt_fr
		else:
			s7_ops.append(dt_fr)
			s7_frs.append(dt_fr)
	m7.queue_free()
	var a7 := await _drain_and_stats()
	_record("7_ARPG_20Enemies_Chase_60f", s7_cold, s7_ops, s7_frs, s7_peak_n, s7_peak_c, b7, a7)

	# ------------------------------------------------------------------
	# Escenario 8: Mover elementos en el Editor de Diseño de Interfaz (50x)
	# ------------------------------------------------------------------
	var b8 := await _drain_and_stats()
	var db8 := ClockGearsDatabaseDialog.new()
	root.add_child(db8)
	db8.open_database()
	db8.select_category_by_key("ui_designer")
	await _step_process(2)
	var uie := db8.get_ui_designer_editor()
	uie.select_screen("hud")
	var el_id := uie.add_element_at("text", Vector2i(100, 100))
	uie.select_element(el_id)
	await _step_process(1)
	var uih: Dictionary = uie.get_ui_handles()
	var ox_spin: SpinBox = uih.get("prop_ox_spin", null)
	var s8_cold := 0.0
	var s8_ops: Array[float] = []
	var s8_frs: Array[float] = []
	var s8_peak_n := 0
	var s8_peak_c := 0
	for it in range(51):
		var t0 := Time.get_ticks_usec()
		if ox_spin != null:
			ox_spin.set_value(100 + (it % 40) * 5)
		var t1 := Time.get_ticks_usec()
		var tf0 := Time.get_ticks_usec()
		await _step_process(1)
		var tf1 := Time.get_ticks_usec()
		s8_peak_n = maxi(s8_peak_n, int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)))
		s8_peak_c = maxi(s8_peak_c, _count_controls(uie))
		var dt_op := float(t1 - t0) / 1000.0
		var dt_fr := float(tf1 - tf0) / 1000.0
		if it == 0:
			s8_cold = dt_op
		else:
			s8_ops.append(dt_op)
			s8_frs.append(dt_fr)
	db8.hide()
	db8.queue_free()
	if dm != null:
		dm.load_all("res://clockgears_data")
	var a8 := await _drain_and_stats()
	_record("8_UIDesigner_MoveElement_50x", s8_cold, s8_ops, s8_frs, s8_peak_n, s8_peak_c, b8, a8)

	print("=== FIN BENCHMARK PUNTO 7 ===")
	quit(0)
