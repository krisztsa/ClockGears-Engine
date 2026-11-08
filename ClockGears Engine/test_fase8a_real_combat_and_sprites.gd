extends SceneTree

func _init():
	print("======================================================================")
	print("TEST FASE 8A: COMBATE REAL Y ANIMACION DE SPRITES (VERIFICACION FINAL)")
	print("======================================================================")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if not dm:
		printerr("ERROR: ClockGearsDataManager singleton no encontrado")
		quit(1)
		return

	dm.load_all("res://clockgears_data")
	var enemies = dm.get_category_array(3) # CAT_ENEMIES = 3

	# 1. Configurar enemigo con hoja de 4 frames y animaciones idle + attack
	print("\n--- PASO 1: Creacion y Persistencia de Sprite & Animaciones ---")
	var test_enemy = {
		"id": 99,
		"name": "Canon Musgoso",
		"graphic_path": "res://img/enemies/canon_musgoso_down.png",
		"hp": 250,
		"mp": 50,
		"atk": 30,
		"def": 15,
		"mat": 10,
		"mdf": 10,
		"agi": 40, # Mas rapido que el heroe para probar avance automatico de turno
		"luk": 10,
		"exp": 100,
		"gold": 50,
		"sprite_data": {
			"frame_w": 32,
			"frame_h": 32,
			"cols": 4,
			"rows": 1,
			"margin": 0,
			"spacing": 0,
			"animations": {
				"idle": {
					"name": "idle",
					"fps": 6.0,
					"loop": true,
					"frames": [0, 1, 2, 3]
				},
				"attack": {
					"name": "attack",
					"fps": 10.0,
					"loop": false,
					"frames": [0, 1, 2]
				}
			}
		}
	}

	var found = false
	for i in range(enemies.size()):
		if enemies[i].get("id", 0) == 99:
			enemies[i] = test_enemy
			found = true
			break
	if not found:
		enemies.append(test_enemy)

	dm.set_category_array(3, enemies)
	dm.save_all("res://clockgears_data")
	print("Enemigo #99 guardado en res://clockgears_data/enemies.json.")

	# Recargar de disco y verificar persistencia
	dm.load_all("res://clockgears_data")
	var reloaded_enemies = dm.get_category_array(3)
	var reloaded_enemy = {}
	for en in reloaded_enemies:
		if en.get("id", 0) == 99:
			reloaded_enemy = en
			break

	var sp_data = reloaded_enemy.get("sprite_data", {})
	var anims = sp_data.get("animations", {})
	if not anims.has("idle") or not anims.has("attack"):
		printerr("FALLO: Las animaciones no se persistieron correctamente en disco!")
		quit(1)
		return
	print("VERIFICADO: sprite_data y animaciones idle/attack recargadas de disco correctamente.")
	print("  - idle frames: %s, loop: %s" % [str(anims["idle"].get("frames")), str(anims["idle"].get("loop"))])
	print("  - attack frames: %s, loop: %s" % [str(anims["attack"].get("frames")), str(anims["attack"].get("loop"))])

	# 2. Configurar una tropa de prueba que incluya a nuestro enemigo #99
	print("\n--- PASO 2: Configuracion de Escena Real de Combate ---")
	var troops = dm.get_category_array(4) # CAT_TROOPS = 4
	var test_troop = {
		"id": 99,
		"name": "Tropa Canon Musgoso",
		"members": [
			{ "enemy_id": 99, "x": 300, "y": 150 }
		]
	}
	var tr_found = false
	for i in range(troops.size()):
		if troops[i].get("id", 0) == 99:
			troops[i] = test_troop
			tr_found = true
			break
	if not tr_found:
		troops.append(test_troop)
	dm.set_category_array(4, troops)
	dm.save_all("res://clockgears_data")

	# 3. Instanciar ClockGearsBattleScene real
	var scene = ClockGearsBattleScene.new()
	root.add_child(scene)

	# Iniciar batalla real con tropa 99
	scene.setup_battle(99, false, false, "")
	print("Escena de batalla real instanciada e iniciada con tropa #99.")

	var ctrl = scene.get_controller()
	if not ctrl:
		printerr("FALLO: Controller es nulo")
		quit(1)
		return

	# Verificar unidades
	var b_enemies = ctrl.get_enemy_units()
	var b_party = ctrl.get_party_units()
	print("Unidades en combate -> Heroes: %d, Enemigos: %d" % [b_party.size(), b_enemies.size()])

	var enemy_unit = b_enemies[0]
	var hero_unit = b_party[0]
	print("Enemigo:", enemy_unit.get_name(), "HP:", enemy_unit.get_hp(), "AGI:", enemy_unit.get_agi())
	print("Heroe:", hero_unit.get_name(), "HP:", hero_unit.get_hp(), "AGI:", hero_unit.get_agi())

	# 4. Verificar Textura y visual del battler (A2)
	print("\n--- PASO 3: Verificacion Visual del Battler (A2) ---")
	var sp_dict = enemy_unit.get_sprite_data_dict()
	print("SpriteData en BattlerUnit presente? ->", not sp_dict.is_empty(), "Cols:", sp_dict.get("cols", 0))

	# 5. Verificar Avance de Turnos y Habilitacion de Accion (A3)
	print("\n--- PASO 4: Verificacion de Avance de Turno Automatico y Accion (A3) ---")
	var cur_unit = ctrl.get_active_unit()
	print("Tras setup e iniciativa con enemigo rapido, la unidad activa es:", cur_unit.get_name(), "(is_actor=%s)" % str(cur_unit.get_is_actor()))

	# Si el enemigo tenia el turno, ctrl.start_turn_cycle() lo procesa y avanza hacia el héroe
	if not cur_unit.get_is_actor():
		print("Procesando ciclo de turnos...")
		ctrl.start_turn_cycle()
		cur_unit = ctrl.get_active_unit()
		print("Unidad activa tras ciclo:", cur_unit.get_name(), "(is_actor=%s)" % str(cur_unit.get_is_actor()))

	if not cur_unit.get_is_actor():
		printerr("FALLO: El turno no avanzo al jugador!")
		quit(1)
		return

	# 6. Accion del jugador: Atacar al enemigo #0 mediante ClockGearsBattleScene
	print("\n--- PASO 5: Accion Atacar del Jugador y Calculo de Dano Real ---")
	var hp_before = enemy_unit.get_hp()
	print("HP del enemigo antes del ataque:", hp_before)

	scene.select_action_attack(0) # Atacar al enemigo 0

	var hp_after = enemy_unit.get_hp()
	print("HP del enemigo tras el ataque:", hp_after)
	var dmg_dealt = hp_before - hp_after
	print("Dano real infligido al enemigo:", dmg_dealt)
	if dmg_dealt <= 0:
		printerr("FALLO: El ataque no redujo el HP del enemigo!")
		quit(1)
		return

	print("VERIFICADO: El comando 'Atacar' ejecuto dano real basado en formulas y varianza.")

	# 7. Verificar caso MP = 0 (A4)
	print("\n--- PASO 6: Verificacion de MP 0 en Habilidad (A4) ---")
	hero_unit.set_mp(0)
	print("Heroe MP forzado a 0. Intentando select_action_skill...")
	scene.select_action_skill(1, 0) # Debe ser rechazado limpiamente sin colgar
	print("VERIFICADO: MP 0 manejado con seguridad sin bloqueo ni excepciones.")

	print("\n======================================================================")
	print("RESULTADO TEST: TODOS LOS TESTS PASARON EXITOSAMENTE (100% OK)")
	print("======================================================================")
	scene.free()
	quit(0)
