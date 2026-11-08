#include "clockgears_map.h"
#include "clockgears_debug_panel.h"
#include "clockgears_database.h"
#include "core/clockgears_data_manager.h"
#include "core/clockgears_pixel_art.h"
#include "core/clockgears_system_catalog.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "editor/clockgears_project_manager.h"
#include "runtime/clockgears_runtime.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/main/canvas_layer.h"
#include "modules/tilemap/tile_set.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/texture.h"
#include "servers/display/display_server.h"

void ClockGearsMap::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_grid_resolution", "resolution"), &ClockGearsMap::set_grid_resolution);
	ClassDB::bind_method(D_METHOD("get_grid_resolution"), &ClockGearsMap::get_grid_resolution);

	ClassDB::bind_method(D_METHOD("set_snap_to_grid", "snap"), &ClockGearsMap::set_snap_to_grid);
	ClassDB::bind_method(D_METHOD("is_snap_to_grid"), &ClockGearsMap::is_snap_to_grid);

	ClassDB::bind_method(D_METHOD("set_combat_mode", "mode"), &ClockGearsMap::set_combat_mode);
	ClassDB::bind_method(D_METHOD("get_combat_mode"), &ClockGearsMap::get_combat_mode);

	ClassDB::bind_method(D_METHOD("set_map_name", "name"), &ClockGearsMap::set_map_name);
	ClassDB::bind_method(D_METHOD("get_map_name"), &ClockGearsMap::get_map_name);

	ClassDB::bind_method(D_METHOD("set_map_size", "size"), &ClockGearsMap::set_map_size);
	ClassDB::bind_method(D_METHOD("get_map_size"), &ClockGearsMap::get_map_size);

	ClassDB::bind_method(D_METHOD("set_bgm_path", "path"), &ClockGearsMap::set_bgm_path);
	ClassDB::bind_method(D_METHOD("get_bgm_path"), &ClockGearsMap::get_bgm_path);

	ClassDB::bind_method(D_METHOD("set_region_map", "map"), &ClockGearsMap::set_region_map);
	ClassDB::bind_method(D_METHOD("get_region_map"), &ClockGearsMap::get_region_map);
	ClassDB::bind_method(D_METHOD("set_cell_region", "cell", "region_id"), &ClockGearsMap::set_cell_region);
	ClassDB::bind_method(D_METHOD("get_cell_region", "cell"), &ClockGearsMap::get_cell_region);

	ClassDB::bind_method(D_METHOD("set_shadow_map", "map"), &ClockGearsMap::set_shadow_map);
	ClassDB::bind_method(D_METHOD("get_shadow_map"), &ClockGearsMap::get_shadow_map);
	ClassDB::bind_method(D_METHOD("set_cell_shadow", "cell", "enabled"), &ClockGearsMap::set_cell_shadow);
	ClassDB::bind_method(D_METHOD("get_cell_shadow", "cell"), &ClockGearsMap::get_cell_shadow);

	ClassDB::bind_method(D_METHOD("import_image_to_map", "image_path", "layer_idx", "world_pos"), &ClockGearsMap::import_image_to_map);
	ClassDB::bind_method(D_METHOD("snap_position", "pos"), &ClockGearsMap::snap_position);
	ClassDB::bind_method(D_METHOD("add_clockgears_event", "event", "pos"), &ClockGearsMap::add_clockgears_event);
	ClassDB::bind_method(D_METHOD("import_tileset_png", "path"), &ClockGearsMap::import_tileset_png);
	ClassDB::bind_method(D_METHOD("set_tile_at", "layer_idx", "cell", "atlas_coords"), &ClockGearsMap::set_tile_at);
	ClassDB::bind_method(D_METHOD("get_tile_at", "layer_idx", "cell"), &ClockGearsMap::get_tile_at);
	ClassDB::bind_method(D_METHOD("set_player_start_position", "cell", "dir"), &ClockGearsMap::set_player_start_position, DEFVAL(2));
	ClassDB::bind_method(D_METHOD("ensure_default_layers"), &ClockGearsMap::ensure_default_layers);
	ClassDB::bind_method(D_METHOD("create_default_layers"), &ClockGearsMap::create_default_layers);
	ClassDB::bind_method(D_METHOD("init_runtime_session"), &ClockGearsMap::init_runtime_session);
	ClassDB::bind_method(D_METHOD("get_runtime_player"), &ClockGearsMap::get_runtime_player);
	ClassDB::bind_method(D_METHOD("get_runtime_followers"), &ClockGearsMap::get_runtime_followers);
	ClassDB::bind_method(D_METHOD("get_runtime_camera"), &ClockGearsMap::get_runtime_camera);
	ClassDB::bind_method(D_METHOD("set_runtime_camera", "camera"), &ClockGearsMap::set_runtime_camera);
	ClassDB::bind_method(D_METHOD("get_runtime_interpreter"), &ClockGearsMap::get_runtime_interpreter);
	ClassDB::bind_method(D_METHOD("set_tileset_id", "tileset_id"), &ClockGearsMap::set_tileset_id);
	ClassDB::bind_method(D_METHOD("get_tileset_id"), &ClockGearsMap::get_tileset_id);
	ClassDB::bind_method(D_METHOD("setup_tileset_from_database", "tileset_id"), &ClockGearsMap::setup_tileset_from_database);
	ClassDB::bind_method(D_METHOD("get_layer", "layer_idx"), &ClockGearsMap::get_layer);
	ClassDB::bind_method(D_METHOD("is_tile_passable", "cell", "dir"), &ClockGearsMap::is_tile_passable, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_cell_collision_type", "cell"), &ClockGearsMap::get_cell_collision_type);
	ClassDB::bind_method(D_METHOD("get_runtime_debug_overlay"), &ClockGearsMap::get_runtime_debug_overlay);
	ClassDB::bind_method(D_METHOD("set_runtime_debug_overlay", "overlay"), &ClockGearsMap::set_runtime_debug_overlay);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_resolution", PROPERTY_HINT_ENUM, "16x16:16,32x32:32,48x48:48"), "set_grid_resolution", "get_grid_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "snap_to_grid"), "set_snap_to_grid", "is_snap_to_grid");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "combat_mode", PROPERTY_HINT_ENUM, "Por Turnos:0,Action RPG (Tiempo Real):1"), "set_combat_mode", "get_combat_mode");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "map_name"), "set_map_name", "get_map_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tileset_id"), "set_tileset_id", "get_tileset_id");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "map_size"), "set_map_size", "get_map_size");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "bgm_path"), "set_bgm_path", "get_bgm_path");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "region_map"), "set_region_map", "get_region_map");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "shadow_map"), "set_shadow_map", "get_shadow_map");
}

