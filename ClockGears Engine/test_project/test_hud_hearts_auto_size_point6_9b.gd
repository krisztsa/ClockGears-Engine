extends SceneTree

# Punto 6: tamaño automático de corazones. Ningún corazón se sale del widget
# con tamaños 32, 64, 96 y 128, en 960x540, 1366x768 y 1920x1080.

func _fail(msg: String) -> void:
	print("FAIL: ", msg)
	quit(1)

func _check_hearts_inside(hud, label: String, res: Vector2) -> bool:
	var wnode: Control = hud.get_widget_node("corazones")
	if wnode == null:
		_fail(label + " sin nodo de widget")
		return false
	var root_rect := Rect2(wnode.global_position, wnode.size)
	var vp_rect := Rect2(Vector2.ZERO, res)
	var count := 0
	for n in wnode.find_children("*", "TextureRect", true, false):
		var tr: TextureRect = n
		if not tr.visible or tr.texture == null:
			continue
		count += 1
		var r := Rect2(tr.global_position, tr.size)
		if not root_rect.grow(0.51).encloses(r):
			_fail("%s corazón fuera del widget: %s no cabe en %s" % [label, r, root_rect])
			return false
	if count == 0:
		_fail(label + " sin corazones visibles")
		return false
	if not vp_rect.grow(0.51).encloses(root_rect):
		_fail("%s widget fuera de pantalla: %s" % [label, root_rect])
		return false
	return true

func _init():
	print("=== TEST PUNTO 6: TAMAÑO AUTOMÁTICO DE CORAZONES ===")
	var holder := Control.new()
	root.add_child(holder)
	var hud = ClockGearsHudOverlay.new()
	holder.add_child(hud)

	var def: Dictionary = ClockGearsDataManager.make_default_hud_widget("hearts")
	if def.get("auto_size", false) != true:
		_fail("widget nuevo debería tener auto_size=true"); return
	print("OK: widget nuevo con auto_size=true.")

	var legacy = ClockGearsDataManager.normalize_hud_widget({"type": "hearts", "width": 200, "height": 40}, 1)
	if legacy.get("auto_size", true) != false or int(legacy.get("width", 0)) != 200:
		_fail("widget antiguo debería migrar a modo manual conservando ancho"); return
	print("OK: widget antiguo migra a modo manual (ancho 200 conservado).")

	for res in [Vector2(960, 540), Vector2(1366, 768), Vector2(1920, 1080)]:
		hud.set_viewport_size_override(res)
		for sz in [32, 64, 96, 128]:
			var w = def.duplicate()
			w["heart_size"] = sz
			w["spacing"] = 4
			w["max_hearts_per_row"] = 10
			w["max_rows"] = 2
			w["width"] = 160
			w["height"] = 36
			hud.apply_hud_config(ClockGearsDataManager.normalize_hud_config({"widgets": [w]}))
			if not _check_hearts_inside(hud, "%dx%d tam %d" % [res.x, res.y, sz], res):
				return
		print("OK: %dx%d - tamaños 32/64/96/128 sin cortes ni salir de pantalla." % [res.x, res.y])

	# Modo manual sigue respetando ancho/alto fijos.
	hud.set_viewport_size_override(Vector2(960, 540))
	var m = def.duplicate()
	m["auto_size"] = false
	m["width"] = 160
	m["height"] = 36
	m["heart_size"] = 20
	hud.apply_hud_config(ClockGearsDataManager.normalize_hud_config({"widgets": [m]}))
	var mn: Control = hud.get_widget_node("corazones")
	if absf(mn.size.x - 160) > 1 or absf(mn.size.y - 36) > 1:
		_fail("modo manual no respeta 160x36: %s" % mn.size); return
	print("OK: modo manual conserva 160x36.")

	print("ALL PASS: Tamaño automático de corazones verificado.")
	quit(0)
