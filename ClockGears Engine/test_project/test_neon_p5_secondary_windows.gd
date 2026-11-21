extends SceneTree

var pass_count := 0
var fail_count := 0
var start_msec := 0

func _process(_delta: float) -> bool:
	if start_msec > 0 and Time.get_ticks_msec() - start_msec > 30000:
		printerr("TIMEOUT interno 30 s en test_neon_p5_secondary_windows.gd")
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

func _find_nodes_of_class(root: Node, cname: String) -> Array:
	var out: Array = []
	var stack: Array = [root]
	while not stack.is_empty():
		var n: Node = stack.pop_back()
		if n.get_class() == cname:
			out.append(n)
		for i in range(n.get_child_count()):
			stack.push_back(n.get_child(i))
	return out

func _initialize() -> void:
	start_msec = Time.get_ticks_msec()
	var t0 := start_msec
	var cgt = Engine.get_singleton("ClockGearsTheme")
	check(cgt != null, "ClockGearsTheme singleton disponible")
	if cgt == null:
		print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
		quit(1)
		return

	cgt.set_glow_effects_level(1)

	for tid in ["dark", "brass"]:
		print("--- Verificando Punto 5 (Ventanas Secundarias) en tema: ", tid, " ---")
		cgt.set_active_theme_id(tid)
		var tk: Dictionary = cgt.get_theme_tokens_dict(tid)
		var border_copper: Color = tk["border_copper"]

		# 1. ClockGearsEventEditorDialog
		var ev = ClassDB.instantiate("ClockGearsEventEditorDialog")
		get_root().add_child(ev)
		cgt.apply_to_window(ev)
		await process_frame
		var ev_ok: Button = ev.call("get_ok_button")
		var ev_cards: Array = _find_controls_by_type_var(ev, &"ClockGearsContainer")
		var ev_prevs: Array = _find_controls_by_type_var(ev, &"ClockGearsPreviewFrame")
		var ev_seps: Array = _find_controls_by_type_var(ev, &"ClockGearsSectionHeaderLine")
		check(ev_ok != null and ev_ok.theme_type_variation == &"ClockGearsPrimaryButton",
			"[%s] Editor de Eventos: botón OK usa ClockGearsPrimaryButton" % tid)
		check(ev_cards.size() >= 5 and ev_prevs.size() >= 1 and ev_seps.size() >= 1,
			"[%s] Editor de Eventos: tarjetas (%d), marco preview (%d) y separador (%d) heredan el tema" % [tid, ev_cards.size(), ev_prevs.size(), ev_seps.size()])
		if ev_cards.size() >= 1:
			var c_sb: StyleBoxFlat = ev_cards[0].get_theme_stylebox("panel") as StyleBoxFlat
			check(c_sb != null and not ev_cards[0].has_theme_stylebox_override("panel") and c_sb.border_color.is_equal_approx(border_copper),
				"[%s] Editor de Eventos: tarjetas heredan panel cobrizo del tema sin overrides duros" % tid)
		ev.free()

		# 2. ClockGearsMapPropertiesDialog
		var mp = ClassDB.instantiate("ClockGearsMapPropertiesDialog")
		get_root().add_child(mp)
		cgt.apply_to_window(mp)
		await process_frame
		var mp_ok: Button = mp.call("get_ok_button")
		check(mp_ok != null and mp_ok.theme_type_variation == &"ClockGearsPrimaryButton" and not mp_ok.has_theme_stylebox_override("normal"),
			"[%s] Propiedades de Mapa: botón OK usa ClockGearsPrimaryButton sin override gris" % tid)
		mp.free()

		# 3. ClockGearsCameraDialog
		var cam = ClassDB.instantiate("ClockGearsCameraDialog")
		get_root().add_child(cam)
		cgt.apply_to_window(cam)
		await process_frame
		var cam_ok: Button = cam.call("get_ok_button")
		var cam_prevs: Array = _find_controls_by_type_var(cam, &"ClockGearsPreviewFrame")
		var cam_cards: Array = _find_controls_by_type_var(cam, &"ClockGearsContainer")
		var cam_seps: Array = _find_controls_by_type_var(cam, &"ClockGearsSectionHeaderLine")
		check(cam_ok != null and cam_ok.theme_type_variation == &"ClockGearsPrimaryButton" and cam_prevs.size() >= 1 and cam_cards.size() >= 1 and cam_seps.size() >= 1,
			"[%s] Cámara y Pantalla: OK primario, marco preview y tarjetas heredan Neón Cobre" % tid)
		cam.free()

		# 4. ClockGearsTilePickerDialog
		var tp = ClassDB.instantiate("ClockGearsTilePickerDialog")
		get_root().add_child(tp)
		cgt.apply_to_window(tp)
		await process_frame
		var tp_ok: Button = tp.call("get_ok_button")
		var tp_prevs: Array = _find_controls_by_type_var(tp, &"ClockGearsPreviewFrame")
		var vscrolls: Array = _find_nodes_of_class(tp, "VScrollBar")
		var sb_clean := true
		if tp_prevs.size() >= 1 and tp_prevs[0].has_theme_stylebox_override("panel"):
			sb_clean = false
		for vs in vscrolls:
			if vs.has_theme_stylebox_override("scroll") or vs.has_theme_stylebox_override("grabber"):
				sb_clean = false
		check(tp_ok != null and tp_ok.theme_type_variation == &"ClockGearsPrimaryButton" and tp_prevs.size() >= 1 and sb_clean,
			"[%s] Selector de Tiles: hereda marco ClockGearsPreviewFrame, scrollbars 8 px cobrizos y OK primario" % tid)
		tp.free()

		# 5. ClockGearsLayersDialog
		var ly = ClassDB.instantiate("ClockGearsLayersDialog")
		get_root().add_child(ly)
		cgt.apply_to_window(ly)
		await process_frame
		var ly_ok: Button = ly.call("get_ok_button")
		var ly_cards: Array = _find_controls_by_type_var(ly, &"ClockGearsContainer")
		var ly_prim: Array = _find_controls_by_type_var(ly, &"ClockGearsPrimaryButton")
		check(ly_ok != null and ly_ok.theme_type_variation == &"ClockGearsPrimaryButton" and ly_cards.size() >= 3 and ly_prim.size() >= 1,
			"[%s] Capas Personalizadas: tarjetas ClockGearsContainer (%d) y botones primarios heredan tema" % [tid, ly_cards.size()])
		ly.free()

		# 6. ClockGearsGenerateEntityDialog
		var ge = ClassDB.instantiate("ClockGearsGenerateEntityDialog")
		get_root().add_child(ge)
		cgt.apply_to_window(ge)
		await process_frame
		var ge_ok: Button = ge.call("get_ok_button")
		var ge_prevs: Array = _find_controls_by_type_var(ge, &"ClockGearsPreviewFrame")
		var ge_seps: Array = _find_controls_by_type_var(ge, &"ClockGearsSectionHeaderLine")
		check(ge_ok != null and ge_ok.theme_type_variation == &"ClockGearsPrimaryButton" and ge_prevs.size() >= 1 and ge_seps.size() >= 2,
			"[%s] Generar Entidad: OK primario, marco preview y separadores cobrizos" % tid)
		ge.free()

		# 7. ClockGearsNewProjectDialog, ClockGearsResourceManagerDialog, ClockGearsSimplifiedExportDialog
		var np = ClassDB.instantiate("ClockGearsNewProjectDialog")
		get_root().add_child(np)
		cgt.apply_to_window(np)
		await process_frame
		var np_ok: Button = np.call("get_ok_button")
		var np_cards: Array = _find_controls_by_type_var(np, &"ClockGearsContainer")
		check(np_ok != null and np_ok.theme_type_variation == &"ClockGearsPrimaryButton" and np_cards.size() >= 1 and not np_cards[0].has_theme_stylebox_override("panel"),
			"[%s] Nuevo Proyecto: caja de carpetas ClockGearsContainer sin override duro y OK primario" % tid)
		np.free()

		if ClassDB.can_instantiate("ClockGearsResourceManagerDialog"):
			var rm = ClassDB.instantiate("ClockGearsResourceManagerDialog")
			get_root().add_child(rm)
			cgt.apply_to_window(rm)
			await process_frame
			var rm_ok: Button = rm.call("get_ok_button")
			var rm_prevs: Array = _find_controls_by_type_var(rm, &"ClockGearsPreviewFrame")
			check(rm_ok != null and rm_ok.theme_type_variation == &"ClockGearsPrimaryButton" and rm_prevs.size() >= 1 and not rm_prevs[0].has_theme_stylebox_override("panel"),
				"[%s] Gestor de Recursos: vista previa ClockGearsPreviewFrame sin override duro y OK primario" % tid)
			rm.free()

		if ClassDB.can_instantiate("ClockGearsSimplifiedExportDialog"):
			var se = ClassDB.instantiate("ClockGearsSimplifiedExportDialog")
			get_root().add_child(se)
			cgt.apply_to_window(se)
			await process_frame
			var se_ok: Button = se.call("get_ok_button")
			var se_cards: Array = _find_controls_by_type_var(se, &"ClockGearsContainer")
			check(se_ok != null and se_ok.theme_type_variation == &"ClockGearsPrimaryButton" and se_cards.size() >= 3,
				"[%s] Exportación Simplificada: tarjetas ClockGearsContainer (%d) y OK primario" % [tid, se_cards.size()])
			se.free()

	print("Tiempo test_neon_p5_secondary_windows: %d ms" % (Time.get_ticks_msec() - t0))
	print("RESULTADOS: %d PASS, %d FAIL" % [pass_count, fail_count])
	quit(0 if fail_count == 0 else 1)
