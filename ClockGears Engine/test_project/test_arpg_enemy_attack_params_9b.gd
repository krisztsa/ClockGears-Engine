extends SceneTree

const TIMEOUT_S := 15.0
var _elapsed := 0.0

func _process(delta: float) -> bool:
	_elapsed += delta
	if _elapsed > TIMEOUT_S:
		print("FAIL: Timeout en test_arpg_enemy_attack_params")
		quit(1)
		return true

	_run_tests()
	return true

func _run_tests():
	print("=== INICIANDO TEST: PARAMETROS DE ATAQUE DE ENEMIGOS ARPG ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		print("FAIL: ClockGearsDataManager no disponible")
		quit(1)
		return

	# 1. TEST DE REGISTRO ANTIGUO / POR DEFECTO
	var old_rec = {
		"id": 1,
		"name": "Enemigo Base",
		"hp": 100,
		"atk": 20,
		"def": 10
	}
	var norm_old = ClockGearsDataManager.get_normalized_record(20, old_rec) # 20: CAT_ARPG_ENEMIES
	
	if norm_old["attack_interval"] != 1.5:
		print("FAIL: default attack_interval no es 1.5: ", norm_old["attack_interval"])
		quit(1)
		return
	if norm_old["attack_interval_variance_pct"] != 0.0:
		print("FAIL: default attack_interval_variance_pct no es 0.0: ", norm_old["attack_interval_variance_pct"])
		quit(1)
		return
	if norm_old["attack_speed_pct"] != 100.0:
		print("FAIL: default attack_speed_pct no es 100.0: ", norm_old["attack_speed_pct"])
		quit(1)
		return
	if norm_old["attack_windup_ms"] != 0:
		print("FAIL: default attack_windup_ms no es 0: ", norm_old["attack_windup_ms"])
		quit(1)
		return
	if norm_old["attack_recovery_ms"] != 0:
		print("FAIL: default attack_recovery_ms no es 0: ", norm_old["attack_recovery_ms"])
		quit(1)
		return
	if norm_old["attack_damage_mult_pct"] != 100.0:
		print("FAIL: default attack_damage_mult_pct no es 100.0: ", norm_old["attack_damage_mult_pct"])
		quit(1)
		return
	if norm_old["attack_range"] != 1.0:
		print("FAIL: default attack_range no es 1.0: ", norm_old["attack_range"])
		quit(1)
		return
	print("OK: Registro antiguo migra con todos los valores por defecto esperados.")

	# 2. TEST DE COMPORTAMIENTO EN RUNTIME (ClockGearsEvent)
	var ev_old = ClassDB.instantiate("ClockGearsEvent")
	root.add_child(ev_old)
	ev_old.apply_behavior_record_config(norm_old, 120.0)
	
	if ev_old.get_attack_interval() != 1.5 or ev_old.get_attack_damage_mult_pct() != 100.0:
		print("FAIL: ev_old no cargo los parametros correctamente")
		quit(1)
		return
	
	ev_old.start_arpg_attack()
	if abs(ev_old.get_arpg_attack_cooldown_timer() - 1.5) > 0.01:
		print("FAIL: ev_old cooldown tras atacar != 1.5s: ", ev_old.get_arpg_attack_cooldown_timer())
		quit(1)
		return
	print("OK: Enemigo por defecto ataca con cooldown exacto de 1.5s (sin varianza ni anticipacion).")

	# 3. TEST DE ENEMIGO CON PARAMETROS PERSONALIZADOS
	var custom_rec = {
		"id": 2,
		"name": "Jefe Berserker",
		"hp": 500,
		"atk": 50,
		"def": 25,
		"attack_interval": 2.0,
		"attack_interval_variance_pct": 25.0,
		"attack_speed_pct": 150.0,
		"attack_windup_ms": 300,
		"attack_recovery_ms": 400,
		"attack_damage_mult_pct": 200.0,
		"attack_range": 2.5
	}
	var norm_custom = ClockGearsDataManager.get_normalized_record(20, custom_rec)
	
	var ev_custom = ClassDB.instantiate("ClockGearsEvent")
	root.add_child(ev_custom)
	ev_custom.apply_behavior_record_config(norm_custom, 120.0)
	
	if ev_custom.get_attack_interval() != 2.0:
		print("FAIL: ev_custom attack_interval != 2.0: ", ev_custom.get_attack_interval())
		quit(1)
		return
	if ev_custom.get_attack_interval_variance_pct() != 25.0:
		print("FAIL: ev_custom attack_interval_variance_pct != 25.0: ", ev_custom.get_attack_interval_variance_pct())
		quit(1)
		return
	if ev_custom.get_attack_speed_pct() != 150.0:
		print("FAIL: ev_custom attack_speed_pct != 150.0: ", ev_custom.get_attack_speed_pct())
		quit(1)
		return
	if ev_custom.get_attack_windup_ms() != 300:
		print("FAIL: ev_custom attack_windup_ms != 300: ", ev_custom.get_attack_windup_ms())
		quit(1)
		return
	if ev_custom.get_attack_recovery_ms() != 400:
		print("FAIL: ev_custom attack_recovery_ms != 400: ", ev_custom.get_attack_recovery_ms())
		quit(1)
		return
	if ev_custom.get_attack_damage_mult_pct() != 200.0:
		print("FAIL: ev_custom attack_damage_mult_pct != 200.0: ", ev_custom.get_attack_damage_mult_pct())
		quit(1)
		return
	if ev_custom.get_attack_range() != 2.5:
		print("FAIL: ev_custom attack_range != 2.5: ", ev_custom.get_attack_range())
		quit(1)
		return

	# Iniciar ataque del enemigo personalizado y verificar que el cooldown cae entre [1.5, 2.5] (2.0 ± 25%)
	ev_custom.start_arpg_attack()
	var cd = ev_custom.get_arpg_attack_cooldown_timer()
	if cd < 1.45 or cd > 2.55:
		print("FAIL: ev_custom cooldown fuera del rango con variacion (2.0 ± 25%%): ", cd)
		quit(1)
		return
	print("OK: Enemigo personalizado calcula cooldown con variacion (valor obtenido: %.2fs en rango [1.5, 2.5])." % cd)

	# Limpiar nodos
	ev_old.queue_free()
	ev_custom.queue_free()

	print("ALL PASS: Parametros de ataque de enemigos ARPG aplicados y verificados con exito.")
	quit(0)
