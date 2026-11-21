extends SceneTree

var pass_count: int = 0
var fail_count: int = 0
var start_ms: int = 0

func _check(cond: bool, label: String, detail: String = "") -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + label)
	else:
		fail_count += 1
		print("[FAIL] " + label + ("" if detail.is_empty() else " -> " + detail))

func _find_button_by_text(node: Node, target_text: String) -> Button:
	if node is Button and (node as Button).text == target_text:
		return node as Button
	for c in node.get_children():
		var found := _find_button_by_text(c, target_text)
		if found != null:
			return found
	return null

func _initialize() -> void:
	start_ms = Time.get_ticks_msec()
	var watchdog := create_timer(58.0)
	watchdog.timeout.connect(func():
		fail_count += 1
		print("[FAIL] Timeout interno 58s alcanzado")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
	)
	call_deferred("_run")

func _verify_boot_report(report_path: String, via_label: String) -> void:
	var exists := FileAccess.file_exists(report_path)
	_check(exists, via_label + ": reporte de arranque generado", report_path)
	if not exists:
		return

	var cf := ConfigFile.new()
	var err := cf.load(report_path)
	_check(err == OK, via_label + ": reporte de arranque legible")
	if err != OK:
		return

	var splash_open := bool(cf.get_value("boot_test", "splash_window_open", true))
	var was_opened := bool(cf.get_value("boot_test", "splash_was_opened", false))
	var is_indep := bool(cf.get_value("boot_test", "is_independent_window", false))
	var has_parent := bool(cf.get_value("boot_test", "has_parent", true))
	var is_borderless := bool(cf.get_value("boot_test", "is_borderless", false))
	var has_titlebar := bool(cf.get_value("boot_test", "has_titlebar", true))
	var has_taskbar := bool(cf.get_value("boot_test", "has_taskbar_button", true))
	var ready_logged := bool(cf.get_value("boot_test", "ready_logged", false))
	var first_frame_logged := bool(cf.get_value("boot_test", "first_frame_logged", false))
	var close_fade_logged := bool(cf.get_value("boot_test", "close_fade_logged", false))
	var closed_reason := String(cf.get_value("boot_test", "closed_reason", ""))
	var badge_unclipped := bool(cf.get_value("boot_test", "version_badge_unclipped", false))
	var trace_log_path := String(cf.get_value("boot_test", "trace_log_path", ""))
	var non_black := int(cf.get_value("boot_test", "non_black_pixels", 0))
	var total_sampled := int(cf.get_value("boot_test", "total_sampled", 1))
	var luma_buckets := int(cf.get_value("boot_test", "distinct_luma_buckets", 0))
	var frames_drawn := int(cf.get_value("boot_test", "frames_drawn", 0))

	_check(was_opened and is_indep and (not has_parent) and is_borderless and (not has_titlebar) and (not has_taskbar),
		via_label + ": ventana propia sin bordes, sin barra de título, sin padre y sin botón en barra de tareas",
		"indep=%s parent=%s borderless=%s titlebar=%s taskbar=%s" % [str(is_indep), str(has_parent), str(is_borderless), str(has_titlebar), str(has_taskbar)])

	_check(badge_unclipped, via_label + ": etiqueta 'v0.0.4 ALPHA' completa sin recortarse por arriba")
	_check(ready_logged and first_frame_logged and close_fade_logged,
		via_label + ": eventos 'Listo', primer fotograma y desvanecimiento registrados")
	_check((not splash_open) and closed_reason == "first_frame_drawn",
		via_label + ": pantalla de carga se cerró al dibujar el primer fotograma",
		"open=%s reason=%s" % [str(splash_open), closed_reason])
	_check(frames_drawn >= 1 and non_black > (total_sampled / 2) and luma_buckets >= 2,
		via_label + ": ventana del editor dibuja con normalidad y NO es toda negra (%d/%d píxeles activos, %d niveles)" % [non_black, total_sampled, luma_buckets])

	if not trace_log_path.is_empty() and FileAccess.file_exists(trace_log_path):
		var log_txt := FileAccess.get_file_as_string(trace_log_path)
		var lines := log_txt.strip_edges().split("\n", false)
		var has_all_events := log_txt.contains("event=created") and log_txt.contains("event=ready") and log_txt.contains("event=first_frame_drawn") and log_txt.contains("event=close_fade")
		_check(has_all_events and lines.size() <= 12,
			via_label + ": log de una línea por evento (nunca por fotograma, %d líneas)" % lines.size())
	else:
		_check(false, via_label + ": archivo boot_splash_trace.log encontrado", trace_log_path)

