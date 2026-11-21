extends SceneTree

const CAP_DIR := "C:/Users/Kris/.gemini/antigravity/brain/a6932071-8427-491c-87cb-779e9c006e46/scratch/neon_captures"

var pass_count := 0
var fail_count := 0
var start_msec := 0

func _process(_delta: float) -> bool:
	if start_msec > 0 and Time.get_ticks_msec() - start_msec > 55000:
		printerr("TIMEOUT interno 55 s en test_neon_p7_verificacion_y_capturas.gd")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count + 1])
		quit(2)
		return true
	return false

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] ", msg)
	else:
		fail_count += 1
		print("  [FAIL] ", msg)

func _wait_frames(n: int = 3) -> void:
	for _i in range(n):
		await process_frame

func _image_has_real_content(img: Image) -> bool:
	if img == null or img.is_empty():
		return false
	var w := img.get_width()
	var h := img.get_height()
	if w < 100 or h < 100:
		return false
	var ref_c := img.get_pixel(w / 2, h / 2)
	var diff_count := 0
	for sy in range(8):
		for sx in range(8):
			var px := int((float(sx) + 0.5) / 8.0 * float(w))
			var py := int((float(sy) + 0.5) / 8.0 * float(h))
			var c := img.get_pixel(px, py)
			if not c.is_equal_approx(ref_c):
				diff_count += 1
	return diff_count >= 5

func _save_capture(vp: Viewport, filename: String) -> bool:
	var tex := vp.get_texture()
	if tex == null:
		return false
	var img := tex.get_image()
	if not _image_has_real_content(img):
		return false
	var full_path := CAP_DIR.path_join(filename)
	var err := img.save_png(full_path)
	return err == OK and FileAccess.file_exists(full_path)

func _build_editor_preview_shell(cgt) -> Dictionary:
	var shell := PanelContainer.new()
	shell.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var sb_bg := StyleBoxFlat.new()
	sb_bg.bg_color = (cgt.get_theme_tokens_dict(cgt.get_active_theme_id())["surface_base"] as Color)
	shell.add_theme_stylebox_override("panel", sb_bg)

	var vbox := VBoxContainer.new()
	vbox.set_h_size_flags(Control.SIZE_EXPAND_FILL)
	vbox.set_v_size_flags(Control.SIZE_EXPAND_FILL)
	shell.add_child(vbox)

	# Row 0: MenuBar píldoras
	var layers_dlg = ClassDB.instantiate("ClockGearsLayersDialog")
	var ui_h: Dictionary = layers_dlg.call("get_ui_handles")
	var mbar_row: Control = ui_h.get("title_bar_row")
	if mbar_row != null:
		mbar_row.set_v_size_flags(Control.SIZE_SHRINK_BEGIN)
		vbox.add_child(mbar_row)

	# Row 1: TopBar
	var top_bar = ClassDB.instantiate("ClockGearsTopBar")
	top_bar.set_v_size_flags(Control.SIZE_SHRINK_BEGIN)
	vbox.add_child(top_bar)

	# Row 2: MapPanel + CanvasFrame
	var body := HSplitContainer.new()
	body.set_h_size_flags(Control.SIZE_EXPAND_FILL)
	body.set_v_size_flags(Control.SIZE_EXPAND_FILL)
	vbox.add_child(body)

	var map_panel = ClassDB.instantiate("ClockGearsMapPanel")
	map_panel.custom_minimum_size = Vector2(300, 0)
	map_panel.set_v_size_flags(Control.SIZE_EXPAND_FILL)
	body.add_child(map_panel)

	var canvas_frame := PanelContainer.new()
	canvas_frame.theme_type_variation = &"ClockGearsCanvasFrame"
	canvas_frame.set_h_size_flags(Control.SIZE_EXPAND_FILL)
	canvas_frame.set_v_size_flags(Control.SIZE_EXPAND_FILL)
	body.add_child(canvas_frame)

	var canvas_inner := ColorRect.new()
	canvas_inner.color = Color(0.08, 0.09, 0.12, 1.0)
	canvas_inner.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	canvas_frame.add_child(canvas_inner)

	# Row 3: StatusBar
	var status_bar = ClassDB.instantiate("ClockGearsStatusBar")
	status_bar.set_v_size_flags(Control.SIZE_SHRINK_END)
	vbox.add_child(status_bar)

	return {
		"shell": shell,
		"layers_dlg": layers_dlg,
		"top_bar": top_bar,
		"map_panel": map_panel,
		"status_bar": status_bar,
		"bg_sb": sb_bg,
	}

