extends SceneTree

# Test Punto 6 (Segunda Pasada UI 9B): Estilo, forma y ocultado por tipo de elemento
# - Oculta propiedades que no aplican segun el tipo seleccionado (frame, image, text, button, bar, hearts, currency)
# - Cambiar estilo, forma (rect/rounded/pill/circle/ellipse + clip), radio, relleno, degradado, borde, sombra, brillo, opacidad, texto, fuente y tamano altera el lienzo (compute_image_metrics) y F5

var pass_count: int = 0
var fail_count: int = 0
var _finished: bool = false

func _check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] " + msg)
	else:
		fail_count += 1
		printerr("  [FAIL] " + msg)

func _finish(code: int) -> void:
	if _finished:
		return
	_finished = true
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(code)

func _step_frames(n: int = 2) -> void:
	var c := mini(n, 600)
	for _i in range(c):
		await process_frame

func _select_opt_meta(opt: OptionButton, meta: String) -> bool:
	if opt == null:
		return false
	for i in range(opt.item_count):
		if str(opt.get_item_metadata(i)) == meta:
			opt.select(i)
			opt.emit_signal("item_selected", i)
			return true
	return false

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 6 (SEGUNDA PASADA UI 9B): ESTILO, FORMA Y PROPIEDADES POR TIPO ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()
	dm.reset_ui_screen_design_to_default("game_menu_main", false)

	# Imagen de prueba para elemento image
	var test_img := Image.create_empty(32, 32, false, Image.FORMAT_RGBA8)
	test_img.fill(Color(0.2, 0.85, 0.45, 1.0))
	var abs_png := ProjectSettings.globalize_path("user://test_p6_box.png")
	test_img.save_png(abs_png)

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("game_menu")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	ed.set_props_advanced_mode(true)
	await _step_frames(2)

	var h: Dictionary = ed.get_ui_handles()
	var stage: Control = h.get("canvas_stage", null) as Control
	var sr := stage.get_global_rect()
	var stage_r := Rect2i(Vector2i(sr.position), Vector2i(sr.size))

	var text_row: Control = h.get("prop_text_row", null) as Control
	var img_row: Control = h.get("prop_image_row", null) as Control
	var act_row: Control = h.get("prop_action_row", null) as Control
	var bind_row: Control = h.get("prop_binding_row", null) as Control
	var font_sz_sl: HSlider = h.get("prop_font_size_slider", null) as HSlider

	# 1) Ocultacion por tipo en inspector
	var frm_id: String = ed.add_element_at("frame", Vector2i(180, 120))
	await _step_frames(2)
	_check(not text_row.visible and not act_row.visible and not bind_row.visible and img_row.visible, "Tipo 'frame' oculta Texto/Acciones/Enlace y mantiene Imagen")

	var img_id: String = ed.drop_image_file_on_canvas(abs_png, Vector2i(360, 120))
	await _step_frames(2)
	_check(not text_row.visible and not act_row.visible and not bind_row.visible and img_row.visible and not font_sz_sl.is_visible_in_tree(), "Tipo 'image' oculta Texto/Fuente/Acciones/Enlace")

	var hrt_id: String = ed.add_element_at("hearts", Vector2i(500, 120))
	await _step_frames(2)
	_check(not text_row.visible and not img_row.visible and not act_row.visible, "Tipo 'hearts' oculta Texto/Imagen/Acciones")

	var bar_id: String = ed.add_element_at("bar", Vector2i(500, 200))
	await _step_frames(2)
	_check(text_row.visible and not img_row.visible and not act_row.visible, "Tipo 'bar' muestra Texto y oculta Imagen/Acciones")

	var cur_id: String = ed.add_element_at("currency", Vector2i(500, 270))
	await _step_frames(2)
	_check(not text_row.visible and img_row.visible and not act_row.visible and font_sz_sl.is_visible_in_tree(), "Tipo 'currency' oculta Texto libre/Acciones y mantiene Fuente/Imagen")

	var txt_id: String = ed.add_element_at("text", Vector2i(180, 280))
	await _step_frames(2)
	_check(text_row.visible and not img_row.visible and not act_row.visible, "Tipo 'text' muestra Texto/Fuente y oculta Imagen/Acciones")

	var btn_id: String = ed.add_element_at("button", Vector2i(360, 280))
	await _step_frames(2)
	_check(text_row.visible and img_row.visible and act_row.visible, "Tipo 'button' muestra Texto, Imagen y Acciones")

	# Controles de Aspecto y Texto
	var style_opt: OptionButton = h.get("prop_style_opt", null) as OptionButton
	var shape_opt: OptionButton = h.get("prop_shape_opt", null) as OptionButton
	var rad_sl: HSlider = h.get("prop_radius_slider", null) as HSlider
	var op_sl: HSlider = h.get("prop_opacity_slider", null) as HSlider
	var fill_btn: ColorPickerButton = h.get("prop_fill_color_btn", null) as ColorPickerButton
	var grad_chk: CheckBox = h.get("prop_grad_chk", null) as CheckBox
	var grad_top: ColorPickerButton = h.get("prop_grad_top_btn", null) as ColorPickerButton
	var grad_bot: ColorPickerButton = h.get("prop_grad_bot_btn", null) as ColorPickerButton
	var bw_sl: HSlider = h.get("prop_border_w_slider", null) as HSlider
	var bcol_btn: ColorPickerButton = h.get("prop_border_col_btn", null) as ColorPickerButton
	var shd_chk: CheckBox = h.get("prop_shadow_chk", null) as CheckBox
	var glw_chk: CheckBox = h.get("prop_glow_chk", null) as CheckBox
	var glw_col: ColorPickerButton = h.get("prop_glow_col_btn", null) as ColorPickerButton
	var clip_chk: CheckBox = h.get("prop_clip_chk", null) as CheckBox
	var show_bg_chk: CheckBox = h.get("prop_show_bg_chk", null) as CheckBox
	var txt_edit: LineEdit = h.get("prop_text_edit", null) as LineEdit
	var font_col: ColorPickerButton = h.get("prop_font_color_btn", null) as ColorPickerButton

	# 2) Probar diffs visuales en lienzo para TODAS las propiedades en FRAME
	ed.select_element("")
	await _step_frames(2)
	var before: Image = db.get_texture().get_image().get_region(stage_r)
	ed.select_element(frm_id)
	await _step_frames(1)
	_select_opt_meta(style_opt, "none")
	ed.select_element("")
	await _step_frames(2)
	var after_style: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(before.compute_image_metrics(after_style, false).get("max", 0.0)) > 5.0, "[frame] window_style_id 'none' altera lienzo")
	ed.select_element(frm_id)
	await _step_frames(1)

	_select_opt_meta(shape_opt, "circle")
	clip_chk.button_pressed = true
	clip_chk.emit_signal("toggled", true)
	fill_btn.color = Color(0.95, 0.15, 0.25, 1.0)
	fill_btn.emit_signal("color_changed", fill_btn.color)
	await _step_frames(2)
	var after_circle: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(after_style.compute_image_metrics(after_circle, false).get("max", 0.0)) > 10.0, "[frame] shape='circle' + fill_color altera lienzo")

	grad_chk.button_pressed = true
	grad_chk.emit_signal("toggled", true)
	grad_top.color = Color(0.1, 0.9, 0.9, 1.0)
	grad_top.emit_signal("color_changed", grad_top.color)
	grad_bot.color = Color(0.1, 0.1, 0.8, 1.0)
	grad_bot.emit_signal("color_changed", grad_bot.color)
	await _step_frames(2)
	var after_grad: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(after_circle.compute_image_metrics(after_grad, false).get("max", 0.0)) > 10.0, "[frame] degradado top/bottom altera lienzo")

	bw_sl.value = 8
	bw_sl.emit_signal("value_changed", 8.0)
	bcol_btn.color = Color(1.0, 1.0, 0.1, 1.0)
	bcol_btn.emit_signal("color_changed", bcol_btn.color)
	await _step_frames(2)
	var after_border: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(after_grad.compute_image_metrics(after_border, false).get("max", 0.0)) > 10.0, "[frame] border_width=8 + border_color altera lienzo")

	shd_chk.button_pressed = true
	shd_chk.emit_signal("toggled", true)
	glw_chk.button_pressed = true
	glw_chk.emit_signal("toggled", true)
	glw_col.color = Color(0.2, 1.0, 0.9, 0.9)
	glw_col.emit_signal("color_changed", glw_col.color)
	await _step_frames(2)
	var after_fx: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(after_border.compute_image_metrics(after_fx, false).get("max", 0.0)) > 5.0, "[frame] shadow + glow altera lienzo")

	op_sl.value = 35
	op_sl.emit_signal("value_changed", 35.0)
	await _step_frames(2)
	var after_op: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(after_fx.compute_image_metrics(after_op, false).get("max", 0.0)) > 10.0, "[frame] opacity=35% altera lienzo")

	# 3) Probar BUTTON (pill + radio + borde + fill + texto + color + tamano + opacidad)
	ed.select_element(btn_id)
	await _step_frames(2)
	var b_before: Image = db.get_texture().get_image().get_region(stage_r)
	_select_opt_meta(shape_opt, "pill")
	rad_sl.value = 18
	rad_sl.emit_signal("value_changed", 18.0)
	fill_btn.color = Color(0.85, 0.3, 0.1, 1.0)
	fill_btn.emit_signal("color_changed", fill_btn.color)
	txt_edit.text = "PULSA"
	txt_edit.emit_signal("text_changed", "PULSA")
	font_sz_sl.value = 24
	font_sz_sl.emit_signal("value_changed", 24.0)
	font_col.color = Color(0.2, 1.0, 0.3, 1.0)
	font_col.emit_signal("color_changed", font_col.color)
	await _step_frames(2)
	var b_after: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(b_before.compute_image_metrics(b_after, false).get("max", 0.0)) > 10.0, "[button] forma pill + relleno + texto + tamano + color altera lienzo")

	# 4) Probar IMAGE (recorte ellipse + opacidad)
	ed.select_element(img_id)
	await _step_frames(2)
	var i_before: Image = db.get_texture().get_image().get_region(stage_r)
	_select_opt_meta(shape_opt, "ellipse")
	clip_chk.button_pressed = true
	clip_chk.emit_signal("toggled", true)
	bw_sl.value = 4
	bw_sl.emit_signal("value_changed", 4.0)
	bcol_btn.color = Color(1.0, 0.2, 0.8, 1.0)
	bcol_btn.emit_signal("color_changed", bcol_btn.color)
	await _step_frames(2)
	var i_after: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(i_before.compute_image_metrics(i_after, false).get("max", 0.0)) > 10.0, "[image] shape='ellipse' con recorte + borde altera lienzo")

	# 5) Probar TEXT (fondo visible + fuente grande + color + borde) y BAR (color relleno)
	ed.select_element(txt_id)
	await _step_frames(2)
	var t_before: Image = db.get_texture().get_image().get_region(stage_r)
	show_bg_chk.button_pressed = true
	show_bg_chk.emit_signal("toggled", true)
	fill_btn.color = Color(0.4, 0.1, 0.7, 1.0)
	fill_btn.emit_signal("color_changed", fill_btn.color)
	txt_edit.text = "TITULO VIVO"
	txt_edit.emit_signal("text_changed", "TITULO VIVO")
	font_sz_sl.value = 26
	font_sz_sl.emit_signal("value_changed", 26.0)
	await _step_frames(2)
	var t_after: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(t_before.compute_image_metrics(t_after, false).get("max", 0.0)) > 10.0, "[text] show_background + fill + texto + font_size altera lienzo")

	ed.select_element(bar_id)
	await _step_frames(2)
	var bar_before: Image = db.get_texture().get_image().get_region(stage_r)
	fill_btn.color = Color(0.1, 0.9, 0.8, 1.0)
	fill_btn.emit_signal("color_changed", fill_btn.color)
	await _step_frames(2)
	var bar_after: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(bar_before.compute_image_metrics(bar_after, false).get("max", 0.0)) > 10.0, "[bar] fill_color altera lienzo")

	db.hide()
	db.queue_free()
	await _step_frames(2)

	# 6) Verificacion real en F5 (ClockGearsGameMenu): render con cambios vs por defecto + propiedades activas
	var saved_custom: Dictionary = dm.get_ui_screen_design("game_menu_main").duplicate(true)
	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	root.size = Vector2i(960, 540)
	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	gm.open_menu_by_id("game_menu")
	await _step_frames(4)
	var f5_def: Image = root.get_texture().get_image()
	gm.close_menu()
	await _step_frames(2)

	dm.set_ui_screen_design("game_menu_main", saved_custom, false)
	gm.open_menu_by_id("game_menu")
	await _step_frames(4)
	var f5_cus: Image = root.get_texture().get_image()
	_check(float(f5_def.compute_image_metrics(f5_cus, false).get("max", 0.0)) > 20.0, "F5 renderiza las formas, degradados, bordes, opacidad y textos personalizados")

	var f5_dl: Node = gm.get_custom_ui_layer()
	var f5_frm: Control = f5_dl.call("get_element_control", frm_id) as Control
	var f5_img: Control = f5_dl.call("get_element_control", img_id) as Control
	_check(f5_frm != null and f5_frm.find_child("ShapedClip", true, false) != null and is_equal_approx(f5_frm.modulate.a, 0.35), "F5 aplica ShapedClip y opacidad=0.35 al frame")
	_check(f5_img != null and f5_img.find_child("ShapedClip", true, false) != null, "F5 aplica ShapedClip (recorte elipse) al elemento image")

	gm.close_menu()
	gm.queue_free()
	await _step_frames(2)

	# Limpieza
	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	var imp_path := ProjectSettings.globalize_path("res://img/ui/test_p6_box.png")
	if FileAccess.file_exists("user://test_p6_box.png"):
		DirAccess.remove_absolute(abs_png)
	if FileAccess.file_exists("res://img/ui/test_p6_box.png"):
		DirAccess.remove_absolute(imp_path)

	_finish(0 if fail_count == 0 else 1)
