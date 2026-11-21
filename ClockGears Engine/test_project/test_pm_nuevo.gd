extends SceneTree

var pass_count: int = 0
var fail_count: int = 0
var start_ms: int = 0
var aborted: bool = false

func _check(cond: bool, name: String, detail: String = "") -> void:
	if cond:
		pass_count += 1
		print("[PASS] " + name)
	else:
		fail_count += 1
		print("[FAIL] " + name + ("" if detail.is_empty() else " -> " + detail))

func _check_deadline() -> bool:
	if Time.get_ticks_msec() - start_ms > 55000:
		fail_count += 1
		print("[FAIL] Timeout interno (>55s)")
		aborted = true
		return true
	return false

func _step_frames(n: int = 3) -> void:
	for i in range(n):
		if _check_deadline():
			return
		await process_frame

func _find_controls_by_class(node: Node, cls_name: String, out: Array) -> void:
	if node.get_class() == cls_name:
		out.append(node)
	for c in node.get_children():
		_find_controls_by_class(c, cls_name, out)

func _find_button_by_text(root: Node, target_text: String) -> Button:
	var btns: Array = []
	_find_controls_by_class(root, "Button", btns)
	for b in btns:
		if b is Button and b.text == target_text:
			return b
	return null

func _find_labels_with_text(root: Node, substr: String) -> Array:
	var lbls: Array = []
	_find_controls_by_class(root, "Label", lbls)
	var hits: Array = []
	for l in lbls:
		if l is Label and substr in l.text:
			hits.append(l)
	return hits

func _write_dummy_project(dir_abs: String, proj_name: String, tags: PackedStringArray = PackedStringArray(), features: PackedStringArray = PackedStringArray()) -> void:
	DirAccess.make_dir_recursive_absolute(dir_abs)
	var cfg := ConfigFile.new()
	cfg.set_value("application", "config/name", proj_name)
	if not features.is_empty():
		cfg.set_value("application", "config/features", features)
	else:
		cfg.set_value("application", "config/features", PackedStringArray(["4.6", "Forward Plus"]))
	if not tags.is_empty():
		cfg.set_value("application", "config/tags", tags)
	cfg.save(dir_abs.path_join("project.godot"))

func _write_projects_cfg(cfg_path: String, paths: PackedStringArray, favorites: PackedStringArray = PackedStringArray()) -> void:
	var cfg := ConfigFile.new()
	for p in paths:
		cfg.set_value(p, "favorite", favorites.has(p))
	cfg.save(cfg_path)

func _rm_dir_recursive(dir_abs: String) -> void:
	var da := DirAccess.open(dir_abs)
	if da == null:
		return
	da.list_dir_begin()
	var f := da.get_next()
	while f != "":
		if f != "." and f != "..":
			var sub := dir_abs.path_join(f)
			if da.current_is_dir():
				_rm_dir_recursive(sub)
			else:
				DirAccess.remove_absolute(sub)
		f = da.get_next()
	da.list_dir_end()
	DirAccess.remove_absolute(dir_abs)

func _assert_no_cuts_or_overlaps(pm: ProjectManager, res: Vector2i, tag: String) -> void:
	root.size = res
	pm.size = res
	await _step_frames(4)
	var m: Dictionary = pm.inspect_layout_metrics()
	var overflows: PackedStringArray = m.get("horizontal_overflows", PackedStringArray())
	var sb_vis: bool = bool(m.get("sidebar_visible", false))
	var hscroll_vis: bool = bool(m.get("hscroll_visible", false))
	var search_rect: Rect2 = m.get("search_rect", Rect2())
	var settings_rect: Rect2 = m.get("settings_rect", Rect2())
	var list_rect: Rect2 = m.get("list_panel_rect", Rect2())
	var sb_rect: Rect2 = m.get("sidebar_rect", Rect2())

	var no_header_overlap := (search_rect.position.x + search_rect.size.x) <= (settings_rect.position.x + 1.0)
	var no_split_overlap := (list_rect.position.x + list_rect.size.x) <= (sb_rect.position.x + 2.0)
	var ok := overflows.is_empty() and sb_vis and (not hscroll_vis) and no_header_overlap and no_split_overlap
	var detail := ""
	if not ok:
		detail = "overflows=%s sb_vis=%s hscroll=%s search=%s settings=%s list=%s sb=%s" % [str(overflows), str(sb_vis), str(hscroll_vis), str(search_rect), str(settings_rect), str(list_rect), str(sb_rect)]
	_check(ok, "Layout %s @ %dx%d (sin cortes/solapes/scrollH)" % [tag, res.x, res.y], detail)