void ClockGearsMap::set_grid_resolution(int p_res) {
	if (p_res != 16 && p_res != 32 && p_res != 48) {
		p_res = 32;
	}
	grid_resolution = p_res;
}

void ClockGearsMap::set_map_size(Vector2i p_size) {
	map_width = MAX(1, p_size.x);
	map_height = MAX(1, p_size.y);
}

void ClockGearsMap::_bind_layer_pointers() {
	if (!layer_ground) {
		Node *n = get_node_or_null(NodePath("Capa_Suelo"));
		if (n) {
			layer_ground = Object::cast_to<TileMapLayer>(n);
		} else {
			layer_ground = Object::cast_to<TileMapLayer>(find_child("Capa_Suelo", false, false));
		}
	}

	if (!layer_decoration) {
		Node *n = get_node_or_null(NodePath("Capa_Decoracion"));
		if (n) {
			layer_decoration = Object::cast_to<TileMapLayer>(n);
		} else {
			layer_decoration = Object::cast_to<TileMapLayer>(find_child("Capa_Decoracion", false, false));
		}
	}

	if (!layer_objects) {
		Node *n = get_node_or_null(NodePath("Capa_Objetos"));
		if (n) {
			layer_objects = Object::cast_to<TileMapLayer>(n);
		} else {
			layer_objects = Object::cast_to<TileMapLayer>(find_child("Capa_Objetos", false, false));
		}
	}

	if (!events_container) {
		Node *n = get_node_or_null(NodePath("Eventos"));
		if (n) {
			events_container = Object::cast_to<Node2D>(n);
		} else {
			events_container = Object::cast_to<Node2D>(find_child("Eventos", false, false));
		}
	}

	if (!layer_overhead) {
		Node *n = get_node_or_null(NodePath("Capa_Techo"));
		if (n) {
			layer_overhead = Object::cast_to<TileMapLayer>(n);
		} else {
			layer_overhead = Object::cast_to<TileMapLayer>(find_child("Capa_Techo", false, false));
		}
	}
}

void ClockGearsMap::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_READY: {
			set_position(Vector2(0, 0));
			set_meta("_edit_lock_", true);
			set_meta("_edit_group_", true);
			ensure_default_layers();
			_bind_layer_pointers();
			if (tileset_id > 0) {
				setup_tileset_from_database(tileset_id);
			}
			if (p_what == NOTIFICATION_READY && !Engine::get_singleton()->is_editor_hint()) {
				init_runtime_session();
			}
		} break;
	}
}

