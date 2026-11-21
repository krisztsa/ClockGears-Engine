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

func _col_eq(a: Color, b: Color, tol := 0.015) -> bool:
	return absf(a.r - b.r) <= tol and absf(a.g - b.g) <= tol and absf(a.b - b.b) <= tol and absf(a.a - b.a) <= tol

func _verify_container_sb(sb: StyleBox, expected_bd: Color, min_bw: int, expected_cr: int, min_margin_x: float, min_margin_y: float, tag: String) -> void:
	_check(sb is StyleBoxFlat, tag + " es StyleBoxFlat")
	if not (sb is StyleBoxFlat):
		return
	var sbf := sb as StyleBoxFlat
	var bd := sbf.border_color
	_check(_col_eq(bd, expected_bd), "%s border_color (%s == %s)" % [tag, str(bd), str(expected_bd)])
	_check(sbf.border_width_left == min_bw and sbf.border_width_top == min_bw and sbf.border_width_right == min_bw and sbf.border_width_bottom == min_bw, "%s border_width == %d (got %d/%d/%d/%d)" % [tag, min_bw, sbf.border_width_left, sbf.border_width_top, sbf.border_width_right, sbf.border_width_bottom])
	_check(sbf.corner_radius_top_left == expected_cr and sbf.corner_radius_top_right == expected_cr and sbf.corner_radius_bottom_left == expected_cr and sbf.corner_radius_bottom_right == expected_cr, "%s corner_radius == %d (got %d)" % [tag, expected_cr, sbf.corner_radius_top_left])
	_check(sbf.content_margin_left >= min_margin_x and sbf.content_margin_right >= min_margin_x and sbf.content_margin_top >= min_margin_y and sbf.content_margin_bottom >= min_margin_y, "%s content_margin >= (%s,%s) (got %s,%s)" % [tag, str(min_margin_x), str(min_margin_y), str(sbf.content_margin_left), str(sbf.content_margin_top)])

func _find_first_by_name(node: Node, target_name: String) -> Node:
	if node == null:
		return null
	if node.name == target_name:
		return node
	for ch in node.get_children():
		var found := _find_first_by_name(ch, target_name)
		if found != null:
			return found
	return null

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

