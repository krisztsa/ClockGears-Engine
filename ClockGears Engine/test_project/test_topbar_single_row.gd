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

func _init() -> void:
	await _run()

func _run() -> void:
	var cgt = Engine.get_singleton("ClockGearsTheme")
	check(cgt != null, "ClockGearsTheme singleton available")
	if cgt == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	var orig_tid: String = str(cgt.get_active_theme_id())
	for tid in ["dark", "brass"]:
		cgt.set_active_theme_id(tid)
		var bar = ClassDB.instantiate("ClockGearsTopBar")
		bar.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
		get_root().add_child(bar)
		cgt.apply_to_control(bar)
		await pump(3)

		var lbtn0: Button = bar.call("get_layer_button", 0)
		var tbtn0: Button = bar.call("get_tool_button", 0)
		var pt: Button = bar.call("get_playtest_button")
		var ov: MenuButton = bar.call("get_overflow_button")
		check(lbtn0 != null and tbtn0 != null and pt != null and ov != null, "[%s] TopBar buttons & overflow MenuButton present" % tid)

		if lbtn0:
			var sb_n: StyleBoxFlat = lbtn0.get_theme_stylebox("normal")
			var sb_h: StyleBoxFlat = lbtn0.get_theme_stylebox("hover")
			var sb_p: StyleBoxFlat = lbtn0.get_theme_stylebox("pressed")
			check(sb_n != null and (not sb_n.draw_center or sb_n.bg_color.a < 0.01) and (sb_n.border_width_left == 0 or sb_n.border_color.a < 0.01),
				"[%s] Toolbar button normal state has NO box/border (transparent)" % tid)
			check(sb_h != null and sb_h.border_width_left >= 1 and sb_h.border_color.a > 0.5 and sb_h.shadow_size > 0,
				"[%s] Hover shows copper border and halo" % tid)
			check(sb_p != null and sb_p.border_width_left == 2 and sb_p.shadow_size > 0,
				"[%s] Active button shows 2px border, halo and tint" % tid)
			check(is_equal_approx(lbtn0.size.x, 40.0) and is_equal_approx(lbtn0.size.y, 40.0),
				"[%s] Button box = 32px icon + 4px/side = 40x40 (got %s)" % [tid, str(lbtn0.size)])
			check(lbtn0.get_theme_constant("icon_max_width") == 32 and lbtn0.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST,
				"[%s] Icon stays 32px nearest unfiltered" % tid)
			var grp_p = lbtn0.get_parent() as HBoxContainer
			check(grp_p != null and grp_p.get_theme_constant("separation") == 2,
				"[%s] Group button separation == 2 px" % tid)

		# 1366x768: ALL 8 groups on ONE single row (~44px tall), Playtest icon-only with tooltip
		get_root().size = Vector2i(1366, 768)
		bar.size = Vector2(1366, 44)
		await pump(4)
		check(bar.call("is_single_row") and bar.size.y >= 40.0 and bar.size.y <= 48.0,
			"[%s] 1366x768 bar is 1 row ~44px (h=%.1f)" % [tid, bar.size.y])
		check(int(bar.call("get_visible_group_count")) == int(bar.call("get_total_group_count")) and not ov.visible,
			"[%s] 1366x768 all %d groups visible without overflow" % [tid, int(bar.call("get_total_group_count"))])
		check(pt.text == "" and pt.tooltip_text.contains("Probar") and is_equal_approx(pt.size.x, 40.0),
			"[%s] 1366x768 Playtest fits cleanly as icon-only with tooltip (w=%.1f)" % [tid, pt.size.x])
		check(pt.global_position.x + pt.size.x <= 1366.0,
			"[%s] 1366x768 Playtest ends inside viewport (right=%.1f)" % [tid, pt.global_position.x + pt.size.x])

		# 1920x1080 and 2560x1440: Playtest shows "Probar (F5)" text in same single row
		for r_wide in [Vector2i(1920, 1080), Vector2i(2560, 1440)]:
			get_root().size = r_wide
			bar.size = Vector2(r_wide.x, 44)
			await pump(4)
			check(bar.size.y <= 48.0 and not ov.visible and pt.text == "Probar (F5)",
				"[%s] %s single row <=48px with 'Probar (F5)' visible (h=%.1f, text='%s')" % [tid, str(r_wide), bar.size.y, pt.text])

		# 960x540: NEVER wraps to 2 rows, sends trailing groups to "»" overflow menu
		get_root().size = Vector2i(960, 540)
		bar.size = Vector2(960, 44)
		await pump(4)
		var vis_g: int = int(bar.call("get_visible_group_count"))
		var ov_g: int = int(bar.call("get_overflow_group_count"))
		var pop: PopupMenu = ov.get_popup()
		check(bar.size.y <= 48.0, "[%s] 960x540 NEVER wraps into 2 rows (h=%.1f)" % [tid, bar.size.y])
		check(ov.visible and ov.text == "»" and vis_g < int(bar.call("get_total_group_count")) and ov_g >= 1,
			"[%s] 960x540 shows '»' overflow button (vis=%d, ov=%d)" % [tid, vis_g, ov_g])
		check(pop != null and pop.item_count >= 5 and ov.global_position.x + ov.size.x <= 960.0,
			"[%s] 960x540 '»' menu holds overflowed tools (%d items) inside 960px" % [tid, pop.item_count if pop else 0])

		# Check separator margins (8px each side) & overflow action dispatch
		for ch in bar.find_children("*", "MarginContainer", true, false):
			check(ch.get_theme_constant("margin_left") == 8 and ch.get_theme_constant("margin_right") == 8,
				"[%s] Separator has 8px margin on left and right" % tid)
			break
		if pop and pop.item_count >= 1:
			pop.emit_signal("about_to_popup")
			pop.emit_signal("id_pressed", 0)
			await pump(1)
			var tbtn_first_ov := false
			for i in 8:
				if bar.call("get_tool_button", i).button_pressed:
					tbtn_first_ov = true
					break
			check(tbtn_first_ov, "[%s] Overflow menu dispatch keeps tool toggle state valid" % tid)

		bar.free()

	cgt.set_active_theme_id(orig_tid)
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
