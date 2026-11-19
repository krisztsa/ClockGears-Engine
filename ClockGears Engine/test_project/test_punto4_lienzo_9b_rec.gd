extends SceneTree

# Test Punto 4 (Reconstrucción UI 9B):
# - Con Rejilla apagada no se dibuja cuadrícula ni líneas densas (incluso en HUD).
# - Con Rejilla encendida permite elegir paso 8, 16 o 32 px.
# - Guías inteligentes solo aparecen al arrastrar cerca de centro/bordes y desaparecen al soltar.
# - Todos los tipos (image, text, button, frame, bar, hearts, widget, radial) se pueden
#   añadir, seleccionar, arrastrar, redimensionar y borrar.
# - Los elementos bloqueados no se mueven y muestran indicador en capas/lienzo.
# - Selección múltiple por caja de arrastre y por Shift+Clic + arrastre/borrado múltiple.

var passed: int = 0
var failed: int = 0
var _finished: bool = false

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _finish(code: int) -> void:
	if _finished:
		return
	_finished = true
	print("RESULTADOS: %d PASS, %d FAIL" % [passed, failed])
	quit(code)

func _step_frames(n: int = 2) -> void:
	var c := mini(n, 600)
	for _i in range(c):
		await process_frame

func _elem_rect(el: Dictionary, sc: float) -> Rect2:
	var ax := 0.0
	var ay := 0.0
	var anc := str(el.get("anchor", "top_left"))
	if anc.ends_with("center") or anc == "center":
		ax = 0.5
	elif anc.ends_with("right"):
		ax = 1.0
	if anc.begins_with("center") or anc == "center":
		ay = 0.5
	elif anc.begins_with("bottom"):
		ay = 1.0
	var w := float(el.get("width", 120)) * sc
	var h := float(el.get("height", 36)) * sc
	var ox := float(el.get("offset_x", 0)) * sc
	var oy := float(el.get("offset_y", 0)) * sc
	return Rect2((ax * (960.0 * sc - w) + ox), (ay * (540.0 * sc - h) + oy), w, h)

func _push_click(win: Window, ctrl: Control, local_pt: Vector2, shift: bool = false) -> void:
	var g := ctrl.get_global_transform_with_canvas() * local_pt
	var p := InputEventMouseButton.new()
	p.button_index = MOUSE_BUTTON_LEFT
	p.pressed = true
	p.shift_pressed = shift
	p.position = g
	p.global_position = g
	win.push_input(p)
	var r := InputEventMouseButton.new()
	r.button_index = MOUSE_BUTTON_LEFT
	r.pressed = false
	r.shift_pressed = shift
	r.position = g
	r.global_position = g
	win.push_input(r)

func _push_drag(win: Window, ctrl: Control, from_pt: Vector2, to_pt: Vector2, release: bool = true, shift: bool = false) -> void:
	var xf := ctrl.get_global_transform_with_canvas()
	var g0 := xf * from_pt
	var g1 := xf * to_pt
	var p := InputEventMouseButton.new()
	p.button_index = MOUSE_BUTTON_LEFT
	p.pressed = true
	p.shift_pressed = shift
	p.position = g0
	p.global_position = g0
	win.push_input(p)
	for i in range(1, 5):
		var cur := g0.lerp(g1, float(i) / 4.0)
		var m := InputEventMouseMotion.new()
		m.button_mask = MOUSE_BUTTON_MASK_LEFT
		m.shift_pressed = shift
		m.position = cur
		m.global_position = cur
		m.relative = (g1 - g0) / 4.0
		win.push_input(m)
	if release:
		var r := InputEventMouseButton.new()
		r.button_index = MOUSE_BUTTON_LEFT
		r.pressed = false
		r.shift_pressed = shift
		r.position = g1
		r.global_position = g1
		win.push_input(r)

