extends SceneTree

var pass_count := 0
var fail_count := 0
var frames_pumped := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] ", msg)
	else:
		fail_count += 1
		print("  [FAIL] ", msg)

func pump(n: int = 2) -> void:
	for i in range(n):
		frames_pumped += 1
		if frames_pumped > 580:
			break
		await process_frame

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	var t0 := Time.get_ticks_msec()
	var dm := Engine.get_singleton("ClockGearsDataManager")
	var dm_owned := false
	if dm == null:
		dm = ClassDB.instantiate("ClockGearsDataManager")
		dm_owned = true
	dm.load_all("res://clockgears_data")

	var db := ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	db.size = Vector2i(1366, 768)
	db.open_database()
	await pump(5)

	# --- PUNTO 1: Plantillas: X redonda, Esc, no cerrar al clic fuera, cabe en 1366x768 ---
	db.select_category_by_key("main_menu")
	db.adapt_to_window_size(Vector2i(1366, 768))
	await pump(4)
	var h1: Dictionary = db.call("get_ui_designer_ui_handles")
	var tbtn: Button = h1.get("templates_btn")
	var gal: AcceptDialog = h1.get("template_gallery_dialog")
	var xbtn: Button = h1.get("tpl_close_x_btn")
	var tabs: TabContainer = h1.get("template_tabs")
	var sgrid: GridContainer = h1.get("tpl_screen_grid")
	var confirm_dlg: ConfirmationDialog = h1.get("template_apply_confirm_dialog")
	var before_elems: Array = dm.get_ui_screen_elements("title")

	check(tbtn != null and gal != null and xbtn != null and tabs != null,
		"Punto 1: Template gallery, round X button and tabs present")
	if tbtn and gal and xbtn and tabs:
		check(tabs.get_tab_count() == 3, "Punto 1: Has 3 tabs (Pantallas completas, Componentes, Mis plantillas)")
		check(xbtn.theme_type_variation == &"ClockGearsCircleCloseButton",
			"Punto 1: X button uses ClockGearsCircleCloseButton theme variation")
		check(not gal.get_flag(Window.FLAG_POPUP),
			"Punto 1: Outside click does not close dialog (FLAG_POPUP=false)")
		tbtn.emit_signal("pressed")
		await pump(3)
		check(gal.visible, "Punto 1: Template gallery opens")
		check(gal.size.x <= 1366 and gal.size.y <= 768 - 32,
			"Punto 1 (1366x768): Gallery fits inside screen with margin (%s)" % str(gal.size))
		if tabs.get_tab_count() > 0:
			var sc0 := tabs.get_tab_control(0) as ScrollContainer
			check(sc0 != null and sc0.vertical_scroll_mode == ScrollContainer.SCROLL_MODE_AUTO and sc0.horizontal_scroll_mode == ScrollContainer.SCROLL_MODE_DISABLED,
				"Punto 1: Tab content uses vertical ScrollContainer")
		if sgrid and sgrid.get_child_count() > 0:
			var card0 := sgrid.get_child(0)
			if card0.get_child_count() > 0 and card0.get_child(0).get_child_count() > 1:
				var sb0 := card0.get_child(0).get_child(1) as Button
				check(sb0 != null and sb0.clip_text and sb0.text_overrun_behavior == TextServer.OVERRUN_TRIM_ELLIPSIS and not sb0.tooltip_text.is_empty(),
					"Punto 1: Long template names clip with ellipsis and full tooltip")
		# Close via X button without applying
		xbtn.emit_signal("pressed")
		await pump(2)
		check(not gal.visible, "Punto 1: Clicking round X button closes gallery")
		var after_x_elems: Array = dm.get_ui_screen_elements("title")
		check(JSON.stringify(before_elems) == JSON.stringify(after_x_elems),
			"Punto 1: Closing via X applies nothing")

		# Close via Esc without applying
		tbtn.emit_signal("pressed")
		await pump(3)
		var esc := InputEventKey.new()
		esc.keycode = KEY_ESCAPE
		esc.pressed = true
		gal.emit_signal("window_input", esc)
		await pump(2)
		check(not gal.visible, "Punto 1: Esc closes gallery without applying")

		# Confirm dialog still triggers when applying
		tbtn.emit_signal("pressed")
		await pump(3)
		var abtn: Button = h1.get("tpl_apply_btn")
		if abtn:
			abtn.emit_signal("pressed")
			await pump(2)
			check(confirm_dlg != null and confirm_dlg.visible, "Punto 1: Applying template asks for confirmation")
			if confirm_dlg and confirm_dlg.visible:
				confirm_dlg.hide()
		xbtn.emit_signal("pressed")
		await pump(2)

	# --- PUNTO 2: Sub-pantallas, Ver contexto, Escena de ejemplo, Rects reales y Migración v5 ---
	var _get_panel_rect_by_elem := func(p_sid: String, p_elem_id: String) -> Rect2:
		var d: Dictionary = dm.get_default_ui_screen_template(p_sid)
		for el in d.get("elements", []):
			if str(el.get("id", "")) == p_elem_id:
				return Rect2(float(el.get("offset_x", 0)), float(el.get("offset_y", 0)), float(el.get("width", 0)), float(el.get("height", 0)))
		return Rect2()

	# 2a. Desplegable Pantalla con iconos e indentación + Ver contexto + Escena de ejemplo
	var expected_msg_subs := ["messages", "messages_choices", "messages_number", "messages_history", "messages_gold"]
	var expected_bat_subs := ["battle", "battle_party", "battle_enemies", "battle_notice", "battle_submenu", "battle_target"]

	db.select_category_by_key("messages")
	await pump(4)
	var hm: Dictionary = db.call("get_ui_designer_ui_handles")
	var sopt: OptionButton = hm.get("screen_opt")
	var vctx_chk: CheckBox = hm.get("view_context_chk")
	var sctx_chk: CheckBox = hm.get("sample_scene_chk")
	var ctx_root: Control = hm.get("context_layers_root")
	var bbd: Control = hm.get("ctx_battle_backdrop_node")
	var ed_panel: Node = hm.get("panel")

	var msg_opt_ids: Array = []
	var msg_icons_ok := true
	var msg_indent_ok := true
	if sopt:
		for i in range(sopt.get_item_count()):
			var sid_i := str(sopt.get_item_metadata(i))
			msg_opt_ids.append(sid_i)
			if sopt.get_item_icon(i) == null:
				msg_icons_ok = false
			if sid_i != "messages" and not sopt.get_item_text(i).contains("↳"):
				msg_indent_ok = false
	check(msg_opt_ids == expected_msg_subs and msg_icons_ok and msg_indent_ok,
		"Punto 2 (Mensajes): Dropdown lists all 5 subscreens indented with icons (%s)" % [str(msg_opt_ids)])

	if sopt and sopt.get_item_count() >= 2:
		sopt.select(1)
		sopt.emit_signal("item_selected", 1)
		await pump(3)
		var dl: Control = ed_panel.call("get_ui_handles").get("design_layer") if ed_panel else null
		check(dl != null and str(dl.call("get_screen_id")) == "messages_choices",
			"Punto 2 (Mensajes): Selecting Ventana de elecciones opens only messages_choices in design_layer")
		# Abrir desde cualquiera (messages_choices) debe mostrar Ver contexto al 35% con 4 subpantallas hermanas no seleccionables
		check(vctx_chk != null and vctx_chk.visible and vctx_chk.button_pressed,
			"Punto 2 (Mensajes): Ver contexto visible and enabled by default")
		check(ctx_root != null and ctx_root.visible and absf(ctx_root.modulate.a - 0.35) < 0.02 and ctx_root.mouse_filter == Control.MOUSE_FILTER_IGNORE and ctx_root.get_child_count() == 4,
			"Punto 2 (Mensajes): Context draws 4 sibling subscreens at 35%% opacity and MOUSE_FILTER_IGNORE (count=%d)" % [ctx_root.get_child_count() if ctx_root else -1])
		# Verificar Probar desde sub-pantalla hija abre ventana con TODAS las subpantallas juntas
		var pbtn_m: Button = hm.get("test_btn")
		if pbtn_m:
			pbtn_m.emit_signal("pressed")
			await pump(2)
			var twin_m := ed_panel.get_node_or_null("UiDesignTestPopup") as Window if ed_panel else null
			var m_comp := twin_m.find_child("TestUiIsolatedLayer", true, false) if twin_m else null
			var m_sub_cnt := m_comp.get_child_count() if m_comp else 0
			if twin_m:
				twin_m.emit_signal("close_requested")
				await pump(2)
			check(m_sub_cnt == 5, "Punto 2 (Mensajes): Probar from child subscreen opens full 5-subscreen preview (cnt=%d)" % m_sub_cnt)

		vctx_chk.button_pressed = false
		await pump(2)
		check(not ctx_root.visible, "Punto 2: Unchecking Ver contexto hides sibling subscreens")
		vctx_chk.button_pressed = true
		await pump(2)

	# Tab Batalla dropdown + Ver contexto + Escena de ejemplo
	db.select_category_by_key("battle")
	await pump(4)
	var hb: Dictionary = db.call("get_ui_designer_ui_handles")
	var bat_opt_ids: Array = []
	var bat_icons_ok := true
	var bat_indent_ok := true
	for i in range(sopt.get_item_count()):
		var bsid := str(sopt.get_item_metadata(i))
		bat_opt_ids.append(bsid)
		if sopt.get_item_icon(i) == null:
			bat_icons_ok = false
		if bsid != "battle" and not sopt.get_item_text(i).contains("↳"):
			bat_indent_ok = false
	check(bat_opt_ids == expected_bat_subs and bat_icons_ok and bat_indent_ok,
		"Punto 2 (Batalla): Dropdown lists all 6 subscreens indented with icons (%s)" % [str(bat_opt_ids)])
	check(sctx_chk != null and sctx_chk.visible and sctx_chk.button_pressed and bbd != null and bbd.visible and absf(bbd.modulate.a - 0.35) < 0.02 and bbd.mouse_filter == Control.MOUSE_FILTER_IGNORE,
		"Punto 2 (Batalla): Escena de ejemplo visible at 35%% non-editable backdrop")
	sctx_chk.button_pressed = false
	await pump(2)
	check(not bbd.visible, "Punto 2 (Batalla): Unchecking Escena de ejemplo hides backdrop")
	sctx_chk.button_pressed = true
	await pump(2)

	# Probar desde subpantalla battle_party abre todas las subpantallas de batalla
	sopt.select(1)
	sopt.emit_signal("item_selected", 1)
	await pump(3)
	var pbtn_b: Button = hb.get("test_btn")
	if pbtn_b:
		pbtn_b.emit_signal("pressed")
		await pump(2)
		var twin_b := ed_panel.get_node_or_null("UiDesignTestPopup") as Window if ed_panel else null
		var b_comp := twin_b.find_child("TestUiIsolatedLayer", true, false) if twin_b else null
		var b_sub_cnt := b_comp.get_child_count() if b_comp else 0
		if twin_b:
			twin_b.emit_signal("close_requested")
			await pump(2)
		check(b_sub_cnt == 6, "Punto 2 (Batalla): Probar from child subscreen opens full 6-subscreen preview (cnt=%d)" % b_sub_cnt)

	# 2b. Comparación rectángulos reales vs diseño por defecto a 960x540 (tolerancia 2 px)
	var vp_real := SubViewport.new()
	vp_real.size = Vector2i(960, 540)
	vp_real.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(vp_real)

	var mw_r := ClockGearsMessageWindow.new()
	vp_real.add_child(mw_r)
	mw_r.show_message("\\>Hola", "Personaje", "", 0, 0)
	mw_r.show_choices(["A", "B"], 0, 0)
	mw_r.show_number_input(1, 3)
	mw_r.open_history()
	await pump(4)
	# En ClockGearsMessageWindow constructor se añaden PanelContainers en orden fijo:
	# [0]=main_panel, [1]=name_panel, [2]=gold_panel, [3]=choices_panel, [4]=number_panel, [5]=history_panel
	var mw_panels: Array = []
	for ch in mw_r.get_children():
		if ch is PanelContainer:
			mw_panels.append(ch)
	var _rect_close := func(a: Rect2, b: Rect2) -> bool:
		return absf(a.position.x - b.position.x) <= 2.0 and absf(a.position.y - b.position.y) <= 2.0 and absf(a.size.x - b.size.x) <= 2.0 and absf(a.size.y - b.size.y) <= 2.0
	if mw_panels.size() >= 6:
		var r_main := (mw_panels[0] as Control).get_rect()
		var r_gold := (mw_panels[2] as Control).get_rect()
		var r_choices := (mw_panels[3] as Control).get_rect()
		var r_num := (mw_panels[4] as Control).get_rect()
		var r_hist := (mw_panels[5] as Control).get_rect()
		check(_rect_close.call(r_main, _get_panel_rect_by_elem.call("messages", "msg_box_frame")),
			"Punto 2 Real Match (960x540): msg_box_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("messages", "msg_box_frame")), str(r_main)])
		check(_rect_close.call(r_choices, _get_panel_rect_by_elem.call("messages_choices", "msg_choices_frame")),
			"Punto 2 Real Match (960x540): msg_choices_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("messages_choices", "msg_choices_frame")), str(r_choices)])
		check(_rect_close.call(r_num, _get_panel_rect_by_elem.call("messages_number", "msg_number_frame")),
			"Punto 2 Real Match (960x540): msg_number_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("messages_number", "msg_number_frame")), str(r_num)])
		check(_rect_close.call(r_hist, _get_panel_rect_by_elem.call("messages_history", "msg_history_frame")),
			"Punto 2 Real Match (960x540): msg_history_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("messages_history", "msg_history_frame")), str(r_hist)])
		check(_rect_close.call(r_gold, _get_panel_rect_by_elem.call("messages_gold", "msg_gold_frame")),
			"Punto 2 Real Match (960x540): msg_gold_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("messages_gold", "msg_gold_frame")), str(r_gold)])

	mw_r.queue_free()
	var bs_r := ClockGearsBattleScene.new()
	vp_real.add_child(bs_r)
	bs_r.setup_battle(1, true, false, "", null, true)
	bs_r.set_process(false)
	await pump(4)
	# Encontrar party_container y actions_panel dentro del árbol de batalla real
	var r_party := Rect2()
	var r_cmds := Rect2()
	var find_q: Array = [bs_r]
	while not find_q.is_empty():
		var cur: Node = find_q.pop_front()
		for c in cur.get_children():
			find_q.push_back(c)
			if c is HBoxContainer and c.get_parent() is HBoxContainer and c.get_parent().get_child_count() == 2:
				r_party = (c as Control).get_global_rect()
			elif c is PanelContainer and c.get_parent() is HBoxContainer and c.get_parent().get_child_count() == 2:
				r_cmds = (c as Control).get_global_rect()
	# Abrir submenú de habilidades para verificar sub_menu_panel
	var skill_btn := bs_r.find_child("*Habilidad*", true, false) as Button
	if skill_btn == null:
		for n in bs_r.find_children("*", "Button", true, false):
			if (n as Button).text == "Habilidad":
				skill_btn = n as Button
				break
	if skill_btn:
		skill_btn.emit_signal("pressed")
		await pump(2)
	var r_sub := Rect2()
	for ch in bs_r.get_children():
		if ch is PanelContainer and (ch as Control).visible and (ch as Control).custom_minimum_size == Vector2(220, 160):
			r_sub = (ch as Control).get_global_rect()
	check(_rect_close.call(r_cmds, _get_panel_rect_by_elem.call("battle", "battle_cmd_frame")),
		"Punto 2 Real Match (960x540): battle_cmd_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("battle", "battle_cmd_frame")), str(r_cmds)])
	check(_rect_close.call(r_party, _get_panel_rect_by_elem.call("battle_party", "battle_party_frame")),
		"Punto 2 Real Match (960x540): battle_party_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("battle_party", "battle_party_frame")), str(r_party)])
	check(_rect_close.call(r_sub, _get_panel_rect_by_elem.call("battle_submenu", "battle_submenu_frame")),
		"Punto 2 Real Match (960x540): battle_submenu_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("battle_submenu", "battle_submenu_frame")), str(r_sub)])
	check(_rect_close.call(r_sub, _get_panel_rect_by_elem.call("battle_target", "battle_target_frame")),
		"Punto 2 Real Match (960x540): battle_target_frame %s vs real %s" % [str(_get_panel_rect_by_elem.call("battle_target", "battle_target_frame")), str(r_sub)])

	# 2c. Capturar render pre-migración v4 -> v5 + migración sin pérdidas en carpeta aislada
	var mig_v5 := "user://test_subscreens_v5_tmp"
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(mig_v5))
	var da_v5 := DirAccess.open("res://clockgears_data")
	if da_v5:
		da_v5.list_dir_begin()
		var f5 := da_v5.get_next()
		while f5 != "":
			if not da_v5.current_is_dir() and f5.ends_with(".json"):
				var b5 := FileAccess.get_file_as_bytes("res://clockgears_data".path_join(f5))
				var fw5 := FileAccess.open(mig_v5.path_join(f5), FileAccess.WRITE)
				fw5.store_buffer(b5)
				fw5.close()
			f5 = da_v5.get_next()
		da_v5.list_dir_end()

	# Capturar baselines de juego con proyecto actual antes de migrar v4
	dm.load_all("res://clockgears_data")
	var vp_cmp := SubViewport.new()
	vp_cmp.size = Vector2i(960, 540)
	vp_cmp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(vp_cmp)
	var bgc := ColorRect.new()
	bgc.size = Vector2(960, 540)
	bgc.color = Color(0.08, 0.1, 0.14, 1.0)
	vp_cmp.add_child(bgc)
	var mwc := ClockGearsMessageWindow.new()
	vp_cmp.add_child(mwc)
	mwc.show_message("\\>Prueba mensaje", "Personaje", "", 2, 0)
	await pump(3)
	var pre_m_img := vp_cmp.get_texture().get_image()
	mwc.queue_free()
	var bsc := ClockGearsBattleScene.new()
	vp_cmp.add_child(bsc)
	bsc.setup_battle(1, true, false, "", null, true)
	bsc.set_process(false)
	await pump(3)
	var pre_b_img := vp_cmp.get_texture().get_image()
	bsc.queue_free()
	await pump(1)

	# Escribir v4 combinada en mig_v5/ui_designs.json
	var v4_uid := {
		"schema_version": 4,
		"screens": {
			"messages": {
				"screen_id": "messages", "custom_enabled": true,
				"elements": [
					{"id": "msg_box_frame", "type": "frame", "name": "Cuadro de diálogo", "offset_x": 38, "offset_y": 373, "width": 883, "height": 151},
					{"id": "msg_portrait", "type": "image", "parent_id": "msg_box_frame", "name": "Retrato hablante", "offset_x": 16, "offset_y": 31, "width": 88, "height": 88},
					{"id": "msg_choices_frame", "type": "frame", "name": "Ventana de elecciones", "offset_x": 595, "offset_y": 149, "width": 326, "height": 216},
					{"id": "msg_number_frame", "type": "frame", "name": "Entrada de número", "offset_x": 340, "offset_y": 235, "width": 280, "height": 70},
					{"id": "msg_history_frame", "type": "frame", "name": "Historial de mensajes", "offset_x": 38, "offset_y": 20, "width": 883, "height": 500}
				]
			},
			"battle": {
				"screen_id": "battle", "custom_enabled": true,
				"elements": [
					{"id": "battle_cmd_frame", "type": "frame", "name": "Marco comandos batalla", "offset_x": 776, "offset_y": 384, "width": 160, "height": 140},
					{"id": "btn_battle_1", "type": "button", "parent_id": "battle_cmd_frame", "name": "Atacar", "text": "Atacar", "offset_x": 8, "offset_y": 6, "width": 144, "height": 22},
					{"id": "battle_party_frame", "type": "frame", "name": "Panel del grupo", "offset_x": 16, "offset_y": 384, "width": 744, "height": 140},
					{"id": "battle_enemies_frame", "type": "frame", "name": "Panel de enemigos", "offset_x": 16, "offset_y": 12, "width": 920, "height": 360},
					{"id": "battle_notice_frame", "type": "frame", "name": "Aviso de batalla", "offset_x": 38, "offset_y": 373, "width": 883, "height": 151}
				]
			}
		}
	}
	var bak_v5 := mig_v5.path_join("ui_designs.json.pre_subscreens_v5.bak")
	if FileAccess.file_exists(bak_v5):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(bak_v5))
	var v4_raw := JSON.stringify(v4_uid, "\t")
	var fw4 := FileAccess.open(mig_v5.path_join("ui_designs.json"), FileAccess.WRITE)
	fw4.store_string(v4_raw)
	fw4.close()

	dm.load_all(mig_v5)
	var rep5: Dictionary = ClockGearsDataManager.get_last_ui_migration_report()
	check(FileAccess.file_exists(bak_v5) and FileAccess.get_file_as_string(bak_v5) == v4_raw,
		"Punto 2 Migration: Exact .pre_subscreens_v5.bak created before overwrite")
	var saved5: Variant = JSON.parse_string(FileAccess.get_file_as_string(mig_v5.path_join("ui_designs.json")))
	check(saved5 is Dictionary and int(saved5.get("schema_version", 0)) == 5,
		"Punto 2 Migration: ui_designs.json upgraded to schema_version=5")
	check(dm.get_ui_screen_elements("messages").size() == 2 and dm.get_ui_screen_elements("messages_choices").size() == 1 and dm.get_ui_screen_elements("messages_number").size() == 1 and dm.get_ui_screen_elements("messages_history").size() == 1,
		"Punto 2 Migration: Messages elements partitioned across subscreens without loss")
	check(dm.get_ui_screen_elements("battle").size() == 2 and dm.get_ui_screen_elements("battle_party").size() == 1 and dm.get_ui_screen_elements("battle_enemies").size() == 1 and dm.get_ui_screen_elements("battle_notice").size() == 1,
		"Punto 2 Migration: Battle elements partitioned across subscreens without loss")
	print("--- MIGRATION V5 START ---")
	for r in rep5.get("elements", []):
		print("  MIG5: %s (%s) : %s -> %s" % [str(r.get("id", "")), str(r.get("name", "")), str(r.get("from", "")), str(r.get("to", ""))])
	print("--- MIGRATION V5 END ---")

	# Comparación visual post-migración v5 (si tenemos OpenGL real)
	var mwc2 := ClockGearsMessageWindow.new()
	vp_cmp.add_child(mwc2)
	mwc2.show_message("\\>Prueba mensaje", "Personaje", "", 2, 0)
	await pump(3)
	var post_m_img := vp_cmp.get_texture().get_image()
	mwc2.queue_free()
	var bsc2 := ClockGearsBattleScene.new()
	vp_cmp.add_child(bsc2)
	bsc2.setup_battle(1, true, false, "", null, true)
	bsc2.set_process(false)
	await pump(3)
	var post_b_img := vp_cmp.get_texture().get_image()
	vp_cmp.queue_free()
	vp_real.queue_free()
	await pump(1)
	var dm_diff := 0
	if pre_m_img and post_m_img and pre_m_img.get_size() == post_m_img.get_size():
		for yy in range(0, pre_m_img.get_height(), 4):
			for xx in range(0, pre_m_img.get_width(), 4):
				if pre_m_img.get_pixel(xx, yy) != post_m_img.get_pixel(xx, yy):
					dm_diff += 1
	var db_diff := 0
	if pre_b_img and post_b_img and pre_b_img.get_size() == post_b_img.get_size():
		for yy in range(0, pre_b_img.get_height(), 4):
			for xx in range(0, pre_b_img.get_width(), 4):
				if pre_b_img.get_pixel(xx, yy) != post_b_img.get_pixel(xx, yy):
					db_diff += 1
	check(pre_m_img != null and post_m_img != null and dm_diff == 0 and pre_b_img != null and post_b_img != null and db_diff == 0,
		"Punto 2 Game Look: In-game message (diff=%d) & battle (diff=%d) match 100%%" % [dm_diff, db_diff])

	dm.load_all("res://clockgears_data")
	var da_cl5 := DirAccess.open(mig_v5)
	if da_cl5:
		da_cl5.list_dir_begin()
		var c5 := da_cl5.get_next()
		while c5 != "":
			if not da_cl5.current_is_dir():
				DirAccess.remove_absolute(ProjectSettings.globalize_path(mig_v5.path_join(c5)))
			c5 = da_cl5.get_next()
		da_cl5.list_dir_end()
		DirAccess.remove_absolute(ProjectSettings.globalize_path(mig_v5))

	db.queue_free()
	await pump(2)
	if dm_owned and dm and is_instance_valid(dm):
		dm.free()
	print("Tiempo: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
