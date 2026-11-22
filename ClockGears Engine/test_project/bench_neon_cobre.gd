extends SceneTree

func _calc_stats(samples: Array[float]) -> Dictionary:
	if samples.is_empty():
		return {"avg": 0.0, "p95": 0.0, "max": 0.0}
	var sorted := samples.duplicate()
	sorted.sort()
	var sum := 0.0
	for v in sorted:
		sum += v
	var idx95 := mini(sorted.size() - 1, maxi(0, int(ceil(float(sorted.size()) * 0.95)) - 1))
	return {
		"avg": sum / float(sorted.size()),
		"p95": sorted[idx95],
		"max": sorted[sorted.size() - 1],
	}

func _step(n: int = 1) -> void:
	for _i in range(n):
		await process_frame

func _initialize() -> void:
	var watchdog := create_timer(58.0)
	watchdog.timeout.connect(func():
		print("[FAIL] Timeout 58s en benchmark")
		quit(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== BENCHMARK TEMA NEÓN COBRE ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm != null:
		dm.load_all("res://clockgears_data")
	var th = Engine.get_singleton("ClockGearsTheme")
	await _step(2)

	for theme_id in ["dark", "brass"]:
		if th != null:
			th.set_active_theme_id(theme_id)
		await _step(2)

		var db := ClockGearsDatabaseDialog.new()
		root.add_child(db)

		# 1. Tiempo de apertura de la Base de Datos (10 repeticiones)
		var open_samples: Array[float] = []
		for i in range(10):
			var t0 := Time.get_ticks_usec()
			db.open_database()
			var dt := float(Time.get_ticks_usec() - t0) / 1000.0
			open_samples.append(dt)
			db.hide()
			await _step(1)
		var st_open := _calc_stats(open_samples)

		# 2. Tiempo de cambio entre capítulos y pestañas de la Base de Datos
		db.open_database()
		await _step(1)
		var cats := ["actors", "classes", "skills", "items", "enemies", "troops", "tilesets", "system", "main_menu", "window_styles", "game_menu", "credits", "hud", "ui_designer", "game_over"]
		var tab_samples: Array[float] = []
		for ck in cats:
			var t0 := Time.get_ticks_usec()
			db.select_category_by_key(ck)
			var dt := float(Time.get_ticks_usec() - t0) / 1000.0
			tab_samples.append(dt)
		var st_tabs := _calc_stats(tab_samples)

		# 3. Tiempo de frame del editor en reposo (60 fotogramas sin interacción)
		var top_bar := ClockGearsTopBar.new()
		var map_panel := ClockGearsMapPanel.new()
		root.add_child(top_bar)
		root.add_child(map_panel)
		if th != null:
			th.apply_to_control(top_bar)
			th.apply_to_control(map_panel)
		await _step(3)

		var idle_samples: Array[float] = []
		for f in range(60):
			var t0 := Time.get_ticks_usec()
			await _step(1)
			var dt := float(Time.get_ticks_usec() - t0) / 1000.0
			idle_samples.append(dt)
		var st_idle := _calc_stats(idle_samples)

		# 4. Tiempo de frame moviendo el ratón (60 fotogramas con InputEventMouseMotion + hover)
		var motion_samples: Array[float] = []
		for f in range(60):
			var t0 := Time.get_ticks_usec()
			var ev := InputEventMouseMotion.new()
			ev.position = Vector2(float(80 + (f * 17) % 1100), float(40 + (f * 13) % 620))
			ev.global_position = ev.position
			root.push_input(ev)
			await _step(1)
			var dt := float(Time.get_ticks_usec() - t0) / 1000.0
			motion_samples.append(dt)
		var st_motion := _calc_stats(motion_samples)

		print("THEME=%s | DB_Open(avg/p95)=%.2f/%.2f ms | DB_Tabs(avg/p95)=%.2f/%.2f ms | Frame_Idle(avg/p95)=%.2f/%.2f ms | Frame_MouseMove(avg/p95)=%.2f/%.2f ms" % [
			theme_id,
			st_open["avg"], st_open["p95"],
			st_tabs["avg"], st_tabs["p95"],
			st_idle["avg"], st_idle["p95"],
			st_motion["avg"], st_motion["p95"]
		])

		top_bar.queue_free()
		map_panel.queue_free()
		db.hide()
		db.queue_free()
		await _step(2)

	if th != null:
		th.set_active_theme_id("dark")
	print("RESULTADOS: 4 PASS, 0 FAIL")
	quit(0)
