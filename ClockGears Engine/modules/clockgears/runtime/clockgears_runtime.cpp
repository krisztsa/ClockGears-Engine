#include "clockgears_runtime.h"
#include "../clockgears_battle_window.h"
#include "../core/clockgears_data_manager.h"
#include "../core/clockgears_system_catalog.h"
#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/style_box_flat.h"

#define CG_RT_TR(m_str) String::utf8(m_str)

static void _cg_collect_events_recursive(Node *p_node, Vector<ClockGearsEvent *> &r_events) {
	if (!p_node) {
		return;
	}
	ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(p_node);
	if (ev) {
		r_events.push_back(ev);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_cg_collect_events_recursive(p_node->get_child(i), r_events);
	}
}

static Vector<ClockGearsEvent *> _cg_get_map_events(ClockGearsMap *p_map) {
	Vector<ClockGearsEvent *> list;
	if (!p_map) {
		return list;
	}
	_cg_collect_events_recursive(p_map, list);
	return list;
}

static Ref<Texture2D> _cg_load_runtime_texture(const String &p_path) {
	if (p_path.is_empty()) {
		return Ref<Texture2D>();
	}
	if (ResourceLoader::exists(p_path)) {
		Ref<Texture2D> res = ResourceLoader::load(p_path);
		if (res.is_valid()) {
			return res;
		}
	}
	Ref<Image> img = Image::load_from_file(p_path);
	if (img.is_valid() && !img->is_empty()) {
		return ImageTexture::create_from_image(img);
	}
	return Ref<Texture2D>();
}

static Ref<Texture2D> _cg_make_pixel_char_sheet(const Color &p_body, const Color &p_hair) {
	// Genera una hoja 3x4 (96x128, celdas de 32x32) pixel-perfect cuando el actor aún no tiene PNG asignado
	const int w = 96;
	const int h = 128;
	Ref<Image> img = Image::create_empty(w, h, false, Image::FORMAT_RGBA8);
	img->fill(Color(0, 0, 0, 0));

	Color outline(0.08f, 0.08f, 0.10f, 1.0f);
	Color skin(0.96f, 0.82f, 0.68f, 1.0f);
	Color eye(0.12f, 0.14f, 0.20f, 1.0f);

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 3; col++) {
			int ox = col * 32;
			int oy = row * 32;
			int step_shift = (col == 0) ? -1 : ((col == 2) ? 1 : 0);

			// Sombra bajo los pies
			for (int x = 9; x <= 22; x++) {
				for (int y = 27; y <= 30; y++) {
					img->set_pixel(ox + x, oy + y, Color(0, 0, 0, 0.35f));
				}
			}
			// Piernas / botas (con paso animado)
			int left_leg_y = 22 + (step_shift < 0 ? -1 : 0);
			int right_leg_y = 22 + (step_shift > 0 ? -1 : 0);
			for (int x = 11; x <= 14; x++) {
				for (int y = left_leg_y; y <= 27; y++) {
					img->set_pixel(ox + x, oy + y, outline);
				}
			}
			for (int x = 17; x <= 20; x++) {
				for (int y = right_leg_y; y <= 27; y++) {
					img->set_pixel(ox + x, oy + y, outline);
				}
			}
			// Torso / túnica
			for (int x = 9; x <= 22; x++) {
				for (int y = 14; y <= 22; y++) {
					bool border = (x == 9 || x == 22 || y == 14 || y == 22);
					img->set_pixel(ox + x, oy + y, border ? outline : p_body);
				}
			}
			// Cabeza
			for (int x = 9; x <= 22; x++) {
				for (int y = 4; y <= 14; y++) {
					bool border = (x == 9 || x == 22 || y == 4 || y == 14);
					Color c = skin;
					if (border) {
						c = outline;
					} else if (y <= 8 || (row == 3 && y <= 12)) {
						c = p_hair;
					}
					img->set_pixel(ox + x, oy + y, c);
				}
			}
			// Ojos según dirección (row 0=Abajo, 1=Izquierda, 2=Derecha, 3=Arriba)
			if (row == 0) {
				img->set_pixel(ox + 13, oy + 10, eye);
				img->set_pixel(ox + 13, oy + 11, eye);
				img->set_pixel(ox + 18, oy + 10, eye);
				img->set_pixel(ox + 18, oy + 11, eye);
			} else if (row == 1) {
				img->set_pixel(ox + 11, oy + 10, eye);
				img->set_pixel(ox + 11, oy + 11, eye);
			} else if (row == 2) {
				img->set_pixel(ox + 20, oy + 10, eye);
				img->set_pixel(ox + 20, oy + 11, eye);
			}
		}
	}
	return ImageTexture::create_from_image(img);
}

// ============================================================================
// ClockGearsPlayer
// ============================================================================

void ClockGearsPlayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_grid_cell_immediate", "cell", "dir"), &ClockGearsPlayer::set_grid_cell_immediate, DEFVAL(2));
	ClassDB::bind_method(D_METHOD("get_grid_cell"), &ClockGearsPlayer::get_grid_cell);
	ClassDB::bind_method(D_METHOD("set_direction", "dir"), &ClockGearsPlayer::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &ClockGearsPlayer::get_direction);
	ClassDB::bind_method(D_METHOD("set_allow_diagonal", "allow"), &ClockGearsPlayer::set_allow_diagonal);
	ClassDB::bind_method(D_METHOD("is_allow_diagonal"), &ClockGearsPlayer::is_allow_diagonal);
	ClassDB::bind_method(D_METHOD("set_through_enabled", "through"), &ClockGearsPlayer::set_through_enabled);
	ClassDB::bind_method(D_METHOD("is_through_enabled"), &ClockGearsPlayer::is_through_enabled);
	ClassDB::bind_method(D_METHOD("set_walk_speed", "speed"), &ClockGearsPlayer::set_walk_speed);
	ClassDB::bind_method(D_METHOD("get_walk_speed"), &ClockGearsPlayer::get_walk_speed);
	ClassDB::bind_method(D_METHOD("is_moving_step"), &ClockGearsPlayer::is_moving_step);
	ClassDB::bind_method(D_METHOD("try_step", "dir", "is_dash"), &ClockGearsPlayer::try_step, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("try_activate_front_event"), &ClockGearsPlayer::try_activate_front_event);
	ClassDB::bind_method(D_METHOD("lock_action_key"), &ClockGearsPlayer::lock_action_key);
	ClassDB::bind_method(D_METHOD("unlock_action_key"), &ClockGearsPlayer::unlock_action_key);
	ClassDB::bind_method(D_METHOD("is_action_key_locked"), &ClockGearsPlayer::is_action_key_locked);
	ClassDB::bind_method(D_METHOD("check_touch_events_at", "cell"), &ClockGearsPlayer::check_touch_events_at);
	ClassDB::bind_method(D_METHOD("load_actor_data", "actor_id"), &ClockGearsPlayer::load_actor_data);
	ClassDB::bind_method(D_METHOD("setup_on_map", "map", "followers", "interpreter"), &ClockGearsPlayer::setup_on_map, DEFVAL(Variant()), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("can_pass", "from", "to", "dir"), &ClockGearsPlayer::can_pass);

	ADD_SIGNAL(MethodInfo("step_started", PropertyInfo(Variant::VECTOR2I, "from_cell"), PropertyInfo(Variant::VECTOR2I, "to_cell"), PropertyInfo(Variant::INT, "direction")));
	ADD_SIGNAL(MethodInfo("step_finished", PropertyInfo(Variant::VECTOR2I, "cell")));
}

Vector2i ClockGearsPlayer::direction_to_vector(int p_dir) {
	switch (p_dir) {
		case 2: return Vector2i(0, 1);   // Abajo
		case 4: return Vector2i(-1, 0);  // Izquierda
		case 6: return Vector2i(1, 0);   // Derecha
		case 8: return Vector2i(0, -1);  // Arriba
		case 1: return Vector2i(-1, 1);  // Diagonal Abajo-Izq
		case 3: return Vector2i(1, 1);   // Diagonal Abajo-Der
		case 7: return Vector2i(-1, -1); // Diagonal Arriba-Izq
		case 9: return Vector2i(1, -1);  // Diagonal Arriba-Der
		default: return Vector2i(0, 1);
	}
}

int ClockGearsPlayer::opposite_direction(int p_dir) {
	switch (p_dir) {
		case 2: return 8;
		case 8: return 2;
		case 4: return 6;
		case 6: return 4;
		case 1: return 9;
		case 9: return 1;
		case 3: return 7;
		case 7: return 3;
		default: return 2;
	}
}

int ClockGearsPlayer::_direction_to_row(int p_dir) const {
	switch (p_dir) {
		case 2:
		case 1:
		case 3:
			return 0; // Abajo
		case 4:
			return 1; // Izquierda
		case 6:
			return 2; // Derecha
		case 8:
		case 7:
		case 9:
			return 3; // Arriba
		default:
			return 0;
	}
}

void ClockGearsPlayer::_update_sprite_frame() {
	if (!sprite || sprite->get_texture().is_null()) {
		return;
	}
	if (!is_spritesheet) {
		sprite->set_hframes(1);
		sprite->set_vframes(1);
		sprite->set_frame(0);
		sprite->set_position(Vector2(0, 0));
		return;
	}
	sprite->set_hframes(sprite_cols);
	sprite->set_vframes(sprite_rows);
	const int walk_cycle[4] = { 0, 1, 2, 1 };
	int col = moving_step ? (walk_cycle[anim_pattern_idx % 4] % sprite_cols) : (1 % sprite_cols);
	int row = CLAMP(_direction_to_row(direction), 0, sprite_rows - 1);
	sprite->set_frame(row * sprite_cols + col);

	int frame_h = sprite->get_texture()->get_height() / sprite_rows;
	int frame_w = sprite->get_texture()->get_width() / sprite_cols;
	sprite->set_position(Vector2((tile_size - frame_w) / 2, tile_size - frame_h));
}

void ClockGearsPlayer::load_actor_data(int p_actor_id) {
	actor_id = p_actor_id;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && actor_id > 0) {
		Dictionary rec = dm->get_record_by_id(CAT_ACTORS, actor_id);
		if (!rec.is_empty()) {
			actor_name = rec.get("name", "");
			graphic_path = rec.get("graphic_path", "");
		}
	}

	Ref<Texture2D> tex = _cg_load_runtime_texture(graphic_path);
	if (tex.is_valid()) {
		int tw = tex->get_width();
		int th = tex->get_height();
		if (th >= 64 && th % 4 == 0 && (tw % 3 == 0 || tw % 4 == 0)) {
			is_spritesheet = true;
			sprite_rows = 4;
			sprite_cols = (tw % 3 == 0) ? 3 : 4;
		} else {
			is_spritesheet = false;
			sprite_rows = 1;
			sprite_cols = 1;
		}
		sprite->set_texture(tex);
	} else {
		is_spritesheet = true;
		sprite_cols = 3;
		sprite_rows = 4;
		sprite->set_texture(_cg_make_pixel_char_sheet(Color(0.22f, 0.52f, 0.88f, 1.0f), Color(0.88f, 0.65f, 0.25f, 1.0f)));
	}
	_update_sprite_frame();
}

