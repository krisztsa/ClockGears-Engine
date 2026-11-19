extends SceneTree

# Test Punto 1 (Correcciones UI 9B):
# - Barra superior: 8 menús visibles en píldora redondeada con borde ámbar (tema actual y Latón), brillo hover y estado abierto visible
# - Etiqueta final "Capa: ..." eliminada
# - Menú "Capas" muestra la capa actual dinámicamente ("Capas · Suelo", "Capas · 1", sin repetir "Capa" dos veces)
# - Capas 1..4 en ClockGearsTopBar intactas y barra completa en 960x540, 1366x768, 1920x1080 y 2560x1440

var passed: int = 0
var failed: int = 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _count_word_occurrences(haystack: String, word: String) -> int:
	var low := haystack.to_lower()
	var w := word.to_lower()
	var count := 0
	var pos := 0
	while true:
		var idx := low.find(w, pos)
		if idx < 0:
			break
		count += 1
		pos = idx + w.length()
	return count

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 1 (9B CORR): BARRA SUPERIOR, PÍLDORAS Y CAPAS SIN DUPLICAR ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var th = Engine.get_singleton("ClockGearsTheme")
	if dm == null or th == null:
		_check(false, "Singletons ClockGearsDataManager y ClockGearsTheme disponibles")
		quit(1)
		return

	dm.init_empty()
	var prev_theme_id: String = th.get_active_theme_id()

	var vp := SubViewport.new()
	vp.size = Vector2i(1366, 200)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(vp)

	var vbox := VBoxContainer.new()
	vbox.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	vp.add_child(vbox)

	var layers_dlg := ClockGearsLayersDialog.new()
	root.add_child(layers_dlg)
	await _step_frames(1)
	var ui_handles_top: Dictionary = layers_dlg.get_ui_handles()
	var row: HBoxContainer = ui_handles_top.get("title_bar_row", null) as HBoxContainer
	_check(row != null, "Fila de barra superior creada")
	vbox.add_child(row)

	var top_bar := ClockGearsTopBar.new()
	vbox.add_child(top_bar)
	await _step_frames(2)

	# 1. Etiqueta final eliminada (no debe existir)
	var old_wrap := row.get_node_or_null("ClockGearsActiveLayerWrap")
	var old_ind := row.get_node_or_null("ClockGearsActiveLayerIndicator")
	_check(old_wrap == null and old_ind == null and ui_handles_top.get("active_indicator", null) == null,
		"Etiqueta final 'Capa: ...' eliminada de la barra superior")

	# 2. Los 8 menús visibles en MenuBar con píldoras redondeadas y borde ámbar
	var mbar: MenuBar = row.get_node_or_null("MainMenuBar") as MenuBar
	_check(mbar != null and mbar.get_menu_count() == 9, "MainMenuBar presente con 9 entradas (8 visibles + Editar oculto)")
	var visible_count := 0
	var layers_idx := -1
	var visible_titles: Array[String] = []
	for i in range(mbar.get_menu_count()):
		if not mbar.is_menu_hidden(i):
			visible_count += 1
			var t := mbar.get_menu_title(i)
			visible_titles.append(t)
			if t.begins_with("Capas"):
				layers_idx = i
	_check(visible_count == 8 and layers_idx >= 0, "Barra muestra exactamente los 8 menús esperados: " + str(visible_titles))

	var sb_norm := mbar.get_theme_stylebox("normal") as StyleBoxFlat
	var sb_hov := mbar.get_theme_stylebox("hover") as StyleBoxFlat
	var sb_prs := mbar.get_theme_stylebox("pressed") as StyleBoxFlat
	var sb_hp := mbar.get_theme_stylebox("hover_pressed") as StyleBoxFlat
	_check(not mbar.flat and sb_norm != null and sb_norm.corner_radius_top_left >= 5 and sb_norm.border_width_left >= 1 and sb_norm.border_color.r > sb_norm.border_color.b,
		"Menús en tema actual son píldoras redondeadas con borde ámbar (r=%d, bd=%s)" % [sb_norm.corner_radius_top_left if sb_norm else 0, str(sb_norm.border_color if sb_norm else Color())])
	_check(sb_hov != null and sb_hov.bg_color != sb_norm.bg_color and sb_prs != null and sb_prs.bg_color != sb_hov.bg_color and sb_hp != null,
		"Estados hover (brillo) y menú abierto (pressed/hover_pressed) distintos y visibles en tema actual")

	# 3. Tema Latón mantiene píldoras con degradado y borde ámbar/latón, y restaura al cambiar
	th.set_active_theme_id("brass")
	th.apply_theme_to_node_tree(row)
	await _step_frames(1)
	var b_norm := mbar.get_theme_stylebox("normal") as StyleBoxFlat
	var b_hov := mbar.get_theme_stylebox("hover") as StyleBoxFlat
	var b_prs := mbar.get_theme_stylebox("pressed") as StyleBoxFlat
	_check(b_norm != null and b_norm.get_class() == "ClockGearsBrassStyleBox" and b_norm.border_color.r > b_norm.border_color.b and b_hov != null and b_prs != null and b_prs.bg_color != b_hov.bg_color,
		"Menús en tema Latón mantienen píldora redondeada latón con hover y estado abierto visible")

	th.set_active_theme_id("dark")
	th.apply_theme_to_node_tree(row)
	await _step_frames(1)
	var restored_norm := mbar.get_theme_stylebox("normal") as StyleBoxFlat
	_check(restored_norm != null and restored_norm.get_class() == "StyleBoxFlat" and restored_norm.border_color.r > restored_norm.border_color.b,
		"Al volver de Latón a tema actual conserva píldoras con borde ámbar")

	# 4. Menú Capas dinámico y sin repetir "Capa" dos veces seguidas
	top_bar.select_layer_button(1)
	await _step_frames(1)
	var t1 := mbar.get_menu_title(layers_idx)
	_check(t1 == "Capas · Suelo" and _count_word_occurrences(t1, "capa") == 1,
		"Capa 1 muestra '%s' (una sola mención de 'Capa')" % t1)

	top_bar.select_layer_button(2)
	await _step_frames(1)
	_check(mbar.get_menu_title(layers_idx) == "Capas · Decoración", "Capa 2 muestra 'Capas · Decoración'")

	top_bar.select_layer_button(3)
	await _step_frames(1)
	_check(mbar.get_menu_title(layers_idx) == "Capas · Objetos", "Capa 3 muestra 'Capas · Objetos'")

	top_bar.select_layer_button(4)
	await _step_frames(1)
	var t4 := mbar.get_menu_title(layers_idx)
	_check(t4.begins_with("Capas · Techo") and _count_word_occurrences(t4, "capa") == 1,
		"Capa 4 muestra '%s' sin duplicar 'Capa'" % t4)

	# Capa personalizada con nombre "Capa 1" / "Capa" (sin nombre propio) -> "Capas · 1" / sin repetir "Capa"
	var c_named: Dictionary = layers_dlg.create_layer_from_ui("Puente Ñandú", 3)
	var cid_named: String = String(c_named.get("id", ""))
	layers_dlg.select_layer_from_ui(cid_named)
	await _step_frames(1)
	_check(mbar.get_menu_title(layers_idx) == "Capas · Puente Ñandú",
		"Capa personalizada con nombre propio muestra '%s'" % mbar.get_menu_title(layers_idx))

	var c_unnamed: Dictionary = layers_dlg.create_layer_from_ui("Capa", 5)
	var cid_unnamed: String = String(c_unnamed.get("id", ""))
	layers_dlg.select_layer_from_ui(cid_unnamed)
	await _step_frames(1)
	var tu := mbar.get_menu_title(layers_idx)
	_check(tu.begins_with("Capas · ") and _count_word_occurrences(tu, "capa") == 1,
		"Capa sin nombre propio muestra '%s' (sin repetir 'Capa')" % tu)

	# Volver a capa base 1
	dm.set_editor_custom_layer("")
	top_bar.select_layer_button(1)
	await _step_frames(1)

	# 5. Botones de capas 1..4 en ClockGearsTopBar intactos
	var row1_b: Node = top_bar.get_child(0).get_child(0).get_child(0)
	var row2_b: Node = top_bar.get_child(0).get_child(0).get_child(1)
	var group_counts: Array = []
	for rn in [row1_b, row2_b]:
		for i in range(rn.get_child_count()):
			var ch: Node = rn.get_child(i)
			if ch is HBoxContainer:
				group_counts.append(ch.get_child_count())
	_check(group_counts == [4, 5, 3, 6, 5, 3, 2, 1], "Grupos de ClockGearsTopBar (incluidas capas 1..4) intactos: %s" % str(group_counts))

	# 6. Encaje completo 16:9 en 960x540, 1366x768, 1920x1080 y 2560x1440
	for w in [960, 1366, 1920, 2560]:
		vp.size = Vector2i(w, 200)
		vbox.size = Vector2(w, 200)
		await _step_frames(2)
		var r := mbar.get_global_rect()
		_check(r.position.x >= 0.0 and r.end.x <= float(w) and r.size.x > 100.0,
			"Barra de menús completa y sin cortes en ancho %dpx (rect=%s)" % [w, str(r)])

	th.set_active_theme_id(prev_theme_id)
	layers_dlg.queue_free()
	vp.queue_free()
	await _step_frames(1)
	dm.init_empty()

	print("=== RESULTADO TEST PUNTO 1: %d PASSED / %d FAILED ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
