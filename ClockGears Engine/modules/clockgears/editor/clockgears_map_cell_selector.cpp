#include "clockgears_map_cell_selector.h"
#include "clockgears_event.h"
#include "clockgears_map.h"
#include "clockgears_types.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/separator.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/style_box_flat.h"

void ClockGearsMapCellSelector::_bind_methods() {
	ClassDB::bind_method(D_METHOD("populate_maps"), &ClockGearsMapCellSelector::populate_maps);
	ClassDB::bind_method(D_METHOD("set_target_map", "map_path"), &ClockGearsMapCellSelector::set_target_map);
	ClassDB::bind_method(D_METHOD("get_target_map"), &ClockGearsMapCellSelector::get_target_map);
	ClassDB::bind_method(D_METHOD("set_target_coords", "x", "y"), &ClockGearsMapCellSelector::set_target_coords);
	ClassDB::bind_method(D_METHOD("get_target_coords"), &ClockGearsMapCellSelector::get_target_coords);
	ClassDB::bind_method(D_METHOD("set_target_direction", "dir"), &ClockGearsMapCellSelector::set_target_direction);
	ClassDB::bind_method(D_METHOD("get_target_direction"), &ClockGearsMapCellSelector::get_target_direction);
	ClassDB::bind_method(D_METHOD("select_cell_at_pixel", "pixel_pos"), &ClockGearsMapCellSelector::select_cell_at_pixel);
	ClassDB::bind_method(D_METHOD("load_map_preview", "map_path"), &ClockGearsMapCellSelector::load_map_preview);
	ClassDB::bind_method(D_METHOD("get_map_option_button"), &ClockGearsMapCellSelector::get_map_option_button);
	ClassDB::bind_method(D_METHOD("get_dir_option_button"), &ClockGearsMapCellSelector::get_dir_option_button);
	ClassDB::bind_method(D_METHOD("get_x_spinbox"), &ClockGearsMapCellSelector::get_x_spinbox);
	ClassDB::bind_method(D_METHOD("get_y_spinbox"), &ClockGearsMapCellSelector::get_y_spinbox);
	ClassDB::bind_method(D_METHOD("get_scroll_container"), &ClockGearsMapCellSelector::get_scroll_container);
	ClassDB::bind_method(D_METHOD("get_overlay_control"), &ClockGearsMapCellSelector::get_overlay_control);

	ClassDB::bind_static_method("ClockGearsMapCellSelector", D_METHOD("create_quick_door_event", "scene", "cell", "map", "x", "y", "dir", "sprite_path", "hframes"), &ClockGearsMapCellSelector::create_quick_door_event);

	ADD_SIGNAL(MethodInfo("target_changed", PropertyInfo(Variant::STRING, "map_path"), PropertyInfo(Variant::VECTOR2I, "coords"), PropertyInfo(Variant::STRING, "direction")));
}

