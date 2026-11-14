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

func _step_frames(n: int = 3) -> void:
	for i in range(n):
		await process_frame

func _snapshot_data_dir(res_data_dir: String) -> Dictionary:
	var backup := {}
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				backup[full] = FileAccess.get_file_as_string(full)
			fname = da.get_next()
		da.list_dir_end()
	return backup

func _restore_data_dir(res_data_dir: String, backup: Dictionary) -> void:
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				if not backup.has(full):
					DirAccess.remove_absolute(ProjectSettings.globalize_path(full))
			fname = da.get_next()
		da.list_dir_end()
	for full in backup.keys():
		var wf := FileAccess.open(full, FileAccess.WRITE)
		if wf:
			wf.store_string(backup[full])

func _find_button_by_text(node: Node, needle: String) -> Button:
	if node == null:
		return null
	if node is Button:
		var b := node as Button
		if not (b is OptionButton) and b.text.find(needle) >= 0:
			return b
	for c in node.get_children():
		var res := _find_button_by_text(c, needle)
		if res != null:
			return res
	return null

func _find_equip_preview_colors(node: Node, out: Array) -> void:
	if node is Label:
		var l := node as Label
		if l.has_theme_color_override("font_color"):
			out.append(l.get_theme_color("font_color"))
	for c in node.get_children():
		_find_equip_preview_colors(c, out)

