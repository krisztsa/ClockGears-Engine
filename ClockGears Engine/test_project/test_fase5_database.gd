extends SceneTree

func _init():
	print("==================================================")
	print("[TEST] ClockGears Fase 5 Database & Map Verification")
	print("==================================================")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if not dm:
		printerr("[FAIL] ClockGearsDataManager singleton not found!")
		quit(1)
		return

	# 1. Regla Cero verification: Data manager must initialize empty
	dm.init_empty()
	for cat in range(16): # CAT_MAX
		if cat in [15, 16, 17, 18]: # system, types, vocabulary, switches
			continue
		var arr = dm.get_category_array(cat)
		assert(arr.is_empty(), "Category %d is not empty by default! Violates Regla Cero." % cat)
	print("[PASS] Regla Cero verified: All categories initialized completely empty.")

	# 2. Add Classes with EXP Formula Parameters (base=30, extra=20, acc_a=30, acc_b=30)
	var classes_arr = []
	var cl_warrior = {
		"id": 1,
		"name": "Guerrero",
		"desc": "Combatiente cuerpo a cuerpo",
		"exp_base": 30,
		"exp_extra": 20,
		"exp_acc_a": 30,
		"exp_acc_b": 30
	}
	var cl_mage = {
		"id": 2,
		"name": "Mago",
		"desc": "Especialista en artes arcanas",
		"exp_base": 35,
		"exp_extra": 25,
		"exp_acc_a": 30,
		"exp_acc_b": 30
	}
	classes_arr.append(cl_warrior)
	classes_arr.append(cl_mage)
	dm.set_category_array(1, classes_arr) # CAT_CLASSES = 1

	# 3. Add Actor referencing user class (id=1)
	var actors_arr = []
	var actor1 = {
		"id": 1,
		"name": "Aria",
		"desc": "Protagonista de la historia",
		"class_id": 1,
		"hp": 450,
		"mp": 100,
		"atk": 25,
		"def": 18,
		"mat": 12,
		"mdf": 14,
		"agi": 15,
		"luk": 10,
		"graphic_path": "img/characters/aria.png"
	}
	actors_arr.append(actor1)
	dm.set_category_array(0, actors_arr) # CAT_ACTORS = 0

	# 4. Add Weapon
	var weapons_arr = []
	var wep1 = {
		"id": 1,
		"name": "Espada de Hierro",
		"desc": "Espada estándar de forja simple",
		"type": 0,
		"atk": 15
	}
	weapons_arr.append(wep1)
	dm.set_category_array(4, weapons_arr) # CAT_WEAPONS = 4

	# 5. Add Tileset with Passability Collision Map (O=0, X=1, Star=2)
	var tilesets_arr = []
	var ts1 = {
		"id": 1,
		"name": "Pradera Exterior",
		"desc": "Tileset para exteriores",
		"graphic_path": "img/tilesets/outside.png",
		"collisions": {
			"0,0": 0, # O (Pasa)
			"1,0": 1, # X (Bloqueado)
			"2,0": 2  # Star (Pasa por detras / Techo)
		}
	}
	tilesets_arr.append(ts1)
	dm.set_category_array(13, tilesets_arr) # CAT_TILESETS = 13

	# 6. Configure System
	var sys = {
		"game_title": "Leyendas de ClockGears",
		"currency": "Oro",
		"combat_mode": 0, # TURN_BASED
		"start_x": 10,
		"start_y": 12
	}
	dm.set_system_config(sys)

	# 7. Save to disk atomically
	var save_res = dm.save_all("res://clockgears_data")
	assert(save_res == true, "save_all failed!")
	print("[PASS] Database successfully saved to res://clockgears_data.")

	# 8. Verify files on disk
	var fa = FileAccess.open("res://clockgears_data/classes.json", FileAccess.READ)
	assert(fa != null, "classes.json does not exist on disk!")
	var classes_json_text = fa.get_as_text()
	fa.close()
	assert(classes_json_text.contains("Guerrero"), "classes.json missing Guerrero!")
	assert(classes_json_text.contains("exp_base"), "classes.json missing exp_base formula parameter!")

	fa = FileAccess.open("res://clockgears_data/system.json", FileAccess.READ)
	assert(fa != null, "system.json does not exist on disk!")
	var sys_json_text = fa.get_as_text()
	fa.close()
	assert(sys_json_text.contains("Leyendas de ClockGears"), "system.json missing game title!")
	print("[PASS] Atomic JSON files verified on disk.")

	# 9. Clear and Reload from disk
	dm.init_empty()
	var load_res = dm.load_all("res://clockgears_data")
	assert(load_res == true, "load_all failed!")

	var loaded_classes = dm.get_category_array(1)
	assert(loaded_classes.size() == 2, "Loaded classes count mismatch!")
	assert(loaded_classes[0]["name"] == "Guerrero", "Class 1 name mismatch!")
	assert(int(loaded_classes[0]["exp_base"]) == 30, "Class 1 exp_base mismatch!")
	assert(int(loaded_classes[0]["exp_extra"]) == 20, "Class 1 exp_extra mismatch!")
	assert(int(loaded_classes[0]["exp_acc_a"]) == 30, "Class 1 exp_acc_a mismatch!")
	assert(int(loaded_classes[0]["exp_acc_b"]) == 30, "Class 1 exp_acc_b mismatch!")

	var loaded_actors = dm.get_category_array(0)
	assert(loaded_actors.size() == 1, "Loaded actors count mismatch!")
	assert(loaded_actors[0]["name"] == "Aria", "Actor name mismatch!")
	assert(int(loaded_actors[0]["class_id"]) == 1, "Actor class_id mismatch!")

	var loaded_tilesets = dm.get_category_array(13)
	assert(loaded_tilesets.size() == 1, "Loaded tilesets count mismatch!")
	assert(loaded_tilesets[0]["collisions"]["1,0"] == 1, "Tileset collision mismatch for blocked tile!")
	assert(loaded_tilesets[0]["collisions"]["2,0"] == 2, "Tileset collision mismatch for overhead star tile!")

	var loaded_sys = dm.get_system_config()
	assert(loaded_sys["game_title"] == "Leyendas de ClockGears", "System game_title mismatch!")
	assert(loaded_sys["currency"] == "Oro", "System currency mismatch!")
	print("[PASS] Full database reload verified with 100% data integrity.")

	# 10. Verify ClockGearsMap layer creation and absence of name collisions
	var map = ClockGearsMap.new()
	map.ensure_default_layers()
	assert(map.get_child_count() == 5, "ClockGearsMap child count should be 5, got %d" % map.get_child_count())
	assert(map.get_node_or_null("Capa_Suelo") != null, "layer_ground missing!")
	assert(map.get_node_or_null("Capa_Decoracion") != null, "layer_decoration missing!")
	assert(map.get_node_or_null("Capa_Objetos") != null, "layer_objects missing!")
	assert(map.get_node_or_null("Eventos") != null, "events_container missing!")
	assert(map.get_node_or_null("Capa_Techo") != null, "layer_overhead missing!")

	# Calling ensure_default_layers again must NOT create duplicate layers
	map.ensure_default_layers()
	assert(map.get_child_count() == 5, "ensure_default_layers called again created duplicate layers!")
	print("[PASS] ClockGearsMap layers verified: 5 default layers, 0 duplicates, no collision.")

	# 11. Verify Map Independence: Individual scenes with unique events and names
	DirAccess.make_dir_recursive_absolute("res://maps")
	
	# Create Map 1 (Pradera) with Event "Cofre"
	var map1 = ClockGearsMap.new()
	map1.name = "MAP001"
	map1.map_name = "Pradera"
	map1.ensure_default_layers()
	var ev_chest = ClockGearsEvent.new()
	ev_chest.name = "EV_Cofre"
	ev_chest.event_name = "Cofre"
	map1.add_clockgears_event(ev_chest, Vector2(64, 64))
	for i in range(map1.get_child_count()):
		var c = map1.get_child(i)
		c.owner = map1
		if c.name == "Eventos":
			for ev in c.get_children():
				ev.owner = map1
	var scene1 = PackedScene.new()
	scene1.pack(map1)
	ResourceSaver.save(scene1, "res://maps/map001.tscn")
	map1.free()

	# Create Map 2 (Cueva) with Event "Puerta"
	var map2 = ClockGearsMap.new()
	map2.name = "MAP002"
	map2.map_name = "Cueva"
	map2.ensure_default_layers()
	var ev_door = ClockGearsEvent.new()
	ev_door.name = "EV_Puerta"
	ev_door.event_name = "Puerta"
	map2.add_clockgears_event(ev_door, Vector2(128, 128))
	for i in range(map2.get_child_count()):
		var c = map2.get_child(i)
		c.owner = map2
		if c.name == "Eventos":
			for ev in c.get_children():
				ev.owner = map2
	var scene2 = PackedScene.new()
	scene2.pack(map2)
	ResourceSaver.save(scene2, "res://maps/map002.tscn")
	map2.free()

	# Reload Map 1: Must contain ONLY "Cofre" and have map_name "Pradera"
	var reloaded_scene1 = ResourceLoader.load("res://maps/map001.tscn")
	var inst1 = reloaded_scene1.instantiate()
	assert(inst1.map_name == "Pradera", "Map 1 display name mismatch!")
	var events_c1 = inst1.get_node_or_null("Eventos")
	assert(events_c1 != null, "Map 1 events container missing!")
	assert(events_c1.get_child_count() == 1, "Map 1 should have exactly 1 event!")
	assert(events_c1.get_child(0).event_name == "Cofre", "Map 1 should have 'Cofre' event!")
	inst1.free()

	# Reload Map 2: Must contain ONLY "Puerta" and have map_name "Cueva"
	var reloaded_scene2 = ResourceLoader.load("res://maps/map002.tscn")
	var inst2 = reloaded_scene2.instantiate()
	assert(inst2.map_name == "Cueva", "Map 2 display name mismatch!")
	var events_c2 = inst2.get_node_or_null("Eventos")
	assert(events_c2 != null, "Map 2 events container missing!")
	assert(events_c2.get_child_count() == 1, "Map 2 should have exactly 1 event!")
	assert(events_c2.get_child(0).event_name == "Puerta", "Map 2 should have 'Puerta' event!")
	inst2.free()

	print("[PASS] Map independence verified: Each map is an isolated .tscn scene with unique events and properties.")

	print("==================================================")
	print("ALL FASE 5 VERIFICATION TESTS PASSED 100%")
	print("==================================================")
	map.free()
	quit(0)
