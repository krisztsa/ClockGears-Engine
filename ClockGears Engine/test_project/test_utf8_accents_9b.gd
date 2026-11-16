extends SceneTree

class WatchdogNode extends Node:
	var start_msec: int = 0
	var limit_msec: int = 55000
	var owner_tree: SceneTree = null
	func _process(_delta: float) -> void:
		if owner_tree != null and (Time.get_ticks_msec() - start_msec) >= limit_msec:
			owner_tree.call("_on_watchdog_timeout")

var passed: int = 0
var failed: int = 0
var _finished: bool = false

var _res_data_dir: String = "res://clockgears_data"
var _backup: Dictionary = {}
var _prev_rt: Dictionary = {}
var _has_backup: bool = false

var _seen_broken: Dictionary = {}
var _broken_count: int = 0
var _total_scanned_strings: int = 0
var _intact_accent_chars: Dictionary = {}

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  [PASS] " + msg)
	else:
		failed += 1
		printerr("  [FAIL] " + msg)

func _step_frames(n: int = 2) -> void:
	for i in range(n):
		await process_frame

func _snapshot_data_dir(res_data_dir: String) -> Dictionary:
	var backup := {}
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				backup[full] = FileAccess.get_file_as_string(full)
			fname = da.get_next()
		da.list_dir_end()
	return backup

func _restore_data_dir(res_data_dir: String, backup: Dictionary) -> void:
	var da := DirAccess.open(res_data_dir)
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var full := res_data_dir.path_join(fname)
				if not backup.has(full):
					DirAccess.remove_absolute(ProjectSettings.globalize_path(full))
			fname = da.get_next()
		da.list_dir_end()
	for full in backup.keys():
		var wf := FileAccess.open(full, FileAccess.WRITE)
		if wf:
			wf.store_string(backup[full])

func _cleanup() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm != null and _has_backup:
		dm.import_runtime_state(_prev_rt)
		_restore_data_dir(_res_data_dir, _backup)
		dm.load_all(_res_data_dir)
		_has_backup = false

func _finish_test(exit_override: int = -1) -> void:
	if _finished:
		return
	_finished = true
	_cleanup()
	var code := exit_override if exit_override >= 0 else (0 if failed == 0 else 1)
	print("=== RESULTADO TEST UTF-8 9B: %d PASS, %d FAIL (strings=%d, mojibake=%d, acentos=%s) ===" % [
		passed, failed, _total_scanned_strings, _broken_count, "".join(_intact_accent_chars.keys())
	])
	quit(code)

func _on_watchdog_timeout() -> void:
	if _finished:
		return
	failed += 1
	printerr("  [FAIL] Timeout 55s alcanzado en test_utf8_accents_9b.gd")
	_finish_test(1)

func _has_mojibake(s: String) -> bool:
	if s.is_empty():
		return false
	for pat in ["Ã", "Â", "â€", "\ufffd", "ï¿½"]:
		if s.find(pat) >= 0:
			return true
	return false

func _record_text(context: String, origin: String, txt: String) -> void:
	if txt.is_empty():
		return
	_total_scanned_strings += 1
	for ch in ["á", "é", "í", "ó", "ú", "ñ", "Á", "É", "Í", "Ó", "Ú", "Ñ", "¿", "¡"]:
		if txt.find(ch) >= 0:
			_intact_accent_chars[ch] = true
	if _has_mojibake(txt):
		var single_line := txt.replace("\n", "\\n").replace("\r", "")
		var key := context + "|" + single_line
		if not _seen_broken.has(key):
			_seen_broken[key] = true
			_broken_count += 1
			printerr("  [MOJIBAKE] %s (%s): \"%s\"" % [context, origin, single_line])

func _scan_tree_item(context: String, tree_name: String, item: TreeItem, cols: int) -> void:
	if item == null:
		return
	for c in range(cols):
		_record_text(context, "TreeItem:%s[%d]" % [tree_name, c], item.get_text(c))
	var ch := item.get_first_child()
	while ch != null:
		_scan_tree_item(context, tree_name, ch, cols)
		ch = ch.get_next()

