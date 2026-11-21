extends SceneTree

var pass_count := 0
var fail_count := 0
var start_msec := 0

func _check(cond: bool, label: String) -> void:
	if cond:
		pass_count += 1
	else:
		fail_count += 1
		print("FAIL: " + label)

func _initialize() -> void:
	start_msec = Time.get_ticks_msec()
	call_deferred("_run_test")

func _process(_delta: float) -> bool:
	if Time.get_ticks_msec() - start_msec > 55000:
		print("TIMEOUT GUARD TRIGGERED")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count + 1])
		quit(1)
		return true
	return false

func _capture_layout(root_ctrl: Control) -> Dictionary:
	var out := {}
	var ctrls := root_ctrl.find_children("*", "Control", true, false)
	for n in ctrls:
		var c := n as Control
		if c != null and c.visible_in_tree and not c.name.is_empty():
			var key := String(c.get_path())
			out[key] = Rect2(c.position, c.size)
	return out

func _check_no_overlap_in_box(box: Control, tag: String) -> void:
	var rects: Array[Rect2] = []
	for ch in box.get_children():
		if ch is Control and (ch as Control).visible:
			var cc := ch as Control
			rects.append(Rect2(cc.position, cc.size))
	for i in range(rects.size()):
		for j in range(i + 1, rects.size()):
			var inter := rects[i].intersection(rects[j])
			_check(inter.size.x <= 1.0 or inter.size.y <= 1.0, "%s sin solapes entre hijos %d y %d (%s)" % [tag, i, j, str(inter)])

