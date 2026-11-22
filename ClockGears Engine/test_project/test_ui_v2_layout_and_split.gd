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
	var single_ok: bool = (not row2_flow.visible and row1_flow.get_line_count() == 1 and top_card.size.y <= 52.0)
	var two_clean_ok: bool = (row2_flow.visible and row1_flow.get_line_count() == 1 and row2_flow.get_line_count() == 1 and top_card.size.y <= 96.0)
	check(single_ok or two_clean_ok,
		"Punto 2 (1920x1080): Clean 1-row or 2-row toolbar without wrapping (total=%.1f, r2_vis=%s, lines=%d,%d)" % [top_card.size.y, str(row2_flow.visible), row1_flow.get_line_count(), row2_flow.get_line_count()])

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
		var lcard: Control = h3.get("left_card")
		var rcard: Control = h3.get("right_card")
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
				check(l_pct >= 0.17 and l_pct <= 0.25 and r_pct >= 0.30 and r_pct <= 0.38,
					"Punto 3 %s: Three zones stay ~21%% / rest / ~34%% (left=%.1f%%, right=%.1f%%)" % [str(res_p3), l_pct * 100.0, r_pct * 100.0])

	db.queue_free()
	await pump(2)
	if dm_owned and dm and is_instance_valid(dm):
		dm.free()
	print("Tiempo: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
