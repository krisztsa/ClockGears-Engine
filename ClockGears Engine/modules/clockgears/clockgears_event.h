#pragma once

#include "clockgears_animation.h"
#include "clockgears_types.h"
#include "core/math/random_pcg.h"
#include "core/variant/array.h"
#include "scene/2d/node_2d.h"
#include "scene/2d/sprite_2d.h"

class CharacterBody2D;
class ClockGearsDetectionZone2D;
class ClockGearsMap;

class ClockGearsEvent : public Node2D {
	GDCLASS(ClockGearsEvent, Node2D);

private:
	int event_id = 1;
	String event_name = "EV001";
	int trigger_type = TRIGGER_ACTION_BUTTON;
	int move_type = MOVE_FIXED;
	float move_speed = 100.0f;
	float move_frequency = 1.0f;
	bool through = false;
	bool direction_fix = false;
	int direction = 2; // Down
	String graphic_path;
	int sprite_hframes = 1;
	int sprite_vframes = 1;
	int sprite_frame = 0;
	int behavior_type = 0; // 0=Normal, 1=NPC, 2=NPC ARPG, 3=Enemigo ARPG
	int behavior_ref_id = 0;
	Array commands;
	Array pages;

	bool condition_switch_1_enabled = false;
	String condition_switch_1_name;
	bool condition_switch_2_enabled = false;
	String condition_switch_2_name;
	bool condition_self_switch_enabled = false;
	String condition_self_switch_letter = "A";
	Dictionary self_switches;
	int active_page_index = 0;
	bool condition_var_enabled = false;
	String condition_var_name;
	int condition_var_value = 0;
	bool condition_item_enabled = false;
	String condition_item_name;
	bool condition_actor_enabled = false;
	String condition_actor_name;
	int priority_type = 1; // 0: Below, 1: Same, 2: Above

	Sprite2D *sprite = nullptr;
	void _update_sprite_region();

	// Runtime Cuerpo, Movimiento IA, A* y Zonas de Detección (behavior_type != 0)
	ClockGearsHitShape body_shape = ClockGearsHitShape::make_circle(12.0f);
	bool body_through = false;
	bool avoid_obstacles = true;
	int path_recalc_ms = 250;
	int path_max_nodes = 256;
	float walk_move_speed = 120.0f;
	float run_move_speed = 180.0f;
	float attack_speed_pct = 100.0f;
	float wander_radius = 3.0f;
	Vector<Vector2> patrol_points;
	bool patrol_loop = false;
	float lose_sight_sec = 4.0f;
	float min_distance = 3.0f;
	float max_distance = 5.0f;
	String ai_type_str = "quieto";
	String npc_move_type_str = "quieto";
	float follow_distance = 2.0f;
	bool face_player_on_event = true;
	Array detection_zones_cfg;

	bool db_record_loaded = false;
	Dictionary cached_db_rec;
	ClockGearsSpriteSheetData cached_sprite_data;
	bool cached_directional_idle = false;
	HashMap<String, Ref<Texture2D>> cached_slot_textures;

	CharacterBody2D *entity_body = nullptr;
	uint32_t entity_body_shape_owner = 0;
	Node2D *zones_root = nullptr;
	Vector<ClockGearsDetectionZone2D *> zone_nodes;
	String cached_zones_sig;
	String cached_body_sig;

	Vector2 spawn_origin_px;
	int spawn_direction = 2;
	bool spawn_origin_set = false;
	int ai_state = 0; // ClockGearsEntityAIState
	ObjectID target_object_id;
	float time_without_sight = 0.0f;
	bool astar_no_path = false;
	Vector<Vector2i> astar_path;
	int astar_path_idx = 0;
	float astar_timer = 0.0f;

	RandomPCG wander_rng;
	Vector2 wander_target_px;
	bool wander_has_target = false;
	float wander_pause_timer = 0.0f;
	int patrol_idx = 0;
	int patrol_dir = 1;

	String ev_action_state = "quieto";
	String ev_resolved_slot;
	float ev_anim_timer = 0.0f;
	int ev_anim_frame = 0;
	float ev_attack_timer = 0.0f;
	bool was_interpreter_running_me = false;
	bool paused_by_event = false;