func _run_test() -> void:
	var th = Engine.get_singleton("ClockGearsTheme")
	_check(th != null, "ClockGearsTheme singleton existe")
	if th == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	# Referencias exactas del borde píldora del menú superior ("Capas · Suelo")
	var expected_dark_bd := Color(0.88, 0.63, 0.25, 0.62)
	var brass_base := Color.html("#c9923e")
	var expected_brass_bd := Color(brass_base.r, brass_base.g, brass_base.b, 0.65)

	# 1. Tokens y StyleBoxes dentro del sistema de temas (dark & brass)
	th.set_active_theme_id("dark")
	var dark_th: Theme = th.get_theme(false)
	_check(dark_th != null, "Tema dark válido")
	_check(_col_eq(dark_th.get_color("container_border", "ClockGears"), expected_dark_bd), "Dark theme token container_border coincide con píldora Capas")
	_check(dark_th.get_constant("container_border_width", "ClockGears") == 1, "Dark theme container_border_width == 1")
	_check(dark_th.get_constant("container_corner_radius", "ClockGears") >= 6, "Dark theme container_corner_radius >= 6")
	var dark_r: int = dark_th.get_constant("container_corner_radius", "ClockGears")
	_verify_container_sb(dark_th.get_stylebox("panel", "PanelContainer"), expected_dark_bd, 1, dark_r, 6.0, 6.0, "Dark PanelContainer")
	_verify_container_sb(dark_th.get_stylebox("panel", "ClockGearsBarContainer"), expected_dark_bd, 1, dark_r, 6.0, 4.0, "Dark ClockGearsBarContainer")
	_verify_container_sb(dark_th.get_stylebox("panel", "ClockGearsSubBarContainer"), expected_dark_bd, 1, dark_r, 6.0, 4.0, "Dark ClockGearsSubBarContainer")
	_verify_container_sb(dark_th.get_stylebox("panel", "TabContainer"), expected_dark_bd, 1, dark_r, 6.0, 6.0, "Dark TabContainer.panel")
	_verify_container_sb(dark_th.get_stylebox("panel", "ClockGearsCanvasFrame"), expected_dark_bd, 1, dark_r, 0.0, 0.0, "Dark ClockGearsCanvasFrame")

	th.set_active_theme_id("brass")
	var brass_th: Theme = th.get_brass_theme()
	_check(brass_th != null, "Tema brass válido")
	_check(_col_eq(brass_th.get_color("container_border", "ClockGears"), expected_brass_bd), "Brass theme token container_border coincide con MainMenuBar Latón")
	var brass_r: int = brass_th.get_constant("container_corner_radius", "ClockGears")
	_verify_container_sb(brass_th.get_stylebox("panel", "PanelContainer"), expected_brass_bd, 1, brass_r, 6.0, 6.0, "Brass PanelContainer")
	_verify_container_sb(brass_th.get_stylebox("panel", "ClockGearsBarContainer"), expected_brass_bd, 1, brass_r, 6.0, 4.0, "Brass ClockGearsBarContainer")
	_verify_container_sb(brass_th.get_stylebox("panel", "TabContainer"), expected_brass_bd, 1, brass_r, 6.0, 6.0, "Brass TabContainer.panel")
	_verify_container_sb(brass_th.get_stylebox("panel", "ClockGearsCanvasFrame"), expected_brass_bd, 1, brass_r, 0.0, 0.0, "Brass ClockGearsCanvasFrame")

	# 2. Instanciar contenedores reales y validar ambos temas
	var top_bar := ClockGearsTopBar.new()
	var map_panel := ClockGearsMapPanel.new()
	var db := ClockGearsDatabaseDialog.new()
	var ev := ClockGearsEventEditorDialog.new()
	var cam := ClockGearsCameraDialog.new()
	root.add_child(top_bar)
	root.add_child(map_panel)
	root.add_child(db)
	root.add_child(ev)
	root.add_child(cam)

	var tileset_p := _find_first_by_name(map_panel, "ClockGearsTilesetPanel") as Control
	var maps_p := _find_first_by_name(map_panel, "ClockGearsMapsPanel") as Control
	var db_chap := _find_first_by_name(db, "ClockGearsDbChaptersPanel") as Control
	var db_cats := _find_first_by_name(db, "ClockGearsDbCatsPanel") as Control
	var db_tabs := _find_first_by_name(db, "ClockGearsDbMainTabs") as TabContainer
	var db_ts_col := _find_first_by_name(db, "ClockGearsDbTilesetCollisionPanel") as Control
	var db_arpg_p := _find_first_by_name(db, "ClockGearsDbSysArpgPanel") as Control
	_check(tileset_p != null and maps_p != null, "Paneles Paleta de tiles y Árbol de mapas encontrados")
	_check(db_chap != null and db_cats != null and db_tabs != null, "Contenedores de Base de Datos encontrados")

	for mode in ["dark", "brass"]:
		th.set_active_theme_id(mode)
		th.apply_to_control(top_bar)
		th.apply_to_control(map_panel)
		th.apply_to_window(db)
		th.apply_to_window(ev)
		th.apply_to_window(cam)
		await process_frame
		await process_frame

		var exp_bd := expected_brass_bd if mode == "brass" else expected_dark_bd
		var exp_r := brass_r if mode == "brass" else dark_r
		_check(_col_eq(ClockGearsTheme.get_active_container_border_color(), exp_bd), mode + ": get_active_container_border_color coincide")

		_verify_container_sb(top_bar.get_theme_stylebox("panel"), exp_bd, 1, exp_r, 6.0, 3.0, mode + " ClockGearsTopBar")
		_verify_container_sb(tileset_p.get_theme_stylebox("panel"), exp_bd, 1, exp_r, 6.0, 6.0, mode + " Paleta de tiles")
		_verify_container_sb(maps_p.get_theme_stylebox("panel"), exp_bd, 1, exp_r, 6.0, 6.0, mode + " Árbol de mapas")

		# Recuadro del lienzo del mapa
		var canvas_sb: StyleBoxFlat = th.get_container_stylebox(1.0, 0, 0, 3)
		_verify_container_sb(canvas_sb, exp_bd, 1, 6, 0.0, 0.0, mode + " Recuadro lienzo mapa (variant 3)")
		_check(canvas_sb != null and not canvas_sb.draw_center, mode + " Recuadro lienzo mapa transparente en el centro")

		# Base de datos en diferentes categorías / pestañas internas
		_verify_container_sb(db_chap.get_theme_stylebox("panel"), exp_bd, 1, exp_r, 6.0, 4.0, mode + " DB Barra capítulos")
		_verify_container_sb(db_cats.get_theme_stylebox("panel"), exp_bd, 1, exp_r, 6.0, 4.0, mode + " DB Barra categorías")
		_verify_container_sb(db_tabs.get_theme_stylebox("panel", "TabContainer"), exp_bd, 1, exp_r, 6.0, 6.0, mode + " DB main_editor_tabs")
		if db_ts_col != null:
			_verify_container_sb(db_ts_col.get_theme_stylebox("panel"), exp_bd, 1, exp_r, 6.0, 4.0, mode + " DB tileset_collision_panel")
		if db_arpg_p != null:
			_verify_container_sb(db_arpg_p.get_theme_stylebox("panel"), exp_bd, 1, exp_r, 6.0, 6.0, mode + " DB sys_arpg_realtime_panel")

		# Cambiar categorías y comprobar tabs internos de Enemigos y Tilesets
		db.select_category_by_key("enemigos")
		await process_frame
		_verify_container_sb(db_tabs.get_theme_stylebox("panel", "TabContainer"), exp_bd, 1, exp_r, 6.0, 6.0, mode + " DB Enemigos main_editor_tabs")
		db.select_category_by_key("tilesets")
		await process_frame
		_verify_container_sb(db_tabs.get_theme_stylebox("panel", "TabContainer"), exp_bd, 1, exp_r, 6.0, 6.0, mode + " DB Tilesets main_editor_tabs")

		# Legibilidad / ajustes sin cortes a 1366x768 y 1920x1080
		for res in [Vector2i(1366, 768), Vector2i(1920, 1080)]:
			var leg_db: Dictionary = th.check_tree_legibility_and_layout(db, res)
			_check(bool(leg_db.get("ok", false)), "%s %s BD legibilidad/ajuste OK (%s)" % [mode, str(res), str(leg_db.get("worst_control", ""))])

	# Restaurar modo dark limpio
	th.set_active_theme_id("dark")
	top_bar.queue_free()
	map_panel.queue_free()
	db.queue_free()
	ev.queue_free()
	cam.queue_free()
	await process_frame

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
