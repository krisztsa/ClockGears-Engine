extends SceneTree

func _init():
	print("=== INICIO REPRODUCCIÓN CRASH BOTON X EN TIRA DE ANIMACIÓN ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm:
		dm.load_all()

	var db = ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.load_from_disk()
	db.select_category(7) # CAT_ENEMIES

	var all_buttons = db.find_children("*", "Button", true, false)
	var x_buttons = []
	for b in all_buttons:
		if b.get_text() == "x":
			x_buttons.append(b)

	print("Botones 'x' encontrados en la tira: ", x_buttons.size())
	if x_buttons.size() > 0:
		var btn = x_buttons[0]
		print("Pulsando botón x (emit_signal('pressed'))...")
		btn.emit_signal("pressed")
		print("Tras emitir 'pressed'")

	print("=== FIN SCRIPT REPRODUCCIÓN ===")
	quit(0)
