extends SceneTree

func _init():
	var root = Node2D.new()
	get_root().add_child(root)

	var map = ClockGearsMap.new()
	root.add_child(map)
	map.ensure_default_layers()

	var ground: TileMapLayer = map.get_layer(0)
	print("map is_inside_tree: ", map.is_inside_tree())
	print("ground is_inside_tree: ", ground.is_inside_tree())
	root.free()
	quit(0)
