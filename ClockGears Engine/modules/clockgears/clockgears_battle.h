#pragma once

#include "clockgears_types.h"
#include "clockgears_animation.h"
#include "core/clockgears_formulas.h"
#include "core/clockgears_data_manager.h"
#include "core/clockgears_system_catalog.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

// ============================================================================
// Unidad de Combate (Battler para Personaje o Enemigo en Fase 8A)
// ============================================================================
class ClockGearsBattleUnit : public RefCounted {
	GDCLASS(ClockGearsBattleUnit, RefCounted);

private:
	int id = 1;
	String name = "Heroe";
	bool is_actor = true;
	int level = 1;
	int exp = 0;

	// Recursos actuales
	int hp = 500;
	int max_hp = 500;
	int mp = 50;
	int max_mp = 50;

	// Atributos de combate
	int atk = 30;
	int def = 20;
	int mat = 15;
	int mdf = 15;
	int agi = 25;
	int luk = 15;

	// Recompensas al morir (Enemigos)
	int exp_reward = 0;
	int gold_reward = 0;
	Array drops; // Hasta 3 drops: [ {kind, item_id, prob}, ... ]

	// Estados activos: [ {id, name, turns, restriction}, ... ]
	Array states;
	Array traits;
	Array actions;
	Array cast_animations;

	bool is_defending = false;
	bool is_dead = false;
	int64_t action_speed = 0;

	String graphic_path;
	Ref<ClockGearsAnimationTriggerTable> trigger_table;
	String current_anim_slot = "idle";
	String last_played_anim_slot = "idle";
	float anim_slot_timer = 0.0f;
	int active_effect_anim_id = 0;
	float active_effect_timer = 0.0f;

protected:
	static void _bind_methods();

public:
	// Identificación
	void set_id(int p_id) { id = p_id; }
	int get_id() const { return id; }

	void set_name(const String &p_name) { name = p_name; }
	String get_name() const { return name; }

	void set_is_actor(bool p_actor) { is_actor = p_actor; }
	bool get_is_actor() const { return is_actor; }

	void set_level(int p_level) { level = MAX(1, p_level); }
	int get_level() const { return level; }

	void set_exp(int p_exp) { exp = MAX(0, p_exp); }
	int get_exp() const { return exp; }

	// Recursos
	void set_hp(int p_val) {
		hp = CLAMP(p_val, 0, max_hp);
		if (hp <= 0) is_dead = true;
		else is_dead = false;
	}
	int get_hp() const { return hp; }

	void set_max_hp(int p_val) {
		max_hp = MAX(1, p_val);
		hp = CLAMP(hp, 0, max_hp);
	}
	int get_max_hp() const { return max_hp; }

	void set_mp(int p_val) { mp = CLAMP(p_val, 0, max_mp); }
	int get_mp() const { return mp; }

	void set_max_mp(int p_val) {
		max_mp = MAX(0, p_val);
		mp = CLAMP(mp, 0, max_mp);
	}
	int get_max_mp() const { return max_mp; }

	// Stats
	void set_atk(int p_val) { atk = MAX(1, p_val); }
	int get_atk() const { return atk; }

	void set_def(int p_val) { def = MAX(0, p_val); }
	int get_def() const { return def; }

	void set_mat(int p_val) { mat = MAX(1, p_val); }
	int get_mat() const { return mat; }

	void set_mdf(int p_val) { mdf = MAX(0, p_val); }
	int get_mdf() const { return mdf; }

	void set_agi(int p_val) { agi = MAX(1, p_val); }
	int get_agi() const { return agi; }

	void set_luk(int p_val) { luk = MAX(1, p_val); }
	int get_luk() const { return luk; }

	// Recompensas
	void set_exp_reward(int p_exp) { exp_reward = MAX(0, p_exp); }
	int get_exp_reward() const { return exp_reward; }

	void set_gold_reward(int p_gold) { gold_reward = MAX(0, p_gold); }
	int get_gold_reward() const { return gold_reward; }

	void set_drops(const Array &p_drops) { drops = p_drops; }
	Array get_drops() const { return drops; }

	// Estados
	void set_states(const Array &p_states) { states = p_states; }
	Array get_states() const { return states; }

	void set_traits(const Array &p_traits) { traits = p_traits; }
	Array get_traits() const { return traits; }
	float get_element_rate(int p_element_id, const String &p_element_name = "") const;
	int get_attack_element_id() const;

	void set_actions(const Array &p_actions) { actions = p_actions; }
	Array get_actions() const { return actions; }

	void set_cast_animations(const Array &p_casts) { cast_animations = p_casts; }
	Array get_cast_animations() const { return cast_animations; }

