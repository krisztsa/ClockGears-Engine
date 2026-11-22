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
	print("Tiempo: %d ms" % (Time.get_ticks_msec() - start_msec))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
