#include "clockgears_data_manager.h"
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

	system_config.clear();
	system_config["game_title"] = "";
	system_config["currency_unit"] = "G";
	system_config["combat_mode"] = 0; // 0 = Turnos Clásico, 1 = Action RPG Tiempo Real
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

void ClockGearsDataManager::_migrate_schema_if_needed(int p_from_version) {
	if (p_from_version >= CURRENT_SCHEMA_VERSION) {
		return;
	}
	// Garantiza que registros de versiones previas tengan todos los campos esperados con valores por defecto seguros
	for (int cat = 0; cat < CAT_MAX; cat++) {
		Array &arr = category_data[cat];
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary rec = arr[i];
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

void ClockGearsDataManager::set_system_config(const Dictionary &p_cfg) {
	system_config = p_cfg.duplicate(true);
	dirty = true;
	emit_signal("data_changed", "system", 0);
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

	Variant types_var;
	if (_read_json_with_backup(p_dir_path.path_join("types.json"), types_var) && types_var.get_type() == Variant::DICTIONARY) {
		if (ClockGearsSystemCatalog::get_singleton()) {
			ClockGearsSystemCatalog::get_singleton()->from_dict(types_var);
		}
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

	_migrate_schema_if_needed(loaded_schema);
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
	ClassDB::bind_method(D_METHOD("get_system_config"), &ClockGearsDataManager::get_system_config);
	ClassDB::bind_method(D_METHOD("set_system_config", "config"), &ClockGearsDataManager::set_system_config);
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