void ClockGearsMap::ensure_default_layers() {
	set_position(Vector2(0, 0));
	set_meta("_edit_lock_", true);
	set_meta("_edit_group_", true);
	_bind_layer_pointers();

	// 1. Capa_Suelo
	if (!layer_ground) {
		for (int i = 0; i < get_child_count(); i++) {
			if (get_child(i)->get_name() == "Capa_Suelo") {
				layer_ground = Object::cast_to<TileMapLayer>(get_child(i));
				break;
			}
		}
	}
	if (!layer_ground) {
		layer_ground = memnew(TileMapLayer);
		layer_ground->set_name("Capa_Suelo");
		layer_ground->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		add_child(layer_ground);
		layer_ground->set_owner(this);
	}
	if (layer_ground) {
		layer_ground->set_position(Vector2(0, 0));
		layer_ground->set_meta("_edit_lock_", true);
		layer_ground->set_meta("_edit_group_", true);
	}

	// 2. Capa_Decoracion
	if (!layer_decoration) {
		for (int i = 0; i < get_child_count(); i++) {
			if (get_child(i)->get_name() == "Capa_Decoracion") {
				layer_decoration = Object::cast_to<TileMapLayer>(get_child(i));
				break;
			}
		}
	}
	if (!layer_decoration) {
		layer_decoration = memnew(TileMapLayer);
		layer_decoration->set_name("Capa_Decoracion");
		layer_decoration->set_z_index(1);
		layer_decoration->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		add_child(layer_decoration);
		layer_decoration->set_owner(this);
	}
	if (layer_decoration) {
		layer_decoration->set_position(Vector2(0, 0));
		layer_decoration->set_meta("_edit_lock_", true);
		layer_decoration->set_meta("_edit_group_", true);
	}

	// 3. Capa_Objetos
	if (!layer_objects) {
		for (int i = 0; i < get_child_count(); i++) {
			if (get_child(i)->get_name() == "Capa_Objetos") {
				layer_objects = Object::cast_to<TileMapLayer>(get_child(i));
				break;
			}
		}
	}
	if (!layer_objects) {
		layer_objects = memnew(TileMapLayer);
		layer_objects->set_name("Capa_Objetos");
		layer_objects->set_z_index(2);
		layer_objects->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		add_child(layer_objects);
		layer_objects->set_owner(this);
	}
	if (layer_objects) {
		layer_objects->set_position(Vector2(0, 0));
		layer_objects->set_meta("_edit_lock_", true);
		layer_objects->set_meta("_edit_group_", true);
	}

	// 4. Eventos
	if (!events_container) {
		for (int i = 0; i < get_child_count(); i++) {
			if (get_child(i)->get_name() == "Eventos") {
				events_container = Object::cast_to<Node2D>(get_child(i));
				break;
			}
		}
	}
	if (!events_container) {
		events_container = memnew(Node2D);
		events_container->set_name("Eventos");
		events_container->set_z_index(3);
		events_container->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		add_child(events_container);
		events_container->set_owner(this);
	}
	if (events_container) {
		events_container->set_position(Vector2(0, 0));
		events_container->set_meta("_edit_lock_", true);
		events_container->set_meta("_edit_group_", true);
	}

	// 5. Capa_Techo
	if (!layer_overhead) {
		for (int i = 0; i < get_child_count(); i++) {
			if (get_child(i)->get_name() == "Capa_Techo") {
				layer_overhead = Object::cast_to<TileMapLayer>(get_child(i));
				break;
			}
		}
	}
	if (!layer_overhead) {
		layer_overhead = memnew(TileMapLayer);
		layer_overhead->set_name("Capa_Techo");
		layer_overhead->set_z_index(4);
		layer_overhead->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		add_child(layer_overhead);
		layer_overhead->set_owner(this);
	}
	if (layer_overhead) {
		layer_overhead->set_position(Vector2(0, 0));
		layer_overhead->set_meta("_edit_lock_", true);
		layer_overhead->set_meta("_edit_group_", true);
	}
}

bool ClockGearsMap::is_tile_passable(const Vector2i &p_cell, int p_dir) const {
	return get_cell_collision_type(p_cell) != 1;
}

int ClockGearsMap::get_cell_collision_type(const Vector2i &p_cell) const {
	if (p_cell.x < 0 || p_cell.y < 0 || p_cell.x >= map_width || p_cell.y >= map_height) {
		return 1; // 1 = Bloqueado (fuera de límites)
	}

	Dictionary slot_cols;
	Dictionary legacy_cols;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Array ts_arr = dm->get_category_array(CAT_TILESETS);
		for (int i = 0; i < ts_arr.size(); i++) {
			if (ts_arr[i].get_type() == Variant::DICTIONARY) {
				Dictionary d = ts_arr[i];
				int id = int(d.get("id", i + 1));
				if ((tileset_id > 0 && id == tileset_id) || (tileset_id <= 0 && i == 0)) {
					slot_cols = d.get("slot_collisions", Dictionary());
					legacy_cols = d.get("collisions", Dictionary());
					break;
				}
			}
		}
	}

	const TileMapLayer *layers[4] = { layer_overhead, layer_objects, layer_decoration, layer_ground };
	for (int i = 0; i < 4; i++) {
		const TileMapLayer *tlayer = layers[i];
		if (!tlayer || tlayer->get_cell_source_id(p_cell) == -1) {
			continue;
		}
		const TileData *td = tlayer->get_cell_tile_data(p_cell);
		if (td && td->get_z_index() > 3) {
			return 2; // Tile elevado sobre el jugador (★)
		}
		Vector2i atlas_coords = tlayer->get_cell_atlas_coords(p_cell);
		String key = vformat("%d,%d", atlas_coords.x, atlas_coords.y);

		Dictionary md;
		bool found_meta = false;
		static const char *skeys[9] = { "A5", "B", "C", "D", "E", "A1", "A2", "A3", "A4" };
		for (int s = 0; s < 9; s++) {
			if (slot_cols.has(skeys[s])) {
				Dictionary smap = slot_cols[skeys[s]];
				if (smap.has(key)) {
					md = smap[key];
					found_meta = true;
					break;
				}
			}
		}
		if (!found_meta && legacy_cols.has(key)) {
			Variant lm = legacy_cols[key];
			if (lm.get_type() == Variant::DICTIONARY) {
				md = lm;
				found_meta = true;
			} else if (int(lm) == 2) {
				return 2; // Modo legacy 2 = ★
			}
		}

		if (found_meta) {
			int passage = int(md.get("passage", 15));
			int layer = int(md.get("layer", 0));
			if (layer == 1 || int(md.get("mode", 0)) == 2) {
				return 2; // Estrella (★): pasable por detrás
			}
			if (passage == 0 || int(md.get("mode", 0)) == 1) {
				return 1; // Bloqueado (X)
			}
		}
	}

	return 0; // 0 = Pasable (sin marca)
}

