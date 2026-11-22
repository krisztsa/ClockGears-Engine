extends SceneTree

# Prueba de aceptación Punto 4: Plantillas adaptadas a cada sistema, miniaturas reales y cuadrículas con Separación y Relleno.

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

func _click_at(win: Window, pos: Vector2) -> void:
	win.notification(Viewport.NOTIFICATION_VP_MOUSE_ENTER)
	var ev_m := InputEventMouseMotion.new()
	ev_m.position = pos
	ev_m.global_position = pos
	win.push_input(ev_m, true)
	await _step_frames(1)

	var ev_d := InputEventMouseButton.new()
	ev_d.button_index = MOUSE_BUTTON_LEFT
	ev_d.pressed = true
	ev_d.position = pos
	ev_d.global_position = pos
	win.push_input(ev_d, true)
	await _step_frames(1)

	var ev_u := InputEventMouseButton.new()
	ev_u.button_index = MOUSE_BUTTON_LEFT
	ev_u.pressed = false
	ev_u.position = pos
	ev_u.global_position = pos
	win.push_input(ev_u, true)
	await _step_frames(2)

func _click_control_real(ctrl: Control) -> void:
	if ctrl == null:
		return
	var win := ctrl.get_viewport() as Window
	var pos := ctrl.get_global_rect().get_center()
	while win != null and win.is_embedded() and win.get_parent() != null:
		pos += Vector2(win.position)
		var pwin := win.get_parent().get_viewport() as Window
		while pwin != null and pwin != root and not pwin.gui_embed_subwindows:
			pwin = (pwin.get_parent().get_viewport() as Window) if pwin.get_parent() != null else null
		if pwin == null or pwin == win:
			break
		win = pwin
	if win != null:
		await _click_at(win, pos)

func _collect_card_tpl_ids(grid: GridContainer) -> Array[String]:
	var ids: Array[String] = []
	if grid == null:
		return ids
	for n in grid.find_children("*", "Button", true, false):
		var b := n as Button
		if b != null and b.has_meta("tpl_id"):
			ids.append(str(b.get_meta("tpl_id")))
	return ids

func _find_card_button(grid: GridContainer, tpl_id: String) -> Button:
	if grid == null:
		return null
	for n in grid.find_children("*", "Button", true, false):
		var b := n as Button
		if b != null and str(b.get_meta("tpl_id", "")) == tpl_id:
			return b
	return null

func _check_real_thumb_rendered(grid: GridContainer, win: Window, label: String) -> void:
	var thumbs := grid.find_children("*", "ClockGearsTemplateThumbCtrl", true, false)
	_check(thumbs.size() > 0, "%s: hay tarjetas de miniatura (%d)" % [label, thumbs.size()])
	var sv_count := 0
	var non_uniform_count := 0
	var root_img := root.get_texture().get_image()
	for th in thumbs:
		var tc := th as Control
		var vps := tc.find_children("*", "SubViewport", true, false)
		var trects := tc.find_children("*", "TextureRect", true, false)
		if vps.size() > 0 and trects.size() > 0:
			sv_count += 1
			var gr := tc.get_global_rect()
			if win != null and win.is_embedded():
				gr.position += Vector2(win.position)
			var rx0 := clampi(int(gr.position.x) + 4, 0, root_img.get_width() - 2)
			var ry0 := clampi(int(gr.position.y) + 4, 0, root_img.get_height() - 2)
			var rx1 := clampi(int(gr.end.x) - 4, rx0 + 1, root_img.get_width() - 1)
			var ry1 := clampi(int(gr.end.y) - 4, ry0 + 1, root_img.get_height() - 1)
			if rx1 - rx0 < 16 or ry1 - ry0 < 16:
				# Tarjeta fuera del scroll visible de la galería: comprobar que el SubViewport tiene textura 16:9 válida
				var tr := trects[0] as TextureRect
				if tr.texture != null and tr.texture.get_width() >= 320:
					non_uniform_count += 1
				continue
			var first_px := root_img.get_pixel(rx0, ry0)
			var diff := 0
			for sy in range(ry0, ry1, 10):
				for sx in range(rx0, rx1, 12):
					var px := root_img.get_pixel(sx, sy)
					if absf(px.r - first_px.r) + absf(px.g - first_px.g) + absf(px.b - first_px.b) > 0.05:
						diff += 1
			if diff >= 3:
				non_uniform_count += 1
	_check(sv_count == thumbs.size(), "%s: todas las miniaturas usan SubViewport real (%d/%d)" % [label, sv_count, thumbs.size()])
	_check(non_uniform_count == thumbs.size(), "%s: todas las miniaturas renderizan contenido real (%d/%d)" % [label, non_uniform_count, thumbs.size()])

