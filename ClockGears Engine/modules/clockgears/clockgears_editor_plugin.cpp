#include "clockgears_editor_plugin.h"
#include "clockgears_map.h"
#include "core/clockgears_data_manager.h"
#include "core/clockgears_system_catalog.h"
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "editor/clockgears_theme.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/split_container.h"
#include "scene/main/viewport.h"
#include "scene/resources/packed_scene.h"

ClockGearsEditorPlugin *ClockGearsEditorPlugin::singleton = nullptr;

void ClockGearsEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_database"), &ClockGearsEditorPlugin::open_database);
	ClassDB::bind_method(D_METHOD("open_resource_manager"), &ClockGearsEditorPlugin::open_resource_manager);
	ClassDB::bind_method(D_METHOD("open_new_project"), &ClockGearsEditorPlugin::open_new_project);
	ClassDB::bind_method(D_METHOD("open_export"), &ClockGearsEditorPlugin::open_export);
	ClassDB::bind_method(D_METHOD("open_preferences"), &ClockGearsEditorPlugin::open_preferences);
	ClassDB::bind_method(D_METHOD("open_camera_dialog"), &ClockGearsEditorPlugin::open_camera_dialog);
	ClassDB::bind_method(D_METHOD("validate_playtest_conditions"), &ClockGearsEditorPlugin::validate_playtest_conditions);
	ClassDB::bind_method(D_METHOD("start_playtest"), &ClockGearsEditorPlugin::start_playtest);
	ClassDB::bind_method(D_METHOD("open_event_editor_at_cell", "x", "y"), &ClockGearsEditorPlugin::open_event_editor_at_cell);
	ClassDB::bind_method(D_METHOD("show_event_context_menu", "x", "y", "screen_pos"), &ClockGearsEditorPlugin::show_event_context_menu);
}

void ClockGearsEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			singleton = this;
			set_process_shortcut_input(true);
		} break;
		case NOTIFICATION_READY: {
			if (ClockGearsDataManager::get_singleton() && FileAccess::exists("res://clockgears_data/actors.json")) {
				ClockGearsDataManager::get_singleton()->load_all("res://clockgears_data");
			}
		} break;
	}
}

void ClockGearsEditorPlugin::shortcut_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed() || k->is_echo()) {
		return;
	}
	// Atajo global F9: abrir Base de Datos (Sección 11.1)
	if (k->get_keycode() == Key::F9 && !k->is_ctrl_pressed() && !k->is_alt_pressed() && !k->is_shift_pressed()) {
		open_database();
		if (get_viewport()) {
			get_viewport()->set_input_as_handled();
		}
	} else if (k->get_keycode() == Key::F10 && !k->is_ctrl_pressed() && !k->is_alt_pressed() && !k->is_shift_pressed()) {
		open_resource_manager();
		if (get_viewport()) {
			get_viewport()->set_input_as_handled();
		}
	} else if ((k->get_keycode() == Key::F5 || k->get_keycode() == Key::F12) && !k->is_alt_pressed() && !k->is_shift_pressed()) {
		start_playtest();
		if (get_viewport()) {
			get_viewport()->set_input_as_handled();
		}
	}
}

void ClockGearsEditorPlugin::save_external_data() {
	// Al guardar proyecto/escena (Ctrl+S), persistir también la Base de Datos de ClockGears (Sección 12)
	if (ClockGearsDataManager::get_singleton() && ClockGearsDataManager::get_singleton()->is_dirty()) {
		ClockGearsDataManager::get_singleton()->save_all("res://clockgears_data");
	}
}

void ClockGearsEditorPlugin::make_visible(bool p_visible) {
	if (automation_panel) {
		automation_panel->set_visible(p_visible);
	}
}

Node *ClockGearsEditorPlugin::ensure_active_map() {
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (scene) {
		return scene;
	}

	// 1. If maps exist in res://maps/, load the first one
	if (DirAccess::exists("res://maps")) {
		Ref<DirAccess> da = DirAccess::open("res://maps");
		if (da.is_valid()) {
			da->list_dir_begin();
			String f = da->get_next();
			Vector<String> maps;
			while (!f.is_empty()) {
				if (!da->current_is_dir() && f.ends_with(".tscn")) {
					maps.push_back(String("res://maps").path_join(f));
				}
				f = da->get_next();
			}
			da->list_dir_end();
			maps.sort();
			if (!maps.is_empty()) {
				EditorNode::get_singleton()->open_scene(maps[0]);
				if (map_panel) {
					map_panel->refresh_maps_from_disk();
				}
				return EditorNode::get_singleton()->get_edited_scene();
			}
		}
	}

	// 2. No map exists: automatically create default Map 001
	DirAccess::make_dir_recursive_absolute("res://maps");
	String map_path = "res://maps/map001.tscn";

	ClockGearsMap *map = memnew(ClockGearsMap);
	map->set_name("Map001");
	map->set_map_name("Mapa001");
	map->set_map_size(Vector2i(25, 20));
	map->ensure_default_layers();

	for (int i = 0; i < map->get_child_count(); i++) {
		Node *c = map->get_child(i);
		if (c) {
			c->set_owner(map);
		}
	}

	Ref<PackedScene> packed = memnew(PackedScene);
	packed->pack(map);
	ResourceSaver::save(packed, map_path);
	memdelete(map);

	if (ProjectSettings::get_singleton()) {
		ProjectSettings::get_singleton()->set_setting("application/run/main_scene", map_path);
		ProjectSettings::get_singleton()->save();
	}

	EditorNode::get_singleton()->open_scene(map_path);

	if (map_panel) {
		map_panel->refresh_maps_from_disk();
	}

	return EditorNode::get_singleton()->get_edited_scene();
}

ClockGearsEvent *ClockGearsEditorPlugin::get_event_at_cell(int p_x, int p_y) const {
	Node *scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (!scene) {
		return nullptr;
	}
	int grid_size = 32;
	if (CanvasItemEditor::get_singleton()) {
		grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
	}
	TypedArray<Node> events = scene->find_children("*", "ClockGearsEvent", true, false);
	for (int i = 0; i < events.size(); i++) {
		ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(events[i]);
		if (ev) {
			int ev_cx = Math::floor(ev->get_position().x / (real_t)grid_size);
			int ev_cy = Math::floor(ev->get_position().y / (real_t)grid_size);
			if (ev_cx == p_x && ev_cy == p_y) {
				return ev;
			}
		}
	}
	return nullptr;
}

