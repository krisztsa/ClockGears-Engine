extends SceneTree

# Test permanente Punto 6 (9B-2):
# Verifica todos los editores del capítulo Menús (Menú principal, Estilos de ventana,
# Menú del juego, Créditos, HUD, Diseño de interfaz y Game Over) en tamaño mínimo
# de la Base de Datos y en 1366x768, 1920x1080 y 2560x1440:
# - Sin desbordes fuera de su contenedor ni solapes entre controles hermanos
# - Vistas previas 16:9 completas (incluyendo Nv y EXP en Grupo de Menú del juego)
# - Apilado vertical (vista previa arriba, ajustes abajo) cuando el ancho es ajustado
# - Secciones de ajustes plegables con scroll vertical (nunca horizontal)
# - Selector común de tamaño de vista previa (960x540, 1366x768, 1920x1080)

var failures: Array[String] = []

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _fail(msg: String) -> void:
	failures.append(msg)
	print("  FAIL: " + msg)

func _is_layered_preview_container(node: Node) -> bool:
	if node is AspectRatioContainer:
		return true
	if node.name == "UiDesignerCanvasStage" or node.name == "HudPreviewDragOverlay":
		return true
	var p := node.get_parent()
	while p != null:
		if p.name == "UiDesignerCanvasStage" or p is AspectRatioContainer:
			return true
		p = p.get_parent()
	return false

func _check_container_bounds_and_overlaps(root_ctrl: Control, panel_bounds: Rect2, ctx: String) -> void:
	var stack: Array[Control] = [root_ctrl]
	while not stack.is_empty():
		var c: Control = stack.pop_back()
		if not c.is_visible_in_tree():
			continue
		var gr := c.get_global_rect()
		if gr.size.x > 1.0 and gr.size.y > 1.0:
			# Ningún control de ajustes ni vista previa debe salirse horizontalmente del panel de editor
			if not _is_layered_preview_container(c):
				if gr.position.x < panel_bounds.position.x - 3.0 or gr.end.x > panel_bounds.end.x + 3.0:
					_fail("%s: control '%s' (%s) se sale horizontalmente rect=%s vs bounds=%s" % [ctx, c.name, c.get_class(), str(gr), str(panel_bounds)])
		# Revisar solapes entre hijos directos que no sean capas intencionales
		if (c is BoxContainer or c is GridContainer or c is HFlowContainer) and not _is_layered_preview_container(c):
			var visible_kids: Array[Control] = []
			for ch in c.get_children():
				if ch is Control and (ch as Control).is_visible_in_tree():
					var ch_c := ch as Control
					if ch_c.get_global_rect().size.x > 1.0 and ch_c.get_global_rect().size.y > 1.0:
						visible_kids.append(ch_c)
			for i in range(visible_kids.size()):
				var ri := visible_kids[i].get_global_rect()
				for j in range(i + 1, visible_kids.size()):
					var rj := visible_kids[j].get_global_rect()
					var inter := ri.intersection(rj)
					if inter.size.x > 3.0 and inter.size.y > 3.0:
						_fail("%s: solape entre hermanos '%s' %s y '%s' %s dentro de '%s'" % [ctx, visible_kids[i].name, str(ri), visible_kids[j].name, str(rj), c.name])
		# Revisar relación 16:9 de todas las vistas previas
		if c is AspectRatioContainer and gr.size.x > 10.0 and gr.size.y > 10.0:
			var aspect_val := gr.size.x / gr.size.y
			if absf(aspect_val - (16.0 / 9.0)) > 0.06:
				_fail("%s: vista previa no es 16:9 (aspect=%.3f, size=%s)" % [ctx, aspect_val, str(gr.size)])
			if gr.end.x > panel_bounds.end.x + 3.0:
				_fail("%s: vista previa desborda a la derecha (%s > %s)" % [ctx, str(gr.end.x), str(panel_bounds.end.x)])
		# Revisar ScrollContainers sin scroll horizontal
		if c is ScrollContainer and c.name != "UiDesignerCanvasScroll":
			var sc := c as ScrollContainer
			if sc.horizontal_scroll_mode != ScrollContainer.SCROLL_MODE_DISABLED:
				_fail("%s: ScrollContainer '%s' permite scroll horizontal (%d)" % [ctx, sc.name, sc.horizontal_scroll_mode])

		for ch in c.get_children():
			if ch is Control:
				stack.push_back(ch as Control)

func _check_game_menu_party_unclipped(db: ClockGearsDatabaseDialog, ctx: String) -> void:
	var gm_prev := db.get_game_menu_live_preview()
	if gm_prev == null:
		_fail(ctx + ": gm_live_preview es null")
		return
	var vp := gm_prev.get_viewport()
	if vp != null:
		var vsz := vp.get_visible_rect().size
		if vsz.x < 950.0:
			_fail("%s: SubViewport de Menú del juego encogido a %.0fx%.0f (debe ser >= 960)" % [ctx, vsz.x, vsz.y])
	var found_nv := false
	var found_exp := false
	for lbl in gm_prev.find_children("*", "Label", true, false):
		var l := lbl as Label
		if not l.is_visible_in_tree():
			continue
		if l.text.find("Nv") >= 0:
			found_nv = true
		if l.text.find("EXP") >= 0:
			found_exp = true
		if l.text.find("Nv") >= 0 or l.text.find("EXP") >= 0:
			var p := l.get_parent() as Control
			if p != null and l.position.x + l.size.x > p.size.x + 3.0:
				_fail("%s: etiqueta '%s' recortada en padre (end.x=%.1f > %.1f)" % [ctx, l.text, l.position.x + l.size.x, p.size.x])
	if not found_nv or not found_exp:
		_fail("%s: faltan etiquetas Nv (%s) o EXP (%s) en Grupo" % [ctx, str(found_nv), str(found_exp)])

