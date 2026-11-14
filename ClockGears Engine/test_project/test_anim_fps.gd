extends SceneTree

var passed := 0
var failed := 0
var _done := false
var _data_snap := {}
var _tmp_sheet := "res://img/characters/_tmp_anim_fps_sheet.png"

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		printerr("[FAIL] ", msg)

func _snapshot_dir(dir_res: String) -> Dictionary:
	var snap := {}
	var d := DirAccess.open(dir_res)
	if d == null:
		return snap
	d.list_dir_begin()
	var fn := d.get_next()
	while fn != "":
		if not d.current_is_dir() and fn.ends_with(".json"):
			var fa := FileAccess.open(dir_res.path_join(fn), FileAccess.READ)
			if fa:
				snap[fn] = fa.get_as_text()
				fa.close()
		fn = d.get_next()
	d.list_dir_end()
	return snap

func _restore_dir(dir_res: String, snap: Dictionary) -> void:
	var d := DirAccess.open(dir_res)
	if d:
		d.list_dir_begin()
		var fn := d.get_next()
		while fn != "":
			if not d.current_is_dir() and fn.ends_with(".json") and not snap.has(fn):
				d.remove(fn)
			fn = d.get_next()
		d.list_dir_end()
	for fn in snap.keys():
		var fa := FileAccess.open(dir_res.path_join(fn), FileAccess.WRITE)
		if fa:
			fa.store_string(snap[fn])
			fa.close()
	for p in [_tmp_sheet, _tmp_sheet + ".import"]:
		var gp := ProjectSettings.globalize_path(p)
		if FileAccess.file_exists(gp):
			DirAccess.remove_absolute(gp)

func _finish(code: int) -> void:
	if _done:
		return
	_done = true
	_restore_dir("res://clockgears_data", _data_snap)
	print("RESUMEN ANIM FPS -> PASS:%d FAIL:%d" % [passed, failed])
	quit(code)

func _watchdog_loop() -> void:
	await create_timer(45.0).timeout
	if not _done:
		printerr("[TIMEOUT] Watchdog 45s alcanzado en test_anim_fps.gd")
		failed += 1
		_finish(1)

func _find_controls(node: Node, cls_name: String) -> Array:
	var out := []
	if node.get_class() == cls_name or node.is_class(cls_name):
		out.append(node)
	for c in node.get_children():
		out.append_array(_find_controls(c, cls_name))
	return out

func _find_button(node: Node, needle: String) -> Button:
	if node is Button and needle in (node as Button).text:
		return node as Button
	for c in node.get_children():
		var r := _find_button(c, needle)
		if r:
			return r
	return null

func _find_fps_spin(db: Node) -> SpinBox:
	for sp in _find_controls(db, "SpinBox"):
		var sb := sp as SpinBox
		if sb.is_visible_in_tree() and int(sb.min_value) == 1 and int(sb.max_value) == 60:
			var par := sb.get_parent()
			if par:
				for sib in par.get_children():
					if sib is Label and "FPS" in (sib as Label).text:
						return sb
	return null

func _find_frame_dur_spins(db: Node) -> Array:
	var out := []
	for sp in _find_controls(db, "SpinBox"):
		var sb := sp as SpinBox
		if sb.is_visible_in_tree() and absf(sb.min_value - 0.01) < 0.005 and absf(sb.max_value - 60.0) < 0.1 and absf(sb.step - 0.01) < 0.005:
			var par := sb.get_parent()
			if par:
				for sib in par.get_children():
					if sib is Label and (sib as Label).text.begins_with("s:"):
						out.append(sb)
						break
	return out

func _find_anim_preview_rect(db: Node) -> TextureRect:
	for tr in _find_controls(db, "TextureRect"):
		var t := tr as TextureRect
		if t.is_visible_in_tree() and t.texture is AtlasTexture and t.custom_minimum_size.x >= 100.0:
			return t
	return null