	String resolve_slot_animation_name(const String &p_slot_type, int p_element_id = 0, const String &p_element_name = "") const;
	void play_slot_animation(const String &p_slot_name, float p_duration = 0.5f);
	String get_current_anim_slot() const { return current_anim_slot; }
	void set_current_anim_slot(const String &p_slot) { current_anim_slot = p_slot; }
	String get_last_played_anim_slot() const { return last_played_anim_slot; }
	float get_anim_slot_timer() const { return anim_slot_timer; }
	void set_anim_slot_timer(float p_t) { anim_slot_timer = MAX(0.0f, p_t); }

	void trigger_target_effect_anim(int p_anim_id, float p_duration = 0.5f) {
		active_effect_anim_id = p_anim_id;
		active_effect_timer = p_duration;
	}
	int get_active_effect_anim_id() const { return active_effect_anim_id; }
	float get_active_effect_timer() const { return active_effect_timer; }
	void set_active_effect_timer(float p_t) {
		active_effect_timer = MAX(0.0f, p_t);
		if (active_effect_timer <= 0.0f) {
			active_effect_anim_id = 0;
		}
	}

	bool has_state(int p_state_id) const;
	void apply_state(int p_state_id, const String &p_name, int p_turns, int p_restriction = 0);
	void remove_state(int p_state_id);

	// Estado en batalla
	void set_is_defending(bool p_def) { is_defending = p_def; }
	bool get_is_defending() const { return is_defending; }

	void set_is_dead(bool p_dead) {
		is_dead = p_dead;
		if (is_dead) hp = 0;
	}
	bool get_is_dead() const { return is_dead; }

	void set_action_speed(int64_t p_speed) { action_speed = p_speed; }
	int64_t get_action_speed() const { return action_speed; }

	void set_graphic_path(const String &p_path) { graphic_path = p_path; }
	String get_graphic_path() const { return graphic_path; }

	Vector2 battle_pos;
	void set_battle_pos(const Vector2 &p_pos) { battle_pos = p_pos; }
	Vector2 get_battle_pos() const { return battle_pos; }

	int sprite_scale = 1;
	void set_sprite_scale(int p_scale) { sprite_scale = CLAMP(p_scale, 1, 6); }
	int get_sprite_scale() const { return sprite_scale; }

	static constexpr float FREE_HP_BAR_WIDTH = 110.0f;
	static Vector2i clamp_free_hp_bar_pos(const Vector2i &p_pos, float p_ref_w = 960.0f, float p_ref_h = 540.0f) {
		float half_w = FREE_HP_BAR_WIDTH * 0.5f;
		float ref_lbl_h = float(MAX(9, int(13.0f))) + 3.0f;
		float ref_hp_h = MAX(6.0f, 10.0f);
		float ref_grp_h = ref_lbl_h + 5.0f + ref_hp_h;
		int min_x = int(Math::ceil(half_w));
		int max_x = MAX(min_x, int(Math::floor(p_ref_w - half_w)));
		int min_y = 0;
		int max_y = MAX(min_y, int(Math::floor(p_ref_h - ref_grp_h)));
		return Vector2i(CLAMP(p_pos.x, min_x, max_x), CLAMP(p_pos.y, min_y, max_y));
	}

	String hp_bar_mode = "abajo";
	Vector2i hp_bar_pos = Vector2i(-1, -1);
	static String normalize_hp_bar_mode(const String &p_mode) {
		String clean = p_mode.strip_edges().to_lower();
		if (clean == "arriba" || clean == "abajo" || clean == "libre" || clean == "oculta") {
			return clean;
		}
		return "abajo";
	}
	void set_hp_bar_mode(const String &p_mode) { hp_bar_mode = normalize_hp_bar_mode(p_mode); }
	String get_hp_bar_mode() const { return hp_bar_mode; }
	void set_hp_bar_pos(const Vector2i &p_pos) { hp_bar_pos = p_pos; }
	Vector2i get_hp_bar_pos() const { return hp_bar_pos; }

	ClockGearsSpriteSheetData sprite_data;
	void set_sprite_data(const ClockGearsSpriteSheetData &p_data) { sprite_data = p_data; }
	ClockGearsSpriteSheetData get_sprite_data() const { return sprite_data; }
	void set_sprite_data_dict(const Dictionary &p_d) { sprite_data.from_dict(p_d); }
	Dictionary get_sprite_data_dict() const { return sprite_data.to_dict(); }

	void set_trigger_table(const Ref<ClockGearsAnimationTriggerTable> &p_table) { trigger_table = p_table; }
	Ref<ClockGearsAnimationTriggerTable> get_trigger_table() const { return trigger_table; }

	String get_combat_animation(int p_trigger, const String &p_fallback = "") const {
		if (trigger_table.is_valid() && trigger_table->has_trigger(p_trigger)) {
			String a = trigger_table->get_animation(p_trigger);
			if (!a.is_empty()) return a;
		}
		return p_fallback;
	}