	// Estado de combate ARPG para Enemigos (behavior_type == 3) - Tanda 3C-1
	bool arpg_stats_initialized = false;
	int cached_behavior_ref_id = 0;
	int arpg_hp = 0;
	int arpg_max_hp = 50;
	int arpg_atk = 10;
	int arpg_def = 0;
	int arpg_exp_reward = 0;
	int arpg_gold_reward = 0;
	float arpg_knockback_resist = 0.0f;
	float knockback_duration = 0.16f;
	bool arpg_dying = false;
	bool arpg_dead = false;
	bool arpg_rewards_granted = false;
	bool saved_body_through_before_death = false;
	float arpg_knockback_timer = 0.0f;
	Vector2 arpg_knockback_velocity;
	float arpg_hitstop_timer = 0.0f;
	float arpg_hp_bar_timer = 0.0f;
	Ref<ClockGearsAttackSwingTracker> ev_swing_tracker;
	int64_t ev_attack_swing_id = 0;

	void _begin_arpg_death();
	void _grant_arpg_kill_rewards();
	void _apply_entity_body_collision_state();
	void _begin_enemy_attack_swing();
	void _end_enemy_attack_swing();
	void _process_enemy_attack_hits(ClockGearsMap *p_map);

	void _apply_behavior_record_config(const Dictionary &p_rec, float p_base_ev_speed);
	void _sync_runtime_entity_components();
	void _clear_runtime_entity_components();
	void _update_behavior_visual_frame();
	void _advance_behavior_animation(float p_delta);
	void _physics_tick_behavior(float p_delta);
	Node *_resolve_target_node(ClockGearsMap *p_map) const;
	bool _has_any_target_in_zones() const;
	Vector2 _compute_move_toward(ClockGearsMap *p_map, const Vector2 &p_target_px, float p_delta, int p_tile_size);
	void _face_toward_world_pos(const Vector2 &p_target_pos);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_event_id(int p_id) { event_id = p_id; }
	int get_event_id() const { return event_id; }

	void set_event_name(const String &p_name) { event_name = p_name; }
	String get_event_name() const { return event_name; }

	void set_trigger_type(int p_trigger) { trigger_type = p_trigger; }
	int get_trigger_type() const { return trigger_type; }

	void set_move_type(int p_move) { move_type = p_move; }
	int get_move_type() const { return move_type; }

	void set_move_speed(float p_speed) { move_speed = p_speed; }
	float get_move_speed() const { return move_speed; }

	void set_move_frequency(float p_freq) { move_frequency = p_freq; }
	float get_move_frequency() const { return move_frequency; }

	void set_through(bool p_through) { through = p_through; }
	bool is_through() const { return through; }

	void set_direction_fix(bool p_fix) { direction_fix = p_fix; }
	bool is_direction_fix() const { return direction_fix; }

	void set_direction(int p_dir);
	int get_direction() const { return direction; }

	void set_graphic_path(const String &p_path);
	String get_graphic_path() const { return graphic_path; }

	void set_sprite_hframes(int p_hframes);
	int get_sprite_hframes() const { return sprite_hframes; }

	void set_sprite_vframes(int p_vframes);
	int get_sprite_vframes() const { return sprite_vframes; }

	void set_sprite_frame(int p_frame);
	int get_sprite_frame() const { return sprite_frame; }

	void set_condition_switch_1_enabled(bool p_en) { condition_switch_1_enabled = p_en; }
	bool is_condition_switch_1_enabled() const { return condition_switch_1_enabled; }
	void set_condition_switch_1_name(const String &p_name) { condition_switch_1_name = p_name; }
	String get_condition_switch_1_name() const { return condition_switch_1_name; }

	void set_condition_switch_2_enabled(bool p_en) { condition_switch_2_enabled = p_en; }
	bool is_condition_switch_2_enabled() const { return condition_switch_2_enabled; }
	void set_condition_switch_2_name(const String &p_name) { condition_switch_2_name = p_name; }
	String get_condition_switch_2_name() const { return condition_switch_2_name; }

	void set_condition_self_switch_enabled(bool p_en) { condition_self_switch_enabled = p_en; }
	bool is_condition_self_switch_enabled() const { return condition_self_switch_enabled; }
	void set_condition_self_switch_letter(const String &p_letter) { condition_self_switch_letter = p_letter; }
	String get_condition_self_switch_letter() const { return condition_self_switch_letter; }

	void set_self_switch(const String &p_letter, bool p_val);
	bool get_self_switch(const String &p_letter) const;
	Dictionary get_self_switches() const { return self_switches; }
	void set_self_switches(const Dictionary &p_sw);

	void set_behavior_type(int p_type);
	int get_behavior_type() const { return behavior_type; }
	void set_behavior_ref_id(int p_id);
	int get_behavior_ref_id() const { return behavior_ref_id; }
	static int behavior_type_to_category(int p_type);
	void _update_behavior_sprite(const Dictionary &p_rec, bool p_found);

