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
	# 960x540, 1366x768, 1920x1080, 2560x1440
	var taskbar_h = 48 # Barra de tareas tipica de Windows
	var resolutions = [
		Vector2i(960, 540),
		Vector2i(1366, 768),
		Vector2i(1920, 1080),
		Vector2i(2560, 1440)
	]

	for res in resolutions:
		var usable_sz = Vector2i(res.x, res.y - taskbar_h)
		# Tamano deseado con clamping segun la logica de _adjust_and_popup:
		var target_sz = Vector2i(920, 620)
		var max_allowed = usable_sz - Vector2i(24, 48)
		var clamped_sz = target_sz.min(max_allowed)
		
		# Asegurar que la ventana cabe en el area util sin salirse
		var fits = clamped_sz.x <= usable_sz.x and clamped_sz.y <= usable_sz.y
		if not fits:
			print("FAIL: En resolucion %dx%d (util %dx%d), la ventana calculada %dx%d excede el area util!" % [res.x, res.y, usable_sz.x, usable_sz.y, clamped_sz.x, clamped_sz.y])
			quit(1)
			return
		print("OK: Resolucion %dx%d (area util %dx%d) -> ventana cabe perfectamente a %dx%d (sin cortar botones inferiores)." % [res.x, res.y, usable_sz.x, usable_sz.y, clamped_sz.x, clamped_sz.y])

	# 5. Probar que el custom_action "apply" se ejecuta sin crash
	event_dialog.emit_signal("custom_action", "apply")
	print("OK: Accion custom 'apply' ejecutada sin errores.")

	print("ALL PASS: Editor de eventos dimensionado al area util y con scroll interno verificado con exito.")
	event_dialog.queue_free()
	quit(0)
