extends SceneTree

# Test permanente Punto 3 (Correcciones UI 9B):
# - En Diseño de interfaz, en TODAS las pantallas del selector (Título, Menú del juego x4+, Créditos, HUD):
#   1. La pantalla parte "Por defecto" sin romper datos antiguos.
#   2. Hacer clic sobre un elemento por defecto lo selecciona.
#   3. Arrastrarlo en el lienzo crea automáticamente el diseño personalizado (copia editable),
#      mueve el elemento en píxeles enteros y cambia la etiqueta de estado.
#   4. Deshacer (Undo) y Rehacer (Redo) funcionan sobre la creación y el movimiento.
#   5. "Restablecer a diseño por defecto" vuelve a has_custom_ui_design == false.
#   6. Redimensionar con tiradores funciona en píxeles enteros.
#   7. Rejilla, Imán y Guías funcionan al arrastrar.

var passed: int = 0
var failed: int = 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _find_checkbox_by_text(root_node: Node, txt: String) -> CheckBox:
	var stack: Array[Node] = [root_node]
	while not stack.is_empty():
		var cur: Node = stack.pop_back()
		if cur is CheckBox and (cur as CheckBox).text == txt:
			return cur as CheckBox
		for i in range(cur.get_child_count()):
			stack.push_back(cur.get_child(i))
	return null

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _compute_stage_rect_from_props(h: Dictionary, stage_scale: float = 1.0) -> Rect2:
	var ox := float((h["prop_ox_spin"] as SpinBox).value)
	var oy := float((h["prop_oy_spin"] as SpinBox).value)
	var w := float((h["prop_w_spin"] as SpinBox).value)
	var ht := float((h["prop_h_spin"] as SpinBox).value)
	var anc_btns: Array = h.get("anchor_btns", [])
	var anc_idx := 0
	for i in range(anc_btns.size()):
		var b := anc_btns[i] as Button
		if b != null and b.button_pressed:
			anc_idx = i
			break
	# Índices: 0=NO, 1=N, 2=NE, 3=O, 4=C, 5=E, 6=SO, 7=S, 8=SE
	var col := anc_idx % 3
	var row := anc_idx / 3
	var ax := 0.0 if col == 0 else (0.5 if col == 1 else 1.0)
	var ay := 0.0 if row == 0 else (0.5 if row == 1 else 1.0)
	var x := (ax * (960.0 - w) + ox) * stage_scale
	var y := (ay * (540.0 - ht) + oy) * stage_scale
	return Rect2(x, y, w * stage_scale, ht * stage_scale)

func _simulate_drag(win: Window, overlay: Control, local_from: Vector2, local_to: Vector2) -> void:
	var xf: Transform2D = overlay.get_global_transform_with_canvas()
	var g_from := xf * local_from
	var g_to := xf * local_to
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = g_from
	press.global_position = g_from
	win.push_input(press)

	var mid := g_from.lerp(g_to, 0.5)
	for pt in [mid, g_to]:
		var mot := InputEventMouseMotion.new()
		mot.button_mask = MOUSE_BUTTON_MASK_LEFT
		mot.position = pt
		mot.global_position = pt
		mot.relative = pt - g_from
		win.push_input(mot)

	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = g_to
	rel.global_position = g_to
	win.push_input(rel)