ClockGearsMapCellSelector::ClockGearsMapCellSelector() {
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", 6);

	// 1. Barra Superior: Mapa y Dirección
	HBoxContainer *top_hb = memnew(HBoxContainer);
	top_hb->add_theme_constant_override("separation", 8);
	add_child(top_hb);

	top_hb->add_child(memnew(Label(CG_TR("Mapa:"))));
	map_opt = memnew(OptionButton);
	map_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	map_opt->connect("item_selected", callable_mp(this, &ClockGearsMapCellSelector::_on_map_selected));
	top_hb->add_child(map_opt);

	top_hb->add_child(memnew(VSeparator));

	top_hb->add_child(memnew(Label(CG_TR("Dirección al llegar:"))));
	dir_opt = memnew(OptionButton);
	dir_opt->add_item(CG_TR("Mantener"), 0);
	dir_opt->add_item(CG_TR("Arriba"), 1);
	dir_opt->add_item(CG_TR("Abajo"), 2);
	dir_opt->add_item(CG_TR("Izquierda"), 3);
	dir_opt->add_item(CG_TR("Derecha"), 4);
	dir_opt->select(2); // Abajo por defecto
	dir_opt->connect("item_selected", callable_mp(this, &ClockGearsMapCellSelector::_on_dir_selected));
	top_hb->add_child(dir_opt);

	// 2. Barra de Coordenadas y Destino
	HBoxContainer *coord_hb = memnew(HBoxContainer);
	coord_hb->add_theme_constant_override("separation", 8);
	add_child(coord_hb);

	coords_label = memnew(Label(CG_TR("Destino seleccionado: Casilla (0, 0)")));
	coords_label->add_theme_color_override("font_color", Color(0.95, 0.85, 0.35));
	coords_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	coord_hb->add_child(coords_label);

	coord_hb->add_child(memnew(Label("X:")));
	x_spin = memnew(SpinBox);
	x_spin->set_min(0);
	x_spin->set_max(999);
	x_spin->set_value(0);
	x_spin->connect("value_changed", callable_mp(this, &ClockGearsMapCellSelector::_on_spin_changed));
	coord_hb->add_child(x_spin);

	coord_hb->add_child(memnew(Label("Y:")));
	y_spin = memnew(SpinBox);
	y_spin->set_min(0);
	y_spin->set_max(999);
	y_spin->set_value(0);
	y_spin->connect("value_changed", callable_mp(this, &ClockGearsMapCellSelector::_on_spin_changed));
	coord_hb->add_child(y_spin);

	map_path_edit = memnew(LineEdit);
	map_path_edit->set_visible(false);
	add_child(map_path_edit);

	// 3. ScrollContainer con la vista previa completa del mapa (rueda del raton para desplazamiento, sin zoom)
	scroll_container = memnew(ScrollContainer);
	scroll_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->set_custom_minimum_size(Size2(480, 280) * EDSCALE);
	scroll_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	scroll_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
	add_child(scroll_container);

	canvas_wrapper = memnew(Control);
	canvas_wrapper->set_custom_minimum_size(Size2(map_tiles_w * 32, map_tiles_h * 32));
	scroll_container->add_child(canvas_wrapper);

	subvp_container = memnew(SubViewportContainer);
	subvp_container->set_stretch(false);
	subvp_container->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	subvp_container->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	canvas_wrapper->add_child(subvp_container);

	subvp = memnew(SubViewport);
	subvp->set_size(Vector2i(map_tiles_w * 32, map_tiles_h * 32));
	subvp->set_update_mode(SubViewport::UPDATE_WHEN_VISIBLE);
	subvp->set_disable_3d(true);
	subvp_container->add_child(subvp);

	overlay_control = memnew(Control);
	overlay_control->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	overlay_control->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	overlay_control->connect("draw", callable_mp(this, &ClockGearsMapCellSelector::_on_overlay_draw));
	overlay_control->connect("gui_input", callable_mp(this, &ClockGearsMapCellSelector::_on_overlay_gui_input));
	canvas_wrapper->add_child(overlay_control);

	_populate_maps_internal(false);
}

void ClockGearsMapCellSelector::_notification(int p_what) {
	if (p_what == NOTIFICATION_VISIBILITY_CHANGED) {
		if (is_visible_in_tree() && !loaded_map_instance) {
			String target = get_target_map();
			if (!target.is_empty()) {
				load_map_preview(target);
			}
		}
	}
}

ClockGearsMapCellSelector::~ClockGearsMapCellSelector() {
	if (loaded_map_instance) {
		loaded_map_instance = nullptr;
	}
}

void ClockGearsMapCellSelector::populate_maps() {
	_populate_maps_internal(is_visible_in_tree() || loaded_map_instance != nullptr);
}

void ClockGearsMapCellSelector::_populate_maps_internal(bool p_load_preview) {
	if (!map_opt) return;
	map_opt->clear();
	Vector<String> maps;
	if (DirAccess::exists("res://maps")) {
		Ref<DirAccess> da = DirAccess::open("res://maps");
		if (da.is_valid()) {
			da->list_dir_begin();
			String f = da->get_next();
			while (!f.is_empty()) {
				if (!da->current_is_dir() && f.ends_with(".tscn")) {
					maps.push_back(String("res://maps").path_join(f));
				}
				f = da->get_next();
			}
			da->list_dir_end();
		}
	}
	maps.sort();
	for (int i = 0; i < maps.size(); i++) {
		map_opt->add_item(maps[i].get_file(), i);
		map_opt->set_item_metadata(i, maps[i]);
	}
	if (!maps.is_empty()) {
		if (map_path_edit && map_path_edit->get_text().strip_edges().is_empty()) {
			map_opt->select(0);
			map_path_edit->set_text(maps[0]);
			if (p_load_preview) {
				load_map_preview(maps[0]);
			}
		}
	}
}