func _initialize() -> void:
	start_ms = Time.get_ticks_msec()
	var watchdog := create_timer(55.0)
	watchdog.timeout.connect(func():
		fail_count += 1
		print("[FAIL] Watchdog timeout 55s")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
	)
	call_deferred("_run")

func _run() -> void:
	var args := OS.get_cmdline_user_args()
	var render_shots := args.has("--render-shots")
	var base_tmp := ProjectSettings.globalize_path("res://_tmp_pm_test").simplify_path()
	var shots_dir := ProjectSettings.globalize_path("res://_tmp_pm_shots").simplify_path()
	_rm_dir_recursive(base_tmp)
	DirAccess.make_dir_recursive_absolute(base_tmp)
	if render_shots:
		DirAccess.make_dir_recursive_absolute(shots_dir)

	var tmp_cfg := base_tmp.path_join("test_projects.cfg")

	# Crear PM inicial en escena real
	var pm: ProjectManager = ProjectManager.new()
	root.add_child(pm)
	await _step_frames(5)

	# Verificar titulo de ventana y footer
	var m0: Dictionary = pm.inspect_layout_metrics()
	_check(String(m0.get("window_title", "")) == "ClockGears Engine - Proyectos", "Título de ventana exacto", str(m0.get("window_title", "")))

	var footer_off := _find_labels_with_text(pm, "Modo sin conexión")
	var footer_link_ok := false
	for l in footer_off:
		if "Comprobación de actualizaciones deshabilitada" in l.text:
			footer_link_ok = true
	if not footer_link_ok:
		var link_btns: Array = []
		_find_controls_by_class(pm, "EngineUpdateLabel", link_btns)
		_find_controls_by_class(pm, "LinkButton", link_btns)
		for lb_node in link_btns:
			var lb := lb_node as LinkButton
			if lb != null and "Modo sin conexión" in lb.text and "deshabilitada" in lb.text:
				footer_link_ok = true
	_check(footer_link_ok, "Footer izquierda: Modo sin conexión y ortografía 'deshabilitada'")

	var ver_btns: Array = []
	_find_controls_by_class(pm, "EditorVersionButton", ver_btns)
	var ver_btn := ver_btns[0] as LinkButton if not ver_btns.is_empty() else null
	var ver_text: String = ver_btn.text if ver_btn != null else ""
	var ver_ok: bool = ver_btn != null and "Alpha-0.0.4" in ver_text
	_check(ver_ok, "Footer derecha: Alpha-0.0.4 visible", ver_text if not ver_text.is_empty() else "missing")

	# Verificar botones esperados y flechita de opciones
	_check(_find_button_by_text(pm, "+ Crear Proyecto") != null, "Botón '+ Crear Proyecto' existe")
	_check(_find_button_by_text(pm, "Importar") != null, "Botón 'Importar' existe")
	_check(_find_button_by_text(pm, "Configuración") != null, "Botón 'Configuración' existe")
	_check(_find_button_by_text(pm, "Editar Proyecto") != null, "Botón 'Editar Proyecto' existe")
	_check(_find_button_by_text(pm, "Ejecutar / Probar") != null, "Botón 'Ejecutar / Probar' existe")
	_check(_find_button_by_text(pm, "Renombrar") != null, "Botón 'Renombrar' existe")
	_check(_find_button_by_text(pm, "Duplicar") != null, "Botón 'Duplicar' existe")
	_check(_find_button_by_text(pm, "Administrar Etiquetas") != null, "Botón 'Administrar Etiquetas' existe")
	_check(_find_button_by_text(pm, "Ver en Carpeta") != null, "Botón 'Ver en Carpeta' existe")
	_check(_find_button_by_text(pm, "Eliminar de la lista") != null, "Botón 'Eliminar de la lista' existe")
	_check(_find_button_by_text(pm, "Eliminar Faltantes") != null, "Botón 'Eliminar Faltantes' existe")

	# Verificar opciones de flecha junto a Editar Proyecto
	var popups: Array = []
	_find_controls_by_class(pm, "PopupMenu", popups)
	var has_verbose := false
	var has_recovery := false
	for p_node in popups:
		var p: PopupMenu = p_node as PopupMenu
		if p == null:
			continue
		for ii in range(p.item_count):
			var it: String = p.get_item_text(ii)
			if "verbose mode" in it:
				has_verbose = true
			if "recovery mode" in it:
				has_recovery = true
	_check(has_verbose and has_recovery, "Opciones de Editar conservadas (verbose y recovery mode)")

	# 1) Estado 0 proyectos (vacio)
	_write_projects_cfg(tmp_cfg, PackedStringArray())
	pm.reload_projects_from_config_for_test(tmp_cfg)
	await _step_frames(4)
	var m_empty: Dictionary = pm.inspect_layout_metrics()
	_check(int(m_empty["project_count"]) == 0 and bool(m_empty["placeholder_visible"]), "Estado vacío (0 proyectos) muestra placeholder")
	_check("Aún no tienes proyectos" in String(m_empty["placeholder_text"]), "Texto estado vacío: 'Aún no tienes proyectos'")
	_check(bool(m_empty["open_disabled"]) and bool(m_empty["run_disabled"]) and bool(m_empty["rename_disabled"]) and bool(m_empty["duplicate_disabled"]) and bool(m_empty["tags_disabled"]) and bool(m_empty["folder_disabled"]) and bool(m_empty["erase_disabled"]),
		"Sin selección todos los botones de acción del proyecto están deshabilitados")

	# 2) Estado 1 proyecto y 5 proyectos (con uno faltante y ruta muy larga)
	var p1 := base_tmp.path_join("Crónica del Reloj")
	var p2 := base_tmp.path_join("subcarpeta_muy_larga_para_probar_elipsis/otro_nivel_profundo_clockgears/Proyecto_Épico_De_Aventuras")
	var p3 := base_tmp.path_join("Taller ARPG")
	var p4 := base_tmp.path_join("Prueba Demo")
	var p_miss := base_tmp.path_join("Proyecto_Carpeta_Borrada")

	_write_dummy_project(p1, "Crónica del Reloj", PackedStringArray(["rpg", "pixel"]))
	_write_projects_cfg(tmp_cfg, PackedStringArray([p1]), PackedStringArray([p1]))
	pm.reload_projects_from_config_for_test(tmp_cfg)
	await _step_frames(4)
	var m_one: Dictionary = pm.inspect_layout_metrics()
	_check(int(m_one["project_count"]) == 1 and not bool(m_one["placeholder_visible"]), "Estado 1 proyecto carga sin placeholder")

	_write_dummy_project(p2, "Aventuras del Engranaje Dorado con Título Largo", PackedStringArray(["aventura", "largo", "arpg", "extra"]))
	_write_dummy_project(p3, "Taller ARPG", PackedStringArray(["arpg"]))
	_write_dummy_project(p4, "Prueba Demo", PackedStringArray(["demo"]))
	# p_miss intencionalmente NO se crea en disco -> debe detectarse como faltante
	_write_projects_cfg(tmp_cfg, PackedStringArray([p1, p2, p3, p4, p_miss]), PackedStringArray([p1, p3]))
	pm.reload_projects_from_config_for_test(tmp_cfg)
	await _step_frames(5)

	var m_five: Dictionary = pm.inspect_layout_metrics()
	_check(int(m_five["project_count"]) == 5, "5 proyectos cargados (4 válidos + 1 faltante)")
	_check(not bool(m_five["erase_missing_disabled"]), "'Eliminar Faltantes' habilitado cuando hay proyecto faltante")

	# Seleccionar el proyecto de ruta larga y comprobar elipsis media + tooltip completo
	var plist: ProjectList = pm.get_project_list()
	var sbox_long := pm.get_search_box()
	sbox_long.text = "Engranaje Dorado"
	sbox_long.emit_signal("text_changed", sbox_long.text)
	await _step_frames(3)
	sbox_long.text = ""
	sbox_long.emit_signal("text_changed", "")
	# Tras limpiar, select_first_visible selecciona el primero; seleccionemos via busqueda para encontrar p2
	sbox_long.text = "Engranaje Dorado"
	sbox_long.emit_signal("text_changed", sbox_long.text)
	await _step_frames(3)
	var m_long: Dictionary = pm.inspect_layout_metrics()
	_check(String(m_long["preview_tooltip"]) == p2 and "…" in String(m_long["preview_path"]),
		"Ruta larga en panel derecho usa tooltip completo y elipsis media '…'", str(m_long["preview_path"]))
	sbox_long.text = ""
	sbox_long.emit_signal("text_changed", "")
	await _step_frames(3)

	# Verificar tarjeta Faltante tiene insignia 'Faltante' y una sola insignia de versión por tarjeta
	var miss_badges := _find_labels_with_text(pm, "Faltante")
	_check(not miss_badges.is_empty(), "Tarjeta faltante muestra insignia roja 'Faltante'")

	# Verificar resoluciones 16:9 con 5 proyectos en vista Lista y Cuadrícula
	for res in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		await _assert_no_cuts_or_overlaps(pm, res, "Lista-5")
		if _check_deadline():
			break

	# Cambiar a vista Cuadrícula y probar 16:9 + persistencia
	pm.simulate_select_view_mode(1)
	pm.simulate_select_order_option(1) # Nombre
	await _step_frames(4)
	_check(plist.get_view_mode() == 1, "Cambio a vista Cuadrícula aplicado")
	for res in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		await _assert_no_cuts_or_overlaps(pm, res, "Cuadrícula-5")
		if _check_deadline():
			break

	# Probar filtro sin coincidencias -> "No hay proyectos que coincidan" y limpiar filtro
	var sbox := pm.get_search_box()
	sbox.text = "zzzz_no_existe_ninguno"
	sbox.emit_signal("text_changed", sbox.text)
	await _step_frames(3)
	var m_nomatch: Dictionary = pm.inspect_layout_metrics()
	_check(int(m_nomatch["filtered_count"]) == 0 and bool(m_nomatch["placeholder_visible"]) and "No hay proyectos que coincidan" in String(m_nomatch["placeholder_text"]) and bool(m_nomatch["clear_filter_visible"]),
		"Filtro sin coincidencias muestra 'No hay proyectos que coincidan' y botón Limpiar")

	var clear_btn := _find_button_by_text(pm, "Limpiar búsqueda")
	if clear_btn != null:
		clear_btn.emit_signal("pressed")
	await _step_frames(3)
	_check(plist.get_filtered_project_count() == 5 and sbox.text == "", "Botón Limpiar búsqueda restaura los 5 proyectos")

	# Probar filtro por texto con acento y ordenamiento
	sbox.text = "Crónica"
	sbox.emit_signal("text_changed", sbox.text)
	await _step_frames(3)
	_check(plist.get_filtered_project_count() == 1, "Búsqueda por nombre acentuado 'Crónica' filtra a 1")
	sbox.text = ""
	sbox.emit_signal("text_changed", "")
	await _step_frames(3)

	# Probar 'Eliminar de la lista' NO borra archivos del disco (probar erase_selected_projects(false))
	plist.select_project(0, true)
	await _step_frames(2)
	var sel_items: PackedStringArray = plist.get_selected_project_paths()
	var sel_path := sel_items[0] if not sel_items.is_empty() else ""
	plist.erase_selected_projects(false)
	await _step_frames(3)
	_check(plist.get_project_count() == 4 and DirAccess.dir_exists_absolute(sel_path) and FileAccess.file_exists(sel_path.path_join("project.godot")),
		"Eliminar de la lista quita de projects.cfg sin borrar carpeta del proyecto")

	# Probar Eliminar Faltantes quita el faltante y desactiva el botón
	plist.erase_missing_projects()
	await _step_frames(3)
	_check(plist.get_project_count() == 3 and not plist.is_any_project_missing(), "Eliminar Faltantes quita proyecto faltante")

	# Volver a vista Lista y tomar capturas si se pidió --render-shots dentro de SubViewport exacto 1:1
	if render_shots:
		var sv := SubViewport.new()
		sv.render_target_update_mode = SubViewport.UPDATE_ALWAYS
		root.add_child(sv)
		pm.reparent(sv)
		pm.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
		pm.simulate_select_view_mode(0)
		_write_projects_cfg(tmp_cfg, PackedStringArray([p1, p2, p3, p4, p_miss]), PackedStringArray([p1, p3]))
		pm.reload_projects_from_config_for_test(tmp_cfg)
		plist.select_project(0, true)
		for shot_res in [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080)]:
			sv.size = shot_res
			pm.size = shot_res
			await _step_frames(6)
			var img := sv.get_texture().get_image()
			img.save_png(shots_dir.path_join("pm_dark_%dx%d.png" % [shot_res.x, shot_res.y]))
		# Captura Grid y Latón en 1366x768
		pm.simulate_select_view_mode(1)
		ClockGearsTheme.set_active_theme_id("brass")
		sv.size = Vector2i(1366, 768)
		pm.size = Vector2i(1366, 768)
		await _step_frames(6)
		sv.get_texture().get_image().save_png(shots_dir.path_join("pm_brass_grid_1366x768.png"))
		ClockGearsTheme.set_active_theme_id("dark")
		pm.simulate_select_view_mode(0)
		pm.reparent(root)
		sv.queue_free()
		await _step_frames(3)

	# 3) Prueba de virtualización con 200 proyectos
	var paths_200 := PackedStringArray()
	var t_gen_start := Time.get_ticks_msec()
	for idx in range(200):
		var pd := base_tmp.path_join("pool_200/p_%03d" % idx)
		_write_dummy_project(pd, "Proyecto #%03d" % idx, PackedStringArray(["rpg"]))
		paths_200.append(pd)
	_write_projects_cfg(tmp_cfg, paths_200, PackedStringArray([paths_200[0], paths_200[5]]))
	pm.simulate_select_view_mode(0)
	root.size = Vector2i(1366, 768)
	pm.size = Vector2i(1366, 768)
	pm.reload_projects_from_config_for_test(tmp_cfg)
	await _step_frames(6)
	var m200: Dictionary = pm.inspect_layout_metrics()
	var pool_cnt := int(m200["pool_count"])
	_check(int(m200["project_count"]) == 200 and pool_cnt > 0 and pool_cnt <= 32,
		"Lista virtualizada con 200 proyectos instancia solo tarjetas visibles (pool=%d)" % pool_cnt,
		"count=%d pool=%d" % [int(m200["project_count"]), pool_cnt])

	# Scroll en los 200 proyectos y comprobar reutilización del pool sin inflar controles
	plist.scroll_vertical = 4500
	plist.emit_signal("scroll_ended")
	await _step_frames(4)
	var m200_scroll: Dictionary = pm.inspect_layout_metrics()
	_check(int(m200_scroll["pool_count"]) <= 36, "Tras hacer scroll en 200 proyectos el pool se reutiliza (pool=%d)" % int(m200_scroll["pool_count"]))

	# Comprobar modo Cuadrícula con 200 proyectos también virtualiza
	pm.simulate_select_view_mode(1)
	pm.simulate_select_order_option(2) # Ruta
	await _step_frames(5)
	var m200_grid: Dictionary = pm.inspect_layout_metrics()
	_check(int(m200_grid["view_mode"]) == 1 and int(m200_grid["pool_count"]) <= 45,
		"Cuadrícula virtualizada con 200 proyectos (pool=%d)" % int(m200_grid["pool_count"]))

	# 4) Probar persistencia de Vista y Orden al destruir y reabrir ProjectManager
	pm.queue_free()
	await _step_frames(4)
	var pm2: ProjectManager = ProjectManager.new()
	root.add_child(pm2)
	await _step_frames(5)
	var m_persist: Dictionary = pm2.inspect_layout_metrics()
	_check(int(m_persist["view_mode"]) == 1 and int(m_persist["order_option"]) == 2,
		"Vista (Cuadrícula=1) y Orden (Ruta=2) persisten al reabrir",
		"view=%d order=%d" % [int(m_persist["view_mode"]), int(m_persist["order_option"])])

	# Restaurar Vista=Lista (0) y Orden=Último editado (0) para dejar EditorSettings limpio
	pm2.simulate_select_view_mode(0)
	pm2.simulate_select_order_option(0)
	await _step_frames(3)
	pm2.queue_free()
	await _step_frames(3)

	# Limpiar carpeta temporal
	_rm_dir_recursive(base_tmp)

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
