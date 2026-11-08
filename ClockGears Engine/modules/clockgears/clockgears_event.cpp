#include "clockgears_event.h"
#include "core/clockgears_data_manager.h"
#include "core/config/engine.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/texture.h"
#include "scene/theme/theme_db.h"

void ClockGearsEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_event_id", "id"), &ClockGearsEvent::set_event_id);
	ClassDB::bind_method(D_METHOD("get_event_id"), &ClockGearsEvent::get_event_id);

	ClassDB::bind_method(D_METHOD("set_event_name", "name"), &ClockGearsEvent::set_event_name);
	ClassDB::bind_method(D_METHOD("get_event_name"), &ClockGearsEvent::get_event_name);

	ClassDB::bind_method(D_METHOD("set_trigger_type", "trigger"), &ClockGearsEvent::set_trigger_type);
	ClassDB::bind_method(D_METHOD("get_trigger_type"), &ClockGearsEvent::get_trigger_type);

	ClassDB::bind_method(D_METHOD("set_move_type", "move_type"), &ClockGearsEvent::set_move_type);
	ClassDB::bind_method(D_METHOD("get_move_type"), &ClockGearsEvent::get_move_type);

	ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &ClockGearsEvent::set_move_speed);
	ClassDB::bind_method(D_METHOD("get_move_speed"), &ClockGearsEvent::get_move_speed);

	ClassDB::bind_method(D_METHOD("set_move_frequency", "freq"), &ClockGearsEvent::set_move_frequency);
	ClassDB::bind_method(D_METHOD("get_move_frequency"), &ClockGearsEvent::get_move_frequency);

	ClassDB::bind_method(D_METHOD("set_through", "through"), &ClockGearsEvent::set_through);
	ClassDB::bind_method(D_METHOD("is_through"), &ClockGearsEvent::is_through);

	ClassDB::bind_method(D_METHOD("set_direction_fix", "fix"), &ClockGearsEvent::set_direction_fix);
	ClassDB::bind_method(D_METHOD("is_direction_fix"), &ClockGearsEvent::is_direction_fix);

	ClassDB::bind_method(D_METHOD("set_direction", "dir"), &ClockGearsEvent::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &ClockGearsEvent::get_direction);

	ClassDB::bind_method(D_METHOD("set_graphic_path", "path"), &ClockGearsEvent::set_graphic_path);
	ClassDB::bind_method(D_METHOD("get_graphic_path"), &ClockGearsEvent::get_graphic_path);

	ClassDB::bind_method(D_METHOD("set_sprite_hframes", "hframes"), &ClockGearsEvent::set_sprite_hframes);
	ClassDB::bind_method(D_METHOD("get_sprite_hframes"), &ClockGearsEvent::get_sprite_hframes);
	ClassDB::bind_method(D_METHOD("set_sprite_vframes", "vframes"), &ClockGearsEvent::set_sprite_vframes);
	ClassDB::bind_method(D_METHOD("get_sprite_vframes"), &ClockGearsEvent::get_sprite_vframes);
	ClassDB::bind_method(D_METHOD("set_sprite_frame", "frame"), &ClockGearsEvent::set_sprite_frame);
	ClassDB::bind_method(D_METHOD("get_sprite_frame"), &ClockGearsEvent::get_sprite_frame);

	ClassDB::bind_method(D_METHOD("set_condition_switch_1_enabled", "enabled"), &ClockGearsEvent::set_condition_switch_1_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_switch_1_enabled"), &ClockGearsEvent::is_condition_switch_1_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_switch_1_name", "name"), &ClockGearsEvent::set_condition_switch_1_name);
	ClassDB::bind_method(D_METHOD("get_condition_switch_1_name"), &ClockGearsEvent::get_condition_switch_1_name);

	ClassDB::bind_method(D_METHOD("set_condition_switch_2_enabled", "enabled"), &ClockGearsEvent::set_condition_switch_2_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_switch_2_enabled"), &ClockGearsEvent::is_condition_switch_2_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_switch_2_name", "name"), &ClockGearsEvent::set_condition_switch_2_name);
	ClassDB::bind_method(D_METHOD("get_condition_switch_2_name"), &ClockGearsEvent::get_condition_switch_2_name);

	ClassDB::bind_method(D_METHOD("set_condition_self_switch_enabled", "enabled"), &ClockGearsEvent::set_condition_self_switch_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_self_switch_enabled"), &ClockGearsEvent::is_condition_self_switch_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_self_switch_letter", "letter"), &ClockGearsEvent::set_condition_self_switch_letter);
	ClassDB::bind_method(D_METHOD("get_condition_self_switch_letter"), &ClockGearsEvent::get_condition_self_switch_letter);

	ClassDB::bind_method(D_METHOD("set_self_switch", "letter", "val"), &ClockGearsEvent::set_self_switch);
	ClassDB::bind_method(D_METHOD("get_self_switch", "letter"), &ClockGearsEvent::get_self_switch);
	ClassDB::bind_method(D_METHOD("get_self_switches"), &ClockGearsEvent::get_self_switches);
	ClassDB::bind_method(D_METHOD("set_self_switches", "switches"), &ClockGearsEvent::set_self_switches);

	ClassDB::bind_method(D_METHOD("get_active_page_index"), &ClockGearsEvent::get_active_page_index);
	ClassDB::bind_method(D_METHOD("update_active_page"), &ClockGearsEvent::update_active_page);

	ClassDB::bind_method(D_METHOD("set_condition_var_enabled", "enabled"), &ClockGearsEvent::set_condition_var_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_var_enabled"), &ClockGearsEvent::is_condition_var_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_var_name", "name"), &ClockGearsEvent::set_condition_var_name);
	ClassDB::bind_method(D_METHOD("get_condition_var_name"), &ClockGearsEvent::get_condition_var_name);
	ClassDB::bind_method(D_METHOD("set_condition_var_value", "value"), &ClockGearsEvent::set_condition_var_value);
	ClassDB::bind_method(D_METHOD("get_condition_var_value"), &ClockGearsEvent::get_condition_var_value);

	ClassDB::bind_method(D_METHOD("set_condition_item_enabled", "enabled"), &ClockGearsEvent::set_condition_item_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_item_enabled"), &ClockGearsEvent::is_condition_item_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_item_name", "name"), &ClockGearsEvent::set_condition_item_name);
	ClassDB::bind_method(D_METHOD("get_condition_item_name"), &ClockGearsEvent::get_condition_item_name);

	ClassDB::bind_method(D_METHOD("set_condition_actor_enabled", "enabled"), &ClockGearsEvent::set_condition_actor_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_actor_enabled"), &ClockGearsEvent::is_condition_actor_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_actor_name", "name"), &ClockGearsEvent::set_condition_actor_name);
	ClassDB::bind_method(D_METHOD("get_condition_actor_name"), &ClockGearsEvent::get_condition_actor_name);

	ClassDB::bind_method(D_METHOD("set_priority_type", "priority"), &ClockGearsEvent::set_priority_type);
	ClassDB::bind_method(D_METHOD("get_priority_type"), &ClockGearsEvent::get_priority_type);

	ClassDB::bind_method(D_METHOD("set_commands", "commands"), &ClockGearsEvent::set_commands);
	ClassDB::bind_method(D_METHOD("get_commands"), &ClockGearsEvent::get_commands);

	ClassDB::bind_method(D_METHOD("set_pages", "pages"), &ClockGearsEvent::set_pages);
	ClassDB::bind_method(D_METHOD("get_pages"), &ClockGearsEvent::get_pages);

	ClassDB::bind_method(D_METHOD("add_command", "command"), &ClockGearsEvent::add_command);
	ClassDB::bind_method(D_METHOD("clear_commands"), &ClockGearsEvent::clear_commands);
	ClassDB::bind_method(D_METHOD("serialize_to_meta"), &ClockGearsEvent::serialize_to_meta);
	ClassDB::bind_method(D_METHOD("deserialize_from_meta"), &ClockGearsEvent::deserialize_from_meta);
	ClassDB::bind_method(D_METHOD("execute"), &ClockGearsEvent::execute);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "event_id"), "set_event_id", "get_event_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "event_name"), "set_event_name", "get_event_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "trigger_type"), "set_trigger_type", "get_trigger_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_type"), "set_move_type", "get_move_type");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_speed"), "set_move_speed", "get_move_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_frequency"), "set_move_frequency", "get_move_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "through"), "set_through", "is_through");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "direction_fix"), "set_direction_fix", "is_direction_fix");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "direction"), "set_direction", "get_direction");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority_type"), "set_priority_type", "get_priority_type");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "graphic_path"), "set_graphic_path", "get_graphic_path");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sprite_hframes"), "set_sprite_hframes", "get_sprite_hframes");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sprite_vframes"), "set_sprite_vframes", "get_sprite_vframes");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sprite_frame"), "set_sprite_frame", "get_sprite_frame");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "commands"), "set_commands", "get_commands");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "pages"), "set_pages", "get_pages");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_switch_1_enabled"), "set_condition_switch_1_enabled", "is_condition_switch_1_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_switch_1_name"), "set_condition_switch_1_name", "get_condition_switch_1_name");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_switch_2_enabled"), "set_condition_switch_2_enabled", "is_condition_switch_2_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_switch_2_name"), "set_condition_switch_2_name", "get_condition_switch_2_name");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_self_switch_enabled"), "set_condition_self_switch_enabled", "is_condition_self_switch_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_self_switch_letter"), "set_condition_self_switch_letter", "get_condition_self_switch_letter");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "self_switches"), "set_self_switches", "get_self_switches");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "active_page_index"), "", "get_active_page_index");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_var_enabled"), "set_condition_var_enabled", "is_condition_var_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_var_name"), "set_condition_var_name", "get_condition_var_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "condition_var_value"), "set_condition_var_value", "get_condition_var_value");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_item_enabled"), "set_condition_item_enabled", "is_condition_item_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_item_name"), "set_condition_item_name", "get_condition_item_name");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_actor_enabled"), "set_condition_actor_enabled", "is_condition_actor_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_actor_name"), "set_condition_actor_name", "get_condition_actor_name");

	ADD_SIGNAL(MethodInfo("event_triggered", PropertyInfo(Variant::OBJECT, "event")));
}

