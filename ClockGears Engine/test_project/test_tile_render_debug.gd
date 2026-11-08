extends SceneTree

func _init():
	# Load map001.tscn from test_project/maps/map001.tscn
	var packed = ResourceLoader.load("res://maps/map001.tscn", "PackedScene", ResourceLoader.CACHE_MODE_IGNORE)
	print("map001.tscn loaded: ", packed != null)
	if packed:
		var map = packed.instantiate()
		print("map name: ", map.name, " class: ", map.get_class())
		for c in map.get_children():
			print("  child: ", c.name, " class: ", c.get_class())
			if c is TileMapLayer:
				print("    tileset: ", c.tile_set != null)
				print("    enabled: ", c.enabled)
				print("    visible: ", c.visible)
		map.free()
	quit(0)