void ClockGearsPlayer::setup_on_map(ClockGearsMap *p_map, ClockGearsFollowerManager *p_followers, ClockGearsEventInterpreter *p_interpreter) {
	current_map = p_map;
	follower_manager = p_followers;
	event_interpreter = p_interpreter;
	if (current_map) {
		tile_size = MAX(16, current_map->get_grid_resolution());
	}
	_update_sprite_frame();
}

void ClockGearsPlayer::set_grid_cell_immediate(const Vector2i &p_cell, int p_dir) {
	grid_cell = p_cell;
	direction = p_dir;
	moving_step = false;
	step_start_px = Vector2(grid_cell.x * tile_size, grid_cell.y * tile_size);
	step_target_px = step_start_px;
	set_position(step_start_px);
	anim_pattern_idx = 1;
	_update_sprite_frame();
}

void ClockGearsPlayer::set_direction(int p_dir) {
	direction = p_dir;
	_update_sprite_frame();
}

int ClockGearsPlayer::_get_tile_passage_mode(const Vector2i &p_cell, int &r_dir4_mask) const {
	r_dir4_mask = 15; // 1|2|4|8 = todas las direcciones permitidas por defecto
	if (!current_map) {
		return 0;
	}

	// Revisar metadatos de colisión del Tileset activo en el mapa actual
	Dictionary slot_cols;
	Dictionary legacy_cols;
	int ts_id = current_map->get_tileset_id();
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Array ts_arr = dm->get_category_array(CAT_TILESETS);
		for (int i = 0; i < ts_arr.size(); i++) {
			if (ts_arr[i].get_type() == Variant::DICTIONARY) {
				Dictionary d = ts_arr[i];
				int id = int(d.get("id", i + 1));
				if (ts_id == 0 || id == ts_id) {
					slot_cols = d.get("slot_collisions", Dictionary());
					legacy_cols = d.get("collisions", Dictionary());
					break;
				}
			}
		}
	}

	const int check_layers[4] = { LAYER_OVERHEAD, LAYER_OBJECTS, LAYER_DECORATION, LAYER_GROUND };
	for (int i = 0; i < 4; i++) {
		TileMapLayer *tlayer = current_map->get_layer(check_layers[i]);
		if (!tlayer || tlayer->get_cell_source_id(p_cell) == -1) {
			continue;
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
			Variant lmeta = legacy_cols[key];
			if (lmeta.get_type() == Variant::DICTIONARY) {
				md = lmeta;
				found_meta = true;
			} else if (int(lmeta) == 1) {
				return 1; // Bloqueado legacy
			}
		}

		TileData *td = tlayer->get_cell_tile_data(p_cell);
		if (td && td->get_z_index() > 3) {
			continue; // Tile elevado sobre jugador (★), no bloquea
		}

		if (found_meta) {
			int passage = int(md.get("passage", 15));
			int layer = int(md.get("layer", 0));
			r_dir4_mask = passage & 15;
			if (layer == 1 || int(md.get("mode", 0)) == 2) {
				continue; // Estrella (★): no bloquea, el personaje pasa por detrás
			}
			if (passage == 0 || int(md.get("mode", 0)) == 1) {
				return 1; // Bloqueado (X)
			}
		}
		Ref<TileSet> ts = tlayer->get_tile_set();
		if (td && ts.is_valid()) {
			if (ts->has_custom_data_layer_by_name("cg_layer")) {
				int ly = int(td->get_custom_data("cg_layer"));
				if (ly == 1) {
					return 0; // Estrella (★)
				}
			}
			if (ts->has_custom_data_layer_by_name("cg_passage_4dir")) {
				Variant d4 = td->get_custom_data("cg_passage_4dir");
				if (d4.get_type() == Variant::INT) {
					r_dir4_mask = int(d4);
				}
			}
			if (ts->has_custom_data_layer_by_name("cg_passage")) {
				Variant pm = td->get_custom_data("cg_passage");
				if (pm.get_type() == Variant::INT) {
					int pval = int(pm);
					if (pval == 0) {
						return 1; // Bloqueado (X)
					}
					r_dir4_mask = pval & 15;
				}
			}
		}
	}
	return 0;
}

bool ClockGearsPlayer::_is_tile_counter(const Vector2i &p_cell) const {
	if (!current_map) {
		return false;
	}
	Dictionary slot_cols;
	Dictionary db_collisions;
	int ts_id = current_map->get_tileset_id();
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm) {
		Array ts_arr = dm->get_category_array(CAT_TILESETS);
		for (int i = 0; i < ts_arr.size(); i++) {
			if (ts_arr[i].get_type() == Variant::DICTIONARY) {
				Dictionary d = ts_arr[i];
				int id = int(d.get("id", i + 1));
				if (ts_id == 0 || id == ts_id) {
					slot_cols = d.get("slot_collisions", Dictionary());
					db_collisions = d.get("collisions", Dictionary());
					break;
				}
			}
		}
	}
	const int check_layers[4] = { LAYER_OVERHEAD, LAYER_OBJECTS, LAYER_DECORATION, LAYER_GROUND };
	for (int i = 0; i < 4; i++) {
		TileMapLayer *tlayer = current_map->get_layer(check_layers[i]);
		if (!tlayer || tlayer->get_cell_source_id(p_cell) == -1) {
			continue;
		}
		Vector2i ac = tlayer->get_cell_atlas_coords(p_cell);
		String key = vformat("%d,%d", ac.x, ac.y);
		static const char *skeys[9] = { "A5", "B", "C", "D", "E", "A1", "A2", "A3", "A4" };
		for (int s = 0; s < 9; s++) {
			if (slot_cols.has(skeys[s])) {
				Dictionary smap = slot_cols[skeys[s]];
				if (smap.has(key) && bool(Dictionary(smap[key]).get("counter", false))) {
					return true;
				}
			}
		}
		if (db_collisions.has(key) && db_collisions[key].get_type() == Variant::DICTIONARY) {
			Dictionary md = db_collisions[key];
			if (bool(md.get("counter", false))) {
				return true;
			}
		}
		TileData *td = tlayer->get_cell_tile_data(p_cell);
		Ref<TileSet> ts = tlayer->get_tile_set();
		if (td && ts.is_valid() && ts->has_custom_data_layer_by_name("cg_counter")) {
			if (bool(td->get_custom_data("cg_counter"))) {
				return true;
			}
		}
	}
	return false;
}

bool ClockGearsPlayer::can_pass_cell(const Vector2i &p_from, const Vector2i &p_to, int p_dir, ClockGearsEvent **r_blocking_event) const {
	if (r_blocking_event) {
		*r_blocking_event = nullptr;
	}
	if (!current_map) {
		return true;
	}
	Vector2i msize = current_map->get_map_size();
	if (p_to.x < 0 || p_to.y < 0 || p_to.x >= msize.x || p_to.y >= msize.y) {
		return false;
	}
	if (through_enabled) {
		return true;
	}

	int from_dir4 = 15;
	_get_tile_passage_mode(p_from, from_dir4);
	int to_dir4 = 15;
	int to_mode = _get_tile_passage_mode(p_to, to_dir4);
	if (to_mode == 1) {
		return false; // Tile bloqueado (X)
	}

	// Comprobar pasabilidad direccional de 4 direcciones (bitmask: 1=Abajo, 2=Izq, 4=Der, 8=Arriba)
	auto dir_to_bit = [](int d) -> int {
		switch (d) {
			case 2: return 1;
			case 4: return 2;
			case 6: return 4;
			case 8: return 8;
			default: return 0;
		}
	};
	int exit_bit = dir_to_bit(p_dir);
	int enter_bit = dir_to_bit(opposite_direction(p_dir));
	if (exit_bit != 0 && (from_dir4 & exit_bit) == 0) {
		return false;
	}
	if (enter_bit != 0 && (to_dir4 & enter_bit) == 0) {
		return false;
	}

	// Comprobar colisión con Eventos en la celda destino (Prioridad 1 = Mismo nivel que el jugador, sin Through)
	Vector<ClockGearsEvent *> ev_nodes = _cg_get_map_events(current_map);
	for (int i = 0; i < ev_nodes.size(); i++) {
		ClockGearsEvent *ev = ev_nodes[i];
		if (!ev || !ev->is_visible() || ev->is_through()) {
			continue;
		}
		int ecx = int(Math::floor(ev->get_position().x / float(tile_size)));
		int ecy = int(Math::floor(ev->get_position().y / float(tile_size)));
		if (ecx == p_to.x && ecy == p_to.y && ev->get_priority_type() == 1) {
			if (r_blocking_event) {
				*r_blocking_event = ev;
			}
			return false;
		}
	}
	return true;
}

bool ClockGearsPlayer::try_step(int p_dir, bool p_is_dash) {
	if (moving_step) {
		return false;
	}
	if (event_interpreter && event_interpreter->is_blocking_player()) {
		return false;
	}

	set_direction(p_dir);
	Vector2i delta = direction_to_vector(p_dir);
	Vector2i target_cell = grid_cell + delta;

	ClockGearsEvent *blocking_ev = nullptr;
	if (!can_pass_cell(grid_cell, target_cell, p_dir, &blocking_ev)) {
		// Si chocamos con un evento que tiene disparador de Contacto (Jugador o Evento), lo activamos
		if (blocking_ev && event_interpreter) {
			int trig = blocking_ev->get_trigger_type();
			if (trig == TRIGGER_PLAYER_TOUCH || trig == TRIGGER_EVENT_TOUCH) {
				if (!blocking_ev->is_direction_fix()) {
					blocking_ev->set_direction(opposite_direction(direction));
				}
				event_interpreter->start_event(blocking_ev);
			}
		}
		return false;
	}

	Vector2i prev_cell = grid_cell;
	grid_cell = target_cell;
	step_start_px = get_position();
	step_target_px = Vector2(grid_cell.x * tile_size, grid_cell.y * tile_size);
	step_target_cell = target_cell;
	current_step_speed = p_is_dash ? dash_speed : walk_speed;
	moving_step = true;

	if (follower_manager) {
		follower_manager->record_leader_step(prev_cell, p_dir, current_step_speed);
	}
	emit_signal("step_started", prev_cell, target_cell, p_dir);
	return true;
}