void ClockGearsMapCellSelector::set_target_map(const String &p_map_path) {
	if (map_path_edit) {
		map_path_edit->set_text(p_map_path);
	}
	if (map_opt) {
		bool found = false;
		for (int i = 0; i < map_opt->get_item_count(); i++) {
			if (String(map_opt->get_item_metadata(i)) == p_map_path || map_opt->get_item_text(i) == p_map_path.get_file()) {
				map_opt->select(i);
				found = true;
				break;
			}
		}
		if (!found && !p_map_path.is_empty()) {
			int idx = map_opt->get_item_count();
			map_opt->add_item(p_map_path.get_file(), idx);
			map_opt->set_item_metadata(idx, p_map_path);
			map_opt->select(idx);
		}
	}
	if (is_visible_in_tree() || loaded_map_instance != nullptr) {
		load_map_preview(p_map_path);
	}
}

String ClockGearsMapCellSelector::get_target_map() const {
	if (map_path_edit && !map_path_edit->get_text().strip_edges().is_empty()) {
		return map_path_edit->get_text().strip_edges();
	}
	if (map_opt && map_opt->get_selected() >= 0) {
		return map_opt->get_item_metadata(map_opt->get_selected());
	}
	return String();
}

void ClockGearsMapCellSelector::load_map_preview(const String &p_map_path) {
	if (!p_map_path.is_empty() && p_map_path == loaded_map_path && loaded_map_instance != nullptr) {
		return;
	}
	loaded_map_path = "";
	if (loaded_map_instance) {
		loaded_map_instance->queue_free();
		loaded_map_instance = nullptr;
	}
	if (subvp) {
		while (subvp->get_child_count() > 0) {
			Node *child = subvp->get_child(0);
			subvp->remove_child(child);
			child->queue_free();
		}
	}

	map_tiles_w = 25;
	map_tiles_h = 20;

	if (!p_map_path.is_empty() && ResourceLoader::exists(p_map_path)) {
		Ref<PackedScene> sdata = ResourceLoader::load(p_map_path);
		if (sdata.is_valid()) {
			Node *inst = sdata->instantiate();
			if (inst) {
				ClockGearsMap *cgmap = Object::cast_to<ClockGearsMap>(inst);
				if (cgmap) {
					cgmap->set_preview_only(true);
					loaded_map_instance = cgmap;
					loaded_map_path = p_map_path;
					Vector2i sz = cgmap->get_map_size();
					if (sz.x > 0 && sz.y > 0) {
						map_tiles_w = sz.x;
						map_tiles_h = sz.y;
					}
				}
				if (subvp) {
					subvp->add_child(inst);
				}
			}
		}
	}

	int px_w = map_tiles_w * 32;
	int px_h = map_tiles_h * 32;

	if (subvp) subvp->set_size(Vector2i(px_w, px_h));
	if (canvas_wrapper) canvas_wrapper->set_custom_minimum_size(Size2(px_w, px_h));
	if (subvp_container) subvp_container->set_custom_minimum_size(Size2(px_w, px_h));
	if (overlay_control) overlay_control->set_custom_minimum_size(Size2(px_w, px_h));

	if (x_spin) x_spin->set_max(MAX(0, map_tiles_w - 1));
	if (y_spin) y_spin->set_max(MAX(0, map_tiles_h - 1));

	if (overlay_control) overlay_control->queue_redraw();
}

void ClockGearsMapCellSelector::set_target_coords(int p_x, int p_y) {
	selected_cell_x = CLAMP(p_x, 0, MAX(0, map_tiles_w - 1));
	selected_cell_y = CLAMP(p_y, 0, MAX(0, map_tiles_h - 1));

	is_updating_coords = true;
	if (x_spin) x_spin->set_value(selected_cell_x);
	if (y_spin) y_spin->set_value(selected_cell_y);
	is_updating_coords = false;

	if (coords_label) {
		coords_label->set_text(vformat(CG_TR("Destino seleccionado: Casilla (%d, %d)"), selected_cell_x, selected_cell_y));
	}
	if (overlay_control) {
		overlay_control->queue_redraw();
	}
}

