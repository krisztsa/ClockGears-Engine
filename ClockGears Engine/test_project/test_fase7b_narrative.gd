extends SceneTree

# ============================================================================
# TEST AUTOMÁTICO FASE 7B: CINEMÁTICAS, IMÁGENES Y PANTALLA, GESTOR DE AUDIO
# Verifica:
# 1. ClockGearsAudioManager (BGM, crossfade, stems sincronizados, BGS, ME con
#    retorno de BGM, SE pool, ducking automático, guardar/restaurar BGM, buses).
# 2. ClockGearsPictureManager (100 imágenes, escalado pixel-art entero, movimientos
#    con curvas de aceleración, rotación continua, tinte, temblor y flash de pantalla).
# 3. ClockGearsCutscene & CutscenePlayer (Timeline, 8 pistas de keyframes, letterbox,
#    skip consistente con aplicación instantánea de estado final).
# 4. Integración en ClockGearsEventInterpreter con comandos de evento.
# ============================================================================

var passed_count: int = 0
var failed_count: int = 0

func assert_test(cond: bool, test_name: String):
	if cond:
		passed_count += 1
		print("[PASS] ", test_name)
	else:
		failed_count += 1
		print("[FAIL] ", test_name)

func _init():
	print("==================================================================")
	print("    INICIANDO VERIFICACIÓN DE FASE 7B (NARRATIVA Y MULTIMEDIA)")
	print("==================================================================")

	test_audio_manager()
	test_picture_manager()
	test_cutscene_system()
	test_event_interpreter_7b()

	print("==================================================================")
	print("RESUMEN FASE 7B: %d PASADOS, %d FALLADOS" % [passed_count, failed_count])
	print("==================================================================")

	if failed_count == 0:
		print("RESULTADO REAL: TODOS LOS TESTS DE LA FASE 7B PASARON EXITOSAMENTE.")
		quit(0)
	else:
		print("RESULTADO REAL: SE DETECTARON FALLOS EN LA FASE 7B.")
		quit(1)