TileMapLayer *ClockGearsMap::get_layer(int p_layer_idx) {
	ensure_default_layers();
	switch (p_layer_idx) {
		case LAYER_GROUND: // 0
			return layer_ground;
		case LAYER_DECORATION: // 1
			return layer_decoration;
		case LAYER_OBJECTS: // 2
			return layer_objects;
		case LAYER_OVERHEAD: // 3
		case 4: // Soporte para llamada 1-based (Capa 4: Techo)
			return layer_overhead;
		default:
			return layer_ground;
	}
}

Node2D *ClockGearsMap::get_events_container() {
	ensure_default_layers();
	return events_container;
}

Vector2 ClockGearsMap::snap_position(Vector2 p_pos) const {
	if (!snap_to_grid) {
		return p_pos; // Free transform / "Salir de la casilla"
	}
	float sz = (float)grid_resolution;
	float sx = Math::floor(p_pos.x / sz) * sz;
	float sy = Math::floor(p_pos.y / sz) * sz;
	return Vector2(sx, sy);
}

void ClockGearsMap::set_cell_region(const Vector2i &p_cell, int p_region_id) {
	String key = vformat("%d,%d", p_cell.x, p_cell.y);
	int clamped = CLAMP(p_region_id, 0, 255);
	if (clamped == 0) {
		region_map.erase(key);
	} else {
		region_map[key] = clamped;
	}
}

int ClockGearsMap::get_cell_region(const Vector2i &p_cell) const {
	String key = vformat("%d,%d", p_cell.x, p_cell.y);
	return region_map.has(key) ? (int)region_map[key] : 0;
}

void ClockGearsMap::set_cell_shadow(const Vector2i &p_cell, bool p_enabled) {
	String key = vformat("%d,%d", p_cell.x, p_cell.y);
	if (!p_enabled) {
		shadow_map.erase(key);
	} else {
		shadow_map[key] = true;
	}
}

bool ClockGearsMap::get_cell_shadow(const Vector2i &p_cell) const {
	String key = vformat("%d,%d", p_cell.x, p_cell.y);
	return shadow_map.has(key) ? (bool)shadow_map[key] : false;
}

Node2D *ClockGearsMap::import_image_to_map(const String &p_image_path, int p_layer_idx, Vector2 p_world_pos) {
	Ref<Texture2D> tex = ResourceLoader::load(p_image_path);
	ERR_FAIL_COND_V(tex.is_null(), nullptr);

	Sprite2D *sprite = memnew(Sprite2D);
	sprite->set_texture(tex);
	sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST); // A4.1: Pixel art nítido
	sprite->set_position(snap_position(p_world_pos));
	sprite->set_centered(false);

	TileMapLayer *target_layer = get_layer(p_layer_idx);
	if (target_layer) {
		target_layer->add_child(sprite);
	} else {
		add_child(sprite);
	}
	return sprite;
}

void ClockGearsMap::add_clockgears_event(ClockGearsEvent *p_event, Vector2 p_pos) {
	ERR_FAIL_NULL(p_event);
	ensure_default_layers();
	p_event->set_position(snap_position(p_pos));
	if (events_container) {
		events_container->add_child(p_event);
	} else {
		add_child(p_event);
	}
}