void ClockGearsEvent::serialize_to_meta() {
	set_meta("clockgears_event", true);
	set_meta("event_id", event_id);
	set_meta("event_name", event_name);
	set_meta("trigger_type", trigger_type);
	set_meta("move_type", move_type);
	set_meta("move_speed", move_speed);
	set_meta("move_frequency", move_frequency);
	set_meta("through", through);
	set_meta("direction_fix", direction_fix);
	set_meta("direction", direction);
	set_meta("priority_type", priority_type);
	set_meta("graphic_path", graphic_path);
	set_meta("sprite_hframes", sprite_hframes);
	set_meta("sprite_vframes", sprite_vframes);
	set_meta("sprite_frame", sprite_frame);
	set_meta("commands", commands.duplicate(true));
	set_meta("pages", pages.duplicate(true));
	set_meta("condition_switch_1_enabled", condition_switch_1_enabled);
	set_meta("condition_switch_1_name", condition_switch_1_name);
	set_meta("condition_switch_2_enabled", condition_switch_2_enabled);
	set_meta("condition_switch_2_name", condition_switch_2_name);
	set_meta("condition_self_switch_enabled", condition_self_switch_enabled);
	set_meta("condition_self_switch_letter", condition_self_switch_letter);
	set_meta("self_switches", self_switches.duplicate(true));
	set_meta("condition_var_enabled", condition_var_enabled);
	set_meta("condition_var_name", condition_var_name);
	set_meta("condition_var_value", condition_var_value);
	set_meta("condition_item_enabled", condition_item_enabled);
	set_meta("condition_item_name", condition_item_name);
	set_meta("condition_actor_enabled", condition_actor_enabled);
	set_meta("condition_actor_name", condition_actor_name);
}

