extends SceneTree

var passed: int = 0
var failed: int = 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _remove_dir_recursive_abs(abs_dir: String) -> void:
	var da := DirAccess.open(abs_dir)
	if da == null:
		return
	da.list_dir_begin()
	var name := da.get_next()
	while name != "":
		if name != "." and name != "..":
			var p := abs_dir.path_join(name)
			if da.current_is_dir():
				_remove_dir_recursive_abs(p)
				DirAccess.remove_absolute(p)
			else:
				DirAccess.remove_absolute(p)
		name = da.get_next()
	da.list_dir_end()
	DirAccess.remove_absolute(abs_dir)

func _run_tests() -> void:
	print("=== TEST 9B TANDA 3: MENÚ DEL JUEGO, INVENTARIO Y EQUIPO ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var wsm = Engine.get_singleton("ClockGearsWindowStyleManager")
	check(dm != null, "ClockGearsDataManager disponible")
	check(wsm != null, "ClockGearsWindowStyleManager disponible")
	if dm == null:
		return

	var tmp_root := "user://test_9b3_root"
	var data_dir := tmp_root.path_join("data")
	var legacy_dir := tmp_root.path_join("legacy_data")
	var saves_dir := tmp_root.path_join("saves")
	var opts_path := tmp_root.path_join("options.json")
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(tmp_root))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(data_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(legacy_dir))
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(saves_dir))

	dm.init_empty()
	dm.set_save_directory(saves_dir)
	dm.set_player_options_path(opts_path)

	# 1. Sistema, Clases, Estados, Objetos, Armas, Armaduras, Accesorios, Actores
	dm.set_currency_unit("Dracmas")
	var sys: Dictionary = dm.get_system_config()
	sys["party"] = [1, 2]
	sys["combat_mode"] = 0
	sys["default_max_item_stack"] = 99
	dm.set_system_config(sys)
	check(dm.get_default_max_item_stack() == 99, "Límite por defecto en Sistema = 99")

	# Clases (CAT_CLASSES = 1)
	dm.set_category_array(1, [
		{
			"id": 1,
			"name": "Guerrero",
			"exp_base": 30,
			"exp_extra": 20,
			"exp_acc_a": 30,
			"exp_acc_b": 30,
			"traits": [
				{"code": 51, "data_id": 1, "value": 1.0}, # CG_TRAIT_EQUIP_WTYPE: 1=Espadas
				{"code": 52, "data_id": 1, "value": 1.0}, # CG_TRAIT_EQUIP_ATYPE: 1=Ligera
				{"code": 52, "data_id": 2, "value": 1.0}, # 2=Escudo
				{"code": 11, "data_id": 1, "value": 150.0} # CG_TRAIT_ELEMENT_RATE: 150%
			]
		}
	])

	# Estados (CAT_STATES = 9)
	dm.set_category_array(9, [
		{"id": 1, "name": "Veneno"},
		{"id": 2, "name": "Bendición"}
	])

	# Objetos (CAT_ITEMS = 3): Poción curación, Éter MP, Elixir estado, Pan stack custom=5, Llave Maestra (Key item), Bomba solo batalla
	dm.set_category_array(3, [
		{
			"id": 1, "name": "Poción", "desc": "Recupera 60 HP",
			"type": 0, "secondary_type": 7, "tertiary_type": 0, "consumable": true,
			"effects": [{"code": 11, "data_id": 0, "value1": 0.0, "value2": 60.0}]
		},
		{
			"id": 2, "name": "Éter", "desc": "Recupera 25 MP y quita Veneno",
			"type": 0, "secondary_type": 7, "tertiary_type": 2, "consumable": true,
			"effects": [
				{"code": 12, "data_id": 0, "value1": 0.0, "value2": 25.0},
				{"code": 22, "data_id": 1, "value1": 1.0, "value2": 0.0}
			]
		},
		{
			"id": 3, "name": "Pan", "desc": "Max stack 5",
			"type": 0, "secondary_type": 7, "tertiary_type": 0, "consumable": true, "max_stack": 5,
			"effects": [{"code": 21, "data_id": 2, "value1": 1.0, "value2": 0.0}]
		},
		{
			"id": 4, "name": "Llave Antigua", "desc": "Objeto clave no descartable",
			"type": 1, "secondary_type": 0, "tertiary_type": 3, "consumable": false,
			"effects": []
		},
		{
			"id": 5, "name": "Bomba Humo", "desc": "Solo batalla",
			"type": 0, "secondary_type": 1, "tertiary_type": 1, "consumable": true,
			"effects": []
		}
	])

	# Armas (CAT_WEAPONS = 4)
	dm.set_category_array(4, [
		{"id": 1, "name": "Espada Hierro", "type": 1, "atk": 18, "agi": 2},
		{"id": 2, "name": "Hacha Pesada", "type": 2, "atk": 35}
	])

	# Armaduras (CAT_ARMORS = 5)
	dm.set_category_array(5, [
		{"id": 1, "name": "Escudo Madera", "type": 2, "secondary_type": 1, "def": 8},
		{"id": 2, "name": "Túnica Ligera", "type": 1, "secondary_type": 3, "def": 12, "hp": 30},
		{"id": 3, "name": "Coraza Mágica", "type": 3, "secondary_type": 3, "def": 25}
	])

	# Accesorios ARPG (CAT_ARPG_ACCESSORIES = 25) + Tropas batalla prueba
	dm.set_category_array(25, [
		{"id": 1, "name": "Amuleto Suerte", "luk": 10, "mat": 5}
	])
	dm.set_category_array(7, [{"id": 1, "name": "Slime", "hp": 30, "atk": 5}])
	dm.set_category_array(8, [{"id": 1, "name": "Tropa 1", "members": [{"enemy_id": 1, "x": 400, "y": 240}]}])

	# Actores (CAT_ACTORS = 0)
	dm.set_category_array(0, [
		{"id": 1, "name": "Alden", "class_id": 1, "hp": 120, "mp": 40, "atk": 15, "def": 10, "mat": 8, "mdf": 8, "agi": 10, "luk": 5},
		{"id": 2, "name": "Lyra", "class_id": 1, "hp": 100, "mp": 60, "atk": 10, "def": 8, "mat": 16, "mdf": 12, "agi": 12, "luk": 9,
			"traits": [{"code": 53, "data_id": 2, "value": 1.0}]} # Bloquea hueco 2 (Cabeza)
	])

	dm.reset_runtime_state()
	dm.set_gold(340)
	dm.set_actor_runtime_hp(1, 50)
	dm.set_actor_runtime_mp(1, 10)
	dm.add_actor_runtime_state(1, 1) # Veneno
	dm.change_item("Poción", 3)
	dm.change_item("Éter", 2)
	dm.change_item("Pan", 10) # Debe capar a max_stack = 5
	dm.change_item("Llave Antigua", 1)
	dm.change_item("Bomba Humo", 2)
	dm.change_item("Espada Hierro", 1)
	dm.change_item("Hacha Pesada", 1)
	dm.change_item("Escudo Madera", 1)
	dm.change_item("Túnica Ligera", 1)
	dm.change_item("Coraza Mágica", 1)
	dm.change_item("Amuleto Suerte", 1)

	check(dm.get_item_qty("Pan") == 5, "Límite propio de objeto (max_stack=5) limita inventario")
	check(dm.is_key_item("Llave Antigua"), "Objeto tipo 1 reconocido como objeto clave")
	check(not dm.can_discard_item("Llave Antigua"), "can_discard_item bloquea objeto clave")
	check(not dm.discard_item("Llave Antigua", 1), "discard_item rechazado para objeto clave")
	check(dm.get_item_qty("Llave Antigua") == 1, "Cantidad de objeto clave intacta")

	# 2. Instanciar mapa + runtime UI (ClockGearsMap + Interpreter + GameMenu)
	var gm_root := Node.new()
	root.add_child(gm_root)
	var m := ClockGearsMap.new()
	m.set_name("MapaPrueba9B3")
	m.set_map_name("Villa del Reloj")
	m.set_map_size(Vector2i(16, 12))
	m.set_grid_resolution(32)
	gm_root.add_child(m)
	m.init_runtime_session()
	await process_frame

	var interp := m.get_runtime_interpreter()
	var player := m.get_runtime_player()
	check(interp != null and player != null, "Mapa inicializa intérprete y jugador runtime")
	var gmenu = interp.get_game_menu()
	check(gmenu != null, "ClockGearsGameMenu conectado al intérprete")
	check(gmenu.can_open_menu(), "Se puede abrir el menú sobre el mapa en reposo")

	# Bloqueo con ventana de mensajes abierta
	var mw := interp.get_message_window()
	mw.show_message("Hola jugador")
	check(not gmenu.can_open_menu(), "Menú bloqueado con ventana de mensajes abierta")
	check(not gmenu.open_menu(), "open_menu falla mientras hay mensaje abierto")
	mw.close_window()

	# Bloqueo con evento/batalla en curso
	interp.start_command_list([{"type": "wait", "frames": 30}])
	check(not gmenu.can_open_menu(), "Menú bloqueado mientras un evento corre (o espera batalla)")
	for _i in range(32):
		interp.step_frame()
	check(gmenu.can_open_menu(), "Menú vuelve a estar disponible tras terminar evento")

	# Bloqueo con escena de batalla simulada / estado batalla
	interp.start_command_list([{"type": "battle", "troop_id": 1}])
	check(interp.has_active_battle(), "Intérprete entra en estado de batalla")
	check(not gmenu.open_menu(), "Menú rechazado en batalla")
	var b_children := interp.find_children("*", "ClockGearsBattleScene", true, false)
	for bc in b_children:
		bc.emit_signal("battle_finished", 0)
	await process_frame
	check(not interp.has_active_battle(), "Batalla terminada correctamente")

	# Apertura con Esc real sobre el mapa (via SceneTree push_input)
	var esc_ev := InputEventKey.new()
	esc_ev.keycode = KEY_ESCAPE
	esc_ev.pressed = true
	root.push_input(esc_ev)
	await process_frame
	check(gmenu.is_menu_open(), "Esc abre el menú sobre el mapa")
	check(interp.is_blocking_player(), "Menú abierto bloquea movimiento del jugador")

	# Verificar entradas por defecto y Magia oculta
	var vis_ids: PackedStringArray = gmenu.get_visible_entry_ids()
	check(not vis_ids.has("magic"), "Entrada reservada 'Magia' oculta por defecto")
	check(vis_ids.has("items") and vis_ids.has("equip") and vis_ids.has("status") and vis_ids.has("save") and vis_ids.has("options") and vis_ids.has("to_title"), "Las 6 entradas activas aparecen visibles")
	check(gmenu.get_displayed_gold_text() == "340 Dracmas", "Unidad monetaria mostrada junto al oro: " + gmenu.get_displayed_gold_text())

	# Uso de objetos desde menú (Poción + Éter + Pan + Bomba)
	gmenu.open_subscreen("items")
	gmenu.select_item("Poción")
	check(gmenu.use_selected_item(1), "Usar Poción fuera de combate")
	check(dm.get_actor_runtime_hp(1) == 110, "Poción curó HP (50 -> 110)")
	check(dm.get_item_qty("Poción") == 2, "Poción consumida (3 -> 2)")

	gmenu.select_item("Éter")
	check(gmenu.use_selected_item(1), "Usar Éter (solo menú)")
	check(dm.get_actor_runtime_mp(1) == 35, "Éter curó MP (10 -> 35)")
	check(not dm.get_actor_runtime_states(1).has(1), "Éter curó estado Veneno")

	check(not dm.use_item_on_actor("Bomba Humo", 1), "Objeto Solo Batalla no se puede usar en menú")
	check(dm.discard_item("Poción", 1) and dm.get_item_qty("Poción") == 1, "Tirar objeto regular descuenta inventario")

	# 3. Equipo en Modo Por Turnos (combat_mode = 0)
	gmenu.open_subscreen("equip")
	gmenu.select_equip_actor(1)
	gmenu.select_equip_slot(0) # Mano Derecha
	var cands0: Array = dm.get_equippable_pieces_for_slot(1, 0, 0)
	var has_sword := false
	var has_axe := false
	for cr in cands0:
		if String(cr.get("key", "")) == "Espada Hierro":
			has_sword = true
		if String(cr.get("key", "")) == "Hacha Pesada":
			has_axe = true
	check(has_sword and not has_axe, "Rasgos equipables filtran arma permitida (Espada sí, Hacha no)")

	var prev0: Dictionary = dm.preview_equip_actor_stats(1, 0, 4, 1, 0)
	check(int(prev0["before"]["atk"]) == 15 and int(prev0["after"]["atk"]) == 33 and int(prev0["diff"]["atk"]) == 18,
		"Vista previa antes/después predice +18 ATQ")
	check(gmenu.select_equip_candidate("Espada Hierro") and gmenu.equip_selected_piece(), "Equipar Espada Hierro en modo turnos")
	check(dm.get_item_qty("Espada Hierro") == 0, "Pieza equipada se descuenta del inventario")
	check(int(dm.get_actor_effective_stats(1, 0)["atk"]) == 33, "ATQ efectivo actualizado tras equipar")
	check(gmenu.unequip_selected_slot(), "Desequipar devuelve la pieza al inventario")
	check(dm.get_item_qty("Espada Hierro") == 1, "Espada Hierro devuelta al inventario al desequipar")
	check(int(dm.get_actor_effective_stats(1, 0)["atk"]) == 15, "ATQ vuelve a 15 tras desequipar")

	# Rasgo de bloqueo de hueco en actor 2 (cabeza = slot 2 bloqueado)
	check(dm.is_actor_equip_slot_locked(2, 2, 0), "Rasgo EQUIP_LOCK bloquea hueco 2 en Actor 2")
	# Re-equipar Espada + Escudo para persistencia en guardado
	dm.equip_actor_piece(1, 0, "Espada Hierro", 0)
	dm.equip_actor_piece(1, 1, "Escudo Madera", 0)

	# 4. Equipo en Modo ARPG (5 huecos: combat_mode = 1)
	sys["combat_mode"] = 1
	dm.set_system_config(sys)
	var arpg_slots: Array = dm.get_actor_equip_slots(1, 1)
	check(arpg_slots.size() == 5, "Modo ARPG expone exactamente 5 huecos")
	check(dm.equip_actor_piece(1, 3, "Amuleto Suerte", 1), "Equipar accesorio en hueco 3 ARPG (Accesorio 1)")
	check(dm.get_item_qty("Amuleto Suerte") == 0, "Accesorio ARPG consumido del inventario")
	var eff_arpg: Dictionary = dm.get_actor_effective_stats(1, 1)
	check(int(eff_arpg["luk"]) == 15, "LUK efectiva en ARPG refleja Amuleto (+10 -> 15)")

	# 5. Estado completo
	gmenu.open_subscreen("items")
	gmenu.select_item("Pan")
	gmenu.use_selected_item(1) # Aplica estado Bendición (id=2)
	gmenu.open_subscreen("status")
	gmenu.select_status_actor(1)
	var sheet: Dictionary = dm.get_actor_status_sheet(1)
	check(String(sheet.get("name", "")) == "Alden" and String(sheet.get("class_name", "")) == "Guerrero", "Estado muestra nombre y clase")
	var summary := gmenu.get_status_summary_text()
	check(summary.contains("Bendición"), "Estado muestra estados activos (Bendición)")
	check(summary.contains("150%"), "Estado muestra afinidades elementales (150% [Débil])")

	# 6. Guardar y Cargar conserva inventario y equipo (normal + ARPG)
	gmenu.open_subscreen("save")
	check(gmenu.save_to_slot(2), "Guardar partida desde menú en Espacio 2")
	# Mutar estado y cargar para verificar roundtrip real
	dm.set_gold(0)
	dm.change_item("Poción", 10)
	dm.unequip_actor_slot(1, 0, 0)
	dm.unequip_actor_slot(1, 3, 1)
	check(dm.load_game_slot(2), "Cargar partida desde Espacio 2")
	check(dm.get_gold() == 340, "Carga restaura oro")
	check(dm.get_item_qty("Poción") == 1, "Carga restaura inventario exacto")
	check(int(dm.get_actor_equip_slots(1, 0)[0]["piece_id"]) == 1, "Carga restaura equipo por turnos")
	check(int(dm.get_actor_equip_slots(1, 1)[3]["piece_id"]) == 1, "Carga restaura equipo ARPG")

	# 7. Entradas desactivadas / renombradas / reordenadas persisten en game_menu.json
	var gm_cfg: Dictionary = dm.get_game_menu_config()
	var ents: Array = gm_cfg["entries"]
	for i in range(ents.size()):
		if String(ents[i]["id"]) == "items":
			ents[i]["text"] = "Mochila"
		elif String(ents[i]["id"]) == "save":
			ents[i]["enabled"] = false
	# Mover 'options' primero
	for i in range(ents.size()):
		if String(ents[i]["id"]) == "options":
			var opt_entry = ents[i]
			ents.remove_at(i)
			ents.insert(0, opt_entry)
			break
	gm_cfg["entries"] = ents
	dm.set_game_menu_config(gm_cfg)
	check(dm.save_all(data_dir), "save_all persiste game_menu.json")
	check(FileAccess.file_exists(data_dir.path_join("game_menu.json")), "Archivo game_menu.json existe en disco")

	dm.init_empty()
	dm.set_save_directory(saves_dir)
	dm.set_player_options_path(opts_path)
	gmenu.close_menu()
	check(dm.load_all(data_dir), "load_all recarga game_menu.json")
	gmenu.open_menu()
	var r_ids: PackedStringArray = gmenu.get_visible_entry_ids()
	var r_txts: PackedStringArray = gmenu.get_visible_entry_texts()
	check(r_ids.size() > 0 and r_ids[0] == "options", "Reordenación de entradas persiste ('options' primero)")
	check(not r_ids.has("save"), "Entrada desactivada ('save') persiste oculta")
	check(r_txts.has("Mochila"), "Renombrado ('Objetos' -> 'Mochila') persiste")

	# 8. Opciones, Estilo game_menu y Regla 16:9
	check(gmenu.open_subscreen("options") and gmenu.get_current_subscreen() == "options", "Subpantalla Opciones abre dentro del menú")
	var opt_box := gmenu.find_child("GameMenuOptionsBox", true, false)
	check(opt_box != null, "Widget ClockGearsOptionsBox reutilizado dentro del menú")
	if opt_box != null:
		opt_box.apply_values(0.45, 0.65, 1.1, false)
		var applied: Dictionary = dm.get_applied_player_options_state()
		check(absf(float(applied.get("music_volume", 0.0)) - 0.45) < 0.01, "Opciones desde menú aplican volumen música en vivo")
	gmenu.open_subscreen("main")

	check(wsm != null, "Estilos activos para menú")
	var reg_types: Array = dm.get_registered_window_types()
	var has_gm_type := false
	for rt in reg_types:
		if String(rt.get("id", "")) == "game_menu":
			has_gm_type = true
	check(has_gm_type, "Tipo de ventana 'game_menu' registrado en sistema de estilos")

	for vp in [Vector2(960, 540), Vector2(1366, 768), Vector2(1920, 1080)]:
		var bi: Dictionary = gmenu.get_layout_bounds_info(vp)
		check(not bool(bi.get("clips", true)) and not bool(bi.get("overlaps", true)),
			"Layout menú 16:9 sin cortes ni solapamientos en %dx%d" % [int(vp.x), int(vp.y)])

	# 9. Proyecto antiguo sin game_menu.json ni default_max_item_stack abre sin errores
	var f_db := FileAccess.open(legacy_dir.path_join("database.json"), FileAccess.WRITE)
	f_db.store_string("{\"engine\": \"ClockGears Engine\", \"schema_version\": 1}")
	f_db.close()
	var f_sys := FileAccess.open(legacy_dir.path_join("system.json"), FileAccess.WRITE)
	f_sys.store_string("{\"game_title\": \"Legacy\", \"party\": [1]}")
	f_sys.close()
	var f_act := FileAccess.open(legacy_dir.path_join("actors.json"), FileAccess.WRITE)
	f_act.store_string("[{\"id\": 1, \"name\": \"Hero\", \"hp\": 100}]")
	f_act.close()
	dm.init_empty()
	check(dm.load_all(legacy_dir), "Proyecto antiguo sin game_menu.json carga limpiamente")
	check(dm.get_default_max_item_stack() == 99, "Proyecto antiguo migra default_max_item_stack=99")
	check(gmenu.open_menu(), "Menú abre correctamente sobre proyecto antiguo migrado")

	# 10. Confirmación Salir al título
	var title_existed_before := FileAccess.file_exists("res://title_screen.tscn")
	check(gmenu.open_subscreen("to_title") and gmenu.get_current_subscreen() == "to_title", "Subpantalla confirmación Salir al título abre")
	gmenu.confirm_return_to_title()
	check(gmenu.was_return_to_title_triggered(), "Confirmar Salir al título dispara retorno al título")
	check(not gmenu.is_menu_open(), "Confirmar Salir al título cierra el menú")
	await process_frame
	if not title_existed_before and FileAccess.file_exists("res://title_screen.tscn"):
		DirAccess.remove_absolute(ProjectSettings.globalize_path("res://title_screen.tscn"))

	# Limpieza
	gm_root.queue_free()
	await process_frame
	_remove_dir_recursive_abs(ProjectSettings.globalize_path(tmp_root))

	print("RESUMEN TEST 9B3: %d PASS, %d FAIL" % [passed, failed])
	quit(0 if failed == 0 else 1)

func _initialize() -> void:
	await process_frame
	await _run_tests()
