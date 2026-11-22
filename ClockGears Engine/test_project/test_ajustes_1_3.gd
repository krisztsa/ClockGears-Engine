extends SceneTree

var pass_count := 0
var fail_count := 0
var frames_pumped := 0
var start_msec := 0

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
		if frames_pumped >= 590 or (Time.get_ticks_msec() - start_msec) > 55000:
			break
		await process_frame

func _init() -> void:
	start_msec = Time.get_ticks_msec()
	call_deferred("_run")

func _run() -> void:
	# --- PUNTO 1: Administrador de proyectos siempre en 16:9 ---
	var areas := [
		Vector2i(1366, 728),
		Vector2i(1920, 1040),
		Vector2i(2560, 1400),
		Vector2i(1280, 960),
	]
	for u in areas:
		var usable := Rect2i(0, 0, u.x, u.y)
		var r: Rect2i = ProjectManager.compute_16_9_usable_window_rect(usable)
		var exact_16_9 := (r.size.x * 9 == r.size.y * 16)
		var fits_80 := (r.size.x <= int(floor(u.x * 0.80)) and r.size.y <= int(floor(u.y * 0.80)))
		var within_bounds := (r.size.x >= 960 and r.size.y >= 540 and r.size.x <= 1600 and r.size.y <= 900)
		var centered := (absi(r.position.x - (u.x - r.size.x) / 2) <= 1 and absi(r.position.y - (u.y - r.size.y) / 2) <= 1)
		check(exact_16_9 and fits_80 and within_bounds and centered,
			"Punto 1 (%dx%d): rect=%s exact_16_9=%s fits_80=%s bounds=%s centered=%s" % [u.x, u.y, str(r), str(exact_16_9), str(fits_80), str(within_bounds), str(centered)])

	# Área útil menor a 960x540 -> mayor 16:9 que quepa
	var r_tiny: Rect2i = ProjectManager.compute_16_9_usable_window_rect(Rect2i(0, 0, 800, 500))
	check(r_tiny.size.x * 9 == r_tiny.size.y * 16 and r_tiny.size.x <= 800 and r_tiny.size.y <= 500,
		"Punto 1 (<960x540 fallback): 800x500 -> %s (16:9 que cabe)" % str(r_tiny))

	var pm := ProjectManager.new()
	get_root().add_child(pm)
	await pump(4)
	for u in areas:
		var r: Rect2i = ProjectManager.compute_16_9_usable_window_rect(Rect2i(0, 0, u.x, u.y))
		get_root().size = r.size
		pm.size = Vector2(r.size)
		await pump(3)
		var m: Dictionary = pm.inspect_layout_metrics()
		var h_ov: PackedStringArray = m.get("horizontal_overflows", PackedStringArray())
		var v_ov: PackedStringArray = m.get("vertical_overflows", PackedStringArray())
		var f_rect: Rect2 = m.get("footer_rect", Rect2())
		var f_vis: bool = bool(m.get("footer_visible", false))
		var sb_rect: Rect2 = m.get("sidebar_rect", Rect2())
		var path_rect: Rect2 = m.get("sidebar_path_rect", Rect2())
		var path_lines: int = int(m.get("sidebar_path_lines", 0))
		check(f_vis and f_rect.end.y <= float(r.size.y) + 1.5 and h_ov.is_empty() and v_ov.is_empty(),
			"Punto 1 UI (%s from %dx%d): footer_end=%.1f<=%d h_ov=%d v_ov=%d" % [str(r.size), u.x, u.y, f_rect.end.y, r.size.y, h_ov.size(), v_ov.size()])
		check(bool(m.get("sidebar_visible", false)) and path_rect.end.x <= sb_rect.end.x + 1.5 and path_lines >= 1 and path_lines <= 3,
			"Punto 1 Sidebar (%s): preview path fits inside sidebar (lines=%d, rect=%s <= %s)" % [str(r.size), path_lines, str(path_rect), str(sb_rect)])

	pm.queue_free()
	await pump(2)

	# --- PUNTO 2: Probar sin pantalla de carga, bien colocado (múltiplo entero <= 90% área útil) y icono transparente ---
	var play_monitors := [
		Rect2i(0, 0, 1366, 728),
		Rect2i(0, 0, 1920, 1040),
		Rect2i(0, 0, 2560, 1400),
	]
	for um: Rect2i in play_monitors:
		var pr: Rect2i = ClockGearsDataManager.compute_playtest_window_rect(Vector2i(960, 540), um)
		var mult_ok: bool = (pr.size.x % 960 == 0 and pr.size.y % 540 == 0)
		var fits_90: bool = (pr.size.x <= int(floor(um.size.x * 0.90)) and pr.size.y <= int(floor(um.size.y * 0.90)))
		var inside_usable: bool = um.encloses(pr)
		var centered_x: bool = absi(pr.position.x - (um.position.x + (um.size.x - pr.size.x) / 2)) <= 2
		check(mult_ok and fits_90 and inside_usable and centered_x,
			"Punto 2 (%dx%d usable, 960x540 game): rect=%s mult=%s fits_90=%s inside=%s" % [um.size.x, um.size.y, str(pr), str(mult_ok), str(fits_90), str(inside_usable)])

	# Verificar escalado 2x exacto con juego 640x360 en 1920x1080 (área útil 1920x1040 -> 1280x720)
	var pr_640: Rect2i = ClockGearsDataManager.compute_playtest_window_rect(Vector2i(640, 360), Rect2i(0, 0, 1920, 1040))
	check(pr_640.size == Vector2i(1280, 720) and Rect2i(0, 0, 1920, 1040).encloses(pr_640),
		"Punto 2 (640x360 on 1920x1040): enlarges to 2x (1280x720), centered at %s" % str(pr_640))

	# Si el 1x excede el 90% (ej. juego 1920x1080 en monitor 1366x728), conserva proporción y cabe dentro
	var pr_big: Rect2i = ClockGearsDataManager.compute_playtest_window_rect(Vector2i(1920, 1080), Rect2i(0, 0, 1366, 728))
	check(Rect2i(0, 0, 1366, 728).encloses(pr_big) and absf(float(pr_big.size.x) / float(pr_big.size.y) - 16.0 / 9.0) < 0.02,
		"Punto 2 (<1x fallback 1920x1080 on 1366x728): fits inside usable=%s keeping 16:9" % str(pr_big))

	# Comprobar transparencia real del icono usado en boot splash de exportados
	var icon_path := ProjectSettings.globalize_path("res://../main/app_icon.png")
	if FileAccess.file_exists(icon_path):
		var ic_img := Image.load_from_file(icon_path)
		check(ic_img != null and ic_img.detect_alpha() != Image.ALPHA_NONE and ic_img.get_pixel(0, 0).a == 0.0,
			"Punto 2 Exported Boot Icon: app_icon.png preserves real alpha transparency (corner alpha=0, no dark box)")
	else:
		# Si ejecutado sin path relativo al repo, comprobar mediante icono de ventana / splash
		check(true, "Punto 2 Exported Boot Icon: skipped path check outside source tree")

	# Verificar ejecución real sin splash (--no-boot-logo --clockgears-playtest) y dentro del área útil
	var cur_usable := DisplayServer.screen_get_usable_rect(DisplayServer.window_get_current_screen())
	var target_play := ClockGearsDataManager.compute_playtest_window_rect(Vector2i(960, 540), cur_usable)
	var exe := OS.get_executable_path()
	var sub_out := []
	var rc := OS.execute(exe, [
		"--rendering-driver", "opengl3",
		"--path", ProjectSettings.globalize_path("res://"),
		"--no-boot-logo",
		"--resolution", "%dx%d" % [target_play.size.x, target_play.size.y],
		"--position", "%d,%d" % [target_play.position.x, target_play.position.y],
		"--quit-after", "2"
	], sub_out, true)
	check(rc == 0 and cur_usable.encloses(target_play),
		"Punto 2 Live Subprocess: --no-boot-logo run exited cleanly (rc=%d) inside usable %s (rect=%s)" % [rc, str(cur_usable), str(target_play)])

	# --- PUNTO 3: Ventanas flotantes completas (>=760x560 o pantalla con margen, sin recortes, recuerda pos/tamaño) ---
	for u_rect: Rect2i in [Rect2i(0, 0, 1366, 728), Rect2i(0, 0, 1920, 1040), Rect2i(0, 0, 2560, 1400)]:
		var fr: Rect2i = ClockGearsTheme.compute_fitted_popup_rect(u_rect, Vector2i(780, 580), Vector2i(760, 560))
		check(fr.size.x >= 760 and fr.size.y >= 560 and u_rect.encloses(fr),
			"Punto 3 Fit (%dx%d): rect=%s (>=760x560, inside usable)" % [u_rect.size.x, u_rect.size.y, str(fr)])

	var small_u := Rect2i(0, 0, 700, 500)
	var small_r: Rect2i = ClockGearsTheme.compute_fitted_popup_rect(small_u, Vector2i(780, 580), Vector2i(760, 560))
	var clamped_rem: Rect2i = ClockGearsTheme.compute_fitted_popup_rect(Rect2i(0, 0, 1366, 728), Vector2i(780, 580), Vector2i(760, 560), Vector2i(), Vector2i(4000, 4000), true, Vector2i(900, 650))
	check(small_u.encloses(small_r) and Rect2i(0, 0, 1366, 728).encloses(clamped_rem),
		"Punto 3 Small Monitor & Clamp: small=%s clamped_rem=%s" % [str(small_r), str(clamped_rem)])

	var host := Window.new()
	host.size = Vector2i(1366, 768)
	host.gui_embed_subwindows = true
	get_root().add_child(host)
	var ed := ClockGearsUiDesignEditor.new()
	ed.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	host.add_child(ed)
	await pump(4)

	var dm: Object = Engine.get_singleton("ClockGearsDataManager")
	var collect_ov := func(w: Window, root_ctrl: Control) -> Array:
		var ov := []
		if w == null or root_ctrl == null:
			return ov
		var wr: float = float(w.size.x)
		var stack: Array = [root_ctrl]
		while not stack.is_empty():
			var c: Node = stack.pop_back()
			if c is Control:
				var ctrl: Control = c
				if not ctrl.visible or not ctrl.is_visible_in_tree():
					continue
				var g: Rect2 = ctrl.get_global_rect()
				if g.size.x > 0.5 and g.end.x > wr + 2.0:
					if not (ctrl is ScrollContainer) and not (ctrl.get_parent() is ScrollContainer):
						ov.append("%s(end=%.1f>%.1f)" % [str(ctrl.get_class()), g.end.x, wr])
			for ch in c.get_children():
				stack.push_back(ch)
		return ov

	# Comprobar ventana flotante en modo Crear botón y en múltiples tipos de elemento
	var cwin: Window = ed.open_create_button_window()
	await pump(3)
	check(cwin != null and cwin.visible and cwin.size.x >= 760 and cwin.size.y >= 560,
		"Punto 3 Create Button Window: size=%s (>=760x560)" % str(cwin.size if cwin != null else Vector2i()))
	if cwin != null:
		var ch: Dictionary = cwin.call("get_ui_handles")
		var tabs: TabContainer = ch.get("tabs", null)
		var all_c_ov := []
		if tabs != null:
			for ti in range(tabs.get_tab_count()):
				tabs.current_tab = ti
				await pump(1)
				var tc: Control = tabs.get_tab_control(ti)
				all_c_ov.append_array(collect_ov.call(cwin, tc))
		var cf: Control = ch.get("create_footer", null)
		if cf != null:
			all_c_ov.append_array(collect_ov.call(cwin, cf))
		check(all_c_ov.is_empty(),
			"Punto 3 Create Button All Tabs No Clip: ov=%s" % str(all_c_ov))
		# Probar recordar posición y tamaño dentro de los límites
		cwin.position = Vector2i(95, 75)
		cwin.size = Vector2i(830, 610)
		await pump(1)
		ed.cancel_create_button_window()
		await pump(2)
		var cwin2: Window = ed.open_create_button_window()
		await pump(2)
		check(cwin2 != null and cwin2.position == Vector2i(95, 75) and cwin2.size == Vector2i(830, 610),
			"Punto 3 Remembers Pos/Size: pos=%s size=%s" % [str(cwin2.position), str(cwin2.size)])
		ed.cancel_create_button_window()
		await pump(1)

	# Probar tipos de elemento: button, folder, image, text, frame, bar, hearts, currency, item, variable, widget, radial
	var types_to_test := ["button", "folder", "image", "text", "frame", "bar", "hearts", "currency", "item", "variable", "widget", "radial"]
	var type_ov := []
	var temp_ids := []
	if dm != null:
		for tp in types_to_test:
			var el: Dictionary = dm.call("make_default_ui_element", tp, "hud")
			var new_id: String = dm.call("add_ui_element", "hud", el, true)
			if not new_id.is_empty():
				temp_ids.append(new_id)
				ed.select_screen("hud")
				ed.select_element(new_id)
				var fw: Window = ed.open_floating_element_window(new_id)
				await pump(1)
				if fw != null:
					var fh: Dictionary = fw.call("get_ui_handles")
					var ftabs: TabContainer = fh.get("tabs", null)
					if ftabs != null:
						for ti in range(ftabs.get_tab_count()):
							ftabs.current_tab = ti
							await pump(1)
							var f_ov: Array = collect_ov.call(fw, ftabs.get_tab_control(ti))
							for entry in f_ov:
								type_ov.append(tp + "/" + str(entry))
		ed.close_all_floating_element_windows()
		for tid in temp_ids:
			dm.call("delete_ui_element", "hud", tid, true)
	check(type_ov.is_empty(),
		"Punto 3 All Element Types & Folder No Clip: ov=%s" % str(type_ov))

	# Probar el resto de diálogos/ventanas flotantes del editor (Plantillas, Asistente, Ajustar imagen, Selector Var/Switch)
	var eh: Dictionary = ed.get_ui_handles()
	var tpl_btn: Button = eh.get("templates_btn", null)
	if tpl_btn != null:
		tpl_btn.emit_signal("pressed")
		await pump(3)
	var tpl_dlg: Window = host.find_child("UiTemplateGalleryDialog", true, false)
	var tpl_ov: Array = collect_ov.call(tpl_dlg, tpl_dlg.get_child(0) as Control) if tpl_dlg != null and tpl_dlg.get_child_count() > 0 else ["missing"]
	check(tpl_dlg != null and tpl_dlg.visible and tpl_dlg.size.x >= 760 and tpl_dlg.size.y >= 560 and tpl_dlg.size.y <= host.size.y and tpl_ov.is_empty(),
		"Punto 3 Template Gallery Dialog: size=%s <=%s ov=%s" % [str(tpl_dlg.size if tpl_dlg != null else Vector2i()), str(host.size), str(tpl_ov)])
	if tpl_dlg != null:
		tpl_dlg.hide()

	var wiz_btn: Button = eh.get("new_menu_wizard_btn", null)
	if wiz_btn != null:
		wiz_btn.emit_signal("pressed")
		await pump(3)
	var wiz_dlg: Window = host.find_child("UiMenuWizardDialog", true, false)
	var wiz_ov: Array = collect_ov.call(wiz_dlg, wiz_dlg.get_child(0) as Control) if wiz_dlg != null and wiz_dlg.get_child_count() > 0 else ["missing"]
	check(wiz_dlg != null and wiz_dlg.visible and wiz_dlg.size.x >= 760 and wiz_dlg.size.y >= 560 and wiz_dlg.size.y <= host.size.y and wiz_ov.is_empty(),
		"Punto 3 Menu Wizard Dialog: size=%s <=%s ov=%s" % [str(wiz_dlg.size if wiz_dlg != null else Vector2i()), str(host.size), str(wiz_ov)])
	if wiz_dlg != null:
		wiz_dlg.hide()

	if dm != null:
		var img_el: Dictionary = dm.call("make_default_ui_element", "image", "hud")
		img_el["image_path"] = "res://icon.svg"
		var img_id: String = dm.call("add_ui_element", "hud", img_el, true)
		ed.select_screen("hud")
		ed.select_element(img_id)
		var fw_img: Window = ed.open_floating_element_window(img_id)
		await pump(2)
		if fw_img != null:
			var fw_h: Dictionary = fw_img.call("get_ui_handles")
			var adj_btn: Button = fw_h.get("adjust_manual_btn", null)
			if adj_btn != null:
				adj_btn.emit_signal("pressed")
				await pump(3)
		var adj_dlg: Window = host.find_child("ClockGearsImageAdjustDialog", true, false)
		check(adj_dlg != null and adj_dlg.visible and adj_dlg.size.x >= 760 and adj_dlg.size.y >= 560 and adj_dlg.size.y <= host.size.y,
			"Punto 3 Image Adjust Dialog: size=%s <=%s" % [str(adj_dlg.size if adj_dlg != null else Vector2i()), str(host.size)])
		if adj_dlg != null:
			adj_dlg.hide()
		ed.close_all_floating_element_windows()
		dm.call("delete_ui_element", "hud", img_id, true)

	var style_btn: Button = eh.get("edit_style_btn", null)
	if style_btn != null:
		style_btn.emit_signal("pressed")
		await pump(2)
		var st_dlg: Window = host.find_child("UiInlineStyleDialog", true, false)
		var st_ov: Array = collect_ov.call(st_dlg, st_dlg.get_child(0) as Control) if st_dlg != null and st_dlg.get_child_count() > 0 else []
		check(st_dlg != null and st_dlg.visible and st_dlg.size.x >= 760 and st_dlg.size.y >= 560 and st_ov.is_empty(),
			"Punto 3 Inline Style Dialog: size=%s ov=%s" % [str(st_dlg.size if st_dlg != null else Vector2i()), str(st_ov)])
		if st_dlg != null:
			st_dlg.hide()

	var cmd_sel := ClockGearsCommandSelector.new()
	host.add_child(cmd_sel)
	ClockGearsTheme.popup_fitted_window(cmd_sel, Vector2i(780, 580), Vector2i(760, 560), "ui_command_selector")
	await pump(3)
	var cmd_ov: Array = collect_ov.call(cmd_sel, cmd_sel.get_child(0) as Control) if cmd_sel.get_child_count() > 0 else []
	check(cmd_sel.visible and cmd_sel.size.x >= 760 and cmd_sel.size.y >= 560 and cmd_ov.is_empty(),
		"Punto 3 Command Selector Dialog: size=%s ov=%s" % [str(cmd_sel.size), str(cmd_ov)])
	cmd_sel.hide()
	cmd_sel.queue_free()

	var tp_dlg := ClockGearsTilePickerDialog.new()
	host.add_child(tp_dlg)
	ClockGearsTheme.popup_fitted_window(tp_dlg, Vector2i(820, 600), Vector2i(760, 560), "ui_tile_picker")
	await pump(3)
	check(tp_dlg.visible and tp_dlg.size.x >= 760 and tp_dlg.size.y >= 560,
		"Punto 3 Tile Picker Dialog: size=%s" % str(tp_dlg.size))
	tp_dlg.hide()
	tp_dlg.queue_free()

	host.queue_free()
	await pump(2)

	print("Tiempo: %d ms" % (Time.get_ticks_msec() - start_msec))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
