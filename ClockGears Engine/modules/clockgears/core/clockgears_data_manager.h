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
	CAT_NPCS,
	CAT_ARPG_ENEMIES,
	CAT_ARPG_NPCS,
	CAT_ARPG_WEAPONS,
	CAT_ARPG_ARMORS,
	CAT_ARPG_SHIELDS,
	CAT_ARPG_ACCESSORIES,
	CAT_MAX
};

class ClockGearsDataManager : public Object {
	GDCLASS(ClockGearsDataManager, Object);

public:
	static constexpr int CURRENT_SCHEMA_VERSION = 3;

	static void normalize_record_defaults(int p_category, Dictionary &r_record);
	static Dictionary get_normalized_record(int p_category, const Dictionary &p_record);
	static PackedStringArray get_arpg_ai_types();
	static PackedStringArray get_npc_move_types();
	static PackedStringArray get_detection_zone_triggers();
	static PackedStringArray get_detection_zone_actions();
	static Dictionary normalize_detection_zone(const Dictionary &p_zone, int p_idx = 0);

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
	Dictionary runtime_arpg_equips;
	Dictionary runtime_actor_states;
	Dictionary runtime_defeated_enemies;
	double runtime_playtime_sec = 0.0;
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

	static int normalize_mannequin_size(int p_size);

	Dictionary get_system_config() const { return system_config.duplicate(true); }
	void set_system_config(const Dictionary &p_cfg);
	int get_mannequin_size() const;
	void set_mannequin_size(int p_size);
	Dictionary get_exported_system_config() const;

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
	int get_item_qty(const String &p_name) const;
	Dictionary get_runtime_inventory() const { return runtime_inventory.duplicate(true); }
	void change_item(const String &p_name, int p_amount);
	int get_gold() const { return runtime_gold; }
	void set_gold(int p_gold) { runtime_gold = MAX(0, p_gold); }
	void change_gold(int p_amount) { runtime_gold = MAX(0, runtime_gold + p_amount); }
	void reset_runtime_state();

	// Estado persistente de actor, combate ARPG, drops y respawn (Tanda 3C-1)
	static float evaluate_traits_element_rate(const Array &p_traits, int p_element_id, const String &p_element_name = String());
	static int resolve_traits_attack_element_id(const Array &p_traits, String &r_element_key);
	int get_actor_arpg_attack_element_id(int p_actor_id, String &r_element_key) const;
	Dictionary get_actor_runtime_state(int p_actor_id);
	void set_actor_runtime_hp(int p_actor_id, int p_hp);
	int get_actor_runtime_hp(int p_actor_id);
	int get_actor_runtime_max_hp(int p_actor_id);
	int get_actor_runtime_level(int p_actor_id);
	int64_t get_actor_runtime_exp(int p_actor_id);
	Array grant_party_exp(int64_t p_exp_amount);
	Array resolve_arpg_enemy_drops(const Dictionary &p_enemy_rec);

	void advance_runtime_clock(double p_delta_sec);
	double get_runtime_clock_sec() const { return runtime_playtime_sec; }
	void set_runtime_clock_sec(double p_sec) { runtime_playtime_sec = MAX(0.0, p_sec); }
	void mark_arpg_enemy_defeated(const String &p_map_key, int p_event_id);
	bool is_arpg_enemy_defeated(const String &p_map_key, int p_event_id) const;
	Array process_map_respawns(const String &p_map_key, int p_respawn_mode, double p_respawn_minutes, bool p_is_entering_map);

	// Equipo ARPG por actor (Tanda 1C)
	static Array get_default_arpg_equip_slots();
	static Array normalize_arpg_equip_slots(const Array &p_raw_slots);
	Array get_actor_arpg_equip_slots(int p_actor_id) const;
	bool set_actor_arpg_equip(int p_actor_id, const String &p_slot_id, int p_piece_id);
	int get_actor_arpg_equip(int p_actor_id, const String &p_slot_id) const;
	Dictionary get_actor_arpg_equip_bonuses(int p_actor_id) const;
	Dictionary get_actor_effective_stats(int p_actor_id) const;
	Dictionary export_runtime_state() const;
	void import_runtime_state(const Dictionary &p_state);

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
