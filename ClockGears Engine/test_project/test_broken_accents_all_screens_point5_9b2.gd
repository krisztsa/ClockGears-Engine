extends SceneTree

# Test permanente Punto 5 (9B-2): Recorre TODAS las pantallas, diálogos, editores,
# catálogos y archivos JSON buscando acentos o símbolos rotos (Ã, Â, â€, U+FFFD).

var bad_hits: Array[String] = []

func _step_frames(n: int = 2) -> void:
	for _i in range(n):
		await process_frame

func _is_broken_text(s: String) -> bool:
	if s.is_empty():
		return false
	if s.find(String.chr(0xFFFD)) >= 0:
		return true
	# Patrones típicos de UTF-8 interpretado como Latin-1 o Windows-1252:
	# Ã (U+00C3) seguido de byte Latin-1 común (¡..¿, ±, ©, etc.) o cualquier Ã seguido de carácter >= 0x80
	for i in range(s.length()):
		var c := s.unicode_at(i)
		if c == 0x00C3 and i + 1 < s.length():
			var n := s.unicode_at(i + 1)
			if n >= 0x0080 and n <= 0x00BF:
				return true
		if c == 0x00C2 and i + 1 < s.length():
			var n := s.unicode_at(i + 1)
			if n >= 0x0080 and n <= 0x00BF:
				return true
		if c == 0x00E2 and i + 1 < s.length():
			var n := s.unicode_at(i + 1)
			if n == 0x20AC or (n >= 0x0080 and n <= 0x00BF):
				return true
	return false

func _check_str(source: String, val: String) -> void:
	if _is_broken_text(val):
		var msg := "TEXTO_ROTO [%s]: %s" % [source, val]
		bad_hits.append(msg)
		print("  FAIL " + msg)

func _scan_tree_texts(root_node: Node, prefix: String) -> void:
	if root_node == null:
		return
	var node_tag := "%s/%s(%s)" % [prefix, root_node.name, root_node.get_class()]
	if root_node is Window:
		_check_str(node_tag + ".title", (root_node as Window).title)
	if root_node is Control:
		_check_str(node_tag + ".tooltip", (root_node as Control).tooltip_text)
	if root_node is Label:
		_check_str(node_tag + ".text", (root_node as Label).text)
	elif root_node is RichTextLabel:
		_check_str(node_tag + ".text", (root_node as RichTextLabel).text)
	elif root_node is Button:
		var b := root_node as Button
		_check_str(node_tag + ".text", b.text)
		if b is OptionButton:
			var ob := b as OptionButton
			for i in range(ob.item_count):
				_check_str(node_tag + ".item[%d]" % i, ob.get_item_text(i))
	elif root_node is LineEdit:
		var le := root_node as LineEdit
		_check_str(node_tag + ".text", le.text)
		_check_str(node_tag + ".placeholder", le.placeholder_text)
	elif root_node is TextEdit:
		_check_str(node_tag + ".text", (root_node as TextEdit).text)
	elif root_node is TabBar:
		var tb := root_node as TabBar
		for i in range(tb.tab_count):
			_check_str(node_tag + ".tab[%d]" % i, tb.get_tab_title(i))
	elif root_node is TabContainer:
		var tc := root_node as TabContainer
		for i in range(tc.get_tab_count()):
			_check_str(node_tag + ".tab[%d]" % i, tc.get_tab_title(i))
	elif root_node is ItemList:
		var il := root_node as ItemList
		for i in range(il.item_count):
			_check_str(node_tag + ".item[%d]" % i, il.get_item_text(i))
	elif root_node is PopupMenu:
		var pm := root_node as PopupMenu
		for i in range(pm.item_count):
			_check_str(node_tag + ".menu[%d]" % i, pm.get_item_text(i))
	elif root_node is Tree:
		var tr := root_node as Tree
		_scan_tree_items(tr.get_root(), tr.columns, node_tag)

	for ch in root_node.get_children():
		_scan_tree_texts(ch, prefix)

