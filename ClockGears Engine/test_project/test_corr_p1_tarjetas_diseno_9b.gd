extends SceneTree

var pass_count := 0
var fail_count := 0
var _start_msec := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + msg)
	else:
		fail_count += 1
		printerr("[FAIL] " + msg)

func _wait_frames(n: int) -> void:
	for i in range(n):
		if Time.get_ticks_msec() - _start_msec > 55000:
			printerr("INTERNAL TIMEOUT 55s")
			_finish(1)
			return
		await process_frame

func _finish(forced_code: int = -1) -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	if forced_code >= 0:
		quit(forced_code)
	else:
		quit(0 if fail_count == 0 else 1)

func _find_card(cards: Array, elem_id: String) -> Dictionary:
	for item in cards:
		if typeof(item) == TYPE_DICTIONARY and str(item.get("id", "")) == elem_id:
			return item
	return {}

func _index_of_card(cards: Array, elem_id: String) -> int:
	for i in range(cards.size()):
		if typeof(cards[i]) == TYPE_DICTIONARY and str(cards[i].get("id", "")) == elem_id:
			return i
	return -1

func _find_controls_by_type(n: Node, cls: String, out: Array) -> void:
	if n == null:
		return
	if n.get_class() == cls:
		out.append(n)
	for ch in n.get_children():
		_find_controls_by_type(ch, cls, out)

func _find_arrow_child(card: Node) -> Node:
	if card == null:
		return null
	for ch in card.get_children():
		if ch.get_class() == "ClockGearsDesignChildArrow":
			return ch
		var sub := _find_arrow_child(ch)
		if sub != null:
			return sub
	return null

func _initialize() -> void:
	_start_msec = Time.get_ticks_msec()
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s")
		_finish(1)
	)
	call_deferred("_run_test")