func _measure_preview_cycle_sec(prev_rect: TextureRect, btn_play: Button, max_sec: float = 12.0) -> float:
	if "Detener" in btn_play.text:
		btn_play.emit_signal("pressed")
		await process_frame
	btn_play.emit_signal("pressed")
	await process_frame
	var dt := 1.0 / 60.0
	var get_col := func() -> int:
		if prev_rect.texture is AtlasTexture:
			var at := prev_rect.texture as AtlasTexture
			return int(round(at.region.position.x / 32.0))
		return -1
	var last_col: int = get_col.call()
	# Sincronizar al borde exacto del primer cambio de frame
	var sync_steps := int(ceil(max_sec * 60.0))
	for _s in range(sync_steps):
		await process_frame
		var c: int = get_col.call()
		if c != last_col:
			last_col = c
			break
	var start_col := last_col
	var elapsed := 0.0
	var transitions := 0
	var max_steps := int(ceil(max_sec * 60.0))
	for _step in range(max_steps):
		await process_frame
		elapsed += dt
		var col: int = get_col.call()
		if col != last_col:
			last_col = col
			transitions += 1
			if col == start_col and transitions >= 4:
				break
	if "Detener" in btn_play.text:
		btn_play.emit_signal("pressed")
		await process_frame
	if transitions < 4:
		return -1.0
	return elapsed

func _measure_f5_cycle_sec(dm, actor_id: int, max_sec: float = 12.0) -> float:
	var sys: Dictionary = dm.get_system_config()
	var ts_id: int = dm.create_record(13, {"name": "TS_FPS"})
	sys["party"] = [actor_id]
	sys["start_x"] = 2
	sys["start_y"] = 2
	dm.set_system_config(sys)

	var map := ClockGearsMap.new()
	map.set_name("MapAnimFPS")
	map.set_map_size(Vector2i(8, 8))
	map.set_tileset_id(ts_id)
	map.ensure_default_layers()
	get_root().add_child(map)
	map.init_runtime_session()
	await process_frame

	var player: ClockGearsPlayer = map.get_runtime_player()
	player.set_grid_cell_immediate(Vector2i(2, 2), 2)
	await process_frame

	var dt := 1.0 / 60.0
	var last_fi := player.get_active_slot_frame_index()
	var sync_steps := int(ceil(max_sec * 60.0))
	for _s in range(sync_steps):
		await process_frame
		var fi_sync := player.get_active_slot_frame_index()
		if fi_sync != last_fi:
			last_fi = fi_sync
			break
	var start_fi := last_fi
	var elapsed := 0.0
	var transitions := 0
	var max_steps := int(ceil(max_sec * 60.0))
	for _step in range(max_steps):
		await process_frame
		elapsed += dt
		var fi := player.get_active_slot_frame_index()
		if fi != last_fi:
			last_fi = fi
			transitions += 1
			if fi == start_fi and transitions >= 4:
				break

	map.queue_free()
	await process_frame
	if transitions < 4:
		return -1.0
	return elapsed

func _set_fps_via_enter(fps_spin: SpinBox, val: int) -> void:
	var le := fps_spin.get_line_edit()
	le.grab_focus()
	le.text = str(val)
	le.emit_signal("text_changed", str(val))
	le.emit_signal("text_submitted", str(val))

func _set_fps_via_blur(fps_spin: SpinBox, other_ctrl: Control, val: int) -> void:
	var le := fps_spin.get_line_edit()
	le.grab_focus()
	le.text = str(val)
	le.emit_signal("text_changed", str(val))
	if other_ctrl:
		other_ctrl.grab_focus()
	le.emit_signal("focus_exited")

func _initialize() -> void:
	_watchdog_loop()
	await _run_tests()

