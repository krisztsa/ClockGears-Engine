extends SceneTree

var passed := 0
var failed := 0

func check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("[PASS] ", msg)
	else:
		failed += 1
		printerr("[FAIL] ", msg)

func _make_png(path: String, w: int, h: int, color_fn: Callable) -> String:
	var img := Image.create(w, h, false, Image.FORMAT_RGBA8)
	for y in range(h):
		for x in range(w):
			img.set_pixel(x, y, color_fn.call(x, y))
	var abs_p := ProjectSettings.globalize_path(path)
	DirAccess.make_dir_recursive_absolute(abs_p.get_base_dir())
	img.save_png(abs_p)
	return path

func _find_button_by_text(root: Node, needle: String, exact: bool = false) -> Button:
	for n in root.find_children("*", "Button", true, false):
		var b := n as Button
		if exact:
			if b.text == needle:
				return b
		elif b.text.find(needle) != -1:
			return b
	return null

func _find_tab_index(tc: TabContainer, title: String) -> int:
	if tc == null:
		return -1
	for i in range(tc.get_tab_count()):
		if tc.get_tab_title(i).find(title) != -1:
			return i
	return -1

func _setup_test_data(body_walk: String, sword_walk: String, armor_walk: String, cape_normal_down: String, cape_normal_left: String, shield_normal_down: String) -> void:
	var dm = ClockGearsDataManager
	dm.init_empty()

	var sys: Dictionary = dm.get_system_config()
	sys["combat_mode"] = 1
	sys["movement_directions"] = 4
	sys["party"] = [1]
	dm.set_system_config(sys)

	dm.set_records("actors", [{
		"id": 1,
		"name": "HeroeARPG",
		"hp": 100, "mp": 20, "atk": 10, "def": 8, "mat": 5, "mdf": 6, "agi": 12, "luk": 7,
		"directional_idle": false,
		"sprite_data": {
			"animations": {
				"mover_abajo": {
					"spritesheet_path": body_walk,
					"cols": 2, "rows": 1, "fps": 6.0, "loop": true,
					"frames_data": [
						{"sheet_path": body_walk, "region": Rect2i(0, 0, 32, 32), "duration": 1.0},
						{"sheet_path": body_walk, "region": Rect2i(32, 0, 32, 32), "duration": 1.0}
					]
				},
				"atacar_abajo": {
					"spritesheet_path": body_walk,
					"cols": 2, "rows": 1, "fps": 6.0, "loop": false,
					"frames_data": [
						{"sheet_path": body_walk, "region": Rect2i(0, 0, 32, 32), "duration": 1.0},
						{"sheet_path": body_walk, "region": Rect2i(32, 0, 32, 32), "duration": 1.0}
					]
				}
			}
		},
		"arpg_equip_slots": [
			{"slot": "weapon", "label": "Arma", "category": "arpg_weapons", "piece_id": 0},
			{"slot": "armor", "label": "Armadura", "category": "arpg_armors", "piece_id": 0},
			{"slot": "shield", "label": "Escudo", "category": "arpg_shields", "piece_id": 0},
			{"slot": "accessory_1", "label": "Accesorio 1", "category": "arpg_accessories", "piece_id": 0},
			{"slot": "accessory_2", "label": "Accesorio 2", "category": "arpg_accessories", "piece_id": 0}
		]
	}])

	dm.set_records("arpg_weapons", [{
		"id": 1, "name": "Espada ARPG",
		"atk": 15, "agi": 3, "hp": 0, "mp": 0, "def": 0, "mat": 0, "mdf": 0, "luk": 0,
		"traits": [{"code": 22, "data_id": 0, "value": 0.05}],
		"draw_mode": "completo", "layer_order": 3,
		"sprite_data": {
			"animations": {
				"mover_abajo": {
					"spritesheet_path": sword_walk, "cols": 3, "rows": 1, "fps": 6.0, "loop": true, "behind_body": false,
					"frames_data": [
						{"sheet_path": sword_walk, "region": Rect2i(0, 0, 32, 32), "duration": 1.0},
						{"sheet_path": sword_walk, "region": Rect2i(32, 0, 32, 32), "duration": 1.0},
						{"sheet_path": sword_walk, "region": Rect2i(64, 0, 32, 32), "duration": 1.0}
					]
				}
			}
		}
	}])
	dm.set_records("arpg_armors", [{
		"id": 1, "name": "Cota ARPG",
		"hp": 40, "def": 12, "atk": 0, "mp": 0, "mat": 0, "mdf": 4, "agi": -1, "luk": 0,
		"draw_mode": "completo", "layer_order": 1,
		"sprite_data": {
			"animations": {
				"mover_abajo": {
					"spritesheet_path": armor_walk, "cols": 1, "rows": 1, "fps": 6.0, "loop": true, "behind_body": false,
					"frames_data": [
						{"sheet_path": armor_walk, "region": Rect2i(0, 0, 32, 32), "duration": 1.0}
					]
				}
			}
		}
	}])
	dm.set_records("arpg_shields", [{
		"id": 1, "name": "Broquel Simple",
		"def": 6, "mdf": 2,
		"draw_mode": "simple", "layer_order": 4,
		"simple_images": {
			"normal": {
				"abajo": {"path": shield_normal_down, "offset_x": 5, "offset_y": -3, "behind_body": false}
			}
		}
	}])
	dm.set_records("arpg_accessories", [{
		"id": 1, "name": "Capa Simple",
		"agi": 4, "luk": 5,
		"draw_mode": "simple", "layer_order": 2,
		"simple_images": {
			"normal": {
				"abajo": {"path": cape_normal_down, "offset_x": 0, "offset_y": 2, "behind_body": true},
				"izquierda": {"path": cape_normal_left, "offset_x": -4, "offset_y": 1, "behind_body": true}
			}
		}
	}])
	dm.reset_runtime_state()