func _scan_tree_items(item: TreeItem, cols: int, tag: String) -> void:
	if item == null:
		return
	for c in range(cols):
		_check_str("%s.cell[%d]" % [tag, c], item.get_text(c))
		_check_str("%s.tip[%d]" % [tag, c], item.get_tooltip_text(c))
	_scan_tree_items(item.get_first_child(), cols, tag)
	_scan_tree_items(item.get_next(), cols, tag)

func _scan_variant(source: String, v: Variant) -> void:
	match typeof(v):
		TYPE_STRING:
			_check_str(source, String(v))
		TYPE_PACKED_STRING_ARRAY:
			var arr: PackedStringArray = v
			for i in range(arr.size()):
				_check_str("%s[%d]" % [source, i], arr[i])
		TYPE_ARRAY:
			var a: Array = v
			for i in range(a.size()):
				_scan_variant("%s[%d]" % [source, i], a[i])
		TYPE_DICTIONARY:
			var d: Dictionary = v
			for k in d.keys():
				_check_str("%s.key(%s)" % [source, str(k)], String(k))
				_scan_variant("%s.%s" % [source, str(k)], d[k])

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 5 (9B-2): DETECCION DE ACENTOS ROTOS ===")

	# 1. DataManager (datos cargados + defaults)
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm != null:
		for c in range(18):
			_scan_variant("DM.cat[%d]" % c, dm.get_category_array(c))
		_scan_variant("DM.system", dm.get_system_config())
		_scan_variant("DM.title", dm.get_title_menu_config())
		_scan_variant("DM.title_default", ClockGearsDataManager.get_default_title_menu_config())
		_scan_variant("DM.game_menu", dm.get_game_menu_config())
		_scan_variant("DM.game_menu_default", ClockGearsDataManager.get_default_game_menu_config())
		_scan_variant("DM.credits", dm.get_credits_config())
		_scan_variant("DM.game_over", dm.get_game_over_config())
		_scan_variant("DM.hud", dm.get_hud_config())
		_scan_variant("DM.ui_designs", dm.get_ui_designs_config())
		_scan_variant("DM.window_styles", dm.get_window_styles())

	# 2. SystemCatalog
	if ClassDB.can_instantiate("ClockGearsSystemCatalog"):
		var cat = ClassDB.instantiate("ClockGearsSystemCatalog")
		if cat != null:
			for mname in ["get_weapon_types", "get_armor_types", "get_skill_types", "get_elements", "get_trigger_types", "get_all_commands", "get_all_traits", "get_all_effects", "get_escape_codes"]:
				if cat.has_method(mname):
					_scan_variant("SystemCatalog." + mname, cat.call(mname))

	# 3. Base de Datos con TODOS sus capítulos, categorías y sub-pestañas
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await _step_frames(2)
	db.open_database()
	await _step_frames(2)
	var chap_info: Dictionary = db.get_chapter_structure_info()
	for ch in chap_info.get("chapters", []):
		_check_str("DB.chapter", String(ch.get("name", "")))
		for ckey in ch.get("category_keys", []):
			db.select_category_by_key(String(ckey))
			await _step_frames(1)
			for tc in db.find_children("*", "TabContainer", true, false):
				var tcont: TabContainer = tc
				for ti in range(tcont.get_tab_count()):
					tcont.current_tab = ti
			await _step_frames(1)
			_scan_tree_texts(db, "DB[%s]" % String(ckey))
	db.queue_free()
	await _step_frames(1)

	# 4. Editor de eventos y catálogo/subdiálogos de comandos
	var ev := ClockGearsEvent.new()
	root.add_child(ev)
	var ev_dlg := ClockGearsEventEditorDialog.new()
	root.add_child(ev_dlg)
	ev_dlg.edit_event(ev)
	await _step_frames(2)
	_scan_tree_texts(ev_dlg, "EventEditor")
	var ev_h: Dictionary = ev_dlg.get_menu_command_ui_handles()
	var cmd_sel: Node = ev_h.get("command_selector", null)
	if cmd_sel != null:
		_scan_tree_texts(cmd_sel, "CommandSelector")
	ev_dlg.queue_free()
	ev.queue_free()

	# 5. Menú de mapas, Generar Entidad, F3 (abierto), TopBar, TilePicker y utilidades
	for cls_name in ["ClockGearsMapPanel", "ClockGearsMapPropertiesDialog", "ClockGearsGenerateEntityDialog", "ClockGearsCameraDialog", "ClockGearsTilePicker", "ClockGearsTilePickerDialog", "ClockGearsShapeEditor", "ClockGearsUiDesignEditor", "ClockGearsDebugOverlay", "ClockGearsTopBar", "ClockGearsPreferencesDialog", "ClockGearsNewProjectDialog", "ClockGearsResourceManagerDialog"]:
		if ClassDB.can_instantiate(cls_name):
			var inst: Node = ClassDB.instantiate(cls_name)
			root.add_child(inst)
			if cls_name == "ClockGearsDebugOverlay" and inst.has_method("toggle_debug_panel"):
				inst.toggle_debug_panel()
			await _step_frames(1)
			_scan_tree_texts(inst, cls_name)
			inst.queue_free()

	# 6. Juego completo: Map + Player + GameMenu + TitleScreen + Credits + GameOver + HUD + MessageWindow + BattleScene
	for rt_cls in ["ClockGearsCreditsPlayer", "ClockGearsHudOverlay", "ClockGearsMessageWindow", "ClockGearsBattleScene"]:
		if ClassDB.can_instantiate(rt_cls):
			var rinst: Node = ClassDB.instantiate(rt_cls)
			root.add_child(rinst)
			await _step_frames(1)
			_scan_tree_texts(rinst, rt_cls)
			rinst.queue_free()

	var title := ClockGearsTitleScreen.new()
	root.add_child(title)
	await _step_frames(1)
	_scan_tree_texts(title, "TitleScreen")
	title.queue_free()

	var gm := ClockGearsGameMenu.new()
	root.add_child(gm)
	gm.open_menu()
	await _step_frames(1)
	_scan_tree_texts(gm, "GameMenu.main")
	for sub in ["items", "equip", "status", "save", "options"]:
		if gm.has_method("open_sub_menu"):
			gm.open_sub_menu(sub)
			await _step_frames(1)
			_scan_tree_texts(gm, "GameMenu." + sub)
	gm.close_menu()
	gm.queue_free()

	var go := ClockGearsGameOverScreen.new()
	root.add_child(go)
	go.show_game_over()
	await _step_frames(1)
	_scan_tree_texts(go, "GameOver")
	go.queue_free()

	var m := ClockGearsMap.new()
	m.set_map_size(Vector2i(8, 8))
	root.add_child(m)
	m.ensure_default_layers()
	await _step_frames(1)
	_scan_tree_texts(m, "RuntimeMap")
	m.queue_free()
	await _step_frames(1)

	# 7. Archivos JSON de res://clockgears_data/ y escenas en res://maps/
	var da := DirAccess.open("res://clockgears_data")
	if da != null:
		da.list_dir_begin()
		var fname := da.get_next()
		while fname != "":
			if not da.current_is_dir() and fname.ends_with(".json"):
				var content := FileAccess.get_file_as_string("res://clockgears_data/" + fname)
				_check_str("JSON/" + fname, content)
			fname = da.get_next()
		da.list_dir_end()

	var mda := DirAccess.open("res://maps")
	if mda != null:
		mda.list_dir_begin()
		var mname := mda.get_next()
		while mname != "":
			if not mda.current_is_dir() and (mname.ends_with(".tscn") or mname.ends_with(".json")):
				var mcontent := FileAccess.get_file_as_string("res://maps/" + mname)
				_check_str("MAP/" + mname, mcontent)
			mname = mda.get_next()
		mda.list_dir_end()

	# 8. Verificación de recorte por caracteres UTF-8 con "…"
	var sample_trunc := "Canción española número uno"
	var cut_res := ClockGearsDataManager.truncate_with_ellipsis(sample_trunc, 12)
	var expected_cut := "Canción espa…"
	if cut_res != expected_cut:
		var msg := "TRUNC_FAIL: esperado '%s', obtenido '%s'" % [expected_cut, cut_res]
		bad_hits.append(msg)
		print("  FAIL " + msg)

	# 9. Verificación de reparación automática de mojibake JSON + copia de seguridad al cargar proyecto
	if dm != null:
		var tmp_dir := "user://tmp_point5_mojibake_9b2"
		DirAccess.make_dir_recursive_absolute(tmp_dir)
		var orig_db := "{\"schema_version\":1,\"title\":\"MenÃº Principal\"}"
		var orig_actors := "[{\"id\":1,\"name\":\"HÃ©roe de EspaÃ±a\",\"desc\":\"Árbol sano y PociÃ³n\"}]"
		var orig_troops := "[{\"id\":1,\"name\":\"Grupo de DragÃ³n â€” Â¡Alerta!\"}]"
		var orig_gmenu := "{\"sections\":{\"items\":\"Objetos Ãºtiles\"}}"
		var orig_cevs := "[{\"id\":1,\"name\":\"DiÃ¡logo â€¦ Fin\",\"commands\":[{\"type\":\"message\",\"params\":{\"text\":\"Â¿EstÃ¡s listo?\"}}]}]"
		var sample_files := {
			"database.json": orig_db,
			"actors.json": orig_actors,
			"troops.json": orig_troops,
			"game_menu.json": orig_gmenu,
			"common_events.json": orig_cevs,
		}
		for fn in sample_files.keys():
			var p := tmp_dir.path_join(fn)
			var fa := FileAccess.open(p, FileAccess.WRITE)
			fa.store_string(sample_files[fn])
			fa.close()
			if FileAccess.file_exists(p + ".pre_utf8_fix.bak"):
				DirAccess.remove_absolute(p + ".pre_utf8_fix.bak")

		dm.load_all(tmp_dir)
		var repaired_files: PackedStringArray = dm.get_last_repaired_json_files()
		print("  REPAIRED_FILES_TEST=", repaired_files)
		for fn in sample_files.keys():
			var p := tmp_dir.path_join(fn)
			var bak_p := p + ".pre_utf8_fix.bak"
			if not repaired_files.has(p):
				var msg := "MOJIBAKE_NOT_REPORTED: " + p
				bad_hits.append(msg)
				print("  FAIL " + msg)
			if not FileAccess.file_exists(bak_p):
				var msg := "BACKUP_MISSING: " + bak_p
				bad_hits.append(msg)
				print("  FAIL " + msg)
			else:
				var bak_txt := FileAccess.get_file_as_string(bak_p)
				if bak_txt != sample_files[fn]:
					var msg := "BACKUP_MISMATCH: " + bak_p
					bad_hits.append(msg)
					print("  FAIL " + msg)
			var fixed_txt := FileAccess.get_file_as_string(p)
			_check_str("REPAIRED_DISK/" + fn, fixed_txt)

		var actors_arr: Array = dm.get_records("actors")
		if actors_arr.is_empty() or actors_arr[0].get("name", "") != "Héroe de España" or actors_arr[0].get("desc", "") != "Árbol sano y Poción":
			var msg := "REPAIRED_ACTORS_MISMATCH: " + str(actors_arr)
			bad_hits.append(msg)
			print("  FAIL " + msg)
		var cevs_arr: Array = dm.get_records("common_events")
		if cevs_arr.is_empty() or cevs_arr[0].get("name", "") != "Diálogo … Fin":
			var msg := "REPAIRED_CEVS_MISMATCH: " + str(cevs_arr)
			bad_hits.append(msg)
			print("  FAIL " + msg)

		# Limpiar archivos temporales y recargar datos reales del proyecto
		var tda := DirAccess.open(tmp_dir)
		if tda != null:
			tda.list_dir_begin()
			var tf := tda.get_next()
			while tf != "":
				if not tda.current_is_dir():
					tda.remove(tf)
				tf = tda.get_next()
			tda.list_dir_end()
		DirAccess.remove_absolute(tmp_dir)
		dm.load_all("res://clockgears_data")

	print("RESUMEN PUNTO 5: total_textos_rotos=%d" % bad_hits.size())
	if bad_hits.is_empty():
		print("ALL PASS: 0 textos rotos en todo el motor.")
		quit(0)
	else:
		print("FAIL: %d textos rotos detectados." % bad_hits.size())
		quit(1)
