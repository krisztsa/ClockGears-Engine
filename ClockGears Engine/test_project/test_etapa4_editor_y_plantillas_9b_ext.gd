extends SceneTree

var passed: int = 0
var failed: int = 0

var _db: ClockGearsDatabaseDialog = null
var _has_backup: bool = false
var _backup: Dictionary = {}
var _prev_rt: Dictionary = {}
var _res_data_dir := "res://clockgears_data"

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

func _snapshot_data_dir(res_dir: String) -> Dictionary:
	var out: Dictionary = {"files": {}}
	var da := DirAccess.open(res_dir)
	if da == null:
		return out
	da.list_dir_begin()
	var fn := da.get_next()
	while fn != "":
		if not da.current_is_dir():
			var p := res_dir.path_join(fn)
			var f := FileAccess.open(p, FileAccess.READ)
			if f != null:
				out["files"][fn] = f.get_buffer(f.get_length())
		fn = da.get_next()
	return out

func _restore_data_dir(res_dir: String, snap: Dictionary) -> void:
	var files: Dictionary = snap.get("files", {})
	var da := DirAccess.open(res_dir)
	if da != null:
		da.list_dir_begin()
		var fn := da.get_next()
		while fn != "":
			if not da.current_is_dir() and not files.has(fn):
				da.remove(fn)
			fn = da.get_next()
	for fn in files.keys():
		var f := FileAccess.open(res_dir.path_join(fn), FileAccess.WRITE)
		if f != null:
			f.store_buffer(files[fn])

func _has_visible_hscroll(node: Node) -> bool:
	if node == null:
		return false
	if node is ScrollContainer:
		var sc := node as ScrollContainer
		var hs := sc.get_h_scroll_bar()
		if hs != null and hs.visible and hs.max_value > hs.page + 1.0:
			return true
	for c in node.get_children():
		if _has_visible_hscroll(c):
			return true
	return false

func _find_clipped_control(node: Node, max_w: float) -> String:
	if node == null:
		return ""
	if node is Control:
		var c := node as Control
		if c.is_visible_in_tree():
			var gr := c.get_global_rect()
			if gr.position.x + gr.size.x > max_w + 4.0:
				return "%s (right=%.1f > %.1f)" % [c.name, gr.position.x + gr.size.x, max_w]
	for ch in node.get_children():
		var hit := _find_clipped_control(ch, max_w)
		if not hit.is_empty():
			return hit
	return ""

func _initialize() -> void:
	call_deferred("_run_tests")

func _finish(code: int) -> void:
	if _db != null and is_instance_valid(_db):
		_db.queue_free()
		_db = null
		await _step_frames(2)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm != null and _has_backup:
		dm.import_runtime_state(_prev_rt)
		_restore_data_dir(_res_data_dir, _backup)
		dm.load_all()
	for p in [
		"res://img/ui/_test_etapa4_icon.png",
		"res://img/ui/_test_etapa4_icon.png.import",
		"user://_test_etapa4_pkg.cgmenu",
		"res://img/ui/imported_test4/_test_etapa4_icon.png",
	]:
		var abs_p := ProjectSettings.globalize_path(p)
		if FileAccess.file_exists(abs_p):
			DirAccess.remove_absolute(abs_p)
	var imp_dir := ProjectSettings.globalize_path("res://img/ui/imported_test4")
	if DirAccess.dir_exists_absolute(imp_dir):
		DirAccess.remove_absolute(imp_dir)

	print("RESUMEN TEST ETAPA 4: PASSED=%d FAILED=%d" % [passed, failed])
	quit(code)

