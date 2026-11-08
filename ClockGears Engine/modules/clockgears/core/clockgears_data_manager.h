#pragma once

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "scene/main/node.h"

enum ClockGearsDatabaseCategory {
	CAT_ACTORS = 0,
	CAT_CLASSES,
	CAT_SKILLS,
	CAT_ITEMS,
	CAT_WEAPONS,
	CAT_ARMORS,
	CAT_ACCESSORIES,
	CAT_ENEMIES,
	CAT_TROOPS,
	CAT_STATES,
	CAT_FLORA,
	CAT_FAUNA,
	CAT_ANIMATIONS,
	CAT_TILESETS,
	CAT_COMMON_EVENTS,
	CAT_SYSTEM,
	CAT_TYPES,
	CAT_VOCABULARY,
	CAT_SWITCHES_VARS,
	CAT_MAX
};

class ClockGearsDataManager : public Object {
	GDCLASS(ClockGearsDataManager, Object);

public:
	static constexpr int CURRENT_SCHEMA_VERSION = 2;

private:
	static ClockGearsDataManager *singleton;

	Array category_data[CAT_MAX];
	Dictionary system_config;
	Dictionary vocabulary_config;
	Dictionary switches_names;
	Dictionary variables_names;
	Dictionary runtime_switches;
	Dictionary runtime_variables;
	Dictionary runtime_inventory;
	int runtime_gold = 0;
	bool dirty = false;
	Node *active_editor_map = nullptr;

	Variant _normalize_json_numbers(const Variant &p_val) const;
	bool _write_json_atomic(const String &p_file_path, const Variant &p_data) const;
	bool _read_json_with_backup(const String &p_file_path, Variant &r_data) const;
	void _migrate_schema_if_needed(int p_from_version);
	int _clean_cross_references_on_delete(int p_category, int p_deleted_id);

protected:
	static void _bind_methods();

public:
	static ClockGearsDataManager *get_singleton() { return singleton; }

	static String get_category_key(int p_category);
	static String get_category_filename(int p_category);
	static int get_category_from_key(const String &p_key);

	// Regla Cero: Inicializa el 100% de las listas de usuario vacías
	void init_empty();

	bool is_dirty() const { return dirty; }
	void mark_dirty(bool p_dirty = true) { dirty = p_dirty; }

	Array get_category_array(int p_category) const;
	void set_category_array(int p_category, const Array &p_array);

	Array get_records(const String &p_category_key) const;
	void set_records(const String &p_category_key, const Array &p_records);

	Dictionary get_record_by_id(int p_category, int p_id) const;
	int create_record(int p_category, const Dictionary &p_initial_data = Dictionary());
	bool update_record(int p_category, int p_id, const Dictionary &p_record);
	bool delete_record_by_id(int p_category, int p_id);

	Dictionary get_system_config() const { return system_config.duplicate(true); }
	void set_system_config(const Dictionary &p_cfg);

	Dictionary get_vocabulary_config() const { return vocabulary_config.duplicate(true); }
	void set_vocabulary_config(const Dictionary &p_vocab);

	Dictionary get_switches_names() const { return switches_names.duplicate(true); }
	void set_switches_names(const Dictionary &p_names);

	Dictionary get_variables_names() const { return variables_names.duplicate(true); }
	void set_variables_names(const Dictionary &p_names);

	// Runtime game state (switches, variables, inventory, gold) - Fase 4
	bool get_switch(const String &p_name) const;
	void set_switch(const String &p_name, bool p_val);
	int get_variable(const String &p_name) const;
	void set_variable(const String &p_name, int p_val);
	bool has_item(const String &p_name, int p_min_qty = 1) const;
	void change_item(const String &p_name, int p_amount);
	int get_gold() const { return runtime_gold; }
	void set_gold(int p_gold) { runtime_gold = MAX(0, p_gold); }
	void change_gold(int p_amount) { runtime_gold = MAX(0, runtime_gold + p_amount); }
	void reset_runtime_state();

	String format_text(const String &p_text) const;

	bool save_all(const String &p_dir_path = "res://clockgears_data");
	bool load_all(const String &p_dir_path = "res://clockgears_data");
	bool save_autosave(const String &p_base_dir = "res://clockgears_data/autosave");

	// Alias de compatibilidad con llamadas existentes
	bool save_database(const String &p_path = "res://clockgears_data/database.json");
	bool load_database(const String &p_path = "res://clockgears_data/database.json");

	// Editor simulation helpers for automated tests
	void set_editor_map(Node *p_map);
	Node *get_editor_map() const { return active_editor_map; }
	void simulate_editor_mouse_press(const Vector2 &p_canvas_pos, bool p_right_button = false);
	void simulate_editor_mouse_motion(const Vector2 &p_canvas_pos);
	void simulate_editor_mouse_release(const Vector2 &p_canvas_pos, bool p_right_button = false);
	void simulate_editor_click_cell(int p_cell_x, int p_cell_y, bool p_right_button = false);
	void simulate_editor_drag_rect(int p_x0, int p_y0, int p_x1, int p_y1, bool p_right_button = false);
	void set_editor_draw_tool(int p_tool);
	void set_editor_layer(int p_layer);
	void trigger_editor_undo();
	void trigger_editor_redo();

	ClockGearsDataManager();
	~ClockGearsDataManager();
};
