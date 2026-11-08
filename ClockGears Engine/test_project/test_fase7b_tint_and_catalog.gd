extends SceneTree

# ============================================================================
# TEST AUTOMÁTICO FASE 7B: TEÑIR PANTALLA (PIXEL GPU) Y CATÁLOGO DE COMANDOS
# ============================================================================

var passed_count: int = 0
var failed_count: int = 0
var step: int = 0

var vp: SubViewport
var bg_rect: ColorRect
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
	print("   TEST FASE 7B: TEÑIR PANTALLA (PIXEL GPU) Y CATÁLOGO EVENTOS    ")
	print("==================================================================")

	# 1. Probar Catálogo de Comandos y Diálogos de Editor
	test_command_catalog_and_editor()

	# 2. Configurar Viewport para test de Teñir Pantalla con lectura de píxeles
	setup_viewport_tint_test()

# ----------------------------------------------------------------------------
# 1. VERIFICACIÓN DE CATÁLOGO Y EDITOR DE EVENTOS
# ----------------------------------------------------------------------------
func test_command_catalog_and_editor():
	print("\n--- 1. Catálogo de Comandos y Diálogos del Editor ---")

	var selector = ClockGearsCommandSelector.new()
	get_root().add_child(selector)
	assert_test(selector != null, "ClockGearsCommandSelector instanciado correctamente")

	var received_cmd = [-1]
	selector.connect("command_selected", func(cmd_id): received_cmd[0] = cmd_id)

	# Verificar emisión de IDs de comandos nuevos
	var test_cmd_ids = [
		ClockGearsCommandSelector.CMD_SHOW_PICTURE,
		ClockGearsCommandSelector.CMD_MOVE_PICTURE,
		ClockGearsCommandSelector.CMD_ROTATE_PICTURE,
		ClockGearsCommandSelector.CMD_TINT_PICTURE,
		ClockGearsCommandSelector.CMD_ERASE_PICTURE,
		ClockGearsCommandSelector.CMD_SHAKE_SCREEN,
		ClockGearsCommandSelector.CMD_WEATHER,
		ClockGearsCommandSelector.CMD_PLAY_BGS,
		ClockGearsCommandSelector.CMD_PLAY_ME,
		ClockGearsCommandSelector.CMD_STOP_FADE_BGM,
		ClockGearsCommandSelector.CMD_SAVE_BGM,
		ClockGearsCommandSelector.CMD_RESTORE_BGM,
		ClockGearsCommandSelector.CMD_AUDIO_STEMS,
		ClockGearsCommandSelector.CMD_PLAY_CUTSCENE,
		ClockGearsCommandSelector.CMD_CREATE_CUTSCENE
	]

	for cid in test_cmd_ids:
		selector.emit_signal("command_selected", cid)
		assert_test(received_cmd[0] == cid, "Comando ID %d despachado correctamente desde catálogo" % cid)

	selector.queue_free()

	# Probar Editor de Eventos y persistencia de comandos
	var editor = ClockGearsEventEditorDialog.new()
	get_root().add_child(editor)
	assert_test(editor != null, "ClockGearsEventEditorDialog instanciado con todos los subdiálogos")

	var evt = ClockGearsEvent.new()
	evt.set_event_name("TestEvent7B")

	var page_cmds: Array = [
		{ "type": "show_picture", "id": 1, "path": "res://pictures/hero.png", "x": 100, "y": 80, "scale_x": 100, "scale_y": 100, "opacity": 1.0 },
		{ "type": "move_picture", "id": 1, "x": 200, "y": 150, "duration": 45, "easing": 1 },
		{ "type": "rotate_picture", "id": 1, "speed": 180.0 },
		{ "type": "tint_picture", "id": 1, "preset": "Oscuro / Noche", "duration": 30 },
		{ "type": "erase_picture", "id": 1 },
		{ "type": "shake_screen", "power": 6, "speed": 8, "duration": 40 },
		{ "type": "weather", "weather": "rain", "power": 7, "duration": 60 },
		{ "type": "play_bgs", "file": "res://audio/bgs/rain.ogg", "volume": 85, "pitch": 100 },
		{ "type": "play_me", "file": "res://audio/me/fanfare.ogg", "volume": 90, "pitch": 100 },
		{ "type": "stop_fade_bgm", "action": "fade", "duration": 60 },
		{ "type": "save_bgm" },
		{ "type": "restore_bgm", "duration": 30 },
		{ "type": "audio_stems", "stem": 2, "volume": 0.75, "fade": 0.5 },
		{ "type": "play_cutscene", "path": "res://cutscenes/intro.tres" },
		{ "type": "play_cutscene", "name": "Cinematica_Test", "cutscene_dict": { "name": "Cinematica_Test", "duration": 8.0, "letterbox": true, "keyframes": [] } }
	]

	evt.set_commands(page_cmds)
	editor.edit_event(evt)

	var read_cmds = evt.get_commands()
	assert_test(read_cmds.size() == 15, "15 comandos multimedia registrados y persistidos en el evento")

	editor.queue_free()
	evt.queue_free()

