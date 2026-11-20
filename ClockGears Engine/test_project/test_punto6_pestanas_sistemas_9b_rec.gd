extends SceneTree

# Test Punto 6 (UI Reconst 9B):
# - Sustitucion de "Interfaz" por 12 pestanas de sistema en capitulo "Menus"
# - Reutilizacion de UNA unica instancia de ClockGearsUiDesignEditor en las 11 pestanas de diseno
# - Ramificacion jerarquica de submenus propios (+ Anadir submenu, parent_id, system_group)
# - Botones superiores claros (+ Nuevo diseno, + Anadir submenu, + Anadir boton, Restaurar plantilla original, Probar)
# - Plantilla Rueda radial + multi-accion + condiciones por interruptor + selector 16:9 + Probar

var pass_count: int = 0
var fail_count: int = 0
var _watchdog_tripped: bool = false

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] " + msg)
	else:
		fail_count += 1
		print("  [FAIL] " + msg)

func _step_frames(n: int = 2) -> void:
	for i in range(n):
		await process_frame

func _find_tree_item_by_meta(item: TreeItem, target_meta: String) -> TreeItem:
	if item == null:
		return null
	if item.is_selectable(0) and str(item.get_metadata(0)) == target_meta:
		return item
	var ch: TreeItem = item.get_first_child()
	while ch != null:
		var found := _find_tree_item_by_meta(ch, target_meta)
		if found != null:
			return found
		ch = ch.get_next()
	return null

func _count_selectable_tree_items(item: TreeItem) -> int:
	if item == null:
		return 0
	var cnt: int = 1 if (item.get_parent() != null and item.is_selectable(0)) else 0
	var ch: TreeItem = item.get_first_child()
	while ch != null:
		cnt += _count_selectable_tree_items(ch)
		ch = ch.get_next()
	return cnt

func _drag_rect(win: Window, overlay: Control, from_pt: Vector2, to_pt: Vector2) -> void:
	var xf: Transform2D = overlay.get_global_transform_with_canvas()
	var g0 := xf * from_pt
	var g1 := xf * to_pt
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = g0
	press.global_position = g0
	win.push_input(press)
	var mot := InputEventMouseMotion.new()
	mot.button_mask = MOUSE_BUTTON_MASK_LEFT
	mot.position = g1
	mot.global_position = g1
	mot.relative = g1 - g0
	win.push_input(mot)
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = g1
	rel.global_position = g1
	win.push_input(rel)

func _elem_rect(el: Dictionary, sc: float) -> Rect2:
	var anc := str(el.get("anchor", "top_left"))
	var ax := 0.0
	var ay := 0.0
	if anc.ends_with("_center") or anc == "center":
		ax = 0.5
	elif anc.ends_with("_right"):
		ax = 1.0
	if anc.begins_with("center_") or anc == "center":
		ay = 0.5
	elif anc.begins_with("bottom_"):
		ay = 1.0
	var w := float(el.get("width", 120))
	var h := float(el.get("height", 36))
	var ox := float(el.get("offset_x", 0))
	var oy := float(el.get("offset_y", 0))
	return Rect2((ax * (960.0 - w) + ox) * sc, (ay * (540.0 - h) + oy) * sc, w * sc, h * sc)

