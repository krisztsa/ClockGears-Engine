#include "clockgears_data_manager.h"
#include "../clockgears_animation.h"
#include "../clockgears_battle.h"
#include "clockgears_id_registry.h"
#include "clockgears_system_catalog.h"
#include "clockgears_time_manager.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/math/math_funcs.h"
#include "core/os/time.h"
#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#endif

ClockGearsDataManager *ClockGearsDataManager::singleton = nullptr;

String ClockGearsDataManager::get_category_key(int p_category) {
	switch (p_category) {
		case CAT_ACTORS: return "actors";
		case CAT_CLASSES: return "classes";
		case CAT_SKILLS: return "skills";
		case CAT_ITEMS: return "items";
		case CAT_WEAPONS: return "weapons";
		case CAT_ARMORS: return "armors";
		case CAT_ACCESSORIES: return "accessories";
		case CAT_ENEMIES: return "enemies";
		case CAT_TROOPS: return "troops";
		case CAT_STATES: return "states";
		case CAT_FLORA: return "flora";
		case CAT_FAUNA: return "fauna";
		case CAT_ANIMATIONS: return "animations";
		case CAT_TILESETS: return "tilesets";
		case CAT_COMMON_EVENTS: return "common_events";
		case CAT_SYSTEM: return "system";
		case CAT_TYPES: return "types";
		case CAT_VOCABULARY: return "vocabulary";
		case CAT_SWITCHES_VARS: return "switches_vars";
		case CAT_NPCS: return "npcs";
		case CAT_ARPG_ENEMIES: return "arpg_enemies";
		case CAT_ARPG_NPCS: return "arpg_npcs";
		case CAT_ARPG_WEAPONS: return "arpg_weapons";
		case CAT_ARPG_ARMORS: return "arpg_armors";
		case CAT_ARPG_SHIELDS: return "arpg_shields";
		case CAT_ARPG_ACCESSORIES: return "arpg_accessories";
		default: return "unknown";
	}
}

String ClockGearsDataManager::get_category_filename(int p_category) {
	return get_category_key(p_category) + ".json";
}

int ClockGearsDataManager::get_category_from_key(const String &p_key) {
	String k = p_key.to_lower().strip_edges();
	for (int i = 0; i < CAT_MAX; i++) {
		if (get_category_key(i) == k) {
			return i;
		}
	}
	return -1;
}

void ClockGearsDataManager::init_empty() {
	// REGLA CERO ESTRICTA: Todas las listas de contenido de usuario inician 100% vacías
	for (int i = 0; i < CAT_MAX; i++) {
		category_data[i].clear();
	}

	if (ClockGearsIdRegistry::get_singleton()) {
		ClockGearsIdRegistry::get_singleton()->reset_all();
	}
	if (ClockGearsTimeManager::get_singleton()) {
		ClockGearsTimeManager::get_singleton()->init_empty();
	}
	if (ClockGearsSystemCatalog::get_singleton()) {
		ClockGearsSystemCatalog::get_singleton()->ensure_default_elements_if_empty();
		category_data[CAT_TYPES] = ClockGearsSystemCatalog::get_singleton()->get_element_records();
	}

	system_config.clear();
	system_config["game_title"] = "";
	system_config["currency_unit"] = "G";
	system_config["combat_mode"] = 0; // 0 = Turnos Clásico, 1 = Action RPG Tiempo Real
	system_config["movement_directions"] = 4; // 4 o 8 direcciones de movimiento
	system_config["mannequin_size"] = 32; // Tamaño de maniquí de referencia (32, 48, 64; no se exporta)
	system_config["arpg_move_while_attacking"] = true;
	system_config["arpg_attack_move_speed_pct"] = 100;
	system_config["arpg_move_while_blocking"] = false;
	system_config["arpg_block_move_speed_pct"] = 50;
	system_config["start_map"] = "";
	system_config["start_x"] = 0;
	system_config["start_y"] = 0;
	system_config["max_followers"] = 3;
	system_config["sell_price_ratio"] = 0.5;
	system_config["autosave_minutes"] = 5;
	system_config["camera_zoom"] = 2;
	system_config["camera_smooth"] = true;
	system_config["camera_clamp"] = true;

	vocabulary_config.clear();
	vocabulary_config["level"] = "Nivel";
	vocabulary_config["hp"] = "PV";
	vocabulary_config["mp"] = "PM";
	vocabulary_config["tp"] = "PT";
	vocabulary_config["exp"] = "EXP";
	vocabulary_config["fight"] = "Luchar";
	vocabulary_config["escape"] = "Huir";
	vocabulary_config["attack"] = "Atacar";
	vocabulary_config["guard"] = "Defender";
	vocabulary_config["item"] = "Objeto";
	vocabulary_config["skill"] = "Habilidad";
	vocabulary_config["equip"] = "Equipar";
	vocabulary_config["status"] = "Estado";
	vocabulary_config["formation"] = String::utf8("Formación");
	vocabulary_config["save"] = "Guardar";
	vocabulary_config["game_end"] = "Salir";

	switches_names.clear();
	variables_names.clear();
	runtime_switches.clear();
	runtime_variables.clear();
	runtime_inventory.clear();
	runtime_arpg_equips.clear();
	runtime_gold = 0;
	dirty = false;
}

Variant ClockGearsDataManager::_normalize_json_numbers(const Variant &p_val) const {
	// Bug #20: Godot convierte números de JSON a float. Reconvertimos enteros exactos a int64_t.
	switch (p_val.get_type()) {
		case Variant::FLOAT: {
			double d = double(p_val);
			if (!Math::is_nan(d) && !Math::is_inf(d) && Math::floor(d) == d && d >= -9007199254740991.0 && d <= 9007199254740991.0) {
				return int64_t(d);
			}
			return p_val;
		}
		case Variant::ARRAY: {
			Array in_arr = p_val;
			Array out_arr;
			out_arr.resize(in_arr.size());
			for (int i = 0; i < in_arr.size(); i++) {
				out_arr[i] = _normalize_json_numbers(in_arr[i]);
			}
			return out_arr;
		}
		case Variant::DICTIONARY: {
			Dictionary in_dict = p_val;
			Dictionary out_dict;
			for (const Variant &key : in_dict.keys()) {
				out_dict[key] = _normalize_json_numbers(in_dict[key]);
			}
			return out_dict;
		}
		default:
			return p_val;
	}
}