void ClockGearsMap::setup_tileset_from_database(int p_tileset_id) {
	set_tileset_id(p_tileset_id);
	ensure_default_layers();

	TileMapLayer *layers[4] = { layer_ground, layer_decoration, layer_objects, layer_overhead };

	if (p_tileset_id <= 0) {
		for (int i = 0; i < 4; i++) {
			if (layers[i]) {
				layers[i]->set_tile_set(Ref<TileSet>());
				layers[i]->update_internals();
				layers[i]->queue_redraw();
			}
		}
		return;
	}

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	if (dm->get_category_array(CAT_TILESETS).is_empty() && FileAccess::exists("res://clockgears_data/tilesets.json")) {
		dm->load_all("res://clockgears_data");
	}

	Array ts_arr = dm->get_category_array(CAT_TILESETS);
	Dictionary ts_dict;
	for (int i = 0; i < ts_arr.size(); i++) {
		if (ts_arr[i].get_type() == Variant::DICTIONARY) {
			Dictionary d = ts_arr[i];
			if (int(d.get("id", i + 1)) == p_tileset_id) {
				ts_dict = d;
				break;
			}
		}
	}

	if (ts_dict.is_empty()) {
		for (int i = 0; i < 4; i++) {
			if (layers[i]) {
				layers[i]->set_tile_set(Ref<TileSet>());
				layers[i]->update_internals();
				layers[i]->queue_redraw();
			}
		}
		return;
	}

	Dictionary slots = ts_dict.get("slots", Dictionary());
	Dictionary slot_cols = ts_dict.get("slot_collisions", Dictionary());
	Dictionary legacy_cols = ts_dict.get("collisions", Dictionary());
	String legacy_graphic = ts_dict.get("graphic_path", "");

	Ref<TileSet> ts;
	ts.instantiate();
	ts->set_tile_size(Size2i(grid_resolution, grid_resolution));
	int star_count = 0;

	// Slot A (tab 0: A5, A2, A1, A3, A4, or legacy), B (tab 1), C (tab 2), D (tab 3), E (tab 4)
	for (int tab = 0; tab < 5; tab++) {
		String img_path;
		if (tab == 0) {
			static const char *a_keys[5] = { "A5", "A2", "A1", "A3", "A4" };
			for (int k = 0; k < 5; k++) {
				String cand = slots.get(a_keys[k], "");
				if (!cand.is_empty() && FileAccess::exists(cand)) {
					img_path = cand;
					break;
				}
			}
			if (img_path.is_empty() && !legacy_graphic.is_empty() && FileAccess::exists(legacy_graphic)) {
				img_path = legacy_graphic;
			}
		} else {
			static const char *bcde_keys[4] = { "B", "C", "D", "E" };
			String cand = slots.get(bcde_keys[tab - 1], "");
			if (!cand.is_empty() && FileAccess::exists(cand)) {
				img_path = cand;
			}
		}

		if (img_path.is_empty() || !FileAccess::exists(img_path)) {
			continue;
		}

		Ref<Texture2D> tex = ResourceLoader::load(img_path);
		if (tex.is_null()) {
			Ref<Image> img;
			img.instantiate();
			if (ImageLoader::load_image(img_path, img) == OK) {
				Ref<ImageTexture> itex = ImageTexture::create_from_image(img);
				itex->set_path(img_path, true);
				tex = itex;
			}
		}

		if (tex.is_valid()) {
			Ref<TileSetAtlasSource> atlas;
			atlas.instantiate();
			atlas->set_texture(tex);
			atlas->set_texture_region_size(Size2i(grid_resolution, grid_resolution));
			atlas->set_use_texture_padding(true);
			int cols = MAX(1, tex->get_width() / grid_resolution);
			int rows = MAX(1, tex->get_height() / grid_resolution);
			for (int ry = 0; ry < rows; ry++) {
				for (int rx = 0; rx < cols; rx++) {
					Vector2i coords(rx, ry);
					atlas->create_tile(coords);
					TileData *td = atlas->get_tile_data(coords, 0);
					if (td) {
						String key = vformat("%d,%d", rx, ry);
						bool is_star = false;
						if (tab == 0) {
							static const char *a_keys[5] = { "A5", "A2", "A1", "A3", "A4" };
							for (int k = 0; k < 5; k++) {
								if (slot_cols.has(a_keys[k])) {
									Dictionary smap = slot_cols[a_keys[k]];
									if (smap.has(key)) {
										Dictionary md = smap[key];
										if (int(md.get("layer", 0)) == 1 || int(md.get("mode", 0)) == 2) {
											is_star = true;
										}
										break;
									}
								}
							}
						} else {
							static const char *bcde_keys[4] = { "B", "C", "D", "E" };
							String sk = bcde_keys[tab - 1];
							if (slot_cols.has(sk)) {
								Dictionary smap = slot_cols[sk];
								if (smap.has(key)) {
									Dictionary md = smap[key];
									if (int(md.get("layer", 0)) == 1 || int(md.get("mode", 0)) == 2) {
										is_star = true;
									}
								}
							}
						}
						if (!is_star && legacy_cols.has(key)) {
							Variant lm = legacy_cols[key];
							if (lm.get_type() == Variant::DICTIONARY) {
								Dictionary md = lm;
								if (int(md.get("layer", 0)) == 1 || int(md.get("mode", 0)) == 2) {
									is_star = true;
								}
							} else if (int(lm) == 2) {
								is_star = true;
							}
						}
						td->set_z_index(is_star ? 5 : 0);
						if (is_star) {
							star_count++;
						}
					}
				}
			}
			ts->add_source(atlas, tab);
		}
	}

	for (int i = 0; i < 4; i++) {
		if (layers[i]) {
			layers[i]->set_tile_set(ts);
			layers[i]->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
			layers[i]->update_internals();
			layers[i]->queue_redraw();
		}
	}

	print_line(vformat(String::utf8("[ClockGears StarTile] Tileset #%03d configurado: %d tiles con ★ (z_index=5 sobre jugador z=3, pasabilidad=true)."), p_tileset_id, star_count));
}