func _run_pm_open_helper(proj_path: String, tmp_projects_cfg: String) -> void:
	var pcfg := ConfigFile.new()
	pcfg.set_value(proj_path, "favorite", true)
	pcfg.save(tmp_projects_cfg)

	var pm: Control = ClassDB.instantiate("ProjectManager")
	root.add_child(pm)
	for i in range(4):
		await process_frame

	pm.call("reload_projects_from_config_for_test", tmp_projects_cfg)
	for i in range(4):
		await process_frame

	var plist: Object = pm.call("get_project_list")
	plist.call("select_project", 0, true)
	for i in range(2):
		await process_frame

	var edit_btn := _find_button_by_text(pm, "Editar Proyecto")
	if edit_btn != null and not edit_btn.disabled:
		# Pressing "Editar Proyecto" runs _open_selected_projects() which calls OS.create_instance(--path <proj> --editor) and get_tree().quit()
		edit_btn.emit_signal("pressed")
	else:
		quit(1)

func _run() -> void:
	var user_args := OS.get_cmdline_user_args()
	var exe_path := OS.get_executable_path()
	var proj_path := ProjectSettings.globalize_path("res://").simplify_path()
	var tmp_dir := ProjectSettings.globalize_path("res://_tmp_boot_test").simplify_path()
	DirAccess.make_dir_recursive_absolute(tmp_dir)
	var tmp_projects_cfg := tmp_dir.path_join("pm_projects.cfg")

	if user_args.has("--trigger-pm-open"):
		await _run_pm_open_helper(proj_path, tmp_projects_cfg)
		return

	var report_via1 := tmp_dir.path_join("report_via1_cli.cfg")
	var report_via2 := tmp_dir.path_join("report_via2_pm.cfg")
	if FileAccess.file_exists(report_via1):
		DirAccess.remove_absolute(report_via1)
	if FileAccess.file_exists(report_via2):
		DirAccess.remove_absolute(report_via2)

	# =========================================================================
	# VÍA 1: Lanzar el editor directo por línea de comandos (-e --path)
	# =========================================================================
	OS.set_environment("CG_BOOT_TEST_REPORT", report_via1)
	var pid1 := OS.create_process(exe_path, PackedStringArray(["-e", "--path", proj_path]))
	_check(pid1 > 0, "Vía 1 (-e --path): proceso del editor lanzado (pid=%d)" % pid1)

	var frames_waited_1 := 0
	while frames_waited_1 < 600 and OS.is_process_running(pid1) and (Time.get_ticks_msec() - start_ms) < 26000:
		OS.delay_msec(40)
		await process_frame
		frames_waited_1 += 1

	if OS.is_process_running(pid1):
		OS.kill(pid1)
		_check(false, "Vía 1 (-e --path): proceso terminó dentro de 600 fotogramas")
	else:
		_check(true, "Vía 1 (-e --path): proceso terminó limpiamente con quit(0) en %d pasos" % frames_waited_1)

	_verify_boot_report(report_via1, "Vía 1 (-e --path)")

	# =========================================================================
	# VÍA 2: Abrir el proyecto desde el Administrador de proyectos
	# =========================================================================
	OS.set_environment("CG_BOOT_TEST_REPORT", report_via2)
	var pid_pm := OS.create_process(exe_path, PackedStringArray(["--path", proj_path, "-s", "test_arreglo_splash_editor.gd", "--", "--trigger-pm-open"]))
	_check(pid_pm > 0, "Vía 2 (Administrador): Administrador lanzado y 'Editar Proyecto' pulsado (pid=%d)" % pid_pm)

	var frames_waited_2 := 0
	while frames_waited_2 < 600 and (not FileAccess.file_exists(report_via2)) and (Time.get_ticks_msec() - start_ms) < 54000:
		OS.delay_msec(45)
		await process_frame
		frames_waited_2 += 1

	OS.delay_msec(250)
	_check(FileAccess.file_exists(report_via2), "Vía 2 (Administrador): el editor abierto desde el Administrador terminó dentro de 600 fotogramas (%d pasos)" % frames_waited_2)
	_verify_boot_report(report_via2, "Vía 2 (Administrador)")

	OS.set_environment("CG_BOOT_TEST_REPORT", "")

	# Limpiar temporales de prueba
	if FileAccess.file_exists(report_via1):
		DirAccess.remove_absolute(report_via1)
	if FileAccess.file_exists(report_via2):
		DirAccess.remove_absolute(report_via2)
	if FileAccess.file_exists(tmp_projects_cfg):
		DirAccess.remove_absolute(tmp_projects_cfg)
	DirAccess.remove_absolute(tmp_dir)

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
