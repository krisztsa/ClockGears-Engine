#include "clockgears_damage_engine.h"
#include "core/math/expression.h"
#include "core/math/math_funcs.h"

void ClockGearsDamageEngine::_bind_methods() {
	ClassDB::bind_static_method("ClockGearsDamageEngine", D_METHOD("evaluate_damage", "attacker", "target", "formula", "element", "variance_pct", "crit_chance", "crit_mult"), &ClockGearsDamageEngine::evaluate_damage, DEFVAL(0), DEFVAL(20.0f), DEFVAL(0.05f), DEFVAL(2.0f));
	ClassDB::bind_static_method("ClockGearsDamageEngine", D_METHOD("sanitize_formula", "formula"), &ClockGearsDamageEngine::sanitize_formula);
	ClassDB::bind_static_method("ClockGearsDamageEngine", D_METHOD("get_element_name", "element"), &ClockGearsDamageEngine::get_element_name);
	ClassDB::bind_static_method("ClockGearsDamageEngine", D_METHOD("get_element_color", "element"), &ClockGearsDamageEngine::get_element_color);
	ClassDB::bind_static_method("ClockGearsDamageEngine", D_METHOD("get_affinity_name", "affinity"), &ClockGearsDamageEngine::get_affinity_name);
	ClassDB::bind_static_method("ClockGearsDamageEngine", D_METHOD("get_affinity_multiplier", "affinity"), &ClockGearsDamageEngine::get_affinity_multiplier);
}

String ClockGearsDamageEngine::sanitize_formula(const String &p_formula) {
	String f = p_formula.strip_edges();
	if (f.is_empty()) {
		f = "a_atk * 4 - b_def * 2";
	}

	// Normalize RPG Maker 'a.' and 'b.' notation to identifier safe 'a_' and 'b_'
	f = f.replace("a.hp", "a_hp");
	f = f.replace("a.mhp", "a_mhp");
	f = f.replace("a.mp", "a_mp");
	f = f.replace("a.mmp", "a_mmp");
	f = f.replace("a.atk", "a_atk");
	f = f.replace("a.def", "a_def");
	f = f.replace("a.matk", "a_mat");
	f = f.replace("a.mat", "a_mat");
	f = f.replace("a.mdef", "a_mdf");
	f = f.replace("a.mdf", "a_mdf");
	f = f.replace("a.agi", "a_agi");
	f = f.replace("a.luk", "a_luk");
	f = f.replace("a.level", "a_lvl");
	f = f.replace("a.lvl", "a_lvl");

	f = f.replace("b.hp", "b_hp");
	f = f.replace("b.mhp", "b_mhp");
	f = f.replace("b.mp", "b_mp");
	f = f.replace("b.mmp", "b_mmp");
	f = f.replace("b.atk", "b_atk");
	f = f.replace("b.def", "b_def");
	f = f.replace("b.matk", "b_mat");
	f = f.replace("b.mat", "b_mat");
	f = f.replace("b.mdef", "b_mdf");
	f = f.replace("b.mdf", "b_mdf");
	f = f.replace("b.agi", "b_agi");
	f = f.replace("b.luk", "b_luk");
	f = f.replace("b.level", "b_lvl");
	f = f.replace("b.lvl", "b_lvl");

	return f;
}