bool ClockGearsPlayer::try_activate_front_event() {
	if (moving_step || !current_map || !event_interpreter || event_interpreter->is_blocking_player() || action_cooldown > 0.0f) {
		return false;
	}

	Vector<ClockGearsEvent *> ev_nodes = _cg_get_map_events(current_map);

	auto find_event_at = [&](const Vector2i &p_c) -> ClockGearsEvent * {
		for (int i = 0; i < ev_nodes.size(); i++) {
			ClockGearsEvent *ev = ev_nodes[i];
			if (!ev || !ev->is_visible()) {
				continue;
			}
			int ecx = int(Math::floor(ev->get_position().x / float(tile_size)));
			int ecy = int(Math::floor(ev->get_position().y / float(tile_size)));
			if (ecx == p_c.x && ecy == p_c.y && ev->get_trigger_type() == TRIGGER_ACTION_BUTTON) {
				return ev;
			}
		}
		return nullptr;
	};

	// 1. Misma celda (Prioridad 0 = Debajo del jugador o cualquier evento con Botón de Acción bajo los pies)
	ClockGearsEvent *same_ev = find_event_at(grid_cell);
	if (same_ev) {
		event_interpreter->start_event(same_ev);
		return true;
	}

	// 2. Celda frontal inmediata
	Vector2i front_cell = grid_cell + direction_to_vector(direction);
	ClockGearsEvent *front_ev = find_event_at(front_cell);
	if (front_ev) {
		if (!front_ev->is_direction_fix()) {
			front_ev->set_direction(opposite_direction(direction));
		}
		event_interpreter->start_event(front_ev);
		return true;
	}

	// 3. Si la celda frontal tiene flag Mostrador (♦), buscar 1 celda más allá
	if (_is_tile_counter(front_cell)) {
		Vector2i counter_target = front_cell + direction_to_vector(direction);
		ClockGearsEvent *counter_ev = find_event_at(counter_target);
		if (counter_ev) {
			if (!counter_ev->is_direction_fix()) {
				counter_ev->set_direction(opposite_direction(direction));
			}
			event_interpreter->start_event(counter_ev);
			return true;
		}
	}

	return false;
}

void ClockGearsPlayer::lock_action_key() {
	action_key_was_pressed = true;
	action_key_released = false;
	action_cooldown = 0.20f;
	diag_key_blocked_printed = false;
}

void ClockGearsPlayer::unlock_action_key() {
	action_key_released = true;
	action_key_was_pressed = false;
	action_cooldown = 0.0f;
	diag_key_blocked_printed = false;
}

bool ClockGearsPlayer::is_action_key_locked() const {
	return !action_key_released || action_cooldown > 0.0f;
}

bool ClockGearsPlayer::check_touch_events_at(const Vector2i &p_cell) {
	if (!current_map || !event_interpreter || event_interpreter->is_blocking_player()) {
		return false;
	}
	Vector<ClockGearsEvent *> ev_nodes = _cg_get_map_events(current_map);
	for (int i = 0; i < ev_nodes.size(); i++) {
		ClockGearsEvent *ev = ev_nodes[i];
		if (!ev || !ev->is_visible()) {
			continue;
		}
		int ecx = int(Math::floor(ev->get_position().x / float(tile_size)));
		int ecy = int(Math::floor(ev->get_position().y / float(tile_size)));
		if (ecx == p_cell.x && ecy == p_cell.y) {
			int trig = ev->get_trigger_type();
			if (trig == TRIGGER_PLAYER_TOUCH || trig == TRIGGER_EVENT_TOUCH) {
				event_interpreter->start_event(ev);
				return true;
			}
		}
	}
	return false;
}

void ClockGearsPlayer::_notification(int p_what) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_physics_process(true);
		} break;

		case NOTIFICATION_PHYSICS_PROCESS: {
			float dt = get_physics_process_delta_time();
			Input *inp = Input::get_singleton();

			if (action_cooldown > 0.0f) {
				action_cooldown -= dt;
			}

			if (moving_step) {
				Vector2 cur = get_position();
				Vector2 diff = step_target_px - cur;
				float dist = diff.length();
				float move_dist = current_step_speed * dt;

				anim_timer += dt * (current_step_speed / walk_speed);
				if (anim_timer >= 0.14f) {
					anim_timer = 0.0f;
					anim_pattern_idx = (anim_pattern_idx + 1) % 4;
					_update_sprite_frame();
				}

				if (dist <= move_dist) {
					set_position(step_target_px.round());
					moving_step = false;
					emit_signal("step_finished", grid_cell);
					check_touch_events_at(grid_cell);
				} else {
					Vector2 next_pos = cur + diff.normalized() * move_dist;
					// A4.1: Redondear posición visual a píxel entero
					set_position(next_pos.round());
				}
			}

			if (!moving_step && inp && (!event_interpreter || !event_interpreter->is_blocking_player())) {
				bool act_now = inp->is_key_pressed(Key::ENTER) || inp->is_key_pressed(Key::KP_ENTER) ||
						inp->is_key_pressed(Key::SPACE) || inp->is_key_pressed(Key::Z);

				if (!act_now) {
					action_key_released = true;
					action_key_was_pressed = false;
					diag_key_blocked_printed = false;
				} else if (!action_key_released) {
					// Jugador mantiene pulsada la tecla de cierre; esperar a que la suelte.
				} else if (act_now && !action_key_was_pressed && action_key_released && action_cooldown <= 0.0f) {
					action_key_was_pressed = true;
					action_key_released = false;
					diag_key_blocked_printed = false;
					if (try_activate_front_event()) {
						return;
					}
				}

				bool up = inp->is_key_pressed(Key::UP) || inp->is_key_pressed(Key::W);
				bool down = inp->is_key_pressed(Key::DOWN) || inp->is_key_pressed(Key::S);
				bool left = inp->is_key_pressed(Key::LEFT) || inp->is_key_pressed(Key::A);
				bool right = inp->is_key_pressed(Key::RIGHT) || inp->is_key_pressed(Key::D);
				bool dash = inp->is_key_pressed(Key::SHIFT);

				int desired_dir = 0;
				if (allow_diagonal) {
					if (down && left) desired_dir = 1;
					else if (down && right) desired_dir = 3;
					else if (up && left) desired_dir = 7;
					else if (up && right) desired_dir = 9;
				}
				if (desired_dir == 0) {
					if (down) desired_dir = 2;
					else if (up) desired_dir = 8;
					else if (left) desired_dir = 4;
					else if (right) desired_dir = 6;
				}

				if (desired_dir != 0) {
					try_step(desired_dir, dash);
				} else if (anim_pattern_idx != 1) {
					anim_pattern_idx = 1;
					_update_sprite_frame();
				}
			} else if (inp) {
				bool act_now = inp->is_key_pressed(Key::ENTER) || inp->is_key_pressed(Key::KP_ENTER) ||
						inp->is_key_pressed(Key::SPACE) || inp->is_key_pressed(Key::Z);
				if (!act_now) {
					action_key_released = true;
					action_key_was_pressed = false;
				} else {
					action_key_was_pressed = true;
					action_key_released = false;
				}
			}
		} break;
	}
}

ClockGearsPlayer::ClockGearsPlayer() {
	set_z_index(3);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	sprite = memnew(Sprite2D);
	sprite->set_centered(false);
	sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	add_child(sprite);
}

// ============================================================================
// ClockGearsFollower
// ============================================================================

void ClockGearsFollower::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_grid_cell"), &ClockGearsFollower::get_grid_cell);
	ClassDB::bind_method(D_METHOD("get_direction"), &ClockGearsFollower::get_direction);
	ClassDB::bind_method(D_METHOD("set_follower_enabled", "enabled"), &ClockGearsFollower::set_follower_enabled);
	ClassDB::bind_method(D_METHOD("is_follower_enabled"), &ClockGearsFollower::is_follower_enabled);
}

void ClockGearsFollower::_update_sprite_frame() {
	if (!sprite || sprite->get_texture().is_null()) {
		return;
	}
	if (!is_spritesheet) {
		sprite->set_hframes(1);
		sprite->set_vframes(1);
		sprite->set_frame(0);
		sprite->set_position(Vector2(0, 0));
		return;
	}
	sprite->set_hframes(sprite_cols);
	sprite->set_vframes(sprite_rows);
	const int walk_cycle[4] = { 0, 1, 2, 1 };
	int col = moving_step ? (walk_cycle[anim_pattern_idx % 4] % sprite_cols) : (1 % sprite_cols);
	int row = 0;
	if (direction == 4) row = 1;
	else if (direction == 6) row = 2;
	else if (direction == 8 || direction == 7 || direction == 9) row = 3;
	sprite->set_frame(CLAMP(row, 0, sprite_rows - 1) * sprite_cols + col);

	int frame_h = sprite->get_texture()->get_height() / sprite_rows;
	int frame_w = sprite->get_texture()->get_width() / sprite_cols;
	sprite->set_position(Vector2((tile_size - frame_w) / 2, tile_size - frame_h));
}

void ClockGearsFollower::setup_follower(int p_member_index, int p_actor_id, int p_tile_size) {
	member_index = p_member_index;
	actor_id = p_actor_id;
	tile_size = MAX(16, p_tile_size);

	String gpath;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && actor_id > 0) {
		Dictionary rec = dm->get_record_by_id(CAT_ACTORS, actor_id);
		gpath = rec.get("graphic_path", "");
	}

	Ref<Texture2D> tex = _cg_load_runtime_texture(gpath);
	if (tex.is_valid()) {
		int tw = tex->get_width();
		int th = tex->get_height();
		if (th >= 64 && th % 4 == 0 && (tw % 3 == 0 || tw % 4 == 0)) {
			is_spritesheet = true;
			sprite_rows = 4;
			sprite_cols = (tw % 3 == 0) ? 3 : 4;
		} else {
			is_spritesheet = false;
			sprite_rows = 1;
			sprite_cols = 1;
		}
		sprite->set_texture(tex);
	} else {
		is_spritesheet = true;
		sprite_cols = 3;
		sprite_rows = 4;
		const Color palette[4] = {
			Color(0.28f, 0.72f, 0.42f, 1.0f),
			Color(0.82f, 0.32f, 0.38f, 1.0f),
			Color(0.68f, 0.38f, 0.85f, 1.0f),
			Color(0.85f, 0.62f, 0.22f, 1.0f)
		};
		sprite->set_texture(_cg_make_pixel_char_sheet(palette[(member_index - 1) % 4], Color(0.35f, 0.25f, 0.18f, 1.0f)));
	}
	_update_sprite_frame();
}

