extends SceneTree

var pass_count := 0
var fail_count := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] ", msg)
	else:
		fail_count += 1
		print("  [FAIL] ", msg)

func _init() -> void:
	print("=== TEST PUNTO 2: BARRA DE CARGA CON BRILLO ===")
	var t0 := Time.get_ticks_msec()

	var sz := Vector2i(1920, 1080)
	var m: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando base de datos...", "", false, sz, 0.5)
	var bar_rect: Rect2i = m.get("progress_bar_rect", Rect2i())
	check(bar_rect.size.y == 10, "1080p: altura de barra ~10 px (%d px)" % bar_rect.size.y)
	check(bool(m.get("glow_enabled", false)), "Efectos de brillo activos por defecto")

	# 1. Degradado horizontal de ámbar a dorado claro
	var luma_l := float(m.get("fill_left_luma", 0.0))
	var luma_r := float(m.get("fill_right_luma", 0.0))
	check(luma_r > luma_l + 0.04, "Relleno con degradado horizontal de ámbar a dorado claro (luma izq=%.3f -> der=%.3f)" % [luma_l, luma_r])

	# 2. Halo exterior ámbar y cabeza de avance más brillante con destello
	var halo_delta := float(m.get("outer_halo_delta", 0.0))
	var head_luma := float(m.get("head_luma", 0.0))
	var flare_delta := float(m.get("head_flare_delta", 0.0))
	check(halo_delta > 0.02, "Halo exterior ámbar presente alrededor de la parte llena (delta=%.4f)" % halo_delta)
	check(head_luma > luma_l + 0.08, "Cabeza de avance más brillante que el cuerpo de la barra (head_luma=%.3f vs left=%.3f)" % [head_luma, luma_l])
	check(flare_delta > 0.04, "Destello precalculado con mezcla aditiva sobre la cabeza de avance (flare_delta=%.4f)" % flare_delta)

	# 3. Respiración suave entre 85 % y 100 % con ciclo de 2 s
	var b_peak: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando...", "", false, sz, 0.5)   # t=0.5s -> sin(pi/2)=1 -> 1.00
	var b_mid: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando...", "", false, sz, 1.0)    # t=1.0s -> sin(pi)=0 -> 0.925
	var b_trough: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando...", "", false, sz, 1.5) # t=1.5s -> sin(3pi/2)=-1 -> 0.85
	var b_cycle: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando...", "", false, sz, 2.5)  # t=2.5s -> ciclo de 2s completo -> 1.00

	var f_peak := float(b_peak.get("glow_breath_factor", 0.0))
	var f_mid := float(b_mid.get("glow_breath_factor", 0.0))
	var f_trough := float(b_trough.get("glow_breath_factor", 0.0))
	var f_cycle := float(b_cycle.get("glow_breath_factor", 0.0))
	check(is_equal_approx(f_peak, 1.00), "Respiración en t=0.5s alcanza 100%% (%.3f)" % f_peak)
	check(is_equal_approx(f_mid, 0.925), "Respiración en t=1.0s pasa por 92.5%% (%.3f)" % f_mid)
	check(is_equal_approx(f_trough, 0.85), "Respiración en t=1.5s baja a 85%% (%.3f)" % f_trough)
	check(is_equal_approx(f_cycle, f_peak), "Ciclo exacto de 2.0 s verificado (t=0.5s %.3f == t=2.5s %.3f)" % [f_peak, f_cycle])

	# 4. Modo indeterminado cuando una etapa no puede medir su progreso
	var m_ind1: Dictionary = ClockGearsTheme.inspect_boot_splash(-1, 7, "Preparando módulos...", "", false, sz, 0.2)
	var m_ind2: Dictionary = ClockGearsTheme.inspect_boot_splash(-1, 7, "Preparando módulos...", "", false, sz, 0.9)
	check(bool(m_ind1.get("indeterminate", false)), "Modo indeterminado activo cuando stage < 0")
	check(String(m_ind1.get("status_text", "")) == "Preparando módulos...", "Modo indeterminado no inventa porcentaje (%s)" % String(m_ind1.get("status_text", "")))
	check(absf(float(m_ind1.get("head_x", 0.0)) - float(m_ind2.get("head_x", 0.0))) > 40.0, "Segmento indeterminado recorre la barra de lado a lado (x1=%.1f -> x2=%.1f)" % [float(m_ind1.get("head_x", 0.0)), float(m_ind2.get("head_x", 0.0))])

	# 5. Al llegar al 100 % ("Listo")
	var m_done: Dictionary = ClockGearsTheme.inspect_boot_splash(7, 7, "Listo", "", false, sz, 0.5)
	check(String(m_done.get("status_text", "")) == "Listo", "Al 100%% el texto cambia a 'Listo'")
	check(int(m_done.get("filled_bar_px", 0)) >= bar_rect.size.x - 4, "Al 100%% la barra está completamente llena (%d/%d px)" % [int(m_done.get("filled_bar_px", 0)), bar_rect.size.x])

	# 6. Si "Efectos de brillo" está desactivado, barra estática sin halo ni destello animado
	ProjectSettings.set_setting("clockgears/editor/glow_effects", false)
	var m_noglow: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando...", "", false, sz, 0.5)
	check(not bool(m_noglow.get("glow_enabled", true)), "Desactivar 'clockgears/editor/glow_effects' desactiva el brillo animado")
	check(absf(float(m_noglow.get("outer_halo_delta", 1.0))) < 0.005, "Sin efectos de brillo no se dibuja halo exterior (delta=%.4f)" % float(m_noglow.get("outer_halo_delta", 1.0)))
	ProjectSettings.set_setting("clockgears/editor/glow_effects", true)

	print("Tiempo test: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
