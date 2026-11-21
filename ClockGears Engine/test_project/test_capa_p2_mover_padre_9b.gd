extends SceneTree

var pass_count: int = 0
var fail_count: int = 0
var _timed_out: bool = false

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + msg)
	else:
		fail_count += 1
		printerr("[FAIL] " + msg)

func _finish(code: int = -1) -> void:
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	var exit_code: int = code if code >= 0 else (0 if fail_count == 0 else 1)
	quit(exit_code)

func _initialize() -> void:
	var t := create_timer(60.0)
	t.timeout.connect(func():
		_timed_out = true
		printerr("[TIMEOUT] Test P2 excedió 60s")
		_check(false, "timeout_60s")
		_finish(1)
	)
	call_deferred("_run")

func _find_elem(elems: Array, eid: String) -> Dictionary:
	for e in elems:
		if e is Dictionary and str(e.get("id", "")) == eid:
			return e
	return {}

func _get_elems(dm, screen_id: String) -> Array:
	return dm.get_ui_screen_design(screen_id).get("elements", [])

func _get_elem(dm, screen_id: String, eid: String) -> Dictionary:
	return _find_elem(_get_elems(dm, screen_id), eid)

func _run() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager singleton disponible")
	if dm == null:
		_finish(1)
		return
	dm.load_all()

	var db_dlg := ClockGearsDatabaseDialog.new()
	db_dlg.exclusive = false
	get_root().add_child(db_dlg)
	db_dlg.open_database()
	await process_frame
	await process_frame
	db_dlg.select_category_by_key("title")
	await process_frame
	await process_frame

	var ed = db_dlg.get_ui_designer_editor()
	_check(ed != null, "Editor de UI obtenido desde DatabaseDialog")
	if ed == null:
		db_dlg.queue_free()
		_finish(1)
		return

	var handles: Dictionary = ed.get_ui_handles()
	var dl = ed.get_canvas_design_layer()
	_check(dl != null, "DesignLayer presente")

	# --- 1. Carpeta con 3 niveles + rueda radial anidada ---
	ed.select_screen("title")
	await process_frame

	var f1: String = ed.create_folder_element("Nivel1", "")
	var f2: String = ed.create_folder_element("Nivel2", f1)
	var f3: String = ed.create_folder_element("Nivel3", f2)
	var leaf: String = ed.add_element_at("text", Vector2i(100, 80))
	var rad_id: String = ed.add_element_at("radial", Vector2i(220, 120))
	ed.reorder_layer_tree(PackedStringArray([leaf, rad_id]), f3, 0)
	await process_frame

	var elems_before: Array = _get_elems(dm, "title")
	var stage_1x := Rect2(0, 0, 960, 540)
	var r_leaf_0: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(leaf, elems_before, stage_1x, 1.0)
	var r_rad_0: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(rad_id, elems_before, stage_1x, 1.0)

	var ef1: Dictionary = _get_elem(dm, "title", f1)
	ef1["offset_x"] = int(ef1.get("offset_x", 0)) + 60
	ef1["offset_y"] = int(ef1.get("offset_y", 0)) + 35
	ed.update_dragged_element_fast(f1, ef1)
	await process_frame

	var elems_after: Array = _get_elems(dm, "title")
	var r_leaf_1: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(leaf, elems_after, stage_1x, 1.0)
	var r_rad_1: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(rad_id, elems_after, stage_1x, 1.0)
	_check(abs(r_leaf_1.position.x - (r_leaf_0.position.x + 60.0)) < 1.5 and abs(r_leaf_1.position.y - (r_leaf_0.position.y + 35.0)) < 1.5, "Carpeta 3 niveles mueve nieto texto exacto (+60,+35)")
	_check(abs(r_rad_1.position.x - (r_rad_0.position.x + 60.0)) < 1.5 and abs(r_rad_1.position.y - (r_rad_0.position.y + 35.0)) < 1.5, "Carpeta mueve rueda radial hija (+60,+35)")

	var rad_ctrl = dl.get_element_control(rad_id)
	var leaf_ctrl = dl.get_element_control(leaf)
	var stage_ctrl: Control = ed.get_canvas_stage()
	var st_scale: float = (stage_ctrl.size.x / 960.0) if (stage_ctrl != null and stage_ctrl.size.x > 0.0) else 1.0
	var rad_node = rad_ctrl.find_child("ElemRadialCanvas", true, false) if is_instance_valid(rad_ctrl) else null
	var dl_sc: Vector2 = dl.get_global_transform().get_scale() if is_instance_valid(dl) else Vector2.ONE
	var rad_pos_x_diff: float = abs((rad_ctrl.position.x - leaf_ctrl.position.x) / max(0.01, (dl.size.x / 960.0)) - (r_rad_1.position.x - r_leaf_1.position.x)) if (is_instance_valid(rad_ctrl) and is_instance_valid(leaf_ctrl)) else 999.0
	_check(rad_node != null, "ElemRadialCanvas montado dentro de rueda hija")
	_check(rad_pos_x_diff < 3.0, "Render real sincronizado al arrastrar carpeta (diff=%.2f, dl_w=%.1f, st_w=%.1f)" % [rad_pos_x_diff, dl.size.x, stage_ctrl.size.x])

	# --- 2. Redimensionar padre sin auto-layout preserva tamaño/pos relativa de hijos ---
	var pnl: String = ed.add_element_at("frame", Vector2i(40, 40))
	var dpnl: Dictionary = _get_elem(dm, "title", pnl)
	dpnl["offset_x"] = 40
	dpnl["offset_y"] = 40
	dpnl["width"] = 300
	dpnl["height"] = 200
	ed.update_dragged_element_fast(pnl, dpnl)

	var child_btn: String = ed.add_element_at("button", Vector2i(80, 70))
	ed.reorder_layer_tree(PackedStringArray([child_btn]), pnl, 0)
	await process_frame
	var dch_0: Dictionary = _get_elem(dm, "title", child_btn)
	var ch_w0: int = int(dch_0.get("width", 0))
	var ch_h0: int = int(dch_0.get("height", 0))
	var ch_ox0: int = int(dch_0.get("offset_x", 0))
	var ch_oy0: int = int(dch_0.get("offset_y", 0))

	dpnl = _get_elem(dm, "title", pnl)
	dpnl["width"] = 420
	dpnl["height"] = 280
	ed.update_dragged_element_fast(pnl, dpnl)
	await process_frame
	var dch_1: Dictionary = _get_elem(dm, "title", child_btn)
	_check(int(dch_1.get("width", 0)) == ch_w0 and int(dch_1.get("height", 0)) == ch_h0, "Redimensionar padre libre no estira hijos")
	_check(int(dch_1.get("offset_x", 0)) == ch_ox0 and int(dch_1.get("offset_y", 0)) == ch_oy0, "Redimensionar padre libre conserva posición relativa")

	# --- 3. Auto-layout en padre rejilla recalcula hijos al redimensionar ---
	var ch_b2: String = ed.add_element_at("button", Vector2i(200, 70))
	ed.reorder_layer_tree(PackedStringArray([ch_b2]), pnl, 0)
	await process_frame
	dpnl = _get_elem(dm, "title", pnl)
	dpnl["layout_mode"] = "grid"
	dpnl["layout_columns"] = 2
	dpnl["layout_spacing"] = 10
	ed.update_dragged_element_fast(pnl, dpnl)
	await process_frame
	var rg_0: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(child_btn, _get_elems(dm, "title"), stage_1x, 1.0)
	dpnl["width"] = 620
	ed.update_dragged_element_fast(pnl, dpnl)
	await process_frame
	var rg_1: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(child_btn, _get_elems(dm, "title"), stage_1x, 1.0)
	_check(rg_1.size.x > rg_0.size.x + 50.0, "Padre con auto-layout rejilla reordena tamaño de hijos al redimensionar")

	# quitar auto-layout y activar clip_children usando el inspector real
	dpnl["layout_mode"] = "free"
	dm.update_ui_element("title", pnl, dpnl, false)
	ed.refresh_from_data_manager()
	ed.select_element(pnl)
	await process_frame
	var clip_chk: CheckBox = handles.get("prop_clip_children_chk", null)
	_check(clip_chk != null, "CheckBox prop_clip_children_chk disponible")
	if clip_chk != null:
		clip_chk.button_pressed = true
	await process_frame
	var pnl_ctrl = dl.get_element_control(pnl)
	var ch_ctrl = dl.get_element_control(child_btn)
	_check(pnl_ctrl != null and pnl_ctrl.clip_contents, "clip_children activa clip_contents en padre")
	_check(ch_ctrl != null and ch_ctrl.get_parent() == pnl_ctrl, "clip_children emparenta visualmente al hijo bajo el padre")

	# --- 4. Ocultar y bloquear padre ---
	ed.toggle_element_visibility(pnl)
	await process_frame
	var ch_ctrl_after = dl.get_element_control(child_btn)
	_check(not ClockGearsUiDesignLayer.is_element_effectively_visible(child_btn, _get_elems(dm, "title")), "Ocultar padre oculta hijos en cadena")
	_check(not is_instance_valid(ch_ctrl_after) or not ch_ctrl_after.is_visible_in_tree(), "Control hijo invisible cuando padre está oculto")
	ed.toggle_element_visibility(pnl)
	ed.toggle_element_locked(pnl)
	await process_frame
	_check(ClockGearsUiDesignLayer.is_element_effectively_locked(child_btn, _get_elems(dm, "title")), "Bloquear padre bloquea hijos")
	ed.select_element(child_btn)
	_check(not ed.delete_selected_element_with_mode(0), "Hijo bajo padre bloqueado no se borra")
	ed.toggle_element_locked(pnl)
	await process_frame

	# --- 5. Borrar padre: diálogo + Sacar hijos a la raíz + Undo + Borrar hijos también ---
	ed.select_element(pnl)
	var g_before_del: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(child_btn, _get_elems(dm, "title"), stage_1x, 1.0)
	ed.delete_selected_element() # Debe abrir confirmación sin borrar aún
	await process_frame
	var del_dlg = handles.get("delete_parent_dialog", null)
	_check(del_dlg != null and del_dlg.visible, "Borrar padre con hijos muestra diálogo de opciones")
	if del_dlg != null:
		del_dlg.hide()

	_check(ed.delete_selected_element_with_mode(0), "Modo 0 (Sacar hijos a la raíz) borra solo al padre")
	await process_frame
	var e_after_extract: Array = _get_elems(dm, "title")
	var ch_after_ex: Dictionary = _find_elem(e_after_extract, child_btn)
	var g_after_extract: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect(child_btn, e_after_extract, stage_1x, 1.0)
	_check(_find_elem(e_after_extract, pnl).is_empty() and not ch_after_ex.is_empty(), "Padre eliminado e hijo conservado")
	_check(str(ch_after_ex.get("parent_id", "")) == "", "Hijo quedó en la raíz")
	_check(abs(g_after_extract.position.x - g_before_del.position.x) < 2.0 and abs(g_after_extract.position.y - g_before_del.position.y) < 2.0, "Hijo conserva posición global tras sacar a la raíz")

	# Undo restaura padre + hijo emparentado
	ed.apply_undo()
	await process_frame
	var ch_undo: Dictionary = _get_elem(dm, "title", child_btn)
	_check(not _get_elem(dm, "title", pnl).is_empty() and str(ch_undo.get("parent_id", "")) == pnl, "Undo restaura padre e hijo emparentado")

	# Modo 1: Borrar hijos también
	ed.select_element(pnl)
	_check(ed.delete_selected_element_with_mode(1), "Modo 1 (Borrar hijos también) ejecuta correctamente")
	await process_frame
	_check(_get_elem(dm, "title", pnl).is_empty() and _get_elem(dm, "title", child_btn).is_empty() and _get_elem(dm, "title", ch_b2).is_empty(), "Modo 1 borra padre y todos sus hijos")

	# --- 6. Game Menu F5 con jerarquía ---
	ed.select_screen("game_menu_main")
	await process_frame
	var mf: Dictionary = _get_elem(dm, "game_menu_main", "menu_frame")
	if not mf.is_empty():
		var b_item0: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect("btn_items", _get_elems(dm, "game_menu_main"), stage_1x, 1.0)
		ed.toggle_element_visibility("menu_frame")
		ed.toggle_element_visibility("menu_frame")
		mf = _get_elem(dm, "game_menu_main", "menu_frame")
		mf["offset_x"] = int(mf.get("offset_x", 0)) + 25
		ed.update_dragged_element_fast("menu_frame", mf)
		await process_frame
		var b_item1: Rect2 = ClockGearsUiDesignLayer.compute_element_global_rect("btn_items", _get_elems(dm, "game_menu_main"), stage_1x, 1.0)
		_check(abs(b_item1.position.x - (b_item0.position.x + 25.0)) < 1.5, "Game Menu: mover menu_frame mueve hijos btn_items")
	var tw = ed.open_test_popup()
	await process_frame
	_check(tw != null and tw.visible, "F5 popup abre con jerarquía modificada")
	ed.close_test_popup()
	await process_frame

	# --- 7. Migración proyecto antiguo (schema v2 -> v3 + .bak + colocación idéntica) ---
	var proj_dir: String = ProjectSettings.globalize_path("res://")
	var data_dir: String = proj_dir.path_join("clockgears_data")
	var ui_path: String = data_dir.path_join("ui_designs.json")
	var orig_exists: bool = FileAccess.file_exists(ui_path)
	var orig_text: String = FileAccess.get_file_as_string(ui_path) if orig_exists else ""

	var legacy_dict := {
		"schema_version": 2,
		"screens": {
			"title": {
				"screen_id": "title",
				"customized": true,
				"elements": [
					{"id": "legacy_root_txt", "name": "Legacy Txt", "type": "text", "anchor": "top_left", "offset_x": 123, "offset_y": 77, "width": 180, "height": 30, "text": "Hola", "layer_order": 5}
				]
			}
		}
	}
	DirAccess.make_dir_recursive_absolute(data_dir)
	var fw := FileAccess.open(ui_path, FileAccess.WRITE)
	fw.store_string(JSON.stringify(legacy_dict, "\t"))
	fw.close()

	dm.load_all()
	await process_frame
	var bak_path: String = ui_path + ".pre_hierarchy_v3.bak"
	_check(FileAccess.file_exists(bak_path), "Migración v2->v3 crea respaldo .pre_hierarchy_v3.bak")
	var title_d: Dictionary = dm.get_ui_screen_design("title")
	var leg_el: Dictionary = _find_elem(title_d.get("elements", []), "legacy_root_txt")
	_check(int(leg_el.get("offset_x", -1)) == 123 and int(leg_el.get("offset_y", -1)) == 77, "Migración conserva colocación exacta de elementos raíz antiguos")

	# Restaurar estado limpio del proyecto
	DirAccess.remove_absolute(bak_path)
	if orig_exists:
		var rfw := FileAccess.open(ui_path, FileAccess.WRITE)
		rfw.store_string(orig_text)
		rfw.close()
	else:
		DirAccess.remove_absolute(ui_path)

	db_dlg.queue_free()
	_finish()
