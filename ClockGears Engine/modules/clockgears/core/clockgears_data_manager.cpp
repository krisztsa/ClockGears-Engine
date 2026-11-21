#include "clockgears_data_manager.h"
#include "../clockgears_animation.h"
#include "../clockgears_battle.h"
#include "../clockgears_map.h"
#include "../clockgears_types.h"
#include "../icons/clockgears_icons.h"
#include "../runtime/clockgears_audio_manager.h"
#include "../runtime/clockgears_runtime.h"
#include "clockgears_formulas.h"
#include "clockgears_id_registry.h"
#include "clockgears_system_catalog.h"
#include "clockgears_time_manager.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/input/input_map.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/json.h"
#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "core/math/math_funcs.h"
#include "core/object/undo_redo.h"
#include "core/os/keyboard.h"
#include "core/os/time.h"
#include "scene/gui/color_rect.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/canvas_item_material.h"
#include "scene/resources/material.h"
#include "modules/tilemap/tile_set.h"
#include "servers/display/display_server.h"
#ifdef TOOLS_ENABLED
#include "../clockgears_top_bar.h"
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
	system_config["currency"] = "G";
	system_config["combat_mode"] = 0; // 0 = Turnos Clásico, 1 = Action RPG Tiempo Real
	system_config["movement_directions"] = 4; // 4 o 8 direcciones de movimiento
	system_config["mannequin_size"] = 32; // Tamaño de maniquí de referencia (32, 48, 64; no se exporta)
	normalize_arpg_system_config(system_config);
	system_config["start_map"] = "";
	system_config["start_x"] = 0;
	system_config["start_y"] = 0;
	system_config["max_followers"] = 3;
	system_config["sell_price_ratio"] = 0.5;
	system_config["autosave_minutes"] = 5;
	system_config["default_max_item_stack"] = 99;
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

	title_menu_config = get_default_title_menu_config();
	game_menu_config = get_default_game_menu_config();
	credits_config = get_default_credits_config();
	game_over_config = get_default_game_over_config();
	hud_config = get_default_hud_config();
	ui_designs_config = get_default_ui_designs_config();
	ui_design_undo_stacks.clear();
	ui_design_redo_stacks.clear();
	runtime_hud_visible = true;
	runtime_hud_widget_visibility.clear();
	ensure_default_window_types_registered();
	window_styles_config = get_default_window_styles_config();
	if (!player_options_loaded) {
		player_options = get_default_player_options();
	}
	pending_loaded_map_path = "";

	switches_names.clear();
	variables_names.clear();
	runtime_switches.clear();
	runtime_variables.clear();
	runtime_inventory.clear();
	runtime_equips.clear();
	runtime_arpg_equips.clear();
	runtime_actor_states.clear();
	runtime_defeated_enemies.clear();
	runtime_window_style_overrides.clear();
	runtime_playtime_sec = 0.0;
	runtime_gold = 0;
	runtime_game_menu_disabled = false;
	built_tileset_cache.clear();
	entry_icon_cache.clear();
	_invalidate_window_styles_cache();
	active_editor_map = nullptr;
	active_editor_map_id = ObjectID();
	active_editor_layer = 1;
	active_editor_custom_layer_id = String();
	headless_draw_tool = 0;
	headless_selected_source_id = 0;
	headless_selected_tile_coords = Vector2i(0, 0);
	headless_has_selection = false;
	headless_selection_rect = Rect2i();
	headless_clipboard.clear();
	if (headless_undo_redo) {
		headless_undo_redo->clear_history();
	}
	if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
		dim_inactive_layers = bool(ps->get_setting("clockgears/editor/dim_inactive_layers", true));
		inactive_layer_opacity_pct = CLAMP(int(ps->get_setting("clockgears/editor/inactive_layer_opacity_pct", 40)), 10, 100);
	} else {
		dim_inactive_layers = true;
		inactive_layer_opacity_pct = 40;
	}
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
	String base_dir = p_file_path.get_base_dir();
	if (!base_dir.is_empty()) {
		DirAccess::make_dir_recursive_absolute(base_dir);
	}
	String tmp_path = p_file_path + ".tmp";
	String bak_path = p_file_path + ".bak";

	String json_text = JSON::stringify(p_data, "\t");
	{
		Ref<FileAccess> f = FileAccess::open(tmp_path, FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(f.is_null(), false, vformat("ClockGearsDataManager: No se pudo escribir archivo temporal '%s'", tmp_path));
		f->store_string(json_text);
		f->flush();
	}

	Ref<DirAccess> dir = DirAccess::create_for_path(p_file_path);
	if (dir.is_null()) {
		dir = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	}
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

static int _unicode_to_win1252_byte(char32_t p_ch) {
	if (p_ch <= 0xFF) {
		return int(p_ch);
	}
	switch (p_ch) {
		case 0x20AC: return 0x80; // €
		case 0x201A: return 0x82; // ‚
		case 0x0192: return 0x83; // ƒ
		case 0x201E: return 0x84; // „
		case 0x2026: return 0x85; // …
		case 0x2020: return 0x86; // †
		case 0x2021: return 0x87; // ‡
		case 0x02C6: return 0x88; // ˆ
		case 0x2030: return 0x89; // ‰
		case 0x0160: return 0x8A; // Š
		case 0x2039: return 0x8B; // ‹
		case 0x0152: return 0x8C; // Œ
		case 0x017D: return 0x8E; // Ž
		case 0x2018: return 0x91; // ‘
		case 0x2019: return 0x92; // ’
		case 0x201C: return 0x93; // “
		case 0x201D: return 0x94; // ”
		case 0x2022: return 0x95; // •
		case 0x2013: return 0x96; // –
		case 0x2014: return 0x97; // —
		case 0x02DC: return 0x98; // ˜
		case 0x2122: return 0x99; // ™
		case 0x0161: return 0x9A; // š
		case 0x203A: return 0x9B; // ›
		case 0x0153: return 0x9C; // œ
		case 0x017E: return 0x9E; // ž
		case 0x0178: return 0x9F; // Ÿ
		default: return -1;
	}
}

bool ClockGearsDataManager::is_likely_latin1_mojibake(const String &p_text) {
	const int n = p_text.length();
	for (int i = 0; i < n; i++) {
		char32_t c = p_text[i];
		if ((c == 0x00C3 || c == 0x00C2) && i + 1 < n) {
			int b1 = _unicode_to_win1252_byte(p_text[i + 1]);
			if (b1 >= 0x80 && b1 <= 0xBF) {
				return true;
			}
		}
		if (c == 0x00E2 && i + 2 < n) {
			int b1 = _unicode_to_win1252_byte(p_text[i + 1]);
			int b2 = _unicode_to_win1252_byte(p_text[i + 2]);
			if (b1 >= 0x80 && b1 <= 0xBF && b2 >= 0x80 && b2 <= 0xBF) {
				return true;
			}
		}
		if (c == 0x00F0 && i + 3 < n) {
			int b1 = _unicode_to_win1252_byte(p_text[i + 1]);
			int b2 = _unicode_to_win1252_byte(p_text[i + 2]);
			int b3 = _unicode_to_win1252_byte(p_text[i + 3]);
			if (b1 >= 0x80 && b1 <= 0xBF && b2 >= 0x80 && b2 <= 0xBF && b3 >= 0x80 && b3 <= 0xBF) {
				return true;
			}
		}
	}
	return false;
}

String ClockGearsDataManager::repair_mojibake_string(const String &p_text, bool *r_repaired) {
	if (r_repaired) {
		*r_repaired = false;
	}
	if (p_text.is_empty() || !is_likely_latin1_mojibake(p_text)) {
		return p_text;
	}

	String cur = p_text;
	bool any_changed = false;
	for (int pass = 0; pass < 2; pass++) {
		if (!is_likely_latin1_mojibake(cur)) {
			break;
		}
		String out;
		bool changed = false;
		const int n = cur.length();
		for (int i = 0; i < n; i++) {
			int b0 = _unicode_to_win1252_byte(cur[i]);
			if (b0 >= 0xC2 && b0 <= 0xDF && i + 1 < n) {
				int b1 = _unicode_to_win1252_byte(cur[i + 1]);
				if (b1 >= 0x80 && b1 <= 0xBF) {
					char32_t cp = char32_t(((b0 & 0x1F) << 6) | (b1 & 0x3F));
					if (cp >= 0x0080 && cp != 0xFFFD) {
						out += String::chr(cp);
						i += 1;
						changed = true;
						continue;
					}
				}
			} else if (b0 >= 0xE0 && b0 <= 0xEF && i + 2 < n) {
				int b1 = _unicode_to_win1252_byte(cur[i + 1]);
				int b2 = _unicode_to_win1252_byte(cur[i + 2]);
				if (b1 >= 0x80 && b1 <= 0xBF && b2 >= 0x80 && b2 <= 0xBF) {
					char32_t cp = char32_t(((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F));
					if (cp >= 0x0800 && !(cp >= 0xD800 && cp <= 0xDFFF) && cp != 0xFFFD) {
						out += String::chr(cp);
						i += 2;
						changed = true;
						continue;
					}
				}
			} else if (b0 >= 0xF0 && b0 <= 0xF4 && i + 3 < n) {
				int b1 = _unicode_to_win1252_byte(cur[i + 1]);
				int b2 = _unicode_to_win1252_byte(cur[i + 2]);
				int b3 = _unicode_to_win1252_byte(cur[i + 3]);
				if (b1 >= 0x80 && b1 <= 0xBF && b2 >= 0x80 && b2 <= 0xBF && b3 >= 0x80 && b3 <= 0xBF) {
					char32_t cp = char32_t(((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F));
					if (cp >= 0x10000 && cp <= 0x10FFFF) {
						out += String::chr(cp);
						i += 3;
						changed = true;
						continue;
					}
				}
			}
			out += cur[i];
		}
		if (!changed) {
			break;
		}
		cur = out;
		any_changed = true;
	}
	if (r_repaired) {
		*r_repaired = any_changed;
	}
	return cur;
}

Variant ClockGearsDataManager::repair_mojibake_variant(const Variant &p_val, int &r_repaired_strings) {
	switch (p_val.get_type()) {
		case Variant::STRING: {
			bool rep = false;
			String fixed = repair_mojibake_string(String(p_val), &rep);
			if (rep) {
				r_repaired_strings++;
			}
			return fixed;
		}
		case Variant::ARRAY: {
			Array in_arr = p_val;
			Array out_arr;
			out_arr.resize(in_arr.size());
			for (int i = 0; i < in_arr.size(); i++) {
				out_arr[i] = repair_mojibake_variant(in_arr[i], r_repaired_strings);
			}
			return out_arr;
		}
		case Variant::DICTIONARY: {
			Dictionary in_dict = p_val;
			Dictionary out_dict;
			for (const Variant &key : in_dict.keys()) {
				Variant fixed_k = repair_mojibake_variant(key, r_repaired_strings);
				Variant fixed_v = repair_mojibake_variant(in_dict[key], r_repaired_strings);
				out_dict[fixed_k] = fixed_v;
			}
			return out_dict;
		}
		default:
			return p_val;
	}
}

String ClockGearsDataManager::truncate_with_ellipsis(const String &p_text, int p_max_chars) {
	if (p_max_chars <= 0 || p_text.length() <= p_max_chars) {
		return p_text;
	}
	// Godot String indexa por codepoints UTF-32 (caracteres reales, nunca por bytes UTF-8).
	return p_text.substr(0, p_max_chars) + String::utf8("…");
}

PackedStringArray ClockGearsDataManager::get_last_repaired_json_files() const {
	PackedStringArray out;
	for (int i = 0; i < last_repaired_json_files.size(); i++) {
		out.push_back(last_repaired_json_files[i]);
	}
	return out;
}

bool ClockGearsDataManager::_read_json_with_backup(const String &p_file_path, Variant &r_data, bool p_repair_mojibake) const {
	String bak_path = p_file_path + ".bak";
	String loaded_from;
	Variant parsed_data;

	if (FileAccess::exists(p_file_path)) {
		Ref<FileAccess> f = FileAccess::open(p_file_path, FileAccess::READ);
		if (f.is_valid()) {
			String text = f->get_as_text();
			Ref<JSON> json;
			json.instantiate();
			if (json->parse(text) == OK) {
				parsed_data = _normalize_json_numbers(json->get_data());
				loaded_from = p_file_path;
			} else {
				WARN_PRINT(vformat(String::utf8("ClockGearsDataManager: Archivo '%s' corrupto (línea %d: %s). Intentando recuperar desde '%s'..."), p_file_path, json->get_error_line(), json->get_error_message(), bak_path));
			}
		}
	}

	if (loaded_from.is_empty() && FileAccess::exists(bak_path)) {
		Ref<FileAccess> f_bak = FileAccess::open(bak_path, FileAccess::READ);
		if (f_bak.is_valid()) {
			String text = f_bak->get_as_text();
			Ref<JSON> json;
			json.instantiate();
			if (json->parse(text) == OK) {
				parsed_data = _normalize_json_numbers(json->get_data());
				loaded_from = bak_path;
				WARN_PRINT(vformat("ClockGearsDataManager: Recuperado exitosamente desde respaldo '%s'.", bak_path));
			}
		}
	}

	if (loaded_from.is_empty()) {
		return false;
	}

	if (p_repair_mojibake) {
		int repaired_strings = 0;
		Variant repaired_data = repair_mojibake_variant(parsed_data, repaired_strings);
		if (repaired_strings > 0) {
			String orig_bak = loaded_from + ".pre_utf8_fix.bak";
			if (!FileAccess::exists(orig_bak)) {
				PackedByteArray raw_bytes = FileAccess::get_file_as_bytes(loaded_from);
				Ref<FileAccess> fb = FileAccess::open(orig_bak, FileAccess::WRITE);
				if (fb.is_valid()) {
					fb->store_buffer(raw_bytes);
					fb->flush();
				}
			}
			_write_json_atomic(p_file_path, repaired_data);
			if (!last_repaired_json_files.has(p_file_path)) {
				last_repaired_json_files.push_back(p_file_path);
			}
			parsed_data = repaired_data;
		}
	}

	r_data = parsed_data;
	return true;
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
	return ClockGearsHitShape::make_default_body().to_dict();
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
	double atk_iv_var = r_record.has("attack_interval_variance_pct") ? CLAMP(double(r_record["attack_interval_variance_pct"]), 0.0, 100.0) : 0.0;
	double atk_spd = r_record.has("attack_speed_pct") ? MAX(1.0, double(r_record["attack_speed_pct"])) : 100.0;
	int atk_windup = r_record.has("attack_windup_ms") ? MAX(0, int(r_record["attack_windup_ms"])) : 0;
	int atk_recov = r_record.has("attack_recovery_ms") ? MAX(0, int(r_record["attack_recovery_ms"])) : 0;
	double atk_dmg_mult = r_record.has("attack_damage_mult_pct") ? MAX(0.0, double(r_record["attack_damage_mult_pct"])) : 100.0;
	r_record["vision_radius"] = vis_r;
	r_record["attack_range"] = atk_r;
	r_record["attack_interval"] = atk_iv;
	r_record["attack_interval_variance_pct"] = atk_iv_var;
	r_record["attack_speed_pct"] = atk_spd;
	r_record["attack_windup_ms"] = atk_windup;
	r_record["attack_recovery_ms"] = atk_recov;
	r_record["attack_damage_mult_pct"] = atk_dmg_mult;
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
		Array norm_zones = s_normalize_detection_zones(r_record["detection_zones"]);
		for (int zi = 0; zi < norm_zones.size(); zi++) {
			if (norm_zones[zi].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary zd = norm_zones[zi];
			String zname = String(zd.get("name", ""));
			String tf = String(zd.get("target_filter", "player")).strip_edges().to_lower();
			String trig = String(zd.get("trigger", "enter")).strip_edges().to_lower();
			Array acts = zd.get("actions", Array());
			String act0_type;
			if (acts.size() == 1 && acts[0].get_type() == Variant::DICTIONARY) {
				act0_type = String(Dictionary(acts[0]).get("type", "")).strip_edges().to_lower();
			}
			ClockGearsHitShape sh = ClockGearsShapeGeometry::shape_from_variant(zd.get("shape", Dictionary()));
			bool is_centered_circle = (sh.type == CG_SHAPE_CIRCLE && Math::abs(sh.offset.x) < 0.01f && Math::abs(sh.offset.y) < 0.01f);

			// Solo migrar/actualizar si coincide con la zona migrada original y no fue editada por el creador.
			if (zname == "Ataque" && tf == "player" && trig == "enter" && act0_type == "atacar" &&
					is_centered_circle && atk_r > 0.0 && Math::abs(sh.get_radius() - float(atk_r * 32.0)) < 0.1f) {
				zd["trigger"] = "while_inside";
				zd["interval_sec"] = MAX(0.1, atk_iv);
				norm_zones[zi] = zd;
			} else if (zname == "Zona 2" && tf == "player" && (act0_type == "perseguir" || act0_type == "huir") &&
					is_centered_circle && vis_r > 0.0 && Math::abs(sh.get_radius() - float(vis_r * 32.0)) < 0.1f) {
				zd["name"] = String::utf8("Visión");
				norm_zones[zi] = zd;
			}
		}
		r_record["detection_zones"] = norm_zones;
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
	if (p_category == CAT_TILESETS) {
		built_tileset_cache.clear();
	}
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
				if (p_category == CAT_TILESETS) {
					built_tileset_cache.clear();
				}
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
				if (p_category == CAT_TILESETS) {
					built_tileset_cache.clear();
				}
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

bool ClockGearsDataManager::parse_tile_coords_variant(const Variant &p_var, Vector2i &r_coords) {
	switch (p_var.get_type()) {
		case Variant::VECTOR2I: {
			r_coords = p_var;
			return (r_coords.x >= 0 && r_coords.y >= 0);
		}
		case Variant::VECTOR2: {
			Vector2 v = p_var;
			r_coords = Vector2i(int(Math::round(v.x)), int(Math::round(v.y)));
			return (r_coords.x >= 0 && r_coords.y >= 0);
		}
		case Variant::ARRAY: {
			Array a = p_var;
			if (a.size() >= 2) {
				r_coords = Vector2i(int(a[0]), int(a[1]));
				return (r_coords.x >= 0 && r_coords.y >= 0);
			}
			return false;
		}
		case Variant::DICTIONARY: {
			Dictionary d = p_var;
			if (d.has("x") && d.has("y")) {
				r_coords = Vector2i(int(d["x"]), int(d["y"]));
				return (r_coords.x >= 0 && r_coords.y >= 0);
			}
			return false;
		}
		case Variant::STRING: {
			String s = String(p_var).strip_edges();
			s = s.replace("(", "").replace(")", "").replace("[", "").replace("]", "").strip_edges();
			PackedStringArray parts = s.split(",", false);
			if (parts.size() >= 2) {
				String p0 = parts[0].strip_edges();
				String p1 = parts[1].strip_edges();
				if (p0.is_valid_int() && p1.is_valid_int()) {
					r_coords = Vector2i(p0.to_int(), p1.to_int());
					return (r_coords.x >= 0 && r_coords.y >= 0);
				}
			}
			return false;
		}
		default:
			return false;
	}
}

Vector2i ClockGearsDataManager::parse_tile_block_variant(const Variant &p_var) {
	Vector2i blk(1, 1);
	if (parse_tile_coords_variant(p_var, blk)) {
		return Vector2i(MAX(1, blk.x), MAX(1, blk.y));
	}
	return Vector2i(1, 1);
}

void ClockGearsDataManager::clear_entry_icon_cache() {
	entry_icon_cache.clear();
}

Ref<Texture2D> ClockGearsDataManager::get_entry_icon_texture(const Variant &p_entry, int p_target_px, const Ref<Texture2D> &p_fallback) const {
	if (p_target_px <= 0) {
		p_target_px = 32;
	}

	Dictionary rec;
	int cat_hint = -1;

	if (p_entry.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_entry;
		if (d.has("record") && d["record"].get_type() == Variant::DICTIONARY) {
			rec = d["record"];
			cat_hint = int(d.get("category", -1));
		} else if (d.has("piece_id") && d.has("category")) {
			int cat = get_category_from_key(String(d.get("category", "")));
			int pid = int(d.get("piece_id", 0));
			if (cat >= 0 && pid > 0) {
				rec = get_record_by_id(cat, pid);
				cat_hint = cat;
			}
		} else {
			rec = d;
			cat_hint = int(d.get("category", -1));
		}
	} else if (p_entry.get_type() == Variant::STRING) {
		String key_or_name = p_entry;
		Dictionary info = find_inventory_record(key_or_name);
		if (bool(info.get("found", false))) {
			rec = info.get("record", Dictionary());
			cat_hint = int(info.get("category", -1));
		} else {
			static const int k_cats[8] = {
				CAT_ITEMS, CAT_WEAPONS, CAT_ARMORS, CAT_ACCESSORIES,
				CAT_ARPG_WEAPONS, CAT_ARPG_ARMORS, CAT_ARPG_SHIELDS, CAT_ARPG_ACCESSORIES
			};
			String kn = key_or_name.strip_edges().to_lower();
			for (int ci = 0; ci < 8; ci++) {
				int cat = k_cats[ci];
				const Array &arr = category_data[cat];
				for (int i = 0; i < arr.size(); i++) {
					if (arr[i].get_type() != Variant::DICTIONARY) continue;
					Dictionary r = arr[i];
					if (String(r.get("name", "")).strip_edges().to_lower() == kn) {
						rec = r;
						cat_hint = cat;
						break;
					}
				}
				if (!rec.is_empty()) break;
			}
		}
	}

	if (rec.is_empty()) {
		if (p_fallback.is_valid()) {
			return p_fallback;
		}
		if (ClockGearsIcons::get_singleton() && cat_hint >= 0) {
			return ClockGearsIcons::get_singleton()->get_db_category_icon(cat_hint, float(p_target_px) / 32.0f);
		}
		return Ref<Texture2D>();
	}

	String gpath = String(rec.get("graphic_path", rec.get("icon_path", ""))).strip_edges();
	if (gpath.is_empty()) {
		if (p_fallback.is_valid()) {
			return p_fallback;
		}
		if (ClockGearsIcons::get_singleton() && cat_hint >= 0) {
			return ClockGearsIcons::get_singleton()->get_db_category_icon(cat_hint, float(p_target_px) / 32.0f);
		}
		return Ref<Texture2D>();
	}

	Vector2i tc;
	bool has_tc = parse_tile_coords_variant(rec.get("tile_coords", Variant()), tc);
	Vector2i blk = has_tc ? parse_tile_block_variant(rec.get("tile_block_size", Variant())) : Vector2i(1, 1);

	String cache_key = has_tc
			? vformat("%s|%d,%d|%dx%d|%d", gpath, tc.x, tc.y, blk.x, blk.y, p_target_px)
			: vformat("%s|full|%d", gpath, p_target_px);

	if (entry_icon_cache.has(cache_key)) {
		Ref<Texture2D> cached = entry_icon_cache[cache_key];
		if (cached.is_valid()) {
			return cached;
		}
	}

	Ref<Image> src_img;
	if (ResourceLoader::exists(gpath)) {
		Ref<Texture2D> tex = ResourceLoader::load(gpath);
		if (tex.is_valid()) {
			src_img = tex->get_image();
		}
	}
	if (src_img.is_null() || src_img->is_empty()) {
		if (FileAccess::exists(gpath)) {
			src_img = Image::load_from_file(gpath);
		} else {
			String gl_path = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(gpath) : gpath;
			if (FileAccess::exists(gl_path)) {
				src_img = Image::load_from_file(gl_path);
			}
		}
	}

	if (src_img.is_null() || src_img->is_empty()) {
		if (p_fallback.is_valid()) {
			return p_fallback;
		}
		if (ClockGearsIcons::get_singleton() && cat_hint >= 0) {
			return ClockGearsIcons::get_singleton()->get_db_category_icon(cat_hint, float(p_target_px) / 32.0f);
		}
		return Ref<Texture2D>();
	}

	Ref<Image> src_crop;
	if (has_tc) {
		const int tsz = 32;
		Rect2i r(tc.x * tsz, tc.y * tsz, blk.x * tsz, blk.y * tsz);
		r = r.intersection(Rect2i(0, 0, src_img->get_width(), src_img->get_height()));
		if (r.size.x <= 0 || r.size.y <= 0) {
			return p_fallback;
		}
		src_crop = src_img->get_region(r);
	} else {
		src_crop = src_img->duplicate();
	}

	if (src_crop.is_null() || src_crop->is_empty()) {
		return p_fallback;
	}

	int sw = src_crop->get_width();
	int sh = src_crop->get_height();
	if (sw <= 0 || sh <= 0) {
		return p_fallback;
	}

	int dw = sw;
	int dh = sh;
	if (sw <= p_target_px && sh <= p_target_px) {
		int mult = MAX(1, MIN(p_target_px / sw, p_target_px / sh));
		dw = sw * mult;
		dh = sh * mult;
	} else {
		int div = 2;
		while ((sw / div) > p_target_px || (sh / div) > p_target_px) {
			div++;
		}
		dw = MAX(1, sw / div);
		dh = MAX(1, sh / div);
	}

	if (src_crop->get_format() != Image::FORMAT_RGBA8) {
		src_crop->convert(Image::FORMAT_RGBA8);
	}
	if (dw != sw || dh != sh) {
		src_crop->resize(dw, dh, Image::INTERPOLATE_NEAREST);
	}

	Ref<Image> canvas = Image::create_empty(p_target_px, p_target_px, false, Image::FORMAT_RGBA8);
	canvas->fill(Color(0, 0, 0, 0));
	canvas->blit_rect(src_crop, Rect2i(0, 0, dw, dh), Point2i((p_target_px - dw) / 2, (p_target_px - dh) / 2));

	Ref<ImageTexture> out = ImageTexture::create_from_image(canvas);
	if (out.is_null()) {
		return p_fallback;
	}

	entry_icon_cache[cache_key] = out;
	return out;
}

void ClockGearsDataManager::normalize_arpg_system_config(Dictionary &r_cfg) {
	if (!r_cfg.has("arpg_move_while_attacking")) {
		r_cfg["arpg_move_while_attacking"] = true;
	} else {
		r_cfg["arpg_move_while_attacking"] = bool(r_cfg["arpg_move_while_attacking"]);
	}
	r_cfg["arpg_attack_move_speed_pct"] = CLAMP(int(r_cfg.get("arpg_attack_move_speed_pct", 100)), 0, 200);
	if (!r_cfg.has("arpg_move_while_blocking")) {
		r_cfg["arpg_move_while_blocking"] = false;
	} else {
		r_cfg["arpg_move_while_blocking"] = bool(r_cfg["arpg_move_while_blocking"]);
	}
	r_cfg["arpg_block_move_speed_pct"] = CLAMP(int(r_cfg.get("arpg_block_move_speed_pct", 50)), 0, 200);

	// Tanda 3C-2: Invulnerabilidad, bloqueo (% reducción) y esquivar
	double inv_sec = r_cfg.has("arpg_invuln_seconds") ? double(r_cfg["arpg_invuln_seconds"]) : 0.5;
	r_cfg["arpg_invuln_seconds"] = MAX(0.0, inv_sec);
	r_cfg["arpg_block_reduction_pct"] = CLAMP(int(r_cfg.get("arpg_block_reduction_pct", 50)), 0, 100);

	String dtype = String(r_cfg.get("arpg_dodge_type", "voltereta")).strip_edges().to_lower();
	if (dtype != "dash") {
		dtype = "voltereta";
	}
	r_cfg["arpg_dodge_type"] = dtype;

	String dkey = String(r_cfg.get("arpg_dodge_key", "Space")).strip_edges();
	if (dkey.is_empty()) {
		dkey = "Space";
	}
	r_cfg["arpg_dodge_key"] = dkey;

	double ddur = r_cfg.has("arpg_dodge_duration") ? double(r_cfg["arpg_dodge_duration"]) : 0.25;
	r_cfg["arpg_dodge_duration"] = CLAMP(ddur, 0.05, 5.0);
	double ddist = r_cfg.has("arpg_dodge_distance") ? double(r_cfg["arpg_dodge_distance"]) : 96.0;
	r_cfg["arpg_dodge_distance"] = CLAMP(ddist, 0.0, 2000.0);
	double dcd = r_cfg.has("arpg_dodge_cooldown") ? double(r_cfg["arpg_dodge_cooldown"]) : 0.4;
	r_cfg["arpg_dodge_cooldown"] = CLAMP(dcd, 0.0, 30.0);
}

void ClockGearsDataManager::_normalize_system_and_vocabulary_config() {
	// Fuente única de verdad para Unidad Monetaria: system_config["currency"]
	String cur;
	if (system_config.has("currency")) {
		cur = String(system_config["currency"]).strip_edges();
	}
	if (cur.is_empty() && system_config.has("currency_unit")) {
		cur = String(system_config["currency_unit"]).strip_edges();
	}
	if (cur.is_empty() && system_config.has("currency_name")) {
		cur = String(system_config["currency_name"]).strip_edges();
	}
	if (cur.is_empty() && vocabulary_config.has("gold")) {
		cur = String(vocabulary_config["gold"]).strip_edges();
	}
	if (cur.is_empty()) {
		cur = "G";
	}
	system_config["currency"] = cur;
	system_config.erase("currency_unit");
	system_config.erase("currency_name");
	vocabulary_config.erase("gold");

	if (!system_config.has("game_title")) {
		system_config["game_title"] = "";
	}

	if (system_config.has("custom_map_layers")) {
		Array norm_layers = get_custom_map_layers();
		if (norm_layers.is_empty()) {
			system_config.erase("custom_map_layers");
			active_editor_custom_layer_id = String();
		} else {
			system_config["custom_map_layers"] = norm_layers;
			if (!active_editor_custom_layer_id.is_empty() && get_custom_map_layer(active_editor_custom_layer_id).is_empty()) {
				active_editor_custom_layer_id = String();
			}
		}
	} else {
		active_editor_custom_layer_id = String();
	}
}

void ClockGearsDataManager::set_system_config(const Dictionary &p_cfg) {
	system_config = p_cfg.duplicate(true);
	system_config["mannequin_size"] = normalize_mannequin_size(int(system_config.get("mannequin_size", 32)));
	normalize_arpg_system_config(system_config);
	if (system_config.has("title_menu") && system_config["title_menu"].get_type() == Variant::DICTIONARY) {
		title_menu_config = normalize_title_menu_config(Dictionary(system_config["title_menu"]));
		system_config.erase("title_menu");
	}
	_normalize_system_and_vocabulary_config();
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
	if (!system_config.has("currency") || String(system_config["currency"]).strip_edges().is_empty()) {
		if (vocabulary_config.has("gold") && !String(vocabulary_config["gold"]).strip_edges().is_empty()) {
			system_config["currency"] = String(vocabulary_config["gold"]).strip_edges();
		}
	}
	vocabulary_config.erase("gold");
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
	Ref<DirAccess> dir = DirAccess::create_for_path(p_dir_path);
	if (dir.is_null()) {
		dir = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	}
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

	// 5c. Guardar configuración del Menú Principal (title_menu.json), Menú del Juego (game_menu.json) y Créditos (credits.json)
	title_menu_config = normalize_title_menu_config(title_menu_config);
	title_menu_config["credits_configured"] = has_credits_lines();
	if (!_write_json_atomic(p_dir_path.path_join("title_menu.json"), title_menu_config)) {
		return false;
	}
	game_menu_config = normalize_game_menu_config(game_menu_config);
	if (!_write_json_atomic(p_dir_path.path_join("game_menu.json"), game_menu_config)) {
		return false;
	}
	credits_config = normalize_credits_config(credits_config);
	if (!_write_json_atomic(p_dir_path.path_join("credits.json"), credits_config)) {
		return false;
	}
	game_over_config = normalize_game_over_config(game_over_config);
	if (!_write_json_atomic(p_dir_path.path_join("game_over.json"), game_over_config)) {
		return false;
	}
	hud_config = normalize_hud_config(hud_config);
	if (!_write_json_atomic(p_dir_path.path_join("hud.json"), hud_config)) {
		return false;
	}
	ui_designs_config = normalize_ui_designs_config(ui_designs_config);
	if (!_write_json_atomic(p_dir_path.path_join("ui_designs.json"), ui_designs_config)) {
		return false;
	}

	// 5c-bis. Guardar estilos de ventana (window_styles.json)
	window_styles_config = normalize_window_styles_config(window_styles_config);
	if (!_write_json_atomic(p_dir_path.path_join("window_styles.json"), window_styles_config)) {
		return false;
	}

	// 5d. Guardar estado de partida / equipo ARPG runtime (runtime_state.json)
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
	last_repaired_json_files.clear();
	last_migrated_files.clear();
	built_tileset_cache.clear();
	entry_icon_cache.clear();
	_invalidate_window_styles_cache();
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
	system_config["default_max_item_stack"] = CLAMP(int(system_config.get("default_max_item_stack", 99)), 1, 9999);
	normalize_arpg_system_config(system_config);

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

	// Migración de title_menu.json (o fallback desde system.json / valores por defecto en proyectos antiguos)
	Variant title_var;
	if (_read_json_with_backup(p_dir_path.path_join("title_menu.json"), title_var) && title_var.get_type() == Variant::DICTIONARY) {
		title_menu_config = normalize_title_menu_config(Dictionary(title_var));
	} else if (system_config.has("title_menu") && system_config["title_menu"].get_type() == Variant::DICTIONARY) {
		title_menu_config = normalize_title_menu_config(Dictionary(system_config["title_menu"]));
	} else {
		title_menu_config = get_default_title_menu_config();
	}
	system_config.erase("title_menu");
	_normalize_system_and_vocabulary_config();

	// Migración de game_menu.json (proyectos antiguos heredan valores por defecto)
	Variant gmenu_var;
	if (_read_json_with_backup(p_dir_path.path_join("game_menu.json"), gmenu_var) && gmenu_var.get_type() == Variant::DICTIONARY) {
		game_menu_config = normalize_game_menu_config(Dictionary(gmenu_var));
	} else {
		game_menu_config = get_default_game_menu_config();
	}

	// Migración de credits.json (o fallback desde title_menu.json.credits en proyectos previos)
	Variant cred_var;
	if (_read_json_with_backup(p_dir_path.path_join("credits.json"), cred_var) && cred_var.get_type() == Variant::DICTIONARY) {
		credits_config = normalize_credits_config(Dictionary(cred_var));
	} else if (title_menu_config.has("credits") && title_menu_config["credits"].get_type() == Variant::ARRAY && !Array(title_menu_config["credits"]).is_empty()) {
		Dictionary legacy_cred;
		legacy_cred["items"] = title_menu_config["credits"];
		credits_config = normalize_credits_config(legacy_cred);
	} else {
		credits_config = get_default_credits_config();
	}
	title_menu_config["credits_configured"] = has_credits_lines();

	// Migración de game_over.json (proyectos previos heredan valores por defecto)
	Variant go_var;
	if (_read_json_with_backup(p_dir_path.path_join("game_over.json"), go_var) && go_var.get_type() == Variant::DICTIONARY) {
		game_over_config = normalize_game_over_config(Dictionary(go_var));
	} else {
		game_over_config = get_default_game_over_config();
	}

	// Migración de estilos de ventana (window_styles.json; proyectos antiguos heredan "Redondeado")
	ensure_default_window_types_registered();
	Variant ws_var;
	if (_read_json_with_backup(p_dir_path.path_join("window_styles.json"), ws_var) && ws_var.get_type() == Variant::DICTIONARY) {
		window_styles_config = normalize_window_styles_config(Dictionary(ws_var));
	} else {
		window_styles_config = get_default_window_styles_config();
	}

	// Migración de HUD (hud.json; proyectos antiguos o vacíos inician con 0 widgets para no cambiar nada visualmente)
	Variant hud_var;
	if (_read_json_with_backup(p_dir_path.path_join("hud.json"), hud_var) && hud_var.get_type() == Variant::DICTIONARY) {
		hud_config = normalize_hud_config(Dictionary(hud_var));
	} else if (system_config.has("hud") && system_config["hud"].get_type() == Variant::DICTIONARY) {
		hud_config = normalize_hud_config(Dictionary(system_config["hud"]));
	} else {
		hud_config = get_default_hud_config();
	}
	system_config.erase("hud");

	// Diseños visuales de interfaz y menús ilimitados (ui_designs.json; proyectos antiguos migran conservando .bak y aspecto idéntico)
	String ui_designs_path = p_dir_path.path_join("ui_designs.json");
	Variant uid_var;
	if (_read_json_with_backup(ui_designs_path, uid_var) && uid_var.get_type() == Variant::DICTIONARY) {
		Dictionary raw_uid = Dictionary(uid_var);
		int raw_ui_schema = int(raw_uid.get("schema_version", 1));
		ui_designs_config = normalize_ui_designs_config(raw_uid);
		if (raw_ui_schema < CURRENT_UI_DESIGNS_SCHEMA_VERSION && FileAccess::exists(ui_designs_path)) {
			String orig_bak = ui_designs_path + (raw_ui_schema < 2 ? String(".pre_menu_v2.bak") : String(".pre_hierarchy_v3.bak"));
			if (!FileAccess::exists(orig_bak)) {
				PackedByteArray raw_bytes = FileAccess::get_file_as_bytes(ui_designs_path);
				Ref<FileAccess> fb = FileAccess::open(orig_bak, FileAccess::WRITE);
				if (fb.is_valid()) {
					fb->store_buffer(raw_bytes);
					fb->flush();
				}
			}
			_write_json_atomic(ui_designs_path, ui_designs_config);
			if (!last_migrated_files.has(ui_designs_path)) {
				last_migrated_files.push_back(ui_designs_path);
			}
		}
	} else {
		ui_designs_config = get_default_ui_designs_config();
	}
	ui_design_undo_stacks.clear();
	ui_design_redo_stacks.clear();

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
	bool had = runtime_switches.has(p_name);
	bool prev = had ? bool(runtime_switches[p_name]) : false;
	runtime_switches[p_name] = p_val;
	if (!had || prev != p_val) {
		emit_signal("runtime_state_changed", "switch", p_name);
	}
}

int ClockGearsDataManager::get_variable(const String &p_name) const {
	if (runtime_variables.has(p_name)) {
		return int(runtime_variables[p_name]);
	}
	return 0;
}

void ClockGearsDataManager::set_variable(const String &p_name, int p_val) {
	bool had = runtime_variables.has(p_name);
	int prev = had ? int(runtime_variables[p_name]) : 0;
	runtime_variables[p_name] = p_val;
	if (!had || prev != p_val) {
		emit_signal("runtime_state_changed", "variable", p_name);
	}
}

void ClockGearsDataManager::set_gold(int p_gold) {
	int next = MAX(0, p_gold);
	if (runtime_gold != next) {
		runtime_gold = next;
		emit_signal("runtime_state_changed", "gold", String());
	}
}

void ClockGearsDataManager::change_gold(int p_amount) {
	int next = MAX(0, runtime_gold + p_amount);
	if (runtime_gold != next) {
		runtime_gold = next;
		emit_signal("runtime_state_changed", "gold", String());
	}
}

bool ClockGearsDataManager::has_item(const String &p_name, int p_min_qty) const {
	if (runtime_inventory.has(p_name)) {
		return int(runtime_inventory[p_name]) >= p_min_qty;
	}
	return false;
}

int ClockGearsDataManager::get_item_qty(const String &p_name) const {
	return runtime_inventory.has(p_name) ? MAX(0, int(runtime_inventory[p_name])) : 0;
}

int ClockGearsDataManager::get_default_max_item_stack() const {
	return CLAMP(int(system_config.get("default_max_item_stack", 99)), 1, 9999);
}

void ClockGearsDataManager::set_default_max_item_stack(int p_max_stack) {
	system_config["default_max_item_stack"] = CLAMP(p_max_stack, 1, 9999);
	dirty = true;
}

String ClockGearsDataManager::get_record_inventory_key(int p_category, int p_id) const {
	if (p_category < 0 || p_category >= CAT_MAX || p_id <= 0) {
		return String();
	}
	String cat_key = get_category_key(p_category);
	String sc_key = cat_key + ":" + itos(p_id);
	if (runtime_inventory.has(sc_key)) {
		return sc_key;
	}
	Dictionary rec = get_record_by_id(p_category, p_id);
	String rname = String(rec.get("name", "")).strip_edges();
	if (!rname.is_empty()) {
		return rname;
	}
	return sc_key;
}

Dictionary ClockGearsDataManager::find_inventory_record(const String &p_item_key) const {
	Dictionary out;
	out["found"] = false;
	out["category"] = -1;
	out["category_key"] = "";
	out["id"] = 0;
	out["key"] = p_item_key;
	out["record"] = Dictionary();

	String k = p_item_key.strip_edges();
	if (k.is_empty()) {
		return out;
	}

	static const int k_cats[8] = {
		CAT_ITEMS, CAT_WEAPONS, CAT_ARMORS, CAT_ACCESSORIES,
		CAT_ARPG_WEAPONS, CAT_ARPG_ARMORS, CAT_ARPG_SHIELDS, CAT_ARPG_ACCESSORIES
	};

	int colon = k.find(":");
	if (colon > 0) {
		String cpart = k.substr(0, colon);
		int id_part = k.substr(colon + 1).to_int();
		int cidx = get_category_from_key(cpart);
		if (cidx >= 0 && id_part > 0) {
			Dictionary rec = get_record_by_id(cidx, id_part);
			if (!rec.is_empty()) {
				out["found"] = true;
				out["category"] = cidx;
				out["category_key"] = get_category_key(cidx);
				out["id"] = id_part;
				out["record"] = rec;
				return out;
			}
		}
	}

	String k_low = k.to_lower();
	for (int ci = 0; ci < 8; ci++) {
		int cat = k_cats[ci];
		const Array &arr = category_data[cat];
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary rec = arr[i];
			String rname = String(rec.get("name", "")).strip_edges();
			if (!rname.is_empty() && (rname == k || rname.to_lower() == k_low)) {
				out["found"] = true;
				out["category"] = cat;
				out["category_key"] = get_category_key(cat);
				out["id"] = int(rec.get("id", i + 1));
				out["record"] = rec.duplicate(true);
				return out;
			}
		}
	}
	return out;
}

int ClockGearsDataManager::get_item_max_stack(const String &p_item_key) const {
	Dictionary info = find_inventory_record(p_item_key);
	if (bool(info.get("found", false))) {
		Dictionary rec = info.get("record", Dictionary());
		if (rec.has("max_stack")) {
			int ms = int(rec["max_stack"]);
			if (ms > 0) {
				return CLAMP(ms, 1, 9999);
			}
		}
	}
	return get_default_max_item_stack();
}

void ClockGearsDataManager::change_item(const String &p_name, int p_amount) {
	int cur = runtime_inventory.has(p_name) ? int(runtime_inventory[p_name]) : 0;
	int next = MAX(0, cur + p_amount);
	if (p_amount > 0) {
		int max_stack = get_item_max_stack(p_name);
		next = MIN(MAX(cur, max_stack), next);
	}
	if (next == 0) {
		runtime_inventory.erase(p_name);
	} else {
		runtime_inventory[p_name] = next;
	}
	if (next != cur) {
		emit_signal("runtime_state_changed", "item", p_name);
	}
}

bool ClockGearsDataManager::is_key_item(const String &p_item_key) const {
	Dictionary info = find_inventory_record(p_item_key);
	if (!bool(info.get("found", false))) {
		return false;
	}
	if (int(info.get("category", -1)) != CAT_ITEMS) {
		return false;
	}
	Dictionary rec = info.get("record", Dictionary());
	return int(rec.get("type", 0)) == 1;
}

bool ClockGearsDataManager::can_discard_item(const String &p_item_key) const {
	if (get_item_qty(p_item_key) <= 0) {
		return false;
	}
	return !is_key_item(p_item_key);
}

bool ClockGearsDataManager::discard_item(const String &p_item_key, int p_amount) {
	if (p_amount <= 0 || !can_discard_item(p_item_key)) {
		return false;
	}
	int cur = get_item_qty(p_item_key);
	if (cur <= 0) {
		return false;
	}
	change_item(p_item_key, -MIN(cur, p_amount));
	return true;
}

Array ClockGearsDataManager::get_party_actor_ids() const {
	Array party = system_config.get("party", Array());
	Array out;
	for (int i = 0; i < party.size(); i++) {
		int aid = int(party[i]);
		if (aid > 0 && !out.has(aid)) {
			out.push_back(aid);
		}
	}
	if (out.is_empty() && !category_data[CAT_ACTORS].is_empty() && category_data[CAT_ACTORS][0].get_type() == Variant::DICTIONARY) {
		int first_id = int(Dictionary(category_data[CAT_ACTORS][0]).get("id", 1));
		if (first_id > 0) {
			out.push_back(first_id);
		}
	}
	return out;
}

Array ClockGearsDataManager::get_inventory_entries() const {
	Array out;
	Array keys = runtime_inventory.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = String(keys[i]);
		int qty = MAX(0, int(runtime_inventory[keys[i]]));
		if (qty <= 0) {
			continue;
		}
		Dictionary info = find_inventory_record(key);
		bool found = bool(info.get("found", false));
		int cat = int(info.get("category", -1));
		int rec_id = int(info.get("id", 0));
		Dictionary rec = info.get("record", Dictionary());

		String name = found ? String(rec.get("name", key)).strip_edges() : key;
		if (name.is_empty()) {
			name = key;
		}
		int item_type = (cat == CAT_ITEMS) ? int(rec.get("type", 0)) : -1;
		int scope = (cat == CAT_ITEMS) ? int(rec.get("secondary_type", rec.get("scope", 7))) : 0;
		int occasion = (cat == CAT_ITEMS) ? int(rec.get("tertiary_type", rec.get("occasion", 0))) : 3;
		bool key_item = (cat == CAT_ITEMS && item_type == 1);
		bool usable_menu = (cat == CAT_ITEMS && (occasion == 0 || occasion == 2));

		Dictionary row;
		row["key"] = key;
		row["category"] = cat;
		row["category_key"] = found ? String(info.get("category_key", "")) : "";
		row["id"] = rec_id;
		row["name"] = name;
		row["desc"] = found ? String(rec.get("desc", "")) : "";
		row["icon_path"] = found ? String(rec.get("icon_path", rec.get("graphic_path", ""))) : "";
		row["qty"] = qty;
		row["max_stack"] = get_item_max_stack(key);
		row["is_key_item"] = key_item;
		row["can_discard"] = !key_item;
		row["consumable"] = found ? bool(rec.get("consumable", true)) : true;
		row["item_type"] = item_type;
		row["scope"] = scope;
		row["occasion"] = occasion;
		row["usable_in_menu"] = usable_menu;
		row["record"] = rec;
		out.push_back(row);
	}
	return out;
}

bool ClockGearsDataManager::use_item_on_actor(const String &p_item_key, int p_target_actor_id) {
	if (get_item_qty(p_item_key) <= 0) {
		return false;
	}
	Dictionary info = find_inventory_record(p_item_key);
	if (!bool(info.get("found", false)) || int(info.get("category", -1)) != CAT_ITEMS) {
		return false;
	}
	Dictionary rec = info.get("record", Dictionary());
	int occasion = int(rec.get("tertiary_type", rec.get("occasion", 0)));
	// 0 = Siempre, 1 = Solo en Batalla, 2 = Solo en el Menú, 3 = Nunca
	if (occasion != 0 && occasion != 2) {
		return false;
	}
	int scope = int(rec.get("secondary_type", rec.get("scope", 7)));
	// Ámbitos enemigos (1..6, 12) o Ninguno (0) no se usan sobre actores en el menú
	if (scope >= 1 && scope <= 6) {
		return false;
	}

	Array targets;
	if (scope == 8 || scope == 10) {
		targets = get_party_actor_ids();
	}
	if (targets.is_empty() && p_target_actor_id > 0) {
		targets.push_back(p_target_actor_id);
	}
	if (targets.is_empty()) {
		return false;
	}

	Array effects = (rec.has("effects") && rec["effects"].get_type() == Variant::ARRAY) ? Array(rec["effects"]) : Array();
	bool any_applied = false;
	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();

	for (int ti = 0; ti < targets.size(); ti++) {
		int aid = int(targets[ti]);
		if (aid <= 0 || get_record_by_id(CAT_ACTORS, aid).is_empty()) {
			continue;
		}
		Dictionary st = get_actor_runtime_state(aid);
		int cur_hp = int(st.get("hp", 0));
		int max_hp = MAX(1, int(st.get("max_hp", 100)));
		int cur_mp = int(st.get("mp", 0));
		int max_mp = MAX(0, int(st.get("max_mp", 0)));

		// Scopes KO (9, 10) solo aplican a K.O.; aliados vivos (7, 8, 11) requieren hp > 0 salvo al remover estado
		bool is_ko_scope = (scope == 9 || scope == 10);
		if (is_ko_scope && cur_hp > 0) {
			continue;
		}

		for (int ei = 0; ei < effects.size(); ei++) {
			if (effects[ei].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary ef = effects[ei];
			int code = int(ef.get("code", 0));
			int data_id = int(ef.get("data_id", 0));
			double val1 = double(ef.get("value1", 0.0));
			double val2 = double(ef.get("value2", 0.0));

			if (code == CG_EFFECT_RECOVER_HP) {
				if (!is_ko_scope && cur_hp <= 0) {
					continue;
				}
				int delta = int(Math::round(double(max_hp) * (val1 / 100.0))) + int(Math::round(val2));
				if (delta != 0) {
					int next_hp = CLAMP(cur_hp + delta, is_ko_scope ? 1 : 0, max_hp);
					if (next_hp != cur_hp) {
						set_actor_runtime_hp(aid, next_hp);
						cur_hp = next_hp;
						any_applied = true;
					}
				}
			} else if (code == CG_EFFECT_RECOVER_MP) {
				if (!is_ko_scope && cur_hp <= 0) {
					continue;
				}
				int delta = int(Math::round(double(max_mp) * (val1 / 100.0))) + int(Math::round(val2));
				if (delta != 0 && max_mp > 0) {
					int next_mp = CLAMP(cur_mp + delta, 0, max_mp);
					if (next_mp != cur_mp) {
						set_actor_runtime_mp(aid, next_mp);
						cur_mp = next_mp;
						any_applied = true;
					}
				}
			} else if (code == CG_EFFECT_ADD_STATE && data_id > 0) {
				double rate = (val1 <= 0.0) ? 1.0 : ((val1 <= 1.0) ? val1 : (val1 / 100.0));
				bool hit = !fm || (fm->rand_float() <= rate);
				if (hit) {
					add_actor_runtime_state(aid, data_id);
					any_applied = true;
				}
			} else if (code == CG_EFFECT_REMOVE_STATE && data_id > 0) {
				Array cur_states = get_actor_runtime_states(aid);
				if (cur_states.has(data_id)) {
					double rate = (val1 <= 0.0) ? 1.0 : ((val1 <= 1.0) ? val1 : (val1 / 100.0));
					bool hit = !fm || (fm->rand_float() <= rate);
					if (hit) {
						remove_actor_runtime_state(aid, data_id);
						any_applied = true;
					}
				}
			} else if (code == CG_EFFECT_GROW_PARAM) {
				int delta = int(Math::round(val1 + val2));
				if (delta != 0 && data_id >= 0 && data_id < 8) {
					static const char *bonus_keys[8] = {
						"max_hp_bonus", "max_mp_bonus", "atk_bonus", "def_bonus",
						"mat_bonus", "mdf_bonus", "agi_bonus", "luk_bonus"
					};
					Dictionary st_edit = get_actor_runtime_state(aid);
					st_edit[bonus_keys[data_id]] = int(st_edit.get(bonus_keys[data_id], 0)) + delta;
					st_edit.erase("max_hp");
					st_edit.erase("max_mp");
					runtime_actor_states[String::num_int64(aid)] = st_edit;
					any_applied = true;
				}
			} else if (code == CG_EFFECT_LEARN_SKILL && data_id > 0) {
				Dictionary st_edit = get_actor_runtime_state(aid);
				Array skills = st_edit.has("learned_skills") && st_edit["learned_skills"].get_type() == Variant::ARRAY
						? Array(st_edit["learned_skills"]).duplicate(true)
						: Array();
				if (!skills.has(data_id)) {
					skills.push_back(data_id);
					st_edit["learned_skills"] = skills;
					st_edit.erase("max_hp");
					st_edit.erase("max_mp");
					runtime_actor_states[String::num_int64(aid)] = st_edit;
					any_applied = true;
				}
			}
		}
	}

	if (any_applied && bool(rec.get("consumable", true))) {
		change_item(p_item_key, -1);
	}
	return any_applied;
}

void ClockGearsDataManager::reset_runtime_state() {
	runtime_switches.clear();
	runtime_variables.clear();
	runtime_inventory.clear();
	runtime_gold = 0;
	runtime_equips.clear();
	runtime_arpg_equips.clear();
	runtime_actor_states.clear();
	runtime_defeated_enemies.clear();
	runtime_window_style_overrides.clear();
	runtime_playtime_sec = 0.0;
	runtime_game_menu_disabled = false;
	runtime_hud_visible = true;
	runtime_hud_widget_visibility.clear();
	pending_post_transfer_commands.clear();
	set_runtime_fade_alpha(0.0f);
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	PackedStringArray sys_slots = cat ? cat->get_equip_slots() : PackedStringArray();
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

			Array norm_slots;
			Array raw_eq = (act.has("equips") && act["equips"].get_type() == Variant::ARRAY) ? Array(act["equips"]) : Array();
			for (int s = 0; s < sys_slots.size(); s++) {
				Dictionary sd;
				sd["slot_idx"] = s;
				sd["name"] = sys_slots[s];
				int pid = (s < raw_eq.size()) ? int(raw_eq[s]) : 0;
				String dcat = (s == 0) ? String("weapons") : ((s == 4) ? String("accessories") : String("armors"));
				if (s < raw_eq.size() && raw_eq[s].get_type() == Variant::DICTIONARY) {
					Dictionary rd = raw_eq[s];
					pid = int(rd.get("piece_id", 0));
					dcat = String(rd.get("category", dcat));
				}
				sd["category"] = dcat;
				sd["piece_id"] = MAX(0, pid);
				norm_slots.push_back(sd);
			}
			runtime_equips[String::num_int64(aid)] = norm_slots;
		}
	}
	emit_signal("runtime_state_changed", "all", String());
}

float ClockGearsDataManager::evaluate_traits_element_rate(const Array &p_traits, int p_element_id, const String &p_element_name) {
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	int resolved_id = p_element_id;
	if (resolved_id <= 0 && !p_element_name.strip_edges().is_empty() && cat) {
		int from_cat = cat->resolve_element_id(p_element_name);
		if (from_cat > 0) {
			resolved_id = from_cat;
		}
	}
	String target_key = (cat && resolved_id > 0) ? cat->get_element_key(resolved_id).to_lower() : p_element_name.strip_edges().to_lower();
	String target_name = (cat && resolved_id > 0) ? cat->get_element_name(resolved_id).to_lower() : p_element_name.strip_edges().to_lower();
	if (resolved_id == 0 && target_key.is_empty()) {
		target_key = "fisico";
		target_name = "fisico";
	}

	float rate = 1.0f;
	bool found = false;
	for (int i = 0; i < p_traits.size(); i++) {
		if (p_traits[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary tr = p_traits[i];
		if (int(tr.get("code", 0)) != CG_TRAIT_ELEMENT_RATE) {
			continue;
		}
		int d_id = int(tr.get("data_id", -1));
		String tr_key = String(tr.get("element_key", tr.get("element", ""))).strip_edges().to_lower();
		bool match = false;
		if (!tr_key.is_empty() && (!target_key.is_empty() && (tr_key == target_key || tr_key == target_name))) {
			match = true;
		} else if (d_id == resolved_id) {
			match = true;
		} else if (tr_key.is_empty() && cat && resolved_id > 0 && d_id >= 0 && (d_id + 1) == resolved_id && !bool(tr.get("one_based", false))) {
			bool has_exact_one_based = false;
			for (int j = 0; j < p_traits.size(); j++) {
				if (p_traits[j].get_type() == Variant::DICTIONARY) {
					Dictionary tj = p_traits[j];
					if (int(tj.get("code", 0)) == CG_TRAIT_ELEMENT_RATE && int(tj.get("data_id", -1)) == resolved_id) {
						has_exact_one_based = true;
						break;
					}
				}
			}
			if (!has_exact_one_based) {
				match = true;
			}
		}
		if (match) {
			float pct = float(tr.get("value", 100.0));
			rate = MAX(0.0f, pct / 100.0f);
			found = true;
		}
	}
	return found ? rate : 1.0f;
}

int ClockGearsDataManager::resolve_traits_attack_element_id(const Array &p_traits, String &r_element_key) {
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	for (int i = 0; i < p_traits.size(); i++) {
		if (p_traits[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary tr = p_traits[i];
		if (int(tr.get("code", 0)) == CG_TRAIT_ATTACK_ELEMENT) {
			String ek = String(tr.get("element_key", tr.get("element", ""))).strip_edges().to_lower();
			int did = int(tr.get("data_id", 0));
			if (!ek.is_empty()) {
				r_element_key = ek;
				if (cat) {
					int rid = cat->resolve_element_id(ek);
					if (rid > 0) {
						return rid;
					}
				}
			}
			if (did > 0) {
				if (r_element_key.is_empty() && cat) {
					r_element_key = cat->get_element_key(did).to_lower();
				}
				return did;
			}
		}
	}
	r_element_key = "fisico";
	return 0;
}

int ClockGearsDataManager::get_actor_arpg_attack_element_id(int p_actor_id, String &r_element_key) const {
	r_element_key = "fisico";
	Array slots = get_actor_arpg_equip_slots(p_actor_id);
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	for (int i = 0; i < slots.size(); i++) {
		if (slots[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary sd = slots[i];
		int pid = int(sd.get("piece_id", 0));
		if (pid <= 0) {
			continue;
		}
		String slot_id = String(sd.get("slot", "")).strip_edges().to_lower();
		int cat_idx = get_category_from_key(String(sd.get("category", "")));
		if (slot_id != "arma" && cat_idx != CAT_ARPG_WEAPONS && cat_idx != CAT_WEAPONS) {
			continue;
		}
		Dictionary piece = get_record_by_id(cat_idx, pid);
		if (piece.is_empty()) {
			continue;
		}
		// 1) Campos directos en el arma ARPG si existen (element_id / element_key / element)
		String direct_key = String(piece.get("element_key", piece.get("element", ""))).strip_edges().to_lower();
		int direct_id = int(piece.get("element_id", 0));
		if (direct_id > 0 || (!direct_key.is_empty() && direct_key != "fisico" && direct_key != "physical")) {
			if (!direct_key.is_empty()) {
				r_element_key = direct_key;
				if (direct_id <= 0 && cat) {
					direct_id = cat->resolve_element_id(direct_key);
				}
			} else if (cat && direct_id > 0) {
				r_element_key = cat->get_element_key(direct_id).to_lower();
			}
			return MAX(0, direct_id);
		}
		// 2) Rasgo CG_TRAIT_ATTACK_ELEMENT en el arma ARPG
		Variant tv = piece.get("traits", Array());
		if (tv.get_type() == Variant::ARRAY) {
			String tr_key;
			int tr_id = resolve_traits_attack_element_id(Array(tv), tr_key);
			if (tr_id > 0 || (!tr_key.is_empty() && tr_key != "fisico")) {
				r_element_key = tr_key;
				return tr_id;
			}
		}
	}
	return 0;
}

float ClockGearsDataManager::get_actor_element_rate(int p_actor_id, int p_element_id, const String &p_element_key) const {
	float rate = 1.0f;
	if (p_actor_id <= 0) {
		return rate;
	}
	Dictionary act = get_record_by_id(CAT_ACTORS, p_actor_id);
	Variant atv = act.get("traits", Array());
	if (atv.get_type() == Variant::ARRAY) {
		rate *= evaluate_traits_element_rate(Array(atv), p_element_id, p_element_key);
	}
	int class_id = int(act.get("class_id", 0));
	if (class_id > 0) {
		Dictionary cls = get_record_by_id(CAT_CLASSES, class_id);
		Variant ctv = cls.get("traits", Array());
		if (ctv.get_type() == Variant::ARRAY) {
			rate *= evaluate_traits_element_rate(Array(ctv), p_element_id, p_element_key);
		}
	}
	Array slots = get_actor_equip_slots(p_actor_id);
	for (int i = 0; i < slots.size(); i++) {
		if (slots[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary sd = slots[i];
		int pid = int(sd.get("piece_id", 0));
		if (pid <= 0) {
			continue;
		}
		int cat_idx = get_category_from_key(String(sd.get("category", "")));
		if (cat_idx < 0) {
			continue;
		}
		Dictionary piece = get_record_by_id(cat_idx, pid);
		Variant ptv = piece.get("traits", Array());
		if (ptv.get_type() == Variant::ARRAY) {
			rate *= evaluate_traits_element_rate(Array(ptv), p_element_id, p_element_key);
		}
	}
	String rt_key = String::num_int64(p_actor_id);
	if (runtime_actor_states.has(rt_key) && runtime_actor_states[rt_key].get_type() == Variant::DICTIONARY) {
		Dictionary rt = runtime_actor_states[rt_key];
		Array st_ids = rt.has("states") && rt["states"].get_type() == Variant::ARRAY ? Array(rt["states"]) : Array();
		for (int i = 0; i < st_ids.size(); i++) {
			Dictionary srec = get_record_by_id(CAT_STATES, int(st_ids[i]));
			Variant stv = srec.get("traits", Array());
			if (stv.get_type() == Variant::ARRAY) {
				rate *= evaluate_traits_element_rate(Array(stv), p_element_id, p_element_key);
			}
		}
	}
	return MAX(0.0f, rate);
}

int ClockGearsDataManager::compute_arpg_hit_damage(const Dictionary &p_attacker, const Dictionary &p_defender, float p_elem_mult, double p_hitbox_mult_pct, double p_hurtbox_mult_pct, bool &r_is_crit) {
	r_is_crit = false;
	int64_t base_dmg = 1;
	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
	if (fm) {
		double luk = double(int(p_attacker.get("luk", 0)));
		double crit_chance = fm->calculate_critical_chance(luk * 0.005, 0.0);
		r_is_crit = (crit_chance > 0.0) ? (fm->rand_float() <= crit_chance) : false;
		Dictionary ctx;
		ctx["is_critical"] = r_is_crit;
		ctx["critical_multiplier"] = 2.0;
		ctx["variance"] = 20.0;
		base_dmg = fm->calculate_damage("max(a.atk * 4 - b.def * 2, 0)", p_attacker, p_defender, ctx);
	} else {
		base_dmg = MAX(int64_t(0), int64_t(p_attacker.get("atk", 0)) * 4 - int64_t(p_defender.get("def", 0)) * 2);
	}
	if (base_dmg <= 0) {
		base_dmg = 1;
	}
	double hb_m = MAX(0.0, p_hitbox_mult_pct);
	double hu_m = MAX(0.0, p_hurtbox_mult_pct);
	if (p_elem_mult <= 0.0f || hb_m <= 0.0 || hu_m <= 0.0) {
		return 0;
	}
	double total = double(base_dmg) * double(p_elem_mult) * (hb_m / 100.0) * (hu_m / 100.0);
	return MAX(1, int(Math::round(total)));
}

Dictionary ClockGearsDataManager::get_actor_runtime_state(int p_actor_id) {
	if (p_actor_id <= 0) {
		return Dictionary();
	}
	String key = String::num_int64(p_actor_id);
	Dictionary act = get_record_by_id(CAT_ACTORS, p_actor_id);
	Dictionary bonuses = get_actor_equip_bonuses(p_actor_id);

	if (!runtime_actor_states.has(key) || runtime_actor_states[key].get_type() != Variant::DICTIONARY) {
		int lv = MAX(1, int(act.get("level", act.get("initial_level", 1))));
		int exp_base = 30, exp_extra = 20, exp_a = 30, exp_b = 30;
		int class_id = int(act.get("class_id", 1));
		Dictionary cls = get_record_by_id(CAT_CLASSES, class_id);
		if (!cls.is_empty()) {
			exp_base = int(cls.get("exp_base", 30));
			exp_extra = int(cls.get("exp_extra", 20));
			exp_a = int(cls.get("exp_acc_a", 30));
			exp_b = int(cls.get("exp_acc_b", 30));
		}
		ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
		int64_t init_exp = int64_t(act.get("exp", 0));
		if (init_exp <= 0 && lv > 1 && fm) {
			init_exp = fm->calculate_exp_for_level(lv, exp_base, exp_extra, exp_a, exp_b);
		}
		int base_mhp = MAX(1, int(act.get("hp", 100)) + int(bonuses.get("hp", 0)));
		int base_mmp = MAX(0, int(act.get("mp", 20)) + int(bonuses.get("mp", 0)));
		Dictionary st;
		st["level"] = lv;
		st["exp"] = init_exp;
		st["hp"] = base_mhp;
		st["mp"] = base_mmp;
		st["max_hp_bonus"] = 0;
		st["max_mp_bonus"] = 0;
		st["atk_bonus"] = 0;
		st["def_bonus"] = 0;
		st["mat_bonus"] = 0;
		st["mdf_bonus"] = 0;
		st["agi_bonus"] = 0;
		st["luk_bonus"] = 0;
		st["states"] = Array();
		st["learned_skills"] = Array();
		runtime_actor_states[key] = st;
	}
	Dictionary st = Dictionary(runtime_actor_states[key]).duplicate(true);
	int eff_mhp = MAX(1, int(act.get("hp", 100)) + int(bonuses.get("hp", 0)) + int(st.get("max_hp_bonus", 0)));
	int eff_mmp = MAX(0, int(act.get("mp", 20)) + int(bonuses.get("mp", 0)) + int(st.get("max_mp_bonus", 0)));
	st["max_hp"] = eff_mhp;
	st["hp"] = CLAMP(int(st.get("hp", eff_mhp)), 0, eff_mhp);
	st["max_mp"] = eff_mmp;
	st["mp"] = CLAMP(int(st.get("mp", eff_mmp)), 0, eff_mmp);
	if (!st.has("states") || st["states"].get_type() != Variant::ARRAY) {
		st["states"] = Array();
	}
	return st;
}

void ClockGearsDataManager::set_actor_runtime_hp(int p_actor_id, int p_hp) {
	if (p_actor_id <= 0) {
		return;
	}
	Dictionary st = get_actor_runtime_state(p_actor_id);
	int prev_hp = int(st.get("hp", 0));
	int eff_mhp = MAX(1, int(st.get("max_hp", 100)));
	int next_hp = CLAMP(p_hp, 0, eff_mhp);
	st["hp"] = next_hp;
	st.erase("max_hp");
	st.erase("max_mp");
	runtime_actor_states[String::num_int64(p_actor_id)] = st;
	if (next_hp != prev_hp) {
		emit_signal("runtime_state_changed", "actor_hp", String::num_int64(p_actor_id));
	}
}

int ClockGearsDataManager::get_actor_runtime_hp(int p_actor_id) {
	return int(get_actor_runtime_state(p_actor_id).get("hp", 0));
}

int ClockGearsDataManager::get_actor_runtime_max_hp(int p_actor_id) {
	return MAX(1, int(get_actor_runtime_state(p_actor_id).get("max_hp", 100)));
}

void ClockGearsDataManager::set_actor_runtime_mp(int p_actor_id, int p_mp) {
	if (p_actor_id <= 0) {
		return;
	}
	Dictionary st = get_actor_runtime_state(p_actor_id);
	int prev_mp = int(st.get("mp", 0));
	int eff_mmp = MAX(0, int(st.get("max_mp", 20)));
	int next_mp = CLAMP(p_mp, 0, eff_mmp);
	st["mp"] = next_mp;
	st.erase("max_hp");
	st.erase("max_mp");
	runtime_actor_states[String::num_int64(p_actor_id)] = st;
	if (next_mp != prev_mp) {
		emit_signal("runtime_state_changed", "actor_mp", String::num_int64(p_actor_id));
	}
}

int ClockGearsDataManager::get_actor_runtime_mp(int p_actor_id) {
	return MAX(0, int(get_actor_runtime_state(p_actor_id).get("mp", 0)));
}

int ClockGearsDataManager::get_actor_runtime_max_mp(int p_actor_id) {
	return MAX(0, int(get_actor_runtime_state(p_actor_id).get("max_mp", 0)));
}

void ClockGearsDataManager::add_actor_runtime_state(int p_actor_id, int p_state_id) {
	if (p_actor_id <= 0 || p_state_id <= 0) {
		return;
	}
	Dictionary st = get_actor_runtime_state(p_actor_id);
	Array arr = Array(st.get("states", Array())).duplicate(true);
	if (!arr.has(p_state_id)) {
		arr.push_back(p_state_id);
	}
	st["states"] = arr;
	st.erase("max_hp");
	st.erase("max_mp");
	runtime_actor_states[String::num_int64(p_actor_id)] = st;
}

void ClockGearsDataManager::remove_actor_runtime_state(int p_actor_id, int p_state_id) {
	if (p_actor_id <= 0 || p_state_id <= 0) {
		return;
	}
	Dictionary st = get_actor_runtime_state(p_actor_id);
	Array arr = Array(st.get("states", Array())).duplicate(true);
	int idx = arr.find(p_state_id);
	if (idx >= 0) {
		arr.remove_at(idx);
	}
	st["states"] = arr;
	st.erase("max_hp");
	st.erase("max_mp");
	runtime_actor_states[String::num_int64(p_actor_id)] = st;
}

Array ClockGearsDataManager::get_actor_runtime_states(int p_actor_id) {
	return Array(get_actor_runtime_state(p_actor_id).get("states", Array())).duplicate(true);
}

int ClockGearsDataManager::get_actor_runtime_level(int p_actor_id) {
	return MAX(1, int(get_actor_runtime_state(p_actor_id).get("level", 1)));
}

int64_t ClockGearsDataManager::get_actor_runtime_exp(int p_actor_id) {
	return int64_t(get_actor_runtime_state(p_actor_id).get("exp", 0));
}

Array ClockGearsDataManager::grant_party_exp(int64_t p_exp_amount) {
	Array msgs;
	if (p_exp_amount <= 0) {
		return msgs;
	}
	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();
	Array party = system_config.get("party", Array());
	if (party.is_empty() && !category_data[CAT_ACTORS].is_empty() && category_data[CAT_ACTORS][0].get_type() == Variant::DICTIONARY) {
		party.push_back(int(Dictionary(category_data[CAT_ACTORS][0]).get("id", 1)));
	}

	for (int i = 0; i < party.size(); i++) {
		int aid = int(party[i]);
		if (aid <= 0) {
			continue;
		}
		Dictionary act = get_record_by_id(CAT_ACTORS, aid);
		if (act.is_empty()) {
			continue;
		}
		Dictionary st = get_actor_runtime_state(aid);
		String key = String::num_int64(aid);

		int class_id = int(act.get("class_id", 1));
		Dictionary cls = get_record_by_id(CAT_CLASSES, class_id);
		int exp_base = cls.is_empty() ? 30 : int(cls.get("exp_base", 30));
		int exp_extra = cls.is_empty() ? 20 : int(cls.get("exp_extra", 20));
		int exp_a = cls.is_empty() ? 30 : int(cls.get("exp_acc_a", 30));
		int exp_b = cls.is_empty() ? 30 : int(cls.get("exp_acc_b", 30));

		int cur_lv = MAX(1, int(st.get("level", 1)));
		int64_t cur_exp = int64_t(st.get("exp", 0)) + p_exp_amount;
		int mhp_b = int(st.get("max_hp_bonus", 0));
		int atk_b = int(st.get("atk_bonus", 0));
		int def_b = int(st.get("def_bonus", 0));
		int cur_hp = int(st.get("hp", 100));

		if (fm) {
			while (cur_lv < 999) {
				int64_t req = fm->calculate_exp_for_level(cur_lv + 1, exp_base, exp_extra, exp_a, exp_b);
				if (cur_exp >= req) {
					cur_lv++;
					mhp_b += 50;
					cur_hp += 50;
					atk_b += 3;
					def_b += 2;
					String aname = String(act.get("name", vformat("Actor_%d", aid)));
					msgs.push_back(vformat(String::utf8("¡%s ha subido al Nivel %d!"), aname, cur_lv));
					emit_signal("actor_leveled_up", aid, cur_lv);
				} else {
					break;
				}
			}
		}

		st["level"] = cur_lv;
		st["exp"] = cur_exp;
		st["max_hp_bonus"] = mhp_b;
		st["atk_bonus"] = atk_b;
		st["def_bonus"] = def_b;
		st["hp"] = cur_hp;
		st.erase("max_hp");
		runtime_actor_states[key] = st;
	}
	if (p_exp_amount != 0) {
		emit_signal("runtime_state_changed", "actor_exp", String());
	}
	return msgs;
}

Array ClockGearsDataManager::resolve_arpg_enemy_drops(const Dictionary &p_enemy_rec) {
	Array obtained;
	Variant dv = p_enemy_rec.get("drops", Array());
	if (dv.get_type() != Variant::ARRAY) {
		return obtained;
	}
	Array drops = dv;
	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();

	for (int s = 0; s < drops.size() && s < 3; s++) {
		if (drops[s].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary dr = drops[s];
		int prob = int(dr.get("prob", 0));
		int item_id = int(dr.get("item_id", 0));
		if (item_id <= 0 || prob <= 0) {
			continue;
		}
		bool won = fm ? fm->roll_drop(prob) : (prob >= 100);
		if (!won) {
			continue;
		}
		int cat = CAT_ITEMS;
		Variant kv = dr.get("kind", CAT_ITEMS);
		if (kv.get_type() == Variant::STRING) {
			int parsed = get_category_from_key(String(kv).strip_edges().to_lower());
			if (parsed >= 0) {
				cat = parsed;
			}
		} else {
			int ik = int(kv);
			if (ik == CAT_ITEMS || ik == CAT_WEAPONS || ik == CAT_ARMORS || ik == CAT_ACCESSORIES ||
					ik == CAT_ARPG_WEAPONS || ik == CAT_ARPG_ARMORS || ik == CAT_ARPG_SHIELDS || ik == CAT_ARPG_ACCESSORIES) {
				cat = ik;
			} else if (ik == 1) {
				cat = CAT_WEAPONS;
			} else if (ik == 2) {
				cat = CAT_ARMORS;
			}
		}
		Dictionary item_rec = get_record_by_id(cat, item_id);
		if (item_rec.is_empty() && cat != CAT_ITEMS) {
			item_rec = get_record_by_id(CAT_ITEMS, item_id);
		}
		String iname = String(item_rec.get("name", vformat("Item_%d", item_id)));
		int qty = MAX(1, int(dr.get("qty", dr.get("amount", 1))));
		change_item(iname, qty);
		Dictionary entry;
		entry["name"] = iname;
		entry["amount"] = qty;
		entry["category"] = cat;
		entry["item_id"] = item_id;
		obtained.push_back(entry);
		emit_signal("drop_obtained", iname, qty);
	}
	return obtained;
}

void ClockGearsDataManager::set_runtime_fade_alpha(float p_alpha, Node *p_context) {
	runtime_fade_alpha = CLAMP(p_alpha, 0.0f, 1.0f);
	ColorRect *rect = Object::cast_to<ColorRect>(ObjectDB::get_instance(persistent_fade_rect_id));
	if (!rect && p_context && p_context->is_inside_tree()) {
		rect = ensure_persistent_fade_rect(p_context);
	}
	if (rect) {
		rect->set_color(Color(0.0f, 0.0f, 0.0f, runtime_fade_alpha));
		rect->set_visible(runtime_fade_alpha > 0.001f);
		if (Viewport *vp = rect->get_viewport()) {
			Size2 vsz = vp->get_visible_rect().size;
			if (vsz.x > 0 && vsz.y > 0) {
				rect->set_position(Vector2(0, 0));
				rect->set_size(vsz);
			}
		}
	}
}

CanvasLayer *ClockGearsDataManager::get_persistent_fade_layer() const {
	return Object::cast_to<CanvasLayer>(ObjectDB::get_instance(persistent_fade_layer_id));
}

ColorRect *ClockGearsDataManager::ensure_persistent_fade_rect(Node *p_context, CanvasLayer **r_layer) {
	CanvasLayer *layer = Object::cast_to<CanvasLayer>(ObjectDB::get_instance(persistent_fade_layer_id));
	ColorRect *rect = Object::cast_to<ColorRect>(ObjectDB::get_instance(persistent_fade_rect_id));

	Window *root_win = nullptr;
	if (p_context && p_context->is_inside_tree() && p_context->get_tree()) {
		root_win = p_context->get_tree()->get_root();
	}

	if (layer && (!layer->is_inside_tree() || (root_win && layer->get_parent() != root_win))) {
		layer->queue_free();
		layer = nullptr;
		rect = nullptr;
		persistent_fade_layer_id = ObjectID();
		persistent_fade_rect_id = ObjectID();
	}

	if (!layer && root_win) {
		layer = memnew(CanvasLayer);
		layer->set_name("ClockGearsGlobalFadeLayer");
		layer->set_layer(120); // Por encima del mundo (0), HUD (50) y UI (100).
		root_win->add_child(layer);
		persistent_fade_layer_id = layer->get_instance_id();

		rect = memnew(ColorRect);
		rect->set_name("GlobalFadeRect");
		rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		rect->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		rect->set_color(Color(0.0f, 0.0f, 0.0f, runtime_fade_alpha));
		rect->set_visible(runtime_fade_alpha > 0.001f);
		layer->add_child(rect);
		persistent_fade_rect_id = rect->get_instance_id();
	}

	if (rect && root_win) {
		Size2 vsz = root_win->get_visible_rect().size;
		if (vsz.x > 0 && vsz.y > 0) {
			rect->set_position(Vector2(0, 0));
			rect->set_size(vsz);
		}
		rect->set_color(Color(0.0f, 0.0f, 0.0f, runtime_fade_alpha));
		rect->set_visible(runtime_fade_alpha > 0.001f);
	}
	if (r_layer) {
		*r_layer = layer;
	}
	return rect;
}

void ClockGearsDataManager::set_pending_post_transfer_commands(const Array &p_cmds) {
	pending_post_transfer_commands = p_cmds.duplicate(true);
}

Array ClockGearsDataManager::take_pending_post_transfer_commands() {
	Array out = pending_post_transfer_commands.duplicate(true);
	pending_post_transfer_commands.clear();
	return out;
}

void ClockGearsDataManager::advance_runtime_clock(double p_delta_sec) {
	if (p_delta_sec > 0.0) {
		runtime_playtime_sec += p_delta_sec;
	}
}

void ClockGearsDataManager::mark_arpg_enemy_defeated(const String &p_map_key, int p_event_id) {
	if (p_map_key.is_empty() || p_event_id <= 0) {
		return;
	}
	Dictionary map_dict = (runtime_defeated_enemies.has(p_map_key) && runtime_defeated_enemies[p_map_key].get_type() == Variant::DICTIONARY)
			? Dictionary(runtime_defeated_enemies[p_map_key])
			: Dictionary();
	Dictionary info;
	info["defeated_at_sec"] = runtime_playtime_sec;
	map_dict[String::num_int64(p_event_id)] = info;
	runtime_defeated_enemies[p_map_key] = map_dict;
}

bool ClockGearsDataManager::is_arpg_enemy_defeated(const String &p_map_key, int p_event_id) const {
	if (p_map_key.is_empty() || p_event_id <= 0) {
		return false;
	}
	if (!runtime_defeated_enemies.has(p_map_key) || runtime_defeated_enemies[p_map_key].get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary map_dict = runtime_defeated_enemies[p_map_key];
	return map_dict.has(String::num_int64(p_event_id));
}

Array ClockGearsDataManager::process_map_respawns(const String &p_map_key, int p_respawn_mode, double p_respawn_minutes, bool p_is_entering_map) {
	Array respawned_ids;
	if (p_map_key.is_empty() || !runtime_defeated_enemies.has(p_map_key) || runtime_defeated_enemies[p_map_key].get_type() != Variant::DICTIONARY) {
		return respawned_ids;
	}
	Dictionary map_dict = Dictionary(runtime_defeated_enemies[p_map_key]);
	if (map_dict.is_empty()) {
		return respawned_ids;
	}
	if (p_respawn_mode == RESPAWN_NEVER) {
		return respawned_ids;
	}
	if (p_respawn_mode == RESPAWN_ON_REENTER) {
		if (!p_is_entering_map) {
			return respawned_ids;
		}
		for (const Variant &k : map_dict.keys()) {
			respawned_ids.push_back(String(k).to_int());
		}
		runtime_defeated_enemies.erase(p_map_key);
		return respawned_ids;
	}
	if (p_respawn_mode == RESPAWN_AFTER_MINUTES) {
		double req_sec = MAX(0.0, p_respawn_minutes) * 60.0;
		Array to_erase;
		for (const Variant &k : map_dict.keys()) {
			Dictionary info = (map_dict[k].get_type() == Variant::DICTIONARY) ? Dictionary(map_dict[k]) : Dictionary();
			double def_at = double(info.get("defeated_at_sec", runtime_playtime_sec));
			if ((runtime_playtime_sec - def_at) >= req_sec) {
				to_erase.push_back(k);
				respawned_ids.push_back(String(k).to_int());
			}
		}
		for (int i = 0; i < to_erase.size(); i++) {
			map_dict.erase(to_erase[i]);
		}
		if (map_dict.is_empty()) {
			runtime_defeated_enemies.erase(p_map_key);
		} else {
			runtime_defeated_enemies[p_map_key] = map_dict;
		}
	}
	return respawned_ids;
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
	emit_signal("runtime_state_changed", "actor_stats", String::num_int64(p_actor_id));
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
	return get_actor_equip_bonuses(p_actor_id, 1);
}

bool ClockGearsDataManager::is_arpg_mode_active(int p_mode_override) const {
	if (p_mode_override == COMBAT_MODE_ACTION_RPG) {
		return true;
	}
	if (p_mode_override == COMBAT_MODE_TURN_BASED) {
		return false;
	}
	return int(system_config.get("combat_mode", COMBAT_MODE_TURN_BASED)) == COMBAT_MODE_ACTION_RPG;
}

static Array _cg_normalize_normal_equips(ClockGearsDataManager *p_dm, int p_actor_id, const Array &p_raw) {
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	PackedStringArray sys_slots = cat ? cat->get_equip_slots() : PackedStringArray();
	if (sys_slots.is_empty()) {
		sys_slots.push_back("Mano Derecha (Arma)");
		sys_slots.push_back("Mano Izquierda (Escudo)");
		sys_slots.push_back("Cabeza (Casco)");
		sys_slots.push_back("Cuerpo (Pechera)");
		sys_slots.push_back("Accesorio");
	}
	Array raw = p_raw;
	if (raw.is_empty() && p_dm) {
		Dictionary act = p_dm->get_record_by_id(CAT_ACTORS, p_actor_id);
		if (act.has("equips") && act["equips"].get_type() == Variant::ARRAY) {
			raw = act["equips"];
		}
	}
	Array out;
	for (int s = 0; s < sys_slots.size(); s++) {
		String dcat = (s == 0) ? String("weapons") : ((s == 4) ? String("accessories") : String("armors"));
		int pid = 0;
		String cat_key = dcat;
		if (s < raw.size()) {
			if (raw[s].get_type() == Variant::DICTIONARY) {
				Dictionary rd = raw[s];
				pid = MAX(0, int(rd.get("piece_id", 0)));
				cat_key = String(rd.get("category", dcat));
			} else {
				pid = MAX(0, int(raw[s]));
			}
		}
		Dictionary sd;
		sd["slot_idx"] = s;
		sd["slot"] = vformat("slot_%d", s);
		sd["name"] = sys_slots[s];
		sd["label"] = sys_slots[s];
		sd["default_category"] = dcat;
		sd["category"] = cat_key;
		sd["piece_id"] = pid;
		out.push_back(sd);
	}
	return out;
}

Array ClockGearsDataManager::get_actor_equip_slots(int p_actor_id, int p_mode_override) const {
	if (is_arpg_mode_active(p_mode_override)) {
		Array arpg = get_actor_arpg_equip_slots(p_actor_id);
		for (int i = 0; i < arpg.size(); i++) {
			if (arpg[i].get_type() == Variant::DICTIONARY) {
				Dictionary sd = Dictionary(arpg[i]).duplicate(true);
				sd["slot_idx"] = i;
				sd["name"] = String(sd.get("label", sd.get("slot", vformat("Slot %d", i + 1))));
				arpg[i] = sd;
			}
		}
		return arpg;
	}
	String key = String::num_int64(p_actor_id);
	Array raw;
	if (runtime_equips.has(key) && runtime_equips[key].get_type() == Variant::ARRAY) {
		raw = runtime_equips[key];
	}
	return _cg_normalize_normal_equips(const_cast<ClockGearsDataManager *>(this), p_actor_id, raw);
}

static Array _cg_collect_actor_traits(ClockGearsDataManager *p_dm, int p_actor_id, int p_exclude_slot, int p_mode_override) {
	Array combined;
	if (!p_dm || p_actor_id <= 0) {
		return combined;
	}
	auto append_traits = [&](const Variant &p_arr) {
		if (p_arr.get_type() != Variant::ARRAY) {
			return;
		}
		Array arr = p_arr;
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].get_type() == Variant::DICTIONARY) {
				combined.push_back(arr[i]);
			}
		}
	};
	Dictionary act = p_dm->get_record_by_id(CAT_ACTORS, p_actor_id);
	append_traits(act.get("traits", Array()));
	int class_id = int(act.get("class_id", 0));
	if (class_id > 0) {
		Dictionary cls = p_dm->get_record_by_id(CAT_CLASSES, class_id);
		append_traits(cls.get("traits", Array()));
	}
	Array slots = p_dm->get_actor_equip_slots(p_actor_id, p_mode_override);
	for (int i = 0; i < slots.size(); i++) {
		if (i == p_exclude_slot || slots[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary sd = slots[i];
		int pid = int(sd.get("piece_id", 0));
		int cat = p_dm->get_category_from_key(String(sd.get("category", "")));
		if (pid > 0 && cat >= 0) {
			Dictionary piece = p_dm->get_record_by_id(cat, pid);
			append_traits(piece.get("traits", Array()));
		}
	}
	Array states = p_dm->get_actor_runtime_states(p_actor_id);
	for (int i = 0; i < states.size(); i++) {
		Dictionary srec = p_dm->get_record_by_id(CAT_STATES, int(states[i]));
		append_traits(srec.get("traits", Array()));
	}
	return combined;
}

bool ClockGearsDataManager::is_actor_equip_slot_locked(int p_actor_id, int p_slot_idx, int p_mode_override) const {
	Array traits = _cg_collect_actor_traits(const_cast<ClockGearsDataManager *>(this), p_actor_id, -1, p_mode_override);
	for (int i = 0; i < traits.size(); i++) {
		Dictionary tr = traits[i];
		if (int(tr.get("code", 0)) == CG_TRAIT_EQUIP_LOCK && int(tr.get("data_id", -1)) == p_slot_idx) {
			return true;
		}
	}
	return false;
}

bool ClockGearsDataManager::is_actor_equip_slot_sealed(int p_actor_id, int p_slot_idx, int p_mode_override) const {
	Array traits = _cg_collect_actor_traits(const_cast<ClockGearsDataManager *>(this), p_actor_id, -1, p_mode_override);
	for (int i = 0; i < traits.size(); i++) {
		Dictionary tr = traits[i];
		if (int(tr.get("code", 0)) == CG_TRAIT_EQUIP_SEAL && int(tr.get("data_id", -1)) == p_slot_idx) {
			return true;
		}
	}
	return false;
}

bool ClockGearsDataManager::can_actor_equip_piece(int p_actor_id, int p_slot_idx, int p_category, int p_piece_id, int p_mode_override) const {
	if (p_actor_id <= 0 || p_piece_id <= 0) {
		return false;
	}
	Dictionary act = get_record_by_id(CAT_ACTORS, p_actor_id);
	if (act.is_empty()) {
		return false;
	}
	Array slots = get_actor_equip_slots(p_actor_id, p_mode_override);
	if (p_slot_idx < 0 || p_slot_idx >= slots.size()) {
		return false;
	}
	if (is_actor_equip_slot_locked(p_actor_id, p_slot_idx, p_mode_override) || is_actor_equip_slot_sealed(p_actor_id, p_slot_idx, p_mode_override)) {
		return false;
	}
	Dictionary piece = get_record_by_id(p_category, p_piece_id);
	if (piece.is_empty()) {
		return false;
	}
	Dictionary sd = slots[p_slot_idx];
	bool arpg = is_arpg_mode_active(p_mode_override);
	if (arpg) {
		if (get_category_key(p_category) != String(sd.get("category", ""))) {
			return false;
		}
	} else {
		if (p_category == CAT_WEAPONS) {
			if (int(piece.get("secondary_type", 0)) != p_slot_idx) {
				return false;
			}
		} else if (p_category == CAT_ARMORS) {
			if (int(piece.get("secondary_type", 3)) != p_slot_idx) {
				return false;
			}
		} else if (p_category == CAT_ACCESSORIES) {
			if (int(piece.get("type", 4)) != p_slot_idx) {
				return false;
			}
		} else {
			return false;
		}
	}

	Array traits = _cg_collect_actor_traits(const_cast<ClockGearsDataManager *>(this), p_actor_id, p_slot_idx, p_mode_override);
	Vector<int> allowed_wtypes;
	Vector<int> allowed_atypes;
	for (int i = 0; i < traits.size(); i++) {
		Dictionary tr = traits[i];
		int code = int(tr.get("code", 0));
		int did = int(tr.get("data_id", 0));
		if (code == CG_TRAIT_EQUIP_WTYPE && !allowed_wtypes.has(did)) {
			allowed_wtypes.push_back(did);
		} else if (code == CG_TRAIT_EQUIP_ATYPE && !allowed_atypes.has(did)) {
			allowed_atypes.push_back(did);
		}
	}
	if ((p_category == CAT_WEAPONS || (p_category == CAT_ARPG_WEAPONS && piece.has("weapon_type"))) && !allowed_wtypes.is_empty()) {
		int wt = piece.has("weapon_type") ? int(piece.get("weapon_type", 0)) : int(piece.get("type", 0));
		if (!allowed_wtypes.has(wt)) {
			return false;
		}
	}
	if ((p_category == CAT_ARMORS || ((p_category == CAT_ARPG_ARMORS || p_category == CAT_ARPG_SHIELDS) && piece.has("armor_type"))) && !allowed_atypes.is_empty()) {
		int at = piece.has("armor_type") ? int(piece.get("armor_type", 0)) : int(piece.get("type", 0));
		if (!allowed_atypes.has(at)) {
			return false;
		}
	}
	return true;
}

static bool _cg_find_matching_piece_in_cats(ClockGearsDataManager *p_dm, const String &p_key, const Vector<int> &p_cats, int p_actor_id, int p_slot_idx, int p_mode_override, int &r_cat, int &r_id, Dictionary &r_rec) {
	Dictionary direct = p_dm->find_inventory_record(p_key);
	if (bool(direct.get("found", false))) {
		int dcat = int(direct.get("category", -1));
		int did = int(direct.get("id", 0));
		if (p_cats.has(dcat) && p_dm->can_actor_equip_piece(p_actor_id, p_slot_idx, dcat, did, p_mode_override)) {
			r_cat = dcat;
			r_id = did;
			r_rec = direct.get("record", Dictionary());
			return true;
		}
	}
	if (p_key.find(":") != -1) {
		return false;
	}
	String k_low = p_key.strip_edges().to_lower();
	for (int ci = 0; ci < p_cats.size(); ci++) {
		int cat = p_cats[ci];
		Array arr = p_dm->get_category_array(cat);
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary rec = arr[i];
			String rname = String(rec.get("name", "")).strip_edges();
			if (!rname.is_empty() && (rname == p_key.strip_edges() || rname.to_lower() == k_low)) {
				int rid = int(rec.get("id", i + 1));
				if (p_dm->can_actor_equip_piece(p_actor_id, p_slot_idx, cat, rid, p_mode_override)) {
					r_cat = cat;
					r_id = rid;
					r_rec = rec;
					return true;
				}
			}
		}
	}
	return false;
}

Array ClockGearsDataManager::get_equippable_pieces_for_slot(int p_actor_id, int p_slot_idx, int p_mode_override) const {
	Array out;
	Array slots = get_actor_equip_slots(p_actor_id, p_mode_override);
	if (p_slot_idx < 0 || p_slot_idx >= slots.size()) {
		return out;
	}
	if (is_actor_equip_slot_locked(p_actor_id, p_slot_idx, p_mode_override) || is_actor_equip_slot_sealed(p_actor_id, p_slot_idx, p_mode_override)) {
		return out;
	}
	Vector<int> candidate_cats;
	if (is_arpg_mode_active(p_mode_override)) {
		int scat = get_category_from_key(String(Dictionary(slots[p_slot_idx]).get("category", "")));
		if (scat >= 0) {
			candidate_cats.push_back(scat);
		}
	} else {
		candidate_cats.push_back(CAT_WEAPONS);
		candidate_cats.push_back(CAT_ARMORS);
		candidate_cats.push_back(CAT_ACCESSORIES);
	}

	Array keys = runtime_inventory.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = String(keys[i]);
		int qty = MAX(0, int(runtime_inventory[keys[i]]));
		if (qty <= 0) {
			continue;
		}
		int matched_cat = -1;
		int matched_id = 0;
		Dictionary rec;
		if (_cg_find_matching_piece_in_cats(const_cast<ClockGearsDataManager *>(this), key, candidate_cats, p_actor_id, p_slot_idx, p_mode_override, matched_cat, matched_id, rec)) {
			Dictionary row;
			row["key"] = key;
			row["category"] = matched_cat;
			row["category_key"] = get_category_key(matched_cat);
			row["id"] = matched_id;
			row["name"] = String(rec.get("name", key));
			row["desc"] = String(rec.get("desc", ""));
			row["icon_path"] = String(rec.get("icon_path", rec.get("graphic_path", "")));
			row["qty"] = qty;
			row["record"] = rec;
			out.push_back(row);
		}
	}
	return out;
}

bool ClockGearsDataManager::equip_actor_piece(int p_actor_id, int p_slot_idx, const String &p_inventory_key, int p_mode_override) {
	if (get_item_qty(p_inventory_key) <= 0) {
		return false;
	}
	Array slots = get_actor_equip_slots(p_actor_id, p_mode_override);
	if (p_slot_idx < 0 || p_slot_idx >= slots.size()) {
		return false;
	}
	Vector<int> candidate_cats;
	bool arpg = is_arpg_mode_active(p_mode_override);
	if (arpg) {
		int scat = get_category_from_key(String(Dictionary(slots[p_slot_idx]).get("category", "")));
		if (scat >= 0) {
			candidate_cats.push_back(scat);
		}
	} else {
		candidate_cats.push_back(CAT_WEAPONS);
		candidate_cats.push_back(CAT_ARMORS);
		candidate_cats.push_back(CAT_ACCESSORIES);
	}
	int new_cat = -1;
	int new_id = 0;
	Dictionary new_rec;
	if (!_cg_find_matching_piece_in_cats(this, p_inventory_key, candidate_cats, p_actor_id, p_slot_idx, p_mode_override, new_cat, new_id, new_rec)) {
		return false;
	}

	Dictionary cur_slot = slots[p_slot_idx];
	int old_id = int(cur_slot.get("piece_id", 0));
	int old_cat = get_category_from_key(String(cur_slot.get("category", "")));

	change_item(p_inventory_key, -1);
	if (old_id > 0 && old_cat >= 0) {
		String old_key = get_record_inventory_key(old_cat, old_id);
		if (!old_key.is_empty()) {
			change_item(old_key, 1);
		}
	}

	cur_slot["category"] = get_category_key(new_cat);
	cur_slot["piece_id"] = new_id;
	slots[p_slot_idx] = cur_slot;

	if (arpg) {
		runtime_arpg_equips[String::num_int64(p_actor_id)] = normalize_arpg_equip_slots(slots);
		emit_signal("data_changed", "arpg_equip", p_actor_id);
	} else {
		runtime_equips[String::num_int64(p_actor_id)] = slots;
		emit_signal("data_changed", "equip", p_actor_id);
	}
	emit_signal("runtime_state_changed", "actor_stats", String::num_int64(p_actor_id));
	return true;
}

bool ClockGearsDataManager::unequip_actor_slot(int p_actor_id, int p_slot_idx, int p_mode_override) {
	if (p_actor_id <= 0 || is_actor_equip_slot_locked(p_actor_id, p_slot_idx, p_mode_override)) {
		return false;
	}
	Array slots = get_actor_equip_slots(p_actor_id, p_mode_override);
	if (p_slot_idx < 0 || p_slot_idx >= slots.size()) {
		return false;
	}
	Dictionary sd = slots[p_slot_idx];
	int old_id = int(sd.get("piece_id", 0));
	if (old_id <= 0) {
		return false;
	}
	int old_cat = get_category_from_key(String(sd.get("category", "")));
	if (old_cat >= 0) {
		String old_key = get_record_inventory_key(old_cat, old_id);
		if (!old_key.is_empty()) {
			change_item(old_key, 1);
		}
	}
	sd["piece_id"] = 0;
	if (!is_arpg_mode_active(p_mode_override) && sd.has("default_category")) {
		sd["category"] = sd["default_category"];
	}
	slots[p_slot_idx] = sd;
	if (is_arpg_mode_active(p_mode_override)) {
		runtime_arpg_equips[String::num_int64(p_actor_id)] = normalize_arpg_equip_slots(slots);
		emit_signal("data_changed", "arpg_equip", p_actor_id);
	} else {
		runtime_equips[String::num_int64(p_actor_id)] = slots;
		emit_signal("data_changed", "equip", p_actor_id);
	}
	emit_signal("runtime_state_changed", "actor_stats", String::num_int64(p_actor_id));
	return true;
}

Dictionary ClockGearsDataManager::get_actor_equip_bonuses(int p_actor_id, int p_mode_override) const {
	static const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
	Dictionary sum;
	for (int k = 0; k < 8; k++) {
		sum[stat_keys[k]] = 0;
	}
	Array slots = get_actor_equip_slots(p_actor_id, p_mode_override);
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

Dictionary ClockGearsDataManager::get_actor_effective_stats(int p_actor_id, int p_mode_override) const {
	static const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
	static const char *bonus_keys[8] = {
		"max_hp_bonus", "max_mp_bonus", "atk_bonus", "def_bonus",
		"mat_bonus", "mdf_bonus", "agi_bonus", "luk_bonus"
	};
	Dictionary act = get_record_by_id(CAT_ACTORS, p_actor_id);
	Dictionary bonuses = get_actor_equip_bonuses(p_actor_id, p_mode_override);
	Dictionary rt;
	String rt_key = String::num_int64(p_actor_id);
	if (runtime_actor_states.has(rt_key) && runtime_actor_states[rt_key].get_type() == Variant::DICTIONARY) {
		rt = runtime_actor_states[rt_key];
	}
	Dictionary res;
	for (int k = 0; k < 8; k++) {
		int val = int(act.get(stat_keys[k], 0)) + int(bonuses.get(stat_keys[k], 0)) + int(rt.get(bonus_keys[k], 0));
		res[stat_keys[k]] = MAX(0, val);
	}
	return res;
}

Dictionary ClockGearsDataManager::preview_equip_actor_stats(int p_actor_id, int p_slot_idx, int p_candidate_category, int p_candidate_piece_id, int p_mode_override) const {
	static const char *stat_keys[8] = { "hp", "mp", "atk", "def", "mat", "mdf", "agi", "luk" };
	Dictionary before = get_actor_effective_stats(p_actor_id, p_mode_override);
	Dictionary after = before.duplicate(true);
	Array slots = get_actor_equip_slots(p_actor_id, p_mode_override);
	if (p_slot_idx >= 0 && p_slot_idx < slots.size()) {
		Dictionary cur_sd = slots[p_slot_idx];
		int cur_pid = int(cur_sd.get("piece_id", 0));
		int cur_cat = get_category_from_key(String(cur_sd.get("category", "")));
		Dictionary old_piece = (cur_pid > 0 && cur_cat >= 0) ? get_record_by_id(cur_cat, cur_pid) : Dictionary();
		Dictionary new_piece = (p_candidate_piece_id > 0 && p_candidate_category >= 0) ? get_record_by_id(p_candidate_category, p_candidate_piece_id) : Dictionary();
		for (int k = 0; k < 8; k++) {
			int cur_v = int(before.get(stat_keys[k], 0));
			int next_v = cur_v - int(old_piece.get(stat_keys[k], 0)) + int(new_piece.get(stat_keys[k], 0));
			after[stat_keys[k]] = MAX(0, next_v);
		}
	}
	Dictionary diff;
	for (int k = 0; k < 8; k++) {
		diff[stat_keys[k]] = int(after.get(stat_keys[k], 0)) - int(before.get(stat_keys[k], 0));
	}
	Dictionary out;
	out["before"] = before;
	out["after"] = after;
	out["diff"] = diff;
	return out;
}

Dictionary ClockGearsDataManager::get_actor_status_sheet(int p_actor_id, int p_mode_override) {
	Dictionary out;
	Dictionary act = get_record_by_id(CAT_ACTORS, p_actor_id);
	Dictionary st = get_actor_runtime_state(p_actor_id);
	Dictionary stats = get_actor_effective_stats(p_actor_id, p_mode_override);

	int class_id = int(act.get("class_id", 0));
	Dictionary cls = (class_id > 0) ? get_record_by_id(CAT_CLASSES, class_id) : Dictionary();
	int lv = MAX(1, int(st.get("level", 1)));
	int64_t exp = int64_t(st.get("exp", 0));
	int64_t next_exp = 0;
	if (ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton()) {
		int eb = cls.is_empty() ? 30 : int(cls.get("exp_base", 30));
		int ee = cls.is_empty() ? 20 : int(cls.get("exp_extra", 20));
		int ea = cls.is_empty() ? 30 : int(cls.get("exp_acc_a", 30));
		int ebp = cls.is_empty() ? 30 : int(cls.get("exp_acc_b", 30));
		next_exp = fm->calculate_exp_for_level(lv + 1, eb, ee, ea, ebp);
	}

	int max_hp = MAX(1, int(stats.get("hp", st.get("max_hp", 100))));
	int max_mp = MAX(0, int(stats.get("mp", st.get("max_mp", 20))));

	out["actor_id"] = p_actor_id;
	out["name"] = String(act.get("name", vformat("Actor %d", p_actor_id)));
	out["class_id"] = class_id;
	out["class_name"] = cls.is_empty() ? String("") : String(cls.get("name", ""));
	out["level"] = lv;
	out["exp"] = exp;
	out["next_level_exp"] = next_exp;
	out["remaining_exp"] = MAX(int64_t(0), next_exp - exp);
	out["hp"] = CLAMP(int(st.get("hp", max_hp)), 0, max_hp);
	out["max_hp"] = max_hp;
	out["mp"] = CLAMP(int(st.get("mp", max_mp)), 0, max_mp);
	out["max_mp"] = max_mp;
	out["stats"] = stats;

	Array states_out;
	Array st_ids = get_actor_runtime_states(p_actor_id);
	for (int i = 0; i < st_ids.size(); i++) {
		int sid = int(st_ids[i]);
		Dictionary srec = get_record_by_id(CAT_STATES, sid);
		Dictionary srow;
		srow["id"] = sid;
		srow["name"] = srec.is_empty() ? vformat("Estado %d", sid) : String(srec.get("name", vformat("Estado %d", sid)));
		srow["icon_path"] = srec.is_empty() ? String("") : String(srec.get("icon_path", srec.get("graphic_path", "")));
		states_out.push_back(srow);
	}
	out["states"] = states_out;

	Array elem_rates;
	ClockGearsSystemCatalog *cat = ClockGearsSystemCatalog::get_singleton();
	if (cat) {
		PackedStringArray elems = cat->get_elements();
		PackedStringArray ekeys = cat->get_element_keys();
		for (int i = 0; i < elems.size(); i++) {
			int eid = i + 1;
			String ekey = (i < ekeys.size()) ? ekeys[i] : String();
			float rate = get_actor_element_rate(p_actor_id, eid, ekey);
			Dictionary erow;
			erow["id"] = eid;
			erow["key"] = ekey;
			erow["name"] = elems[i];
			erow["rate"] = rate;
			erow["pct"] = int(Math::round(rate * 100.0f));
			elem_rates.push_back(erow);
		}
	}
	out["element_rates"] = elem_rates;
	out["equips"] = get_actor_equip_slots(p_actor_id, p_mode_override);
	return out;
}

Dictionary ClockGearsDataManager::export_runtime_state() const {
	Dictionary d;
	d["switches"] = runtime_switches.duplicate(true);
	d["variables"] = runtime_variables.duplicate(true);
	d["inventory"] = runtime_inventory.duplicate(true);
	d["gold"] = runtime_gold;
	d["equips"] = runtime_equips.duplicate(true);
	d["arpg_equips"] = runtime_arpg_equips.duplicate(true);
	d["actor_states"] = runtime_actor_states.duplicate(true);
	d["defeated_enemies"] = runtime_defeated_enemies.duplicate(true);
	d["playtime_sec"] = runtime_playtime_sec;
	d["game_menu_disabled"] = runtime_game_menu_disabled;
	d["window_style_overrides"] = runtime_window_style_overrides.duplicate(true);
	d["hud_visible"] = runtime_hud_visible;
	d["hud_widget_visibility"] = runtime_hud_widget_visibility.duplicate(true);
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
	runtime_equips.clear();
	if (p_state.has("equips") && p_state["equips"].get_type() == Variant::DICTIONARY) {
		Dictionary neq = p_state["equips"];
		for (const Variant &k : neq.keys()) {
			if (neq[k].get_type() == Variant::ARRAY) {
				int aid = String(k).to_int();
				runtime_equips[String(k)] = _cg_normalize_normal_equips(this, aid, Array(neq[k]));
			}
		}
	}
	runtime_arpg_equips.clear();
	if (p_state.has("arpg_equips") && p_state["arpg_equips"].get_type() == Variant::DICTIONARY) {
		Dictionary eq = p_state["arpg_equips"];
		for (const Variant &k : eq.keys()) {
			if (eq[k].get_type() == Variant::ARRAY) {
				runtime_arpg_equips[String(k)] = normalize_arpg_equip_slots(Array(eq[k]));
			}
		}
	}
	runtime_actor_states.clear();
	if (p_state.has("actor_states") && p_state["actor_states"].get_type() == Variant::DICTIONARY) {
		runtime_actor_states = Dictionary(p_state["actor_states"]).duplicate(true);
	}
	runtime_defeated_enemies.clear();
	if (p_state.has("defeated_enemies") && p_state["defeated_enemies"].get_type() == Variant::DICTIONARY) {
		runtime_defeated_enemies = Dictionary(p_state["defeated_enemies"]).duplicate(true);
	}
	runtime_playtime_sec = MAX(0.0, double(p_state.get("playtime_sec", 0.0)));
	runtime_game_menu_disabled = bool(p_state.get("game_menu_disabled", false));
	runtime_window_style_overrides.clear();
	if (p_state.has("window_style_overrides") && p_state["window_style_overrides"].get_type() == Variant::DICTIONARY) {
		Dictionary wov = p_state["window_style_overrides"];
		for (const Variant &k : wov.keys()) {
			String wt = String(k).strip_edges().to_lower();
			String sid = String(wov[k]).strip_edges().to_lower();
			if (!wt.is_empty() && !sid.is_empty()) {
				runtime_window_style_overrides[wt] = sid;
			}
		}
	}
	runtime_hud_visible = p_state.has("hud_visible") ? bool(p_state["hud_visible"]) : true;
	runtime_hud_widget_visibility.clear();
	if (p_state.has("hud_widget_visibility") && p_state["hud_widget_visibility"].get_type() == Variant::DICTIONARY) {
		Dictionary hvw = p_state["hud_widget_visibility"];
		for (const Variant &k : hvw.keys()) {
			String key = String(k).strip_edges().to_lower();
			if (!key.is_empty()) {
				runtime_hud_widget_visibility[key] = bool(hvw[k]);
			}
		}
	}
	emit_signal("runtime_state_changed", "all", String());
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
				out += get_currency_unit();
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

static String s_normalize_scale_mode(const Variant &p_val) {
	String s = String(p_val).strip_edges().to_lower();
	if (s == "fill" || s == "rellenar") return "fill";
	if (s == "stretch" || s == "estirar") return "stretch";
	return "fit";
}

static String s_normalize_texture_filter_mode(const Variant &p_val) {
	String s = String(p_val).strip_edges().to_lower();
	if (s == "linear" || s == "lineal") return "linear";
	return "nearest";
}

Dictionary ClockGearsDataManager::get_default_title_menu_config() {
	Dictionary cfg;
	cfg["schema_version"] = 1;
	cfg["save_slots"] = 10;
	cfg["credits_configured"] = false;
	cfg["credits"] = Array();
	cfg["bgm_path"] = "";

	Dictionary bg;
	bg["type"] = "color";
	bg["color"] = "#12151e";
	bg["image_path"] = "";
	bg["scale_mode"] = "fit";
	bg["filter"] = "nearest";
	cfg["background"] = bg;

	Dictionary title;
	title["type"] = "text";
	title["text"] = "";
	title["image_path"] = "";
	title["filter"] = "nearest";
	cfg["title"] = title;

	static const char *k_ids[5] = { "new_game", "continue", "options", "credits", "quit" };
	static const char *k_labels[5] = { "Nueva partida", "Continuar", "Opciones", "Créditos", "Salir" };
	Array entries;
	for (int i = 0; i < 5; i++) {
		Dictionary e;
		e["id"] = k_ids[i];
		e["text"] = String::utf8(k_labels[i]);
		e["enabled"] = true;
		e["order"] = i;
		entries.push_back(e);
	}
	cfg["entries"] = entries;
	return cfg;
}

Dictionary ClockGearsDataManager::normalize_title_menu_config(const Dictionary &p_cfg) {
	Dictionary def = get_default_title_menu_config();
	Dictionary out;
	out["schema_version"] = int(p_cfg.get("schema_version", 1));
	out["save_slots"] = CLAMP(int(p_cfg.get("save_slots", 10)), 1, 99);
	out["credits_configured"] = bool(p_cfg.get("credits_configured", false));
	out["credits"] = (p_cfg.has("credits") && p_cfg["credits"].get_type() == Variant::ARRAY) ? Array(p_cfg["credits"]).duplicate(true) : Array();
	out["bgm_path"] = String(p_cfg.get("bgm_path", p_cfg.get("music_path", ""))).strip_edges();

	Dictionary def_bg = def["background"];
	Dictionary raw_bg = (p_cfg.has("background") && p_cfg["background"].get_type() == Variant::DICTIONARY) ? Dictionary(p_cfg["background"]) : Dictionary();
	Dictionary bg;
	String bg_img = String(raw_bg.get("image_path", raw_bg.get("path", ""))).strip_edges();
	String bg_type = String(raw_bg.get("type", bg_img.is_empty() ? "color" : "image")).strip_edges().to_lower();
	if (bg_type != "image") {
		bg_type = "color";
	}
	String bg_col = String(raw_bg.get("color", def_bg["color"])).strip_edges();
	if (bg_col.is_empty() || !Color::html_is_valid(bg_col)) {
		bg_col = String(def_bg["color"]);
	}
	bg["type"] = bg_type;
	bg["color"] = bg_col;
	bg["image_path"] = bg_img;
	bg["scale_mode"] = s_normalize_scale_mode(raw_bg.get("scale_mode", def_bg["scale_mode"]));
	bg["filter"] = s_normalize_texture_filter_mode(raw_bg.get("filter", def_bg["filter"]));
	out["background"] = bg;

	Dictionary def_title = def["title"];
	Dictionary raw_title = (p_cfg.has("title") && p_cfg["title"].get_type() == Variant::DICTIONARY) ? Dictionary(p_cfg["title"]) : Dictionary();
	Dictionary title;
	String t_img = String(raw_title.get("image_path", raw_title.get("path", ""))).strip_edges();
	String t_type = String(raw_title.get("type", "text")).strip_edges().to_lower();
	if (t_type != "image") {
		t_type = "text";
	}
	title["type"] = t_type;
	title["text"] = String(raw_title.get("text", def_title["text"]));
	title["image_path"] = t_img;
	title["filter"] = s_normalize_texture_filter_mode(raw_title.get("filter", def_title["filter"]));
	out["title"] = title;

	Array def_entries = def["entries"];
	Dictionary merged_by_id;
	for (int i = 0; i < def_entries.size(); i++) {
		Dictionary de = Dictionary(def_entries[i]).duplicate(true);
		merged_by_id[String(de["id"])] = de;
	}

	if (p_cfg.has("entries") && p_cfg["entries"].get_type() == Variant::ARRAY) {
		Array raw_entries = p_cfg["entries"];
		for (int i = 0; i < raw_entries.size(); i++) {
			if (raw_entries[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary re = raw_entries[i];
			String id = String(re.get("id", "")).strip_edges().to_lower();
			if (!merged_by_id.has(id)) {
				continue;
			}
			Dictionary e = Dictionary(merged_by_id[id]).duplicate(true);
			String txt = String(re.get("text", e["text"]));
			if (!txt.strip_edges().is_empty()) {
				e["text"] = txt;
			}
			if (re.has("enabled")) {
				e["enabled"] = bool(re["enabled"]);
			}
			if (re.has("order")) {
				e["order"] = int(re["order"]);
			} else {
				e["order"] = i;
			}
			merged_by_id[id] = e;
		}
	}

	Vector<Dictionary> sorted;
	for (int i = 0; i < def_entries.size(); i++) {
		String id = String(Dictionary(def_entries[i])["id"]);
		sorted.push_back(Dictionary(merged_by_id[id]));
	}
	for (int i = 0; i < sorted.size(); i++) {
		for (int j = i + 1; j < sorted.size(); j++) {
			int oi = int(sorted[i].get("order", i));
			int oj = int(sorted[j].get("order", j));
			if (oj < oi) {
				Dictionary tmp = sorted[i];
				sorted.write[i] = sorted[j];
				sorted.write[j] = tmp;
			}
		}
	}
	Array norm_entries;
	for (int i = 0; i < sorted.size(); i++) {
		Dictionary e = sorted[i].duplicate(true);
		e["order"] = i;
		norm_entries.push_back(e);
	}
	out["entries"] = norm_entries;
	return out;
}

void ClockGearsDataManager::set_title_menu_config(const Dictionary &p_cfg) {
	title_menu_config = normalize_title_menu_config(p_cfg);
	dirty = true;
	emit_signal("data_changed", "title_menu", 0);
}

Array ClockGearsDataManager::get_visible_title_entries() const {
	Dictionary norm = normalize_title_menu_config(title_menu_config);
	Array entries = norm.get("entries", Array());
	bool has_saves = has_any_save();
	bool has_credits = has_credits_lines();

	Array out;
	for (int i = 0; i < entries.size(); i++) {
		Dictionary e = entries[i];
		if (!bool(e.get("enabled", true))) {
			continue;
		}
		String id = String(e.get("id", ""));
		if (id == "continue" && !has_saves) {
			continue;
		}
		if (id == "credits" && !has_credits) {
			continue;
		}
		out.push_back(e.duplicate(true));
	}
	return out;
}

int ClockGearsDataManager::get_save_slot_count() const {
	return CLAMP(int(title_menu_config.get("save_slots", 10)), 1, 99);
}

void ClockGearsDataManager::set_save_slot_count(int p_count) {
	title_menu_config["save_slots"] = CLAMP(p_count, 1, 99);
	dirty = true;
	emit_signal("data_changed", "title_menu", 0);
}

// --- Menú dentro del juego (9B Tanda 3) ---

static Dictionary _cg_default_menu_bg(const String &p_col = "#0c101ccc") {
	Dictionary bg;
	bg["type"] = "color";
	bg["color"] = p_col;
	bg["image_path"] = "";
	bg["scale_mode"] = "fit";
	bg["filter"] = "nearest";
	return bg;
}

static Dictionary _cg_normalize_menu_bg(const Variant &p_raw, const Dictionary &p_fallback) {
	Dictionary raw = (p_raw.get_type() == Variant::DICTIONARY) ? Dictionary(p_raw) : Dictionary();
	Dictionary bg;
	String img = String(raw.get("image_path", raw.get("path", ""))).strip_edges();
	String btype = String(raw.get("type", img.is_empty() ? "color" : "image")).strip_edges().to_lower();
	if (btype != "image") {
		btype = "color";
	}
	String col = String(raw.get("color", p_fallback.get("color", "#0c101ccc"))).strip_edges();
	if (col.is_empty() || !Color::html_is_valid(col)) {
		col = String(p_fallback.get("color", "#0c101ccc"));
	}
	bg["type"] = btype;
	bg["color"] = col;
	bg["image_path"] = img;
	bg["scale_mode"] = s_normalize_scale_mode(raw.get("scale_mode", p_fallback.get("scale_mode", "fit")));
	bg["filter"] = s_normalize_texture_filter_mode(raw.get("filter", p_fallback.get("filter", "nearest")));
	return bg;
}

Dictionary ClockGearsDataManager::get_default_game_menu_config() {
	Dictionary cfg;
	cfg["schema_version"] = 1;

	Dictionary bgs;
	bgs["menu"] = _cg_default_menu_bg("#0c101ccc");
	bgs["items"] = _cg_default_menu_bg("#0c101ccc");
	bgs["equip"] = _cg_default_menu_bg("#0c101ccc");
	bgs["status"] = _cg_default_menu_bg("#0c101ccc");
	cfg["backgrounds"] = bgs;

	struct EntrySpec {
		const char *id;
		const char *label;
		bool enabled;
		bool reserved;
	};
	static const EntrySpec k_specs[7] = {
		{ "items", "Objetos", true, false },
		{ "equip", "Equipo", true, false },
		{ "status", "Estado", true, false },
		{ "save", "Guardar", true, false },
		{ "options", "Opciones", true, false },
		{ "to_title", "Salir al título", true, false },
		{ "magic", "Magia", false, true }
	};
	Array entries;
	for (int i = 0; i < 7; i++) {
		Dictionary e;
		e["id"] = String(k_specs[i].id);
		e["text"] = String::utf8(k_specs[i].label);
		e["enabled"] = k_specs[i].enabled;
		e["reserved"] = k_specs[i].reserved;
		e["order"] = i;
		entries.push_back(e);
	}
	cfg["entries"] = entries;

	Dictionary item_prev;
	item_prev["enabled"] = true;
	item_prev["position"] = "right";
	item_prev["show_icon"] = true;
	item_prev["show_name"] = true;
	item_prev["show_desc"] = true;
	item_prev["icon_scale"] = 2;
	item_prev["rotation_enabled"] = true;
	item_prev["rotation_step"] = 15;
	item_prev["preview_window_style"] = "";
	item_prev["desc_window_style"] = "";
	cfg["item_preview"] = item_prev;

	return cfg;
}

Dictionary ClockGearsDataManager::normalize_game_menu_config(const Dictionary &p_cfg) {
	Dictionary def = get_default_game_menu_config();
	Dictionary out;
	out["schema_version"] = int(p_cfg.get("schema_version", 1));

	Dictionary def_bgs = def["backgrounds"];
	Dictionary raw_bgs = (p_cfg.has("backgrounds") && p_cfg["backgrounds"].get_type() == Variant::DICTIONARY) ? Dictionary(p_cfg["backgrounds"]) : Dictionary();
	Dictionary norm_bgs;
	static const char *k_bg_keys[4] = { "menu", "items", "equip", "status" };
	for (int i = 0; i < 4; i++) {
		String bk = k_bg_keys[i];
		norm_bgs[bk] = _cg_normalize_menu_bg(raw_bgs.get(bk, Dictionary()), def_bgs[bk]);
	}
	out["backgrounds"] = norm_bgs;

	Array def_entries = def["entries"];
	Dictionary merged_by_id;
	for (int i = 0; i < def_entries.size(); i++) {
		Dictionary de = Dictionary(def_entries[i]).duplicate(true);
		merged_by_id[String(de["id"])] = de;
	}
	Vector<String> seen_in_raw;
	if (p_cfg.has("entries") && p_cfg["entries"].get_type() == Variant::ARRAY) {
		Array raw_entries = p_cfg["entries"];
		for (int i = 0; i < raw_entries.size(); i++) {
			if (raw_entries[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary re = raw_entries[i];
			String id = String(re.get("id", "")).strip_edges().to_lower();
			if (!merged_by_id.has(id) || seen_in_raw.has(id)) {
				continue;
			}
			Dictionary e = Dictionary(merged_by_id[id]).duplicate(true);
			String txt = String(re.get("text", e["text"]));
			if (!txt.strip_edges().is_empty()) {
				e["text"] = txt;
			}
			if (re.has("enabled")) {
				e["enabled"] = (id == "magic") ? false : bool(re["enabled"]);
			}
			e["order"] = int(seen_in_raw.size());
			seen_in_raw.push_back(id);
			merged_by_id[id] = e;
		}
	}

	Vector<Dictionary> sorted;
	for (int i = 0; i < seen_in_raw.size(); i++) {
		sorted.push_back(Dictionary(merged_by_id[seen_in_raw[i]]));
	}
	for (int i = 0; i < def_entries.size(); i++) {
		String id = String(Dictionary(def_entries[i])["id"]);
		if (!seen_in_raw.has(id)) {
			sorted.push_back(Dictionary(merged_by_id[id]));
		}
	}
	Array norm_entries;
	for (int i = 0; i < sorted.size(); i++) {
		Dictionary e = sorted[i].duplicate(true);
		e["order"] = i;
		norm_entries.push_back(e);
	}
	out["entries"] = norm_entries;

	Dictionary def_item_prev = def.get("item_preview", Dictionary());
	Dictionary raw_item_prev = (p_cfg.has("item_preview") && p_cfg["item_preview"].get_type() == Variant::DICTIONARY)
			? Dictionary(p_cfg["item_preview"])
			: Dictionary();
	Dictionary norm_item_prev;
	norm_item_prev["enabled"] = bool(raw_item_prev.get("enabled", def_item_prev.get("enabled", true)));
	String pos = String(raw_item_prev.get("position", def_item_prev.get("position", "right"))).to_lower().strip_edges();
	norm_item_prev["position"] = (pos == "left") ? "left" : "right";
	norm_item_prev["show_icon"] = bool(raw_item_prev.get("show_icon", def_item_prev.get("show_icon", true)));
	norm_item_prev["show_name"] = bool(raw_item_prev.get("show_name", def_item_prev.get("show_name", true)));
	norm_item_prev["show_desc"] = bool(raw_item_prev.get("show_desc", def_item_prev.get("show_desc", true)));
	int sc = int(raw_item_prev.get("icon_scale", def_item_prev.get("icon_scale", 2)));
	norm_item_prev["icon_scale"] = (sc == 3 || sc == 4) ? sc : 2;
	norm_item_prev["rotation_enabled"] = bool(raw_item_prev.get("rotation_enabled", def_item_prev.get("rotation_enabled", true)));
	int step = int(raw_item_prev.get("rotation_step", def_item_prev.get("rotation_step", 15)));
	if (step <= 0 || step > 90) {
		step = 15;
	}
	norm_item_prev["rotation_step"] = step;
	norm_item_prev["preview_window_style"] = String(raw_item_prev.get("preview_window_style", def_item_prev.get("preview_window_style", "")));
	norm_item_prev["desc_window_style"] = String(raw_item_prev.get("desc_window_style", def_item_prev.get("desc_window_style", "")));
	out["item_preview"] = norm_item_prev;

	return out;
}

void ClockGearsDataManager::set_game_menu_config(const Dictionary &p_cfg) {
	game_menu_config = normalize_game_menu_config(p_cfg);
	dirty = true;
	emit_signal("data_changed", "game_menu", 0);
}

Array ClockGearsDataManager::get_visible_game_menu_entries() const {
	Dictionary norm = normalize_game_menu_config(game_menu_config);
	Array entries = norm.get("entries", Array());
	Array out;
	for (int i = 0; i < entries.size(); i++) {
		Dictionary e = entries[i];
		String id = String(e.get("id", ""));
		if (id == "magic" || bool(e.get("reserved", false)) || !bool(e.get("enabled", true))) {
			continue;
		}
		out.push_back(e.duplicate(true));
	}
	return out;
}

Dictionary ClockGearsDataManager::get_game_menu_screen_background(const String &p_screen_id) const {
	Dictionary norm = normalize_game_menu_config(game_menu_config);
	Dictionary bgs = norm.get("backgrounds", Dictionary());
	String k = p_screen_id.strip_edges().to_lower();
	if (bgs.has(k) && bgs[k].get_type() == Variant::DICTIONARY) {
		return Dictionary(bgs[k]).duplicate(true);
	}
	return _cg_default_menu_bg("#0c101ccc");
}

// --- Créditos (9B Tanda 4) ---

static String _cg_normalize_credits_align(const Variant &p_val) {
	String a = String(p_val).strip_edges().to_lower();
	if (a == "left" || a == "izquierda" || a == "right" || a == "derecha") {
		return (a == "izquierda") ? String("left") : ((a == "derecha") ? String("right") : a);
	}
	return "center";
}

static Dictionary _cg_make_credits_text_style(int p_size, const String &p_col, const String &p_align = "") {
	Dictionary s;
	s["font_size"] = CLAMP(p_size, 8, 96);
	s["color"] = Color::html_is_valid(p_col) ? p_col : String("#ffffff");
	s["align"] = p_align;
	return s;
}

static Dictionary _cg_normalize_credits_item(const Dictionary &p_raw) {
	Dictionary item;
	String t = String(p_raw.get("type", "text")).strip_edges().to_lower();
	if (t == "title" || t == "seccion" || t == String::utf8("sección")) {
		t = "section";
	} else if (t == "role" || t == "credit" || t == "rol_nombre") {
		t = "role_name";
	} else if (t == "space" || t == "espacio") {
		t = "spacer";
	} else if (t == "img" || t == "imagen") {
		t = "image";
	} else if (t != "section" && t != "role_name" && t != "text" && t != "image" && t != "spacer") {
		t = "text";
	}
	item["type"] = t;
	if (t == "section" || t == "text") {
		item["text"] = String(p_raw.get("text", p_raw.get("title", "")));
	} else if (t == "role_name") {
		item["role"] = String(p_raw.get("role", ""));
		item["name"] = String(p_raw.get("name", p_raw.get("text", "")));
	} else if (t == "image") {
		item["image_path"] = String(p_raw.get("image_path", p_raw.get("path", ""))).strip_edges();
		item["filter"] = s_normalize_texture_filter_mode(p_raw.get("filter", "nearest"));
		item["max_width"] = CLAMP(double(p_raw.get("max_width", 420.0)), 16.0, 960.0);
		item["max_height"] = CLAMP(double(p_raw.get("max_height", 220.0)), 16.0, 540.0);
	} else if (t == "spacer") {
		item["height"] = CLAMP(double(p_raw.get("height", 32.0)), 4.0, 480.0);
	}
	if (p_raw.has("align") && !String(p_raw["align"]).strip_edges().is_empty()) {
		item["align"] = _cg_normalize_credits_align(p_raw["align"]);
	}
	return item;
}

static bool _cg_credits_item_counts_as_line(const Dictionary &p_item) {
	String t = String(p_item.get("type", "")).strip_edges().to_lower();
	if (t == "section" || t == "text") {
		return !String(p_item.get("text", "")).strip_edges().is_empty();
	}
	if (t == "role_name") {
		return !String(p_item.get("role", "")).strip_edges().is_empty() || !String(p_item.get("name", "")).strip_edges().is_empty();
	}
	if (t == "image") {
		return !String(p_item.get("image_path", "")).strip_edges().is_empty();
	}
	return false;
}

Dictionary ClockGearsDataManager::get_default_credits_config() {
	Dictionary d;
	d["schema_version"] = 1;
	d["background"] = _cg_default_menu_bg("#090b12ff");
	d["bgm_path"] = "";
	d["scroll_speed"] = 60.0;
	d["alignment"] = "center";

	Dictionary styles;
	styles["section"] = _cg_make_credits_text_style(26, "#ffd97dff");
	styles["role"] = _cg_make_credits_text_style(14, "#9fb4d9ff");
	styles["name"] = _cg_make_credits_text_style(20, "#ffffffff");
	styles["text"] = _cg_make_credits_text_style(16, "#dce4f2ff");
	d["styles"] = styles;

	d["items"] = Array();
	return d;
}

Dictionary ClockGearsDataManager::normalize_credits_config(const Dictionary &p_cfg) {
	Dictionary def = get_default_credits_config();
	Dictionary out;
	out["schema_version"] = int(p_cfg.get("schema_version", 1));
	out["background"] = _cg_normalize_menu_bg(p_cfg.get("background", Dictionary()), def["background"]);
	out["bgm_path"] = String(p_cfg.get("bgm_path", "")).strip_edges();
	out["scroll_speed"] = CLAMP(double(p_cfg.get("scroll_speed", 60.0)), 5.0, 600.0);
	out["alignment"] = _cg_normalize_credits_align(p_cfg.get("alignment", p_cfg.get("align", "center")));

	Dictionary def_styles = def["styles"];
	Dictionary raw_styles = (p_cfg.has("styles") && p_cfg["styles"].get_type() == Variant::DICTIONARY) ? Dictionary(p_cfg["styles"]) : Dictionary();
	Dictionary norm_styles;
	static const char *k_style_keys[4] = { "section", "role", "name", "text" };
	for (int i = 0; i < 4; i++) {
		String sk = k_style_keys[i];
		Dictionary ds = def_styles[sk];
		Dictionary rs = (raw_styles.has(sk) && raw_styles[sk].get_type() == Variant::DICTIONARY) ? Dictionary(raw_styles[sk]) : Dictionary();
		int fsz = CLAMP(int(rs.get("font_size", ds["font_size"])), 8, 96);
		String col = String(rs.get("color", ds["color"])).strip_edges();
		if (col.is_empty() || !Color::html_is_valid(col)) {
			col = String(ds["color"]);
		}
		String al = rs.has("align") && !String(rs["align"]).strip_edges().is_empty() ? _cg_normalize_credits_align(rs["align"]) : String("");
		norm_styles[sk] = _cg_make_credits_text_style(fsz, col, al);
	}
	out["styles"] = norm_styles;

	Array norm_items;
	if (p_cfg.has("items") && p_cfg["items"].get_type() == Variant::ARRAY) {
		Array raw_items = p_cfg["items"];
		for (int i = 0; i < raw_items.size(); i++) {
			if (raw_items[i].get_type() == Variant::DICTIONARY) {
				norm_items.push_back(_cg_normalize_credits_item(Dictionary(raw_items[i])));
			} else if (raw_items[i].get_type() == Variant::STRING) {
				Dictionary td;
				td["type"] = "text";
				td["text"] = String(raw_items[i]);
				norm_items.push_back(_cg_normalize_credits_item(td));
			}
		}
	} else if (p_cfg.has("sections") && p_cfg["sections"].get_type() == Variant::ARRAY) {
		Array secs = p_cfg["sections"];
		for (int i = 0; i < secs.size(); i++) {
			if (secs[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary sd = secs[i];
			String stitle = String(sd.get("title", sd.get("text", ""))).strip_edges();
			if (!stitle.is_empty()) {
				Dictionary sitem;
				sitem["type"] = "section";
				sitem["text"] = stitle;
				norm_items.push_back(sitem);
			}
			Array sub = sd.has("items") && sd["items"].get_type() == Variant::ARRAY ? Array(sd["items"])
					: (sd.has("lines") && sd["lines"].get_type() == Variant::ARRAY ? Array(sd["lines"]) : Array());
			for (int j = 0; j < sub.size(); j++) {
				if (sub[j].get_type() == Variant::DICTIONARY) {
					norm_items.push_back(_cg_normalize_credits_item(Dictionary(sub[j])));
				}
			}
		}
	}
	out["items"] = norm_items;
	return out;
}

void ClockGearsDataManager::set_credits_config(const Dictionary &p_cfg) {
	credits_config = normalize_credits_config(p_cfg);
	title_menu_config["credits_configured"] = has_credits_lines();
	dirty = true;
	emit_signal("data_changed", "credits", 0);
}

bool ClockGearsDataManager::has_credits_lines() const {
	Dictionary norm = normalize_credits_config(credits_config);
	Array items = norm.get("items", Array());
	for (int i = 0; i < items.size(); i++) {
		if (items[i].get_type() == Variant::DICTIONARY && _cg_credits_item_counts_as_line(Dictionary(items[i]))) {
			return true;
		}
	}
	return false;
}

Dictionary ClockGearsDataManager::get_default_game_over_config() {
	Dictionary d;
	d["schema_version"] = 1;
	d["background"] = _cg_default_menu_bg("#0c070cff");
	d["title_text"] = "GAME OVER";
	d["title_color"] = "#e84a5fff";
	d["title_font_size"] = 48;
	d["bgm_path"] = "";
	d["bgm_volume_db"] = 0.0;
	d["retry_enabled"] = true;
	d["retry_label"] = "Reintentar";
	d["title_enabled"] = true;
	d["title_label"] = String::utf8("Volver al menú");
	d["window_style_id"] = "default";
	return d;
}

Dictionary ClockGearsDataManager::normalize_game_over_config(const Dictionary &p_cfg) {
	Dictionary def = get_default_game_over_config();
	Dictionary out;
	out["schema_version"] = int(p_cfg.get("schema_version", 1));
	out["background"] = _cg_normalize_menu_bg(p_cfg.get("background", Dictionary()), def["background"]);
	String t = String(p_cfg.get("title_text", def["title_text"])).strip_edges();
	out["title_text"] = t.is_empty() ? String("GAME OVER") : t;
	String tc = String(p_cfg.get("title_color", def["title_color"])).strip_edges();
	if (tc.is_empty() || !Color::html_is_valid(tc)) {
		tc = String(def["title_color"]);
	}
	out["title_color"] = tc;
	out["title_font_size"] = CLAMP(int(p_cfg.get("title_font_size", def["title_font_size"])), 16, 128);
	out["bgm_path"] = String(p_cfg.get("bgm_path", "")).strip_edges();
	out["bgm_volume_db"] = CLAMP(double(p_cfg.get("bgm_volume_db", 0.0)), -40.0, 6.0);
	out["retry_enabled"] = p_cfg.has("retry_enabled") ? bool(p_cfg["retry_enabled"]) : true;
	String rl = String(p_cfg.get("retry_label", def["retry_label"])).strip_edges();
	out["retry_label"] = rl.is_empty() ? String("Reintentar") : rl;
	out["title_enabled"] = p_cfg.has("title_enabled") ? bool(p_cfg["title_enabled"]) : true;
	String tl = String(p_cfg.get("title_label", def["title_label"])).strip_edges();
	out["title_label"] = tl.is_empty() ? String::utf8("Volver al menú") : tl;
	String wsid = String(p_cfg.get("window_style_id", "default")).strip_edges().to_lower();
	out["window_style_id"] = wsid.is_empty() ? String("default") : wsid;
	return out;
}

void ClockGearsDataManager::set_game_over_config(const Dictionary &p_cfg) {
	game_over_config = normalize_game_over_config(p_cfg);
	dirty = true;
	emit_signal("data_changed", "game_over", 0);
}

void ClockGearsDataManager::set_runtime_window_style_override(const String &p_window_type_id, const String &p_style_id) {
	String wt = p_window_type_id.strip_edges().to_lower();
	if (wt.is_empty()) {
		return;
	}
	String sid = p_style_id.strip_edges().to_lower();
	if (sid.is_empty()) {
		runtime_window_style_overrides.erase(wt);
	} else {
		runtime_window_style_overrides[wt] = sid;
	}
	_invalidate_window_styles_cache();
	emit_signal("runtime_state_changed", "window_style", wt);
}

void ClockGearsDataManager::clear_runtime_window_style_overrides() {
	runtime_window_style_overrides.clear();
	_invalidate_window_styles_cache();
	emit_signal("runtime_state_changed", "window_style", String());
}

namespace {
static String _cg_norm_hud_color(const Variant &p_val, const String &p_default) {
	if (p_val.get_type() == Variant::COLOR) {
		return "#" + Color(p_val).to_html(true).to_upper();
	}
	String s = String(p_val).strip_edges();
	if (!s.is_empty() && Color::html_is_valid(s)) {
		return "#" + Color::html(s).to_html(true).to_upper();
	}
	return p_default;
}

static String _cg_norm_hud_anchor(const Variant &p_val) {
	String a = String(p_val).strip_edges().to_lower();
	if (a == "top_left" || a == "top_center" || a == "top_right" ||
			a == "middle_left" || a == "center" || a == "middle_right" ||
			a == "bottom_left" || a == "bottom_center" || a == "bottom_right") {
		return a;
	}
	return "top_left";
}

static String _cg_norm_hud_type(const Variant &p_val) {
	String t = String(p_val).strip_edges().to_lower();
	if (t == "bar" || t == "hearts" || t == "currency" || t == "item" || t == "text" || t == "image" ||
			t == "variable" || t == "frame" || t == "button") {
		return t;
	}
	if (t == "marco") {
		return "frame";
	}
	if (t == "boton" || t == String::utf8("botón")) {
		return "button";
	}
	return "bar";
}

static String _cg_norm_ui_elem_type(const Variant &p_val) {
	String t = String(p_val).strip_edges().to_lower();
	if (t == "image" || t == "text" || t == "button" || t == "frame" ||
			t == "bar" || t == "hearts" || t == "currency" || t == "item" || t == "variable" ||
			t == "container" || t == "radial" || t == "widget" || t == "folder") {
		return t;
	}
	if (t == "carpeta") {
		return "folder";
	}
	if (t == "imagen") {
		return "image";
	}
	if (t == "texto") {
		return "text";
	}
	if (t == "boton" || t == String::utf8("botón")) {
		return "button";
	}
	if (t == "marco") {
		return "frame";
	}
	if (t == "barra") {
		return "bar";
	}
	if (t == "corazones") {
		return "hearts";
	}
	if (t == "monedas") {
		return "currency";
	}
	if (t == "objeto" || t == "contador_objetos") {
		return "item";
	}
	if (t == "contenedor") {
		return "container";
	}
	if (t == "lista" || t == "lista/widget") {
		return "widget";
	}
	return "text";
}

static String _cg_slugify_menu_id(const String &p_raw) {
	String s = p_raw.strip_edges().to_lower();
	// Plegar vocales acentuadas / ñ comunes al slug del ID (el nombre visible conserva UTF-8 íntegro)
	s = s.replace(String::utf8("á"), "a").replace(String::utf8("é"), "e").replace(String::utf8("í"), "i")
			.replace(String::utf8("ó"), "o").replace(String::utf8("ú"), "u").replace(String::utf8("ü"), "u")
			.replace(String::utf8("ñ"), "n");
	String out;
	bool last_us = false;
	for (int i = 0; i < s.length(); i++) {
		char32_t c = s[i];
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
			out += c;
			last_us = false;
		} else if ((c == '_' || c == '-' || c == ' ') && !last_us && !out.is_empty()) {
			out += '_';
			last_us = true;
		}
	}
	while (out.ends_with("_")) {
		out = out.substr(0, out.length() - 1);
	}
	return out;
}

static String _cg_norm_ui_screen_id(const String &p_raw) {
	String s = p_raw.strip_edges().to_lower();
	if (s.is_empty() || s == "title" || s == "titulo" || s == String::utf8("título")) {
		return "title";
	}
	if (s == "game_menu_main" || s == "game_menu" || s == "menu" || s == "main") {
		return "game_menu_main";
	}
	if (s == "game_menu_items" || s == "items" || s == "objetos") {
		return "game_menu_items";
	}
	if (s == "game_menu_equip" || s == "equip" || s == "equipo") {
		return "game_menu_equip";
	}
	if (s == "game_menu_status" || s == "status" || s == "estado") {
		return "game_menu_status";
	}
	if (s == "game_menu_save" || s == "save" || s == "guardar") {
		return "game_menu_save";
	}
	if (s == "game_menu_options" || s == "options" || s == "opciones") {
		return "game_menu_options";
	}
	if (s == "credits" || s == "creditos" || s == String::utf8("créditos")) {
		return "credits";
	}
	if (s == "hud") {
		return "hud";
	}
	if (s == "game_over" || s == "gameover" || s == "fin_del_juego") {
		return "game_over";
	}
	if (s == "messages_battle" || s == "mensajes_batalla" || s == "mensajes y batalla" || s == "battle_messages" || s == "messages") {
		return "messages_battle";
	}
	String slug = _cg_slugify_menu_id(s);
	return slug.is_empty() ? String("menu_custom") : slug;
}

static bool _cg_is_system_menu_id(const String &p_id) {
	return p_id == "title" || p_id == "game_menu_main" || p_id == "game_menu_items" ||
			p_id == "game_menu_equip" || p_id == "game_menu_status" ||
			p_id == "game_menu_save" || p_id == "game_menu_options" ||
			p_id == "credits" || p_id == "hud" || p_id == "game_over" ||
			p_id == "messages_battle";
}

static String _cg_default_system_menu_name(const String &p_id) {
	if (p_id == "title") return String::utf8("Menú de inicio");
	if (p_id == "game_menu_main") return String::utf8("Menú del juego: Principal");
	if (p_id == "game_menu_items") return "Inventario";
	if (p_id == "game_menu_equip") return "Equipo";
	if (p_id == "game_menu_status") return "Estado";
	if (p_id == "game_menu_save") return "Partidas";
	if (p_id == "game_menu_options") return "Opciones";
	if (p_id == "credits") return String::utf8("Créditos");
	if (p_id == "hud") return "HUD";
	if (p_id == "game_over") return "Game Over";
	if (p_id == "messages_battle") return "Mensajes y batalla";
	return p_id;
}

static String _cg_default_system_menu_win_type(const String &p_id) {
	if (p_id == "title") return "title_buttons";
	if (p_id == "game_menu_save") return "save_list";
	if (p_id == "game_menu_options") return "options";
	if (p_id == "hud") return "hud";
	if (p_id == "messages_battle") return "dialog";
	return "game_menu";
}

static String _cg_norm_color_rgba(const Variant &p_val, const String &p_fallback) {
	if (p_val.get_type() == Variant::COLOR) {
		return "#" + Color(p_val).to_html(true);
	}
	String s = String(p_val).strip_edges();
	if (!s.is_empty() && Color::html_is_valid(s)) {
		return "#" + Color::html(s).to_html(true);
	}
	return "#" + Color::html(p_fallback).to_html(true);
}

static String _cg_norm_shape_type(const Variant &p_val) {
	String s = String(p_val).strip_edges().to_lower();
	if (s == "rect" || s == "recto" || s == "square") return "rect";
	if (s == "rounded" || s == "redondeado") return "rounded";
	if (s == "pill" || s == "pildora" || s == String::utf8("píldora") || s == "capsule") return "pill";
	if (s == "circle" || s == "circulo" || s == String::utf8("círculo")) return "circle";
	if (s == "ellipse" || s == "elipse" || s == "oval") return "ellipse";
	if (s == "image" || s == "imagen" || s == "nine_slice" || s == "9slice") return "image";
	return "";
}

static String _cg_norm_trigger_context(const Variant &p_val) {
	String c = String(p_val).strip_edges().to_lower();
	if (c == "map_and_battle" || c == "mapa_y_combate" || c == "combat" || c == "battle") {
		return "map_and_battle";
	}
	if (c == "any" || c == "cualquier" || c == "all" || c == "always" || c == "siempre") {
		return "any";
	}
	return "map";
}

static String _cg_norm_layout_mode(const Variant &p_val) {
	String m = String(p_val).strip_edges().to_lower();
	if (m == "vertical_list" || m == "vertical" || m == "list_v" || m == "lista_vertical") return "vertical_list";
	if (m == "horizontal_list" || m == "horizontal" || m == "list_h" || m == "lista_horizontal") return "horizontal_list";
	if (m == "grid" || m == "cuadricula" || m == String::utf8("cuadrícula")) return "grid";
	if (m == "tabs" || m == "pestanas" || m == String::utf8("pestañas")) return "tabs";
	if (m == "carousel" || m == "carrusel") return "carousel";
	if (m == "radial" || m == "ring" || m == "anillo") return "radial";
	return "free";
}

static Array _cg_default_game_menu_triggers() {
	Array out;
	Dictionary t_esc;
	t_esc["kind"] = "key";
	t_esc["keycode"] = (int64_t)Key::ESCAPE;
	t_esc["key_name"] = "Escape";
	out.push_back(t_esc);

	Dictionary t_m;
	t_m["kind"] = "key";
	t_m["keycode"] = (int64_t)Key::M;
	t_m["key_name"] = "M";
	out.push_back(t_m);

	Dictionary t_act1;
	t_act1["kind"] = "action";
	t_act1["action_name"] = "game_menu";
	out.push_back(t_act1);

	Dictionary t_act2;
	t_act2["kind"] = "action";
	t_act2["action_name"] = "ui_menu";
	out.push_back(t_act2);
	return out;
}

static Dictionary _cg_norm_button_action_step(const Dictionary &p_step) {
	Dictionary out;
	String t = String(p_step.get("type", p_step.get("action", ""))).strip_edges().to_lower();
	if (t == "close_menu") t = "close";
	if (t == "return_to_title") t = "to_title";
	if (t == "quit_game" || t == "exit") t = "quit";
	if (t == "items" || t == "equip" || t == "status" || t == "save" || t == "options" || t == "credits") {
		out["type"] = "open_system_screen";
		out["screen_id"] = t;
		return out;
	}
	if (t == "open_system_screen") {
		out["type"] = "open_system_screen";
		out["screen_id"] = String(p_step.get("screen_id", p_step.get("target", "items"))).strip_edges().to_lower();
		return out;
	}
	if (t == "open_menu") {
		out["type"] = "open_menu";
		out["menu_id"] = _cg_norm_ui_screen_id(String(p_step.get("menu_id", p_step.get("target", "game_menu_main"))));
		out["push_history"] = p_step.has("push_history") ? bool(p_step["push_history"]) : true;
		return out;
	}
	if (t == "common_event" || t == "run_common_event") {
		out["type"] = "common_event";
		out["common_event_id"] = MAX(1, int(p_step.get("common_event_id", p_step.get("id", 1))));
		return out;
	}
	if (t == "set_switch" || t == "switch") {
		out["type"] = "set_switch";
		out["switch_name"] = String(p_step.get("switch_name", p_step.get("switch", ""))).strip_edges();
		Variant v = p_step.get("value", true);
		if (v.get_type() == Variant::STRING && String(v).strip_edges().to_lower() == "toggle") {
			out["value"] = "toggle";
		} else {
			out["value"] = bool(v);
		}
		return out;
	}
	if (t == "set_variable" || t == "variable") {
		out["type"] = "set_variable";
		out["variable_name"] = String(p_step.get("variable_name", p_step.get("var_name", p_step.get("variable", "")))).strip_edges();
		String op = String(p_step.get("op", "set")).strip_edges().to_lower();
		if (op == "+" || op == "+=") op = "add";
		else if (op == "-" || op == "-=") op = "sub";
		else if (op != "add" && op != "sub" && op != "*" && op != "/" && op != "%") op = "set";
		out["op"] = op;
		out["value"] = int(p_step.get("value", 0));
		return out;
	}
	if (t == "play_sound" || t == "play_se") {
		out["type"] = "play_sound";
		out["sound_path"] = String(p_step.get("sound_path", p_step.get("sound", p_step.get("path", "")))).strip_edges();
		out["volume_db"] = float(p_step.get("volume_db", 0.0));
		return out;
	}
	if (t == "play_music" || t == "play_bgm") {
		out["type"] = "play_music";
		out["sound_path"] = String(p_step.get("sound_path", p_step.get("music_path", p_step.get("path", "")))).strip_edges();
		out["volume_db"] = float(p_step.get("volume_db", 0.0));
		return out;
	}
	if (t == "hud_visibility" || t == "show_hide_hud") {
		out["type"] = "hud_visibility";
		Variant vis = p_step.get("visible", true);
		if (vis.get_type() == Variant::STRING && String(vis).strip_edges().to_lower() == "toggle") {
			out["visible"] = "toggle";
		} else {
			out["visible"] = bool(vis);
		}
		out["scope"] = String(p_step.get("scope", "all")).strip_edges().to_lower();
		out["widget_id"] = String(p_step.get("widget_id", p_step.get("widget", p_step.get("widget_name", "")))).strip_edges();
		return out;
	}
	if (t == "load_game" || t == "save_game") {
		out["type"] = t;
		out["slot"] = MAX(0, int(p_step.get("slot", 0)));
		return out;
	}
	if (t == "close" || t == "back" || t == "new_game" || t == "continue" || t == "to_title" || t == "quit") {
		out["type"] = t;
		return out;
	}
	out["type"] = t.is_empty() ? String("close") : t;
	return out;
}

static String _cg_norm_ui_data_binding(const Variant &p_val) {
	String b = String(p_val).strip_edges().to_lower();
	if (b == "project_title" || b == "currency_name" || b == "gold" || b == "item_qty" ||
			b == "variable" || b == "player_hp" || b == "player_mp" ||
			b == "actor_name" || b == "actor_level" || b == "playtime") {
		return b;
	}
	return "none";
}

static void _cg_normalize_hud_core_fields(const Dictionary &p_in, const Dictionary &p_def, const String &p_wtype, Dictionary &r_out) {
	if (p_wtype == "bar") {
		String vs = String(p_in.get("value_source", p_def.get("value_source", "hp"))).strip_edges().to_lower();
		if (vs != "hp" && vs != "mp" && vs != "exp" && vs != "variable") {
			vs = "hp";
		}
		r_out["value_source"] = vs;
		r_out["variable_name"] = String(p_in.get("variable_name", p_def.get("variable_name", ""))).strip_edges();
		r_out["variable_max"] = CLAMP(int(p_in.get("variable_max", p_def.get("variable_max", 100))), 1, 99999999);
		r_out["bg_color"] = _cg_norm_hud_color(p_in.get("bg_color", p_def.get("bg_color", "#1A1E2ECC")), String(p_def.get("bg_color", "#1A1E2ECC")));
		String default_fill = (vs == "mp") ? String("#3B82F6FF") : ((vs == "exp") ? String("#EAB308FF") : String(p_def.get("fill_color", "#D93B3BFF")));
		r_out["fill_color"] = _cg_norm_hud_color(p_in.get("fill_color", default_fill), default_fill);
		r_out["border_color"] = _cg_norm_hud_color(p_in.get("border_color", p_def.get("border_color", "#E2E8F0FF")), String(p_def.get("border_color", "#E2E8F0FF")));
		r_out["bg_image_path"] = String(p_in.get("bg_image_path", "")).strip_edges();
		r_out["fill_image_path"] = String(p_in.get("fill_image_path", "")).strip_edges();
		r_out["show_numbers"] = p_in.has("show_numbers") ? bool(p_in["show_numbers"]) : bool(p_def.get("show_numbers", true));
		r_out["label_text"] = String(p_in.get("label_text", ""));
		r_out["font_size"] = CLAMP(int(p_in.get("font_size", p_def.get("font_size", 13))), 8, 64);
		r_out["font_color"] = _cg_norm_hud_color(p_in.get("font_color", p_def.get("font_color", "#FFFFFFFF")), String(p_def.get("font_color", "#FFFFFFFF")));
	} else if (p_wtype == "hearts") {
		r_out["hp_per_heart"] = CLAMP(int(p_in.get("hp_per_heart", p_def.get("hp_per_heart", 20))), 1, 99999);
		r_out["full_image_path"] = String(p_in.get("full_image_path", "")).strip_edges();
		r_out["half_image_path"] = String(p_in.get("half_image_path", "")).strip_edges();
		r_out["empty_image_path"] = String(p_in.get("empty_image_path", "")).strip_edges();
		r_out["heart_size"] = CLAMP(int(p_in.get("heart_size", p_def.get("heart_size", 20))), 8, 128);
		r_out["spacing"] = CLAMP(int(p_in.get("spacing", p_def.get("spacing", 4))), 0, 64);
		r_out["max_hearts_per_row"] = CLAMP(int(p_in.get("max_hearts_per_row", p_def.get("max_hearts_per_row", 10))), 1, 50);
		r_out["max_rows"] = CLAMP(int(p_in.get("max_rows", p_def.get("max_rows", 2))), 1, 20);
		r_out["show_background"] = p_in.has("show_background") ? bool(p_in["show_background"]) : false;
		r_out["bg_color"] = String(p_in.get("bg_color", "#1a1a1a")).strip_edges();
		r_out["bg_opacity"] = CLAMP(int(p_in.get("bg_opacity", 60)), 0, 100);
		// Datos antiguos sin la clave conservan el modo manual (ancho/alto guardados).
		r_out["auto_size"] = p_in.has("auto_size") ? bool(p_in["auto_size"]) : false;
	} else if (p_wtype == "currency") {
		r_out["icon_path"] = String(p_in.get("icon_path", "")).strip_edges();
		r_out["show_icon"] = p_in.has("show_icon") ? bool(p_in["show_icon"]) : true;
		r_out["show_currency_name"] = p_in.has("show_currency_name") ? bool(p_in["show_currency_name"]) : true;
		r_out["font_size"] = CLAMP(int(p_in.get("font_size", p_def.get("font_size", 15))), 8, 64);
		r_out["font_color"] = _cg_norm_hud_color(p_in.get("font_color", p_def.get("font_color", "#FDE047FF")), String(p_def.get("font_color", "#FDE047FF")));
	} else if (p_wtype == "item") {
		r_out["item_key"] = String(p_in.get("item_key", "")).strip_edges();
		r_out["icon_path"] = String(p_in.get("icon_path", "")).strip_edges();
		r_out["hide_when_zero"] = p_in.has("hide_when_zero") ? bool(p_in["hide_when_zero"]) : false;
		r_out["show_name"] = p_in.has("show_name") ? bool(p_in["show_name"]) : false;
		r_out["font_size"] = CLAMP(int(p_in.get("font_size", p_def.get("font_size", 15))), 8, 64);
		r_out["font_color"] = _cg_norm_hud_color(p_in.get("font_color", p_def.get("font_color", "#FFFFFFFF")), String(p_def.get("font_color", "#FFFFFFFF")));
	} else if (p_wtype == "text" || p_wtype == "variable") {
		String tm = String(p_in.get("text_mode", p_wtype == "variable" ? "variable" : p_def.get("text_mode", "fixed"))).strip_edges().to_lower();
		if (p_wtype == "variable") {
			tm = "variable";
		} else if (tm != "variable") {
			tm = "fixed";
		}
		r_out["text_mode"] = tm;
		r_out["text"] = String(p_in.get("text", p_def.get("text", p_wtype == "variable" ? "" : "Texto")));
		r_out["prefix"] = String(p_in.get("prefix", p_def.get("prefix", "")));
		r_out["variable_name"] = String(p_in.get("variable_name", "")).strip_edges();
		r_out["font_size"] = CLAMP(int(p_in.get("font_size", p_def.get("font_size", 16))), 8, 96);
		r_out["font_color"] = _cg_norm_hud_color(p_in.get("font_color", p_def.get("font_color", "#FFFFFFFF")), String(p_def.get("font_color", "#FFFFFFFF")));
		String al = String(p_in.get("align", p_def.get("align", "left"))).strip_edges().to_lower();
		if (al != "center" && al != "right") {
			al = "left";
		}
		r_out["align"] = al;
	} else if (p_wtype == "button") {
		r_out["text"] = String(p_in.get("text", p_def.get("text", String::utf8("Botón"))));
		r_out["action"] = String(p_in.get("action", p_def.get("action", ""))).strip_edges().to_lower();
		r_out["focus_order"] = CLAMP(int(p_in.get("focus_order", p_def.get("focus_order", 0))), 0, 9999);
		r_out["font_size"] = CLAMP(int(p_in.get("font_size", p_def.get("font_size", 16))), 8, 64);
		r_out["font_color"] = _cg_norm_hud_color(p_in.get("font_color", p_def.get("font_color", "#FFFFFFFF")), String(p_def.get("font_color", "#FFFFFFFF")));
		r_out["image_path"] = String(p_in.get("image_path", "")).strip_edges();
		String bsm = String(p_in.get("scale_mode", "stretch")).strip_edges().to_lower();
		if (bsm != "fit" && bsm != "fill" && bsm != "tile" && bsm != "native" && bsm != "nine_slice" && bsm != "integer") {
			bsm = "stretch";
		}
		r_out["scale_mode"] = bsm;
		r_out["nine_slice_margin"] = CLAMP(int(p_in.get("nine_slice_margin", 12)), 0, 128);
		r_out["disabled"] = p_in.has("disabled") ? bool(p_in["disabled"]) : false;
	} else if (p_wtype == "frame") {
		r_out["opacity"] = CLAMP(double(p_in.get("opacity", p_def.get("opacity", 1.0))), 0.0, 1.0);
		r_out["image_path"] = String(p_in.get("image_path", "")).strip_edges();
		String fsm = String(p_in.get("scale_mode", "stretch")).strip_edges().to_lower();
		if (fsm != "fit" && fsm != "fill" && fsm != "tile" && fsm != "native" && fsm != "nine_slice" && fsm != "integer") {
			fsm = "stretch";
		}
		r_out["scale_mode"] = fsm;
		r_out["nine_slice_margin"] = CLAMP(int(p_in.get("nine_slice_margin", 12)), 0, 128);
	} else {
		r_out["image_path"] = String(p_in.get("image_path", "")).strip_edges();
		String sm = String(p_in.get("scale_mode", p_def.get("scale_mode", "fit"))).strip_edges().to_lower();
		if (sm != "stretch" && sm != "tile" && sm != "native" && sm != "fill" && sm != "nine_slice" && sm != "integer") {
			sm = "fit";
		}
		r_out["scale_mode"] = sm;
		r_out["nine_slice_margin"] = CLAMP(int(p_in.get("nine_slice_margin", 12)), 0, 128);
		r_out["opacity"] = CLAMP(double(p_in.get("opacity", p_def.get("opacity", 1.0))), 0.0, 1.0);
	}
}

static String _cg_slugify_hud_id(const String &p_raw) {
	String lower = p_raw.strip_edges().to_lower();
	String out;
	bool last_us = false;
	for (int i = 0; i < lower.length(); i++) {
		char32_t c = lower[i];
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
			out += c;
			last_us = false;
		} else if (!last_us && !out.is_empty()) {
			out += '_';
			last_us = true;
		}
	}
	while (out.ends_with("_")) {
		out = out.substr(0, out.length() - 1);
	}
	return out;
}
} // namespace

Array ClockGearsDataManager::get_hud_anchor_ids() {
	Array out;
	out.push_back("top_left");
	out.push_back("top_center");
	out.push_back("top_right");
	out.push_back("middle_left");
	out.push_back("center");
	out.push_back("middle_right");
	out.push_back("bottom_left");
	out.push_back("bottom_center");
	out.push_back("bottom_right");
	return out;
}

Dictionary ClockGearsDataManager::make_default_hud_widget(const String &p_type) {
	String wtype = _cg_norm_hud_type(p_type);
	Dictionary w;
	w["id"] = "";
	w["type"] = wtype;
	w["anchor"] = "top_left";
	w["offset_x"] = 16;
	w["offset_y"] = 16;
	w["layer_order"] = 0;
	w["window_style_id"] = "";
	w["visibility_mode"] = "always";
	w["visibility_switch"] = "";
	w["hide_during_messages_and_menus"] = true;
	w["pixel_art"] = true;

	if (wtype == "bar") {
		w["name"] = "Barra";
		w["width"] = 220;
		w["height"] = 34;
		w["value_source"] = "hp";
		w["variable_name"] = "";
		w["variable_max"] = 100;
		w["bg_color"] = "#1A1E2ECC";
		w["fill_color"] = "#D93B3BFF";
		w["border_color"] = "#E2E8F0FF";
		w["bg_image_path"] = "";
		w["fill_image_path"] = "";
		w["show_numbers"] = true;
		w["label_text"] = "";
		w["font_size"] = 13;
		w["font_color"] = "#FFFFFFFF";
	} else if (wtype == "hearts") {
		w["name"] = "Corazones";
		w["width"] = 220;
		w["height"] = 48;
		w["hp_per_heart"] = 20;
		w["full_image_path"] = "";
		w["half_image_path"] = "";
		w["empty_image_path"] = "";
		w["heart_size"] = 20;
		w["spacing"] = 4;
		w["max_hearts_per_row"] = 10;
		w["max_rows"] = 2;
		w["show_background"] = false;
		w["bg_color"] = "#1a1a1a";
		w["bg_opacity"] = 60;
		w["auto_size"] = true;
	} else if (wtype == "currency") {
		w["name"] = "Monedas";
		w["anchor"] = "top_right";
		w["offset_x"] = -16;
		w["offset_y"] = 16;
		w["width"] = 160;
		w["height"] = 36;
		w["icon_path"] = "";
		w["show_icon"] = true;
		w["show_currency_name"] = true;
		w["font_size"] = 15;
		w["font_color"] = "#FDE047FF";
	} else if (wtype == "item") {
		w["name"] = "Objeto";
		w["anchor"] = "bottom_left";
		w["offset_x"] = 16;
		w["offset_y"] = -16;
		w["width"] = 140;
		w["height"] = 38;
		w["item_key"] = "";
		w["icon_path"] = "";
		w["hide_when_zero"] = false;
		w["show_name"] = false;
		w["font_size"] = 15;
		w["font_color"] = "#FFFFFFFF";
	} else if (wtype == "text" || wtype == "variable") {
		w["name"] = (wtype == "variable") ? String("Variable") : String("Texto");
		w["width"] = 200;
		w["height"] = 36;
		w["text_mode"] = (wtype == "variable") ? "variable" : "fixed";
		w["text"] = (wtype == "variable") ? "" : "Texto";
		w["prefix"] = "";
		w["variable_name"] = "";
		w["font_size"] = 16;
		w["font_color"] = "#FFFFFFFF";
		w["align"] = "left";
	} else if (wtype == "button") {
		w["name"] = String::utf8("Botón");
		w["width"] = 180;
		w["height"] = 40;
		w["text"] = String::utf8("Botón");
		w["action"] = "";
		w["focus_order"] = 0;
		w["font_size"] = 16;
		w["font_color"] = "#FFFFFFFF";
		w["image_path"] = "";
		w["disabled"] = false;
	} else if (wtype == "frame") {
		w["name"] = "Marco";
		w["width"] = 260;
		w["height"] = 180;
		w["opacity"] = 1.0;
	} else {
		w["name"] = "Imagen";
		w["width"] = 96;
		w["height"] = 96;
		w["image_path"] = "";
		w["scale_mode"] = "fit";
		w["opacity"] = 1.0;
	}
	return w;
}

Dictionary ClockGearsDataManager::normalize_hud_widget(const Dictionary &p_widget, int p_index_hint) {
	String wtype = _cg_norm_hud_type(p_widget.get("type", "bar"));
	Dictionary def = make_default_hud_widget(wtype);
	Dictionary out;

	String raw_name = String(p_widget.get("name", def["name"])).strip_edges();
	if (raw_name.is_empty()) {
		raw_name = String(def["name"]) + " " + itos(MAX(1, p_index_hint));
	}
	String raw_id = _cg_slugify_hud_id(String(p_widget.get("id", "")));
	if (raw_id.is_empty()) {
		raw_id = _cg_slugify_hud_id(raw_name);
	}
	if (raw_id.is_empty()) {
		raw_id = "widget_" + itos(MAX(1, p_index_hint));
	}

	out["id"] = raw_id;
	out["name"] = raw_name;
	out["type"] = wtype;
	String raw_anchor = String(p_widget.get("anchor", def["anchor"])).strip_edges().to_lower();
	out["anchor"] = (raw_anchor == "free") ? String("free") : _cg_norm_hud_anchor(raw_anchor);
	out["offset_x"] = CLAMP(int(p_widget.get("offset_x", def["offset_x"])), -960, 960);
	out["offset_y"] = CLAMP(int(p_widget.get("offset_y", def["offset_y"])), -540, 540);
	out["width"] = CLAMP(int(p_widget.get("width", def["width"])), 16, 960);
	out["height"] = CLAMP(int(p_widget.get("height", def["height"])), 16, 540);
	if (raw_anchor == "free") {
		// Modo libre: posición absoluta en píxeles enteros sobre 960x540, sin salirse de la pantalla.
		out["offset_x"] = CLAMP(int(out["offset_x"]), 0, 960 - int(out["width"]));
		out["offset_y"] = CLAMP(int(out["offset_y"]), 0, 540 - int(out["height"]));
	}
	out["layer_order"] = CLAMP(int(p_widget.get("layer_order", def["layer_order"])), -100, 1000);

	String ws = String(p_widget.get("window_style_id", def["window_style_id"])).strip_edges().to_lower();
	if (ws == "none") {
		ws = "";
	}
	out["window_style_id"] = ws;

	String vmode = String(p_widget.get("visibility_mode", def["visibility_mode"])).strip_edges().to_lower();
	if (vmode != "switch") {
		vmode = "always";
	}
	out["visibility_mode"] = vmode;
	out["visibility_switch"] = String(p_widget.get("visibility_switch", def["visibility_switch"])).strip_edges();
	out["hide_during_messages_and_menus"] = p_widget.has("hide_during_messages_and_menus") ? bool(p_widget["hide_during_messages_and_menus"]) : true;
	out["pixel_art"] = p_widget.has("pixel_art") ? bool(p_widget["pixel_art"]) : bool(def["pixel_art"]);
	out["visible"] = p_widget.has("visible") ? bool(p_widget["visible"]) : true;
	out["data_binding"] = _cg_norm_ui_data_binding(p_widget.get("data_binding", (wtype == "variable") ? Variant("variable") : Variant("none")));
	out["actor_id"] = MAX(0, int(p_widget.get("actor_id", 0)));

	_cg_normalize_hud_core_fields(p_widget, def, wtype, out);
	return out;
}

Dictionary ClockGearsDataManager::get_default_hud_config() {
	Dictionary out;
	out["schema_version"] = 1;
	out["widgets"] = Array();
	return out;
}

Dictionary ClockGearsDataManager::normalize_hud_config(const Dictionary &p_cfg) {
	Dictionary out;
	out["schema_version"] = int(p_cfg.get("schema_version", 1));
	Array norm_widgets;
	HashSet<String> seen_ids;
	if (p_cfg.has("widgets") && p_cfg["widgets"].get_type() == Variant::ARRAY) {
		Array raw = p_cfg["widgets"];
		for (int i = 0; i < raw.size(); i++) {
			if (raw[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary nw = normalize_hud_widget(Dictionary(raw[i]), i + 1);
			String base_id = String(nw["id"]);
			String unique_id = base_id;
			int suffix = 2;
			while (seen_ids.has(unique_id)) {
				unique_id = base_id + "_" + itos(suffix++);
			}
			seen_ids.insert(unique_id);
			nw["id"] = unique_id;
			norm_widgets.push_back(nw);
		}
	}
	out["widgets"] = norm_widgets;
	return out;
}

void ClockGearsDataManager::set_hud_config(const Dictionary &p_cfg) {
	hud_config = normalize_hud_config(p_cfg);
	dirty = true;
	emit_signal("data_changed", "hud", 0);
	emit_signal("runtime_state_changed", "hud", String());
}

Array ClockGearsDataManager::get_hud_widgets() const {
	Dictionary norm = normalize_hud_config(hud_config);
	return Array(norm.get("widgets", Array())).duplicate(true);
}

Dictionary ClockGearsDataManager::get_hud_widget(const String &p_widget_id) const {
	String target = p_widget_id.strip_edges().to_lower();
	Array widgets = get_hud_widgets();
	for (int i = 0; i < widgets.size(); i++) {
		Dictionary w = widgets[i];
		if (String(w.get("id", "")).to_lower() == target || String(w.get("name", "")).strip_edges().to_lower() == target) {
			return w;
		}
	}
	return Dictionary();
}

String ClockGearsDataManager::create_hud_widget(const Dictionary &p_widget) {
	Dictionary norm = normalize_hud_config(hud_config);
	Array widgets = Array(norm.get("widgets", Array())).duplicate(true);
	Dictionary nw = normalize_hud_widget(p_widget, widgets.size() + 1);
	String base_id = String(nw["id"]);
	String unique_id = base_id;
	int suffix = 2;
	HashSet<String> existing;
	for (int i = 0; i < widgets.size(); i++) {
		existing.insert(String(Dictionary(widgets[i]).get("id", "")));
	}
	while (existing.has(unique_id)) {
		unique_id = base_id + "_" + itos(suffix++);
	}
	nw["id"] = unique_id;
	widgets.push_back(nw);
	norm["widgets"] = widgets;
	set_hud_config(norm);
	return unique_id;
}

bool ClockGearsDataManager::update_hud_widget(const String &p_widget_id, const Dictionary &p_widget) {
	String target = p_widget_id.strip_edges().to_lower();
	Dictionary norm = normalize_hud_config(hud_config);
	Array widgets = Array(norm.get("widgets", Array())).duplicate(true);
	for (int i = 0; i < widgets.size(); i++) {
		Dictionary cur = widgets[i];
		if (String(cur.get("id", "")).to_lower() == target) {
			Dictionary merged = p_widget.duplicate(true);
			if (!merged.has("id") || String(merged["id"]).strip_edges().is_empty()) {
				merged["id"] = cur["id"];
			}
			Dictionary nw = normalize_hud_widget(merged, i + 1);
			nw["id"] = cur["id"]; // Conservar ID estable en ediciones desde UI
			widgets[i] = nw;
			norm["widgets"] = widgets;
			set_hud_config(norm);
			return true;
		}
	}
	return false;
}

bool ClockGearsDataManager::delete_hud_widget(const String &p_widget_id) {
	String target = p_widget_id.strip_edges().to_lower();
	Dictionary norm = normalize_hud_config(hud_config);
	Array widgets = Array(norm.get("widgets", Array())).duplicate(true);
	for (int i = 0; i < widgets.size(); i++) {
		if (String(Dictionary(widgets[i]).get("id", "")).to_lower() == target) {
			widgets.remove_at(i);
			norm["widgets"] = widgets;
			set_hud_config(norm);
			return true;
		}
	}
	return false;
}

String ClockGearsDataManager::duplicate_hud_widget(const String &p_source_id, const String &p_new_name) {
	Dictionary src = get_hud_widget(p_source_id);
	if (src.is_empty()) {
		return String();
	}
	Dictionary copy = src.duplicate(true);
	copy["id"] = "";
	String nn = p_new_name.strip_edges();
	if (nn.is_empty()) {
		nn = String(src.get("name", "Widget")) + " (Copia)";
	}
	copy["name"] = nn;
	copy["offset_x"] = CLAMP(int(copy.get("offset_x", 16)) + 16, -960, 960);
	copy["offset_y"] = CLAMP(int(copy.get("offset_y", 16)) + 16, -540, 540);
	return create_hud_widget(copy);
}

bool ClockGearsDataManager::move_hud_widget(const String &p_widget_id, int p_delta) {
	if (p_delta == 0) {
		return false;
	}
	String target = p_widget_id.strip_edges().to_lower();
	Dictionary norm = normalize_hud_config(hud_config);
	Array widgets = Array(norm.get("widgets", Array())).duplicate(true);
	int idx = -1;
	for (int i = 0; i < widgets.size(); i++) {
		if (String(Dictionary(widgets[i]).get("id", "")).to_lower() == target) {
			idx = i;
			break;
		}
	}
	if (idx < 0) {
		return false;
	}
	int new_idx = CLAMP(idx + p_delta, 0, widgets.size() - 1);
	if (new_idx == idx) {
		return false;
	}
	Variant item = widgets[idx];
	widgets.remove_at(idx);
	widgets.insert(new_idx, item);
	norm["widgets"] = widgets;
	set_hud_config(norm);
	return true;
}

void ClockGearsDataManager::set_hud_runtime_visible(bool p_visible) {
	if (runtime_hud_visible != p_visible) {
		runtime_hud_visible = p_visible;
		emit_signal("runtime_state_changed", "hud_visibility", String());
	}
}

void ClockGearsDataManager::set_hud_widget_runtime_visible(const String &p_widget_name_or_id, bool p_visible) {
	String key = p_widget_name_or_id.strip_edges().to_lower();
	if (key.is_empty()) {
		return;
	}
	bool had = runtime_hud_widget_visibility.has(key);
	bool prev = had ? bool(runtime_hud_widget_visibility[key]) : true;
	runtime_hud_widget_visibility[key] = p_visible;
	if (!had || prev != p_visible) {
		emit_signal("runtime_state_changed", "hud_visibility", key);
	}
}

void ClockGearsDataManager::clear_hud_widget_runtime_visibility(const String &p_widget_name_or_id) {
	String key = p_widget_name_or_id.strip_edges().to_lower();
	if (key.is_empty()) {
		if (!runtime_hud_widget_visibility.is_empty()) {
			runtime_hud_widget_visibility.clear();
			emit_signal("runtime_state_changed", "hud_visibility", String());
		}
		return;
	}
	if (runtime_hud_widget_visibility.has(key)) {
		runtime_hud_widget_visibility.erase(key);
		emit_signal("runtime_state_changed", "hud_visibility", key);
	}
}

bool ClockGearsDataManager::is_hud_widget_runtime_visible(const String &p_widget_id, const String &p_widget_name) const {
	if (!runtime_hud_visible) {
		return false;
	}
	String kid = p_widget_id.strip_edges().to_lower();
	if (!kid.is_empty() && runtime_hud_widget_visibility.has(kid)) {
		return bool(runtime_hud_widget_visibility[kid]);
	}
	String kname = p_widget_name.strip_edges().to_lower();
	if (!kname.is_empty() && runtime_hud_widget_visibility.has(kname)) {
		return bool(runtime_hud_widget_visibility[kname]);
	}
	return true;
}

Dictionary ClockGearsDataManager::get_hud_runtime_visibility_state() const {
	Dictionary out;
	out["hud_visible"] = runtime_hud_visible;
	out["widget_visibility"] = runtime_hud_widget_visibility.duplicate(true);
	return out;
}

// --- Editor Visual de Interfaz: Diseños por Pantalla, Enlaces, Acciones y Undo/Redo (9B Tanda 6A) ---

PackedStringArray ClockGearsDataManager::get_system_menu_ids() {
	PackedStringArray out;
	out.push_back("title");
	out.push_back("game_menu_options");
	out.push_back("game_menu_save");
	out.push_back("game_menu_main");
	out.push_back("game_menu_items");
	out.push_back("game_menu_equip");
	out.push_back("game_menu_status");
	out.push_back("credits");
	out.push_back("game_over");
	out.push_back("hud");
	out.push_back("messages_battle");
	return out;
}

bool ClockGearsDataManager::is_system_menu_id(const String &p_menu_id) {
	return _cg_is_system_menu_id(_cg_norm_ui_screen_id(p_menu_id));
}

String ClockGearsDataManager::normalize_menu_id(const String &p_raw_id) {
	return _cg_norm_ui_screen_id(p_raw_id);
}

Array ClockGearsDataManager::get_ui_design_screens() {
	if (singleton) {
		return singleton->get_all_menus();
	}
	Array out;
	PackedStringArray sids = get_system_menu_ids();
	for (int i = 0; i < sids.size(); i++) {
		const String &sid = sids[i];
		Dictionary d;
		d["id"] = sid;
		d["name"] = _cg_default_system_menu_name(sid);
		d["window_type"] = _cg_default_system_menu_win_type(sid);
		d["supports_focus"] = (sid != "hud");
		d["is_system"] = true;
		d["order"] = i;
		out.push_back(d);
	}
	return out;
}

PackedStringArray ClockGearsDataManager::get_ui_design_screen_ids() {
	if (singleton) {
		PackedStringArray out = get_system_menu_ids();
		PackedStringArray custom = singleton->get_custom_menu_ids();
		for (int i = 0; i < custom.size(); i++) {
			out.push_back(custom[i]);
		}
		return out;
	}
	return get_system_menu_ids();
}

PackedStringArray ClockGearsDataManager::get_ui_element_types() {
	PackedStringArray out;
	out.push_back("image");
	out.push_back("text");
	out.push_back("button");
	out.push_back("frame");
	out.push_back("bar");
	out.push_back("hearts");
	out.push_back("currency");
	out.push_back("item");
	out.push_back("variable");
	out.push_back("container");
	out.push_back("radial");
	out.push_back("folder");
	return out;
}

Array ClockGearsDataManager::get_menu_layout_modes() {
	Array out;
	auto add_m = [&](const String &p_id, const String &p_lbl) {
		Dictionary d;
		d["id"] = p_id;
		d["label"] = p_lbl;
		out.push_back(d);
	};
	add_m("free", "Libre (posicionamiento manual)");
	add_m("vertical_list", "Lista vertical");
	add_m("horizontal_list", "Lista horizontal");
	add_m("grid", String::utf8("Cuadrícula"));
	add_m("tabs", String::utf8("Pestañas"));
	add_m("carousel", "Carrusel");
	add_m("radial", "Radial (anillo / arco)");
	return out;
}

Array ClockGearsDataManager::get_ui_shape_types() {
	Array out;
	auto add_s = [&](const String &p_id, const String &p_lbl) {
		Dictionary d;
		d["id"] = p_id;
		d["label"] = p_lbl;
		out.push_back(d);
	};
	add_s("rect", String::utf8("Rectángulo"));
	add_s("rounded", String::utf8("Rectángulo redondeado"));
	add_s("pill", String::utf8("Píldora"));
	add_s("circle", String::utf8("Círculo"));
	add_s("ellipse", "Elipse");
	add_s("image", "Marco con imagen (9 porciones)");
	return out;
}

Array ClockGearsDataManager::get_menu_trigger_contexts() {
	Array out;
	auto add_c = [&](const String &p_id, const String &p_lbl) {
		Dictionary d;
		d["id"] = p_id;
		d["label"] = p_lbl;
		out.push_back(d);
	};
	add_c("map", "Solo en el mapa");
	add_c("map_and_battle", "En el mapa y en combate");
	add_c("any", "En cualquier pantalla");
	return out;
}

Array ClockGearsDataManager::get_all_ui_button_action_types() {
	Array out;
	auto push_t = [&](const String &p_id, const String &p_lbl) {
		Dictionary d;
		d["id"] = p_id;
		d["label"] = p_lbl;
		out.push_back(d);
	};
	push_t("open_menu", String::utf8("Abrir menú"));
	push_t("close", String::utf8("Cerrar menú"));
	push_t("back", "Volver al anterior");
	push_t("open_system_screen", "Abrir pantalla de sistema");
	push_t("new_game", "Nueva partida");
	push_t("continue", "Continuar");
	push_t("load_game", "Cargar partida");
	push_t("save_game", "Guardar partida");
	push_t("options", "Opciones");
	push_t("to_title", String::utf8("Salir al título"));
	push_t("quit", "Salir del juego");
	push_t("common_event", String::utf8("Ejecutar evento común"));
	push_t("set_switch", "Cambiar interruptor");
	push_t("set_variable", "Cambiar variable");
	push_t("play_sound", "Reproducir sonido");
	push_t("play_music", String::utf8("Reproducir música"));
	push_t("hud_visibility", "Mostrar u ocultar HUD");
	push_t("item_use", "Usar objeto seleccionado");
	push_t("item_discard", "Tirar objeto seleccionado");
	push_t("equip_apply", "Equipar pieza seleccionada");
	push_t("equip_clear", "Desequipar hueco seleccionado");
	return out;
}

Dictionary ClockGearsDataManager::normalize_menu_trigger(const Dictionary &p_trig) {
	Dictionary out;
	String kind = String(p_trig.get("kind", p_trig.get("type", "key"))).strip_edges().to_lower();
	if (kind != "key" && kind != "joy_button" && kind != "mouse_button" && kind != "action") {
		if ((p_trig.has("action_name") && !String(p_trig["action_name"]).strip_edges().is_empty()) ||
				(p_trig.has("action") && !String(p_trig["action"]).strip_edges().is_empty())) {
			kind = "action";
		} else if (p_trig.has("joy_button")) {
			kind = "joy_button";
		} else if (p_trig.has("mouse_button")) {
			kind = "mouse_button";
		} else {
			kind = "key";
		}
	}
	out["kind"] = kind;
	if (kind == "key") {
		int64_t kc = int64_t(p_trig.get("keycode", 0));
		String kname = String(p_trig.get("key_name", p_trig.get("key", p_trig.get("value", "")))).strip_edges();
		if (kc == 0 && !kname.is_empty()) {
			kc = (int64_t)find_keycode(kname);
		}
		if (kname.is_empty() && kc > 0) {
			kname = keycode_get_string((Key)kc);
		}
		out["keycode"] = kc;
		out["key_name"] = kname;
	} else if (kind == "joy_button") {
		out["joy_button"] = MAX(0, int(p_trig.get("joy_button", p_trig.get("button", p_trig.get("button_index", p_trig.get("value", 0))))));
	} else if (kind == "mouse_button") {
		out["mouse_button"] = MAX(1, int(p_trig.get("mouse_button", p_trig.get("button", p_trig.get("button_index", p_trig.get("value", 1))))));
	} else {
		out["action_name"] = String(p_trig.get("action_name", p_trig.get("action", p_trig.get("value", "game_menu")))).strip_edges();
	}
	return out;
}

Dictionary ClockGearsDataManager::normalize_menu_layout(const Dictionary &p_layout) {
	Dictionary out;
	out["mode"] = _cg_norm_layout_mode(p_layout.get("mode", "free"));
	out["origin_anchor"] = _cg_norm_hud_anchor(p_layout.get("origin_anchor", "center"));
	out["offset_x"] = CLAMP(int(p_layout.get("offset_x", 0)), -960, 960);
	out["offset_y"] = CLAMP(int(p_layout.get("offset_y", 0)), -540, 540);
	out["width"] = CLAMP(int(p_layout.get("width", 420)), 32, 960);
	out["height"] = CLAMP(int(p_layout.get("height", 320)), 32, 540);
	out["spacing_x"] = CLAMP(int(p_layout.get("spacing_x", 10)), 0, 128);
	out["spacing_y"] = CLAMP(int(p_layout.get("spacing_y", 10)), 0, 128);
	out["columns"] = CLAMP(int(p_layout.get("columns", 4)), 1, 20);
	out["rows"] = CLAMP(int(p_layout.get("rows", 0)), 0, 20);
	out["page_size"] = CLAMP(int(p_layout.get("page_size", 0)), 0, 100);
	out["scroll_enabled"] = p_layout.has("scroll_enabled") ? bool(p_layout["scroll_enabled"]) : true;
	out["current_page"] = MAX(0, int(p_layout.get("current_page", 0)));
	out["active_tab"] = MAX(0, int(p_layout.get("active_tab", 0)));
	Array tab_labels;
	if (p_layout.has("tab_labels") && p_layout["tab_labels"].get_type() == Variant::ARRAY) {
		Array raw_tl = p_layout["tab_labels"];
		for (int i = 0; i < raw_tl.size(); i++) {
			tab_labels.push_back(String(raw_tl[i]));
		}
	}
	out["tab_labels"] = tab_labels;

	// Parámetros de anillo / arco radial (prioritario, estilo Zelda 3D)
	out["radial_center_x"] = CLAMP(int(p_layout.get("radial_center_x", 480)), 0, 960);
	out["radial_center_y"] = CLAMP(int(p_layout.get("radial_center_y", 270)), 0, 540);
	out["radial_radius"] = CLAMP(int(p_layout.get("radial_radius", 145)), 48, 250);
	out["radial_slot_size"] = CLAMP(int(p_layout.get("radial_slot_size", 64)), 28, 140);
	out["radial_selected_scale"] = CLAMP(float(p_layout.get("radial_selected_scale", 1.28)), 1.0f, 1.8f);
	out["radial_start_angle_deg"] = float(p_layout.get("radial_start_angle_deg", -90.0));
	out["radial_sweep_angle_deg"] = CLAMP(float(p_layout.get("radial_sweep_angle_deg", 360.0)), 45.0f, 360.0f);
	out["radial_clockwise"] = p_layout.has("radial_clockwise") ? bool(p_layout["radial_clockwise"]) : true;
	out["radial_show_center_info"] = p_layout.has("radial_show_center_info") ? bool(p_layout["radial_show_center_info"]) : true;
	String roa = String(p_layout.get("radial_open_anim", "radial_spin")).strip_edges().to_lower();
	if (roa != "none" && roa != "fade" && roa != "scale") {
		roa = "radial_spin";
	}
	out["radial_open_anim"] = roa;
	out["radial_ring_color"] = _cg_norm_color_rgba(p_layout.get("radial_ring_color", "#1a1e2ad9"), "#1a1e2ad9");
	out["radial_accent_color"] = _cg_norm_color_rgba(p_layout.get("radial_accent_color", "#f5bc54ff"), "#f5bc54ff");

	Array r_slots;
	if (p_layout.has("radial_slots") && p_layout["radial_slots"].get_type() == Variant::ARRAY) {
		Array raw_rs = p_layout["radial_slots"];
		for (int i = 0; i < raw_rs.size(); i++) {
			Dictionary rs;
			if (raw_rs[i].get_type() == Variant::STRING) {
				String sid_s = String(raw_rs[i]).strip_edges();
				if (sid_s.is_empty()) continue;
				rs["id"] = sid_s;
				rs["title"] = sid_s;
			} else if (raw_rs[i].get_type() == Variant::DICTIONARY) {
				rs = raw_rs[i];
			} else {
				continue;
			}
			Dictionary nrs;
			nrs["id"] = String(rs.get("id", vformat("slot_%d", i + 1))).strip_edges();
			nrs["title"] = String(rs.get("title", rs.get("name", nrs["id"])));
			nrs["description"] = String(rs.get("description", ""));
			nrs["icon_path"] = String(rs.get("icon_path", rs.get("image_path", ""))).strip_edges();
			nrs["pixel_art"] = rs.has("pixel_art") ? bool(rs["pixel_art"]) : true;
			nrs["action"] = String(rs.get("action", "")).strip_edges().to_lower();
			Array acts;
			if (rs.has("actions") && rs["actions"].get_type() == Variant::ARRAY) {
				Array raw_a = rs["actions"];
				for (int ai = 0; ai < raw_a.size(); ai++) {
					if (raw_a[ai].get_type() == Variant::DICTIONARY) {
						acts.push_back(_cg_norm_button_action_step(raw_a[ai]));
					}
				}
			}
			nrs["actions"] = acts;
			nrs["enabled_mode"] = String(rs.get("enabled_mode", "always")).strip_edges().to_lower();
			nrs["enabled_switch"] = String(rs.get("enabled_switch", "")).strip_edges();
			nrs["visibility_mode"] = String(rs.get("visibility_mode", "always")).strip_edges().to_lower();
			nrs["visibility_switch"] = String(rs.get("visibility_switch", "")).strip_edges();
			r_slots.push_back(nrs);
		}
	}
	out["radial_slots"] = r_slots;
	return out;
}

Dictionary ClockGearsDataManager::normalize_ui_background(const Dictionary &p_bg) {
	Dictionary out;
	String bt = String(p_bg.get("type", "none")).strip_edges().to_lower();
	if (bt != "none" && bt != "color" && bt != "gradient" && bt != "image") {
		bt = "none";
	}
	out["type"] = bt;
	out["color"] = _cg_norm_color_rgba(p_bg.get("color", "#0d111ae6"), "#0d111ae6");
	out["gradient_top"] = _cg_norm_color_rgba(p_bg.get("gradient_top", "#1b2234ff"), "#1b2234ff");
	out["gradient_bottom"] = _cg_norm_color_rgba(p_bg.get("gradient_bottom", "#0b0e16ff"), "#0b0e16ff");
	String ga = String(p_bg.get("gradient_angle", "vertical")).strip_edges().to_lower();
	out["gradient_angle"] = (ga == "horizontal") ? "horizontal" : "vertical";
	out["image_path"] = String(p_bg.get("image_path", "")).strip_edges();
	String sm = String(p_bg.get("scale_mode", "stretch")).strip_edges().to_lower();
	if (sm != "stretch" && sm != "fit" && sm != "fill" && sm != "tile" && sm != "nine_slice" && sm != "integer") {
		sm = "stretch";
	}
	out["scale_mode"] = sm;
	out["pixel_art"] = p_bg.has("pixel_art") ? bool(p_bg["pixel_art"]) : true;
	out["scroll_speed_x"] = CLAMP(int(p_bg.get("scroll_speed_x", 0)), -400, 400);
	out["scroll_speed_y"] = CLAMP(int(p_bg.get("scroll_speed_y", 0)), -400, 400);
	bool v_en = p_bg.has("veil_enabled") ? bool(p_bg["veil_enabled"]) : bool(p_bg.get("dark_veil", false));
	out["veil_enabled"] = v_en;
	out["dark_veil"] = v_en;
	out["veil_color"] = _cg_norm_color_rgba(p_bg.get("veil_color", "#05070ca6"), "#05070ca6");
	out["blur_enabled"] = bool(p_bg.get("blur_enabled", false));
	out["blur_intensity"] = CLAMP(int(p_bg.get("blur_intensity", 45)), 0, 100);
	return out;
}

Dictionary ClockGearsDataManager::normalize_ui_transition(const Dictionary &p_tr) {
	Dictionary out;
	auto norm_t = [](const String &p_raw) -> String {
		String s = p_raw.strip_edges().to_lower();
		if (s == "fade" || s == "slide_left" || s == "slide_right" || s == "slide_top" || s == "slide_bottom" || s == "scale" || s == "radial_spin") {
			return s;
		}
		return "none";
	};
	out["in_type"] = norm_t(String(p_tr.get("in_type", "fade")));
	out["out_type"] = norm_t(String(p_tr.get("out_type", "fade")));
	out["duration_sec"] = CLAMP(float(p_tr.get("duration_sec", 0.16)), 0.0f, 1.5f);
	String c = String(p_tr.get("curve", "ease_out")).strip_edges().to_lower();
	if (c != "linear" && c != "ease_out" && c != "ease_in_out" && c != "back_out") {
		c = "ease_out";
	}
	out["curve"] = c;
	return out;
}

Dictionary ClockGearsDataManager::normalize_ui_shape(const Dictionary &p_shape) {
	Dictionary out;
	String st = _cg_norm_shape_type(p_shape.get("shape_type", p_shape.get("type", "rounded")));
	if (st.is_empty()) st = "rounded";
	out["shape_type"] = st;
	out["corner_radius"] = CLAMP(int(p_shape.get("corner_radius", 8)), 0, 256);
	out["clip_content"] = p_shape.has("clip_content") ? bool(p_shape["clip_content"]) : (st == "circle" || st == "ellipse" || st == "pill" || st == "rounded");
	out["fill_type"] = (String(p_shape.get("fill_type", "color")).strip_edges().to_lower() == "image") ? "image" : "color";
	out["fill_color"] = _cg_norm_color_rgba(p_shape.get("fill_color", "#14171ff0"), "#14171ff0");
	out["fill_image_path"] = String(p_shape.get("fill_image_path", "")).strip_edges();
	out["image_path"] = String(p_shape.get("image_path", "")).strip_edges();
	out["pixel_art"] = p_shape.has("pixel_art") ? bool(p_shape["pixel_art"]) : true;
	out["gradient_enabled"] = bool(p_shape.get("gradient_enabled", false));
	out["gradient_top_color"] = _cg_norm_color_rgba(p_shape.get("gradient_top_color", "#28221aff"), "#28221aff");
	out["gradient_bottom_color"] = _cg_norm_color_rgba(p_shape.get("gradient_bottom_color", "#14110eff"), "#14110eff");
	String ga = String(p_shape.get("gradient_angle", "vertical")).strip_edges().to_lower();
	out["gradient_angle"] = (ga == "horizontal") ? "horizontal" : "vertical";
	out["border_width"] = CLAMP(int(p_shape.get("border_width", 2)), 0, 24);
	out["border_color"] = _cg_norm_color_rgba(p_shape.get("border_color", "#e0a040ff"), "#e0a040ff");
	out["shadow_enabled"] = bool(p_shape.get("shadow_enabled", false));
	out["shadow_color"] = _cg_norm_color_rgba(p_shape.get("shadow_color", "#00000073"), "#00000073");
	out["shadow_size"] = CLAMP(int(p_shape.get("shadow_size", 4)), 0, 32);
	out["shadow_offset_x"] = CLAMP(int(p_shape.get("shadow_offset_x", 0)), -32, 32);
	out["shadow_offset_y"] = CLAMP(int(p_shape.get("shadow_offset_y", 2)), -32, 32);
	out["glow_enabled"] = bool(p_shape.get("glow_enabled", false));
	out["glow_color"] = _cg_norm_color_rgba(p_shape.get("glow_color", "#f5bc5466"), "#f5bc5466");
	out["glow_size"] = CLAMP(int(p_shape.get("glow_size", 4)), 0, 24);
	return out;
}

Array ClockGearsDataManager::get_ui_data_bindings() {
	Array out;
	auto push_b = [&](const String &p_id, const String &p_label) {
		Dictionary d;
		d["id"] = p_id;
		d["label"] = p_label;
		out.push_back(d);
	};
	push_b("none", "Ninguno (texto fijo)");
	push_b("project_title", String::utf8("Título del proyecto"));
	push_b("currency_name", "Nombre de la moneda");
	push_b("gold", "Dinero");
	push_b("item_qty", "Cantidad de un objeto");
	push_b("variable", "Variable");
	push_b("player_hp", "HP del jugador");
	push_b("player_mp", "MP del jugador");
	push_b("actor_name", "Nombre del actor");
	push_b("actor_level", "Nivel del actor");
	push_b("playtime", "Tiempo jugado");
	return out;
}

Array ClockGearsDataManager::get_ui_button_actions(const String &p_screen_id) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Array out;
	auto push_a = [&](const String &p_id, const String &p_label) {
		Dictionary d;
		d["id"] = p_id;
		d["label"] = p_label;
		out.push_back(d);
	};
	if (sid == "title") {
		push_a("new_game", "Nueva partida");
		push_a("continue", "Continuar");
		push_a("options", "Opciones");
		push_a("credits", String::utf8("Créditos"));
		push_a("quit", "Salir");
	} else if (sid == "game_menu_main") {
		push_a("items", "Objetos");
		push_a("equip", "Equipo");
		push_a("status", "Estado");
		push_a("save", "Guardar");
		push_a("options", "Opciones");
		push_a("to_title", String::utf8("Salir al título"));
		push_a("back", "Volver");
	} else if (sid == "game_menu_items" || sid == "game_menu_equip" || sid == "game_menu_status" || sid == "game_menu_save" || sid == "game_menu_options") {
		push_a("back", "Volver");
		if (sid == "game_menu_items") {
			push_a("item_use", "Usar objeto seleccionado");
			push_a("item_discard", "Tirar objeto seleccionado");
		} else if (sid == "game_menu_equip") {
			push_a("equip_apply", "Equipar pieza seleccionada");
			push_a("equip_clear", "Desequipar hueco seleccionado");
		}
		push_a("items", "Objetos");
		push_a("equip", "Equipo");
		push_a("status", "Estado");
		push_a("save", "Guardar");
		push_a("options", "Opciones");
		push_a("close", String::utf8("Cerrar menú"));
	} else if (sid == "credits") {
		push_a("back", "Volver");
		push_a("close", String::utf8("Cerrar créditos"));
	} else if (sid == "messages_battle") {
		push_a("close", "Continuar / Confirmar");
		push_a("open_system_screen", "Abrir pantalla de sistema");
		push_a("common_event", String::utf8("Ejecutar evento común"));
		push_a("set_switch", "Cambiar interruptor");
		push_a("set_variable", "Cambiar variable");
	}
	// Ampliar con todas las acciones configurables de 3D en cualquier pantalla rediseñable (5A)
	Array all_acts = get_all_ui_button_action_types();
	HashSet<String> seen;
	for (int i = 0; i < out.size(); i++) {
		seen.insert(String(Dictionary(out[i]).get("id", "")));
	}
	for (int i = 0; i < all_acts.size(); i++) {
		Dictionary ad = all_acts[i];
		String aid = String(ad.get("id", ""));
		if (!aid.is_empty() && !seen.has(aid)) {
			seen.insert(aid);
			out.push_back(ad);
		}
	}
	return out;
}

Array ClockGearsDataManager::make_default_radial_slots(int p_count, const String &p_screen_id) {
	int cnt = CLAMP(p_count, 1, 16);
	Array avail = get_ui_button_actions(p_screen_id);
	Array slots;
	for (int i = 0; i < cnt; i++) {
		Dictionary s;
		s["id"] = vformat("slot_%d", i + 1);
		String title = String::utf8("Ranura ") + itos(i + 1);
		String act = "";
		Array acts;
		if (i < avail.size() && avail[i].get_type() == Variant::DICTIONARY) {
			Dictionary ad = avail[i];
			String aid = String(ad.get("id", ""));
			if (!aid.is_empty() && aid != "quit") {
				act = aid;
				String lbl = String(ad.get("label", ""));
				if (!lbl.is_empty()) {
					title = lbl;
				}
				Dictionary step;
				step["type"] = aid;
				acts.push_back(step);
			}
		}
		s["title"] = title;
		s["description"] = "";
		s["icon_path"] = "";
		s["pixel_art"] = true;
		s["action"] = act;
		s["actions"] = acts;
		s["enabled_mode"] = "always";
		s["enabled_switch"] = "";
		s["visibility_mode"] = "always";
		s["visibility_switch"] = "";
		slots.push_back(s);
	}
	return slots;
}

Dictionary ClockGearsDataManager::make_default_ui_element(const String &p_type, const String &p_screen_id) {
	String etype = _cg_norm_ui_elem_type(p_type);
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary el = make_default_hud_widget(etype);
	el["type"] = etype;
	el["visible"] = true;
	el["data_binding"] = (etype == "variable") ? "variable" : "none";
	el["actor_id"] = 0;

	if (etype == "button") {
		Array actions = get_ui_button_actions(sid);
		String def_act = (!actions.is_empty() && actions[0].get_type() == Variant::DICTIONARY) ? String(Dictionary(actions[0]).get("id", "")) : String("close");
		el["action"] = def_act;
		el["actions"] = Array();
		el["focus_order"] = 0;
		el["window_style_id"] = "";
	} else if (etype == "frame") {
		el["name"] = "Marco";
		el["width"] = 300;
		el["height"] = 200;
	} else if (etype == "container" || etype == "radial") {
		el["name"] = (etype == "radial") ? "Rueda radial" : "Contenedor";
		el["anchor"] = "top_left";
		el["offset_x"] = (etype == "radial") ? 300 : 270;
		el["offset_y"] = (etype == "radial") ? 90 : 110;
		el["width"] = (etype == "radial") ? 360 : 420;
		el["height"] = (etype == "radial") ? 360 : 320;
		el["window_style_id"] = "none";
		Dictionary lay;
		if (etype == "radial") {
			lay["mode"] = "radial";
			lay["radial_center_x"] = 480;
			lay["radial_center_y"] = 270;
			lay["radial_radius"] = 128;
			lay["radial_slot_size"] = 56;
			lay["radial_sweep_angle_deg"] = 360.0;
			lay["radial_start_angle_deg"] = -90.0;
			lay["radial_clockwise"] = true;
			lay["radial_show_center_info"] = true;
			lay["radial_open_anim"] = "radial_spin";
			lay["radial_slots"] = make_default_radial_slots(6, sid);
		}
		el["layout"] = normalize_menu_layout(lay);
	} else if (etype == "widget") {
		el["name"] = "Panel / Lista";
		el["width"] = 360;
		el["height"] = 220;
		el["widget_kind"] = (sid == "game_menu_main") ? String("party_list") : ((sid == "game_menu_items") ? String("items_list") : ((sid == "game_menu_equip") ? String("equip_slots") : ((sid == "game_menu_status") ? String("status_params") : ((sid == "game_menu_save") ? String("save_slots") : ((sid == "game_menu_options") ? String("options_controls") : String("party_list"))))));
	} else if (etype == "text") {
		el["anchor"] = "top_left";
		el["offset_x"] = 24;
		el["offset_y"] = 24;
	} else if (etype == "folder") {
		el["name"] = "Carpeta";
		el["anchor"] = "top_left";
		el["offset_x"] = 0;
		el["offset_y"] = 0;
		el["width"] = 0;
		el["height"] = 0;
	}
	el["parent_id"] = "";
	el["locked"] = false;
	el["collapsed"] = false;
	return el;
}

Dictionary ClockGearsDataManager::normalize_ui_element(const Dictionary &p_elem, int p_index_hint, const String &p_screen_id) {
	String etype = _cg_norm_ui_elem_type(p_elem.get("type", "text"));
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary def = make_default_ui_element(etype, sid);

	Dictionary out;
	String raw_name = String(p_elem.get("name", def.get("name", "Elemento"))).strip_edges();
	if (raw_name.is_empty()) {
		raw_name = String(def.get("name", "Elemento")) + " " + itos(MAX(1, p_index_hint));
	}
	String raw_id = _cg_slugify_hud_id(String(p_elem.get("id", "")));
	if (raw_id.is_empty()) {
		raw_id = _cg_slugify_hud_id(raw_name);
	}
	if (raw_id.is_empty()) {
		raw_id = "elem_" + itos(MAX(1, p_index_hint));
	}

	out["id"] = raw_id;
	out["name"] = raw_name;
	out["type"] = etype;
	out["parent_id"] = String(p_elem.get("parent_id", def.get("parent_id", ""))).strip_edges();
	out["anchor"] = _cg_norm_hud_anchor(p_elem.get("anchor", def["anchor"]));
	out["offset_x"] = CLAMP(int(p_elem.get("offset_x", def["offset_x"])), -960, 960);
	out["offset_y"] = CLAMP(int(p_elem.get("offset_y", def["offset_y"])), -540, 540);
	out["width"] = (etype == "folder") ? CLAMP(int(p_elem.get("width", 0)), 0, 960) : CLAMP(int(p_elem.get("width", def["width"])), 16, 960);
	out["height"] = (etype == "folder") ? CLAMP(int(p_elem.get("height", 0)), 0, 540) : CLAMP(int(p_elem.get("height", def["height"])), 16, 540);
	out["layer_order"] = CLAMP(int(p_elem.get("layer_order", def["layer_order"])), -100, 1000);
	out["visible"] = p_elem.has("visible") ? bool(p_elem["visible"]) : true;
	out["locked"] = p_elem.has("locked") ? bool(p_elem["locked"]) : false;
	out["collapsed"] = p_elem.has("collapsed") ? bool(p_elem["collapsed"]) : false;
	out["clip_children"] = p_elem.has("clip_children") ? bool(p_elem["clip_children"]) : false;
	String lm = String(p_elem.get("layout_mode", "free")).strip_edges().to_lower();
	if (lm != "vertical_list" && lm != "horizontal_list" && lm != "grid") {
		lm = "free";
	}
	if (lm != "free" || p_elem.has("layout_mode")) {
		out["layout_mode"] = lm;
	}
	if (lm != "free" || p_elem.has("layout_columns")) {
		out["layout_columns"] = CLAMP(int(p_elem.get("layout_columns", 2)), 1, 12);
	}
	if (lm != "free" || p_elem.has("layout_spacing")) {
		out["layout_spacing"] = CLAMP(int(p_elem.get("layout_spacing", 8)), 0, 64);
	}

	String ws = String(p_elem.get("window_style_id", def.get("window_style_id", ""))).strip_edges().to_lower();
	out["window_style_id"] = ws;
	out["pixel_art"] = p_elem.has("pixel_art") ? bool(p_elem["pixel_art"]) : true;
	String vis_m = String(p_elem.get("visibility_mode", "always")).strip_edges().to_lower();
	String vis_sw = String(p_elem.get("visibility_switch", "")).strip_edges();
	bool vis_inv = bool(p_elem.get("visibility_invert", false));
	if (p_elem.has("visible_condition") && p_elem["visible_condition"].get_type() == Variant::DICTIONARY) {
		Dictionary vc = p_elem["visible_condition"];
		String vcm = String(vc.get("mode", "always")).strip_edges().to_lower();
		if (vcm == "switch_on" || vcm == "switch_off" || vcm == "switch") {
			vis_m = "switch";
			vis_inv = (vcm == "switch_off");
			vis_sw = String(vc.get("switch_name", vis_sw)).strip_edges();
		}
	}
	out["visibility_mode"] = (vis_m == "switch") ? "switch" : "always";
	out["visibility_switch"] = vis_sw;
	out["visibility_invert"] = vis_inv;

	String emode = String(p_elem.get("enabled_mode", "always")).strip_edges().to_lower();
	String en_sw = String(p_elem.get("enabled_switch", "")).strip_edges();
	bool en_inv = bool(p_elem.get("enabled_invert", false));
	if (p_elem.has("enabled_condition") && p_elem["enabled_condition"].get_type() == Variant::DICTIONARY) {
		Dictionary ec = p_elem["enabled_condition"];
		String ecm = String(ec.get("mode", "always")).strip_edges().to_lower();
		if (ecm == "switch_on" || ecm == "switch_off" || ecm == "switch") {
			emode = "switch";
			en_inv = (ecm == "switch_off");
			en_sw = String(ec.get("switch_name", en_sw)).strip_edges();
		} else if (ecm == "disabled") {
			emode = "disabled";
		}
	}
	if (emode != "switch" && emode != "disabled") {
		emode = bool(p_elem.get("disabled", false)) ? "disabled" : "always";
	}
	out["enabled_mode"] = emode;
	out["enabled_switch"] = en_sw;
	out["enabled_invert"] = en_inv;
	out["hide_during_messages_and_menus"] = p_elem.has("hide_during_messages_and_menus") ? bool(p_elem["hide_during_messages_and_menus"]) : (sid == "hud");

	String db_def = (etype == "variable") ? "variable" : "none";
	out["data_binding"] = _cg_norm_ui_data_binding(p_elem.get("data_binding", db_def));
	out["actor_id"] = MAX(0, int(p_elem.get("actor_id", 0)));

	_cg_normalize_hud_core_fields(p_elem, def, etype, out);

	// Preservar item_key también para text/button cuando usan data_binding == "item_qty"
	if ((etype == "text" || etype == "button") && !out.has("item_key")) {
		out["item_key"] = String(p_elem.get("item_key", "")).strip_edges();
	}
	if ((etype == "text" || etype == "button" || etype == "variable") && !out.has("variable_name")) {
		out["variable_name"] = String(p_elem.get("variable_name", p_elem.get("var_name", p_elem.get("variable", "")))).strip_edges();
	}
	if (p_elem.has("tab_index")) {
		out["tab_index"] = MAX(0, int(p_elem["tab_index"]));
	}
	if (p_elem.has("description")) {
		out["description"] = String(p_elem["description"]);
	}
	if (p_elem.has("icon_path") && !out.has("icon_path")) {
		out["icon_path"] = String(p_elem["icon_path"]).strip_edges();
	}
	if (p_elem.has("font_path")) {
		out["font_path"] = String(p_elem["font_path"]).strip_edges();
	}
	if (p_elem.has("font_size")) {
		out["font_size"] = CLAMP(int(p_elem["font_size"]), 8, 72);
	}
	if (p_elem.has("font_color")) {
		out["font_color"] = _cg_norm_color_rgba(p_elem["font_color"], "#eef2faff");
	}
	if (p_elem.has("color") && !out.has("fill_color")) {
		out["color"] = _cg_norm_color_rgba(p_elem["color"], "#eef2faff");
	}
	if (p_elem.has("group_id")) {
		out["group_id"] = String(p_elem["group_id"]).strip_edges();
	}
	if (p_elem.has("locked")) {
		out["locked"] = bool(p_elem["locked"]);
	}
	if (p_elem.has("opacity") || !out.has("opacity")) {
		out["opacity"] = CLAMP(double(p_elem.get("opacity", out.get("opacity", 1.0))), 0.0, 1.0);
	}
	if (p_elem.has("show_background")) {
		out["show_background"] = bool(p_elem["show_background"]);
	}
	if (etype == "widget") {
		String wk = String(p_elem.get("widget_kind", def.get("widget_kind", "party_list"))).strip_edges().to_lower();
		if (wk.is_empty()) {
			wk = String(def.get("widget_kind", "party_list"));
		}
		out["widget_kind"] = wk;
	}
	// Forma avanzada opcional (3F: rect, rounded, pill, circle, ellipse, image + recorte, degradado, sombra, brillo)
	if (p_elem.has("shape") && p_elem["shape"].get_type() == Variant::DICTIONARY) {
		out["shape"] = normalize_ui_shape(Dictionary(p_elem["shape"]));
	}
	if (etype == "container" || etype == "radial") {
		Dictionary raw_lay = (p_elem.has("layout") && p_elem["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(p_elem["layout"]) : Dictionary();
		if (etype == "radial" && !raw_lay.has("mode")) {
			raw_lay["mode"] = "radial";
		}
		out["layout"] = normalize_menu_layout(raw_lay);
	}
	if (etype == "button") {
		String raw_action = String(p_elem.get("action", def.get("action", ""))).strip_edges().to_lower();
		if (_cg_is_system_menu_id(sid)) {
			Array valid_actions = get_ui_button_actions(sid);
			bool found_action = raw_action.is_empty() || raw_action == "none";
			if (!found_action) {
				Array all_acts = get_all_ui_button_action_types();
				for (int i = 0; i < valid_actions.size(); i++) {
					if (String(Dictionary(valid_actions[i]).get("id", "")) == raw_action) {
						found_action = true;
						break;
					}
				}
				if (!found_action) {
					for (int i = 0; i < all_acts.size(); i++) {
						if (String(Dictionary(all_acts[i]).get("id", "")) == raw_action) {
							found_action = true;
							break;
						}
					}
				}
			}
			if (!found_action && !valid_actions.is_empty()) {
				raw_action = String(Dictionary(valid_actions[0]).get("id", ""));
			}
		}
		out["action"] = raw_action;
		for (const char *pk : { "switch_name", "switch", "variable_name", "var_name", "variable", "value", "op", "menu_id", "target", "screen_id", "screen", "common_event_id", "sound_path", "sound", "slot", "visible", "scope", "widget_id", "widget", "pitch", "sound_hover", "sound_click", "image_normal", "image_hover", "image_pressed", "image_disabled" }) {
			if (p_elem.has(pk)) {
				out[pk] = p_elem[pk];
			}
		}
		if (p_elem.has("shortcut") && p_elem["shortcut"].get_type() == Variant::DICTIONARY) {
			Dictionary sc = p_elem["shortcut"];
			if (!sc.is_empty() && (int64_t(sc.get("keycode", 0)) > 0 || sc.has("joy_button") || sc.has("mouse_button") || !String(sc.get("action_name", "")).is_empty())) {
				out["shortcut"] = normalize_menu_trigger(sc);
			}
		}
		if (p_elem.has("actions") && p_elem["actions"].get_type() == Variant::ARRAY) {
			Array acts_in = p_elem["actions"];
			Array acts_out;
			for (int ai = 0; ai < acts_in.size(); ai++) {
				if (acts_in[ai].get_type() == Variant::DICTIONARY) {
					acts_out.push_back(_cg_norm_button_action_step(Dictionary(acts_in[ai])));
				}
			}
			out["actions"] = acts_out;
		}
		out["focus_order"] = CLAMP(int(p_elem.get("focus_order", def.get("focus_order", p_index_hint))), 0, 9999);
		if (p_elem.has("focus_next_up")) out["focus_next_up"] = String(p_elem["focus_next_up"]).strip_edges();
		if (p_elem.has("focus_next_down")) out["focus_next_down"] = String(p_elem["focus_next_down"]).strip_edges();
		if (p_elem.has("focus_next_left")) out["focus_next_left"] = String(p_elem["focus_next_left"]).strip_edges();
		if (p_elem.has("focus_next_right")) out["focus_next_right"] = String(p_elem["focus_next_right"]).strip_edges();
	}
	return out;
}

Dictionary ClockGearsDataManager::get_default_ui_screen_design(const String &p_screen_id) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary d;
	d["screen_id"] = sid;
	d["name"] = _cg_default_system_menu_name(sid);
	d["is_system"] = _cg_is_system_menu_id(sid);
	d["order"] = 0;
	d["window_type"] = _cg_default_system_menu_win_type(sid);
	d["custom_enabled"] = (!bool(d["is_system"]) && sid != "game_over");
	d["elements"] = Array();
	d["triggers"] = (sid == "game_menu_main") ? _cg_default_game_menu_triggers() : Array();
	d["trigger_context"] = "map";
	d["pause_on_open"] = (sid != "hud");
	d["respect_menu_access"] = true;
	d["layout"] = normalize_menu_layout(Dictionary());

	Dictionary cursor;
	cursor["enabled"] = false;
	cursor["shape"] = "ring";
	cursor["image_path"] = "";
	cursor["pixel_art"] = true;
	cursor["color"] = "#f5bc54ff";
	cursor["size"] = 3;
	cursor["animation"] = "pulse";
	d["cursor"] = cursor;

	Dictionary sounds;
	sounds["open"] = "";
	sounds["close"] = "";
	sounds["move"] = "";
	sounds["accept"] = "";
	sounds["cancel"] = "";
	d["sounds"] = sounds;
	return d;
}

Dictionary ClockGearsDataManager::normalize_ui_screen_design(const String &p_screen_id, const Dictionary &p_design) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	bool is_sys = _cg_is_system_menu_id(sid);
	Dictionary out;
	out["screen_id"] = sid;
	String raw_name = String(p_design.get("name", is_sys ? _cg_default_system_menu_name(sid) : sid)).strip_edges();
	if (raw_name.is_empty()) {
		raw_name = is_sys ? _cg_default_system_menu_name(sid) : sid;
	}
	out["name"] = raw_name;
	out["is_system"] = is_sys;
	out["order"] = int(p_design.get("order", 0));
	String raw_parent = String(p_design.get("parent_id", "")).strip_edges();
	if (!raw_parent.is_empty()) {
		raw_parent = _cg_norm_ui_screen_id(raw_parent);
	}
	if (is_sys || raw_parent == sid) {
		raw_parent = "";
	}
	out["parent_id"] = raw_parent;
	String raw_sg = String(p_design.get("system_group", "")).strip_edges();
	if (is_sys) {
		raw_sg = sid;
	} else if (!raw_sg.is_empty()) {
		raw_sg = _cg_norm_ui_screen_id(raw_sg);
	} else if (!raw_parent.is_empty() && _cg_is_system_menu_id(raw_parent)) {
		raw_sg = raw_parent;
	}
	out["system_group"] = raw_sg;
	String wt = String(p_design.get("window_type", _cg_default_system_menu_win_type(sid))).strip_edges().to_lower();
	out["window_type"] = wt.is_empty() ? _cg_default_system_menu_win_type(sid) : wt;

	Array norm_elems;
	HashSet<String> seen_ids;
	if (p_design.has("elements") && p_design["elements"].get_type() == Variant::ARRAY) {
		Array raw = p_design["elements"];
		for (int i = 0; i < raw.size(); i++) {
			if (raw[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary ne = normalize_ui_element(Dictionary(raw[i]), i + 1, sid);
			String base_id = String(ne["id"]);
			String uid = base_id;
			int suffix = 2;
			while (seen_ids.has(uid)) {
				uid = base_id + "_" + itos(suffix++);
			}
			seen_ids.insert(uid);
			ne["id"] = uid;
			norm_elems.push_back(ne);
		}
	}
	// Validar parent_id (existencia + evitar ciclos / autoreferencias)
	HashMap<String, int> id_to_idx;
	for (int i = 0; i < norm_elems.size(); i++) {
		Dictionary ne = norm_elems[i];
		id_to_idx[String(ne.get("id", "")).to_lower()] = i;
	}
	for (int i = 0; i < norm_elems.size(); i++) {
		Dictionary ne = norm_elems[i];
		String pid = String(ne.get("parent_id", "")).strip_edges();
		if (pid.is_empty()) {
			continue;
		}
		String pid_low = pid.to_lower();
		if (!id_to_idx.has(pid_low)) {
			ne["parent_id"] = "";
			norm_elems[i] = ne;
			continue;
		}
		String self_low = String(ne.get("id", "")).to_lower();
		HashSet<String> visited;
		visited.insert(self_low);
		String cur = pid_low;
		bool valid = true;
		String canonical_pid = String(Dictionary(norm_elems[id_to_idx[pid_low]]).get("id", pid));
		while (!cur.is_empty()) {
			if (visited.has(cur) || !id_to_idx.has(cur)) {
				valid = false;
				break;
			}
			visited.insert(cur);
			cur = String(Dictionary(norm_elems[id_to_idx[cur]]).get("parent_id", "")).strip_edges().to_lower();
		}
		ne["parent_id"] = valid ? canonical_pid : String("");
		norm_elems[i] = ne;
	}
	out["elements"] = norm_elems;
	bool can_toggle_default = is_sys || (sid == "game_over");
	if (p_design.has("custom_enabled")) {
		out["custom_enabled"] = can_toggle_default ? bool(p_design["custom_enabled"]) : true;
	} else {
		out["custom_enabled"] = can_toggle_default ? !norm_elems.is_empty() : true;
	}

	Array trigs;
	if (p_design.has("triggers") && p_design["triggers"].get_type() == Variant::ARRAY) {
		Array raw_tr = p_design["triggers"];
		for (int i = 0; i < raw_tr.size(); i++) {
			if (raw_tr[i].get_type() == Variant::DICTIONARY) {
				trigs.push_back(normalize_menu_trigger(Dictionary(raw_tr[i])));
			}
		}
	} else if (sid == "game_menu_main") {
		trigs = _cg_default_game_menu_triggers();
	}
	out["triggers"] = trigs;
	out["trigger_context"] = _cg_norm_trigger_context(p_design.get("trigger_context", "map"));
	out["pause_on_open"] = p_design.has("pause_on_open") ? bool(p_design["pause_on_open"]) : (sid != "hud");
	out["respect_menu_access"] = p_design.has("respect_menu_access") ? bool(p_design["respect_menu_access"]) : true;

	Dictionary raw_layout = (p_design.has("layout") && p_design["layout"].get_type() == Variant::DICTIONARY) ? Dictionary(p_design["layout"]) : Dictionary();
	out["layout"] = normalize_menu_layout(raw_layout);

	Dictionary raw_cur = (p_design.has("cursor") && p_design["cursor"].get_type() == Variant::DICTIONARY) ? Dictionary(p_design["cursor"]) : Dictionary();
	Dictionary cur;
	cur["enabled"] = bool(raw_cur.get("enabled", false));
	String cshape = String(raw_cur.get("shape", "ring")).strip_edges().to_lower();
	if (cshape != "ring" && cshape != "rect" && cshape != "rounded" && cshape != "pill" && cshape != "arrow" && cshape != "image") cshape = "ring";
	cur["shape"] = cshape;
	cur["image_path"] = String(raw_cur.get("image_path", "")).strip_edges();
	cur["pixel_art"] = raw_cur.has("pixel_art") ? bool(raw_cur["pixel_art"]) : true;
	cur["color"] = _cg_norm_color_rgba(raw_cur.get("color", "#f5bc54ff"), "#f5bc54ff");
	cur["size"] = CLAMP(int(raw_cur.get("size", 3)), 1, 24);
	String canim = String(raw_cur.get("animation", "pulse")).strip_edges().to_lower();
	if (canim != "none" && canim != "pulse" && canim != "bob" && canim != "spin") canim = "pulse";
	cur["animation"] = canim;
	out["cursor"] = cur;

	Dictionary raw_snd = (p_design.has("sounds") && p_design["sounds"].get_type() == Variant::DICTIONARY) ? Dictionary(p_design["sounds"]) : Dictionary();
	Dictionary snd;
	snd["open"] = String(raw_snd.get("open", "")).strip_edges();
	snd["close"] = String(raw_snd.get("close", "")).strip_edges();
	snd["move"] = String(raw_snd.get("move", "")).strip_edges();
	snd["accept"] = String(raw_snd.get("accept", "")).strip_edges();
	snd["cancel"] = String(raw_snd.get("cancel", "")).strip_edges();
	out["sounds"] = snd;

	if (p_design.has("background") && p_design["background"].get_type() == Variant::DICTIONARY) {
		out["background"] = normalize_ui_background(Dictionary(p_design["background"]));
	}
	if (p_design.has("transition") && p_design["transition"].get_type() == Variant::DICTIONARY) {
		out["transition"] = normalize_ui_transition(Dictionary(p_design["transition"]));
	}
	String bgm_p = String(p_design.get("bgm_path", "")).strip_edges();
	if (!bgm_p.is_empty()) {
		out["bgm_path"] = bgm_p;
	}
	return out;
}

Dictionary ClockGearsDataManager::get_default_ui_designs_config() {
	Dictionary out;
	out["schema_version"] = CURRENT_UI_DESIGNS_SCHEMA_VERSION;
	Dictionary screens;
	PackedStringArray sids = get_system_menu_ids();
	for (int i = 0; i < sids.size(); i++) {
		if (sids[i] == "hud") {
			continue; // HUD se guarda en hud.json como fuente única de verdad
		}
		Dictionary def = get_default_ui_screen_design(sids[i]);
		def["order"] = i;
		screens[sids[i]] = def;
	}
	out["screens"] = screens;
	out["user_templates"] = Dictionary();
	out["recent_colors"] = Array();
	return out;
}

Dictionary ClockGearsDataManager::normalize_ui_designs_config(const Dictionary &p_cfg) {
	Dictionary out;
	out["schema_version"] = CURRENT_UI_DESIGNS_SCHEMA_VERSION;
	Dictionary screens;
	Dictionary raw_screens = (p_cfg.has("screens") && p_cfg["screens"].get_type() == Variant::DICTIONARY)
			? Dictionary(p_cfg["screens"])
			: Dictionary();
	PackedStringArray sys_sids = get_system_menu_ids();
	HashSet<String> seen_keys;
	for (int i = 0; i < sys_sids.size(); i++) {
		const String &sid = sys_sids[i];
		if (sid == "hud") {
			continue;
		}
		seen_keys.insert(sid);
		Dictionary sd = raw_screens.has(sid) && raw_screens[sid].get_type() == Variant::DICTIONARY
				? Dictionary(raw_screens[sid])
				: Dictionary();
		Dictionary norm = normalize_ui_screen_design(sid, sd);
		norm["order"] = i;
		screens[sid] = norm;
	}

	// Preservar e incluir TODOS los menús propios ilimitados
	Array raw_keys = raw_screens.keys();
	int next_custom_order = 0;
	for (int i = 0; i < raw_keys.size(); i++) {
		String raw_k = String(raw_keys[i]);
		String sid = _cg_norm_ui_screen_id(raw_k);
		if (sid == "hud" || seen_keys.has(sid)) {
			continue;
		}
		if (raw_screens[raw_keys[i]].get_type() != Variant::DICTIONARY) {
			continue;
		}
		seen_keys.insert(sid);
		Dictionary sd = Dictionary(raw_screens[raw_keys[i]]);
		Dictionary norm = normalize_ui_screen_design(sid, sd);
		if (!sd.has("order")) {
			norm["order"] = next_custom_order;
		}
		next_custom_order = MAX(next_custom_order + 1, int(norm["order"]) + 1);
		screens[sid] = norm;
	}
	out["screens"] = screens;
	if (p_cfg.has("user_templates") && p_cfg["user_templates"].get_type() == Variant::DICTIONARY) {
		out["user_templates"] = Dictionary(p_cfg["user_templates"]).duplicate(true);
	} else {
		out["user_templates"] = Dictionary();
	}
	Array rc;
	if (p_cfg.has("recent_colors") && p_cfg["recent_colors"].get_type() == Variant::ARRAY) {
		Array raw_rc = p_cfg["recent_colors"];
		for (int i = 0; i < MIN(raw_rc.size(), 16); i++) {
			rc.push_back(_cg_norm_color_rgba(raw_rc[i], "#e0a040ff"));
		}
	}
	out["recent_colors"] = rc;
	return out;
}

Dictionary ClockGearsDataManager::get_default_ui_screen_template(const String &p_screen_id) const {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Array elems;
	if (sid == "title") {
		Dictionary title_el = make_default_ui_element("text", sid);
		title_el["id"] = "title_heading";
		title_el["name"] = String::utf8("Título");
		title_el["data_binding"] = "project_title";
		title_el["anchor"] = "top_center";
		title_el["offset_x"] = 0;
		title_el["offset_y"] = 48;
		title_el["width"] = 560;
		title_el["height"] = 60;
		title_el["font_size"] = 36;
		title_el["align"] = "center";
		elems.push_back(title_el);

		Array vis = get_visible_title_entries();
		int y_cursor = 180;
		for (int i = 0; i < vis.size(); i++) {
			Dictionary e = vis[i];
			Dictionary b = make_default_ui_element("button", sid);
			String act = String(e.get("id", ""));
			b["id"] = "btn_" + act;
			b["name"] = String(e.get("text", act));
			b["text"] = String(e.get("text", act));
			b["action"] = act;
			b["anchor"] = "top_center";
			b["offset_x"] = 0;
			b["offset_y"] = y_cursor;
			b["width"] = 240;
			b["height"] = 40;
			b["focus_order"] = i;
			elems.push_back(b);
			y_cursor += 48;
		}
	} else if (sid == "game_menu_main") {
		auto add_box = [&](const String &p_id, const String &p_name, int p_x, int p_y, int p_w, int p_h, int p_layer) {
			Dictionary f = make_default_ui_element("frame", sid);
			f["id"] = p_id;
			f["name"] = p_name;
			f["anchor"] = "top_left";
			f["offset_x"] = p_x;
			f["offset_y"] = p_y;
			f["width"] = p_w;
			f["height"] = p_h;
			f["layer_order"] = p_layer;
			elems.push_back(f);
		};
		auto add_txt = [&](const String &p_id, const String &p_name, const String &p_text, int p_x, int p_y, int p_w, int p_h, int p_fsz, const String &p_align, const String &p_bind, int p_layer) {
			Dictionary t = make_default_ui_element("text", sid);
			t["id"] = p_id;
			t["name"] = p_name;
			t["text"] = p_text;
			t["anchor"] = "top_left";
			t["offset_x"] = p_x;
			t["offset_y"] = p_y;
			t["width"] = p_w;
			t["height"] = p_h;
			t["font_size"] = p_fsz;
			t["align"] = p_align;
			t["data_binding"] = p_bind;
			t["layer_order"] = p_layer;
			elems.push_back(t);
		};
		auto add_wid = [&](const String &p_id, const String &p_name, const String &p_kind, int p_x, int p_y, int p_w, int p_h, int p_layer) {
			Dictionary w = make_default_ui_element("widget", sid);
			w["id"] = p_id;
			w["name"] = p_name;
			w["widget_kind"] = p_kind;
			w["anchor"] = "top_left";
			w["offset_x"] = p_x;
			w["offset_y"] = p_y;
			w["width"] = p_w;
			w["height"] = p_h;
			w["layer_order"] = p_layer;
			elems.push_back(w);
		};

		add_box("menu_frame", "Panel Comandos", 24, 20, 220, 384, 0);
		add_txt("menu_heading", String::utf8("Título Menú"), String::utf8("Menú"), 12, 10, 196, 28, 18, "left", "none", 1);
		if (!elems.is_empty()) {
			Dictionary mh = elems[elems.size() - 1];
			mh["parent_id"] = "menu_frame";
			elems[elems.size() - 1] = mh;
		}

		Array entries = get_visible_game_menu_entries();
		int y_cursor = 46;
		for (int i = 0; i < entries.size(); i++) {
			Dictionary e = entries[i];
			Dictionary b = make_default_ui_element("button", sid);
			String act = String(e.get("id", ""));
			b["id"] = "menu_btn_" + act;
			b["parent_id"] = "menu_frame";
			b["name"] = String(e.get("text", act));
			b["text"] = String(e.get("text", act));
			b["action"] = act;
			b["anchor"] = "top_left";
			b["offset_x"] = 12;
			b["offset_y"] = y_cursor;
			b["width"] = 196;
			b["height"] = 34;
			b["layer_order"] = 2;
			b["focus_order"] = i;
			elems.push_back(b);
			y_cursor += 40;
		}
		Dictionary close_b = make_default_ui_element("button", sid);
		close_b["id"] = "menu_btn_close";
		close_b["parent_id"] = "menu_frame";
		close_b["name"] = "Cerrar (Esc)";
		close_b["text"] = "Cerrar (Esc)";
		close_b["action"] = "close";
		close_b["anchor"] = "top_left";
		close_b["offset_x"] = 12;
		close_b["offset_y"] = 338;
		close_b["width"] = 196;
		close_b["height"] = 34;
		close_b["layer_order"] = 2;
		close_b["focus_order"] = entries.size();
		elems.push_back(close_b);

		add_box("currency_time_frame", "Tarjeta Moneda y Tiempo", 24, 414, 220, 106, 0);
		Dictionary gold_el = make_default_ui_element("currency", sid);
		gold_el["id"] = "menu_gold";
		gold_el["parent_id"] = "currency_time_frame";
		gold_el["name"] = "Monedas";
		gold_el["data_binding"] = "gold";
		gold_el["anchor"] = "top_left";
		gold_el["offset_x"] = 12;
		gold_el["offset_y"] = 14;
		gold_el["width"] = 196;
		gold_el["height"] = 28;
		gold_el["layer_order"] = 1;
		elems.push_back(gold_el);
		add_txt("menu_playtime", "Tiempo jugado", "Tiempo: 00:00:00", 12, 54, 196, 26, 13, "right", "playtime", 1);
		{
			Dictionary mp = elems[elems.size() - 1];
			mp["parent_id"] = "currency_time_frame";
			elems[elems.size() - 1] = mp;
		}

		add_box("party_frame", "Panel Grupo", 260, 20, 676, 500, 0);
		add_txt("party_heading", String::utf8("Título Grupo"), "Grupo", 16, 10, 240, 28, 18, "left", "none", 1);
		{
			Dictionary ph = elems[elems.size() - 1];
			ph["parent_id"] = "party_frame";
			elems[elems.size() - 1] = ph;
		}
		add_wid("party_list", "Lista del grupo", "party_list", 14, 48, 648, 438, 1);
		{
			Dictionary pl = elems[elems.size() - 1];
			pl["parent_id"] = "party_frame";
			elems[elems.size() - 1] = pl;
		}
	} else if (sid == "game_menu_items" || sid == "game_menu_equip" || sid == "game_menu_status" || sid == "game_menu_save" || sid == "game_menu_options") {
		auto add_panel = [&](const String &p_id, const String &p_name, int p_x, int p_y, int p_w, int p_h, int p_layer) {
			Dictionary f = make_default_ui_element("frame", sid);
			f["id"] = p_id;
			f["name"] = p_name;
			f["anchor"] = "top_left";
			f["offset_x"] = p_x;
			f["offset_y"] = p_y;
			f["width"] = p_w;
			f["height"] = p_h;
			f["layer_order"] = p_layer;
			elems.push_back(f);
		};
		auto add_folder = [&](const String &p_id, const String &p_name, const String &p_parent_id, int p_x, int p_y, int p_layer) {
			Dictionary fl = make_default_ui_element("folder", sid);
			fl["id"] = p_id;
			fl["name"] = p_name;
			fl["parent_id"] = p_parent_id;
			fl["offset_x"] = p_x;
			fl["offset_y"] = p_y;
			fl["layer_order"] = p_layer;
			elems.push_back(fl);
		};
		auto add_hdr = [&](const String &p_id, const String &p_name, const String &p_text, int p_x, int p_y, int p_w, int p_h, int p_fsz, int p_layer, const String &p_parent_id = "") {
			Dictionary h = make_default_ui_element("text", sid);
			h["id"] = p_id;
			h["parent_id"] = p_parent_id;
			h["name"] = p_name;
			h["text"] = p_text;
			h["anchor"] = "top_left";
			h["offset_x"] = p_x;
			h["offset_y"] = p_y;
			h["width"] = p_w;
			h["height"] = p_h;
			h["font_size"] = p_fsz;
			h["layer_order"] = p_layer;
			elems.push_back(h);
		};
		auto add_widget = [&](const String &p_id, const String &p_name, const String &p_kind, int p_x, int p_y, int p_w, int p_h, int p_layer, const String &p_parent_id = "") {
			Dictionary w = make_default_ui_element("widget", sid);
			w["id"] = p_id;
			w["parent_id"] = p_parent_id;
			w["name"] = p_name;
			w["widget_kind"] = p_kind;
			w["anchor"] = "top_left";
			w["offset_x"] = p_x;
			w["offset_y"] = p_y;
			w["width"] = p_w;
			w["height"] = p_h;
			w["layer_order"] = p_layer;
			elems.push_back(w);
		};
		auto add_act_btn = [&](const String &p_id, const String &p_text, const String &p_act, int p_x, int p_y, int p_w, int p_h, int p_focus, const String &p_parent_id = "") {
			Dictionary b = make_default_ui_element("button", sid);
			b["id"] = p_id;
			b["parent_id"] = p_parent_id;
			b["name"] = p_text;
			b["text"] = p_text;
			b["action"] = p_act;
			b["anchor"] = "top_left";
			b["offset_x"] = p_x;
			b["offset_y"] = p_y;
			b["width"] = p_w;
			b["height"] = p_h;
			b["layer_order"] = 2;
			b["focus_order"] = p_focus;
			elems.push_back(b);
		};

		if (sid == "game_menu_items") {
			add_panel("items_panel_frame", "Panel Objetos", 24, 20, 912, 500, 0);
			add_hdr("sub_heading", "Encabezado", "Objetos", 16, 12, 150, 34, 22, 1, "items_panel_frame");
			add_widget("items_target_selector", "Selector de objetivo", "items_target", 176, 12, 320, 34, 1, "items_panel_frame");
			add_act_btn("btn_back", "Volver", "back", 786, 12, 110, 34, 2, "items_panel_frame");
			add_widget("items_list", "Lista de objetos", "items_list", 16, 58, 568, 426, 1, "items_panel_frame");
			add_folder("col_items_details", "Columna detalles", "items_panel_frame", 596, 58, 1);
			add_widget("item_preview", "Vista previa del objeto", "item_preview", 0, 0, 300, 140, 1, "col_items_details");
			add_widget("item_details", String::utf8("Descripción del objeto"), "item_details", 0, 148, 300, 228, 1, "col_items_details");
			add_act_btn("btn_item_use", "Usar", "item_use", 12, 384, 132, 36, 0, "col_items_details");
			add_act_btn("btn_item_discard", "Tirar", "item_discard", 156, 384, 132, 36, 1, "col_items_details");
		} else if (sid == "game_menu_equip") {
			add_panel("equip_panel_frame", "Panel Equipo", 24, 20, 912, 500, 0);
			add_hdr("sub_heading", "Encabezado", "Equipo", 12, 10, 140, 34, 20, 1, "equip_panel_frame");
			add_widget("equip_target_selector", "Selector de personaje", "equip_target", 160, 10, 260, 34, 1, "equip_panel_frame");
			add_act_btn("btn_back", "Volver", "back", 790, 10, 110, 34, 2, "equip_panel_frame");
			add_folder("col_equip_slots", "Columna ranuras", "equip_panel_frame", 12, 53, 1);
			add_widget("equip_slots", "Huecos de equipo", "equip_slots", 0, 0, 304, 390, 1, "col_equip_slots");
			add_act_btn("btn_equip_clear", "Desequipar", "equip_clear", 0, 397, 304, 38, 0, "col_equip_slots");
			add_folder("col_equip_inventory", "Columna inventario", "equip_panel_frame", 326, 53, 1);
			add_widget("equip_candidates", "Inventario compatible", "equip_candidates", 0, 0, 304, 390, 1, "col_equip_inventory");
			add_act_btn("btn_equip_apply", "Equipar", "equip_apply", 0, 397, 304, 38, 1, "col_equip_inventory");
			add_folder("col_equip_details", "Columna detalles", "equip_panel_frame", 640, 53, 1);
			add_widget("equip_item_preview", "Vista previa pieza", "equip_preview", 0, 0, 260, 100, 1, "col_equip_details");
			add_widget("equip_desc", "Detalles pieza", "equip_desc", 0, 106, 260, 107, 1, "col_equip_details");
			add_widget("equip_params", String::utf8("Parámetros"), "equip_params", 0, 219, 260, 218, 1, "col_equip_details");
		} else if (sid == "game_menu_status") {
			add_panel("status_panel_frame", "Panel Estado", 24, 20, 912, 500, 0);
			add_hdr("sub_heading", "Encabezado", "Estado", 16, 12, 140, 34, 22, 1, "status_panel_frame");
			add_widget("status_target_selector", "Selector de personaje", "status_target", 166, 12, 320, 34, 1, "status_panel_frame");
			add_act_btn("btn_back", "Volver", "back", 786, 12, 110, 34, 0, "status_panel_frame");
			add_widget("status_basic", String::utf8("Retrato y datos básicos"), "status_basic", 16, 58, 430, 176, 1, "status_panel_frame");
			add_widget("status_equips", "Equipo equipado", "status_equips", 16, 244, 430, 240, 1, "status_panel_frame");
			add_widget("status_params", "Atributos y afinidades", "status_params", 462, 58, 434, 426, 1, "status_panel_frame");
		} else if (sid == "game_menu_save") {
			add_panel("save_panel_frame", "Panel Partidas", 24, 20, 912, 500, 0);
			add_hdr("sub_heading", "Encabezado", "Guardar partida", 16, 12, 480, 36, 22, 1, "save_panel_frame");
			add_widget("save_slots", "Ranuras de guardado", "save_slots", 16, 58, 880, 382, 1, "save_panel_frame");
			add_act_btn("btn_back", "Volver", "back", 16, 448, 160, 36, 0, "save_panel_frame");
		} else {
			add_panel("options_panel_frame", "Panel Opciones", 24, 20, 912, 500, 0);
			add_hdr("sub_heading", "Encabezado", "Opciones", 16, 12, 480, 36, 22, 1, "options_panel_frame");
			add_widget("options_controls", "Controles de opciones", "options_controls", 16, 58, 880, 382, 1, "options_panel_frame");
			add_act_btn("btn_back", "Volver", "back", 16, 448, 160, 36, 0, "options_panel_frame");
		}
	} else if (sid == "credits") {
		Dictionary chdr = make_default_ui_element("text", sid);
		chdr["id"] = "credits_heading";
		chdr["name"] = String::utf8("Título Créditos");
		chdr["text"] = String::utf8("Créditos");
		chdr["anchor"] = "top_center";
		chdr["offset_x"] = 0;
		chdr["offset_y"] = 42;
		chdr["width"] = 480;
		chdr["height"] = 44;
		chdr["font_size"] = 30;
		chdr["align"] = "center";
		elems.push_back(chdr);

		Dictionary cbody = make_default_ui_element("text", sid);
		cbody["id"] = "credits_body";
		cbody["name"] = String::utf8("Lista de créditos");
		cbody["text"] = String::utf8("Dirección · Arte · Música\nClockGears Studio");
		cbody["anchor"] = "center";
		cbody["offset_x"] = 0;
		cbody["offset_y"] = 0;
		cbody["width"] = 620;
		cbody["height"] = 220;
		cbody["font_size"] = 18;
		cbody["align"] = "center";
		elems.push_back(cbody);

		Dictionary back_b = make_default_ui_element("button", sid);
		back_b["id"] = "btn_credits_back";
		back_b["name"] = "Volver";
		back_b["text"] = "Volver";
		back_b["action"] = "back";
		back_b["anchor"] = "bottom_right";
		back_b["offset_x"] = -20;
		back_b["offset_y"] = -20;
		back_b["width"] = 140;
		back_b["height"] = 36;
		back_b["focus_order"] = 0;
		elems.push_back(back_b);
	} else if (sid == "hud") {
		Dictionary frame_el = make_default_ui_element("frame", sid);
		frame_el["id"] = "hud_panel";
		frame_el["name"] = "Panel Estado HUD";
		frame_el["anchor"] = "top_left";
		frame_el["offset_x"] = 16;
		frame_el["offset_y"] = 16;
		frame_el["width"] = 240;
		frame_el["height"] = 112;
		frame_el["layer_order"] = 0;
		elems.push_back(frame_el);

		Dictionary hp_bar = make_default_ui_element("bar", sid);
		hp_bar["id"] = "hud_hp_bar";
		hp_bar["parent_id"] = "hud_panel";
		hp_bar["name"] = "Barra PV";
		hp_bar["label_text"] = "PV";
		hp_bar["data_binding"] = "player_hp";
		hp_bar["anchor"] = "top_left";
		hp_bar["offset_x"] = 12;
		hp_bar["offset_y"] = 12;
		hp_bar["width"] = 216;
		hp_bar["height"] = 24;
		hp_bar["layer_order"] = 1;
		elems.push_back(hp_bar);

		Dictionary mp_bar = make_default_ui_element("bar", sid);
		mp_bar["id"] = "hud_mp_bar";
		mp_bar["parent_id"] = "hud_panel";
		mp_bar["name"] = "Barra PM";
		mp_bar["label_text"] = "PM";
		mp_bar["data_binding"] = "player_mp";
		mp_bar["anchor"] = "top_left";
		mp_bar["offset_x"] = 12;
		mp_bar["offset_y"] = 44;
		mp_bar["width"] = 216;
		mp_bar["height"] = 22;
		mp_bar["layer_order"] = 1;
		elems.push_back(mp_bar);

		Dictionary gold_el = make_default_ui_element("currency", sid);
		gold_el["id"] = "hud_gold";
		gold_el["parent_id"] = "hud_panel";
		gold_el["name"] = "Monedas";
		gold_el["data_binding"] = "gold";
		gold_el["anchor"] = "top_left";
		gold_el["offset_x"] = 12;
		gold_el["offset_y"] = 74;
		gold_el["width"] = 216;
		gold_el["height"] = 28;
		gold_el["layer_order"] = 1;
		elems.push_back(gold_el);
	} else if (sid == "game_over") {
		Dictionary go = normalize_game_over_config(game_over_config);
		Dictionary gh = make_default_ui_element("text", sid);
		gh["id"] = "game_over_title";
		gh["name"] = String::utf8("Título Fin del Juego");
		gh["text"] = String(go.get("title_text", "GAME OVER"));
		gh["anchor"] = "top_center";
		gh["offset_x"] = 0;
		gh["offset_y"] = 96;
		gh["width"] = 620;
		gh["height"] = 72;
		gh["font_size"] = int(go.get("title_font_size", 48));
		gh["font_color"] = String(go.get("title_color", "#e84a5f"));
		gh["align"] = "center";
		elems.push_back(gh);

		Dictionary rb = make_default_ui_element("button", sid);
		rb["id"] = "btn_game_over_retry";
		rb["name"] = String::utf8("Botón Reintentar");
		rb["text"] = String(go.get("retry_label", "Reintentar"));
		rb["visible"] = bool(go.get("retry_enabled", true));
		rb["anchor"] = "bottom_center";
		rb["offset_x"] = 0;
		rb["offset_y"] = -140;
		rb["width"] = 240;
		rb["height"] = 42;
		rb["focus_order"] = 0;
		elems.push_back(rb);

		Dictionary tb = make_default_ui_element("button", sid);
		tb["id"] = "btn_game_over_title";
		tb["name"] = String::utf8("Botón Volver al título");
		tb["text"] = String(go.get("title_label", String::utf8("Volver al menú")));
		tb["visible"] = bool(go.get("title_enabled", true));
		tb["anchor"] = "bottom_center";
		tb["offset_x"] = 0;
		tb["offset_y"] = -86;
		tb["width"] = 240;
		tb["height"] = 42;
		tb["focus_order"] = 1;
		elems.push_back(tb);
	} else if (sid == "messages_battle") {
		Dictionary mfr = make_default_ui_element("frame", sid);
		mfr["id"] = "msg_box_frame";
		mfr["name"] = String::utf8("Cuadro de diálogo");
		mfr["anchor"] = "bottom_center";
		mfr["offset_x"] = 0;
		mfr["offset_y"] = -16;
		mfr["width"] = 880;
		mfr["height"] = 156;
		mfr["window_style_id"] = "dialog";
		mfr["layer_order"] = 0;
		elems.push_back(mfr);

		Dictionary por = make_default_ui_element("image", sid);
		por["id"] = "msg_portrait";
		por["parent_id"] = "msg_box_frame";
		por["name"] = "Retrato hablante";
		por["anchor"] = "top_left";
		por["offset_x"] = 16;
		por["offset_y"] = 32;
		por["width"] = 96;
		por["height"] = 96;
		por["layer_order"] = 2;
		elems.push_back(por);

		Dictionary spk = make_default_ui_element("text", sid);
		spk["id"] = "msg_speaker_lbl";
		spk["parent_id"] = "msg_box_frame";
		spk["name"] = "Nombre hablante";
		spk["text"] = "Personaje";
		spk["anchor"] = "top_left";
		spk["offset_x"] = 128;
		spk["offset_y"] = 16;
		spk["width"] = 300;
		spk["height"] = 28;
		spk["font_size"] = 20;
		spk["layer_order"] = 2;
		elems.push_back(spk);

		Dictionary bdy = make_default_ui_element("text", sid);
		bdy["id"] = "msg_body_lbl";
		bdy["parent_id"] = "msg_box_frame";
		bdy["name"] = "Texto mensaje";
		bdy["text"] = String::utf8("Texto de diálogo o aviso de batalla...");
		bdy["anchor"] = "top_left";
		bdy["offset_x"] = 128;
		bdy["offset_y"] = 52;
		bdy["width"] = 700;
		bdy["height"] = 86;
		bdy["font_size"] = 18;
		bdy["layer_order"] = 2;
		elems.push_back(bdy);

		Dictionary bfr = make_default_ui_element("frame", sid);
		bfr["id"] = "battle_cmd_frame";
		bfr["name"] = "Marco comandos batalla";
		bfr["anchor"] = "top_right";
		bfr["offset_x"] = -24;
		bfr["offset_y"] = 24;
		bfr["width"] = 220;
		bfr["height"] = 212;
		bfr["window_style_id"] = "game_menu";
		bfr["layer_order"] = 0;
		elems.push_back(bfr);

		const char *b_lbls[4] = { "Atacar", "Habilidad", "Objeto", "Huir" };
		const char *b_acts[4] = { "close", "open_system_screen", "open_system_screen", "close" };
		for (int i = 0; i < 4; i++) {
			Dictionary bb = make_default_ui_element("button", sid);
			bb["id"] = vformat("btn_battle_%d", i + 1);
			bb["parent_id"] = "battle_cmd_frame";
			bb["name"] = String::utf8(b_lbls[i]);
			bb["text"] = String::utf8(b_lbls[i]);
			bb["action"] = String(b_acts[i]);
			if (i == 2) {
				bb["target_screen"] = "game_menu_items";
			}
			bb["anchor"] = "top_left";
			bb["offset_x"] = 16;
			bb["offset_y"] = 16 + i * 46;
			bb["width"] = 188;
			bb["height"] = 38;
			bb["focus_order"] = i;
			bb["layer_order"] = 2;
			elems.push_back(bb);
		}
	} else {
		Dictionary close_b = make_default_ui_element("button", sid);
		close_b["id"] = "btn_close";
		close_b["name"] = "Cerrar";
		close_b["text"] = "Cerrar";
		close_b["action"] = "close";
		close_b["anchor"] = "bottom_right";
		close_b["offset_x"] = -24;
		close_b["offset_y"] = -24;
		close_b["width"] = 140;
		close_b["height"] = 36;
		close_b["focus_order"] = 0;
		elems.push_back(close_b);
	}
	Dictionary out;
	out["screen_id"] = sid;
	out["custom_enabled"] = true;
	out["elements"] = elems;
	return normalize_ui_screen_design(sid, out);
}

Array ClockGearsDataManager::get_all_menus() const {
	Array out;
	PackedStringArray sys_sids = get_system_menu_ids();
	for (int i = 0; i < sys_sids.size(); i++) {
		const String &sid = sys_sids[i];
		Dictionary meta = get_menu_metadata(sid);
		meta["supports_focus"] = (sid != "hud");
		out.push_back(meta);
	}
	PackedStringArray custom_ids = get_custom_menu_ids();
	for (int i = 0; i < custom_ids.size(); i++) {
		Dictionary meta = get_menu_metadata(custom_ids[i]);
		meta["supports_focus"] = true;
		out.push_back(meta);
	}
	return out;
}

PackedStringArray ClockGearsDataManager::get_custom_menu_ids() const {
	Dictionary screens = ui_designs_config.get("screens", Dictionary());
	struct CustomItem {
		String id;
		int order = 0;
	};
	Vector<CustomItem> list;
	Array keys = screens.keys();
	for (int i = 0; i < keys.size(); i++) {
		String sid = _cg_norm_ui_screen_id(String(keys[i]));
		if (_cg_is_system_menu_id(sid)) {
			continue;
		}
		Dictionary d = screens[keys[i]].get_type() == Variant::DICTIONARY ? Dictionary(screens[keys[i]]) : Dictionary();
		CustomItem it;
		it.id = sid;
		it.order = int(d.get("order", i));
		list.push_back(it);
	}
	struct OrderCmp {
		bool operator()(const CustomItem &a, const CustomItem &b) const {
			if (a.order != b.order) return a.order < b.order;
			return a.id < b.id;
		}
	};
	list.sort_custom<OrderCmp>();
	PackedStringArray out;
	for (int i = 0; i < list.size(); i++) {
		out.push_back(list[i].id);
	}
	return out;
}

Array ClockGearsDataManager::get_all_menus_for_selector() const {
	return get_all_menus();
}

bool ClockGearsDataManager::has_menu(const String &p_menu_id) const {
	String sid = _cg_norm_ui_screen_id(p_menu_id);
	if (sid.is_empty()) {
		return false;
	}
	if (_cg_is_system_menu_id(sid)) {
		return true;
	}
	Dictionary screens = ui_designs_config.get("screens", Dictionary());
	return screens.has(sid);
}

String ClockGearsDataManager::get_menu_system_group(const String &p_menu_id) const {
	String cur = _cg_norm_ui_screen_id(p_menu_id);
	if (cur.is_empty()) {
		return "";
	}
	if (_cg_is_system_menu_id(cur)) {
		return cur;
	}
	Dictionary screens = ui_designs_config.get("screens", Dictionary());
	for (int hop = 0; hop < 8; hop++) {
		if (!screens.has(cur) || screens[cur].get_type() != Variant::DICTIONARY) {
			break;
		}
		Dictionary d = screens[cur];
		String sg = String(d.get("system_group", "")).strip_edges();
		String pid = String(d.get("parent_id", "")).strip_edges();
		if (pid.is_empty()) {
			return sg.is_empty() ? String("") : _cg_norm_ui_screen_id(sg);
		}
		pid = _cg_norm_ui_screen_id(pid);
		if (_cg_is_system_menu_id(pid)) {
			return pid;
		}
		if (!sg.is_empty()) {
			return _cg_norm_ui_screen_id(sg);
		}
		cur = pid;
	}
	return "";
}

Dictionary ClockGearsDataManager::get_menu_metadata(const String &p_menu_id) const {
	String sid = _cg_norm_ui_screen_id(p_menu_id);
	Dictionary d;
	if (sid == "hud") {
		d["name"] = _cg_default_system_menu_name(sid);
		d["window_type"] = _cg_default_system_menu_win_type(sid);
		Array w = hud_config.get("widgets", Array());
		d["custom_enabled"] = !w.is_empty();
		d["pause_on_open"] = false;
	} else {
		Dictionary screens = ui_designs_config.get("screens", Dictionary());
		if (screens.has(sid) && screens[sid].get_type() == Variant::DICTIONARY) {
			d = screens[sid];
		} else {
			d = get_default_ui_screen_design(sid);
		}
	}
	Dictionary out;
	out["id"] = sid;
	out["screen_id"] = sid;
	out["name"] = String(d.get("name", _cg_is_system_menu_id(sid) ? _cg_default_system_menu_name(sid) : sid));
	out["is_system"] = _cg_is_system_menu_id(sid);
	out["order"] = int(d.get("order", 0));
	out["parent_id"] = _cg_is_system_menu_id(sid) ? String("") : String(d.get("parent_id", ""));
	out["system_group"] = get_menu_system_group(sid);
	out["window_type"] = String(d.get("window_type", _cg_default_system_menu_win_type(sid)));
	out["custom_enabled"] = bool(d.get("custom_enabled", false));
	out["triggers"] = Array(d.get("triggers", Array())).duplicate(true);
	out["trigger_context"] = String(d.get("trigger_context", "map"));
	out["pause_on_open"] = bool(d.get("pause_on_open", sid != "hud"));
	out["respect_menu_access"] = bool(d.get("respect_menu_access", true));
	Dictionary lay = d.get("layout", Dictionary());
	out["layout_mode"] = String(lay.get("mode", "free"));
	return out;
}

String ClockGearsDataManager::create_custom_menu(const String &p_name, const Dictionary &p_initial_design) {
	String nm = p_name.strip_edges();
	if (nm.is_empty()) {
		nm = String::utf8("Menú personalizado");
	}
	if (!ui_designs_config.has("screens") || ui_designs_config["screens"].get_type() != Variant::DICTIONARY) {
		ui_designs_config = normalize_ui_designs_config(ui_designs_config);
	}
	Dictionary screens = Dictionary(ui_designs_config["screens"]).duplicate(true);

	String base = _cg_slugify_menu_id(nm);
	if (base.is_empty() || _cg_is_system_menu_id(base)) {
		base = base.is_empty() ? String("menu_propio") : ("custom_" + base);
	}
	String uid = base;
	int suffix = 2;
	while (screens.has(uid) || _cg_is_system_menu_id(uid)) {
		uid = base + "_" + itos(suffix++);
	}

	PackedStringArray existing_custom = get_custom_menu_ids();
	int max_order = -1;
	for (int i = 0; i < existing_custom.size(); i++) {
		Dictionary m = get_ui_screen_design(existing_custom[i]);
		max_order = MAX(max_order, int(m.get("order", i)));
	}

	Dictionary cfg = p_initial_design.duplicate(true);
	cfg["screen_id"] = uid;
	cfg["name"] = nm;
	cfg["is_system"] = false;
	cfg["custom_enabled"] = true;
	if (!cfg.has("order")) {
		cfg["order"] = max_order + 1;
	}
	Dictionary norm = normalize_ui_screen_design(uid, cfg);
	screens[uid] = norm;
	ui_designs_config["screens"] = screens;
	dirty = true;
	emit_signal("data_changed", "ui_designs", 0);
	emit_signal("runtime_state_changed", "ui_design", uid);
	return uid;
}

String ClockGearsDataManager::create_submenu_design(const String &p_parent_id, const String &p_name, const Dictionary &p_initial_design) {
	String parent = _cg_norm_ui_screen_id(p_parent_id);
	if (parent.is_empty() || !has_menu(parent)) {
		parent = "game_menu_main";
	}
	Dictionary pmeta = get_menu_metadata(parent);
	String pname = String(pmeta.get("name", parent));
	String nm = p_name.strip_edges();
	if (nm.is_empty()) {
		nm = String::utf8("Submenú de ") + pname;
	}
	Dictionary init = p_initial_design.duplicate(true);
	init["parent_id"] = parent;
	String sg = get_menu_system_group(parent);
	init["system_group"] = sg.is_empty() ? parent : sg;
	init["window_type"] = String(init.get("window_type", pmeta.get("window_type", "game_menu")));
	if (!init.has("elements") || Array(init["elements"]).is_empty()) {
		Array els;
		Dictionary fr = make_default_ui_element("frame", parent);
		fr["id"] = "submenu_frame";
		fr["name"] = "Panel Submenu";
		fr["anchor"] = "center";
		fr["offset_x"] = 0;
		fr["offset_y"] = 0;
		fr["width"] = 480;
		fr["height"] = 300;
		els.push_back(fr);

		Dictionary tit = make_default_ui_element("text", parent);
		tit["id"] = "submenu_title";
		tit["name"] = String::utf8("Título Submenú");
		tit["text"] = nm;
		tit["anchor"] = "center";
		tit["offset_x"] = 0;
		tit["offset_y"] = -114;
		tit["width"] = 420;
		tit["height"] = 34;
		tit["align"] = "center";
		tit["font_size"] = 22;
		tit["layer_order"] = 2;
		els.push_back(tit);

		Dictionary back_b = make_default_ui_element("button", parent);
		back_b["id"] = "btn_back";
		back_b["name"] = "Volver";
		back_b["text"] = "Volver";
		back_b["action"] = "back";
		back_b["anchor"] = "center";
		back_b["offset_x"] = 0;
		back_b["offset_y"] = 96;
		back_b["width"] = 180;
		back_b["height"] = 40;
		back_b["focus_order"] = 0;
		back_b["layer_order"] = 2;
		els.push_back(back_b);
		init["elements"] = els;
	}
	return create_custom_menu(nm, init);
}

bool ClockGearsDataManager::rename_menu(const String &p_menu_id, const String &p_new_name) {
	String sid = _cg_norm_ui_screen_id(p_menu_id);
	String nm = p_new_name.strip_edges();
	if (nm.is_empty() || sid == "hud") {
		return false;
	}
	Dictionary d = get_ui_screen_design(sid);
	d["name"] = nm;
	set_ui_screen_design(sid, d, true);
	return true;
}

String ClockGearsDataManager::duplicate_menu(const String &p_source_id, const String &p_new_name) {
	String src_id = _cg_norm_ui_screen_id(p_source_id);
	Dictionary src = get_ui_screen_design(src_id);
	String nn = p_new_name.strip_edges();
	if (nn.is_empty()) {
		nn = String(src.get("name", src_id)) + " (Copia)";
	}
	Dictionary copy = src.duplicate(true);
	copy["screen_id"] = "";
	copy["name"] = nn;
	copy["is_system"] = false;
	copy["custom_enabled"] = true;
	if (_cg_is_system_menu_id(src_id)) {
		copy["parent_id"] = src_id;
		copy["system_group"] = src_id;
	} else {
		copy["parent_id"] = String(src.get("parent_id", ""));
		String sg = get_menu_system_group(src_id);
		if (!sg.is_empty()) {
			copy["system_group"] = sg;
		}
	}
	copy.erase("order");
	return create_custom_menu(nn, copy);
}

bool ClockGearsDataManager::reorder_custom_menu(const String &p_menu_id, int p_new_order) {
	String sid = _cg_norm_ui_screen_id(p_menu_id);
	if (_cg_is_system_menu_id(sid)) {
		return false;
	}
	PackedStringArray cids = get_custom_menu_ids();
	int cur_idx = -1;
	for (int i = 0; i < cids.size(); i++) {
		if (cids[i] == sid) {
			cur_idx = i;
			break;
		}
	}
	if (cur_idx < 0) {
		return false;
	}
	Vector<String> ordered;
	for (int i = 0; i < cids.size(); i++) {
		if (i != cur_idx) ordered.push_back(cids[i]);
	}
	int dst = CLAMP(p_new_order, 0, ordered.size());
	ordered.insert(dst, sid);

	Dictionary screens = Dictionary(ui_designs_config.get("screens", Dictionary())).duplicate(true);
	for (int i = 0; i < ordered.size(); i++) {
		Dictionary d = normalize_ui_screen_design(ordered[i], Dictionary(screens.get(ordered[i], Dictionary())));
		d["order"] = i;
		screens[ordered[i]] = d;
	}
	ui_designs_config["screens"] = screens;
	dirty = true;
	emit_signal("data_changed", "ui_designs", 0);
	return true;
}

bool ClockGearsDataManager::move_custom_menu(const String &p_menu_id, int p_delta) {
	if (p_delta == 0) {
		return false;
	}
	String sid = _cg_norm_ui_screen_id(p_menu_id);
	PackedStringArray cids = get_custom_menu_ids();
	int idx = -1;
	for (int i = 0; i < cids.size(); i++) {
		if (cids[i] == sid) {
			idx = i;
			break;
		}
	}
	if (idx < 0) {
		return false;
	}
	int target = CLAMP(idx + p_delta, 0, cids.size() - 1);
	if (target == idx) {
		return false;
	}
	return reorder_custom_menu(sid, target);
}

bool ClockGearsDataManager::delete_custom_menu(const String &p_menu_id) {
	String sid = _cg_norm_ui_screen_id(p_menu_id);
	if (_cg_is_system_menu_id(sid)) {
		return false; // Los menús del sistema no se pueden borrar (3A)
	}
	if (!ui_designs_config.has("screens") || ui_designs_config["screens"].get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary screens = Dictionary(ui_designs_config["screens"]).duplicate(true);
	if (!screens.has(sid)) {
		return false;
	}
	String old_parent;
	if (screens[sid].get_type() == Variant::DICTIONARY) {
		old_parent = String(Dictionary(screens[sid]).get("parent_id", ""));
	}
	Array keys = screens.keys();
	for (int i = 0; i < keys.size(); i++) {
		String k = String(keys[i]);
		if (k != sid && screens[k].get_type() == Variant::DICTIONARY) {
			Dictionary cd = Dictionary(screens[k]);
			if (String(cd.get("parent_id", "")) == sid) {
				Dictionary upd = cd.duplicate(true);
				upd["parent_id"] = old_parent;
				screens[k] = upd;
			}
		}
	}
	screens.erase(sid);
	ui_designs_config["screens"] = screens;
	ui_design_undo_stacks.erase(sid);
	ui_design_redo_stacks.erase(sid);
	dirty = true;
	emit_signal("data_changed", "ui_designs", 0);
	emit_signal("runtime_state_changed", "ui_design", sid);
	return true;
}

bool ClockGearsDataManager::restore_system_menu_template(const String &p_menu_id, bool p_enable_custom_design) {
	String sid = _cg_norm_ui_screen_id(p_menu_id);
	if (!_cg_is_system_menu_id(sid)) {
		return false;
	}
	if (p_enable_custom_design) {
		Dictionary tpl = get_default_ui_screen_template(sid);
		set_ui_screen_design(sid, tpl, true);
	} else {
		reset_ui_screen_design_to_default(sid, true);
	}
	return true;
}

static bool _cg_menu_contexts_overlap(const String &p_a, const String &p_b) {
	if (p_a == "any" || p_b == "any" || p_a == p_b) {
		return true;
	}
	if ((p_a == "map" && p_b == "map_and_battle") || (p_b == "map" && p_a == "map_and_battle")) {
		return true;
	}
	return false;
}

static String _cg_trigger_signature(const Dictionary &p_trig, String &r_label) {
	String kind = String(p_trig.get("kind", "key"));
	if (kind == "key") {
		int64_t kc = int64_t(p_trig.get("keycode", 0));
		String kn = String(p_trig.get("key_name", ""));
		if (kn.is_empty() && kc > 0) kn = keycode_get_string((Key)kc);
		r_label = "Tecla " + kn;
		return (kc > 0) ? ("key:" + String::num_int64(kc)) : String();
	}
	if (kind == "joy_button") {
		int jb = int(p_trig.get("joy_button", 0));
		r_label = vformat("Mando B%d", jb);
		return "joy:" + itos(jb);
	}
	if (kind == "mouse_button") {
		int mb = int(p_trig.get("mouse_button", 1));
		r_label = vformat(String::utf8("Ratón B%d"), mb);
		return "mouse:" + itos(mb);
	}
	String an = String(p_trig.get("action_name", "")).strip_edges();
	r_label = vformat(String::utf8("Acción '%s'"), an);
	return an.is_empty() ? String() : ("action:" + an.to_lower());
}

Array ClockGearsDataManager::get_menu_trigger_conflicts() const {
	Array menus = get_all_menus();
	struct MenuTrigEntry {
		String menu_id;
		String menu_name;
		String context;
		String sig;
		String label;
	};
	Vector<MenuTrigEntry> entries;
	for (int i = 0; i < menus.size(); i++) {
		Dictionary m = menus[i];
		String mid = String(m.get("id", ""));
		String mname = String(m.get("name", mid));
		String mctx = String(m.get("trigger_context", "map"));
		Array trigs = m.get("triggers", Array());
		for (int t = 0; t < trigs.size(); t++) {
			if (trigs[t].get_type() != Variant::DICTIONARY) continue;
			String lbl;
			String sig = _cg_trigger_signature(Dictionary(trigs[t]), lbl);
			if (sig.is_empty()) continue;
			entries.push_back({ mid, mname, mctx, sig, lbl });
		}
	}

	Array conflicts;
	HashSet<String> reported_pairs;
	for (int i = 0; i < entries.size(); i++) {
		for (int j = i + 1; j < entries.size(); j++) {
			if (entries[i].menu_id == entries[j].menu_id) continue;
			if (entries[i].sig != entries[j].sig) continue;
			if (!_cg_menu_contexts_overlap(entries[i].context, entries[j].context)) continue;

			String a = entries[i].menu_id < entries[j].menu_id ? entries[i].menu_id : entries[j].menu_id;
			String b = entries[i].menu_id < entries[j].menu_id ? entries[j].menu_id : entries[i].menu_id;
			String pkey = a + "|" + b + "|" + entries[i].sig;
			if (reported_pairs.has(pkey)) continue;
			reported_pairs.insert(pkey);

			Dictionary c;
			Array mids;
			mids.push_back(entries[i].menu_id);
			mids.push_back(entries[j].menu_id);
			Array mnames;
			mnames.push_back(entries[i].menu_name);
			mnames.push_back(entries[j].menu_name);
			c["signature"] = entries[i].sig;
			c["trigger_label"] = entries[i].label;
			c["menu_ids"] = mids;
			c["menu_names"] = mnames;
			c["context_a"] = entries[i].context;
			c["context_b"] = entries[j].context;
			c["message"] = vformat(String::utf8("Conflicto de entrada (%s): '%s' y '%s' comparten entrada en el mismo contexto."),
					entries[i].label, entries[i].menu_name, entries[j].menu_name);
			conflicts.push_back(c);
		}
	}
	return conflicts;
}

PackedStringArray ClockGearsDataManager::get_last_migrated_files() const {
	PackedStringArray out;
	for (int i = 0; i < last_migrated_files.size(); i++) {
		out.push_back(last_migrated_files[i]);
	}
	return out;
}

bool ClockGearsDataManager::has_custom_ui_design(const String &p_screen_id) const {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	if (sid == "hud") {
		Array w = hud_config.get("widgets", Array());
		return !w.is_empty();
	}
	Dictionary d = get_ui_screen_design(sid);
	return bool(d.get("custom_enabled", false));
}

Dictionary ClockGearsDataManager::get_ui_screen_design(const String &p_screen_id) const {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	if (sid == "hud") {
		Dictionary d;
		d["screen_id"] = "hud";
		Array widgets = get_hud_widgets();
		Array elems;
		for (int i = 0; i < widgets.size(); i++) {
			if (widgets[i].get_type() == Variant::DICTIONARY) {
				elems.push_back(normalize_ui_element(Dictionary(widgets[i]), i + 1, "hud"));
			}
		}
		d["elements"] = elems;
		d["custom_enabled"] = !elems.is_empty();
		return d;
	}
	Dictionary screens = ui_designs_config.get("screens", Dictionary());
	if (screens.has(sid) && screens[sid].get_type() == Variant::DICTIONARY) {
		return Dictionary(screens[sid]).duplicate(true);
	}
	return get_default_ui_screen_design(sid);
}

void ClockGearsDataManager::_push_ui_undo_snapshot_internal(const String &p_screen_id, const Dictionary &p_snapshot, bool p_clear_redo) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Vector<Dictionary> &stack = ui_design_undo_stacks[sid];
	stack.push_back(p_snapshot.duplicate(true));
	while (stack.size() > MAX_UI_DESIGN_UNDO_STEPS) {
		stack.remove_at(0);
	}
	if (p_clear_redo && ui_design_redo_stacks.has(sid)) {
		ui_design_redo_stacks[sid].clear();
	}
}

void ClockGearsDataManager::set_ui_screen_design(const String &p_screen_id, const Dictionary &p_design, bool p_record_undo) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary prev = get_ui_screen_design(sid);
	Dictionary norm = normalize_ui_screen_design(sid, p_design);
	if (p_record_undo && prev != norm) {
		_push_ui_undo_snapshot_internal(sid, prev, true);
	}

	if (sid == "hud") {
		Dictionary hcfg;
		hcfg["schema_version"] = 1;
		Array elems = norm.get("elements", Array());
		Array widgets;
		for (int i = 0; i < elems.size(); i++) {
			if (elems[i].get_type() == Variant::DICTIONARY) {
				widgets.push_back(normalize_hud_widget(Dictionary(elems[i]), i + 1));
			}
		}
		hcfg["widgets"] = widgets;
		set_hud_config(hcfg);
		emit_signal("data_changed", "ui_designs", 0);
		emit_signal("runtime_state_changed", "ui_design", sid);
		return;
	}

	if (!ui_designs_config.has("screens") || ui_designs_config["screens"].get_type() != Variant::DICTIONARY) {
		ui_designs_config = normalize_ui_designs_config(ui_designs_config);
	}
	Dictionary screens = Dictionary(ui_designs_config["screens"]).duplicate(true);
	screens[sid] = norm;
	ui_designs_config["screens"] = screens;
	dirty = true;
	emit_signal("data_changed", "ui_designs", 0);
	emit_signal("runtime_state_changed", "ui_design", sid);
}

Array ClockGearsDataManager::get_ui_screen_elements(const String &p_screen_id) const {
	Dictionary d = get_ui_screen_design(p_screen_id);
	return Array(d.get("elements", Array())).duplicate(true);
}

Dictionary ClockGearsDataManager::get_ui_element(const String &p_screen_id, const String &p_elem_id) const {
	String target = p_elem_id.strip_edges().to_lower();
	Array elems = get_ui_screen_elements(p_screen_id);
	for (int i = 0; i < elems.size(); i++) {
		Dictionary el = elems[i];
		if (String(el.get("id", "")).to_lower() == target || String(el.get("name", "")).strip_edges().to_lower() == target) {
			return el;
		}
	}
	return Dictionary();
}

String ClockGearsDataManager::add_ui_element(const String &p_screen_id, const Dictionary &p_elem, bool p_record_undo) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary d = get_ui_screen_design(sid);
	Array elems = Array(d.get("elements", Array())).duplicate(true);
	Dictionary ne = normalize_ui_element(p_elem, elems.size() + 1, sid);

	HashSet<String> existing;
	for (int i = 0; i < elems.size(); i++) {
		existing.insert(String(Dictionary(elems[i]).get("id", "")));
	}
	String base_id = String(ne["id"]);
	String uid = base_id;
	int suffix = 2;
	while (existing.has(uid)) {
		uid = base_id + "_" + itos(suffix++);
	}
	ne["id"] = uid;
	elems.push_back(ne);

	d["elements"] = elems;
	d["custom_enabled"] = true;
	set_ui_screen_design(sid, d, p_record_undo);
	return uid;
}

bool ClockGearsDataManager::update_ui_element(const String &p_screen_id, const String &p_elem_id, const Dictionary &p_elem, bool p_record_undo) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	String target = p_elem_id.strip_edges().to_lower();

	// Camino rápido O(1) durante arrastre interactivo (sin duplicar las otras 99 capas ni re-normalizar toda la pantalla)
	if (!p_record_undo && sid == "hud" && hud_config.has("widgets") && hud_config["widgets"].get_type() == Variant::ARRAY) {
		Array widgets = hud_config["widgets"];
		for (int i = 0; i < widgets.size(); i++) {
			if (widgets[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary cur = widgets[i];
			if (String(cur.get("id", "")).to_lower() == target) {
				Array keys = p_elem.keys();
				for (int k = 0; k < keys.size(); k++) {
					Variant kv = keys[k];
					String ks = String(kv);
					if (ks != "id") {
						cur[kv] = p_elem[kv];
					}
				}
				dirty = true;
				return true;
			}
		}
	}
	if (!p_record_undo && sid != "hud" && ui_designs_config.has("screens") && ui_designs_config["screens"].get_type() == Variant::DICTIONARY) {
		Dictionary screens = ui_designs_config["screens"];
		if (screens.has(sid) && screens[sid].get_type() == Variant::DICTIONARY) {
			Dictionary scr = screens[sid];
			if (scr.has("elements") && scr["elements"].get_type() == Variant::ARRAY) {
				Array elems = scr["elements"];
				for (int i = 0; i < elems.size(); i++) {
					if (elems[i].get_type() != Variant::DICTIONARY) continue;
					Dictionary cur = elems[i];
					if (String(cur.get("id", "")).to_lower() == target) {
						Array keys = p_elem.keys();
						for (int k = 0; k < keys.size(); k++) {
							Variant kv = keys[k];
							if (String(kv) != "id") {
								cur[kv] = p_elem[kv];
							}
						}
						scr["custom_enabled"] = true;
						dirty = true;
						return true;
					}
				}
			}
		}
	}

	Dictionary d = get_ui_screen_design(sid);
	Array elems = Array(d.get("elements", Array())).duplicate(true);
	for (int i = 0; i < elems.size(); i++) {
		Dictionary cur = elems[i];
		if (String(cur.get("id", "")).to_lower() == target) {
			Dictionary merged = cur.duplicate(true);
			Array keys = p_elem.keys();
			for (int k = 0; k < keys.size(); k++) {
				merged[keys[k]] = p_elem[keys[k]];
			}
			merged["id"] = cur["id"];
			Dictionary ne = normalize_ui_element(merged, i + 1, sid);
			ne["id"] = cur["id"];
			elems[i] = ne;
			d["elements"] = elems;
			d["custom_enabled"] = true;
			set_ui_screen_design(sid, d, p_record_undo);
			return true;
		}
	}
	return false;
}

static Vector2 _cg_ui_anchor_factors(const String &p_anchor) {
	String a = p_anchor.strip_edges().to_lower();
	float fx = (a == "top_center" || a == "center" || a == "bottom_center") ? 0.5f : ((a == "top_right" || a == "middle_right" || a == "bottom_right") ? 1.0f : 0.0f);
	float fy = (a == "middle_left" || a == "center" || a == "middle_right") ? 0.5f : ((a == "bottom_left" || a == "bottom_center" || a == "bottom_right") ? 1.0f : 0.0f);
	return Vector2(fx, fy);
}

static HashMap<String, Rect2> _cg_resolve_all_logical_rects(const HashMap<String, Dictionary> &p_by_id) {
	HashMap<String, Rect2> cache;
	struct Resolver {
		static Rect2 run(const String &p_id, const HashMap<String, Dictionary> &p_by_id, HashMap<String, Rect2> &r_cache, int depth) {
			if (p_id.is_empty() || !p_by_id.has(p_id) || depth > 32) {
				return Rect2(0, 0, 960, 540);
			}
			if (r_cache.has(p_id)) {
				return r_cache[p_id];
			}
			const Dictionary &e = p_by_id[p_id];
			String pid = String(e.get("parent_id", "")).strip_edges();
			Rect2 pr(0, 0, 960, 540);
			if (!pid.is_empty() && p_by_id.has(pid) && pid != p_id) {
				pr = run(pid, p_by_id, r_cache, depth + 1);
			}
			float ref_w = (pr.size.x > 0.5f) ? pr.size.x : 960.0f;
			float ref_h = (pr.size.y > 0.5f) ? pr.size.y : 540.0f;
			String etype = String(e.get("type", "text")).strip_edges().to_lower();
			float ew = (etype == "folder") ? float(CLAMP(int(e.get("width", 0)), 0, 960)) : float(CLAMP(int(e.get("width", 120)), 16, 960));
			float eh = (etype == "folder") ? float(CLAMP(int(e.get("height", 0)), 0, 540)) : float(CLAMP(int(e.get("height", 36)), 16, 540));
			Vector2 af = _cg_ui_anchor_factors(String(e.get("anchor", "top_left")));
			float ox = float(int(e.get("offset_x", 0)));
			float oy = float(int(e.get("offset_y", 0)));
			Rect2 out(pr.position.x + af.x * (ref_w - ew) + ox, pr.position.y + af.y * (ref_h - eh) + oy, ew, eh);
			r_cache[p_id] = out;
			return out;
		}
	};
	for (const KeyValue<String, Dictionary> &kv : p_by_id) {
		Resolver::run(kv.key, p_by_id, cache, 0);
	}
	return cache;
}

static void _cg_preserve_global_pos_for_new_parent(Dictionary &r_elem, const Vector2 &p_global_pos, const Rect2 &p_new_parent_rect) {
	float ref_w = (p_new_parent_rect.size.x > 0.5f) ? p_new_parent_rect.size.x : 960.0f;
	float ref_h = (p_new_parent_rect.size.y > 0.5f) ? p_new_parent_rect.size.y : 540.0f;
	String etype = String(r_elem.get("type", "text")).strip_edges().to_lower();
	float ew = (etype == "folder") ? float(CLAMP(int(r_elem.get("width", 0)), 0, 960)) : float(CLAMP(int(r_elem.get("width", 120)), 16, 960));
	float eh = (etype == "folder") ? float(CLAMP(int(r_elem.get("height", 0)), 0, 540)) : float(CLAMP(int(r_elem.get("height", 36)), 16, 540));
	Vector2 af = _cg_ui_anchor_factors(String(r_elem.get("anchor", "top_left")));
	r_elem["offset_x"] = CLAMP(int(Math::round(p_global_pos.x - p_new_parent_rect.position.x - af.x * (ref_w - ew))), -960, 960);
	r_elem["offset_y"] = CLAMP(int(Math::round(p_global_pos.y - p_new_parent_rect.position.y - af.y * (ref_h - eh))), -540, 540);
}

bool ClockGearsDataManager::delete_ui_element_with_mode(const String &p_screen_id, const String &p_elem_id, bool p_delete_children, bool p_record_undo) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	String target = p_elem_id.strip_edges().to_lower();
	Dictionary d = get_ui_screen_design(sid);
	Array elems = Array(d.get("elements", Array())).duplicate(true);
	int del_idx = -1;
	String del_id;
	String del_parent;
	HashMap<String, Dictionary> by_id;
	for (int i = 0; i < elems.size(); i++) {
		Dictionary cur = elems[i];
		String cid = String(cur.get("id", ""));
		if (!cid.is_empty()) {
			by_id[cid] = cur;
		}
		if (cid.to_lower() == target) {
			del_idx = i;
			del_id = cid;
			del_parent = String(cur.get("parent_id", ""));
		}
	}
	if (del_idx < 0) {
		return false;
	}
	if (p_delete_children) {
		HashSet<String> to_remove;
		to_remove.insert(del_id.to_lower());
		bool changed = true;
		while (changed) {
			changed = false;
			for (int i = 0; i < elems.size(); i++) {
				Dictionary ch = elems[i];
				String ch_id = String(ch.get("id", "")).to_lower();
				String ch_pid = String(ch.get("parent_id", "")).to_lower();
				if (!ch_id.is_empty() && !to_remove.has(ch_id) && to_remove.has(ch_pid)) {
					to_remove.insert(ch_id);
					changed = true;
				}
			}
		}
		Array kept;
		for (int i = 0; i < elems.size(); i++) {
			Dictionary cur = elems[i];
			if (!to_remove.has(String(cur.get("id", "")).to_lower())) {
				kept.push_back(cur);
			}
		}
		elems = kept;
	} else {
		HashMap<String, Rect2> old_rects = _cg_resolve_all_logical_rects(by_id);
		Rect2 new_pr(0, 0, 960, 540);
		if (!del_parent.is_empty() && old_rects.has(del_parent)) {
			new_pr = old_rects[del_parent];
		}
		elems.remove_at(del_idx);
		for (int i = 0; i < elems.size(); i++) {
			Dictionary ch = elems[i];
			String ch_id = String(ch.get("id", ""));
			if (String(ch.get("parent_id", "")).to_lower() == target) {
				if (old_rects.has(ch_id)) {
					_cg_preserve_global_pos_for_new_parent(ch, old_rects[ch_id].position, new_pr);
				}
				ch["parent_id"] = del_parent;
				elems[i] = ch;
			}
		}
	}
	d["elements"] = elems;
	if (sid == "hud") {
		d["custom_enabled"] = !elems.is_empty();
	}
	set_ui_screen_design(sid, d, p_record_undo);
	return true;
}

bool ClockGearsDataManager::delete_ui_element(const String &p_screen_id, const String &p_elem_id, bool p_record_undo) {
	return delete_ui_element_with_mode(p_screen_id, p_elem_id, false, p_record_undo);
}

static void _cg_collect_ui_tree_sorted(const Array &p_elems, HashMap<String, Dictionary> &r_by_id, HashMap<String, Vector<String>> &r_children, HashMap<String, int> &r_orig_idx) {
	r_by_id.clear();
	r_children.clear();
	r_orig_idx.clear();
	HashMap<String, String> canon_id;
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary e = Dictionary(p_elems[i]).duplicate(true);
		String id = String(e.get("id", ""));
		if (id.is_empty()) continue;
		r_by_id[id] = e;
		r_orig_idx[id] = i;
		canon_id[id.to_lower()] = id;
		r_children[id] = Vector<String>();
	}
	r_children[String("")] = Vector<String>();
	for (int i = 0; i < p_elems.size(); i++) {
		if (p_elems[i].get_type() != Variant::DICTIONARY) continue;
		String id = String(Dictionary(p_elems[i]).get("id", ""));
		if (!r_by_id.has(id)) continue;
		String raw_pid = String(r_by_id[id].get("parent_id", "")).strip_edges().to_lower();
		String pid = (!raw_pid.is_empty() && canon_id.has(raw_pid) && canon_id[raw_pid] != id) ? canon_id[raw_pid] : String("");
		r_by_id[id]["parent_id"] = pid;
		r_children[pid].push_back(id);
	}
	// Calcular máximo layer_order por subárbol para ordenar hermanos arriba = más al frente
	HashMap<String, int> sub_max_z;
	for (const KeyValue<String, Dictionary> &kv : r_by_id) {
		sub_max_z[kv.key] = int(kv.value.get("layer_order", 0));
	}
	bool changed = true;
	for (int pass = 0; pass < 32 && changed; pass++) {
		changed = false;
		for (const KeyValue<String, Vector<String>> &kv : r_children) {
			if (kv.key.is_empty() || !sub_max_z.has(kv.key)) continue;
			int mz = sub_max_z[kv.key];
			for (int c = 0; c < kv.value.size(); c++) {
				int cz = sub_max_z[kv.value[c]];
				if (cz > mz) {
					mz = cz;
				}
			}
			if (mz != sub_max_z[kv.key]) {
				sub_max_z[kv.key] = mz;
				changed = true;
			}
		}
	}
	struct SiblingCmp {
		const HashMap<String, Dictionary> *by_id = nullptr;
		const HashMap<String, int> *sub_max = nullptr;
		const HashMap<String, int> *orig_idx = nullptr;
		bool operator()(const String &a, const String &b) const {
			int sa = sub_max->get(a);
			int sb = sub_max->get(b);
			if (sa != sb) return sa > sb;
			int za = int(by_id->get(a).get("layer_order", 0));
			int zb = int(by_id->get(b).get("layer_order", 0));
			if (za != zb) return za > zb;
			return orig_idx->get(a) > orig_idx->get(b);
		}
	};
	SiblingCmp cmp{ &r_by_id, &sub_max_z, &r_orig_idx };
	for (KeyValue<String, Vector<String>> &kv : r_children) {
		if (kv.value.size() > 1) {
			kv.value.sort_custom<SiblingCmp>(cmp);
		}
	}
}

static void _cg_emit_ui_tree_back_to_front(const String &p_node_id, const HashMap<String, Vector<String>> &p_children, HashMap<String, Dictionary> &r_by_id, Array &r_out, int &r_next_z) {
	if (!p_node_id.is_empty() && r_by_id.has(p_node_id)) {
		Dictionary d = r_by_id[p_node_id];
		d["layer_order"] = r_next_z++;
		r_out.push_back(d);
	}
	if (!p_children.has(p_node_id)) {
		return;
	}
	const Vector<String> &kids = p_children[p_node_id];
	for (int i = kids.size() - 1; i >= 0; i--) {
		_cg_emit_ui_tree_back_to_front(kids[i], p_children, r_by_id, r_out, r_next_z);
	}
}

static bool _cg_is_ui_descendant(const String &p_candidate, const String &p_ancestor, const HashMap<String, Dictionary> &p_by_id) {
	String cur = p_candidate;
	HashSet<String> vis;
	while (!cur.is_empty() && p_by_id.has(cur) && !vis.has(cur)) {
		if (cur == p_ancestor) {
			return true;
		}
		vis.insert(cur);
		cur = String(p_by_id[cur].get("parent_id", ""));
	}
	return false;
}

bool ClockGearsDataManager::reorder_ui_element_tree(const String &p_screen_id, const PackedStringArray &p_elem_ids, const String &p_target_id, int p_drop_section, bool p_record_undo) {
	if (p_elem_ids.is_empty()) {
		return false;
	}
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary d = get_ui_screen_design(sid);
	Array elems = Array(d.get("elements", Array())).duplicate(true);
	HashMap<String, Dictionary> by_id;
	HashMap<String, Vector<String>> children;
	HashMap<String, int> orig_idx;
	_cg_collect_ui_tree_sorted(elems, by_id, children, orig_idx);

	HashSet<String> raw_sel;
	for (int i = 0; i < p_elem_ids.size(); i++) {
		String id = p_elem_ids[i].strip_edges();
		for (const KeyValue<String, Dictionary> &kv : by_id) {
			if (kv.key.to_lower() == id.to_lower()) {
				raw_sel.insert(kv.key);
				break;
			}
		}
	}
	if (raw_sel.is_empty()) {
		return false;
	}
	// Quedarse solo con los ancestros más altos seleccionados (si padre e hijo están seleccionados, el hijo sigue dentro del padre)
	Vector<String> moving;
	HashMap<String, String> canon_lower;
	for (const KeyValue<String, Dictionary> &kv : by_id) {
		canon_lower[kv.key.to_lower()] = kv.key;
	}
	String target_id;
	if (!p_target_id.strip_edges().is_empty() && canon_lower.has(p_target_id.strip_edges().to_lower())) {
		target_id = canon_lower[p_target_id.strip_edges().to_lower()];
	}
	for (int i = 0; i < p_elem_ids.size(); i++) {
		String kl = p_elem_ids[i].strip_edges().to_lower();
		if (!canon_lower.has(kl)) continue;
		String id = canon_lower[kl];
		if (moving.has(id)) continue;
		String cur = String(by_id[id].get("parent_id", ""));
		bool anc_selected = false;
		HashSet<String> vis;
		while (!cur.is_empty() && by_id.has(cur) && !vis.has(cur)) {
			if (raw_sel.has(cur)) {
				anc_selected = true;
				break;
			}
			vis.insert(cur);
			cur = String(by_id[cur].get("parent_id", ""));
		}
		if (!anc_selected) {
			moving.push_back(id);
		}
	}
	if (moving.is_empty()) {
		return false;
	}
	String new_parent;
	if (p_drop_section == 0 && !target_id.is_empty()) {
		new_parent = target_id;
	} else if ((p_drop_section == -1 || p_drop_section == 1) && !target_id.is_empty()) {
		new_parent = String(by_id[target_id].get("parent_id", ""));
	} else {
		new_parent = "";
	}
	if (!new_parent.is_empty()) {
		for (int i = 0; i < moving.size(); i++) {
			if (_cg_is_ui_descendant(new_parent, moving[i], by_id)) {
				return false;
			}
		}
	}
	if (!target_id.is_empty() && moving.has(target_id) && (p_drop_section == 0 || p_drop_section == -1 || p_drop_section == 1)) {
		return false;
	}
	HashMap<String, Rect2> old_rects = _cg_resolve_all_logical_rects(by_id);
	Rect2 new_pr(0, 0, 960, 540);
	if (!new_parent.is_empty() && old_rects.has(new_parent)) {
		new_pr = old_rects[new_parent];
	}
	for (int i = 0; i < moving.size(); i++) {
		String mid = moving[i];
		String old_p = String(by_id[mid].get("parent_id", ""));
		if (children.has(old_p)) {
			children[old_p].erase(mid);
		}
		if (old_p != new_parent && old_rects.has(mid)) {
			_cg_preserve_global_pos_for_new_parent(by_id[mid], old_rects[mid].position, new_pr);
		}
		by_id[mid]["parent_id"] = new_parent;
	}
	Vector<String> &dest_list = children[new_parent];
	int ins_idx = dest_list.size();
	if (p_drop_section == 0 && !target_id.is_empty()) {
		ins_idx = 0;
	} else if ((p_drop_section == -1 || p_drop_section == 1) && !target_id.is_empty()) {
		int tidx = dest_list.find(target_id);
		if (tidx >= 0) {
			ins_idx = (p_drop_section == -1) ? tidx : (tidx + 1);
		}
	}
	ins_idx = CLAMP(ins_idx, 0, dest_list.size());
	for (int i = 0; i < moving.size(); i++) {
		dest_list.insert(ins_idx + i, moving[i]);
	}
	Array reordered;
	int next_z = 0;
	_cg_emit_ui_tree_back_to_front(String(""), children, by_id, reordered, next_z);
	d["elements"] = reordered;
	d["custom_enabled"] = true;
	set_ui_screen_design(sid, d, p_record_undo);
	return true;
}

bool ClockGearsDataManager::move_ui_element_tree_step(const String &p_screen_id, const String &p_elem_id, int p_delta, bool p_record_undo) {
	if (p_delta == 0) {
		return false;
	}
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary d = get_ui_screen_design(sid);
	Array elems = Array(d.get("elements", Array())).duplicate(true);
	HashMap<String, Dictionary> by_id;
	HashMap<String, Vector<String>> children;
	HashMap<String, int> orig_idx;
	_cg_collect_ui_tree_sorted(elems, by_id, children, orig_idx);
	String target;
	for (const KeyValue<String, Dictionary> &kv : by_id) {
		if (kv.key.to_lower() == p_elem_id.strip_edges().to_lower()) {
			target = kv.key;
			break;
		}
	}
	if (target.is_empty()) {
		return false;
	}
	String pid = String(by_id[target].get("parent_id", ""));
	const Vector<String> &sibs = children[pid];
	int pos = sibs.find(target);
	if (pos < 0) {
		return false;
	}
	if (p_delta > 0 && pos > 0) {
		PackedStringArray ids;
		ids.push_back(target);
		return reorder_ui_element_tree(sid, ids, sibs[pos - 1], -1, p_record_undo);
	}
	if (p_delta < 0 && pos + 1 < sibs.size()) {
		PackedStringArray ids;
		ids.push_back(target);
		return reorder_ui_element_tree(sid, ids, sibs[pos + 1], 1, p_record_undo);
	}
	return false;
}

String ClockGearsDataManager::group_ui_elements_into_folder(const String &p_screen_id, const PackedStringArray &p_elem_ids, const String &p_folder_name, bool p_record_undo) {
	if (p_elem_ids.is_empty()) {
		return String();
	}
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	if (p_record_undo) {
		push_ui_design_undo_state(sid);
	}
	Dictionary d = get_ui_screen_design(sid);
	Array elems = Array(d.get("elements", Array()));
	HashMap<String, Dictionary> by_id;
	HashMap<String, Vector<String>> children;
	HashMap<String, int> orig_idx;
	_cg_collect_ui_tree_sorted(elems, by_id, children, orig_idx);

	String first_canon;
	String common_parent;
	for (int i = 0; i < p_elem_ids.size(); i++) {
		String kl = p_elem_ids[i].strip_edges().to_lower();
		for (const KeyValue<String, Dictionary> &kv : by_id) {
			if (kv.key.to_lower() == kl) {
				if (first_canon.is_empty()) {
					first_canon = kv.key;
					common_parent = String(kv.value.get("parent_id", ""));
				} else if (String(kv.value.get("parent_id", "")) != common_parent) {
					common_parent = "";
				}
				break;
			}
		}
	}
	if (first_canon.is_empty()) {
		return String();
	}
	Dictionary folder = make_default_ui_element("folder", sid);
	String fname = p_folder_name.strip_edges();
	folder["name"] = fname.is_empty() ? String("Carpeta") : fname;
	folder["parent_id"] = common_parent;
	HashMap<String, Rect2> cur_rects = _cg_resolve_all_logical_rects(by_id);
	bool has_bb = false;
	Vector2 bb_min(99999.0f, 99999.0f);
	for (int i = 0; i < p_elem_ids.size(); i++) {
		String kl = p_elem_ids[i].strip_edges().to_lower();
		for (const KeyValue<String, Rect2> &kv : cur_rects) {
			if (kv.key.to_lower() == kl) {
				has_bb = true;
				bb_min.x = MIN(bb_min.x, kv.value.position.x);
				bb_min.y = MIN(bb_min.y, kv.value.position.y);
				break;
			}
		}
	}
	if (has_bb) {
		Rect2 pr(0, 0, 960, 540);
		if (!common_parent.is_empty() && cur_rects.has(common_parent)) {
			pr = cur_rects[common_parent];
		}
		_cg_preserve_global_pos_for_new_parent(folder, bb_min, pr);
	}
	String folder_id = add_ui_element(sid, folder, false);
	if (folder_id.is_empty()) {
		return String();
	}
	PackedStringArray fn;
	fn.push_back(folder_id);
	reorder_ui_element_tree(sid, fn, first_canon, -1, false);
	reorder_ui_element_tree(sid, p_elem_ids, folder_id, 0, false);
	return folder_id;
}

bool ClockGearsDataManager::ungroup_ui_folder(const String &p_screen_id, const String &p_folder_id, bool p_record_undo) {
	return delete_ui_element_with_mode(p_screen_id, p_folder_id, false, p_record_undo);
}

bool ClockGearsDataManager::move_ui_element_layer(const String &p_screen_id, const String &p_elem_id, int p_delta, bool p_record_undo) {
	if (p_delta == 0) {
		return false;
	}
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	String target = p_elem_id.strip_edges().to_lower();
	Dictionary d = get_ui_screen_design(sid);
	Array elems = Array(d.get("elements", Array())).duplicate(true);
	int idx = -1;
	for (int i = 0; i < elems.size(); i++) {
		if (String(Dictionary(elems[i]).get("id", "")).to_lower() == target) {
			idx = i;
			break;
		}
	}
	if (idx < 0) {
		return false;
	}
	int new_idx = CLAMP(idx + p_delta, 0, elems.size() - 1);
	if (new_idx == idx) {
		return false;
	}
	Variant moved = elems[idx];
	elems.remove_at(idx);
	elems.insert(new_idx, moved);
	d["elements"] = elems;
	set_ui_screen_design(sid, d, p_record_undo);
	return true;
}

void ClockGearsDataManager::reset_ui_screen_design_to_default(const String &p_screen_id, bool p_record_undo) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	set_ui_screen_design(sid, get_default_ui_screen_design(sid), p_record_undo);
}

Dictionary ClockGearsDataManager::get_ui_designs_config() const {
	Dictionary cfg = normalize_ui_designs_config(ui_designs_config);
	Dictionary screens = Dictionary(cfg["screens"]).duplicate(true);
	screens["hud"] = get_ui_screen_design("hud");
	cfg["screens"] = screens;
	return cfg;
}

void ClockGearsDataManager::set_ui_designs_config(const Dictionary &p_cfg) {
	ui_designs_config = normalize_ui_designs_config(p_cfg);
	if (p_cfg.has("screens") && p_cfg["screens"].get_type() == Variant::DICTIONARY) {
		Dictionary raw_screens = Dictionary(p_cfg["screens"]);
		if (raw_screens.has("hud") && raw_screens["hud"].get_type() == Variant::DICTIONARY) {
			set_ui_screen_design("hud", Dictionary(raw_screens["hud"]), false);
			return;
		}
	}
	dirty = true;
	emit_signal("data_changed", "ui_designs", 0);
	emit_signal("runtime_state_changed", "ui_design", String());
}

static Dictionary _cg_make_tpl_meta(const String &p_id, const String &p_name, const String &p_cat, const String &p_desc, const Dictionary &p_design) {
	Dictionary d;
	d["id"] = p_id;
	d["name"] = p_name;
	d["category"] = p_cat;
	d["description"] = p_desc;
	d["is_component"] = false;
	d["is_builtin"] = true;
	d["design"] = p_design;
	return d;
}

static Dictionary _cg_make_tpl_btn(const String &p_id, const String &p_text, const String &p_act, int p_ox, int p_oy, int p_w, int p_h, const String &p_anchor = "top_left", const String &p_shape_type = "rounded", int p_radius = 8) {
	Dictionary b;
	b["id"] = p_id;
	b["name"] = p_text;
	b["type"] = "button";
	b["text"] = p_text;
	b["anchor"] = p_anchor;
	b["offset_x"] = p_ox;
	b["offset_y"] = p_oy;
	b["width"] = p_w;
	b["height"] = p_h;
	b["action"] = p_act;
	Dictionary act;
	act["type"] = p_act;
	Array acts;
	acts.push_back(act);
	b["actions"] = acts;
	Dictionary sh;
	sh["shape_type"] = p_shape_type;
	sh["corner_radius"] = p_radius;
	sh["fill_color"] = "#1c202cf2";
	sh["border_width"] = 2;
	sh["border_color"] = "#d49b42ff";
	b["shape"] = sh;
	return b;
}

Array ClockGearsDataManager::get_builtin_menu_templates() {
	Array out;

	// 1. Clásico JRPG
	{
		Dictionary d;
		d["custom_enabled"] = true;
		d["background"] = normalize_ui_background(Dictionary());
		Dictionary f_left;
		f_left["id"] = "jrpg_cmd_panel";
		f_left["name"] = "Panel Comandos";
		f_left["type"] = "frame";
		f_left["anchor"] = "top_left";
		f_left["offset_x"] = 28;
		f_left["offset_y"] = 28;
		f_left["width"] = 230;
		f_left["height"] = 340;
		f_left["layer_order"] = 0;

		Dictionary f_right;
		f_right["id"] = "jrpg_status_panel";
		f_right["name"] = "Panel Grupo";
		f_right["type"] = "frame";
		f_right["anchor"] = "top_left";
		f_right["offset_x"] = 276;
		f_right["offset_y"] = 28;
		f_right["width"] = 656;
		f_right["height"] = 484;
		f_right["layer_order"] = 0;

		Dictionary f_gold;
		f_gold["id"] = "jrpg_gold_card";
		f_gold["name"] = "Tarjeta Oro";
		f_gold["type"] = "currency";
		f_gold["anchor"] = "top_left";
		f_gold["offset_x"] = 28;
		f_gold["offset_y"] = 384;
		f_gold["width"] = 230;
		f_gold["height"] = 64;
		f_gold["layer_order"] = 1;

		Dictionary t_hp;
		t_hp["id"] = "jrpg_party_hp";
		t_hp["name"] = "HP Lider";
		t_hp["type"] = "text";
		t_hp["data_binding"] = "player_hp";
		t_hp["text"] = "HP: {value}";
		t_hp["anchor"] = "top_left";
		t_hp["offset_x"] = 308;
		t_hp["offset_y"] = 64;
		t_hp["width"] = 260;
		t_hp["height"] = 34;

		Array els;
		els.push_back(f_left);
		els.push_back(f_right);
		els.push_back(f_gold);
		els.push_back(t_hp);
		const char *lbls[5] = { "Objetos", "Equipo", "Estado", "Guardar", "Cerrar" };
		const char *acts[5] = { "items", "equip", "status", "save", "close" };
		for (int i = 0; i < 5; i++) {
			Dictionary btn = _cg_make_tpl_btn(vformat("jrpg_b%d", i + 1), String::utf8(lbls[i]), String(acts[i]), 44, 48 + i * 54, 198, 42);
			btn["focus_order"] = i;
			btn["layer_order"] = 2;
			els.push_back(btn);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("classic_jrpg", String::utf8("Clásico JRPG"), "menu", String::utf8("Lista lateral clásica, panel de estado y tarjeta de moneda."), d));
	}

	// 2. Anillo radial (estilo disposición Zelda 3D, dibujo propio)
	{
		Dictionary d;
		d["custom_enabled"] = true;
		Dictionary bg;
		bg["veil_enabled"] = true;
		bg["veil_color"] = "#05070cb8";
		d["background"] = bg;
		Dictionary tr;
		tr["in_type"] = "radial_spin";
		tr["out_type"] = "fade";
		tr["duration_sec"] = 0.22;
		d["transition"] = tr;
		Dictionary lay;
		lay["mode"] = "radial";
		lay["radial_center_x"] = 480;
		lay["radial_center_y"] = 270;
		lay["radial_radius"] = 155;
		lay["radial_slot_size"] = 62;
		lay["radial_selected_scale"] = 1.32;
		lay["radial_start_angle_deg"] = -90.0;
		lay["radial_sweep_angle_deg"] = 360.0;
		lay["radial_clockwise"] = true;
		lay["radial_show_center_info"] = true;
		Array slots;
		const char *s_titles[8] = { "Objetos", "Equipo", "Estado", "Opciones", "Guardar", "Cargar", "Mapa", "Cerrar" };
		const char *s_descs[8] = { "Mochila de consumibles", "Armas y armaduras", "Atributos del héroe", "Volumen y pantalla", "Guardar avance", "Cargar partida", "Diario de viaje", "Volver al juego" };
		const char *s_acts[8] = { "items", "equip", "status", "options", "save", "load_game", "close", "close" };
		for (int i = 0; i < 8; i++) {
			Dictionary sl;
			sl["id"] = vformat("rad_slot_%d", i + 1);
			sl["title"] = String::utf8(s_titles[i]);
			sl["description"] = String::utf8(s_descs[i]);
			sl["action"] = String(s_acts[i]);
			Dictionary st;
			st["type"] = String(s_acts[i]);
			Array sa;
			sa.push_back(st);
			sl["actions"] = sa;
			slots.push_back(sl);
		}
		lay["radial_slots"] = slots;
		d["layout"] = lay;
		d["elements"] = Array();
		out.push_back(_cg_make_tpl_meta("radial_ring", "Anillo radial", "radial", String::utf8("Círculo de 8 ranuras dibujado sin nodos y descripción central."), d));
	}

	// 3. Rueda de objetos rápida (medio círculo)
	{
		Dictionary d;
		d["custom_enabled"] = true;
		Dictionary bg;
		bg["veil_enabled"] = true;
		bg["veil_color"] = "#05070ca0";
		d["background"] = bg;
		Dictionary lay;
		lay["mode"] = "radial";
		lay["radial_center_x"] = 480;
		lay["radial_center_y"] = 370;
		lay["radial_radius"] = 170;
		lay["radial_slot_size"] = 60;
		lay["radial_selected_scale"] = 1.3;
		lay["radial_start_angle_deg"] = -180.0;
		lay["radial_sweep_angle_deg"] = 180.0;
		lay["radial_clockwise"] = true;
		lay["radial_show_center_info"] = true;
		Array slots;
		for (int i = 0; i < 5; i++) {
			Dictionary sl;
			sl["id"] = vformat("wheel_slot_%d", i + 1);
			sl["title"] = vformat(String::utf8("Rápido %d"), i + 1);
			sl["description"] = String::utf8("Ranura rápida de inventario");
			sl["action"] = (i == 4) ? "close" : "items";
			Dictionary st;
			st["type"] = sl["action"];
			Array sa;
			sa.push_back(st);
			sl["actions"] = sa;
			slots.push_back(sl);
		}
		lay["radial_slots"] = slots;
		d["layout"] = lay;
		d["elements"] = Array();
		out.push_back(_cg_make_tpl_meta("quick_item_wheel", String::utf8("Rueda de objetos rápida"), "radial", String::utf8("Semicírculo inferior superior con 5 ranuras rápidas."), d));
	}

	// 4. Cuadrícula de inventario (ARPG)
	{
		Dictionary d;
		d["custom_enabled"] = true;
		Dictionary lay;
		lay["mode"] = "grid";
		lay["origin_anchor"] = "center";
		lay["offset_x"] = 0;
		lay["offset_y"] = -20;
		lay["width"] = 640;
		lay["height"] = 300;
		lay["columns"] = 4;
		lay["rows"] = 2;
		lay["spacing_x"] = 14;
		lay["spacing_y"] = 14;
		d["layout"] = lay;
		Array els;
		for (int i = 0; i < 8; i++) {
			Dictionary b = _cg_make_tpl_btn(vformat("inv_slot_%d", i + 1), vformat("Ranura %d", i + 1), (i == 7) ? "close" : "items", 160 + (i % 4) * 160, 120 + (i / 4) * 140, 140, 120, "top_left", "rounded", 10);
			b["focus_order"] = i;
			els.push_back(b);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("arpg_inventory_grid", String::utf8("Cuadrícula de inventario (ARPG)"), "grid", String::utf8("Rejilla 4x2 con navegación direccional para inventario o habilidades."), d));
	}

	// 5. Pestañas a pantalla completa
	{
		Dictionary d;
		d["custom_enabled"] = true;
		Dictionary lay;
		lay["mode"] = "tabs";
		lay["origin_anchor"] = "center";
		lay["width"] = 780;
		lay["height"] = 380;
		Array tlbls;
		tlbls.push_back("General");
		tlbls.push_back("Equipo");
		tlbls.push_back("Sistema");
		lay["tab_labels"] = tlbls;
		d["layout"] = lay;
		Array els;
		const char *acts[4] = { "items", "equip", "status", "close" };
		const char *txts[4] = { "Objetos", "Armamento", "Estado", "Cerrar" };
		for (int i = 0; i < 4; i++) {
			Dictionary b = _cg_make_tpl_btn(vformat("tab_el_%d", i + 1), String::utf8(txts[i]), String(acts[i]), 100, 100 + i * 52, 220, 44);
			b["tab_index"] = i % 3;
			b["focus_order"] = i;
			els.push_back(b);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("fullscreen_tabs", String::utf8("Pestañas a pantalla completa"), "tabs", String::utf8("Contenedor con pestañas superiores y cambio instantáneo."), d));
	}

	// 6. Lateral deslizante
	{
		Dictionary d;
		d["custom_enabled"] = true;
		Dictionary bg;
		bg["veil_enabled"] = true;
		bg["veil_color"] = "#05070c70";
		d["background"] = bg;
		Dictionary tr;
		tr["in_type"] = "slide_left";
		tr["out_type"] = "slide_left";
		tr["duration_sec"] = 0.2;
		d["transition"] = tr;
		Array els;
		Dictionary f;
		f["id"] = "sidebar_panel";
		f["name"] = "Barra Lateral";
		f["type"] = "frame";
		f["anchor"] = "top_left";
		f["offset_x"] = 0;
		f["offset_y"] = 0;
		f["width"] = 270;
		f["height"] = 540;
		els.push_back(f);
		const char *lbls[4] = { "Objetos", "Estado", "Opciones", "Cerrar" };
		const char *acts[4] = { "items", "status", "options", "close" };
		for (int i = 0; i < 4; i++) {
			Dictionary b = _cg_make_tpl_btn(vformat("side_b%d", i + 1), String::utf8(lbls[i]), String(acts[i]), 24, 56 + i * 58, 220, 44, "top_left", "pill", 22);
			b["focus_order"] = i;
			b["layer_order"] = 2;
			els.push_back(b);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("sliding_sidebar", "Lateral deslizante", "menu", String::utf8("Panel lateral izquierdo que se desliza suavemente sobre el mapa."), d));
	}

	// 7. Minimalista (solo botones)
	{
		Dictionary d;
		d["custom_enabled"] = true;
		Array els;
		const char *lbls[4] = { "Objetos", "Estado", "Opciones", "Cerrar" };
		const char *acts[4] = { "items", "status", "options", "close" };
		for (int i = 0; i < 4; i++) {
			Dictionary b = _cg_make_tpl_btn(vformat("min_b%d", i + 1), String::utf8(lbls[i]), String(acts[i]), 0, -110 + i * 62, 220, 46, "center", "pill", 23);
			b["focus_order"] = i;
			els.push_back(b);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("minimalist", "Minimalista (solo botones)", "menu", String::utf8("Columna limpia centrada de botones tipo píldora sin marcos extra."), d));
	}

	// 8. Cartas o carrusel
	{
		Dictionary d;
		d["custom_enabled"] = true;
		Dictionary lay;
		lay["mode"] = "carousel";
		lay["origin_anchor"] = "center";
		lay["width"] = 480;
		lay["height"] = 280;
		d["layout"] = lay;
		Array els;
		const char *lbls[4] = { "Objetos", "Equipo", "Estado", "Opciones" };
		const char *acts[4] = { "items", "equip", "status", "options" };
		for (int i = 0; i < 4; i++) {
			Dictionary b = _cg_make_tpl_btn(vformat("card_b%d", i + 1), String::utf8(lbls[i]), String(acts[i]), 320, 140, 300, 200, "top_left", "rounded", 14);
			b["focus_order"] = i;
			els.push_back(b);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("cards_carousel", "Cartas o carrusel", "carousel", String::utf8("Tarjetas amplias en carrusel con avance izquierda/derecha."), d));
	}

	// 9. Pausa rápida (ventana pequeña centrada)
	{
		Dictionary d;
		d["custom_enabled"] = true;
		d["pause_on_open"] = true;
		Dictionary bg;
		bg["veil_enabled"] = true;
		bg["veil_color"] = "#05070cb4";
		d["background"] = bg;
		Dictionary tr;
		tr["in_type"] = "scale";
		tr["out_type"] = "fade";
		tr["duration_sec"] = 0.14;
		tr["curve"] = "back_out";
		d["transition"] = tr;
		Array els;
		Dictionary fr;
		fr["id"] = "pause_box";
		fr["name"] = "Marco Pausa";
		fr["type"] = "frame";
		fr["anchor"] = "center";
		fr["width"] = 300;
		fr["height"] = 290;
		els.push_back(fr);
		Dictionary tit;
		tit["id"] = "pause_title";
		tit["name"] = String::utf8("Título Pausa");
		tit["type"] = "text";
		tit["text"] = "PAUSA";
		tit["anchor"] = "center";
		tit["offset_y"] = -112;
		tit["width"] = 240;
		tit["height"] = 34;
		tit["align"] = "center";
		tit["font_size"] = 20;
		els.push_back(tit);
		const char *lbls[4] = { "Reanudar", "Opciones", "Ir al título", "Salir" };
		const char *acts[4] = { "close", "options", "to_title", "quit" };
		for (int i = 0; i < 4; i++) {
			Dictionary b = _cg_make_tpl_btn(vformat("pause_b%d", i + 1), String::utf8(lbls[i]), String(acts[i]), 0, -56 + i * 50, 220, 40, "center", "rounded", 8);
			b["focus_order"] = i;
			b["layer_order"] = 2;
			els.push_back(b);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("quick_pause", String::utf8("Pausa rápida"), "menu", String::utf8("Ventana compacta centrada que pausa el juego con velo oscuro."), d));
	}

	// 10. Título clásico
	{
		Dictionary d;
		d["custom_enabled"] = true;
		d["window_type"] = "title";
		Dictionary bg;
		bg["type"] = "gradient";
		bg["gradient_top"] = "#1b2438ff";
		bg["gradient_bottom"] = "#0a0d16ff";
		d["background"] = bg;
		Array els;
		Dictionary tit;
		tit["id"] = "title_classic_txt";
		tit["name"] = String::utf8("Título del juego");
		tit["type"] = "text";
		tit["data_binding"] = "project_title";
		tit["anchor"] = "top_center";
		tit["offset_y"] = 60;
		tit["width"] = 640;
		tit["height"] = 72;
		tit["font_size"] = 40;
		tit["align"] = "center";
		els.push_back(tit);
		const char *lbls[4] = { "Nueva partida", "Continuar", "Opciones", "Salir" };
		const char *acts[4] = { "new_game", "continue", "options", "quit" };
		for (int i = 0; i < 4; i++) {
			Dictionary b = _cg_make_tpl_btn(vformat("tcls_b%d", i + 1), String::utf8(lbls[i]), String(acts[i]), 0, 20 + i * 52, 240, 42, "center", "rounded", 8);
			b["focus_order"] = i;
			els.push_back(b);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("classic_title", String::utf8("Título clásico"), "title", String::utf8("Título superior y columna inferior de acciones principales."), d));
	}

	// 11. Título con logo grande y botones centrados
	{
		Dictionary d;
		d["custom_enabled"] = true;
		d["window_type"] = "title";
		Array els;
		Dictionary logo_fr;
		logo_fr["id"] = "hero_logo_frame";
		logo_fr["name"] = "Marco Logo Grande";
		logo_fr["type"] = "frame";
		logo_fr["anchor"] = "top_center";
		logo_fr["offset_y"] = 42;
		logo_fr["width"] = 520;
		logo_fr["height"] = 160;
		els.push_back(logo_fr);
		Dictionary logo_lbl;
		logo_lbl["id"] = "hero_logo_txt";
		logo_lbl["name"] = "Logo Texto";
		logo_lbl["type"] = "text";
		logo_lbl["data_binding"] = "project_title";
		logo_lbl["anchor"] = "top_center";
		logo_lbl["offset_y"] = 96;
		logo_lbl["width"] = 480;
		logo_lbl["height"] = 56;
		logo_lbl["font_size"] = 36;
		logo_lbl["align"] = "center";
		logo_lbl["layer_order"] = 2;
		els.push_back(logo_lbl);
		const char *lbls[4] = { "Comenzar", "Continuar", "Opciones", "Créditos" };
		const char *acts[4] = { "new_game", "continue", "options", "credits" };
		for (int i = 0; i < 4; i++) {
			Dictionary b = _cg_make_tpl_btn(vformat("hlogo_b%d", i + 1), String::utf8(lbls[i]), String(acts[i]), 0, 250 + i * 54, 240, 44, "top_center", "pill", 22);
			b["focus_order"] = i;
			els.push_back(b);
		}
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("hero_logo_title", String::utf8("Título con logo grande y botones centrados"), "title", String::utf8("Gran emblema centrado arriba y botones en píldora debajo."), d));
	}

	// 12. HUD de esquina
	{
		Dictionary d;
		d["custom_enabled"] = true;
		d["window_type"] = "hud";
		d["pause_on_open"] = false;
		Array els;
		Dictionary panel;
		panel["id"] = "hud_corner_panel";
		panel["name"] = "Fondo HUD Esquina";
		panel["type"] = "frame";
		panel["anchor"] = "top_left";
		panel["offset_x"] = 16;
		panel["offset_y"] = 16;
		panel["width"] = 248;
		panel["height"] = 114;
		els.push_back(panel);
		Dictionary hp;
		hp["id"] = "hud_hp_bar";
		hp["name"] = "Barra HP";
		hp["type"] = "bar";
		hp["anchor"] = "top_left";
		hp["offset_x"] = 28;
		hp["offset_y"] = 28;
		hp["width"] = 224;
		hp["height"] = 24;
		hp["layer_order"] = 2;
		els.push_back(hp);
		Dictionary hts;
		hts["id"] = "hud_hearts";
		hts["name"] = "Corazones";
		hts["type"] = "hearts";
		hts["anchor"] = "top_left";
		hts["offset_x"] = 28;
		hts["offset_y"] = 60;
		hts["width"] = 140;
		hts["height"] = 24;
		hts["layer_order"] = 2;
		els.push_back(hts);
		Dictionary gold;
		gold["id"] = "hud_gold";
		gold["name"] = "Monedas";
		gold["type"] = "currency";
		gold["anchor"] = "top_left";
		gold["offset_x"] = 28;
		gold["offset_y"] = 90;
		gold["width"] = 180;
		gold["height"] = 28;
		gold["layer_order"] = 2;
		els.push_back(gold);
		d["elements"] = els;
		out.push_back(_cg_make_tpl_meta("corner_hud", "HUD de esquina", "hud", String::utf8("Barra de salud, corazones y monedas agrupados en esquina superior izquierda."), d));
	}

	// 13. Rueda radial (alias claro de rueda/anillo radial para menú rápido o inventario)
	{
		Dictionary d;
		d["custom_enabled"] = true;
		Dictionary bg;
		bg["veil_enabled"] = true;
		bg["veil_color"] = "#05070cb8";
		d["background"] = bg;
		Dictionary tr;
		tr["in_type"] = "radial_spin";
		tr["out_type"] = "fade";
		tr["duration_sec"] = 0.20;
		d["transition"] = tr;
		Dictionary lay;
		lay["mode"] = "radial";
		lay["radial_center_x"] = 480;
		lay["radial_center_y"] = 270;
		lay["radial_radius"] = 160;
		lay["radial_slot_size"] = 64;
		lay["radial_selected_scale"] = 1.32;
		lay["radial_start_angle_deg"] = -90.0;
		lay["radial_sweep_angle_deg"] = 360.0;
		lay["radial_clockwise"] = true;
		lay["radial_show_center_info"] = true;
		Array slots;
		const char *s_titles[6] = { "Usar", "Equipar", "Objetos", "Estado", "Opciones", "Cerrar" };
		const char *s_acts[6] = { "item_use", "equip", "items", "status", "options", "close" };
		for (int i = 0; i < 6; i++) {
			Dictionary sl;
			sl["id"] = vformat("rueda_slot_%d", i + 1);
			sl["title"] = String::utf8(s_titles[i]);
			sl["description"] = String::utf8("Acceso rápido radial");
			sl["action"] = String(s_acts[i]);
			Dictionary st;
			st["type"] = String(s_acts[i]);
			Array sa;
			sa.push_back(st);
			sl["actions"] = sa;
			slots.push_back(sl);
		}
		lay["radial_slots"] = slots;
		d["layout"] = lay;
		d["elements"] = Array();
		out.push_back(_cg_make_tpl_meta("rueda_radial", "Rueda radial", "radial", String::utf8("Rueda radial interactiva de 6 ranuras con escalado de selección."), d));
	}

	// 14. Mensajes y batalla clásico
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		Dictionary mb_tpl = dm->get_default_ui_screen_template("messages_battle");
		out.push_back(_cg_make_tpl_meta("messages_battle_classic", String::utf8("Diálogo y batalla clásico"), "messages_battle", String::utf8("Cuadro de diálogo inferior con retrato y panel táctico lateral."), mb_tpl));
	}

	return out;
}

Array ClockGearsDataManager::get_ui_templates_for_system(const String &p_screen_id) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Array all = get_builtin_menu_templates();
	String pref_cat = "menu";
	if (sid == "title") {
		pref_cat = "title";
	} else if (sid == "hud") {
		pref_cat = "hud";
	} else if (sid == "game_menu_items" || sid == "game_menu_equip") {
		pref_cat = "grid";
	} else if (sid == "messages_battle") {
		pref_cat = "messages_battle";
	}
	Array prioritized;
	Array radial_list;
	Array others;
	for (int i = 0; i < all.size(); i++) {
		Dictionary t = all[i];
		String cat = String(t.get("category", "")).to_lower();
		if (cat == pref_cat) {
			prioritized.push_back(t);
		} else if (cat == "radial") {
			radial_list.push_back(t);
		} else {
			others.push_back(t);
		}
	}
	for (int i = 0; i < radial_list.size(); i++) {
		prioritized.push_back(radial_list[i]);
	}
	for (int i = 0; i < others.size(); i++) {
		prioritized.push_back(others[i]);
	}
	return prioritized;
}

Array ClockGearsDataManager::get_builtin_component_templates() {
	Array out;
	auto push_comp = [&](const String &p_id, const String &p_name, const String &p_desc, const Dictionary &p_el) {
		Dictionary d;
		d["id"] = p_id;
		d["name"] = p_name;
		d["category"] = "component";
		d["description"] = p_desc;
		d["is_component"] = true;
		d["is_builtin"] = true;
		d["element"] = p_el;
		out.push_back(d);
	};

	push_comp("comp_round_btn", String::utf8("Botón redondo"), String::utf8("Botón circular ideal para accesos rápidos."),
			_cg_make_tpl_btn("comp_round", String::utf8("Acción"), "close", 440, 230, 82, 82, "top_left", "circle", 41));

	Dictionary fr;
	fr["id"] = "comp_panel";
	fr["name"] = "Panel con marco";
	fr["type"] = "frame";
	fr["anchor"] = "center";
	fr["width"] = 280;
	fr["height"] = 180;
	Dictionary fsh;
	fsh["shape_type"] = "rounded";
	fsh["corner_radius"] = 10;
	fsh["gradient_enabled"] = true;
	fsh["gradient_top_color"] = "#24201aff";
	fsh["gradient_bottom_color"] = "#12100eff";
	fsh["border_width"] = 2;
	fsh["border_color"] = "#e0a040ff";
	fsh["shadow_enabled"] = true;
	fr["shape"] = fsh;
	push_comp("comp_framed_panel", "Panel con marco", String::utf8("Panel redondeado con degradado latón y sombra."), fr);

	Dictionary cbar;
	cbar["id"] = "comp_circ_bar";
	cbar["name"] = "Barra circular";
	cbar["type"] = "bar";
	cbar["anchor"] = "top_left";
	cbar["offset_x"] = 48;
	cbar["offset_y"] = 48;
	cbar["width"] = 96;
	cbar["height"] = 96;
	Dictionary csh;
	csh["shape_type"] = "circle";
	csh["border_width"] = 3;
	csh["border_color"] = "#f5bc54ff";
	cbar["shape"] = csh;
	push_comp("comp_circular_bar", "Barra circular", String::utf8("Medidor recortado en círculo con borde ámbar."), cbar);

	Dictionary clist;
	clist["id"] = "comp_item_list";
	clist["name"] = "Lista de objetos";
	clist["type"] = "container";
	clist["anchor"] = "top_left";
	clist["offset_x"] = 40;
	clist["offset_y"] = 40;
	clist["width"] = 260;
	clist["height"] = 300;
	Dictionary clay;
	clay["mode"] = "vertical_list";
	clay["spacing_y"] = 8;
	clist["layout"] = normalize_menu_layout(clay);
	push_comp("comp_item_list", "Lista de objetos", String::utf8("Contenedor vertical para organizar botones en lista."), clist);

	Dictionary slot;
	slot = _cg_make_tpl_btn("comp_slot", "", "items", 80, 80, 68, 68, "top_left", "rounded", 12);
	slot["show_background"] = true;
	push_comp("comp_icon_slot", "Ranura con icono", String::utf8("Casilla cuadrada redondeada lista para asignar icono."), slot);

	return out;
}

Dictionary ClockGearsDataManager::get_builtin_template_by_id(const String &p_template_id) {
	String tid = p_template_id.strip_edges().to_lower();
	Array all = get_builtin_menu_templates();
	for (int i = 0; i < all.size(); i++) {
		Dictionary d = all[i];
		if (String(d.get("id", "")) == tid) {
			return d;
		}
	}
	Array comps = get_builtin_component_templates();
	for (int i = 0; i < comps.size(); i++) {
		Dictionary d = comps[i];
		if (String(d.get("id", "")) == tid) {
			return d;
		}
	}
	return Dictionary();
}

Array ClockGearsDataManager::get_user_menu_templates() const {
	Array out;
	if (!ui_designs_config.has("user_templates") || ui_designs_config["user_templates"].get_type() != Variant::DICTIONARY) {
		return out;
	}
	Dictionary ut = ui_designs_config["user_templates"];
	Array keys = ut.keys();
	keys.sort();
	for (int i = 0; i < keys.size(); i++) {
		if (ut[keys[i]].get_type() == Variant::DICTIONARY) {
			out.push_back(Dictionary(ut[keys[i]]).duplicate(true));
		}
	}
	return out;
}

String ClockGearsDataManager::save_screen_as_user_template(const String &p_screen_id, const String &p_name) {
	String nm = p_name.strip_edges();
	if (nm.is_empty()) {
		nm = String::utf8("Mi plantilla");
	}
	String base = _cg_slugify_menu_id(nm);
	if (base.is_empty()) base = "tpl";
	if (!ui_designs_config.has("user_templates") || ui_designs_config["user_templates"].get_type() != Variant::DICTIONARY) {
		ui_designs_config["user_templates"] = Dictionary();
	}
	Dictionary ut = Dictionary(ui_designs_config["user_templates"]).duplicate(true);
	String tid = "user_" + base;
	int sfx = 2;
	while (ut.has(tid)) {
		tid = "user_" + base + "_" + itos(sfx++);
	}
	Dictionary src = get_ui_screen_design(p_screen_id);
	Dictionary tpl;
	tpl["id"] = tid;
	tpl["name"] = nm;
	tpl["category"] = "user";
	tpl["description"] = String::utf8("Plantilla guardada desde ") + String(src.get("name", p_screen_id));
	tpl["is_component"] = false;
	tpl["is_builtin"] = false;
	tpl["design"] = src;
	ut[tid] = tpl;
	ui_designs_config["user_templates"] = ut;
	dirty = true;
	emit_signal("data_changed", "ui_designs", 0);
	return tid;
}

String ClockGearsDataManager::save_element_as_component_template(const String &p_screen_id, const String &p_elem_id, const String &p_name) {
	Dictionary el = get_ui_element(p_screen_id, p_elem_id);
	if (el.is_empty()) {
		return String();
	}
	String nm = p_name.strip_edges();
	if (nm.is_empty()) {
		nm = String(el.get("name", "Componente"));
	}
	String base = _cg_slugify_menu_id(nm);
	if (base.is_empty()) base = "comp";
	if (!ui_designs_config.has("user_templates") || ui_designs_config["user_templates"].get_type() != Variant::DICTIONARY) {
		ui_designs_config["user_templates"] = Dictionary();
	}
	Dictionary ut = Dictionary(ui_designs_config["user_templates"]).duplicate(true);
	String tid = "user_comp_" + base;
	int sfx = 2;
	while (ut.has(tid)) {
		tid = "user_comp_" + base + "_" + itos(sfx++);
	}
	Dictionary tpl;
	tpl["id"] = tid;
	tpl["name"] = nm;
	tpl["category"] = "component";
	tpl["description"] = String::utf8("Componente guardado por el creador");
	tpl["is_component"] = true;
	tpl["is_builtin"] = false;
	tpl["element"] = el;
	ut[tid] = tpl;
	ui_designs_config["user_templates"] = ut;
	dirty = true;
	emit_signal("data_changed", "ui_designs", 0);
	return tid;
}

bool ClockGearsDataManager::delete_user_template(const String &p_template_id) {
	String tid = p_template_id.strip_edges().to_lower();
	if (!ui_designs_config.has("user_templates") || ui_designs_config["user_templates"].get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary ut = Dictionary(ui_designs_config["user_templates"]).duplicate(true);
	if (!ut.has(tid)) {
		return false;
	}
	ut.erase(tid);
	ui_designs_config["user_templates"] = ut;
	dirty = true;
	emit_signal("data_changed", "ui_designs", 0);
	return true;
}

bool ClockGearsDataManager::apply_menu_template(const String &p_screen_id, const String &p_template_id, bool p_preserve_buttons, bool p_record_undo) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	String tid = p_template_id.strip_edges().to_lower();
	Dictionary tpl = get_builtin_template_by_id(tid);
	if (tpl.is_empty() && ui_designs_config.has("user_templates") && ui_designs_config["user_templates"].get_type() == Variant::DICTIONARY) {
		Dictionary ut = ui_designs_config["user_templates"];
		if (ut.has(tid) && ut[tid].get_type() == Variant::DICTIONARY) {
			tpl = Dictionary(ut[tid]).duplicate(true);
		}
	}
	if (tpl.is_empty() || bool(tpl.get("is_component", false))) {
		return false;
	}
	Dictionary tpl_design = Dictionary(tpl.get("design", Dictionary())).duplicate(true);
	Dictionary cur = get_ui_screen_design(sid);

	// Recolectar botones previos si se pidió conservar botones y acciones
	Vector<Dictionary> prev_buttons;
	if (p_preserve_buttons) {
		Array cur_elems = cur.get("elements", Array());
		for (int i = 0; i < cur_elems.size(); i++) {
			if (cur_elems[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary ce = cur_elems[i];
			if (String(ce.get("type", "")) == "button") {
				prev_buttons.push_back(ce);
			}
		}
	}

	// Preservar metadatos propios del menú (id, nombre, entradas, contexto, pausa, sonidos)
	Dictionary next = tpl_design.duplicate(true);
	next["id"] = sid;
	next["screen_id"] = sid;
	next["name"] = cur.get("name", sid);
	next["is_system"] = cur.get("is_system", _cg_is_system_menu_id(sid));
	next["order"] = cur.get("order", 0);
	if (!cur.get("window_type", "").operator String().is_empty()) {
		next["window_type"] = cur["window_type"];
	}
	next["custom_enabled"] = true;
	if (cur.has("triggers")) next["triggers"] = cur["triggers"];
	if (cur.has("trigger_context")) next["trigger_context"] = cur["trigger_context"];
	if (cur.has("pause_on_open")) next["pause_on_open"] = cur["pause_on_open"];
	if (cur.has("respect_menu_access")) next["respect_menu_access"] = cur["respect_menu_access"];
	if (cur.has("sounds")) next["sounds"] = cur["sounds"];

	if (p_preserve_buttons && !prev_buttons.is_empty()) {
		Array next_elems = next.get("elements", Array());
		int mapped = 0;
		for (int i = 0; i < next_elems.size() && mapped < prev_buttons.size(); i++) {
			if (next_elems[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary ne = next_elems[i];
			if (String(ne.get("type", "")) == "button") {
				const Dictionary &pb = prev_buttons[mapped++];
				if (pb.has("text")) ne["text"] = pb["text"];
				if (pb.has("name")) ne["name"] = pb["name"];
				if (pb.has("action")) ne["action"] = pb["action"];
				if (pb.has("actions")) ne["actions"] = Array(pb["actions"]).duplicate(true);
				next_elems[i] = ne;
			}
		}
		next["elements"] = next_elems;

		Dictionary lay = next.get("layout", Dictionary());
		if (String(lay.get("mode", "")) == "radial" && lay.has("radial_slots") && lay["radial_slots"].get_type() == Variant::ARRAY) {
			Array rslots = lay["radial_slots"];
			int rm = 0;
			for (int i = 0; i < rslots.size() && rm < prev_buttons.size(); i++) {
				if (rslots[i].get_type() != Variant::DICTIONARY) continue;
				Dictionary rs = rslots[i];
				const Dictionary &pb = prev_buttons[rm++];
				String ptxt = String(pb.get("text", pb.get("name", ""))).strip_edges();
				if (!ptxt.is_empty()) rs["title"] = ptxt;
				if (pb.has("action")) rs["action"] = pb["action"];
				if (pb.has("actions")) rs["actions"] = Array(pb["actions"]).duplicate(true);
				rslots[i] = rs;
			}
			lay["radial_slots"] = rslots;
			next["layout"] = lay;
		}
	}

	set_ui_screen_design(sid, next, p_record_undo);
	return true;
}

String ClockGearsDataManager::instantiate_component_template(const String &p_screen_id, const String &p_template_id, const Vector2i &p_pos) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	String tid = p_template_id.strip_edges().to_lower();
	Dictionary tpl = get_builtin_template_by_id(tid);
	if (tpl.is_empty() && ui_designs_config.has("user_templates") && ui_designs_config["user_templates"].get_type() == Variant::DICTIONARY) {
		Dictionary ut = ui_designs_config["user_templates"];
		if (ut.has(tid) && ut[tid].get_type() == Variant::DICTIONARY) {
			tpl = Dictionary(ut[tid]).duplicate(true);
		}
	}
	if (tpl.is_empty() || !bool(tpl.get("is_component", false))) {
		return String();
	}
	Dictionary el = Dictionary(tpl.get("element", Dictionary())).duplicate(true);
	el.erase("id");
	if (p_pos.x >= 0 && p_pos.y >= 0) {
		el["anchor"] = "top_left";
		int w = int(el.get("width", 120));
		int h = int(el.get("height", 36));
		el["offset_x"] = CLAMP(p_pos.x - w / 2, 0, 960 - w);
		el["offset_y"] = CLAMP(p_pos.y - h / 2, 0, 540 - h);
	}
	return add_ui_element(sid, el, true);
}

String ClockGearsDataManager::create_menu_from_wizard(const Dictionary &p_opts) {
	String mname = String(p_opts.get("name", String::utf8("Nuevo Menú"))).strip_edges();
	if (mname.is_empty()) {
		mname = String::utf8("Nuevo Menú");
	}
	String tpl_id = String(p_opts.get("template_id", "classic_jrpg")).strip_edges().to_lower();
	String ctx = _cg_norm_trigger_context(p_opts.get("trigger_context", "map"));
	bool pause_open = p_opts.has("pause_on_open") ? bool(p_opts["pause_on_open"]) : true;
	Array triggers = p_opts.has("triggers") && p_opts["triggers"].get_type() == Variant::ARRAY ? Array(p_opts["triggers"]).duplicate(true) : Array();

	String mid = create_custom_menu(mname);
	if (mid.is_empty()) {
		return String();
	}
	if (!tpl_id.is_empty()) {
		apply_menu_template(mid, tpl_id, false, false);
	}

	Dictionary d = get_ui_screen_design(mid);
	d["name"] = mname;
	d["trigger_context"] = ctx;
	d["pause_on_open"] = pause_open;
	if (p_opts.has("parent_id")) {
		String pid = String(p_opts["parent_id"]).strip_edges();
		if (!pid.is_empty()) {
			d["parent_id"] = _cg_norm_ui_screen_id(pid);
		}
	}
	if (p_opts.has("system_group")) {
		String sg = String(p_opts["system_group"]).strip_edges();
		if (!sg.is_empty()) {
			d["system_group"] = _cg_norm_ui_screen_id(sg);
		}
	} else if (d.has("parent_id") && !String(d["parent_id"]).is_empty()) {
		d["system_group"] = get_menu_system_group(String(d["parent_id"]));
	}
	if (!triggers.is_empty()) {
		Array norm_tr;
		for (int i = 0; i < triggers.size(); i++) {
			if (triggers[i].get_type() == Variant::DICTIONARY) {
				norm_tr.push_back(normalize_menu_trigger(Dictionary(triggers[i])));
			}
		}
		d["triggers"] = norm_tr;
	}

	// Personalizar botones elegidos en paso 2 del asistente
	Array sel_btns;
	if (p_opts.has("selected_buttons") && p_opts["selected_buttons"].get_type() == Variant::ARRAY) {
		sel_btns = p_opts["selected_buttons"];
	} else if (p_opts.has("buttons") && p_opts["buttons"].get_type() == Variant::ARRAY) {
		sel_btns = p_opts["buttons"];
	}
	if (!sel_btns.is_empty()) {
		auto norm_wiz_act = [](const String &p_a) -> String {
			String a = p_a.strip_edges().to_lower();
			if (a == "open_items") return "items";
			if (a == "open_equip") return "equip";
			if (a == "open_status") return "status";
			if (a == "open_save") return "save";
			if (a == "open_options") return "options";
			if (a == "close_menu") return "close";
			if (a == "continue_game") return "continue";
			if (a == "return_title") return "to_title";
			return a;
		};
		Dictionary lay = d.get("layout", Dictionary());
		if (String(lay.get("mode", "")) == "radial") {
			Array rslots;
			for (int i = 0; i < sel_btns.size(); i++) {
				Dictionary sb = sel_btns[i];
				Dictionary rs;
				rs["id"] = vformat("wiz_slot_%d", i + 1);
				rs["title"] = String(sb.get("label", sb.get("text", vformat(String::utf8("Opción %d"), i + 1))));
				rs["description"] = String(sb.get("description", ""));
				String act = norm_wiz_act(String(sb.get("action", "close")));
					rs["action"] = act;
					Dictionary st;
					st["type"] = act;
					Array sa;
					sa.push_back(st);
					rs["actions"] = sa;
					rslots.push_back(rs);
				}
				lay["radial_slots"] = rslots;
				d["layout"] = lay;
			} else {
				Array old_els = d.get("elements", Array());
				Array kept_els;
				Dictionary first_btn_tpl;
				for (int i = 0; i < old_els.size(); i++) {
					Dictionary oe = old_els[i];
					if (String(oe.get("type", "")) == "button") {
						if (first_btn_tpl.is_empty()) first_btn_tpl = oe;
					} else {
						kept_els.push_back(oe);
					}
				}
				int bx = first_btn_tpl.is_empty() ? 48 : int(first_btn_tpl.get("offset_x", 48));
				int by = first_btn_tpl.is_empty() ? 56 : int(first_btn_tpl.get("offset_y", 56));
				int bw = first_btn_tpl.is_empty() ? 200 : int(first_btn_tpl.get("width", 200));
				int bh = first_btn_tpl.is_empty() ? 42 : int(first_btn_tpl.get("height", 42));
				String banc = first_btn_tpl.is_empty() ? String("top_left") : String(first_btn_tpl.get("anchor", "top_left"));
				String bshape = first_btn_tpl.has("shape") ? String(Dictionary(first_btn_tpl["shape"]).get("shape_type", "rounded")) : String("rounded");
				for (int i = 0; i < sel_btns.size(); i++) {
					Dictionary sb = sel_btns[i];
					String lbl = String(sb.get("label", sb.get("text", vformat(String::utf8("Botón %d"), i + 1))));
					String act = norm_wiz_act(String(sb.get("action", "close")));
					Dictionary nb = _cg_make_tpl_btn(vformat("wiz_btn_%d", i + 1), lbl, act, bx, by + i * (bh + 10), bw, bh, banc, bshape, bshape == "pill" ? bh / 2 : 8);
					Dictionary st;
					st["type"] = act;
					Array ba;
					ba.push_back(st);
					nb["actions"] = ba;
					nb["focus_order"] = i;
					nb["layer_order"] = 2;
					kept_els.push_back(nb);
				}
				d["elements"] = kept_els;
			}
		}

	set_ui_screen_design(mid, d, false);
	return mid;
}

static void _cg_collect_asset_paths_from_variant(const Variant &p_var, HashSet<String> &r_paths) {
	if (p_var.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_var;
		Array keys = d.keys();
		for (int i = 0; i < keys.size(); i++) {
			Variant v = d[keys[i]];
			if (v.get_type() == Variant::STRING) {
				String s = String(v).strip_edges();
				if (s.begins_with("res://") && (s.ends_with(".png") || s.ends_with(".webp") || s.ends_with(".jpg") || s.ends_with(".jpeg") || s.ends_with(".ttf") || s.ends_with(".otf") || s.ends_with(".wav") || s.ends_with(".ogg"))) {
					r_paths.insert(s);
				}
			} else {
				_cg_collect_asset_paths_from_variant(v, r_paths);
			}
		}
	} else if (p_var.get_type() == Variant::ARRAY) {
		Array a = p_var;
		for (int i = 0; i < a.size(); i++) {
			_cg_collect_asset_paths_from_variant(a[i], r_paths);
		}
	}
}

static Variant _cg_remap_asset_paths_in_variant(const Variant &p_var, const Dictionary &p_remap) {
	if (p_var.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_var;
		Dictionary out;
		Array keys = d.keys();
		for (int i = 0; i < keys.size(); i++) {
			out[keys[i]] = _cg_remap_asset_paths_in_variant(d[keys[i]], p_remap);
		}
		return out;
	}
	if (p_var.get_type() == Variant::ARRAY) {
		Array a = p_var;
		Array out;
		for (int i = 0; i < a.size(); i++) {
			out.push_back(_cg_remap_asset_paths_in_variant(a[i], p_remap));
		}
		return out;
	}
	if (p_var.get_type() == Variant::STRING) {
		String s = String(p_var).strip_edges();
		if (p_remap.has(s)) {
			return p_remap[s];
		}
	}
	return p_var;
}

bool ClockGearsDataManager::export_template_package(const String &p_id, const String &p_dst_path, bool p_is_screen) const {
	String dst = p_dst_path.strip_edges();
	if (dst.is_empty()) {
		return false;
	}
	Dictionary payload;
	payload["package_format"] = "clockgears_ui_package_v1";
	if (p_is_screen) {
		Dictionary scr = get_ui_screen_design(p_id);
		payload["kind"] = "screen";
		payload["name"] = String(scr.get("name", p_id));
		payload["design"] = scr;
	} else {
		String tid = p_id.strip_edges().to_lower();
		Dictionary tpl = get_builtin_template_by_id(tid);
		if (tpl.is_empty() && ui_designs_config.has("user_templates") && ui_designs_config["user_templates"].get_type() == Variant::DICTIONARY) {
			Dictionary ut = ui_designs_config["user_templates"];
			if (ut.has(tid) && ut[tid].get_type() == Variant::DICTIONARY) {
				tpl = Dictionary(ut[tid]).duplicate(true);
			}
		}
		if (tpl.is_empty()) {
			return false;
		}
		payload["kind"] = "template";
		payload["name"] = String(tpl.get("name", tid));
		payload["template"] = tpl;
	}

	HashSet<String> asset_paths;
	_cg_collect_asset_paths_from_variant(payload, asset_paths);
	Dictionary embedded;
	for (const String &ap : asset_paths) {
		if (!FileAccess::exists(ap)) {
			continue;
		}
		PackedByteArray bytes = FileAccess::get_file_as_bytes(ap);
		if (!bytes.is_empty()) {
			Dictionary rec;
			rec["filename"] = ap.get_file();
			rec["base64"] = CryptoCore::b64_encode_str(bytes.ptr(), bytes.size());
			embedded[ap] = rec;
		}
	}
	payload["embedded_assets"] = embedded;
	return _write_json_atomic(dst, payload);
}

Dictionary ClockGearsDataManager::import_template_package(const String &p_src_path, bool p_import_as_custom_menu, const String &p_assets_subdir) {
	Dictionary res;
	res["ok"] = false;
	Variant parsed;
	if (!_read_json_with_backup(p_src_path, parsed, false) || parsed.get_type() != Variant::DICTIONARY) {
		return res;
	}
	Dictionary pkg = parsed;
	Dictionary remap;
	String target_dir = p_assets_subdir.strip_edges();
	if (target_dir.is_empty()) {
		target_dir = "res://img/ui/imported";
	}
	if (pkg.has("embedded_assets") && pkg["embedded_assets"].get_type() == Variant::DICTIONARY) {
		Dictionary emb = pkg["embedded_assets"];
		if (!emb.is_empty()) {
			String abs_dir = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(target_dir) : target_dir;
			DirAccess::make_dir_recursive_absolute(abs_dir);
			Array ekeys = emb.keys();
			int extracted = 0;
			for (int i = 0; i < ekeys.size(); i++) {
				String orig_path = String(ekeys[i]);
				if (emb[ekeys[i]].get_type() != Variant::DICTIONARY) continue;
				Dictionary rec = emb[ekeys[i]];
				String fname = String(rec.get("filename", orig_path.get_file())).get_file();
				String b64 = String(rec.get("base64", ""));
				if (fname.is_empty() || b64.is_empty()) continue;
				CharString cstr = b64.utf8();
				PackedByteArray raw;
				raw.resize(cstr.length());
				size_t dec_len = 0;
				if (CryptoCore::b64_decode(raw.ptrw(), raw.size(), &dec_len, (const uint8_t *)cstr.get_data(), cstr.length()) != OK || dec_len == 0) {
					continue;
				}
				raw.resize(dec_len);
				String new_res_path = target_dir.path_join(fname);
				Ref<FileAccess> fw = FileAccess::open(new_res_path, FileAccess::WRITE);
				if (fw.is_valid()) {
					fw->store_buffer(raw);
					fw->flush();
					remap[orig_path] = new_res_path;
					extracted++;
				}
			}
			res["extracted_assets"] = extracted;
		}
	}

	String kind = String(pkg.get("kind", "template"));
	String pkg_name = String(pkg.get("name", "Importado")).strip_edges();
	if (pkg_name.is_empty()) pkg_name = "Importado";

	if (kind == "screen" || pkg.has("design")) {
		Dictionary d = _cg_remap_asset_paths_in_variant(pkg.get("design", Dictionary()), remap);
		if (p_import_as_custom_menu) {
			String mid = create_custom_menu(pkg_name, d);
			res["ok"] = !mid.is_empty();
			res["menu_id"] = mid;
			res["kind"] = "menu";
		} else {
			if (!ui_designs_config.has("user_templates") || ui_designs_config["user_templates"].get_type() != Variant::DICTIONARY) {
				ui_designs_config["user_templates"] = Dictionary();
			}
			Dictionary ut = Dictionary(ui_designs_config["user_templates"]).duplicate(true);
			String base = _cg_slugify_menu_id(pkg_name);
			if (base.is_empty()) base = "imported";
			String tid = "user_" + base;
			int sfx = 2;
			while (ut.has(tid)) tid = "user_" + base + "_" + itos(sfx++);
			Dictionary tpl;
			tpl["id"] = tid;
			tpl["name"] = pkg_name;
			tpl["category"] = "user";
			tpl["description"] = String::utf8("Importado desde paquete");
			tpl["is_component"] = false;
			tpl["is_builtin"] = false;
			tpl["design"] = d;
			ut[tid] = tpl;
			ui_designs_config["user_templates"] = ut;
			dirty = true;
			emit_signal("data_changed", "ui_designs", 0);
			res["ok"] = true;
			res["template_id"] = tid;
			res["kind"] = "template";
		}
		return res;
	}

	Dictionary raw_tpl = _cg_remap_asset_paths_in_variant(pkg.get("template", Dictionary()), remap);
	if (!raw_tpl.is_empty()) {
		if (!ui_designs_config.has("user_templates") || ui_designs_config["user_templates"].get_type() != Variant::DICTIONARY) {
			ui_designs_config["user_templates"] = Dictionary();
		}
		Dictionary ut = Dictionary(ui_designs_config["user_templates"]).duplicate(true);
		String base = _cg_slugify_menu_id(String(raw_tpl.get("name", pkg_name)));
		if (base.is_empty()) base = "imported";
		String tid = "user_" + base;
		int sfx = 2;
		while (ut.has(tid)) tid = "user_" + base + "_" + itos(sfx++);
		raw_tpl["id"] = tid;
		raw_tpl["is_builtin"] = false;
		raw_tpl["category"] = bool(raw_tpl.get("is_component", false)) ? String("component") : String("user");
		ut[tid] = raw_tpl;
		ui_designs_config["user_templates"] = ut;
		dirty = true;
		emit_signal("data_changed", "ui_designs", 0);
		res["ok"] = true;
		res["template_id"] = tid;
		res["kind"] = "template";
		if (p_import_as_custom_menu && !bool(raw_tpl.get("is_component", false))) {
			String mid = create_custom_menu(pkg_name);
			apply_menu_template(mid, tid, false, false);
			res["menu_id"] = mid;
			res["kind"] = "menu";
		}
	}
	return res;
}

String ClockGearsDataManager::duplicate_ui_element(const String &p_screen_id, const String &p_elem_id, const Vector2i &p_offset_delta) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary src = get_ui_element(sid, p_elem_id);
	if (src.is_empty()) {
		return String();
	}
	Dictionary d = get_ui_screen_design(sid);
	Array elems = Array(d.get("elements", Array())).duplicate(true);
	HashMap<String, Dictionary> by_id;
	HashMap<String, Vector<String>> children;
	HashMap<String, int> orig_idx;
	_cg_collect_ui_tree_sorted(elems, by_id, children, orig_idx);

	String root_canon = String(src.get("id", p_elem_id));
	push_ui_design_undo_state(sid);

	struct CloneHelper {
		static String run(ClockGearsDataManager *dm, const String &sid, const String &old_id, const String &new_parent_id, const Vector2i &delta, bool is_root, const HashMap<String, Dictionary> &by_id, const HashMap<String, Vector<String>> &children) {
			if (!by_id.has(old_id)) {
				return String();
			}
			Dictionary copy = by_id[old_id].duplicate(true);
			copy.erase("id");
			if (is_root) {
				copy["name"] = String(copy.get("name", "Elemento")) + " (Copia)";
				copy["offset_x"] = CLAMP(int(copy.get("offset_x", 0)) + delta.x, -960, 960);
				copy["offset_y"] = CLAMP(int(copy.get("offset_y", 0)) + delta.y, -540, 540);
			} else {
				copy["parent_id"] = new_parent_id;
			}
			String nid = dm->add_ui_element(sid, copy, false);
			if (!nid.is_empty() && children.has(old_id)) {
				const Vector<String> &kids = children[old_id];
				for (int i = kids.size() - 1; i >= 0; i--) {
					(void)run(dm, sid, kids[i], nid, Vector2i(), false, by_id, children);
				}
			}
			return nid;
		}
	};
	return CloneHelper::run(this, sid, root_canon, String(src.get("parent_id", "")), p_offset_delta, true, by_id, children);
}

void ClockGearsDataManager::copy_ui_elements_to_clipboard(const String &p_screen_id, const PackedStringArray &p_elem_ids) {
	ui_elements_clipboard.clear();
	for (int i = 0; i < p_elem_ids.size(); i++) {
		Dictionary el = get_ui_element(p_screen_id, p_elem_ids[i]);
		if (!el.is_empty()) {
			ui_elements_clipboard.push_back(el.duplicate(true));
		}
	}
}

PackedStringArray ClockGearsDataManager::paste_ui_elements_from_clipboard(const String &p_screen_id) {
	PackedStringArray added;
	if (ui_elements_clipboard.is_empty()) {
		return added;
	}
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	push_ui_design_undo_state(sid);
	for (int i = 0; i < ui_elements_clipboard.size(); i++) {
		if (ui_elements_clipboard[i].get_type() != Variant::DICTIONARY) continue;
		Dictionary el = Dictionary(ui_elements_clipboard[i]).duplicate(true);
		el.erase("id");
		el["offset_x"] = CLAMP(int(el.get("offset_x", 0)) + 16, -960, 960);
		el["offset_y"] = CLAMP(int(el.get("offset_y", 0)) + 16, -540, 540);
		String nid = add_ui_element(sid, el, false);
		if (!nid.is_empty()) {
			added.push_back(nid);
		}
	}
	return added;
}

PackedStringArray ClockGearsDataManager::get_recent_ui_colors() const {
	PackedStringArray out;
	if (ui_designs_config.has("recent_colors") && ui_designs_config["recent_colors"].get_type() == Variant::ARRAY) {
		Array rc = ui_designs_config["recent_colors"];
		for (int i = 0; i < rc.size(); i++) {
			out.push_back(String(rc[i]));
		}
	}
	return out;
}

void ClockGearsDataManager::record_recent_ui_color(const String &p_hex_color) {
	String c = _cg_norm_color_rgba(p_hex_color, "#e0a040ff");
	Array rc;
	rc.push_back(c);
	if (ui_designs_config.has("recent_colors") && ui_designs_config["recent_colors"].get_type() == Variant::ARRAY) {
		Array prev = ui_designs_config["recent_colors"];
		for (int i = 0; i < prev.size() && rc.size() < 12; i++) {
			String pc = String(prev[i]);
			if (pc != c) rc.push_back(pc);
		}
	}
	ui_designs_config["recent_colors"] = rc;
	dirty = true;
}

String ClockGearsDataManager::resolve_ui_data_binding(const String &p_binding, const Dictionary &p_element) const {
	String b = _cg_norm_ui_data_binding(p_binding);
	if (b == "none") {
		return String();
	}
	if (b == "project_title") {
		String t = get_effective_game_title().strip_edges();
		return t.is_empty() ? String("ClockGears RPG") : t;
	}
	if (b == "currency_name") {
		return get_currency_unit();
	}
	if (b == "gold") {
		return String::num_int64(runtime_gold);
	}
	if (b == "item_qty") {
		String ik = String(p_element.get("item_key", "")).strip_edges();
		return ik.is_empty() ? String("0") : String::num_int64(get_item_qty(ik));
	}
	if (b == "variable") {
		String vn = String(p_element.get("variable_name", "")).strip_edges();
		return vn.is_empty() ? String("0") : String::num_int64(get_variable(vn));
	}
	if (b == "playtime") {
		return format_playtime_hms(runtime_playtime_sec);
	}

	int actor_id = int(p_element.get("actor_id", 0));
	if (actor_id <= 0) {
		Array party = get_party_actor_ids();
		if (!party.is_empty()) {
			actor_id = int(party[0]);
		} else if (!category_data[CAT_ACTORS].is_empty()) {
			actor_id = int(Dictionary(category_data[CAT_ACTORS][0]).get("id", 1));
		}
	}
	if (b == "player_hp") {
		if (actor_id <= 0) {
			return "0/0";
		}
		ClockGearsDataManager *self_mut = const_cast<ClockGearsDataManager *>(this);
		return vformat("%d/%d", self_mut->get_actor_runtime_hp(actor_id), self_mut->get_actor_runtime_max_hp(actor_id));
	}
	if (b == "player_mp") {
		if (actor_id <= 0) {
			return "0/0";
		}
		ClockGearsDataManager *self_mut = const_cast<ClockGearsDataManager *>(this);
		return vformat("%d/%d", self_mut->get_actor_runtime_mp(actor_id), self_mut->get_actor_runtime_max_mp(actor_id));
	}
	if (b == "actor_name") {
		if (actor_id <= 0) {
			return String();
		}
		Dictionary rec = get_record_by_id(CAT_ACTORS, actor_id);
		return String(rec.get("name", ""));
	}
	if (b == "actor_level") {
		if (actor_id <= 0) {
			return "1";
		}
		ClockGearsDataManager *self_mut = const_cast<ClockGearsDataManager *>(this);
		return String::num_int64(self_mut->get_actor_runtime_level(actor_id));
	}
	return String();
}

void ClockGearsDataManager::push_ui_design_undo_state(const String &p_screen_id, const Dictionary &p_snapshot) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	Dictionary snap = p_snapshot.is_empty() ? get_ui_screen_design(sid) : normalize_ui_screen_design(sid, p_snapshot);
	_push_ui_undo_snapshot_internal(sid, snap, true);
}

bool ClockGearsDataManager::can_undo_ui_design(const String &p_screen_id) const {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	return ui_design_undo_stacks.has(sid) && !ui_design_undo_stacks[sid].is_empty();
}

bool ClockGearsDataManager::can_redo_ui_design(const String &p_screen_id) const {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	return ui_design_redo_stacks.has(sid) && !ui_design_redo_stacks[sid].is_empty();
}

int ClockGearsDataManager::get_ui_design_undo_count(const String &p_screen_id) const {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	return ui_design_undo_stacks.has(sid) ? ui_design_undo_stacks[sid].size() : 0;
}

int ClockGearsDataManager::get_ui_design_redo_count(const String &p_screen_id) const {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	return ui_design_redo_stacks.has(sid) ? ui_design_redo_stacks[sid].size() : 0;
}

bool ClockGearsDataManager::undo_ui_design(const String &p_screen_id) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	if (!can_undo_ui_design(sid)) {
		return false;
	}
	Vector<Dictionary> &undo_stack = ui_design_undo_stacks[sid];
	Dictionary prev = undo_stack[undo_stack.size() - 1];
	undo_stack.remove_at(undo_stack.size() - 1);

	Vector<Dictionary> &redo_stack = ui_design_redo_stacks[sid];
	redo_stack.push_back(get_ui_screen_design(sid));
	while (redo_stack.size() > MAX_UI_DESIGN_UNDO_STEPS) {
		redo_stack.remove_at(0);
	}

	set_ui_screen_design(sid, prev, false);
	return true;
}

bool ClockGearsDataManager::redo_ui_design(const String &p_screen_id) {
	String sid = _cg_norm_ui_screen_id(p_screen_id);
	if (!can_redo_ui_design(sid)) {
		return false;
	}
	Vector<Dictionary> &redo_stack = ui_design_redo_stacks[sid];
	Dictionary next = redo_stack[redo_stack.size() - 1];
	redo_stack.remove_at(redo_stack.size() - 1);

	_push_ui_undo_snapshot_internal(sid, get_ui_screen_design(sid), false);
	set_ui_screen_design(sid, next, false);
	return true;
}

void ClockGearsDataManager::clear_ui_design_history(const String &p_screen_id) {
	String raw = p_screen_id.strip_edges();
	if (raw.is_empty()) {
		ui_design_undo_stacks.clear();
		ui_design_redo_stacks.clear();
		return;
	}
	String sid = _cg_norm_ui_screen_id(raw);
	ui_design_undo_stacks.erase(sid);
	ui_design_redo_stacks.erase(sid);
}

// --- Estilos de Ventana y Botones (9B Tanda 2) ---

namespace {
struct CGWindowTypeEntry {
	String id;
	String label;
};

static Vector<CGWindowTypeEntry> &s_get_window_type_registry() {
	static Vector<CGWindowTypeEntry> registry;
	return registry;
}

static String s_normalize_border_type(const String &p_raw) {
	String v = p_raw.strip_edges().to_lower();
	if (v == "rect" || v == "recto" || v == "square" || v == "cuadrado") {
		return "rect";
	}
	if (v == "none" || v == "sin borde" || v == "sin bordes" || v == "sin_borde" || v == "sin_bordes") {
		return "none";
	}
	if (v == "image" || v == "imagen" || v == "nine_slice" || v == "9slice") {
		return "image";
	}
	return "rounded";
}

static String s_normalize_fill_type(const String &p_raw) {
	String v = p_raw.strip_edges().to_lower();
	if (v == "image" || v == "imagen" || v == "texture" || v == "textura") {
		return "image";
	}
	return "color";
}

static String s_normalize_center_mode(const String &p_raw) {
	String v = p_raw.strip_edges().to_lower();
	if (v == "repeat" || v == "repetir" || v == "tile") {
		return "repeat";
	}
	return "stretch";
}

static String s_normalize_color_html(const Variant &p_val, const String &p_fallback, double p_opacity_override = -1.0) {
	Color c;
	bool valid = false;
	if (p_val.get_type() == Variant::COLOR) {
		c = Color(p_val);
		valid = true;
	} else {
		String s = String(p_val).strip_edges();
		if (!s.is_empty() && Color::html_is_valid(s)) {
			c = Color::html(s);
			valid = true;
		}
	}
	if (!valid) {
		c = Color::html(p_fallback);
	}
	if (p_opacity_override >= 0.0) {
		c.a = CLAMP((float)p_opacity_override, 0.0f, 1.0f);
	}
	return "#" + c.to_html(true);
}

static Dictionary s_normalize_insets_dict(const Variant &p_val, const Dictionary &p_default) {
	int dl = MAX(0, int(p_default.get("left", 0)));
	int dt = MAX(0, int(p_default.get("top", 0)));
	int dr = MAX(0, int(p_default.get("right", 0)));
	int db = MAX(0, int(p_default.get("bottom", 0)));
	if (p_val.get_type() == Variant::INT || p_val.get_type() == Variant::FLOAT) {
		int uniform = MAX(0, int(p_val));
		dl = dt = dr = db = uniform;
	} else if (p_val.get_type() == Variant::DICTIONARY) {
		Dictionary d = p_val;
		dl = MAX(0, int(d.get("left", dl)));
		dt = MAX(0, int(d.get("top", dt)));
		dr = MAX(0, int(d.get("right", dr)));
		db = MAX(0, int(d.get("bottom", db)));
	}
	Dictionary out;
	out["left"] = dl;
	out["top"] = dt;
	out["right"] = dr;
	out["bottom"] = db;
	return out;
}

static Dictionary s_make_insets_dict(int p_left, int p_top, int p_right, int p_bottom) {
	Dictionary out;
	out["left"] = MAX(0, p_left);
	out["top"] = MAX(0, p_top);
	out["right"] = MAX(0, p_right);
	out["bottom"] = MAX(0, p_bottom);
	return out;
}

static Dictionary s_make_surface_props(
		const String &p_border_type,
		int p_border_width,
		int p_corner_radius,
		const String &p_border_color,
		const String &p_fill_type,
		const String &p_fill_color,
		const String &p_fill_image_path,
		const Dictionary &p_padding,
		const String &p_image_path,
		const Dictionary &p_nine_slice,
		const String &p_center_mode,
		bool p_pixel_art,
		const String &p_font_color) {
	Dictionary out;
	out["border_type"] = s_normalize_border_type(p_border_type);
	out["border_width"] = (out["border_type"] == "none") ? 0 : MAX(0, p_border_width);
	out["corner_radius"] = (out["border_type"] == "rect" || out["border_type"] == "none") ? 0 : MAX(0, p_corner_radius);
	out["border_color"] = s_normalize_color_html(p_border_color, "#e0a040ff");
	out["fill_type"] = s_normalize_fill_type(p_fill_type);
	out["fill_color"] = s_normalize_color_html(p_fill_color, "#14171ff0");
	out["fill_image_path"] = p_fill_image_path.strip_edges();
	out["padding"] = p_padding.duplicate(true);
	out["image_path"] = p_image_path.strip_edges();
	out["nine_slice"] = p_nine_slice.duplicate(true);
	out["center_mode"] = s_normalize_center_mode(p_center_mode);
	out["pixel_art"] = p_pixel_art;
	out["font_color"] = s_normalize_color_html(p_font_color, "#eef2faff");
	return out;
}

static Dictionary s_normalize_surface_props(const Dictionary &p_raw, const Dictionary &p_fallback) {
	String bt = s_normalize_border_type(String(p_raw.get("border_type", p_fallback.get("border_type", "rounded"))));
	int bw = MAX(0, int(p_raw.get("border_width", p_fallback.get("border_width", 2))));
	int cr = MAX(0, int(p_raw.get("corner_radius", p_fallback.get("corner_radius", 8))));
	if (bt == "none") {
		bw = 0;
		cr = 0;
	} else if (bt == "rect") {
		cr = 0;
	}
	double opacity_override = p_raw.has("fill_opacity") ? double(p_raw["fill_opacity"]) : -1.0;
	String bcol = s_normalize_color_html(p_raw.get("border_color", p_fallback.get("border_color", "#e0a040ff")), String(p_fallback.get("border_color", "#e0a040ff")));
	String fcol = s_normalize_color_html(p_raw.get("fill_color", p_fallback.get("fill_color", "#14171ff0")), String(p_fallback.get("fill_color", "#14171ff0")), opacity_override);
	String ft = s_normalize_fill_type(String(p_raw.get("fill_type", p_fallback.get("fill_type", "color"))));
	String fimg = String(p_raw.get("fill_image_path", p_fallback.get("fill_image_path", ""))).strip_edges();
	Dictionary pad = s_normalize_insets_dict(p_raw.has("padding") ? p_raw["padding"] : p_fallback.get("padding", s_make_insets_dict(12, 10, 12, 10)),
			Dictionary(p_fallback.get("padding", s_make_insets_dict(12, 10, 12, 10))));
	String img = String(p_raw.get("image_path", p_fallback.get("image_path", ""))).strip_edges();
	Dictionary slice = s_normalize_insets_dict(p_raw.has("nine_slice") ? p_raw["nine_slice"] : p_fallback.get("nine_slice", s_make_insets_dict(8, 8, 8, 8)),
			Dictionary(p_fallback.get("nine_slice", s_make_insets_dict(8, 8, 8, 8))));
	String cm = s_normalize_center_mode(String(p_raw.get("center_mode", p_fallback.get("center_mode", "stretch"))));
	bool pa = p_raw.has("pixel_art") ? bool(p_raw["pixel_art"]) : bool(p_fallback.get("pixel_art", true));
	String tcol = s_normalize_color_html(p_raw.get("font_color", p_fallback.get("font_color", "#eef2faff")), String(p_fallback.get("font_color", "#eef2faff")));

	Dictionary out = s_make_surface_props(bt, bw, cr, bcol, ft, fcol, fimg, pad, img, slice, cm, pa, tcol);
	// Preservar propiedades avanzadas de forma/degradado/sombra/brillo (3F) solo si están presentes
	if (p_raw.has("shape_type") || p_fallback.has("shape_type")) {
		String st = _cg_norm_shape_type(p_raw.get("shape_type", p_fallback.get("shape_type", "")));
		if (!st.is_empty()) out["shape_type"] = st;
	}
	if (p_raw.has("clip_content") || p_fallback.has("clip_content")) {
		out["clip_content"] = p_raw.has("clip_content") ? bool(p_raw["clip_content"]) : bool(p_fallback["clip_content"]);
	}
	if (p_raw.has("gradient_enabled") || p_fallback.has("gradient_enabled")) {
		out["gradient_enabled"] = p_raw.has("gradient_enabled") ? bool(p_raw["gradient_enabled"]) : bool(p_fallback["gradient_enabled"]);
		out["gradient_top_color"] = s_normalize_color_html(p_raw.get("gradient_top_color", p_fallback.get("gradient_top_color", "#28221aff")), "#28221aff");
		out["gradient_bottom_color"] = s_normalize_color_html(p_raw.get("gradient_bottom_color", p_fallback.get("gradient_bottom_color", "#14110eff")), "#14110eff");
		String ga = String(p_raw.get("gradient_angle", p_fallback.get("gradient_angle", "vertical"))).strip_edges().to_lower();
		out["gradient_angle"] = (ga == "horizontal") ? "horizontal" : "vertical";
	}
	if (p_raw.has("shadow_enabled") || p_fallback.has("shadow_enabled")) {
		out["shadow_enabled"] = p_raw.has("shadow_enabled") ? bool(p_raw["shadow_enabled"]) : bool(p_fallback["shadow_enabled"]);
		out["shadow_color"] = s_normalize_color_html(p_raw.get("shadow_color", p_fallback.get("shadow_color", "#00000073")), "#00000073");
		out["shadow_size"] = CLAMP(int(p_raw.get("shadow_size", p_fallback.get("shadow_size", 4))), 0, 32);
		out["shadow_offset_x"] = CLAMP(int(p_raw.get("shadow_offset_x", p_fallback.get("shadow_offset_x", 0))), -32, 32);
		out["shadow_offset_y"] = CLAMP(int(p_raw.get("shadow_offset_y", p_fallback.get("shadow_offset_y", 2))), -32, 32);
	}
	if (p_raw.has("glow_enabled") || p_fallback.has("glow_enabled")) {
		out["glow_enabled"] = p_raw.has("glow_enabled") ? bool(p_raw["glow_enabled"]) : bool(p_fallback["glow_enabled"]);
		out["glow_color"] = s_normalize_color_html(p_raw.get("glow_color", p_fallback.get("glow_color", "#f5bc5466")), "#f5bc5466");
		out["glow_size"] = CLAMP(int(p_raw.get("glow_size", p_fallback.get("glow_size", 4))), 0, 24);
	}
	return out;
}

static Dictionary s_make_builtin_style_rounded() {
	Dictionary pad = s_make_insets_dict(12, 10, 12, 10);
	Dictionary btn_pad = s_make_insets_dict(12, 6, 12, 6);
	Dictionary slice = s_make_insets_dict(8, 8, 8, 8);
	Dictionary s = s_make_surface_props("rounded", 2, 8, "#e0a040ff", "color", "#14171ff0", "", pad, "", slice, "stretch", true, "#eef2faff");
	s["id"] = "rounded";
	s["name"] = "Redondeado";
	s["built_in"] = true;

	Dictionary btns;
	Dictionary st_norm = s_make_surface_props("rounded", 1, 8, "#e0a040ff", "color", "#1f2536f0", "", btn_pad, "", slice, "stretch", true, "#eef2faff");
	st_norm["inherit_normal"] = false;
	Dictionary st_hover = s_make_surface_props("rounded", 1, 8, "#ffd25aff", "color", "#2e3854fa", "", btn_pad, "", slice, "stretch", true, "#ffffffff");
	st_hover["inherit_normal"] = false;
	Dictionary st_pressed = s_make_surface_props("rounded", 1, 8, "#c88a2eff", "color", "#141a29fa", "", btn_pad, "", slice, "stretch", true, "#ffe6a8ff");
	st_pressed["inherit_normal"] = false;
	Dictionary st_disabled = s_make_surface_props("rounded", 1, 8, "#474e5db3", "color", "#14171f8c", "", btn_pad, "", slice, "stretch", true, "#737a8cff");
	st_disabled["inherit_normal"] = false;

	btns["normal"] = st_norm;
	btns["hover"] = st_hover;
	btns["pressed"] = st_pressed;
	btns["disabled"] = st_disabled;
	s["buttons"] = btns;
	return s;
}

static Dictionary s_make_builtin_style_rect() {
	Dictionary pad = s_make_insets_dict(12, 10, 12, 10);
	Dictionary btn_pad = s_make_insets_dict(12, 6, 12, 6);
	Dictionary slice = s_make_insets_dict(8, 8, 8, 8);
	Dictionary s = s_make_surface_props("rect", 2, 0, "#d8dde8ff", "color", "#10131cf2", "", pad, "", slice, "stretch", true, "#f0f4fcff");
	s["id"] = "rect";
	s["name"] = "Recto";
	s["built_in"] = true;

	Dictionary btns;
	Dictionary st_norm = s_make_surface_props("rect", 2, 0, "#9aa6bdff", "color", "#1a2030f2", "", btn_pad, "", slice, "stretch", true, "#f0f4fcff");
	st_norm["inherit_normal"] = false;
	Dictionary st_hover = s_make_surface_props("rect", 2, 0, "#ffffffff", "color", "#2a344dfa", "", btn_pad, "", slice, "stretch", true, "#ffffffff");
	st_hover["inherit_normal"] = false;
	Dictionary st_pressed = s_make_surface_props("rect", 2, 0, "#e0a040ff", "color", "#111622fa", "", btn_pad, "", slice, "stretch", true, "#ffe6a8ff");
	st_pressed["inherit_normal"] = false;
	Dictionary st_disabled = s_make_surface_props("rect", 2, 0, "#464d5cb3", "color", "#10131c8c", "", btn_pad, "", slice, "stretch", true, "#70788aff");
	st_disabled["inherit_normal"] = false;

	btns["normal"] = st_norm;
	btns["hover"] = st_hover;
	btns["pressed"] = st_pressed;
	btns["disabled"] = st_disabled;
	s["buttons"] = btns;
	return s;
}

static Dictionary s_make_builtin_style_none() {
	Dictionary pad = s_make_insets_dict(10, 8, 10, 8);
	Dictionary btn_pad = s_make_insets_dict(10, 6, 10, 6);
	Dictionary slice = s_make_insets_dict(8, 8, 8, 8);
	Dictionary s = s_make_surface_props("none", 0, 0, "#00000000", "color", "#0f121ae6", "", pad, "", slice, "stretch", true, "#eef2faff");
	s["id"] = "none";
	s["name"] = "Sin bordes";
	s["built_in"] = true;

	Dictionary btns;
	Dictionary st_norm = s_make_surface_props("none", 0, 0, "#00000000", "color", "#1c2233d9", "", btn_pad, "", slice, "stretch", true, "#eef2faff");
	st_norm["inherit_normal"] = false;
	Dictionary st_hover = s_make_surface_props("none", 0, 0, "#00000000", "color", "#2f3a57f2", "", btn_pad, "", slice, "stretch", true, "#ffffffff");
	st_hover["inherit_normal"] = false;
	Dictionary st_pressed = s_make_surface_props("none", 0, 0, "#00000000", "color", "#121724f2", "", btn_pad, "", slice, "stretch", true, "#ffe6a8ff");
	st_pressed["inherit_normal"] = false;
	Dictionary st_disabled = s_make_surface_props("none", 0, 0, "#00000000", "color", "#12151f73", "", btn_pad, "", slice, "stretch", true, "#70788aff");
	st_disabled["inherit_normal"] = false;

	btns["normal"] = st_norm;
	btns["hover"] = st_hover;
	btns["pressed"] = st_pressed;
	btns["disabled"] = st_disabled;
	s["buttons"] = btns;
	return s;
}

static String s_slugify_style_id(const String &p_name) {
	String base = p_name.strip_edges().to_lower();
	String out;
	bool last_us = false;
	for (int i = 0; i < base.length(); i++) {
		char32_t c = base[i];
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
			out += c;
			last_us = false;
		} else if (!last_us && !out.is_empty()) {
			out += '_';
			last_us = true;
		}
	}
	while (out.ends_with("_")) {
		out = out.substr(0, out.length() - 1);
	}
	return out.is_empty() ? String("style") : out;
}
} // namespace

void ClockGearsDataManager::ensure_default_window_types_registered() {
	register_window_type("message", "Mensajes");
	register_window_type("choices", "Elecciones");
	register_window_type("battle_party", "Panel de grupo (Batalla)");
	register_window_type("battle_commands", "Panel de comandos (Batalla)");
	register_window_type("title_buttons", String::utf8("Botones del título"));
	register_window_type("options", "Opciones");
	register_window_type("save_list", "Lista de partidas");
	register_window_type("game_menu", String::utf8("Menú del juego"));
	register_window_type("hud", "HUD y Widgets");
	register_window_type("object_preview", String::utf8("Vista previa de objeto"));
	register_window_type("object_desc", String::utf8("Descripción de objeto"));
}

void ClockGearsDataManager::register_window_type(const String &p_id, const String &p_label) {
	String id = p_id.strip_edges().to_lower();
	if (id.is_empty()) {
		return;
	}
	String label = p_label.strip_edges().is_empty() ? p_id : p_label.strip_edges();
	Vector<CGWindowTypeEntry> &reg = s_get_window_type_registry();
	for (int i = 0; i < reg.size(); i++) {
		if (reg[i].id == id) {
			if (!p_label.strip_edges().is_empty()) {
				reg.write[i].label = label;
			}
			return;
		}
	}
	reg.push_back({ id, label });
}

Array ClockGearsDataManager::get_registered_window_types() {
	ensure_default_window_types_registered();
	const Vector<CGWindowTypeEntry> &reg = s_get_window_type_registry();
	Array out;
	for (int i = 0; i < reg.size(); i++) {
		Dictionary d;
		d["id"] = reg[i].id;
		d["label"] = reg[i].label;
		out.push_back(d);
	}
	return out;
}

Dictionary ClockGearsDataManager::normalize_window_style(const Dictionary &p_style) {
	Dictionary fallback = s_make_builtin_style_rounded();
	String id = String(p_style.get("id", "")).strip_edges().to_lower();
	if (id == "rect") {
		fallback = s_make_builtin_style_rect();
	} else if (id == "none") {
		fallback = s_make_builtin_style_none();
	}

	Dictionary out = s_normalize_surface_props(p_style, fallback);
	String name = String(p_style.get("name", fallback.get("name", "Nuevo estilo"))).strip_edges();
	if (name.is_empty()) {
		name = "Nuevo estilo";
	}
	if (id.is_empty()) {
		id = s_slugify_style_id(name);
	}
	bool is_builtin = (id == "rounded" || id == "rect" || id == "none");
	out["id"] = id;
	out["name"] = name;
	out["built_in"] = is_builtin;

	Dictionary fallback_btns = fallback["buttons"];
	Dictionary raw_btns = (p_style.has("buttons") && p_style["buttons"].get_type() == Variant::DICTIONARY) ? Dictionary(p_style["buttons"]) : Dictionary();

	// Fallback de normal: hereda de los propios valores de la ventana si se crea un estilo desde cero sin buttons.normal
	Dictionary base_norm_fallback = fallback_btns["normal"];
	if (!p_style.has("buttons")) {
		base_norm_fallback = out.duplicate(true);
		base_norm_fallback["padding"] = s_make_insets_dict(12, 6, 12, 6);
		if (out["border_type"] == "rounded" && !is_builtin) {
			base_norm_fallback["border_width"] = MAX(1, int(out["border_width"]));
		}
	}

	Dictionary raw_norm = (raw_btns.has("normal") && raw_btns["normal"].get_type() == Variant::DICTIONARY) ? Dictionary(raw_btns["normal"]) : Dictionary();
	Dictionary norm_btn = s_normalize_surface_props(raw_norm, base_norm_fallback);
	norm_btn["inherit_normal"] = false;

	Dictionary btns_out;
	btns_out["normal"] = norm_btn;

	static const char *k_states[3] = { "hover", "pressed", "disabled" };
	for (int i = 0; i < 3; i++) {
		String sk = k_states[i];
		Dictionary fb_state = is_builtin ? Dictionary(fallback_btns[sk]) : norm_btn;
		Dictionary raw_st = (raw_btns.has(sk) && raw_btns[sk].get_type() == Variant::DICTIONARY) ? Dictionary(raw_btns[sk]) : Dictionary();
		bool inherit = raw_st.has("inherit_normal") ? bool(raw_st["inherit_normal"]) : (raw_st.is_empty() && !is_builtin);
		Dictionary st_out;
		if (inherit) {
			st_out = norm_btn.duplicate(true);
			st_out["inherit_normal"] = true;
		} else {
			st_out = s_normalize_surface_props(raw_st, raw_st.is_empty() ? fb_state : norm_btn);
			st_out["inherit_normal"] = false;
		}
		btns_out[sk] = st_out;
	}
	out["buttons"] = btns_out;
	return out;
}

Dictionary ClockGearsDataManager::get_default_window_styles_config() {
	ensure_default_window_types_registered();
	Dictionary cfg;
	cfg["schema_version"] = 1;
	cfg["default_style_id"] = "rounded";

	Dictionary assignments;
	const Vector<CGWindowTypeEntry> &reg = s_get_window_type_registry();
	for (int i = 0; i < reg.size(); i++) {
		assignments[reg[i].id] = "";
	}
	cfg["assignments"] = assignments;

	Array styles;
	styles.push_back(s_make_builtin_style_rounded());
	styles.push_back(s_make_builtin_style_rect());
	styles.push_back(s_make_builtin_style_none());
	cfg["styles"] = styles;
	return cfg;
}

Dictionary ClockGearsDataManager::normalize_window_styles_config(const Dictionary &p_cfg) {
	ensure_default_window_types_registered();
	Dictionary out;
	out["schema_version"] = int(p_cfg.get("schema_version", 1));

	HashMap<String, Dictionary> by_id;
	Vector<String> order;

	auto add_or_replace = [&](const Dictionary &p_st) {
		Dictionary norm = normalize_window_style(p_st);
		String id = norm["id"];
		if (!by_id.has(id)) {
			order.push_back(id);
		}
		by_id[id] = norm;
	};

	if (p_cfg.has("styles") && p_cfg["styles"].get_type() == Variant::ARRAY) {
		Array raw_styles = p_cfg["styles"];
		for (int i = 0; i < raw_styles.size(); i++) {
			if (raw_styles[i].get_type() == Variant::DICTIONARY) {
				add_or_replace(raw_styles[i]);
			}
		}
	}

	// Garantizar que los 3 estilos incluidos siempre existan (no se pueden borrar)
	if (!by_id.has("rounded")) {
		order.insert(0, "rounded");
		by_id["rounded"] = s_make_builtin_style_rounded();
	}
	if (!by_id.has("rect")) {
		order.push_back("rect");
		by_id["rect"] = s_make_builtin_style_rect();
	}
	if (!by_id.has("none")) {
		order.push_back("none");
		by_id["none"] = s_make_builtin_style_none();
	}

	Array norm_styles;
	for (int i = 0; i < order.size(); i++) {
		norm_styles.push_back(by_id[order[i]]);
	}
	out["styles"] = norm_styles;

	String def_id = String(p_cfg.get("default_style_id", "rounded")).strip_edges().to_lower();
	if (!by_id.has(def_id)) {
		def_id = by_id.has("rounded") ? String("rounded") : order[0];
	}
	out["default_style_id"] = def_id;

	Dictionary norm_assign;
	const Vector<CGWindowTypeEntry> &reg = s_get_window_type_registry();
	for (int i = 0; i < reg.size(); i++) {
		norm_assign[reg[i].id] = "";
	}
	if (p_cfg.has("assignments") && p_cfg["assignments"].get_type() == Variant::DICTIONARY) {
		Dictionary raw_assign = p_cfg["assignments"];
		Array keys = raw_assign.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k = String(keys[i]).strip_edges().to_lower();
			if (k.is_empty()) {
				continue;
			}
			String v = String(raw_assign[keys[i]]).strip_edges().to_lower();
			norm_assign[k] = (!v.is_empty() && by_id.has(v)) ? v : String("");
		}
	}
	out["assignments"] = norm_assign;
	return out;
}

void ClockGearsDataManager::_invalidate_window_styles_cache() {
	cached_ws_norm.clear();
	cached_ws_by_id.clear();
	cached_ws_norm_gen = 0;
	window_styles_generation++;
}

void ClockGearsDataManager::_ensure_window_styles_cache() const {
	if (cached_ws_norm_gen == window_styles_generation && !cached_ws_norm.is_empty()) {
		return;
	}
	cached_ws_norm = normalize_window_styles_config(window_styles_config);
	cached_ws_by_id.clear();
	Array styles = cached_ws_norm.get("styles", Array());
	for (int i = 0; i < styles.size(); i++) {
		if (styles[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary s = styles[i];
		String id = String(s.get("id", "")).strip_edges().to_lower();
		if (!id.is_empty()) {
			cached_ws_by_id[id] = s;
		}
	}
	cached_ws_norm_gen = window_styles_generation;
}

Ref<TileSet> ClockGearsDataManager::get_cached_tileset(const String &p_key) const {
	if (const Ref<TileSet> *found = built_tileset_cache.getptr(p_key)) {
		return *found;
	}
	return Ref<TileSet>();
}

void ClockGearsDataManager::set_cached_tileset(const String &p_key, const Ref<TileSet> &p_tileset) const {
	if (p_tileset.is_valid()) {
		built_tileset_cache[p_key] = p_tileset;
	}
}

void ClockGearsDataManager::set_window_styles_config(const Dictionary &p_cfg) {
	window_styles_config = normalize_window_styles_config(p_cfg);
	_invalidate_window_styles_cache();
	dirty = true;
	emit_signal("data_changed", "window_styles", 0);
}

Array ClockGearsDataManager::get_window_styles() const {
	_ensure_window_styles_cache();
	return Array(cached_ws_norm.get("styles", Array())).duplicate(true);
}

Dictionary ClockGearsDataManager::get_window_style(const String &p_style_id) const {
	_ensure_window_styles_cache();
	String target = p_style_id.strip_edges().to_lower();
	if (const Dictionary *found = cached_ws_by_id.getptr(target)) {
		return found->duplicate(true);
	}
	return Dictionary();
}

String ClockGearsDataManager::create_window_style(const Dictionary &p_style) {
	Dictionary raw = p_style.duplicate(true);
	raw["built_in"] = false;
	String base_id = String(raw.get("id", "")).strip_edges().to_lower();
	if (base_id.is_empty() || base_id == "rounded" || base_id == "rect" || base_id == "none") {
		base_id = s_slugify_style_id(String(raw.get("name", "custom_style")));
		if (base_id == "rounded" || base_id == "rect" || base_id == "none") {
			base_id += "_custom";
		}
	}

	Array existing = get_window_styles();
	HashMap<String, bool> used;
	for (int i = 0; i < existing.size(); i++) {
		used[String(Dictionary(existing[i]).get("id", ""))] = true;
	}
	String unique_id = base_id;
	int suffix = 2;
	while (used.has(unique_id)) {
		unique_id = vformat("%s_%d", base_id, suffix++);
	}
	raw["id"] = unique_id;
	Dictionary norm = normalize_window_style(raw);
	norm["built_in"] = false;

	Dictionary cfg = normalize_window_styles_config(window_styles_config);
	Array styles = cfg["styles"];
	styles.push_back(norm);
	cfg["styles"] = styles;
	set_window_styles_config(cfg);
	return unique_id;
}

String ClockGearsDataManager::duplicate_window_style(const String &p_source_id, const String &p_new_name) {
	Dictionary src = get_window_style(p_source_id);
	if (src.is_empty()) {
		src = get_window_style("rounded");
	}
	Dictionary copy = src.duplicate(true);
	copy["built_in"] = false;
	copy.erase("id");
	String new_name = p_new_name.strip_edges();
	if (new_name.is_empty()) {
		new_name = vformat("%s (Copia)", String(src.get("name", "Estilo")));
	}
	copy["name"] = new_name;
	return create_window_style(copy);
}

bool ClockGearsDataManager::update_window_style(const String &p_style_id, const Dictionary &p_style) {
	String target = p_style_id.strip_edges().to_lower();
	Dictionary cfg = normalize_window_styles_config(window_styles_config);
	Array styles = cfg["styles"];
	for (int i = 0; i < styles.size(); i++) {
		Dictionary cur = styles[i];
		if (String(cur.get("id", "")) == target) {
			Dictionary merged = cur.duplicate(true);
			Array keys = p_style.keys();
			for (int k = 0; k < keys.size(); k++) {
				String key = String(keys[k]);
				if (key == "id" || key == "built_in") {
					continue;
				}
				Variant val = p_style[keys[k]];
				if (val.get_type() == Variant::DICTIONARY) {
					val = Dictionary(val).duplicate(true);
				} else if (val.get_type() == Variant::ARRAY) {
					val = Array(val).duplicate(true);
				}
				merged[key] = val;
			}
			merged["id"] = target;
			styles[i] = normalize_window_style(merged);
			cfg["styles"] = styles;
			set_window_styles_config(cfg);
			return true;
		}
	}
	return false;
}

bool ClockGearsDataManager::delete_window_style(const String &p_style_id) {
	String target = p_style_id.strip_edges().to_lower();
	Dictionary cfg = normalize_window_styles_config(window_styles_config);
	Array styles = cfg["styles"];
	if (styles.size() <= 1) {
		return false;
	}
	int found_idx = -1;
	for (int i = 0; i < styles.size(); i++) {
		Dictionary s = styles[i];
		if (String(s.get("id", "")) == target) {
			if (bool(s.get("built_in", false))) {
				return false;
			}
			found_idx = i;
			break;
		}
	}
	if (found_idx < 0) {
		return false;
	}
	styles.remove_at(found_idx);
	cfg["styles"] = styles;
	if (String(cfg.get("default_style_id", "")) == target) {
		cfg["default_style_id"] = "rounded";
	}
	Dictionary assign = cfg["assignments"];
	Array akeys = assign.keys();
	for (int i = 0; i < akeys.size(); i++) {
		if (String(assign[akeys[i]]) == target) {
			assign[akeys[i]] = "";
		}
	}
	cfg["assignments"] = assign;
	set_window_styles_config(cfg);
	return true;
}

String ClockGearsDataManager::get_default_window_style_id() const {
	_ensure_window_styles_cache();
	return String(cached_ws_norm.get("default_style_id", "rounded"));
}

bool ClockGearsDataManager::set_default_window_style_id(const String &p_style_id) {
	String target = p_style_id.strip_edges().to_lower();
	_ensure_window_styles_cache();
	if (!cached_ws_by_id.has(target)) {
		return false;
	}
	Dictionary cfg = cached_ws_norm.duplicate(true);
	cfg["default_style_id"] = target;
	set_window_styles_config(cfg);
	return true;
}

String ClockGearsDataManager::get_window_type_assignment(const String &p_window_type_id) const {
	_ensure_window_styles_cache();
	Dictionary assign = cached_ws_norm.get("assignments", Dictionary());
	return String(assign.get(p_window_type_id.strip_edges().to_lower(), ""));
}

bool ClockGearsDataManager::set_window_type_assignment(const String &p_window_type_id, const String &p_style_id) {
	String wt = p_window_type_id.strip_edges().to_lower();
	if (wt.is_empty()) {
		return false;
	}
	String sid = p_style_id.strip_edges().to_lower();
	_ensure_window_styles_cache();
	if (!sid.is_empty() && !cached_ws_by_id.has(sid)) {
		return false;
	}
	Dictionary cfg = cached_ws_norm.duplicate(true);
	Dictionary assign = cfg.get("assignments", Dictionary());
	assign[wt] = sid;
	cfg["assignments"] = assign;
	set_window_styles_config(cfg);
	return true;
}

String ClockGearsDataManager::resolve_window_style_id(const String &p_window_type_id) const {
	_ensure_window_styles_cache();
	String wt = p_window_type_id.strip_edges().to_lower();
	if (runtime_window_style_overrides.has(wt)) {
		String ov = String(runtime_window_style_overrides[wt]).strip_edges().to_lower();
		if (!ov.is_empty() && cached_ws_by_id.has(ov)) {
			return ov;
		}
	}
	Dictionary assign = cached_ws_norm.get("assignments", Dictionary());
	String assigned = String(assign.get(wt, "")).strip_edges().to_lower();
	if (!assigned.is_empty() && cached_ws_by_id.has(assigned)) {
		return assigned;
	}
	String def_id = String(cached_ws_norm.get("default_style_id", "rounded")).strip_edges().to_lower();
	if (!def_id.is_empty() && cached_ws_by_id.has(def_id)) {
		return def_id;
	}
	return "rounded";
}

Dictionary ClockGearsDataManager::resolve_window_style(const String &p_window_type_id) const {
	_ensure_window_styles_cache();
	String resolved_id = resolve_window_style_id(p_window_type_id);
	if (const Dictionary *found = cached_ws_by_id.getptr(resolved_id)) {
		return *found;
	}
	if (const Dictionary *rb = cached_ws_by_id.getptr("rounded")) {
		return *rb;
	}
	return s_make_builtin_style_rounded();
}

String ClockGearsDataManager::get_effective_game_title() const {
	String gt = String(system_config.get("game_title", "")).strip_edges();
	if (!gt.is_empty()) {
		return gt;
	}
	if (ProjectSettings::get_singleton()) {
		String pn = String(ProjectSettings::get_singleton()->get_setting("application/config/name", "")).strip_edges();
		if (!pn.is_empty()) {
			return pn;
		}
	}
	return "ClockGears Game";
}

void ClockGearsDataManager::apply_game_window_title(Node *p_context) {
	String title = get_effective_game_title();
	applied_window_title = title;
	if (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (DisplayServer::get_singleton() && !DisplayServer::get_singleton()->get_window_list().is_empty()) {
		DisplayServer::get_singleton()->window_set_title(title);
	}
	SceneTree *st = p_context && p_context->is_inside_tree() ? p_context->get_tree() : SceneTree::get_singleton();
	if (st && st->get_root()) {
		st->get_root()->set_title(title);
	}
}

String ClockGearsDataManager::get_currency_unit() const {
	String cur = String(system_config.get("currency", system_config.get("currency_unit", "G"))).strip_edges();
	return cur.is_empty() ? String("G") : cur;
}

void ClockGearsDataManager::set_currency_unit(const String &p_unit) {
	String clean = p_unit.strip_edges();
	system_config["currency"] = clean.is_empty() ? String("G") : clean;
	system_config.erase("currency_unit");
	system_config.erase("currency_name");
	vocabulary_config.erase("gold");
	dirty = true;
	emit_signal("data_changed", "system", 0);
	emit_signal("runtime_state_changed", "currency_unit", String());
}

Dictionary ClockGearsDataManager::get_default_player_options() {
	Dictionary opts;
	opts["schema_version"] = 1;
	opts["music_volume"] = 0.8; // Rango 0.0 .. 1.0 (80 %)
	opts["sfx_volume"] = 0.8;   // Rango 0.0 .. 1.0 (80 %)
	opts["brightness"] = 1.0;   // Rango 0.5 .. 1.5 (100 % neutro)
	opts["fullscreen"] = false; // Ventana por defecto
	return opts;
}

Dictionary ClockGearsDataManager::normalize_player_options(const Dictionary &p_opts) {
	Dictionary out = get_default_player_options();
	if (p_opts.has("music_volume")) {
		out["music_volume"] = CLAMP(double(p_opts["music_volume"]), 0.0, 1.0);
	}
	if (p_opts.has("sfx_volume")) {
		out["sfx_volume"] = CLAMP(double(p_opts["sfx_volume"]), 0.0, 1.0);
	}
	if (p_opts.has("brightness")) {
		out["brightness"] = CLAMP(double(p_opts["brightness"]), 0.5, 1.5);
	}
	if (p_opts.has("fullscreen")) {
		out["fullscreen"] = bool(p_opts["fullscreen"]);
	}
	return out;
}

void ClockGearsDataManager::set_player_options_path(const String &p_path) {
	player_options_path_override = p_path.strip_edges();
	player_options_loaded = false;
}

String ClockGearsDataManager::get_player_options_path() const {
	return player_options_path_override.is_empty() ? String("user://clockgears_options.json") : player_options_path_override;
}

void ClockGearsDataManager::set_player_options(const Dictionary &p_opts, bool p_save_and_apply) {
	player_options = normalize_player_options(p_opts);
	player_options_loaded = true;
	if (p_save_and_apply) {
		save_player_options();
		apply_player_options();
	}
	emit_signal("player_options_changed");
}

bool ClockGearsDataManager::load_player_options() {
	String path = get_player_options_path();
	Variant v;
	if (_read_json_with_backup(path, v) && v.get_type() == Variant::DICTIONARY) {
		player_options = normalize_player_options(Dictionary(v));
		player_options_loaded = true;
		return true;
	}
	player_options = get_default_player_options();
	player_options_loaded = true;
	return false;
}

bool ClockGearsDataManager::save_player_options() const {
	Dictionary clean = normalize_player_options(player_options);
	return _write_json_atomic(get_player_options_path(), clean);
}

void ClockGearsDataManager::ensure_player_options_loaded() {
	if (!player_options_loaded) {
		load_player_options();
	}
}

static void s_apply_brightness_overlay(SceneTree *p_tree, Node *p_context, double p_brightness) {
	if (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	Node *host = nullptr;
	if (p_tree && p_tree->get_root()) {
		host = p_tree->get_root();
	} else if (p_context && p_context->is_inside_tree() && p_context->get_tree() && p_context->get_tree()->get_root()) {
		host = p_context->get_tree()->get_root();
	}
	if (!host) {
		return;
	}

	const String overlay_name = "ClockGearsBrightnessOverlay";
	CanvasLayer *layer = Object::cast_to<CanvasLayer>(host->get_node_or_null(NodePath(overlay_name)));
	if (!layer) {
		layer = memnew(CanvasLayer);
		layer->set_name(overlay_name);
		layer->set_layer(126);
		host->add_child(layer);
	}
	ColorRect *rect = Object::cast_to<ColorRect>(layer->get_node_or_null(NodePath("BrightnessRect")));
	if (!rect) {
		rect = memnew(ColorRect);
		rect->set_name("BrightnessRect");
		rect->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		layer->add_child(rect);
	}

	if (Math::abs(p_brightness - 1.0) < 0.005) {
		rect->set_visible(false);
		rect->set_color(Color(0, 0, 0, 0));
		rect->set_material(Ref<Material>());
	} else if (p_brightness < 1.0) {
		float alpha = CLAMP(float(1.0 - p_brightness), 0.0f, 0.85f);
		rect->set_material(Ref<Material>());
		rect->set_color(Color(0.0f, 0.0f, 0.0f, alpha));
		rect->set_visible(true);
	} else {
		float boost = CLAMP(float(p_brightness - 1.0), 0.0f, 0.5f);
		Ref<CanvasItemMaterial> add_mat;
		add_mat.instantiate();
		add_mat->set_blend_mode(CanvasItemMaterial::BLEND_MODE_ADD);
		rect->set_material(add_mat);
		rect->set_color(Color(boost, boost, boost, 1.0f));
		rect->set_visible(true);
	}
}

void ClockGearsDataManager::apply_player_options(Node *p_context) {
	ensure_player_options_loaded();
	Dictionary opts = normalize_player_options(player_options);
	applied_music_volume = double(opts.get("music_volume", 0.8));
	applied_sfx_volume = double(opts.get("sfx_volume", 0.8));
	applied_brightness = double(opts.get("brightness", 1.0));
	applied_fullscreen = bool(opts.get("fullscreen", false));

	if (ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton()) {
		am->set_bus_volume(ClockGearsAudioManager::BUS_BGM, float(applied_music_volume));
		am->set_bus_volume(ClockGearsAudioManager::BUS_ME, float(applied_music_volume));
		am->set_bus_volume(ClockGearsAudioManager::BUS_SE, float(applied_sfx_volume));
		am->set_bus_volume(ClockGearsAudioManager::BUS_BGS, float(applied_sfx_volume));
		am->set_bus_volume(ClockGearsAudioManager::BUS_VOICE, float(applied_sfx_volume));
	}

	SceneTree *st = (p_context && p_context->is_inside_tree()) ? p_context->get_tree() : SceneTree::get_singleton();
	s_apply_brightness_overlay(st, p_context, applied_brightness);

	if (!(Engine::get_singleton() && Engine::get_singleton()->is_editor_hint())) {
		DisplayServer *ds = DisplayServer::get_singleton();
		if (ds && ds->get_name() != "headless" && !ds->get_window_list().is_empty()) {
			DisplayServerEnums::WindowMode target = applied_fullscreen
					? DisplayServerEnums::WINDOW_MODE_FULLSCREEN
					: DisplayServerEnums::WINDOW_MODE_WINDOWED;
			if (ds->window_get_mode() != target) {
				ds->window_set_mode(target);
			}
		}
	}
}

Dictionary ClockGearsDataManager::get_applied_player_options_state() const {
	Dictionary d;
	d["music_volume"] = applied_music_volume;
	d["sfx_volume"] = applied_sfx_volume;
	d["brightness"] = applied_brightness;
	d["fullscreen"] = applied_fullscreen;
	return d;
}

void ClockGearsDataManager::set_save_directory(const String &p_dir) {
	save_directory_override = p_dir.strip_edges();
}

String ClockGearsDataManager::get_save_directory() const {
	return save_directory_override.is_empty() ? String("user://saves") : save_directory_override;
}

String ClockGearsDataManager::get_save_slot_path(int p_slot) const {
	return get_save_directory().path_join(vformat("slot_%02d.json", p_slot));
}

String ClockGearsDataManager::format_playtime_hms(double p_seconds) {
	int total = MAX(0, int(Math::floor(p_seconds)));
	int hrs = total / 3600;
	int mins = (total % 3600) / 60;
	int secs = total % 60;
	return vformat("%02d:%02d:%02d", hrs, mins, secs);
}

bool ClockGearsDataManager::save_game_slot(int p_slot, const Dictionary &p_context) {
	int max_slots = get_save_slot_count();
	if (p_slot < 1 || p_slot > max_slots) {
		return false;
	}

	Dictionary save_doc = export_runtime_state();
	save_doc["schema_version"] = 1;
	save_doc["slot_index"] = p_slot;
	save_doc["saved_at"] = Time::get_singleton() ? Time::get_singleton()->get_datetime_string_from_system(false, true) : "";
	save_doc["game_title"] = get_effective_game_title();
	save_doc["currency_unit"] = get_currency_unit();

	Array party;
	if (p_context.has("party") && p_context["party"].get_type() == Variant::ARRAY) {
		party = Array(p_context["party"]).duplicate(true);
	} else {
		party = Array(system_config.get("party", Array())).duplicate(true);
	}
	if (party.is_empty() && !category_data[CAT_ACTORS].is_empty() && category_data[CAT_ACTORS][0].get_type() == Variant::DICTIONARY) {
		party.push_back(int(Dictionary(category_data[CAT_ACTORS][0]).get("id", 1)));
	}
	save_doc["party"] = party;

	String map_path = String(p_context.get("map_path", "")).strip_edges();
	String map_name = String(p_context.get("map_name", "")).strip_edges();
	Vector2i p_cell(int(p_context.get("player_x", 0)), int(p_context.get("player_y", 0)));
	Vector2 p_pos(float(p_context.get("player_px_x", 0.0f)), float(p_context.get("player_px_y", 0.0f)));
	bool has_explicit_px = p_context.has("player_px_x") || p_context.has("player_px_y") || p_context.has("player_pos");
	if (p_context.has("player_pos") && p_context["player_pos"].get_type() == Variant::DICTIONARY) {
		Dictionary pd = p_context["player_pos"];
		p_pos = Vector2(float(pd.get("x", 0.0f)), float(pd.get("y", 0.0f)));
	} else if (p_context.has("player_pos") && p_context["player_pos"].get_type() == Variant::VECTOR2) {
		p_pos = Vector2(p_context["player_pos"]);
	}
	int p_dir = int(p_context.get("player_dir", int(system_config.get("start_dir", 2))));

	if (map_path.is_empty() && SceneTree::get_singleton() && SceneTree::get_singleton()->get_current_scene()) {
		if (ClockGearsMap *cur_map = Object::cast_to<ClockGearsMap>(SceneTree::get_singleton()->get_current_scene())) {
			map_path = cur_map->get_scene_file_path();
			if (map_name.is_empty()) {
				map_name = cur_map->get_map_name();
			}
			if (ClockGearsPlayer *pl = cur_map->get_runtime_player()) {
				if (!p_context.has("player_x") && !p_context.has("player_y")) {
					p_cell = pl->get_grid_cell();
				}
				if (!has_explicit_px) {
					p_pos = pl->get_position();
					has_explicit_px = true;
				}
				if (!p_context.has("player_dir")) {
					p_dir = pl->get_direction();
				}
			}
		}
	}

	if (map_path.is_empty()) {
		map_path = String(system_config.get("start_map", ""));
	}
	if (map_name.is_empty()) {
		map_name = map_path.get_file().get_basename();
	}
	if (!has_explicit_px) {
		p_pos = Vector2(p_cell.x * 32.0f, p_cell.y * 32.0f);
	}

	save_doc["map_path"] = map_path;
	save_doc["map_name"] = map_name;

	Dictionary cell_d;
	cell_d["x"] = p_cell.x;
	cell_d["y"] = p_cell.y;
	save_doc["player_cell"] = cell_d;

	Dictionary pos_d;
	pos_d["x"] = double(p_pos.x);
	pos_d["y"] = double(p_pos.y);
	save_doc["player_pos"] = pos_d;
	save_doc["player_dir"] = (p_dir == 2 || p_dir == 4 || p_dir == 6 || p_dir == 8) ? p_dir : 2;

	bool ok = _write_json_atomic(get_save_slot_path(p_slot), save_doc);
	if (ok) {
		emit_signal("save_slots_changed");
	}
	return ok;
}

bool ClockGearsDataManager::load_game_slot(int p_slot) {
	int max_slots = get_save_slot_count();
	if (p_slot < 1 || p_slot > max_slots) {
		return false;
	}
	Variant v;
	if (!_read_json_with_backup(get_save_slot_path(p_slot), v) || v.get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary data = v;
	import_runtime_state(data);

	if (data.has("party") && data["party"].get_type() == Variant::ARRAY) {
		Array saved_party = data["party"];
		if (!saved_party.is_empty()) {
			system_config["party"] = saved_party.duplicate(true);
		}
	}

	Dictionary cell_d = (data.has("player_cell") && data["player_cell"].get_type() == Variant::DICTIONARY)
			? Dictionary(data["player_cell"])
			: Dictionary();
	int tx = int(cell_d.get("x", system_config.get("start_x", 0)));
	int ty = int(cell_d.get("y", system_config.get("start_y", 0)));
	int tdir = int(data.get("player_dir", system_config.get("start_dir", 2)));
	if (tdir != 2 && tdir != 4 && tdir != 6 && tdir != 8) {
		tdir = 2;
	}

	system_config["runtime_has_transfer"] = true;
	system_config["runtime_transfer_x"] = tx;
	system_config["runtime_transfer_y"] = ty;
	system_config["runtime_transfer_dir"] = tdir;
	if (data.has("player_pos") && data["player_pos"].get_type() == Variant::DICTIONARY) {
		Dictionary pos_d = data["player_pos"];
		system_config["runtime_has_exact_pos"] = true;
		system_config["runtime_transfer_px_x"] = double(pos_d.get("x", double(tx * 32)));
		system_config["runtime_transfer_px_y"] = double(pos_d.get("y", double(ty * 32)));
	} else {
		system_config.erase("runtime_has_exact_pos");
	}

	pending_loaded_map_path = String(data.get("map_path", system_config.get("start_map", ""))).strip_edges();
	return true;
}

bool ClockGearsDataManager::start_game_from_save(int p_slot, Node *p_context) {
	if (!load_game_slot(p_slot)) {
		return false;
	}
	String map_path = pending_loaded_map_path;
	if (map_path.is_empty()) {
		map_path = String(system_config.get("start_map", ""));
	}
	SceneTree *st = (p_context && p_context->is_inside_tree()) ? p_context->get_tree() : SceneTree::get_singleton();
	if (st && !map_path.is_empty() && ResourceLoader::exists(map_path)) {
		return st->change_scene_to_file(map_path) == OK;
	}
	return true;
}

Dictionary ClockGearsDataManager::get_save_slot_info(int p_slot) const {
	Dictionary info;
	info["slot_index"] = p_slot;
	info["exists"] = false;
	if (p_slot < 1 || p_slot > get_save_slot_count()) {
		return info;
	}
	Variant v;
	if (!_read_json_with_backup(get_save_slot_path(p_slot), v) || v.get_type() != Variant::DICTIONARY) {
		return info;
	}
	Dictionary data = v;
	info["exists"] = true;
	info["saved_at"] = String(data.get("saved_at", ""));
	info["playtime_sec"] = double(data.get("playtime_sec", 0.0));
	info["playtime_formatted"] = format_playtime_hms(double(data.get("playtime_sec", 0.0)));
	info["map_path"] = String(data.get("map_path", ""));
	info["map_name"] = String(data.get("map_name", ""));
	info["party"] = (data.has("party") && data["party"].get_type() == Variant::ARRAY) ? Array(data["party"]).duplicate(true) : Array();
	info["gold"] = int(data.get("gold", 0));
	info["player_cell"] = data.get("player_cell", Dictionary());
	info["player_pos"] = data.get("player_pos", Dictionary());
	info["player_dir"] = int(data.get("player_dir", 2));
	return info;
}

Array ClockGearsDataManager::list_save_slots() const {
	Array out;
	int count = get_save_slot_count();
	for (int s = 1; s <= count; s++) {
		out.push_back(get_save_slot_info(s));
	}
	return out;
}

bool ClockGearsDataManager::has_any_save() const {
	int count = get_save_slot_count();
	for (int s = 1; s <= count; s++) {
		String p = get_save_slot_path(s);
		if (FileAccess::exists(p) || FileAccess::exists(p + ".bak")) {
			return true;
		}
	}
	return false;
}

int ClockGearsDataManager::get_latest_save_slot() const {
	int count = get_save_slot_count();
	int best_slot = -1;
	String best_saved_at;
	uint64_t best_mtime = 0;
	for (int s = 1; s <= count; s++) {
		Dictionary info = get_save_slot_info(s);
		if (!bool(info.get("exists", false))) {
			continue;
		}
		String p = get_save_slot_path(s);
		uint64_t mtime = FileAccess::exists(p) ? FileAccess::get_modified_time(p) : (FileAccess::exists(p + ".bak") ? FileAccess::get_modified_time(p + ".bak") : 0);
		String sat = String(info.get("saved_at", "")).strip_edges();
		bool newer = false;
		if (best_slot < 0) {
			newer = true;
		} else if (!sat.is_empty() && !best_saved_at.is_empty() && sat != best_saved_at) {
			newer = (sat > best_saved_at);
		} else if (mtime != best_mtime) {
			newer = (mtime > best_mtime);
		} else {
			newer = (s > best_slot);
		}
		if (newer) {
			best_slot = s;
			best_saved_at = sat;
			best_mtime = mtime;
		}
	}
	return best_slot;
}

bool ClockGearsDataManager::delete_save_slot(int p_slot) {
	if (p_slot < 1) {
		return false;
	}
	String p = get_save_slot_path(p_slot);
	Ref<DirAccess> dir = DirAccess::create_for_path(p);
	if (dir.is_null()) {
		return false;
	}
	bool removed = false;
	const String paths[3] = { p, p + ".bak", p + ".tmp" };
	for (int i = 0; i < 3; i++) {
		if (FileAccess::exists(paths[i])) {
			if (dir->remove(paths[i]) == OK) {
				removed = true;
			}
		}
	}
	if (removed) {
		emit_signal("save_slots_changed");
	}
	return removed;
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
	ClassDB::bind_method(D_METHOD("get_entry_icon_texture", "entry", "target_px", "fallback"), &ClockGearsDataManager::get_entry_icon_texture, DEFVAL(32), DEFVAL(Ref<Texture2D>()));
	ClassDB::bind_method(D_METHOD("clear_entry_icon_cache"), &ClockGearsDataManager::clear_entry_icon_cache);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("parse_tile_block_variant", "var"), &ClockGearsDataManager::parse_tile_block_variant);
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
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_title_menu_config"), &ClockGearsDataManager::get_default_title_menu_config);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_title_menu_config", "config"), &ClockGearsDataManager::normalize_title_menu_config);
	ClassDB::bind_method(D_METHOD("get_title_menu_config"), &ClockGearsDataManager::get_title_menu_config);
	ClassDB::bind_method(D_METHOD("set_title_menu_config", "config"), &ClockGearsDataManager::set_title_menu_config);
	ClassDB::bind_method(D_METHOD("get_visible_title_entries"), &ClockGearsDataManager::get_visible_title_entries);
	ClassDB::bind_method(D_METHOD("get_save_slot_count"), &ClockGearsDataManager::get_save_slot_count);
	ClassDB::bind_method(D_METHOD("set_save_slot_count", "count"), &ClockGearsDataManager::set_save_slot_count);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("register_window_type", "id", "label"), &ClockGearsDataManager::register_window_type);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_registered_window_types"), &ClockGearsDataManager::get_registered_window_types);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_window_styles_config"), &ClockGearsDataManager::get_default_window_styles_config);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_window_style", "style"), &ClockGearsDataManager::normalize_window_style);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_window_styles_config", "config"), &ClockGearsDataManager::normalize_window_styles_config);
	ClassDB::bind_method(D_METHOD("get_window_styles_config"), &ClockGearsDataManager::get_window_styles_config);
	ClassDB::bind_method(D_METHOD("set_window_styles_config", "config"), &ClockGearsDataManager::set_window_styles_config);
	ClassDB::bind_method(D_METHOD("get_window_styles"), &ClockGearsDataManager::get_window_styles);
	ClassDB::bind_method(D_METHOD("get_window_style", "style_id"), &ClockGearsDataManager::get_window_style);
	ClassDB::bind_method(D_METHOD("create_window_style", "style"), &ClockGearsDataManager::create_window_style);
	ClassDB::bind_method(D_METHOD("duplicate_window_style", "source_id", "new_name"), &ClockGearsDataManager::duplicate_window_style, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("update_window_style", "style_id", "style"), &ClockGearsDataManager::update_window_style);
	ClassDB::bind_method(D_METHOD("delete_window_style", "style_id"), &ClockGearsDataManager::delete_window_style);
	ClassDB::bind_method(D_METHOD("get_default_window_style_id"), &ClockGearsDataManager::get_default_window_style_id);
	ClassDB::bind_method(D_METHOD("set_default_window_style_id", "style_id"), &ClockGearsDataManager::set_default_window_style_id);
	ClassDB::bind_method(D_METHOD("get_window_type_assignment", "window_type_id"), &ClockGearsDataManager::get_window_type_assignment);
	ClassDB::bind_method(D_METHOD("set_window_type_assignment", "window_type_id", "style_id"), &ClockGearsDataManager::set_window_type_assignment);
	ClassDB::bind_method(D_METHOD("resolve_window_style_id", "window_type_id"), &ClockGearsDataManager::resolve_window_style_id);
	ClassDB::bind_method(D_METHOD("resolve_window_style", "window_type_id"), &ClockGearsDataManager::resolve_window_style);
	ClassDB::bind_method(D_METHOD("get_effective_game_title"), &ClockGearsDataManager::get_effective_game_title);
	ClassDB::bind_method(D_METHOD("apply_game_window_title", "context"), &ClockGearsDataManager::apply_game_window_title, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_applied_window_title"), &ClockGearsDataManager::get_applied_window_title);
	ClassDB::bind_method(D_METHOD("get_currency_unit"), &ClockGearsDataManager::get_currency_unit);
	ClassDB::bind_method(D_METHOD("set_currency_unit", "unit"), &ClockGearsDataManager::set_currency_unit);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_player_options"), &ClockGearsDataManager::get_default_player_options);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_player_options", "options"), &ClockGearsDataManager::normalize_player_options);
	ClassDB::bind_method(D_METHOD("set_player_options_path", "path"), &ClockGearsDataManager::set_player_options_path);
	ClassDB::bind_method(D_METHOD("get_player_options_path"), &ClockGearsDataManager::get_player_options_path);
	ClassDB::bind_method(D_METHOD("get_player_options"), &ClockGearsDataManager::get_player_options);
	ClassDB::bind_method(D_METHOD("set_player_options", "options", "save_and_apply"), &ClockGearsDataManager::set_player_options, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("load_player_options"), &ClockGearsDataManager::load_player_options);
	ClassDB::bind_method(D_METHOD("save_player_options"), &ClockGearsDataManager::save_player_options);
	ClassDB::bind_method(D_METHOD("ensure_player_options_loaded"), &ClockGearsDataManager::ensure_player_options_loaded);
	ClassDB::bind_method(D_METHOD("apply_player_options", "context"), &ClockGearsDataManager::apply_player_options, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_applied_player_options_state"), &ClockGearsDataManager::get_applied_player_options_state);
	ClassDB::bind_method(D_METHOD("set_save_directory", "dir"), &ClockGearsDataManager::set_save_directory);
	ClassDB::bind_method(D_METHOD("get_save_directory"), &ClockGearsDataManager::get_save_directory);
	ClassDB::bind_method(D_METHOD("get_save_slot_path", "slot"), &ClockGearsDataManager::get_save_slot_path);
	ClassDB::bind_method(D_METHOD("save_game_slot", "slot", "context"), &ClockGearsDataManager::save_game_slot, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("load_game_slot", "slot"), &ClockGearsDataManager::load_game_slot);
	ClassDB::bind_method(D_METHOD("start_game_from_save", "slot", "context"), &ClockGearsDataManager::start_game_from_save, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_save_slot_info", "slot"), &ClockGearsDataManager::get_save_slot_info);
	ClassDB::bind_method(D_METHOD("list_save_slots"), &ClockGearsDataManager::list_save_slots);
	ClassDB::bind_method(D_METHOD("has_any_save"), &ClockGearsDataManager::has_any_save);
	ClassDB::bind_method(D_METHOD("get_latest_save_slot"), &ClockGearsDataManager::get_latest_save_slot);
	ClassDB::bind_method(D_METHOD("delete_save_slot", "slot"), &ClockGearsDataManager::delete_save_slot);
	ClassDB::bind_method(D_METHOD("get_pending_loaded_map_path"), &ClockGearsDataManager::get_pending_loaded_map_path);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("format_playtime_hms", "seconds"), &ClockGearsDataManager::format_playtime_hms);
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
	ClassDB::bind_method(D_METHOD("get_item_qty", "name"), &ClockGearsDataManager::get_item_qty);
	ClassDB::bind_method(D_METHOD("get_runtime_inventory"), &ClockGearsDataManager::get_runtime_inventory);
	ClassDB::bind_method(D_METHOD("change_item", "name", "amount"), &ClockGearsDataManager::change_item);
	ClassDB::bind_method(D_METHOD("get_default_max_item_stack"), &ClockGearsDataManager::get_default_max_item_stack);
	ClassDB::bind_method(D_METHOD("set_default_max_item_stack", "max_stack"), &ClockGearsDataManager::set_default_max_item_stack);
	ClassDB::bind_method(D_METHOD("get_item_max_stack", "item_key"), &ClockGearsDataManager::get_item_max_stack);
	ClassDB::bind_method(D_METHOD("get_record_inventory_key", "category", "id"), &ClockGearsDataManager::get_record_inventory_key);
	ClassDB::bind_method(D_METHOD("find_inventory_record", "item_key"), &ClockGearsDataManager::find_inventory_record);
	ClassDB::bind_method(D_METHOD("is_key_item", "item_key"), &ClockGearsDataManager::is_key_item);
	ClassDB::bind_method(D_METHOD("can_discard_item", "item_key"), &ClockGearsDataManager::can_discard_item);
	ClassDB::bind_method(D_METHOD("discard_item", "item_key", "amount"), &ClockGearsDataManager::discard_item, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("get_inventory_entries"), &ClockGearsDataManager::get_inventory_entries);
	ClassDB::bind_method(D_METHOD("use_item_on_actor", "item_key", "target_actor_id"), &ClockGearsDataManager::use_item_on_actor);
	ClassDB::bind_method(D_METHOD("get_party_actor_ids"), &ClockGearsDataManager::get_party_actor_ids);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_game_menu_config"), &ClockGearsDataManager::get_default_game_menu_config);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_game_menu_config", "cfg"), &ClockGearsDataManager::normalize_game_menu_config);
	ClassDB::bind_method(D_METHOD("get_game_menu_config"), &ClockGearsDataManager::get_game_menu_config);
	ClassDB::bind_method(D_METHOD("set_game_menu_config", "cfg"), &ClockGearsDataManager::set_game_menu_config);
	ClassDB::bind_method(D_METHOD("get_visible_game_menu_entries"), &ClockGearsDataManager::get_visible_game_menu_entries);
	ClassDB::bind_method(D_METHOD("get_game_menu_screen_background", "screen_id"), &ClockGearsDataManager::get_game_menu_screen_background);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_credits_config"), &ClockGearsDataManager::get_default_credits_config);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_credits_config", "cfg"), &ClockGearsDataManager::normalize_credits_config);
	ClassDB::bind_method(D_METHOD("get_credits_config"), &ClockGearsDataManager::get_credits_config);
	ClassDB::bind_method(D_METHOD("set_credits_config", "cfg"), &ClockGearsDataManager::set_credits_config);
	ClassDB::bind_method(D_METHOD("has_credits_lines"), &ClockGearsDataManager::has_credits_lines);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_game_over_config"), &ClockGearsDataManager::get_default_game_over_config);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_game_over_config", "cfg"), &ClockGearsDataManager::normalize_game_over_config);
	ClassDB::bind_method(D_METHOD("get_game_over_config"), &ClockGearsDataManager::get_game_over_config);
	ClassDB::bind_method(D_METHOD("set_game_over_config", "cfg"), &ClockGearsDataManager::set_game_over_config);
	ClassDB::bind_method(D_METHOD("is_game_menu_disabled"), &ClockGearsDataManager::is_game_menu_disabled);
	ClassDB::bind_method(D_METHOD("set_game_menu_disabled", "disabled"), &ClockGearsDataManager::set_game_menu_disabled);
	ClassDB::bind_method(D_METHOD("set_runtime_window_style_override", "window_type_id", "style_id"), &ClockGearsDataManager::set_runtime_window_style_override);
	ClassDB::bind_method(D_METHOD("clear_runtime_window_style_overrides"), &ClockGearsDataManager::clear_runtime_window_style_overrides);
	ClassDB::bind_method(D_METHOD("get_runtime_window_style_overrides"), &ClockGearsDataManager::get_runtime_window_style_overrides);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_hud_anchor_ids"), &ClockGearsDataManager::get_hud_anchor_ids);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_hud_config"), &ClockGearsDataManager::get_default_hud_config);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("make_default_hud_widget", "type"), &ClockGearsDataManager::make_default_hud_widget, DEFVAL("bar"));
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_hud_widget", "widget", "index_hint"), &ClockGearsDataManager::normalize_hud_widget, DEFVAL(1));
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_hud_config", "cfg"), &ClockGearsDataManager::normalize_hud_config);
	ClassDB::bind_method(D_METHOD("get_hud_config"), &ClockGearsDataManager::get_hud_config);
	ClassDB::bind_method(D_METHOD("set_hud_config", "cfg"), &ClockGearsDataManager::set_hud_config);
	ClassDB::bind_method(D_METHOD("get_hud_widgets"), &ClockGearsDataManager::get_hud_widgets);
	ClassDB::bind_method(D_METHOD("get_hud_widget", "widget_id"), &ClockGearsDataManager::get_hud_widget);
	ClassDB::bind_method(D_METHOD("create_hud_widget", "widget"), &ClockGearsDataManager::create_hud_widget);
	ClassDB::bind_method(D_METHOD("update_hud_widget", "widget_id", "widget"), &ClockGearsDataManager::update_hud_widget);
	ClassDB::bind_method(D_METHOD("delete_hud_widget", "widget_id"), &ClockGearsDataManager::delete_hud_widget);
	ClassDB::bind_method(D_METHOD("duplicate_hud_widget", "source_id", "new_name"), &ClockGearsDataManager::duplicate_hud_widget, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("move_hud_widget", "widget_id", "delta"), &ClockGearsDataManager::move_hud_widget);
	ClassDB::bind_method(D_METHOD("is_hud_runtime_visible"), &ClockGearsDataManager::is_hud_runtime_visible);
	ClassDB::bind_method(D_METHOD("set_hud_runtime_visible", "visible"), &ClockGearsDataManager::set_hud_runtime_visible);
	ClassDB::bind_method(D_METHOD("set_hud_widget_runtime_visible", "widget_name_or_id", "visible"), &ClockGearsDataManager::set_hud_widget_runtime_visible);
	ClassDB::bind_method(D_METHOD("clear_hud_widget_runtime_visibility", "widget_name_or_id"), &ClockGearsDataManager::clear_hud_widget_runtime_visibility, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("is_hud_widget_runtime_visible", "widget_id", "widget_name"), &ClockGearsDataManager::is_hud_widget_runtime_visible, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_hud_runtime_visibility_state"), &ClockGearsDataManager::get_hud_runtime_visibility_state);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_ui_design_screens"), &ClockGearsDataManager::get_ui_design_screens);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_ui_design_screen_ids"), &ClockGearsDataManager::get_ui_design_screen_ids);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_system_menu_ids"), &ClockGearsDataManager::get_system_menu_ids);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("is_system_menu_id", "menu_id"), &ClockGearsDataManager::is_system_menu_id);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_menu_id", "raw_id"), &ClockGearsDataManager::normalize_menu_id);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_ui_element_types"), &ClockGearsDataManager::get_ui_element_types);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_ui_data_bindings"), &ClockGearsDataManager::get_ui_data_bindings);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_ui_button_actions", "screen_id"), &ClockGearsDataManager::get_ui_button_actions);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_all_ui_button_action_types"), &ClockGearsDataManager::get_all_ui_button_action_types);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_menu_layout_modes"), &ClockGearsDataManager::get_menu_layout_modes);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_ui_shape_types"), &ClockGearsDataManager::get_ui_shape_types);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_menu_trigger_contexts"), &ClockGearsDataManager::get_menu_trigger_contexts);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_menu_trigger", "trig"), &ClockGearsDataManager::normalize_menu_trigger);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_menu_layout", "layout"), &ClockGearsDataManager::normalize_menu_layout);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_ui_shape", "shape"), &ClockGearsDataManager::normalize_ui_shape);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("make_default_ui_element", "type", "screen_id"), &ClockGearsDataManager::make_default_ui_element, DEFVAL("text"), DEFVAL("title"));
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_ui_element", "elem", "index_hint", "screen_id"), &ClockGearsDataManager::normalize_ui_element, DEFVAL(1), DEFVAL("title"));
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_ui_screen_design", "screen_id"), &ClockGearsDataManager::get_default_ui_screen_design);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_ui_screen_design", "screen_id", "design"), &ClockGearsDataManager::normalize_ui_screen_design);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_ui_designs_config"), &ClockGearsDataManager::get_default_ui_designs_config);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_ui_designs_config", "cfg"), &ClockGearsDataManager::normalize_ui_designs_config);
	ClassDB::bind_method(D_METHOD("get_default_ui_screen_template", "screen_id"), &ClockGearsDataManager::get_default_ui_screen_template);
	ClassDB::bind_method(D_METHOD("get_all_menus"), &ClockGearsDataManager::get_all_menus);
	ClassDB::bind_method(D_METHOD("get_custom_menu_ids"), &ClockGearsDataManager::get_custom_menu_ids);
	ClassDB::bind_method(D_METHOD("get_all_menus_for_selector"), &ClockGearsDataManager::get_all_menus_for_selector);
	ClassDB::bind_method(D_METHOD("has_menu", "menu_id"), &ClockGearsDataManager::has_menu);
	ClassDB::bind_method(D_METHOD("get_menu_system_group", "menu_id"), &ClockGearsDataManager::get_menu_system_group);
	ClassDB::bind_method(D_METHOD("get_menu_metadata", "menu_id"), &ClockGearsDataManager::get_menu_metadata);
	ClassDB::bind_method(D_METHOD("create_custom_menu", "name", "initial_design"), &ClockGearsDataManager::create_custom_menu, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("create_submenu_design", "parent_id", "name", "initial_design"), &ClockGearsDataManager::create_submenu_design, DEFVAL(""), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("rename_menu", "menu_id", "new_name"), &ClockGearsDataManager::rename_menu);
	ClassDB::bind_method(D_METHOD("duplicate_menu", "source_id", "new_name"), &ClockGearsDataManager::duplicate_menu, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("reorder_custom_menu", "menu_id", "new_order"), &ClockGearsDataManager::reorder_custom_menu);
	ClassDB::bind_method(D_METHOD("move_custom_menu", "menu_id", "delta"), &ClockGearsDataManager::move_custom_menu);
	ClassDB::bind_method(D_METHOD("delete_custom_menu", "menu_id"), &ClockGearsDataManager::delete_custom_menu);
	ClassDB::bind_method(D_METHOD("restore_system_menu_template", "menu_id", "enable_custom_design"), &ClockGearsDataManager::restore_system_menu_template, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_menu_trigger_conflicts"), &ClockGearsDataManager::get_menu_trigger_conflicts);
	ClassDB::bind_method(D_METHOD("get_last_migrated_files"), &ClockGearsDataManager::get_last_migrated_files);
	ClassDB::bind_method(D_METHOD("clear_last_migrated_files"), &ClockGearsDataManager::clear_last_migrated_files);
	ClassDB::bind_method(D_METHOD("has_custom_ui_design", "screen_id"), &ClockGearsDataManager::has_custom_ui_design);
	ClassDB::bind_method(D_METHOD("get_ui_screen_design", "screen_id"), &ClockGearsDataManager::get_ui_screen_design);
	ClassDB::bind_method(D_METHOD("set_ui_screen_design", "screen_id", "design", "record_undo"), &ClockGearsDataManager::set_ui_screen_design, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_ui_screen_elements", "screen_id"), &ClockGearsDataManager::get_ui_screen_elements);
	ClassDB::bind_method(D_METHOD("get_ui_element", "screen_id", "elem_id"), &ClockGearsDataManager::get_ui_element);
	ClassDB::bind_method(D_METHOD("add_ui_element", "screen_id", "elem", "record_undo"), &ClockGearsDataManager::add_ui_element, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("update_ui_element", "screen_id", "elem_id", "elem", "record_undo"), &ClockGearsDataManager::update_ui_element, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("delete_ui_element", "screen_id", "elem_id", "record_undo"), &ClockGearsDataManager::delete_ui_element, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("delete_ui_element_with_mode", "screen_id", "elem_id", "delete_children", "record_undo"), &ClockGearsDataManager::delete_ui_element_with_mode, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("move_ui_element_layer", "screen_id", "elem_id", "delta", "record_undo"), &ClockGearsDataManager::move_ui_element_layer, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("group_ui_elements_into_folder", "screen_id", "elem_ids", "folder_name", "record_undo"), &ClockGearsDataManager::group_ui_elements_into_folder, DEFVAL(""), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("ungroup_ui_folder", "screen_id", "folder_id", "record_undo"), &ClockGearsDataManager::ungroup_ui_folder, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("reorder_ui_element_tree", "screen_id", "elem_ids", "target_id", "drop_section", "record_undo"), &ClockGearsDataManager::reorder_ui_element_tree, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("move_ui_element_tree_step", "screen_id", "elem_id", "delta", "record_undo"), &ClockGearsDataManager::move_ui_element_tree_step, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("reset_ui_screen_design_to_default", "screen_id", "record_undo"), &ClockGearsDataManager::reset_ui_screen_design_to_default, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_ui_designs_config"), &ClockGearsDataManager::get_ui_designs_config);
	ClassDB::bind_method(D_METHOD("set_ui_designs_config", "cfg"), &ClockGearsDataManager::set_ui_designs_config);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_ui_background", "bg"), &ClockGearsDataManager::normalize_ui_background);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_ui_transition", "tr"), &ClockGearsDataManager::normalize_ui_transition);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_builtin_menu_templates"), &ClockGearsDataManager::get_builtin_menu_templates);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_ui_templates_for_system", "screen_id"), &ClockGearsDataManager::get_ui_templates_for_system);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_builtin_component_templates"), &ClockGearsDataManager::get_builtin_component_templates);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_builtin_template_by_id", "template_id"), &ClockGearsDataManager::get_builtin_template_by_id);
	ClassDB::bind_method(D_METHOD("get_user_menu_templates"), &ClockGearsDataManager::get_user_menu_templates);
	ClassDB::bind_method(D_METHOD("save_screen_as_user_template", "screen_id", "name"), &ClockGearsDataManager::save_screen_as_user_template);
	ClassDB::bind_method(D_METHOD("save_element_as_component_template", "screen_id", "elem_id", "name"), &ClockGearsDataManager::save_element_as_component_template);
	ClassDB::bind_method(D_METHOD("delete_user_template", "template_id"), &ClockGearsDataManager::delete_user_template);
	ClassDB::bind_method(D_METHOD("apply_menu_template", "screen_id", "template_id", "preserve_buttons", "record_undo"), &ClockGearsDataManager::apply_menu_template, DEFVAL(true), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("instantiate_component_template", "screen_id", "template_id", "pos"), &ClockGearsDataManager::instantiate_component_template, DEFVAL(Vector2i(-1, -1)));
	ClassDB::bind_method(D_METHOD("create_menu_from_wizard", "opts"), &ClockGearsDataManager::create_menu_from_wizard);
	ClassDB::bind_method(D_METHOD("export_template_package", "id", "dst_path", "is_screen"), &ClockGearsDataManager::export_template_package, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("import_template_package", "src_path", "import_as_custom_menu", "assets_subdir"), &ClockGearsDataManager::import_template_package, DEFVAL(false), DEFVAL("res://img/ui/imported"));
	ClassDB::bind_method(D_METHOD("duplicate_ui_element", "screen_id", "elem_id", "offset_delta"), &ClockGearsDataManager::duplicate_ui_element, DEFVAL(Vector2i(16, 16)));
	ClassDB::bind_method(D_METHOD("copy_ui_elements_to_clipboard", "screen_id", "elem_ids"), &ClockGearsDataManager::copy_ui_elements_to_clipboard);
	ClassDB::bind_method(D_METHOD("paste_ui_elements_from_clipboard", "screen_id"), &ClockGearsDataManager::paste_ui_elements_from_clipboard);
	ClassDB::bind_method(D_METHOD("get_ui_clipboard_count"), &ClockGearsDataManager::get_ui_clipboard_count);
	ClassDB::bind_method(D_METHOD("get_recent_ui_colors"), &ClockGearsDataManager::get_recent_ui_colors);
	ClassDB::bind_method(D_METHOD("record_recent_ui_color", "hex_color"), &ClockGearsDataManager::record_recent_ui_color);
	ClassDB::bind_method(D_METHOD("resolve_ui_data_binding", "binding", "element"), &ClockGearsDataManager::resolve_ui_data_binding, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("push_ui_design_undo_state", "screen_id", "snapshot"), &ClockGearsDataManager::push_ui_design_undo_state, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("can_undo_ui_design", "screen_id"), &ClockGearsDataManager::can_undo_ui_design);
	ClassDB::bind_method(D_METHOD("can_redo_ui_design", "screen_id"), &ClockGearsDataManager::can_redo_ui_design);
	ClassDB::bind_method(D_METHOD("get_ui_design_undo_count", "screen_id"), &ClockGearsDataManager::get_ui_design_undo_count);
	ClassDB::bind_method(D_METHOD("get_ui_design_redo_count", "screen_id"), &ClockGearsDataManager::get_ui_design_redo_count);
	ClassDB::bind_method(D_METHOD("undo_ui_design", "screen_id"), &ClockGearsDataManager::undo_ui_design);
	ClassDB::bind_method(D_METHOD("redo_ui_design", "screen_id"), &ClockGearsDataManager::redo_ui_design);
	ClassDB::bind_method(D_METHOD("clear_ui_design_history", "screen_id"), &ClockGearsDataManager::clear_ui_design_history, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("get_gold"), &ClockGearsDataManager::get_gold);
	ClassDB::bind_method(D_METHOD("set_gold", "gold"), &ClockGearsDataManager::set_gold);
	ClassDB::bind_method(D_METHOD("change_gold", "amount"), &ClockGearsDataManager::change_gold);
	ClassDB::bind_method(D_METHOD("reset_runtime_state"), &ClockGearsDataManager::reset_runtime_state);
	ClassDB::bind_method(D_METHOD("get_runtime_fade_alpha"), &ClockGearsDataManager::get_runtime_fade_alpha);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("evaluate_traits_element_rate", "traits", "element_id", "element_name"), &ClockGearsDataManager::evaluate_traits_element_rate, DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("get_actor_runtime_state", "actor_id"), &ClockGearsDataManager::get_actor_runtime_state);
	ClassDB::bind_method(D_METHOD("set_actor_runtime_hp", "actor_id", "hp"), &ClockGearsDataManager::set_actor_runtime_hp);
	ClassDB::bind_method(D_METHOD("get_actor_runtime_hp", "actor_id"), &ClockGearsDataManager::get_actor_runtime_hp);
	ClassDB::bind_method(D_METHOD("get_actor_runtime_max_hp", "actor_id"), &ClockGearsDataManager::get_actor_runtime_max_hp);
	ClassDB::bind_method(D_METHOD("set_actor_runtime_mp", "actor_id", "mp"), &ClockGearsDataManager::set_actor_runtime_mp);
	ClassDB::bind_method(D_METHOD("get_actor_runtime_mp", "actor_id"), &ClockGearsDataManager::get_actor_runtime_mp);
	ClassDB::bind_method(D_METHOD("get_actor_runtime_max_mp", "actor_id"), &ClockGearsDataManager::get_actor_runtime_max_mp);
	ClassDB::bind_method(D_METHOD("add_actor_runtime_state", "actor_id", "state_id"), &ClockGearsDataManager::add_actor_runtime_state);
	ClassDB::bind_method(D_METHOD("remove_actor_runtime_state", "actor_id", "state_id"), &ClockGearsDataManager::remove_actor_runtime_state);
	ClassDB::bind_method(D_METHOD("get_actor_runtime_states", "actor_id"), &ClockGearsDataManager::get_actor_runtime_states);
	ClassDB::bind_method(D_METHOD("get_actor_runtime_level", "actor_id"), &ClockGearsDataManager::get_actor_runtime_level);
	ClassDB::bind_method(D_METHOD("get_actor_runtime_exp", "actor_id"), &ClockGearsDataManager::get_actor_runtime_exp);
	ClassDB::bind_method(D_METHOD("grant_party_exp", "exp_amount"), &ClockGearsDataManager::grant_party_exp);
	ClassDB::bind_method(D_METHOD("resolve_arpg_enemy_drops", "enemy_rec"), &ClockGearsDataManager::resolve_arpg_enemy_drops);
	ClassDB::bind_method(D_METHOD("advance_runtime_clock", "delta_sec"), &ClockGearsDataManager::advance_runtime_clock);
	ClassDB::bind_method(D_METHOD("get_runtime_clock_sec"), &ClockGearsDataManager::get_runtime_clock_sec);
	ClassDB::bind_method(D_METHOD("set_runtime_clock_sec", "sec"), &ClockGearsDataManager::set_runtime_clock_sec);
	ClassDB::bind_method(D_METHOD("mark_arpg_enemy_defeated", "map_key", "event_id"), &ClockGearsDataManager::mark_arpg_enemy_defeated);
	ClassDB::bind_method(D_METHOD("is_arpg_enemy_defeated", "map_key", "event_id"), &ClockGearsDataManager::is_arpg_enemy_defeated);
	ClassDB::bind_method(D_METHOD("process_map_respawns", "map_key", "respawn_mode", "respawn_minutes", "is_entering_map"), &ClockGearsDataManager::process_map_respawns);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("get_default_arpg_equip_slots"), &ClockGearsDataManager::get_default_arpg_equip_slots);
	ClassDB::bind_method(D_METHOD("get_actor_arpg_equip_slots", "actor_id"), &ClockGearsDataManager::get_actor_arpg_equip_slots);
	ClassDB::bind_method(D_METHOD("set_actor_arpg_equip", "actor_id", "slot_id", "piece_id"), &ClockGearsDataManager::set_actor_arpg_equip);
	ClassDB::bind_method(D_METHOD("get_actor_arpg_equip", "actor_id", "slot_id"), &ClockGearsDataManager::get_actor_arpg_equip);
	ClassDB::bind_method(D_METHOD("get_actor_arpg_equip_bonuses", "actor_id"), &ClockGearsDataManager::get_actor_arpg_equip_bonuses);
	ClassDB::bind_method(D_METHOD("is_arpg_mode_active", "mode_override"), &ClockGearsDataManager::is_arpg_mode_active, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_actor_equip_slots", "actor_id", "mode_override"), &ClockGearsDataManager::get_actor_equip_slots, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("is_actor_equip_slot_locked", "actor_id", "slot_idx", "mode_override"), &ClockGearsDataManager::is_actor_equip_slot_locked, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("is_actor_equip_slot_sealed", "actor_id", "slot_idx", "mode_override"), &ClockGearsDataManager::is_actor_equip_slot_sealed, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("can_actor_equip_piece", "actor_id", "slot_idx", "category", "piece_id", "mode_override"), &ClockGearsDataManager::can_actor_equip_piece, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_equippable_pieces_for_slot", "actor_id", "slot_idx", "mode_override"), &ClockGearsDataManager::get_equippable_pieces_for_slot, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("equip_actor_piece", "actor_id", "slot_idx", "inventory_key", "mode_override"), &ClockGearsDataManager::equip_actor_piece, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("unequip_actor_slot", "actor_id", "slot_idx", "mode_override"), &ClockGearsDataManager::unequip_actor_slot, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("preview_equip_actor_stats", "actor_id", "slot_idx", "candidate_category", "candidate_piece_id", "mode_override"), &ClockGearsDataManager::preview_equip_actor_stats, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_actor_equip_bonuses", "actor_id", "mode_override"), &ClockGearsDataManager::get_actor_equip_bonuses, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_actor_effective_stats", "actor_id", "mode_override"), &ClockGearsDataManager::get_actor_effective_stats, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_actor_status_sheet", "actor_id", "mode_override"), &ClockGearsDataManager::get_actor_status_sheet, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_actor_element_rate", "actor_id", "element_id", "element_key"), &ClockGearsDataManager::get_actor_element_rate, DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("export_runtime_state"), &ClockGearsDataManager::export_runtime_state);
	ClassDB::bind_method(D_METHOD("import_runtime_state", "state"), &ClockGearsDataManager::import_runtime_state);
	ClassDB::bind_method(D_METHOD("format_text", "text"), &ClockGearsDataManager::format_text);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("repair_mojibake_text", "text"), &ClockGearsDataManager::repair_mojibake_text);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("truncate_with_ellipsis", "text", "max_chars"), &ClockGearsDataManager::truncate_with_ellipsis);
	ClassDB::bind_method(D_METHOD("get_last_repaired_json_files"), &ClockGearsDataManager::get_last_repaired_json_files);

	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("sanitize_custom_layer_node_name", "id"), &ClockGearsDataManager::sanitize_custom_layer_node_name);
	ClassDB::bind_static_method("ClockGearsDataManager", D_METHOD("normalize_custom_map_layer", "raw", "idx"), &ClockGearsDataManager::normalize_custom_map_layer, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_custom_map_layers"), &ClockGearsDataManager::get_custom_map_layers);
	ClassDB::bind_method(D_METHOD("set_custom_map_layers", "layers"), &ClockGearsDataManager::set_custom_map_layers);
	ClassDB::bind_method(D_METHOD("get_custom_map_layer", "id"), &ClockGearsDataManager::get_custom_map_layer);
	ClassDB::bind_method(D_METHOD("create_custom_map_layer", "name", "order"), &ClockGearsDataManager::create_custom_map_layer, DEFVAL(2));
	ClassDB::bind_method(D_METHOD("rename_custom_map_layer", "id", "new_name"), &ClockGearsDataManager::rename_custom_map_layer);
	ClassDB::bind_method(D_METHOD("set_custom_map_layer_order", "id", "order"), &ClockGearsDataManager::set_custom_map_layer_order);
	ClassDB::bind_method(D_METHOD("set_custom_map_layer_visible", "id", "visible"), &ClockGearsDataManager::set_custom_map_layer_visible);
	ClassDB::bind_method(D_METHOD("set_custom_map_layer_locked", "id", "locked"), &ClockGearsDataManager::set_custom_map_layer_locked);
	ClassDB::bind_method(D_METHOD("duplicate_custom_map_layer", "source_id", "new_name"), &ClockGearsDataManager::duplicate_custom_map_layer, DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("delete_custom_map_layer", "id"), &ClockGearsDataManager::delete_custom_map_layer);

	ClassDB::bind_method(D_METHOD("set_editor_dim_inactive_layers_enabled", "enabled"), &ClockGearsDataManager::set_editor_dim_inactive_layers_enabled);
	ClassDB::bind_method(D_METHOD("is_editor_dim_inactive_layers_enabled"), &ClockGearsDataManager::is_editor_dim_inactive_layers_enabled);
	ClassDB::bind_method(D_METHOD("set_editor_inactive_layer_opacity_pct", "pct"), &ClockGearsDataManager::set_editor_inactive_layer_opacity_pct);
	ClassDB::bind_method(D_METHOD("get_editor_inactive_layer_opacity_pct"), &ClockGearsDataManager::get_editor_inactive_layer_opacity_pct);
	ClassDB::bind_method(D_METHOD("sync_editor_layer_dimming_from_settings"), &ClockGearsDataManager::sync_editor_layer_dimming_from_settings);

	ClassDB::bind_method(D_METHOD("set_editor_map", "map"), &ClockGearsDataManager::set_editor_map);
	ClassDB::bind_method(D_METHOD("get_editor_map"), &ClockGearsDataManager::get_editor_map);

	ClassDB::bind_method(D_METHOD("simulate_editor_mouse_press", "canvas_pos", "right_button"), &ClockGearsDataManager::simulate_editor_mouse_press, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("simulate_editor_mouse_motion", "canvas_pos"), &ClockGearsDataManager::simulate_editor_mouse_motion);
	ClassDB::bind_method(D_METHOD("simulate_editor_mouse_release", "canvas_pos", "right_button"), &ClockGearsDataManager::simulate_editor_mouse_release, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("simulate_editor_click_cell", "cell_x", "cell_y", "right_button"), &ClockGearsDataManager::simulate_editor_click_cell, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("simulate_editor_drag_rect", "x0", "y0", "x1", "y1", "right_button"), &ClockGearsDataManager::simulate_editor_drag_rect, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("set_editor_draw_tool", "tool"), &ClockGearsDataManager::set_editor_draw_tool);
	ClassDB::bind_method(D_METHOD("get_editor_draw_tool"), &ClockGearsDataManager::get_editor_draw_tool);
	ClassDB::bind_method(D_METHOD("set_editor_layer", "layer"), &ClockGearsDataManager::set_editor_layer);
	ClassDB::bind_method(D_METHOD("get_editor_layer"), &ClockGearsDataManager::get_editor_layer);
	ClassDB::bind_method(D_METHOD("set_editor_custom_layer", "id"), &ClockGearsDataManager::set_editor_custom_layer);
	ClassDB::bind_method(D_METHOD("get_editor_custom_layer"), &ClockGearsDataManager::get_editor_custom_layer);
	ClassDB::bind_method(D_METHOD("get_active_editor_layer_display_name"), &ClockGearsDataManager::get_active_editor_layer_display_name);
	ClassDB::bind_method(D_METHOD("set_editor_selected_tile", "coords", "source_id"), &ClockGearsDataManager::set_editor_selected_tile, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_editor_selected_tile"), &ClockGearsDataManager::get_editor_selected_tile);
	ClassDB::bind_method(D_METHOD("set_editor_map_selection", "rect"), &ClockGearsDataManager::set_editor_map_selection);
	ClassDB::bind_method(D_METHOD("get_editor_map_selection"), &ClockGearsDataManager::get_editor_map_selection);
	ClassDB::bind_method(D_METHOD("has_editor_map_selection"), &ClockGearsDataManager::has_editor_map_selection);
	ClassDB::bind_method(D_METHOD("trigger_editor_copy"), &ClockGearsDataManager::trigger_editor_copy);
	ClassDB::bind_method(D_METHOD("trigger_editor_cut"), &ClockGearsDataManager::trigger_editor_cut);
	ClassDB::bind_method(D_METHOD("trigger_editor_paste", "target_cell"), &ClockGearsDataManager::trigger_editor_paste, DEFVAL(Vector2i(-999999, -999999)));
	ClassDB::bind_method(D_METHOD("trigger_editor_delete"), &ClockGearsDataManager::trigger_editor_delete);
	ClassDB::bind_method(D_METHOD("trigger_editor_undo"), &ClockGearsDataManager::trigger_editor_undo);
	ClassDB::bind_method(D_METHOD("trigger_editor_redo"), &ClockGearsDataManager::trigger_editor_redo);
	ClassDB::bind_method(D_METHOD("get_main_menu_inspection_info"), &ClockGearsDataManager::get_main_menu_inspection_info);
	ClassDB::bind_method(D_METHOD("simulate_editor_shortcut", "action_name"), &ClockGearsDataManager::simulate_editor_shortcut);

	BIND_CONSTANT(MAX_CUSTOM_MAP_LAYERS);

	ADD_SIGNAL(MethodInfo("data_changed", PropertyInfo(Variant::STRING, "category"), PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("drop_obtained", PropertyInfo(Variant::STRING, "item_name"), PropertyInfo(Variant::INT, "amount")));
	ADD_SIGNAL(MethodInfo("actor_leveled_up", PropertyInfo(Variant::INT, "actor_id"), PropertyInfo(Variant::INT, "new_level")));
	ADD_SIGNAL(MethodInfo("player_options_changed"));
	ADD_SIGNAL(MethodInfo("save_slots_changed"));
	ADD_SIGNAL(MethodInfo("runtime_state_changed", PropertyInfo(Variant::STRING, "topic"), PropertyInfo(Variant::STRING, "key")));
}

String ClockGearsDataManager::sanitize_custom_layer_node_name(const String &p_id) {
	String trimmed = p_id.strip_edges();
	String out;
	for (int i = 0; i < trimmed.length(); i++) {
		char32_t ch = trimmed[i];
		if ((ch >= U'a' && ch <= U'z') || (ch >= U'A' && ch <= U'Z') || (ch >= U'0' && ch <= U'9') || ch == U'_') {
			out += ch;
		} else {
			out += U'_';
		}
	}
	if (out.is_empty()) {
		out = "custom";
	}
	return out;
}

Dictionary ClockGearsDataManager::normalize_custom_map_layer(const Dictionary &p_raw, int p_idx) {
	Dictionary out;
	String raw_id = String(p_raw.get("id", "")).strip_edges();
	if (raw_id.is_empty()) {
		raw_id = vformat("custom_%d", MAX(1, p_idx + 1));
	}
	raw_id = sanitize_custom_layer_node_name(raw_id);
	String raw_name = String(p_raw.get("name", "")).strip_edges();
	if (raw_name.is_empty()) {
		raw_name = vformat(String::utf8("Capa personalizada %d"), MAX(1, p_idx + 1));
	}
	int ord = CLAMP(int(p_raw.get("order", 2)), -50, 50);
	bool vis = p_raw.has("visible") ? bool(p_raw["visible"]) : true;
	bool lck = p_raw.has("locked") ? bool(p_raw["locked"]) : false;
	out["id"] = raw_id;
	out["name"] = raw_name;
	out["order"] = ord;
	out["visible"] = vis;
	out["locked"] = lck;
	return out;
}

Dictionary ClockGearsDataManager::_make_unique_custom_layer_dict(const String &p_name, int p_order, const String &p_id_hint) const {
	Array existing = get_custom_map_layers();
	HashMap<String, bool> used_ids;
	int max_num = 0;
	for (int i = 0; i < existing.size(); i++) {
		Dictionary d = existing[i];
		String eid = String(d.get("id", ""));
		used_ids[eid] = true;
		if (eid.begins_with("custom_")) {
			max_num = MAX(max_num, eid.substr(7).to_int());
		}
	}
	String cand_id = p_id_hint.is_empty() ? String() : sanitize_custom_layer_node_name(p_id_hint);
	if (cand_id.is_empty() || used_ids.has(cand_id)) {
		int n = MAX(1, max_num + 1);
		do {
			cand_id = vformat("custom_%d", n++);
		} while (used_ids.has(cand_id));
	}
	Dictionary raw;
	raw["id"] = cand_id;
	raw["name"] = p_name;
	raw["order"] = p_order;
	raw["visible"] = true;
	raw["locked"] = false;
	return normalize_custom_map_layer(raw, existing.size());
}

Array ClockGearsDataManager::get_custom_map_layers() const {
	Array out;
	if (!system_config.has("custom_map_layers") || system_config["custom_map_layers"].get_type() != Variant::ARRAY) {
		return out;
	}
	Array raw = system_config["custom_map_layers"];
	HashMap<String, bool> seen;
	for (int i = 0; i < raw.size() && out.size() < MAX_CUSTOM_MAP_LAYERS; i++) {
		if (raw[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary norm = normalize_custom_map_layer(Dictionary(raw[i]), i);
		String id = String(norm["id"]);
		if (seen.has(id)) {
			int suffix = 2;
			while (seen.has(vformat("%s_%d", id, suffix))) {
				suffix++;
			}
			id = vformat("%s_%d", id, suffix);
			norm["id"] = id;
		}
		seen[id] = true;
		out.push_back(norm);
	}
	return out;
}

void ClockGearsDataManager::set_custom_map_layers(const Array &p_layers) {
	Array normalized;
	HashMap<String, bool> seen;
	for (int i = 0; i < p_layers.size() && normalized.size() < MAX_CUSTOM_MAP_LAYERS; i++) {
		if (p_layers[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary norm = normalize_custom_map_layer(Dictionary(p_layers[i]), i);
		String id = String(norm["id"]);
		if (seen.has(id)) {
			int suffix = 2;
			while (seen.has(vformat("%s_%d", id, suffix))) {
				suffix++;
			}
			id = vformat("%s_%d", id, suffix);
			norm["id"] = id;
		}
		seen[id] = true;
		normalized.push_back(norm);
	}
	if (normalized.is_empty()) {
		system_config.erase("custom_map_layers");
	} else {
		system_config["custom_map_layers"] = normalized;
	}
	if (!active_editor_custom_layer_id.is_empty() && !seen.has(active_editor_custom_layer_id)) {
		active_editor_custom_layer_id = String();
		active_editor_layer = 1;
	}
	dirty = true;
	_sync_editor_map_layers_and_topbar(true);
	emit_signal("data_changed", "system", 0);
	emit_signal("data_changed", "custom_layers", 0);
}

Dictionary ClockGearsDataManager::get_custom_map_layer(const String &p_id) const {
	if (p_id.strip_edges().is_empty()) {
		return Dictionary();
	}
	Array arr = get_custom_map_layers();
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		if (String(d.get("id", "")) == p_id) {
			return d;
		}
	}
	return Dictionary();
}

Dictionary ClockGearsDataManager::create_custom_map_layer(const String &p_name, int p_order) {
	Array arr = get_custom_map_layers();
	if (arr.size() >= MAX_CUSTOM_MAP_LAYERS) {
		return Dictionary();
	}
	Dictionary created = _make_unique_custom_layer_dict(p_name, p_order);
	arr.push_back(created);
	system_config["custom_map_layers"] = arr;
	dirty = true;
	_sync_editor_map_layers_and_topbar(true);
	emit_signal("data_changed", "custom_layers", 0);
	return created;
}

bool ClockGearsDataManager::rename_custom_map_layer(const String &p_id, const String &p_new_name) {
	String trimmed = p_new_name.strip_edges();
	if (trimmed.is_empty()) {
		return false;
	}
	Array arr = get_custom_map_layers();
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		if (String(d.get("id", "")) == p_id) {
			d["name"] = trimmed;
			arr[i] = d;
			system_config["custom_map_layers"] = arr;
			dirty = true;
			_sync_editor_map_layers_and_topbar(false);
			emit_signal("data_changed", "custom_layers", 0);
			return true;
		}
	}
	return false;
}

bool ClockGearsDataManager::set_custom_map_layer_order(const String &p_id, int p_order) {
	Array arr = get_custom_map_layers();
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		if (String(d.get("id", "")) == p_id) {
			d["order"] = CLAMP(p_order, -50, 50);
			arr[i] = d;
			system_config["custom_map_layers"] = arr;
			dirty = true;
			_sync_editor_map_layers_and_topbar(true);
			emit_signal("data_changed", "custom_layers", 0);
			return true;
		}
	}
	return false;
}

bool ClockGearsDataManager::set_custom_map_layer_visible(const String &p_id, bool p_visible) {
	Array arr = get_custom_map_layers();
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		if (String(d.get("id", "")) == p_id) {
			d["visible"] = p_visible;
			arr[i] = d;
			system_config["custom_map_layers"] = arr;
			dirty = true;
			_sync_editor_map_layers_and_topbar(false);
			emit_signal("data_changed", "custom_layers", 0);
			return true;
		}
	}
	return false;
}

bool ClockGearsDataManager::set_custom_map_layer_locked(const String &p_id, bool p_locked) {
	Array arr = get_custom_map_layers();
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		if (String(d.get("id", "")) == p_id) {
			d["locked"] = p_locked;
			arr[i] = d;
			system_config["custom_map_layers"] = arr;
			dirty = true;
			_sync_editor_map_layers_and_topbar(false);
			emit_signal("data_changed", "custom_layers", 0);
			return true;
		}
	}
	return false;
}

Dictionary ClockGearsDataManager::duplicate_custom_map_layer(const String &p_source_id, const String &p_new_name) {
	Array arr = get_custom_map_layers();
	if (arr.size() >= MAX_CUSTOM_MAP_LAYERS) {
		return Dictionary();
	}
	Dictionary src = get_custom_map_layer(p_source_id);
	if (src.is_empty()) {
		return Dictionary();
	}
	String chosen_name = p_new_name.strip_edges();
	if (chosen_name.is_empty()) {
		chosen_name = String(src.get("name", "")) + String::utf8(" (Copia)");
	}
	// Decisión mía: al duplicar una capa se coloca un nivel por encima (order + 1) y desbloqueada/visible para distinguirla enseguida.
	int dst_order = CLAMP(int(src.get("order", 2)) + 1, -50, 50);
	Dictionary created = _make_unique_custom_layer_dict(chosen_name, dst_order);
	arr.push_back(created);
	system_config["custom_map_layers"] = arr;
	String new_id = String(created["id"]);

	ClockGearsMap *amap = (active_editor_map_id.is_valid() && ObjectDB::get_instance(active_editor_map_id) == active_editor_map) ? Object::cast_to<ClockGearsMap>(active_editor_map) : nullptr;
	if (amap) {
		amap->duplicate_custom_layer_tiles(p_source_id, new_id);
	}
#ifdef TOOLS_ENABLED
	if (EditorNode::get_singleton()) {
		ClockGearsMap *emap = Object::cast_to<ClockGearsMap>(EditorNode::get_singleton()->get_edited_scene());
		if (emap && emap != amap) {
			emap->duplicate_custom_layer_tiles(p_source_id, new_id);
		}
	}
#endif
	dirty = true;
	_sync_editor_map_layers_and_topbar(true);
	emit_signal("data_changed", "custom_layers", 0);
	return created;
}

bool ClockGearsDataManager::delete_custom_map_layer(const String &p_id) {
	Array arr = get_custom_map_layers();
	int found_idx = -1;
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		if (String(d.get("id", "")) == p_id) {
			found_idx = i;
			break;
		}
	}
	if (found_idx < 0) {
		return false;
	}
	arr.remove_at(found_idx);
	if (arr.is_empty()) {
		system_config.erase("custom_map_layers");
	} else {
		system_config["custom_map_layers"] = arr;
	}
	if (active_editor_custom_layer_id == p_id) {
		active_editor_custom_layer_id = String();
		active_editor_layer = 1;
#ifdef TOOLS_ENABLED
		if (CanvasItemEditor::get_singleton()) {
			CanvasItemEditor::get_singleton()->set_clockgears_current_layer(1);
		}
#endif
	}

	ClockGearsMap *amap = (active_editor_map_id.is_valid() && ObjectDB::get_instance(active_editor_map_id) == active_editor_map) ? Object::cast_to<ClockGearsMap>(active_editor_map) : nullptr;
	if (amap) {
		amap->remove_custom_layer_node(p_id);
	}
#ifdef TOOLS_ENABLED
	if (EditorNode::get_singleton()) {
		ClockGearsMap *emap = Object::cast_to<ClockGearsMap>(EditorNode::get_singleton()->get_edited_scene());
		if (emap && emap != amap) {
			emap->remove_custom_layer_node(p_id);
		}
	}
#endif
	dirty = true;
	_sync_editor_map_layers_and_topbar(false);
	emit_signal("data_changed", "custom_layers", 0);
	return true;
}

void ClockGearsDataManager::set_editor_dim_inactive_layers_enabled(bool p_enabled) {
	dim_inactive_layers = p_enabled;
	if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
		ps->set_setting("clockgears/editor/dim_inactive_layers", dim_inactive_layers);
	}
	_sync_editor_map_layers_and_topbar(false);
}

bool ClockGearsDataManager::is_editor_dim_inactive_layers_enabled() const {
	if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
		if (ps->has_setting("clockgears/editor/dim_inactive_layers")) {
			return bool(ps->get_setting("clockgears/editor/dim_inactive_layers"));
		}
	}
	return dim_inactive_layers;
}

void ClockGearsDataManager::set_editor_inactive_layer_opacity_pct(int p_pct) {
	inactive_layer_opacity_pct = CLAMP(p_pct, 10, 100);
	if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
		ps->set_setting("clockgears/editor/inactive_layer_opacity_pct", inactive_layer_opacity_pct);
	}
	_sync_editor_map_layers_and_topbar(false);
}

int ClockGearsDataManager::get_editor_inactive_layer_opacity_pct() const {
	if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
		if (ps->has_setting("clockgears/editor/inactive_layer_opacity_pct")) {
			return CLAMP(int(ps->get_setting("clockgears/editor/inactive_layer_opacity_pct")), 10, 100);
		}
	}
	return CLAMP(inactive_layer_opacity_pct, 10, 100);
}

void ClockGearsDataManager::sync_editor_layer_dimming_from_settings() {
	if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
		dim_inactive_layers = bool(ps->get_setting("clockgears/editor/dim_inactive_layers", true));
		inactive_layer_opacity_pct = CLAMP(int(ps->get_setting("clockgears/editor/inactive_layer_opacity_pct", 40)), 10, 100);
	}
	_sync_editor_map_layers_and_topbar(false);
}

void ClockGearsDataManager::_sync_editor_map_layers_and_topbar(bool p_rebuild_nodes) {
	ClockGearsMap *amap = (active_editor_map_id.is_valid() && ObjectDB::get_instance(active_editor_map_id) == active_editor_map) ? Object::cast_to<ClockGearsMap>(active_editor_map) : nullptr;
	if (amap) {
		if (p_rebuild_nodes) {
			amap->sync_custom_layers_from_project(false);
		}
		amap->invalidate_passability_cache();
		amap->apply_editor_layer_dimming(true);
	}
#ifdef TOOLS_ENABLED
	if (EditorNode::get_singleton()) {
		ClockGearsMap *emap = Object::cast_to<ClockGearsMap>(EditorNode::get_singleton()->get_edited_scene());
		if (emap && emap != amap) {
			if (p_rebuild_nodes) {
				emap->sync_custom_layers_from_project(false);
			}
			emap->invalidate_passability_cache();
			emap->apply_editor_layer_dimming(true);
		}
	}
	EditorNode::sync_clockgears_active_layer_indicator_all();
	if (ClockGearsTopBar::get_singleton()) {
		ClockGearsTopBar::get_singleton()->sync_active_layer_display();
	}
#endif
}

bool ClockGearsDataManager::_is_active_layer_editable_in_editor() const {
	if (active_editor_custom_layer_id.is_empty()) {
		return true;
	}
	Dictionary layer = get_custom_map_layer(active_editor_custom_layer_id);
	if (layer.is_empty()) {
		return false;
	}
	if (!bool(layer.get("visible", true))) {
		return false;
	}
	if (bool(layer.get("locked", false))) {
		return false;
	}
	return true;
}

void ClockGearsDataManager::_ensure_headless_undo_redo() {
	if (!headless_undo_redo) {
		headless_undo_redo = memnew(UndoRedo);
	}
}

void ClockGearsDataManager::_apply_headless_tile_undo_redo_cell(Node *p_map_node, int p_layer_idx, const String &p_custom_id, const Vector2i &p_cell, int p_source_id, const Vector2i &p_atlas_coords, int p_alt) {
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(p_map_node);
	if (!map) {
		return;
	}
	TileMapLayer *layer = p_custom_id.is_empty() ? map->get_layer(CLAMP(p_layer_idx, 1, 4) - 1) : map->get_or_create_custom_layer(p_custom_id);
	if (!layer) {
		return;
	}
	if (p_source_id < 0 || p_atlas_coords.x < 0 || p_atlas_coords.y < 0) {
		layer->erase_cell(p_cell);
	} else {
		Ref<TileSet> ts = layer->get_tile_set();
		if (ts.is_null() && map->get_layer(0) && map->get_layer(0)->get_tile_set().is_valid()) {
			ts = map->get_layer(0)->get_tile_set();
			layer->set_tile_set(ts);
		}
		if (ts.is_valid() && ts->has_source(p_source_id)) {
			Ref<TileSetAtlasSource> atlas = ts->get_source(p_source_id);
			if (atlas.is_valid() && !atlas->has_tile(p_atlas_coords)) {
				atlas->create_tile(p_atlas_coords);
			}
		}
		layer->set_cell(p_cell, p_source_id, p_atlas_coords, p_alt);
	}
	map->invalidate_passability_cache();
	map->apply_editor_layer_dimming(true);
}

void ClockGearsDataManager::_apply_headless_shadow_undo_redo_cell(Node *p_map_node, const Vector2i &p_cell, bool p_enabled) {
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(p_map_node);
	if (!map) {
		return;
	}
	map->set_cell_shadow(p_cell, p_enabled);
}

Node *ClockGearsDataManager::get_editor_map() const {
	if (active_editor_map_id.is_valid() && ObjectDB::get_instance(active_editor_map_id) == active_editor_map) {
		return active_editor_map;
	}
	return nullptr;
}

void ClockGearsDataManager::set_editor_map(Node *p_map) {
	active_editor_map = p_map;
	active_editor_map_id = p_map ? p_map->get_instance_id() : ObjectID();
	if (ClockGearsMap *m = Object::cast_to<ClockGearsMap>(p_map)) {
		m->sync_custom_layers_from_project(false);
		m->apply_editor_layer_dimming(true);
	}
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
		return;
	}
#endif
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(get_editor_map());
	if (map) {
		int gs = MAX(1, map->get_grid_resolution());
		simulate_editor_click_cell(int(Math::floor(p_canvas_pos.x / float(gs))), int(Math::floor(p_canvas_pos.y / float(gs))), p_right_button);
	}
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
		return;
	}
#endif
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(get_editor_map());
	if (!map) {
		return;
	}
	Vector2i cell(p_cell_x, p_cell_y);
	if (headless_draw_tool == 5) {
		headless_has_selection = true;
		headless_selection_rect = Rect2i(cell, Vector2i(1, 1));
		return;
	}
	if (headless_draw_tool == 4) {
		_ensure_headless_undo_redo();
		bool old_s = map->get_cell_shadow(cell);
		bool new_s = !p_right_button;
		if (old_s != new_s) {
			headless_undo_redo->create_action(String::utf8("Sombra ClockGears"));
			headless_undo_redo->add_do_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_shadow_undo_redo_cell).bind(map, cell, new_s));
			headless_undo_redo->add_undo_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_shadow_undo_redo_cell).bind(map, cell, old_s));
			headless_undo_redo->commit_action(true);
		}
		return;
	}
	if (!_is_active_layer_editable_in_editor()) {
		return;
	}

	int eff_layer = active_editor_layer;
	String custom_id = active_editor_custom_layer_id;
	TileMapLayer *target = nullptr;
	if (!custom_id.is_empty()) {
		target = map->get_or_create_custom_layer(custom_id);
	} else {
		eff_layer = CLAMP(eff_layer, 1, 4);
		target = map->get_layer(eff_layer - 1);
	}
	if (!target) {
		return;
	}

	bool erase = p_right_button || headless_draw_tool == 7;
	_ensure_headless_undo_redo();

	if (headless_draw_tool == 3) {
		Vector2i msz = map->get_map_size();
		if (cell.x < 0 || cell.y < 0 || cell.x >= msz.x || cell.y >= msz.y) {
			return;
		}
		int target_src = target->get_cell_source_id(cell);
		Vector2i target_ac = target->get_cell_atlas_coords(cell);
		int target_alt = target->get_cell_alternative_tile(cell);
		int new_src = erase ? -1 : headless_selected_source_id;
		Vector2i new_ac = erase ? Vector2i(-1, -1) : headless_selected_tile_coords;
		if (target_src == new_src && target_ac == new_ac) {
			return;
		}
		LocalVector<Vector2i> queue;
		HashMap<Vector2i, bool> visited;
		queue.push_back(cell);
		visited[cell] = true;
		headless_undo_redo->create_action(String::utf8("Relleno ClockGears"));
		uint32_t head = 0;
		const Vector2i dirs[4] = { Vector2i(1, 0), Vector2i(-1, 0), Vector2i(0, 1), Vector2i(0, -1) };
		while (head < queue.size() && queue.size() < 4096) {
			Vector2i cur = queue[head++];
			headless_undo_redo->add_do_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, cur, new_src, new_ac, 0));
			headless_undo_redo->add_undo_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, cur, target_src, target_ac, target_alt));
			for (int d = 0; d < 4; d++) {
				Vector2i nxt = cur + dirs[d];
				if (nxt.x >= 0 && nxt.y >= 0 && nxt.x < msz.x && nxt.y < msz.y && !visited.has(nxt)) {
					if (target->get_cell_source_id(nxt) == target_src && target->get_cell_atlas_coords(nxt) == target_ac) {
						visited[nxt] = true;
						queue.push_back(nxt);
					}
				}
			}
		}
		headless_undo_redo->commit_action(true);
		return;
	}

	int old_src = target->get_cell_source_id(cell);
	Vector2i old_ac = target->get_cell_atlas_coords(cell);
	int old_alt = target->get_cell_alternative_tile(cell);
	int new_src = erase ? -1 : headless_selected_source_id;
	Vector2i new_ac = erase ? Vector2i(-1, -1) : headless_selected_tile_coords;
	if (old_src == new_src && old_ac == new_ac) {
		return;
	}
	headless_undo_redo->create_action(erase ? String::utf8("Borrar Tile") : String::utf8("Pintar Tile"));
	headless_undo_redo->add_do_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, cell, new_src, new_ac, 0));
	headless_undo_redo->add_undo_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, cell, old_src, old_ac, old_alt));
	headless_undo_redo->commit_action(true);
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
		return;
	}
#endif
	int minx = MIN(p_x0, p_x1);
	int maxx = MAX(p_x0, p_x1);
	int miny = MIN(p_y0, p_y1);
	int maxy = MAX(p_y0, p_y1);
	if (headless_draw_tool == 5) {
		headless_has_selection = true;
		headless_selection_rect = Rect2i(minx, miny, maxx - minx + 1, maxy - miny + 1);
		return;
	}
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(get_editor_map());
	if (!map) {
		return;
	}
	if (headless_draw_tool == 4) {
		_ensure_headless_undo_redo();
		headless_undo_redo->create_action(String::utf8("Sombra en Área"));
		bool new_s = !p_right_button;
		for (int cy = miny; cy <= maxy; cy++) {
			for (int cx = minx; cx <= maxx; cx++) {
				Vector2i c(cx, cy);
				bool old_s = map->get_cell_shadow(c);
				if (old_s != new_s) {
					headless_undo_redo->add_do_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_shadow_undo_redo_cell).bind(map, c, new_s));
					headless_undo_redo->add_undo_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_shadow_undo_redo_cell).bind(map, c, old_s));
				}
			}
		}
		headless_undo_redo->commit_action(true);
		return;
	}
	if (!_is_active_layer_editable_in_editor()) {
		return;
	}
	int eff_layer = active_editor_layer;
	String custom_id = active_editor_custom_layer_id;
	TileMapLayer *target = !custom_id.is_empty() ? map->get_or_create_custom_layer(custom_id) : map->get_layer(CLAMP(eff_layer, 1, 4) - 1);
	if (!target) {
		return;
	}
	bool erase = p_right_button || headless_draw_tool == 7;
	int new_src = erase ? -1 : headless_selected_source_id;
	Vector2i new_ac = erase ? Vector2i(-1, -1) : headless_selected_tile_coords;
	bool is_ellipse = (headless_draw_tool == 2);
	double cx_mid = (minx + maxx) * 0.5;
	double cy_mid = (miny + maxy) * 0.5;
	double rx = MAX(0.5, (maxx - minx) * 0.5 + 0.5);
	double ry = MAX(0.5, (maxy - miny) * 0.5 + 0.5);

	_ensure_headless_undo_redo();
	headless_undo_redo->create_action(String::utf8("Pintar Área ClockGears"));
	int changed = 0;
	for (int cy = miny; cy <= maxy; cy++) {
		for (int cx = minx; cx <= maxx; cx++) {
			if (is_ellipse) {
				double dx = (cx - cx_mid) / rx;
				double dy = (cy - cy_mid) / ry;
				if ((dx * dx + dy * dy) > 1.0) {
					continue;
				}
			}
			Vector2i cell(cx, cy);
			int old_src = target->get_cell_source_id(cell);
			Vector2i old_ac = target->get_cell_atlas_coords(cell);
			int old_alt = target->get_cell_alternative_tile(cell);
			if (old_src != new_src || old_ac != new_ac) {
				headless_undo_redo->add_do_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, cell, new_src, new_ac, 0));
				headless_undo_redo->add_undo_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, cell, old_src, old_ac, old_alt));
				changed++;
			}
		}
	}
	if (changed > 0) {
		headless_undo_redo->commit_action(true);
	} else {
		headless_undo_redo->clear_history(false);
	}
}

void ClockGearsDataManager::set_editor_draw_tool(int p_tool) {
	headless_draw_tool = p_tool;
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->set_clockgears_draw_tool(p_tool);
	}
#endif
}

void ClockGearsDataManager::set_editor_layer(int p_layer) {
	active_editor_custom_layer_id = String();
	active_editor_layer = CLAMP(p_layer, 1, 6);
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->set_clockgears_current_layer(active_editor_layer);
	}
#endif
	_sync_editor_map_layers_and_topbar(false);
}

bool ClockGearsDataManager::set_editor_custom_layer(const String &p_id) {
	Dictionary info = get_custom_map_layer(p_id);
	if (info.is_empty()) {
		return false;
	}
	active_editor_custom_layer_id = p_id;
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->set_clockgears_current_custom_layer(p_id);
	}
#endif
	_sync_editor_map_layers_and_topbar(false);
	return true;
}

String ClockGearsDataManager::get_active_editor_layer_display_name() const {
	if (!active_editor_custom_layer_id.is_empty()) {
		Dictionary info = get_custom_map_layer(active_editor_custom_layer_id);
		if (!info.is_empty()) {
			return String(info.get("name", active_editor_custom_layer_id));
		}
	}
	switch (active_editor_layer) {
		case 1: return "Capa 1: Suelo";
		case 2: return String::utf8("Capa 2: Decoración");
		case 3: return "Capa 3: Objetos";
		case 4: return String::utf8("Capa 4: Techo (★)");
		case 5: return String::utf8("Capa Automática");
		case 6: return "Capa R: Regiones";
		default: return "Capa 1: Suelo";
	}
}

void ClockGearsDataManager::set_editor_selected_tile(const Vector2i &p_coords, int p_source_id) {
	headless_selected_tile_coords = p_coords;
	headless_selected_source_id = MAX(0, p_source_id);
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->set_clockgears_selected_tile(0, p_coords, Ref<Texture2D>(), p_source_id);
	}
#endif
}

void ClockGearsDataManager::set_editor_map_selection(const Rect2i &p_rect) {
	headless_selection_rect = p_rect;
	headless_has_selection = p_rect.size.x > 0 && p_rect.size.y > 0;
}

void ClockGearsDataManager::trigger_editor_copy() {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed && ed->has_clockgears_map_selection()) {
		ed->clockgears_copy_map_selection();
		return;
	}
#endif
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(get_editor_map());
	if (!map || !headless_has_selection) {
		return;
	}
	TileMapLayer *target = !active_editor_custom_layer_id.is_empty() ? map->get_custom_layer(active_editor_custom_layer_id) : map->get_layer(CLAMP(active_editor_layer, 1, 4) - 1);
	if (!target) {
		return;
	}
	Dictionary cells;
	for (int dy = 0; dy < headless_selection_rect.size.y; dy++) {
		for (int dx = 0; dx < headless_selection_rect.size.x; dx++) {
			Vector2i c(headless_selection_rect.position.x + dx, headless_selection_rect.position.y + dy);
			int src = target->get_cell_source_id(c);
			if (src >= 0) {
				Dictionary cd;
				cd["source_id"] = src;
				cd["atlas_coords"] = target->get_cell_atlas_coords(c);
				cd["alt"] = target->get_cell_alternative_tile(c);
				cells[vformat("%d,%d", dx, dy)] = cd;
			}
		}
	}
	headless_clipboard.clear();
	headless_clipboard["size"] = headless_selection_rect.size;
	headless_clipboard["cells"] = cells;
}

void ClockGearsDataManager::trigger_editor_cut() {
	trigger_editor_copy();
	trigger_editor_delete();
}

void ClockGearsDataManager::trigger_editor_paste(const Vector2i &p_target_cell) {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed && ed->has_clockgears_map_selection()) {
		ed->clockgears_paste_map_selection();
		return;
	}
#endif
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(get_editor_map());
	if (!map || headless_clipboard.is_empty() || !_is_active_layer_editable_in_editor()) {
		return;
	}
	int eff_layer = CLAMP(active_editor_layer, 1, 4);
	String custom_id = active_editor_custom_layer_id;
	TileMapLayer *target = !custom_id.is_empty() ? map->get_or_create_custom_layer(custom_id) : map->get_layer(eff_layer - 1);
	if (!target) {
		return;
	}
	Vector2i anchor = (p_target_cell.x > -900000 && p_target_cell.y > -900000) ? p_target_cell : (headless_has_selection ? headless_selection_rect.position : Vector2i(0, 0));
	Dictionary cells = headless_clipboard.get("cells", Dictionary());
	if (cells.is_empty()) {
		return;
	}
	Array keys = cells.keys();
	_ensure_headless_undo_redo();
	headless_undo_redo->create_action(String::utf8("Pegar Área ClockGears"));
	int changed = 0;
	for (int i = 0; i < keys.size(); i++) {
		String k = String(keys[i]);
		int comma = k.find(",");
		if (comma <= 0) {
			continue;
		}
		Vector2i c(anchor.x + k.substr(0, comma).to_int(), anchor.y + k.substr(comma + 1).to_int());
		Dictionary cd = cells[keys[i]];
		int new_src = int(cd.get("source_id", 0));
		Vector2i new_ac = cd.get("atlas_coords", Vector2i(-1, -1));
		int new_alt = int(cd.get("alt", 0));
		int old_src = target->get_cell_source_id(c);
		Vector2i old_ac = target->get_cell_atlas_coords(c);
		int old_alt = target->get_cell_alternative_tile(c);
		if (old_src != new_src || old_ac != new_ac || old_alt != new_alt) {
			headless_undo_redo->add_do_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, c, new_src, new_ac, new_alt));
			headless_undo_redo->add_undo_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, c, old_src, old_ac, old_alt));
			changed++;
		}
	}
	if (changed > 0) {
		headless_undo_redo->commit_action(true);
	} else {
		headless_undo_redo->clear_history(false);
	}
}

void ClockGearsDataManager::trigger_editor_delete() {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed && ed->has_clockgears_map_selection()) {
		ed->clockgears_delete_map_selection();
		return;
	}
#endif
	ClockGearsMap *map = Object::cast_to<ClockGearsMap>(get_editor_map());
	if (!map || !headless_has_selection || !_is_active_layer_editable_in_editor()) {
		return;
	}
	int eff_layer = CLAMP(active_editor_layer, 1, 4);
	String custom_id = active_editor_custom_layer_id;
	TileMapLayer *target = !custom_id.is_empty() ? map->get_custom_layer(custom_id) : map->get_layer(eff_layer - 1);
	if (!target) {
		return;
	}
	_ensure_headless_undo_redo();
	headless_undo_redo->create_action(String::utf8("Eliminar Selección ClockGears"));
	int changed = 0;
	for (int dy = 0; dy < headless_selection_rect.size.y; dy++) {
		for (int dx = 0; dx < headless_selection_rect.size.x; dx++) {
			Vector2i c(headless_selection_rect.position.x + dx, headless_selection_rect.position.y + dy);
			int old_src = target->get_cell_source_id(c);
			if (old_src >= 0) {
				Vector2i old_ac = target->get_cell_atlas_coords(c);
				int old_alt = target->get_cell_alternative_tile(c);
				headless_undo_redo->add_do_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, c, -1, Vector2i(-1, -1), 0));
				headless_undo_redo->add_undo_method(callable_mp(this, &ClockGearsDataManager::_apply_headless_tile_undo_redo_cell).bind(map, eff_layer, custom_id, c, old_src, old_ac, old_alt));
				changed++;
			}
		}
	}
	if (changed > 0) {
		headless_undo_redo->commit_action(true);
	} else {
		headless_undo_redo->clear_history(false);
	}
}

void ClockGearsDataManager::trigger_editor_undo() {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->trigger_undo();
		return;
	}
#endif
	if (headless_undo_redo && headless_undo_redo->has_undo()) {
		headless_undo_redo->undo();
	}
}

void ClockGearsDataManager::trigger_editor_redo() {
#ifdef TOOLS_ENABLED
	CanvasItemEditor *ed = CanvasItemEditor::get_singleton();
	if (ed) {
		ed->trigger_redo();
		return;
	}
#endif
	if (headless_undo_redo && headless_undo_redo->has_redo()) {
		headless_undo_redo->redo();
	}
}

Dictionary ClockGearsDataManager::get_main_menu_inspection_info() const {
#ifdef TOOLS_ENABLED
	return EditorNode::inspect_clockgears_menus(EditorNode::get_singleton());
#else
	return Dictionary();
#endif
}

bool ClockGearsDataManager::simulate_editor_shortcut(const String &p_action_name) {
	String act = p_action_name.strip_edges().to_lower();
#ifdef TOOLS_ENABLED
	if (EditorNode::get_singleton() && EditorNode::get_singleton()->is_inside_tree()) {
		Ref<InputEventKey> k;
		k.instantiate();
		k->set_pressed(true);
		k->set_ctrl_pressed(true);
		if (act == "undo") {
			k->set_keycode(Key::Z);
		} else if (act == "redo") {
			k->set_keycode(Key::Z);
			k->set_shift_pressed(true);
		} else if (act == "redo_y") {
			k->set_keycode(Key::Y);
		} else if (act == "copy") {
			k->set_keycode(Key::C);
		} else if (act == "cut") {
			k->set_keycode(Key::X);
		} else if (act == "paste") {
			k->set_keycode(Key::V);
		} else if (act == "delete") {
			k->set_ctrl_pressed(false);
			k->set_keycode(Key::KEY_DELETE);
		} else if (act == "save") {
			k->set_keycode(Key::S);
		} else {
			return false;
		}
		if (Viewport *vp = EditorNode::get_singleton()->get_viewport()) {
			vp->push_input(k);
			return true;
		}
	}
#endif
	if (act == "undo") {
		trigger_editor_undo();
		return true;
	} else if (act == "redo" || act == "redo_y") {
		trigger_editor_redo();
		return true;
	} else if (act == "copy") {
		trigger_editor_copy();
		return true;
	} else if (act == "cut") {
		trigger_editor_cut();
		return true;
	} else if (act == "paste") {
		trigger_editor_paste();
		return true;
	} else if (act == "delete") {
		trigger_editor_delete();
		return true;
	}
	return false;
}

ClockGearsDataManager::ClockGearsDataManager() {
	singleton = this;
	init_empty();
}

ClockGearsDataManager::~ClockGearsDataManager() {
	if (headless_undo_redo) {
		memdelete(headless_undo_redo);
		headless_undo_redo = nullptr;
	}
	built_tileset_cache.clear();
	entry_icon_cache.clear();
	cached_ws_norm.clear();
	cached_ws_by_id.clear();
	if (singleton == this) {
		singleton = nullptr;
	}
}