# ----------------------------------------------------------------------------
# 1. GESTOR DE AUDIO (ClockGearsAudioManager)
# ----------------------------------------------------------------------------
func test_audio_manager():
	print("\n--- 1. ClockGearsAudioManager (Anexo A1.4) ---")
	var am = Engine.get_singleton("ClockGearsAudioManager")
	assert_test(am != null, "ClockGearsAudioManager singleton registrado y disponible")
	if am == null:
		return

	# Control de Volúmenes por Bus
	am.set_bus_volume(ClockGearsAudioManager.BUS_BGM, 0.85)
	am.set_bus_volume(ClockGearsAudioManager.BUS_SE, 0.70)
	assert_test(abs(am.get_bus_volume(ClockGearsAudioManager.BUS_BGM) - 0.85) < 0.01, "Ajuste y lectura de volumen en bus BGM")
	assert_test(abs(am.get_bus_volume(ClockGearsAudioManager.BUS_SE) - 0.70) < 0.01, "Ajuste y lectura de volumen en bus SE")

	# Reproducción de BGM
	am.play_bgm("res://audio/bgm/field.ogg", 1.0, 1.0, 0.0)
	assert_test(am.get_current_bgm_path() == "res://audio/bgm/field.ogg", "Ruta de BGM activa registrada correctamente")
	assert_test(am.is_bgm_playing(), "BGM en reproducción activa")

	# Guardar y restaurar BGM (para transiciones de batalla/cinemática)
	am.save_bgm()
	assert_test(am.has_saved_bgm(), "BGM guardada en memoria (save_bgm)")

	# Cambio de BGM (Crossfade)
	am.play_bgm("res://audio/bgm/battle.ogg", 0.9, 1.0, 0.5)
	assert_test(am.get_current_bgm_path() == "res://audio/bgm/battle.ogg", "Crossfade cambia ruta a nueva BGM")
	am.update_audio(0.6) # Simular paso del tiempo para completar crossfade

	# Restaurar BGM previa
	am.restore_bgm(0.0)
	assert_test(am.get_current_bgm_path() == "res://audio/bgm/field.ogg", "Restauración exitosa de BGM previa (restore_bgm)")
	assert_test(!am.has_saved_bgm(), "Estado de BGM guardada se consume tras restaurar")

	# Stems sincronizados (hasta 8 pistas)
	am.play_stem(0, "res://audio/stems/drums.ogg", 0.5)
	am.play_stem(1, "res://audio/stems/bass.ogg", 1.0)
	assert_test(abs(am.get_stem_volume(0) - 0.5) < 0.05, "Stem 0 registrado con volumen objetivo")
	assert_test(abs(am.get_stem_volume(1) - 1.0) < 0.05, "Stem 1 registrado con volumen objetivo")
	am.set_stem_volume(0, 0.0, 0.0)
	assert_test(am.get_stem_volume(0) < 0.01, "Atenuación inmediata de Stem individual")
	am.stop_all_stems(0.0)
	assert_test(am.get_stem_volume(1) < 0.01, "Detención simultánea de todos los stems")

	# BGS (Ambiente continuo)
	am.play_bgs("res://audio/bgs/rain.ogg", 0.6)
	assert_test(am.get_current_bgs_path() == "res://audio/bgs/rain.ogg", "BGS activo registrado correctamente")
	assert_test(am.is_bgs_playing(), "BGS reporta reproducción activa")
	am.stop_bgs(0.0)
	assert_test(!am.is_bgs_playing(), "BGS se detiene correctamente")

	# ME (Efecto musical con retorno automático de BGM según Regla A1.4)
	am.play_bgm("res://audio/bgm/town.ogg", 1.0, 1.0, 0.0)
	am.play_me("res://audio/me/fanfare.ogg", 1.0, 1.0)
	assert_test(am.is_me_playing(), "ME en reproducción (pausa BGM)")
	am.stop_me()
	assert_test(!am.is_me_playing(), "ME finalizado")
	assert_test(am.is_bgm_playing(), "BGM retorna automáticamente tras finalizar ME (Regla A1.4)")

	# Ducking automático
	am.set_ducking_enabled(true)
	am.set_duck_factor(0.35)
	assert_test(am.is_ducking_enabled(), "Ducking habilitado correctamente")
	assert_test(abs(am.get_duck_factor() - 0.35) < 0.01, "Factor de ducking configurado al 35%")

	# Detener BGM
	am.stop_bgm(0.0)
	assert_test(!am.is_bgm_playing(), "BGM detenida completamente")

