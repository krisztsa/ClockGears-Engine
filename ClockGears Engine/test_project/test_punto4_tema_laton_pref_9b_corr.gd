extends SceneTree

# Test Punto 4 (Correcciones UI 9B):
# - El tema "ClockGears Latón" aparece visible en Preferencias sin hacer scroll.
# - Cambia en caliente (Oscuro / Claro / ClockGears Latón) sin reiniciar.
# - Si falla al cargar, vuelve al tema actual sin error.
# - El tema actual por defecto queda intacto.

var passed: int = 0
var failed: int = 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 4 (9B CORR): TEMA CLOCKGEARS LATÓN EN PREFERENCIAS ===")
	var th = Engine.get_singleton("ClockGearsTheme")
	if th == null:
		_check(false, "ClockGearsTheme disponible")
		quit(1)
		return

	_check(String(th.get_active_theme_id()) == "dark", "Tema por defecto sigue siendo 'dark'")

	var pref := ClockGearsPreferencesDialog.new()
	root.add_child(pref)
	pref.open_preferences()
	await _step_frames(2)

	var opt: OptionButton = pref.get_editor_theme_option_button()
	_check(opt != null, "Selector de tema presente en Preferencias")
	var brass_idx := -1
	var dark_idx := -1
	var light_idx := -1
	for i in range(opt.item_count):
		var meta := String(opt.get_item_metadata(i))
		var txt := opt.get_item_text(i)
		if meta == "brass" and txt.find("Latón") >= 0:
			brass_idx = i
		elif meta == "dark":
			dark_idx = i
		elif meta == "light":
			light_idx = i
	_check(dark_idx >= 0 and light_idx >= 0 and brass_idx >= 0, "Los 3 temas están en la lista de Preferencias (dark=%d light=%d brass=%d)" % [dark_idx, light_idx, brass_idx])

	# Verificar que el selector de tema es visible arriba sin hacer scroll
	var pref_rect := Rect2(Vector2.ZERO, Vector2(pref.size))
	var opt_rect := opt.get_global_rect()
	_check(opt.is_visible_in_tree() and pref_rect.encloses(opt_rect), "Selector de tema visible de inmediato en Preferencias sin hacer scroll (opt=%s win=%s)" % [str(opt_rect), str(pref_rect)])

	# Cambio en caliente a ClockGears Latón
	var db := ClockGearsDatabaseDialog.new()
	db.exclusive = false
	root.add_child(db)
	db.popup_centered(Vector2i(1366, 768))
	await _step_frames(1)

	pref.select_editor_theme_by_id("brass")
	await _step_frames(2)
	_check(String(th.get_active_theme_id()) == "brass" and String(pref.get_selected_editor_theme_id()) == "brass", "Cambio en caliente a 'brass' (ClockGears Latón) sin reiniciar")
	_check(db.has_meta("_cg_brass_tree_applied"), "DatabaseDialog recibe tema Latón en caliente")

	# Fallo simulado al cargar vuelve al tema anterior sin error
	pref.select_editor_theme_by_id("dark")
	await _step_frames(1)
	th.set_simulate_brass_load_failure(true)
	var ok_fail := pref.select_editor_theme_by_id("brass")
	await _step_frames(1)
	_check(not ok_fail and String(th.get_active_theme_id()) == "dark", "Si Latón falla al cargar, vuelve al tema actual ('dark') sin error")
	th.set_simulate_brass_load_failure(false)

	# Volver al tema actual por defecto
	pref.select_editor_theme_by_id("dark")
	await _step_frames(2)
	_check(String(th.get_active_theme_id()) == "dark" and not db.has_meta("_cg_brass_tree_applied"), "Vuelta en caliente a 'dark' restaura estado previo")

	db.queue_free()
	pref.queue_free()
	print("=== RESULTADO PUNTO 4: %d PASSED, %d FAILED ===" % [passed, failed])
	quit(0 if failed == 0 else 1)