func _run_tests() -> void:
	print("=== TEST 9B TANDA 3 UI: PANTALLAS Y EDITOR DEL MENÚ DEL JUEGO ===")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager singleton disponible")
	if dm == null:
		return

	var res_data_dir := "res://clockgears_data"
	var backup := _snapshot_data_dir(res_data_dir)
	await _step_frames(1)

	var portrait_res := "res://_test_portrait_9b3.png"
	var sprite_res := "res://_test_sprite_9b3.png"
	var bg_res := "res://_test_gm_bg_9b3.png"
	var img_p := Image.create(32, 32, false, Image.FORMAT_RGBA8)
	img_p.fill(Color(0.25, 0.55, 0.85, 1.0))
	img_p.save_png(ProjectSettings.globalize_path(portrait_res))

	var img_s := Image.create(128, 128, false, Image.FORMAT_RGBA8)
	img_s.fill(Color(0.85, 0.45, 0.25, 1.0))
	img_s.save_png(ProjectSettings.globalize_path(sprite_res))

	var img_bg := Image.create(64, 64, false, Image.FORMAT_RGBA8)
	img_bg.fill(Color(0.12, 0.18, 0.26, 1.0))
	img_bg.save_png(ProjectSettings.globalize_path(bg_res))

	dm.load_all(res_data_dir)
	var orig_sys: Dictionary = dm.get_system_config().duplicate(true)

	# Configurar actores (uno con retrato, otro con sprite), moneda, oro, tiempo e inventario
	var actors := [
		{"id": 1, "name": "Elena", "class_id": 1, "initial_level": 2, "hp": 120, "mp": 40, "atk": 18, "def": 12, "mat": 10, "mdf": 10, "agi": 14, "luk": 8, "portrait_path": portrait_res},
		{"id": 2, "name": "Kael", "class_id": 1, "initial_level": 3, "hp": 140, "mp": 25, "atk": 22, "def": 15, "mat": 8, "mdf": 9, "agi": 11, "luk": 7, "graphic_path": sprite_res}
	]
	dm.set_category_array(0, actors)
	dm.set_category_array(1, [{"id": 1, "name": "Espadachín", "exp_curve_type": 0}])
	dm.set_category_array(3, [
		{"id": 1, "name": "Poción Menor", "desc": "Recupera 30 HP", "type": 0, "secondary_type": 7, "tertiary_type": 0, "consumable": true, "is_key_item": false, "max_stack": 15, "effects": [{"code": 11, "data_id": 0, "value1": 0.0, "value2": 30.0}]},
		{"id": 2, "name": "Reliquia Antigua", "desc": "Objeto de historia", "type": 1, "is_key_item": true}
	])
	dm.set_category_array(4, [
		{"id": 1, "name": "Espada de Acero", "desc": "Hoja equilibrada", "etype_id": 0, "atk": 14, "def": -2}
	])

	var sys: Dictionary = dm.get_system_config().duplicate(true)
	sys["combat_mode"] = 0
	sys["currency"] = "Monedas"
	sys["party"] = [1, 2]
	dm.set_system_config(sys)
	dm.set_game_menu_config(dm.get_default_game_menu_config())

	dm.reset_runtime_state()
	dm.set_actor_runtime_hp(1, 70)
	dm.set_actor_runtime_mp(1, 30)
	dm.set_gold(420)
	dm.set_runtime_clock_sec(3723.0) # 01:02:03
	dm.change_item("Poción Menor", 3)
	dm.change_item("Reliquia Antigua", 1)
	dm.change_item("Espada de Acero", 2)

	# 1. Runtime ClockGearsGameMenu: tarjetas, selector de miembro, navegación ratón/teclado y 16:9
	var gm_root := Node.new()
	root.add_child(gm_root)
	var tmap := ClockGearsMap.new()
	tmap.set_name("MapaUI9B3")
	tmap.set_map_name("Ciudad del Norte")
	tmap.set_map_size(Vector2i(16, 12))
	tmap.set_grid_resolution(32)
	gm_root.add_child(tmap)
	tmap.init_runtime_session()
	await _step_frames(2)

	var interp := tmap.get_runtime_interpreter()
	var gmenu: ClockGearsGameMenu = interp.get_game_menu() if interp != null else null
	check(gmenu != null, "ClockGearsGameMenu presente en ClockGearsEventInterpreter")
	check(gmenu.handle_input_key(KEY_ESCAPE), "Esc abre el menú del juego en el mapa")
	await _step_frames(2)

	var party_info: Dictionary = gmenu.get_party_cards_info()
	check(party_info.get("gold_text", "") == "420 Monedas", "Tarjeta muestra moneda con unidad visible: " + String(party_info.get("gold_text", "")))
	check(String(party_info.get("playtime_text", "")).find("01:02:03") >= 0, "Tarjeta muestra tiempo jugado HH:MM:SS: " + String(party_info.get("playtime_text", "")))
	var cards: Array = party_info.get("cards", [])
	check(cards.size() == 2, "Panel de grupo muestra los 2 miembros")
	if cards.size() == 2:
		check(bool(cards[0].get("has_portrait", false)), "Actor 1 muestra su retrato")
		check(bool(cards[1].get("has_portrait", false)), "Actor 2 muestra primer frame de su sprite")
		check(String(cards[0].get("vitals_text", "")).find("HP: 70 / 120") >= 0, "HP actual visible en tarjeta de grupo")

	# Selector de miembro visible con 2 miembros, oculto con 1 miembro
	for sub in ["items", "equip", "status"]:
		gmenu.open_subscreen(sub)
		await _step_frames(1)
	var sel2: Dictionary = gmenu.get_member_selectors_info()
	check(bool(sel2.get("items_visible", false)) and bool(sel2.get("equip_visible", false)) and bool(sel2.get("status_visible", false)),
		"Selector de miembro visible en Objetos, Equipo y Estado con 2 actores")
	check(bool(sel2.get("status_has_portrait", false)), "Ficha de Estado muestra retrato/sprite del actor")

	# Probar ciclo de miembro con botón '>' en Estado
	var prev_actor := int(sel2.get("selected_status_actor_id", 1))
	var status_panel := gmenu.find_child("StatusPanel", true, false)
	var btn_next := _find_button_by_text(status_panel, ">")
	check(btn_next != null, "Botón '>' de selector de miembro encontrado en Estado")
	if btn_next:
		btn_next.emit_signal("pressed")
		await _step_frames(1)
	var sel_after_next: Dictionary = gmenu.get_member_selectors_info()
	check(int(sel_after_next.get("selected_status_actor_id", 0)) != prev_actor, "Botón '>' cambia al siguiente miembro")

	# Con 1 solo miembro en el grupo se oculta el selector de miembro
	var sys1: Dictionary = dm.get_system_config().duplicate(true)
	sys1["party"] = [1]
	dm.set_system_config(sys1)
	gmenu.open_subscreen("equip")
	await _step_frames(1)
	var sel1: Dictionary = gmenu.get_member_selectors_info()
	check(not bool(sel1.get("equip_visible", true)), "Selector de miembro oculto en Equipo cuando hay 1 solo miembro")
	sys1["party"] = [1, 2]
	dm.set_system_config(sys1)
	gmenu.open_subscreen("main")
	await _step_frames(1)

	# Navegación teclado (Flechas + Enter + Esc)
	var f0 := gmenu.get_focused_nav_button()
	check(f0 != null and f0.text == "Objetos", "Foco inicial en primera entrada del menú (Objetos)")
	check(gmenu.handle_input_key(KEY_DOWN), "KEY_DOWN mueve foco a siguiente entrada")
	var f1 := gmenu.get_focused_nav_button()
	check(f1 != null and f1.text == "Equipo", "Foco movido a 'Equipo' tras KEY_DOWN")
	check(gmenu.handle_input_key(KEY_UP), "KEY_UP vuelve a entrada anterior")
	var f2 := gmenu.get_focused_nav_button()
	check(f2 != null and f2.text == "Objetos", "Foco devuelto a 'Objetos' tras KEY_UP")
	check(gmenu.handle_input_key(KEY_ENTER), "KEY_ENTER abre la subpantalla enfocada")
	await _step_frames(1)
	check(gmenu.get_current_subscreen() == "items", "Enter abrió la subpantalla Objetos")

	# Elegir miembro herido (Actor 1: Elena) con el botón '<' en Objetos y usar + tirar
	var items_panel := gmenu.find_child("ItemsPanel", true, false)
	var btn_items_prev := _find_button_by_text(items_panel, "<")
	if btn_items_prev and int(gmenu.get_member_selectors_info().get("selected_status_actor_id", 1)) != 1:
		btn_items_prev.emit_signal("pressed")
		await _step_frames(1)
	var poc_btn_before := _find_button_by_text(items_panel, "Poción Menor")
	check(poc_btn_before != null and poc_btn_before.text.find("x3") >= 0, "Botón de Poción muestra x3")
	poc_btn_before.emit_signal("pressed")
	await _step_frames(1)
	var use_btn := _find_button_by_text(items_panel, "Usar")
	check(use_btn != null, "Botón 'Usar' presente en Objetos")
	if use_btn:
		use_btn.emit_signal("pressed")
		await _step_frames(1)
	var poc_btn_after_use := _find_button_by_text(items_panel, "Poción Menor")
	check(poc_btn_after_use != null and poc_btn_after_use.text.find("x2") >= 0, "Usar Poción actualiza x2 al instante en lista UI")
	var discard_btn := _find_button_by_text(items_panel, "Tirar")
	if discard_btn:
		discard_btn.emit_signal("pressed")
		await _step_frames(1)
	var poc_btn_after_drop := _find_button_by_text(items_panel, "Poción Menor")
	check(poc_btn_after_drop != null and poc_btn_after_drop.text.find("x1") >= 0, "Tirar Poción actualiza x1 al instante en lista UI")

	# Esc vuelve al menú principal y Equipo muestra colores verde/rojo y refresca al equipar/quitar
	check(gmenu.handle_input_key(KEY_ESCAPE), "Esc vuelve de Objetos a Menú principal")
	check(gmenu.get_current_subscreen() == "main", "Subpantalla tras Esc es 'main'")
	gmenu.open_subscreen("equip")
	await _step_frames(1)
	var equip_panel := gmenu.find_child("EquipPanel", true, false)
	var sw_btn := _find_button_by_text(equip_panel, "Espada de Acero")
	check(sw_btn != null, "Pieza candidata 'Espada de Acero' visible en Equipo")
	if sw_btn:
		sw_btn.emit_signal("pressed")
		await _step_frames(1)
	var preview_colors: Array = []
	_find_equip_preview_colors(equip_panel, preview_colors)
	var has_green := false
	var has_red := false
	for col in preview_colors:
		if col.g > col.r + 0.2 and col.g > col.b + 0.2:
			has_green = true
		if col.r > col.g + 0.2 and col.r > col.b + 0.2:
			has_red = true
	check(has_green and has_red, "Comparación antes/después en Equipo muestra colores de subida (verde) y bajada (rojo)")
	var apply_btn := _find_button_by_text(equip_panel, "Equipar")
	if apply_btn:
		apply_btn.emit_signal("pressed")
		await _step_frames(1)
	var equipped_slot_btn := _find_button_by_text(equip_panel, "Espada de Acero")
	check(equipped_slot_btn != null, "Equipar refleja la pieza equipada en el hueco al instante")

	# Verificar que todas las pantallas caben en 960x540, 1366x768 y 1920x1080 sin cortes ni solapes
	for vp in [Vector2(960, 540), Vector2(1366, 768), Vector2(1920, 1080)]:
		for sub in ["main", "items", "equip", "status", "save", "options", "to_title"]:
			gmenu.open_subscreen(sub)
			await _step_frames(1)
			var bi: Dictionary = gmenu.get_layout_bounds_info(vp)
			check(bool(bi.get("all_within_viewport", false)),
				"Subpantalla '%s' cabe en %dx%d sin cortes ni solapamientos (oob=%s)" % [sub, int(vp.x), int(vp.y), str(bi.get("out_of_bounds_controls", []))])

	# 2. Editor en la Base de Datos: Menús -> Menú del juego
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(2)

	var ch_info: Dictionary = db.get_chapter_structure_info()
	var menus_names: Array = []
	for ch in ch_info.get("chapters", []):
		if String(ch.get("name", "")) == "Menús":
			menus_names = ch.get("category_names", [])
	check(menus_names.has("Menú del juego"), "Capítulo 'Menús' incluye 'Menú del juego': " + str(menus_names))

	db.select_category_by_key("game_menu")
	await _step_frames(2)
	var handles: Dictionary = db.get_game_menu_editor_ui_handles()
	var gm_panel: Control = handles.get("panel", null)
	check(gm_panel != null and gm_panel.visible, "Panel editor de Menú del juego visible")
	var live_prev: ClockGearsGameMenu = handles.get("live_preview", null)
	check(live_prev != null, "Vista previa en vivo de ClockGearsGameMenu instanciada")

	# Verifica que "magic" no aparezca editable en el editor ni en vista previa
	var entries_vbox: Node = handles.get("entries_vbox", null)
	var has_magic_row := false
	if entries_vbox:
		for ch in entries_vbox.get_children():
			if ch.has_meta("cg_entry_id") and String(ch.get_meta("cg_entry_id")) == "magic":
				has_magic_row = true
	check(not has_magic_row, "Entrada reservada 'Magia' oculta en el editor del Menú del juego")

	# Editar entrada y orden -> actualiza vista previa en vivo al instante
	db.set_game_menu_entry_state("items", true, "Mochila")
	db.move_game_menu_entry("status", -1)
	await _step_frames(1)
	var prev_texts: PackedStringArray = live_prev.get_visible_entry_texts()
	check(Array(prev_texts).has("Mochila"), "Renombrar entrada en DB actualiza vista previa en vivo: " + str(prev_texts))

	# Editar fondo por pantalla ('items') -> actualiza vista previa en vivo al instante
	db.set_game_menu_screen_background("items", "image", bg_res, "fill", "nearest")
	await _step_frames(1)
	check(live_prev.get_current_subscreen() == "items", "Cambiar pestaña de pantalla cambia vista previa a 'items'")
	var draft_after: Dictionary = db.get_game_menu_draft_copy()
	var bgs_draft: Dictionary = draft_after.get("backgrounds", {})
	check(Dictionary(bgs_draft.get("items", {})).get("image_path", "") == bg_res, "Fondo personalizado de Objetos guardado en borrador")

	# Botón "Probar" abre ventana interactiva
	var test_btn: Button = handles.get("test_btn", null)
	check(test_btn != null, "Botón 'Probar' presente en Menú del juego")
	if test_btn:
		test_btn.emit_signal("pressed")
		await _step_frames(2)
	var test_win: Window = db.get_game_menu_test_popup()
	var test_menu: ClockGearsGameMenu = handles.get("test_menu", null)
	check(test_win != null and test_win.visible, "Botón 'Probar' abre ventana emergente de Menú del juego")
	check(test_menu != null and test_menu.is_menu_open(), "Menú del juego en 'Probar' inicia abierto")
	if test_menu:
		test_menu.handle_input_key(KEY_ESCAPE)
		await _step_frames(1)
	check(test_win != null and not test_win.visible, "Esc cierra el popup de 'Probar'")

	# Regla 16:9 en Database Editor
	for win_sz in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		db.adapt_to_window_size(win_sz)
		await _step_frames(1)
		var lrep: Dictionary = db.check_all_tabs_layout(win_sz)
		check(bool(lrep.get("all_ok", false)), "Database 'Menú del juego' cabe en %dx%d sin scroll horizontal" % [win_sz.x, win_sz.y])

	# Guardar con OK y comprobar persistencia
	db.get_ok_button().emit_signal("pressed")
	await _step_frames(2)
	dm.load_all(res_data_dir)
	var persisted_cfg: Dictionary = dm.get_game_menu_config()
	var found_mochila := false
	for ent in persisted_cfg.get("entries", []):
		if String(ent.get("id", "")) == "items" and String(ent.get("text", "")) == "Mochila":
			found_mochila = true
	check(found_mochila, "Cambios en 'Menú del juego' persisten en disk JSON tras pulsar OK")

	# Limpieza
	db.queue_free()
	gm_root.queue_free()
	await _step_frames(2)

	for p in [portrait_res, sprite_res, bg_res]:
		var abs_p := ProjectSettings.globalize_path(p)
		if FileAccess.file_exists(abs_p):
			DirAccess.remove_absolute(abs_p)
	dm.set_system_config(orig_sys)
	_restore_data_dir(res_data_dir, backup)
	dm.load_all(res_data_dir)

func _initialize() -> void:
	await _run_tests()
	print("RESULTADOS: %d PASS, %d FAIL" % [passed, failed])
	quit(0 if failed == 0 else 1)
