extends SceneTree

var dm
var passes := 0
var fails := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passes += 1
		print("PASS: " + msg)
	else:
		fails += 1
		printerr("FAIL: " + msg)

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		check(false, "ClockGearsDataManager singleton missing")
		_finish()
		return
	dm.load_all("res://clockgears_data")

	var had_custom_items: bool = dm.has_custom_ui_design("game_menu_items")
	var orig_items_design: Dictionary = dm.get_ui_screen_design("game_menu_items")
	var had_custom_equip: bool = dm.has_custom_ui_design("game_menu_equip")
	var orig_equip_design: Dictionary = dm.get_ui_screen_design("game_menu_equip")
	var had_custom_status: bool = dm.has_custom_ui_design("game_menu_status")
	var orig_status_design: Dictionary = dm.get_ui_screen_design("game_menu_status")
	var had_custom_save: bool = dm.has_custom_ui_design("game_menu_save")
	var orig_save_design: Dictionary = dm.get_ui_screen_design("game_menu_save")
	var orig_rt = dm.export_runtime_state()
	var orig_cat_items: Array = dm.get_category_array(3).duplicate(true)
	var orig_cat_weapons: Array = dm.get_category_array(4).duplicate(true)

	# Asegurar objetos/armas reales de prueba con efecto usable (sin guardar en disco)
	dm.set_category_array(3, [
		{"id": 101, "name": "Poción de prueba", "desc": "Cura 50 HP", "type": 0, "consumable": true, "secondary_type": 7, "tertiary_type": 0, "price": 50, "effects": [{"code": 11, "value1": 0.0, "value2": 50.0}]}
	])
	dm.set_category_array(4, [
		{"id": 201, "name": "Espada de prueba", "desc": "Arma de prueba", "atk": 12, "price": 120, "type": 0}
	])

	await _test_punto_1()
	await _test_punto_2()

	# Restaurar estado original del proyecto de pruebas para no ensuciarlo
	dm.set_category_array(3, orig_cat_items)
	dm.set_category_array(4, orig_cat_weapons)
	dm.import_runtime_state(orig_rt)
	dm.set_editor_test_data_count(-1)
	if had_custom_items:
		dm.set_ui_screen_design("game_menu_items", orig_items_design, false)
	else:
		dm.reset_ui_screen_design_to_default("game_menu_items", false)
	if had_custom_equip:
		dm.set_ui_screen_design("game_menu_equip", orig_equip_design, false)
	else:
		dm.reset_ui_screen_design_to_default("game_menu_equip", false)
	if had_custom_status:
		dm.set_ui_screen_design("game_menu_status", orig_status_design, false)
	else:
		dm.reset_ui_screen_design_to_default("game_menu_status", false)
	if had_custom_save:
		dm.set_ui_screen_design("game_menu_save", orig_save_design, false)
	else:
		dm.reset_ui_screen_design_to_default("game_menu_save", false)

	_finish()

