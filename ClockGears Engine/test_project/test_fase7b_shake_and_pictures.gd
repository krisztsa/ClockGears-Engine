extends SceneTree

# ============================================================================
# TEST AUTOMÁTICO FASE 7B: TEMBLOR DE PANTALLA, DURACIÓN DE IMÁGENES Y SELECTOR
# ============================================================================

var passed_count: int = 0
var failed_count: int = 0
var step: int = 0

var vp: SubViewport
var map_node: ClockGearsMap
var interp: ClockGearsEventInterpreter
var initial_map_pixel: Color
var initial_ui_pixel: Color

func assert_test(cond: bool, test_name: String):
	if cond:
		passed_count += 1
		print("[PASS] ", test_name)
	else:
		failed_count += 1
		print("[FAIL] ", test_name)

func _initialize():
	print("==================================================================")
	print("   TEST FASE 7B: TEMBLOR (GPU), DURACIÓN IMÁGENES Y SELECTOR     ")
	print("==================================================================")

	# 2. Tests de Runtime: Duración de Imágenes y Reemplazo de Ranura
	test_picture_durations_and_slot_replacement()

	# 3. Test de Viewport GPU: Temblor de Pantalla (Mapa se mueve, UI estática)
	setup_viewport_shake_test()

# ----------------------------------------------------------------------------
# 1. SELECTOR VISUAL INTERACTIVO Y ANCLAJES (ITEM 3)
# ----------------------------------------------------------------------------
func test_visual_picker_and_anchors():
	print("\n--- 1. Selector Visual Interactivo y Anclajes Rápidos ---")

	var picker = ClockGearsPictureVisualPicker.new()
	get_root().add_child(picker)
	assert_test(picker != null, "ClockGearsPictureVisualPicker instanciado correctamente")

	# Configurar SpinBoxes simuladas para verificar sincronización bidireccional
	var spin_x = SpinBox.new()
	var spin_y = SpinBox.new()
	var spin_sx = SpinBox.new()
	var spin_sy = SpinBox.new()
	spin_x.set_min(-9999); spin_x.set_max(9999); spin_x.set_value(0)
	spin_y.set_min(-9999); spin_y.set_max(9999); spin_y.set_value(0)
	spin_sx.set_min(10); spin_sx.set_max(2000); spin_sx.set_value(100)
	spin_sy.set_min(10); spin_sy.set_max(2000); spin_sy.set_value(100)
	get_root().add_child(spin_x)
	get_root().add_child(spin_y)
	get_root().add_child(spin_sx)
	get_root().add_child(spin_sy)

	picker.bind_spinboxes(spin_x, spin_y, spin_sx, spin_sy)

	# Modificar picker visualmente (como al arrastrar en lienzo) -> debe actualizar SpinBoxes
	picker.set_visual_values(320, 180, 200, 200)
	assert_test(int(spin_x.get_value()) == 320 && int(spin_y.get_value()) == 180, "Sincronización Picker -> SpinBoxes de Posición (320, 180)")
	assert_test(int(spin_sx.get_value()) == 200 && int(spin_sy.get_value()) == 200, "Sincronización Picker -> SpinBoxes de Escala (200%, 200%)")

	# Modificar SpinBoxes -> debe actualizar Picker
	spin_x.set_value(150)
	spin_y.set_value(90)
	assert_test(picker.get_pos_x() == 150 && picker.get_pos_y() == 90, "Sincronización SpinBoxes -> Picker (150, 90)")

	# Probar anclajes rápidos en modo Origen Centro (origen = 1)
	picker.set_origin_mode(1) # ORIGIN_CENTER
	picker.quick_anchor_center()
	assert_test(picker.get_pos_x() == 480 && picker.get_pos_y() == 270, "Botón [Centro] ubica en (480, 270) en modo Centro")

	picker.quick_anchor_left()
	assert_test(picker.get_pos_x() < 480, "Botón [Izquierda] alinea hacia la izquierda de la pantalla")

	picker.quick_anchor_right()
	assert_test(picker.get_pos_x() > 480, "Botón [Derecha] alinea hacia la derecha de la pantalla")

	# Probar anclajes rápidos en modo Origen Superior Izquierda (origen = 0)
	picker.set_origin_mode(0) # ORIGIN_TOP_LEFT
	picker.quick_anchor_top()
	assert_test(picker.get_pos_y() == 0, "Botón [Arriba] ubica Y=0 en modo Superior Izquierda")

	# Probar snap a escala entera (Pixel Art)
	picker.set_integer_snap(true)
	assert_test(picker.is_integer_snap() == true, "Snap entero de pixel art activado por defecto")

	picker.queue_free()
	spin_x.queue_free()
	spin_y.queue_free()
	spin_sx.queue_free()
	spin_sy.queue_free()