func _run_tests() -> void:
	print("=== TEST ETAPA 4 (9B-EXT): EDITOR INTUITIVO Y PLANTILLAS ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager disponible")
	if dm == null:
		_finish(1)
		return

	_backup = _snapshot_data_dir(_res_data_dir)
	_prev_rt = dm.export_runtime_state()
	_has_backup = true

	var db := ClockGearsDatabaseDialog.new()
	_db = db
	root.add_child(db)
	db.popup_centered(Vector2i(1366, 768))
	await _step_frames(3)
	db.select_category_by_key("ui_designer")
	await _step_frames(3)

	var editor: ClockGearsUiDesignEditor = db.get_ui_designer_editor()
	var h: Dictionary = db.get_ui_designer_ui_handles()
	check(editor != null and not h.is_empty(), "Editor visual abierto correctamente con nuevos controles")
	if editor == null:
		_finish(1)
		return

	var ui_panel: Control = h["panel"]

	# --- 1. Asistente de 3 pasos (4A) + F5 runtime ---
	print("[1] Asistente 3 pasos y funcionamiento en juego")
	var wiz_dlg: AcceptDialog = h["menu_wizard_dialog"]
	check(wiz_dlg != null and h["new_menu_wizard_btn"] != null and h["menus_list_vbox"] != null,
		"Existen boton '+ Nuevo menú', panel 'Menús' y diálogo asistente")

	var new_mid: String = editor.create_menu_via_wizard({
		"name": "Menú Mágico",
		"template_id": "classic_jrpg",
		"buttons": [
			{"label": "Mochila", "action": "open_items"},
			{"label": "Salir", "action": "close_menu"}
		],
		"triggers": [{"kind": "key", "keycode": KEY_M}],
		"trigger_context": "map_only",
		"pause_on_open": true,
	})
	await _step_frames(2)
	check(not new_mid.is_empty() and dm.has_menu(new_mid) and editor.get_current_screen() == new_mid,
		"Asistente crea menú '%s' funcional y lo abre en el editor" % new_mid)

	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	await _step_frames(1)
	var kev := InputEventKey.new()
	kev.keycode = KEY_M
	kev.pressed = true
	var opened_by_key: bool = gm.handle_trigger_input_event(kev, "map")
	await _step_frames(2)
	check(opened_by_key and gm.is_menu_open() and gm.get_active_menu_id() == new_mid,
		"El menú creado con el asistente abre en F5 pulsando la tecla asignada (M)")
	gm.close_menu()
	gm.queue_free()

	# --- 2. Las 12 plantillas de menú + 5 de componente + Undo ---
	print("[2] Aplicar las 12 plantillas de pantalla, 5 componentes y Deshacer")
	var built_menus: Array = ClockGearsDataManager.get_builtin_menu_templates()
	check(built_menus.size() == 12, "Catálogo tiene exactamente 12 plantillas de menú (actual=%d)" % built_menus.size())
	var before_elems_count: int = dm.get_ui_screen_elements(new_mid).size()
	var all_12_ok := true
	for t in built_menus:
		var tid := String(t.get("id", ""))
		var ok := editor.apply_template_by_id(tid, true, true)
		await _step_frames(1)
		var d_cur: Dictionary = dm.get_ui_screen_design(new_mid)
		var lay_cur: Dictionary = d_cur.get("layout", {})
		var has_content: bool = (not dm.get_ui_screen_elements(new_mid).is_empty()) or (String(lay_cur.get("mode", "")) == "radial" and not Array(lay_cur.get("radial_slots", [])).is_empty())
		if not ok or not has_content:
			all_12_ok = false
			printerr("    Fallo plantilla pantalla: " + tid)
	check(all_12_ok, "Las 12 plantillas integradas se aplican sin errores conservando elementos")
	check(editor.apply_undo() and dm.get_ui_screen_elements(new_mid).size() > 0, "Deshacer revierte plantilla cleanly")
	for _u in range(11):
		editor.apply_undo()
	await _step_frames(1)
	check(dm.get_ui_screen_elements(new_mid).size() == before_elems_count,
		"Deshacer 12 veces restaura el menú previo al estado exacto anterior")

	var built_comps: Array = ClockGearsDataManager.get_builtin_component_templates()
	check(built_comps.size() == 5, "Catálogo tiene 5 plantillas de componente (actual=%d)" % built_comps.size())
	var comp_ok := true
	for c in built_comps:
		var cid := String(c.get("id", ""))
		if not editor.apply_template_by_id(cid, true, true):
			comp_ok = false
	check(comp_ok, "Las 5 plantillas de componente se instancian en el lienzo")

	# --- 3. Guardar plantilla propia + Exportar e Importar paquete portable ---
	print("[3] Guardar plantilla propia y exportar/importar paquete portable")
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("res://img/ui"))
	var icon_img := Image.create(16, 16, false, Image.FORMAT_RGBA8)
	icon_img.fill(Color(0.95, 0.72, 0.30, 1.0))
	icon_img.save_png(ProjectSettings.globalize_path("res://img/ui/_test_etapa4_icon.png"))

	var img_eid: String = editor.add_element_at("image", Vector2i(200, 140))
	var iel: Dictionary = dm.get_ui_element(new_mid, img_eid)
	iel["image_path"] = "res://img/ui/_test_etapa4_icon.png"
	dm.update_ui_element(new_mid, img_eid, iel, false)

	var utid: String = editor.save_current_as_user_template("Plantilla Prueba Áurea")
	check(not utid.is_empty(), "Guardar pantalla actual como plantilla propia ('%s')" % utid)

	var pkg_path := ProjectSettings.globalize_path("user://_test_etapa4_pkg.cgmenu")
	var exported: bool = dm.export_template_package(utid, pkg_path, false)
	check(exported and FileAccess.file_exists(pkg_path), "Exportar paquete portable con recursos incrustados")

	# Simular otro proyecto: borrar la plantilla propia y el png original, luego importar
	dm.delete_user_template(utid)
	DirAccess.remove_absolute(ProjectSettings.globalize_path("res://img/ui/_test_etapa4_icon.png"))
	var imp_res: Dictionary = dm.import_template_package(pkg_path, true, "res://img/ui/imported_test4")
	check(bool(imp_res.get("ok", false)) and FileAccess.file_exists(ProjectSettings.globalize_path("res://img/ui/imported_test4/_test_etapa4_icon.png")),
		"Importar paquete portable en proyecto limpio extrae el PNG e incorpora la plantilla/menú")

	# --- 4. Propiedades Simple/Avanzado, Buscador, Edición directa y Probar ---
	print("[4] Propiedades, buscador, edición directa, radial, scroll fondo y Probar")
	editor.select_element(img_eid)
	await _step_frames(1)
	editor.set_props_advanced_mode(false)
	var cnt_simple: int = editor.get_visible_prop_row_count()
	editor.set_props_advanced_mode(true)
	var cnt_adv: int = editor.get_visible_prop_row_count()
	check(cnt_adv > cnt_simple and cnt_simple > 0,
		"Modo Simple (%d filas) vs Avanzado (%d filas) oculta opciones complejas" % [cnt_simple, cnt_adv])

	editor.set_props_search_filter("radio")
	var cnt_search: int = editor.get_visible_prop_row_count()
	check(cnt_search >= 1 and cnt_search < cnt_simple,
		"Buscador de propiedades filtra correctamente (%d filas para 'radio')" % cnt_search)
	editor.set_props_search_filter("")

	# Edición directa: duplicar, centrar, copiar/pegar entre menús, edición en línea
	var dup_ok: bool = editor.duplicate_selected_element()
	check(dup_ok, "Duplicar elemento seleccionado funciona")
	var align_ok: bool = editor.align_selected_elements("center_both")
	var moved_el: Dictionary = dm.get_ui_element(new_mid, editor.get_selected_element_id())
	check(align_ok and int(moved_el.get("offset_x", -1)) > 200, "Alinear centrado horizontal/vertical funciona")
	editor.copy_selected_to_clipboard()
	editor.select_screen("title")
	var pasted: PackedStringArray = editor.paste_from_clipboard()
	check(pasted.size() == 1, "Copiar/Pegar entre menús distintos funciona")
	editor.select_screen(new_mid)
	check(editor.add_radial_slot_from_editor(), "'+ Añadir ranura' radial crea y registra botón en anillo")
	check(editor.begin_inline_edit_for_selected(), "Edición de texto en línea en el lienzo abre el LineEdit sobre botones/texto")

	# Fondo con scroll + transición en runtime y Probar popup con datos de ejemplo
	var md: Dictionary = dm.get_ui_screen_design(new_mid)
	md["background"] = {"type": "gradient", "color": "#1c2438", "gradient_bottom": "#0e1220", "dark_veil": true, "scroll_speed_x": 40, "scroll_speed_y": 20}
	md["transition_in"] = {"type": "fade", "duration_ms": 150, "curve": "ease_out"}
	dm.set_ui_screen_design(new_mid, md, false)
	dm.set_gold(0)
	var test_win: Window = editor.open_test_popup()
	await _step_frames(3)
	var tinfo: Dictionary = editor.get_test_popup_info()
	check(bool(tinfo.get("open", false)) and dm.get_gold() == 2500 and test_win != null,
		"Botón 'Probar' abre popup en vivo e inyecta datos de ejemplo temporales (oro=%d)" % dm.get_gold())
	editor.close_test_popup()
	await _step_frames(2)
	check(dm.get_gold() == 0, "Cerrar 'Probar' restaura exactamente el estado anterior de partida")

	# --- 5. Regla 16:9 y tamaño mínimo sin cortes ni scroll horizontal ---
	print("[5] Validación 16:9 (1366x768, 1920x1080, 2560x1440) y mínimo Base de Datos")
	for sz in [Vector2i(1024, 600), Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		db.size = sz
		db.adapt_to_window_size(sz)
		await _step_frames(3)
		var clip := _find_clipped_control(ui_panel, float(sz.x))
		var hscr := _has_visible_hscroll(ui_panel)
		check(not hscr and clip.is_empty(),
			"Sin solapes/recortes ni scroll horizontal a %dx%d (clip='%s')" % [sz.x, sz.y, clip])

	# --- 6. Rendimiento: mover widget con 100 widgets en el lienzo ---
	print("[6] Rendimiento de arrastre con 100 widgets en pantalla")
	var perf_mid: String = editor.create_menu_via_wizard({"name": "Perf 100", "template_id": "minimalist", "buttons": []})
	var target_eid := ""
	for i in range(100):
		var eid: String = editor.add_element_at("text", Vector2i(40 + (i % 10) * 84, 30 + int(i / 10) * 44))
		if i == 0:
			target_eid = eid
	await _step_frames(2)
	var drag_el: Dictionary = dm.get_ui_element(perf_mid, target_eid)
	var t0: int = Time.get_ticks_usec()
	for s in range(30):
		drag_el["offset_x"] = 40 + s * 4
		drag_el["offset_y"] = 40 + s * 2
		editor.update_dragged_element_fast(target_eid, drag_el)
	var elapsed_ms: float = float(Time.get_ticks_usec() - t0) / 1000.0
	var per_step_ms: float = elapsed_ms / 30.0
	check(per_step_ms < 16.7 and dm.get_ui_screen_elements(perf_mid).size() >= 100,
		"Mover entre 100 widgets en %0.2f ms/paso (total %0.2f ms para 30 pasos < 16.7ms)" % [per_step_ms, elapsed_ms])

	_finish(0 if failed == 0 else 1)