Dictionary ClockGearsDamageEngine::evaluate_damage(
		ClockGearsBattler *p_attacker,
		ClockGearsBattler *p_target,
		const String &p_formula,
		int p_element,
		float p_variance_pct,
		float p_crit_chance,
		float p_crit_mult) {
	String clean_f = sanitize_formula(p_formula);

	// Setup variable inputs
	Vector<String> input_names;
	input_names.push_back("a_atk");
	input_names.push_back("a_def");
	input_names.push_back("a_mat");
	input_names.push_back("a_mdf");
	input_names.push_back("a_agi");
	input_names.push_back("a_luk");
	input_names.push_back("a_hp");
	input_names.push_back("a_mp");
	input_names.push_back("a_mhp");
	input_names.push_back("a_mmp");
	input_names.push_back("a_lvl");

	input_names.push_back("b_atk");
	input_names.push_back("b_def");
	input_names.push_back("b_mat");
	input_names.push_back("b_mdf");
	input_names.push_back("b_agi");
	input_names.push_back("b_luk");
	input_names.push_back("b_hp");
	input_names.push_back("b_mp");
	input_names.push_back("b_mhp");
	input_names.push_back("b_mmp");
	input_names.push_back("b_lvl");

	int a_atk = p_attacker ? p_attacker->get_atk() : 15;
	int a_def = p_attacker ? p_attacker->get_def() : 10;
	int a_mat = p_attacker ? p_attacker->get_mat() : 10;
	int a_mdf = p_attacker ? p_attacker->get_mdf() : 10;
	int a_agi = p_attacker ? p_attacker->get_agi() : 10;
	int a_luk = p_attacker ? p_attacker->get_luk() : 10;
	int a_hp = p_attacker ? p_attacker->get_current_hp() : 100;
	int a_mp = p_attacker ? p_attacker->get_current_mp() : 20;
	int a_mhp = p_attacker ? p_attacker->get_max_hp() : 100;
	int a_mmp = p_attacker ? p_attacker->get_max_mp() : 20;
	int a_lvl = p_attacker ? p_attacker->get_level() : 1;

	int b_atk = p_target ? p_target->get_atk() : 10;
	int b_def = p_target ? p_target->get_def() : 10;
	int b_mat = p_target ? p_target->get_mat() : 10;
	int b_mdf = p_target ? p_target->get_mdf() : 10;
	int b_agi = p_target ? p_target->get_agi() : 10;
	int b_luk = p_target ? p_target->get_luk() : 10;
	int b_hp = p_target ? p_target->get_current_hp() : 100;
	int b_mp = p_target ? p_target->get_current_mp() : 20;
	int b_mhp = p_target ? p_target->get_max_hp() : 100;
	int b_mmp = p_target ? p_target->get_max_mp() : 20;
	int b_lvl = p_target ? p_target->get_level() : 1;

	Array input_vals;
	input_vals.push_back(a_atk);
	input_vals.push_back(a_def);
	input_vals.push_back(a_mat);
	input_vals.push_back(a_mdf);
	input_vals.push_back(a_agi);
	input_vals.push_back(a_luk);
	input_vals.push_back(a_hp);
	input_vals.push_back(a_mp);
	input_vals.push_back(a_mhp);
	input_vals.push_back(a_mmp);
	input_vals.push_back(a_lvl);

	input_vals.push_back(b_atk);
	input_vals.push_back(b_def);
	input_vals.push_back(b_mat);
	input_vals.push_back(b_mdf);
	input_vals.push_back(b_agi);
	input_vals.push_back(b_luk);
	input_vals.push_back(b_hp);
	input_vals.push_back(b_mp);
	input_vals.push_back(b_mhp);
	input_vals.push_back(b_mmp);
	input_vals.push_back(b_lvl);

	Ref<Expression> expr;
	expr.instantiate();
	double base_damage = 0.0;
	Error err = expr->parse(clean_f, input_names);
	if (err == OK) {
		Variant res = expr->execute(input_vals, nullptr, false);
		if (!expr->has_execute_failed() && res.is_num()) {
			base_damage = (double)res;
		} else {
			base_damage = (double)(a_atk * 4 - b_def * 2);
		}
	} else {
		base_damage = (double)(a_atk * 4 - b_def * 2);
	}

	// Critical calculation
	float total_crit = p_crit_chance;
	if (p_attacker && p_target) {
		total_crit += MAX(0.0f, (float)(a_luk - b_luk) * 0.003f);
	}
	bool is_critical = (Math::randf() <= total_crit);
	if (is_critical) {
		base_damage *= (double)p_crit_mult;
	}

	// Elemental affinity check
	float elem_rate = p_target ? p_target->get_element_rate(p_element) : 1.0f;
	bool is_immune = (elem_rate == 0.0f);
	bool is_weak = (elem_rate > 1.0f);
	bool is_absorbed = (elem_rate < 0.0f);

	// Variance application (+/- variance_pct)
	if (p_variance_pct > 0.0f && !is_immune) {
		float v = p_variance_pct / 100.0f;
		float factor = (1.0f - v) + (Math::randf() * (v * 2.0f));
		base_damage *= factor;
	}

	// Clean integer result
	int final_damage = 0;
	if (is_immune) {
		final_damage = 0;
	} else if (is_absorbed) {
		final_damage = -Math::round(Math::abs(base_damage));
	} else {
		final_damage = Math::round(base_damage * elem_rate);
		if (final_damage < 0) {
			final_damage = 0;
		}
	}

	Dictionary result;
	result["damage"] = final_damage;
	result["is_critical"] = is_critical;
	result["element"] = p_element;
	result["element_rate"] = elem_rate;
	result["is_immune"] = is_immune;
	result["is_weak"] = is_weak;
	result["is_absorbed"] = is_absorbed;

	return result;
}

