extends SceneTree

var pass_count := 0
var fail_count := 0
var start_msec := 0

func _check(cond: bool, label: String) -> void:
	if cond:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: " + label)

func _initialize() -> void:
	start_msec = Time.get_ticks_msec()
	call_deferred("_run_test")

func _process(_delta: float) -> bool:
	if Time.get_ticks_msec() - start_msec > 55000:
		print("TIMEOUT GUARD TRIGGERED")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count + 1])
		quit(1)
		return true
	return false

func _inside_card(p_r: Rect2i, p_w: int, p_h: int) -> bool:
	return p_r.size.x > 0 and p_r.size.y > 0 and p_r.position.x >= 0 and p_r.position.y >= 0 and p_r.end.x <= p_w and p_r.end.y <= p_h

func _run_test() -> void:
	var preview_dir := "C:/Proyectos/NEW MOTOR/ClockGears Engine/test_project/_tmp_p3_splash"
	DirAccess.make_dir_recursive_absolute(preview_dir)
	var out_dark_4 := preview_dir.path_join("splash_dark_s4.png")
	var out_dark_7 := preview_dir.path_join("splash_dark_s7.png")
	var out_brass_4 := preview_dir.path_join("splash_brass_s4.png")

	var d1: Dictionary = ClockGearsTheme.inspect_boot_splash(1, 7, "Inicializando entorno gráfico y audio...", "", false, Vector2i(1366, 768))
	var d4: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando núcleo y herramientas...", out_dark_4, false, Vector2i(1920, 1080))
	var d7: Dictionary = ClockGearsTheme.inspect_boot_splash(7, 7, "Listo", out_dark_7, false, Vector2i(2560, 1440))
	var b4: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando núcleo y herramientas...", out_brass_4, true, Vector2i(1920, 1080))

	# 1. Dimensiones 16:9, esquinas redondeadas y cero margen gris
	_check(int(d4.get("width", 0)) == 800 and int(d4.get("height", 0)) == 450, "Tarjeta de carga es 800x450 exacto")
	_check(bool(d4.get("aspect_ratio_16_9", false)), "Proporción 16:9 exacta")
	_check(int(d4.get("corner_radius", 0)) == ClockGearsTheme.get_container_corner_radius(1.0), "Radio de esquina coincide con el tema (6 px)")
	_check(int(d4.get("polygon_points", 0)) >= 32, "Polígono redondeado para recorte borderless en Windows generado")
	_check(float(d4.get("corner_outer_alpha", 1.0)) == 0.0, "Esquina externa recortada transparente (sin margen gris)")

	var top_bd: Color = d4.get("top_border_color", Color())
	var exp_bd: Color = d4.get("expected_border_color", Color())
	_check(top_bd.r > top_bd.b and absf(top_bd.r - exp_bd.r) < 0.35, "Borde superior cobrizo del tema presente")

	var cbg: Color = d4.get("center_bg_color", Color())
	_check(cbg.get_luminance() < 0.15, "Fondo oscuro sin rectángulo gris (luminancia=%f)" % cbg.get_luminance())

	# 2. Centrado y ajuste en 1366x768, 1920x1080 y 2560x1440
	for item in [[d1, "1366x768"], [d4, "1920x1080"], [d7, "2560x1440"]]:
		var d: Dictionary = item[0]
		var lbl: String = item[1]
		_check(bool(d.get("fits_screen", false)), "Cabe centrada sin cortes en " + lbl)

	# 3. Versión sincronizada con punto 2 y textos completos dentro de la tarjeta sin solapes
	_check(String(d4.get("version_text", "")) == ClockGearsNewProjectDialog.get_splash_version(), "Versión del splash coincide con Point 2 (v0.0.4 ALPHA)")
	_check(String(d4.get("status_text", "")).contains("(57 %)"), "Fase real muestra porcentaje calculado real: " + String(d4.get("status_text", "")))
	_check(String(d7.get("status_text", "")) == "Listo", "Al finalizar muestra 'Listo'")

	for key in ["icon_rect", "badge_rect", "title_rect", "subtitle_rect", "credits_rect", "status_rect", "right_brand_rect", "progress_bar_rect"]:
		var r: Rect2i = d4.get(key, Rect2i())
		_check(_inside_card(r, 800, 450), key + " dentro de 800x450 sin recortes")

	var icon_r: Rect2i = d4.get("icon_rect", Rect2i())
	var title_r: Rect2i = d4.get("title_rect", Rect2i())
	var sub_r: Rect2i = d4.get("subtitle_rect", Rect2i())
	var cred_r: Rect2i = d4.get("credits_rect", Rect2i())
	var st_r: Rect2i = d4.get("status_rect", Rect2i())
	var rb_r: Rect2i = d4.get("right_brand_rect", Rect2i())
	var bar_r: Rect2i = d4.get("progress_bar_rect", Rect2i())

	_check(icon_r.size == Vector2i(128, 128), "Icono central es 128x128 píxel art")
	_check(not icon_r.intersects(title_r), "Icono no solapa título CLOCKGEARS ENGINE")
	_check(not title_r.intersects(sub_r) and not sub_r.intersects(cred_r), "Títulos y créditos no se solapan")
	_check(not st_r.intersects(rb_r), "Texto de fase izquierdo y marca derecha no se solapan")
	_check(not st_r.intersects(bar_r) and not rb_r.intersects(bar_r), "Textos inferiores no solapan la barra de progreso")

	# 4. Progreso real monotónico en barras (sin barra falsa ni saltos)
	var f1 := int(d1.get("filled_bar_px", 0))
	var f4 := int(d4.get("filled_bar_px", 0))
	var f7 := int(d7.get("filled_bar_px", 0))
	_check(f1 > 60 and f1 < f4 and f4 < f7 and f7 >= 735, "Progreso real de barra ámbar: %d -> %d -> %d px" % [f1, f4, f7])

	# 5. Coherencia con tema Latón y PNGs generados legibles
	var brass_bg: Color = b4.get("center_bg_color", Color())
	_check(brass_bg.r > brass_bg.b, "Variante Latón usa tonalidad cálida del tema ClockGearsTheme")
	_check(FileAccess.file_exists(out_dark_4) and FileAccess.file_exists(out_dark_7) and FileAccess.file_exists(out_brass_4), "Capturas PNG de pantalla de carga generadas")

	# 6. Verificar integración real de arranque en main.cpp
	var main_cpp := FileAccess.get_file_as_string("C:/Proyectos/NEW MOTOR/ClockGears Engine/main/main.cpp")
	_check(main_cpp.contains("ClockGearsBootSplash::present_stage"), "main.cpp llama a ClockGearsBootSplash::present_stage en fases reales")
	_check(main_cpp.contains("WINDOW_FLAG_BORDERLESS_BIT") and main_cpp.contains("_cg_finish_splash()"), "main.cpp usa borderless 800x450 durante arranque y restaura suavemente")

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