func _test_punto_1() -> void:
	print("--- Punto 1: Contenedores con datos reales ---")
	var dsrcs: Array = dm.get_ui_container_data_sources()
	var ds_ids: Array = []
	for ds in dsrcs:
		ds_ids.append(str(ds.get("id", "")))
	for req in ["manual", "inventory_all", "inventory_consumables", "inventory_key_materials", "inventory_weapons", "inventory_armors", "inventory_shields", "inventory_accessories", "equip_compatible", "equip_slots", "party_members", "actor_skills", "actor_states", "save_slots", "battle_commands"]:
		check(ds_ids.has(req), "Fuente de datos registrada: " + req)

	var ed = ClassDB.instantiate("ClockGearsUiDesignEditor")
	get_root().add_child(ed)
	ed.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	ed.set_size(Vector2(1366, 768))
	await process_frame
	await process_frame

	ed.select_screen("game_menu_items")
	await process_frame

	var handles: Dictionary = ed.get_ui_handles()
	var test_data_opt: OptionButton = handles.get("test_data_opt")
	check(test_data_opt != null and test_data_opt.get_item_count() >= 4, "Selector Datos de prueba visible con 0/8/40/vivo")

	var cid: String = ed.add_element_at("container", Vector2i(40, 80))
	check(not cid.is_empty(), "Añadido elemento container desde paleta")
	ed.select_element(cid)
	await process_frame

	var cel: Dictionary = dm.get_ui_element("game_menu_items", cid)
	cel["data_source"] = "inventory_all"
	cel["layout_mode"] = "grid"
	cel["layout_columns"] = 2
	cel["layout_rows"] = 3
	cel["width"] = 420
	cel["height"] = 220
	cel["layout_scroll"] = true
	cel["data_show_empty"] = true
	cel["data_empty_count"] = 8
	cel["slot_action"] = "show_detail"
	dm.update_ui_element("game_menu_items", cid, cel, true)
	ed.refresh_from_data_manager()
	await process_frame

	# Botón "Crear plantilla del elemento"
	var h2: Dictionary = ed.get_ui_handles()
	var tpl_btn: Button = h2.get("prop_create_item_tpl_btn")
	check(tpl_btn != null, "Existe botón 'Crear plantilla del elemento'")
	if tpl_btn:
		tpl_btn.emit_signal("pressed")
		await process_frame
	var updated_cel: Dictionary = dm.get_ui_element("game_menu_items", cid)
	check(not str(updated_cel.get("item_template_folder_id", "")).is_empty(), "Carpeta repetible 'Plantilla del elemento' creada e id asignado")

	# Añadir icono rotable y texto enlazado para foco
	var prev_icon_id: String = ed.add_element_at("image", Vector2i(520, 90))
	var icon_el: Dictionary = dm.get_ui_element("game_menu_items", prev_icon_id)
	icon_el["data_binding"] = "item_icon"
	icon_el["rotatable"] = true
	icon_el["width"] = 96
	icon_el["height"] = 96
	dm.update_ui_element("game_menu_items", prev_icon_id, icon_el, true)

	var prev_name_id: String = ed.add_element_at("text", Vector2i(520, 200))
	var name_el: Dictionary = dm.get_ui_element("game_menu_items", prev_name_id)
	name_el["data_binding"] = "item_name"
	dm.update_ui_element("game_menu_items", prev_name_id, name_el, true)
	ed.refresh_from_data_manager()
	await process_frame

	var dl = ed.get_canvas_design_layer()
	# Probar datos de prueba: 0
	dm.set_editor_test_data_count(0)
	ed.refresh_from_data_manager()
	await process_frame
	var info0: Dictionary = dl.get_data_container_info(cid)
	check(int(info0.get("entry_count", 0)) == 8, "Modo prueba 0 objetos con casillas vacías activas muestra 8 huecos vacíos (obtenido: %d)" % int(info0.get("entry_count", 0)))

	# Con show_empty = false -> 0 celdas
	var cel0: Dictionary = dm.get_ui_element("game_menu_items", cid)
	cel0["data_show_empty"] = false
	dm.update_ui_element("game_menu_items", cid, cel0, true)
	ed.refresh_from_data_manager()
	await process_frame
	info0 = dl.get_data_container_info(cid)
	check(int(info0.get("entry_count", -1)) == 0, "Modo prueba 0 sin casillas vacías muestra 0 elementos")

	# Restaurar casillas vacías y probar 8
	cel0["data_show_empty"] = true
	dm.update_ui_element("game_menu_items", cid, cel0, true)
	dm.set_editor_test_data_count(8)
	ed.refresh_from_data_manager()
	await process_frame
	var info8: Dictionary = dl.get_data_container_info(cid)
	check(int(info8.get("entry_count", 0)) == 8, "Modo prueba 8 muestra 8 objetos de ejemplo")
	var e0: Dictionary = info8.get("entries", [])[0]
	check(not bool(e0.get("is_empty", true)), "Primera casilla con 8 objetos de prueba no está vacía")

	# Probar 40 y verificar scroll
	dm.set_editor_test_data_count(40)
	ed.refresh_from_data_manager()
	await process_frame
	var info40: Dictionary = dl.get_data_container_info(cid)
	check(int(info40.get("entry_count", 0)) == 40, "Modo prueba 40 genera 40 objetos reales de prueba")
	check(int(info40.get("scroll_max_v", 0)) > 220, "Con 40 objetos el área desplazable supera la altura del contenedor (%d > 220)" % int(info40.get("scroll_max_v", 0)))

	# Probar popup ("Probar") con 8 objetos de prueba
	dm.set_editor_test_data_count(8)
	var tw = ed.open_test_popup()
	await process_frame
	check(tw != null and tw.is_visible(), "Ventana Probar abre en Editor de UI")
	ed.close_test_popup()
	await process_frame

	# Probar modo F5 / tiempo de juego con inventario real (use_editor_test_data = false)
	dm.set_editor_test_data_count(-1)
	var test_item_key := "items:101"
	var second_item_key := "weapons:201"
	dm.change_item(test_item_key, 3)
	dm.change_item(second_item_key, 1)

	var gm = ClassDB.instantiate("ClockGearsGameMenu")
	get_root().add_child(gm)
	gm.open_menu_by_id("game_menu_items", false, false, true)
	await process_frame
	await process_frame
	var gml = gm.get_custom_ui_layer()
	check(gml != null and gml.is_visible(), "Capa UI personalizada activa en menú del juego")
	var live_info: Dictionary = gml.get_data_container_info(cid)
	var ents_live: Array = live_info.get("entries", [])
	var real_cnt := 0
	for re in ents_live:
		if not bool(re.get("is_empty", true)):
			real_cnt += 1
	check(real_cnt >= 1, "En partida (F5) el contenedor muestra objetos reales del inventario (%d reales)" % real_cnt)
	check(gml.focus_data_slot(cid, 0), "Foco en ranura 0 del contenedor funciona")
	await process_frame

	# Verificar que el foco actualiza la etiqueta item_name y la vista previa rotable item_icon
	var name_lbl: Node = gml.find_child("TextLabel", true, false)
	var any_rot_img: TextureRect = null
	for ch in gml.find_children("*", "TextureRect", true, false):
		if ch.mouse_filter == Control.MOUSE_FILTER_STOP:
			any_rot_img = ch
			break
	if any_rot_img != null:
		var mb_down := InputEventMouseButton.new()
		mb_down.button_index = MOUSE_BUTTON_LEFT
		mb_down.pressed = true
		any_rot_img.emit_signal("gui_input", mb_down)
		var mm := InputEventMouseMotion.new()
		mm.relative = Vector2(30, 0)
		any_rot_img.emit_signal("gui_input", mm)
		var mb_up := InputEventMouseButton.new()
		mb_up.button_index = MOUSE_BUTTON_LEFT
		mb_up.pressed = false
		any_rot_img.emit_signal("gui_input", mb_up)
		check(absf(any_rot_img.rotation_degrees) > 1.0, "Icono de objeto con rotatable=true gira al arrastrar (rot=%.1f)" % any_rot_img.rotation_degrees)
	else:
		check(false, "Icono rotable item_icon no encontrado en runtime")

	# Verificar ordenación / filtrado en DataManager
	var sorted_name: Array = dm.resolve_ui_container_entries({"data_source": "inventory_all", "data_sort_by": "name", "data_show_empty": false}, 1, 0, false)
	var weapons_only: Array = dm.resolve_ui_container_entries({"data_source": "inventory_weapons", "data_show_empty": false}, 1, 0, false)
	check(sorted_name.size() >= 2, "Ordenación por nombre en inventory_all devuelve >= 2 entradas reales (%d)" % sorted_name.size())
	if sorted_name.size() >= 2:
		check(str(sorted_name[0].get("name", "")).naturalnocasecmp_to(str(sorted_name[1].get("name", ""))) <= 0, "Entradas ordenadas alfabéticamente por nombre (%s <= %s)" % [str(sorted_name[0].get("name", "")), str(sorted_name[1].get("name", ""))])
	check(weapons_only.size() <= sorted_name.size(), "Filtro por categoría inventory_weapons devuelve subconjunto válido (%d <= %d)" % [weapons_only.size(), sorted_name.size()])

	# Verificar usar y descartar con slot_action = item_use y item_discard en capa real
	if not test_item_key.is_empty():
		var party: Array = dm.get_party_actor_ids()
		var aid: int = int(party[0]) if not party.is_empty() else 1
		dm.set_actor_runtime_hp(aid, 10)
		var qty_before_use: int = _get_qty(test_item_key)
		var g_el: Dictionary = dm.get_ui_element("game_menu_items", cid)
		g_el["slot_action"] = "item_use"
		dm.update_ui_element("game_menu_items", cid, g_el, true)
		gm.open_menu_by_id("game_menu_items", false, false, true)
		await process_frame
		gml = gm.get_custom_ui_layer()
		var live_u: Dictionary = gml.get_data_container_info(cid)
		var slot_u := -1
		var ents_u: Array = live_u.get("entries", [])
		for idx in range(ents_u.size()):
			if str(ents_u[idx].get("key", "")) == test_item_key:
				slot_u = idx
				break
		if slot_u >= 0:
			gml.activate_data_slot(cid, slot_u)
			await process_frame
			var qty_after_use: int = _get_qty(test_item_key)
			check(qty_after_use == qty_before_use - 1, "Acción item_use consume objeto real en F5 (%d -> %d)" % [qty_before_use, qty_after_use])
		else:
			check(false, "Objeto usable real no encontrado para item_use")

		var qty_before: int = _get_qty(test_item_key)
		g_el["slot_action"] = "item_discard"
		dm.update_ui_element("game_menu_items", cid, g_el, true)
		gm.open_menu_by_id("game_menu_items", false, false, true)
		await process_frame
		gml = gm.get_custom_ui_layer()
		var live2: Dictionary = gml.get_data_container_info(cid)
		var slot_to_act := -1
		var ents2: Array = live2.get("entries", [])
		for idx in range(ents2.size()):
			if str(ents2[idx].get("key", "")) == test_item_key:
				slot_to_act = idx
				break
		check(slot_to_act >= 0, "Objeto real presente en contenedor en F5")
		if slot_to_act >= 0:
			gml.activate_data_slot(cid, slot_to_act)
			await process_frame
			var qty_after: int = _get_qty(test_item_key)
			check(qty_after == qty_before - 1, "Acción item_discard reduce cantidad real del objeto (%d -> %d)" % [qty_before, qty_after])

	gm.queue_free()
	ed.queue_free()
	await process_frame
	await process_frame

