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
	print("=== TEST PUNTO 3: PARTÍCULAS PIXEL-ART, POOL FIJO Y PRESUPUESTO ===")
	var t0 := Time.get_ticks_msec()

	# 1. Simular 600 fotogramas con semilla fija (12345) y verificar límite <= 120, motas <= 20 y pool fijo sin crecer
	var sim: Dictionary = ClockGearsTheme.simulate_boot_splash_particles(600, 12345, 2.0, true, Vector2i(1920, 1080))
	var max_tot := int(sim.get("max_observed_total", 0))
	var max_spk := int(sim.get("max_observed_sparks", 0))
	var max_amb := int(sim.get("max_observed_ambient", 0))
	var pool_cap := int(sim.get("pool_capacity", 0))
	var never_grew := bool(sim.get("pool_never_grew", false))
	var burst_done := bool(sim.get("burst_done", false))
	var sparks_at_burst := int(sim.get("sparks_at_burst", 0))

	check(max_tot > 25 and max_tot <= 120, "600 fotogramas (seed=12345): partículas activas siempre en (25..120], máx=%d" % max_tot)
	check(max_amb == 20, "Motas de ambiente doradas activas <= 20 (%d)" % max_amb)
	check(max_spk > 10 and max_spk <= 100, "Chispas de la cabeza de avance activas <= 100 (%d)" % max_spk)
	check(pool_cap == 120 and never_grew, "Pool fijo reutilizable de capacidad 120 que nunca crece ni reserva memoria por fotograma")
	check(burst_done and sparks_at_burst >= 15, "Al llegar al 100%% ('Listo') se emite ráfaga de partículas (activas en ráfaga=%d)" % sparks_at_burst)

	# 2. Reducción automática si el fotograma tarda más de 16,7 ms
	var sim_slow: Dictionary = ClockGearsTheme.simulate_boot_splash_particles(60, 12345, 22.0, true, Vector2i(1920, 1080))
	var min_cap_slow := int(sim_slow.get("min_observed_cap", 120))
	var max_tot_slow := int(sim_slow.get("max_observed_total", 120))
	check(min_cap_slow < 120, "Si el fotograma tarda > 16,7 ms (22 ms), se reduce automáticamente el tope de partículas (cap=%d < 120)" % min_cap_slow)
	check(max_tot_slow <= 120, "Bajo carga > 16,7 ms las partículas activas respetan el presupuesto (%d)" % max_tot_slow)

	# 3. Si "Efectos de brillo" está desactivado, 0 partículas activas
	var sim_off: Dictionary = ClockGearsTheme.simulate_boot_splash_particles(120, 12345, 2.0, false, Vector2i(1920, 1080))
	check(int(sim_off.get("max_observed_total", 99)) == 0, "Con 'Efectos de brillo' desactivado no se emiten ni dibujan partículas (total=%d)" % int(sim_off.get("max_observed_total", 99)))

	# 4. Render real en inspect_boot_splash incluye métricas de partículas activas
	var m_render: Dictionary = ClockGearsTheme.inspect_boot_splash(4, 7, "Cargando mapas...", "", false, Vector2i(1920, 1080), 0.5)
	check(int(m_render.get("active_particle_count", 0)) > 0 and int(m_render.get("active_particle_count", 0)) <= 120,
			"Render real dibuja partículas dentro del presupuesto (%d activas)" % int(m_render.get("active_particle_count", 0)))

	print("Tiempo test: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
