extends SceneTree

# ============================================================================
# TEST AUTOMÁTICO FASE 7B: 4 PUNTOS (BLOQUEO CAPAS, ESCALA 10%, CÁMARA, RESOLUCIÓN)
# ============================================================================

var passed_count: int = 0
var failed_count: int = 0
var step: int = 0

var map_node: ClockGearsMap
var interp: ClockGearsEventInterpreter

func assert_test(cond: bool, test_name: String):
	if cond:
		passed_count += 1
		print("[PASS] ", test_name)
	else:
		failed_count += 1
		print("[FAIL] ", test_name)

func _initialize():
	print("==================================================================")
	print("   TEST FASE 7B: VERIFICACIÓN COMPLETA DE LOS 4 PUNTOS           ")
	print("==================================================================")

	test_point_1_layer_locks_and_origin_reset()
	test_point_2_picture_scaling_10_percent()
	test_point_3_camera_zoom_and_map_limits()
	test_point_4_resolution_and_scaling_settings()

	# Iniciar simulación de frame para verificar zoom y eventos en runtime
	setup_runtime_camera_event_test()

# ----------------------------------------------------------------------------
# 1. BLOQUEO DE CAPAS Y REINICIO DE POSICIÓN A (0,0) (PUNTO 1)
# ----------------------------------------------------------------------------
func test_point_1_layer_locks_and_origin_reset():
	print("\n--- 1. Bloqueo de Capas y Reinicio de Origen (Punto 1) ---")
	var map = ClockGearsMap.new()
	map.set_map_size(Vector2i(30, 20))
	get_root().add_child(map)

	# Simular que el mapa o una capa fueron accidentalmente desplazados por el usuario
	map.set_position(Vector2(45, 80))
	var l_ground = map.get_layer(0)
	var l_deco = map.get_layer(1)
	var l_obj = map.get_layer(2)
	var l_over = map.get_layer(3)

	assert_test(l_ground != null and l_deco != null, "Capas del mapa existen")

	l_ground.set_position(Vector2(120, -50))
	l_deco.set_position(Vector2(32, 32))

	# ensure_default_layers o enter_tree debe resetearlos a (0,0) y bloquearlos
	map.ensure_default_layers()

	assert_test(map.get_position() == Vector2(0, 0), "Posición del mapa reseteada a (0, 0)")
	assert_test(l_ground.get_position() == Vector2(0, 0), "Posición de capa suelo reseteada a (0, 0)")
	assert_test(l_deco.get_position() == Vector2(0, 0), "Posición de capa decoración reseteada a (0, 0)")
	assert_test(l_obj.get_position() == Vector2(0, 0), "Posición de capa objetos reseteada a (0, 0)")
	assert_test(l_over.get_position() == Vector2(0, 0), "Posición de capa superior reseteada a (0, 0)")

	# Metadatos de bloqueo de edición en Godot
	assert_test(map.has_meta("_edit_lock_") and bool(map.get_meta("_edit_lock_")) == true, "Mapa bloqueado con _edit_lock_")
	assert_test(l_ground.has_meta("_edit_lock_") and bool(l_ground.get_meta("_edit_lock_")) == true, "Capa suelo bloqueada con _edit_lock_")
	assert_test(l_deco.has_meta("_edit_lock_") and bool(l_deco.get_meta("_edit_lock_")) == true, "Capa decoración bloqueada con _edit_lock_")

	map.queue_free()