bool ClockGearsMap::import_tileset_png(const String &p_path) {
	if (p_path.strip_edges().is_empty()) {
		print_line(String::utf8("[ClockGears Tileset] ERROR: Ruta de archivo vacía."));
		return false;
	}
	if (!FileAccess::exists(p_path)) {
		print_line(vformat(String::utf8("[ClockGears Tileset] ERROR: No existe el archivo '%s'."), p_path));
		return false;
	}

	Ref<Image> image;
	image.instantiate();
	String global_src = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(p_path) : p_path;
	Error err = ImageLoader::load_image(global_src, image);
	if (err != OK || image.is_null() || image->is_empty()) {
		err = image->load(p_path);
	}
	if (err != OK || image.is_null() || image->is_empty()) {
		print_line(vformat(String::utf8("[ClockGears Tileset] ERROR: El archivo '%s' no es una imagen PNG válida o está corrupto."), p_path));
		return false;
	}

	int tile_sz = (grid_resolution > 0) ? grid_resolution : 32;
	if (image->get_width() < tile_sz || image->get_height() < tile_sz) {
		print_line(vformat(String::utf8("[ClockGears Tileset] ERROR: El tileset '%s' (%dx%d px) es menor que el tamaño mínimo de tile (%dx%d px)."),
				p_path.get_file(), image->get_width(), image->get_height(), tile_sz, tile_sz));
		return false;
	}

	String file_name = p_path.get_file();
	if (!file_name.to_lower().ends_with(".png")) {
		file_name = file_name.get_basename() + ".png";
	}
	String res_dir = "res://img/tilesets";
	String global_dir = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(res_dir) : res_dir;
	DirAccess::make_dir_recursive_absolute(global_dir);

	String res_path = res_dir.path_join(file_name);
	String global_dest = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(res_path) : res_path;

	if (global_src.simplify_path() != global_dest.simplify_path()) {
		image->save_png(global_dest);
	}
	ClockGearsProjectStructure::write_pixel_art_import_preset(res_path);

	Ref<ImageTexture> tex = ImageTexture::create_from_image(image);
	if (tex.is_null()) {
		print_line(vformat(String::utf8("[ClockGears Tileset] ERROR: No se pudo crear ImageTexture para '%s'."), res_path));
		return false;
	}
	tex->set_path(res_path, true);

	ensure_default_layers();

	Ref<TileSet> ts = layer_ground ? layer_ground->get_tile_set() : Ref<TileSet>();
	if (ts.is_null()) {
		ts.instantiate();
	}
	ts->set_tile_size(Size2i(tile_sz, tile_sz));

	Ref<TileSetAtlasSource> atlas;
	if (ts->has_source(0)) {
		atlas = ts->get_source(0);
	}
	if (atlas.is_null()) {
		atlas.instantiate();
		ts->add_source(atlas, 0);
	}
	atlas->set_texture(tex);
	atlas->set_texture_region_size(Size2i(tile_sz, tile_sz));
	atlas->set_use_texture_padding(true);

	int cols = MAX(1, image->get_width() / tile_sz);
	int rows = MAX(1, image->get_height() / tile_sz);
	for (int ry = 0; ry < rows; ry++) {
		for (int rx = 0; rx < cols; rx++) {
			Vector2i coords(rx, ry);
			if (!atlas->has_tile(coords)) {
				atlas->create_tile(coords);
			}
		}
	}

	TileMapLayer *layers[4] = { layer_ground, layer_decoration, layer_objects, layer_overhead };
	for (int i = 0; i < 4; i++) {
		if (layers[i]) {
			layers[i]->set_tile_set(ts);
			layers[i]->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		}
	}

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Array ts_arr = dm->get_category_array(CAT_TILESETS);
		bool found = false;
		for (int i = 0; i < ts_arr.size(); i++) {
			Dictionary d = ts_arr[i];
			if (String(d.get("graphic_path", "")) == res_path) {
				found = true;
				break;
			}
		}
		if (!found) {
			Dictionary d;
			d["id"] = ts_arr.size() + 1;
			d["name"] = file_name.get_basename();
			d["graphic_path"] = res_path;
			d["tile_size"] = tile_sz;
			d["columns"] = cols;
			d["rows"] = rows;
			ts_arr.push_back(d);
			dm->set_category_array(CAT_TILESETS, ts_arr);
			dm->save_all("res://clockgears_data");
		}
	}

	print_line(vformat(String::utf8("[ClockGears Tileset] OK: Importado '%s' (%dx%d px -> %dx%d = %d tiles de %dx%d)."),
			res_path, image->get_width(), image->get_height(), cols, rows, cols * rows, tile_sz, tile_sz));
	return true;
}

void ClockGearsMap::set_tile_at(int p_layer_idx, const Vector2i &p_cell, const Vector2i &p_atlas_coords) {
	int effective_layer = p_layer_idx;
	if (effective_layer != LAYER_OVERHEAD && p_atlas_coords.x >= 0 && p_atlas_coords.y >= 0) {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (dm) {
			Array ts_arr = dm->get_category_array(CAT_TILESETS);
			for (int i = 0; i < ts_arr.size(); i++) {
				if (ts_arr[i].get_type() == Variant::DICTIONARY) {
					Dictionary d = ts_arr[i];
					int id = int(d.get("id", i + 1));
					if (tileset_id > 0 && id == tileset_id) {
						Dictionary slot_cols = d.get("slot_collisions", Dictionary());
						String key = vformat("%d,%d", p_atlas_coords.x, p_atlas_coords.y);
						static const char *skeys[9] = { "A5", "B", "C", "D", "E", "A1", "A2", "A3", "A4" };
						for (int s = 0; s < 9; s++) {
							if (slot_cols.has(skeys[s])) {
								Dictionary smap = slot_cols[skeys[s]];
								if (smap.has(key) && int(Dictionary(smap[key]).get("layer", 0)) == 1) {
									effective_layer = LAYER_OVERHEAD;
									break;
								}
							}
						}
						break;
					}
				}
			}
		}
	}

	TileMapLayer *layer = get_layer(effective_layer);
	if (!layer) {
		return;
	}
	if (p_atlas_coords.x < 0 || p_atlas_coords.y < 0) {
		layer->erase_cell(p_cell);
		if (layer_overhead && layer_overhead != layer) {
			layer_overhead->erase_cell(p_cell);
		}
		return;
	}
	Ref<TileSet> ts = layer->get_tile_set();
	if (ts.is_null() && layer_ground && layer_ground->get_tile_set().is_valid()) {
		ts = layer_ground->get_tile_set();
		layer->set_tile_set(ts);
	}
	if (ts.is_valid() && ts->has_source(0)) {
		Ref<TileSetAtlasSource> atlas = ts->get_source(0);
		if (atlas.is_valid() && !atlas->has_tile(p_atlas_coords)) {
			atlas->create_tile(p_atlas_coords);
		}
	}
	layer->set_cell(p_cell, 0, p_atlas_coords, 0);
}

Vector2i ClockGearsMap::get_tile_at(int p_layer_idx, const Vector2i &p_cell) {
	TileMapLayer *layer = get_layer(p_layer_idx);
	if (!layer) {
		return Vector2i(-1, -1);
	}
	return layer->get_cell_atlas_coords(p_cell);
}