	// Conversión para ClockGearsFormulas
	Dictionary to_formula_dict() const;

	ClockGearsBattleUnit();
};

// ============================================================================
// Controlador del Combate por Turnos (ClockGearsBattleController)
// ============================================================================
class ClockGearsBattleController : public RefCounted {
	GDCLASS(ClockGearsBattleController, RefCounted);

public:
	enum BattleResult {
		RESULT_NONE = 0,
		RESULT_VICTORY = 1,
		RESULT_DEFEAT = 2,
		RESULT_ESCAPED = 3,
		RESULT_GAME_OVER = 4
	};

	enum BattlePhase {
		PHASE_INIT = 0,
		PHASE_TURN_INPUT = 1,
		PHASE_ACTION_EXECUTION = 2,
		PHASE_FINISHED = 3
	};

private:
	int troop_id = 1;
	bool can_escape = true;
	bool can_lose = false;
	bool test_mode = false;
	String battle_bgm;
	String troop_bg_path;
	String troop_floor_path;
	Dictionary troop_bg_cfg;
	Dictionary troop_floor_cfg;
	Dictionary test_consumed_items;

	Vector<Ref<ClockGearsBattleUnit>> party;
	Vector<Ref<ClockGearsBattleUnit>> enemies;
	Vector<Ref<ClockGearsBattleUnit>> turn_queue;

	int current_queue_idx = 0;
	int round_counter = 1;

	BattleResult battle_result = RESULT_NONE;
	BattlePhase phase = PHASE_INIT;

	// Recompensas acumuladas
	int total_exp_reward = 0;
	int total_gold_reward = 0;
	Array dropped_items;
	Array level_up_messages;
	PackedStringArray log_history;
	String last_hit_log;
	float last_element_multiplier = 1.0f;

	void _log_hit(const String &p_attacker, int p_a_atk, const String &p_target, int p_b_def, const String &p_action, const String &p_formula, int64_t p_damage, bool p_crit, bool p_hit, const String &p_elem_info = "");

protected:
	static void _bind_methods();

public:
	String get_last_hit_log() const { return last_hit_log; }
	float get_last_element_multiplier() const { return last_element_multiplier; }
	String get_troop_bg_path() const { return troop_bg_path; }
	void set_troop_bg_path(const String &p_path) { troop_bg_path = p_path; }
	String get_troop_floor_path() const { return troop_floor_path; }
	void set_troop_floor_path(const String &p_path) { troop_floor_path = p_path; }
	Dictionary get_troop_bg_cfg() const { return troop_bg_cfg; }
	void set_troop_bg_cfg(const Dictionary &p_cfg) { troop_bg_cfg = p_cfg; }
	Dictionary get_troop_floor_cfg() const { return troop_floor_cfg; }
	void set_troop_floor_cfg(const Dictionary &p_cfg) { troop_floor_cfg = p_cfg; }
	void set_test_mode(bool p_test) { test_mode = p_test; }
	bool get_test_mode() const { return test_mode; }
	// Configuración de la batalla
	void setup_battle(int p_troop_id, bool p_can_escape, bool p_can_lose, const String &p_bgm = "");
	void add_party_unit(const Ref<ClockGearsBattleUnit> &p_unit);
	void add_enemy_unit(const Ref<ClockGearsBattleUnit> &p_unit);

	Array get_party_units() const;
	Array get_enemy_units() const;

	// Iniciativa y orden de turnos
	void calculate_initiative();
	Array get_turn_order() const;
	Ref<ClockGearsBattleUnit> get_active_unit() const;
	int get_round() const { return round_counter; }

	// Comandos de acción del jugador
	bool action_attack(int p_target_idx);
	bool action_skill(int p_skill_id, int p_target_idx);
	bool action_item(const String &p_item_name, int p_target_idx);
	bool action_defend();
	bool action_flee();

	// Ejecución de turno de enemigo
	void execute_enemy_turn(const Ref<ClockGearsBattleUnit> &p_enemy);

	// Progresión de turnos
	void start_turn_cycle();
	bool advance_turn();
	void tick_states(const Ref<ClockGearsBattleUnit> &p_unit);

	// Comprobación de finalización
	bool check_battle_end();
	bool is_all_enemies_dead() const;
	bool is_all_actors_dead() const;
	void process_victory_rewards();

	// Getters de estado y recompensas
	int get_result() const { return int(battle_result); }
	bool is_finished() const { return battle_result != RESULT_NONE; }
	int get_total_exp() const { return total_exp_reward; }
	int get_total_gold() const { return total_gold_reward; }
	Array get_dropped_items() const { return dropped_items; }
	Array get_level_up_messages() const { return level_up_messages; }
	PackedStringArray get_log_history() const { return log_history; }

	ClockGearsBattleController();
	~ClockGearsBattleController();
};
