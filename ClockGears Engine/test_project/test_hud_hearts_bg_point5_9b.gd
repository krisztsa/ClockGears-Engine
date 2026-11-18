extends SceneTree

func _init():
	print("=== TEST PUNTO 5: FONDO DE CORAZONES DEL HUD ===")
	var root_node = Node2D.new()
	root.add_child(root_node)

	var hud = ClockGearsHudOverlay.new()
	root_node.add_child(hud)

	# --- 1. Widget por defecto: show_background debe ser false ---
	var def = ClockGearsDataManager.make_default_hud_widget("hearts")
	if def.get("show_background", true) != false:
		print("FAIL: Widget 'hearts' por defecto deberia tener show_background=false, obtenido: ", def.get("show_background", "AUSENTE"))
		quit(1); return
	print("OK: Widget 'hearts' por defecto tiene show_background=false.")

	# --- 2. Con show_background=false (default), el BgWindow del widget es invisible ---
	var cfg_no_bg = ClockGearsDataManager.normalize_hud_config({"widgets": [def]})
	hud.apply_hud_config(cfg_no_bg)
	var wnode: Control = hud.get_widget_node("corazones")
	if wnode == null:
		print("FAIL: No se encontro nodo del widget 'corazones'.")
		quit(1); return
	var bg_panel_no_bg: Panel = wnode.get_node_or_null("BgWindow")
	if bg_panel_no_bg == null:
		print("FAIL: BgWindow no encontrado en el widget de corazones.")
		quit(1); return
	if bg_panel_no_bg.is_visible():
		print("FAIL: BgWindow deberia estar INVISIBLE cuando show_background=false, pero esta visible.")
		quit(1); return
	print("OK: BgWindow INVISIBLE con show_background=false (transparente por defecto).")

	# --- 3. Con show_background=true, el BgWindow es visible ---
	var def_with_bg = def.duplicate()
	def_with_bg["show_background"] = true
	var cfg_with_bg = ClockGearsDataManager.normalize_hud_config({"widgets": [def_with_bg]})
	hud.apply_hud_config(cfg_with_bg)
	var wnode2: Control = hud.get_widget_node("corazones")
	if wnode2 == null:
		print("FAIL: No se encontro nodo del widget 'corazones' en config con fondo.")
		quit(1); return
	var bg_panel_with_bg: Panel = wnode2.get_node_or_null("BgWindow")
	if bg_panel_with_bg == null:
		print("FAIL: BgWindow no encontrado en config con fondo.")
		quit(1); return
	if not bg_panel_with_bg.is_visible():
		print("FAIL: BgWindow deberia estar VISIBLE cuando show_background=true.")
		quit(1); return
	print("OK: BgWindow VISIBLE con show_background=true.")

	# --- 3b. Color y opacidad propios del fondo ---
	var def_col = def.duplicate()
	def_col["show_background"] = true
	def_col["bg_color"] = "#ff0000"
	def_col["bg_opacity"] = 50
	hud.apply_hud_config(ClockGearsDataManager.normalize_hud_config({"widgets": [def_col]}))
	var sb = hud.get_widget_node("corazones").get_node("BgWindow").get_theme_stylebox("panel")
	if not (sb is StyleBoxFlat) or absf(sb.bg_color.r - 1.0) > 0.01 or absf(sb.bg_color.a - 0.5) > 0.01:
		print("FAIL: color/opacidad del fondo no aplicados: ", sb.bg_color if sb is StyleBoxFlat else sb)
		quit(1); return
	print("OK: Fondo usa color #ff0000 y opacidad 50%.")

	# --- 4. normalize_hud_widget preserva show_background ---
	var w_norm_false = ClockGearsDataManager.normalize_hud_widget({"type": "hearts", "show_background": false}, 1)
	if w_norm_false.get("show_background", true) != false:
		print("FAIL: normalize_hud_widget no conserva show_background=false.")
		quit(1); return
	var w_norm_true = ClockGearsDataManager.normalize_hud_widget({"type": "hearts", "show_background": true}, 1)
	if w_norm_true.get("show_background", false) != true:
		print("FAIL: normalize_hud_widget no conserva show_background=true.")
		quit(1); return
	print("OK: normalize_hud_widget conserva show_background correctamente.")

	# --- 5. Migración: widget hearts sin show_background -> false por defecto ---
	var legacy_widget = {"type": "hearts", "hp_per_heart": 20}
	var migrated = ClockGearsDataManager.normalize_hud_widget(legacy_widget, 1)
	if migrated.get("show_background", true) != false:
		print("FAIL: Widget hearts legacy sin show_background deberia migrar a false, obtenido: ", migrated.get("show_background", "AUSENTE"))
		quit(1); return
	print("OK: Widget hearts legacy migra con show_background=false (sin romper datos existentes).")

	print("ALL PASS: Fondo de corazones del HUD transparente por defecto y casilla 'Mostrar fondo' verificados.")
	quit(0)