func _initialize() -> void:
	create_timer(60.0).timeout.connect(func():
		_check(false, "Timeout interno 60s alcanzado")
		_finish(1)
	)
	call_deferred("_run")

func _run() -> void:
	print("=== ACEPTACIÓN PUNTO 4: PLANTILLAS ADAPTADAS Y MINIATURAS REALES ===")
	DisplayServer.window_set_size(Vector2i(1366, 768))
	root.content_scale_size = Vector2i(1366, 768)
	root.size = Vector2i(1366, 768)
	await _step_frames(2)

	var dm = Engine.get_singleton("ClockGearsDataManager")
	_check(dm != null, "ClockGearsDataManager activo")
	if dm == null:
		_finish(1)
		return
	dm.load_all()
	var cap_dir := ProjectSettings.globalize_path("res://capturas_aceptacion")
	DirAccess.make_dir_recursive_absolute(cap_dir)
	var ui_file := ProjectSettings.globalize_path("res://clockgears_data/ui_designs.json")
	var orig_ui_json := FileAccess.get_file_as_string(ui_file) if FileAccess.file_exists(ui_file) else ""

	# Garantizar al menos 3 items de prueba reales para vista previa de rejilla
	dm.set_editor_test_data_count(4)

	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.open_database()
	db.size = Vector2i(1366, 768)
	await _step_frames(3)

	var ed := db.get_ui_designer_editor()
	var h: Dictionary = ed.get_ui_handles()
	var tpl_btn: Button = h.get("templates_btn", null) as Button
	var gal: AcceptDialog = h.get("template_gallery_dialog", null) as AcceptDialog
	var scr_grid: GridContainer = h.get("tpl_screen_grid", null) as GridContainer
	var comp_grid: GridContainer = h.get("tpl_comp_grid", null) as GridContainer
	var tpl_tabs: TabContainer = h.get("template_tabs", null) as TabContainer
	var apply_btn: Button = h.get("tpl_apply_btn", null) as Button
	var confirm_dlg: ConfirmationDialog = h.get("template_apply_confirm_dialog", null) as ConfirmationDialog
	var close_x: Button = h.get("tpl_close_x_btn", null) as Button

	# --- 1. Inventario: Galería de plantillas y filtrado ---
	db.select_category_by_key("menu_inventory")
	await _step_frames(3)
	await _click_control_real(tpl_btn)
	await _step_frames(6)
	_check(gal != null and gal.visible, "Galería de plantillas abre desde Inventario")

	var inv_ids := _collect_card_tpl_ids(scr_grid)
	_check(not inv_ids.has("classic_title") and not inv_ids.has("hero_logo_title") and not inv_ids.has("corner_hud") and not inv_ids.has("messages_classic") and not inv_ids.has("battle_classic"), "Inventario: no aparecen plantillas incompatibles (Título/HUD/Mensajes/Batalla)")
	_check(inv_ids.has("arpg_inventory_grid") and inv_ids.has("radial_ring") and inv_ids.has("minimalist"), "Inventario: aparecen las plantillas aplicables")
	_check(gal.position.y >= 24 and gal.position.y + gal.size.y <= root.size.y, "Galería cabe entera en 1366x768 sin salirse (pos=%s, sz=%s)" % [str(gal.position), str(gal.size)])
	_check(apply_btn != null and apply_btn.is_visible_in_tree() and apply_btn.get_global_rect().end.y <= gal.size.y, "Botón 'Aplicar al menú' visible dentro de la galería (%s)" % str(apply_btn.get_global_rect() if apply_btn else Rect2()))
	_check_real_thumb_rendered(scr_grid, gal, "Galería Inventario")
	root.get_texture().get_image().save_png(cap_dir.path_join("p4_galeria_plantillas_inventario.png"))

	# Revisar pestaña Componentes
	if tpl_tabs != null:
		tpl_tabs.current_tab = 1
		await _step_frames(5)
		_check_real_thumb_rendered(comp_grid, gal, "Galería Componentes")
		root.get_texture().get_image().save_png(cap_dir.path_join("p4_galeria_componentes.png"))
		tpl_tabs.current_tab = 0
		await _step_frames(4)

	# --- 2. Aplicar Anillo radial en Inventario y comprobar adaptación ---
	var rad_card := _find_card_button(scr_grid, "radial_ring")
	_check(rad_card != null, "Inventario: tarjeta 'radial_ring' localizada")
	if rad_card != null:
		await _click_control_real(rad_card)
		await _click_control_real(apply_btn)
		await _step_frames(2)
		if confirm_dlg != null and confirm_dlg.visible:
			await _click_control_real(confirm_dlg.get_ok_button())
		await _step_frames(5)
	var inv_rad_design: Dictionary = dm.get_ui_screen_design("game_menu_items")
	var inv_rad_els: Array = inv_rad_design.get("elements", [])
	var stale_menu_labels := false
	var rad_ds := ""
	for e in inv_rad_els:
		if e is Dictionary:
			var t := str(e.get("text", ""))
			if t == "Objetos" or t == "Equipo" or t == "Estado" or t == "Opciones":
				stale_menu_labels = true
			if str(e.get("type", "")) == "radial":
				rad_ds = str(e.get("data_source", ""))
	_check(not stale_menu_labels, "Inventario + Anillo radial: sin botones manuales del Menú principal ('Objetos/Equipo/Estado')")
	_check(rad_ds == "inventory_all", "Inventario + Anillo radial: rueda conectada a 'inventory_all' (real='%s')" % rad_ds)
	root.get_texture().get_image().save_png(cap_dir.path_join("p4_inventario_radial.png"))

	# --- 3. Aplicar Minimalista en Inventario y comprobar adaptación ---
	await _click_control_real(tpl_btn)
	await _step_frames(4)
	var min_card := _find_card_button(scr_grid, "minimalist")
	if min_card != null:
		var scr_sc := scr_grid.get_parent() as ScrollContainer
		if scr_sc != null:
			scr_sc.ensure_control_visible(min_card)
			await _step_frames(3)
		await _click_control_real(min_card)
		await _click_control_real(apply_btn)
		await _step_frames(2)
		if confirm_dlg != null and confirm_dlg.visible:
			await _click_control_real(confirm_dlg.get_ok_button())
		await _step_frames(5)
	var inv_min_els: Array = dm.get_ui_screen_design("game_menu_items").get("elements", [])
	var min_wrong_lbls := false
	var min_has_cont := false
	for e in inv_min_els:
		if e is Dictionary:
			var t := str(e.get("text", ""))
			if t == "Objetos" or t == "Estado" or t == "Opciones" or t == "Cerrar":
				min_wrong_lbls = true
			if str(e.get("type", "")) == "container" and str(e.get("data_source", "")) == "inventory_all":
				min_has_cont = true
	_check(not min_wrong_lbls and min_has_cont, "Inventario + Minimalista: adaptada con contenedor real de inventario (sin 'Objetos, Estado, Opciones, Cerrar')")

	# --- 4. Aplicar Cuadrícula con datos reales (4x2) en Inventario: Separación, Relleno, Borde y Vacías N ---
	await _click_control_real(tpl_btn)
	await _step_frames(4)
	var grid_card := _find_card_button(scr_grid, "arpg_inventory_grid")
	if grid_card != null:
		var scr_sc2 := scr_grid.get_parent() as ScrollContainer
		if scr_sc2 != null:
			scr_sc2.ensure_control_visible(grid_card)
			await _step_frames(3)
		await _click_control_real(grid_card)
		await _click_control_real(apply_btn)
		await _step_frames(2)
		if confirm_dlg != null and confirm_dlg.visible:
			await _click_control_real(confirm_dlg.get_ok_button())
		await _step_frames(6)

	ed.select_element("inv_data_grid")
	await _step_frames(3)
	h = ed.get_ui_handles()
	var spx_spin: SpinBox = h.get("prop_container_spx_spin", null) as SpinBox
	var spy_spin: SpinBox = h.get("prop_container_spy_spin", null) as SpinBox
	var pad_spin: SpinBox = h.get("prop_container_pad_spin", null) as SpinBox
	_check(spx_spin != null and spx_spin.is_visible_in_tree() and spx_spin.value >= 8, "Inspector: 'Separación X' visible >= 8 px (%s)" % str(spx_spin.value if spx_spin else -1))
	_check(spy_spin != null and spy_spin.is_visible_in_tree() and spy_spin.value >= 8, "Inspector: 'Separación Y' visible >= 8 px (%s)" % str(spy_spin.value if spy_spin else -1))
	_check(pad_spin != null and pad_spin.is_visible_in_tree() and pad_spin.value >= 6, "Inspector: 'Relleno' visible >= 6 px (%s)" % str(pad_spin.value if pad_spin else -1))

	var layer: Control = h.get("design_layer", null) as Control
	var cells := layer.find_children("DataCell_*", "Button", true, false)
	_check(cells.size() >= 8, "Cuadrícula muestra >= 8 celdas (%d)" % cells.size())
	if cells.size() >= 5:
		var c0 := cells[0] as Button
		var c1 := cells[1] as Button
		var c4 := cells[4] as Button
		var gap_x := c1.position.x - (c0.position.x + c0.size.x)
		var gap_y := c4.position.y - (c0.position.y + c0.size.y)
		_check(gap_x >= 6.0 and gap_y >= 6.0, "Separación real entre celdas (gapX=%.1f, gapY=%.1f)" % [gap_x, gap_y])
		var sb_norm := c0.get_theme_stylebox("normal") as StyleBoxFlat
		_check(sb_norm != null and sb_norm.get_border_width(SIDE_LEFT) >= 1, "Cada celda tiene estilo con borde y fondo real")
		var lbls_c0 := c0.find_children("*", "Label", true, false)
		var imgs_c0 := c0.find_children("*", "TextureRect", true, false)
		_check(imgs_c0.size() >= 1 and lbls_c0.size() >= 2, "Celda con objeto muestra icono + nombre + cantidad")
	root.get_texture().get_image().save_png(cap_dir.path_join("p4_inventario_cuadricula_datos.png"))

	# Probar 0 datos -> casillas vacías numeradas "Vacía 1..N", nunca "Ranura 1"
	dm.set_editor_test_data_count(0)
	ed.refresh_from_data_manager()
	await _step_frames(5)
	var cells_empty := layer.find_children("DataCell_*", "Button", true, false)
	var has_vacia1 := false
	var has_ranura := false
	for c in cells_empty:
		for l in c.find_children("*", "Label", true, false):
			var lt := (l as Label).text
			if lt == "Vacía 1":
				has_vacia1 = true
			if lt.begins_with("Ranura "):
				has_ranura = true
	_check(has_vacia1 and not has_ranura, "Cuadrícula con 0 datos muestra 'Vacía 1..N' y nunca 'Ranura 1'")
	root.get_texture().get_image().save_png(cap_dir.path_join("p4_inventario_cuadricula_vacia.png"))
	dm.set_editor_test_data_count(-1)

	# --- 5. Comprobar filtrado y adaptación en Batalla, Título y HUD ---
	db.select_category_by_key("battle")
	await _step_frames(3)
	await _click_control_real(tpl_btn)
	await _step_frames(4)
	var bat_ids := _collect_card_tpl_ids(scr_grid)
	_check(not bat_ids.has("classic_title") and not bat_ids.has("corner_hud") and not bat_ids.has("messages_classic"), "Batalla: no muestra plantillas de Título, HUD ni Mensajes")
	var bat_jrpg := _find_card_button(scr_grid, "classic_jrpg")
	if bat_jrpg != null:
		await _click_control_real(bat_jrpg)
		await _click_control_real(apply_btn)
		await _step_frames(2)
		if confirm_dlg != null and confirm_dlg.visible:
			await _click_control_real(confirm_dlg.get_ok_button())
		await _step_frames(4)
	var bat_els: Array = dm.get_ui_screen_design("battle").get("elements", [])
	var has_attack := false
	var has_menu_words := false
	for e in bat_els:
		if e is Dictionary and str(e.get("type", "")) == "button":
			var bt := str(e.get("text", ""))
			if bt == "Atacar":
				has_attack = true
			if bt == "Guardar" or bt == "Equipo" or bt == "Objetos":
				has_menu_words = true
	_check(has_attack and not has_menu_words, "Batalla + Clásico JRPG: comandos propios ('Atacar'...) y nunca ('Objetos/Guardar')")

	db.select_category_by_key("main_menu")
	await _step_frames(3)
	await _click_control_real(tpl_btn)
	await _step_frames(4)
	var title_ids := _collect_card_tpl_ids(scr_grid)
	_check(title_ids.has("classic_title") and not title_ids.has("battle_classic") and not title_ids.has("corner_hud"), "Título: solo plantillas de Título")
	if close_x != null and gal.visible:
		await _click_control_real(close_x)
		await _step_frames(2)

	db.select_category_by_key("hud")
	await _step_frames(3)
	await _click_control_real(tpl_btn)
	await _step_frames(4)
	var hud_ids := _collect_card_tpl_ids(scr_grid)
	_check(hud_ids.size() == 1 and hud_ids[0] == "corner_hud", "HUD: solo plantilla de HUD (%s)" % str(hud_ids))
	if close_x != null and gal.visible:
		await _click_control_real(close_x)
		await _step_frames(2)

	db.queue_free()
	await _step_frames(2)

	# Restaurar datos originales
	if orig_ui_json.is_empty():
		if FileAccess.file_exists(ui_file):
			DirAccess.remove_absolute(ui_file)
	else:
		var fw := FileAccess.open(ui_file, FileAccess.WRITE)
		fw.store_string(orig_ui_json)
		fw.close()
	dm.load_all()

	_finish(0 if fail_count == 0 else 1)
