extends SceneTree

# Punto 2 (9B-2): Insertar "Transferir Jugador" abre directamente el selector visual;
# cancelar no crea el comando; confirmar sí lo crea; editar vuelve a abrir el selector.

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

func _find_button_by_text(node: Node, needle: String) -> Button:
	if node == null:
		return null
	if node is Button:
		var b := node as Button
		if not (b is OptionButton) and b.text.find(needle) >= 0:
			return b
	for c in node.get_children():
		var res := _find_button_by_text(c, needle)
		if res != null:
			return res
	return null

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	print("=== TEST PUNTO 2 (9B-2): TRANSFERIR JUGADOR AL INSERTAR ===")
	var ev := ClockGearsEvent.new()
	root.add_child(ev)
	var dlg := ClockGearsEventEditorDialog.new()
	root.add_child(dlg)
	dlg.edit_event(ev)
	await _step_frames(2)

	var h: Dictionary = dlg.get_menu_command_ui_handles()
	var selector_win: Window = h.get("command_selector", null)
	var dlg_teleport: ConfirmationDialog = h.get("dlg_teleport", null)
	var tele_sel = dlg.get_teleport_selector()
	_check(selector_win != null and dlg_teleport != null and tele_sel != null, "Controles de catálogo y selector visual presentes")

	# 1. Simular pulsar botón "+ Insertar..." y elegir "Transferir Jugador..."
	var btn_insert := _find_button_by_text(dlg, "Insertar")
	_check(btn_insert != null, "Botón '+ Insertar...' encontrado")
	if btn_insert != null:
		btn_insert.emit_signal("pressed")
		await _step_frames(2)
		_check(selector_win.visible, "Catálogo de comandos abierto")

		var btn_tp := _find_button_by_text(selector_win, "Transferir Jugador")
		_check(btn_tp != null, "Botón 'Transferir Jugador' encontrado en catálogo")
		if btn_tp != null:
			btn_tp.emit_signal("pressed")
			await _step_frames(2)
			_check(not selector_win.visible, "Catálogo se cierra al pulsar Transferir Jugador")
			_check(dlg_teleport.visible, "Selector visual de Transferir Jugador se abre directamente al insertar")

			# 2. Cancelar -> NO crea el comando
			dlg_teleport.hide()
			dlg_teleport.emit_signal("canceled")
			await _step_frames(1)
			var pages_after_cancel: Array = dlg.get_working_pages()
			var cmds_after_cancel: Array = pages_after_cancel[0].get("commands", [])
			_check(cmds_after_cancel.is_empty(), "Cancelar no crea ningún comando (count=%d)" % cmds_after_cancel.size())

			# 3. Insertar de nuevo y confirmar -> SÍ crea el comando
			btn_insert.emit_signal("pressed")
			await _step_frames(1)
			btn_tp.emit_signal("pressed")
			await _step_frames(2)
			_check(dlg_teleport.visible, "Selector visual vuelve a abrirse al reinsertar")
			tele_sel.set_target_coords(4, 7)
			tele_sel.set_target_direction("Arriba")
			dlg_teleport.get_ok_button().emit_signal("pressed")
			await _step_frames(1)

			var pages_after_ok: Array = dlg.get_working_pages()
			var cmds_after_ok: Array = pages_after_ok[0].get("commands", [])
			_check(cmds_after_ok.size() == 1, "Confirmar crea el comando teleport (count=%d)" % cmds_after_ok.size())
			if cmds_after_ok.size() == 1:
				var c: Dictionary = cmds_after_ok[0]
				_check(String(c.get("type", "")) == "teleport" and int(c.get("x", -1)) == 4 and int(c.get("y", -1)) == 7 and String(c.get("dir", "")) == "Arriba",
					"Comando creado guarda coords (4, 7) y dir Arriba")

	dlg.queue_free()
	ev.queue_free()
	await _step_frames(1)

	if failed == 0:
		print("ALL PASS: Transferir Jugador al insertar verificado (%d checks)." % passed)
		quit(0)
	else:
		print("FAIL: %d comprobaciones fallaron." % failed)
		quit(1)
