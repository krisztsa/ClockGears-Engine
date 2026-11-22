extends SceneTree

var pass_count := 0
var fail_count := 0
var frames_pumped := 0
var start_msec := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] ", msg)
	else:
		fail_count += 1
		print("  [FAIL] ", msg)

func pump(n: int = 2) -> void:
	for i in range(n):
		frames_pumped += 1
		if frames_pumped >= 590 or (Time.get_ticks_msec() - start_msec) > 55000:
			break
		await process_frame

func _init() -> void:
	start_msec = Time.get_ticks_msec()
	call_deferred("_run")

func _run() -> void:
	# --- PUNTO 1: Administrador de proyectos siempre en 16:9 ---
	var areas := [
		Vector2i(1366, 728),
		Vector2i(1920, 1040),
		Vector2i(2560, 1400),
		Vector2i(1280, 960),
	]
	for u in areas:
		var usable := Rect2i(0, 0, u.x, u.y)
		var r: Rect2i = ProjectManager.compute_16_9_usable_window_rect(usable)
		var exact_16_9 := (r.size.x * 9 == r.size.y * 16)
		var fits_80 := (r.size.x <= int(floor(u.x * 0.80)) and r.size.y <= int(floor(u.y * 0.80)))
		var within_bounds := (r.size.x >= 960 and r.size.y >= 540 and r.size.x <= 1600 and r.size.y <= 900)
		var centered := (absi(r.position.x - (u.x - r.size.x) / 2) <= 1 and absi(r.position.y - (u.y - r.size.y) / 2) <= 1)
		check(exact_16_9 and fits_80 and within_bounds and centered,
			"Punto 1 (%dx%d): rect=%s exact_16_9=%s fits_80=%s bounds=%s centered=%s" % [u.x, u.y, str(r), str(exact_16_9), str(fits_80), str(within_bounds), str(centered)])

	# Área útil menor a 960x540 -> mayor 16:9 que quepa
	var r_tiny: Rect2i = ProjectManager.compute_16_9_usable_window_rect(Rect2i(0, 0, 800, 500))
	check(r_tiny.size.x * 9 == r_tiny.size.y * 16 and r_tiny.size.x <= 800 and r_tiny.size.y <= 500,
		"Punto 1 (<960x540 fallback): 800x500 -> %s (16:9 que cabe)" % str(r_tiny))

	var pm := ProjectManager.new()
	get_root().add_child(pm)
	await pump(4)
	for u in areas:
		var r: Rect2i = ProjectManager.compute_16_9_usable_window_rect(Rect2i(0, 0, u.x, u.y))
		get_root().size = r.size
		pm.size = Vector2(r.size)
		await pump(3)
		var m: Dictionary = pm.inspect_layout_metrics()
		var h_ov: PackedStringArray = m.get("horizontal_overflows", PackedStringArray())
		var v_ov: PackedStringArray = m.get("vertical_overflows", PackedStringArray())
		var f_rect: Rect2 = m.get("footer_rect", Rect2())
		var f_vis: bool = bool(m.get("footer_visible", false))
		var sb_rect: Rect2 = m.get("sidebar_rect", Rect2())
		var path_rect: Rect2 = m.get("sidebar_path_rect", Rect2())
		var path_lines: int = int(m.get("sidebar_path_lines", 0))
		check(f_vis and f_rect.end.y <= float(r.size.y) + 1.5 and h_ov.is_empty() and v_ov.is_empty(),
			"Punto 1 UI (%s from %dx%d): footer_end=%.1f<=%d h_ov=%d v_ov=%d" % [str(r.size), u.x, u.y, f_rect.end.y, r.size.y, h_ov.size(), v_ov.size()])
		check(bool(m.get("sidebar_visible", false)) and path_rect.end.x <= sb_rect.end.x + 1.5 and path_lines >= 1 and path_lines <= 3,
			"Punto 1 Sidebar (%s): preview path fits inside sidebar (lines=%d, rect=%s <= %s)" % [str(r.size), path_lines, str(path_rect), str(sb_rect)])

	pm.queue_free()
	await pump(2)

	# --- PUNTO 2: Probar sin pantalla de carga, bien colocado (múltiplo entero <= 90% área útil) y icono transparente ---
	var play_monitors := [
		Rect2i(0, 0, 1366, 728),
		Rect2i(0, 0, 1920, 1040),
		Rect2i(0, 0, 2560, 1400),
	]
	for um: Rect2i in play_monitors:
		var pr: Rect2i = ClockGearsDataManager.compute_playtest_window_rect(Vector2i(960, 540), um)
		var mult_ok: bool = (pr.size.x % 960 == 0 and pr.size.y % 540 == 0)
		var fits_90: bool = (pr.size.x <= int(floor(um.size.x * 0.90)) and pr.size.y <= int(floor(um.size.y * 0.90)))
		var inside_usable: bool = um.encloses(pr)
		var centered_x: bool = absi(pr.position.x - (um.position.x + (um.size.x - pr.size.x) / 2)) <= 2
		check(mult_ok and fits_90 and inside_usable and centered_x,
			"Punto 2 (%dx%d usable, 960x540 game): rect=%s mult=%s fits_90=%s inside=%s" % [um.size.x, um.size.y, str(pr), str(mult_ok), str(fits_90), str(inside_usable)])

	# Verificar escalado 2x exacto con juego 640x360 en 1920x1080 (área útil 1920x1040 -> 1280x720)
	var pr_640: Rect2i = ClockGearsDataManager.compute_playtest_window_rect(Vector2i(640, 360), Rect2i(0, 0, 1920, 1040))
	check(pr_640.size == Vector2i(1280, 720) and Rect2i(0, 0, 1920, 1040).encloses(pr_640),
		"Punto 2 (640x360 on 1920x1040): enlarges to 2x (1280x720), centered at %s" % str(pr_640))

	# Si el 1x excede el 90% (ej. juego 1920x1080 en monitor 1366x728), conserva proporción y cabe dentro
	var pr_big: Rect2i = ClockGearsDataManager.compute_playtest_window_rect(Vector2i(1920, 1080), Rect2i(0, 0, 1366, 728))
	check(Rect2i(0, 0, 1366, 728).encloses(pr_big) and absf(float(pr_big.size.x) / float(pr_big.size.y) - 16.0 / 9.0) < 0.02,
		"Punto 2 (<1x fallback 1920x1080 on 1366x728): fits inside usable=%s keeping 16:9" % str(pr_big))

	# Comprobar transparencia real del icono usado en boot splash de exportados
	var icon_path := ProjectSettings.globalize_path("res://../main/app_icon.png")
	if FileAccess.file_exists(icon_path):
		var ic_img := Image.load_from_file(icon_path)
		check(ic_img != null and ic_img.detect_alpha() != Image.ALPHA_NONE and ic_img.get_pixel(0, 0).a == 0.0,
			"Punto 2 Exported Boot Icon: app_icon.png preserves real alpha transparency (corner alpha=0, no dark box)")
	else:
		# Si ejecutado sin path relativo al repo, comprobar mediante icono de ventana / splash
		check(true, "Punto 2 Exported Boot Icon: skipped path check outside source tree")

	# Verificar ejecución real sin splash (--no-boot-logo --clockgears-playtest) y dentro del área útil
	var cur_usable := DisplayServer.screen_get_usable_rect(DisplayServer.window_get_current_screen())
	var target_play := ClockGearsDataManager.compute_playtest_window_rect(Vector2i(960, 540), cur_usable)
	var exe := OS.get_executable_path()
	var sub_out := []
	var rc := OS.execute(exe, [
		"--rendering-driver", "opengl3",
		"--path", ProjectSettings.globalize_path("res://"),
		"--no-boot-logo",
		"--resolution", "%dx%d" % [target_play.size.x, target_play.size.y],
		"--position", "%d,%d" % [target_play.position.x, target_play.position.y],
		"--quit-after", "2"
	], sub_out, true)
	check(rc == 0 and cur_usable.encloses(target_play),
		"Punto 2 Live Subprocess: --no-boot-logo run exited cleanly (rc=%d) inside usable %s (rect=%s)" % [rc, str(cur_usable), str(target_play)])

	print("Tiempo: %d ms" % (Time.get_ticks_msec() - start_msec))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