void ClockGearsEvent::deserialize_from_meta() {
	if (has_meta("event_id")) event_id = get_meta("event_id");
	if (has_meta("event_name")) event_name = get_meta("event_name");
	if (has_meta("trigger_type")) trigger_type = get_meta("trigger_type");
	if (has_meta("move_type")) move_type = get_meta("move_type");
	if (has_meta("move_speed")) move_speed = get_meta("move_speed");
	if (has_meta("move_frequency")) move_frequency = get_meta("move_frequency");
	if (has_meta("through")) through = get_meta("through");
	if (has_meta("direction_fix")) direction_fix = get_meta("direction_fix");
	if (has_meta("direction")) direction = get_meta("direction");
	if (has_meta("priority_type")) priority_type = get_meta("priority_type");
	if (has_meta("sprite_hframes")) sprite_hframes = MAX(1, int(get_meta("sprite_hframes")));
	if (has_meta("sprite_vframes")) sprite_vframes = MAX(1, int(get_meta("sprite_vframes")));
	if (has_meta("sprite_frame")) sprite_frame = MAX(0, int(get_meta("sprite_frame")));
	if (has_meta("graphic_path")) set_graphic_path(get_meta("graphic_path"));
	if (has_meta("commands")) commands = get_meta("commands");
	if (has_meta("pages")) pages = get_meta("pages");
	if (has_meta("condition_switch_1_enabled")) condition_switch_1_enabled = get_meta("condition_switch_1_enabled");
	if (has_meta("condition_switch_1_name")) condition_switch_1_name = get_meta("condition_switch_1_name");
	if (has_meta("condition_switch_2_enabled")) condition_switch_2_enabled = get_meta("condition_switch_2_enabled");
	if (has_meta("condition_switch_2_name")) condition_switch_2_name = get_meta("condition_switch_2_name");
	if (has_meta("condition_self_switch_enabled")) condition_self_switch_enabled = get_meta("condition_self_switch_enabled");
	if (has_meta("condition_self_switch_letter")) condition_self_switch_letter = get_meta("condition_self_switch_letter");
	if (has_meta("self_switches")) self_switches = get_meta("self_switches");
	if (has_meta("condition_var_enabled")) condition_var_enabled = get_meta("condition_var_enabled");
	if (has_meta("condition_var_name")) condition_var_name = get_meta("condition_var_name");
	if (has_meta("condition_var_value")) condition_var_value = get_meta("condition_var_value");
	if (has_meta("condition_item_enabled")) condition_item_enabled = get_meta("condition_item_enabled");
	if (has_meta("condition_item_name")) condition_item_name = get_meta("condition_item_name");
	if (has_meta("condition_actor_enabled")) condition_actor_enabled = get_meta("condition_actor_enabled");
	if (has_meta("condition_actor_name")) condition_actor_name = get_meta("condition_actor_name");
}