func _run_test() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "DataManager singleton disponible")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var dlg := ClockGearsDatabaseDialog.new()
	dlg.exclusive = false
	dlg.size = Vector2i(1366, 768)
	root.add_child(dlg)
	dlg.open_database()
	await _wait_frames(3)

	dlg.select_category_by_key("game_menu")
	await _wait_frames(3)
	var ed = dlg.get_ui_designer_editor()
	_check(ed != null, "Editor UI Menú del juego disponible")
	if ed == null:
		dlg.queue_free()
		_finish(1)
		return

	ed.select_screen("game_menu_main")
	await _wait_frames(4)

	var h: Dictionary = ed.get_ui_handles()
	var layers_card = h.get("layers_card", null)
	var menus_card: Control = h.get("menus_card", null)
	var left_scroll: ScrollContainer = h.get("left_col_scroll", null) as ScrollContainer
	var cards_scroll: ScrollContainer = h.get("design_cards_scroll", null) as ScrollContainer
	var cards_vbox: Control = h.get("design_cards_vbox", null)
	var wide_hbox = h.get("wide_hbox", null)

	_check(layers_card == null, "El árbol inferior 'Capas' fue eliminado (layers_card == null)")
	_check(menus_card != null and cards_scroll != null and cards_vbox != null, "Panel 'Diseños' tiene contenedor de tarjetas")
	_check(wide_hbox != null and wide_hbox.get_class() == "HSplitContainer", "Panel izquierdo tiene divisor arrastrable HSplitContainer")
	_check(left_scroll != null and left_scroll.size.x >= 280.0, "Columna izquierda >= 280px (w=%.1f)" % (left_scroll.size.x if left_scroll else 0.0))
	if cards_scroll != null:
		_check(cards_scroll.horizontal_scroll_mode == ScrollContainer.SCROLL_MODE_DISABLED, "Tarjetas sin scroll horizontal")

	# Verificar que Menu del juego muestra menu_frame y sus hijos indentados con flecha arriba
	var cards: Array = ed.get_design_cards_info()
	_check(cards.size() >= 10, "Lista 'Diseños' muestra todos los elementos de Menu del juego (%d tarjetas)" % cards.size())
	var c_mf := _find_card(cards, "menu_frame")
	var c_items := _find_card(cards, "menu_btn_items")
	_check(not c_mf.is_empty() and not c_items.is_empty(), "menu_frame y botón Objetos presentes como tarjetas")
	_check(int(c_items.get("depth", -1)) == 1 and str(c_items.get("parent_id", "")) == "menu_frame", "menu_btn_items aparece indentado como hijo de menu_frame")
	var card_items_ctl: Control = c_items.get("card_control", null)
	var card_mf_ctl: Control = c_mf.get("card_control", null)
	_check(card_items_ctl != null and _find_arrow_child(card_items_ctl) != null, "Tarjeta hija tiene flechita pequeña apuntando arriba (ClockGearsDesignChildArrow)")
	_check(card_mf_ctl != null and _find_arrow_child(card_mf_ctl) == null, "Tarjeta raíz no muestra flecha de hijo")

	# Tarjetas usan Label completo (ellipsis por carácter si hiciera falta) + tooltip
	var lbls := []
	if card_items_ctl != null:
		_find_controls_by_type(card_items_ctl, "Label", lbls)
		_check(not lbls.is_empty() and str((lbls[0] as Label).text) == str(c_items.get("name", "")), "Tarjeta muestra el nombre completo del elemento")

	# 2. Botones de ojo y candado dentro de la tarjeta funcionan
	if card_items_ctl != null:
		var btns := []
		_find_controls_by_type(card_items_ctl, "Button", btns)
		_check(btns.size() >= 2, "Tarjeta tiene botones discretos de ojo y candado")
		if btns.size() >= 2:
			var eye_btn: Button = btns[btns.size() - 2]
			var lock_btn: Button = btns[btns.size() - 1]
			eye_btn.emit_signal("pressed")
			await _wait_frames(2)
			_check(not bool(ed.get_effective_element("menu_btn_items").get("visible", true)), "Botón ojo de la tarjeta oculta el elemento")
			cards = ed.get_design_cards_info()
			card_items_ctl = ed.get_design_card_control("menu_btn_items")
			if card_items_ctl != null:
				btns.clear()
				_find_controls_by_type(card_items_ctl, "Button", btns)
				btns[btns.size() - 2].emit_signal("pressed")
				await _wait_frames(2)
				_check(bool(ed.get_effective_element("menu_btn_items").get("visible", false)), "Segundo clic en ojo vuelve a mostrar el elemento")
			card_items_ctl = ed.get_design_card_control("menu_btn_items")
			if card_items_ctl != null:
				btns.clear()
				_find_controls_by_type(card_items_ctl, "Button", btns)
				lock_btn = btns[btns.size() - 1]
				lock_btn.emit_signal("pressed")
				await _wait_frames(2)
				_check(bool(ed.get_effective_element("menu_btn_items").get("locked", false)), "Botón candado bloquea el elemento")
				card_items_ctl = ed.get_design_card_control("menu_btn_items")
				btns.clear()
				_find_controls_by_type(card_items_ctl, "Button", btns)
				btns[btns.size() - 1].emit_signal("pressed")
				await _wait_frames(2)

	# 3. Context menu -> Anadir hijo + nivel multinivel + menu contextual completo
	var ctx_menu: PopupMenu = h.get("layer_context_menu", null) as PopupMenu
	var add_sub: PopupMenu = h.get("add_child_submenu", null) as PopupMenu
	var rep_sub: PopupMenu = h.get("reparent_submenu", null) as PopupMenu
	_check(ctx_menu != null and add_sub != null and rep_sub != null and add_sub.item_count >= 10, "Menú contextual de tarjeta tiene submenús 'Añadir hijo' y 'Hacer hijo de...'")
	var added_child: String = ed.add_child_element_of_type("menu_btn_items", "text")
	await _wait_frames(2)
	cards = ed.get_design_cards_info()
	var c_grandchild := _find_card(cards, added_child)
	_check(not added_child.is_empty() and int(c_grandchild.get("depth", -1)) == 2, "Añadir hijo soporta múltiples niveles (nieto con depth=2)")

	# 4. Arrastrar tarjeta SOBRE otra tarjeta (drop_section == 0) la convierte en hija; mover padre mueve hijos
	var b1: String = ed.add_element_at("button", Vector2i(500, 180))
	var b2: String = ed.add_element_at("button", Vector2i(560, 260))
	await _wait_frames(2)
	var b1_ctl: Control = ed.get_design_card_control(b1)
	var drop_on_b1_ok: bool = ed.simulate_card_drop_at(b1, Vector2(20.0, b1_ctl.size.y * 0.5), PackedStringArray([b2]))
	_check(drop_on_b1_ok, "Arrastrar y soltar tarjeta SOBRE otra tarjeta (centro) funciona")
	await _wait_frames(2)
	cards = ed.get_design_cards_info()
	var c_b2 := _find_card(cards, b2)
	_check(str(c_b2.get("parent_id", "")) == b1 and int(c_b2.get("depth", -1)) == 1, "Soltar tarjeta sobre otra la vuelve su hija con flechita")

	# Al mover padre b1 con mouse_delta simulado (propiedades offset_x + 40), b2 sigue moviéndose con él
	var design_layer = ed.get_canvas_design_layer()
	var b2_ctl_canvas: Control = design_layer.get_element_control(b2)
	var b2_global_before: Vector2 = b2_ctl_canvas.global_position
	var b1_el: Dictionary = ed.get_effective_element(b1)
	b1_el["offset_x"] = int(b1_el.get("offset_x", 0)) + 40
	ed.update_dragged_element_fast(b1, b1_el)
	await _wait_frames(2)
	var b2_global_after: Vector2 = design_layer.get_element_control(b2).global_position
	_check(b2_global_after.x > b2_global_before.x + 20.0, "Mover elemento padre mueve también a sus hijos en el lienzo")

	# 5. Reordenar arrastrando ENTRE tarjetas (drop_section == -1 arriba / +1 abajo)
	var r1: String = ed.add_element_at("button", Vector2i(700, 140))
	var r2: String = ed.add_element_at("button", Vector2i(700, 220))
	await _wait_frames(2)
	var idx_r1_before := _index_of_card(ed.get_design_cards_info(), r1)
	var idx_r2_before := _index_of_card(ed.get_design_cards_info(), r2)
	_check(idx_r2_before < idx_r1_before, "Elemento nuevo queda arriba (al frente) en la lista")
	var r2_card: Control = ed.get_design_card_control(r2)
	ed.simulate_card_drop_at(r2, Vector2(20.0, r2_card.size.y * 0.92), PackedStringArray([r1]))
	await _wait_frames(2)
	cards = ed.get_design_cards_info()
	var idx_r1_after := _index_of_card(cards, r1)
	var idx_r2_after := _index_of_card(cards, r2)
	_check(idx_r1_after > idx_r2_after, "Soltar debajo de tarjeta reordena hermanos")

	# 6. Sacar del padre + Seleccionar en lienzo resalta tarjeta + doble clic abre ventana flotante
	_check(ed.unparent_element(b2), "Acción 'Sacar del padre' saca a b2 al nivel raíz")
	await _wait_frames(2)
	_check(str(ed.get_effective_element(b2).get("parent_id", "")) == "", "Tras sacar del padre, b2 está en nivel raíz")
	ed.select_element("menu_btn_items")
	await _wait_frames(2)
	var item_card_sel := _find_card(ed.get_design_cards_info(), "menu_btn_items")
	_check(bool(item_card_sel.get("selected", false)), "Seleccionar en lienzo resalta su tarjeta")
	ed.open_floating_element_window("menu_btn_items")
	await _wait_frames(2)
	_check(ed.get_open_floating_window_count() >= 1, "Ventana flotante de elemento abre desde la selección de tarjeta")
	ed.close_all_floating_element_windows()

	# 7. Persistencia guardar/recargar + limpiar datos temporales de prueba
	_check(dm.save_all(), "ui_designs.json guarda jerarquía sin errores")
	dm.load_all()
	ed.refresh_from_data_manager()
	await _wait_frames(2)
	var reloaded := _find_card(ed.get_design_cards_info(), added_child)
	_check(not reloaded.is_empty() and str(reloaded.get("parent_id", "")) == "menu_btn_items", "Jerarquía anidada persiste tras recargar DataManager")
	ed.request_reset_to_default(true)
	await _wait_frames(2)

	# 8. Todas las 12 pestañas en 960x540, 1366x768, 1920x1080, 2560x1440 sin cortes ni scroll horizontal en tarjetas
	var sizes := [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]
	var cats := ["title", "game_menu", "inventory", "skills", "equip", "status", "save_load", "options", "shop", "messages", "credits", "hud"]
	var res_ok := true
	for sz in sizes:
		dlg.size = sz
		await _wait_frames(2)
		for ck in cats:
			dlg.select_category_by_key(ck)
			await _wait_frames(1)
			var ced = dlg.get_ui_designer_editor()
			if ced == null:
				res_ok = false
				continue
			var ch: Dictionary = ced.get_ui_handles()
			var ls: ScrollContainer = ch.get("left_col_scroll", null) as ScrollContainer
			var dcs: ScrollContainer = ch.get("design_cards_scroll", null) as ScrollContainer
			if ls == null or ls.size.x < 279.0 or dcs == null or dcs.size.y < 100.0:
				res_ok = false
	_check(res_ok, "Todas las 12 pestañas en 960x540, 1366x768, 1920x1080 y 2560x1440 tienen columna izquierda >=280px sin cortes")

	dlg.queue_free()
	await _wait_frames(1)
	_finish()
