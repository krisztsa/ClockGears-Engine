#pragma once

#include "clockgears_types.h"
#include "clockgears_animation.h"
#include "core/clockgears_formulas.h"
#include "core/clockgears_data_manager.h"
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

	bool is_defending = false;
	bool is_dead = false;
	int64_t action_speed = 0;

	String graphic_path;
	Ref<ClockGearsAnimationTriggerTable> trigger_table;

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

	ClockGearsSpriteSheetData sprite_data;
	void set_sprite_data(const ClockGearsSpriteSheetData &p_data) { sprite_data = p_data; }
	ClockGearsSpriteSheetData get_sprite_data() const { return sprite_data; }
	void set_sprite_data_dict(const Dictionary &p_d) { sprite_data.from_dict(p_d); }
	Dictionary get_sprite_data_dict() const { return sprite_data.to_dict(); }

	void set_trigger_table(const Ref<ClockGearsAnimationTriggerTable> &p_table) { trigger_table = p_table; }
	Ref<ClockGearsAnimationTriggerTable> get_trigger_table() const { return trigger_table; }

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
	String battle_bgm;

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

	void _log_hit(const String &p_attacker, int p_a_atk, const String &p_target, int p_b_def, const String &p_action, const String &p_formula, int64_t p_damage, bool p_crit, bool p_hit);

protected:
	static void _bind_methods();

public:
	String get_last_hit_log() const { return last_hit_log; }
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