void ClockGearsEditorPlugin::open_event_editor(ClockGearsEvent *p_ev) {
	if (event_editor_dialog) {
		event_editor_dialog->edit_event(p_ev);
	}
}

void ClockGearsEditorPlugin::open_event_editor_at_cell(int p_x, int p_y) {
	if (!event_editor_dialog) {
		return;
	}

	Node *scene = ensure_active_map();
	if (!scene) {
		ERR_PRINT("ClockGears: No se pudo obtener ni crear un mapa activo para el evento.");
		return;
	}

	ClockGearsEvent *found_ev = get_event_at_cell(p_x, p_y);
	if (!found_ev) {
		found_ev = memnew(ClockGearsEvent);
		int grid_size = 32;
		if (CanvasItemEditor::get_singleton()) {
			grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
		}
		found_ev->set_position(Vector2(p_x * grid_size, p_y * grid_size));
		found_ev->set_event_name(vformat("EV_%03d_%03d", p_x, p_y));
		found_ev->set_name(found_ev->get_event_name());
		ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
		if (map && map->get_events_container()) {
			map->get_events_container()->add_child(found_ev);
		} else {
			scene->add_child(found_ev);
		}
		found_ev->set_owner(scene);
	}

	event_editor_dialog->edit_event(found_ev);
}

void ClockGearsEditorPlugin::show_event_context_menu(int p_x, int p_y, const Vector2 &p_screen_pos) {
	if (!event_context_menu) {
		return;
	}
	context_menu_cell = Point2i(p_x, p_y);
	ClockGearsEvent *ev = get_event_at_cell(p_x, p_y);

	event_context_menu->clear(false);
	if (ev) {
		event_context_menu->add_item(CG_TR("Editar Evento..."), ACTION_EDIT_OR_CREATE);
	} else {
		event_context_menu->add_item(CG_TR("Crear Nuevo Evento..."), ACTION_EDIT_OR_CREATE);
		if (quick_events_menu) {
			quick_events_menu->clear();
			quick_events_menu->add_item(CG_TR("Cofre del Tesoro..."), QUICK_EV_CHEST);
			quick_events_menu->add_item(CG_TR("Puerta..."), QUICK_EV_DOOR);
			quick_events_menu->add_item(CG_TR("Transferir..."), QUICK_EV_TRANSFER);
			quick_events_menu->add_item(CG_TR("Posada..."), QUICK_EV_INN);
			quick_events_menu->add_item(CG_TR("Punto de Guardado..."), QUICK_EV_SAVE);
			event_context_menu->add_submenu_node_item(CG_TR("Creación Rápida de Eventos"), quick_events_menu, ACTION_QUICK_EVENTS_SUBMENU);
			int q_idx = event_context_menu->get_item_index(ACTION_QUICK_EVENTS_SUBMENU);
			if (q_idx >= 0) {
				event_context_menu->set_item_disabled(q_idx, false);
			}
		}
	}
	event_context_menu->add_separator();
	event_context_menu->add_item(CG_TR("Fijar Posición Inicial del Jugador aquí"), ACTION_SET_START_POS);
	event_context_menu->add_separator();
	event_context_menu->add_item(CG_TR("Copiar"), ACTION_COPY);
	int copy_idx = event_context_menu->get_item_index(ACTION_COPY);
	if (copy_idx >= 0) {
		event_context_menu->set_item_disabled(copy_idx, ev == nullptr);
	}

	event_context_menu->add_item(CG_TR("Pegar"), ACTION_PASTE);
	int paste_idx = event_context_menu->get_item_index(ACTION_PASTE);
	if (paste_idx >= 0) {
		event_context_menu->set_item_disabled(paste_idx, clipboard_event_data.is_empty());
	}

	event_context_menu->add_separator();
	event_context_menu->add_item(CG_TR("Eliminar"), ACTION_DELETE);
	int del_idx = event_context_menu->get_item_index(ACTION_DELETE);
	if (del_idx >= 0) {
		event_context_menu->set_item_disabled(del_idx, ev == nullptr);
	}

	event_context_menu->set_position(p_screen_pos);
	event_context_menu->reset_size();
	event_context_menu->popup();
}