	void set_body_shape(const Dictionary &p_shape);
	Dictionary get_body_shape() const { return body_shape.to_dict(); }
	const ClockGearsHitShape &get_body_shape_struct() const { return body_shape; }
	void set_body_through(bool p_through);
	bool is_body_through() const { return body_through; }
	void set_avoid_obstacles(bool p_avoid) { avoid_obstacles = p_avoid; }
	bool is_avoid_obstacles() const { return avoid_obstacles; }

	Array get_detection_zones() const { return detection_zones_cfg; }
	int get_zone_count() const { return zone_nodes.size(); }
	ClockGearsDetectionZone2D *get_zone_node(int p_idx) const;
	CharacterBody2D *get_entity_body() const { return entity_body; }

	void reset_spawn_origin();
	Vector2 get_spawn_origin() const { return spawn_origin_px; }
	int get_ai_state() const { return ai_state; }
	String get_ai_state_name() const;
	String get_action_state() const { return ev_action_state; }
	String get_active_visual_slot() const { return ev_resolved_slot; }
	bool is_paused_by_event() const { return paused_by_event; }
	Array get_current_astar_path() const;
	bool has_astar_no_path() const { return astar_no_path; }

	void notify_zone_sight(bool p_in, Node *p_target);
	void execute_zone_actions(const Array &p_actions, Node *p_target);

	// Combate ARPG (Tanda 3C-1)
	int get_arpg_hp() const { return arpg_hp; }
	int get_arpg_max_hp() const { return arpg_max_hp; }
	int get_arpg_atk() const { return arpg_atk; }
	int get_arpg_def() const { return arpg_def; }
	float get_arpg_knockback_resist() const { return arpg_knockback_resist; }
	void set_knockback_duration(float p_dur) { knockback_duration = MAX(0.02f, p_dur); }
	float get_knockback_duration() const { return knockback_duration; }
	bool is_arpg_dying() const { return arpg_dying; }
	bool is_arpg_dead() const { return arpg_dead; }
	bool is_arpg_alive() const { return behavior_type == 3 && !arpg_dying && !arpg_dead && is_visible(); }
	bool is_arpg_hp_bar_visible() const { return behavior_type == 3 && !arpg_dying && !arpg_dead && is_visible() && arpg_hp_bar_timer > 0.0f; }
	float get_arpg_hp_bar_remaining() const { return arpg_hp_bar_timer; }
	float get_arpg_hitstop_remaining() const { return arpg_hitstop_timer; }
	float get_arpg_knockback_remaining() const { return arpg_knockback_timer; }
	Vector2 get_arpg_world_center() const;
	Dictionary get_arpg_effective_hurtbox() const;
	Dictionary get_arpg_enemy_record() const { return cached_db_rec; }
	int get_arpg_anim_frame() const { return ev_anim_frame; }
	bool take_arpg_hit(int p_damage, float p_knockback_force, const Vector2 &p_hit_dir, int p_hitstop_ms, bool p_is_crit = false);
	void start_arpg_attack(Node *p_target = nullptr);
	void sync_arpg_defeated_state();
	void respawn_arpg_enemy();

	int get_active_page_index() const { return active_page_index; }
	void update_active_page();

	void set_condition_var_enabled(bool p_en) { condition_var_enabled = p_en; }
	bool is_condition_var_enabled() const { return condition_var_enabled; }
	void set_condition_var_name(const String &p_name) { condition_var_name = p_name; }
	String get_condition_var_name() const { return condition_var_name; }
	void set_condition_var_value(int p_val) { condition_var_value = p_val; }
	int get_condition_var_value() const { return condition_var_value; }

	void set_condition_item_enabled(bool p_en) { condition_item_enabled = p_en; }
	bool is_condition_item_enabled() const { return condition_item_enabled; }
	void set_condition_item_name(const String &p_name) { condition_item_name = p_name; }
	String get_condition_item_name() const { return condition_item_name; }

	void set_condition_actor_enabled(bool p_en) { condition_actor_enabled = p_en; }
	bool is_condition_actor_enabled() const { return condition_actor_enabled; }
	void set_condition_actor_name(const String &p_name) { condition_actor_name = p_name; }
	String get_condition_actor_name() const { return condition_actor_name; }

	void set_priority_type(int p_priority) { priority_type = p_priority; }
	int get_priority_type() const { return priority_type; }

	void set_commands(const Array &p_commands) { commands = p_commands; }
	Array get_commands() const { return commands; }

	void set_pages(const Array &p_pages) { pages = p_pages; }
	Array get_pages() const { return pages; }

	void add_command(const Dictionary &p_cmd);
	void clear_commands() { commands.clear(); }

	void serialize_to_meta();
	void deserialize_from_meta();

	void execute();

	ClockGearsEvent();
};
