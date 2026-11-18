#pragma once

#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "scene/main/node.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/texture.h"

class CanvasLayer;
class ColorRect;

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
	Dictionary title_menu_config;
	Dictionary game_menu_config;
	Dictionary credits_config;
	Dictionary game_over_config;
	Dictionary window_styles_config;
	Dictionary hud_config;
	Dictionary ui_designs_config;
	HashMap<String, Vector<Dictionary>> ui_design_undo_stacks;
	HashMap<String, Vector<Dictionary>> ui_design_redo_stacks;
	void _push_ui_undo_snapshot_internal(const String &p_screen_id, const Dictionary &p_snapshot, bool p_clear_redo = true);
	Dictionary player_options;
	bool player_options_loaded = false;
	String player_options_path_override;
	String save_directory_override;
	String pending_loaded_map_path;
	double applied_music_volume = 0.8;
	double applied_sfx_volume = 0.8;
	double applied_brightness = 1.0;
	bool applied_fullscreen = false;
	String applied_window_title;
	Dictionary switches_names;
	Dictionary variables_names;
	Dictionary runtime_switches;
	Dictionary runtime_variables;
	Dictionary runtime_inventory;
	Dictionary runtime_equips;
	Dictionary runtime_arpg_equips;
	Dictionary runtime_actor_states;
	Dictionary runtime_defeated_enemies;
	Dictionary runtime_window_style_overrides;
	double runtime_playtime_sec = 0.0;
	int runtime_gold = 0;
	bool runtime_game_menu_disabled = false;
	bool runtime_hud_visible = true;
	Dictionary runtime_hud_widget_visibility;
	float runtime_fade_alpha = 0.0f;
	Array pending_post_transfer_commands;
	ObjectID persistent_fade_layer_id;
	ObjectID persistent_fade_rect_id;
	bool dirty = false;
	Node *active_editor_map = nullptr;
	mutable HashMap<String, Ref<Texture2D>> entry_icon_cache;
	mutable Vector<String> last_repaired_json_files;

	Variant _normalize_json_numbers(const Variant &p_val) const;
	bool _write_json_atomic(const String &p_file_path, const Variant &p_data) const;
	bool _read_json_with_backup(const String &p_file_path, Variant &r_data, bool p_repair_mojibake = true) const;
	void _normalize_system_and_vocabulary_config();
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

	static bool parse_tile_coords_variant(const Variant &p_var, Vector2i &r_coords);
	static Vector2i parse_tile_block_variant(const Variant &p_var);
	Ref<Texture2D> get_entry_icon_texture(const Variant &p_entry, int p_target_px = 32, const Ref<Texture2D> &p_fallback = Ref<Texture2D>()) const;
	void clear_entry_icon_cache();

	Dictionary get_system_config() const { return system_config.duplicate(true); }
	void set_system_config(const Dictionary &p_cfg);
	int get_mannequin_size() const;
	void set_mannequin_size(int p_size);
	Dictionary get_exported_system_config() const;

	Dictionary get_vocabulary_config() const { return vocabulary_config.duplicate(true); }
	void set_vocabulary_config(const Dictionary &p_vocab);

	// Datos del Menú Principal (9B Tanda 1)
	static Dictionary get_default_title_menu_config();
	static Dictionary normalize_title_menu_config(const Dictionary &p_cfg);
	Dictionary get_title_menu_config() const { return title_menu_config.duplicate(true); }
	void set_title_menu_config(const Dictionary &p_cfg);
	Array get_visible_title_entries() const;
	int get_save_slot_count() const;
	void set_save_slot_count(int p_count);

	// Menú dentro del juego (9B Tanda 3)
	static Dictionary get_default_game_menu_config();
	static Dictionary normalize_game_menu_config(const Dictionary &p_cfg);
	Dictionary get_game_menu_config() const { return game_menu_config.duplicate(true); }
	void set_game_menu_config(const Dictionary &p_cfg);
	Array get_visible_game_menu_entries() const;
	Dictionary get_game_menu_screen_background(const String &p_screen_id) const;

	// Créditos (9B Tanda 4)
	static Dictionary get_default_credits_config();
	static Dictionary normalize_credits_config(const Dictionary &p_cfg);
	Dictionary get_credits_config() const { return credits_config.duplicate(true); }
	void set_credits_config(const Dictionary &p_cfg);
	bool has_credits_lines() const;

	// Game Over (9B Punto 8)
	static Dictionary get_default_game_over_config();
	static Dictionary normalize_game_over_config(const Dictionary &p_cfg);
	Dictionary get_game_over_config() const { return game_over_config.duplicate(true); }
	void set_game_over_config(const Dictionary &p_cfg);
	bool is_game_menu_disabled() const { return runtime_game_menu_disabled; }
	void set_game_menu_disabled(bool p_disabled) { runtime_game_menu_disabled = p_disabled; }
	void set_runtime_window_style_override(const String &p_window_type_id, const String &p_style_id);
	void clear_runtime_window_style_overrides();
	Dictionary get_runtime_window_style_overrides() const { return runtime_window_style_overrides.duplicate(true); }

	// HUD y Widgets (9B Tanda 5)
	static Array get_hud_anchor_ids();
	static Dictionary get_default_hud_config();
	static Dictionary make_default_hud_widget(const String &p_type = "bar");
	static Dictionary normalize_hud_widget(const Dictionary &p_widget, int p_index_hint = 1);
	static Dictionary normalize_hud_config(const Dictionary &p_cfg);
	Dictionary get_hud_config() const { return hud_config.duplicate(true); }
	void set_hud_config(const Dictionary &p_cfg);
	Array get_hud_widgets() const;
	Dictionary get_hud_widget(const String &p_widget_id) const;
	String create_hud_widget(const Dictionary &p_widget);
	bool update_hud_widget(const String &p_widget_id, const Dictionary &p_widget);
	bool delete_hud_widget(const String &p_widget_id);
	String duplicate_hud_widget(const String &p_source_id, const String &p_new_name = "");
	bool move_hud_widget(const String &p_widget_id, int p_delta);
	bool is_hud_runtime_visible() const { return runtime_hud_visible; }
	void set_hud_runtime_visible(bool p_visible);
	void set_hud_widget_runtime_visible(const String &p_widget_name_or_id, bool p_visible);
	void clear_hud_widget_runtime_visibility(const String &p_widget_name_or_id = "");
	bool is_hud_widget_runtime_visible(const String &p_widget_id, const String &p_widget_name = "") const;
	Dictionary get_hud_runtime_visibility_state() const;

	// Editor Visual de Interfaz: Diseños por pantalla, enlaces de datos, acciones y Undo/Redo (9B Tanda 6A)
	static constexpr int MAX_UI_DESIGN_UNDO_STEPS = 100;
	static Array get_ui_design_screens();
	static PackedStringArray get_ui_design_screen_ids();
	static PackedStringArray get_ui_element_types();
	static Array get_ui_data_bindings();
	static Array get_ui_button_actions(const String &p_screen_id);
	static Dictionary make_default_ui_element(const String &p_type = "text", const String &p_screen_id = "title");
	static Dictionary normalize_ui_element(const Dictionary &p_elem, int p_index_hint = 1, const String &p_screen_id = "title");
	static Dictionary get_default_ui_screen_design(const String &p_screen_id);
	static Dictionary normalize_ui_screen_design(const String &p_screen_id, const Dictionary &p_design);
	static Dictionary get_default_ui_designs_config();
	static Dictionary normalize_ui_designs_config(const Dictionary &p_cfg);
	Dictionary get_default_ui_screen_template(const String &p_screen_id) const;

	bool has_custom_ui_design(const String &p_screen_id) const;
	Dictionary get_ui_screen_design(const String &p_screen_id) const;
	void set_ui_screen_design(const String &p_screen_id, const Dictionary &p_design, bool p_record_undo = true);
	Array get_ui_screen_elements(const String &p_screen_id) const;
	Dictionary get_ui_element(const String &p_screen_id, const String &p_elem_id) const;
	String add_ui_element(const String &p_screen_id, const Dictionary &p_elem, bool p_record_undo = true);
	bool update_ui_element(const String &p_screen_id, const String &p_elem_id, const Dictionary &p_elem, bool p_record_undo = true);
	bool delete_ui_element(const String &p_screen_id, const String &p_elem_id, bool p_record_undo = true);
	bool move_ui_element_layer(const String &p_screen_id, const String &p_elem_id, int p_delta, bool p_record_undo = true);
	void reset_ui_screen_design_to_default(const String &p_screen_id, bool p_record_undo = true);
	Dictionary get_ui_designs_config() const;
	void set_ui_designs_config(const Dictionary &p_cfg);

	String resolve_ui_data_binding(const String &p_binding, const Dictionary &p_element = Dictionary()) const;

	void push_ui_design_undo_state(const String &p_screen_id, const Dictionary &p_snapshot = Dictionary());
	bool can_undo_ui_design(const String &p_screen_id) const;
	bool can_redo_ui_design(const String &p_screen_id) const;
	int get_ui_design_undo_count(const String &p_screen_id) const;
	int get_ui_design_redo_count(const String &p_screen_id) const;
	bool undo_ui_design(const String &p_screen_id);
	bool redo_ui_design(const String &p_screen_id);
	void clear_ui_design_history(const String &p_screen_id = "");

	// Estilos de ventana y botones (9B Tanda 2)
	static void ensure_default_window_types_registered();
	static void register_window_type(const String &p_id, const String &p_label);
	static Array get_registered_window_types();
	static Dictionary get_default_window_styles_config();
	static Dictionary normalize_window_style(const Dictionary &p_style);
	static Dictionary normalize_window_styles_config(const Dictionary &p_cfg);
	Dictionary get_window_styles_config() const { return window_styles_config.duplicate(true); }
	void set_window_styles_config(const Dictionary &p_cfg);
	Array get_window_styles() const;
	Dictionary get_window_style(const String &p_style_id) const;
	String create_window_style(const Dictionary &p_style);
	String duplicate_window_style(const String &p_source_id, const String &p_new_name = "");
	bool update_window_style(const String &p_style_id, const Dictionary &p_style);
	bool delete_window_style(const String &p_style_id);
	String get_default_window_style_id() const;
	bool set_default_window_style_id(const String &p_style_id);
	String get_window_type_assignment(const String &p_window_type_id) const;
	bool set_window_type_assignment(const String &p_window_type_id, const String &p_style_id);
	String resolve_window_style_id(const String &p_window_type_id) const;
	Dictionary resolve_window_style(const String &p_window_type_id) const;

	// Título del proyecto y Unidad Monetaria (fuente única de verdad)
	String get_effective_game_title() const;
	void apply_game_window_title(Node *p_context = nullptr);
	String get_applied_window_title() const { return applied_window_title; }
	String get_currency_unit() const;
	void set_currency_unit(const String &p_unit);

	// Opciones del jugador (archivo separado en user://)
	static Dictionary get_default_player_options();
	static Dictionary normalize_player_options(const Dictionary &p_opts);
	void set_player_options_path(const String &p_path);
	String get_player_options_path() const;
	Dictionary get_player_options() const { return player_options.duplicate(true); }
	void set_player_options(const Dictionary &p_opts, bool p_save_and_apply = true);
	bool load_player_options();
	bool save_player_options() const;
	void ensure_player_options_loaded();
	void apply_player_options(Node *p_context = nullptr);
	Dictionary get_applied_player_options_state() const;

	// Guardado y carga de N ranuras (separadas de opciones)
	void set_save_directory(const String &p_dir);
	String get_save_directory() const;
	String get_save_slot_path(int p_slot) const;
	bool save_game_slot(int p_slot, const Dictionary &p_context = Dictionary());
	bool load_game_slot(int p_slot);
	bool start_game_from_save(int p_slot, Node *p_context = nullptr);
	Dictionary get_save_slot_info(int p_slot) const;
	Array list_save_slots() const;
	bool has_any_save() const;
	int get_latest_save_slot() const;
	bool delete_save_slot(int p_slot);
	String get_pending_loaded_map_path() const { return pending_loaded_map_path; }
	static String format_playtime_hms(double p_seconds);

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
	int get_default_max_item_stack() const;
	void set_default_max_item_stack(int p_max_stack);
	int get_item_max_stack(const String &p_item_key) const;
	String get_record_inventory_key(int p_category, int p_id) const;
	Dictionary find_inventory_record(const String &p_item_key) const;
	bool is_key_item(const String &p_item_key) const;
	bool can_discard_item(const String &p_item_key) const;
	bool discard_item(const String &p_item_key, int p_amount = 1);
	Array get_inventory_entries() const;
	bool use_item_on_actor(const String &p_item_key, int p_target_actor_id);
	Array get_party_actor_ids() const;
	int get_gold() const { return runtime_gold; }
	void set_gold(int p_gold);
	void change_gold(int p_amount);
	void reset_runtime_state();

	// Utilidades UTF-8 y reparación automática de mojibake en JSON (9B-2 Punto 5)
	static bool is_likely_latin1_mojibake(const String &p_text);
	static String repair_mojibake_string(const String &p_text, bool *r_repaired = nullptr);
	static String repair_mojibake_text(const String &p_text) { return repair_mojibake_string(p_text, nullptr); }
	static Variant repair_mojibake_variant(const Variant &p_val, int &r_repaired_strings);
	static String truncate_with_ellipsis(const String &p_text, int p_max_chars);
	PackedStringArray get_last_repaired_json_files() const;
	void clear_last_repaired_json_files() { last_repaired_json_files.clear(); }

	// Fundido persistente entre mapas y cola de comandos post-transferencia (9B Punto 4)
	float get_runtime_fade_alpha() const { return runtime_fade_alpha; }
	void set_runtime_fade_alpha(float p_alpha, Node *p_context = nullptr);
	ColorRect *ensure_persistent_fade_rect(Node *p_context, CanvasLayer **r_layer = nullptr);
	CanvasLayer *get_persistent_fade_layer() const;
	void set_pending_post_transfer_commands(const Array &p_cmds);
	bool has_pending_post_transfer_commands() const { return !pending_post_transfer_commands.is_empty(); }
	Array take_pending_post_transfer_commands();

	// Estado persistente de actor, combate ARPG, drops y respawn (Tanda 3C-1)
	static float evaluate_traits_element_rate(const Array &p_traits, int p_element_id, const String &p_element_name = String());
	static int resolve_traits_attack_element_id(const Array &p_traits, String &r_element_key);
	int get_actor_arpg_attack_element_id(int p_actor_id, String &r_element_key) const;
	float get_actor_element_rate(int p_actor_id, int p_element_id, const String &p_element_key = String()) const;
	static int compute_arpg_hit_damage(const Dictionary &p_attacker, const Dictionary &p_defender, float p_elem_mult, double p_hitbox_mult_pct, double p_hurtbox_mult_pct, bool &r_is_crit);
	static void normalize_arpg_system_config(Dictionary &r_cfg);
	Dictionary get_actor_runtime_state(int p_actor_id);
	void set_actor_runtime_hp(int p_actor_id, int p_hp);
	int get_actor_runtime_hp(int p_actor_id);
	int get_actor_runtime_max_hp(int p_actor_id);
	void set_actor_runtime_mp(int p_actor_id, int p_mp);
	int get_actor_runtime_mp(int p_actor_id);
	int get_actor_runtime_max_mp(int p_actor_id);
	void add_actor_runtime_state(int p_actor_id, int p_state_id);
	void remove_actor_runtime_state(int p_actor_id, int p_state_id);
	Array get_actor_runtime_states(int p_actor_id);
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

	// Equipo normal y ARPG por actor (Tanda 1C & 9B Tanda 3)
	static Array get_default_arpg_equip_slots();
	static Array normalize_arpg_equip_slots(const Array &p_raw_slots);
	Array get_actor_arpg_equip_slots(int p_actor_id) const;
	bool set_actor_arpg_equip(int p_actor_id, const String &p_slot_id, int p_piece_id);
	int get_actor_arpg_equip(int p_actor_id, const String &p_slot_id) const;
	Dictionary get_actor_arpg_equip_bonuses(int p_actor_id) const;
	bool is_arpg_mode_active(int p_mode_override = -1) const;
	Array get_actor_equip_slots(int p_actor_id, int p_mode_override = -1) const;
	bool is_actor_equip_slot_locked(int p_actor_id, int p_slot_idx, int p_mode_override = -1) const;
	bool is_actor_equip_slot_sealed(int p_actor_id, int p_slot_idx, int p_mode_override = -1) const;
	bool can_actor_equip_piece(int p_actor_id, int p_slot_idx, int p_category, int p_piece_id, int p_mode_override = -1) const;
	Array get_equippable_pieces_for_slot(int p_actor_id, int p_slot_idx, int p_mode_override = -1) const;
	bool equip_actor_piece(int p_actor_id, int p_slot_idx, const String &p_inventory_key, int p_mode_override = -1);
	bool unequip_actor_slot(int p_actor_id, int p_slot_idx, int p_mode_override = -1);
	Dictionary preview_equip_actor_stats(int p_actor_id, int p_slot_idx, int p_candidate_category, int p_candidate_piece_id, int p_mode_override = -1) const;
	Dictionary get_actor_equip_bonuses(int p_actor_id, int p_mode_override = -1) const;
	Dictionary get_actor_effective_stats(int p_actor_id, int p_mode_override = -1) const;
	Dictionary get_actor_status_sheet(int p_actor_id, int p_mode_override = -1);
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