func _scan_node_texts(node: Node, context: String) -> void:
	if node == null or not is_instance_valid(node):
		return
	var cls := node.get_class()
	var nm := String(node.name)
	if node is AcceptDialog:
		var ad := node as AcceptDialog
		_record_text(context, cls + ":" + nm + ".title", ad.title)
		_record_text(context, cls + ":" + nm + ".dialog_text", ad.dialog_text)
	elif node is Window:
		var w := node as Window
		_record_text(context, cls + ":" + nm + ".title", w.title)
	if node is Label:
		_record_text(context, "Label:" + nm, (node as Label).text)
	elif node is OptionButton:
		var ob := node as OptionButton
		_record_text(context, "OptionButton:" + nm + ".text", ob.text)
		for i in range(ob.item_count):
			_record_text(context, "OptionButton:" + nm + "[%d]" % i, ob.get_item_text(i))
	elif node is Button:
		_record_text(context, cls + ":" + nm, (node as Button).text)
	elif node is LineEdit:
		var le := node as LineEdit
		_record_text(context, "LineEdit:" + nm, le.text)
		_record_text(context, "LineEdit:" + nm + ".placeholder", le.placeholder_text)
	elif node is TextEdit:
		var te := node as TextEdit
		_record_text(context, "TextEdit:" + nm, te.text)
		_record_text(context, "TextEdit:" + nm + ".placeholder", te.placeholder_text)
	elif node is RichTextLabel:
		_record_text(context, "RichTextLabel:" + nm, (node as RichTextLabel).text)
	elif node is TabBar:
		var tb := node as TabBar
		for i in range(tb.tab_count):
			_record_text(context, "TabBar:" + nm + "[%d]" % i, tb.get_tab_title(i))
	elif node is TabContainer:
		var tc := node as TabContainer
		for i in range(tc.get_tab_count()):
			_record_text(context, "TabContainer:" + nm + "[%d]" % i, tc.get_tab_title(i))
	elif node is ItemList:
		var il := node as ItemList
		for i in range(il.item_count):
			_record_text(context, "ItemList:" + nm + "[%d]" % i, il.get_item_text(i))
	elif node is Tree:
		var tr := node as Tree
		for c in range(tr.columns):
			_record_text(context, "TreeCol:" + nm + "[%d]" % c, tr.get_column_title(c))
		_scan_tree_item(context, nm, tr.get_root(), tr.columns)

	for child in node.get_children():
		_scan_node_texts(child, context)

func _check_context_clean(context: String, node: Node) -> void:
	var before := _broken_count
	_scan_node_texts(node, context)
	var added := _broken_count - before
	check(added == 0, "Sin mojibake en %s (rotas nuevas=%d)" % [context, added])