void ClockGearsEditorPlugin::_on_event_context_menu_id_pressed(int p_id) {
	switch (p_id) {
		case ACTION_EDIT_OR_CREATE: { // Crear / Editar
			open_event_editor_at_cell(context_menu_cell.x, context_menu_cell.y);
		} break;

		case ACTION_SET_START_POS: {
			Node *scene = ensure_active_map();
			ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
			if (map) {
				map->set_player_start_position(Vector2i(context_menu_cell.x, context_menu_cell.y), 2);
				if (CanvasItemEditor::get_singleton()) {
					CanvasItemEditor::get_singleton()->update_viewport();
				}
			}
		} break;

		case ACTION_QUICK_EVENTS_SUBMENU: {
			if (quick_events_menu) {
				Point2 mpos = event_context_menu->get_position();
				Size2 msize = event_context_menu->get_size();
				quick_events_menu->set_position(mpos + Point2(msize.x - 4, 28));
				quick_events_menu->reset_size();
				quick_events_menu->popup();
			}
		} break;

		case ACTION_COPY: { // Copiar
			ClockGearsEvent *ev = get_event_at_cell(context_menu_cell.x, context_menu_cell.y);
			if (ev) {
				clipboard_event_data["name"] = ev->get_event_name();
				clipboard_event_data["trigger"] = ev->get_trigger_type();
				clipboard_event_data["move_type"] = ev->get_move_type();
				clipboard_event_data["speed"] = ev->get_move_speed();
				clipboard_event_data["freq"] = ev->get_move_frequency();
				clipboard_event_data["through"] = ev->is_through();
				clipboard_event_data["direction_fix"] = ev->is_direction_fix();
				clipboard_event_data["direction"] = ev->get_direction();
				clipboard_event_data["priority"] = ev->get_priority_type();
				clipboard_event_data["graphic_path"] = ev->get_graphic_path();
				clipboard_event_data["hframes"] = ev->get_sprite_hframes();
				clipboard_event_data["vframes"] = ev->get_sprite_vframes();
				clipboard_event_data["frame"] = ev->get_sprite_frame();
				clipboard_event_data["commands"] = ev->get_commands().duplicate(true);
				clipboard_event_data["pages"] = ev->get_pages().duplicate(true);
				clipboard_event_data["sw1_en"] = ev->is_condition_switch_1_enabled();
				clipboard_event_data["sw1_name"] = ev->get_condition_switch_1_name();
				clipboard_event_data["sw2_en"] = ev->is_condition_switch_2_enabled();
				clipboard_event_data["sw2_name"] = ev->get_condition_switch_2_name();
				clipboard_event_data["self_sw_en"] = ev->is_condition_self_switch_enabled();
				clipboard_event_data["self_sw_letter"] = ev->get_condition_self_switch_letter();
				clipboard_event_data["self_switches"] = ev->get_self_switches().duplicate(true);
				clipboard_event_data["var_en"] = ev->is_condition_var_enabled();
				clipboard_event_data["var_name"] = ev->get_condition_var_name();
				clipboard_event_data["var_val"] = ev->get_condition_var_value();
				clipboard_event_data["item_en"] = ev->is_condition_item_enabled();
				clipboard_event_data["item_name"] = ev->get_condition_item_name();
				clipboard_event_data["actor_en"] = ev->is_condition_actor_enabled();
				clipboard_event_data["actor_name"] = ev->get_condition_actor_name();
			}
		} break;

		case ACTION_PASTE: { // Pegar
			if (!clipboard_event_data.is_empty()) {
				Node *scene = ensure_active_map();
				if (scene) {
					int grid_size = 32;
					if (CanvasItemEditor::get_singleton()) {
						grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
					}
					ClockGearsEvent *target = get_event_at_cell(context_menu_cell.x, context_menu_cell.y);
					if (!target) {
						target = memnew(ClockGearsEvent);
						target->set_position(Vector2(context_menu_cell.x * grid_size, context_menu_cell.y * grid_size));
						target->set_event_name(clipboard_event_data.get("name", vformat("EV_%03d_%03d", context_menu_cell.x, context_menu_cell.y)));
						target->set_name(target->get_event_name());
						ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
						if (map && map->get_events_container()) {
							map->get_events_container()->add_child(target);
						} else {
							scene->add_child(target);
						}
						target->set_owner(scene);
					}
					target->set_event_name(clipboard_event_data.get("name", "EV_Pasted"));
					target->set_trigger_type(clipboard_event_data.get("trigger", 0));
					target->set_move_type(clipboard_event_data.get("move_type", 0));
					target->set_move_speed(clipboard_event_data.get("speed", 100.0f));
					target->set_move_frequency(clipboard_event_data.get("freq", 1.0f));
					target->set_through(clipboard_event_data.get("through", false));
					target->set_direction_fix(clipboard_event_data.get("direction_fix", false));
					target->set_direction(clipboard_event_data.get("direction", 2));
					target->set_priority_type(clipboard_event_data.get("priority", 1));
					target->set_graphic_path(clipboard_event_data.get("graphic_path", ""));
					target->set_sprite_hframes(clipboard_event_data.get("hframes", 1));
					target->set_sprite_vframes(clipboard_event_data.get("vframes", 1));
					target->set_sprite_frame(clipboard_event_data.get("frame", 0));
					target->set_commands(clipboard_event_data.get("commands", Array()));
					if (clipboard_event_data.has("pages")) {
						target->set_pages(clipboard_event_data["pages"].duplicate(true));
					}
					target->set_condition_switch_1_enabled(clipboard_event_data.get("sw1_en", false));
					target->set_condition_switch_1_name(clipboard_event_data.get("sw1_name", ""));
					target->set_condition_switch_2_enabled(clipboard_event_data.get("sw2_en", false));
					target->set_condition_switch_2_name(clipboard_event_data.get("sw2_name", ""));
					target->set_condition_self_switch_enabled(clipboard_event_data.get("self_sw_en", false));
					target->set_condition_self_switch_letter(clipboard_event_data.get("self_sw_letter", "A"));
					target->set_self_switches(clipboard_event_data.get("self_switches", Dictionary()));
					target->set_condition_var_enabled(clipboard_event_data.get("var_en", false));
					target->set_condition_var_name(clipboard_event_data.get("var_name", ""));
					target->set_condition_var_value(clipboard_event_data.get("var_val", 0));
					target->set_condition_item_enabled(clipboard_event_data.get("item_en", false));
					target->set_condition_item_name(clipboard_event_data.get("item_name", ""));
					target->set_condition_actor_enabled(clipboard_event_data.get("actor_en", false));
					target->set_condition_actor_name(clipboard_event_data.get("actor_name", ""));

					target->update_active_page();
					target->serialize_to_meta();

					EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
					if (ur) {
						int history_id = ur->get_history_id_for_object(scene);
						ur->set_history_as_unsaved(history_id);
					}

					String scene_path = scene->get_scene_file_path();
					if (!scene_path.is_empty()) {
						EditorNode::get_singleton()->save_scene_if_open(scene_path);
					}

					if (CanvasItemEditor::get_singleton()) {
						CanvasItemEditor::get_singleton()->update_viewport();
					}
				}
			}
		} break;

		case ACTION_DELETE: { // Eliminar
			ClockGearsEvent *ev = get_event_at_cell(context_menu_cell.x, context_menu_cell.y);
			if (ev) {
				Node *scene = ev->get_owner() ? ev->get_owner() : ev->get_parent();
				ev->queue_free();
				if (scene) {
					EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
					if (ur) {
						int history_id = ur->get_history_id_for_object(scene);
						ur->set_history_as_unsaved(history_id);
					}
					String scene_path = scene->get_scene_file_path();
					if (!scene_path.is_empty()) {
						EditorNode::get_singleton()->save_scene_if_open(scene_path);
					}
				}
				if (CanvasItemEditor::get_singleton()) {
					CanvasItemEditor::get_singleton()->update_viewport();
				}
			}
		} break;
	}
}

