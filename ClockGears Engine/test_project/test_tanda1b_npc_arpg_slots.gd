extends SceneTree

var passed := 0
var failed := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		printerr("[FAIL] ", msg)

func _make_png(path: String, w: int, h: int, color_fn: Callable) -> String:
	var img := Image.create(w, h, false, Image.FORMAT_RGBA8)
	for y in range(h):
		for x in range(w):
			img.set_pixel(x, y, color_fn.call(x, y))
	var abs_p := ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs_p.get_base_dir())
	img.save_png(abs_p)
	return path

func _get_anim_keys(db: Node) -> Array[String]:
	var opt: OptionButton = null
	for n in db.find_children("*", "OptionButton", true, false):
		var o := n as OptionButton
		for i in range(o.item_count):
			var meta = o.get_item_metadata(i)
			if typeof(meta) == TYPE_STRING and str(meta) == "mover_abajo":
				opt = o
				break
		if opt != null:
			break
	var res: Array[String] = []
	if opt != null:
		for i in range(opt.item_count):
			res.append(str(opt.get_item_metadata(i)))
	return res

func _initialize():
	_run_tests()

func _run_tests() -> void:
	var tmp_dir := "res://tmp_import_src/tanda1b_test"
	var npc_strip := _make_png(tmp_dir + "/npc_walk.png", 64, 32, func(x, _y):
		return Color(0.15, 0.85, 0.25, 1.0) if x < 32 else Color(0.95, 0.35, 0.15, 1.0)
	)

	var dm = ClockGearsDataManager
	dm.init_empty()

	# ---------------------------------------------------------
	# 1. DataManager: Categorías NPC + Action RPG
	# ---------------------------------------------------------
	dm.set_records("npcs", [{"id": 1, "name": "Aldeano"}])
	var npcs_loaded = dm.get_records("npcs")
	check(typeof(npcs_loaded) == TYPE_ARRAY and npcs_loaded.size() == 1 and str(npcs_loaded[0].get("name", "")) == "Aldeano", "DataManager soporta categoría 'npcs'")

	var arpg_keys := ["arpg_enemies", "arpg_npcs", "arpg_weapons", "arpg_armors", "arpg_shields", "arpg_accessories"]
	var arpg_ok := true
	for k in arpg_keys:
		dm.set_records(k, [{"id": 1, "name": "Test_" + k}])
		var got = dm.get_records(k)
		if typeof(got) != TYPE_ARRAY or got.size() != 1 or str(got[0].get("name", "")) != "Test_" + k:
			arpg_ok = false
			break
	check(arpg_ok, "DataManager soporta las 6 categorías Action RPG")

	# ---------------------------------------------------------
	# 2. InputMap por defecto: atacar (click izq) y bloquear (click der)
	# ---------------------------------------------------------
	check(InputMap.has_action("atacar"), "InputMap registra acción por defecto 'atacar'")
	check(InputMap.has_action("bloquear"), "InputMap registra acción por defecto 'bloquear'")
	var atk_has_lmb := false
	if InputMap.has_action("atacar"):
		for ev in InputMap.action_get_events("atacar"):
			if ev is InputEventMouseButton and ev.button_index == MOUSE_BUTTON_LEFT:
				atk_has_lmb = true
	var blk_has_rmb := false
	if InputMap.has_action("bloquear"):
		for ev in InputMap.action_get_events("bloquear"):
			if ev is InputEventMouseButton and ev.button_index == MOUSE_BUTTON_RIGHT:
				blk_has_rmb = true
	check(atk_has_lmb, "Acción 'atacar' asignada a botón izquierdo del ratón por defecto")
	check(blk_has_rmb, "Acción 'bloquear' asignada a botón derecho del ratón por defecto")

	# ---------------------------------------------------------
	# 3. Registry & Database UI: Capítulos, Categorías, Ranuras ARPG y Botón/Pestaña Evento
	# ---------------------------------------------------------
	var db := ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	await process_frame
	db.open_database()

	var chap_info = db.call("get_chapter_structure_info") if db.has_method("get_chapter_structure_info") else null
	check(typeof(chap_info) == TYPE_DICTIONARY and chap_info.has("chapters"), "Database expone estructura de capítulos para verificación")
	if typeof(chap_info) == TYPE_DICTIONARY and chap_info.has("chapters"):
		var chaps: Array = chap_info["chapters"]
		var combate_idx := -1
		var arpg_idx := -1
		var personajes_cats := []
		var arpg_cats := []
		for i in range(chaps.size()):
			var c: Dictionary = chaps[i]
			if str(c.get("name", "")) == "Personajes":
				personajes_cats = c.get("categories", [])
			elif str(c.get("name", "")) == "Combate":
				combate_idx = i
			elif str(c.get("name", "")) == "Action RPG":
				arpg_idx = i
				arpg_cats = c.get("categories", [])
		check(personajes_cats.has("npcs"), "Capítulo 'Personajes' incluye categoría 'npcs' junto a Actores y Clases")
		check(combate_idx >= 0 and arpg_idx == combate_idx + 1, "Capítulo 'Action RPG' está justo después de 'Combate'")
		check(arpg_cats == ["arpg_enemies", "arpg_npcs", "arpg_weapons", "arpg_armors", "arpg_shields", "arpg_accessories"], "Capítulo 'Action RPG' tiene las 6 categorías en orden exacto")
		var arpg_tooltips: Dictionary = chap_info.get("tooltips", {})
		var tt_ok := true
		for k in arpg_keys:
			if str(arpg_tooltips.get(k, "")) != "Para combate en tiempo real":
				tt_ok = false
		check(tt_ok, "Las 6 categorías de Action RPG tienen tooltip 'Para combate en tiempo real'")

	var csr = ClockGearsCharacterStateRegistry
	var has_cat_slots_api: bool = csr.has_method("get_category_slot_ids_by_key")
	check(has_cat_slots_api, "Registry expone get_category_slot_ids_by_key")
	if has_cat_slots_api:
		var slots_npc_arpg: PackedStringArray = csr.call("get_category_slot_ids_by_key", "npcs", 4, true, 1)
		check(slots_npc_arpg.has("mover_abajo") and slots_npc_arpg.has("quieto_abajo"), "NPC tiene ranuras mover_* y quieto_* cuando idle direccional está activo")
		check(not slots_npc_arpg.has("atacar_abajo") and not slots_npc_arpg.has("bloquear_abajo"), "NPC normal NO tiene ranuras de ataque ni bloqueo ni en modo ARPG")

		var slots_actor_tb: PackedStringArray = csr.call("get_category_slot_ids_by_key", "actors", 4, false, 0)
		var slots_actor_arpg: PackedStringArray = csr.call("get_category_slot_ids_by_key", "actors", 4, false, 1)
		check(not slots_actor_tb.has("atacar_abajo") and not slots_actor_tb.has("bloquear_abajo"), "Actores en modo Turnos NO muestran atacar_* ni bloquear_*")
		check(slots_actor_arpg.has("atacar_abajo") and slots_actor_arpg.has("bloquear_abajo"), "Actores en modo ARPG muestran atacar_* y bloquear_*")

		var slots_arpg_en: PackedStringArray = csr.call("get_category_slot_ids_by_key", "arpg_enemies", 4, false, 1)
		check(slots_arpg_en.has("atacar_abajo") and not slots_arpg_en.has("bloquear_abajo"), "Enemigos ARPG tienen solo atacar_* y NO bloquear_*")

		var slots_arpg_wpn: PackedStringArray = csr.call("get_category_slot_ids_by_key", "arpg_weapons", 4, false, 1)
		check(slots_arpg_wpn.has("atacar_abajo") and slots_arpg_wpn.has("bloquear_abajo"), "Armas ARPG tienen atacar_* y bloquear_*")

	# Persistencia de frames al cambiar Turnos <-> ARPG
	var actor_id: int = dm.create_record(0, {
		"name": "Heroe",
		"sprite_data": {
			"cols": 2,
			"rows": 1,
			"animations": {
				"atacar_abajo": {
					"name": "atacar_abajo",
					"spritesheet_path": npc_strip,
					"fps": 10.0,
					"loop": false,
					"frames": [0, 1]
				}
			}
		}
	})
	var sys_cfg: Dictionary = dm.get_system_config()
	sys_cfg["combat_mode"] = 0
	dm.set_system_config(sys_cfg)
	if db.has_method("select_category_by_key"):
		db.call("select_category_by_key", "actors")
	else:
		db.select_category(0)
	var tb_keys := _get_anim_keys(db)
	check(not tb_keys.has("atacar_abajo"), "UI oculta atacar_abajo en Actores al cambiar Sistema de Combate a Turnos")

	sys_cfg["combat_mode"] = 1
	dm.set_system_config(sys_cfg)
	if db.has_method("select_category_by_key"):
		db.call("select_category_by_key", "actors")
	else:
		db.select_category(0)
	var arpg_keys_ui := _get_anim_keys(db)
	check(arpg_keys_ui.has("atacar_abajo"), "UI restaura atacar_abajo al volver a Action RPG")
	var act_after: Dictionary = dm.get_record_by_id(0, actor_id)
	var saved_atk_frames: Array = act_after.get("sprite_data", {}).get("animations", {}).get("atacar_abajo", {}).get("frames", [])
	check(saved_atk_frames.size() == 2, "Los datos de atacar_abajo no se borran al alternar Turnos <-> ARPG")

	# Guardar entrada NPC con sprite y páginas de evento compartidas
	var npc_shared_pages := [
		{
			"name": "P1",
			"trigger": 0,
			"priority": 1,
			"through": false,
			"direction_fix": false,
			"move_type": 0,
			"move_speed": 3,
			"move_frequency": 3,
			"commands": [
				{"code": 101, "type": "message", "text": "Hola desde BD compartida"}
			]
		}
	]
	dm.set_records("npcs", [{
		"id": 1,
		"name": "Tendero",
		"desc": "",
		"directional_idle": false,
		"sprite_data": {
			"cols": 2,
			"rows": 1,
			"animations": {
				"mover_abajo": {
					"name": "mover_abajo",
					"spritesheet_path": npc_strip,
					"fps": 6.0,
					"loop": true,
					"frames": [0, 1]
				}
			}
		},
		"event_pages": npc_shared_pages
	}])
	if db.has_method("select_category_by_key"):
		db.call("select_category_by_key", "npcs")
	var npc_icon = db.call("get_record_list_icon", 0) if db.has_method("get_record_list_icon") else null
	check(npc_icon != null and npc_icon.get_width() > 0, "NPC lista muestra textura de icono desde mover_abajo")
	check(db.has_method("open_record_event_editor") and db.call("open_record_event_editor"), "Pestaña 'Evento' en NPC de BD abre el editor de eventos")
	db.hide()
	await process_frame

	# Verificar Evento en mapa con Comportamiento = NPC + herencia de sprite/páginas + selector oculto + marcador '?' al borrar
	var map := ClockGearsMap.new()
	map.set_name("TestMap1B")
	map.set_map_size(Vector2i(12, 12))
	map.ensure_default_layers()
	var ev_npc := ClockGearsEvent.new()
	ev_npc.set_event_id(1)
	ev_npc.set_direction(2)
	if ev_npc.has_method("set_behavior_type"):
		ev_npc.call("set_behavior_type", 1) # NPC
		ev_npc.call("set_behavior_ref_id", 1)
	map.add_clockgears_event(ev_npc, Vector2(4 * 32, 3 * 32))

	var ed := ClockGearsEventEditorDialog.new()
	get_root().add_child(ed)
	await process_frame
	ed.edit_event(ev_npc)
	var ed_spr_hidden := (not bool(ed.call("is_sprite_selector_visible"))) if ed.has_method("is_sprite_selector_visible") else false
	check(ed_spr_hidden, "Selector de sprite en Evento del mapa queda oculto cuando Comportamiento != Normal")

	var db_ev_spr_hidden := false
	if ed.has_method("edit_database_event"):
		ed.call("edit_database_event", "npcs", 1, "Tendero", npc_shared_pages, Callable())
		db_ev_spr_hidden = not bool(ed.call("is_sprite_selector_visible"))
	check(db_ev_spr_hidden, "Editor de Evento desde BD también oculta el selector de sprite")
	ed.hide()
	await process_frame

	# Runtime + Interacción compartida y bloqueo de mouse ARPG cuando hay ventana de mensaje abierta
	get_root().add_child(map)
	var player := ClockGearsPlayer.new()
	var interp := ClockGearsEventInterpreter.new()
	map.add_child(player)
	map.add_child(interp)
	interp.setup_interpreter(map, player, null)
	player.setup_on_map(map, null, interp)
	player.set_grid_cell_immediate(Vector2i(4, 4), 8) # mirando arriba hacia ev_npc (4,3)

	sys_cfg["combat_mode"] = 1
	dm.set_system_config(sys_cfg)
	var arpg_allowed_idle := bool(player.call("is_arpg_action_allowed")) if player.has_method("is_arpg_action_allowed") else false
	check(arpg_allowed_idle, "En modo ARPG sin ventanas abiertas, 'atacar'/'bloquear' están permitidos")

	var qtex = csr.call("get_missing_question_marker_texture") if csr.has_method("get_missing_question_marker_texture") else null
	var ev_spr: Sprite2D = ev_npc.get_node_or_null("EventSprite") as Sprite2D
	if ev_spr == null:
		for c in ev_npc.get_children():
			if c is Sprite2D:
				ev_spr = c as Sprite2D
				break
	var ev_spr_ok := false
	if ev_spr != null and ev_spr.texture != null and ev_spr.texture != qtex:
		var ev_img := ev_spr.texture.get_image()
		if ev_img != null and ev_spr.texture.get_width() == 64 and ev_spr.region_enabled and ev_spr.region_rect.size == Vector2(32, 32):
			var px := ev_img.get_pixel(0, 0)
			ev_spr_ok = (px.g > 0.7 and px.r < 0.3)
	check(ev_spr_ok, "Evento con comportamiento NPC toma sprite de las ranuras de la entrada BD")

	ev_npc.update_active_page()
	var act_cmds: Array = ev_npc.get_active_commands()
	check(act_cmds.size() == 1 and str(act_cmds[0].get("text", "")) == "Hola desde BD compartida", "Evento con comportamiento NPC hereda las páginas de evento compartidas de la BD")

	# Dos eventos de mapa compartiendo la misma entrada NPC
	var ev_npc_2 := ClockGearsEvent.new()
	ev_npc_2.set_event_id(2)
	ev_npc_2.set_direction(6)
	ev_npc_2.set_move_type(1)
	ev_npc_2.set_move_speed(160.0)
	ev_npc_2.set_move_frequency(4)
	ev_npc_2.call("set_behavior_type", 1)
	ev_npc_2.call("set_behavior_ref_id", 1)
	map.add_clockgears_event(ev_npc_2, Vector2(6 * 32, 3 * 32))
	ev_npc_2.update_active_page()
	check(ev_npc_2.get_active_commands().size() == 1 and ev_npc_2.get_direction() == 6 and ev_npc.get_direction() == 2 and ev_npc_2.get_move_type() == 1 and ev_npc.get_move_type() == 0, "Dos eventos compartiendo el mismo NPC conservan dirección y movimiento propios")

	# Evento legado Normal con hoja 4x4 sigue funcionando intacto
	var legacy_sheet := _make_png(tmp_dir + "/legacy_4x4.png", 128, 128, func(_x, _y): return Color(0.3, 0.5, 0.9, 1.0))
	var ev_norm := ClockGearsEvent.new()
	ev_norm.set_event_id(3)
	ev_norm.set_sprite_hframes(4)
	ev_norm.set_sprite_vframes(4)
	ev_norm.set_graphic_path(legacy_sheet)
	ev_norm.set_direction(4)
	check(ev_norm.get_behavior_type() == 0 and ev_norm.get_sprite_frame() == 4, "Evento legado 'Normal' con hoja 4x4 mantiene funcionamiento intacto")
	ev_norm.queue_free()

	check(player.try_activate_front_event(), "Jugador puede interactuar con evento de comportamiento NPC")
	check(interp.is_blocking_player(), "Intérprete ejecuta la página compartida y abre ventana de mensaje")
	var arpg_blocked_by_msg := (not bool(player.call("is_arpg_action_allowed"))) if player.has_method("is_arpg_action_allowed") else false
	check(arpg_blocked_by_msg, "Con ventana de mensaje abierta, los clics NO disparan atacar/bloquear")

	interp.advance_message_or_choice(0)
	interp.step_frame()
	sys_cfg["combat_mode"] = 0
	dm.set_system_config(sys_cfg)
	var arpg_blocked_in_tb := (not bool(player.call("is_arpg_action_allowed"))) if player.has_method("is_arpg_action_allowed") else false
	check(arpg_blocked_in_tb, "En modo por turnos, 'atacar'/'bloquear' no están activos")

	# Borrar entrada NPC en BD -> evento muestra marcador '?' sin romper
	dm.set_records("npcs", [])
	ev_npc.update_active_page()
	var missing_spr: Sprite2D = ev_spr
	check(missing_spr != null and qtex != null and missing_spr.texture == qtex, "Si se borra la entrada de la BD, el evento muestra marcador '?' sin error ni crash")

	db.hide()
	db.queue_free()
	ed.hide()
	ed.queue_free()
	map.queue_free()
	await process_frame
	await process_frame

	print("TANDA 1B SUMMARY: %d passed, %d failed" % [passed, failed])
	quit(1 if failed > 0 else 0)
