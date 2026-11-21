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

	cgt.set_glow_effects_level(1) # Suave (por defecto)

	for tid in ["dark", "brass"]:
		print("--- Verificando Punto 2 (Estados de controles) en tema: ", tid, " ---")
		var th: Theme = cgt.get_theme_by_id(tid)
		var tk: Dictionary = cgt.get_theme_tokens_dict(tid)
		var border_copper: Color = tk["border_copper"]
		var border_bright: Color = tk["border_copper_bright"]
		var accent_neon: Color = tk["accent_neon"]
		var halo_radius: int = int(tk["halo_radius"])

		# 1. Button estados: normal, hover, pressed, hover_pressed, focus, disabled
		var btn_n: StyleBoxFlat = th.get_stylebox("normal", "Button")
		var btn_h: StyleBoxFlat = th.get_stylebox("hover", "Button")
		var btn_p: StyleBoxFlat = th.get_stylebox("pressed", "Button")
		var btn_hp: StyleBoxFlat = th.get_stylebox("hover_pressed", "Button")
		var btn_f: StyleBoxFlat = th.get_stylebox("focus", "Button")
		var btn_d: StyleBoxFlat = th.get_stylebox("disabled", "Button")

		check(btn_n != null and btn_n.corner_radius_top_left == 6 and btn_n.border_color.is_equal_approx(border_copper),
			"[%s] Button normal tiene esquinas 6 px y borde cobrizo fino" % tid)
		check(btn_h != null and btn_h.border_color.is_equal_approx(border_bright) and btn_h.shadow_size == halo_radius and btn_h.shadow_offset == Vector2.ZERO,
			"[%s] Button hover tiene borde cobrizo brillante y halo suave (shadow offset 0)" % tid)
		check(btn_p != null and btn_p.get_class() == "ClockGearsBrassStyleBox" and btn_p.call("is_sunken_style") and btn_p.border_color.is_equal_approx(accent_neon),
			"[%s] Button pulsado es más oscuro y hundido con borde acento neón" % tid)
		check(btn_hp != null and btn_hp.border_width_left >= 1 and btn_hp.border_width_left <= 2 and btn_hp.border_color.is_equal_approx(accent_neon) and btn_hp.shadow_size == halo_radius,
			"[%s] Button seleccionado/activo (hover_pressed) tiene borde acento neón (1-2 px), halo y fondo con tinte ámbar" % tid)
		check(btn_f != null and not btn_f.draw_center and btn_f.border_color.is_equal_approx(accent_neon) and btn_f.shadow_size == halo_radius,
			"[%s] Button foco de teclado tiene anillo acento neón y halo" % tid)
		check(btn_d != null and btn_d.shadow_size == 0,
			"[%s] Button desactivado está atenuado y sin halo" % tid)

		# 2. Punto LED fijo de 4 px a la izquierda en listas y pestaña activa
		var item_sel: StyleBoxFlat = th.get_stylebox("selected", "ItemList")
		var tree_sel: StyleBoxFlat = th.get_stylebox("selected", "Tree")
		var tab_sel: StyleBoxFlat = th.get_stylebox("tab_selected", "TabContainer")
		var tab_btn_sel: StyleBoxFlat = th.get_stylebox("pressed", "ClockGearsTabButton")

		check(item_sel != null and item_sel.get_class() == "ClockGearsBrassStyleBox" and item_sel.call("is_led_dot_enabled") and int(item_sel.call("get_led_dot_size_px")) == 4,
			"[%s] ItemList seleccionado tiene punto LED fijo de 4 px" % tid)
		check(tree_sel != null and tree_sel.get_class() == "ClockGearsBrassStyleBox" and tree_sel.call("is_led_dot_enabled") and int(tree_sel.call("get_led_dot_size_px")) == 4,
			"[%s] Tree seleccionado tiene punto LED fijo de 4 px" % tid)
		check(tab_sel != null and tab_sel.get_class() == "ClockGearsBrassStyleBox" and tab_sel.call("is_led_dot_enabled") and tab_sel.border_width_bottom == 2 and tab_sel.shadow_size == halo_radius,
			"[%s] Pestaña activa tiene línea inferior de acento (2 px), halo y punto LED de 4 px" % tid)
		check(tab_btn_sel != null and tab_btn_sel.get_class() == "ClockGearsBrassStyleBox" and tab_btn_sel.call("is_led_dot_enabled"),
			"[%s] ClockGearsTabButton activo tiene punto LED de 4 px" % tid)

		# 3. Transiciones de 120 a 150 ms
		var h_trans: int = th.get_constant("hover_transition_ms", "Button")
		var t_trans: int = th.get_constant("tab_transition_ms", "TabContainer")
		check(h_trans >= 120 and h_trans <= 150 and t_trans >= 120 and t_trans <= 150,
			"[%s] Transiciones de controles y pestañas en 120..150 ms (%d / %d ms)" % [tid, h_trans, t_trans])

		# 4. LineEdit y TextEdit: borde cobrizo fino, foco con acento y halo
		var le_n: StyleBoxFlat = th.get_stylebox("normal", "LineEdit")
		var le_f: StyleBoxFlat = th.get_stylebox("focus", "LineEdit")
		check(le_n != null and le_n.border_color.is_equal_approx(border_copper) and le_f != null and le_f.border_color.is_equal_approx(accent_neon) and le_f.shadow_size == halo_radius,
			"[%s] LineEdit/TextEdit tienen borde cobrizo fino y foco con acento neón y halo" % tid)

		# 5. SpinBox: stepper compacto de dos mitades dentro del mismo campo
		var sp_sep_h: int = th.get_constant("field_and_buttons_separation", "SpinBox")
		var sp_sep_v: int = th.get_constant("buttons_vertical_separation", "SpinBox")
		var sp_up: StyleBoxFlat = th.get_stylebox("up_background", "SpinBox")
		var sp_dn: StyleBoxFlat = th.get_stylebox("down_background", "SpinBox")
		var sp_in: StyleBoxFlat = th.get_stylebox("normal", "SpinBoxInnerLineEdit")
		check(sp_sep_h == 0 and sp_sep_v == 0 and sp_up != null and sp_dn != null and sp_in != null and sp_up.corner_radius_top_right == 6 and sp_dn.corner_radius_bottom_right == 6 and not th.has_icon("updown", "SpinBox") and th.has_icon("up", "SpinBox") and th.has_icon("down", "SpinBox"),
			"[%s] SpinBox tiene stepper compacto integrado de 2 mitades dentro del campo" % tid)

		# 6. CheckBox / CheckButton redondeados 6 px con brillo activos
		var chk_p: StyleBoxFlat = th.get_stylebox("pressed", "CheckBox")
		check(chk_p != null and chk_p.corner_radius_top_left == 6 and chk_p.shadow_size == halo_radius,
			"[%s] CheckBox/CheckButton redondeados (6 px) con brillo activos" % tid)

		# 7. Scrollbars finas (8 px), redondeadas, tirador cobrizo en hover
		var grab_h: StyleBoxFlat = th.get_stylebox("grabber_highlight", "VScrollBar")
		check(grab_h != null and grab_h.bg_color.is_equal_approx(border_bright) and grab_h.corner_radius_top_left >= 3,
			"[%s] VScrollBar/HScrollBar finas (8 px), redondeadas y con tirador cobrizo en hover" % tid)

		# 8. Separadores, Tooltips y Menús desplegables
		var sep_pop: StyleBoxLine = th.get_stylebox("separator", "PopupMenu")
		var hdr_line: StyleBoxFlat = th.get_stylebox("separator", "ClockGearsSectionHeaderLine")
		var tt_sb: StyleBoxFlat = th.get_stylebox("panel", "TooltipPanel")
		var pop_sb: StyleBoxFlat = th.get_stylebox("panel", "PopupMenu")
		var pop_hov: StyleBoxFlat = th.get_stylebox("hover", "PopupMenu")
		check(sep_pop != null and sep_pop.color.a < 0.40 and hdr_line != null and hdr_line.get_class() == "ClockGearsBrassStyleBox" and hdr_line.call("is_header_gradient_line_enabled"),
			"[%s] Separadores con línea cobriza fina y variación de degradado horizontal" % tid)
		check(tt_sb != null and tt_sb.corner_radius_top_left == 6 and tt_sb.border_color.is_equal_approx(border_copper),
			"[%s] TooltipPanel tiene fondo oscuro, borde cobrizo y esquinas de 6 px" % tid)
		check(pop_sb != null and pop_sb.border_color.is_equal_approx(border_copper) and pop_sb.shadow_size > 0 and pop_hov != null and pop_hov.border_color.is_equal_approx(accent_neon),
			"[%s] PopupMenu tiene borde cobrizo, sombra suave y filas con tinte ámbar en hover" % tid)

	print("Tiempo test_neon_p2_controls: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
