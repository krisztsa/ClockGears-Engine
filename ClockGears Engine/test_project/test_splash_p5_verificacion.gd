extends SceneTree

var pass_count := 0
var fail_count := 0
var start_ms := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] ", msg)
	else:
		fail_count += 1
		print("  [FAIL] ", msg)

func _initialize() -> void:
	start_ms = Time.get_ticks_msec()
	var watchdog := create_timer(58.0)
	watchdog.timeout.connect(func():
		fail_count += 1
		print("  [FAIL] Timeout interno 58s alcanzado")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 5: VERIFICACIÓN COMPLETA DE LA PANTALLA DE CARGA ===")

	# 1. Simular 600 fotogramas con semilla fija (12345):
	#    - partículas activas <= 120
	#    - motas de ambiente <= 20
	#    - el pool no crece nunca
	#    - el progreso de 0 a 100 % termina en "Listo" con ráfaga final
	var sim600: Dictionary = ClockGearsTheme.simulate_boot_splash_particles(600, 12345, 2.0, true, Vector2i(1920, 1080))
	check(int(sim600.get("frames_simulated", 0)) == 600, "Simulados 600 fotogramas con semilla fija 12345")
	check(int(sim600.get("max_observed_total", 999)) <= 120 and int(sim600.get("max_observed_total", 0)) > 30,
		"Partículas activas en 600 fotogramas nunca superan 120 (máx=%d)" % int(sim600.get("max_observed_total", 0)))
	check(int(sim600.get("max_observed_ambient", 99)) == 20,
		"Motas doradas de ambiente <= 20 (máx=%d)" % int(sim600.get("max_observed_ambient", 0)))
	check(int(sim600.get("pool_capacity", 0)) == 120 and bool(sim600.get("pool_never_grew", false)),
		"Pool fijo de 120 partículas reutilizables que no crece ni reserva memoria por fotograma")
	check(bool(sim600.get("burst_done", false)) and int(sim600.get("sparks_at_burst", 0)) >= 15,
		"El progreso de 0 a 100 %% emite ráfaga final al llegar a 'Listo'")

	# 2. Presupuesto de 16,7 ms y opción "Efectos de brillo" en Preferencias > Apariencia
	var sim_overload: Dictionary = ClockGearsTheme.simulate_boot_splash_particles(60, 12345, 21.0, true, Vector2i(1920, 1080))
	check(int(sim_overload.get("min_observed_cap", 120)) < 120,
		"Si el fotograma supera 16,7 ms se reduce automáticamente el máximo de partículas activas (cap=%d)" % int(sim_overload.get("min_observed_cap", 120)))

	var sim_noglow: Dictionary = ClockGearsTheme.simulate_boot_splash_particles(120, 12345, 2.0, false, Vector2i(1920, 1080))
	check(int(sim_noglow.get("max_observed_total", 99)) == 0,
		"Con 'Efectos de brillo' desactivado no se dibujan partículas ni brillo animado")

	# 3. Generar y verificar capturas reales en 1366x768, 1920x1080, 2560x1440 y 16:10 (1920x1200)
	var out_dir := ProjectSettings.globalize_path("res://../../ICONO/capturas_pantalla_carga").simplify_path()
	DirAccess.make_dir_recursive_absolute(out_dir)

	var resolutions := [
		{"sz": Vector2i(1366, 768), "file": "splash_1366x768.png", "label": "1366x768 (16:9)"},
		{"sz": Vector2i(1920, 1080), "file": "splash_1920x1080.png", "label": "1920x1080 (16:9 base)"},
		{"sz": Vector2i(2560, 1440), "file": "splash_2560x1440.png", "label": "2560x1440 (16:9 QHD)"},
		{"sz": Vector2i(1920, 1200), "file": "splash_1920x1200_16_10.png", "label": "1920x1200 (16:10)"},
	]

	for entry in resolutions:
		var sz: Vector2i = entry["sz"]
		var png_path := out_dir.path_join(String(entry["file"]))
		var lbl := String(entry["label"])

		# Sembrar y avanzar partículas para que la captura muestre motas de ambiente y chispas de la cabeza de avance
		ClockGearsTheme.simulate_boot_splash_particles(45, 12345, 2.0, true, sz)
		var m: Dictionary = ClockGearsTheme.inspect_boot_splash(5, 7, "Cargando sistemas y escena principal...", png_path, false, sz, 0.5)

		check(FileAccess.file_exists(png_path), "%s: captura PNG guardada en %s" % [lbl, png_path])
		check(int(m.get("width", 0)) == sz.x and int(m.get("height", 0)) == sz.y,
			"%s: cubre toda la pantalla (%dx%d)" % [lbl, sz.x, sz.y])
		check(is_equal_approx(float(m.get("corner_outer_alpha", 0.0)), 1.0),
			"%s: sin bordes grises ni esquinas recortadas (alpha=1.0)" % lbl)

		var icon_mult := int(m.get("icon_scale_multiple", 0))
		var icon_rect: Rect2i = m.get("icon_rect", Rect2i())
		check(icon_mult >= 2 and icon_rect.size.x >= 128 and icon_rect.size.x <= 256 and (icon_rect.size.x % 64) == 0,
			"%s: icono nítido en múltiplo entero (%d px, %dx)" % [lbl, icon_rect.size.x, icon_mult])

		var frame_rect: Rect2i = m.get("inner_frame_rect", Rect2i())
		var badge_rect: Rect2i = m.get("badge_rect", Rect2i())
		var title_rect: Rect2i = m.get("title_rect", Rect2i())
		var sub_rect: Rect2i = m.get("subtitle_rect", Rect2i())
		var cred_rect: Rect2i = m.get("credits_rect", Rect2i())
		var bar_rect: Rect2i = m.get("progress_bar_rect", Rect2i())
		var status_rect: Rect2i = m.get("status_rect", Rect2i())
		var brand_rect: Rect2i = m.get("right_brand_rect", Rect2i())

		var all_inside := frame_rect.encloses(badge_rect) and frame_rect.encloses(title_rect) and frame_rect.encloses(sub_rect) and frame_rect.encloses(cred_rect) and frame_rect.encloses(bar_rect)
		var no_overlap := (badge_rect.end.y < icon_rect.position.y) and (icon_rect.end.y < title_rect.position.y) and (title_rect.end.y < sub_rect.position.y) and (sub_rect.end.y < cred_rect.position.y) and (cred_rect.end.y < status_rect.position.y) and (status_rect.end.x < brand_rect.position.x)
		check(all_inside and no_overlap, "%s: textos sin cortar ni solapar y composición centrada dentro del marco cobrizo" % lbl)

		var luma_l := float(m.get("fill_left_luma", 0.0))
		var luma_r := float(m.get("fill_right_luma", 0.0))
		var halo_d := float(m.get("outer_halo_delta", 0.0))
		var flare_d := float(m.get("head_flare_delta", 0.0))
		var act_p := int(m.get("active_particle_count", 0))
		check(luma_r > luma_l + 0.04 and halo_d > 0.02 and flare_d > 0.04 and act_p > 20 and act_p <= 120,
			"%s: barra con brillo, halo exterior, destello aditivo y partículas pixel-art (%d activas)" % [lbl, act_p])

	# Captura adicional al 100 % ("Listo") con destello completo y ráfaga de partículas
	var png_ready := out_dir.path_join("splash_1920x1080_listo.png")
	ClockGearsTheme.simulate_boot_splash_particles(120, 12345, 2.0, true, Vector2i(1920, 1080))
	var m_ready: Dictionary = ClockGearsTheme.inspect_boot_splash(7, 7, "Listo", png_ready, false, Vector2i(1920, 1080), 0.5)
	check(FileAccess.file_exists(png_ready) and String(m_ready.get("status_text", "")) == "Listo",
		"1920x1080 ('Listo' 100%%): captura guardada con destello completo y texto 'Listo'")

	# 4. Verificación en vivo del arranque 0 -> 100 % -> "Listo" -> cierre al dibujar el primer fotograma
	var exe_path := OS.get_executable_path()
	var proj_path := ProjectSettings.globalize_path("res://").simplify_path()
	var tmp_dir := ProjectSettings.globalize_path("res://_tmp_p5_verif").simplify_path()
	DirAccess.make_dir_recursive_absolute(tmp_dir)
	var report_path := tmp_dir.path_join("report_p5.cfg")
	if FileAccess.file_exists(report_path):
		DirAccess.remove_absolute(report_path)

	OS.set_environment("CG_BOOT_TEST_REPORT", report_path)
	var pid := OS.create_process(exe_path, PackedStringArray(["-e", "--path", proj_path]))
	var steps := 0
	while steps < 600 and OS.is_process_running(pid) and (Time.get_ticks_msec() - start_ms) < 35000:
		OS.delay_msec(40)
		await process_frame
		steps += 1
	if OS.is_process_running(pid):
		OS.kill(pid)
	OS.set_environment("CG_BOOT_TEST_REPORT", "")

	var cf := ConfigFile.new()
	check(cf.load(report_path) == OK, "Ejecución real 0 -> 100%% completada y reporte leído")
	if cf.load(report_path) == OK:
		check(bool(cf.get_value("boot_test", "ready_logged", false)) and (not bool(cf.get_value("boot_test", "splash_window_open", true))) and String(cf.get_value("boot_test", "closed_reason", "")) == "first_frame_drawn",
			"El progreso de 0 a 100 %% termina en 'Listo' y cierra la pantalla de carga con desvanecimiento")

	if FileAccess.file_exists(report_path):
		DirAccess.remove_absolute(report_path)
	DirAccess.remove_absolute(tmp_dir)

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
