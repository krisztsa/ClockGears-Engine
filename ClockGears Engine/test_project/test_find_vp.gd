extends SceneTree

var frames: int = 0

func _initialize() -> void:
	print(">>> INITIALIZE REAL RENDER TEST IN EDITOR <<<")

func _process(delta: float) -> bool:
	frames += 1
	var ed_node = Engine.get_singleton("EditorNode")
	if not ed_node:
		# In GDScript, EditorInterface or EditorNode can be accessed
		pass

	if frames == 2:
		var scene_root: SubViewport = null
		# Find the SubViewport from EditorNode
		var root_children = get_root().get_children()
		print("Root children count: ", root_children.size())
		for c in root_children:
			print("  root child: ", c.name, " class: ", c.get_class())

		var all_vp = get_root().find_children("*", "SubViewport", true, false)
		print("Found SubViewports: ", all_vp.size())
		for v in all_vp:
			print("  VP: ", v.get_path(), " size: ", v.size)

		quit(0)
		return true
	return false
