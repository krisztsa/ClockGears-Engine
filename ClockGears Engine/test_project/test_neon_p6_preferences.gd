extends SceneTree

var pass_count := 0
var fail_count := 0
var start_msec := 0

func _process(_delta: float) -> bool:
	if start_msec > 0 and Time.get_ticks_msec() - start_msec > 30000:
		printerr("TIMEOUT interno 30 s en test_neon_p6_preferences.gd")
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count + 1])
		quit(2)
		return true
	return false

func check(cond: bool, msg: String) -> void:
	if cond:
		pass_count += 1
		print("  [PASS] ", msg)
	else:
		fail_count += 1
		print("  [FAIL] ", msg)

func _find_controls_by_type_var(root: Node, tvar: StringName) -> Array:
	var out: Array = []
	var stack: Array = [root]
	while not stack.is_empty():
		var n: Node = stack.pop_back()
		if n is Control and (n as Control).theme_type_variation == tvar:
			out.append(n)
		for i in range(n.get_child_count()):
			stack.push_back(n.get_child(i))
	return out

func _initialize() -> void:
	start_msec = Time.get_ticks_msec()
	var cgt = Engine.get_singleton("ClockGearsTheme")
	check(cgt != null, "ClockGearsTheme singleton disponible")
	if cgt == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	ProjectSettings.set_setting("clockgears/editor/glow_effects_level", 1)
	ProjectSettings.set_setting("clockgears/editor/glow_effects", true)
	cgt.set_glow_effects_level(1)
	cgt.set_active_theme_id("dark")

	var prefs = ClassDB.instantiate("ClockGearsPreferencesDialog")
	get_root().add_child(prefs)
	cgt.apply_to_window(prefs)
	await process_frame

	# 1. Tarjetas, OK primario y OptionButton de 3 opciones de brillo
	var cards: Array = _find_controls_by_type_var(prefs, &"ClockGearsContainer")
	var seps: Array = _find_controls_by_type_var(prefs, &"ClockGearsSectionHeaderLine")
	var ok_btn: Button = prefs.call("get_ok_button")
	check(cards.size() >= 4 and seps.size() >= 1 and ok_btn != null and ok_btn.theme_type_variation == &"ClockGearsPrimaryButton",
		"Preferencias usa tarjetas ClockGearsContainer (%d), separador y OK primario" % cards.size())

	var glow_opt: OptionButton = prefs.call("get_glow_effects_option_button")
	check(glow_opt != null and glow_opt.item_count == 3,
		"Efectos de brillo convertido en desplegable de 3 opciones (Desactivado / Suave / Intenso)")
	if glow_opt != null:
		check(glow_opt.get_item_id(0) == 0 and glow_opt.get_item_id(1) == 1 and glow_opt.get_item_id(2) == 2 and glow_opt.get_selected_id() == 1,
			"Opción por defecto es Suave (id=1), con IDs 0, 1 y 2")

	# Probe live Button inside another themed window to verify instant rebuild without restart
	var probe_win := Window.new()
	probe_win.size = Vector2i(320, 200)
	var probe_btn := Button.new()
	probe_btn.text = "Probe"
	probe_win.add_child(probe_btn)
	get_root().add_child(probe_win)
	cgt.apply_to_window(probe_win)
	await process_frame

	# 2. Probar cambio inmediato a Desactivado (0)
	if glow_opt != null:
		glow_opt.select(0)
		glow_opt.emit_signal("item_selected", 0)
		await process_frame
		var tk0: Dictionary = cgt.get_theme_tokens_dict("dark")
		var b_hov0: StyleBoxFlat = probe_btn.get_theme_stylebox("hover") as StyleBoxFlat
		check(cgt.get_glow_effects_level() == 0 and int(tk0["halo_radius"]) == 0 and int(tk0["transition_ms"]) == 0,
			"Nivel 0 (Desactivado) anula halo_radius y transition_ms al instante")
		check(b_hov0 != null and b_hov0.shadow_size == 0 and (tk0["border_copper"] as Color).a > 0.2,
			"Con brillo Desactivado desaparece el halo (shadow_size=0) pero se conservan los bordes cobrizos")

		# 3. Suave (1)
		glow_opt.select(1)
		glow_opt.emit_signal("item_selected", 1)
		await process_frame
		var tk1: Dictionary = cgt.get_theme_tokens_dict("dark")
		var r1 := int(tk1["halo_radius"])
		var t1 := int(tk1["transition_ms"])
		var b_hov1: StyleBoxFlat = probe_btn.get_theme_stylebox("hover") as StyleBoxFlat
		check(cgt.get_glow_effects_level() == 1 and r1 > 0 and t1 > 0 and t1 <= 150 and b_hov1 != null and b_hov1.shadow_size == r1,
			"Nivel 1 (Suave) restaura halo=%d y transición=%d ms en vivo" % [r1, t1])

		# 4. Intenso (2)
		glow_opt.select(2)
		glow_opt.emit_signal("item_selected", 2)
		await process_frame
		var tk2: Dictionary = cgt.get_theme_tokens_dict("dark")
		var r2 := int(tk2["halo_radius"])
		var b_hov2: StyleBoxFlat = probe_btn.get_theme_stylebox("hover") as StyleBoxFlat
		check(cgt.get_glow_effects_level() == 2 and r2 > r1 and b_hov2 != null and b_hov2.shadow_size == r2,
			"Nivel 2 (Intenso) aumenta halo (%d > %d) al instante sin reiniciar" % [r2, r1])

	# 5. Cambio de tema Oscuro <-> Latón en Preferencias actualiza acentos y tokens en vivo
	for tid in ["brass", "dark"]:
		prefs.call("select_editor_theme_by_id", tid)
		await process_frame
		var tk_t: Dictionary = cgt.get_theme_tokens_dict(tid)
		check(cgt.get_active_theme_id() == tid, "Tema cambiado en vivo desde Preferencias a %s" % tid)
		var card_sb: StyleBoxFlat = cards[0].get_theme_stylebox("panel") as StyleBoxFlat
		check(card_sb != null and card_sb.border_color.is_equal_approx(tk_t["border_copper"]),
			"[%s] Tarjetas de Preferencias heredan borde cobrizo del tema activo" % tid)

	# Restore default glow level 1 ("Suave") so baseline settings stay clean
	prefs.call("select_glow_effects_level", 1)
	probe_win.free()
	prefs.free()

	print("Tiempo test_neon_p6_preferences: %d ms" % (Time.get_ticks_msec() - start_msec))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