func _initialize() -> void:
	start_msec = Time.get_ticks_msec()
	DirAccess.make_dir_recursive_absolute(CAP_DIR)

	var cgt = Engine.get_singleton("ClockGearsTheme")
	check(cgt != null, "ClockGearsTheme singleton disponible en entorno visual real")
	if cgt == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm != null:
		dm.load_all("res://clockgears_data")

	cgt.set_glow_effects_level(1)

	var cap_vp := SubViewport.new()
	cap_vp.gui_embed_subwindows = true
	cap_vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	cap_vp.size = Vector2i(1366, 768)
	root.add_child(cap_vp)

	var shell_dict := _build_editor_preview_shell(cgt)
	var shell: Control = shell_dict["shell"]
	var top_bar = shell_dict["top_bar"]
	var map_panel = shell_dict["map_panel"]
	cap_vp.add_child(shell)

	# A. Regla 16:9 en las 4 resoluciones (960x540, 1366x768, 1920x1080, 2560x1440) en Dark y Brass
	var resolutions := [Vector2i(960, 540), Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]
	var db = ClassDB.instantiate("ClockGearsDatabaseDialog")
	cap_vp.add_child(db)

	for tid in ["dark", "brass"]:
		print("--- Verificando Punto 7 (16:9 + Capturas) en tema: ", tid, " ---")
		cgt.set_active_theme_id(tid)
		(shell_dict["bg_sb"] as StyleBoxFlat).bg_color = (cgt.get_theme_tokens_dict(tid)["surface_base"] as Color)
		cgt.apply_to_control(shell)
		cgt.apply_to_window(db)

		for res in resolutions:
			cap_vp.size = res
			await _wait_frames(3)

			var ed_rep: Dictionary = cgt.check_tree_legibility_and_layout(shell, res)
			var status_snap: Dictionary = (shell_dict["status_bar"] as Node).call("get_status_snapshot")
			check(bool(ed_rep.get("ok", false)) and not status_snap.is_empty(),
				"[%s %dx%d] Editor (Barra + Panel + Estado) legible (>=4.5:1) y sin desbordar 16:9" % [tid, res.x, res.y])

			db.open_database()
			db.size = res
			db.position = Vector2i.ZERO
			db.call("adapt_to_window_size", res)
			await _wait_frames(2)
			var lay_rep: Dictionary = db.call("check_all_tabs_layout", res)
			check(bool(lay_rep.get("all_ok", false)),
				"[%s %dx%d] Base de Datos 16:9 sin cortes ni scroll horizontal" % [tid, res.x, res.y])
			db.hide()
			await _wait_frames(1)

	# B. Capturas reales (GPU Vulkan, SubViewport exacto) a 1366x768 y 1920x1080 en Dark y Brass
	var cap_resolutions := [Vector2i(1366, 768), Vector2i(1920, 1080)]
	var db_chapters := [
		["cap1_personajes", "actors"],
		["cap2_equipo", "weapons"],
		["cap3_objetos", "items"],
		["cap4_combate", "enemies"],
		["cap5_animaciones", "animations"],
		["cap6_mundo", "tilesets"],
		["cap7_sistema", "system"],
		["cap8_menus_estilos", "window_styles"],
		["cap8_menus_diseno", "ui_designer"],
	]

	for tid in ["dark", "brass"]:
		cgt.set_active_theme_id(tid)
		(shell_dict["bg_sb"] as StyleBoxFlat).bg_color = (cgt.get_theme_tokens_dict(tid)["surface_base"] as Color)
		cgt.apply_to_control(shell)
		cgt.apply_to_window(db)

		for res in cap_resolutions:
			cap_vp.size = res
			db.hide()
			await _wait_frames(4)
			var ed_fname := "editor_%s_%dx%d.png" % [tid, res.x, res.y]
			check(_save_capture(cap_vp, ed_fname), "[%s %dx%d] Captura Editor Principal (%s)" % [tid, res.x, res.y, ed_fname])

			# Mostrar Base de Datos embebida ocupando todo el viewport en esa resolución
			db.open_database()
			db.position = Vector2i.ZERO
			db.size = res
			db.call("adapt_to_window_size", res)
			for ch in db_chapters:
				var ch_tag: String = ch[0]
				var ch_key: String = ch[1]
				if res.x == 1920 and ch_tag != "cap1_personajes" and ch_tag != "cap8_menus_diseno" and ch_tag != "cap7_sistema":
					continue
				db.call("select_category_by_key", ch_key)
				db.size = res
				await _wait_frames(3)
				var db_fname := "db_%s_%s_%dx%d.png" % [tid, ch_tag, res.x, res.y]
				var ok_cap := _save_capture(cap_vp, db_fname)
				check(ok_cap, "[%s %dx%d] Captura Base de Datos (%s)" % [tid, res.x, res.y, db_fname])
			db.hide()

	cgt.set_active_theme_id("dark")
	cgt.set_glow_effects_level(1)
	db.free()
	(shell_dict["layers_dlg"] as Node).free()
	shell.free()
	cap_vp.free()

	print("Tiempo test_neon_p7_verificacion_y_capturas: %d ms" % (Time.get_ticks_msec() - start_msec))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