func _run_test() -> void:
	var th = Engine.get_singleton("ClockGearsTheme")
	_check(th != null, "ClockGearsTheme singleton disponible")
	if th == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	var prefs := ClockGearsPreferencesDialog.new()
	var host := VBoxContainer.new()
	host.add_theme_constant_override("separation", 6)
	var top_bar := ClockGearsTopBar.new()
	var map_panel := ClockGearsMapPanel.new()
	var db := ClockGearsDatabaseDialog.new()
	var ev := ClockGearsEventEditorDialog.new()
	host.add_child(top_bar)
	host.add_child(map_panel)
	map_panel.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(host)
	root.add_child(prefs)
	root.add_child(db)
	root.add_child(ev)

	th.register_themed_root(host)
	th.register_themed_root(prefs)
	th.register_themed_root(db)
	th.register_themed_root(ev)

	var tileset_p := map_panel.find_child("ClockGearsTilesetPanel", true, false) as Control
	var maps_p := map_panel.find_child("ClockGearsMapsPanel", true, false) as Control
	var map_tree := map_panel.find_child("ClockGearsMapTree", true, false) as Control
	var db_chap := db.find_child("ClockGearsDbChaptersPanel", true, false) as Control
	var db_cats := db.find_child("ClockGearsDbCatsPanel", true, false) as Control
	var db_tabs := db.find_child("ClockGearsDbMainTabs", true, false) as Control
	_check(tileset_p != null and maps_p != null and map_tree != null, "Nodos de MapPanel listos")
	_check(db_chap != null and db_cats != null and db_tabs != null, "Nodos de Database listos")

	for res in [Vector2i(1366, 768), Vector2i(1920, 1080)]:
		prefs.select_editor_theme_by_id("dark")
		host.size = Vector2(res.x, res.y)
		db.show()
		db.min_size = res
		db.size = res
		await process_frame
		await process_frame

		var top_bar_rect_dark := Rect2(top_bar.position, top_bar.size)
		var map_panel_rect_dark := Rect2(map_panel.position, map_panel.size)
		var tileset_rect_dark := Rect2(tileset_p.position, tileset_p.size)
		var maps_rect_dark := Rect2(maps_p.position, maps_p.size)
		var tree_rect_dark := Rect2(map_tree.position, map_tree.size)
		var chap_rect_dark := Rect2(db_chap.position, db_chap.size)
		var cats_rect_dark := Rect2(db_cats.position, db_cats.size)
		var tabs_rect_dark := Rect2(db_tabs.position, db_tabs.size)
		_check_no_overlap_in_box(host, "Host dark %s" % str(res))
		_check(top_bar_rect_dark.end.y <= float(res.y) and map_panel_rect_dark.end.y <= float(res.y) + 1.0, "Dark %s workspace dentro de límites" % str(res))

		var leg_host_dark: Dictionary = th.check_tree_legibility_and_layout(host, res)
		var leg_db_dark: Dictionary = th.check_tree_legibility_and_layout(db, res)
		var leg_ev_dark: Dictionary = th.check_tree_legibility_and_layout(ev, res)
		_check(bool(leg_host_dark.get("ok", false)), "Dark %s Host legibilidad/16:9 OK" % str(res))
		_check(bool(leg_db_dark.get("ok", false)), "Dark %s DB legibilidad/16:9 OK" % str(res))
		_check(bool(leg_ev_dark.get("ok", false)), "Dark %s EventEditor legibilidad/16:9 OK" % str(res))

		# Cambiar a Brass en caliente vía Preferencias y verificar mismas posiciones/tamaños y bordes
		prefs.select_editor_theme_by_id("brass")
		await process_frame
		await process_frame
		_check(th.get_active_theme_id() == "brass", "Cambio a 'brass' en Preferencias OK (%s)" % str(res))

		var top_bar_rect_brass := Rect2(top_bar.position, top_bar.size)
		var map_panel_rect_brass := Rect2(map_panel.position, map_panel.size)
		var tileset_rect_brass := Rect2(tileset_p.position, tileset_p.size)
		var maps_rect_brass := Rect2(maps_p.position, maps_p.size)
		var tree_rect_brass := Rect2(map_tree.position, map_tree.size)
		var chap_rect_brass := Rect2(db_chap.position, db_chap.size)
		var cats_rect_brass := Rect2(db_cats.position, db_cats.size)
		var tabs_rect_brass := Rect2(db_tabs.position, db_tabs.size)
		_check(absf(top_bar_rect_dark.size.y - top_bar_rect_brass.size.y) <= 2.0, "%s TopBar mantiene altura al cambiar Dark->Brass (%s vs %s)" % [str(res), str(top_bar_rect_dark.size.y), str(top_bar_rect_brass.size.y)])
		_check(absf(map_panel_rect_dark.position.y - map_panel_rect_brass.position.y) <= 2.0, "%s MapPanel mantiene posición Y al cambiar Dark->Brass" % str(res))
		_check(tileset_rect_dark.is_equal_approx(tileset_rect_brass), "%s Paleta de tiles mantiene posición y tamaño" % str(res))
		_check(maps_rect_dark.is_equal_approx(maps_rect_brass), "%s Árbol de mapas panel mantiene posición y tamaño" % str(res))
		_check(tree_rect_dark.is_equal_approx(tree_rect_brass), "%s Árbol de mapas lista mantiene posición y tamaño (%s vs %s)" % [str(res), str(tree_rect_dark), str(tree_rect_brass)])
		_check(chap_rect_dark.is_equal_approx(chap_rect_brass), "%s DB capítulos mantiene posición y tamaño (%s vs %s)" % [str(res), str(chap_rect_dark), str(chap_rect_brass)])
		_check(cats_rect_dark.is_equal_approx(cats_rect_brass), "%s DB categorías mantiene posición y tamaño (%s vs %s)" % [str(res), str(cats_rect_dark), str(cats_rect_brass)])
		_check(tabs_rect_dark.is_equal_approx(tabs_rect_brass), "%s DB pestañas internas mantiene posición y tamaño (%s vs %s)" % [str(res), str(tabs_rect_dark), str(tabs_rect_brass)])
		_check_no_overlap_in_box(host, "Host brass %s" % str(res))

		var leg_host_brass: Dictionary = th.check_tree_legibility_and_layout(host, res)
		var leg_db_brass: Dictionary = th.check_tree_legibility_and_layout(db, res)
		var leg_ev_brass: Dictionary = th.check_tree_legibility_and_layout(ev, res)
		_check(bool(leg_host_brass.get("ok", false)), "Brass %s Host legibilidad/16:9 OK" % str(res))
		_check(bool(leg_db_brass.get("ok", false)), "Brass %s DB legibilidad/16:9 OK" % str(res))
		_check(bool(leg_ev_brass.get("ok", false)), "Brass %s EventEditor legibilidad/16:9 OK" % str(res))

		# Verificar también que no queda ningún scroll horizontal accidental en Preferencias o MapPanel
		for sc in map_panel.find_children("*", "ScrollContainer", true, false):
			var scc := sc as ScrollContainer
			_check(scc.horizontal_scroll_mode != ScrollContainer.SCROLL_MODE_SHOW_ALWAYS, "%s MapPanel sin scroll horizontal forzado" % str(res))

		# Volver en caliente a Dark y validar que tamaño/posición vuelven exactos y bordes/listas se restauran
		prefs.select_editor_theme_by_id("dark")
		await process_frame
		await process_frame
		_check(th.get_active_theme_id() == "dark", "Vuelta a 'dark' en Preferencias OK (%s)" % str(res))
		var top_bar_rect_back := Rect2(top_bar.position, top_bar.size)
		_check(top_bar_rect_back.is_equal_approx(top_bar_rect_dark), "%s TopBar vuelve al mismo tamaño/posición exactos" % str(res))

	# Limpieza
	prefs.select_editor_theme_by_id("dark")
	host.queue_free()
	prefs.queue_free()
	db.queue_free()
	ev.queue_free()
	await process_frame

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