func _initialize():
	call_deferred("_run_tests")

func _run_tests() -> void:
	var tmp_dir := "res://tmp_import_src/tanda1c_test"
	var body_walk := _make_png(tmp_dir + "/body_walk.png", 64, 32, func(x, _y):
		return Color(0.9, 0.2, 0.2, 1.0) if x < 32 else Color(0.8, 0.3, 0.3, 1.0)
	)
	var sword_walk := _make_png(tmp_dir + "/sword_walk.png", 96, 32, func(x, _y):
		if x < 32:
			return Color(0.9, 0.9, 0.1, 1.0)
		elif x < 64:
			return Color(0.8, 0.8, 0.2, 1.0)
		return Color(0.7, 0.7, 0.3, 1.0)
	)
	var armor_walk := _make_png(tmp_dir + "/armor_walk.png", 32, 32, func(_x, _y):
		return Color(0.2, 0.5, 0.9, 1.0)
	)
	var cape_normal_down := _make_png(tmp_dir + "/cape_down.png", 24, 24, func(_x, _y):
		return Color(0.5, 0.1, 0.7, 1.0)
	)
	var cape_normal_left := _make_png(tmp_dir + "/cape_left.png", 20, 20, func(_x, _y):
		return Color(0.4, 0.2, 0.8, 1.0)
	)
	var shield_normal_down := _make_png(tmp_dir + "/shield_down.png", 16, 16, func(_x, _y):
		return Color(0.2, 0.9, 0.4, 1.0)
	)

	var dm = ClockGearsDataManager

	# ---------------------------------------------------------
	# 1. Base de Datos UI (open_database carga proyecto de disco primero)
	# ---------------------------------------------------------
	var db := ClockGearsDatabaseDialog.new()
	root.add_child(db)
	await process_frame
	db.open_database()

	var sys := dm.get_system_config()
	var orig_mode := int(sys.get("combat_mode", 0))
	sys["combat_mode"] = 1
	dm.set_system_config(sys)
	db.call("select_category_by_key", "arpg_weapons")
	var btn_add_rec := _find_button_by_text(db, "+", true)
	var rec_lists := db.find_children("*", "ItemList", true, false)
	if not rec_lists.is_empty():
		var rlist := rec_lists[0] as ItemList
		if rlist != null and rlist.item_count == 0 and btn_add_rec != null:
			btn_add_rec.emit_signal("pressed")
	var arpg_wep_count_before := dm.get_records("arpg_weapons").size()

	var main_tabs: TabContainer = null
	for n in db.find_children("*", "TabContainer", true, false):
		main_tabs = n as TabContainer
		break
	var rasgos_tab := _find_tab_index(main_tabs, "Rasgos")
	check(rasgos_tab >= 0 and main_tabs != null and not main_tabs.is_tab_hidden(rasgos_tab), "Armas ARPG muestra pestana 'Rasgos'")

	var anim_tab_idx := _find_tab_index(main_tabs, "Animaciones")
	if main_tabs != null and anim_tab_idx >= 0:
		main_tabs.current_tab = anim_tab_idx
	await process_frame

	var draw_mode_opt: OptionButton = null
	for n in db.find_children("*", "OptionButton", true, false):
		var o := n as OptionButton
		var has_c := false
		var has_s := false
		for i in range(o.item_count):
			var t := o.get_item_text(i).to_lower()
			if t.find("completo") != -1: has_c = true
			if t.find("simple") != -1: has_s = true
		if has_c and has_s:
			draw_mode_opt = o
			break
	check(draw_mode_opt != null, "Editor Animaciones de pieza ARPG tiene selector Completo / Simple")

	var anim_track_opt: OptionButton = null
	if main_tabs != null and anim_tab_idx >= 0:
		var anim_ctrl := main_tabs.get_tab_control(anim_tab_idx)
		for n in anim_ctrl.find_children("*", "OptionButton", true, false):
			var opt := n as OptionButton
			for oi in range(opt.item_count):
				if str(opt.get_item_metadata(oi)) == "mover_abajo":
					anim_track_opt = opt
					break
			if anim_track_opt != null:
				break
	if draw_mode_opt != null:
		var count_c := anim_track_opt.item_count if anim_track_opt != null else 0
		draw_mode_opt.select(1) # Simple
		draw_mode_opt.emit_signal("item_selected", 1)
		draw_mode_opt.select(0) # Volver a Completo
		draw_mode_opt.emit_signal("item_selected", 0)
		var count_c_after := anim_track_opt.item_count if anim_track_opt != null else 0
		check(count_c >= 4 and count_c_after == count_c, "Cambiar entre Completo y Simple no borra ranuras de animacion de Completo")

	var ch_equipo := _find_button_by_text(db, "Equipo", true)
	check(ch_equipo != null and not ch_equipo.visible, "En modo Action RPG se oculta el capitulo 'Equipo' (turnos)")
	var ch_arpg := _find_button_by_text(db, "Action RPG", true)
	if ch_arpg != null:
		ch_arpg.emit_signal("pressed")
	var btn_arpg_armor := _find_button_by_text(db, "Armaduras ARPG", true)
	check(btn_arpg_armor != null and btn_arpg_armor.visible, "En modo Action RPG se muestran las 4 categorias de equipo ARPG")

	sys = dm.get_system_config()
	sys["combat_mode"] = 0 # Por turnos
	dm.set_system_config(sys)
	db.call("select_category_by_key", "actors")
	check(ch_equipo != null and ch_equipo.visible, "En modo Por Turnos se muestra el capitulo 'Equipo'")
	check(btn_arpg_armor != null and not btn_arpg_armor.visible, "En modo Por Turnos se ocultan las categorias de equipo ARPG sin borrar datos")
	check(dm.get_records("arpg_weapons").size() == arpg_wep_count_before, "Cambiar a Por Turnos no borra piezas ARPG guardadas")

	# Restaurar modo ARPG y probar arrastre Simple en Escudos ARPG
	sys = dm.get_system_config()
	sys["combat_mode"] = 1
	dm.set_system_config(sys)
	db.call("select_category_by_key", "arpg_shields")
	if main_tabs != null and anim_tab_idx >= 0:
		main_tabs.current_tab = anim_tab_idx
	await process_frame
	if draw_mode_opt != null:
		draw_mode_opt.select(1)
		draw_mode_opt.emit_signal("item_selected", 1)
	var chk_rest := _find_button_by_text(db, "Mostrar el resto del equipo")
	check(chk_rest != null, "Vista previa ARPG incluye checkbox 'Mostrar el resto del equipo'")
	var prev_canvas: Control = null
	if chk_rest != null:
		var preview_root := chk_rest.get_parent().get_parent()
		for n in preview_root.find_children("*", "Control", true, false):
			var c := n as Control
			if c.get_class() == "Control" and c.custom_minimum_size.y >= 100 and c.mouse_filter == Control.MOUSE_FILTER_STOP:
				prev_canvas = c
				break
	var ox_spin: SpinBox = null
	var oy_spin: SpinBox = null
	for n in db.find_children("*", "SpinBox", true, false):
		var sb := n as SpinBox
		if sb.min_value == -256 and sb.max_value == 256:
			if ox_spin == null:
				ox_spin = sb
			else:
				oy_spin = sb
				break
	if prev_canvas != null and ox_spin != null and oy_spin != null:
		var ox0 := int(ox_spin.value)
		var oy0 := int(oy_spin.value)
		var ev_down := InputEventMouseButton.new()
		ev_down.button_index = MOUSE_BUTTON_LEFT
		ev_down.pressed = true
		ev_down.position = Vector2(80, 80)
		prev_canvas.emit_signal("gui_input", ev_down)
		var ev_move := InputEventMouseMotion.new()
		ev_move.position = Vector2(96, 72)
		ev_move.button_mask = MOUSE_BUTTON_MASK_LEFT
		prev_canvas.emit_signal("gui_input", ev_move)
		var ev_up := InputEventMouseButton.new()
		ev_up.button_index = MOUSE_BUTTON_LEFT
		ev_up.pressed = false
		ev_up.position = Vector2(96, 72)
		prev_canvas.emit_signal("gui_input", ev_up)
		check(int(ox_spin.value) != ox0 or int(oy_spin.value) != oy0, "Arrastrar capa Simple en la vista previa actualiza offset X/Y")
	else:
		check(false, "Arrastrar capa Simple en la vista previa actualiza offset X/Y")

	# Restaurar combat_mode original antes de pasar a memoria de pruebas
	sys = dm.get_system_config()
	sys["combat_mode"] = orig_mode
	dm.set_system_config(sys)
	db.hide()
	db.queue_free()
	await process_frame

	# ---------------------------------------------------------
	# 2. DataManager: Huecos como lista + Estadisticas y Rasgos + Equipar/Quitar
	# ---------------------------------------------------------
	_setup_test_data(body_walk, sword_walk, armor_walk, cape_normal_down, cape_normal_left, shield_normal_down)

	check(dm.has_method("get_actor_arpg_equip_slots"), "DataManager expone get_actor_arpg_equip_slots")
	check(dm.has_method("set_actor_arpg_equip"), "DataManager expone set_actor_arpg_equip")
	check(dm.has_method("get_actor_effective_stats"), "DataManager expone get_actor_effective_stats")
	var slots = dm.get_actor_arpg_equip_slots(1)
	check(typeof(slots) == TYPE_ARRAY and slots.size() == 5, "Actor guarda 5 huecos ARPG como lista ordenada")
	var base_stats: Dictionary = dm.get_actor_effective_stats(1)
	check(int(base_stats.get("atk", -1)) == 10 and int(base_stats.get("hp", -1)) == 100, "Stats base sin equipo correctas (ATK=10, HP=100)")

	dm.set_actor_arpg_equip(1, "weapon", 1)
	dm.set_actor_arpg_equip(1, "armor", 1)
	dm.set_actor_arpg_equip(1, "shield", 1)
	dm.set_actor_arpg_equip(1, "accessory_1", 1)
	var eq_stats: Dictionary = dm.get_actor_effective_stats(1)
	check(int(eq_stats.get("atk", -1)) == 25 and int(eq_stats.get("hp", -1)) == 140 and int(eq_stats.get("def", -1)) == 26,
		"Equipar piezas ARPG suma bonos de stats (ATK=25, HP=140, DEF=26)")

	dm.set_actor_arpg_equip(1, "weapon", 0)
	var un_stats: Dictionary = dm.get_actor_effective_stats(1)
	check(int(un_stats.get("atk", -1)) == 10, "Quitar arma ARPG resta su bono de ATK (vuelve a 10)")
	dm.set_actor_arpg_equip(1, "weapon", 1)

	# ---------------------------------------------------------
	# 3. Runtime F5: Capas simultaneas, Frame clamping, Slot faltante, Simple fallback, Orden Z y Modo Turnos
	# ---------------------------------------------------------
	var player := ClockGearsPlayer.new()
	root.add_child(player)
	player.load_actor_data(1)
	player.set_direction(2) # Abajo
	player.set_action_state("caminar")

	check(player.has_method("get_active_arpg_equip_layer_count") and player.has_method("get_arpg_equip_layer_info"),
		"ClockGearsPlayer expone API de capas ARPG activas")

	var layer_sprites := {}
	for ch in player.get_children():
		if ch is Sprite2D and str(ch.name).begins_with("ARPG_Layer_"):
			layer_sprites[str(ch.name)] = ch

	check(layer_sprites.size() >= 4, "F5 dibuja multiples capas ARPG simultaneamente (sin reemplazarse ni ocultar al cuerpo)")
	var body_sprite: Sprite2D = player.get_node_or_null("Sprite")
	check(body_sprite != null and body_sprite.visible and body_sprite.texture != null, "Cuerpo base del jugador sigue visible con su sprite")

	var cape_sp: Sprite2D = player.get_node_or_null("ARPG_Layer_accessory_1")
	var armor_sp: Sprite2D = player.get_node_or_null("ARPG_Layer_armor")
	var weapon_sp: Sprite2D = player.get_node_or_null("ARPG_Layer_weapon")
	var shield_sp: Sprite2D = player.get_node_or_null("ARPG_Layer_shield")

	check(cape_sp != null and cape_sp.visible and cape_sp.z_index < body_sprite.z_index, "Capa con behind_body=true tiene z_index menor que el cuerpo")
	check(armor_sp != null and armor_sp.visible and weapon_sp != null and weapon_sp.visible and shield_sp != null and shield_sp.visible and armor_sp.z_index > body_sprite.z_index and armor_sp.z_index < weapon_sp.z_index and weapon_sp.z_index < shield_sp.z_index,
		"Orden de capas delante del cuerpo respeta Armadura(1) < Arma(3) < Escudo(4)")
	check(shield_sp != null and shield_sp.texture != null and shield_sp.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST,
		"Capa Simple tiene PNG cargado y filtro NEAREST")

	# Avanzar animacion 1 tick (duration=1.0s) y verificar clamping de armadura (1 frame) vs espada (3 frames)
	player.debug_advance_animation(1.05)
	check(armor_sp != null and armor_sp.visible and armor_sp.region_rect.position.x == 0,
		"Armadura de 1 frame hace clamp al ultimo frame cuando el cuerpo avanza al frame 1")
	check(weapon_sp != null and weapon_sp.visible and weapon_sp.region_rect.position.x == 32,
		"Espada completo sincroniza exactamente el frame 1 con el cuerpo")

	# Respaldo Simple: estado 'atacar' sin imagen propia cae a 'normal'
	player.set_action_state("atacar")
	check(shield_sp != null and shield_sp.visible and shield_sp.texture != null, "Modo Simple: estado 'atacar' sin imagen cae al respaldo 'normal'")
	# Espada completo sin 'atacar_abajo' no falla y oculta esa capa
	check(weapon_sp == null or not weapon_sp.visible, "Modo Completo: ranura faltante no da error y no dibuja nada")

	# Respaldo Simple modo 8: diagonal 'abajo_izq' (dir 1) sin imagen diagonal cae a 'izquierda' (4)
	sys = dm.get_system_config()
	sys["movement_directions"] = 8
	dm.set_system_config(sys)
	player.set_action_state("caminar")
	player.set_direction(1) # Diagonal Abajo-Izq -> debe respaldarse en Cape Normal Izquierda (20x20)
	check(cape_sp != null and cape_sp.visible and cape_sp.texture != null and cape_sp.texture.get_width() == 20,
		"Modo Simple en 8 direcciones: diagonal faltante cae a direccion horizontal mas cercana (izquierda)")

	# Ocultacion de capas al cambiar a Turn-Based (0) sin perder datos equipados
	sys["combat_mode"] = 0
	dm.set_system_config(sys)
	player.load_actor_data(1)
	var any_vis_tb := false
	for ch in player.get_children():
		if ch is Sprite2D and str(ch.name).begins_with("ARPG_Layer_") and ch.visible:
			any_vis_tb = true
	check(not any_vis_tb, "En modo Por Turnos no se dibujan capas de equipo ARPG sobre el jugador")
	sys["combat_mode"] = 1
	dm.set_system_config(sys)
	player.load_actor_data(1)
	player.set_direction(2)
	player.set_action_state("caminar")

	# ---------------------------------------------------------
	# 4. Evento Cambiar Equipo ARPG + Overlay F3 Equipo ARPG + Persistencia
	# ---------------------------------------------------------
	var cmd_sel := ClockGearsCommandSelector.new()
	root.add_child(cmd_sel)
	var btn_cmd_equip := _find_button_by_text(cmd_sel, "Cambiar Equipo ARPG")
	check(btn_cmd_equip != null, "Selector de comandos incluye 'Cambiar Equipo ARPG'")

	var interp := ClockGearsEventInterpreter.new()
	root.add_child(interp)
	interp.setup_interpreter(null, player, null)
	interp.start_command_list([{"type": "change_arpg_equip", "code": 206, "actor_id": 1, "slot": "weapon", "piece_id": 0}], null)
	check(int(dm.call("get_actor_arpg_equip", 1, "weapon")) == 0, "Interprete ejecuta 'Cambiar Equipo ARPG' para quitar pieza")
	interp.start_command_list([{"type": "change_arpg_equip", "code": 206, "actor_id": 1, "slot": "weapon", "piece_id": 1}], null)
	check(int(dm.call("get_actor_arpg_equip", 1, "weapon")) == 1, "Interprete ejecuta 'Cambiar Equipo ARPG' para equipar pieza")

	# F3 Debug Overlay: pestana "Equipo ARPG" y cambio en vivo
	var dbg := ClockGearsDebugOverlay.new()
	root.add_child(dbg)
	dbg.setup_overlay(null, player, null)
	var dbg_tabs: TabContainer = null
	for n in dbg.find_children("*", "TabContainer", true, false):
		dbg_tabs = n as TabContainer
		break
	var dbg_arpg_tab := _find_tab_index(dbg_tabs, "Equipo ARPG")
	check(dbg_arpg_tab >= 0, "Overlay F3 incluye pestana 'Equipo ARPG'")
	dbg.call("set_arpg_equip_debug_slot", 1, "weapon", 0)
	check(int(dm.call("get_actor_arpg_equip", 1, "weapon")) == 0 and (weapon_sp == null or not weapon_sp.visible),
		"Overlay F3 quita pieza ARPG y oculta su capa en tiempo real")
	dbg.call("set_arpg_equip_debug_slot", 1, "weapon", 1)
	check(int(dm.call("get_actor_arpg_equip", 1, "weapon")) == 1 and weapon_sp != null and weapon_sp.visible,
		"Overlay F3 equipa pieza ARPG y dibuja su capa en tiempo real")

	# Editor de Eventos: subdialogo 'Cambiar Equipo ARPG' + selector de pieza filtrado por hueco
	var ev_dlg := ClockGearsEventEditorDialog.new()
	root.add_child(ev_dlg)
	ev_dlg.edit_database_event("common_events", 1, "TestEquipEvent", [])
	var sub_aeq_dlg: ConfirmationDialog = null
	var piece_picker_dlg: ConfirmationDialog = null
	for n in ev_dlg.find_children("*", "ConfirmationDialog", true, false):
		var cd := n as ConfirmationDialog
		if cd.title.find("Cambiar Equipo ARPG") != -1:
			sub_aeq_dlg = cd
		elif cd.title.find("Seleccionar Pieza ARPG") != -1:
			piece_picker_dlg = cd
	check(sub_aeq_dlg != null and piece_picker_dlg != null, "Editor de Eventos incluye subdialogo 'Cambiar Equipo ARPG' y selector filtrado de piezas")
	if sub_aeq_dlg != null and piece_picker_dlg != null:
		var btn_sel_piece := _find_button_by_text(sub_aeq_dlg, "Seleccionar...", true)
		if btn_sel_piece != null:
			btn_sel_piece.emit_signal("pressed")
		var plist: ItemList = null
		for n in piece_picker_dlg.find_children("*", "ItemList", true, false):
			plist = n as ItemList
			break
		check(plist != null and plist.item_count == 2 and plist.get_item_text(1).find("Espada ARPG") != -1,
			"Selector de pieza ARPG filtra exactamente a la categoria del hueco seleccionado (solo Arma + Quitar)")
		if plist != null and plist.item_count >= 2:
			plist.select(1)
			piece_picker_dlg.emit_signal("confirmed")
		sub_aeq_dlg.emit_signal("confirmed")
		var pages: Array = ev_dlg.get_working_pages()
		var cmds: Array = pages[0].get("commands", []) if not pages.is_empty() else []
		check(cmds.size() >= 1 and String(cmds[0].get("type", "")) == "change_arpg_equip" and int(cmds[0].get("piece_id", 0)) == 1,
			"Confirmar 'Cambiar Equipo ARPG' inserta comando change_arpg_equip con piece_id=1 en la pagina")

	# Guardar y recargar partida / proyecto
	var save_dir := "res://tmp_import_src/tanda1c_save"
	check(dm.save_all(save_dir), "DataManager.save_all guarda datos y estado ARPG")
	dm.init_empty()
	check(dm.load_all(save_dir), "DataManager.load_all recarga datos y estado ARPG")
	var reloaded_stats: Dictionary = dm.get_actor_effective_stats(1)
	check(int(reloaded_stats.get("atk", -1)) == 25, "Equipo ARPG equipado y estadisticas persisten al guardar y recargar")

	print("RESUMEN TANDA 1C -> PASS:", passed, " FAIL:", failed)
	ev_dlg.queue_free()
	player.queue_free()
	cmd_sel.queue_free()
	interp.queue_free()
	dbg.queue_free()
	quit(1 if failed > 0 else 0)
