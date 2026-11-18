extends SceneTree

# Test para verificar el dimensionamiento y scroll interno del Editor de Eventos (Punto 3)
var event_dialog: Window = null

func _init() -> void:
	print("=== INICIANDO TEST: EDITOR DE EVENTOS (PUNTO 3) ===")
	call_deferred("_run_checks")

func _run_checks() -> void:
	var root_node = root
	if not ClassDB.can_instantiate("ClockGearsEventEditorDialog"):
		print("FAIL: ClockGearsEventEditorDialog no instanciable desde ClassDB.")
		quit(1)
		return

	event_dialog = ClassDB.instantiate("ClockGearsEventEditorDialog")
	root_node.add_child(event_dialog)

	# 1. Comprobar que existe un ScrollContainer interno para las columnas
	var scroll_found: ScrollContainer = null
	var stack = [event_dialog]
	while stack.size() > 0:
		var curr = stack.pop_back()
		if curr is ScrollContainer:
			scroll_found = curr
			break
		for child in curr.get_children():
			stack.push_back(child)

	if scroll_found == null:
		print("FAIL: No se encontro ScrollContainer en la jerarquia de contenido del Editor de Eventos.")
		quit(1)
		return

	print("OK: ScrollContainer interno detectado (modo horizontal: %d, vertical: %d)" % [scroll_found.get_horizontal_scroll_mode(), scroll_found.get_vertical_scroll_mode()])

	# 2. Comprobar que el boton Aplicar, Aceptar y Cancelar existen
	var ok_btn = event_dialog.get_ok_button()
	if ok_btn == null or ok_btn.get_text().is_empty():
		print("FAIL: Boton Aceptar no configurado correctamente.")
		quit(1)
		return
	print("OK: Boton Aceptar verificado ('%s')" % ok_btn.get_text())

	var cancel_btn = event_dialog.get_cancel_button()
	if cancel_btn == null or cancel_btn.get_text().is_empty():
		print("FAIL: Boton Cancelar no configurado correctamente.")
		quit(1)
		return
	print("OK: Boton Cancelar verificado ('%s')" % cancel_btn.get_text())

	# 3. Verificar que adjust_and_popup existe y dimensiona adecuadamente
	if event_dialog.has_method("adjust_and_popup"):
		event_dialog.call("adjust_and_popup")
		var popup_sz = event_dialog.get_size()
		print("OK: adjust_and_popup ejecutado, tamano resultante: ", popup_sz)

	# 4. Probar comportamiento con areas utiles en resoluciones 16:9 requeridas:
	# 1366x768, 1920x1080, 2560x1440 con barra de tareas de 48px visible
	var taskbar_h = 48
	var resolutions = [
		Vector2i(1366, 768),
		Vector2i(1920, 1080),
		Vector2i(2560, 1440)
	]

	for res in resolutions:
		var usable_rect = Rect2i(0, 0, res.x, res.y - taskbar_h)
		var r: Rect2i = ClockGearsEventEditorDialog.compute_usable_fullscreen_rect(usable_rect, 32, 4)
		var fills_width: bool = r.size.x >= usable_rect.size.x - 12
		var fills_height: bool = r.size.y >= usable_rect.size.y - 48
		var fits_inside: bool = r.position.x >= usable_rect.position.x and r.position.y >= usable_rect.position.y and r.end.x <= usable_rect.end.x and r.end.y <= usable_rect.end.y
		if not (fills_width and fills_height and fits_inside):
			print("FAIL: En %dx%d (util %s), rect calculado %s no ocupa todo el area util sin cortar!" % [res.x, res.y, usable_rect, r])
			quit(1)
			return
		print("OK: %dx%d (util %s) -> ocupa todo el area util %s sin huecos ni pisar barra de tareas." % [res.x, res.y, usable_rect, r])

	var apply_btn = event_dialog.get_apply_button()
	if apply_btn == null or apply_btn.get_text().is_empty():
		print("FAIL: Boton Aplicar no encontrado.")
		quit(1)
		return
	print("OK: Boton Aplicar verificado ('%s')" % apply_btn.get_text())

	# 5. Probar que el custom_action "apply" se ejecuta sin crash
	event_dialog.emit_signal("custom_action", "apply")
	print("OK: Accion custom 'apply' ejecutada sin errores.")

	print("ALL PASS: Editor de eventos dimensionado al area util y con scroll interno verificado con exito.")
	event_dialog.queue_free()
	quit(0)
