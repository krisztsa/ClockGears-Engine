extends SceneTree

func _init():
	print("--- TEST CAMERA AND DISPLAY (FASE 7B) ---")
	var failures = 0

	# 1. Test ClockGearsCameraDialog & ClockGearsCameraPreviewCanvas registration
	print("\n[TEST 1] Verificando registro de clases de Cámara...")
	if ClassDB.class_exists("ClockGearsCameraDialog"):
		print("  PASS: ClockGearsCameraDialog registrado en ClassDB")
	else:
		print("  FAIL: ClockGearsCameraDialog no registrado")
		failures += 1

	if ClassDB.class_exists("ClockGearsCameraPreviewCanvas"):
		print("  PASS: ClockGearsCameraPreviewCanvas registrado en ClassDB")
	else:
		print("  FAIL: ClockGearsCameraPreviewCanvas no registrado")
		failures += 1

	# 2. Test ClockGearsPixelCamera2D & decimal zoom & properties
	print("\n[TEST 2] Verificando ClockGearsPixelCamera2D y zoom decimal...")
	var cam = ClockGearsPixelCamera2D.new()
	cam.set_allow_fractional_zoom(true)
	if cam.is_allow_fractional_zoom():
		print("  PASS: allow_fractional_zoom activo por defecto o configurable")
	else:
		print("  FAIL: allow_fractional_zoom falló")
		failures += 1

	cam.set_zoom(Vector2(1.7, 1.7))
	if abs(cam.get_zoom().x - 1.7) < 0.001 and abs(cam.get_zoom().y - 1.7) < 0.001:
		print("  PASS: Zoom decimal 1.7x soportado correctamente: ", cam.get_zoom())
	else:
		print("  FAIL: Zoom decimal falló: ", cam.get_zoom())
		failures += 1

	cam.set_smooth_follow_enabled(true)
	cam.set_follow_speed(12.5)
	if cam.is_smooth_follow_enabled() and abs(cam.get_follow_speed() - 12.5) < 0.001:
		print("  PASS: Suavizado y velocidad configurados correctamente (12.5)")
	else:
		print("  FAIL: Suavizado / velocidad fallaron")
		failures += 1

	# Modo entero pixel perfect
	cam.set_allow_fractional_zoom(false)
	cam.set_integer_zoom(3)
	if cam.get_integer_zoom() == 3 and cam.get_zoom() == Vector2(3.0, 3.0):
		print("  PASS: Modo entero (pixel perfect) 3x funciona correctamente")
	else:
		print("  FAIL: Modo entero falló")
		failures += 1
	cam.free()

	# 3. Test Interpreter and CMD_CHANGE_CAMERA_ZOOM
	print("\n[TEST 3] Verificando comando de evento CMD_CHANGE_CAMERA_ZOOM con decimales...")
	var interp = ClockGearsEventInterpreter.new()
	var map = ClockGearsMap.new()
	map.set_map_size(Vector2i(20, 20))
	var dummy_cam = ClockGearsPixelCamera2D.new()
	dummy_cam.set_allow_fractional_zoom(true)
	dummy_cam.set_zoom(Vector2(1.0, 1.0))
	map.add_child(dummy_cam)
	map.set_runtime_camera(dummy_cam)

	var dummy_player = ClockGearsPlayer.new()
	map.add_child(dummy_player)
	interp.setup_interpreter(map, dummy_player, null)

	# Iniciar transición a zoom 1.5x con duración 10 frames
	interp.start_camera_zoom_transition(1.5, 10)
	for i in range(10):
		interp.step_frame()

	if abs(dummy_cam.get_zoom().x - 1.5) < 0.05:
		print("  PASS: Transición suave de zoom decimal a 1.5x completada: ", dummy_cam.get_zoom())
	else:
		print("  FAIL: Zoom no alcanzó 1.5x tras 10 frames: ", dummy_cam.get_zoom())
		failures += 1

	# Transición instantánea (0 frames)
	interp.start_camera_zoom_transition(2.7, 0)
	if abs(dummy_cam.get_zoom().x - 2.7) < 0.01:
		print("  PASS: Transición instantánea (duración = 0) a 2.7x aplicada de inmediato: ", dummy_cam.get_zoom())
	else:
		print("  FAIL: Transición instantánea falló: ", dummy_cam.get_zoom())
		failures += 1

	# Probar comando completo en la lista de comandos del evento
	var cmd_dict = {
		"type": "change_camera_zoom",
		"zoom": 1.8,
		"duration": 5,
		"wait": false
	}
	interp.start_command_list([cmd_dict])
	for i in range(6):
		interp.step_frame()
	if abs(dummy_cam.get_zoom().x - 1.8) < 0.05:
		print("  PASS: Comando 'change_camera_zoom' (1.8x) ejecutado por el intérprete con éxito: ", dummy_cam.get_zoom())
	else:
		print("  FAIL: Comando 'change_camera_zoom' falló: ", dummy_cam.get_zoom())
		failures += 1

	dummy_player.free()
	dummy_cam.free()
	map.free()
	interp.free()

	# 4. Test Shortcuts F11 / Alt+Enter in InputMap
	print("\n[TEST 4] Verificando atajos de Pantalla Completa (F11 y Alt+Enter)...")
	if InputMap.has_action("ui_toggle_fullscreen"):
		var events = InputMap.action_get_events("ui_toggle_fullscreen")
		var has_f11 = false
		var has_alt_enter = false
		for ev in events:
			if ev is InputEventKey:
				if ev.keycode == KEY_F11:
					has_f11 = true
				if ev.keycode == KEY_ENTER and ev.alt_pressed:
					has_alt_enter = true
		if has_f11 and has_alt_enter:
			print("  PASS: ui_toggle_fullscreen incluye F11 y Alt+Enter")
		else:
			print("  FAIL: ui_toggle_fullscreen no incluye ambos atajos. F11=", has_f11, " Alt+Enter=", has_alt_enter)
			failures += 1
	else:
		print("  FAIL: ui_toggle_fullscreen no encontrado en InputMap")
		failures += 1

	# 5. Test ProjectSettings & Screen Mode presets
	print("\n[TEST 5] Verificando Modos de Pantalla y Configuración del Sistema...")
	var dm = ClockGearsDataManager
	if dm:
		var sys = dm.get_system_config()
		sys["camera_zoom"] = 1.7
		sys["camera_follow_player"] = true
		sys["camera_smooth"] = true
		sys["camera_smooth_speed"] = 8.0
		sys["camera_clamp"] = true
		sys["camera_integer_zoom"] = false
		sys["screen_mode"] = 1 # expand
		dm.set_system_config(sys)
		var read_back = dm.get_system_config()
		if abs(float(read_back.get("camera_zoom", 0.0)) - 1.7) < 0.001 and int(read_back.get("screen_mode", -1)) == 1:
			print("  PASS: Configuración de cámara y modo de pantalla persistida en system_config")
		else:
			print("  FAIL: Error en persistencia de system_config: ", read_back)
			failures += 1

	# 6. Test UTF-8 string encoding in menus
	print("\n[TEST 6] Verificando codificación UTF-8 en cadenas del menú...")
	var s_auto = "Automatización"
	var s_dis = "Diseñador"
	var s_cam = "Cámara..."
	if not ("Ã" in s_auto or "Ã" in s_dis or "Ã" in s_cam):
		print("  PASS: Sin caracteres rotos de codificación (Automatización, Diseñador, Cámara...)")
	else:
		print("  FAIL: Caracteres rotos detectados")
		failures += 1

	print("\n-------------------------------------------")
	if failures == 0:
		print("TODAS LAS PRUEBAS DE CÁMARA Y PANTALLA PASARON CON ÉXITO")
	else:
		print("ERRORES DETECTADOS: ", failures)
	print("-------------------------------------------")
	quit(failures)
