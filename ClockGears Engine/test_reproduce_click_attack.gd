extends SceneTree

func _init():
	print("--- TEST REPRODUCIR COMBATE REAL Y CLIC EN ATACAR ---")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm:
		dm.load_all("res://clockgears_data")

	var scene = ClockGearsBattleScene.new()
	root.add_child(scene)

	# Setup battle troop 1
	scene.setup_battle(1, true, true, "")

	var ctrl = scene.get_controller()
	print("Heroes count: ", ctrl.get_party_units().size())
	print("Enemies count: ", ctrl.get_enemy_units().size())
	for e in ctrl.get_enemy_units():
		print("  Enemigo: ", e.get_name(), " HP: ", e.get_hp(), " graphic: ", e.get_graphic_path())

	var turn_order = ctrl.get_turn_order()
	print("Turn order count: ", turn_order.size())
	for u in turn_order:
		print("  Unidad en turno: ", u.get_name(), " (is_actor=", u.get_is_actor(), ")")

	# Simular clic en el boton btn_attack
	var btn_attack = scene.find_child("actions_vbox", true, false).get_child(0)
	print("Boton Atacar encontrado: ", btn_attack.get_text(), " disabled: ", btn_attack.is_disabled())

	print("Pulsando Atacar...")
	btn_attack.emit_signal("pressed")

	# Ver que hay en sub_menu_vbox
	var sub_menu = scene.find_child("sub_menu_vbox", true, false)
	print("Submenu visible: ", sub_menu.get_parent().is_visible())
	print("Submenu pos: ", sub_menu.get_parent().get_position())
	print("Submenu child count: ", sub_menu.get_child_count())
	for i in range(sub_menu.get_child_count()):
		var c = sub_menu.get_child(i)
		print("  Submenu item ", i, ": ", c.get_text() if c is Button or c is Label else c.name)

	# Ahora simular clic en el primer objetivo (enemigo)
	if sub_menu.get_child_count() > 1:
		var target_btn = sub_menu.get_child(1) # child 0 es el label 'Seleccionar Objetivo:'
		if target_btn is Button:
			print("Seleccionando objetivo: ", target_btn.get_text())
			target_btn.emit_signal("pressed")

	print("Fin de prueba sin crash.")
	scene.free()
	quit(0)