func _run_tests() -> void:
	print("=== TEST UTF-8 ACENTOS 9B + MOTOR COMPLETO ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager disponible")
	if dm == null:
		_finish_test(1)
		return

	_backup = _snapshot_data_dir(_res_data_dir)
	_prev_rt = dm.export_runtime_state()
	_has_backup = true
	await _step_frames(1)

	dm.load_all(_res_data_dir)
	dm.reset_runtime_state()

	# Preparar datos con acentos para ejercitar créditos, HUD, equipo y título
	dm.set_category_array(0, [
		{"id": 1, "name": "Héroe Índigo", "class_id": 1, "initial_level": 2, "hp": 120, "mp": 35, "atk": 18, "def": 10},
	])
	dm.set_category_array(1, [
		{"id": 1, "name": "Espadachín", "exp_curve_type": 0},
	])
	dm.set_category_array(3, [
		{"id": 1, "name": "Poción Mágica", "desc": "Recupera vitalidad rápidamente", "type": 0, "consumable": true, "effects": [{"code": 11, "data_id": 0, "value1": 0.0, "value2": 30.0}]},
	])
	dm.set_category_array(4, [
		{"id": 1, "name": "Báculo Solar", "etype_id": 0, "atk": 9, "def": 2},
	])
	dm.change_item("Poción Mágica", 2)
	dm.change_item("Báculo Solar", 1)

	# Asegurar al menos una línea de créditos para que aparezca la entrada "Créditos" en el título
	var cr_cfg: Dictionary = dm.get_credits_config()
	var cr_lines: Array = cr_cfg.get("lines", []).duplicate(true)
	cr_lines.append({"type": "section", "text": "Dirección Técnica"})
	cr_lines.append({"type": "role_name", "role": "Programación", "name": "Equipo Ñ"})
	cr_cfg["lines"] = cr_lines
	dm.set_credits_config(cr_cfg)

	# 1. PANTALLA DE TÍTULO + OPCIONES + CONTINUAR + CRÉDITOS DESDE TÍTULO
	var ts := ClockGearsTitleScreen.new()
	ts.set_auto_quit_tree(false)
	root.add_child(ts)
	await _step_frames(3)
	_check_context_clean("TitleScreen/Main", ts)

	var opt_btn := ts.find_child("Entry_options", true, false) as Button
	if opt_btn != null:
		opt_btn.emit_signal("pressed")
		await _step_frames(2)
	_check_context_clean("TitleScreen/Options", ts)
	ts.handle_navigation_key(KEY_ESCAPE)
	await _step_frames(2)

	ts.open_continue_menu() if ts.has_method("open_continue_menu") else null
	var cont_btn := ts.find_child("Entry_continue", true, false) as Button
	if cont_btn != null:
		cont_btn.emit_signal("pressed")
		await _step_frames(2)
	_check_context_clean("TitleScreen/Continue", ts)
	ts.queue_free()
	await _step_frames(2)

	# 2. MENÚ DEL JUEGO (TODAS LAS SUBPANTALLAS + CONFIRMACIÓN SALIR AL TÍTULO)
	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	await _step_frames(2)
	gm.open_menu()
	await _step_frames(2)
	for sub in ["main", "items", "equip", "status", "save", "options", "to_title"]:
		gm.open_subscreen(sub)
		await _step_frames(2)
		_check_context_clean("GameMenu/" + sub, gm)
	gm.queue_free()
	await _step_frames(2)

	# 3. CRÉDITOS
	var cp := ClockGearsCreditsPlayer.new()
	root.add_child(cp)
	await _step_frames(2)
	cp.start_credits({"source": "test"})
	await _step_frames(2)
	_check_context_clean("CreditsPlayer", cp)
	cp.queue_free()
	await _step_frames(2)

	# 4. HUD
	var hud := ClockGearsHudOverlay.new()
	root.add_child(hud)
	await _step_frames(2)
	_check_context_clean("HudOverlay", hud)
	hud.queue_free()
	await _step_frames(2)

	# 5. BASE DE DATOS (TODAS LAS CATEGORÍAS + MENÚS + VISTAS PREVIAS EN VIVO)
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(2)

	var cat_keys := [
		"actors", "npcs", "classes",
		"weapons", "armors", "accessories",
		"items",
		"skills", "enemies", "troops", "states",
		"arpg_enemies", "arpg_npcs", "arpg_weapons", "arpg_armors", "arpg_shields", "arpg_ammo",
		"animations",
		"tilesets", "flora", "fauna", "common_events",
		"system", "types", "vocabulary", "switches_vars",
		"main_menu", "window_styles", "game_menu", "credits", "hud", "ui_designer"
	]
	for ck in cat_keys:
		db.select_category_by_key(ck)
		await _step_frames(2)
		_check_context_clean("Database/" + ck, db)
		if ck == "game_menu":
			var gm_handles: Dictionary = db.get_game_menu_editor_ui_handles()
			var live_prev = gm_handles.get("live_preview", null)
			if live_prev != null:
				for sub in ["main", "items", "equip", "status", "save", "options", "to_title"]:
					live_prev.open_subscreen(sub)
					await _step_frames(1)
					_check_context_clean("Database/game_menu_preview/" + sub, live_prev)
		elif ck == "ui_designer":
			var ui_handles: Dictionary = db.get_ui_designer_handles() if db.has_method("get_ui_designer_handles") else {}
			var ed = ui_handles.get("editor", null)
			if ed != null and ed.has_method("select_screen"):
				for sk in ["title", "game_menu_main", "game_menu_items", "game_menu_equip", "game_menu_status", "credits", "hud"]:
					ed.select_screen(sk)
					await _step_frames(1)
					_check_context_clean("Database/ui_designer/" + sk, ed)

	db.queue_free()
	await _step_frames(2)

	# 6. EDITOR DE EVENTOS Y SELECTOR DE COMANDOS
	var ev_dlg := ClockGearsEventEditorDialog.new()
	root.add_child(ev_dlg)
	await _step_frames(2)
	_check_context_clean("EventEditorDialog", ev_dlg)
	if ev_dlg.has_method("get_command_selector"):
		var sel = ev_dlg.get_command_selector()
		if sel != null:
			_check_context_clean("CommandSelector", sel)
	ev_dlg.queue_free()
	await _step_frames(2)

	# 7. PANEL F3 DE DEPURACIÓN
	var dbg := ClockGearsDebugOverlay.new()
	root.add_child(dbg)
	await _step_frames(2)
	_check_context_clean("DebugOverlay", dbg)
	dbg.queue_free()
	await _step_frames(2)

	# 8. PERSISTENCIA UTF-8 EN JSON (GUARDAR Y RECARGAR PROYECTO CON ACENTOS Y PROYECTO LEGADO)
	var sample_phrase := "¿Acción rápida, héroe ñandú y pócima mágica!"
	var sys_cfg: Dictionary = dm.get_system_config().duplicate(true)
	var orig_title := String(sys_cfg.get("game_title", ""))
	sys_cfg["game_title"] = sample_phrase
	dm.set_system_config(sys_cfg)
	dm.set_category_array(0, [
		{"id": 1, "name": "Héroe Índigo", "class_id": 1, "initial_level": 2, "hp": 120, "mp": 35, "atk": 18, "def": 10},
	])
	dm.save_all(_res_data_dir)
	dm.init_empty()
	dm.load_all(_res_data_dir)
	var reloaded_sys: Dictionary = dm.get_system_config()
	var reloaded_actors: Array = dm.get_category_array(0)
	check(String(reloaded_sys.get("game_title", "")) == sample_phrase, "Título con acentos y ñ/¿/! persiste idéntico al guardar/recargar JSON")
	check(reloaded_actors.size() >= 1 and String(Dictionary(reloaded_actors[0]).get("name", "")) == "Héroe Índigo",
		"Actor 'Héroe Índigo' persiste intacto tras guardar/recargar JSON")
	# Restaurar datos originales (proyecto legado) y comprobar que cargan limpios sin alteraciones
	_restore_data_dir(_res_data_dir, _backup)
	dm.load_all(_res_data_dir)
	var legacy_types: Array = dm.get_category_array(16) # CAT_TYPES = 16
	var types_ok := true
	for td in legacy_types:
		if td is Dictionary and _has_mojibake(String(Dictionary(td).get("name", ""))):
			types_ok = false
	check(types_ok, "Proyecto existente/legado con acentos carga limpio sin alterar sus palabras ni introducir mojibake")
	sys_cfg["game_title"] = orig_title

	# Modo opcional para demostrar detección de una cadena rota intencionada (-- --simulate-broken)
	if OS.get_cmdline_user_args().has("--simulate-broken"):
		var sab := Label.new()
		sab.name = "SabotajeUTF8"
		sab.text = "Texto Ã roto"
		root.add_child(sab)
		_check_context_clean("SimulatedBrokenLabel", sab)
		sab.queue_free()

	# 9. VERIFICACIÓN GENERAL DE PRESENCIA DE ACENTOS ESPAÑOLES INTACTOS
	check(_intact_accent_chars.size() >= 5, "Caracteres acentuados en español detectados intactos (%d distintos: %s)" % [
		_intact_accent_chars.size(), "".join(_intact_accent_chars.keys())
	])

	_finish_test()

func _init() -> void:
	var wd := WatchdogNode.new()
	wd.start_msec = Time.get_ticks_msec()
	wd.owner_tree = self
	root.add_child(wd)
	call_deferred("_run_tests")