func _run_tests() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	check(dm != null, "ClockGearsDataManager disponible")
	if dm == null:
		_finish(1)
		return
	dm.load_database()
	_data_snap = _snapshot_dir("res://clockgears_data")

	# Crear spritesheet 128x32 (4 frames de 32x32)
	var img := Image.create(128, 32, false, Image.FORMAT_RGBA8)
	for i in range(4):
		img.fill_rect(Rect2i(i * 32, 0, 32, 32), Color(0.2 + i * 0.2, 0.4, 0.8 - i * 0.15, 1.0))
	var abs_sheet := ProjectSettings.globalize_path(_tmp_sheet)
	DirAccess.make_dir_recursive_absolute(abs_sheet.get_base_dir())
	img.save_png(abs_sheet)

	var make_frames := func(dur: float) -> Array:
		var arr := []
		for i in range(4):
			arr.append({"sheet": _tmp_sheet, "x": i * 32, "y": 0, "w": 32, "h": 32, "duration": dur, "is_random_duration": false, "duration_min": 0.1, "duration_max": 0.5})
		return arr

	# Actor de prueba: 4 frames de 0.13s en quieto_abajo y mover_abajo
	var actor_id: int = dm.create_record(0, {
		"name": "Actor_FPS_Test",
		"graphic_path": _tmp_sheet,
		"directional_idle": true,
		"sprite_data": {
			"spritesheet_path": _tmp_sheet,
			"frame_w": 32, "frame_h": 32, "cols": 4, "rows": 1,
			"animations": {
				"quieto_abajo": {
					"name": "quieto_abajo", "spritesheet_path": _tmp_sheet,
					"frame_w": 32, "frame_h": 32, "cols": 4, "rows": 1,
					"fps": 6.0, "loop": true, "frames": [0, 1, 2, 3],
					"frames_data": make_frames.call(0.13)
				},
				"mover_abajo": {
					"name": "mover_abajo", "spritesheet_path": _tmp_sheet,
					"frame_w": 32, "frame_h": 32, "cols": 4, "rows": 1,
					"fps": 6.0, "loop": true, "frames": [0, 1, 2, 3],
					"frames_data": make_frames.call(0.13)
				}
			}
		}
	})
	# Actor antiguo: duraciones 0.0 con fps 8.0, y otro con duraciones mixtas [0.20, 0.20, 0.20, 0.20]
	var legacy_actor_id: int = dm.create_record(0, {
		"name": "Actor_Legacy_FPS",
		"graphic_path": _tmp_sheet,
		"directional_idle": true,
		"sprite_data": {
			"spritesheet_path": _tmp_sheet,
			"frame_w": 32, "frame_h": 32, "cols": 4, "rows": 1,
			"animations": {
				"quieto_abajo": {
					"name": "quieto_abajo", "spritesheet_path": _tmp_sheet,
					"frame_w": 32, "frame_h": 32, "cols": 4, "rows": 1,
					"fps": 8.0, "loop": true, "frames": [0, 1, 2, 3],
					"frames_data": make_frames.call(0.0)
				}
			}
		}
	})
	dm.save_database()

	var db := ClockGearsDatabaseDialog.new()
	get_root().add_child(db)
	await process_frame
	db.load_from_disk()
	db.open_database()
	db.select_category(0) # CAT_ACTORS
	await process_frame

	var rlist := _find_controls(db, "ItemList")
	var rec_list: ItemList = null
	for il in rlist:
		var list_n := il as ItemList
		for idx in range(list_n.item_count):
			if "Actor_FPS_Test" in list_n.get_item_text(idx):
				rec_list = list_n
				rec_list.select(idx)
				rec_list.emit_signal("item_selected", idx)
				break
		if rec_list:
			break
	await process_frame

	db.select_anim_slot("quieto_abajo")
	await process_frame

	var fps_spin := _find_fps_spin(db)
	var btn_play := _find_button(db, "Probar")
	var prev_rect := _find_anim_preview_rect(db)
	check(fps_spin != null and btn_play != null and prev_rect != null, "Controles FPS, Probar y Preview encontrados (fps=%s play=%s rect=%s)" % [str(fps_spin != null), str(btn_play != null), str(prev_rect != null)])
	if not (fps_spin and btn_play and prev_rect):
		db.hide()
		db.queue_free()
		_finish(1)
		return

	# Comprobar valor inicial calculado de FPS desde 4 frames de 0.13s (1/0.13 = 7.69 -> 8) y conservacion de su ciclo ~0.52s
	check(absf(fps_spin.value - 8.0) <= 0.5, "FPS inicial calculado por promedio de 0.13s es ~8 (obtuvo %.2f)" % fps_spin.value)
	var init_prev := await _measure_preview_cycle_sec(prev_rect, btn_play, 2.5)
	var init_f5 := await _measure_f5_cycle_sec(dm, actor_id, 2.5)
	check(init_prev > 0.45 and init_prev < 0.60 and init_f5 > 0.45 and init_f5 < 0.60,
		"Datos existentes 4x0.13s conservan duracion ~0.52s en preview y F5 (prev=%.3fs, f5=%.3fs)" % [init_prev, init_f5])

	var results_prev := {}
	var results_f5 := {}

	# Probar FPS = 1 (por Enter), 4 (al perder foco), 30 (por Enter)
	_set_fps_via_enter(fps_spin, 1)
	await process_frame
	results_prev[1] = await _measure_preview_cycle_sec(prev_rect, btn_play, 5.5)
	db.get_ok_button().emit_signal("pressed")
	await process_frame
	results_f5[1] = await _measure_f5_cycle_sec(dm, actor_id, 5.5)

	# Reabrir DB y probar FPS = 4 (al perder foco)
	db.open_database()
	db.select_category(0)
	await process_frame
	for idx in range(rec_list.item_count):
		if "Actor_FPS_Test" in rec_list.get_item_text(idx):
			rec_list.select(idx)
			rec_list.emit_signal("item_selected", idx)
			break
	await process_frame
	db.select_anim_slot("quieto_abajo")
	await process_frame
	fps_spin = _find_fps_spin(db)
	btn_play = _find_button(db, "Probar")
	prev_rect = _find_anim_preview_rect(db)

	_set_fps_via_blur(fps_spin, btn_play, 4)
	await process_frame
	results_prev[4] = await _measure_preview_cycle_sec(prev_rect, btn_play, 3.0)
	db.get_ok_button().emit_signal("pressed")
	await process_frame
	results_f5[4] = await _measure_f5_cycle_sec(dm, actor_id, 3.0)

	# Reabrir DB y probar FPS = 30 (por Enter)
	db.open_database()
	db.select_category(0)
	await process_frame
	for idx in range(rec_list.item_count):
		if "Actor_FPS_Test" in rec_list.get_item_text(idx):
			rec_list.select(idx)
			rec_list.emit_signal("item_selected", idx)
			break
	await process_frame
	db.select_anim_slot("quieto_abajo")
	await process_frame
	fps_spin = _find_fps_spin(db)
	btn_play = _find_button(db, "Probar")
	prev_rect = _find_anim_preview_rect(db)

	_set_fps_via_enter(fps_spin, 30)
	await process_frame
	results_prev[30] = await _measure_preview_cycle_sec(prev_rect, btn_play, 2.0)
	db.get_ok_button().emit_signal("pressed")
	await process_frame
	results_f5[30] = await _measure_f5_cycle_sec(dm, actor_id, 2.0)

	# Verificar duraciones esperadas en vista previa y en F5: 1 FPS = ~4.0s, 4 FPS = ~1.0s, 30 FPS = ~0.133s
	check(results_prev[1] > 3.5 and results_prev[1] < 4.5, "Vista previa FPS=1 tarda ~4.0s por ciclo (obtuvo %.3fs)" % results_prev[1])
	check(results_prev[4] > 0.85 and results_prev[4] < 1.15, "Vista previa FPS=4 tarda ~1.0s por ciclo (obtuvo %.3fs)" % results_prev[4])
	check(results_prev[30] > 0.08 and results_prev[30] < 0.22, "Vista previa FPS=30 tarda ~0.133s por ciclo (obtuvo %.3fs)" % results_prev[30])
	check(results_f5[1] > 3.5 and results_f5[1] < 4.5, "F5 FPS=1 tarda ~4.0s por ciclo (obtuvo %.3fs)" % results_f5[1])
	check(results_f5[4] > 0.85 and results_f5[4] < 1.15, "F5 FPS=4 tarda ~1.0s por ciclo (obtuvo %.3fs)" % results_f5[4])
	check(results_f5[30] > 0.08 and results_f5[30] < 0.22, "F5 FPS=30 tarda ~0.133s por ciclo (obtuvo %.3fs)" % results_f5[30])
	check(absf(results_prev[1] - results_f5[1]) <= 0.15 and absf(results_prev[4] - results_f5[4]) <= 0.15 and absf(results_prev[30] - results_f5[30]) <= 0.10,
		"Vista previa y F5 coinciden en duracion (prev=%s vs f5=%s)" % [str(results_prev), str(results_f5)])

	# Ajustar a mano un frame despues de cambiar FPS (FPS=4 -> 0.25s/frame; cambiar frame #1 a 0.50s NO toca los demas)
	db.open_database()
	db.select_category(0)
	await process_frame
	for idx in range(rec_list.item_count):
		if "Actor_FPS_Test" in rec_list.get_item_text(idx):
			rec_list.select(idx)
			rec_list.emit_signal("item_selected", idx)
			break
	await process_frame
	db.select_anim_slot("quieto_abajo")
	await process_frame
	fps_spin = _find_fps_spin(db)
	_set_fps_via_enter(fps_spin, 4)
	await process_frame
	var dur_spins := _find_frame_dur_spins(db)
	check(dur_spins.size() == 4, "Se muestran 4 campos 's:' de frame (count=%d)" % dur_spins.size())
	if dur_spins.size() == 4:
		check(absf(dur_spins[0].value - 0.25) < 0.02 and absf(dur_spins[1].value - 0.25) < 0.02 and absf(dur_spins[2].value - 0.25) < 0.02 and absf(dur_spins[3].value - 0.25) < 0.02,
			"Al cambiar FPS=4 todos los frames pasan a 0.25s ([%.2f, %.2f, %.2f, %.2f])" % [dur_spins[0].value, dur_spins[1].value, dur_spins[2].value, dur_spins[3].value])
		dur_spins[1].value = 0.50
		await process_frame
		dur_spins = _find_frame_dur_spins(db)
		check(absf(dur_spins[0].value - 0.25) < 0.02 and absf(dur_spins[1].value - 0.50) < 0.02 and absf(dur_spins[2].value - 0.25) < 0.02 and absf(dur_spins[3].value - 0.25) < 0.02,
			"Ajustar frame #2 a 0.50s NO toca los demas ([%.2f, %.2f, %.2f, %.2f])" % [dur_spins[0].value, dur_spins[1].value, dur_spins[2].value, dur_spins[3].value])
		# Perder foco sin editar NO debe sobreescribir duraciones personalizadas
		var fps_le := fps_spin.get_line_edit()
		fps_le.grab_focus()
		fps_le.emit_signal("focus_exited")
		await process_frame
		dur_spins = _find_frame_dur_spins(db)
		check(absf(dur_spins[1].value - 0.50) < 0.02 and absf(dur_spins[0].value - 0.25) < 0.02,
			"Perder foco sin editar NO sobreescribe duraciones manuales")
		# Pulsar Enter con un nuevo FPS vuelve a unificar a 1/FPS
		_set_fps_via_enter(fps_spin, int(round(fps_spin.value)))
		await process_frame
		dur_spins = _find_frame_dur_spins(db)
		var exp_uni := 1.0 / float(int(round(fps_spin.value)))
		check(absf(dur_spins[1].value - exp_uni) < 0.02 and absf(dur_spins[0].value - exp_uni) < 0.02,
			"Pulsar Enter en FPS con el mismo entero mostrado reaplica 1/FPS a todos los frames")

	# Animacion antigua (frames con duration=0.0 y fps=8.0) -> conserva comportamiento y muestra FPS=8
	for idx in range(rec_list.item_count):
		if "Actor_Legacy_FPS" in rec_list.get_item_text(idx):
			rec_list.select(idx)
			rec_list.emit_signal("item_selected", idx)
			break
	await process_frame
	db.select_anim_slot("quieto_abajo")
	await process_frame
	fps_spin = _find_fps_spin(db)
	btn_play = _find_button(db, "Probar")
	prev_rect = _find_anim_preview_rect(db)
	check(int(round(fps_spin.value)) == 8, "Animacion antigua con duration=0 y fps=8 muestra FPS=8 (got=%.2f)" % fps_spin.value)
	var leg_prev := await _measure_preview_cycle_sec(prev_rect, btn_play, 2.5)
	db.get_ok_button().emit_signal("pressed")
	await process_frame
	var leg_f5 := await _measure_f5_cycle_sec(dm, legacy_actor_id, 2.5)
	check(leg_prev > 0.40 and leg_prev < 0.62 and leg_f5 > 0.40 and leg_f5 < 0.62,
		"Animacion antigua a 8 FPS tarda ~0.50s en preview y F5 (prev=%.3fs, f5=%.3fs)" % [leg_prev, leg_f5])

	db.queue_free()
	await process_frame
	_finish(0 if failed == 0 else 1)
