extends SceneTree

const TIMEOUT_S := 30.0
var _elapsed := 0.0
var _dlg = null
var _apply_btn: Button = null
var _setup_done := false
var _step := 0
var _categories_to_test := []
var _cur_cat_idx := 0
var _fails := 0

func _process(delta: float) -> bool:
	_elapsed += delta
	if _elapsed > TIMEOUT_S:
		print("FAIL: Timeout alcanzado en test_database_apply_button_all_categories")
		quit(1)
		return true

	if not _setup_done:
		_setup_done = true
		_dlg = ClassDB.instantiate("ClockGearsDatabaseDialog")
		if _dlg == null:
			print("FAIL: No se pudo instanciar ClockGearsDatabaseDialog")
			quit(1)
			return true
		root.add_child(_dlg)
		_dlg.open_database()
		_apply_btn = _find_button_by_text(_dlg, "Aplicar")
		if _apply_btn == null:
			print("FAIL: Botón 'Aplicar' no encontrado")
			quit(1)
			return true
		
		# Probar todas las categorías de la Base de Datos (0 a 25)
		_categories_to_test = [
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25
		]
		_cur_cat_idx = 0
		_step = 0
		print("=== INICIANDO TEST PERMANENTE: BOTON APLICAR EN CATEGORIAS ===")
		return false

	var cat = _categories_to_test[_cur_cat_idx]
	match _step:
		0:
			_dlg.select_category(cat)
			_step = 1
		1:
			# Verificar que sin cambios está opaco
			if not _apply_btn.is_disabled() or _apply_btn.modulate.a > 0.5:
				print("FAIL: Categoria %d: Aplicar NO esta opaco al iniciar (disabled=%s, alpha=%.2f)" % [cat, _apply_btn.is_disabled(), _apply_btn.modulate.a])
				_fails += 1
			else:
				# Editar un campo en la categoría
				var edited = false
				var sp = _find_editable_spinbox(_dlg)
				if sp:
					sp.value = sp.value + 1
					edited = true
				else:
					var le = _find_editable_line_edit(_dlg)
					if le:
						le.text = le.text + " Mod"
						le.emit_signal("text_changed", le.text)
						edited = true
				if not edited:
					print("WARN: Categoria %d no encontro campo editable" % cat)
			_step = 2
		2:
			# Verificar que con cambios pendientes está en aspecto normal
			if _apply_btn.is_disabled() or _apply_btn.modulate.a < 0.99:
				print("FAIL: Categoria %d: Aplicar NO paso a aspecto normal tras cambio (disabled=%s, alpha=%.2f)" % [cat, _apply_btn.is_disabled(), _apply_btn.modulate.a])
				_fails += 1
			else:
				print("OK: Categoria %d: Aplicar paso a aspecto normal" % cat)
			# Pulsar Aplicar
			_apply_btn.emit_signal("pressed")
			_step = 3
		3:
			# Verificar que volvió a opaco
			if not _apply_btn.is_disabled() or _apply_btn.modulate.a > 0.5:
				print("FAIL: Categoria %d: Aplicar NO volvio a opaco tras aplicar (disabled=%s, alpha=%.2f)" % [cat, _apply_btn.is_disabled(), _apply_btn.modulate.a])
				_fails += 1
			else:
				print("OK: Categoria %d: Aplicar volvio a opaco" % cat)
			_cur_cat_idx += 1
			if _cur_cat_idx >= _categories_to_test.size():
				_final_report()
				return true
			else:
				_step = 0

	return false

func _final_report():
	print("=== RESUMEN TEST BOTON APLICAR ===")
	if _fails == 0:
		print("ALL PASS: El botón Aplicar pasa a aspecto normal con cambios y a opaco al aplicar en todas las categorías.")
		quit(0)
	else:
		print("TOTAL FAILS: %d" % _fails)
		quit(1)

func _find_button_by_text(node: Node, text: String) -> Button:
	if node is Button and node.text == text:
		return node
	for c in node.get_children(true):
		var b = _find_button_by_text(c, text)
		if b:
			return b
	return null

func _find_editable_line_edit(node: Node) -> LineEdit:
	if node is LineEdit and node.is_editable() and node.is_visible_in_tree() and not ("filter" in node.name.to_lower() or "search" in node.name.to_lower()):
		# Evitar line edits que sean hijos de SpinBox
		if not (node.get_parent() is SpinBox):
			return node
	for c in node.get_children(true):
		var le = _find_editable_line_edit(c)
		if le:
			return le
	return null

func _find_editable_spinbox(node: Node) -> SpinBox:
	if node is SpinBox and node.is_editable() and node.is_visible_in_tree():
		return node
	for c in node.get_children(true):
		var sp = _find_editable_spinbox(c)
		if sp:
			return sp
	return null