String ClockGearsDamageEngine::get_element_name(int p_element) {
	switch (p_element) {
		case ELEMENT_PHYSICAL: return "Fisico";
		case ELEMENT_FIRE: return "Fuego";
		case ELEMENT_ICE: return "Hielo";
		case ELEMENT_THUNDER: return "Rayo";
		case ELEMENT_WATER: return "Agua";
		case ELEMENT_EARTH: return "Tierra";
		case ELEMENT_WIND: return "Viento";
		case ELEMENT_LIGHT: return "Luz";
		case ELEMENT_DARKNESS: return "Oscuridad";
		default: return "Neutro";
	}
}

Color ClockGearsDamageEngine::get_element_color(int p_element) {
	switch (p_element) {
		case ELEMENT_PHYSICAL: return Color(0.9f, 0.9f, 0.95f);
		case ELEMENT_FIRE: return Color(1.0f, 0.35f, 0.15f);
		case ELEMENT_ICE: return Color(0.35f, 0.85f, 1.0f);
		case ELEMENT_THUNDER: return Color(1.0f, 0.95f, 0.2f);
		case ELEMENT_WATER: return Color(0.2f, 0.5f, 1.0f);
		case ELEMENT_EARTH: return Color(0.7f, 0.5f, 0.3f);
		case ELEMENT_WIND: return Color(0.3f, 0.9f, 0.5f);
		case ELEMENT_LIGHT: return Color(1.0f, 1.0f, 0.6f);
		case ELEMENT_DARKNESS: return Color(0.6f, 0.2f, 0.85f);
		default: return Color(1.0f, 1.0f, 1.0f);
	}
}

String ClockGearsDamageEngine::get_affinity_name(int p_affinity) {
	switch (p_affinity) {
		case AFFINITY_IMMUNE: return "Inmune (0%)";
		case AFFINITY_RESISTANT: return "Resistente (50%)";
		case AFFINITY_NEUTRAL: return "Neutro (100%)";
		case AFFINITY_WEAK: return "Debil (150%)";
		case AFFINITY_SUPER_WEAK: return "Super Debil (200%)";
		case AFFINITY_ABSORB: return "Absorcion (Cura)";
		default: return "Neutro (100%)";
	}
}

float ClockGearsDamageEngine::get_affinity_multiplier(int p_affinity) {
	switch (p_affinity) {
		case AFFINITY_IMMUNE: return 0.0f;
		case AFFINITY_RESISTANT: return 0.5f;
		case AFFINITY_NEUTRAL: return 1.0f;
		case AFFINITY_WEAK: return 1.5f;
		case AFFINITY_SUPER_WEAK: return 2.0f;
		case AFFINITY_ABSORB: return -1.0f;
		default: return 1.0f;
	}
}

ClockGearsDamageEngine::ClockGearsDamageEngine() {}