void ClockGearsMap::set_player_start_position(const Vector2i &p_cell, int p_dir) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary sys = dm->get_system_config();
	String map_path = get_scene_file_path();
	if (map_path.is_empty()) {
		map_path = "res://maps/map001.tscn";
	}
	int clamped_x = CLAMP(p_cell.x, 0, MAX(0, map_width - 1));
	int clamped_y = CLAMP(p_cell.y, 0, MAX(0, map_height - 1));
	int valid_dir = (p_dir == 2 || p_dir == 4 || p_dir == 6 || p_dir == 8) ? p_dir : 2;

	sys["has_start_pos"] = true;
	sys["start_map"] = map_path;
	sys["start_x"] = clamped_x;
	sys["start_y"] = clamped_y;
	sys["start_dir"] = valid_dir;
	dm->set_system_config(sys);
	dm->save_all("res://clockgears_data");

	ClockGearsDatabaseDialog *db = ClockGearsDatabaseDialog::get_singleton();
	if (db) {
		db->sync_system_player_start(map_path, clamped_x, clamped_y);
	}

	print_line(vformat(String::utf8("[ClockGears StartPos] Posición inicial del jugador fijada: Mapa='%s', X=%d, Y=%d, Dir=%d (único punto de inicio)."),
			map_path, clamped_x, clamped_y, valid_dir));
}