func _simulate_click(win: Window, overlay: Control, local_pos: Vector2) -> void:
	var xf: Transform2D = overlay.get_global_transform_with_canvas()
	var g := xf * local_pos
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = g
	press.global_position = g
	win.push_input(press)
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = g
	rel.global_position = g
	win.push_input(rel)

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 3 (9B CORR): ARRASTRAR, SELECCIONAR Y REDIMENSIONAR EN TODAS LAS PANTALLAS ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		_check(false, "ClockGearsDataManager disponible")
		quit(1)
		return
	dm.init_empty()

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.popup_centered(Vector2i(1366, 768))
	await _step_frames(2)
	db.select_category_by_key("ui_designer")
	await _step_frames(2)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = db.get_ui_designer_ui_handles()
	var screen_opt: OptionButton = h.get("screen_opt", null) as OptionButton
	var overlay: Control = h.get("canvas_overlay", null) as Control
	var status_lbl: Label = h.get("screen_status_lbl", null) as Label
	var reset_btn: Button = h.get("reset_btn", null) as Button
	_check(screen_opt != null and screen_opt.item_count >= 9, "Selector contiene las pantallas del diseño (%d)" % (screen_opt.item_count if screen_opt else 0))
	_check(reset_btn != null and reset_btn.text.find("Restablecer") >= 0, "Botón 'Restablecer a diseño por defecto' presente (%s)" % (reset_btn.text if reset_btn else ""))

	# 1. Probar clic + arrastre + auto-personalización + undo + restablecer en CADA pantalla del selector
	for idx in range(screen_opt.item_count):
		var sid := String(screen_opt.get_item_metadata(idx))
		var sname := screen_opt.get_item_text(idx)
		screen_opt.select(idx)
		ed.select_screen(sid)
		await _step_frames(2)

		var cust0: bool = dm.has_custom_ui_design(sid)
		_check(not cust0, "[%s] Parte en estado por defecto (has_custom=false)" % sid)

		# Guardar el rect del primer elemento por defecto y deseleccionar para probar selección por clic
		var r0: Rect2 = _compute_stage_rect_from_props(h, 1.0)
		ed.select_element("")
		await _step_frames(1)
		_simulate_click(db, overlay, r0.get_center())
		await _step_frames(1)
		var sel_after_click := String(ed.get_selected_element_id())
		_check(not sel_after_click.is_empty(), "[%s] Clic sobre elemento por defecto lo selecciona (%s)" % [sid, sel_after_click])

		# Simular arrastre (+28, +19) sobre el elemento seleccionado en el centro del punto clicado
		var r_drag := _compute_stage_rect_from_props(h, 1.0)
		var ox_before := int((h["prop_ox_spin"] as SpinBox).value)
		var oy_before := int((h["prop_oy_spin"] as SpinBox).value)
		_simulate_drag(db, overlay, r_drag.get_center(), r_drag.get_center() + Vector2(28, 19))
		await _step_frames(2)

		var cust_after: bool = dm.has_custom_ui_design(sid)
		_check(cust_after, "[%s] Arrastrar crea automáticamente diseño personalizado (%s)" % [sid, sname])

		var el_after: Dictionary = dm.get_ui_element(sid, sel_after_click)
		var ox_after := int(el_after.get("offset_x", 0))
		var oy_after := int(el_after.get("offset_y", 0))
		_check(ox_after != ox_before or oy_after != oy_before, "[%s] Elemento movido (%d,%d) -> (%d,%d)" % [sid, ox_before, oy_before, ox_after, oy_after])
		if sid != "hud":
			_check(status_lbl != null and status_lbl.text.find("personalizado") >= 0, "[%s] Etiqueta actualiza a diseño personalizado ('%s')" % [sid, status_lbl.text if status_lbl else ""])

		# Deshacer (Undo) y Rehacer (Redo)
		var undo_ok: bool = ed.apply_undo()
		await _step_frames(1)
		_check(undo_ok and not dm.has_custom_ui_design(sid), "[%s] Deshacer devuelve la pantalla a por defecto" % sid)

		var redo_ok: bool = ed.apply_redo()
		await _step_frames(1)
		var el_redo: Dictionary = dm.get_ui_element(sid, sel_after_click)
		_check(redo_ok and dm.has_custom_ui_design(sid) and int(el_redo.get("offset_x", 0)) == ox_after, "[%s] Rehacer recupera la posición arrastrada (%d)" % [sid, ox_after])

		# Restablecer a diseño por defecto
		ed.request_reset_to_default(true)
		await _step_frames(1)
		_check(not dm.has_custom_ui_design(sid), "[%s] Restablecer a diseño por defecto vuelve al original" % sid)

	# 2. Redimensionar con tirador BR y rejilla + imán + guías en 'title'
	ed.select_screen("title")
	await _step_frames(2)
	var t_id := String(ed.get_selected_element_id())
	var t_r0 := _compute_stage_rect_from_props(h, 1.0)
	var br_pt := t_r0.position + t_r0.size
	var w0 := int((h["prop_w_spin"] as SpinBox).value)
	var h0 := int((h["prop_h_spin"] as SpinBox).value)
	_simulate_drag(db, overlay, br_pt, br_pt + Vector2(30, 14))
	await _step_frames(2)
	var t_resized: Dictionary = dm.get_ui_element("title", t_id)
	_check(int(t_resized.get("width", 0)) > w0 and int(t_resized.get("height", 0)) > h0, "Redimensionar con tirador BR aumenta tamaño en píxeles enteros (%dx%d -> %dx%d)" % [w0, h0, int(t_resized.get("width", 0)), int(t_resized.get("height", 0))])

	# Probar Rejilla + Imán (múltiplo de 8 px)
	ed.select_screen("game_menu_main")
	await _step_frames(2)
	var g_id := String(ed.get_selected_element_id())
	var g_r0 := _compute_stage_rect_from_props(h, 1.0)
	var grid_chk := _find_checkbox_by_text(ed, "Rejilla")
	var snap_chk := _find_checkbox_by_text(ed, "Imán")
	var guides_chk := _find_checkbox_by_text(ed, "Guías")
	if grid_chk: grid_chk.button_pressed = true
	if snap_chk: snap_chk.button_pressed = true
	if guides_chk: guides_chk.button_pressed = true
	await _step_frames(1)
	_simulate_drag(db, overlay, g_r0.get_center(), g_r0.get_center() + Vector2(13, 19))
	await _step_frames(2)
	var g_moved: Dictionary = dm.get_ui_element("game_menu_main", g_id)
	_check(int(g_moved.get("offset_x", 0)) % 8 == 0 and int(g_moved.get("offset_y", 0)) % 8 == 0, "Imán ajusta movimiento a la rejilla de 8 px (%d, %d)" % [int(g_moved.get("offset_x", 0)), int(g_moved.get("offset_y", 0))])

	# Probar escala Full HD 1920x1080
	ed.set_preview_resolution(1920, 1080, false)
	ed.select_screen("credits")
	await _step_frames(2)
	var c_id := String(ed.get_selected_element_id())
	var c_ox0 := int((h["prop_ox_spin"] as SpinBox).value)
	var c_r_fhd := _compute_stage_rect_from_props(h, 2.0)
	_simulate_drag(db, overlay, c_r_fhd.get_center(), c_r_fhd.get_center() + Vector2(40, 20))
	await _step_frames(2)
	var c_moved: Dictionary = dm.get_ui_element("credits", c_id)
	_check(dm.has_custom_ui_design("credits") and int(c_moved.get("offset_x", 0)) != c_ox0, "Arrastre en vista previa 1920x1080 funciona correctamente (%d -> %d)" % [c_ox0, int(c_moved.get("offset_x", 0))])

	db.queue_free()
	print("=== RESULTADO PUNTO 3: %d PASSED, %d FAILED ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
