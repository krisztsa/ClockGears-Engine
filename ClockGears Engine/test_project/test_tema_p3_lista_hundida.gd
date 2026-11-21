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

func _lum(c: Color) -> float:
	return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b

func _col_eq(a: Color, b: Color, tol := 0.015) -> bool:
	return absf(a.r - b.r) <= tol and absf(a.g - b.g) <= tol and absf(a.b - b.b) <= tol and absf(a.a - b.a) <= tol

func _is_copper_color(c: Color) -> bool:
	# Cobre/latón tiene rojo notablemente más alto que azul (r - b > 0.12)
	return (c.r - c.b) > 0.12 and c.r > 0.35

func _verify_sunken_list(ctrl: Control, panel_bg: Color, expected_bg: Color, expected_bd: Color, expected_cr: int, expected_margin: float, tag: String) -> void:
	var sb := ctrl.get_theme_stylebox("panel")
	_check(sb is StyleBoxFlat, tag + " panel es StyleBoxFlat")
	if not (sb is StyleBoxFlat):
		return
	var sbf := sb as StyleBoxFlat
	var bg := sbf.bg_color
	var bd := sbf.border_color
	_check(_col_eq(bg, expected_bg), "%s fondo (%s == %s)" % [tag, str(bg), str(expected_bg)])
	_check(_lum(bg) < _lum(panel_bg), "%s fondo más oscuro que el panel (%f < %f)" % [tag, _lum(bg), _lum(panel_bg)])
	_check(_col_eq(bd, expected_bd), "%s borde (%s == %s)" % [tag, str(bd), str(expected_bd)])
	_check(_lum(bd) < _lum(panel_bg), "%s borde más oscuro que el panel (%f < %f)" % [tag, _lum(bd), _lum(panel_bg)])
	_check(not _is_copper_color(bd), "%s borde NO es cobrizo (%s)" % [tag, str(bd)])
	_check(sbf.border_width_left == 1 and sbf.border_width_top == 1 and sbf.border_width_right == 1 and sbf.border_width_bottom == 1, "%s borde == 1 px" % tag)
	_check(sbf.corner_radius_top_left == expected_cr and sbf.corner_radius_bottom_right == expected_cr, "%s corner_radius == %d" % [tag, expected_cr])
	_check(is_equal_approx(sbf.content_margin_left, expected_margin) and is_equal_approx(sbf.content_margin_top, expected_margin) and is_equal_approx(sbf.content_margin_right, expected_margin) and is_equal_approx(sbf.content_margin_bottom, expected_margin), "%s relleno interno == %s (got %s)" % [tag, str(expected_margin), str(sbf.content_margin_left)])

	var sb_sel := ctrl.get_theme_stylebox("selected") as StyleBoxFlat
	var sb_hov := ctrl.get_theme_stylebox("hovered") as StyleBoxFlat
	_check(sb_sel != null and sb_sel.bg_color.a > 0.1, tag + " mantiene resaltado selected")
	_check(sb_hov != null and (sb_hov.bg_color.a > 0.04 or sb_hov.bg_color != bg), tag + " tiene resaltado suave hovered")
	if ctrl is Tree:
		var sb_hs := ctrl.get_theme_stylebox("hovered_selected") as StyleBoxFlat
		_check(sb_hs != null, tag + " Tree tiene hovered_selected")

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

	# 1. Tokens y StyleBoxes dentro del tema ("lista hundida") en dark y brass
	th.set_active_theme_id("dark")
	var dark_th: Theme = th.get_theme(false)
	_check(dark_th.has_color("sunken_list_bg", "ClockGears"), "Dark theme token sunken_list_bg existe")
	_check(dark_th.has_color("sunken_list_border", "ClockGears"), "Dark theme token sunken_list_border existe")
	_check(dark_th.get_constant("sunken_list_border_width", "ClockGears") == 1, "Dark sunken_list_border_width == 1")
	_check(dark_th.get_constant("sunken_list_corner_radius", "ClockGears") == 6, "Dark sunken_list_corner_radius == 6")
	_check(dark_th.get_constant("sunken_list_margin", "ClockGears") == 6, "Dark sunken_list_margin == 6")

	th.set_active_theme_id("brass")
	var brass_th: Theme = th.get_brass_theme()
	_check(brass_th.has_color("sunken_list_bg", "ClockGears"), "Brass theme token sunken_list_bg existe")
	_check(brass_th.has_color("sunken_list_border", "ClockGears"), "Brass theme token sunken_list_border existe")

	# 2. Instanciar Árbol de mapas, Base de Datos, Editor de Eventos y Diálogo de Preferencias
	var map_panel := ClockGearsMapPanel.new()
	var db := ClockGearsDatabaseDialog.new()
	var ev := ClockGearsEventEditorDialog.new()
	var sample_tree := Tree.new()
	var sample_list := ItemList.new()
	root.add_child(map_panel)
	root.add_child(db)
	root.add_child(ev)
	root.add_child(sample_tree)
	root.add_child(sample_list)

	var map_tree := _find_first_by_name(map_panel, "ClockGearsMapTree") as Tree
	var maps_p := _find_first_by_name(map_panel, "ClockGearsMapsPanel") as PanelContainer
	_check(map_tree != null and maps_p != null, "Árbol de mapas y su panel contenedor encontrados")

	var all_db_lists := db.find_children("*", "ItemList", true, false)
	var all_db_trees := db.find_children("*", "Tree", true, false)
	var all_ev_lists := ev.find_children("*", "ItemList", true, false)
	var all_ev_trees := ev.find_children("*", "Tree", true, false)
	_check(all_db_lists.size() >= 3 and all_db_trees.size() >= 3, "BD tiene múltiples listas planas y árboles (%d lists, %d trees)" % [all_db_lists.size(), all_db_trees.size()])
	_check(all_ev_lists.size() >= 2 and all_ev_trees.size() >= 1, "Editor de Eventos tiene listas y árboles (%d lists, %d trees)" % [all_ev_lists.size(), all_ev_trees.size()])

	for mode in ["dark", "brass"]:
		th.set_active_theme_id(mode)
		var cur_th: Theme = brass_th if mode == "brass" else dark_th
		sample_tree.theme = cur_th
		sample_list.theme = cur_th
		th.apply_to_control(map_panel)
		th.apply_to_window(db)
		th.apply_to_window(ev)
		await process_frame
		await process_frame

		var exp_bg: Color = cur_th.get_color("sunken_list_bg", "ClockGears")
		var exp_bd: Color = cur_th.get_color("sunken_list_border", "ClockGears")
		var exp_cr: int = cur_th.get_constant("sunken_list_corner_radius", "ClockGears")
		var exp_mg := float(cur_th.get_constant("sunken_list_margin", "ClockGears"))
		var panel_sb := maps_p.get_theme_stylebox("panel") as StyleBoxFlat
		var panel_bg := panel_sb.bg_color

		_verify_sunken_list(map_tree, panel_bg, exp_bg, exp_bd, exp_cr, exp_mg, mode + " Árbol de mapas")
		_verify_sunken_list(sample_tree, panel_bg, exp_bg, exp_bd, exp_cr, exp_mg, mode + " Tree genérico de tema")
		_verify_sunken_list(sample_list, panel_bg, exp_bg, exp_bd, exp_cr, exp_mg, mode + " ItemList genérico de tema")

		# Verificar TODAS las listas planas de BD y EventEditor
		var list_idx := 0
		for n in all_db_lists:
			var il := n as Control
			_verify_sunken_list(il, panel_bg, exp_bg, exp_bd, exp_cr, exp_mg, "%s DB ItemList[%d:%s]" % [mode, list_idx, str(il.name)])
			list_idx += 1
		var tree_idx := 0
		for n in all_db_trees:
			var tr := n as Control
			_verify_sunken_list(tr, panel_bg, exp_bg, exp_bd, exp_cr, exp_mg, "%s DB Tree[%d:%s]" % [mode, tree_idx, str(tr.name)])
			tree_idx += 1
		for n in all_ev_lists:
			_verify_sunken_list(n as Control, panel_bg, exp_bg, exp_bd, exp_cr, exp_mg, "%s EventEditor ItemList:%s" % [mode, str(n.name)])
		for n in all_ev_trees:
			_verify_sunken_list(n as Control, panel_bg, exp_bg, exp_bd, exp_cr, exp_mg, "%s EventEditor Tree:%s" % [mode, str(n.name)])

	# Verificar también get_sunken_list_stylebox API y tema dark restaurado
	th.set_active_theme_id("dark")
	var api_sb: StyleBoxFlat = th.get_sunken_list_stylebox(1.0)
	_check(api_sb != null and api_sb.border_width_left == 1 and api_sb.corner_radius_top_left == 6 and is_equal_approx(api_sb.content_margin_left, 6.0), "API get_sunken_list_stylebox coherente")

	map_panel.queue_free()
	db.queue_free()
	ev.queue_free()
	sample_tree.queue_free()
	sample_list.queue_free()
	await process_frame

	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
