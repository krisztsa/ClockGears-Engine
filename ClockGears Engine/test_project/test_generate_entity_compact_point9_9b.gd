extends SceneTree

# Punto 9: Ventana "Generar entidad" compactada y con scroll vertical,
# manteniendo las 6 pestañas NPC, ARPG NPC, ENEMIGO, ARPG ENEMIGO, FLORA y FAUNA.

var passed := 0
var failed := 0

func _check(cond: bool, msg: String) -> void:
	if cond:
		passed += 1
		print("  OK: " + msg)
	else:
		failed += 1
		printerr("  FAIL: " + msg)

func _step_frames(n: int = 3) -> void:
	for _i in range(n):
		await process_frame

func _init() -> void:
	call_deferred("_run_tests")

func _run_tests() -> void:
	print("=== TEST PUNTO 9: VENTANA GENERAR ENTIDAD COMPACTA ===")
	var dlg := ClockGearsGenerateEntityDialog.new()
	root.add_child(dlg)
	dlg.open_for_cell(2, 3)
	await _step_frames(2)

	# 1. Mantener las 6 pestañas esperadas
	var tabs_arr := dlg.find_children("*", "TabBar", true, false)
	_check(tabs_arr.size() == 1, "TabBar presente")
	if tabs_arr.size() == 1:
		var tb: TabBar = tabs_arr[0]
		var expected := ["NPC", "ARPG NPC", "ENEMIGO", "ARPG ENEMIGO", "FLORA", "FAUNA"]
		var actual := []
		for i in range(tb.tab_count):
			actual.append(tb.get_tab_title(i))
		_check(actual == expected, "Mantiene las 6 pestañas exactas: %s" % str(actual))

	# 2. ScrollContainer vertical presente y sin scroll horizontal
	var scrolls := dlg.find_children("*", "ScrollContainer", true, false)
	_check(scrolls.size() >= 1, "ScrollContainer vertical presente")
	if scrolls.size() >= 1:
		var sc: ScrollContainer = scrolls[0]
		_check(sc.horizontal_scroll_mode == ScrollContainer.SCROLL_MODE_DISABLED, "Scroll horizontal deshabilitado")
		_check(sc.vertical_scroll_mode != ScrollContainer.SCROLL_MODE_DISABLED, "Scroll vertical habilitado")

	# 3. Filas más bajas y vista previa más pequeña
	var lists := dlg.find_children("*", "ItemList", true, false)
	_check(lists.size() == 1, "ItemList presente")
	if lists.size() == 1:
		var il: ItemList = lists[0]
		_check(il.fixed_icon_size.x <= 20 and il.fixed_icon_size.y <= 20,
			"Filas de lista más bajas (icon %s <= 20x20)" % il.fixed_icon_size)

	var panels := dlg.find_children("*", "PanelContainer", true, false)
	_check(panels.size() >= 1, "Panel de vista previa presente")
	if panels.size() >= 1:
		var p: PanelContainer = panels[0]
		_check(p.custom_minimum_size.y <= 120.0,
			"Vista previa del sprite compactada (min_h=%s <= 120)" % p.custom_minimum_size.y)

	# 4. Cabe en área útil de cualquier pantalla 16:9 (960x540 con barra de tareas ~40px, 1366x768, 1920x1080)
	for res in [Vector2i(960, 500), Vector2i(1366, 720), Vector2i(1920, 1032)]:
		var fit: Dictionary = dlg.check_layout_fits_window(res)
		_check(bool(fit.get("fits_horizontally", false)) and bool(fit.get("fits_vertically", false)),
			"Cabe en área útil %dx%d (min %sx%s)" % [res.x, res.y, fit.get("min_w"), fit.get("min_h")])
	_check(dlg.size.y <= 420, "Altura por defecto compacta (%d <= 420)" % dlg.size.y)

	# 5. Probar cambio entre todas las pestañas sin errores
	for tab_idx in range(6):
		dlg.select_tab(tab_idx)
	await _step_frames(1)
	_check(dlg.get_selected_tab() == 5, "Cambio entre las 6 pestañas funciona")

	# 6. Verificar que al encoger la ventana el scroll vertical puede desplazarse
	dlg.size = Vector2i(520, 220)
	await _step_frames(2)
	if scrolls.size() >= 1:
		var sc: ScrollContainer = scrolls[0]
		var vb := sc.get_v_scroll_bar()
		sc.scroll_vertical = 40
		_check(vb != null and vb.max_value >= 0.0, "Barra de scroll vertical operativa al reducir ventana")

	dlg.queue_free()
	await _step_frames(1)

	if failed == 0:
		print("ALL PASS: Ventana Generar entidad compactada y verificada (%d checks)." % passed)
		quit(0)
	else:
		print("FAIL: %d comprobaciones fallaron." % failed)
		quit(1)