# ----------------------------------------------------------------------------
# 2. DURACIÓN DE IMÁGENES Y REEMPLAZO DE RANURA (ITEM 2)
# ----------------------------------------------------------------------------
func test_picture_durations_and_slot_replacement():
	print("\n--- 2. Duración de Imágenes y Reemplazo de Ranura ---")

	var pm = ClockGearsPictureManager.new()
	get_root().add_child(pm)

	# Test 2.1: Modo "Hasta que se borre" (por defecto, mode 0)
	pm.show_picture(1, "", 0, Vector2(100, 100), Vector2(100, 100), 1.0, 0, 0, 0.0, false)
	assert_test(pm.has_picture(1), "Ranura 1 activa con modo 'Hasta que se borre'")
	assert_test(pm.get_picture_duration_mode(1) == 0, "Modo de duración registrado como 0 (DUR_UNTIL_ERASED)")

	# Test 2.2: Reemplazo en la misma ranura
	pm.show_picture(1, "", 1, Vector2(250, 150), Vector2(150, 150), 0.8, 0, 1, 0.1, false)
	assert_test(pm.get_picture_position(1) == Vector2(250, 150), "Reemplazo de imagen en ranura 1 actualiza posición")
	assert_test(pm.get_picture_duration_mode(1) == 1, "Reemplazo de imagen en ranura 1 actualiza modo a 'X segundos'")
	assert_test(pm.get_picture_life_timer(1) > 0.0, "Temporizador de vida inicializado para X segundos")

	# Simular paso del tiempo para verificar borrado automático
	pm.update_pictures(0.15) # Exceder los 0.1s
	assert_test(!pm.has_picture(1), "Imagen en ranura 1 borrada automáticamente tras expirar su duración de 0.1s")

	# Test 2.3: Modo "Hasta que termine el siguiente mensaje" (mode 2)
	pm.show_picture(2, "", 0, Vector2(50, 50), Vector2(100, 100), 1.0, 0, 2, 0.0, false)
	assert_test(pm.has_picture(2), "Ranura 2 activa con modo 'Hasta que termine el siguiente mensaje'")
	assert_test(pm.get_picture_duration_mode(2) == 2, "Modo registrado como 2 (DUR_NEXT_MESSAGE)")

	# Simular cierre del mensaje en la UI
	pm.erase_pictures_on_message_close()
	assert_test(!pm.has_picture(2), "Ranura 2 borrada automáticamente al cerrarse el mensaje")

	pm.queue_free()

# ----------------------------------------------------------------------------
# 3. TEMBLOR DE PANTALLA: COMPARACIÓN VIEWPORT GPU (ITEM 1)
# ----------------------------------------------------------------------------
func setup_viewport_shake_test():
	print("\n--- 3. Temblor de Pantalla: Comparación de Viewport entre Frames ---")

	vp = SubViewport.new()
	vp.size = Vector2i(320, 240)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	vp.render_target_clear_mode = SubViewport.CLEAR_MODE_ALWAYS
	get_root().add_child(vp)

	# Simular Mapa con división vertical: blanco a la izquierda, negro a la derecha
	map_node = ClockGearsMap.new()
	map_node.set_name("ClockGearsMapTest")
	vp.add_child(map_node)

	var left_rect = ColorRect.new()
	left_rect.size = Vector2(160, 240)
	left_rect.color = Color(1, 1, 1, 1) # Blanco puro
	map_node.add_child(left_rect)

	var right_rect = ColorRect.new()
	right_rect.position = Vector2(160, 0)
	right_rect.size = Vector2(160, 240)
	right_rect.color = Color(0, 0, 0, 1) # Negro puro
	map_node.add_child(right_rect)

	# Instanciar Intérprete y registrar el mapa
	interp = ClockGearsEventInterpreter.new()
	vp.add_child(interp)
	interp.setup_interpreter(map_node, null, null)

	# Elemento distintivo en ui_layer (Capa 100): Caja verde
	var ui_lay = interp.get_ui_layer()
	assert_test(ui_lay != null && ui_lay.layer == 100, "ui_layer en Capa 100 creada correctamente")

	var ui_box = ColorRect.new()
	ui_box.position = Vector2(150, 10)
	ui_box.size = Vector2(20, 20)
	ui_box.color = Color(0, 1, 0, 1) # Verde puro
	ui_lay.add_child(ui_box)