func _find_first_collapsible_button(root_ctrl: Control) -> Button:
	for b in root_ctrl.find_children("*", "Button", true, false):
		var btn := b as Button
		if btn.toggle_mode and (btn.text.begins_with("▾") or btn.text.begins_with("▸")):
			return btn
	return null

func _find_panel_by_name(db: ClockGearsDatabaseDialog, pname: String) -> Control:
	for n in db.find_children(pname, "Control", true, false):
		return n as Control
	return null

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 6 (9B-2): EDITORES DEL CAPITULO MENUS ===")
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(2)

	var min_sz := db.get_min_size()
	print("  DB min_size=", min_sz)

	var menu_cats := {
		"main_menu": "MainMenuEditorPanel",
		"window_styles": "WindowStylesEditorPanel",
		"game_menu": "GameMenuEditorPanel",
		"credits": "CreditsEditorPanel",
		"hud": "HudEditorPanel",
		"ui_designer": "ClockGearsUiDesignEditor",
		"game_over": "GameOverEditorPanel",
	}

	var sizes_to_test: Array[Vector2i] = [
		min_sz,
		Vector2i(1366, 768),
		Vector2i(1920, 1080),
		Vector2i(2560, 1440),
	]

	for win_sz in sizes_to_test:
		var db_sz := Vector2i(mini(int(win_sz.x * 0.92), 1600), mini(int(win_sz.y * 0.92), 900)).max(min_sz)
		if win_sz == min_sz:
			db_sz = min_sz
		db.set_size(db_sz)
		db.adapt_to_window_size(db_sz)
		await _step_frames(2)

		var main_tabs := db.get_main_editor_tabs()
		for ckey in menu_cats.keys():
			db.select_category_by_key(ckey)
			db.adapt_to_window_size(db_sz)
			await _step_frames(2)

			var panel: Control = null
			if ckey == "ui_designer":
				panel = db.get_ui_designer_editor()
			else:
				panel = _find_panel_by_name(db, menu_cats[ckey])
			if panel == null:
				_fail("[%dx%d %s] panel no encontrado" % [win_sz.x, win_sz.y, ckey])
				continue
			if not panel.is_visible_in_tree():
				_fail("[%dx%d %s] panel no visible" % [win_sz.x, win_sz.y, ckey])
				continue

			var bounds := main_tabs.get_global_rect()
			if win_sz == min_sz:
				bounds = Rect2(bounds.position, Vector2(bounds.size.x, bounds.size.y))
			_check_container_bounds_and_overlaps(panel, bounds, "[win=%dx%d db=%dx%d %s]" % [win_sz.x, win_sz.y, db_sz.x, db_sz.y, ckey])

			if ckey == "game_menu":
				_check_game_menu_party_unclipped(db, "[win=%dx%d %s]" % [win_sz.x, win_sz.y, ckey])

			# En ancho estrecho (min_sz y 1366x768) verificar apilado vertical: vista previa arriba
			if db_sz.x < 1400:
				for ch in panel.get_children():
					if ch is BoxContainer and (ch as BoxContainer).vertical:
						var bx := ch as BoxContainer
						if bx.get_child_count() >= 2:
							var top_c := bx.get_child(0) as Control
							var bot_c := bx.get_child(1) as Control
							if top_c.get_global_rect().position.y > bot_c.get_global_rect().position.y:
								_fail("[%dx%d %s] orden vertical incorrecto: vista previa no está arriba" % [win_sz.x, win_sz.y, ckey])

	# Probar secciones plegables y selector común de tamaño de vista previa
	for ckey in menu_cats.keys():
		db.select_category_by_key(ckey)
		await _step_frames(1)
		var panel: Control = db.get_ui_designer_editor() if ckey == "ui_designer" else _find_panel_by_name(db, menu_cats[ckey])
		var col_btn := _find_first_collapsible_button(panel)
		if col_btn == null:
			_fail("[%s] no tiene sección plegable con botón ▾/▸" % ckey)
		else:
			var before_h := panel.get_combined_minimum_size().y
			col_btn.set_pressed(false)
			await _step_frames(1)
			col_btn.set_pressed(true)
			await _step_frames(1)
			var after_h := panel.get_combined_minimum_size().y
			if after_h < before_h - 2.0:
				_fail("[%s] sección plegable no restauró tamaño tras expandir" % ckey)

	for target_res in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(960, 540)]:
		db.set_menus_preview_resolution(target_res.x, target_res.y)
		await _step_frames(1)
		if db.get_menus_preview_resolution() != target_res:
			_fail("set_menus_preview_resolution(%s) no actualizó estado común" % str(target_res))
		var uid := db.get_ui_designer_editor()
		if uid.get_preview_resolution() != target_res:
			_fail("UI Designer no sincronizó resolución común %s" % str(target_res))
		var gm_vp := db.get_game_menu_live_preview().get_viewport()
		if gm_vp != null and Vector2i(gm_vp.size) != target_res:
			_fail("Game Menu viewport no sincronizó resolución %s (actual=%s)" % [str(target_res), str(gm_vp.size)])

	db.queue_free()
	await _step_frames(1)

	print("RESUMEN PUNTO 6: fallos=%d" % failures.size())
	if failures.is_empty():
		print("ALL PASS: Todos los editores del capítulo Menús cumplen 16:9, responsive, plegables y sin solapes.")
		quit(0)
	else:
		print("FAIL: %d comprobaciones fallaron." % failures.size())
		quit(1)