# ----------------------------------------------------------------------------
# 2. CONFIGURACIÓN DEL VIEWPORT PARA TEST DE TEÑIR PANTALLA
# ----------------------------------------------------------------------------
func setup_viewport_tint_test():
	print("\n--- 2. Teñir Pantalla: Lectura de Píxeles en Viewport (GPU) ---")

	vp = SubViewport.new()
	vp.size = Vector2i(320, 240)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	vp.render_target_clear_mode = SubViewport.CLEAR_MODE_ALWAYS
	get_root().add_child(vp)

	# Fondo gris claro simulando mapa y personajes (capa 0)
	bg_rect = ColorRect.new()
	bg_rect.size = Vector2(320, 240)
	bg_rect.color = Color(0.8, 0.8, 0.8, 1.0) # Gris claro
	vp.add_child(bg_rect)

	# Instanciar Intérprete dentro del viewport
	interp = ClockGearsEventInterpreter.new()
	vp.add_child(interp)
	interp.setup_interpreter(null, null, null)

	# Verificar capas del sistema ClockGears
	var tint_layer = interp.get_screen_tint_layer()
	assert_test(tint_layer != null, "screen_tint_layer inicializada en el interprete")
	if tint_layer:
		assert_test(tint_layer.layer == 10, "screen_tint_layer configurada en Capa 10 (encima del mapa, bajo ventana de mensajes)")

	var ui_lay = interp.get_ui_layer()
	assert_test(ui_lay != null, "ui_layer inicializada en el intérprete")
	if ui_lay:
		assert_test(ui_lay.layer == 100, "ui_layer está en Capa 100 (fuera del alcance del tinte de pantalla)")

	var msg_win = interp.get_message_window()
	assert_test(msg_win != null, "Ventana de mensajes inicializada en el intérprete")

	# Ejecutar comando de Teñir Pantalla con preset 'Oscuro / Noche'
	var tint_cmd = {
		"type": "tint",
		"preset": "Oscuro / Noche",
		"duration": 2 # 2 frames para completar interpolación
	}
	interp.start_command_list([tint_cmd], null)

func _process(delta: float) -> bool:
	step += 1

	if step <= 4:
		if interp:
			interp.step_frame()

	if step == 2:
		# En el frame 2, verificar color de tinte en proceso
		var tint_rect = interp.get_screen_tint_rect()
		assert_test(tint_rect != null, "screen_tint_rect nodo ColorRect existe")
		if tint_rect:
			assert_test(tint_rect.is_visible(), "screen_tint_rect visible en el viewport")
			assert_test(tint_rect.size.x >= 320 && tint_rect.size.y >= 240, "screen_tint_rect cubre todo el viewport (no tamaño 0)")

	elif step == 5:
		# Tras completar la duración, realizar LECTURA REAL DE PÍXEL EN VIEWPORT
		var tex = vp.get_texture()
		assert_test(tex != null, "ViewportTexture válida obtenida del Viewport")
		if tex:
			var img = tex.get_image()
			assert_test(img != null && !img.is_empty(), "Imagen extraída de ViewportTexture")
			if img:
				var pixel_map = img.get_pixel(160, 120)
				print("  [Viewport Pixel] Píxel leído en mapa con tinte 'Noche': ", pixel_map)

				# Verificar que el tinte es de TONO AZUL OSCURO:
				# Con Color(0.04, 0.08, 0.35, 0.65) sobre gris (0.8, 0.8, 0.8):
				# Canal azul (B) DEBE predominar sobre rojo (R) y verde (G)
				assert_test(pixel_map.b > pixel_map.r, "Tono AZUL dominante sobre Rojo (B: %.3f > R: %.3f)" % [pixel_map.b, pixel_map.r])
				assert_test(pixel_map.b > pixel_map.g, "Tono AZUL dominante sobre Verde (B: %.3f > G: %.3f)" % [pixel_map.b, pixel_map.g])
				assert_test(pixel_map.r < 0.45 && pixel_map.g < 0.45, "Oscurecimiento nocturno efectivo (R y G atenuados < 0.45)")

	# Simular 10 frames adicionales para comprobar que el tinte NO SE REVIERTE
	elif step == 15:
		print("\n--- Verificación de Persistencia del Tinte (Sin Reversión) ---")
		if interp:
			interp.step_frame()
		var cur_col = interp.get_screen_tint_color()
		print("  Color del tinte en frame 15: ", cur_col)
		assert_test(cur_col.a > 0.5, "El tinte de pantalla NO se revierte a transparente al terminar la duración (Alpha=%.2f)" % cur_col.a)
		assert_test(cur_col.b > cur_col.r, "Tinte mantiene tono azul oscuro target permanentemente (B: %.3f > R: %.3f)" % [cur_col.b, cur_col.r])

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