void ClockGearsFollower::set_cell_immediate(const Vector2i &p_cell, int p_dir) {
	grid_cell = p_cell;
	direction = p_dir;
	moving_step = false;
	stuck_frames = 0;
	step_start_px = Vector2(grid_cell.x * tile_size, grid_cell.y * tile_size);
	step_target_px = step_start_px;
	set_position(step_start_px);
	anim_pattern_idx = 1;
	_update_sprite_frame();
}

void ClockGearsFollower::move_toward_cell(const Vector2i &p_target_cell, int p_dir, float p_speed) {
	if (grid_cell == p_target_cell && !moving_step) {
		direction = p_dir;
		_update_sprite_frame();
		return;
	}
	Vector2i diff = p_target_cell - grid_cell;
	if (diff.y > 0) direction = 2;
	else if (diff.y < 0) direction = 8;
	else if (diff.x < 0) direction = 4;
	else if (diff.x > 0) direction = 6;
	else direction = p_dir;

	grid_cell = p_target_cell;
	step_start_px = get_position();
	step_target_px = Vector2(grid_cell.x * tile_size, grid_cell.y * tile_size);
	current_step_speed = MAX(32.0f, p_speed);
	moving_step = true;
	_update_sprite_frame();
}

void ClockGearsFollower::tick_follower(float p_delta, const Vector2i &p_leader_cell) {
	if (!moving_step) {
		stuck_frames = 0;
		return;
	}
	Vector2 cur = get_position();
	Vector2 diff = step_target_px - cur;
	float dist = diff.length();
	float move_dist = current_step_speed * p_delta;

	anim_timer += p_delta;
	if (anim_timer >= 0.14f) {
		anim_timer = 0.0f;
		anim_pattern_idx = (anim_pattern_idx + 1) % 4;
		_update_sprite_frame();
	}

	if (dist > float(tile_size * 4)) {
		stuck_frames++;
		if (stuck_frames > 120) {
			// Salvaguarda anti-atasco (> 120 frames lejos): teletransportar suavemente a su celda objetivo
			set_position(step_target_px.round());
			moving_step = false;
			stuck_frames = 0;
			anim_pattern_idx = 1;
			_update_sprite_frame();
			return;
		}
	} else {
		stuck_frames = 0;
	}

	if (dist <= move_dist) {
		set_position(step_target_px.round());
		moving_step = false;
		anim_pattern_idx = 1;
		_update_sprite_frame();
	} else {
		set_position((cur + diff.normalized() * move_dist).round());
	}
}

void ClockGearsFollower::set_follower_enabled(bool p_enabled) {
	enabled = p_enabled;
	set_visible(enabled);
}

ClockGearsFollower::ClockGearsFollower() {
	set_z_index(3);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	sprite = memnew(Sprite2D);
	sprite->set_centered(false);
	sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	add_child(sprite);
}

// ============================================================================
// ClockGearsFollowerManager
// ============================================================================

void ClockGearsFollowerManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup_party_followers", "party_actor_ids", "start_cell", "start_dir", "tile_size"), &ClockGearsFollowerManager::setup_party_followers, DEFVAL(2), DEFVAL(32));
	ClassDB::bind_method(D_METHOD("record_leader_step", "previous_leader_cell", "dir", "speed"), &ClockGearsFollowerManager::record_leader_step);
	ClassDB::bind_method(D_METHOD("gather_followers", "cell", "dir"), &ClockGearsFollowerManager::gather_followers, DEFVAL(2));
	ClassDB::bind_method(D_METHOD("set_followers_visible_flag", "visible"), &ClockGearsFollowerManager::set_followers_visible_flag);
	ClassDB::bind_method(D_METHOD("are_followers_visible"), &ClockGearsFollowerManager::are_followers_visible);
	ClassDB::bind_method(D_METHOD("set_follower_member_enabled", "member_idx", "enabled"), &ClockGearsFollowerManager::set_follower_member_enabled);
	ClassDB::bind_method(D_METHOD("get_follower_count"), &ClockGearsFollowerManager::get_follower_count);
	ClassDB::bind_method(D_METHOD("get_follower_cell", "idx"), &ClockGearsFollowerManager::get_follower_cell);
}

void ClockGearsFollowerManager::setup_party_followers(const Array &p_party_actor_ids, const Vector2i &p_start_cell, int p_start_dir, int p_tile_size) {
	tile_size = MAX(16, p_tile_size);
	for (int i = 0; i < followers.size(); i++) {
		if (followers[i]) {
			followers[i]->queue_free();
		}
	}
	followers.clear();
	step_history.clear();

	// El índice 0 del grupo es el Líder (ClockGearsPlayer); los índices 1..8 son los Seguidores
	int count = MIN(p_party_actor_ids.size() - 1, max_followers);
	for (int i = 0; i < count; i++) {
		int actor_id = int(p_party_actor_ids[i + 1]);
		ClockGearsFollower *f = memnew(ClockGearsFollower);
		f->set_name(vformat("Follower_%d", i + 1));
		add_child(f);
		f->setup_follower(i + 1, actor_id, tile_size);
		f->set_cell_immediate(p_start_cell, p_start_dir);
		f->set_visible(followers_visible);
		followers.push_back(f);
	}
}

void ClockGearsFollowerManager::record_leader_step(const Vector2i &p_previous_leader_cell, int p_dir, float p_speed) {
	StepHistoryEntry entry;
	entry.cell = p_previous_leader_cell;
	entry.direction = p_dir;
	entry.speed = p_speed;
	step_history.insert(0, entry);

	int max_hist = MAX(16, followers.size() + 2);
	while (step_history.size() > max_hist) {
		step_history.remove_at(step_history.size() - 1);
	}

	for (int i = 0; i < followers.size(); i++) {
		if (i < step_history.size() && followers[i]) {
			const StepHistoryEntry &st = step_history[i];
			followers[i]->move_toward_cell(st.cell, st.direction, st.speed);
		}
	}
}

void ClockGearsFollowerManager::gather_followers(const Vector2i &p_cell, int p_dir) {
	step_history.clear();
	for (int i = 0; i < followers.size(); i++) {
		if (followers[i]) {
			followers[i]->set_cell_immediate(p_cell, p_dir);
		}
	}
}

void ClockGearsFollowerManager::set_followers_visible_flag(bool p_visible) {
	followers_visible = p_visible;
	for (int i = 0; i < followers.size(); i++) {
		if (followers[i]) {
			followers[i]->set_visible(followers_visible && followers[i]->is_follower_enabled());
		}
	}
}

void ClockGearsFollowerManager::set_follower_member_enabled(int p_member_idx, bool p_enabled) {
	int idx = p_member_idx - 1;
	if (idx >= 0 && idx < followers.size() && followers[idx]) {
		followers[idx]->set_follower_enabled(p_enabled);
		followers[idx]->set_visible(followers_visible && p_enabled);
	}
}

Vector2i ClockGearsFollowerManager::get_follower_cell(int p_idx) const {
	if (p_idx >= 0 && p_idx < followers.size() && followers[p_idx]) {
		return followers[p_idx]->get_grid_cell();
	}
	return Vector2i(0, 0);
}

void ClockGearsFollowerManager::_notification(int p_what) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (p_what == NOTIFICATION_READY) {
		set_physics_process(true);
	} else if (p_what == NOTIFICATION_PHYSICS_PROCESS) {
		float dt = get_physics_process_delta_time();
		Vector2i leader_cell = step_history.is_empty() ? Vector2i(0, 0) : step_history[0].cell;
		for (int i = 0; i < followers.size(); i++) {
			if (followers[i]) {
				followers[i]->tick_follower(dt, leader_cell);
			}
		}
	}
}

ClockGearsFollowerManager::ClockGearsFollowerManager() {
	set_z_index(3);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
}

// ============================================================================
// ClockGearsEventInterpreter
// ============================================================================

void ClockGearsEventInterpreter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_running"), &ClockGearsEventInterpreter::is_running);
	ClassDB::bind_method(D_METHOD("setup_interpreter", "map", "player", "followers"), &ClockGearsEventInterpreter::setup_interpreter, DEFVAL(Variant()), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("is_blocking_player"), &ClockGearsEventInterpreter::is_blocking_player);
	ClassDB::bind_method(D_METHOD("get_state"), &ClockGearsEventInterpreter::get_state);
	ClassDB::bind_method(D_METHOD("get_last_choice_index"), &ClockGearsEventInterpreter::get_last_choice_index);
	ClassDB::bind_method(D_METHOD("start_event", "event"), &ClockGearsEventInterpreter::start_event);
	ClassDB::bind_method(D_METHOD("start_command_list", "commands", "source_event"), &ClockGearsEventInterpreter::start_command_list, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("advance_message_or_choice", "choice_idx"), &ClockGearsEventInterpreter::advance_message_or_choice, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("step_frame"), &ClockGearsEventInterpreter::step_frame);
	ClassDB::bind_method(D_METHOD("get_picture_manager"), &ClockGearsEventInterpreter::get_picture_manager);
	ClassDB::bind_method(D_METHOD("get_cutscene_player"), &ClockGearsEventInterpreter::get_cutscene_player);
	ClassDB::bind_method(D_METHOD("get_screen_tint_color"), &ClockGearsEventInterpreter::get_screen_tint_color);
	ClassDB::bind_method(D_METHOD("get_screen_tint_rect"), &ClockGearsEventInterpreter::get_screen_tint_rect);
	ClassDB::bind_method(D_METHOD("get_screen_tint_layer"), &ClockGearsEventInterpreter::get_screen_tint_layer);
	ClassDB::bind_method(D_METHOD("get_ui_layer"), &ClockGearsEventInterpreter::get_ui_layer);
	ClassDB::bind_method(D_METHOD("get_message_window"), &ClockGearsEventInterpreter::get_message_window);
	ClassDB::bind_method(D_METHOD("start_screen_shake", "power", "speed", "duration", "wait"), &ClockGearsEventInterpreter::start_screen_shake, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("stop_screen_shake"), &ClockGearsEventInterpreter::stop_screen_shake);
	ClassDB::bind_method(D_METHOD("get_current_shake_offset"), &ClockGearsEventInterpreter::get_current_shake_offset);
	ClassDB::bind_method(D_METHOD("is_screen_shaking"), &ClockGearsEventInterpreter::is_screen_shaking);
	ClassDB::bind_method(D_METHOD("_on_battle_scene_finished", "result"), &ClockGearsEventInterpreter::_on_battle_scene_finished);

	BIND_CONSTANT(STATE_IDLE);
	BIND_CONSTANT(STATE_RUNNING);
	BIND_CONSTANT(STATE_WAITING_FRAMES);
	BIND_CONSTANT(STATE_WAITING_MESSAGE);
	BIND_CONSTANT(STATE_WAITING_CHOICE);
	BIND_CONSTANT(STATE_WAITING_NUMBER);
	BIND_CONSTANT(STATE_WAITING_TRANSFER);
	BIND_CONSTANT(STATE_WAITING_BATTLE);
	BIND_CONSTANT(STATE_GAME_OVER);
	ClassDB::bind_method(D_METHOD("start_camera_zoom_transition", "target_zoom", "duration_frames"), &ClockGearsEventInterpreter::start_camera_zoom_transition, DEFVAL(0));

	ADD_SIGNAL(MethodInfo("event_started", PropertyInfo(Variant::OBJECT, "event")));
	ADD_SIGNAL(MethodInfo("event_finished", PropertyInfo(Variant::OBJECT, "event")));
	ADD_SIGNAL(MethodInfo("message_shown", PropertyInfo(Variant::STRING, "text")));
}

