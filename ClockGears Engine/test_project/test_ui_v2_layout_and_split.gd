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

	db.queue_free()
	await pump(2)
	if dm_owned and dm and is_instance_valid(dm):
		dm.free()
	print("Tiempo: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