func _process(delta: float) -> bool:
	step += 1

	if step == 1:
		test_visual_picker_and_anchors()

	if interp:
		interp.step_frame()

	# Paso 3: Viewport ya renderizó buffer base GPU. Capturar estado inicial en reposo.
	if step == 3:
		var tex = vp.get_texture()
		if tex:
			var img = tex.get_image()
			if img:
				initial_map_pixel = img.get_pixel(159, 120)
				initial_ui_pixel = img.get_pixel(160, 20)
				print("  [Step 3 Base] Píxel mapa (159, 120): ", initial_map_pixel, " | Píxel UI (160, 20): ", initial_ui_pixel)
				assert_test(initial_map_pixel.r > 0.9, "Mapa renderizado en reposo (Blanco en X=159)")
				assert_test(initial_ui_pixel.g > 0.9, "UI renderizada en reposo (Verde puro)")

		# Iniciar comando de temblor en paso 3: potencia 8, velocidad 5, duración 25 frames
		var shake_cmd = {
			"type": "shake_screen",
			"power": 8,
			"speed": 5,
			"duration": 25,
			"wait": false
		}
		interp.start_command_list([shake_cmd], null)

	# Paso 5: En pleno temblor
	elif step == 5:
		var off = interp.get_current_shake_offset()
		var is_shaking = interp.is_screen_shaking()
		print("  [Step 5 Shake] Offset actual: ", off, " | Shaking: ", is_shaking, " | Posición Mapa: ", map_node.position)
		assert_test(is_shaking, "Intérprete reporta temblor activo durante la ejecución")
		assert_test(map_node.position == off, "Desplazamiento aplicado directamente al nodo del Mapa")

		var tex = vp.get_texture()
		if tex:
			var img = tex.get_image()
			if img:
				var cur_ui_pixel = img.get_pixel(160, 20)
				print("  [Step 5 Shake] Píxel UI durante temblor: ", cur_ui_pixel)
				# La UI NO se mueve: el píxel verde en ui_layer (160, 20) permanece 100% idéntico
				assert_test(cur_ui_pixel == initial_ui_pixel, "La UI / Ventana de mensajes NO tiembla (píxel idéntico a reposo)")

	# Paso 35: Conclusión del temblor
	elif step == 35:
		var final_off = interp.get_current_shake_offset()
		print("\n--- Verificación de Finalización del Temblor ---")
		print("  Offset final tras concluir duración: ", final_off, " | Posición final mapa: ", map_node.position)
		assert_test(!interp.is_screen_shaking(), "El temblor concluyó al expirar la duración")
		assert_test(final_off == Vector2(0, 0), "Offset retorna a (0, 0) sin deriva residual")
		assert_test(map_node.position == Vector2(0, 0), "El Mapa regresa exactamente a su posición base (0, 0)")

		print("\n==================================================================")
		print("RESUMEN TEST FASE 7B: %d PASADOS, %d FALLADOS" % [passed_count, failed_count])
		print("==================================================================")

		if failed_count == 0:
			print("RESULTADO REAL: TODOS LOS TESTS PASARON SATISFACTORIAMENTE.")
			quit(0)
		else:
			print("RESULTADO REAL: SE DETECTARON FALLOS EN EL TEST.")
			quit(1)

	return false