void ClockGearsMap::init_runtime_session() {
	if (runtime_player) {
		return;
	}
	ensure_default_layers();

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}

	if (dm->get_category_array(CAT_ACTORS).is_empty() && FileAccess::exists("res://clockgears_data/actors.json")) {
		dm->load_all("res://clockgears_data");
	}

	Array actors = dm->get_category_array(CAT_ACTORS);
	Dictionary sys = dm->get_system_config();
	bool has_start_pos = (bool)sys.get("has_start_pos", true);
	if (!sys.has("has_start_pos") && sys.has("start_map") && !String(sys["start_map"]).is_empty()) {
		has_start_pos = true;
	}

	print_line(vformat(String::utf8("[ClockGears Runtime] Comprobando condiciones en '%s': Actores=%d, PosiciónInicial=%s (Mapa='%s', X=%d, Y=%d, Dir=%d)"),
			get_map_name(), actors.size(), has_start_pos ? "OK" : "FALTA",
			String(sys.get("start_map", "")), (int)sys.get("start_x", 0), (int)sys.get("start_y", 0), (int)sys.get("start_dir", 2)));

	if (actors.is_empty() || !has_start_pos) {
		// Regla Cero: si no hay ningún actor o posición de inicio, mostrar aviso claro y no inventar ninguno.
		CanvasLayer *warn_layer = memnew(CanvasLayer);
		warn_layer->set_layer(120);
		add_child(warn_layer);

		PanelContainer *warn_panel = memnew(PanelContainer);
		warn_panel->set_anchors_and_offsets_preset(Control::PRESET_CENTER);
		warn_panel->set_custom_minimum_size(Size2(540, 160));
		Ref<StyleBoxFlat> sb;
		sb.instantiate();
		sb->set_bg_color(Color(0.10f, 0.10f, 0.13f, 0.96f));
		sb->set_border_width_all(2);
		sb->set_border_color(Color(0.88f, 0.63f, 0.25f));
		sb->set_corner_radius_all(6);
		sb->set_content_margin_all(18);
		warn_panel->add_theme_style_override("panel", sb);
		warn_layer->add_child(warn_panel);

		Label *warn_lbl = memnew(Label);
		warn_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		warn_lbl->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		String missing_str = String::utf8("AVISO DE REGLA CERO (ClockGears Engine)\n\n");
		if (actors.is_empty()) {
			missing_str += String::utf8("• Falta crear al menos un Actor en la Base de Datos (F9 > Actores).\n");
		}
		if (!has_start_pos) {
			missing_str += String::utf8("• Falta colocar la Posición Inicial del Jugador en el mapa.\n");
		}
		warn_lbl->set_text(missing_str);
		warn_panel->add_child(warn_lbl);
		return;
	}

	Array party = sys.get("party", Array());
	if (party.is_empty()) {
		Dictionary first_actor = actors[0];
		party.push_back((int)first_actor.get("id", 1));
	}

	int leader_id = (int)party[0];

	Vector2i start_cell(
			(int)sys.get("start_x", 0),
			(int)sys.get("start_y", 0));
	int start_dir = (int)sys.get("start_dir", 2);
	if (start_dir != 2 && start_dir != 4 && start_dir != 6 && start_dir != 8) {
		start_dir = 2;
	}

	if ((bool)sys.get("runtime_has_transfer", false)) {
		start_cell = Vector2i(
				(int)sys.get("runtime_transfer_x", start_cell.x),
				(int)sys.get("runtime_transfer_y", start_cell.y));
		start_dir = (int)sys.get("runtime_transfer_dir", start_dir);
		sys["runtime_has_transfer"] = false;
		dm->set_system_config(sys);
	}

	start_cell.x = CLAMP(start_cell.x, 0, MAX(0, map_width - 1));
	start_cell.y = CLAMP(start_cell.y, 0, MAX(0, map_height - 1));

	// Evaluar páginas iniciales de todos los eventos del mapa
	if (events_container) {
		for (int i = 0; i < events_container->get_child_count(); i++) {
			ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(events_container->get_child(i));
			if (ev) {
				ev->update_active_page();
			}
		}
	}

	// 1. Seguidores (debajo del líder)
	runtime_followers = memnew(ClockGearsFollowerManager);
	runtime_followers->set_name("RuntimeFollowers");
	runtime_followers->set_z_index(3);
	add_child(runtime_followers);
	runtime_followers->setup_party_followers(party, start_cell, start_dir, grid_resolution);

	// 2. Jugador en rejilla 32x32 con pasabilidad
	runtime_player = memnew(ClockGearsPlayer);
	runtime_player->set_name("RuntimePlayer");
	runtime_player->set_z_index(3);
	add_child(runtime_player);
	runtime_player->load_actor_data(leader_id);
	runtime_player->setup_on_map(this, runtime_followers, nullptr);
	runtime_player->set_grid_cell_immediate(start_cell, start_dir);

	// 3. Cámara Pixel-Perfect (Anexo A4.1)
	runtime_camera = memnew(ClockGearsPixelCamera2D);
	runtime_camera->set_name("RuntimePixelCamera");
	add_child(runtime_camera);

	float cam_zoom = 2.0f;
	bool cam_follow = true;
	bool cam_smooth = true;
	float cam_smooth_speed = 8.0f;
	bool cam_clamp = true;
	bool cam_integer = false;

	if (ClockGearsDataManager::get_singleton()) {
		sys = ClockGearsDataManager::get_singleton()->get_system_config();
		cam_zoom = float(sys.get("camera_zoom", 2.0f));
		if (cam_zoom <= 0.05f) {
			cam_zoom = 2.0f;
		}
		cam_follow = bool(sys.get("camera_follow_player", true));
		cam_smooth = bool(sys.get("camera_smooth", true));
		cam_smooth_speed = float(sys.get("camera_smooth_speed", 8.0f));
		cam_clamp = bool(sys.get("camera_clamp", true));
		cam_integer = bool(sys.get("camera_integer_zoom", false));
	}

	if (cam_follow) {
		runtime_camera->set_follow_target_path(runtime_camera->get_path_to(runtime_player));
	} else {
		runtime_camera->set_follow_target_path(NodePath());
	}

	runtime_camera->set_allow_fractional_zoom(!cam_integer);
	if (cam_integer) {
		runtime_camera->set_integer_zoom(MAX(1, int(Math::round(cam_zoom))));
	} else {
		runtime_camera->set_zoom(Vector2(cam_zoom, cam_zoom));
	}
	runtime_camera->set_smooth_follow_enabled(cam_smooth);
	runtime_camera->set_follow_speed(cam_smooth_speed);
	runtime_camera->set_logical_position(runtime_player->get_position() + Vector2(grid_resolution * 0.5f, grid_resolution * 0.5f));

	if (cam_clamp) {
		runtime_camera->set_limit(SIDE_LEFT, 0);
		runtime_camera->set_limit(SIDE_TOP, 0);
		runtime_camera->set_limit(SIDE_RIGHT, map_width * grid_resolution);
		runtime_camera->set_limit(SIDE_BOTTOM, map_height * grid_resolution);
		runtime_camera->set_limit_smoothing_enabled(cam_smooth);
	} else {
		runtime_camera->set_limit(SIDE_LEFT, -10000000);
		runtime_camera->set_limit(SIDE_TOP, -10000000);
		runtime_camera->set_limit(SIDE_RIGHT, 10000000);
		runtime_camera->set_limit(SIDE_BOTTOM, 10000000);
	}

	if (runtime_camera->is_inside_tree()) {
		runtime_camera->make_current();
	}

	// 4. Intérprete de Eventos en Tiempo de Ejecución
	runtime_interpreter = memnew(ClockGearsEventInterpreter);
	runtime_interpreter->set_name("RuntimeEventInterpreter");
	add_child(runtime_interpreter);
	runtime_interpreter->setup_interpreter(this, runtime_player, runtime_followers);
	runtime_player->setup_on_map(this, runtime_followers, runtime_interpreter);

	// 5. Panel y Capa de Depuración (F3)
	runtime_debug_overlay = memnew(ClockGearsDebugOverlay);
	runtime_debug_overlay->set_name("RuntimeDebugOverlay");
	add_child(runtime_debug_overlay);
	runtime_debug_overlay->setup_overlay(this, runtime_player, runtime_camera);
}

void ClockGearsMap::unhandled_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		if (k->get_keycode() == Key::F11 || (k->get_keycode() == Key::ENTER && k->is_alt_pressed())) {
			DisplayServer *ds = DisplayServer::get_singleton();
			if (ds) {
				DisplayServerEnums::WindowMode mode = ds->window_get_mode();
				if (mode == DisplayServerEnums::WINDOW_MODE_FULLSCREEN || mode == DisplayServerEnums::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
					ds->window_set_mode(DisplayServerEnums::WINDOW_MODE_WINDOWED);
				} else {
					ds->window_set_mode(DisplayServerEnums::WINDOW_MODE_FULLSCREEN);
				}
				if (get_viewport()) {
					get_viewport()->set_input_as_handled();
				}
				return;
			}
		} else if (k->get_keycode() == Key::F3) {
			if (runtime_debug_overlay) {
				runtime_debug_overlay->toggle_debug_panel();
				if (get_viewport()) {
					get_viewport()->set_input_as_handled();
				}
				return;
			}
		}
	}
}

ClockGearsMap::ClockGearsMap() {
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	set_process_unhandled_input(true);
}
