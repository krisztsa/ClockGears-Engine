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

func _inside_rect(inner: Rect2i, outer: Rect2i) -> bool:
	return inner.size.x > 0 and inner.size.y > 0 and inner.position.x >= outer.position.x and inner.position.y >= outer.position.y and inner.end.x <= outer.end.x and inner.end.y <= outer.end.y

func _initialize() -> void:
	start_ms = Time.get_ticks_msec()
	var watchdog := create_timer(55.0)
	watchdog.timeout.connect(func():
		fail_count += 1
		print("[FAIL] Watchdog 55s timeout")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
	)
	call_deferred("_run")

func _run() -> void:
	var resolutions := [
		Vector2i(1366, 768),
		Vector2i(1920, 1080),
		Vector2i(2560, 1440),
		Vector2i(3840, 2160),
		Vector2i(1920, 1200), # 16:10
		Vector2i(2560, 1080)  # 21:9 ultrawide
	]

	for res in resolutions:
		var tag := "%dx%d" % [res.x, res.y]
		var d: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando núcleo y herramientas...", "", false, res)
		_check(int(d.get("width", 0)) == res.x and int(d.get("height", 0)) == res.y and bool(d.get("is_fullscreen", false)),
			tag + ": pantalla de carga ocupa toda la resolución completa")
		_check(is_equal_approx(float(d.get("corner_outer_alpha", 0.0)), 1.0),
			tag + ": fondo oscuro opaco hasta los bordes exteriores (sin margen gris)")

		var icon_r: Rect2i = d.get("icon_rect", Rect2i())
		var mult := int(d.get("icon_scale_multiple", 0))
		_check(icon_r.size.x == icon_r.size.y and icon_r.size.x >= 128 and icon_r.size.x <= 256 and (icon_r.size.x % 64) == 0 and mult >= 2 and mult <= 4,
			tag + ": icono en múltiplo entero (%d px, %dx)" % [icon_r.size.x, mult])

		var inner_r: Rect2i = d.get("inner_frame_rect", Rect2i())
		var inset := int(d.get("frame_inset", 0))
		if res == Vector2i(1920, 1080):
			_check(inset == 24, "1920x1080: marco cobrizo interior separado exactamente 24 px del borde")
		else:
			_check(inset >= 12 and inner_r.position.x == inset and inner_r.size.x == res.x - inset * 2,
				tag + ": marco cobrizo interior proporcional (inset=%d px)" % inset)

		var top_bd: Color = d.get("top_border_color", Color())
		var exp_bd: Color = d.get("expected_border_color", Color())
		_check(top_bd.r > top_bd.b and absf(top_bd.r - exp_bd.r) < 0.35,
			tag + ": marco interior cobrizo dibujado con color del tema")

		_check(int(d.get("ornament_gear_count", 0)) >= 8 and int(d.get("circuit_trace_count", 0)) >= 10,
			tag + ": engranajes (8) y circuitos (10) numerosos anclados a esquinas y bordes")

		var badge_r: Rect2i = d.get("badge_rect", Rect2i())
		var title_r: Rect2i = d.get("title_rect", Rect2i())
		var sub_r: Rect2i = d.get("subtitle_rect", Rect2i())
		var cred_r: Rect2i = d.get("credits_rect", Rect2i())
		var st_r: Rect2i = d.get("status_rect", Rect2i())
		var rb_r: Rect2i = d.get("right_brand_rect", Rect2i())
		var bar_r: Rect2i = d.get("progress_bar_rect", Rect2i())

		_check(_inside_rect(badge_r, inner_r) and badge_r.position.x > res.x / 2 and badge_r.position.y < res.y / 4,
			tag + ": etiqueta 'v0.0.4 ALPHA' arriba a la derecha dentro del marco interior sin cortes")
		_check(abs((icon_r.position.x + icon_r.size.x / 2) - res.x / 2) <= 2 and
			abs((title_r.position.x + title_r.size.x / 2) - res.x / 2) <= 4 and
			abs((bar_r.position.x + bar_r.size.x / 2) - res.x / 2) <= 2,
			tag + ": icono, título y bloque de barra centrados horizontalmente")

		var bar_ratio := float(d.get("bar_width_ratio", 0.0))
		_check(bar_ratio >= 0.38 and bar_ratio <= 0.42 and bar_r.position.y >= int(res.y * 0.66),
			tag + ": barra en el tercio inferior con ancho ~40%% (%.1f%%)" % (bar_ratio * 100.0))
		_check(not icon_r.intersects(title_r) and not title_r.intersects(sub_r) and not sub_r.intersects(cred_r) and not st_r.intersects(rb_r) and not st_r.intersects(bar_r),
			tag + ": cero solapes entre icono, textos y barra de progreso")

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
