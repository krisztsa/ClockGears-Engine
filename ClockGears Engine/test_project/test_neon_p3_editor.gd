extends SceneTree

var pass_count := 0
var fail_count := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] ", msg)
	else:
		fail_count += 1
		print("  [FAIL] ", msg)

func _init() -> void:
	var t0 := Time.get_ticks_msec()
	var cgt = Engine.get_singleton("ClockGearsTheme")
	check(cgt != null, "ClockGearsTheme singleton disponible")
	if cgt == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	cgt.set_glow_effects_level(1)

	for tid in ["dark", "brass"]:
		print("--- Verificando Punto 3 (Editor Principal) en tema: ", tid, " ---")
		cgt.set_active_theme_id(tid)
		var tk: Dictionary = cgt.get_theme_tokens_dict(tid)
		var border_copper: Color = tk["border_copper"]
		var border_bright: Color = tk["border_copper_bright"]
		var accent_neon: Color = tk["accent_neon"]
		var halo_radius: int = int(tk["halo_radius"])

		# 1. Barra de menús (píldoras)
		var layers_dlg = ClassDB.instantiate("ClockGearsLayersDialog")
		var ui_h: Dictionary = layers_dlg.call("get_ui_handles")
		var top_row: Control = ui_h.get("title_bar_row")
		var mbar: MenuBar = ui_h.get("main_menu_bar")
		if top_row != null:
			get_root().add_child(top_row)
			cgt.apply_to_control(top_row)
		check(mbar != null, "[%s] MainMenuBar creada" % tid)
		if mbar != null:
			var mb_n: StyleBoxFlat = mbar.get_theme_stylebox("normal")
			var mb_h: StyleBoxFlat = mbar.get_theme_stylebox("hover")
			var mb_p: StyleBoxFlat = mbar.get_theme_stylebox("pressed")
			check(mb_n != null and mb_n.corner_radius_top_left == 6 and mb_h != null and mb_h.shadow_size == halo_radius and mb_p != null and mb_p.border_color.is_equal_approx(accent_neon),
				"[%s] Barra de menús (píldoras) usa estados del punto 2 con halo y acento neón" % tid)

		# 2. Barra de herramientas (ClockGearsTopBar)
		var top_bar = ClassDB.instantiate("ClockGearsTopBar")
		top_bar.size = Vector2(1366, 48)
		get_root().add_child(top_bar)
		cgt.apply_to_control(top_bar)

		var lbtn0: Button = top_bar.call("get_layer_button", 0)
		var tbtn0: Button = top_bar.call("get_tool_button", 0)
		var play_btn: Button = top_bar.call("get_playtest_button")
		check(lbtn0 != null and tbtn0 != null and play_btn != null,
			"[%s] Botones de capa activa, herramienta activa y Probar (F5) presentes" % tid)
		if lbtn0 != null:
			var sb_prs: StyleBoxFlat = lbtn0.get_theme_stylebox("pressed")
			var sb_hov: StyleBoxFlat = lbtn0.get_theme_stylebox("hover")
			check(sb_prs != null and sb_prs.border_width_left == 2 and sb_prs.border_color.is_equal_approx(accent_neon) and sb_prs.shadow_size == halo_radius,
				"[%s] Herramienta/capa activa tiene borde neón (2 px), halo y tinte ámbar" % tid)
			check(sb_hov != null and sb_hov.border_color.is_equal_approx(border_bright) and sb_hov.shadow_size == halo_radius,
				"[%s] Botón de barra en hover tiene borde cobrizo brillante y brillo suave" % tid)
			check(lbtn0.get_theme_color("icon_normal_color") == Color(1, 1, 1, 1) and lbtn0.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST,
				"[%s] Iconos de barra de herramientas NUNCA se tiñen y usan filtro nearest" % tid)
		if play_btn != null:
			check(play_btn.text == "Probar (F5)" and play_btn.icon != null,
				"[%s] Botón Probar (F5) muestra su icono verde y el texto 'Probar (F5)' en 1366x768" % tid)

		# 3. Paneles "Paleta de tiles" y "Árbol de mapas"
		var map_panel = ClassDB.instantiate("ClockGearsMapPanel")
		map_panel.size = Vector2(300, 700)
		get_root().add_child(map_panel)
		cgt.apply_to_control(map_panel)

		var pal_ic: TextureRect = map_panel.call("get_palette_header_icon")
		var map_ic: TextureRect = map_panel.call("get_maps_header_icon")
		var pal_grad = map_panel.find_child("PaletteHeaderGradLine", true, false)
		var map_grad = map_panel.find_child("MapsHeaderGradLine", true, false)
		var mtree: Tree = map_panel.call("get_map_tree")
		check(pal_ic != null and pal_ic.texture != null and pal_ic.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST and map_ic != null and map_ic.texture != null,
			"[%s] Cabeceras de Paleta de tiles y Árbol de mapas tienen icono pixel-art nearest" % tid)
		check(pal_grad != null and pal_grad.theme_type_variation == &"ClockGearsSectionHeaderLine" and map_grad != null and map_grad.theme_type_variation == &"ClockGearsSectionHeaderLine",
			"[%s] Cabeceras tienen línea inferior con degradado de cobrizo a transparente" % tid)
		if mtree != null:
			var mtree_sel: StyleBoxFlat = mtree.get_theme_stylebox("selected")
			check(mtree_sel != null and mtree_sel.get_class() == "ClockGearsBrassStyleBox" and mtree_sel.call("is_led_dot_enabled"),
				"[%s] Árbol de mapas tiene fila seleccionada con neón y punto LED de 4 px" % tid)

		# 4. Marco del lienzo del mapa (ClockGearsCanvasFrame)
		var th: Theme = cgt.get_theme_by_id(tid)
		var cframe: StyleBoxFlat = th.get_stylebox("panel", "ClockGearsCanvasFrame")
		check(cframe != null and not cframe.draw_center and cframe.corner_radius_top_left == 6 and cframe.border_color.is_equal_approx(border_copper) and cframe.shadow_size == halo_radius,
			"[%s] Lienzo del mapa tiene marco cobrizo fino con esquinas suaves (6 px) y halo exterior muy sutil" % tid)

		# 5. Barra de estado inferior (ClockGearsStatusBar)
		var status_bar = ClassDB.instantiate("ClockGearsStatusBar")
		get_root().add_child(status_bar)
		cgt.apply_to_control(status_bar)
		status_bar.call("refresh_status")
		var snap: Dictionary = status_bar.call("get_status_snapshot")
		check(String(snap.get("tool_text", "")).begins_with("Herramienta:") and not String(snap.get("layer_text", "")).is_empty() and String(snap.get("zoom_text", "")).begins_with("Zoom:") and String(snap.get("tile_size_text", "")) == "Tile: 32x32 px" and String(snap.get("version_text", "")) == "Alpha-0.0.4",
			"[%s] Barra de estado inferior muestra herramienta, capa activa, zoom, tile 32x32 px y versión única Alpha-0.0.4" % tid)

		status_bar.free()
		map_panel.free()
		top_bar.free()
		if top_row != null:
			top_row.free()
		layers_dlg.free()

	print("Tiempo test_neon_p3_editor: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