Vector2i ClockGearsMapCellSelector::get_target_coords() const {
	return Vector2i(selected_cell_x, selected_cell_y);
}

void ClockGearsMapCellSelector::set_target_direction(const String &p_dir) {
	if (!dir_opt) return;
	for (int i = 0; i < dir_opt->get_item_count(); i++) {
		if (dir_opt->get_item_text(i).to_lower() == p_dir.to_lower()) {
			dir_opt->select(i);
			return;
		}
	}
	// Fallback si viene en ingles
	String dlow = p_dir.to_lower();
	if (dlow == "retain" || dlow == "keep") dir_opt->select(0);
	else if (dlow == "up") dir_opt->select(1);
	else if (dlow == "down") dir_opt->select(2);
	else if (dlow == "left") dir_opt->select(3);
	else if (dlow == "right") dir_opt->select(4);
}

String ClockGearsMapCellSelector::get_target_direction() const {
	if (dir_opt && dir_opt->get_selected() >= 0) {
		return dir_opt->get_item_text(dir_opt->get_selected());
	}
	return "Abajo";
}

void ClockGearsMapCellSelector::_on_map_selected(int p_idx) {
	if (!map_opt || p_idx < 0 || p_idx >= map_opt->get_item_count()) return;
	String path = map_opt->get_item_metadata(p_idx);
	if (map_path_edit) {
		map_path_edit->set_text(path);
	}
	load_map_preview(path);
	emit_signal("target_changed", get_target_map(), get_target_coords(), get_target_direction());
}

void ClockGearsMapCellSelector::_on_dir_selected(int p_idx) {
	emit_signal("target_changed", get_target_map(), get_target_coords(), get_target_direction());
}

void ClockGearsMapCellSelector::_on_spin_changed(double p_val) {
	if (is_updating_coords) return;
	selected_cell_x = x_spin ? int(x_spin->get_value()) : 0;
	selected_cell_y = y_spin ? int(y_spin->get_value()) : 0;
	if (coords_label) {
		coords_label->set_text(vformat(CG_TR("Destino seleccionado: Casilla (%d, %d)"), selected_cell_x, selected_cell_y));
	}
	if (overlay_control) {
		overlay_control->queue_redraw();
	}
	emit_signal("target_changed", get_target_map(), get_target_coords(), get_target_direction());
}

void ClockGearsMapCellSelector::_on_overlay_draw() {
	if (!overlay_control) return;
	int w = map_tiles_w * 32;
	int h = map_tiles_h * 32;

	// Si no hay instancia de mapa cargada, dibujar fondo de damero 32x32
	if (!loaded_map_instance) {
		overlay_control->draw_rect(Rect2(0, 0, w, h), Color(0.12, 0.14, 0.18, 1.0), true);
		for (int y = 0; y < map_tiles_h; y++) {
			for (int x = 0; x < map_tiles_w; x++) {
				if ((x + y) % 2 == 1) {
					overlay_control->draw_rect(Rect2(x * 32, y * 32, 32, 32), Color(0.16, 0.18, 0.24, 1.0), true);
				}
			}
		}
	}

	// Cuadrícula sutil
	Color grid_col(1.0, 1.0, 1.0, 0.14);
	for (int x = 0; x <= w; x += 32) {
		overlay_control->draw_line(Point2(x, 0), Point2(x, h), grid_col, 1.0);
	}
	for (int y = 0; y <= h; y += 32) {
		overlay_control->draw_line(Point2(0, y), Point2(w, y), grid_col, 1.0);
	}

	// Marcador visual sobre la casilla seleccionada (cuadrado brillante con relleno semitransparente)
	Rect2 sel_rect(selected_cell_x * 32, selected_cell_y * 32, 32, 32);
	overlay_control->draw_rect(sel_rect, Color(1.0, 0.85, 0.2, 0.35), true);
	overlay_control->draw_rect(sel_rect, Color(1.0, 0.95, 0.4, 0.95), false, 2.0);

	// Cruz de centrado suave
	Point2 center = sel_rect.position + sel_rect.size * 0.5f;
	overlay_control->draw_line(Point2(center.x - 5, center.y), Point2(center.x + 5, center.y), Color(1, 1, 1, 0.85), 1.0);
	overlay_control->draw_line(Point2(center.x, center.y - 5), Point2(center.x, center.y + 5), Color(1, 1, 1, 0.85), 1.0);
}

