extends SceneTree

func _find_button_by_text(node: Node, txt: String) -> Button:
	if node is Button and node.text == txt:
		return node
	for c in node.get_children():
		var r = _find_button_by_text(c, txt)
		if r:
			return r
	return null

func _find_first(node: Node, cls_name: String) -> Node:
	if node.get_class() == cls_name or node.is_class(cls_name):
		return node
	for c in node.get_children():
		var r = _find_first(c, cls_name)
		if r:
			return r
	return null

func _find_line_edits(node: Node) -> Array:
	var out: Array = []
	if node is LineEdit:
		out.append(node)
	for c in node.get_children():
		out.append_array(_find_line_edits(c))
	return out

func _backup_file(path: String) -> PackedByteArray:
	if FileAccess.file_exists(path):
		return FileAccess.get_file_as_bytes(path)
	return PackedByteArray()

func _restore_file(path: String, data: PackedByteArray, existed: bool) -> void:
	if existed:
		var fa = FileAccess.open(path, FileAccess.WRITE)
		if fa:
			fa.store_buffer(data)
			fa.close()
	elif FileAccess.file_exists(path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(path))

func _initialize() -> void:
	print("=== REPRO TEST START ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.load_database()

	var troops_p := "res://clockgears_data/troops.json"
	var enemies_p := "res://clockgears_data/enemies.json"
	var actors_p := "res://clockgears_data/actors.json"
	var sys_p := "res://clockgears_data/system.json"
	var troops_ex := FileAccess.file_exists(troops_p)
	var enemies_ex := FileAccess.file_exists(enemies_p)
	var actors_ex := FileAccess.file_exists(actors_p)
	var sys_ex := FileAccess.file_exists(sys_p)
	var troops_bak := _backup_file(troops_p)
	var enemies_bak := _backup_file(enemies_p)
	var actors_bak := _backup_file(actors_p)
	var sys_bak := _backup_file(sys_p)

	# Seed minimal enemy, actor, troop for testing
	var enemies: Array = dm.get_category_array(7)
	if enemies.is_empty():
		enemies.append({"id": 1, "name": "Cañón musgoso", "level": 3, "exp": 25, "gold": 15, "max_hp": 50, "max_mp": 10, "atk": 10, "def": 5, "agi": 5})
		dm.set_category_array(7, enemies)
	var actors: Array = dm.get_category_array(0)
	if actors.is_empty():
		actors.append({"id": 1, "name": "Héroe", "level": 1, "exp": 0, "max_hp": 200, "max_mp": 50, "atk": 40, "def": 10, "agi": 20})
		dm.set_category_array(0, actors)
	var sys: Dictionary = dm.get_system_config()
	if not sys.has("party") or sys["party"].is_empty():
		sys["party"] = [1]
		dm.set_system_config(sys)
	var troops: Array = dm.get_category_array(8)
	if troops.is_empty():
		troops.append({
			"id": 1,
			"name": "Tropa Test",
			"notes": "Nota test",
			"battleback1": "",
			"battleback2": "",
			"members": [{"enemy_id": 1, "x": 480.0, "y": 280.0, "hidden": false}]
		})
		dm.set_category_array(8, troops)
	dm.save_database()

	var db = ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	await process_frame
	db.load_from_disk()
	db.open_database()
	db.select_category(8) # CAT_TROOPS
	await process_frame

	# 1. REPRO BUG 4a (Nombre / Notas ocultos en Tropas)
	var line_edits = _find_line_edits(db)
	var visible_name_notes := 0
	for le in line_edits:
		if le.get_parent() is GridContainer and le.is_visible_in_tree():
			visible_name_notes += 1
	print("[REPRO BUG 4a] CAT_TROOPS visible GridContainer LineEdits (Nombre/Notas): ", visible_name_notes)

	# 2. REPRO BUG 4b (Etiqueta de enemigo cortada)
	var font = ThemeDB.fallback_font
	var font_sz := 9
	var full_str := "Cañón musgoso Nv 3"
	var str_w := font.get_string_size(full_str, HORIZONTAL_ALIGNMENT_LEFT, -1, font_sz).x
	var old_label_w := 120.0 * 0.6
	print("[REPRO BUG 4b] String width=", str_w, " vs old preview label_w=", old_label_w, " clipped=", (str_w > old_label_w))

	# 3. REPRO BUG 1 (Fondo en blanco al cargar PNG nuevo)
	var tmp_src := ProjectSettings.globalize_path("res://tmp_import_src/repro_battleback_test.png")
	DirAccess.make_dir_recursive_absolute(tmp_src.get_base_dir())
	var img := Image.create_empty(64, 64, false, Image.FORMAT_RGBA8)
	img.fill(Color(0.1, 0.7, 0.2, 1.0))
	img.save_png(tmp_src)
	var target_res := "res://img/battlebacks/repro_battleback_test.png"
	var target_global := ProjectSettings.globalize_path(target_res)
	if FileAccess.file_exists(target_global):
		DirAccess.remove_absolute(target_global)
	if FileAccess.file_exists(target_global + ".import"):
		DirAccess.remove_absolute(target_global + ".import")

	var btn_bg = _find_button_by_text(db, "Cambiar fondo")
	if btn_bg:
		btn_bg.emit_signal("pressed")
		await process_frame
	var bg_dlg: AcceptDialog = null
	for c in db.get_children():
		if c is AcceptDialog and c.title.contains("Cambiar fondo"):
			bg_dlg = c
			var b = _find_button_by_text(c, "Seleccionar...")
			if b:
				b.emit_signal("pressed")
				break
	for c in db.get_children():
		if c is FileDialog and c.title.contains("Fondo"):
			c.emit_signal("file_selected", tmp_src)
			if c.visible:
				c.hide()
			break
	if bg_dlg:
		bg_dlg.hide()
	await process_frame
	var rl_exists := ResourceLoader.exists(target_res)
	var rl_tex = ResourceLoader.load(target_res) if rl_exists else null
	var rl_size: Vector2 = rl_tex.get_size() if rl_tex is Texture2D else Vector2(-1, -1)
	print("[REPRO BUG 1] After selecting unimported PNG: file_exists=", FileAccess.file_exists(target_global), " .import=", FileAccess.file_exists(target_global + ".import"), " ResourceLoader.exists=", rl_exists, " rl_tex=", rl_tex, " rl_size=", rl_size)

	# 4. REPRO BUG 3 (Prueba de batalla no termina & muta oro)
	dm.set_gold(100)
	var gold_before: int = dm.get_gold()
	var btn_test = _find_button_by_text(db, "Prueba de batalla")
	if btn_test:
		btn_test.emit_signal("pressed")
		await process_frame
	var test_dlg: AcceptDialog = null
	var bscene: ClockGearsBattleScene = null
	for c in db.get_children():
		if c is AcceptDialog and c.title.contains("Prueba de Batalla"):
			test_dlg = c
			for bc in c.get_children():
				if bc is ClockGearsBattleScene:
					bscene = bc
					break
	if bscene:
		var conn_count := bscene.get_signal_connection_list("battle_finished").size()
		print("[REPRO BUG 3a] BattleTest battle_finished connections=", conn_count)
		var ctrl = bscene.get_controller()
		if ctrl:
			for en in ctrl.get_enemy_units():
				en.set_hp(1)
			ctrl.action_attack(0)
			ctrl.check_battle_end()
			bscene.finish_battle_immediate()
			await process_frame
	var gold_after: int = dm.get_gold()
	var dlg_still_visible := test_dlg.visible if test_dlg else false
	print("[REPRO BUG 3b] BattleTest dialog still visible after win=", dlg_still_visible, " gold_before=", gold_before, " gold_after=", gold_after)

	# Cleanup
	if FileAccess.file_exists(target_global):
		DirAccess.remove_absolute(target_global)
	if FileAccess.file_exists(target_global + ".import"):
		DirAccess.remove_absolute(target_global + ".import")
	if FileAccess.file_exists(tmp_src):
		DirAccess.remove_absolute(tmp_src)
	_restore_file(troops_p, troops_bak, troops_ex)
	_restore_file(enemies_p, enemies_bak, enemies_ex)
	_restore_file(actors_p, actors_bak, actors_ex)
	_restore_file(sys_p, sys_bak, sys_ex)
	dm.set_gold(0)
	dm.load_database()

	print("=== REPRO TEST END ===")
	quit(0)
