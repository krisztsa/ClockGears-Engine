extends SceneTree

var frames: int = 0
var map: ClockGearsMap

func _initialize() -> void:
	var dm = Engine.get_singleton("ClockGearsDataManager")
	dm.init_empty()

	var img := Image.create_empty(64, 64, false, Image.FORMAT_RGBA8)
	img.fill(Color(1.0, 0.7, 0.0, 1.0))
	img.save_png("res://img/tilesets/test_amber_tile.png")

	var ts_dict := {
		"id": 1,
		"name": "Amber TS",
		"mode": 0,
		"slots": { "A5": "res://img/tilesets/test_amber_tile.png" },
		"slot_collisions": {}
	}
	dm.set_category_array(13, [ts_dict])
	dm.save_all("res://clockgears_data")

	var vp := SubViewport.new()
	vp.size = Vector2i(256, 256)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	get_root().add_child(vp)

	map = ClockGearsMap.new()
	map.name = "Map001"
	vp.add_child(map)
	map.ensure_default_layers()
	map.setup_tileset_from_database(1)
	dm.set_editor_map(map)
	dm.set_editor_layer(1) # Capa_Suelo

	# Paint via real simulated mouse click flow at cell (2, 2)
	dm.set_editor_draw_tool(0) # Lapiz
	dm.simulate_editor_click_cell(2, 2, false)

	var layer = map.get_layer(0)
	print("Cell (2, 2) source_id: ", layer.get_cell_source_id(Vector2i(2, 2)))
	print("Cell (2, 2) atlas_coords: ", layer.get_cell_atlas_coords(Vector2i(2, 2)))
	print("Layer used cells: ", layer.get_used_cells())

func _process(delta: float) -> bool:
	frames += 1
	if frames >= 4:
		var vp = get_root().find_child("SubViewport", true, false) as SubViewport
		if vp:
			var tex = vp.get_texture()
			if tex:
				var img = tex.get_image()
				if img:
					var p_cell = img.get_pixel(80, 80)
					var p_bg = img.get_pixel(200, 200)
					print("Pixel at cell (2, 2) [80, 80]: ", p_cell)
					print("Pixel at background [200, 200]: ", p_bg)
					if p_cell != p_bg:
						print("RESULT: REAL EDITOR FLOW RENDERED TO PIXEL IMAGE SUCCESSFULLY!")
					else:
						print("RESULT: REAL EDITOR FLOW FAILED TO RENDER!")
		quit(0)
		return true
	return false