void ClockGearsMapCellSelector::select_cell_at_pixel(const Vector2 &p_pixel_pos) {
	int cx = CLAMP(int(p_pixel_pos.x / 32.0f), 0, MAX(0, map_tiles_w - 1));
	int cy = CLAMP(int(p_pixel_pos.y / 32.0f), 0, MAX(0, map_tiles_h - 1));
	set_target_coords(cx, cy);
	emit_signal("target_changed", get_target_map(), get_target_coords(), get_target_direction());
}

void ClockGearsMapCellSelector::_on_overlay_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		select_cell_at_pixel(mb->get_position());
	}
}

ClockGearsEvent *ClockGearsMapCellSelector::create_quick_door_event(Node *p_scene, const Vector2i &p_cell, const String &p_map, int p_x, int p_y, const String &p_dir, const String &p_sprite_path, int p_hframes) {
	int grid_size = 32;

	ClockGearsEvent *ev = memnew(ClockGearsEvent);
	ev->set_position(Vector2(p_cell.x * grid_size, p_cell.y * grid_size));
	ev->set_name(vformat("EV_Puerta_%03d_%03d", p_cell.x, p_cell.y));
	ev->set_event_name("EV_Puerta");

	Dictionary p1;
	p1["sw1_en"] = false;
	p1["sw2_en"] = false;
	p1["self_sw_en"] = false;
	p1["self_sw_letter"] = "A";
	p1["var_en"] = false;
	p1["item_en"] = false;
	p1["actor_en"] = false;
	p1["trigger"] = TRIGGER_PLAYER_TOUCH;
	p1["priority"] = 1;
	p1["through"] = false;
	p1["dir_fix"] = true;
	p1["graphic_path"] = p_sprite_path;
	p1["hframes"] = MAX(1, p_hframes);
	p1["vframes"] = 1;
	p1["frame"] = 0;
	p1["move_type"] = MOVE_FIXED;
	p1["speed"] = 3;
	p1["freq"] = 3;

	Array cmds;

	// 1. Sonido de puerta
	Dictionary c_se;
	c_se["type"] = "se";
	c_se["file"] = "res://audio/se/door.ogg";
	c_se["volume"] = 90;
	c_se["pitch"] = 100;
	cmds.push_back(c_se);

	// 2. Animación de abrir puerta (si tiene frames)
	if (p_hframes > 1) {
		Dictionary c_wait_open;
		c_wait_open["type"] = "wait";
		c_wait_open["frames"] = 12;
		cmds.push_back(c_wait_open);
	}

	// 3. Fundido a negro (Oscurecer)
	Dictionary c_fade_out;
	c_fade_out["type"] = "fade";
	c_fade_out["mode"] = "Oscurecer";
	c_fade_out["duration"] = 20;
	cmds.push_back(c_fade_out);

	// 4. Comando Transferir Jugador
	Dictionary c_tele;
	c_tele["type"] = "teleport";
	c_tele["map"] = p_map;
	c_tele["x"] = p_x;
	c_tele["y"] = p_y;
	c_tele["dir"] = p_dir.is_empty() ? "Abajo" : p_dir;
	cmds.push_back(c_tele);

	// 5. Fundido desde negro (Aclarar)
	Dictionary c_fade_in;
	c_fade_in["type"] = "fade";
	c_fade_in["mode"] = "Aclarar";
	c_fade_in["duration"] = 20;
	cmds.push_back(c_fade_in);

	p1["commands"] = cmds;
	Array pages;
	pages.push_back(p1);
	ev->set_pages(pages);
	ev->update_active_page();
	ev->serialize_to_meta();

	if (p_scene) {
		ClockGearsMap *map = Object::cast_to<ClockGearsMap>(p_scene);
		if (map && map->get_events_container()) {
			map->get_events_container()->add_child(ev);
		} else {
			p_scene->add_child(ev);
		}
		ev->set_owner(p_scene);
	}

	return ev;
}
