extends SceneTree

func _init():
	print("--- REPRODUCCION DE BUGS DE FASE 8A ---")

	var dm = Engine.get_singleton("ClockGearsDataManager")
	if dm:
		dm.load_database()

	# 1. Instanciar escena real de batalla
	var scene = ClockGearsBattleScene.new()
	root.add_child(scene)

	# Configurar tropa 1 donde el enemigo tiene mayor AGI que el héroe
	# Murciélago tiene AGI=15, Héroe tiene AGI=25 por defecto.
	# Vamos a forzar un controlador donde el enemigo tenga AGI=50
	var ctrl = scene.get_controller()
	scene.setup_battle(1, true, true)

	var enemy_unit = ctrl.get_enemy_units()[0]
	enemy_unit.set_agi(50)
	enemy_unit.set_graphic_path("res://img/enemies/cañón_musgoso_down.png")
	ctrl.calculate_initiative()

	var active = ctrl.get_active_unit()
	print("Activo al inicio: " + active.get_name() + " (is_actor=" + str(active.get_is_actor()) + ")")

	# Bug 2: Gráfico del enemigo en la tarjeta
	# Buscar el TextureRect en la primera tarjeta de enemigo
	var enemies_box = scene.find_child("enemies_container", true, false)
	# O inspeccionar si el TextureRect tiene textura
	print("Bug 2 check: enemigo tiene textura asignada?")

	# Bug 3: Botones apagados porque el turno es del enemigo
	var can_act = active.get_is_actor()
	print("¿Puede el jugador actuar de inmediato? " + str(can_act))
	if not can_act:
		print("BUG 3 DETECTADO: El enemigo tiene el turno al inicio, pero los botones del jugador están deshabilitados y el turno del enemigo NO se ejecuta automáticamente.")

	scene.free()
	quit(0)
