extends SceneTree

var passed: int = 0
var failed: int = 0

func _pump_frames(count: int) -> void:
	for _i in range(count):
		await process_frame

func _find_node_named(node: Node, target_name: String) -> Node:
	if node.name == target_name:
		return node
	for child in node.get_children():
		var r = _find_node_named(child, target_name)
		if r != null:
			return r
	return null

func _find_first_of_type(node: Node, type_name: String, name_hint: String) -> Node:
	if node.get_class() == type_name and name_hint.to_lower() in node.name.to_lower():
		return node
	for child in node.get_children():
		var r = _find_first_of_type(child, type_name, name_hint)
		if r != null:
			return r
	return null

func _find_autowrap_label(node: Node) -> Node:
	if node.get_class() == "Label" and node.get_autowrap_mode() != 0:
		return node
	for child in node.get_children():
		var r = _find_autowrap_label(child)
		if r != null:
			return r
	return null

func _initialize():
	root.set_name("Root")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm == null:
		print("FAIL: DataManager no disponible")
		quit(1)
		return
	var menu = ClassDB.instantiate("ClockGearsGameMenu")
	if menu == null:
		print("FAIL: No se pudo instanciar ClockGearsGameMenu")
		quit(1)
		return
	root.add_child(menu)
	menu.open_menu()
	menu.open_subscreen("items")

	# 2 frames para que el layout se inicialice
	await _pump_frames(2)

	var desc = _find_first_of_type(menu, "Label", "item_detail_desc")
	if desc == null:
		desc = _find_autowrap_label(menu)
	var scroll = _find_first_of_type(menu, "ScrollContainer", "item_desc_scroll")

	var desc_sz = desc.size if desc != null else Vector2(-1, -1)
	var scroll_sz = scroll.size if scroll != null else Vector2(-1, -1)
	print("[fase-2frames] desc.size=%s scroll.size=%s" % [desc_sz, scroll_sz])

	# 30 frames más: simular cambio de selección
	await _pump_frames(28)
	if menu.has_method("_on_item_row_selected"):
		menu.call("_on_item_row_selected", 1)
	await _pump_frames(2)

	desc = _find_first_of_type(menu, "Label", "item_detail_desc")
	if desc == null:
		desc = _find_autowrap_label(menu)
	desc_sz = desc.size if desc != null else Vector2(-1, -1)
	print("[cambio-entrada] desc.size=%s" % desc_sz)

	# Check final
	await _pump_frames(30)
	desc = _find_first_of_type(menu, "Label", "item_detail_desc")
	if desc == null:
		desc = _find_autowrap_label(menu)
	var ok = true
	if desc != null:
		var w = desc.size.x
		if w < 50:
			print("FAIL: item_detail_desc ancho=%s < 50 — CAUSA RAIZ CONFIRMADA (autowrap con ancho 0)" % w)
			ok = false
		else:
			print("OK: item_detail_desc ancho=%s >= 50" % w)
	else:
		print("WARN: No se encontró label de descripción")
	if ok:
		print("PASS: descripción no está en vertical")
	quit(0 if ok else 1)
