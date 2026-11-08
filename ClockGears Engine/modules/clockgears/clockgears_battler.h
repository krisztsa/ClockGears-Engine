#pragma once

#include "clockgears_types.h"
#include "scene/2d/node_2d.h"
#include "core/variant/dictionary.h"

class ClockGearsBattler : public Node2D {
	GDCLASS(ClockGearsBattler, Node2D);

private:
	String battler_name = "Heroe";
	bool is_enemy = false;
	int class_id = 1;
	int level = 1;
	int current_exp = 0;

	// Current resources
	int current_hp = 500;
	int max_hp = 500;
	int current_mp = 50;
	int max_mp = 50;

	// Core combat attributes
	int atk = 30; // Ataque Físico
	int def = 20; // Defensa Física
	int mat = 15; // Ataque Mágico
	int mdf = 15; // Defensa Mágica
	int agi = 25; // Agilidad (determina orden de turnos y velocidad)
	int luk = 15; // Suerte (probabilidad de críticos y evasión)

	// Stat growth curves (Level 1 to Level 99)
	int min_hp = 450, max_curve_hp = 5000;
	int min_mp = 40, max_curve_mp = 800;
	int min_atk = 25, max_curve_atk = 260;
	int min_def = 18, max_curve_def = 210;
	int min_mat = 12, max_curve_mat = 190;
	int min_mdf = 12, max_curve_mdf = 180;
	int min_agi = 20, max_curve_agi = 220;
	int min_luk = 10, max_curve_luk = 150;

	// Elemental affinities: Element ID -> Affinity enum (ClockGearsElementAffinity)
	Dictionary element_affinities;

	// Action RPG Real-Time Combat Support (I-Frames / Invulnerabilidad)
	float invulnerability_duration = 0.4f;
	float invulnerability_timer = 0.0f;
	bool flash_on_damage = true;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	// Identification & Class
	void set_battler_name(const String &p_name) { battler_name = p_name; }
	String get_battler_name() const { return battler_name; }

	void set_is_enemy(bool p_enemy) { is_enemy = p_enemy; }
	bool get_is_enemy() const { return is_enemy; }

	void set_class_id(int p_id) { class_id = p_id; }
	int get_class_id() const { return class_id; }

	// Level & EXP Progression
	void set_level(int p_level);
	int get_level() const { return level; }

	void set_current_exp(int p_exp) { current_exp = p_exp; }
	int get_current_exp() const { return current_exp; }

	int get_exp_for_level(int p_level) const;
	int get_exp_to_next_level() const;
	void gain_exp(int p_amount);
	void recalculate_stats_for_level(int p_level);

	// Core Stats Getters / Setters
	void set_current_hp(int p_hp);
	int get_current_hp() const { return current_hp; }

	void set_max_hp(int p_hp);
	int get_max_hp() const { return max_hp; }

	void set_current_mp(int p_mp);
	int get_current_mp() const { return current_mp; }

	void set_max_mp(int p_mp);
	int get_max_mp() const { return max_mp; }

	void set_atk(int p_val) { atk = MAX(1, p_val); }
	int get_atk() const { return atk; }

	void set_def(int p_val) { def = MAX(1, p_val); }
	int get_def() const { return def; }

	void set_mat(int p_val) { mat = MAX(1, p_val); }
	int get_mat() const { return mat; }

	void set_mdf(int p_val) { mdf = MAX(1, p_val); }
	int get_mdf() const { return mdf; }

	void set_agi(int p_val) { agi = MAX(1, p_val); }
	int get_agi() const { return agi; }

	void set_luk(int p_val) { luk = MAX(1, p_val); }
	int get_luk() const { return luk; }

	// Stat Growth Curve Configuration
	void set_curve_hp(int p_min, int p_max) { min_hp = p_min; max_curve_hp = p_max; }
	void set_curve_mp(int p_min, int p_max) { min_mp = p_min; max_curve_mp = p_max; }
	void set_curve_atk(int p_min, int p_max) { min_atk = p_min; max_curve_atk = p_max; }
	void set_curve_def(int p_min, int p_max) { min_def = p_min; max_curve_def = p_max; }
	void set_curve_mat(int p_min, int p_max) { min_mat = p_min; max_curve_mat = p_max; }
	void set_curve_mdf(int p_min, int p_max) { min_mdf = p_min; max_curve_mdf = p_max; }
	void set_curve_agi(int p_min, int p_max) { min_agi = p_min; max_curve_agi = p_max; }
	void set_curve_luk(int p_min, int p_max) { min_luk = p_min; max_curve_luk = p_max; }

	// Elemental Affinities
	void set_element_affinity(int p_element, int p_affinity);
	int get_element_affinity(int p_element) const;
	float get_element_rate(int p_element) const;
	void set_element_affinities_dict(const Dictionary &p_dict) { element_affinities = p_dict; }
	Dictionary get_element_affinities_dict() const { return element_affinities; }

	// Combat Execution (Agnostic to Turn-Based or Action RPG)
	int take_damage(int p_damage, bool p_critical = false, int p_element = 0, Vector2 p_hit_pos = Vector2());
	void heal_hp(int p_amount);
	void heal_mp(int p_amount);
	bool consume_mp(int p_amount);
	bool is_alive() const { return current_hp > 0; }
	bool is_invulnerable() const { return invulnerability_timer > 0.0f; }

	// Action RPG i-frame tuning
	void set_invulnerability_duration(float p_dur) { invulnerability_duration = p_dur; }
	float get_invulnerability_duration() const { return invulnerability_duration; }

	ClockGearsBattler();
};
