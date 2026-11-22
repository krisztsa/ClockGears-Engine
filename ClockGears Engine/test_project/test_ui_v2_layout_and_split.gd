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

func pump(frames := 3) -> void:
	for i in frames:
		await process_frame

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var t0 := Time.get_ticks_msec()
	var cgt = Engine.get_singleton("ClockGearsTheme")
	var dm = Engine.get_singleton("ClockGearsDataManager") if Engine.has_singleton("ClockGearsDataManager") else null
	var dm_owned := false
	if dm == null and ClassDB.class_exists("ClockGearsDataManager"):
		dm = ClassDB.instantiate("ClockGearsDataManager")
		dm_owned = true
		dm.load_project_data("res://clockgears_data")

	var db = ClassDB.instantiate("ClockGearsDatabaseDialog")
	get_root().add_child(db)
	db.popup_centered(Vector2i(1366, 768))
	db.size = Vector2i(1366, 768)
	await pump(4)

	# --- PUNTO 1: PIE DE LA VENTANA SIN SUPERPONER ---
	var content_root: Control = db.call("get_content_root")
	var footer_sep: HSeparator = db.call("get_footer_separator")
	var footer_bar: Control = db.call("get_footer_bar")
	var ok_btn: Button = db.get_ok_button()
	var app_btn: Button = db.call("get_apply_button")
	check(content_root != null and footer_sep != null and footer_bar != null and ok_btn != null and app_btn != null,
		"Punto 1: Footer bar, separator, content root, OK and Aplicar present")

	if footer_bar and footer_sep and ok_btn and app_btn:
		for res in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
			db.size = res
			db.adapt_to_window_size(res)
			await pump(3)
			check(footer_bar.size.y >= 52.0 and footer_bar.size.y <= 62.0,
				"Punto 1 %s: Footer fixed ~56px (h=%.1f)" % [str(res), footer_bar.size.y])
			var gap := app_btn.global_position.x - (ok_btn.global_position.x + ok_btn.size.x)
			var mid_btns := (ok_btn.global_position.x + app_btn.global_position.x + app_btn.size.x) * 0.5
			var mid_bar := footer_bar.global_position.x + footer_bar.size.x * 0.5
			check(absf(gap - 16.0) <= 2.0 and absf(mid_btns - mid_bar) <= 8.0,
				"Punto 1 %s: OK & Aplicar centered 16px apart (gap=%.1f, ok=%s, app=%s, fbar=%s)" % [str(res), gap, str(ok_btn.get_global_rect()), str(app_btn.get_global_rect()), str(footer_bar.get_global_rect())])
			check(ok_btn.theme_type_variation == &"ClockGearsPrimaryButton",
				"Punto 1 %s: OK is primary button" % str(res))

		# Check NO control in ANY chapter overlaps footer_bar across all 3 resolutions
		var sample_cats := ["actors", "weapons", "items", "enemies", "animations", "tilesets", "system", "main_menu", "game_menu", "hud", "credits", "game_over"]
		var overlaps := 0
		var worst_msg := ""
		for res in [Vector2i(1366, 768), Vector2i(1920, 1080), Vector2i(2560, 1440)]:
			db.size = res
			db.adapt_to_window_size(res)
			await pump(2)
			for cat_k in sample_cats:
				db.select_category_by_key(cat_k)
				db.adapt_to_window_size(res)
				await pump(2)
				var frect := footer_bar.get_global_rect()
				var srect := footer_sep.get_global_rect()
				var crect := content_root.get_global_rect()
				if crect.end.y > srect.position.y + 1.5:
					overlaps += 1
					worst_msg = "%s %s content end %.1f > sep %.1f" % [str(res), cat_k, crect.end.y, srect.position.y]
				if crect.intersects(frect):
					overlaps += 1
					worst_msg = "%s %s content intersects footer" % [str(res), cat_k]
				# Also inspect top-level child panels inside content_root (ignoring scrolled internals clipped inside ScrollContainers)
				var uh: Dictionary = db.call("get_ui_designer_ui_handles")
				for panel_k in [uh.get("layout_root"), uh.get("canvas_card"), uh.get("props_card")]:
					if panel_k is Control and (panel_k as Control).is_visible_in_tree():
						var pr := (panel_k as Control).get_global_rect()
						if pr.intersects(frect):
							overlaps += 1
							worst_msg = "%s %s %s intersects footer (%s vs %s)" % [str(res), cat_k, (panel_k as Node).name, str(pr), str(frect)]
		check(overlaps == 0, "Punto 1: Zero footer overlaps across all chapters at 1366/1920/2560 (%s)" % worst_msg)

	# --- PUNTO 2: BARRA DE HERRAMIENTAS COMPACTA ---
	db.select_category_by_key("main_menu")
	db.size = Vector2i(1366, 768)
	db.adapt_to_window_size(Vector2i(1366, 768))
	await pump(4)
	var h2: Dictionary = db.call("get_ui_designer_ui_handles")
	var top_card: Control = h2.get("top_card")
	var row1_flow: HFlowContainer = h2.get("top_row1_flow")
	var row2_flow: HFlowContainer = h2.get("top_row2_flow")
	var reset_b: Button = h2.get("reset_btn")
	var undo_b: Button = h2.get("undo_btn")
	var redo_b: Button = h2.get("redo_btn")
	var st_lbl: Label = h2.get("screen_status_lbl")
	check(h2.get("add_button_bar_btn") == null, "Punto 2: + Boton removed from top toolbar (stays in Disenos)")
	check(h2.get("layer_add_button_btn") != null, "Punto 2: + Boton still present in Disenos panel")
	if top_card and row1_flow and row2_flow:
		check(top_card.size.y <= 96.0 and row1_flow.size.y <= 44.0 and row2_flow.size.y <= 44.0,
			"Punto 2 (1366x768): Compact 2-row toolbar <=96px total, <=44px/row (total=%.1f, r1=%.1f, r2=%.1f)" % [top_card.size.y, row1_flow.size.y, row2_flow.size.y])
		check(row1_flow.get_line_count() == 1 and row2_flow.get_line_count() == 1,
			"Punto 2 (1366x768): Row1 and Row2 each stay 1 line (lines=%d, %d)" % [row1_flow.get_line_count(), row2_flow.get_line_count()])
	if reset_b and undo_b and redo_b:
		check(reset_b.tooltip_text.contains("Restaurar plantilla original") and reset_b.icon != null and reset_b.text.is_empty(),
			"Punto 2: Restaurar is icon-only with tooltip")
		check(not undo_b.text.contains("(0)") and not redo_b.text.contains("(0)"),
			"Punto 2: Deshacer/Rehacer do not show '(0)' text (u='%s', r='%s')" % [undo_b.text, redo_b.text])
	if st_lbl:
		var ccard: Control = h2.get("canvas_card")
		check(ccard != null and ccard.is_ancestor_of(st_lbl) and not top_card.is_ancestor_of(st_lbl),
			"Punto 2: Status line moved outside top toolbar to bottom of canvas card")

	db.size = Vector2i(1920, 1080)
	db.adapt_to_window_size(Vector2i(1920, 1080))
	await pump(4)
	check(not row2_flow.visible and row1_flow.get_line_count() == 1 and top_card.size.y <= 52.0,
		"Punto 2 (1920x1080): Toolbar fits in 1 single row (total=%.1f, r2_vis=%s, lines=%d)" % [top_card.size.y, str(row2_flow.visible), row1_flow.get_line_count()])

	db.size = Vector2i(2560, 1440)
	db.adapt_to_window_size(Vector2i(2560, 1440))
	await pump(4)
	check(top_card.size.y <= 52.0 and not row2_flow.visible and row1_flow.get_line_count() == 1,
		"Punto 2 (2560x1440): Toolbar fits in 1 single row (total=%.1f, r2_vis=%s, lines=%d)" % [top_card.size.y, str(row2_flow.visible), row1_flow.get_line_count()])

	# --- PUNTO 3: Vista previa completa ---
	for res_p3 in [Vector2i(1366, 768), Vector2i(1920, 1080)]:
		db.size = res_p3
		db.adapt_to_window_size(res_p3)
		await pump(5)
		var h3: Dictionary = db.call("get_ui_designer_ui_handles")
		var cscroll: Control = h3.get("canvas_scroll")
		var cstage: Control = h3.get("canvas_stage")
		var lcard: Control = h3.get("wide_left_col")
		var rcard: Control = h3.get("wide_right_col")
		var whbox: Control = h3.get("wide_hbox")
		if cscroll and cstage and whbox:
			var sc_rect: Rect2 = cscroll.get_global_rect()
			var st_scaled: Vector2 = cstage.size * cstage.scale
			var st_rect: Rect2 = Rect2(cstage.get_global_position(), st_scaled)
			var w_ratio: float = st_scaled.x / max(1.0, sc_rect.size.x)
			var aspect: float = st_scaled.x / max(1.0, st_scaled.y)
			var inside_ok: bool = (st_rect.position.x >= sc_rect.position.x + 6.0 and st_rect.position.y >= sc_rect.position.y + 6.0 and st_rect.end.x <= sc_rect.end.x - 6.0 and st_rect.end.y <= sc_rect.end.y - 6.0)
			if res_p3 == Vector2i(1366, 768):
				check(w_ratio >= 0.90, "Punto 3 (1366x768): Preview fills >=90%% canvas width (%.1f%%, stage_w=%.1f, scroll_w=%.1f)" % [w_ratio * 100.0, st_scaled.x, sc_rect.size.x])
			check(inside_ok and absf(aspect - (16.0 / 9.0)) < 0.03,
				"Punto 3 %s: Full 16:9 preview inside canvas with ~8px margin (aspect=%.3f, st=%s, sc=%s)" % [str(res_p3), aspect, str(st_rect), str(sc_rect)])
			if lcard and rcard and whbox.size.x > 100.0:
				var l_pct: float = lcard.size.x / whbox.size.x
				var r_pct: float = rcard.size.x / whbox.size.x
				if res_p3 == Vector2i(1366, 768):
					check(l_pct >= 0.17 and l_pct <= 0.25 and r_pct >= 0.30 and r_pct <= 0.38,
						"Punto 3 (1366x768): Three zones stay ~21%% / rest / ~34%% (left=%.1f%%, right=%.1f%%)" % [l_pct * 100.0, r_pct * 100.0])
				else:
					check(lcard.size.x >= 316.0 and lcard.size.x <= 324.0 and rcard.size.x >= 556.0 and rcard.size.x <= 564.0,
						"Punto 3 %s: Left/right zones respect 320px / 560px max clamp (left=%.1f, right=%.1f)" % [str(res_p3), lcard.size.x, rcard.size.x])

	# --- PUNTO 4: Separar "Mensajes y batalla" en "Mensajes" y "Batalla" ---
	db.size = Vector2i(1366, 768)
	db.adapt_to_window_size(Vector2i(1366, 768))
	await pump(3)
	var st4: Dictionary = db.call("get_chapter_structure_info")
	var menus_chap := {}
	for cd in st4.get("chapters", []):
		if str(cd.get("name", "")) == "Menús":
			menus_chap = cd
			break
	var cat_keys: Array = menus_chap.get("category_keys", [])
	check(cat_keys.has("messages") and cat_keys.has("battle") and not cat_keys.has("messages_battle"),
		"Punto 4: Chapter Menus has 'messages' and 'battle' and no 'messages_battle' (%s)" % [str(cat_keys)])

	for scr_k in ["messages", "battle"]:
		db.select_category_by_key(scr_k)
		await pump(4)
		var st_cur: Dictionary = db.call("get_chapter_structure_info")
		check(str(st_cur.get("current_category_key", "")) == scr_k, "Punto 4: Tab '%s' selected cleanly" % scr_k)
		var hs: Dictionary = db.call("get_ui_designer_ui_handles")
		var ed: Control = hs.get("editor") if hs.has("editor") else null
		var cur_sid_ok: bool = (hs.get("wide_hbox") is Control and (hs.get("wide_hbox") as Control).is_visible_in_tree() and hs.get("canvas_stage") is Control and hs.get("props_card") is Control)
		check(cur_sid_ok, "Punto 4 (%s): Uses same 3-zone UI designer layout" % scr_k)

	# Verificar elementos por defecto en Mensajes y Batalla
	var msg_def: Dictionary = dm.get_default_ui_screen_template("messages")
	var msg_names: Array = []
	for me in msg_def.get("elements", []):
		msg_names.append(str(me.get("name", "")))
	check(msg_names.has("Cuadro de diálogo") and msg_names.has("Caja de nombre") and msg_names.has("Retrato hablante") and msg_names.has("Ventana de elecciones") and msg_names.has("Entrada de número") and msg_names.has("Historial de mensajes"),
		"Punto 4: Mensajes default contains all 6 dialogue components (%s)" % [str(msg_names)])

	var bat_def: Dictionary = dm.get_default_ui_screen_template("battle")
	var bat_names: Array = []
	for be in bat_def.get("elements", []):
		bat_names.append(str(be.get("name", "")))
	check(bat_names.has("Marco comandos batalla") and bat_names.has("Atacar") and bat_names.has("Habilidad") and bat_names.has("Objeto") and bat_names.has("Defender") and bat_names.has("Huir") and bat_names.has("Panel del grupo") and bat_names.has("Panel de enemigos") and bat_names.has("Aviso de batalla"),
		"Punto 4: Batalla default contains 5 commands, party, enemies and 'Aviso de batalla' (%s)" % [str(bat_names)])

	# Verificar Probar (open_test_popup) en ambas pestañas
	for test_k in ["messages", "battle"]:
		db.select_category_by_key(test_k)
		await pump(3)
		var ht: Dictionary = db.call("get_ui_designer_ui_handles")
		var tbtn: Button = ht.get("test_btn")
		check(tbtn != null and tbtn.is_visible_in_tree(), "Punto 4 (%s): Probar button present" % test_k)
		if tbtn:
			var ed_panel: Node = ht.get("panel")
			tbtn.emit_signal("pressed")
			var test_win: Window = ed_panel.get_node_or_null("UiDesignTestPopup") as Window if ed_panel else null
			if test_win == null:
				await pump(2)
				test_win = ed_panel.get_node_or_null("UiDesignTestPopup") as Window if ed_panel else null
			else:
				await pump(2)
			var opened_ok := false
			var has_layer := false
			if test_win and is_instance_valid(test_win):
				opened_ok = true
				var lyr = test_win.find_child("TestUiIsolatedLayer", true, false)
				has_layer = (lyr != null)
				test_win.emit_signal("close_requested")
				await pump(2)
			check(opened_ok and has_layer, "Punto 4 (%s): Probar opens live preview window with TestUiIsolatedLayer" % test_k)

	# Verificar migración sin pérdidas y respaldo automático en carpeta temporal aislada
	var mig_dir := "user://test_mb_split_tmp"
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(mig_dir))
	var da_src := DirAccess.open("res://clockgears_data")
	if da_src:
		da_src.list_dir_begin()
		var fn := da_src.get_next()
		while fn != "":
			if not da_src.current_is_dir() and fn.ends_with(".json"):
				var content := FileAccess.get_file_as_bytes("res://clockgears_data".path_join(fn))
				var fw := FileAccess.open(mig_dir.path_join(fn), FileAccess.WRITE)
				fw.store_buffer(content)
				fw.close()
			fn = da_src.get_next()
		da_src.list_dir_end()

	# Capturar render pre-migración (estilo estándar) de Mensaje y Combate real
	var _render_msg_img := func(p_text: String) -> Image:
		var vp := SubViewport.new()
		vp.size = Vector2i(960, 540)
		vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
		get_root().add_child(vp)
		var bg := ColorRect.new()
		bg.size = Vector2(960, 540)
		bg.color = Color(0.08, 0.1, 0.14, 1.0)
		vp.add_child(bg)
		var mw := ClockGearsMessageWindow.new()
		vp.add_child(mw)
		mw.show_message("\\>" + p_text, "Personaje", "", 2, 0)
		await pump(3)
		var img := vp.get_texture().get_image()
		vp.queue_free()
		await pump(1)
		return img

	var _render_battle_img := func() -> Dictionary:
		seed(424242)
		var vp := SubViewport.new()
		vp.size = Vector2i(960, 540)
		vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
		get_root().add_child(vp)
		var bg := ColorRect.new()
		bg.size = Vector2(960, 540)
		bg.color = Color(0.06, 0.08, 0.12, 1.0)
		vp.add_child(bg)
		var mw := ClockGearsMessageWindow.new()
		vp.add_child(mw)
		var bs := ClockGearsBattleScene.new()
		vp.add_child(bs)
		bs.setup_battle(1, true, false, "", mw, true)
		bs.set_process(false)
		mw.show_message("\\>¡Slime te ataca!", "", "", 2, 0)
		await pump(3)
		var img := vp.get_texture().get_image()
		var nsrc := str(mw.get_meta("_cg_battle_notice_source")) if mw.has_meta("_cg_battle_notice_source") else ""
		vp.queue_free()
		await pump(1)
		return {"img": img, "notice_src": nsrc}

	dm.load_all("res://clockgears_data")
	var pre_msg_img: Image = await _render_msg_img.call("¡Hola, viajero!")
	var pre_bat_cap: Dictionary = await _render_battle_img.call()
	var pre_bat_img: Image = pre_bat_cap.get("img")

	# Escribir ui_designs.json legacy (schema 3, con los 9 elementos originales en messages_battle)
	var legacy_elems := [
		{"id": "msg_box_frame", "type": "frame", "name": "Cuadro de diálogo", "anchor": "bottom_center", "offset_x": 0, "offset_y": -16, "width": 880, "height": 156, "window_style_id": "dialog", "layer_order": 0},
		{"id": "msg_portrait", "type": "image", "parent_id": "msg_box_frame", "name": "Retrato hablante", "anchor": "top_left", "offset_x": 16, "offset_y": 32, "width": 96, "height": 96, "layer_order": 2},
		{"id": "msg_speaker_lbl", "type": "text", "parent_id": "msg_box_frame", "name": "Nombre hablante", "text": "Personaje", "anchor": "top_left", "offset_x": 128, "offset_y": 16, "width": 300, "height": 28, "font_size": 20, "layer_order": 2},
		{"id": "msg_body_lbl", "type": "text", "parent_id": "msg_box_frame", "name": "Texto mensaje", "text": "Texto de diálogo o aviso de batalla...", "anchor": "top_left", "offset_x": 128, "offset_y": 52, "width": 700, "height": 86, "font_size": 18, "layer_order": 2},
		{"id": "battle_cmd_frame", "type": "frame", "name": "Marco comandos batalla", "anchor": "top_right", "offset_x": -24, "offset_y": 24, "width": 220, "height": 212, "window_style_id": "game_menu", "layer_order": 0},
		{"id": "btn_battle_1", "type": "button", "parent_id": "battle_cmd_frame", "name": "Atacar", "text": "Atacar", "action": "close", "anchor": "top_left", "offset_x": 16, "offset_y": 16, "width": 188, "height": 38, "layer_order": 2},
		{"id": "btn_battle_2", "type": "button", "parent_id": "battle_cmd_frame", "name": "Habilidad", "text": "Habilidad", "action": "open_system_screen", "anchor": "top_left", "offset_x": 16, "offset_y": 62, "width": 188, "height": 38, "layer_order": 2},
		{"id": "btn_battle_3", "type": "button", "parent_id": "battle_cmd_frame", "name": "Objeto", "text": "Objeto", "action": "open_system_screen", "target_screen": "game_menu_items", "anchor": "top_left", "offset_x": 16, "offset_y": 108, "width": 188, "height": 38, "layer_order": 2},
		{"id": "btn_battle_4", "type": "button", "parent_id": "battle_cmd_frame", "name": "Huir", "text": "Huir", "action": "close", "anchor": "top_left", "offset_x": 16, "offset_y": 154, "width": 188, "height": 38, "layer_order": 2}
	]
	var legacy_uid := {
		"schema_version": 3,
		"screens": {
			"messages_battle": {
				"screen_id": "messages_battle",
				"system_group": "messages_battle",
				"name": "Mensajes y batalla",
				"custom_enabled": true,
				"elements": legacy_elems
			}
		}
	}
	var bak_mig := mig_dir.path_join("ui_designs.json.pre_messages_battle_split.bak")
	if FileAccess.file_exists(bak_mig):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(bak_mig))
	var legacy_raw_str := JSON.stringify(legacy_uid, "\t")
	var fw_leg := FileAccess.open(mig_dir.path_join("ui_designs.json"), FileAccess.WRITE)
	fw_leg.store_string(legacy_raw_str)
	fw_leg.close()

	dm.load_all(mig_dir)
	var mig_rep: Dictionary = ClockGearsDataManager.get_last_ui_migration_report()
	check(FileAccess.file_exists(bak_mig) and FileAccess.get_file_as_string(bak_mig) == legacy_raw_str,
		"Punto 4: Migration created exact backup .pre_messages_battle_split.bak before overwrite")
	var saved_uid: Variant = JSON.parse_string(FileAccess.get_file_as_string(mig_dir.path_join("ui_designs.json")))
	var saved_screens: Dictionary = saved_uid.get("screens", {}) if saved_uid is Dictionary else {}
	check(int(saved_uid.get("schema_version", 0)) == 4 and saved_screens.has("messages") and saved_screens.has("battle") and not saved_screens.has("messages_battle"),
		"Punto 4: Saved migrated ui_designs.json schema=4 with messages + battle and no messages_battle")
	var msg_mig_elems: Array = dm.get_ui_screen_elements("messages")
	var bat_mig_elems: Array = dm.get_ui_screen_elements("battle")
	var mig_msg_ids: Array = []
	var mig_bat_ids: Array = []
	for e in msg_mig_elems: mig_msg_ids.append(str(e.get("id", "")))
	for e in bat_mig_elems: mig_bat_ids.append(str(e.get("id", "")))
	check(mig_msg_ids.size() == 4 and mig_msg_ids.has("msg_box_frame") and mig_msg_ids.has("msg_portrait") and mig_msg_ids.has("msg_speaker_lbl") and mig_msg_ids.has("msg_body_lbl"),
		"Punto 4: Migration placed 4 message elements cleanly (%s)" % [str(mig_msg_ids)])
	check(mig_bat_ids.has("battle_cmd_frame") and mig_bat_ids.has("btn_battle_1") and mig_bat_ids.has("btn_battle_2") and mig_bat_ids.has("btn_battle_3") and mig_bat_ids.has("btn_battle_4") and mig_bat_ids.has("battle_notice_frame"),
		"Punto 4: Migration placed 5 battle elements + copied Aviso de batalla (%s)" % [str(mig_bat_ids)])
	print("--- MIGRATION REPORT START ---")
	for row in mig_rep.get("elements", []):
		print("  MIG: %s (%s) -> %s" % [str(row.get("id", "")), str(row.get("name", "")), str(row.get("to", ""))])
	print("--- MIGRATION REPORT END ---")

	# Comparación visual post-migración: mensaje y aviso de batalla en juego idénticos al anterior
	var post_msg_img: Image = await _render_msg_img.call("¡Hola, viajero!")
	var msg_diff := 0
	if pre_msg_img and post_msg_img and pre_msg_img.get_size() == post_msg_img.get_size():
		for yy in range(0, pre_msg_img.get_height(), 4):
			for xx in range(0, pre_msg_img.get_width(), 4):
				if pre_msg_img.get_pixel(xx, yy) != post_msg_img.get_pixel(xx, yy):
					msg_diff += 1
	check(pre_msg_img != null and post_msg_img != null and msg_diff == 0,
		"Punto 4: In-game message image matches pre-split baseline (diff=%d)" % msg_diff)

	var post_bat_cap: Dictionary = await _render_battle_img.call()
	var post_bat_img: Image = post_bat_cap.get("img")
	var bat_diff := 0
	if pre_bat_img and post_bat_img and pre_bat_img.get_size() == post_bat_img.get_size():
		for yy in range(0, pre_bat_img.get_height(), 4):
			for xx in range(0, pre_bat_img.get_width(), 4):
				if pre_bat_img.get_pixel(xx, yy) != post_bat_img.get_pixel(xx, yy):
					bat_diff += 1
	check(pre_bat_img != null and post_bat_img != null and bat_diff == 0,
		"Punto 4: In-game battle scene + notice image matches pre-split baseline (diff=%d)" % bat_diff)

	dm.load_all("res://clockgears_data")
	# Cleanup temp migration dir
	var da_cl := DirAccess.open(mig_dir)
	if da_cl:
		da_cl.list_dir_begin()
		var cfn := da_cl.get_next()
		while cfn != "":
			if not da_cl.current_is_dir():
				DirAccess.remove_absolute(ProjectSettings.globalize_path(mig_dir.path_join(cfn)))
			cfn = da_cl.get_next()
		da_cl.list_dir_end()
		DirAccess.remove_absolute(ProjectSettings.globalize_path(mig_dir))

	db.queue_free()
	await pump(2)
	if dm_owned and dm and is_instance_valid(dm):
		dm.free()
	print("Tiempo: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
