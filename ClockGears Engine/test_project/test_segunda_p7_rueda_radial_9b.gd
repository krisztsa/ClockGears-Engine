extends SceneTree

# Test Punto 7 (Segunda Pasada UI 9B): Rueda radial completa en Editor y F5
# - '+ Rueda radial' crea elemento centrado de 6 ranuras con inspector completo
# - Cambiar a 8 ranuras (slider), arco/CW/radio/tamano ranura/info centro/anim apertura
# - Editar ranura (nombre, descripcion, icono, acciones ordenadas) y asas en lienzo
# - Ejecutar rueda de 8 ranuras en F5: teclado, vector/mando, mouse-angulo, escala activa y ejecucion

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

func _find_opt_meta(opt: OptionButton, meta: String) -> int:
	for i in range(opt.item_count):
		if str(opt.get_item_metadata(i)) == meta:
			return i
	return -1

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 7 (SEGUNDA PASADA UI 9B): RUEDA RADIAL 8 RANURAS ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()
	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	dm.set_switch("sw_radial_ok", false)

	var ic_img := Image.create_empty(16, 16, false, Image.FORMAT_RGBA8)
	ic_img.fill(Color(0.2, 0.9, 0.8, 1.0))
	var abs_ic := ProjectSettings.globalize_path("user://test_p7_slot_icon.png")
	ic_img.save_png(abs_ic)

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("game_menu")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = ed.get_ui_handles()
	var pal: Dictionary = h.get("palette_btns", {})
	var rad_pal_btn: Button = pal.get("radial", null) as Button
	_check(rad_pal_btn != null and rad_pal_btn.text == "+ Rueda radial", "Paleta tiene '+ Rueda radial'")

	# 1) Pulsar '+ Rueda radial' crea rueda centrada de 6 ranuras
	rad_pal_btn.emit_signal("pressed")
	await _step_frames(3)
	var rad_id: String = ed.get_selected_element_id()
	var rad_el: Dictionary = {}
	for e in dm.get_ui_screen_elements("game_menu_main"):
		if e is Dictionary and str(e.get("id", "")) == rad_id:
			rad_el = e
			break
	_check(str(rad_el.get("type", "")) == "radial", "'+ Rueda radial' crea y selecciona elemento radial: " + rad_id)
	var rad_cx := int(rad_el.get("offset_x", 0)) + int(rad_el.get("width", 0)) / 2
	var rad_cy := int(rad_el.get("offset_y", 0)) + int(rad_el.get("height", 0)) / 2
	_check(abs(rad_cx - 480) <= 4 and abs(rad_cy - 270) <= 4, "Rueda radial nace centrada (%d, %d)" % [rad_cx, rad_cy])

	var dl: Control = ed.get_canvas_design_layer()
	var rc_preview: Control = dl.call("get_radial_canvas") as Control
	_check(rc_preview != null and int(rc_preview.call("get_slot_count")) == 6, "Rueda radial inicial tiene 6 ranuras activas en lienzo")

	var stage: Control = h.get("canvas_stage", null) as Control
	var sr := stage.get_global_rect()
	var stage_r := Rect2i(Vector2i(sr.position), Vector2i(sr.size))
	var img_6slots: Image = db.get_texture().get_image().get_region(stage_r)

	# 2) Ajustar a 8 ranuras con slider + editar geometria y animacion
	var cnt_sl: HSlider = h.get("prop_rad_count_slider", null) as HSlider
	var rad_sl: HSlider = h.get("prop_rad_radius_slider", null) as HSlider
	var full_chk: CheckBox = h.get("prop_rad_full_chk", null) as CheckBox
	var sweep_sl: HSlider = h.get("prop_rad_sweep_slider", null) as HSlider
	var cw_chk: CheckBox = h.get("prop_rad_cw_chk", null) as CheckBox
	var ssz_sl: HSlider = h.get("prop_rad_slot_size_slider", null) as HSlider
	var ctr_chk: CheckBox = h.get("prop_rad_center_info_chk", null) as CheckBox
	var oanim_opt: OptionButton = h.get("prop_rad_open_anim_opt", null) as OptionButton
	_check(cnt_sl != null and rad_sl != null and full_chk != null and sweep_sl != null and cw_chk != null and ssz_sl != null and ctr_chk != null and oanim_opt != null, "Inspector radial completo presente")

	cnt_sl.value = 8
	cnt_sl.emit_signal("value_changed", 8.0)
	await _step_frames(2)
	full_chk.button_pressed = false
	full_chk.emit_signal("toggled", false)
	sweep_sl.value = 280
	sweep_sl.emit_signal("value_changed", 280.0)
	cw_chk.button_pressed = false
	cw_chk.emit_signal("toggled", false)
	ssz_sl.value = 46
	ssz_sl.emit_signal("value_changed", 46.0)
	ed.set_radial_layout_live(134, -45.0)
	await _step_frames(3)

	rc_preview = dl.call("get_radial_canvas") as Control
	_check(rc_preview != null and int(rc_preview.call("get_slot_count")) == 8, "Slider actualiza la rueda a 8 ranuras en lienzo")
	var img_8slots: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(img_6slots.compute_image_metrics(img_8slots, false).get("max", 0.0)) > 10.0, "Cambiar a 8 ranuras + arco/sentido altera el lienzo")

	# Restaurar 360 completo y sentido horario para probar F5 comodo
	full_chk.button_pressed = true
	full_chk.emit_signal("toggled", true)
	cw_chk.button_pressed = true
	cw_chk.emit_signal("toggled", true)
	await _step_frames(2)

	# 3) Configurar ranura 3 (indice 2): nombre, descripcion, icono y acciones ordenadas
	var slot_opt: OptionButton = h.get("prop_rad_slot_opt", null) as OptionButton
	var sname: LineEdit = h.get("prop_rad_slot_name_edit", null) as LineEdit
	var sdesc: LineEdit = h.get("prop_rad_slot_desc_edit", null) as LineEdit
	var sicon: LineEdit = h.get("prop_rad_slot_icon_edit", null) as LineEdit
	var add_act_opt: OptionButton = h.get("prop_rad_slot_add_act_opt", null) as OptionButton
	var add_act_btn: Button = h.get("prop_rad_slot_add_act_btn", null) as Button
	var acts_vbox: VBoxContainer = h.get("prop_rad_slot_actions_vbox", null) as VBoxContainer

	slot_opt.select(2)
	slot_opt.emit_signal("item_selected", 2)
	await _step_frames(2)
	sname.text = "Pocion Estelar"
	sname.emit_signal("text_changed", "Pocion Estelar")
	sdesc.text = "Activa estado especial"
	sdesc.emit_signal("text_changed", "Activa estado especial")
	sicon.text = abs_ic
	sicon.emit_signal("text_changed", abs_ic)
	await _step_frames(2)

	# Anadir acciones ordenadas: set_switch + close
	# Primero limpiar accion por defecto si existe
	while acts_vbox.get_child_count() > 0:
		var removed_one := false
		for sub in acts_vbox.get_child(0).get_children():
			for gc in sub.get_children():
				if gc is Button and gc.text == "x":
					gc.emit_signal("pressed")
					await _step_frames(2)
					removed_one = true
					break
			if removed_one:
				break
		if not removed_one:
			break
	var sw_i := _find_opt_meta(add_act_opt, "set_switch")
	add_act_opt.select(sw_i)
	add_act_btn.emit_signal("pressed")
	await _step_frames(2)
	for card in acts_vbox.get_children():
		for sub in card.get_children():
			for gc in sub.get_children():
				if gc is LineEdit:
					gc.text = "sw_radial_ok"
					gc.emit_signal("text_changed", "sw_radial_ok")
				elif gc is CheckBox:
					gc.button_pressed = true
					gc.emit_signal("toggled", true)
	var cl_i := _find_opt_meta(add_act_opt, "close")
	add_act_opt.select(cl_i)
	add_act_btn.emit_signal("pressed")
	await _step_frames(2)
	_check(acts_vbox.get_child_count() >= 2, "Ranura 3 tiene lista ordenada de acciones (%d pasos)" % acts_vbox.get_child_count())

	db.hide()
	db.queue_free()
	await _step_frames(2)

	# 4) Verificacion real en F5 (ClockGearsGameMenu) con teclado, vector/raton, escala y ejecucion
	root.size = Vector2i(960, 540)
	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	gm.open_menu_by_id("game_menu")
	await _step_frames(5)

	var f5_dl: Node = gm.get_custom_ui_layer()
	var f5_rad: Control = f5_dl.call("get_radial_canvas") as Control
	_check(f5_rad != null and int(f5_rad.call("get_slot_count")) == 8, "F5 carga rueda radial de 8 ranuras")

	# Navegar con teclado RIGHT x2 -> indice 2 ("Pocion Estelar")
	for _k in range(2):
		var ev := InputEventKey.new()
		ev.keycode = KEY_RIGHT
		ev.pressed = true
		root.push_input(ev)
		await _step_frames(1)
		var ev_up := InputEventKey.new()
		ev_up.keycode = KEY_RIGHT
		ev_up.pressed = false
		root.push_input(ev_up)
		await _step_frames(2)
	await _step_frames(20)

	var sel_idx := int(f5_rad.call("get_selected_index"))
	var ctr_info: Dictionary = f5_rad.call("get_center_displayed_info")
	_check(sel_idx == 2, "Teclado navega a la ranura 3 (idx=%d)" % sel_idx)
	_check(str(ctr_info.get("title", "")) == "Pocion Estelar" and str(ctr_info.get("description", "")) == "Activa estado especial", "Centro muestra nombre y descripcion de la ranura activa")

	var sc_sel := float(f5_rad.call("get_slot_visual_scale", 2))
	var sc_oth := float(f5_rad.call("get_slot_visual_scale", 0))
	_check(sc_sel > sc_oth + 0.05, "Ranura activa escala visualmente al seleccionarse (sel=%.2f vs other=%.2f)" % [sc_sel, sc_oth])

	# Probar seleccion por angulo (raton / stick) hacia la ranura 0 y luego volver a ranura 2 por punto local
	var p0: Vector2 = f5_rad.call("get_slot_local_pos", 0)
	f5_rad.call("select_by_local_point", p0)
	await _step_frames(2)
	_check(int(f5_rad.call("get_selected_index")) == 0, "Seleccion por angulo/raton selecciona ranura 0")
	var p2: Vector2 = f5_rad.call("get_slot_local_pos", 2)
	f5_rad.call("select_by_local_point", p2)
	await _step_frames(2)
	_check(int(f5_rad.call("get_selected_index")) == 2, "Seleccion por angulo/raton vuelve a ranura 2")

	# Activar con ENTER -> ejecuta set_switch("sw_radial_ok", true) y close
	var enter_ev := InputEventKey.new()
	enter_ev.keycode = KEY_ENTER
	enter_ev.pressed = true
	root.push_input(enter_ev)
	await _step_frames(4)
	_check(dm.get_switch("sw_radial_ok") == true, "Activar ranura radial en F5 ejecuta 'set_switch'")
	_check(not gm.is_menu_open(), "Segundo paso 'close' en ranura radial cierra el menu en F5")

	gm.queue_free()
	await _step_frames(2)

	dm.set_switch("sw_radial_ok", false)
	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	if FileAccess.file_exists("user://test_p7_slot_icon.png"):
		DirAccess.remove_absolute(abs_ic)

	_finish(0 if fail_count == 0 else 1)
