
extends SceneTree

func _init():
	var db = ClockGearsDatabaseDialog.new()
	root.add_child(db)
	db.set_size(Vector2i(1000, 600))
	db.select_category(12)
	db.add_tileset_record('TestTS')
	db.import_tileset_slot('A5', 'res://img/tilesets/forest-tilemap.png')
	print('--- DEBUG SIZES & STATE ---')
	print('db size: ', db.get_size())
	quit(0)