void ClockGearsEvent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_READY: {
			if (has_meta("clockgears_event")) {
				deserialize_from_meta();
			}
			if (!pages.is_empty()) {
				update_active_page();
			} else if (!graphic_path.is_empty()) {
				set_graphic_path(graphic_path);
			}
			queue_redraw();
		} break;
		case NOTIFICATION_DRAW: {
			// Marcadores de evento (EV_0): No se dibujan en el juego por defecto (Punto 1).
			// Solo se dibuja el sprite si lo tiene. La visualización de depuración se activa con F3.
		} break;
	}
}

void ClockGearsEvent::_update_sprite_region() {
	if (!sprite) {
		return;
	}
	int hf = MAX(1, sprite_hframes);
	int vf = MAX(1, sprite_vframes);
	sprite->set_hframes(hf);
	sprite->set_vframes(vf);
	sprite->set_frame(CLAMP(sprite_frame, 0, hf * vf - 1));
	if (sprite->get_texture().is_valid()) {
		int frame_h = sprite->get_texture()->get_height() / vf;
		if (frame_h > 32) {
			sprite->set_position(Vector2(0, 32 - frame_h));
		} else {
			sprite->set_position(Vector2(0, 0));
		}
	}
}

void ClockGearsEvent::set_graphic_path(const String &p_path) {
	graphic_path = p_path;
	if (!sprite) {
		return;
	}
	if (p_path.is_empty()) {
		sprite->set_texture(Ref<Texture2D>());
		return;
	}
	Ref<Texture2D> tex;
	if (ResourceLoader::exists(p_path)) {
		tex = ResourceLoader::load(p_path);
	}
	if (tex.is_null()) {
		Ref<Image> img = Image::load_from_file(p_path);
		if (img.is_valid() && !img->is_empty()) {
			tex = ImageTexture::create_from_image(img);
		}
	}
	if (tex.is_valid()) {
		sprite->set_texture(tex);
		_update_sprite_region();
	}
}