bool ClockGearsDataManager::_write_json_atomic(const String &p_file_path, const Variant &p_data) const {
	String tmp_path = p_file_path + ".tmp";
	String bak_path = p_file_path + ".bak";

	String json_text = JSON::stringify(p_data, "\t");
	{
		Ref<FileAccess> f = FileAccess::open(tmp_path, FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(f.is_null(), false, vformat("ClockGearsDataManager: No se pudo escribir archivo temporal '%s'", tmp_path));
		f->store_string(json_text);
		f->flush();
	}

	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	ERR_FAIL_COND_V(dir.is_null(), false);

	if (FileAccess::exists(p_file_path)) {
		if (FileAccess::exists(bak_path)) {
			dir->remove(bak_path);
		}
		dir->rename(p_file_path, bak_path);
	}

	Error ren_err = dir->rename(tmp_path, p_file_path);
	if (ren_err != OK) {
		// Fallback directo si el sistema de archivos bloquea rename
		Ref<FileAccess> f_out = FileAccess::open(p_file_path, FileAccess::WRITE);
		ERR_FAIL_COND_V(f_out.is_null(), false);
		f_out->store_string(json_text);
		f_out->flush();
		dir->remove(tmp_path);
	}
	return true;
}

bool ClockGearsDataManager::_read_json_with_backup(const String &p_file_path, Variant &r_data) const {
	String bak_path = p_file_path + ".bak";

	if (FileAccess::exists(p_file_path)) {
		Ref<FileAccess> f = FileAccess::open(p_file_path, FileAccess::READ);
		if (f.is_valid()) {
			String text = f->get_as_text();
			Ref<JSON> json;
			json.instantiate();
			if (json->parse(text) == OK) {
				r_data = _normalize_json_numbers(json->get_data());
				return true;
			}
			WARN_PRINT(vformat(String::utf8("ClockGearsDataManager: Archivo '%s' corrupto (línea %d: %s). Intentando recuperar desde '%s'..."), p_file_path, json->get_error_line(), json->get_error_message(), bak_path));
		}
	}

	if (FileAccess::exists(bak_path)) {
		Ref<FileAccess> f_bak = FileAccess::open(bak_path, FileAccess::READ);
		if (f_bak.is_valid()) {
			String text = f_bak->get_as_text();
			Ref<JSON> json;
			json.instantiate();
			if (json->parse(text) == OK) {
				r_data = _normalize_json_numbers(json->get_data());
				WARN_PRINT(vformat("ClockGearsDataManager: Recuperado exitosamente desde respaldo '%s'.", bak_path));
				return true;
			}
		}
	}

	return false;
}

PackedStringArray ClockGearsDataManager::get_arpg_ai_types() {
	PackedStringArray out;
	out.push_back("quieto");
	out.push_back("vagar");
	out.push_back("patrulla");
	out.push_back("persigue_y_ataca");
	out.push_back("huye");
	out.push_back("a_distancia");
	return out;
}

PackedStringArray ClockGearsDataManager::get_npc_move_types() {
	PackedStringArray out;
	out.push_back("quieto");
	out.push_back("vagar");
	out.push_back("patrulla");
	out.push_back("seguir");
	return out;
}

PackedStringArray ClockGearsDataManager::get_detection_zone_triggers() {
	PackedStringArray out;
	out.push_back("enter");
	out.push_back("exit");
	out.push_back("while_inside");
	return out;
}

PackedStringArray ClockGearsDataManager::get_detection_zone_actions() {
	PackedStringArray out;
	out.push_back("perseguir");
	out.push_back("huir");
	out.push_back("volver_puesto");
	out.push_back("mirar_jugador");
	out.push_back("detenerse");
	out.push_back("ejecutar_evento");
	out.push_back("activar_interruptor");
	out.push_back("poner_variable");
	out.push_back("atacar");
	return out;
}

static String s_normalize_arpg_ai_type(const Variant &p_val) {
	String raw = String(p_val).strip_edges().to_lower();
	raw = raw.replace(" ", "_");
	if (raw == "quieto" || raw == "idle" || raw == "fijo") return "quieto";
	if (raw == "vagar" || raw == "wander" || raw == "merodear") return "vagar";
	if (raw == "patrulla" || raw == "patrol" || raw == "ruta") return "patrulla";
	if (raw == "persigue_y_ataca" || raw == "persigue" || raw == "perseguir" || raw == "chase") return "persigue_y_ataca";
	if (raw == "huye" || raw == "huir" || raw == "flee") return "huye";
	if (raw == "a_distancia" || raw == "distancia" || raw == "ranged") return "a_distancia";
	return "persigue_y_ataca";
}

static String s_normalize_npc_move_type(const Variant &p_val) {
	String raw = String(p_val).strip_edges().to_lower();
	raw = raw.replace(" ", "_");
	if (raw == "vagar" || raw == "wander") return "vagar";
	if (raw == "patrulla" || raw == "ruta" || raw == "route" || raw == "patrol") return "patrulla";
	if (raw == "seguir" || raw == "follow") return "seguir";
	return "quieto";
}

static Dictionary s_normalize_body_shape_dict(const Variant &p_val) {
	if (p_val.get_type() == Variant::DICTIONARY) {
		Dictionary raw = p_val;
		if (!raw.is_empty()) {
			ClockGearsHitShape s;
			s.from_dict(raw);
			return s.to_dict();
		}
	}
	return ClockGearsHitShape::make_circle(12.0f).to_dict();
}

static Array s_normalize_patrol_points(const Variant &p_val) {
	Array out;
	if (p_val.get_type() != Variant::ARRAY) {
		return out;
	}
	Array src = p_val;
	for (int i = 0; i < src.size(); i++) {
		Dictionary pt;
		if (src[i].get_type() == Variant::DICTIONARY) {
			Dictionary d = src[i];
			pt["x"] = double(d.get("x", 0.0));
			pt["y"] = double(d.get("y", 0.0));
			out.push_back(pt);
		} else if (src[i].get_type() == Variant::VECTOR2 || src[i].get_type() == Variant::VECTOR2I) {
			Vector2 v = Vector2(src[i]);
			pt["x"] = double(v.x);
			pt["y"] = double(v.y);
			out.push_back(pt);
		}
	}
	return out;
}

static String s_normalize_zone_action_type(const String &p_raw) {
	String a = p_raw.strip_edges().to_lower().replace(" ", "_");
	if (a == "perseguir" || a == "chase") return "perseguir";
	if (a == "huir" || a == "huye" || a == "flee") return "huir";
	if (a == "volver_puesto" || a == "volver" || a == "volver_origen" || a == "return_home") return "volver_puesto";
	if (a == "mirar_jugador" || a == "mirar" || a == "face_player") return "mirar_jugador";
	if (a == "detenerse" || a == "parar" || a == "stop") return "detenerse";
	if (a == "ejecutar_evento" || a == "evento" || a == "run_event") return "ejecutar_evento";
	if (a == "activar_interruptor" || a == "interruptor" || a == "set_switch") return "activar_interruptor";
	if (a == "poner_variable" || a == "variable" || a == "set_variable") return "poner_variable";
	if (a == "atacar" || a == "attack") return "atacar";
	return "detenerse";
}

static Dictionary s_normalize_zone_action(const Variant &p_act) {
	Dictionary out;
	if (p_act.get_type() == Variant::DICTIONARY) {
		Dictionary src = p_act;
		String t = s_normalize_zone_action_type(String(src.get("type", src.get("action", "detenerse"))));
		out["type"] = t;
		if (t == "activar_interruptor") {
			out["switch_name"] = String(src.get("switch_name", src.get("name", "")));
			out["value"] = src.has("value") ? bool(src["value"]) : bool(src.get("val", true));
		} else if (t == "poner_variable") {
			out["var_name"] = String(src.get("var_name", src.get("name", "")));
			out["value"] = src.has("value") ? int(src["value"]) : int(src.get("val", 0));
			String op = String(src.get("op", "=")).strip_edges();
			if (op != "=" && op != "+" && op != "-") {
				op = "=";
			}
			out["op"] = op;
		}
		return out;
	}
	String t = s_normalize_zone_action_type(String(p_act));
	out["type"] = t;
	return out;
}

static String s_normalize_zone_target_filter(const Variant &p_val) {
	String f = String(p_val).strip_edges().to_lower().replace(" ", "_");
	if (f == "enemigos" || f == "enemy" || f == "enemies") return "enemies";
	if (f == "npc" || f == "npcs") return "npc";
	if (f == "todos" || f == "all") return "all";
	return "player";
}

static String s_normalize_zone_trigger(const Variant &p_val) {
	String t = String(p_val).strip_edges().to_lower().replace(" ", "_");
	if (t == "exit" || t == "salir" || t == "on_exit") return "exit";
	if (t == "while_inside" || t == "mientras_dentro" || t == "dentro" || t == "stay") return "while_inside";
	return "enter";
}

Dictionary ClockGearsDataManager::normalize_detection_zone(const Dictionary &p_zone, int p_idx) {
	Dictionary out;
	String zname = String(p_zone.get("name", "")).strip_edges();
	if (zname.is_empty()) {
		zname = vformat("Zona %d", p_idx + 1);
	}
	out["name"] = zname;

	if (p_zone.has("shape") && p_zone["shape"].get_type() == Variant::DICTIONARY && !Dictionary(p_zone["shape"]).is_empty()) {
		ClockGearsHitShape s;
		s.from_dict(Dictionary(p_zone["shape"]));
		out["shape"] = s.to_dict();
	} else {
		out["shape"] = ClockGearsHitShape::make_circle(96.0f).to_dict();
	}

	out["target_filter"] = s_normalize_zone_target_filter(p_zone.get("target_filter", p_zone.get("filter", "player")));
	out["trigger"] = s_normalize_zone_trigger(p_zone.get("trigger", "enter"));
	out["interval_sec"] = MAX(0.05, double(p_zone.get("interval_sec", p_zone.get("interval", 0.5))));

	Array acts_out;
	if (p_zone.has("actions") && p_zone["actions"].get_type() == Variant::ARRAY) {
		Array raw_acts = p_zone["actions"];
		for (int i = 0; i < raw_acts.size(); i++) {
			acts_out.push_back(s_normalize_zone_action(raw_acts[i]));
		}
	}
	out["actions"] = acts_out;
	return out;
}

static Array s_normalize_detection_zones(const Variant &p_val) {
	Array out;
	if (p_val.get_type() != Variant::ARRAY) {
		return out;
	}
	Array src = p_val;
	for (int i = 0; i < src.size(); i++) {
		if (src[i].get_type() == Variant::DICTIONARY) {
			out.push_back(ClockGearsDataManager::normalize_detection_zone(src[i], i));
		}
	}
	return out;
}

void ClockGearsDataManager::normalize_record_defaults(int p_category, Dictionary &r_record) {
	if (p_category == CAT_TROOPS && r_record.has("members") && r_record["members"].get_type() == Variant::ARRAY) {
		Array mems = r_record["members"];
		for (int m_i = 0; m_i < mems.size(); m_i++) {
			if (mems[m_i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary md = mems[m_i];
			String mode = ClockGearsBattleUnit::normalize_hp_bar_mode(String(md.get("hp_bar_mode", "abajo")));
			md["hp_bar_mode"] = mode;
			if (mode == "libre") {
				Vector2i abs_pos;
				if (md.has("hp_bar_x") && md.has("hp_bar_y")) {
					abs_pos = Vector2i(int(md["hp_bar_x"]), int(md["hp_bar_y"]));
				} else {
					// Migra el desplazamiento relativo antiguo a la posición absoluta equivalente en 960x540.
					int mx = int(md.get("x", 200 + m_i * 160));
					int my = int(md.get("y", 260));
					int ox = int(md.get("hp_bar_offset_x", 0));
					int oy = int(md.get("hp_bar_offset_y", 0));
					abs_pos = Vector2i(mx + ox, my + oy);
				}
				Vector2i clamped = ClockGearsBattleUnit::clamp_free_hp_bar_pos(abs_pos);
				md["hp_bar_x"] = clamped.x;
				md["hp_bar_y"] = clamped.y;
			} else {
				md.erase("hp_bar_x");
				md.erase("hp_bar_y");
			}
			md.erase("hp_bar_offset_x");
			md.erase("hp_bar_offset_y");
			mems[m_i] = md;
		}
		r_record["members"] = mems;
		return;
	}

	if (p_category == CAT_ACTORS || p_category == CAT_ARPG_NPCS || p_category == CAT_NPCS || p_category == CAT_ARPG_ENEMIES) {
		int walk_pct = r_record.has("walk_speed_pct") ? int(r_record["walk_speed_pct"]) : 100;
		int run_pct = r_record.has("run_speed_pct") ? int(r_record["run_speed_pct"]) : 100;
		r_record["walk_speed_pct"] = MAX(1, walk_pct);
		r_record["run_speed_pct"] = MAX(1, run_pct);
		if (p_category != CAT_NPCS) {
			int atk_pct = r_record.has("attack_speed_pct") ? int(r_record["attack_speed_pct"]) : 100;
			r_record["attack_speed_pct"] = MAX(1, atk_pct);
		}
		r_record["body_shape"] = s_normalize_body_shape_dict(r_record.get("body_shape", Dictionary()));
		r_record["body_through"] = r_record.has("body_through") ? bool(r_record["body_through"]) : false;
		if (p_category != CAT_ACTORS) {
			r_record["avoid_obstacles"] = r_record.has("avoid_obstacles") ? bool(r_record["avoid_obstacles"]) : true;
			r_record["path_recalc_ms"] = r_record.has("path_recalc_ms") ? MAX(50, int(r_record["path_recalc_ms"])) : 250;
			r_record["path_max_nodes"] = r_record.has("path_max_nodes") ? MAX(16, int(r_record["path_max_nodes"])) : 256;
			r_record["wander_radius"] = r_record.has("wander_radius") ? MAX(0.0, double(r_record["wander_radius"])) : 3.0;
			r_record["patrol_points"] = s_normalize_patrol_points(r_record.get("patrol_points", Array()));
			r_record["patrol_loop"] = r_record.has("patrol_loop") ? bool(r_record["patrol_loop"]) : false;
		}
		if (p_category == CAT_NPCS || p_category == CAT_ARPG_NPCS) {
			r_record["npc_move_type"] = r_record.has("npc_move_type") ? s_normalize_npc_move_type(r_record["npc_move_type"]) : String("quieto");
			r_record["follow_distance"] = r_record.has("follow_distance") ? MAX(0.5, double(r_record["follow_distance"])) : 2.0;
			r_record["face_player_on_event"] = r_record.has("face_player_on_event") ? bool(r_record["face_player_on_event"]) : true;
			r_record["detection_zones"] = s_normalize_detection_zones(r_record.get("detection_zones", Array()));
		}
	}

	const bool is_sprite_anim_cat = (p_category == CAT_ACTORS ||
			p_category == CAT_NPCS ||
			p_category == CAT_ARPG_NPCS ||
			p_category == CAT_ARPG_ENEMIES ||
			(p_category >= CAT_ARPG_WEAPONS && p_category <= CAT_ARPG_ACCESSORIES) ||
			p_category == CAT_ENEMIES);
	if (is_sprite_anim_cat) {
		Dictionary sp_dict;
		if (r_record.has("sprite_data") && r_record["sprite_data"].get_type() == Variant::DICTIONARY) {
			sp_dict = Dictionary(r_record["sprite_data"]).duplicate(true);
		}
		if (!sp_dict.has("animations") && r_record.has("animations") && r_record["animations"].get_type() == Variant::DICTIONARY) {
			sp_dict["animations"] = Dictionary(r_record["animations"]).duplicate(true);
		}
		if (!sp_dict.has("graphic_path") && r_record.has("graphic_path")) {
			sp_dict["graphic_path"] = r_record["graphic_path"];
		}
		ClockGearsSpriteSheetData sp;
		if (!sp_dict.is_empty()) {
			sp.from_dict(sp_dict);
		} else if (r_record.has("battler_frames") && !String(r_record.get("graphic_path", "")).is_empty()) {
			int old_frames = MAX(1, int(r_record.get("battler_frames", 1)));
			float old_fps = float(r_record.get("battler_fps", 8.0));
			sp.cols = old_frames;
			sp.rows = 1;
			ClockGearsSpriteAnimation idle_anim;
			idle_anim.name = "idle";
			idle_anim.spritesheet_path = String(r_record.get("graphic_path", ""));
			idle_anim.fps = old_fps > 0.0f ? old_fps : 6.0f;
			idle_anim.loop = true;
			for (int f = 0; f < old_frames; f++) {
				idle_anim.frames.push_back(f);
			}
			sp.animations[idle_anim.name] = idle_anim;
		}
		sp.ensure_default_hurtbox();
		Dictionary out_sp = sp.to_dict();
		r_record["sprite_data"] = out_sp;
		r_record["animations"] = Dictionary(out_sp.get("animations", Dictionary())).duplicate(true);
	}

	if (p_category != CAT_ARPG_ENEMIES) {
		return;
	}

	// Enemigos ARPG son independientes de los enemigos por turnos: sin MP ni AGI.
	r_record.erase("mp");
	r_record.erase("agi");

	if (!r_record.has("level")) {
		r_record["level"] = 1;
	} else {
		r_record["level"] = MAX(1, int(r_record["level"]));
	}
	if (!r_record.has("exp")) {
		r_record["exp"] = 10;
	} else {
		r_record["exp"] = MAX(0, int(r_record["exp"]));
	}
	if (!r_record.has("gold")) {
		r_record["gold"] = 5;
	} else {
		r_record["gold"] = MAX(0, int(r_record["gold"]));
	}

	if (!r_record.has("hp")) {
		r_record["hp"] = 100;
	} else {
		r_record["hp"] = MAX(1, int(r_record["hp"]));
	}
	if (!r_record.has("atk")) {
		r_record["atk"] = 15;
	} else {
		r_record["atk"] = MAX(0, int(r_record["atk"]));
	}
	if (!r_record.has("def")) {
		r_record["def"] = 10;
	} else {
		r_record["def"] = MAX(0, int(r_record["def"]));
	}
	if (!r_record.has("mat")) {
		r_record["mat"] = 10;
	} else {
		r_record["mat"] = MAX(0, int(r_record["mat"]));
	}
	if (!r_record.has("mdf")) {
		r_record["mdf"] = 10;
	} else {
		r_record["mdf"] = MAX(0, int(r_record["mdf"]));
	}
	if (!r_record.has("luk")) {
		r_record["luk"] = 5;
	} else {
		r_record["luk"] = MAX(0, int(r_record["luk"]));
	}

	// 3 drops (mismo formato que enemigos por turnos)
	Array raw_drops = (r_record.has("drops") && r_record["drops"].get_type() == Variant::ARRAY) ? Array(r_record["drops"]) : Array();
	Array norm_drops;
	for (int s = 0; s < 3; s++) {
		Dictionary dr;
		if (s < raw_drops.size() && raw_drops[s].get_type() == Variant::DICTIONARY) {
			Dictionary src = raw_drops[s];
			dr["kind"] = MAX(0, int(src.get("kind", 0)));
			dr["item_id"] = MAX(0, int(src.get("item_id", 0)));
			dr["prob"] = CLAMP(int(src.get("prob", 100)), 0, 100);
		} else {
			dr["kind"] = 0;
			dr["item_id"] = 0;
			dr["prob"] = 100;
		}
		norm_drops.push_back(dr);
	}
	r_record["drops"] = norm_drops;

	// Debilidades y resistencias: fisico + 10 elementos (mismo formato CG_TRAIT_ELEMENT_RATE)
	Array raw_traits = (r_record.has("traits") && r_record["traits"].get_type() == Variant::ARRAY) ? Array(r_record["traits"]).duplicate(true) : Array();
	Array other_traits;
	HashMap<String, double> rate_by_key;
	HashMap<int, double> rate_by_id;

	for (int i = 0; i < raw_traits.size(); i++) {
		if (raw_traits[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary tr = raw_traits[i];
		if (int(tr.get("code", 0)) == CG_TRAIT_ELEMENT_RATE) {
			String ek = String(tr.get("element_key", "")).strip_edges().to_lower();
			int did = int(tr.get("data_id", -1));
			double val = MAX(0.0, double(tr.get("value", 100.0)));
			if (!ek.is_empty()) {
				if (ek == String::utf8("físico") || ek == "physical") {
					ek = "fisico";
				}
				rate_by_key[ek] = val;
			}
			if (did >= 0) {
				rate_by_id[did] = val;
			}
		} else {
			other_traits.push_back(tr.duplicate(true));
		}
	}

	Array norm_traits;
	// 1. Fisico (data_id = 0, element_key = "fisico")
	{
		Dictionary tr;
		tr["code"] = int(CG_TRAIT_ELEMENT_RATE);
		tr["data_id"] = 0;
		tr["element_key"] = "fisico";
		tr["element_migrated"] = true;
		double val = 100.0;
		if (rate_by_key.has("fisico")) {
			val = rate_by_key["fisico"];
		} else if (rate_by_id.has(0)) {
			val = rate_by_id[0];
		}
		tr["value"] = val;
		norm_traits.push_back(tr);
	}

	// 2. Los elementos del catalogo (por defecto los 10 elementos)
	static const char *fallback_elem_keys[10] = {
		"agua", "fuego", "hielo", "electricidad", "viento",
		"luz", "oscuridad", "astral", "tierra", "naturaleza"
	};
	ClockGearsSystemCatalog *cat_sys = ClockGearsSystemCatalog::get_singleton();
	int elem_count = cat_sys ? cat_sys->get_elements().size() : 10;
	if (elem_count <= 0) {
		elem_count = 10;
	}
	for (int e_id = 1; e_id <= elem_count; e_id++) {
		String ek = cat_sys ? cat_sys->get_element_key(e_id).strip_edges().to_lower() : String(fallback_elem_keys[(e_id - 1) % 10]);
		if (ek.is_empty() && e_id <= 10) {
			ek = fallback_elem_keys[e_id - 1];
		}
		double val = 100.0;
		if (!ek.is_empty() && rate_by_key.has(ek)) {
			val = rate_by_key[ek];
		} else if (rate_by_id.has(e_id)) {
			val = rate_by_id[e_id];
		}
		Dictionary tr;
		tr["code"] = int(CG_TRAIT_ELEMENT_RATE);
		tr["data_id"] = e_id;
		tr["element_key"] = ek;
		tr["element_migrated"] = true;
		tr["value"] = val;
		norm_traits.push_back(tr);
	}
	for (int i = 0; i < other_traits.size(); i++) {
		norm_traits.push_back(other_traits[i]);
	}
	r_record["traits"] = norm_traits;

	// IA de enemigo ARPG
	String norm_ai = r_record.has("ai_type") ? s_normalize_arpg_ai_type(r_record["ai_type"]) : String("persigue_y_ataca");
	r_record["ai_type"] = norm_ai;
	double vis_r = r_record.has("vision_radius") ? MAX(0.0, double(r_record["vision_radius"])) : 6.0;
	double atk_r = r_record.has("attack_range") ? MAX(0.0, double(r_record["attack_range"])) : 1.0;
	double atk_iv = r_record.has("attack_interval") ? MAX(0.0, double(r_record["attack_interval"])) : 1.5;
	r_record["vision_radius"] = vis_r;
	r_record["attack_range"] = atk_r;
	r_record["attack_interval"] = atk_iv;
	r_record["knockback_resist"] = r_record.has("knockback_resist") ? CLAMP(int(r_record["knockback_resist"]), 0, 100) : 0;
	r_record["lose_sight_sec"] = r_record.has("lose_sight_sec") ? MAX(0.0, double(r_record["lose_sight_sec"])) : 4.0;
	double min_d = r_record.has("min_distance") ? MAX(0.0, double(r_record["min_distance"])) : 3.0;
	double max_d = r_record.has("max_distance") ? MAX(min_d, double(r_record["max_distance"])) : MAX(min_d, 5.0);
	r_record["min_distance"] = min_d;
	r_record["max_distance"] = max_d;

	// Migración: si el registro aún no tiene detection_zones, convierte radio de visión y alcance en zonas circulares iniciales.
	if (!r_record.has("detection_zones") || r_record["detection_zones"].get_type() != Variant::ARRAY) {
		Array migrated_zones;
		if (vis_r > 0.0) {
			Dictionary vz;
			vz["name"] = String::utf8("Visión");
			vz["shape"] = ClockGearsHitShape::make_circle(float(vis_r * 32.0)).to_dict();
			vz["target_filter"] = "player";
			vz["trigger"] = "enter";
			Array vacts;
			Dictionary vact;
			vact["type"] = (norm_ai == "huye") ? String("huir") : String("perseguir");
			vacts.push_back(vact);
			vz["actions"] = vacts;
			migrated_zones.push_back(normalize_detection_zone(vz, 0));
		}
		if (atk_r > 0.0) {
			Dictionary az;
			az["name"] = String::utf8("Ataque");
			az["shape"] = ClockGearsHitShape::make_circle(float(atk_r * 32.0)).to_dict();
			az["target_filter"] = "player";
			az["trigger"] = "while_inside";
			az["interval_sec"] = MAX(0.1, atk_iv);
			Array aacts;
			Dictionary aact;
			aact["type"] = "atacar";
			aacts.push_back(aact);
			az["actions"] = aacts;
			migrated_zones.push_back(normalize_detection_zone(az, migrated_zones.size()));
		}
		r_record["detection_zones"] = migrated_zones;
	} else {
		r_record["detection_zones"] = s_normalize_detection_zones(r_record["detection_zones"]);
	}

	// Aislar contenedores anidados existentes
	if (r_record.has("sprite_data") && r_record["sprite_data"].get_type() == Variant::DICTIONARY) {
		r_record["sprite_data"] = Dictionary(r_record["sprite_data"]).duplicate(true);
	}
	if (r_record.has("animations") && r_record["animations"].get_type() == Variant::DICTIONARY) {
		r_record["animations"] = Dictionary(r_record["animations"]).duplicate(true);
	}
	if (r_record.has("event_pages") && r_record["event_pages"].get_type() == Variant::ARRAY) {
		r_record["event_pages"] = Array(r_record["event_pages"]).duplicate(true);
	}
}

Dictionary ClockGearsDataManager::get_normalized_record(int p_category, const Dictionary &p_record) {
	Dictionary out = p_record.duplicate(true);
	normalize_record_defaults(p_category, out);
	return out;
}

void ClockGearsDataManager::_migrate_schema_if_needed(int p_from_version) {
	ClockGearsSystemCatalog *cat_sys = ClockGearsSystemCatalog::get_singleton();
	// Garantiza que registros de versiones previas tengan todos los campos esperados y migra ranuras/elementos
	for (int cat = 0; cat < CAT_MAX; cat++) {
		Array &arr = category_data[cat];
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary rec = arr[i];
			if (p_from_version < CURRENT_SCHEMA_VERSION) {
				if (!rec.has("id")) {
					rec["id"] = i + 1;
				}
				if (!rec.has("name")) {
					rec["name"] = "";
				}
				if (!rec.has("desc")) {
					rec["desc"] = "";
				}
				if (!rec.has("traits")) {
					rec["traits"] = Array();
				}
			}
			if (cat == CAT_ENEMIES) {
				ClockGearsSpriteSheetData::migrate_enemy_record(rec);
			}
			if (cat_sys) {
				cat_sys->migrate_record_elements(rec);
			}
			arr[i] = rec;
		}
	}
}

int ClockGearsDataManager::_clean_cross_references_on_delete(int p_category, int p_deleted_id) {
	int cleaned_count = 0;

	// Si se borra un Objeto, Arma o Armadura, limpiar referencias en Drops de Enemigos y Tablas de Flora/Fauna
	if (p_category == CAT_ITEMS || p_category == CAT_WEAPONS || p_category == CAT_ARMORS) {
		Array &enemies = category_data[CAT_ENEMIES];
		for (int i = 0; i < enemies.size(); i++) {
			if (enemies[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary en = enemies[i];
			bool modified = false;
			if (en.has("drops") && en["drops"].get_type() == Variant::ARRAY) {
				Array drops = en["drops"];
				for (int d = 0; d < drops.size(); d++) {
					if (drops[d].get_type() == Variant::DICTIONARY) {
						Dictionary drop = drops[d];
						int drop_cat = int(drop.get("kind", CAT_ITEMS));
						int drop_id = int(drop.get("item_id", 0));
						if (drop_cat == p_category && drop_id == p_deleted_id) {
							drop["item_id"] = 0;
							drops[d] = drop;
							modified = true;
							cleaned_count++;
						}
					}
				}
				if (modified) {
					en["drops"] = drops;
					enemies[i] = en;
				}
			}
		}
	}

	// Si se borra una Habilidad, limpiar referencias en Patrones de Acción de Enemigos
	if (p_category == CAT_SKILLS) {
		Array &enemies = category_data[CAT_ENEMIES];
		for (int i = 0; i < enemies.size(); i++) {
			if (enemies[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary en = enemies[i];
			if (en.has("actions") && en["actions"].get_type() == Variant::ARRAY) {
				Array actions = en["actions"];
				bool modified = false;
				for (int a = 0; a < actions.size(); a++) {
					if (actions[a].get_type() == Variant::DICTIONARY) {
						Dictionary act = actions[a];
						if (int(act.get("skill_id", 0)) == p_deleted_id) {
							act["skill_id"] = 0;
							actions[a] = act;
							modified = true;
							cleaned_count++;
						}
					}
				}
				if (modified) {
					en["actions"] = actions;
					enemies[i] = en;
				}
			}
		}
	}

	// Si se borra un Enemigo o Flora, limpiar vínculos en Fauna
	if (p_category == CAT_ENEMIES || p_category == CAT_FLORA) {
		Array &fauna_arr = category_data[CAT_FAUNA];
		for (int i = 0; i < fauna_arr.size(); i++) {
			if (fauna_arr[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary fn = fauna_arr[i];
			bool modified = false;
			if (p_category == CAT_ENEMIES && int(fn.get("enemy_id", 0)) == p_deleted_id) {
				fn["enemy_id"] = 0;
				modified = true;
				cleaned_count++;
			}
			if (p_category == CAT_FLORA && int(fn.get("diet_flora_id", 0)) == p_deleted_id) {
				fn["diet_flora_id"] = 0;
				modified = true;
				cleaned_count++;
			}
			if (modified) {
				fauna_arr[i] = fn;
			}
		}
	}

	if (cleaned_count > 0) {
		WARN_PRINT(vformat("ClockGearsDataManager: Se limpiaron %d referencias cruzadas al eliminar ID %d de '%s'.", cleaned_count, p_deleted_id, get_category_key(p_category)));
	}
	return cleaned_count;
}

Array ClockGearsDataManager::get_category_array(int p_category) const {
	ERR_FAIL_INDEX_V(p_category, CAT_MAX, Array());
	return category_data[p_category].duplicate(true);
}

void ClockGearsDataManager::set_category_array(int p_category, const Array &p_array) {
	ERR_FAIL_INDEX(p_category, CAT_MAX);
	category_data[p_category] = p_array.duplicate(true);
	ClockGearsSystemCatalog *cat_sys = ClockGearsSystemCatalog::get_singleton();
	if (p_category == CAT_TYPES && cat_sys) {
		if (category_data[CAT_TYPES].is_empty()) {
			cat_sys->ensure_default_elements_if_empty();
			category_data[CAT_TYPES] = cat_sys->get_element_records();
		} else {
			cat_sys->sync_elements_from_records(category_data[CAT_TYPES]);
			Array merged = cat_sys->get_element_records();
			for (int i = 0; i < category_data[CAT_TYPES].size(); i++) {
				if (category_data[CAT_TYPES][i].get_type() == Variant::DICTIONARY) {
					Dictionary d = category_data[CAT_TYPES][i];
					if (int(d.get("type", 0)) != 0) {
						merged.push_back(d);
					}
				}
			}
			category_data[CAT_TYPES] = merged;
		}
		// Migrate element traits in already-loaded categories if legacy element map changed
		for (int cat = 0; cat < CAT_MAX; cat++) {
			Array &arr = category_data[cat];
			for (int i = 0; i < arr.size(); i++) {
				if (arr[i].get_type() == Variant::DICTIONARY) {
					Dictionary rec = arr[i];
					cat_sys->migrate_record_elements(rec);
					arr[i] = rec;
				}
			}
		}
	} else {
		Array &arr = category_data[p_category];
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].get_type() == Variant::DICTIONARY) {
				Dictionary rec = arr[i];
				if (p_category == CAT_ENEMIES) {
					ClockGearsSpriteSheetData::migrate_enemy_record(rec);
				}
				if (cat_sys) {
					cat_sys->migrate_record_elements(rec);
				}
				normalize_record_defaults(p_category, rec);
				arr[i] = rec;
			}
		}
	}
	if (ClockGearsIdRegistry::get_singleton()) {
		ClockGearsIdRegistry::get_singleton()->sync_category_from_records(get_category_key(p_category), category_data[p_category]);
	}
	dirty = true;
	emit_signal("data_changed", get_category_key(p_category), -1);
}

Array ClockGearsDataManager::get_records(const String &p_category_key) const {
	int cat = get_category_from_key(p_category_key);
	ERR_FAIL_COND_V(cat < 0, Array());
	return get_category_array(cat);
}

void ClockGearsDataManager::set_records(const String &p_category_key, const Array &p_records) {
	int cat = get_category_from_key(p_category_key);
	ERR_FAIL_COND(cat < 0);
	set_category_array(cat, p_records);
}

Dictionary ClockGearsDataManager::get_record_by_id(int p_category, int p_id) const {
	ERR_FAIL_INDEX_V(p_category, CAT_MAX, Dictionary());
	const Array &arr = category_data[p_category];
	for (int i = 0; i < arr.size(); i++) {
		if (arr[i].get_type() == Variant::DICTIONARY) {
			Dictionary d = arr[i];
			if (int(d.get("id", 0)) == p_id) {
				return d.duplicate(true);
			}
		}
	}
	return Dictionary();
}

int ClockGearsDataManager::create_record(int p_category, const Dictionary &p_initial_data) {
	ERR_FAIL_INDEX_V(p_category, CAT_MAX, -1);
	String key = get_category_key(p_category);
	int new_id = ClockGearsIdRegistry::get_singleton() ? ClockGearsIdRegistry::get_singleton()->allocate_id(key) : (category_data[p_category].size() + 1);
	Dictionary rec = p_initial_data.duplicate(true);
	rec["id"] = new_id;
	if (!rec.has("name")) {
		rec["name"] = "";
	}
	if (p_category == CAT_ENEMIES) {
		ClockGearsSpriteSheetData::migrate_enemy_record(rec);
	}
	if (ClockGearsSystemCatalog::get_singleton()) {
		ClockGearsSystemCatalog::get_singleton()->migrate_record_elements(rec);
	}
	normalize_record_defaults(p_category, rec);
	category_data[p_category].push_back(rec);
	dirty = true;
	emit_signal("data_changed", key, new_id);
	return new_id;
}

bool ClockGearsDataManager::update_record(int p_category, int p_id, const Dictionary &p_record) {
	ERR_FAIL_INDEX_V(p_category, CAT_MAX, false);
	ERR_FAIL_COND_V(p_id < 1, false);
	Array &arr = category_data[p_category];
	for (int i = 0; i < arr.size(); i++) {
		if (arr[i].get_type() == Variant::DICTIONARY) {
			Dictionary d = arr[i];
			if (int(d.get("id", 0)) == p_id) {
				Dictionary updated = p_record.duplicate(true);
				updated["id"] = p_id;
				if (p_category == CAT_ENEMIES) {
					ClockGearsSpriteSheetData::migrate_enemy_record(updated);
				}
				if (ClockGearsSystemCatalog::get_singleton()) {
					ClockGearsSystemCatalog::get_singleton()->migrate_record_elements(updated);
				}
				normalize_record_defaults(p_category, updated);
				arr[i] = updated;
				dirty = true;
				emit_signal("data_changed", get_category_key(p_category), p_id);
				return true;
			}
		}
	}
	return false;
}

bool ClockGearsDataManager::delete_record_by_id(int p_category, int p_id) {
	ERR_FAIL_INDEX_V(p_category, CAT_MAX, false);
	ERR_FAIL_COND_V(p_id < 1, false);
	Array &arr = category_data[p_category];
	for (int i = 0; i < arr.size(); i++) {
		if (arr[i].get_type() == Variant::DICTIONARY) {
			Dictionary d = arr[i];
			if (int(d.get("id", 0)) == p_id) {
				arr.remove_at(i);
				if (ClockGearsIdRegistry::get_singleton()) {
					ClockGearsIdRegistry::get_singleton()->retire_id(get_category_key(p_category), p_id);
				}
				_clean_cross_references_on_delete(p_category, p_id);
				dirty = true;
				emit_signal("data_changed", get_category_key(p_category), p_id);
				return true;
			}
		}
	}
	return false;
}

int ClockGearsDataManager::normalize_mannequin_size(int p_size) {
	if (p_size == 48) {
		return 48;
	}
	if (p_size == 64) {
		return 64;
	}
	return 32;
}

void ClockGearsDataManager::set_system_config(const Dictionary &p_cfg) {
	system_config = p_cfg.duplicate(true);
	system_config["mannequin_size"] = normalize_mannequin_size(int(system_config.get("mannequin_size", 32)));
	dirty = true;
	emit_signal("data_changed", "system", 0);
}

int ClockGearsDataManager::get_mannequin_size() const {
	return normalize_mannequin_size(int(system_config.get("mannequin_size", 32)));
}

void ClockGearsDataManager::set_mannequin_size(int p_size) {
	system_config["mannequin_size"] = normalize_mannequin_size(p_size);
	dirty = true;
	emit_signal("data_changed", "system", 0);
}

Dictionary ClockGearsDataManager::get_exported_system_config() const {
	Dictionary out = system_config.duplicate(true);
	out.erase("mannequin_size");
	return out;
}

void ClockGearsDataManager::set_vocabulary_config(const Dictionary &p_vocab) {
	vocabulary_config = p_vocab.duplicate(true);
	dirty = true;
	emit_signal("data_changed", "vocabulary", 0);
}

void ClockGearsDataManager::set_switches_names(const Dictionary &p_names) {
	switches_names = p_names.duplicate(true);
	dirty = true;
	emit_signal("data_changed", "switches_vars", 0);
}

void ClockGearsDataManager::set_variables_names(const Dictionary &p_names) {
	variables_names = p_names.duplicate(true);
	dirty = true;
	emit_signal("data_changed", "switches_vars", 0);
}

bool ClockGearsDataManager::save_all(const String &p_dir_path) {
	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	ERR_FAIL_COND_V(dir.is_null(), false);
	if (!dir->dir_exists(p_dir_path)) {
		Error err = dir->make_dir_recursive(p_dir_path);
		ERR_FAIL_COND_V(err != OK, false);
	}

	// 1. Guardar cada categoría de registros en su propio archivo atómico (items.json, weapons.json, etc.)
	for (int i = 0; i < CAT_MAX; i++) {
		if (i == CAT_SYSTEM || i == CAT_TYPES || i == CAT_VOCABULARY || i == CAT_SWITCHES_VARS) {
			continue;
		}
		String file_path = p_dir_path.path_join(get_category_filename(i));
		if (!_write_json_atomic(file_path, category_data[i])) {
			return false;
		}
	}

	// 2. Guardar system.json
	if (!_write_json_atomic(p_dir_path.path_join("system.json"), system_config)) {
		return false;
	}

	// 3. Guardar types.json (catálogos editables de elementos, armas, armaduras y ranuras)
	if (ClockGearsSystemCatalog::get_singleton()) {
		if (!category_data[CAT_TYPES].is_empty()) {
			ClockGearsSystemCatalog::get_singleton()->sync_elements_from_records(category_data[CAT_TYPES]);
		} else {
			ClockGearsSystemCatalog::get_singleton()->ensure_default_elements_if_empty();
			category_data[CAT_TYPES] = ClockGearsSystemCatalog::get_singleton()->get_element_records();
		}
		if (!_write_json_atomic(p_dir_path.path_join("types.json"), ClockGearsSystemCatalog::get_singleton()->to_dict())) {
			return false;
		}
	}

	// 4. Guardar vocabulary.json
	if (!_write_json_atomic(p_dir_path.path_join("vocabulary.json"), vocabulary_config)) {
		return false;
	}

	// 5. Guardar switches_vars.json
	Dictionary sw_vars;
	sw_vars["switches"] = switches_names;
	sw_vars["variables"] = variables_names;
	if (!_write_json_atomic(p_dir_path.path_join("switches_vars.json"), sw_vars)) {
		return false;
	}

	// 5b. Guardar time_rules.json (Reloj y reglas de ciclo Majora's Mask — Anexo A3.1 y A3.2)
	if (ClockGearsTimeManager::get_singleton()) {
		if (!_write_json_atomic(p_dir_path.path_join("time_rules.json"), ClockGearsTimeManager::get_singleton()->to_dict())) {
			return false;
		}
	}

	// 5c. Guardar estado de partida / equipo ARPG runtime (runtime_state.json)
	if (!_write_json_atomic(p_dir_path.path_join("runtime_state.json"), export_runtime_state())) {
		return false;
	}

	// 6. Guardar database.json índice con schema_version
	Dictionary index_doc;
	index_doc["engine"] = "ClockGears Engine";
	index_doc["author"] = "Kris'ZtS";
	index_doc["schema_version"] = CURRENT_SCHEMA_VERSION;
	index_doc["saved_at"] = Time::get_singleton() ? Time::get_singleton()->get_datetime_string_from_system() : "";
	Array files_list;
	for (int i = 0; i < CAT_MAX; i++) {
		files_list.push_back(get_category_filename(i));
	}
	index_doc["files"] = files_list;
	if (!_write_json_atomic(p_dir_path.path_join("database.json"), index_doc)) {
		return false;
	}

	if (ClockGearsIdRegistry::get_singleton()) {
		ClockGearsIdRegistry::get_singleton()->commit_on_save();
	}
	dirty = false;
	return true;
}

bool ClockGearsDataManager::load_all(const String &p_dir_path) {
	String index_path = p_dir_path.path_join("database.json");
	if (!FileAccess::exists(index_path) && !FileAccess::exists(index_path + ".bak")) {
		init_empty();
		return false;
	}

	int loaded_schema = CURRENT_SCHEMA_VERSION;
	Variant index_var;
	if (_read_json_with_backup(index_path, index_var) && index_var.get_type() == Variant::DICTIONARY) {
		Dictionary index_doc = index_var;
		loaded_schema = int(index_doc.get("schema_version", 1));

		// Soporte retrocompatible si database.json antiguo tenía todas las categorías embebidas
		for (int i = 0; i < CAT_MAX; i++) {
			String k = get_category_key(i);
			if (index_doc.has(k) && index_doc[k].get_type() == Variant::ARRAY) {
				category_data[i] = index_doc[k];
			}
		}
	}

	// Cargar archivos individuales por categoría (sobreescribiendo si existen)
	for (int i = 0; i < CAT_MAX; i++) {
		if (i == CAT_SYSTEM || i == CAT_TYPES || i == CAT_VOCABULARY || i == CAT_SWITCHES_VARS) {
			continue;
		}
		String file_path = p_dir_path.path_join(get_category_filename(i));
		Variant cat_var;
		if (_read_json_with_backup(file_path, cat_var) && cat_var.get_type() == Variant::ARRAY) {
			category_data[i] = cat_var;
		}
		if (ClockGearsIdRegistry::get_singleton()) {
			ClockGearsIdRegistry::get_singleton()->sync_category_from_records(get_category_key(i), category_data[i]);
		}
	}

	Variant sys_var;
	if (_read_json_with_backup(p_dir_path.path_join("system.json"), sys_var) && sys_var.get_type() == Variant::DICTIONARY) {
		system_config = sys_var;
	}
	int loaded_dirs = int(system_config.get("movement_directions", 4));
	system_config["movement_directions"] = (loaded_dirs == 8) ? 8 : 4;
	system_config["mannequin_size"] = normalize_mannequin_size(int(system_config.get("mannequin_size", 32)));
	if (!system_config.has("arpg_move_while_attacking")) {
		system_config["arpg_move_while_attacking"] = true;
	} else {
		system_config["arpg_move_while_attacking"] = bool(system_config["arpg_move_while_attacking"]);
	}
	system_config["arpg_attack_move_speed_pct"] = CLAMP(int(system_config.get("arpg_attack_move_speed_pct", 100)), 0, 200);
	if (!system_config.has("arpg_move_while_blocking")) {
		system_config["arpg_move_while_blocking"] = false;
	} else {
		system_config["arpg_move_while_blocking"] = bool(system_config["arpg_move_while_blocking"]);
	}
	system_config["arpg_block_move_speed_pct"] = CLAMP(int(system_config.get("arpg_block_move_speed_pct", 50)), 0, 200);

	Variant types_var;
	if (_read_json_with_backup(p_dir_path.path_join("types.json"), types_var) && types_var.get_type() == Variant::DICTIONARY) {
		if (ClockGearsSystemCatalog::get_singleton()) {
			ClockGearsSystemCatalog::get_singleton()->from_dict(types_var);
			category_data[CAT_TYPES] = ClockGearsSystemCatalog::get_singleton()->get_element_records();
		}
	} else if (ClockGearsSystemCatalog::get_singleton()) {
		ClockGearsSystemCatalog::get_singleton()->ensure_default_elements_if_empty();
		category_data[CAT_TYPES] = ClockGearsSystemCatalog::get_singleton()->get_element_records();
	}

	Variant vocab_var;
	if (_read_json_with_backup(p_dir_path.path_join("vocabulary.json"), vocab_var) && vocab_var.get_type() == Variant::DICTIONARY) {
		vocabulary_config = vocab_var;
	}

	Variant sw_var;
	if (_read_json_with_backup(p_dir_path.path_join("switches_vars.json"), sw_var) && sw_var.get_type() == Variant::DICTIONARY) {
		Dictionary d = sw_var;
		switches_names = d.get("switches", Dictionary());
		variables_names = d.get("variables", Dictionary());
	}

	Variant time_var;
	if (_read_json_with_backup(p_dir_path.path_join("time_rules.json"), time_var) && time_var.get_type() == Variant::DICTIONARY) {
		if (ClockGearsTimeManager::get_singleton()) {
			ClockGearsTimeManager::get_singleton()->from_dict(time_var);
		}
	}

	Variant rt_var;
	if (_read_json_with_backup(p_dir_path.path_join("runtime_state.json"), rt_var) && rt_var.get_type() == Variant::DICTIONARY) {
		import_runtime_state(Dictionary(rt_var));
	} else {
		reset_runtime_state();
	}

	_migrate_schema_if_needed(loaded_schema);
	for (int cat = 0; cat < CAT_MAX; cat++) {
		Array &arr = category_data[cat];
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].get_type() == Variant::DICTIONARY) {
				Dictionary rec = arr[i];
				normalize_record_defaults(cat, rec);
				arr[i] = rec;
			}
		}
	}
	dirty = false;
	return true;
}

bool ClockGearsDataManager::save_autosave(const String &p_base_dir) {
	return save_all(p_base_dir);
}

bool ClockGearsDataManager::save_database(const String &p_path) {
	String base_dir = p_path.ends_with(".json") ? p_path.get_base_dir() : p_path;
	if (base_dir.is_empty()) {
		base_dir = "res://clockgears_data";
	}
	return save_all(base_dir);
}

bool ClockGearsDataManager::load_database(const String &p_path) {
	String base_dir = p_path.ends_with(".json") ? p_path.get_base_dir() : p_path;
	if (base_dir.is_empty()) {
		base_dir = "res://clockgears_data";
	}
	return load_all(base_dir);
}

bool ClockGearsDataManager::get_switch(const String &p_name) const {
	if (runtime_switches.has(p_name)) {
		return bool(runtime_switches[p_name]);
	}
	return false;
}

void ClockGearsDataManager::set_switch(const String &p_name, bool p_val) {
	runtime_switches[p_name] = p_val;
}

int ClockGearsDataManager::get_variable(const String &p_name) const {
	if (runtime_variables.has(p_name)) {
		return int(runtime_variables[p_name]);
	}
	return 0;
}

void ClockGearsDataManager::set_variable(const String &p_name, int p_val) {
	runtime_variables[p_name] = p_val;
}

bool ClockGearsDataManager::has_item(const String &p_name, int p_min_qty) const {
	if (runtime_inventory.has(p_name)) {
		return int(runtime_inventory[p_name]) >= p_min_qty;
	}
	return false;
}

void ClockGearsDataManager::change_item(const String &p_name, int p_amount) {
	int cur = runtime_inventory.has(p_name) ? int(runtime_inventory[p_name]) : 0;
	int next = MAX(0, cur + p_amount);
	if (next == 0) {
		runtime_inventory.erase(p_name);
	} else {
		runtime_inventory[p_name] = next;
	}
}

void ClockGearsDataManager::reset_runtime_state() {
	runtime_switches.clear();
	runtime_variables.clear();
	runtime_inventory.clear();
	runtime_gold = 0;
	runtime_arpg_equips.clear();
	for (int i = 0; i < category_data[CAT_ACTORS].size(); i++) {
		if (category_data[CAT_ACTORS][i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary act = category_data[CAT_ACTORS][i];
		int aid = int(act.get("id", 0));
		if (aid > 0) {
			Array init_slots = act.has("arpg_equip_slots") && act["arpg_equip_slots"].get_type() == Variant::ARRAY
					? normalize_arpg_equip_slots(Array(act["arpg_equip_slots"]))
					: get_default_arpg_equip_slots();
			runtime_arpg_equips[String::num_int64(aid)] = init_slots;
		}
	}
}

Array ClockGearsDataManager::get_default_arpg_equip_slots() {
	Array out;
	struct SlotSpec {
		const char *id;
		const char *label;
		const char *cat;
	};
	static const SlotSpec specs[5] = {
		{ "weapon", "Arma", "arpg_weapons" },
		{ "armor", "Armadura", "arpg_armors" },
		{ "shield", "Escudo", "arpg_shields" },
		{ "accessory_1", "Accesorio 1", "arpg_accessories" },
		{ "accessory_2", "Accesorio 2", "arpg_accessories" }
	};
	for (int i = 0; i < 5; i++) {
		Dictionary d;
		d["slot"] = String(specs[i].id);
		d["label"] = String::utf8(specs[i].label);
		d["category"] = String(specs[i].cat);
		d["piece_id"] = 0;
		out.push_back(d);
	}
	return out;
}

Array ClockGearsDataManager::normalize_arpg_equip_slots(const Array &p_raw_slots) {
	Array out;
	Array defaults = get_default_arpg_equip_slots();
	HashMap<String, int> seen_piece_by_slot;
	for (int i = 0; i < p_raw_slots.size(); i++) {
		if (p_raw_slots[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary raw = p_raw_slots[i];
		String s_id = String(raw.get("slot", "")).strip_edges().to_lower();
		if (!s_id.is_empty()) {
			seen_piece_by_slot[s_id] = int(raw.get("piece_id", 0));
		}
	}
	// Mantener los huecos fijos por defecto primero (preservando cualquier hueco adicional al final)
	HashMap<String, bool> used;
	for (int i = 0; i < defaults.size(); i++) {
		Dictionary d = Dictionary(defaults[i]).duplicate(true);
		String s_id = String(d["slot"]);
		if (seen_piece_by_slot.has(s_id)) {
			d["piece_id"] = seen_piece_by_slot[s_id];
		}
		used[s_id] = true;
		out.push_back(d);
	}
	for (int i = 0; i < p_raw_slots.size(); i++) {
		if (p_raw_slots[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary raw = p_raw_slots[i];
		String s_id = String(raw.get("slot", "")).strip_edges().to_lower();
		if (!s_id.is_empty() && !used.has(s_id)) {
			Dictionary extra = raw.duplicate(true);
			extra["slot"] = s_id;
			out.push_back(extra);
			used[s_id] = true;
		}
	}
	return out;
}

Array ClockGearsDataManager::get_actor_arpg_equip_slots(int p_actor_id) const {
	String key = String::num_int64(p_actor_id);
	if (runtime_arpg_equips.has(key) && runtime_arpg_equips[key].get_type() == Variant::ARRAY) {
		return normalize_arpg_equip_slots(Array(runtime_arpg_equips[key]));
	}
	Dictionary act = get_record_by_id(CAT_ACTORS, p_actor_id);
	if (!act.is_empty() && act.has("arpg_equip_slots") && act["arpg_equip_slots"].get_type() == Variant::ARRAY) {
		return normalize_arpg_equip_slots(Array(act["arpg_equip_slots"]));
	}
	return get_default_arpg_equip_slots();
}

bool ClockGearsDataManager::set_actor_arpg_equip(int p_actor_id, const String &p_slot_id, int p_piece_id) {
	Dictionary act = get_record_by_id(CAT_ACTORS, p_actor_id);
	if (act.is_empty()) {
		return false;
	}
	String norm_slot = p_slot_id.strip_edges().to_lower();
	Array slots = get_actor_arpg_equip_slots(p_actor_id);
	int target_idx = -1;
	String target_cat_key;
	for (int i = 0; i < slots.size(); i++) {
		Dictionary sd = slots[i];
		if (String(sd.get("slot", "")).strip_edges().to_lower() == norm_slot) {
			target_idx = i;
			target_cat_key = String(sd.get("category", ""));
			break;
		}
	}
	if (target_idx < 0) {
		return false;
	}
	int cat = get_category_from_key(target_cat_key);
	if (p_piece_id > 0) {
		if (cat < 0 || get_record_by_id(cat, p_piece_id).is_empty()) {
			return false;
		}
	}
	Dictionary sd = slots[target_idx];
	sd["piece_id"] = MAX(0, p_piece_id);
	slots[target_idx] = sd;
	runtime_arpg_equips[String::num_int64(p_actor_id)] = slots;
	emit_signal("data_changed", "arpg_equip", p_actor_id);
	return true;
}

int ClockGearsDataManager::get_actor_arpg_equip(int p_actor_id, const String &p_slot_id) const {
	String norm_slot = p_slot_id.strip_edges().to_lower();
	Array slots = get_actor_arpg_equip_slots(p_actor_id);
	for (int i = 0; i < slots.size(); i++) {
		if (slots[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary sd = slots[i];
		if (String(sd.get("slot", "")).strip_edges().to_lower() == norm_slot) {
			return int(sd.get("piece_id", 0));
		}
	}
	return 0;
}

Dictionary ClockGearsDataManager::get_actor_arpg_equip_bonuses(int p_actor_id) const {
	static const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
	Dictionary sum;
	for (int k = 0; k < 8; k++) {
		sum[stat_keys[k]] = 0;
	}
	Array slots = get_actor_arpg_equip_slots(p_actor_id);
	for (int i = 0; i < slots.size(); i++) {
		if (slots[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary sd = slots[i];
		int pid = int(sd.get("piece_id", 0));
		if (pid <= 0) {
			continue;
		}
		int cat = get_category_from_key(String(sd.get("category", "")));
		if (cat < 0) {
			continue;
		}
		Dictionary piece = get_record_by_id(cat, pid);
		if (piece.is_empty()) {
			continue;
		}
		for (int k = 0; k < 8; k++) {
			sum[stat_keys[k]] = int(sum[stat_keys[k]]) + int(piece.get(stat_keys[k], 0));
		}
	}
	return sum;
}

Dictionary ClockGearsDataManager::get_actor_effective_stats(int p_actor_id) const {
	static const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
	Dictionary act = get_record_by_id(CAT_ACTORS, p_actor_id);
	Dictionary bonuses = get_actor_arpg_equip_bonuses(p_actor_id);
	Dictionary res;
	for (int k = 0; k < 8; k++) {
		res[stat_keys[k]] = int(act.get(stat_keys[k], 0)) + int(bonuses.get(stat_keys[k], 0));
	}
	return res;
}

Dictionary ClockGearsDataManager::export_runtime_state() const {
	Dictionary d;
	d["switches"] = runtime_switches.duplicate(true);
	d["variables"] = runtime_variables.duplicate(true);
	d["inventory"] = runtime_inventory.duplicate(true);
	d["gold"] = runtime_gold;
	d["arpg_equips"] = runtime_arpg_equips.duplicate(true);
	return d;
}

void ClockGearsDataManager::import_runtime_state(const Dictionary &p_state) {
	if (p_state.has("switches") && p_state["switches"].get_type() == Variant::DICTIONARY) {
		runtime_switches = Dictionary(p_state["switches"]).duplicate(true);
	} else {
		runtime_switches.clear();
	}
	if (p_state.has("variables") && p_state["variables"].get_type() == Variant::DICTIONARY) {
		runtime_variables = Dictionary(p_state["variables"]).duplicate(true);
	} else {
		runtime_variables.clear();
	}
	if (p_state.has("inventory") && p_state["inventory"].get_type() == Variant::DICTIONARY) {
		runtime_inventory = Dictionary(p_state["inventory"]).duplicate(true);
	} else {
		runtime_inventory.clear();
	}
	runtime_gold = MAX(0, int(p_state.get("gold", 0)));
	runtime_arpg_equips.clear();
	if (p_state.has("arpg_equips") && p_state["arpg_equips"].get_type() == Variant::DICTIONARY) {
		Dictionary eq = p_state["arpg_equips"];
		for (const Variant &k : eq.keys()) {
			if (eq[k].get_type() == Variant::ARRAY) {
				runtime_arpg_equips[String(k)] = normalize_arpg_equip_slots(Array(eq[k]));
			}
		}
	}
}

String ClockGearsDataManager::format_text(const String &p_text) const {
	String out;
	int len = p_text.length();
	for (int i = 0; i < len; i++) {
		char32_t c = p_text[i];
		if (c == '\\' && i + 1 < len) {
			char32_t next = p_text[i + 1];
			if (next == '\\') {
				out += "\\";
				i++;
				continue;
			}
			if (next == 'G' || next == 'g') {
				String cur_unit = vocabulary_config.has("gold") ? String(vocabulary_config["gold"]) : "Oro";
				out += cur_unit;
				i++;
				continue;
			}
			if ((next == 'V' || next == 'v' || next == 'N' || next == 'n' || next == 'P' || next == 'p' || next == 'C' || next == 'c' || next == 'I' || next == 'i') && i + 2 < len && p_text[i + 2] == '[') {
				int close_idx = p_text.find("]", i + 3);
				if (close_idx != -1) {
					String inside = p_text.substr(i + 3, close_idx - (i + 3)).strip_edges();
					if (next == 'V' || next == 'v') {
						int val = get_variable(inside);
						out += itos(val);
					} else if (next == 'N' || next == 'n') {
						int act_id = inside.to_int();
						Dictionary act = get_record_by_id(CAT_ACTORS, act_id);
						String act_name = act.has("name") ? String(act["name"]) : vformat("Actor %d", act_id);
						out += act_name;
					} else if (next == 'C' || next == 'c') {
						int col_idx = inside.to_int();
						static const char *palette[] = {
							"#ffffff", "#20a0d6", "#ff7840", "#66cc40",
							"#80c0ff", "#e080ff", "#f0c040", "#a0a0a0",
							"#38c8ff", "#ff5050", "#ffff50", "#50ff50",
							"#d0d0d0", "#808080", "#ff8080", "#80ff80"
						};
						if (col_idx == 0) {
							out += "[/color]";
						} else {
							int p_idx = CLAMP(col_idx, 0, 15);
							out += vformat("[color=%s]", palette[p_idx]);
						}
					} else if (next == 'I' || next == 'i') {
						out += vformat("[icon=%s]", inside);
					}
					i = close_idx;
					continue;
				}
			}
		}
		out += String::chr(c);
	}
	return out;
}

void ClockGearsDataManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init_empty"), &ClockGearsDataManager::init_empty);
	ClassDB::bind_method(D_METHOD("is_dirty"), &ClockGearsDataManager::is_dirty);
	ClassDB::bind_method(D_METHOD("mark_dirty", "dirty"), &ClockGearsDataManager::mark_dirty, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_category_array", "category"), &ClockGearsDataManager::get_category_array);
	ClassDB::bind_method(D_METHOD("set_category_array", "category", "array"), &ClockGearsDataManager::set_category_array);
	ClassDB::bind_method(D_METHOD("get_records", "category_key"), &ClockGearsDataManager::get_records);
	ClassDB::bind_method(D_METHOD("set_records", "category_key", "records"), &ClockGearsDataManager::set_records);
	ClassDB::bind_method(D_METHOD("get_record_by_id", "category", "id"), &ClockGearsDataManager::get_record_by_id);
	ClassDB::bind_method(D_METHOD("create_record", "category", "initial_data"), &ClockGearsDataManager::create_record, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("update_record", "category", "id", "record"), &ClockGearsDataManager::update_record);
	ClassDB::bind_method(D_METHOD("delete_record_by_id", "category", "id"), &ClockGearsDataManager::delete_record_by_id);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_normalized_record", "category", "record"), &ClockGearsDataManager::get_normalized_record);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_arpg_ai_types"), &ClockGearsDataManager::get_arpg_ai_types);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_npc_move_types"), &ClockGearsDataManager::get_npc_move_types);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_detection_zone_triggers"), &ClockGearsDataManager::get_detection_zone_triggers);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_detection_zone_actions"), &ClockGearsDataManager::get_detection_zone_actions);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_detection_zone", "zone", "idx"), &ClockGearsDataManager::normalize_detection_zone, DEFVAL(0));
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_mannequin_size", "size"), &ClockGearsDataManager::normalize_mannequin_size);
	ClassDB::bind_method(D_METHOD("get_system_config"), &ClockGearsDataManager::get_system_config);
	ClassDB::bind_method(D_METHOD("set_system_config", "config"), &ClockGearsDataManager::set_system_config);
	ClassDB::bind_method(D_METHOD("get_mannequin_size"), &ClockGearsDataManager::get_mannequin_size);
	ClassDB::bind_method(D_METHOD("set_mannequin_size", "size"), &ClockGearsDataManager::set_mannequin_size);
	ClassDB::bind_method(D_METHOD("get_exported_system_config"), &ClockGearsDataManager::get_exported_system_config);
	ClassDB::bind_method(D_METHOD("get_vocabulary_config"), &ClockGearsDataManager::get_vocabulary_config);
	ClassDB::bind_method(D_METHOD("set_vocabulary_config", "vocab"), &ClockGearsDataManager::set_vocabulary_config);
	ClassDB::bind_method(D_METHOD("get_switches_names"), &ClockGearsDataManager::get_switches_names);
	ClassDB::bind_method(D_METHOD("set_switches_names", "names"), &ClockGearsDataManager::set_switches_names);
	ClassDB::bind_method(D_METHOD("get_variables_names"), &ClockGearsDataManager::get_variables_names);
	ClassDB::bind_method(D_METHOD("set_variables_names", "names"), &ClockGearsDataManager::set_variables_names);
	ClassDB::bind_method(D_METHOD("save_all", "dir_path"), &ClockGearsDataManager::save_all, DEFVAL("res://clockgears_data"));
	ClassDB::bind_method(D_METHOD("load_all", "dir_path"), &ClockGearsDataManager::load_all, DEFVAL("res://clockgears_data"));
	ClassDB::bind_method(D_METHOD("save_autosave", "base_dir"), &ClockGearsDataManager::save_autosave, DEFVAL("res://clockgears_data/autosave"));
	ClassDB::bind_method(D_METHOD("save_database", "path"), &ClockGearsDataManager::save_database, DEFVAL("res://clockgears_data/database.json"));
	ClassDB::bind_method(D_METHOD("load_database", "path"), &ClockGearsDataManager::load_database, DEFVAL("res://clockgears_data/database.json"));

	ClassDB::bind_method(D_METHOD("get_switch", "name"), &ClockGearsDataManager::get_switch);
	ClassDB::bind_method(D_METHOD("set_switch", "name", "val"), &ClockGearsDataManager::set_switch);
	ClassDB::bind_method(D_METHOD("get_variable", "name"), &ClockGearsDataManager::get_variable);
	ClassDB::bind_method(D_METHOD("set_variable", "name", "val"), &ClockGearsDataManager::set_variable);
	ClassDB::bind_method(D_METHOD("has_item", "name", "min_qty"), &ClockGearsDataManager::has_item, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("change_item", "name", "amount"), &ClockGearsDataManager::change_item);
	ClassDB::bind_method(D_METHOD("get_gold"), &ClockGearsDataManager::get_gold);
	ClassDB::bind_method(D_METHOD("set_gold", "gold"), &ClockGearsDataManager::set_gold);
	ClassDB::bind_method(D_METHOD("change_gold", "amount"), &ClockGearsDataManager::change_gold);
	ClassDB::bind_method(D_METHOD("reset_runtime_state"), &ClockGearsDataManager::reset_runtime_state);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_arpg_equip_slots"), &ClockGearsDataManager::get_default_arpg_equip_slots);
	ClassDB::bind_method(D_METHOD("get_actor_arpg_equip_slots", "actor_id"), &ClockGearsDataManager::get_actor_arpg_equip_slots);
	ClassDB::bind_method(D_METHOD("set_actor_arpg_equip", "actor_id", "slot_id", "piece_id"), &ClockGearsDataManager::set_actor_arpg_equip);
	ClassDB::bind_method(D_METHOD("get_actor_arpg_equip", "actor_id", "slot_id"), &ClockGearsDataManager::get_actor_arpg_equip);
	ClassDB::bind_method(D_METHOD("get_actor_arpg_equip_bonuses", "actor_id"), &ClockGearsDataManager::get_actor_arpg_equip_bonuses);
	ClassDB::bind_method(D_METHOD("get_actor_effective_stats", "actor_id"), &ClockGearsDataManager::get_actor_effective_stats);
	ClassDB::bind_method(D_METHOD("export_runtime_state"), &ClockGearsDataManager::export_runtime_state);
	ClassDB::bind_method(D_METHOD("import_runtime_state", "state"), &ClockGearsDataManager::import_runtime_state);
	ClassDB::bind_method(D_METHOD("format_text", "text"), &ClockGearsDataManager::format_text);

	ClassDB::bind_method(D_METHOD("set_editor_map", "map"), &ClockGearsDataManager::set_editor_map);
	ClassDB::bind_method(D_METHOD("get_editor_map"), &ClockGearsDataManager::get_editor_map);

	ClassDB::bind_method(D_METHOD("simulate_editor_mouse_press", "canvas_pos", "right_button"), &ClockGearsDataManager::simulate_editor_mouse_press, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("simulate_editor_mouse_motion", "canvas_pos"), &ClockGearsDataManager::simulate_editor_mouse_motion);
	ClassDB::bind_method(D_METHOD("simulate_editor_mouse_release", "canvas_pos", "right_button"), &ClockGearsDataManager::simulate_editor_mouse_release, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("simulate_editor_click_cell", "cell_x", "cell_y", "right_button"), &ClockGearsDataManager::simulate_editor_click_cell, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("simulate_editor_drag_rect", "x0", "y0", "x1", "y1", "right_button"), &ClockGearsDataManager::simulate_editor_drag_rect, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("set_editor_draw_tool", "tool"), &ClockGearsDataManager::set_editor_draw_tool);
	ClassDB::bind_method(D_METHOD("set_editor_layer", "layer"), &ClockGearsDataManager::set_editor_layer);
	ClassDB::bind_method(D_METHOD("trigger_editor_undo"), &ClockGearsDataManager::trigger_editor_undo);
	ClassDB::bind_method(D_METHOD("trigger_editor_redo"), &ClockGearsDataManager::trigger_editor_redo);

	ADD_SIGNAL(MethodInfo("data_changed", PropertyInfo(Variant::STRING, "category"), PropertyInfo(Variant::INT, "id")));
}

void ClockGearsDataManager::set_editor_map(Node *p_map) {
	active_editor_map = p_map;
#ifdef TOOLS_ENABLED
	if (EditorNode::get_singleton() && p_map && !p_map->get_parent()) {
		EditorNode::get_singleton()->set_edited_scene(p_map);
	}
#endif
}

void ClockGearsDataManager::simulate_editor_mouse_press(const Vector2 &p_canvas_pos, bool p_right_button) {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->simulate_mouse_press(p_canvas_pos, p_right_button);
	}
#endif
}

void ClockGearsDataManager::simulate_editor_mouse_motion(const Vector2 &p_canvas_pos) {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->simulate_mouse_motion(p_canvas_pos);
	}
#endif
}

void ClockGearsDataManager::simulate_editor_mouse_release(const Vector2 &p_canvas_pos, bool p_right_button) {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->simulate_mouse_release(p_canvas_pos, p_right_button);
	}
#endif
}

void ClockGearsDataManager::simulate_editor_click_cell(int p_cell_x, int p_cell_y, bool p_right_button) {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		Point2 pos(p_cell_x * 32 + 16, p_cell_y * 32 + 16);
		ed->simulate_mouse_press(pos, p_right_button);
		ed->simulate_mouse_release(pos, p_right_button);
	}
#endif
}

void ClockGearsDataManager::simulate_editor_drag_rect(int p_x0, int p_y0, int p_x1, int p_y1, bool p_right_button) {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		Point2 p0(p_x0 * 32 + 16, p_y0 * 32 + 16);
		Point2 p1(p_x1 * 32 + 16, p_y1 * 32 + 16);
		ed->simulate_mouse_press(p0, p_right_button);
		ed->simulate_mouse_motion(p1);
		ed->simulate_mouse_release(p1, p_right_button);
	}
#endif
}

void ClockGearsDataManager::set_editor_draw_tool(int p_tool) {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->set_clockgears_draw_tool(p_tool);
	}
#endif
}

void ClockGearsDataManager::set_editor_layer(int p_layer) {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->set_clockgears_current_layer(p_layer);
	}
#endif
}

void ClockGearsDataManager::trigger_editor_undo() {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->trigger_undo();
	}
#endif
}

void ClockGearsDataManager::trigger_editor_redo() {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->trigger_redo();
	}
#endif
}

ClockGearsDataManager::ClockGearsDataManager() {
	singleton = this;
	init_empty();
}

ClockGearsDataManager::~ClockGearsDataManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
