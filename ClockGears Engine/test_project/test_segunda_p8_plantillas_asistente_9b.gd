extends SceneTree

# Test Punto 8 (Segunda Pasada UI 9B): Biblioteca de plantillas y asistente '+ Nuevo diseno'
# - Las 8 plantillas reales + priorizadas por sistema/pestana
# - 'Plantillas': confirmacion, aplica al lienzo, visual en editor y Deshacer
# - '+ Nuevo diseno': flujo real 3 pasos (plantilla, nombre+botones, triggers/pausa), crea menu y abre en F5

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

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 8 (SEGUNDA PASADA UI 9B): PLANTILLAS Y ASISTENTE ===")
	root.size = Vector2i(1366, 768)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()
	dm.reset_ui_screen_design_to_default("game_menu_main", false)

	# 1) Verificar las 8 plantillas clave y priorizacion por sistema
	var tpls: Array = dm.get_ui_templates_for_system("game_menu_main")
	var tids: Array[String] = []
	for t in tpls:
		if t is Dictionary:
			tids.append(str(t.get("id", "")))
	for req in ["classic_jrpg", "radial_ring", "quick_item_wheel", "arpg_inventory_grid", "fullscreen_tabs", "sliding_sidebar", "minimalist", "cards_carousel"]:
		_check(tids.has(req), "Plantilla disponible: " + req)

	var title_tpls: Array = dm.get_ui_templates_for_system("title")
	_check(title_tpls.size() > 0 and str(Dictionary(title_tpls[0]).get("category", "")) == "title", "Pestana Titulo prioriza plantillas de titulo")
	var grid_tpls: Array = dm.get_ui_templates_for_system("game_menu_items")
	_check(grid_tpls.size() > 0 and str(Dictionary(grid_tpls[0]).get("category", "")) == "grid", "Pestana Objetos prioriza cuadricula de inventario")

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	db.select_category_by_key("game_menu")
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = ed.get_ui_handles()
	var tpl_btn: Button = h.get("templates_btn", null) as Button
	var wiz_btn: Button = h.get("new_menu_wizard_btn", null) as Button
	var tpl_dlg: AcceptDialog = h.get("template_gallery_dialog", null) as AcceptDialog
	var tpl_grid: GridContainer = h.get("tpl_screen_grid", null) as GridContainer
	var tpl_apply: Button = h.get("tpl_apply_btn", null) as Button
	var tpl_conf: ConfirmationDialog = h.get("template_apply_confirm_dialog", null) as ConfirmationDialog
	var stage: Control = h.get("canvas_stage", null) as Control
	var sr := stage.get_global_rect()
	var stage_r := Rect2i(Vector2i(sr.position), Vector2i(sr.size))
	var img_before: Image = db.get_texture().get_image().get_region(stage_r)

	# 2) Abrir Plantillas desde boton UI -> seleccionar tarjeta distinta -> confirmar -> Deshacer
	_check(tpl_btn != null and tpl_dlg != null, "Boton y dialogo 'Plantillas' presentes")
	tpl_btn.emit_signal("pressed")
	await _step_frames(2)
	_check(tpl_dlg.visible and tpl_grid.get_child_count() >= 8, "Galeria de plantillas abre con las 8+ tarjetas")

	# Pulsar tarjeta 'radial_ring' (Anillo radial)
	var picked := false
	for card in tpl_grid.get_children():
		if card.get_child_count() > 0 and card.get_child(0).get_child_count() > 1:
			var b := card.get_child(0).get_child(1) as Button
			if b != null and str(b.get_meta("tpl_id", "")) == "radial_ring":
				b.emit_signal("pressed")
				picked = true
				break
	_check(picked, "Tarjeta 'Anillo radial' seleccionable en galeria")
	await _step_frames(1)
	tpl_apply.emit_signal("pressed")
	await _step_frames(2)
	_check(tpl_conf.visible, "Pide confirmacion antes de sobrescribir con plantilla")
	tpl_conf.emit_signal("confirmed")
	tpl_conf.hide()
	await _step_frames(3)

	var lay_after: Dictionary = Dictionary(dm.get_ui_screen_design("game_menu_main").get("layout", {}))
	_check(str(lay_after.get("mode", "")) == "radial", "Confirmar aplica plantilla radial_ring al lienzo")
	var img_after_tpl: Image = db.get_texture().get_image().get_region(stage_r)
	_check(float(img_before.compute_image_metrics(img_after_tpl, false).get("max", 0.0)) > 15.0, "Lienzo refleja visualmente la plantilla aplicada")

	# Probar otras plantillas distintas via apply_template_by_id
	for tid in ["fullscreen_tabs", "cards_carousel", "arpg_inventory_grid", "classic_jrpg", "minimalist", "sliding_sidebar"]:
		var ok: bool = ed.apply_template_by_id(tid, true, true)
		await _step_frames(2)
		_check(ok, "Plantilla '%s' aplica sin error" % tid)

	# Probar Deshacer tras aplicar plantilla
	var undo_ok: bool = ed.apply_undo()
	await _step_frames(2)
	_check(undo_ok, "Deshacer revierte la ultima plantilla aplicada")

	# 3) Asistente 3 pasos ('+ Nuevo diseno') en Pestana Menu del juego
	var wiz_dlg: AcceptDialog = h.get("menu_wizard_dialog", null) as AcceptDialog
	var wiz_grid: GridContainer = h.get("wizard_tpl_grid", null) as GridContainer
	var wiz_name: LineEdit = h.get("wizard_name_edit", null) as LineEdit
	var wiz_next: Button = h.get("wizard_next_btn", null) as Button
	var wiz_finish: Button = h.get("wizard_finish_btn", null) as Button
	_check(wiz_btn != null and wiz_dlg != null and wiz_next != null and wiz_finish != null, "Asistente '+ Nuevo diseno' completo")

	wiz_btn.emit_signal("pressed")
	await _step_frames(2)
	_check(wiz_dlg.visible and wiz_grid.get_child_count() >= 8, "Asistente abre en Paso 1 con plantillas")
	# Elegir 'classic_jrpg' en Paso 1
	for card in wiz_grid.get_children():
		if card.get_child_count() > 0 and card.get_child(0).get_child_count() > 1:
			var b := card.get_child(0).get_child(1) as Button
			if b != null and str(b.get_meta("tpl_id", "")) == "classic_jrpg":
				b.emit_signal("pressed")
				break
	wiz_next.emit_signal("pressed")
	await _step_frames(1)
	wiz_name.text = "Menu Asistente P8"
	wiz_next.emit_signal("pressed")
	await _step_frames(1)
	wiz_finish.emit_signal("pressed")
	await _step_frames(3)

	var new_id: String = ed.get_current_screen()
	_check(new_id != "game_menu_main" and dm.has_menu(new_id), "Asistente crea y selecciona nuevo menu: " + new_id)
	var new_design: Dictionary = dm.get_ui_screen_design(new_id)
	var has_wiz_btns := false
	for el in Array(new_design.get("elements", [])):
		if el is Dictionary and str(el.get("type", "")) == "button" and str(el.get("id", "")).begins_with("wiz_btn_"):
			has_wiz_btns = true
			break
	_check(has_wiz_btns, "Asistente inyecta los botones elegidos en Paso 2")

	db.hide()
	db.queue_free()
	await _step_frames(2)

	# 4) Abrir el menu creado por el asistente en F5
	root.size = Vector2i(960, 540)
	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	gm.open_menu_by_id(new_id)
	await _step_frames(4)
	_check(gm.is_menu_open() and gm.get_custom_ui_layer().get_element_count() > 0, "Menu creado por asistente abre completo en F5")
	gm.close_menu()
	gm.queue_free()
	await _step_frames(2)

	dm.delete_custom_menu(new_id)
	dm.reset_ui_screen_design_to_default("game_menu_main", false)
	_finish(0 if fail_count == 0 else 1)
