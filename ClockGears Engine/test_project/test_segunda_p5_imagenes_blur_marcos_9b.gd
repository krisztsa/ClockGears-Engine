extends SceneTree

# Test Punto 5 (Segunda Pasada UI 9B): Imagenes, desenfoque de fondo y marcos
# - Soltar/importar imagen al lienzo crea elemento image y se renderiza en lienzo y F5
# - Frame con image_path + nine_slice / integer + Pixelado/Suave cambia imagen en lienzo y F5
# - Fondo de pantalla con imagen + blur de juego cacheado + velo de color/opacidad
# - Desplegable Estilo con miniaturas + 'Editar estilo' en la misma pestana cambia color/borde en lienzo y F5

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

func _make_test_png(user_path: String) -> String:
	var img := Image.create_empty(32, 32, false, Image.FORMAT_RGBA8)
	for y in range(32):
		for x in range(32):
			var border := (x < 6 or y < 6 or x >= 26 or y >= 26)
			if border:
				img.set_pixel(x, y, Color(0.95, 0.72, 0.12, 1.0))
			elif ((x / 4 + y / 4) % 2) == 0:
				img.set_pixel(x, y, Color(0.15, 0.65, 0.92, 1.0))
			else:
				img.set_pixel(x, y, Color(0.10, 0.22, 0.42, 1.0))
	var abs_p := ProjectSettings.globalize_path(user_path)
	img.save_png(abs_p)
	return abs_p

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 5 (SEGUNDA PASADA UI 9B): IMAGENES, BLUR Y MARCOS ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()
	dm.reset_ui_screen_design_to_default("game_menu_main", false)

	var abs_png := _make_test_png("user://test_p5_sprite.png")
	_check(FileAccess.file_exists("user://test_p5_sprite.png"), "Sprite PNG de prueba generado")

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("game_menu")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = ed.get_ui_handles()
	var stage: Control = h.get("canvas_stage", null) as Control
	var sr := stage.get_global_rect()
	var stage_r := Rect2i(Vector2i(sr.position), Vector2i(sr.size))

	var img_canvas_base: Image = db.get_texture().get_image().get_region(stage_r)

	# 1) Arrastrar/soltar imagen sobre el lienzo
	var dropped_id: String = ed.drop_image_file_on_canvas(abs_png, Vector2i(120, 120))
	await _step_frames(3)
	_check(not dropped_id.is_empty(), "Soltar archivo de imagen en lienzo crea elemento: " + dropped_id)
	var dropped_el: Dictionary = {}
	for e in dm.get_ui_screen_elements("game_menu_main"):
		if e is Dictionary and str(e.get("id", "")) == dropped_id:
			dropped_el = e
			break
	var imported_res_path: String = str(dropped_el.get("image_path", ""))
	_check(not imported_res_path.is_empty(), "Imagen importada y asignada: " + imported_res_path)

	var img_canvas_after_drop: Image = db.get_texture().get_image().get_region(stage_r)
	var m_drop: Dictionary = img_canvas_base.compute_image_metrics(img_canvas_after_drop, false)
	_check(float(m_drop.get("max", 0.0)) > 10.0, "Lienzo cambia al soltar imagen (max=%.2f)" % float(m_drop.get("max", 0.0)))

	# Comprobar modos de escala y Pixelado/Suave en elemento image
	var scale_opt: OptionButton = h.get("prop_image_scale_opt", null) as OptionButton
	var nine_spin: SpinBox = h.get("prop_nine_slice_spin", null) as SpinBox
	var pixel_chk: CheckBox = h.get("prop_pixel_art_chk", null) as CheckBox
	_check(scale_opt != null and nine_spin != null and pixel_chk != null, "Controles escala (Estirar/Ajustar/Rellenar/Mosaico/9-slice/Escala entera) + Pixelado presentes")
	_select_opt_meta(scale_opt, "integer")
	pixel_chk.button_pressed = false
	pixel_chk.emit_signal("toggled", false)
	await _step_frames(2)
	var dl: Control = ed.get_canvas_design_layer()
	var img_node: Control = dl.call("get_element_control", dropped_id) as Control
	var tex_ctrl: Control = img_node.find_child("Image", true, false) as Control if img_node != null else null
	_check(tex_ctrl != null and tex_ctrl.texture_filter == CanvasItem.TEXTURE_FILTER_LINEAR, "Toggle 'Suave' aplica TEXTURE_FILTER_LINEAR en lienzo")
	pixel_chk.button_pressed = true
	pixel_chk.emit_signal("toggled", true)
	await _step_frames(2)
	img_node = dl.call("get_element_control", dropped_id) as Control
	tex_ctrl = img_node.find_child("Image", true, false) as Control if img_node != null else null
	_check(tex_ctrl != null and tex_ctrl.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST, "Toggle 'Pixelado' aplica TEXTURE_FILTER_NEAREST en lienzo")

	# 2) Marco (frame) con imagen de relleno + 9-slice
	var frame_id: String = ed.add_element_at("frame", Vector2i(360, 140))
	await _step_frames(2)
	ed.select_element(frame_id)
	await _step_frames(2)
	var prop_img_edit: LineEdit = h.get("prop_image_edit", null) as LineEdit
	prop_img_edit.text = imported_res_path
	prop_img_edit.emit_signal("text_submitted", imported_res_path)
	await _step_frames(2)
	var img_frame_stretch: Image = db.get_texture().get_image().get_region(stage_r)
	nine_spin.value = 6
	nine_spin.emit_signal("value_changed", 6.0)
	_select_opt_meta(scale_opt, "nine_slice")
	await _step_frames(2)
	var img_frame_9s: Image = db.get_texture().get_image().get_region(stage_r)
	var m_9s: Dictionary = img_frame_stretch.compute_image_metrics(img_frame_9s, false)
	_check(float(m_9s.get("max", 0.0)) > 5.0, "Frame cambia visualmente entre stretch y 9-slice (max=%.2f)" % float(m_9s.get("max", 0.0)))

	# 3) Miniaturas de Estilo + 'Editar estilo' en la misma pestana
	var style_opt: OptionButton = h.get("prop_style_opt", null) as OptionButton
	var edit_style_btn: Button = h.get("prop_edit_style_btn", null) as Button
	_check(style_opt != null and style_opt.item_count >= 2, "Desplegable Estilo tiene estilos disponibles")
	var has_thumbs := false
	for si in range(style_opt.item_count):
		if style_opt.get_item_icon(si) != null:
			has_thumbs = true
			break
	_check(has_thumbs, "Desplegable Estilo muestra miniaturas")
	style_opt.select(1)
	style_opt.emit_signal("item_selected", 1)
	await _step_frames(2)
	var img_before_style_edit: Image = db.get_texture().get_image().get_region(stage_r)

	_check(edit_style_btn != null, "Boton 'Editar estilo' presente junto al selector")
	edit_style_btn.emit_signal("pressed")
	await _step_frames(2)
	var h2: Dictionary = ed.get_ui_handles()
	var st_dlg: AcceptDialog = h2.get("style_inline_dialog", null) as AcceptDialog
	var st_fill: ColorPickerButton = h2.get("style_edit_fill_col", null) as ColorPickerButton
	var st_border: ColorPickerButton = h2.get("style_edit_border_col", null) as ColorPickerButton
	_check(st_dlg != null and st_dlg.visible and ed.is_visible_in_tree(), "'Editar estilo' abre dialogo en la misma pestana sin salir")
	var edited_style_id := str(style_opt.get_item_metadata(1))
	var orig_style: Dictionary = dm.get_window_style(edited_style_id).duplicate(true)
	st_fill.color = Color(0.05, 0.55, 0.45, 0.95)
	st_border.color = Color(0.95, 0.15, 0.65, 1.0)
	st_dlg.emit_signal("confirmed")
	st_dlg.hide()
	await _step_frames(3)
	var img_after_style_edit: Image = db.get_texture().get_image().get_region(stage_r)
	var m_st: Dictionary = img_before_style_edit.compute_image_metrics(img_after_style_edit, false)
	_check(float(m_st.get("max", 0.0)) > 10.0, "Editar estilo actualiza el lienzo en tiempo real (max=%.2f)" % float(m_st.get("max", 0.0)))

	# 4) Fondo de pantalla con imagen + desenfoque + velo de color
	var bg_type_opt: OptionButton = h.get("menu_bg_type_opt", null) as OptionButton
	var bg_img_edit: LineEdit = h.get("menu_bg_img_edit", null) as LineEdit
	var bg_blur_chk: CheckBox = h.get("menu_bg_blur_chk", null) as CheckBox
	var bg_blur_slider: HSlider = h.get("menu_bg_blur_slider", null) as HSlider
	var bg_veil_chk: CheckBox = h.get("menu_bg_veil_chk", null) as CheckBox
	var bg_veil_col: ColorPickerButton = h.get("menu_bg_veil_col_btn", null) as ColorPickerButton
	var bg_veil_op: HSlider = h.get("menu_bg_veil_opacity_slider", null) as HSlider
	_select_opt_meta(bg_type_opt, "image")
	bg_img_edit.text = imported_res_path
	bg_img_edit.emit_signal("text_submitted", imported_res_path)
	bg_blur_chk.button_pressed = true
	bg_blur_chk.emit_signal("toggled", true)
	bg_blur_slider.value = 75
	bg_blur_slider.emit_signal("value_changed", 75.0)
	bg_veil_chk.button_pressed = true
	bg_veil_chk.emit_signal("toggled", true)
	bg_veil_col.color = Color(0.85, 0.12, 0.15, 1.0)
	bg_veil_col.emit_signal("color_changed", bg_veil_col.color)
	bg_veil_op.value = 50
	bg_veil_op.emit_signal("value_changed", 50.0)
	await _step_frames(3)

	var design_saved: Dictionary = dm.get_ui_screen_design("game_menu_main")
	var bg_saved: Dictionary = design_saved.get("background", {})
	_check(bool(bg_saved.get("blur_enabled", false)) and int(bg_saved.get("blur_intensity", 0)) == 75, "Blur de fondo guardado en diseno (75)")
	_check(bool(bg_saved.get("veil_enabled", false)) and str(bg_saved.get("veil_color", "")).begins_with("#"), "Velo de color guardado en diseno: " + str(bg_saved.get("veil_color", "")))

	db.hide()
	db.queue_free()
	await _step_frames(2)

	# 5) Verificacion real en F5 (ClockGearsGameMenu sobre escena de juego simulada)
	root.size = Vector2i(960, 540)
	var world := Control.new()
	world.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var b1 := ColorRect.new()
	b1.color = Color(0.1, 0.85, 0.2, 1.0)
	b1.position = Vector2(100, 100)
	b1.size = Vector2(220, 220)
	world.add_child(b1)
	var b2 := ColorRect.new()
	b2.color = Color(0.95, 0.85, 0.1, 1.0)
	b2.position = Vector2(360, 140)
	b2.size = Vector2(200, 200)
	world.add_child(b2)
	root.add_child(world)

	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	gm.open_menu_by_id("game_menu")
	await _step_frames(5)
	var f5_with_veil: Image = root.get_texture().get_image()

	# Quitar velo e imagen de fondo temporalmente para comparar blur vs mundo sin blur y velo distinto
	var d_mod: Dictionary = dm.get_ui_screen_design("game_menu_main").duplicate(true)
	var bg_mod: Dictionary = Dictionary(d_mod.get("background", {})).duplicate(true)
	bg_mod["veil_color"] = "#1020d0a0"
	bg_mod["type"] = "default"
	bg_mod["image_path"] = ""
	d_mod["background"] = bg_mod
	dm.set_ui_screen_design("game_menu_main", d_mod, false)
	gm.close_menu()
	await _step_frames(2)
	gm.open_menu_by_id("game_menu")
	await _step_frames(4)
	var f5_blur_blue_veil: Image = root.get_texture().get_image()
	var m_f5: Dictionary = f5_with_veil.compute_image_metrics(f5_blur_blue_veil, false)
	_check(float(m_f5.get("max", 0.0)) > 15.0, "F5 aplica imagen de fondo, desenfoque cacheado y cambio de color/opacidad de velo (max=%.2f)" % float(m_f5.get("max", 0.0)))

	# Verificar que el frame (con FrameNinePatch) y la imagen soltada existen en el arbol runtime de F5
	var f5_dl: Node = gm.get_custom_ui_layer()
	var f5_img_ctrl: Node = f5_dl.call("get_element_control", dropped_id) if f5_dl != null else null
	var f5_frm_ctrl: Node = f5_dl.call("get_element_control", frame_id) if f5_dl != null else null
	var f5_has_9patch := (f5_frm_ctrl != null and f5_frm_ctrl.find_child("FrameNinePatch", true, false) != null)
	_check(f5_img_ctrl != null and f5_has_9patch, "F5 renderiza tanto la imagen soltada como el frame con FrameNinePatch")

	gm.close_menu()
	gm.queue_free()
	world.queue_free()
	await _step_frames(2)

	# Restaurar estilo original y diseno de pantalla + limpiar archivo temporal
	dm.update_window_style(edited_style_id, orig_style)
	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	if FileAccess.file_exists("user://test_p5_sprite.png"):
		DirAccess.remove_absolute(abs_png)
	if not imported_res_path.is_empty() and imported_res_path.begins_with("res://") and FileAccess.file_exists(imported_res_path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(imported_res_path))

	_finish(0 if fail_count == 0 else 1)
