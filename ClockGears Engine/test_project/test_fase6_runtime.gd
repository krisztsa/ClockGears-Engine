extends SceneTree

func _initialize():
	print("==================================================")
	print("   TEST FASE 6: SECCIONES 5.1-5.3 + RUNTIME BASE  ")
	print("==================================================")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	var cat = Engine.get_singleton("ClockGearsSystemCatalog")
	assert(dm != null, "ClockGearsDataManager debe existir")
	assert(cat != null, "ClockGearsSystemCatalog debe existir")

	dm.init_empty()

	# 1. Verificar Sección 5.1 (Armas, Armaduras, Accesorios con Ranura, Precio, 8 Params y Traits)
	var wid = dm.create_record(4, {"name": "Espada de Engranajes"}) # CAT_WEAPONS = 4
	var wrec = dm.get_record_by_id(4, wid)
	wrec["weapon_type"] = 4
	wrec["equip_slot"] = 1
	wrec["animation_id"] = 0
	wrec["price"] = 250
	wrec["params"] = [0, 0, 18, -2, 0, 0, 4, 0]
	wrec["traits"] = [{"code": 21, "data_id": 2, "value": 110.0}]
	dm.update_record(4, wid, wrec)

	var saved_w = dm.get_record_by_id(4, wid)
	assert(saved_w["price"] == 250, "5.1 Precio de arma guardado")
	assert(saved_w["params"][2] == 18 and saved_w["params"][3] == -2, "5.1 8 parámetros con modificadores negativos")
	assert(saved_w["traits"].size() == 1, "5.1 Rasgos (Traits) guardados")

	# 2. Verificar Sección 5.2 (Objetos con Tipo, Consumible, Ámbito, Ocasión, Golpe, Fórmula y Efectos)
	var iid = dm.create_record(3, {"name": "Elixir de Vapor"}) # CAT_ITEMS = 3
	var irec = dm.get_record_by_id(3, iid)
	irec["item_type"] = 0 # Objeto Regular
	irec["consumable"] = true
	irec["scope"] = 7 # 1 Aliado
	irec["occasion"] = 0 # Siempre
	irec["speed"] = 10
	irec["success_rate"] = 100
	irec["repeats"] = 1
	irec["tp_gain"] = 5
	irec["hit_type"] = 0 # Golpe Seguro
	irec["damage_element"] = 1
	irec["formula"] = "100 + a.mat * 2"
	irec["variance"] = 20
	irec["critical"] = false
	irec["effects"] = [{"code": 11, "data_id": 0, "value1": 50.0, "value2": 100.0}]
	dm.update_record(3, iid, irec)

	var saved_i = dm.get_record_by_id(3, iid)
	assert(saved_i["consumable"] == true and saved_i["scope"] == 7, "5.2 Consumible y Ámbito guardados")
	assert(saved_i["effects"].size() == 1, "5.2 Efectos múltiples guardados")

	# 3. Verificar Sección 5.3 (Enemigos con 8 Stats, EXP, Oro, 3 Drops y Patrones de Acción)
	var eid = dm.create_record(7, {"name": "Autómata Oxidado"}) # CAT_ENEMIES = 7
	var erec = dm.get_record_by_id(7, eid)
	erec["exp"] = 45
	erec["gold"] = 30
	erec["battler_frames"] = 4
	erec["battler_fps"] = 8
	erec["drops"] = [
		{"kind": 1, "data_id": iid, "probability": 50},
		{"kind": 2, "data_id": wid, "probability": 15},
		{"kind": 0, "data_id": 0, "probability": 100}
	]
	erec["actions"] = [
		{"skill_id": 1, "condition_type": 0, "param1": 0.0, "param2": 0.0, "rating": 5},
		{"skill_id": 2, "condition_type": 2, "param1": 0.0, "param2": 50.0, "rating": 8}
	]
	dm.update_record(7, eid, erec)

	var saved_e = dm.get_record_by_id(7, eid)
	assert(saved_e["drops"].size() == 3 and saved_e["drops"][0]["data_id"] == iid, "5.3 3 Drops de enemigo guardados")
	assert(saved_e["actions"].size() == 2 and saved_e["actions"][1]["rating"] == 8, "5.3 Patrones de acción IA guardados")

	# 4. Verificar FASE 6 Runtime Base (Jugador 32x32, Seguidores, Cámara Pixel-Perfect, Intérprete de Eventos)
	var a1 = dm.create_record(0, {"name": "Chronos"}) # CAT_ACTORS = 0
	var a2 = dm.create_record(0, {"name": "Aethel"})
	var sys = dm.get_system_config()
	sys["party"] = [a1, a2]
	sys["start_x"] = 4
	sys["start_y"] = 5
	dm.set_system_config(sys)

	var map = ClockGearsMap.new()
	map.set_map_name("Mapa de Prueba Fase 6")
	map.set_map_size(Vector2i(20, 15))
	map.ensure_default_layers()

	# Añadir un evento de Cofre en (5, 5) (a la derecha de la posición inicial 4, 5)
	var ev = ClockGearsEvent.new()
	ev.set_event_name("EV_Cofre_Test")
	ev.set_position(Vector2(5 * 32, 5 * 32))
	var p1 = {
		"sw1_en": false, "sw2_en": false, "self_sw_en": false, "self_sw_letter": "A",
		"var_en": false, "item_en": false, "actor_en": false,
		"trigger": 0, "priority": 1, "through": false, "dir_fix": true,
		"graphic_path": "", "hframes": 1, "vframes": 1, "frame": 0,
		"move_type": 0, "speed": 3, "freq": 3,
		"commands": [
			{"type": "gold", "op": "+", "amount": 250},
			{"type": "self_switch", "letter": "A", "val": true}
		]
	}
	var p2 = {
		"sw1_en": false, "sw2_en": false, "self_sw_en": true, "self_sw_letter": "A",
		"var_en": false, "item_en": false, "actor_en": false,
		"trigger": 0, "priority": 1, "through": false, "dir_fix": true,
		"graphic_path": "", "hframes": 1, "vframes": 1, "frame": 0,
		"move_type": 0, "speed": 3, "freq": 3,
		"commands": []
	}
	ev.set_pages([p1, p2])
	map.add_clockgears_event(ev, Vector2(5 * 32, 5 * 32))

	get_root().add_child(map)
	map.init_runtime_session()

	var player = map.get_runtime_player()
	var followers = map.get_runtime_followers()
	var camera = map.get_runtime_camera()
	var interpreter = map.get_runtime_interpreter()

	assert(player != null, "Fase 6: ClockGearsPlayer instanciado en el mapa")
	assert(followers != null and followers.get_follower_count() == 1, "Fase 6: ClockGearsFollowerManager con 1 seguidor activo")
	assert(camera != null and camera.is_smooth_follow_enabled(), "Fase 6: ClockGearsPixelCamera2D activa con seguimiento pixel-perfect")
	assert(interpreter != null, "Fase 6: ClockGearsEventInterpreter activo")
	assert(player.get_grid_cell() == Vector2i(4, 5), "Fase 6: Posición inicial (4, 5) leída desde categoría Sistema")

	# Simular paso del jugador hacia arriba (4, 4) y verificar que el seguidor avanza a (4, 5)
	var moved = player.try_step(8) # DIR_UP = 8
	assert(moved == true, "Fase 6: Movimiento en rejilla 32x32 hacia celda pasable (4, 4)")
	assert(player.get_grid_cell() == Vector2i(4, 4), "Fase 6: Celda destino del jugador actualizada a (4, 4)")

	# Regresar a (4, 5), mirar a la derecha (6) hacia el cofre en (5, 5), verificar bloqueo por prioridad Normal
	player.set_grid_cell_immediate(Vector2i(4, 5), 6) # DIR_RIGHT = 6
	var step_into_event = player.try_step(6)
	assert(step_into_event == false, "Fase 6: Colisión con evento de prioridad Normal (Al mismo nivel) bloquea el paso")

	# Activar evento frontal con try_activate_front_event() y ejecutar sus comandos (oro +250, Interruptor Local A = ON)
	var triggered = player.try_activate_front_event()
	assert(triggered == true, "Fase 6: Activación de evento frontal con Botón de Acción")
	interpreter.step_frame()
	interpreter.step_frame()
	assert(dm.get_gold() == 250, "Fase 6: Intérprete ejecutó comando Cambiar Oro (+250)")
	assert(ev.get_self_switch("A") == true and ev.get_active_page_index() == 1, "Fase 6: Interruptor Local A activó Página 2 en vivo")

	map.free()
	print("[OK] TODAS LAS ASERCIONES DE SECCIONES 5.1-5.3 Y FASE 6 SUPERADAS CON EXITO.")
	quit(0)
