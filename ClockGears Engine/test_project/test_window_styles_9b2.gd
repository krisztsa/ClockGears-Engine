extends SceneTree

var fails: int = 0
var passes: int = 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passes += 1
		print("  [PASS] " + msg)
	else:
		fails += 1
		push_error("  [FAIL] " + msg)
		print("  [FAIL] " + msg)

func _create_temp_frame_png(abs_path: String) -> void:
	var img := Image.create(24, 24, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.10, 0.14, 0.24, 0.92))
	for y in range(24):
		for x in range(24):
			if x < 4 or x >= 20 or y < 4 or y >= 20:
				img.set_pixel(x, y, Color(0.88, 0.65, 0.25, 1.0))
	img.save_png(abs_path)

func _collect_controls_by_meta(node: Node, window_type: String, out: Array) -> void:
	if node is Control:
		var c := node as Control
		if c.has_meta("_cg_window_type_id") and String(c.get_meta("_cg_window_type_id")) == window_type:
			out.append(c)
	for ch in node.get_children():
		_collect_controls_by_meta(ch, window_type, out)

func _initialize() -> void:
	await process_frame
	print("=== TEST 9B TANDA 2: ESTILOS DE VENTANA (LOGICA Y DATOS) ===")
	var dm := ClockGearsDataManager
	var base_dir := ProjectSettings.globalize_path("res://clockgears_data")
	var tmp_root := OS.get_user_data_dir().path_join("cg_test_window_styles_9b2")
	var tmp_data := tmp_root.path_join("clockgears_data")
	var tmp_legacy := tmp_root.path_join("legacy_data")
	DirAccess.make_dir_recursive_absolute(tmp_data)
	DirAccess.make_dir_recursive_absolute(tmp_legacy)

	# 1. Estilos incluidos y proteccion de borrado / duplicado
	dm.init_empty()
	var styles: Array = dm.get_window_styles()
	check(styles.size() >= 3, "Existen al menos 3 estilos incluidos por defecto")
	check(dm.get_default_window_style_id() == "rounded", "Estilo por defecto inicial es 'rounded' (Redondeado)")
	check(not dm.delete_window_style("rounded"), "No permite borrar incluido 'Redondeado'")
	check(not dm.delete_window_style("rect"), "No permite borrar incluido 'Recto'")
	check(not dm.delete_window_style("none"), "No permite borrar incluido 'Sin bordes'")

	var dup_id: String = dm.duplicate_window_style("rounded", "Redondeado Personalizado")
	check(not dup_id.is_empty(), "Duplicar estilo incluido genera nuevo ID: " + dup_id)
	var dup_style: Dictionary = dm.get_window_style(dup_id)
	check(not bool(dup_style.get("built_in", true)), "El estilo duplicado no es built_in y se puede editar/borrar")
	check(dm.delete_window_style(dup_id), "Estilo de usuario duplicado se puede eliminar")

	# 2. Registro abierto de tipos de ventana
	var reg_types: Array = dm.get_registered_window_types()
	var found_ids := {}
	for rt in reg_types:
		found_ids[String(rt.get("id", ""))] = true
	for req in ["message", "choices", "battle_party", "battle_commands", "title_buttons", "options", "save_list"]:
		check(found_ids.has(req), "Tipo de ventana registrado: " + req)

	dm.register_window_type("custom_inventory_9b2", "Inventario Personalizado")
	var reg_after: Array = dm.get_registered_window_types()
	var has_custom_type := false
	for rt in reg_after:
		if String(rt.get("id", "")) == "custom_inventory_9b2":
			has_custom_type = true
	check(has_custom_type, "Registro de tipos de ventana es abierto y acepta nuevos tipos")

	# 3. Botones: 4 estados con herencia del estado normal
	var inherited_style_id: String = dm.create_window_style({
		"name": "Estilo Herencia Boton",
		"border_type": "rounded",
		"border_width": 3,
		"corner_radius": 6,
		"buttons": {
			"normal": {
				"border_type": "rect",
				"border_width": 3,
				"corner_radius": 0,
				"border_color": "#44aaffff",
				"fill_color": "#112233ee"
			},
			"hover": { "inherit_normal": true },
			"pressed": {
				"inherit_normal": false,
				"border_type": "rect",
				"border_width": 2,
				"border_color": "#ffaa33ff",
				"fill_color": "#221100ff"
			},
			"disabled": { "inherit_normal": true }
		}
	})
	var inh_st: Dictionary = dm.get_window_style(inherited_style_id)
	var btns: Dictionary = inh_st.get("buttons", {})
	check(String(btns.get("hover", {}).get("fill_color", "")) == "#112233ee", "Estado 'hover' hereda color de relleno desde 'normal'")
	check(int(btns.get("hover", {}).get("border_width", 0)) == 3, "Estado 'hover' hereda grosor desde 'normal'")
	check(String(btns.get("pressed", {}).get("fill_color", "")) == "#221100ff", "Estado 'pressed' sin herencia conserva estilo propio")

	# 4. Crear estilo Imagen (9 porciones + repeat + pixel art) y verificar persistencia
	var png_path := tmp_root.path_join("test_frame_9slice.png")
	_create_temp_frame_png(png_path)
	var img_style_id: String = dm.create_window_style({
		"name": "Marco 9-Slice Pixel",
		"border_type": "image",
		"image_path": png_path,
		"nine_slice": { "left": 5, "top": 5, "right": 5, "bottom": 5 },
		"center_mode": "repeat",
		"pixel_art": true,
		"padding": 14,
		"fill_color": "#090c14cc",
		"buttons": {
			"normal": {
				"border_type": "image",
				"image_path": png_path,
				"nine_slice": { "left": 4, "top": 4, "right": 4, "bottom": 4 },
				"center_mode": "stretch",
				"pixel_art": true
			},
			"hover": { "inherit_normal": true },
			"pressed": { "inherit_normal": true },
			"disabled": { "inherit_normal": true }
		}
	})

	dm.set_window_type_assignment("message", "rect")
	dm.set_window_type_assignment("choices", "none")
	dm.set_window_type_assignment("title_buttons", img_style_id)
	dm.set_window_type_assignment("battle_commands", "rect")
	dm.set_window_type_assignment("battle_party", "none")
	dm.set_window_type_assignment("options", "rounded")
	dm.set_window_type_assignment("save_list", img_style_id)
	# Datos minimos de actor/enemigo/tropa para que TitleScreen no recargue res://clockgears_data y BattleScene cree tarjetas de grupo
	dm.create_record(0, { "name": "Héroe Test", "hp": 100, "max_hp": 100 })
	dm.create_record(7, { "name": "Slime Test", "hp": 30, "max_hp": 30 })
	dm.create_record(8, { "name": "Tropa Test", "members": [{ "enemy_id": 1, "x": 400, "y": 240 }] })
	check(dm.save_all(tmp_data), "Persistencia: save_all escribe window_styles.json correctamente")
	check(FileAccess.file_exists(tmp_data.path_join("window_styles.json")), "Archivo window_styles.json existe en disco")

	dm.init_empty()
	check(dm.load_all(tmp_data), "Persistencia: load_all recarga estilos y asignaciones")
	check(dm.resolve_window_style_id("message") == "rect", "Asignacion 'message' -> 'rect' persistio")
	check(dm.resolve_window_style_id("choices") == "none", "Asignacion 'choices' -> 'none' persistio")
	check(dm.resolve_window_style_id("title_buttons") == img_style_id, "Asignacion 'title_buttons' -> imagen persistio")

	# 5. Aplicacion en juego mediante el componente unificado (mensajes, elecciones, batalla, titulo)
	var vp := SubViewport.new()
	vp.size = Vector2i(960, 540)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(vp)

	var msg_win := ClockGearsMessageWindow.new()
	vp.add_child(msg_win)
	msg_win.show_message("Hola estilos unificados", "Narrador")
	msg_win.show_choices(PackedStringArray(["Si", "No"]), 0, 0)

	var msg_ctrls: Array = []
	_collect_controls_by_meta(msg_win, "message", msg_ctrls)
	var choice_ctrls: Array = []
	_collect_controls_by_meta(msg_win, "choices", choice_ctrls)
	check(msg_ctrls.size() >= 4, "Cuadro de mensaje y subpaneles registrados bajo componente unificado (%d)" % msg_ctrls.size())
	check(choice_ctrls.size() >= 3, "Panel y botones de elecciones bajo componente unificado (%d)" % choice_ctrls.size())

	var info_msg := ClockGearsWindowStyleManager.inspect_control_style(msg_ctrls[0])
	var info_choice := ClockGearsWindowStyleManager.inspect_control_style(choice_ctrls[0])
	check(bool(info_msg.get("unified", false)) and String(info_msg.get("style_id", "")) == "rect", "Cuadro de mensaje aplica estilo 'Recto'")
	check(int(info_msg.get("corner_radius", -1)) == 0, "Estilo 'Recto' en mensaje tiene radio 0")
	check(bool(info_choice.get("unified", false)) and String(info_choice.get("style_id", "")) == "none", "Elecciones aplica estilo 'Sin bordes'")
	check(int(info_choice.get("border_width", -1)) == 0, "Estilo 'Sin bordes' en elecciones tiene grosor 0")

	var title_scr := ClockGearsTitleScreen.new()
	vp.add_child(title_scr)
	await process_frame
	var title_ctrls: Array = []
	_collect_controls_by_meta(title_scr, "title_buttons", title_ctrls)
	check(title_ctrls.size() >= 2, "Titulo usa componente unificado para panel y botones")
	var info_tpanel := ClockGearsWindowStyleManager.inspect_control_style(title_ctrls[0])
	check(String(info_tpanel.get("border_type", "")) == "image", "Titulo aplica estilo 'Imagen' de 9 porciones")
	check(String(info_tpanel.get("center_mode", "")) == "repeat", "Estilo 'Imagen' conserva modo repetir en centro")
	check(bool(info_tpanel.get("pixel_art", false)) and title_ctrls[0].texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST, "Pixel art activa filtro Nearest")

	var battle_scn := ClockGearsBattleScene.new()
	vp.add_child(battle_scn)
	await process_frame
	battle_scn.setup_battle(1, true, true, "", msg_win, true)
	await process_frame
	var bcmd_ctrls: Array = []
	_collect_controls_by_meta(battle_scn, "battle_commands", bcmd_ctrls)
	check(bcmd_ctrls.size() >= 6, "Batalla usa componente unificado para paneles y botones de comando")
	var info_bcmd := ClockGearsWindowStyleManager.inspect_control_style(bcmd_ctrls[0])
	check(String(info_bcmd.get("style_id", "")) == "rect", "Panel de comandos de batalla refleja estilo 'Recto'")
	var bparty_ctrls: Array = []
	_collect_controls_by_meta(battle_scn, "battle_party", bparty_ctrls)
	check(bparty_ctrls.size() >= 1, "Tarjetas de grupo en batalla usan componente unificado")
	var info_bparty := ClockGearsWindowStyleManager.inspect_control_style(bparty_ctrls[0])
	check(String(info_bparty.get("style_id", "")) == "none" and int(info_bparty.get("border_width", -1)) == 0, "Panel de grupo refleja estilo 'Sin bordes' (grosor 0)")

	# 6. Cambiar grosor y radio actualiza todas las ventanas que usan el estilo
	var edit_id: String = dm.duplicate_window_style("rounded", "Editado Live")
	dm.update_window_style(edit_id, { "border_width": 6, "corner_radius": 14 })
	dm.set_window_type_assignment("message", edit_id)
	msg_win.refresh_window_styles()
	var info_updated := ClockGearsWindowStyleManager.inspect_control_style(msg_ctrls[0])
	check(int(info_updated.get("border_width", 0)) == 6, "Cambiar grosor del borde se refleja al instante en ventanas (6px)")
	check(int(info_updated.get("corner_radius", 0)) == 14, "Cambiar radio de esquinas se refleja al instante en ventanas (14px)")

	# 7. Respaldo si imagen no existe -> cae en "Redondeado" sin errores
	var broken_id: String = dm.create_window_style({
		"name": "Imagen Rota",
		"border_type": "image",
		"image_path": "res://clockgears_data/no_existe_marco_999.png"
	})
	dm.set_window_type_assignment("options", broken_id)
	title_scr.rebuild_title_screen()
	var opt_ctrls: Array = []
	_collect_controls_by_meta(title_scr, "options", opt_ctrls)
	check(opt_ctrls.size() >= 1, "Controles de Opciones encontrados")
	var info_broken := ClockGearsWindowStyleManager.inspect_control_style(opt_ctrls[0])
	check(String(info_broken.get("style_id", "")) == "rounded" and bool(info_broken.get("fallback_used", false)), "Imagen faltante cae en respaldo 'Redondeado' sin error")

	# 8. Escalado 16:9 (1366x768, 1920x1080, 2560x1440) sin cortes ni solapes y pixel_art escala entera
	dm.set_window_type_assignment("title_buttons", "rounded")
	dm.set_window_type_assignment("message", edit_id)
	title_scr.rebuild_title_screen()
	for sz in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
		vp.size = sz
		await process_frame
		await process_frame
		msg_win.refresh_window_styles()
		var layout: Dictionary = title_scr.get_layout_bounds_info(Vector2(sz))
		check(bool(layout.get("all_within_viewport", false)), "Titulo sin cortes en %dx%d" % [sz.x, sz.y])
		check(not bool(layout.get("any_overlap", true)), "Titulo sin solapes en %dx%d" % [sz.x, sz.y])
		var msg_scaled := ClockGearsWindowStyleManager.inspect_control_style(msg_ctrls[0])
		if sz.x == 1920:
			check(int(msg_scaled.get("scale_factor", 1)) == 2 and int(msg_scaled.get("border_width", 0)) == 12, "En 1920x1080 escala entera 2x duplica nitidez (grosor 6->12px)")
		elif sz.x == 2560:
			check(int(msg_scaled.get("scale_factor", 1)) == 3 and int(msg_scaled.get("border_width", 0)) == 18, "En 2560x1440 escala entera 3x escala nitido (grosor 6->18px)")
		else:
			check(int(msg_scaled.get("scale_factor", 0)) == 1, "En 1366x768 escala entera pixel art se mantiene en 1x")

	# 9. Proyecto antiguo (sin window_styles.json) migra y queda en "Redondeado"
	dm.load_all(base_dir)
	check(dm.save_all(tmp_legacy), "Preparando proyecto antiguo de prueba")
	var ws_file := tmp_legacy.path_join("window_styles.json")
	if FileAccess.file_exists(ws_file):
		DirAccess.remove_absolute(ws_file)
	dm.init_empty()
	check(dm.load_all(tmp_legacy), "Carga proyecto antiguo sin window_styles.json")
	check(dm.get_default_window_style_id() == "rounded", "Proyecto antiguo migra con estilo por defecto 'Redondeado'")
	check(dm.resolve_window_style_id("message") == "rounded" and dm.resolve_window_style_id("title_buttons") == "rounded", "Todas las ventanas heredan 'Redondeado' tras migracion")
	title_scr.rebuild_title_screen()
	var legacy_layout: Dictionary = title_scr.get_layout_bounds_info()
	check(int(legacy_layout.get("button_corner_radius", 0)) >= 8, "Proyecto antiguo mantiene botones redondeados igual que antes (>=8px)")

	# Limpieza de recursos de prueba
	vp.queue_free()
	dm.load_all(base_dir)

	print("=== FIN TEST 9B TANDA 2: PASS=%d FAIL=%d ===" % [passes, fails])
	quit(0 if fails == 0 else 1)