func _initialize() -> void:
	var watchdog := create_timer(60.0)
	watchdog.timeout.connect(func():
		_watchdog_tripped = true
		print("  [FAIL] Watchdog interno 60s agotado")
		fail_count += 1
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 6 (UI RECONST 9B): 12 PESTANAS POR SISTEMA Y RAMIFICACION ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager singleton disponible")
	if dm == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	dm.load_all()
	var backup_ui_cfg: Dictionary = dm.get_ui_designs_config().duplicate(true)
	var backup_hud_cfg: Dictionary = dm.get_hud_config().duplicate(true)
	var backup_title_cfg: Dictionary = dm.get_title_menu_config().duplicate(true)

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)

	# 1. Verificar estructura exacta de 12 pestanas en el capitulo "Menus"
	var chap_info: Dictionary = db.get_chapter_structure_info()
	var chaps: Array = chap_info.get("chapters", [])
	var menus_chap: Dictionary = {}
	for c in chaps:
		if str(c.get("name", "")) == "Menús":
			menus_chap = c
			break
	_check(not menus_chap.is_empty(), "Capitulo 'Menús' encontrado")
	var expected_keys: Array = [
		"main_menu", "menu_options", "menu_saves", "game_menu",
		"menu_inventory", "menu_equip", "menu_status", "credits",
		"game_over", "hud", "messages_battle", "window_styles"
	]
	var expected_names: Array = [
		"Menú de inicio", "Opciones", "Partidas", "Menú del juego",
		"Inventario", "Equipo", "Estado", "Créditos",
		"Game Over", "HUD", "Mensajes y batalla", "Estilos de ventana"
	]
	var actual_keys: Array = menus_chap.get("category_keys", [])
	var actual_names: Array = menus_chap.get("category_names", [])
	_check(actual_keys == expected_keys, "Capitulo Menus tiene las 12 claves exactas en orden: " + str(actual_keys))
	_check(actual_names == expected_names, "Capitulo Menus tiene los 12 nombres exactos: " + str(actual_names))
	_check(not actual_keys.has("ui_designer"), "La pestana unica 'Interfaz' ya no figura en el capitulo Menus")

	# 2. Verificar que las 11 pestanas de diseno comparten UNA UNICA instancia del editor
	var tabs_to_screens: Dictionary = {
		"main_menu": "title",
		"menu_options": "game_menu_options",
		"menu_saves": "game_menu_save",
		"game_menu": "game_menu_main",
		"menu_inventory": "game_menu_items",
		"menu_equip": "game_menu_equip",
		"menu_status": "game_menu_status",
		"credits": "credits",
		"game_over": "game_over",
		"hud": "hud",
		"messages_battle": "messages_battle"
	}
	var shared_ed_id: int = -1
	for ckey in tabs_to_screens.keys():
		var root_sid: String = str(tabs_to_screens[ckey])
		db.select_category_by_key(ckey)
		await _step_frames(3)
		var ed := db.get_ui_designer_editor()
		_check(ed != null and ed.is_visible_in_tree(), "[%s] Abre ClockGearsUiDesignEditor visible" % ckey)
		if ed != null:
			if shared_ed_id == -1:
				shared_ed_id = ed.get_instance_id()
			else:
				_check(ed.get_instance_id() == shared_ed_id, "[%s] Reutiliza la misma instancia unica (%d == %d)" % [ckey, ed.get_instance_id(), shared_ed_id])
			_check(ed.get_system_scope() == root_sid, "[%s] Ambito del arbol fijado en '%s'" % [ckey, root_sid])
			_check(ed.get_current_screen() == root_sid, "[%s] Pantalla inicial activa '%s'" % [ckey, root_sid])

			var h_tab: Dictionary = ed.get_ui_handles()
			var tree_ctl: Tree = h_tab.get("screens_tree", null) as Tree
			_check(tree_ctl != null and _find_tree_item_by_meta(tree_ctl.get_root(), root_sid) != null,
				"[%s] Arbol muestra su raiz '%s'" % [ckey, root_sid])
			var other_sid: String = "credits" if root_sid != "credits" else "title"
			_check(_find_tree_item_by_meta(tree_ctl.get_root(), other_sid) == null,
				"[%s] Arbol NO mezcla pantallas ajenas ('%s')" % [ckey, other_sid])

			# Arrastrar / mover un elemento dentro de cada pestana para comprobar estabilidad real
			var raw_elems: Array = dm.get_ui_screen_elements(root_sid)
			var elems: Array = raw_elems if (dm.has_custom_ui_design(root_sid) and not raw_elems.is_empty()) else Array(dm.get_default_ui_screen_template(root_sid).get("elements", []))
			_check(not elems.is_empty(), "[%s] Tiene elementos visibles en su plantilla por defecto (%d)" % [ckey, elems.size()])
			if not elems.is_empty():
				var last_el: Dictionary = Dictionary(elems[elems.size() - 1]).duplicate(true)
				var eid: String = str(last_el.get("id", ""))
				var old_ox: int = int(last_el.get("offset_x", 0))
				var old_oy: int = int(last_el.get("offset_y", 0))
				ed.select_element(eid)
				await _step_frames(1)
				var was_custom: bool = dm.has_custom_ui_design(root_sid)
				var overlay: Control = ed.get_canvas_overlay() as Control
				var pres: Vector2i = ed.get_preview_resolution()
				var sc: float = (float(pres.x) / 960.0) * float(ed.get_canvas_zoom())
				var r0 := _elem_rect(last_el, sc)
				var c0 := r0.position + r0.size * 0.5
				var dx := -24.0 if c0.x > 480.0 * sc else 24.0
				var dy := -16.0 if c0.y > 270.0 * sc else 16.0
				_drag_rect(db, overlay, c0, c0 + Vector2(dx, dy))
				await _step_frames(2)
				var read_back: Dictionary = dm.get_ui_element(root_sid, eid)
				var moved := int(read_back.get("offset_x", old_ox)) != old_ox or int(read_back.get("offset_y", old_oy)) != old_oy
				_check(moved, "[%s] Arrastrar elemento '%s' lo mueve realmente" % [ckey, eid])
				if not was_custom and root_sid != "hud":
					ed.request_reset_to_default(true)
					await _step_frames(1)

	# Pestana 12: Estilos de ventana oculta el disenador unificado y abre estilos
	db.select_category_by_key("window_styles")
	await _step_frames(3)
	var ed_after_ws := db.get_ui_designer_editor()
	var ws_handles: Dictionary = db.get_window_styles_ui_handles()
	var ws_panel: Control = ws_handles.get("panel", null) as Control
	_check(ws_panel != null and ws_panel.is_visible_in_tree(), "[window_styles] Abre editor de Estilos de ventana")
	_check(ed_after_ws != null and not ed_after_ws.visible, "[window_styles] Oculta ClockGearsUiDesignEditor sin destruirlo")

	# 3. Botones superiores requeridos en el editor
	db.select_category_by_key("menu_inventory")
	await _step_frames(3)
	var ed_inv := db.get_ui_designer_editor()
	var hi: Dictionary = ed_inv.get_ui_handles()
	var btn_new_design: Button = hi.get("new_menu_wizard_btn", null) as Button
	var btn_add_sub: Button = hi.get("add_submenu_btn", null) as Button
	var btn_add_btn: Button = hi.get("add_button_bar_btn", null) as Button
	var btn_reset: Button = hi.get("reset_btn", null) as Button
	var btn_test: Button = hi.get("test_btn", null) as Button
	_check(btn_new_design != null and btn_new_design.text.find("Nuevo diseño") >= 0, "Boton '+ Nuevo diseño' presente")
	_check(btn_add_sub != null and btn_add_sub.text.find("Añadir submenú") >= 0, "Boton '+ Añadir submenú' presente")
	_check(btn_add_btn != null and btn_add_btn.text.find("Añadir botón") >= 0, "Boton '+ Añadir botón' presente")
	_check(btn_reset != null and btn_reset.text.find("Restaurar plantilla") >= 0, "Boton 'Restaurar plantilla original' presente")
	_check(btn_test != null and btn_test.text.find("Probar") >= 0, "Boton 'Probar' presente")

	# 4. Ramificacion jerarquica de submenus propios (+ Anadir submenu)
	var sub_inv_id: String = ed_inv.add_submenu_to_current("Submenú Ordenar")
	await _step_frames(2)
	_check(not sub_inv_id.is_empty() and dm.has_menu(sub_inv_id), "Submenu propio creado en Inventario: " + sub_inv_id)
	var sub_inv_meta: Dictionary = dm.get_menu_metadata(sub_inv_id)
	_check(str(sub_inv_meta.get("parent_id", "")) == "game_menu_items", "Submenu Inventario conserva parent_id='game_menu_items'")
	_check(str(sub_inv_meta.get("system_group", "")) == "game_menu_items", "Submenu Inventario conserva system_group='game_menu_items'")

	var inv_tree: Tree = hi.get("screens_tree", null) as Tree
	var inv_parent_ti := _find_tree_item_by_meta(inv_tree.get_root(), "game_menu_items")
	var inv_child_ti := _find_tree_item_by_meta(inv_tree.get_root(), sub_inv_id)
	_check(inv_child_ti != null and inv_child_ti.get_parent() == inv_parent_ti,
		"Arbol de Inventario anida 'Submenú Ordenar' bajo Inventario")

	# Probar segundo sistema: Menu del juego -> Submenu Diario + Rueda radial + Boton con multi-accion y condicion interruptor
	db.select_category_by_key("game_menu")
	await _step_frames(3)
	var sub_diario_id: String = ed_inv.add_submenu_to_current("Submenú Diario")
	await _step_frames(2)
	_check(not sub_diario_id.is_empty() and dm.get_menu_system_group(sub_diario_id) == "game_menu_main",
		"Submenu Diario en Menu del juego pertenece a system_group='game_menu_main'")

	var gm_tpls: Array = ClockGearsDataManager.get_ui_templates_for_system("game_menu_main")
	var has_radial_tpl: bool = false
	for t in gm_tpls:
		var tid: String = str(t.get("id", ""))
		if tid == "rueda_radial" or tid == "radial_ring":
			has_radial_tpl = true
			break
	_check(has_radial_tpl, "Plantillas filtradas del sistema incluyen Rueda/Anillo radial")

	_check(ed_inv.apply_template_by_id("rueda_radial", false, true), "Aplica plantilla 'Rueda radial' al submenu Diario")
	var diario_cfg: Dictionary = dm.get_ui_screen_design(sub_diario_id)
	_check(str(Dictionary(diario_cfg.get("layout", {})).get("mode", "")) == "radial", "Submenu Diario quedo en modo 'radial'")

	# Anadir boton y probar multi-accion + condicion por interruptor
	var added_btn_id: String = ed_inv.quick_add_button_element("Abrir Diario")
	await _step_frames(2)
	_check(not added_btn_id.is_empty(), "'+ Añadir botón' crea boton en el diseno activo: " + added_btn_id)
	var btn_el: Dictionary = dm.get_ui_element(sub_diario_id, added_btn_id)
	btn_el["actions"] = [
		{"type": "play_sound", "se_path": ""},
		{"type": "set_switch", "switch_name": "sw_diario_visto", "value": true}
	]
	btn_el["visibility_switch_mode"] = "on"
	btn_el["visibility_switch_name"] = "sw_diario_disponible"
	dm.update_ui_element(sub_diario_id, added_btn_id, btn_el, false)
	var btn_saved: Dictionary = dm.get_ui_element(sub_diario_id, added_btn_id)
	_check(Array(btn_saved.get("actions", [])).size() == 2 and str(btn_saved.get("visibility_switch_mode", "")) == "on",
		"Boton propio guarda secuencia multi-accion (2 pasos) y condicion de interruptor")

	# Verificar que Menu de inicio sigue aislado y no ve Submenu Diario ni Inventario
	db.select_category_by_key("main_menu")
	await _step_frames(2)
	var title_tree: Tree = hi.get("screens_tree", null) as Tree
	_check(_find_tree_item_by_meta(title_tree.get_root(), sub_diario_id) == null,
		"Menú de inicio no mezcla submenus del Menú del juego")

	# Selector de resolucion 16:9 y Probar popup
	var prev_opt: OptionButton = hi.get("preview_res_opt", null) as OptionButton
	_check(prev_opt != null and prev_opt.item_count >= 4, "Selector de resolucion 16:9 tiene las 4 resoluciones (%d)" % (prev_opt.item_count if prev_opt else -1))
	ed_inv.set_preview_resolution(1366, 768, false)
	await _step_frames(1)
	_check(ed_inv.get_preview_resolution() == Vector2i(1366, 768), "Vista previa cambia a 1366x768 (16:9)")
	ed_inv.set_preview_resolution(960, 540, false)

	var tw := ed_inv.open_test_popup()
	await _step_frames(3)
	var tinfo: Dictionary = ed_inv.get_test_popup_info()
	_check(tw != null and bool(tinfo.get("open", false)), "Boton 'Probar' abre popup en vivo en pestana de sistema")
	ed_inv.close_test_popup()
	await _step_frames(1)

	# Limpieza de submenus temporales y restauracion intacta de configuracion del proyecto
	dm.delete_custom_menu(sub_inv_id)
	dm.delete_custom_menu(sub_diario_id)
	dm.set_ui_designs_config(backup_ui_cfg)
	dm.set_hud_config(backup_hud_cfg)
	dm.set_title_menu_config(backup_title_cfg)

	db.queue_free()
	await _step_frames(2)

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
