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
	print("=== TEST PUNTO 4: FLUIDEZ DURANTE LA CARGA Y TIEMPO DE ARRANQUE (< +5 %) ===")
	var exe_path := OS.get_executable_path()
	var proj_path := ProjectSettings.globalize_path("res://").simplify_path()
	var tmp_dir := ProjectSettings.globalize_path("res://_tmp_p4_fluidez").simplify_path()
	DirAccess.make_dir_recursive_absolute(tmp_dir)
	var report_path := tmp_dir.path_join("report_p4.cfg")
	if FileAccess.file_exists(report_path):
		DirAccess.remove_absolute(report_path)

	OS.set_environment("CG_BOOT_TEST_REPORT", report_path)
	var pid := OS.create_process(exe_path, PackedStringArray(["-e", "--path", proj_path]))
	check(pid > 0, "Proceso del editor lanzado con pantalla de carga a pantalla completa (pid=%d)" % pid)

	var frames_waited := 0
	while frames_waited < 600 and OS.is_process_running(pid) and (Time.get_ticks_msec() - start_ms) < 28000:
		OS.delay_msec(40)
		await process_frame
		frames_waited += 1

	if OS.is_process_running(pid):
		OS.kill(pid)
		check(false, "El editor terminó dentro del límite de 600 fotogramas")
	else:
		check(true, "El editor abrió, dibujó y cerró limpiamente en %d pasos" % frames_waited)

	OS.set_environment("CG_BOOT_TEST_REPORT", "")

	check(FileAccess.file_exists(report_path), "Reporte de arranque generado")
	if FileAccess.file_exists(report_path):
		var cf := ConfigFile.new()
		if cf.load(report_path) == OK:
			var anim_frames := int(cf.get_value("boot_test", "anim_frames_drawn", 0))
			var elapsed_ms := int(cf.get_value("boot_test", "first_frame_elapsed_ms", 99999))
			var trace_path := String(cf.get_value("boot_test", "trace_log_path", ""))
			if not trace_path.is_empty() and FileAccess.file_exists(trace_path):
				print("  [INFO] boot_splash_trace.log:\n" + FileAccess.get_file_as_string(trace_path).strip_edges())
			var splash_open := bool(cf.get_value("boot_test", "splash_window_open", true))
			var closed_reason := String(cf.get_value("boot_test", "closed_reason", ""))
			var is_borderless := bool(cf.get_value("boot_test", "is_borderless", false))
			var has_titlebar := bool(cf.get_value("boot_test", "has_titlebar", true))
			var has_taskbar := bool(cf.get_value("boot_test", "has_taskbar_button", true))

			# Baseline medido antes del cambio: 5182 ms (límite +5 % = 5441 ms)
			var baseline_ms := 5182
			var max_allowed_ms := int(round(baseline_ms * 1.05))
			print("  [INFO] Fotogramas animados en segundo plano durante la carga: %d" % anim_frames)
			print("  [INFO] Tiempo de arranque (antes=%d ms, después=%d ms, límite +5%%=%d ms)" % [baseline_ms, elapsed_ms, max_allowed_ms])

			check(anim_frames >= 15, "La animación NO se congela durante etapas pesadas (%d fotogramas animados en hilo propio)" % anim_frames)
			check(elapsed_ms <= max_allowed_ms, "Tiempo de arranque no aumenta más de un 5%% (antes=%d ms, después=%d ms <= %d ms)" % [baseline_ms, elapsed_ms, max_allowed_ms])
			check(is_borderless and (not has_titlebar) and (not has_taskbar), "Ventana de carga a pantalla completa sin bordes, sin barra de título y sin botón en barra de tareas")
			check((not splash_open) and closed_reason == "first_frame_drawn", "Pantalla de carga llega a 'Listo' y se cierra al dibujar el primer fotograma")

		DirAccess.remove_absolute(report_path)
	DirAccess.remove_absolute(tmp_dir)

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