func _get_qty(k: String) -> int:
	for row in dm.get_inventory_entries():
		if str(row.get("key", "")) == k:
			return int(row.get("qty", 0))
	return 0

func _tpl_ids(list: Array) -> Array:
	var ids := []
	for t in list:
		ids.append(str(t.get("id", "")))
	return ids

func _test_punto_2() -> void:
	print("--- Punto 2: Plantillas independientes y migración ---")
	var title_ids := _tpl_ids(dm.get_ui_templates_for_system("title", true))
	var items_ids := _tpl_ids(dm.get_ui_templates_for_system("game_menu_items", true))
	var msg_ids := _tpl_ids(dm.get_ui_templates_for_system("messages", true))
	check(title_ids.has("classic_title") and not title_ids.has("corner_hud") and not title_ids.has("arpg_inventory_grid"), "Galería Título muestra solo compatibles (%s)" % str(title_ids))
	check(items_ids.has("arpg_inventory_grid") and not items_ids.has("classic_title") and not items_ids.has("corner_hud") and not items_ids.has("messages_classic"), "Galería Inventario excluye Título/HUD/Mensajes")
	check(msg_ids.has("messages_classic") and not msg_ids.has("arpg_inventory_grid") and not msg_ids.has("classic_title"), "Galería Mensajes muestra solo compatibles")

	# Aplicar cuadrícula a Inventario, Equipo, Estado y Guardado -> fuente de datos contextual + independencia
	check(dm.apply_menu_template("game_menu_items", "arpg_inventory_grid", false, false), "Plantilla arpg_inventory_grid aplicada a Inventario")
	check(dm.apply_menu_template("game_menu_equip", "arpg_inventory_grid", false, false), "Plantilla arpg_inventory_grid aplicada a Equipo")
	check(dm.apply_menu_template("game_menu_status", "arpg_inventory_grid", false, false), "Plantilla arpg_inventory_grid aplicada a Estado")
	check(dm.apply_menu_template("game_menu_save", "arpg_inventory_grid", false, false), "Plantilla arpg_inventory_grid aplicada a Guardado")

	var el_items: Dictionary = dm.get_ui_element("game_menu_items", "inv_data_grid")
	var el_equip: Dictionary = dm.get_ui_element("game_menu_equip", "inv_data_grid")
	var el_status: Dictionary = dm.get_ui_element("game_menu_status", "inv_data_grid")
	var el_save: Dictionary = dm.get_ui_element("game_menu_save", "inv_data_grid")
	check(str(el_items.get("data_source", "")) == "inventory_all", "Inventario enlaza inventory_all")
	check(str(el_equip.get("data_source", "")) == "equip_compatible", "Equipo enlaza equip_compatible")
	check(str(el_status.get("data_source", "")) == "actor_states", "Estado enlaza actor_states")
	check(str(el_save.get("data_source", "")) == "save_slots", "Guardado enlaza save_slots")

	# Sin texto literal "Ranura 1" como botones estáticos en arpg_inventory_grid
	var has_static_ranura := false
	for e in dm.get_ui_screen_elements("game_menu_items"):
		if str(e.get("type", "")) == "button" and str(e.get("text", "")).begins_with("Ranura "):
			has_static_ranura = true
	check(not has_static_ranura, "Plantilla arpg_inventory_grid no crea botones estáticos 'Ranura 1..8'")

	# Editar Inventario NO afecta a Equipo
	el_items["width"] = 777
	dm.update_ui_element("game_menu_items", "inv_data_grid", el_items, false)
	var el_equip_after: Dictionary = dm.get_ui_element("game_menu_equip", "inv_data_grid")
	check(int(el_equip_after.get("width", 0)) != 777, "Editar Inventario tras aplicar plantilla no cambia Equipo (%d != 777)" % int(el_equip_after.get("width", 0)))

	# Mis plantillas: guardar y aplicar en otra pantalla es copia independiente
	var user_tid: String = dm.save_screen_as_user_template("game_menu_items", "MiRejillaAislada")
	check(not user_tid.is_empty(), "Guardada plantilla en 'Mis plantillas': " + user_tid)
	dm.apply_menu_template("game_menu_equip", user_tid, false, false)
	var eq_mod: Dictionary = dm.get_ui_element("game_menu_equip", "inv_data_grid")
	eq_mod["width"] = 333
	dm.update_ui_element("game_menu_equip", "inv_data_grid", eq_mod, false)
	check(int(dm.get_ui_element("game_menu_items", "inv_data_grid").get("width", 0)) == 777, "Editar pantalla desde Mis plantillas no muta la pantalla de origen")
	dm.delete_user_template(user_tid)

	# Migración con copia de seguridad (.pre_data_containers.bak) de botones estáticos 'Ranura 1..8'
	var tmp_dir := ProjectSettings.globalize_path("user://test_p2_mig")
	DirAccess.make_dir_recursive_absolute(tmp_dir)
	var fdb := FileAccess.open(tmp_dir.path_join("database.json"), FileAccess.WRITE)
	fdb.store_string("{\"schema_version\": 3}")
	fdb.close()
	var legacy_doc := {
		"schema_version": 5,
		"screens": {
			"game_menu_items": {
				"screen_id": "game_menu_items",
				"custom_enabled": true,
				"elements": []
			},
			"mi_pantalla_ambigua": {
				"screen_id": "mi_pantalla_ambigua",
				"name": "Personalizada",
				"custom_enabled": true,
				"elements": [
					{"id": "inv_slot_1", "name": "Ranura 1", "type": "button", "text": "Ranura 1", "offset_x": 40, "offset_y": 40, "width": 100, "height": 80},
					{"id": "inv_slot_2", "name": "Ranura 2", "type": "button", "text": "Ranura 2", "offset_x": 160, "offset_y": 40, "width": 100, "height": 80}
				]
			}
		}
	}
	for i in range(8):
		legacy_doc["screens"]["game_menu_items"]["elements"].append({
			"id": "inv_slot_%d" % (i + 1),
			"name": "Ranura %d" % (i + 1),
			"type": "button",
			"text": "Ranura %d" % (i + 1),
			"offset_x": 160 + (i % 4) * 160,
			"offset_y": 120 + int(i / 4) * 140,
			"width": 140,
			"height": 120
		})
	var ui_file := tmp_dir.path_join("ui_designs.json")
	var bak_file := ui_file + ".pre_data_containers.bak"
	if FileAccess.file_exists(bak_file):
		DirAccess.remove_absolute(bak_file)
	var fa := FileAccess.open(ui_file, FileAccess.WRITE)
	fa.store_string(JSON.stringify(legacy_doc, "\t"))
	fa.close()

	var mig_dm = ClassDB.instantiate("ClockGearsDataManager")
	mig_dm.load_all(tmp_dir)
	var rep: Dictionary = mig_dm.get_last_ui_migration_report()
	check(FileAccess.file_exists(bak_file), "Migración crea copia de seguridad .pre_data_containers.bak")
	var mig_conts: Array = rep.get("migrated_containers", [])
	check(mig_conts.size() == 1 and str(mig_conts[0].get("data_source", "")) == "inventory_all", "Migración convierte Inventario 'Ranura 1..8' en contenedor inventory_all (report=%s)" % str(mig_conts))
	var amb_elems: Array = mig_dm.get_ui_screen_elements("mi_pantalla_ambigua")
	var amb_kept := false
	for ae in amb_elems:
		if str(ae.get("id", "")) == "inv_slot_1":
			amb_kept = true
	check(amb_kept, "Pantalla personalizada ambigua queda sin tocar durante la migración")
	mig_dm.free()

func _finish() -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [passes, fails])
	quit(0 if fails == 0 else 1)