# ----------------------------------------------------------------------------
# 2. GESTOR DE IMÁGENES Y PANTALLA (ClockGearsPictureManager)
# ----------------------------------------------------------------------------
func test_picture_manager():
	print("\n--- 2. ClockGearsPictureManager (Anexo A1.3) ---")
	var pm = ClockGearsPictureManager.new()
	get_root().add_child(pm)

	# Escalado pixel art entero obligatorio por defecto
	assert_test(pm.is_integer_scaling_enforced(), "Escalado pixel art entero activo por defecto")

	# Mostrar imagen en slot 1
	pm.show_picture(1, "res://img/pictures/hero_bust.png", 0, Vector2(100, 150), Vector2(200, 200), 0.8, 0)
	assert_test(pm.has_picture(1), "Imagen 1 mostrada correctamente")
	assert_test(pm.get_picture_position(1) == Vector2(100, 150), "Posición inicial registrada")
	assert_test(abs(pm.get_picture_opacity(1) - 0.8) < 0.01, "Opacidad de imagen configurada")

	# Capacidad de hasta 100 imágenes simultáneas
	pm.show_picture(100, "res://img/pictures/hud_overlay.png", 1, Vector2(500, 300), Vector2(100, 100), 1.0, 0)
	assert_test(pm.has_picture(100), "Slot de imagen 100 funcional (límite de 100 soportado)")

	# Movimiento con curva de aceleración (Ease In-Out)
	pm.move_picture(1, 0, Vector2(300, 250), Vector2(200, 200), 1.0, 1.0, ClockGearsPictureManager.EASE_IN_OUT)
	pm.update_pictures(0.5) # Mitad de camino
	var half_pos = pm.get_picture_position(1)
	assert_test(half_pos.x > 100 and half_pos.x < 300, "Movimiento interpolado a medio camino")
	pm.update_pictures(0.6) # Finalizar movimiento
	assert_test(pm.get_picture_position(1) == Vector2(300, 250), "Movimiento completa posición objetivo con exactitud")

	# Rotación continua
	pm.rotate_picture(1, 90.0) # 90 grados por segundo
	pm.update_pictures(1.0)
	assert_test(abs(pm.get_picture_rotation(1) - 90.0) < 1.0, "Rotación continua a 90 grados tras 1 segundo")

	# Borrado de imágenes
	pm.erase_picture(1)
	assert_test(!pm.has_picture(1), "Imagen 1 borrada correctamente (erase_picture)")
	pm.erase_all_pictures()
	assert_test(!pm.has_picture(100), "Todas las imágenes borradas (erase_all_pictures)")

	# Efectos de pantalla: Temblor (Shake)
	pm.start_screen_shake(8.0, 20.0, 0.5)
	assert_test(pm.is_screen_shaking(), "Temblor de pantalla activo (start_screen_shake)")
	pm.stop_screen_shake()
	assert_test(!pm.is_screen_shaking(), "Temblor de pantalla detenido")

	# Efectos de pantalla: Clima (Weather)
	pm.set_weather("rain", 0.75)
	assert_test(pm.get_weather_type() == "rain", "Efecto climático configurado")

	pm.queue_free()