void ClockGearsEvent::set_sprite_hframes(int p_hframes) {
	sprite_hframes = MAX(1, p_hframes);
	_update_sprite_region();
}

void ClockGearsEvent::set_sprite_vframes(int p_vframes) {
	sprite_vframes = MAX(1, p_vframes);
	_update_sprite_region();
}

void ClockGearsEvent::set_sprite_frame(int p_frame) {
	sprite_frame = MAX(0, p_frame);
	_update_sprite_region();
}

void ClockGearsEvent::add_command(const Dictionary &p_cmd) {
	commands.push_back(p_cmd);
}

void ClockGearsEvent::set_self_switch(const String &p_letter, bool p_val) {
	self_switches[p_letter] = p_val;
	update_active_page();
}

bool ClockGearsEvent::get_self_switch(const String &p_letter) const {
	if (self_switches.has(p_letter)) {
		return bool(self_switches[p_letter]);
	}
	return false;
}

void ClockGearsEvent::set_self_switches(const Dictionary &p_sw) {
	self_switches = p_sw.duplicate(true);
	update_active_page();
}

void ClockGearsEvent::update_active_page() {
	if (pages.is_empty()) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();

	int matched_page = -1;
	for (int i = pages.size() - 1; i >= 0; i--) {
		Dictionary p = pages[i];
		bool ok = true;

		// Check Switch 1
		if (bool(p.get("sw1_en", false))) {
			String sw_name = p.get("sw1_name", "");
			if (!dm || !dm->get_switch(sw_name)) {
				ok = false;
			}
		}
		// Check Switch 2
		if (ok && bool(p.get("sw2_en", false))) {
			String sw_name = p.get("sw2_name", "");
			if (!dm || !dm->get_switch(sw_name)) {
				ok = false;
			}
		}
		// Check Self Switch (A, B, C, D)
		if (ok && bool(p.get("self_sw_en", false))) {
			String s_letter = p.get("self_sw_letter", "A");
			if (!get_self_switch(s_letter)) {
				ok = false;
			}
		}
		// Check Variable (>= val)
		if (ok && bool(p.get("var_en", false))) {
			String var_name = p.get("var_name", "");
			int target_val = int(p.get("var_val", 0));
			int cur_val = dm ? dm->get_variable(var_name) : 0;
			if (cur_val < target_val) {
				ok = false;
			}
		}
		// Check Item
		if (ok && bool(p.get("item_en", false))) {
			String item_name = p.get("item_name", "");
			if (!dm || !dm->has_item(item_name)) {
				ok = false;
			}
		}

		if (ok) {
			matched_page = i;
			break;
		}
	}

	if (matched_page < 0) {
		matched_page = 0; // Default to page 0
	}

	active_page_index = matched_page;
	Dictionary ap = pages[matched_page];

	// Apply properties from active page
	set_trigger_type(int(ap.get("trigger", 0)));
	set_priority_type(int(ap.get("priority", 1)));
	set_through(ap.get("through", false));
	set_direction_fix(ap.get("dir_fix", false));

	set_sprite_hframes(int(ap.get("hframes", 1)));
	set_sprite_vframes(int(ap.get("vframes", 1)));
	set_sprite_frame(int(ap.get("frame", 0)));
	set_graphic_path(ap.get("graphic_path", ""));

	set_move_type(int(ap.get("move_type", 0)));
	set_move_speed(int(ap.get("speed", 3)) * 40.0f);
	set_move_frequency(int(ap.get("freq", 3)));
	set_commands(ap.get("commands", Array()));
	queue_redraw();
}