void ClockGearsEditorPlugin::_on_quick_event_id_pressed(int p_id) {
	print_line(vformat("[ClockGears] Quick event selected: %d at (%d, %d)", p_id, context_menu_cell.x, context_menu_cell.y));
	Node *scene = ensure_active_map();
	if (!scene) {
		ERR_PRINT(String::utf8("ClockGears: No se pudo obtener ni crear un mapa activo para el evento rápido."));
		return;
	}
	int grid_size = 32;
	if (CanvasItemEditor::get_singleton()) {
		grid_size = CanvasItemEditor::get_singleton()->get_clockgears_grid_size();
	}

	ClockGearsEvent *ev = memnew(ClockGearsEvent);
	ev->set_position(Vector2(context_menu_cell.x * grid_size, context_menu_cell.y * grid_size));

	Array pages;

	switch (p_id) {
		case QUICK_EV_CHEST: {
			ev->set_name(vformat("EV_Cofre_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
			ev->set_event_name("EV_Cofre");
			// Page 1: Cerrado, con contenido
			Dictionary p1;
			p1["sw1_en"] = false;
			p1["sw2_en"] = false;
			p1["self_sw_en"] = false;
			p1["self_sw_letter"] = "A";
			p1["var_en"] = false;
			p1["item_en"] = false;
			p1["actor_en"] = false;
			p1["trigger"] = TRIGGER_ACTION_BUTTON;
			p1["priority"] = 1;
			p1["through"] = false;
			p1["dir_fix"] = true;
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds1;
			Dictionary c_se;
			c_se["type"] = "se";
			c_se["file"] = "res://audio/se/chest.ogg";
			c_se["volume"] = 90;
			c_se["pitch"] = 100;
			cmds1.push_back(c_se);

			Dictionary c_gold;
			c_gold["type"] = "gold";
			c_gold["op"] = "+";
			c_gold["amount"] = 100;
			cmds1.push_back(c_gold);

			Dictionary c_msg;
			c_msg["type"] = "message";
			c_msg["text"] = CG_TR("¡Has encontrado 100 Monedas de Oro en el cofre!");
			c_msg["frame"] = 0;
			c_msg["position"] = 0;
			c_msg["face"] = "";
			cmds1.push_back(c_msg);

			Dictionary c_ssw;
			c_ssw["type"] = "self_switch";
			c_ssw["letter"] = "A";
			c_ssw["val"] = true;
			cmds1.push_back(c_ssw);

			p1["commands"] = cmds1;
			pages.push_back(p1);

			// Page 2: Abierto (Condición Interruptor Local A = ON)
			Dictionary p2;
			p2["sw1_en"] = false;
			p2["sw2_en"] = false;
			p2["self_sw_en"] = true;
			p2["self_sw_letter"] = "A";
			p2["var_en"] = false;
			p2["item_en"] = false;
			p2["actor_en"] = false;
			p2["trigger"] = TRIGGER_ACTION_BUTTON;
			p2["priority"] = 1;
			p2["through"] = false;
			p2["dir_fix"] = true;
			p2["graphic_path"] = "";
			p2["hframes"] = 1;
			p2["vframes"] = 1;
			p2["frame"] = 0;
			p2["move_type"] = MOVE_FIXED;
			p2["speed"] = 3;
			p2["freq"] = 3;

			Array cmds2;
			Dictionary c_empty_msg;
			c_empty_msg["type"] = "message";
			c_empty_msg["text"] = CG_TR("El cofre ya está vacío.");
			c_empty_msg["frame"] = 0;
			c_empty_msg["position"] = 0;
			c_empty_msg["face"] = "";
			cmds2.push_back(c_empty_msg);
			p2["commands"] = cmds2;
			pages.push_back(p2);
		} break;

		case QUICK_EV_DOOR: {
			ev->set_name(vformat("EV_Puerta_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
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
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds;
			Dictionary c_se;
			c_se["type"] = "se";
			c_se["file"] = "res://audio/se/door.ogg";
			c_se["volume"] = 90;
			c_se["pitch"] = 100;
			cmds.push_back(c_se);

			Dictionary c_wait;
			c_wait["type"] = "wait";
			c_wait["frames"] = 15;
			cmds.push_back(c_wait);

			Dictionary c_fade_out;
			c_fade_out["type"] = "fade";
			c_fade_out["mode"] = "Oscurecer";
			c_fade_out["duration"] = 20;
			cmds.push_back(c_fade_out);

			Dictionary c_tele;
			c_tele["type"] = "teleport";
			c_tele["map"] = "maps/map002.tscn";
			c_tele["x"] = 10;
			c_tele["y"] = 10;
			c_tele["dir"] = "Abajo";
			cmds.push_back(c_tele);

			Dictionary c_fade_in;
			c_fade_in["type"] = "fade";
			c_fade_in["mode"] = "Aclarar";
			c_fade_in["duration"] = 20;
			cmds.push_back(c_fade_in);

			p1["commands"] = cmds;
			pages.push_back(p1);
		} break;

		case QUICK_EV_TRANSFER: {
			ev->set_name(vformat("EV_Transfer_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
			ev->set_event_name("EV_Transferir");
			Dictionary p1;
			p1["sw1_en"] = false;
			p1["sw2_en"] = false;
			p1["self_sw_en"] = false;
			p1["self_sw_letter"] = "A";
			p1["var_en"] = false;
			p1["item_en"] = false;
			p1["actor_en"] = false;
			p1["trigger"] = TRIGGER_PLAYER_TOUCH;
			p1["priority"] = 0;
			p1["through"] = true;
			p1["dir_fix"] = false;
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds;
			Dictionary c_fade_out;
			c_fade_out["type"] = "fade";
			c_fade_out["mode"] = "Oscurecer";
			c_fade_out["duration"] = 20;
			cmds.push_back(c_fade_out);

			Dictionary c_tele;
			c_tele["type"] = "teleport";
			c_tele["map"] = "maps/map002.tscn";
			c_tele["x"] = 10;
			c_tele["y"] = 10;
			c_tele["dir"] = "Mantener";
			cmds.push_back(c_tele);

			Dictionary c_fade_in;
			c_fade_in["type"] = "fade";
			c_fade_in["mode"] = "Aclarar";
			c_fade_in["duration"] = 20;
			cmds.push_back(c_fade_in);

			p1["commands"] = cmds;
			pages.push_back(p1);
		} break;

		case QUICK_EV_INN: {
			ev->set_name(vformat("EV_Posada_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
			ev->set_event_name("EV_Posada");
			Dictionary p1;
			p1["sw1_en"] = false;
			p1["sw2_en"] = false;
			p1["self_sw_en"] = false;
			p1["self_sw_letter"] = "A";
			p1["var_en"] = false;
			p1["item_en"] = false;
			p1["actor_en"] = false;
			p1["trigger"] = TRIGGER_ACTION_BUTTON;
			p1["priority"] = 1;
			p1["through"] = false;
			p1["dir_fix"] = true;
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds;
			Dictionary c_msg1;
			c_msg1["type"] = "message";
			c_msg1["text"] = CG_TR("¿Deseas descansar en la posada por 10 monedas de oro?");
			cmds.push_back(c_msg1);

			Dictionary c_choices;
			c_choices["type"] = "choices";
			c_choices["c1"] = CG_TR("Sí, descansar");
			c_choices["c2"] = CG_TR("No, gracias");
			c_choices["c3"] = "";
			cmds.push_back(c_choices);

			Dictionary c_gold;
			c_gold["type"] = "gold";
			c_gold["op"] = "-";
			c_gold["amount"] = 10;
			cmds.push_back(c_gold);

			Dictionary c_fade_out;
			c_fade_out["type"] = "fade";
			c_fade_out["mode"] = "Oscurecer";
			c_fade_out["duration"] = 30;
			cmds.push_back(c_fade_out);

			Dictionary c_se;
			c_se["type"] = "se";
			c_se["file"] = "res://audio/se/inn.ogg";
			c_se["volume"] = 90;
			c_se["pitch"] = 100;
			cmds.push_back(c_se);

			Dictionary c_wait;
			c_wait["type"] = "wait";
			c_wait["frames"] = 60;
			cmds.push_back(c_wait);

			Dictionary c_heal;
			c_heal["type"] = "hp_mp";
			c_heal["stat"] = "HP";
			c_heal["op"] = "+";
			c_heal["amount"] = 9999;
			cmds.push_back(c_heal);

			Dictionary c_fade_in;
			c_fade_in["type"] = "fade";
			c_fade_in["mode"] = "Aclarar";
			c_fade_in["duration"] = 30;
			cmds.push_back(c_fade_in);

			Dictionary c_msg2;
			c_msg2["type"] = "message";
			c_msg2["text"] = CG_TR("¡Has descansado placenteramente y recuperado todas tus fuerzas!");
			cmds.push_back(c_msg2);

			p1["commands"] = cmds;
			pages.push_back(p1);
		} break;

		case QUICK_EV_SAVE: {
			ev->set_name(vformat("EV_Guardar_%03d_%03d", context_menu_cell.x, context_menu_cell.y));
			ev->set_event_name("EV_Guardado");
			Dictionary p1;
			p1["sw1_en"] = false;
			p1["sw2_en"] = false;
			p1["self_sw_en"] = false;
			p1["self_sw_letter"] = "A";
			p1["var_en"] = false;
			p1["item_en"] = false;
			p1["actor_en"] = false;
			p1["trigger"] = TRIGGER_ACTION_BUTTON;
			p1["priority"] = 1;
			p1["through"] = false;
			p1["dir_fix"] = true;
			p1["graphic_path"] = "";
			p1["hframes"] = 1;
			p1["vframes"] = 1;
			p1["frame"] = 0;
			p1["move_type"] = MOVE_FIXED;
			p1["speed"] = 3;
			p1["freq"] = 3;

			Array cmds;
			Dictionary c_se;
			c_se["type"] = "se";
			c_se["file"] = "res://audio/se/save.ogg";
			c_se["volume"] = 90;
			c_se["pitch"] = 100;
			cmds.push_back(c_se);

			Dictionary c_msg;
			c_msg["type"] = "message";
			c_msg["text"] = CG_TR("¡Progreso de la partida guardado con éxito!");
			cmds.push_back(c_msg);

			p1["commands"] = cmds;
			pages.push_back(p1);
		} break;
	}

	ev->set_pages(pages);
	ev->update_active_page();
	ev->serialize_to_meta();

	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(scene);
	if (map && map->get_events_container()) {
		map->get_events_container()->add_child(ev);
	} else {
		scene->add_child(ev);
	}
	ev->set_owner(scene);

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	if (ur) {
		int history_id = ur->get_history_id_for_object(scene);
		ur->set_history_as_unsaved(history_id);
	}

	String scene_path = scene->get_scene_file_path();
	if (!scene_path.is_empty()) {
		EditorNode::get_singleton()->save_scene_if_open(scene_path);
	} else {
		EditorNode::get_singleton()->clockgears_save_scene();
	}

	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->update_viewport();
	}
}

void ClockGearsEditorPlugin::open_database() {
	if (database_dialog) {
		database_dialog->open_database();
	}
}

void ClockGearsEditorPlugin::open_resource_manager() {
	if (resource_manager_dialog) {
		resource_manager_dialog->open_manager();
	}
}

void ClockGearsEditorPlugin::open_new_project() {
	if (new_project_dialog) {
		new_project_dialog->open_dialog();
	}
}

void ClockGearsEditorPlugin::open_export() {
	if (export_dialog) {
		export_dialog->popup_simplified_export();
	}
}

void ClockGearsEditorPlugin::open_preferences() {
	if (preferences_dialog) {
		preferences_dialog->open_preferences();
	}
}

void ClockGearsEditorPlugin::open_camera_dialog() {
	if (camera_dialog) {
		camera_dialog->open_camera_dialog();
	}
}

Dictionary ClockGearsEditorPlugin::validate_playtest_conditions() {
	Dictionary result;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		result["valid"] = false;
		result["error"] = "ClockGearsDataManager no inicializado.";
		return result;
	}

	if (dm->get_category_array(CAT_ACTORS).is_empty() && FileAccess::exists("res://clockgears_data/actors.json")) {
		dm->load_all("res://clockgears_data");
	}

	// 1. Condición: Actor jugable creado en la Base de Datos
	Array actors = dm->get_category_array(CAT_ACTORS);
	bool has_actor = !actors.is_empty();

	// 2. Condición: Posición inicial del jugador y Mapa inicial
	Dictionary sys = dm->get_system_config();
	bool has_start_pos = (bool)sys.get("has_start_pos", false);
	String sys_start_map = sys.get("start_map", "");
	int start_x = (int)sys.get("start_x", -1);
	int start_y = (int)sys.get("start_y", -1);
	int start_dir = (int)sys.get("start_dir", 2);

	if (!has_start_pos && !sys_start_map.is_empty() && start_x >= 0 && start_y >= 0 && FileAccess::exists(sys_start_map)) {
		has_start_pos = true;
	}

	// Determinar el mapa objetivo a cargar (prioridad: start_map de Posición Inicial / Sistema, luego mapa abierto)
	String target_map_path;
	Node *edited_scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (!sys_start_map.is_empty() && FileAccess::exists(sys_start_map)) {
		target_map_path = sys_start_map;
	} else if (edited_scene && Object::cast_to<ClockGearsMap>(edited_scene)) {
		target_map_path = edited_scene->get_scene_file_path();
	}

	if (target_map_path.is_empty() && DirAccess::exists("res://maps")) {
		Ref<DirAccess> da = DirAccess::open("res://maps");
		if (da.is_valid()) {
			Vector<String> maps;
			da->list_dir_begin();
			String f = da->get_next();
			while (!f.is_empty()) {
				if (!da->current_is_dir() && f.ends_with(".tscn")) {
					maps.push_back(String("res://maps").path_join(f));
				}
				f = da->get_next();
			}
			da->list_dir_end();
			maps.sort();
			if (!maps.is_empty()) {
				target_map_path = maps[0];
			}
		}
	}

	bool has_map = !target_map_path.is_empty() && FileAccess::exists(target_map_path);

	// 4. Condición diagnóstica: Tileset del mapa
	bool has_tileset = false;
	String tileset_info = "Sin tileset asignado (opcional, se puede jugar sobre rejilla)";
	if (edited_scene) {
		ClockGearsMap *cm = Object::cast_to<ClockGearsMap>(edited_scene);
		if (cm && cm->get_tileset_id() > 0) {
			has_tileset = true;
			tileset_info = vformat("TileSet ID: %d", cm->get_tileset_id());
		} else if (cm && cm->get_layer(0) && cm->get_layer(0)->get_tile_set().is_valid() && cm->get_layer(0)->get_tile_set()->get_source_count() > 0) {
			has_tileset = true;
			tileset_info = "TileSet activo en mapa";
		}
	}

	print_line("==========================================================");
	print_line("[ClockGears Playtest] Verificando condiciones de arranque (F5 / Jugar):");
	print_line(vformat("  1. Actor en Base de Datos : %s (Total: %d)", has_actor ? "OK" : "FALTA", actors.size()));
	print_line(vformat("  2. Mapa inicial           : %s (Ruta: '%s')", has_map ? "OK" : "FALTA", target_map_path));
	print_line(vformat("  3. Posición inicial (PJ)  : %s (Mapa: '%s', X: %d, Y: %d, Dir: %d)",
			has_start_pos ? "OK" : "FALTA", sys_start_map, start_x, start_y, start_dir));
	print_line(vformat("  4. Tileset                : %s (%s)", has_tileset ? "OK" : "AVISO", tileset_info));
	print_line("==========================================================");

	PackedStringArray missing_items;
	if (!has_actor) {
		missing_items.push_back(String::utf8("Actor jugable en la Base de Datos (F9 > Actores)"));
	}
	if (!has_map) {
		missing_items.push_back(String::utf8("Mapa inicial válido en el proyecto (Árbol de Mapas)"));
	}
	if (!has_start_pos) {
		missing_items.push_back(String::utf8("Posición inicial del jugador (Herramienta 'Posición Inicial del Jugador' en la barra del mapa)"));
	}

	result["has_actor"] = has_actor;
	result["has_map"] = has_map;
	result["has_start_pos"] = has_start_pos;
	result["has_tileset"] = has_tileset;
	result["target_map"] = target_map_path;
	result["start_x"] = start_x;
	result["start_y"] = start_y;
	result["start_dir"] = start_dir;
	result["missing"] = missing_items;
	result["valid"] = (has_actor && has_map && has_start_pos);
	return result;
}

bool ClockGearsEditorPlugin::start_playtest() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		ERR_PRINT("[ClockGears Playtest] Error crítico: ClockGearsDataManager no disponible.");
		return false;
	}

	// Guardar mapa abierto antes de validar por si es un mapa recién creado
	Node *edited_scene = EditorNode::get_singleton() ? EditorNode::get_singleton()->get_edited_scene() : nullptr;
	if (edited_scene && !edited_scene->get_scene_file_path().is_empty()) {
		EditorNode::get_singleton()->save_scene_if_open(edited_scene->get_scene_file_path());
	}

	Dictionary check = validate_playtest_conditions();
	bool is_valid = check.get("valid", false);
	if (!is_valid) {
		bool has_actor = check.get("has_actor", false);
		bool has_map = check.get("has_map", false);
		bool has_start_pos = check.get("has_start_pos", false);

		String msg = String::utf8("No se puede iniciar el juego. Faltan los siguientes requisitos obligatorios:\n\n");
		if (!has_actor) {
			msg += String::utf8("• FALTA ACTOR: No hay ningún Actor creado en la Base de Datos.\n  Solución: Abre Base de Datos (F9) > Actores y crea al menos un personaje.\n\n");
		}
		if (!has_map) {
			msg += String::utf8("• FALTA MAPA INICIAL: No hay ningún mapa guardado en el proyecto.\n  Solución: Crea un mapa en el Árbol de Mapas (panel izquierdo).\n\n");
		}
		if (!has_start_pos) {
			msg += String::utf8("• FALTA POSICIÓN INICIAL DEL JUGADOR: No se ha elegido dónde empieza el jugador.\n  Solución: Selecciona la herramienta 'Posición Inicial del Jugador' (icono de banderín en la barra superior del mapa) y haz clic en una casilla del mapa.\n\n");
		}
		msg += String::utf8("Regla Cero: ClockGears Engine nunca inventa contenido no definido por el autor.");
		print_line("[ClockGears Playtest] ARRANQUE CANCELADO:\n" + msg);

		if (playtest_warning_dialog) {
			playtest_warning_dialog->set_text(msg);
			playtest_warning_dialog->popup_centered(Size2(560, 270) * EDSCALE);
		}
		return false;
	}

	String target_map_path = check.get("target_map", "");
	Array actors = dm->get_category_array(CAT_ACTORS);

	// Asegurar que el grupo inicial tenga al menos al primer actor si estaba vacío
	Dictionary sys = dm->get_system_config();
	Array party = sys.get("party", Array());
	if (party.is_empty() && !actors.is_empty()) {
		Dictionary first_actor = actors[0];
		party.push_back((int)first_actor.get("id", 1));
		sys["party"] = party;
	}
	if (String(sys.get("start_map", "")).is_empty()) {
		sys["start_map"] = target_map_path;
	}
	dm->set_system_config(sys);

	// Persistir Base de Datos JSON antes de lanzar el proceso de juego
	dm->save_all("res://clockgears_data");

	if (ProjectSettings::get_singleton()) {
		ProjectSettings::get_singleton()->set_setting("application/run/main_scene", target_map_path);
		ProjectSettings::get_singleton()->save();
	}

	// Cerrar instancia previa de prueba si seguía abierta
	if (playtest_pid != 0 && OS::get_singleton()->is_process_running(playtest_pid)) {
		OS::get_singleton()->kill(playtest_pid);
		playtest_pid = 0;
	}

	// Lanzar ventana independiente de juego con ruta real sin corromper espacios (%20) ni ocultar en GameView
	List<String> args;
	String resource_path = ProjectSettings::get_singleton()->get_resource_path();
	if (!resource_path.is_empty()) {
		args.push_back("--path");
		args.push_back(resource_path);
	}
	args.push_back("--position");
	args.push_back("120,80");
	args.push_back("--clockgears-playtest");
	args.push_back(target_map_path);

	print_line(vformat("[ClockGears Playtest] Lanzando juego: ejecutable='%s' --path '%s' escena='%s'",
			OS::get_singleton()->get_executable_path(), resource_path, target_map_path));

	Error err = OS::get_singleton()->create_instance(args, &playtest_pid);
	if (err != OK) {
		String err_msg = vformat(String::utf8("Error al lanzar el proceso del juego (código %d)."), (int)err);
		ERR_PRINT("[ClockGears Playtest] " + err_msg);
		if (playtest_warning_dialog) {
			playtest_warning_dialog->set_text(err_msg);
			playtest_warning_dialog->popup_centered(Size2(480, 180) * EDSCALE);
		}
		return false;
	}

	print_line(vformat("[ClockGears Playtest] Juego iniciado correctamente (PID: %d).", (int)playtest_pid));
	return true;
}

void ClockGearsEditorPlugin::_on_create_missing_folders_confirmed() {
	ClockGearsProjectStructure::create_missing_folders("res://");
}

void ClockGearsEditorPlugin::_check_missing_project_folders() {
	PackedStringArray missing = ClockGearsProjectStructure::get_missing_folders("res://");
	if (missing.is_empty() || !missing_folders_dialog) {
		return;
	}
	String preview_list;
	int show_max = MIN(8, missing.size());
	for (int i = 0; i < show_max; i++) {
		preview_list += String::utf8("  • res://") + missing[i] + "\n";
	}
	if (missing.size() > show_max) {
		preview_list += vformat(CG_TR("  ... y %d carpetas más.\n"), missing.size() - show_max);
	}
	missing_folders_dialog->set_text(vformat(
			CG_TR("Al proyecto actual le faltan %d carpetas estándar de ClockGears Engine:\n\n%s\n¿Deseas crearlas ahora junto con LEEME.txt? (No se borrará ni sobrescribirá ningún archivo existente)."),
			missing.size(), preview_list));
	missing_folders_dialog->popup_centered();
}

void ClockGearsEditorPlugin::apply_clockgears_workspace() {
	EditorNode *editor = EditorNode::get_singleton();
	if (!editor) {
		return;
	}

	// Permanently strip the stock Godot shell and lock the 2D map viewport.
	editor->set_clockgears_workspace(true);

	// Desactivar los atajos F5/F6 nativos de EditorRunBar para que F5 siempre pase por ClockGearsEditorPlugin::start_playtest()
	if (EditorSettings::get_singleton()) {
		Ref<Shortcut> sc_run = ED_GET_SHORTCUT("editor/run_project");
		if (sc_run.is_valid()) {
			sc_run->set_events(Array());
		}
		Ref<Shortcut> sc_cur = ED_GET_SHORTCUT("editor/run_current_scene");
		if (sc_cur.is_valid()) {
			sc_cur->set_events(Array());
		}
	}

	VBoxContainer *root = editor->get_clockgears_workspace_root();
	if (root && top_bar) {
		top_bar->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
		if (top_bar->get_parent() != root) {
			if (top_bar->get_parent()) {
				top_bar->reparent(root);
			} else {
				root->add_child(top_bar);
			}
		}
		// Position directly below title_bar
		root->move_child(top_bar, 1);
	}

	if (workspace_built) {
		return;
	}

	// ClockGearsBody is created directly by EditorNode's constructor (zero flicker):
	// the native dock shell already lives in a hidden holder, so we only need to
	// mount the left Map/Tile panel at index 0 of the body.
	HSplitContainer *body = editor->get_clockgears_body();
	if (!root || !body || !map_panel) {
		return;
	}

	// Ensure all children of root other than body do not expand vertically
	for (int i = 0; i < root->get_child_count(); i++) {
		Control *c = Object::cast_to<Control>(root->get_child(i));
		if (c && c != body) {
			c->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
		}
	}
	body->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	// Left column: fixed ~300px (fills vertically, does not expand horizontally).
	map_panel->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	map_panel->set_h_size_flags(Control::SIZE_FILL);
	map_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	if (map_panel->get_parent() != body) {
		if (map_panel->get_parent()) {
			map_panel->reparent(body, false);
		} else {
			body->add_child(map_panel);
		}
	}
	body->move_child(map_panel, 0);
	body->set_split_offset(0);

	if (CanvasItemEditor::get_singleton()) {
		CanvasItemEditor::get_singleton()->set_clockgears_event_double_clicked_callback(
				callable_mp(this, &ClockGearsEditorPlugin::open_event_editor_at_cell));
		CanvasItemEditor::get_singleton()->set_clockgears_event_cell_rmb_callback(
				callable_mp(this, &ClockGearsEditorPlugin::show_event_context_menu));
	}
	editor->set_clockgears_database_callback(callable_mp(this, &ClockGearsEditorPlugin::open_database));
	editor->set_clockgears_resource_manager_callback(callable_mp(this, &ClockGearsEditorPlugin::open_resource_manager));
	editor->set_clockgears_new_project_callback(callable_mp(this, &ClockGearsEditorPlugin::open_new_project));
	editor->set_clockgears_export_callback(callable_mp(this, &ClockGearsEditorPlugin::open_export));
	editor->set_clockgears_preferences_callback(callable_mp(this, &ClockGearsEditorPlugin::open_preferences));
	editor->set_clockgears_camera_callback(callable_mp(this, &ClockGearsEditorPlugin::open_camera_dialog));

	if (ClockGearsTheme::get_singleton()) {
		ClockGearsTheme *th = ClockGearsTheme::get_singleton();
		th->apply_to_window(database_dialog);
		th->apply_to_window(event_editor_dialog);
		th->apply_to_window(export_dialog);
		th->apply_to_window(preferences_dialog);
		th->apply_to_window(camera_dialog);
		th->apply_to_window(new_project_dialog);
		th->apply_to_window(resource_manager_dialog);
		th->apply_to_window(missing_folders_dialog);
		th->apply_to_window(playtest_warning_dialog);
		th->apply_to_window(event_context_menu);
		th->apply_to_window(quick_events_menu);
		th->apply_to_control(top_bar);
		th->apply_to_control(map_panel);
	}

	workspace_built = true;
	if (map_panel) {
		map_panel->refresh_maps_from_disk();
	}
	// Requisito 6: Si al proyecto abierto le faltan carpetas estándar, ofrecer crearlas sin borrar nada.
	callable_mp(this, &ClockGearsEditorPlugin::_check_missing_project_folders).call_deferred();
	print_verbose("ClockGears exclusive organic workspace installed.");
}

ClockGearsEditorPlugin::ClockGearsEditorPlugin() {
	singleton = this;

	// Instantiate dialogs
	database_dialog = memnew(ClockGearsDatabaseDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(database_dialog);

	event_editor_dialog = memnew(ClockGearsEventEditorDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(event_editor_dialog);

	export_dialog = memnew(ClockGearsSimplifiedExportDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(export_dialog);

	preferences_dialog = memnew(ClockGearsPreferencesDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(preferences_dialog);

	camera_dialog = memnew(ClockGearsCameraDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(camera_dialog);

	new_project_dialog = memnew(ClockGearsNewProjectDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(new_project_dialog);

	resource_manager_dialog = memnew(ClockGearsResourceManagerDialog);
	EditorNode::get_singleton()->get_gui_base()->add_child(resource_manager_dialog);

	missing_folders_dialog = memnew(ConfirmationDialog);
	missing_folders_dialog->set_title(CG_TR("Estructura de Carpetas de ClockGears Engine"));
	missing_folders_dialog->set_ok_button_text(CG_TR("Crear Carpetas Faltantes"));
	missing_folders_dialog->set_cancel_button_text(CG_TR("Ahora No"));
	missing_folders_dialog->connect("confirmed", callable_mp(this, &ClockGearsEditorPlugin::_on_create_missing_folders_confirmed));
	EditorNode::get_singleton()->get_gui_base()->add_child(missing_folders_dialog);

	playtest_warning_dialog = memnew(AcceptDialog);
	playtest_warning_dialog->set_title(String::utf8("Aviso de Prueba de Juego - Regla Cero"));
	playtest_warning_dialog->set_ok_button_text(String::utf8("Entendido"));
	EditorNode::get_singleton()->get_gui_base()->add_child(playtest_warning_dialog);

	// Event cell context menu
	event_context_menu = memnew(PopupMenu);
	event_context_menu->connect("id_pressed", callable_mp(this, &ClockGearsEditorPlugin::_on_event_context_menu_id_pressed));
	EditorNode::get_singleton()->get_gui_base()->add_child(event_context_menu);

	quick_events_menu = memnew(PopupMenu);
	quick_events_menu->set_name("QuickEventsSubmenu");
	quick_events_menu->connect("id_pressed", callable_mp(this, &ClockGearsEditorPlugin::_on_quick_event_id_pressed));
	event_context_menu->add_child(quick_events_menu);

	// Instantiate Top Bar
	top_bar = memnew(ClockGearsTopBar);
	top_bar->set_dialogs(database_dialog, event_editor_dialog, export_dialog);

	map_panel = memnew(ClockGearsMapPanel);

	// Mount the ClockGears chrome immediately (before the first frame) to avoid any flicker.
	{
		EditorNode *en = EditorNode::get_singleton();
		VBoxContainer *root = en->get_clockgears_workspace_root();
		HSplitContainer *body = en->get_clockgears_body();
		if (root) {
			top_bar->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
			root->add_child(top_bar);
			root->move_child(top_bar, MIN(1, root->get_child_count() - 1));
		}
		if (body) {
			map_panel->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
			map_panel->set_h_size_flags(Control::SIZE_FILL);
			map_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			body->add_child(map_panel);
			body->move_child(map_panel, 0);
		}
	}
	EditorNode::get_singleton()->add_clockgears_layout_loaded_callback(callable_mp(this, &ClockGearsEditorPlugin::apply_clockgears_workspace));

	// Main screens
	automation_panel = memnew(ClockGearsAutomationPanel);
	automation_panel->set_name(String::utf8("Herramientas de Automatización"));
	EditorInterface::get_singleton()->get_editor_main_screen()->add_child(automation_panel);
	automation_panel->hide();

	ui_editor_panel = memnew(ClockGearsUIEditorPanel);
	ui_editor_panel->set_name(String::utf8("Diseñador UI/HUD"));
	EditorInterface::get_singleton()->get_editor_main_screen()->add_child(ui_editor_panel);
	ui_editor_panel->hide();
}

ClockGearsEditorPlugin::~ClockGearsEditorPlugin() {
	singleton = nullptr;
}