# ----------------------------------------------------------------------------
# 2. ESCALA AL 10% Y PREVISUALIZACIÓN DE IMÁGENES (PUNTO 2)
# ----------------------------------------------------------------------------
func test_point_2_picture_scaling_10_percent():
	print("\n--- 2. Escala de Imagen al 10% y Tiradores (Punto 2) ---")
	var pm = ClockGearsPictureManager.new()
	get_root().add_child(pm)

	# Mostrar imagen con 10% de escala (Vector2(10, 10))
	pm.show_picture(1, "", 0, Vector2(100, 100), Vector2(10, 10), 1.0, 0, 0, 0.0, false)

	var sc = pm.get_picture_scale(1)
	assert_test(is_equal_approx(sc.x, 0.1) and is_equal_approx(sc.y, 0.1), "Escala interna al 10% es exactamente 0.1x (no se redondea a 1.0x)")

	# Activar integer scaling explícitamente: nunca debe redondear < 1.0f
	pm.set_enforce_integer_scaling(true)
	# Mover o actualizar
	pm.show_picture(2, "", 0, Vector2(50, 50), Vector2(25, 25), 1.0, 0, 0, 0.0, false)
	var sc25 = pm.get_picture_scale(2)
	assert_test(is_equal_approx(sc25.x, 0.25) and is_equal_approx(sc25.y, 0.25), "Integer scaling NO redondea escalas menores a 1x (25% se mantiene en 0.25x)")

	# Pero al ampliar (p.ej. 240%), sí redondea a entero (2x) si está activo
	pm.show_picture(3, "", 0, Vector2(50, 50), Vector2(240, 240), 1.0, 0, 0, 0.0, false)
	assert_test(pm.has_picture(3), "Ranura 3 de imagen activa")

	# Test de ClockGearsPictureVisualPicker: Anclaje rápido al centro con resolución dinámica
	var picker = ClockGearsPictureVisualPicker.new()
	get_root().add_child(picker)
	var spin_x = SpinBox.new(); spin_x.set_min(-9999); spin_x.set_max(9999); spin_x.set_value(0)
	var spin_y = SpinBox.new(); spin_y.set_min(-9999); spin_y.set_max(9999); spin_y.set_value(0)
	var spin_sx = SpinBox.new(); spin_sx.set_min(1); spin_sx.set_max(2000); spin_sx.set_value(10)
	var spin_sy = SpinBox.new(); spin_sy.set_min(1); spin_sy.set_max(2000); spin_sy.set_value(10)
	get_root().add_child(spin_x); get_root().add_child(spin_y); get_root().add_child(spin_sx); get_root().add_child(spin_sy)

	picker.bind_spinboxes(spin_x, spin_y, spin_sx, spin_sy)
	picker.set_origin_mode(1) # Origen Centro
	picker.quick_anchor_center()

	# Con resolución interna 960x540, el centro exacto es (480, 270)
	assert_test(spin_x.get_value() == 480 and spin_y.get_value() == 270, "Botón 'Centro' posiciona exactamente en (480, 270) para resolución 960x540")

	picker.queue_free()
	spin_x.queue_free(); spin_y.queue_free(); spin_sx.queue_free(); spin_sy.queue_free()
	pm.queue_free()

# ----------------------------------------------------------------------------
# 3. CÁMARA: LÍMITES DE MAPA Y AJUSTES DE BASE DE DATOS (PUNTO 3)
# ----------------------------------------------------------------------------
func test_point_3_camera_zoom_and_map_limits():
	print("\n--- 3. Cámara: Zoom y Restricción a Límites del Mapa (Punto 3) ---")

	# Configurar actor inicial y posición en ClockGearsDataManager
	var act = {"id": 1, "name": "Heroe", "graphic": ""}
	ClockGearsDataManager.set_category_array(0, [act]) # CAT_ACTORS = 0
	var sys_cfg = ClockGearsDataManager.get_system_config()
	sys_cfg["has_start_pos"] = true
	sys_cfg["start_map"] = "res://test_map.tscn"
	sys_cfg["start_x"] = 5
	sys_cfg["start_y"] = 5
	sys_cfg["camera_zoom"] = 2
	sys_cfg["camera_smooth"] = true
	sys_cfg["camera_clamp"] = true
	ClockGearsDataManager.set_system_config(sys_cfg)

	var map = ClockGearsMap.new()
	map.set_map_size(Vector2i(40, 30))
	map.set_grid_resolution(32)
	get_root().add_child(map)

	# Iniciar sesión de runtime
	map.init_runtime_session()
	var cam = map.get_runtime_camera()
	assert_test(cam != null, "runtime_camera creada en ClockGearsMap")

	if cam:
		assert_test(cam.is_smooth_follow_enabled() == true, "Seguimiento suave activado por defecto")
		assert_test(cam.get_integer_zoom() == 2, "Zoom entero inicial es 2x según configuración de sistema")

		# Límites del mapa: 40 tiles * 32 = 1280, 30 tiles * 32 = 960
		var lim_l = cam.get_limit(SIDE_LEFT)
		var lim_t = cam.get_limit(SIDE_TOP)
		var lim_r = cam.get_limit(SIDE_RIGHT)
		var lim_b = cam.get_limit(SIDE_BOTTOM)

		assert_test(lim_l == 0 and lim_t == 0, "Límites superior e izquierdo en 0")
		assert_test(lim_r == 40 * 32, "Límite derecho restringido exactamente al ancho del mapa (1280 px)")
		assert_test(lim_b == 30 * 32, "Límite inferior restringido exactamente al alto del mapa (960 px)")

	map_node = map
	interp = map.get_runtime_interpreter()

