#pragma once

#include "clockgears_types.h"
#include "clockgears_battler.h"
#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"

class ClockGearsDamageEngine : public RefCounted {
	GDCLASS(ClockGearsDamageEngine, RefCounted);

public:
	struct DamageResult {
		int damage = 0;
		bool is_critical = false;
		int element = 0;
		float element_rate = 1.0f;
		bool is_immune = false;
		bool is_weak = false;
		bool is_absorbed = false;
	};

protected:
	static void _bind_methods();

public:
	static Dictionary evaluate_damage(
			ClockGearsBattler *p_attacker,
			ClockGearsBattler *p_target,
			const String &p_formula,
			int p_element = 0,
			float p_variance_pct = 20.0f,
			float p_crit_chance = 0.05f,
			float p_crit_mult = 2.0f);

	static String sanitize_formula(const String &p_formula);

	static String get_element_name(int p_element);
	static Color get_element_color(int p_element);
	static String get_affinity_name(int p_affinity);
	static float get_affinity_multiplier(int p_affinity);

	ClockGearsDamageEngine();
};
