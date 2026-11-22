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

	db.queue_free()
	await pump(2)
	if dm_owned and dm and is_instance_valid(dm):
		dm.free()
	print("Tiempo: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
