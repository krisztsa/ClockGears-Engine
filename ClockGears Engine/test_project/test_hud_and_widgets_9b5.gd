extends SceneTree

var passed: int = 0
var failed: int = 0

func assert_true(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		push_error("[FAIL] " + msg)
		print("[FAIL] ", msg)

func assert_eq(a, b, msg: String) -> void:
	if a == b:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		var d = "%s (obtenido: %s, esperado: %s)" % [msg, str(a), str(b)]
		push_error("[FAIL] " + d)
		print("[FAIL] ", d)

func assert_near(a: float, b: float, tol: float, msg: String) -> void:
	if abs(a - b) <= tol:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		var d = "%s (obtenido: %.3f, esperado: %.3f)" % [msg, a, b]
		push_error("[FAIL] " + d)
		print("[FAIL] ", d)

func _pump_frames(count: int) -> void:
	for _i in range(count):
		await process_frame

func _cleanup_dir(abs_path: String) -> void:
	if not DirAccess.dir_exists_absolute(abs_path):
		return
	var d = DirAccess.open(abs_path)
	if d == null:
		return
	d.list_dir_begin()
	var fn = d.get_next()
	while fn != "":
		if fn != "." and fn != "..":
			var sub = abs_path.path_join(fn)
			if d.current_is_dir():
				_cleanup_dir(sub)
				DirAccess.remove_absolute(sub)
			else:
				DirAccess.remove_absolute(sub)
		fn = d.get_next()
	d.list_dir_end()
	DirAccess.remove_absolute(abs_path)

func _init() -> void:
	call_deferred("_run_all")

func _run_all() -> void:
	print("=== INICIANDO TEST TANDA 5: HUD Y WIDGETS (LOGICA Y DATOS) ===")
	var dm = ClockGearsDataManager
	var sc = ClockGearsSystemCatalog

	# 0. Verificar comando y tipo de ventana registrados
	var cmds = sc.get_command_definitions()
	var found_hud_cmd = false
	for c in cmds:
		if int(c.get("code", 0)) == 360:
			found_hud_cmd = true
	assert_true(found_hud_cmd, "Comando CG_CMD_CHANGE_HUD_VISIBILITY (360) registrado en catalogo")

	var wtypes = dm.get_registered_window_types()
	var found_hud_wt = false
	for wt in wtypes:
		if String(wt.get("id", "")) == "hud":
			found_hud_wt = true
	assert_true(found_hud_wt, "Tipo de ventana 'hud' registrado en asignacion de estilos")

	# 1. Regla Cero + Retrocompatibilidad proyectos antiguos sin hud.json
	var tmp_root = OS.get_user_data_dir().path_join("cg_hud_tanda5_test")
	_cleanup_dir(tmp_root)
	DirAccess.make_dir_recursive_absolute(tmp_root)
	var save_root = tmp_root.path_join("saves")
	DirAccess.make_dir_recursive_absolute(save_root)
	dm.set_save_directory(save_root)

	dm.init_empty()
	assert_eq(dm.get_hud_widgets().size(), 0, "Proyecto nuevo inicia con HUD vacio (0 widgets)")

	var legacy_dir = tmp_root.path_join("legacy_proj")
	DirAccess.make_dir_recursive_absolute(legacy_dir)
	assert_true(dm.save_all(legacy_dir), "Guardar proyecto temporal")
	var hud_json_path = legacy_dir.path_join("hud.json")
	if FileAccess.file_exists(hud_json_path):
		DirAccess.remove_absolute(hud_json_path)
	if FileAccess.file_exists(hud_json_path + ".bak"):
		DirAccess.remove_absolute(hud_json_path + ".bak")
	dm.create_hud_widget({"type": "bar", "name": "Temp"})
	assert_true(dm.load_all(legacy_dir), "Cargar proyecto antiguo sin hud.json")
	assert_eq(dm.get_hud_widgets().size(), 0, "Proyecto antiguo sin hud.json mantiene HUD vacio sin cambios visuales")

	# 2. Preparar proyecto de prueba completo con Actor, Pocion, Imagen y Moneda
	dm.init_empty()
	dm.set_currency_unit("Monedas")
	var cat_actors = 0 # CAT_ACTORS
	var cat_classes = 1 # CAT_CLASSES
	var cat_items = 3 # CAT_ITEMS
	var class_id = dm.create_record(cat_classes, {
		"name": "Aventurero",
		"exp_base": 30,
		"exp_extra": 20,
		"exp_acc_a": 30,
		"exp_acc_b": 30
	})
	var actor_id = dm.create_record(cat_actors, {
		"name": "Heroe",
		"class_id": class_id,
		"level": 1,
		"hp": 100,
		"mp": 30,
		"atk": 15,
		"def": 10
	})
	var img_test_path = tmp_root.path_join("emblem.png")
	var emblem_img = Image.create(16, 16, false, Image.FORMAT_RGBA8)
	emblem_img.fill(Color(0.2, 0.7, 0.95, 1.0))
	emblem_img.save_png(img_test_path)

	var potion_id = dm.create_record(cat_items, {
		"name": "Pocion",
		"type": 0,
		"secondary_type": 7, # 1 aliado
		"tertiary_type": 0,  # Siempre
		"consumable": true,
		"icon_path": img_test_path,
		"effects": [
			{"code": 11, "data_id": 0, "value1": 0, "value2": 30} # +30 HP
		]
	})
	var potion_key = "items:" + str(potion_id)

	# Crear todos los tipos de widget requeridos, cada uno en su zona sin solaparse en 960x540
	var hp_bar_id = dm.create_hud_widget({
		"name": "BarraVida",
		"type": "bar",
		"value_source": "hp",
		"anchor": "top_left",
		"offset_x": 16,
		"offset_y": 16,
		"width": 200,
		"height": 28,
		"window_style_id": "rounded",
		"show_numbers": true,
		"layer_order": 5
	})
	var hearts_id = dm.create_hud_widget({
		"name": "CorazonesVida",
		"type": "hearts",
		"anchor": "top_left",
		"offset_x": 16,
		"offset_y": 54,
		"width": 160,
		"height": 52,
		"hp_per_heart": 20,
		"max_hearts_per_row": 5,
		"max_rows": 2,
		"heart_size": 20,
		"spacing": 4,
		"layer_order": 6
	})
	var currency_id = dm.create_hud_widget({
		"name": "ContadorMonedas",
		"type": "currency",
		"anchor": "top_right",
		"offset_x": -16,
		"offset_y": 16,
		"width": 180,
		"height": 34,
		"window_style_id": "rect",
		"show_icon": true,
		"show_currency_name": true
	})
	var potion_widget_id = dm.create_hud_widget({
		"name": "ContadorPociones",
		"type": "item",
		"item_key": potion_key,
		"hide_when_zero": true,
		"anchor": "bottom_left",
		"offset_x": 16,
		"offset_y": -16,
		"width": 140,
		"height": 36,
		"window_style_id": "rounded"
	})
	var var_text_id = dm.create_hud_widget({
		"name": "TextoPuntos",
		"type": "text",
		"text_mode": "variable",
		"variable_name": "score",
		"text": "Pts: {val}",
		"anchor": "top_center",
		"offset_x": 0,
		"offset_y": 16,
		"width": 140,
		"height": 30,
		"hide_during_messages_and_menus": false
	})
	var emblem_widget_id = dm.create_hud_widget({
		"name": "Emblema",
		"type": "image",
		"image_path": img_test_path,
		"anchor": "bottom_right",
		"offset_x": -16,
		"offset_y": -16,
		"width": 48,
		"height": 48,
		"visibility_mode": "switch",
		"visibility_switch": "mostrar_emblema"
	})

	# Verificar persistencia en hud.json
	var full_dir = tmp_root.path_join("full_proj")
	DirAccess.make_dir_recursive_absolute(full_dir)
	assert_true(dm.save_all(full_dir), "Guardar proyecto con 6 widgets en hud.json")
	assert_true(FileAccess.file_exists(full_dir.path_join("hud.json")), "Archivo hud.json creado")
	dm.init_empty()
	assert_true(dm.load_all(full_dir), "Recargar proyecto desde disco")
	assert_eq(dm.get_hud_widgets().size(), 6, "Los 6 widgets cargan correctamente desde hud.json")

	dm.reset_runtime_state()

	# 3. Instanciar entorno runtime con Player + EventInterpreter
	var interp = ClockGearsEventInterpreter.new()
	var player = ClockGearsPlayer.new()
	player.load_actor_data(actor_id)
	root.add_child(player)
	root.add_child(interp)
	interp.setup_interpreter(null, player, null)
	await _pump_frames(3)

	var hud = interp.get_hud_overlay()
	assert_true(hud != null, "ClockGearsHudOverlay instanciado en runtime")
	assert_eq(hud.get_widget_count(), 6, "HUD overlay construye los 6 widgets")

	# Estado inicial
	var st_bar = hud.get_widget_state(hp_bar_id)
	assert_eq(int(st_bar.get("value", -1)), 100, "Barra HP inicia en 100")
	assert_eq(int(st_bar.get("max", -1)), 100, "Barra HP max en 100")
	assert_eq(String(st_bar.get("text", "")), "100 / 100", "Barra HP muestra numeros iniciales")

	var st_hearts = hud.get_widget_state(hearts_id)
	assert_eq(int(st_hearts.get("total_hearts", 0)), 5, "100 HP / 20 por corazon = 5 corazones")
	assert_eq(int(st_hearts.get("full_hearts", 0)), 5, "5 corazones llenos al inicio")
	assert_eq(int(st_hearts.get("half_hearts", 0)), 0, "0 medios corazones al inicio")

	var st_cur = hud.get_widget_state(currency_id)
	assert_eq(int(st_cur.get("gold", -1)), 0, "Monedas inician en 0")
	assert_eq(String(st_cur.get("text", "")), "0 Monedas", "Contador muestra valor + unidad monetaria")

	var st_pot = hud.get_widget_state(potion_widget_id)
	assert_true(not bool(st_pot.get("visible", true)), "Contador de pociones con hide_when_zero se oculta en 0")

	var st_emblem = hud.get_widget_state(emblem_widget_id)
	assert_true(not bool(st_emblem.get("visible", true)), "Widget por interruptor oculto al inicio")

	# 4. CERO actualizaciones en frames inactivos
	hud.reset_update_counters()
	await _pump_frames(90)
	assert_eq(hud.get_total_update_count(), 0, "90 frames inactivos generan 0 actualizaciones de widgets")

	# 5. Al recibir danio real ARPG (take_arpg_hit), barra y corazones se actualizan por evento (y otros no)
	hud.reset_update_counters()
	player.take_arpg_hit(30)
	st_bar = hud.get_widget_state(hp_bar_id)
	st_hearts = hud.get_widget_state(hearts_id)
	assert_eq(int(st_bar.get("value", -1)), 70, "Tras recibir 30 de dano, barra HP marca 70")
	assert_near(float(st_bar.get("ratio", 0.0)), 0.7, 0.01, "Ratio de barra HP es 0.70")
	assert_eq(int(st_hearts.get("full_hearts", -1)), 3, "Con 70 HP (20/corazon) hay 3 corazones llenos")
	assert_eq(int(st_hearts.get("half_hearts", -1)), 1, "Con 70 HP hay 1 medio corazon")
	assert_eq(int(st_hearts.get("empty_hearts", -1)), 1, "Con 70 HP hay 1 corazon vacio")
	assert_eq(hud.get_widget_update_count(hp_bar_id), 1, "Solo 1 actualizacion en barra HP por el dano")
	assert_eq(hud.get_widget_update_count(hearts_id), 1, "Solo 1 actualizacion en corazones por el dano")
	assert_eq(hud.get_widget_update_count(currency_id), 0, "Monedas NO se actualiza al recibir dano")

	# 6. Ganar monedas y ganar/usar pocion
	hud.reset_update_counters()
	dm.change_gold(150)
	st_cur = hud.get_widget_state(currency_id)
	assert_eq(int(st_cur.get("gold", -1)), 150, "Monedas actualizadas a 150")
	assert_eq(String(st_cur.get("text", "")), "150 Monedas", "Texto de contador actualizado a '150 Monedas'")
	assert_eq(hud.get_widget_update_count(currency_id), 1, "Monedas actualiza exactamente 1 vez")
	assert_eq(hud.get_widget_update_count(hp_bar_id), 0, "Barra HP NO actualiza cuando cambian monedas")

	dm.change_item(potion_key, 1)
	st_pot = hud.get_widget_state(potion_widget_id)
	assert_true(bool(st_pot.get("visible", false)), "Pocion aparece cuando cantidad sube a 1")
	assert_eq(int(st_pot.get("qty", 0)), 1, "Contador muestra 1 pocion")
	assert_eq(String(st_pot.get("text", "")), "x1", "Texto del objeto muestra 'x1'")

	# Usar la pocion (cura +30 HP => 100/100 HP, y baja pocion a 0 => desaparece)
	assert_true(dm.use_item_on_actor(potion_key, actor_id), "Usar pocion sobre heroe")
	st_bar = hud.get_widget_state(hp_bar_id)
	st_hearts = hud.get_widget_state(hearts_id)
	st_pot = hud.get_widget_state(potion_widget_id)
	assert_eq(int(st_bar.get("value", -1)), 100, "Barra HP recupera a 100 al usar pocion")
	assert_eq(int(st_hearts.get("full_hearts", -1)), 5, "Corazones vuelven a 5 llenos")
	assert_eq(int(st_pot.get("qty", -1)), 0, "Pociones bajan a 0 al usarla")
	assert_true(not bool(st_pot.get("visible", true)), "Contador de pociones se vuelve a ocultar al llegar a 0")

	# 7. Variable en widget Texto y Switch en widget Imagen
	dm.set_variable("score", 420)
	var st_txt = hud.get_widget_state(var_text_id)
	assert_eq(String(st_txt.get("text", "")), "Pts: 420", "Widget Texto en modo variable muestra 'Pts: 420'")

	dm.set_switch("mostrar_emblema", true)
	st_emblem = hud.get_widget_state(emblem_widget_id)
	assert_true(bool(st_emblem.get("visible", false)), "Widget por interruptor se muestra cuando interruptor pasa a true")

	# 8. Ocultacion automatica durante mensajes y menu
	interp.start_command_list([
		{"code": 101, "type": "message", "text": "Hola mundo"}
	])
	await _pump_frames(2)
	assert_true(not bool(hud.get_widget_state(hp_bar_id).get("visible", true)), "HUD (hide_during_messages_and_menus=true) se oculta durante mensaje")
	assert_true(bool(hud.get_widget_state(var_text_id).get("visible", false)), "Widget con hide_during_messages_and_menus=false sigue visible durante mensaje")
	interp.advance_message_or_choice()
	await _pump_frames(2)
	assert_true(bool(hud.get_widget_state(hp_bar_id).get("visible", false)), "HUD reaparece al cerrar mensaje")

	interp.start_command_list([
		{"code": 351, "type": "open_menu"}
	])
	await _pump_frames(2)
	assert_true(not bool(hud.get_widget_state(currency_id).get("visible", true)), "HUD se oculta al abrir Menu del Juego")
	interp.get_game_menu().close_menu()
	await _pump_frames(2)
	assert_true(bool(hud.get_widget_state(currency_id).get("visible", false)), "HUD reaparece al cerrar Menu del Juego")

	# 9. Comando de evento Mostrar/Ocultar HUD y guardado en slot
	interp.start_command_list([
		{"code": 360, "type": "change_hud_visibility", "scope": "widget", "widget_name": "ContadorMonedas", "visible": false}
	])
	await _pump_frames(2)
	assert_true(not bool(hud.get_widget_state(currency_id).get("visible", true)), "Comando oculta widget individual por nombre")
	assert_true(bool(hud.get_widget_state(hp_bar_id).get("visible", false)), "Otros widgets siguen visibles tras ocultar solo uno")

	assert_true(dm.save_game_slot(1, {"map_name": "Mapa Test"}), "Guardar estado del HUD en slot 1")
	interp.start_command_list([
		{"code": 360, "type": "change_hud_visibility", "scope": "all", "visible": false}
	])
	await _pump_frames(2)
	assert_true(not dm.is_hud_runtime_visible(), "Comando oculta el HUD completo")
	assert_true(not bool(hud.get_widget_state(hp_bar_id).get("visible", true)), "Todos los widgets ocultos cuando HUD global=false")

	assert_true(dm.load_game_slot(1), "Cargar slot 1")
	await _pump_frames(2)
	assert_true(dm.is_hud_runtime_visible(), "HUD global vuelve a visible al cargar slot 1")
	assert_true(not bool(hud.get_widget_state(currency_id).get("visible", true)), "Ocultacion individual de widget persiste en guardado")

	interp.start_command_list([
		{"code": 360, "type": "change_hud_visibility", "scope": "widget", "widget_name": "ContadorMonedas", "visible": true}
	])
	await _pump_frames(2)
	assert_true(bool(hud.get_widget_state(currency_id).get("visible", false)), "Comando vuelve a mostrar widget individual")

	# 10. Escalado limpio 16:9 en 960x540, 1366x768, 1920x1080 y 2560x1440 sin recortes ni solapamientos
	for res in [Vector2(960, 540), Vector2(1366, 768), Vector2(1920, 1080), Vector2(2560, 1440)]:
		hud.set_viewport_size_override(res)
		var binfo = hud.get_layout_bounds_info()
		assert_eq(Array(binfo.get("clipped_widgets", [])).size(), 0, "Sin widgets recortados en %dx%d" % [int(res.x), int(res.y)])
		assert_eq(Array(binfo.get("overlapping_pairs", [])).size(), 0, "Sin widgets solapados en %dx%d" % [int(res.x), int(res.y)])

	hud.set_viewport_size_override(Vector2.ZERO)

	# Limpieza
	interp.queue_free()
	player.queue_free()
	await _pump_frames(2)
	dm.set_save_directory("")
	_cleanup_dir(tmp_root)

	print("=== RESULTADO TANDA 5: %d PASS, %d FAIL ===" % [passed, failed])
	if failed == 0:
		print("[PASS] Todas las pruebas de Tanda 5 (HUD y Widgets: Logica y Datos) pasaron: OK")
		quit(0)
	else:
		print("[FAIL] Hubo %d fallos en Tanda 5." % failed)
		quit(1)