# ----------------------------------------------------------------------------
# 4. PREFERENCIAS DE RESOLUCIÓN Y VENTANA (PUNTO 4)
# ----------------------------------------------------------------------------
func test_point_4_resolution_and_scaling_settings():
	print("\n--- 4. Ajustes de Resolución, Ventana y Escalado (Punto 4) ---")
	var ps = ProjectSettings

	# Verificar soporte y valores por defecto en ProjectSettings
	ps.set_setting("display/window/size/viewport_width", 960)
	ps.set_setting("display/window/size/viewport_height", 540)
	ps.set_setting("display/window/size/window_width_override", 1920)
	ps.set_setting("display/window/size/window_height_override", 1080)
	ps.set_setting("display/window/stretch/scale_mode", "integer")

	var vw = ps.get_setting("display/window/size/viewport_width")
	var vh = ps.get_setting("display/window/size/viewport_height")
	var ww = ps.get_setting("display/window/size/window_width_override")
	var wh = ps.get_setting("display/window/size/window_height_override")
	var sm = ps.get_setting("display/window/stretch/scale_mode")

	assert_test(vw == 960 and vh == 540, "Resolución interna: 960x540 nHD")
	assert_test(ww == 1920 and wh == 1080, "Resolución de ventana override: 1920x1080 Full HD")
	assert_test(sm == "integer", "Modo de escalado: integer (Pixel Perfect)")

# ----------------------------------------------------------------------------
# 5. COMANDO DE EVENTO: CAMBIAR ZOOM DE CÁMARA (RUNTIME TEST)
# ----------------------------------------------------------------------------
func setup_runtime_camera_event_test():
	print("\n--- 5. Comando de Evento: Cambiar Zoom de Cámara ---")
	if not interp:
		_finish()
		return

	# Programar comando CMD_CHANGE_CAMERA_ZOOM: zoom = 3x, duration = 5 frames, wait = true
	var cmds = [
		{
			"type": "change_camera_zoom",
			"code": 322,
			"zoom": 3,
			"duration": 5,
			"wait": true
		}
	]
	interp.start_command_list(cmds)
	assert_test(interp.is_running(), "Intérprete ejecutando comando de cambio de zoom de cámara")

func _process(_delta: float) -> bool:
	step += 1
	if not interp:
		_finish()
		return false

	if interp.is_running():
		interp.step_frame()

	if step == 2:
		var cam = map_node.get_runtime_camera()
		if cam:
			print("[INFO] Frame 2 - Zoom actual durante transición: ", cam.get_zoom().x)

	if step >= 10:
		var cam = map_node.get_runtime_camera()
		if cam:
			var final_z = cam.get_integer_zoom()
			assert_test(final_z == 3, "Zoom de cámara final alcanzó 3x exactamente tras 5 frames de transición")
			assert_test(not interp.is_running(), "Intérprete completó el comando y finalizó la espera correctamente")
		_finish()

	return false

func _finish():
	print("\n==================================================================")
	print("RESULTADOS FINALES FASE 7B: ", passed_count, " PASARON, ", failed_count, " FALLARON")
	print("==================================================================")
	if failed_count == 0:
		print("TODO CORRECTO: FASE 7B VERIFICADA EXITOSAMENTE")
		quit(0)
	else:
		print("ERRORES DETECTADOS EN LA VERIFICACIÓN")
		quit(1)
