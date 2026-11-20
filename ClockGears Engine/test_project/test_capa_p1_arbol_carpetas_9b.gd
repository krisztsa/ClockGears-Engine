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

func _find_tree_item(root_item: TreeItem, target_id: String) -> TreeItem:
	if root_item == null:
		return null
	var stack: Array = [root_item]
	while not stack.is_empty():
		var cur: TreeItem = stack.pop_back()
		if cur != root_item and str(cur.get_metadata(0)) == target_id:
			return cur
		var ch := cur.get_first_child()
		while ch != null:
			stack.append(ch)
			ch = ch.get_next()
	return null

func _find_row(info: Array, elem_id: String) -> Dictionary:
	for item in info:
		if typeof(item) == TYPE_DICTIONARY and str(item.get("id", "")) == elem_id:
			return item
	return {}

func _finish(forced_code: int = -1) -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	if forced_code >= 0:
		quit(forced_code)
	else:
		quit(0 if fail_count == 0 else 1)

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
	root.add_child(dlg)
	dlg.open_database()
	await _wait_frames(3)

	dlg.select_category_by_key("game_menu")
	await _wait_frames(2)
	var ed = dlg.get_ui_designer_editor()
	_check(ed != null, "Editor de UI para Menu de pausa disponible")
	if ed == null:
		dlg.queue_free()
		_finish(1)
		return

	ed.select_screen("game_menu_main")
	await _wait_frames(3)

	var handles: Dictionary = ed.get_ui_handles()
	var layers_tree: Tree = handles.get("layers_tree", null)
	var menus_card: Control = handles.get("menus_card", null)
	var layers_card: Control = handles.get("layers_card", null)
	_check(layers_tree != null and menus_card != null and layers_card != null, "Panel izquierdo tiene bloque Disenos y arbol Capas")
	if layers_tree == null:
		dlg.queue_free()
		_finish(1)
		return

	_check(layers_card.size.y >= 180.0, "Tarjeta Capas ocupa espacio vertical generoso (h=%.1f)" % layers_card.size.y)

	# 1. Jerarquia logica por defecto en Menu de pausa
	var info: Array = ed.get_layer_tree_info()
	var mf_row := _find_row(info, "menu_frame")
	var btn_items_row := _find_row(info, "menu_btn_items")
	var pf_row := _find_row(info, "party_frame")
	var pl_row := _find_row(info, "party_list")
	_check(not mf_row.is_empty() and not btn_items_row.is_empty(), "Existen Panel Comandos y botones de menu")
	_check(str(btn_items_row.get("parent_id", "")) == "menu_frame" and int(btn_items_row.get("depth", 0)) >= 2, "Botones cuelgan jerarquicamente de Panel Comandos")
	_check(str(pl_row.get("parent_id", "")) == "party_frame" and int(pl_row.get("depth", 0)) >= 2, "Lista del grupo cuelga de Panel Grupo")

	# 2. Jerarquia logica en Equipo con carpetas de columnas
	ed.select_screen("game_menu_equip")
	await _wait_frames(3)
	var eq_info: Array = ed.get_layer_tree_info()
	var col_slots := _find_row(eq_info, "col_equip_slots")
	var eq_slots := _find_row(eq_info, "equip_slots")
	_check(str(col_slots.get("type", "")) == "folder" and str(col_slots.get("parent_id", "")) == "equip_panel_frame", "Equipo tiene carpeta Columna ranuras bajo Panel Equipo")
	_check(str(eq_slots.get("parent_id", "")) == "col_equip_slots" and int(eq_slots.get("depth", 0)) >= 3, "Huecos de equipo cuelgan de su carpeta de columna")

	# 3. Volver a Principal para pruebas interactivas del arbol
	ed.select_screen("game_menu_main")
	await _wait_frames(3)

	var ti_btn := _find_tree_item(layers_tree.get_root(), "menu_btn_items")
	_check(ti_btn != null and ti_btn.get_icon(0) != null and ti_btn.get_button_count(0) == 2, "Fila de arbol tiene icono por tipo + botones ojo y candado")

	# Ojo (visibilidad) y Candado (bloqueo) via button_clicked
	if ti_btn != null:
		layers_tree.emit_signal("button_clicked", ti_btn, 0, 0, 1)
		await _wait_frames(2)
		var el_vis: Dictionary = dm.get_ui_element("game_menu_main", "menu_btn_items")
		_check(not bool(el_vis.get("visible", true)), "Boton ojo oculta elemento")
		ti_btn = _find_tree_item(layers_tree.get_root(), "menu_btn_items")
		layers_tree.emit_signal("button_clicked", ti_btn, 0, 0, 1)
		await _wait_frames(2)

		ti_btn = _find_tree_item(layers_tree.get_root(), "menu_btn_items")
		layers_tree.emit_signal("button_clicked", ti_btn, 0, 1, 1)
		await _wait_frames(2)
		var el_lk: Dictionary = dm.get_ui_element("game_menu_main", "menu_btn_items")
		_check(bool(el_lk.get("locked", false)), "Boton candado bloquea elemento")
		# Desbloquear para seguir probando
		ti_btn = _find_tree_item(layers_tree.get_root(), "menu_btn_items")
		layers_tree.emit_signal("button_clicked", ti_btn, 0, 1, 1)
		await _wait_frames(2)

	# 4. Renombrado en linea
	ed.rename_element_inline("menu_gold", "Oro del Grupo")
	await _wait_frames(2)
	var gold_el: Dictionary = dm.get_ui_element("game_menu_main", "menu_gold")
	_check(str(gold_el.get("name", "")) == "Oro del Grupo", "Renombrado inline actualiza DataManager y arbol")

	# 5. Crear carpeta, agrupar (Ctrl+G), D&D meter en carpeta, D&D reordenar hermanos, D&D sacar a raiz
	var b1: String = ed.add_element_at("text", Vector2i(400, 100))
	var b2: String = ed.add_element_at("button", Vector2i(400, 160))
	await _wait_frames(2)
	var sel_arr := PackedStringArray([b1, b2])
	ed.select_elements(sel_arr, b2)
	await _wait_frames(2)
	# Verificar sincronizacion bidireccional de multiseleccion hacia Tree
	var ti_b1 := _find_tree_item(layers_tree.get_root(), b1)
	var ti_b2 := _find_tree_item(layers_tree.get_root(), b2)
	_check(ti_b1 != null and ti_b2 != null and ti_b1.is_selected(0) and ti_b2.is_selected(0), "Multiseleccion en lienzo/API se refleja en el Tree")

	var fid: String = ed.group_selected_into_folder("Bloque Prueba")
	await _wait_frames(2)
	_check(not fid.is_empty(), "Agrupar en carpeta crea carpeta con los elementos seleccionados")
	var el_b1: Dictionary = dm.get_ui_element("game_menu_main", b1)
	var el_b2: Dictionary = dm.get_ui_element("game_menu_main", b2)
	_check(str(el_b1.get("parent_id", "")) == fid and str(el_b2.get("parent_id", "")) == fid, "Elementos agrupados heredan parent_id de la carpeta")

	# Verificar orden arriba = mas al frente al reordenar en el arbol
	ed.reorder_layer_tree(PackedStringArray([b1]), b2, 1) # poner b1 debajo de b2
	await _wait_frames(2)
	el_b1 = dm.get_ui_element("game_menu_main", b1)
	el_b2 = dm.get_ui_element("game_menu_main", b2)
	_check(int(el_b2.get("layer_order", 0)) > int(el_b1.get("layer_order", 0)), "Arriba en arbol tiene mayor layer_order (mas al frente)")

	# Verificar prevencion de ciclo (no meter carpeta dentro de su hijo)
	_check(not ed.can_drop_in_layer_tree(PackedStringArray([fid]), b1, 0), "Previene ciclo al intentar meter padre dentro de hijo")

	# Meter b2 dentro de menu_frame (cualquier elemento puede tener hijos)
	ed.reorder_layer_tree(PackedStringArray([b2]), "menu_frame", 0)
	await _wait_frames(2)
	el_b2 = dm.get_ui_element("game_menu_main", b2)
	_check(str(el_b2.get("parent_id", "")) == "menu_frame", "Cualquier elemento (Marco) acepta hijos via D&D")

	# Sacar b2 a la raiz
	ed.reorder_layer_tree(PackedStringArray([b2]), "", -100)
	await _wait_frames(2)
	el_b2 = dm.get_ui_element("game_menu_main", b2)
	_check(str(el_b2.get("parent_id", "")) == "", "D&D permite sacar hijo a la raiz")

	# 6. Duplicar carpeta CON sus hijos
	ed.select_element(fid)
	await _wait_frames(2)
	_check(ed.duplicate_selected_element(), "Duplicar carpeta funciona")
	await _wait_frames(2)
	var fid_dup: String = ed.get_selected_element_id()
	var after_dup: Array = dm.get_ui_screen_elements("game_menu_main")
	var cloned_kids := 0
	for it in after_dup:
		if typeof(it) == TYPE_DICTIONARY and str(it.get("parent_id", "")) == fid_dup:
			cloned_kids += 1
	_check(fid_dup != fid and cloned_kids >= 1, "Duplicar carpeta clona sus hijos dentro de la carpeta duplicada")

	# 7. Desagrupar carpeta
	ed.select_element(fid)
	await _wait_frames(2)
	_check(ed.ungroup_selected_folder(), "Desagrupar carpeta seleccionada funciona")
	await _wait_frames(2)
	_check(dm.get_ui_element("game_menu_main", fid).is_empty(), "Carpeta eliminada tras desagrupar")
	el_b1 = dm.get_ui_element("game_menu_main", b1)
	_check(not el_b1.is_empty() and str(el_b1.get("parent_id", "")) == "", "Hijo promovido tras desagrupar")

	# 8. Requisito C: Fila superior fija y nunca cortada por scroll
	var top_bar_card := ed.find_child("UiDesignerTopBarCard", true, false) as Control
	_check(top_bar_card != null and top_bar_card.visible, "UiDesignerTopBarCard existe y esta visible")
	if top_bar_card != null:
		var has_scroll_anc := false
		var anc: Node = top_bar_card.get_parent()
		while anc != null:
			if anc is ScrollContainer:
				has_scroll_anc = true
				break
			anc = anc.get_parent()
		_check(not has_scroll_anc, "Barra superior del editor de UI NO esta dentro de ningun ScrollContainer")
		_check(top_bar_card.position.y >= 0.0 and top_bar_card.size.y >= 24.0, "Barra superior completa y no cortada por arriba (y=%.1f, h=%.1f)" % [top_bar_card.position.y, top_bar_card.size.y])

	# 9. Requisito B: Campos de texto, tooltips y menus con estilo unificado del tema (dark + brass)
	var th = Engine.get_singleton("ClockGearsTheme")
	_check(th != null, "ClockGearsTheme singleton disponible")
	if th != null:
		var ev_dlg := ClockGearsEventEditorDialog.new()
		root.add_child(ev_dlg)
		th.apply_to_window(ev_dlg)
		var pref_dlg := ClockGearsPreferencesDialog.new()
		root.add_child(pref_dlg)
		th.apply_to_window(pref_dlg)
		await _wait_frames(2)

		var prev_th_id: String = th.get_active_theme_id()
		for tid in ["dark", "brass"]:
			th.set_active_theme_id(tid)
			th.apply_to_window(dlg)
			th.apply_to_window(ev_dlg)
			th.apply_to_window(pref_dlg)
			await _wait_frames(1)

			var metrics: Dictionary = th.inspect_theme_metrics(tid)
			_check(bool(metrics.get("all_contrasts_pass_4_5", false)), "Tema %s cumple contraste >= 4.5:1 (min=%.2f en %s)" % [tid, float(metrics.get("min_contrast", 0.0)), str(metrics.get("min_contrast_pair", ""))])

			var aud_db: Dictionary = th.audit_inputs_and_popups_in_tree(dlg)
			var aud_ev: Dictionary = th.audit_inputs_and_popups_in_tree(ev_dlg)
			var aud_pf: Dictionary = th.audit_inputs_and_popups_in_tree(pref_dlg)
			_check(bool(aud_db.get("ok", false)) and int(aud_db.get("line_edits", 0)) >= 5, "Base de Datos + UI Designer pasan auditoria de inputs/tooltips/menus en %s (le=%d, sp=%d, pm=%d, fail=%s)" % [tid, int(aud_db.get("line_edits", 0)), int(aud_db.get("spin_boxes", 0)), int(aud_db.get("popup_menus", 0)), str(aud_db.get("failures", []))])
			_check(bool(aud_ev.get("ok", false)), "EventEditorDialog pasa auditoria de inputs/tooltips/menus en %s" % tid)
			_check(bool(aud_pf.get("ok", false)), "PreferencesDialog pasa auditoria de inputs/tooltips/menus en %s" % tid)

		th.set_active_theme_id(prev_th_id)
		ev_dlg.queue_free()
		pref_dlg.queue_free()

	# Restaurar estado limpio de test
	ed.request_reset_to_default(true)
	await _wait_frames(2)
	dlg.queue_free()
	await _wait_frames(2)
	_finish()