void ClockGearsEventInterpreter::_build_runtime_ui() {
	Size2 init_vsz = Size2(1920, 1080);
	if (is_inside_tree() && get_viewport()) {
		init_vsz = get_viewport()->get_visible_rect().size;
	}

	// 0. Capa de Tinte de Pantalla dedicada (Capa 10: Entre mapa/personajes a capa 0 y ventana de mensajes a capa 100)
	if (!screen_tint_layer) {
		screen_tint_layer = memnew(CanvasLayer);
		screen_tint_layer->set_name("ClockGearsScreenTintLayer");
		screen_tint_layer->set_layer(10);
		add_child(screen_tint_layer);

		screen_tint_rect = memnew(ColorRect);
		screen_tint_rect->set_name("ScreenTintRect");
		screen_tint_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		screen_tint_rect->set_color(current_tint_color);
		screen_tint_rect->set_position(Vector2(0, 0));
		screen_tint_rect->set_size(init_vsz);
		screen_tint_layer->add_child(screen_tint_rect);
	}

	if (ui_layer) {
		return;
	}
	ui_layer = memnew(CanvasLayer);
	ui_layer->set_name("ClockGearsRuntimeUI");
	ui_layer->set_layer(100);
	add_child(ui_layer);

	// 1. Capa de fundido de pantalla
	screen_fade_rect = memnew(ColorRect);
	screen_fade_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	screen_fade_rect->set_color(Color(0, 0, 0, 0));
	screen_fade_rect->set_position(Vector2(0, 0));
	screen_fade_rect->set_size(init_vsz);
	ui_layer->add_child(screen_fade_rect);

	// 2. Capa de flash de pantalla
	screen_flash_rect = memnew(ColorRect);
	screen_flash_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	screen_flash_rect->set_color(Color(1, 1, 1, 0));
	screen_flash_rect->set_position(Vector2(0, 0));
	screen_flash_rect->set_size(init_vsz);
	ui_layer->add_child(screen_flash_rect);

	// 3. Ventana de Mensajes Completa Pixel-Art (Fase 7A: ClockGearsMessageWindow)
	if (!message_window) {
		message_window = memnew(ClockGearsMessageWindow);
		message_window->connect("message_completed", callable_mp(this, &ClockGearsEventInterpreter::_on_message_window_completed));
		message_window->connect("choice_selected", callable_mp(this, &ClockGearsEventInterpreter::_on_message_window_choice_selected));
		message_window->connect("number_entered", callable_mp(this, &ClockGearsEventInterpreter::_on_message_window_number_entered));
		ui_layer->add_child(message_window);
	}

	// 4. Gestor de Imágenes de Pantalla (Anexo A1.3: hasta 100 imágenes simultáneas)
	if (!picture_manager) {
		picture_manager = memnew(ClockGearsPictureManager);
		picture_manager->set_name("ClockGearsPictureManager");
		ui_layer->add_child(picture_manager);
	}

	// 5. Reproductor de Cinemáticas (Anexo A1.2: Timeline, Letterbox, Skip consistente)
	if (!cutscene_player) {
		cutscene_player = memnew(ClockGearsCutscenePlayer);
		cutscene_player->set_name("ClockGearsCutscenePlayer");
		ui_layer->add_child(cutscene_player);
	}
}

void ClockGearsEventInterpreter::setup_interpreter(ClockGearsMap *p_map, ClockGearsPlayer *p_player, ClockGearsFollowerManager *p_followers) {
	current_map = p_map;
	player = p_player;
	follower_manager = p_followers;
	_build_runtime_ui();
	_refresh_all_map_events();
}

void ClockGearsEventInterpreter::_refresh_all_map_events() {
	if (!current_map) {
		return;
	}
	Node2D *ev_root = current_map->get_events_container();
	Node *search_root = ev_root ? static_cast<Node *>(ev_root) : static_cast<Node *>(current_map);
	TypedArray<Node> ev_nodes = search_root->find_children("*", "ClockGearsEvent", true, false);
	for (int i = 0; i < ev_nodes.size(); i++) {
		ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(ev_nodes[i]);
		if (ev) {
			ev->update_active_page();
		}
	}
}

void ClockGearsEventInterpreter::start_event(ClockGearsEvent *p_event) {
	if (!p_event) {
		return;
	}
	p_event->update_active_page();
	Array cmds = p_event->get_commands();
	if (cmds.is_empty()) {
		return;
	}
	start_command_list(cmds, p_event);
}

void ClockGearsEventInterpreter::start_command_list(const Array &p_commands, ClockGearsEvent *p_source_event) {
	if (p_commands.is_empty()) {
		return;
	}
	active_event = p_source_event;
	active_commands = p_commands.duplicate(true);
	command_index = 0;
	last_message_cmd_idx = 0;
	last_choice_index = -999;
	choice_stack.clear();
	state = STATE_RUNNING;
	key_advance_was_pressed = true; // Evitar que la misma pulsación de Enter cierre el primer mensaje
	if (active_event) {
		emit_signal("event_started", active_event);
	}
	_execute_next_commands();
}

void ClockGearsEventInterpreter::_hide_message_ui() {
	if (message_window) {
		message_window->close_window();
	}
}

void ClockGearsEventInterpreter::_on_message_window_completed() {
	if (picture_manager) {
		picture_manager->erase_pictures_on_message_close();
	}
	if (state == STATE_WAITING_MESSAGE) {
		command_index++;
		state = STATE_RUNNING;
		if (player) {
			player->lock_action_key();
		}
		_execute_next_commands();
	}
}

void ClockGearsEventInterpreter::_on_message_window_choice_selected(int p_idx) {
	if (state == STATE_WAITING_CHOICE) {
		last_choice_index = p_idx;
		choice_stack.push_back(p_idx);

		// Si el comando de choices tenía ramas inline (cmd["branches"]):
		if (command_index < active_commands.size()) {
			Variant cv = active_commands[command_index];
			if (cv.get_type() == Variant::DICTIONARY) {
				Dictionary cur_cmd = cv;
				if (cur_cmd.has("branches")) {
					Array branches = cur_cmd.get("branches", Array());
					if (p_idx >= 0 && p_idx < branches.size()) {
						Array b_cmds = branches[p_idx];
						for (int j = b_cmds.size() - 1; j >= 0; j--) {
							active_commands.insert(command_index + 1, b_cmds[j]);
						}
					}
				}
			}
		}

		command_index++;
		state = STATE_RUNNING;
		if (player) {
			player->lock_action_key();
		}
		_execute_next_commands();
	}
}

void ClockGearsEventInterpreter::_on_message_window_number_entered(int p_val) {
	if (state == STATE_WAITING_NUMBER) {
		if (command_index < active_commands.size()) {
			Variant cv = active_commands[command_index];
			if (cv.get_type() == Variant::DICTIONARY) {
				Dictionary cmd = cv;
				int var_id = int(cmd.get("variable", cmd.get("var_id", 0)));
				if (var_id > 0) {
					ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
					if (dm) {
						dm->set_variable(itos(var_id), p_val);
					}
				}
			}
		}
		command_index++;
		state = STATE_RUNNING;
		if (player) {
			player->lock_action_key();
		}
		_execute_next_commands();
	}
}

void ClockGearsEventInterpreter::_on_battle_scene_finished(int p_result) {
	if (active_battle_scene) {
		active_battle_scene->queue_free();
		active_battle_scene = nullptr;
	}

	if (p_result == 4 /* RESULT_GAME_OVER */ || (p_result == 2 /* RESULT_DEFEAT */ && !battle_can_lose_flag)) {
		state = STATE_GAME_OVER;
		print_line("[ClockGears Event] Batalla terminada en GAME OVER.");
		return;
	}

	command_index++;
	state = STATE_RUNNING;
	if (player) {
		player->lock_action_key();
	}
	_execute_next_commands();
}

void ClockGearsEventInterpreter::advance_message_or_choice(int p_choice_idx) {
	if (state == STATE_WAITING_CHOICE) {
		_on_message_window_choice_selected(p_choice_idx);
	} else if (state == STATE_WAITING_MESSAGE) {
		if (message_window) {
			message_window->advance_or_close();
		}
	} else if (state == STATE_WAITING_NUMBER) {
		_on_message_window_number_entered(p_choice_idx);
	}
}

void ClockGearsEventInterpreter::_execute_next_commands() {
	int budget = 0;
	while (state == STATE_RUNNING && command_index < active_commands.size()) {
		if (++budget > MAX_COMMANDS_PER_FRAME) {
			// Ceder el frame si un evento supera el presupuesto de 200 comandos por fotograma (Bug #18/#19)
			break;
		}
		Variant vcmd = active_commands[command_index];
		if (vcmd.get_type() != Variant::DICTIONARY) {
			command_index++;
			continue;
		}
		Dictionary cmd = vcmd;
		if (!_execute_single_command(cmd)) {
			// El comando pausó el intérprete (ej. Mostrar Mensaje, Mostrar Opciones, Esperar, Transferir)
			// Solo al llegar a CLOSED se hace command_index++ (una única vez) y sigue.
			return;
		}
		command_index++;
	}

	if (state == STATE_RUNNING && command_index >= active_commands.size()) {
		ClockGearsEvent *finished_ev = active_event;
		active_event = nullptr;
		active_commands.clear();
		command_index = 0;
		state = STATE_IDLE;
		_hide_message_ui();
		_refresh_all_map_events();
		if (player) {
			player->lock_action_key();
		}
		if (finished_ev) {
			emit_signal("event_finished", finished_ev);
		}
	}
}