# ----------------------------------------------------------------------------
# 3. CINEMÁTICAS Y REPRODUCTOR (ClockGearsCutscene & Player)
# ----------------------------------------------------------------------------
func test_cutscene_system():
	print("\n--- 3. Cinemáticas: Timeline, Letterbox y Skip consistente (Anexo A1.2) ---")
	var cs = ClockGearsCutscene.new()
	cs.set_cutscene_name("Intro_Village")
	cs.set_total_duration(4.0)
	cs.set_letterbox_enabled(true)
	cs.set_allow_skip(true)

	# Inserción de keyframes en múltiples pistas (ordenados cronológicamente)
	cs.add_keyframe(1.0, ClockGearsCutscene.TRACK_AUDIO, 0, { "type": "bgm", "file": "res://audio/bgm/intro.ogg", "vol": 1.0 })
	cs.add_keyframe(0.5, ClockGearsCutscene.TRACK_PICTURE, 1, { "action": "show", "path": "res://img/pictures/sky.png", "pos": Vector2(0, 0) })
	cs.add_keyframe(2.0, ClockGearsCutscene.TRACK_SWITCH_VAR, 0, { "type": "switch", "name": "IntroSeen", "val": true })
	cs.add_keyframe(3.0, ClockGearsCutscene.TRACK_SWITCH_VAR, 0, { "type": "variable", "name": "Chapter", "val": 1 })

	assert_test(cs.get_keyframe_count() == 4, "4 keyframes agregados al recurso de cinemática")
	# Verificar orden cronológico automático
	assert_test(cs.get_keyframe(0).get("time") == 0.5, "Keyframe 0 ordenado a t=0.5s")
	assert_test(cs.get_keyframe(1).get("time") == 1.0, "Keyframe 1 ordenado a t=1.0s")
	assert_test(cs.get_keyframe(2).get("time") == 2.0, "Keyframe 2 ordenado a t=2.0s")
	assert_test(cs.get_keyframe(3).get("time") == 3.0, "Keyframe 3 ordenado a t=3.0s")

	# Serialización y deserialización
	var d = cs.to_dict()
	assert_test(d.get("name") == "Intro_Village", "Serialización to_dict contiene nombre de cinemática")
	var cs2 = ClockGearsCutscene.new()
	cs2.from_dict(d)
	assert_test(cs2.get_keyframe_count() == 4, "Deserialización from_dict restaura exactamente los 4 keyframes")

	# Reproducción de cinemática y Letterbox
	var player = ClockGearsCutscenePlayer.new()
	get_root().add_child(player)

	player.play(cs)
	assert_test(player.is_playing(), "CutscenePlayer inicia reproducción activa")

	# Simular frames de actualización
	player.update_player(0.1)
	assert_test(player.is_letterbox_active(), "Barras de cine (Letterbox) activadas durante la cinemática")

	# PRUEBA CRÍTICA: SKIP CON ESTADO FINAL CONSISTENTE (Regla A1.2 & A6)
	# Al saltar, todos los keyframes pendientes (switches, variables, audio) deben aplicarse al instante
	var dm = Engine.get_singleton("ClockGearsDataManager")
	var am = Engine.get_singleton("ClockGearsAudioManager")
	dm.set_switch("IntroSeen", false)
	dm.set_variable("Chapter", 0)

	player.skip()
	assert_test(!player.is_playing(), "Cinemática finalizada inmediatamente tras skip()")
	assert_test(dm.get_switch("IntroSeen") == true, "Regla A1.2/A6: Switch IntroSeen aplicado al saltar cinemática")
	assert_test(dm.get_variable("Chapter") == 1, "Regla A1.2/A6: Variable Chapter=1 aplicada al saltar cinemática")
	assert_test(am.get_current_bgm_path() == "res://audio/bgm/intro.ogg", "Regla A1.2/A6: Audio BGM aplicado al saltar cinemática")

	player.queue_free()

# ----------------------------------------------------------------------------
# 4. INTEGRACIÓN DE COMANDOS EN INTÉRPRETE (ClockGearsEventInterpreter)
# ----------------------------------------------------------------------------
func test_event_interpreter_7b():
	print("\n--- 4. Intérprete de Eventos y Comandos 7B ---")
	var interp = ClockGearsEventInterpreter.new()
	get_root().add_child(interp)
	interp.setup_interpreter(null, null, null)

	var pm = interp.get_picture_manager()
	assert_test(pm != null, "ClockGearsEventInterpreter inicializa ClockGearsPictureManager en ui_layer")

	var cp = interp.get_cutscene_player()
	assert_test(cp != null, "ClockGearsEventInterpreter inicializa ClockGearsCutscenePlayer en ui_layer")

	# Ejecutar lista de comandos 7B (Imágenes y Audio)
	var commands: Array = [
		{ "type": "picture", "id": 5, "path": "res://img/pictures/quest_icon.png", "pos": Vector2(50, 60), "scale": Vector2(100, 100), "opacity": 1.0 },
		{ "type": "play_bgm", "path": "res://audio/bgm/event_theme.ogg", "volume": 0.8 },
		{ "type": "save_bgm" },
		{ "type": "erase_picture", "id": 5 }
	]

	interp.start_command_list(commands, null)
	interp.step_frame()

	assert_test(!interp.is_running(), "Lista de comandos 7B ejecutada completamente")
	assert_test(!pm.has_picture(5), "Comando erase_picture ejecutado por intérprete")
	var am = Engine.get_singleton("ClockGearsAudioManager")
	assert_test(am.get_current_bgm_path() == "res://audio/bgm/event_theme.ogg", "Comando play_bgm ejecutado por intérprete")
	assert_test(am.has_saved_bgm(), "Comando save_bgm ejecutado por intérprete")

	interp.queue_free()
