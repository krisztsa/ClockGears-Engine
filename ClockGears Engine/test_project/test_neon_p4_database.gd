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

func _find_controls_by_type_var(root: Node, tvar: StringName) -> Array:
	var out: Array = []
	var stack: Array = [root]
	while not stack.is_empty():
		var n: Node = stack.pop_back()
		if n is Control and (n as Control).theme_type_variation == tvar:
			out.append(n)
		for i in range(n.get_child_count()):
			stack.push_back(n.get_child(i))
	return out

func _find_nodes_of_class(root: Node, cname: String) -> Array:
	var out: Array = []
	var stack: Array = [root]
	while not stack.is_empty():
		var n: Node = stack.pop_back()
		if n.get_class() == cname:
			out.append(n)
		for i in range(n.get_child_count()):
			stack.push_back(n.get_child(i))
	return out

func _initialize() -> void:
	var t0 := Time.get_ticks_msec()
	var cgt = Engine.get_singleton("ClockGearsTheme")
	check(cgt != null, "ClockGearsTheme singleton disponible")
	if cgt == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	cgt.set_glow_effects_level(1)

	for tid in ["dark", "brass"]:
		print("--- Verificando Punto 4 (Base de Datos) en tema: ", tid, " ---")
		cgt.set_active_theme_id(tid)
		var tk: Dictionary = cgt.get_theme_tokens_dict(tid)
		var surf_panel: Color = tk["surface_panel"]
		var border_copper: Color = tk["border_copper"]
		var accent_neon: Color = tk["accent_neon"]

		var db = ClassDB.instantiate("ClockGearsDatabaseDialog")
		db.size = Vector2i(1366, 768)
		get_root().add_child(db)
		cgt.apply_to_window(db)
		await process_frame

		# 1. Capítulos y categorías
		var chap_panel = db.find_child("ClockGearsDbChaptersPanel", true, false)
		var cats_panel = db.find_child("ClockGearsDbCatsPanel", true, false)
		var tab_btns: Array = _find_controls_by_type_var(chap_panel, &"ClockGearsTabButton") if chap_panel else []
		var cat_btns: Array = _find_controls_by_type_var(cats_panel, &"ClockGearsTabButton") if cats_panel else []
		var active_chap_ok := false
		for b in tab_btns:
			if b is Button and b.button_pressed:
				var sb: StyleBox = b.get_theme_stylebox("pressed")
				if sb and sb.get_class() == "ClockGearsBrassStyleBox" and sb.call("is_led_dot_enabled"):
					active_chap_ok = true
		var cat_icon_ok := false
		for b in cat_btns:
			if b is Button and b.visible and b.icon != null:
				if b.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST and b.get_theme_color("icon_normal_color") == Color(1, 1, 1, 1):
					cat_icon_ok = true
					break
		check(tab_btns.size() >= 8 and active_chap_ok, "[%s] Barra de capítulos usa pestañas del punto 2 con punto LED en la activa" % tid)
		check(cat_btns.size() >= 10 and cat_icon_ok, "[%s] Categorías usan iconos pixel-art nearest sin teñir y pestañas del punto 2" % tid)

		# 2. Pestañas internas (sin franja negra a la derecha, línea inferior de acento)
		var tabs: TabContainer = db.call("get_main_editor_tabs")
		check(tabs != null, "[%s] TabContainer principal de la BD presente" % tid)
		if tabs != null:
			var tbbg: StyleBoxFlat = tabs.get_theme_stylebox("tabbar_background") as StyleBoxFlat
			var tbsel: StyleBoxFlat = tabs.get_theme_stylebox("tab_selected") as StyleBoxFlat
			check(tbbg != null and tbbg.bg_color.is_equal_approx(surf_panel),
				"[%s] Pestañas internas sin franja negra a la derecha (tabbar_background = surface_panel)" % tid)
			check(tbsel != null and tbsel.border_width_bottom == 2 and tbsel.border_color.is_equal_approx(accent_neon) and tbsel.get_class() == "ClockGearsBrassStyleBox" and tbsel.call("is_led_dot_enabled"),
				"[%s] Pestaña interna activa con línea inferior de acento y LED de 4 px" % tid)

		# 3. Lista de registros izquierda (campo hundido, tarjetas con icono, selección neón + LED)
		var rlists: Array = _find_controls_by_type_var(db, &"ClockGearsCardItemList")
		check(rlists.size() >= 1, "[%s] Lista de registros usa ClockGearsCardItemList" % tid)
		if rlists.size() >= 1:
			var rlist: ItemList = rlists[0]
			var rl_card: StyleBoxFlat = rlist.get_theme_stylebox("normal")
			var rl_sel: StyleBoxFlat = rlist.get_theme_stylebox("selected")
			check(rl_card != null and rl_card.corner_radius_top_left == 6 and rlist.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST,
				"[%s] Filas de la lista izquierda dibujadas como tarjetas (6 px) con iconos nearest" % tid)
			check(rl_sel != null and rl_sel.border_color.is_equal_approx(accent_neon) and rl_sel.get_class() == "ClockGearsBrassStyleBox" and rl_sel.call("is_led_dot_enabled"),
				"[%s] Fila seleccionada de lista izquierda tiene borde neón y punto LED 4 px" % tid)

		# 4. Cabeceras de sección con línea degradada cobrizo -> transparente y tablas
		var grad_seps: Array = _find_controls_by_type_var(db, &"ClockGearsSectionHeaderLine")
		check(grad_seps.size() >= 4, "[%s] Títulos de sección tienen línea con degradado cobrizo -> transparente (%d)" % [tid, grad_seps.size()])

		var trees: Array = _find_nodes_of_class(db, "Tree")
		var tbl_ok := false
		for t in trees:
			if t is Tree and t.columns >= 3 and t.column_titles_visible:
				var th_sb: StyleBoxFlat = t.get_theme_stylebox("title_button_normal")
				var th_c: Color = t.get_theme_color("title_button_color")
				if th_sb != null and th_sb.border_width_bottom >= 1 and th_sb.border_color.is_equal_approx(border_copper) and th_c.is_equal_approx(accent_neon):
					tbl_ok = true
					break
		check(tbl_ok, "[%s] Tablas (rasgos, efectos, etc.) con cabecera de línea cobriza" % tid)

		# 5. Previsualizaciones con ClockGearsPreviewFrame
		var prev_frames: Array = _find_controls_by_type_var(db, &"ClockGearsPreviewFrame")
		var prev_ok := prev_frames.size() >= 3
		if prev_ok:
			var pf: PanelContainer = prev_frames[0]
			var pf_sb: StyleBoxFlat = pf.get_theme_stylebox("panel")
			prev_ok = (pf_sb != null and pf_sb.border_color.is_equal_approx(border_copper) and pf_sb.shadow_size > 0)
		check(prev_ok, "[%s] Previsualizaciones tienen marco cobrizo con halo sutil (%d frames)" % [tid, prev_frames.size()])

		# 6. Botones OK (primario) y Aplicar (secundario + atenuado si no hay cambios)
		var ok_btn: Button = db.call("get_ok_button")
		check(ok_btn != null and ok_btn.theme_type_variation == &"ClockGearsPrimaryButton",
			"[%s] Botón OK con estilo primario del tema (ClockGearsPrimaryButton)" % tid)
		var apply_btn: Button = db.call("get_apply_button")
		check(apply_btn != null and apply_btn.disabled and apply_btn.modulate.a < 0.5,
			"[%s] Botón Aplicar secundario y atenuado cuando no hay cambios pendientes" % tid)

		# 7. Capítulo Menús: Estilos de ventana y Diseñador UI sin overrides duros
		db.call("select_category", 112) # DB_CAT_WINDOW_STYLES
		var ws_h: Dictionary = db.call("get_window_styles_ui_handles")
		var ws_tabs: TabBar = ws_h.get("btn_state_tabs")
		var ws_prev: Control = ws_h.get("preview_backdrop")
		if ws_tabs != null:
			var ws_sel: StyleBoxFlat = ws_tabs.get_theme_stylebox("tab_selected")
			check(not ws_tabs.has_theme_stylebox_override("tab_selected") and ws_sel != null and ws_sel.get_class() == "ClockGearsBrassStyleBox",
				"[%s] Pestañas de Estilos de ventana heredan el estilo Neón Cobre del tema" % tid)
		if ws_prev != null:
			check(ws_prev.theme_type_variation == &"ClockGearsPreviewFrame",
				"[%s] Vista previa de Estilos de ventana usa ClockGearsPreviewFrame" % tid)

		# 8. Ajuste 16:9 sin solapes ni scroll horizontal
		db.call("select_category", 0)
		db.call("adapt_to_window_size", Vector2i(1366, 768))
		var lay_rep: Dictionary = db.call("check_all_tabs_layout", Vector2i(1366, 768))
		check(bool(lay_rep.get("all_ok", false)),
			"[%s] Base de Datos ajusta en 1366x768 sin cortes ni scroll horizontal" % tid)

		db.free()

	print("Tiempo test_neon_p4_database: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