bool ClockGearsEventInterpreter::_execute_single_command(const Dictionary &p_cmd) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String type = p_cmd.get("type", "");
	int code = int(p_cmd.get("code", 0));

	if (type == "message" || code == CG_CMD_SHOW_TEXT) {
		last_message_cmd_idx = command_index - 1;
		String raw_txt = p_cmd.get("text", "");
		String speaker = p_cmd.get("speaker", "");
		String face = p_cmd.get("face", "");
		int pos = int(p_cmd.get("position", p_cmd.get("pos", 0)));
		int bg = int(p_cmd.get("frame", p_cmd.get("bg", 0)));
		_build_runtime_ui();
		if (message_window) {
			message_window->show_message(raw_txt, speaker, face, pos, bg);
		}
		emit_signal("message_shown", raw_txt);
		state = STATE_WAITING_MESSAGE;
		return false;
	}

	if (type == "choices" || code == CG_CMD_SHOW_CHOICES) {
		PackedStringArray clean_opts;
		if (p_cmd.has("c1")) {
			for (int i = 1; i <= 6; i++) {
				String key = "c" + itos(i);
				if (p_cmd.has(key) && !String(p_cmd.get(key, "")).is_empty()) {
					clean_opts.push_back(p_cmd.get(key, ""));
				}
			}
		} else {
			String opts_str = p_cmd.get("options", CG_RT_TR("Sí, No"));
			PackedStringArray parts = opts_str.split(",");
			for (int i = 0; i < parts.size(); i++) {
				String s = parts[i].strip_edges();
				if (!s.is_empty()) {
					clean_opts.push_back(s);
				}
			}
		}
		if (clean_opts.is_empty()) {
			clean_opts.push_back(CG_RT_TR("Sí"));
			clean_opts.push_back(CG_RT_TR("No"));
		}
		int cancel_opt = int(p_cmd.get("cancel_opt", p_cmd.get("cancel", 0)));
		_build_runtime_ui();
		if (message_window) {
			message_window->show_choices(clean_opts, cancel_opt, 0);
		}
		state = STATE_WAITING_CHOICE;
		return false;
	}

	if (type == "number_input" || code == CG_CMD_INPUT_NUMBER) {
		int var_id = int(p_cmd.get("variable", p_cmd.get("var_id", 1)));
		int digits = int(p_cmd.get("digits", 3));
		_build_runtime_ui();
		if (message_window) {
			message_window->show_number_input(var_id, digits);
		}
		state = STATE_WAITING_NUMBER;
		return false;
	}

	if (type == "when_choice" || code == CG_CMD_WHEN_CHOICE) {
		int target_choice = int(p_cmd.get("choice", p_cmd.get("index", 0)));
		if (last_choice_index == target_choice) {
			// Es la rama elegida por el jugador: ejecutar su contenido
			return true;
		}

		// Rama distinta o ya ejecutada: saltar comandos hasta la siguiente rama o BRANCH_END_CHOICE
		int depth = 0;
		int jump_to = -1;
		for (int i = command_index + 1; i < active_commands.size(); i++) {
			Variant vc = active_commands[i];
			if (vc.get_type() != Variant::DICTIONARY) continue;
			Dictionary c = vc;
			int c_code = int(c.get("code", 0));
			String c_type = c.get("type", "");

			if (c_type == "choices" || c_code == CG_CMD_SHOW_CHOICES) {
				depth++;
			} else if (depth > 0 && (c_type == "branch_end_choice" || c_code == CG_CMD_BRANCH_END_CHOICE)) {
				depth--;
			} else if (depth == 0) {
				if (c_type == "when_choice" || c_code == CG_CMD_WHEN_CHOICE) {
					int next_choice = int(c.get("choice", c.get("index", 0)));
					if (next_choice == last_choice_index) {
						jump_to = i;
						break;
					}
				} else if (c_type == "when_cancel" || c_code == CG_CMD_WHEN_CANCEL) {
					if (last_choice_index == -1) {
						jump_to = i;
						break;
					}
				} else if (c_type == "branch_end_choice" || c_code == CG_CMD_BRANCH_END_CHOICE) {
					jump_to = i;
					break;
				}
			}
		}

		if (jump_to != -1) {
			command_index = jump_to - 1;
		} else {
			command_index = active_commands.size();
		}
		return true;
	}

	if (type == "when_cancel" || code == CG_CMD_WHEN_CANCEL) {
		if (last_choice_index == -1) {
			return true;
		}
		// Rama cancelada no activa: saltar hasta BRANCH_END_CHOICE
		int depth = 0;
		int jump_to = -1;
		for (int i = command_index + 1; i < active_commands.size(); i++) {
			Variant vc = active_commands[i];
			if (vc.get_type() != Variant::DICTIONARY) continue;
			Dictionary c = vc;
			int c_code = int(c.get("code", 0));
			String c_type = c.get("type", "");
			if (c_type == "choices" || c_code == CG_CMD_SHOW_CHOICES) {
				depth++;
			} else if (depth > 0 && (c_type == "branch_end_choice" || c_code == CG_CMD_BRANCH_END_CHOICE)) {
				depth--;
			} else if (depth == 0 && (c_type == "branch_end_choice" || c_code == CG_CMD_BRANCH_END_CHOICE)) {
				jump_to = i;
				break;
			}
		}
		if (jump_to != -1) {
			command_index = jump_to - 1;
		} else {
			command_index = active_commands.size();
		}
		return true;
	}

	if (type == "branch_end_choice" || code == CG_CMD_BRANCH_END_CHOICE) {
		if (!choice_stack.is_empty()) {
			choice_stack.remove_at(choice_stack.size() - 1);
			last_choice_index = choice_stack.is_empty() ? -999 : choice_stack[choice_stack.size() - 1];
		} else {
			last_choice_index = -999;
		}
		return true;
	}

	if (type == "wait" || code == CG_CMD_WAIT) {
		wait_frames_remaining = MAX(1, int(p_cmd.get("frames", 60)));
		state = STATE_WAITING_FRAMES;
		return false;
	}

	if (type == "switch" || code == CG_CMD_CONTROL_SWITCHES) {
		String sw_name = p_cmd.get("name", "");
		bool val = bool(p_cmd.get("val", true));
		if (dm && !sw_name.is_empty()) {
			dm->set_switch(sw_name, val);
		}
		_refresh_all_map_events();
		return true;
	}

	if (type == "self_switch" || code == CG_CMD_CONTROL_SELF_SWITCH) {
		String letter = p_cmd.get("letter", "A");
		bool val = bool(p_cmd.get("val", true));
		if (active_event) {
			active_event->set_self_switch(letter, val);
		}
		return true;
	}

	if (type == "variable" || code == CG_CMD_CONTROL_VARIABLES) {
		String var_name = p_cmd.get("name", "");
		if (var_name.is_empty() && p_cmd.has("var_id")) {
			var_name = itos(int(p_cmd["var_id"]));
		}
		String op = p_cmd.get("op", "=");
		int val = int(p_cmd.get("val", p_cmd.get("value", 0)));
		if (dm && !var_name.is_empty()) {
			int cur = dm->get_variable(var_name);
			if (op == "+") cur += val;
			else if (op == "-") cur -= val;
			else if (op == "*") cur *= val;
			else if (op == "/" && val != 0) cur /= val;
			else if (op == "%" && val != 0) cur %= val;
			else cur = val;
			dm->set_variable(var_name, cur);
		}
		_refresh_all_map_events();
		return true;
	}

	if (type == "item" || code == CG_CMD_CHANGE_ITEMS) {
		String iname = p_cmd.get("name", "");
		String op = p_cmd.get("op", "+");
		int qty = int(p_cmd.get("qty", 1));
		if (dm && !iname.is_empty()) {
			dm->change_item(iname, (op == "-") ? -qty : qty);
		}
		_refresh_all_map_events();
		return true;
	}

	if (type == "gold" || code == CG_CMD_CHANGE_GOLD) {
		String op = p_cmd.get("op", "+");
		int amt = int(p_cmd.get("amount", 0));
		if (dm) {
			dm->change_gold((op == "-") ? -amt : amt);
		}
		return true;
	}

	if (type == "teleport" || code == CG_CMD_TRANSFER_PLAYER) {
		String dest_map = p_cmd.get("map", "");
		int tx = int(p_cmd.get("x", 0));
		int ty = int(p_cmd.get("y", 0));
		String dir_str = p_cmd.get("dir", "");
		int tdir = player ? player->get_direction() : 2;
		if (dir_str.begins_with("Abajo")) tdir = 2;
		else if (dir_str.begins_with("Izquierda")) tdir = 4;
		else if (dir_str.begins_with("Derecha")) tdir = 6;
		else if (dir_str.begins_with("Arriba")) tdir = 8;
		_perform_map_transfer(dest_map, tx, ty, tdir);
		return true;
	}

	if (type == "move_route" || code == CG_CMD_SET_MOVEMENT_ROUTE) {
		String target = p_cmd.get("target", "");
		String action = p_cmd.get("action", "");
		if (target.begins_with("Jugador") && player) {
			if (action.find("Arriba") >= 0) player->try_step(8);
			else if (action.find("Abajo") >= 0) player->try_step(2);
			else if (action.find("Izquierda") >= 0) player->try_step(4);
			else if (action.find("Derecha") >= 0) player->try_step(6);
		}
		return true;
	}

	if (code == CG_CMD_GATHER_FOLLOWERS) {
		if (follower_manager && player) {
			follower_manager->gather_followers(player->get_grid_cell(), player->get_direction());
		}
		return true;
	}

	if (code == CG_CMD_CHANGE_FOLLOWERS) {
		if (follower_manager) {
			follower_manager->set_followers_visible_flag(bool(p_cmd.get("visible", true)));
		}
		return true;
	}

	if (type == "fade" || code == CG_CMD_FADEOUT_SCREEN || code == CG_CMD_FADEIN_SCREEN) {
		String mode = p_cmd.get("mode", (code == CG_CMD_FADEIN_SCREEN) ? "Mostrar" : "Oscurecer");
		int dur_frames = MAX(1, int(p_cmd.get("duration", 30)));
		fade_duration = float(dur_frames) / 60.0f;
		fade_timer = 0.0f;
		if (screen_fade_rect) {
			fade_start_color = screen_fade_rect->get_color();
			fade_target_color = mode.begins_with("Oscurecer") ? Color(0, 0, 0, 1) : Color(0, 0, 0, 0);
		}
		return true;
	}

	if (type == "flash" || code == CG_CMD_FLASH_SCREEN) {
		int dur_frames = MAX(1, int(p_cmd.get("duration", 30)));
		flash_duration = float(dur_frames) / 60.0f;
		flash_timer = flash_duration;
		flash_base_color = Color(1, 1, 1, 0.85f);
		if (screen_flash_rect) {
			screen_flash_rect->set_color(flash_base_color);
		}
		return true;
	}

	if (type == "tint" || code == CG_CMD_TINT_SCREEN) {
		_build_runtime_ui();
		String preset = p_cmd.get("preset", "Normal");
		int dur_frames = MAX(1, int(p_cmd.get("duration", p_cmd.get("frames", 30))));
		tint_duration = float(dur_frames) / 60.0f;
		tint_timer = 0.0f;
		if (screen_tint_rect) {
			tint_start_color = current_tint_color;
			if (preset.find("Noche") >= 0 || preset.find("Oscuro") >= 0 || preset == "Night") {
				// Tono azul oscuro nocturno intenso (A1.3): R=0.04, G=0.08, B=0.35, Alpha=0.65
				tint_target_color = Color(0.04f, 0.08f, 0.35f, 0.65f);
			} else if (preset.find("Atardecer") >= 0 || preset == "Sunset") {
				tint_target_color = Color(0.38f, 0.16f, 0.05f, 0.45f);
			} else if (preset.find("Sepia") >= 0) {
				tint_target_color = Color(0.32f, 0.22f, 0.10f, 0.40f);
			} else if (p_cmd.has("color")) {
				tint_target_color = p_cmd.get("color", Color(0, 0, 0, 0));
			} else {
				// Normal / Sin tinte
				tint_target_color = Color(0, 0, 0, 0);
			}
			print_line(vformat("[ClockGears Tint] Tinte '%s' aplicado: %s en nodo '%s' (capa %d)", preset, tint_target_color.to_html(), screen_tint_rect->get_name(), screen_tint_layer ? screen_tint_layer->get_layer() : 0));
		}
		return true;
	}

	// --- Imágenes (Anexo A1.3) ---
	if (type == "picture" || type == "show_picture" || code == CG_CMD_SHOW_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			int origin = int(p_cmd.get("origin", 0));
			Vector2 pos = p_cmd.get("pos", Vector2(float(p_cmd.get("x", 0)), float(p_cmd.get("y", 0))));
			Vector2 scale = p_cmd.get("scale", Vector2(float(p_cmd.get("scale_x", 100)), float(p_cmd.get("scale_y", 100))));
			float opacity = float(p_cmd.get("opacity", 1.0f));
			int blend = int(p_cmd.get("blend", 0));
			int dur_mode = int(p_cmd.get("duration_mode", 0));
			float dur_sec = float(p_cmd.get("duration_sec", p_cmd.get("duration", 0.0f)));
			bool auto_fade = bool(p_cmd.get("auto_fadeout", false));
			bool wait = bool(p_cmd.get("wait", false));

			picture_manager->show_picture(id, path, origin, pos, scale, opacity, blend, dur_mode, dur_sec, auto_fade);

			if (wait && dur_mode == 1 && dur_sec > 0.0f) {
				wait_frames_remaining = int(dur_sec * 60.0f);
				state = STATE_WAITING_FRAMES;
				return false;
			}
		}
		return true;
	}

	if (type == "move_picture" || code == CG_CMD_MOVE_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			int origin = int(p_cmd.get("origin", 0));
			Vector2 pos = p_cmd.get("pos", Vector2(float(p_cmd.get("x", 0)), float(p_cmd.get("y", 0))));
			Vector2 scale = p_cmd.get("scale", Vector2(float(p_cmd.get("scale_x", 100)), float(p_cmd.get("scale_y", 100))));
			float opacity = float(p_cmd.get("opacity", 1.0f));
			int dur_frames = int(p_cmd.get("duration", p_cmd.get("frames", 60)));
			float dur_sec = float(dur_frames) / 60.0f;
			int easing = int(p_cmd.get("easing", 0));
			bool wait = bool(p_cmd.get("wait", false));
			picture_manager->move_picture(id, origin, pos, scale, opacity, dur_sec, easing);
			if (wait && dur_frames > 0) {
				wait_frames_remaining = dur_frames;
				state = STATE_WAITING_FRAMES;
				return false;
			}
		}
		return true;
	}

	if (type == "rotate_picture" || code == CG_CMD_ROTATE_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			float speed = float(p_cmd.get("speed", 0.0f));
			picture_manager->rotate_picture(id, speed);
		}
		return true;
	}

	if (type == "tint_picture" || code == CG_CMD_TINT_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			Color col = p_cmd.get("color", Color(1, 1, 1, 1));
			int dur_frames = int(p_cmd.get("duration", p_cmd.get("frames", 60)));
			float dur_sec = float(dur_frames) / 60.0f;
			bool wait = bool(p_cmd.get("wait", false));
			picture_manager->tint_picture(id, col, dur_sec);
			if (wait && dur_frames > 0) {
				wait_frames_remaining = dur_frames;
				state = STATE_WAITING_FRAMES;
				return false;
			}
		}
		return true;
	}

	if (type == "erase_picture" || code == CG_CMD_ERASE_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			picture_manager->erase_picture(id);
		}
		return true;
	}

	// --- Audio (Anexo A1.4) ---
	if (type == "play_bgm" || type == "bgm" || code == CG_CMD_PLAY_BGM) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			float vol = float(p_cmd.get("volume", p_cmd.get("vol", 1.0f)));
			float pitch = float(p_cmd.get("pitch", 1.0f));
			float crossfade = float(p_cmd.get("crossfade", 0.5f));
			float pos = float(p_cmd.get("pos", 0.0f));
			am->play_bgm(path, vol, pitch, crossfade, pos);
		}
		return true;
	}

	if (type == "fadeout_bgm" || type == "stop_fade_bgm" || code == CG_CMD_FADEOUT_BGM) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String action = p_cmd.get("action", "fade");
			if (action == "stop" || action.find("Parar") >= 0) {
				am->stop_bgm(0.0f);
			} else {
				float fade = float(p_cmd.get("duration", p_cmd.get("fade", 1.0f)));
				am->stop_bgm(fade);
			}
		}
		return true;
	}

	if (type == "save_bgm" || code == CG_CMD_SAVE_BGM) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			am->save_bgm();
		}
		return true;
	}

	if (type == "resume_bgm" || type == "restore_bgm" || code == CG_CMD_RESUME_BGM) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			float fade = float(p_cmd.get("crossfade", p_cmd.get("fade", 0.5f)));
			am->restore_bgm(fade);
		}
		return true;
	}

	if (type == "play_bgs" || type == "bgs" || code == CG_CMD_PLAY_BGS) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			float vol = float(p_cmd.get("volume", 1.0f));
			float pitch = float(p_cmd.get("pitch", 1.0f));
			float fade = float(p_cmd.get("fade", 0.0f));
			am->play_bgs(path, vol, pitch, fade);
		}
		return true;
	}

	if (type == "fadeout_bgs" || code == CG_CMD_FADEOUT_BGS) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			float fade = float(p_cmd.get("duration", 1.0f));
			am->stop_bgs(fade);
		}
		return true;
	}

	if (type == "play_me" || type == "me" || code == CG_CMD_PLAY_ME) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			float vol = float(p_cmd.get("volume", 1.0f));
			float pitch = float(p_cmd.get("pitch", 1.0f));
			am->play_me(path, vol, pitch);
		}
		return true;
	}

	if (type == "play_se" || type == "se" || code == CG_CMD_PLAY_SE) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			float vol = float(p_cmd.get("volume", 1.0f));
			float pitch = float(p_cmd.get("pitch", 1.0f));
			am->play_se(path, vol, pitch);
		}
		return true;
	}

	if (type == "stop_se" || code == CG_CMD_STOP_SE) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			am->stop_all_se();
		}
		return true;
	}

	if (type == "audio_stems" || type == "stems" || code == CG_CMD_AUDIO_STEMS) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			int stem_idx = int(p_cmd.get("stem", 0));
			float vol = float(p_cmd.get("volume", 1.0f));
			float fade = float(p_cmd.get("fade", 0.3f));
			if (p_cmd.has("path") && !String(p_cmd.get("path", "")).is_empty()) {
				am->play_stem(stem_idx, p_cmd.get("path", ""), vol);
			} else {
				am->set_stem_volume(stem_idx, vol, fade);
			}
		}
		return true;
	}

	// --- Efectos de pantalla (Shake & Weather) ---
	if (type == "shake" || type == "shake_screen" || code == CG_CMD_SHAKE_SCREEN) {
		_build_runtime_ui();
		float power = float(p_cmd.get("power", 5.0f));
		float speed = float(p_cmd.get("speed", 5.0f));
		int dur_frames = MAX(1, int(p_cmd.get("duration", p_cmd.get("frames", 60))));
		float dur_sec = float(dur_frames) / 60.0f;
		bool wait = bool(p_cmd.get("wait", false));

		start_screen_shake(power, speed, dur_sec, wait);
		if (wait && dur_frames > 0) {
			return false;
		}
		return true;
	}

	if (type == "change_camera_zoom" || code == CG_CMD_CHANGE_CAMERA_ZOOM) {
		float target_zoom = float(p_cmd.get("zoom", 2.0f));
		if (target_zoom <= 0.05f) {
			target_zoom = 2.0f;
		}
		int dur_frames = int(p_cmd.get("duration", 0));
		bool wait = bool(p_cmd.get("wait", false));

		start_camera_zoom_transition(target_zoom, dur_frames);
		if (wait && dur_frames > 0) {
			wait_frames_remaining = dur_frames;
			state = STATE_WAITING_FRAMES;
			return false;
		}
		return true;
	}

	if (type == "weather" || code == CG_CMD_SET_WEATHER) {
		_build_runtime_ui();
		if (picture_manager) {
			String wtype = p_cmd.get("weather", p_cmd.get("type_name", "none"));
			float power = float(p_cmd.get("power", 5.0f));
			picture_manager->set_weather(wtype, power);
		}
		return true;
	}

	// --- Cinemáticas (Anexo A1.2) ---
	if (type == "cutscene" || type == "play_cutscene") {
		_build_runtime_ui();
		if (cutscene_player) {
			Ref<ClockGearsCutscene> cs;
			if (p_cmd.has("cutscene_dict")) {
				cs.instantiate();
				cs->from_dict(p_cmd.get("cutscene_dict", Dictionary()));
			} else if (p_cmd.has("path")) {
				String path = p_cmd.get("path", "");
				if (ResourceLoader::exists(path)) {
					cs = ResourceLoader::load(path);
				}
			}
			if (cs.is_valid()) {
				cutscene_player->play(cs);
			}
		}
		return true;
	}

	// --- Batalla por turnos (Fase 8A) ---
	if (type == "battle" || code == CMD_BOSS_BATTLE) {
		_build_runtime_ui();
		Variant tr_var = p_cmd.get("troop_id", p_cmd.get("troop", 1));
		int troop = 1;
		if (tr_var.get_type() == Variant::INT || tr_var.get_type() == Variant::FLOAT) {
			troop = int(tr_var);
		} else {
			String tr_str = tr_var;
			int idx_colon = tr_str.find(":");
			if (idx_colon != -1) {
				troop = tr_str.substr(0, idx_colon).to_int();
			} else {
				troop = tr_str.to_int();
			}
		}
		if (troop <= 0) troop = 1;
		bool can_escape = bool(p_cmd.get("can_escape", true));
		battle_can_lose_flag = bool(p_cmd.get("can_lose", false));
		String bgm = p_cmd.get("bgm", "");

		ClockGearsBattleScene *bs = memnew(ClockGearsBattleScene);
		active_battle_scene = bs;
		bs->setup_battle(troop, can_escape, battle_can_lose_flag, bgm, message_window);
		bs->connect("battle_finished", callable_mp(this, &ClockGearsEventInterpreter::_on_battle_scene_finished));
		add_child(bs);

		state = STATE_WAITING_BATTLE;
		return false;
	}

	return true;
}

