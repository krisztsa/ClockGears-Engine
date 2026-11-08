extends SceneTree

func _init():
	print("==================================================")
	print("🧪 VERIFICACIÓN DEL NÚCLEO DE SISTEMAS RPG (ClockGears Engine)")
	print("==================================================")

	# 1. Test Battler & Stats Curves
	print("\n--- [1] Prueba de Battlers y Curvas de Estadísticas ---")
	var hero = ClockGearsBattler.new()
	hero.battler_name = "Guerrero Crono"
	hero.level = 1
	hero.atk = 35
	hero.def = 25
	hero.mat = 12
	hero.mdf = 15
	hero.agi = 22
	hero.luk = 18
	print("Heroe Nv. %d -> PV: %d/%d, ATK: %d, DEF: %d, AGI: %d, LUK: %d" % [hero.level, hero.current_hp, hero.max_hp, hero.atk, hero.def, hero.agi, hero.luk])

	# Subir nivel a 10 y verificar interpolación de curva
	hero.level = 10
	print("Heroe Nv. %d (Tras Curva) -> PV: %d/%d, ATK: %d, DEF: %d, AGI: %d, LUK: %d" % [hero.level, hero.current_hp, hero.max_hp, hero.atk, hero.def, hero.agi, hero.luk])
	assert(hero.max_hp > 500, "La curva de PV debe aumentar con el nivel")

	# 2. Test Enemies & Elemental Affinities
	print("\n--- [2] Prueba de Enemigos y Afinidades Elementales ---")
	var slime = ClockGearsBattler.new()
	slime.battler_name = "Slime de Fuego"
	slime.is_enemy = true
	slime.set_max_hp(250)
	slime.set_current_hp(250)
	slime.atk = 20
	slime.def = 15
	# Fuego Absorbe (-1.0), Hielo Debilidad (1.5x / 150%), Fisico Neutro (1.0x / 100%)
	slime.set_element_affinity(1, 5) # Element 1: Fire, Affinity 5: Absorb
	slime.set_element_affinity(2, 3) # Element 2: Ice, Affinity 3: Weak (150%)
	slime.set_element_affinity(0, 2) # Element 0: Physical, Affinity 2: Neutral

	print("Slime Afinidades: Tasa Fuego = %s, Tasa Hielo = %s, Tasa Físico = %s" % [slime.get_element_rate(1), slime.get_element_rate(2), slime.get_element_rate(0)])
	assert(slime.get_element_rate(1) == -1.0, "Fuego debe ser absorbido")
	assert(slime.get_element_rate(2) == 1.5, "Hielo debe ser débil (150%)")

	# 3. Test Damage Formula Engine
	print("\n--- [3] Prueba del Evaluador de Fórmulas de Daño ---")
	var formula_phys = "a.atk * 4 - b.def * 2"
	var res_phys = ClockGearsDamageEngine.evaluate_damage(hero, slime, formula_phys, 0, 0.0, 0.0)
	print("Ataque Físico [Formula: %s] -> Dmg: %d, Crit: %s, Mult: %s" % [formula_phys, res_phys["damage"], res_phys["is_critical"], res_phys["element_rate"]])
	assert(res_phys["damage"] > 0, "El daño físico debe ser mayor a cero")

	# Hielo contra Slime (Debilidad)
	var formula_ice = "a.mat * 3 + 50 - b.mdf * 1.5"
	var res_ice = ClockGearsDamageEngine.evaluate_damage(hero, slime, formula_ice, 2, 0.0, 0.0)
	print("Hechizo de Hielo (Debilidad) -> Dmg: %d, Débil: %s, Mult: %s" % [res_ice["damage"], res_ice["is_weak"], res_ice["element_rate"]])
	assert(res_ice["is_weak"] == true, "Debe detectar debilidad al hielo")

	# Fuego contra Slime (Absorción / Curación)
	var res_fire = ClockGearsDamageEngine.evaluate_damage(hero, slime, "a.mat * 4", 1, 0.0, 0.0)
	print("Hechizo de Fuego (Absorción) -> Dmg: %d, Absorbido: %s" % [res_fire["damage"], res_fire["is_absorbed"]])
	assert(res_fire["is_absorbed"] == true, "Debe registrar absorción de fuego")
	assert(res_fire["damage"] < 0, "Daño absorbido debe ser negativo (cura)")

	# 4. Test Action RPG Components (Hitbox & Hurtbox)
	print("\n--- [4] Prueba de Combate Dinámico en Tiempo Real (Action RPG) ---")
	var hurtbox = ClockGearsHurtbox2D.new()
	hurtbox.battler = slime

	var hitbox = ClockGearsHitbox2D.new()
	hitbox.attacker = hero
	hitbox.damage_formula = "a.atk * 3 - b.def"
	hitbox.element = 0

	var hit_result = hurtbox.take_hit(hitbox)
	print("Impacto Action RPG registrado: Dmg causados al Slime = %d (PV restantes: %d/%d)" % [hit_result["damage"], slime.get_current_hp(), slime.get_max_hp()])
	assert(slime.get_current_hp() < 250, "El Slime debe haber recibido daño")

	# 5. Test Turn-Based Battle Setup
	print("\n--- [5] Prueba de Inicialización de Combate por Turnos ---")
	var turn_battle = ClockGearsTurnBattle.new()
	turn_battle.start_battle([hero], [slime])
	print("Batalla por turnos inicializada con éxito en estado: %d" % turn_battle.get_state())

	print("\n✅ ¡TODOS LOS SUBSISTEMAS DEL NÚCLEO RPG HAN PASADO EXITOSAMENTE!")
	print("==================================================")
	quit(0)