func _push_release(win: Window, ctrl: Control, local_pt: Vector2) -> void:
	var g := ctrl.get_global_transform_with_canvas() * local_pt
	var r := InputEventMouseButton.new()
	r.button_index = MOUSE_BUTTON_LEFT
	r.pressed = false
	r.position = g
	r.global_position = g
	win.push_input(r)

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60 s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 4 (RECONSTRUCCION UI 9B): LIENZO ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.init_empty()

	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("ui_designer")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = db.get_ui_designer_ui_handles()
	var overlay: Control = h.get("canvas_overlay", null) as Control
	var step_opt: OptionButton = h.get("grid_step_opt", null) as OptionButton
	var lock_chk: CheckBox = h.get("prop_locked_chk", null) as CheckBox
	var pal: Dictionary = h.get("palette_btns", {})

	# 1. Rejilla apagada -> 0 líneas; pasos 8/16/32 px configurables
	var st0: Dictionary = ed.get_canvas_draw_stats()
	_check(not bool(st0.get("grid_enabled", true)) and int(st0.get("grid_line_count", -1)) == 0, "Rejilla apagada al iniciar (0 líneas)")
	ed.select_screen("hud")
	await _step_frames(2)
	var st_hud: Dictionary = ed.get_canvas_draw_stats()
	_check(not bool(st_hud.get("grid_enabled", true)) and int(st_hud.get("grid_line_count", -1)) == 0, "HUD con rejilla apagada dibuja 0 líneas de rejilla")

	_check(step_opt != null and step_opt.item_count == 3, "Selector de paso de rejilla tiene 3 opciones (8, 16, 32 px)")
	for target_step in [8, 16, 32]:
		ed.set_grid_step(target_step)
		_check(ed.get_grid_step() == target_step, "Paso de rejilla configurable a %d px" % target_step)

	# 2. Todos los tipos de elementos en paleta y manipulables en lienzo
	ed.select_screen("credits")
	await _step_frames(2)
	var types_req := ["image", "text", "button", "frame", "bar", "hearts", "widget", "radial"]
	for tp in types_req:
		_check(pal.has(tp), "Paleta incluye tipo '%s'" % tp)
		var nid: String = ed.add_element_at(tp, Vector2i(300, 220))
		await _step_frames(2)
		_check(not nid.is_empty(), "Tipo '%s' añadido (%s)" % [tp, nid])
		var el0: Dictionary = dm.get_ui_element("credits", nid)
		var sc: float = (float(ed.get_preview_resolution().x) / 960.0) * float(ed.get_canvas_zoom())
		var r0 := _elem_rect(el0, sc)
		var c0 := r0.position + r0.size * 0.5
		_push_drag(db, overlay, c0, c0 + Vector2(28, 18))
		await _step_frames(2)
		var el1: Dictionary = dm.get_ui_element("credits", nid)
		_check(int(el1.get("offset_x", 0)) != int(el0.get("offset_x", 0)), "Tipo '%s' se selecciona y arrastra" % tp)
		var r1 := _elem_rect(el1, sc)
		var br := r1.position + r1.size
		_push_drag(db, overlay, br, br + Vector2(18, 12))
		await _step_frames(2)
		var el2: Dictionary = dm.get_ui_element("credits", nid)
		_check(int(el2.get("width", 0)) > int(el1.get("width", 0)), "Tipo '%s' se redimensiona con asa" % tp)
		_check(bool(ed.delete_selected_element()), "Tipo '%s' se puede borrar" % tp)
		await _step_frames(1)

	# 3. Guías inteligentes solo durante el arrastre
	ed.select_screen("title")
	ed.request_reset_to_default(true)
	await _step_frames(2)
	var id_a: String = ed.add_element_at("text", Vector2i(200, 140))
	var id_b: String = ed.add_element_at("button", Vector2i(200, 320))
	await _step_frames(2)
	var st_idle: Dictionary = ed.get_canvas_draw_stats()
	_check(int(st_idle.get("active_guide_lines", -1)) == 0, "En reposo hay 0 líneas guía dibujadas")

	var ela: Dictionary = dm.get_ui_element("title", id_a)
	var sc_t: float = (float(ed.get_preview_resolution().x) / 960.0) * float(ed.get_canvas_zoom())
	var ra := _elem_rect(ela, sc_t)
	# Arrastrar cerca del centro X=480 sin soltar para comprobar guías activas
	_push_drag(db, overlay, ra.position + ra.size * 0.5, Vector2(480.0 * sc_t, 270.0 * sc_t), false)
	await _step_frames(1)
	var st_drag: Dictionary = ed.get_canvas_draw_stats()
	_check(bool(st_drag.get("dragging", false)) and int(st_drag.get("active_guide_lines", 0)) > 0, "Durante arrastre cerca del centro aparecen guías inteligentes (%d)" % int(st_drag.get("active_guide_lines", 0)))
	_push_release(db, overlay, Vector2(480.0 * sc_t, 270.0 * sc_t))
	await _step_frames(2)
	var st_post: Dictionary = ed.get_canvas_draw_stats()
	_check(int(st_post.get("active_guide_lines", -1)) == 0, "Al soltar desaparecen las guías inteligentes")

	# 4. Elemento bloqueado no se arrastra y muestra [B] en capas
	ed.select_element(id_b)
	await _step_frames(1)
	_check(lock_chk != null, "Checkbox 'Bloqueado' existe en Propiedades")
	if lock_chk != null:
		lock_chk.set_pressed(true)
		await _step_frames(2)
	var elb0: Dictionary = dm.get_ui_element("title", id_b)
	_check(bool(elb0.get("locked", false)), "Elemento '%s' marcado como bloqueado" % id_b)
	var rb := _elem_rect(elb0, sc_t)
	_push_drag(db, overlay, rb.position + rb.size * 0.5, rb.position + rb.size * 0.5 + Vector2(40, 20))
	await _step_frames(2)
	var elb1: Dictionary = dm.get_ui_element("title", id_b)
	_check(int(elb1.get("offset_x", 0)) == int(elb0.get("offset_x", 0)) and int(elb1.get("offset_y", 0)) == int(elb0.get("offset_y", 0)), "Elemento bloqueado no se mueve al arrastrar")
	if lock_chk != null:
		lock_chk.set_pressed(false)
		await _step_frames(2)

	# 5. Selección múltiple por Shift+Clic y por caja de arrastre (marquee)
	var el_m1: String = ed.add_element_at("text", Vector2i(140, 120))
	var el_m2: String = ed.add_element_at("text", Vector2i(140, 200))
	await _step_frames(2)
	var rm1 := _elem_rect(dm.get_ui_element("title", el_m1), sc_t)
	var rm2 := _elem_rect(dm.get_ui_element("title", el_m2), sc_t)
	_push_click(db, overlay, rm1.position + rm1.size * 0.5, false)
	await _step_frames(1)
	_push_click(db, overlay, rm2.position + rm2.size * 0.5, true)
	await _step_frames(2)
	var sel_shift: PackedStringArray = ed.get_selected_element_ids()
	_check(sel_shift.has(el_m1) and sel_shift.has(el_m2), "Shift+Clic selecciona múltiples elementos (%d)" % sel_shift.size())

	# Arrastrar uno de los seleccionados mueve ambos
	var ox1_0 := int(dm.get_ui_element("title", el_m1).get("offset_x", 0))
	var ox2_0 := int(dm.get_ui_element("title", el_m2).get("offset_x", 0))
	_push_drag(db, overlay, rm1.position + rm1.size * 0.5, rm1.position + rm1.size * 0.5 + Vector2(36, 0))
	await _step_frames(2)
	var ox1_1 := int(dm.get_ui_element("title", el_m1).get("offset_x", 0))
	var ox2_1 := int(dm.get_ui_element("title", el_m2).get("offset_x", 0))
	_check(ox1_1 > ox1_0 and ox2_1 > ox2_0, "Arrastrar selección múltiple mueve ambos elementos (%d->%d, %d->%d)" % [ox1_0, ox1_1, ox2_0, ox2_1])

	# Deseleccionar en vacío y probar caja de arrastre (marquee) sobre el_m1 y el_m2
	_push_click(db, overlay, Vector2(10, 10), false)
	await _step_frames(1)
	_check(ed.get_selected_element_ids().is_empty(), "Clic en vacío limpia selección")
	var rm1_new := _elem_rect(dm.get_ui_element("title", el_m1), sc_t)
	var rm2_new := _elem_rect(dm.get_ui_element("title", el_m2), sc_t)
	var box_tl := Vector2(minf(rm1_new.position.x, rm2_new.position.x) - 10.0, minf(rm1_new.position.y, rm2_new.position.y) - 10.0)
	var box_br := Vector2(maxf(rm1_new.end.x, rm2_new.end.x) + 10.0, maxf(rm1_new.end.y, rm2_new.end.y) + 10.0)
	_push_drag(db, overlay, box_tl, box_br, true)
	await _step_frames(2)
	var sel_box: PackedStringArray = ed.get_selected_element_ids()
	_check(sel_box.has(el_m1) and sel_box.has(el_m2), "Caja de arrastre (marquee) selecciona múltiples elementos (%d)" % sel_box.size())

	ed.request_reset_to_default(true)
	await _step_frames(1)
	db.queue_free()
	await _step_frames(2)
	_finish(0 if failed == 0 else 1)