void ClockGearsEventInterpreter::_perform_map_transfer(const String &p_map_path, int p_x, int p_y, int p_dir) {
	Vector2i target_cell(MAX(0, p_x), MAX(0, p_y));
	if (player) {
		player->set_grid_cell_immediate(target_cell, p_dir);
	}
	if (follower_manager) {
		follower_manager->gather_followers(target_cell, p_dir);
	}

	if (!p_map_path.is_empty() && current_map && is_inside_tree()) {
		String path = p_map_path;
		if (!path.begins_with("res://")) {
			if (!path.ends_with(".tscn")) {
				path += ".tscn";
			}
			path = "res://maps/" + path.get_file();
		}
		if (ResourceLoader::exists(path) && path != current_map->get_scene_file_path()) {
			ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
			if (dm) {
				Dictionary sys = dm->get_system_config();
				sys["runtime_transfer_x"] = target_cell.x;
				sys["runtime_transfer_y"] = target_cell.y;
				sys["runtime_transfer_dir"] = p_dir;
				sys["runtime_has_transfer"] = true;
				dm->set_system_config(sys);
			}
			get_tree()->change_scene_to_file(path);
		}
	}
}

void ClockGearsEventInterpreter::start_screen_shake(float p_power, float p_speed, float p_duration, bool p_wait) {
	shake_power = MAX(0.0f, p_power);
	shake_speed = MAX(0.1f, p_speed);
	shake_duration = MAX(0.01f, p_duration);
	shake_timer = shake_duration;
	current_shake_offset = Vector2();

	String target_name = "Ninguno";
	if (current_map && current_map->get_runtime_camera()) {
		target_name = "Cámara (Offset)";
	} else if (current_map) {
		target_name = "Mapa (Posición)";
	}

	print_line(vformat("[ClockGears Shake] Temblor iniciado: potencia=%.1f, vel=%.1f, dur=%.2fs aplicado a '%s'", shake_power, shake_speed, shake_duration, target_name));

	if (picture_manager) {
		picture_manager->start_screen_shake(shake_power, shake_speed, shake_duration);
	}

	if (p_wait && shake_duration > 0.0f) {
		wait_frames_remaining = int(shake_duration * 60.0f);
		state = STATE_WAITING_FRAMES;
	}
}

