extends SceneTree

func _init():
	print("=== TEST REAL: ATAQUE Y MUERTE ===")
	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm:
		dm.load_all("res://clockgears_data")

	var interp = ClockGearsEventInterpreter.new()
	root.add_child(interp)

	var cmd = {
		"type": "battle",
		"troop_id": 1,
		"can_escape": true,
		"can_lose": true
	}
	interp.start_command_list([cmd])

	var bs: ClockGearsBattleScene = null
	for c in interp.get_children():
		if c is ClockGearsBattleScene:
			bs = c
			break

	print("Ataque 1:")
	bs.select_action_attack(0)
	print("  bs viva tras ataque 1?: ", is_instance_valid(bs) and not bs.is_queued_for_deletion())

	print("Ataque 2 (mata al enemigo):")
	bs.select_action_attack(0)
	print("  bs viva tras ataque 2?: ", is_instance_valid(bs) and not bs.is_queued_for_deletion())
	print("  Interprete state tras muerte enemigo: ", interp.get_state())

	quit(0)