void ClockGearsEvent::execute() {
	emit_signal("event_triggered", this);
	print_line(vformat(String::utf8("[ClockGears Event] Ejecutando Evento '%s' (ID: %d) [Página activa: %d] con %d comandos."),
			event_name, event_id, active_page_index + 1, commands.size()));

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();

	for (int i = 0; i < commands.size(); i++) {
		Dictionary cmd = commands[i];
		String type = cmd.get("type", "");

		if (type == "message") {
			String raw_txt = cmd.get("text", "");
			String formatted = dm ? dm->format_text(raw_txt) : raw_txt;
			String face = cmd.get("face", "");
			print_line(vformat("  [Mensaje] %s%s", face.is_empty() ? "" : vformat("(Cara: %s) ", face.get_file()), formatted));
		} else if (type == "switch") {
			String sw_name = cmd.get("name", "");
			bool val = cmd.get("val", true);
			if (dm) {
				dm->set_switch(sw_name, val);
			}
			print_line(vformat("  [Interruptor] %s = %s", sw_name, val ? "ON" : "OFF"));
		} else if (type == "self_switch") {
			String letter = cmd.get("letter", "A");
			bool val = cmd.get("val", true);
			set_self_switch(letter, val);
			print_line(vformat("  [Interruptor Local] %s = %s", letter, val ? "ON" : "OFF"));
		} else if (type == "variable") {
			String var_name = cmd.get("name", "");
			String op = cmd.get("op", "=");
			int val = int(cmd.get("val", 0));
			if (dm) {
				int cur = dm->get_variable(var_name);
				if (op == "+") cur += val;
				else if (op == "-") cur -= val;
				else if (op == "*") cur *= val;
				else if (op == "/" && val != 0) cur /= val;
				else if (op == "%" && val != 0) cur %= val;
				else cur = val;
				dm->set_variable(var_name, cur);
				print_line(vformat("  [Variable] %s %s %d (Nuevo valor: %d)", var_name, op, val, cur));
			}
		} else if (type == "wait") {
			int frames = int(cmd.get("frames", 60));
			print_line(vformat("  [Esperar] %d fotogramas", frames));
		} else if (type == "teleport") {
			String map_dest = cmd.get("map", "");
			int tx = int(cmd.get("x", 0));
			int ty = int(cmd.get("y", 0));
			String dir = cmd.get("dir", "");
			print_line(vformat("  [Transferir] Mapa '%s' a (%d, %d) mirando %s", map_dest, tx, ty, dir));
		} else if (type == "move_route") {
			String target = cmd.get("target", "Este Evento");
			String action = cmd.get("action", "");
			print_line(vformat("  [Fijar Ruta] Objetivo: %s -> %s", target, action));
		} else if (type == "item") {
			String iname = cmd.get("name", "");
			String op = cmd.get("op", "+");
			int qty = int(cmd.get("qty", 1));
			if (dm) {
				dm->change_item(iname, (op == "-") ? -qty : qty);
			}
			print_line(vformat("  [Objeto] %s%d %s", op, qty, iname));
		} else if (type == "gold") {
			String op = cmd.get("op", "+");
			int amount = int(cmd.get("amount", 0));
			if (dm) {
				dm->change_gold((op == "-") ? -amount : amount);
			}
			print_line(vformat("  [Dinero] %s%d Oro", op, amount));
		} else if (type == "hp_mp") {
			String stat = cmd.get("stat", "HP");
			String op = cmd.get("op", "+");
			int amt = int(cmd.get("amount", 50));
			print_line(vformat("  [Cambiar %s] %s%d", stat, op, amt));
		} else if (type == "audio" || type == "bgm" || type == "se") {
			String afile = cmd.get("file", "");
			print_line(vformat("  [Audio] Reproduciendo '%s'", afile));
		} else if (type == "flash") {
			print_line(vformat("  [Flash] Color %s (%d f)", cmd.get("color", ""), int(cmd.get("duration", 30))));
		} else if (type == "tint") {
			print_line(vformat(String::utf8("  [Teñir] %s (%d f)"), cmd.get("preset", ""), int(cmd.get("duration", 60))));
		} else if (type == "fade") {
			print_line(vformat("  [Fundido] %s (%d f)", cmd.get("mode", "Oscurecer"), int(cmd.get("duration", 30))));
		} else if (type == "battle") {
			print_line(vformat("  [Batalla] Iniciando combate contra tropa '%s'", cmd.get("troop", "")));
		}
	}
}

ClockGearsEvent::ClockGearsEvent() {
	sprite = memnew(Sprite2D);
	sprite->set_centered(false);
	sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	add_child(sprite);
}