void ClockGearsEventInterpreter::stop_screen_shake() {
	shake_timer = 0.0f;
	shake_duration = 0.0f;
	current_shake_offset = Vector2();
	if (current_map) {
		ClockGearsPixelCamera2D *cam = current_map->get_runtime_camera();
		if (cam) {
			cam->set_offset(Vector2(0, 0));
		} else {
			current_map->set_position(Vector2(0, 0));
		}
	}
	if (picture_manager) {
		picture_manager->stop_screen_shake();
	}
}

void ClockGearsEventInterpreter::start_camera_zoom_transition(float p_target_zoom, int p_duration_frames) {
	cam_zoom_target = CLAMP(p_target_zoom, 0.25f, 4.0f);
	cam_zoom_start = 2.0f;
	if (current_map && current_map->get_runtime_camera()) {
		cam_zoom_start = current_map->get_runtime_camera()->get_zoom().x;
	}
	if (p_duration_frames <= 0) {
		cam_zoom_frames_left = 0;
		cam_zoom_frames_total = 0;
		if (current_map && current_map->get_runtime_camera()) {
			bool int_mode = false;
			if (ClockGearsDataManager::get_singleton()) {
				int_mode = bool(ClockGearsDataManager::get_singleton()->get_system_config().get("camera_integer_zoom", false));
			}
			if (int_mode) {
				current_map->get_runtime_camera()->set_allow_fractional_zoom(false);
				current_map->get_runtime_camera()->set_integer_zoom(MAX(1, int(Math::round(cam_zoom_target))));
			} else {
				current_map->get_runtime_camera()->set_allow_fractional_zoom(true);
				current_map->get_runtime_camera()->set_zoom(Vector2(cam_zoom_target, cam_zoom_target));
			}
		}
	} else {
		cam_zoom_frames_total = p_duration_frames;
		cam_zoom_frames_left = p_duration_frames;
		if (current_map && current_map->get_runtime_camera()) {
			current_map->get_runtime_camera()->set_allow_fractional_zoom(true);
		}
	}
}

void ClockGearsEventInterpreter::step_frame() {
	float dt = 1.0f / 60.0f;
	if (is_inside_tree()) {
		dt = get_physics_process_delta_time();
	}

	// Transición suave de zoom de cámara
	if (cam_zoom_frames_left > 0) {
		cam_zoom_frames_left--;
		float t = 1.0f - (float)cam_zoom_frames_left / (float)cam_zoom_frames_total;
		if (cam_zoom_frames_left <= 0) {
			if (current_map && current_map->get_runtime_camera()) {
				bool int_mode = false;
				if (ClockGearsDataManager::get_singleton()) {
					int_mode = bool(ClockGearsDataManager::get_singleton()->get_system_config().get("camera_integer_zoom", false));
				}
				if (int_mode) {
					current_map->get_runtime_camera()->set_allow_fractional_zoom(false);
					current_map->get_runtime_camera()->set_integer_zoom(MAX(1, int(Math::round(cam_zoom_target))));
				} else {
					current_map->get_runtime_camera()->set_allow_fractional_zoom(true);
					current_map->get_runtime_camera()->set_zoom(Vector2(cam_zoom_target, cam_zoom_target));
				}
			}
		} else {
			if (current_map && current_map->get_runtime_camera()) {
				current_map->get_runtime_camera()->set_allow_fractional_zoom(true);
				float cur_z = Math::lerp(cam_zoom_start, cam_zoom_target, t);
				current_map->get_runtime_camera()->set_zoom(Vector2(cur_z, cur_z));
			}
		}
	}

	// Sincronizar tamaño de capas de pantalla con el Viewport
	if (is_inside_tree() && get_viewport()) {
		Size2 vsz = get_viewport()->get_visible_rect().size;
		if (vsz.x > 0 && vsz.y > 0) {
			if (screen_tint_rect && screen_tint_rect->get_size() != vsz) screen_tint_rect->set_size(vsz);
			if (screen_fade_rect && screen_fade_rect->get_size() != vsz) screen_fade_rect->set_size(vsz);
			if (screen_flash_rect && screen_flash_rect->get_size() != vsz) screen_flash_rect->set_size(vsz);
		}
	}

	// Actualizar efectos de pantalla (Flash, Fade, Tint y Shake)
	if (shake_timer > 0.0f) {
		shake_timer -= dt;
		float progress = CLAMP(shake_timer / shake_duration, 0.0f, 1.0f);
		float amp = shake_power * 2.5f * progress;
		Vector2 offset = Vector2(0, 0);
		if (amp >= 0.5f) {
			float sx = Math::sin(shake_timer * shake_speed * 12.0f) * amp;
			float sy = Math::cos(shake_timer * shake_speed * 15.0f) * (amp * 0.7f);
			offset = Vector2(Math::round(sx), Math::round(sy));
		}
		current_shake_offset = offset;

		if (current_map) {
			ClockGearsPixelCamera2D *cam = current_map->get_runtime_camera();
			if (cam) {
				cam->set_offset(offset);
			} else {
				current_map->set_position(offset);
			}
		}

		if (shake_timer <= 0.0f) {
			stop_screen_shake();
		}
	}
	if (tint_duration > 0.0f && screen_tint_rect) {
		tint_timer += dt;
		float t = CLAMP(tint_timer / tint_duration, 0.0f, 1.0f);
		current_tint_color = tint_start_color.lerp(tint_target_color, t);
		screen_tint_rect->set_color(current_tint_color);
		if (t >= 1.0f) {
			tint_duration = 0.0f; // Completado: se mantiene en tint_target_color permanentemente
			screen_tint_rect->set_color(tint_target_color);
		}
	}

	if (flash_timer > 0.0f && screen_flash_rect) {
		flash_timer = MAX(0.0f, flash_timer - dt);
		float ratio = (flash_duration > 0.001f) ? (flash_timer / flash_duration) : 0.0f;
		Color c = flash_base_color;
		c.a = flash_base_color.a * ratio;
		screen_flash_rect->set_color(c);
	}
	if (fade_duration > 0.0f && screen_fade_rect) {
		fade_timer += dt;
		float t = CLAMP(fade_timer / fade_duration, 0.0f, 1.0f);
		screen_fade_rect->set_color(fade_start_color.lerp(fade_target_color, t));
		if (t >= 1.0f) {
			fade_duration = 0.0f;
		}
	}

	if (state == STATE_WAITING_FRAMES) {
		wait_frames_remaining--;
		if (wait_frames_remaining <= 0) {
			command_index++;
			state = STATE_RUNNING;
			_execute_next_commands();
		}
		return;
	}

	if (state == STATE_WAITING_MESSAGE || state == STATE_WAITING_CHOICE || state == STATE_WAITING_NUMBER || state == STATE_WAITING_BATTLE || state == STATE_GAME_OVER) {
		return;
	}

	if (state == STATE_RUNNING) {
		_execute_next_commands();
		return;
	}

	// Si está IDLE, evaluar eventos con disparador TRIGGER_AUTORUN en el mapa activo
	if (state == STATE_IDLE && current_map) {
		Vector<ClockGearsEvent *> ev_nodes = _cg_get_map_events(current_map);
		for (int i = 0; i < ev_nodes.size(); i++) {
			ClockGearsEvent *ev = ev_nodes[i];
			if (ev && ev->is_visible() && ev->get_trigger_type() == TRIGGER_AUTORUN && !ev->get_commands().is_empty()) {
				start_event(ev);
				break;
			}
		}
	}
}

void ClockGearsEventInterpreter::_notification(int p_what) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (p_what == NOTIFICATION_READY) {
		set_physics_process(true);
	} else if (p_what == NOTIFICATION_PHYSICS_PROCESS) {
		step_frame();
	}
}

ClockGearsEventInterpreter::ClockGearsEventInterpreter() {
}
