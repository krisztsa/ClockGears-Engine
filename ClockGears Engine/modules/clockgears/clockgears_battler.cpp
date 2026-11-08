#include "clockgears_battler.h"
#include "core/math/math_funcs.h"

void ClockGearsBattler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_battler_name", "name"), &ClockGearsBattler::set_battler_name);
	ClassDB::bind_method(D_METHOD("get_battler_name"), &ClockGearsBattler::get_battler_name);

	ClassDB::bind_method(D_METHOD("set_is_enemy", "is_enemy"), &ClockGearsBattler::set_is_enemy);
	ClassDB::bind_method(D_METHOD("get_is_enemy"), &ClockGearsBattler::get_is_enemy);

	ClassDB::bind_method(D_METHOD("set_class_id", "id"), &ClockGearsBattler::set_class_id);
	ClassDB::bind_method(D_METHOD("get_class_id"), &ClockGearsBattler::get_class_id);

	ClassDB::bind_method(D_METHOD("set_level", "level"), &ClockGearsBattler::set_level);
	ClassDB::bind_method(D_METHOD("get_level"), &ClockGearsBattler::get_level);

	ClassDB::bind_method(D_METHOD("set_current_exp", "exp"), &ClockGearsBattler::set_current_exp);
	ClassDB::bind_method(D_METHOD("get_current_exp"), &ClockGearsBattler::get_current_exp);

	ClassDB::bind_method(D_METHOD("get_exp_for_level", "level"), &ClockGearsBattler::get_exp_for_level);
	ClassDB::bind_method(D_METHOD("get_exp_to_next_level"), &ClockGearsBattler::get_exp_to_next_level);
	ClassDB::bind_method(D_METHOD("gain_exp", "amount"), &ClockGearsBattler::gain_exp);
	ClassDB::bind_method(D_METHOD("recalculate_stats_for_level", "level"), &ClockGearsBattler::recalculate_stats_for_level);

	ClassDB::bind_method(D_METHOD("set_current_hp", "hp"), &ClockGearsBattler::set_current_hp);
	ClassDB::bind_method(D_METHOD("get_current_hp"), &ClockGearsBattler::get_current_hp);

	ClassDB::bind_method(D_METHOD("set_max_hp", "max_hp"), &ClockGearsBattler::set_max_hp);
	ClassDB::bind_method(D_METHOD("get_max_hp"), &ClockGearsBattler::get_max_hp);

	ClassDB::bind_method(D_METHOD("set_current_mp", "mp"), &ClockGearsBattler::set_current_mp);
	ClassDB::bind_method(D_METHOD("get_current_mp"), &ClockGearsBattler::get_current_mp);

	ClassDB::bind_method(D_METHOD("set_max_mp", "max_mp"), &ClockGearsBattler::set_max_mp);
	ClassDB::bind_method(D_METHOD("get_max_mp"), &ClockGearsBattler::get_max_mp);

	ClassDB::bind_method(D_METHOD("set_atk", "atk"), &ClockGearsBattler::set_atk);
	ClassDB::bind_method(D_METHOD("get_atk"), &ClockGearsBattler::get_atk);

	ClassDB::bind_method(D_METHOD("set_def", "def"), &ClockGearsBattler::set_def);
	ClassDB::bind_method(D_METHOD("get_def"), &ClockGearsBattler::get_def);

	ClassDB::bind_method(D_METHOD("set_mat", "mat"), &ClockGearsBattler::set_mat);
	ClassDB::bind_method(D_METHOD("get_mat"), &ClockGearsBattler::get_mat);

	ClassDB::bind_method(D_METHOD("set_mdf", "mdf"), &ClockGearsBattler::set_mdf);
	ClassDB::bind_method(D_METHOD("get_mdf"), &ClockGearsBattler::get_mdf);

	ClassDB::bind_method(D_METHOD("set_agi", "agi"), &ClockGearsBattler::set_agi);
	ClassDB::bind_method(D_METHOD("get_agi"), &ClockGearsBattler::get_agi);

	ClassDB::bind_method(D_METHOD("set_luk", "luk"), &ClockGearsBattler::set_luk);
	ClassDB::bind_method(D_METHOD("get_luk"), &ClockGearsBattler::get_luk);

	ClassDB::bind_method(D_METHOD("set_element_affinity", "element", "affinity"), &ClockGearsBattler::set_element_affinity);
	ClassDB::bind_method(D_METHOD("get_element_affinity", "element"), &ClockGearsBattler::get_element_affinity);
	ClassDB::bind_method(D_METHOD("get_element_rate", "element"), &ClockGearsBattler::get_element_rate);
	ClassDB::bind_method(D_METHOD("set_element_affinities_dict", "affinities"), &ClockGearsBattler::set_element_affinities_dict);
	ClassDB::bind_method(D_METHOD("get_element_affinities_dict"), &ClockGearsBattler::get_element_affinities_dict);

	ClassDB::bind_method(D_METHOD("take_damage", "damage", "critical", "element", "hit_pos"), &ClockGearsBattler::take_damage, DEFVAL(false), DEFVAL(0), DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("heal_hp", "amount"), &ClockGearsBattler::heal_hp);
	ClassDB::bind_method(D_METHOD("heal_mp", "amount"), &ClockGearsBattler::heal_mp);
	ClassDB::bind_method(D_METHOD("consume_mp", "amount"), &ClockGearsBattler::consume_mp);
	ClassDB::bind_method(D_METHOD("is_alive"), &ClockGearsBattler::is_alive);
	ClassDB::bind_method(D_METHOD("is_invulnerable"), &ClockGearsBattler::is_invulnerable);

	ClassDB::bind_method(D_METHOD("set_invulnerability_duration", "duration"), &ClockGearsBattler::set_invulnerability_duration);
	ClassDB::bind_method(D_METHOD("get_invulnerability_duration"), &ClockGearsBattler::get_invulnerability_duration);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "battler_name"), "set_battler_name", "get_battler_name");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_enemy"), "set_is_enemy", "get_is_enemy");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "class_id"), "set_class_id", "get_class_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "level"), "set_level", "get_level");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "current_exp"), "set_current_exp", "get_current_exp");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "current_hp"), "set_current_hp", "get_current_hp");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_hp"), "set_max_hp", "get_max_hp");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "current_mp"), "set_current_mp", "get_current_mp");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_mp"), "set_max_mp", "get_max_mp");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "atk"), "set_atk", "get_atk");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "def"), "set_def", "get_def");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mat"), "set_mat", "get_mat");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mdf"), "set_mdf", "get_mdf");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "agi"), "set_agi", "get_agi");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "luk"), "set_luk", "get_luk");

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "element_affinities"), "set_element_affinities_dict", "get_element_affinities_dict");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "invulnerability_duration"), "set_invulnerability_duration", "get_invulnerability_duration");

	ADD_SIGNAL(MethodInfo("hp_changed", PropertyInfo(Variant::INT, "current_hp"), PropertyInfo(Variant::INT, "max_hp")));
	ADD_SIGNAL(MethodInfo("mp_changed", PropertyInfo(Variant::INT, "current_mp"), PropertyInfo(Variant::INT, "max_mp")));
	ADD_SIGNAL(MethodInfo("damaged", PropertyInfo(Variant::INT, "amount"), PropertyInfo(Variant::BOOL, "is_critical"), PropertyInfo(Variant::INT, "element"), PropertyInfo(Variant::VECTOR2, "hit_pos")));
	ADD_SIGNAL(MethodInfo("healed", PropertyInfo(Variant::INT, "amount")));
	ADD_SIGNAL(MethodInfo("defeated"));
	ADD_SIGNAL(MethodInfo("level_up", PropertyInfo(Variant::INT, "new_level")));
}

void ClockGearsBattler::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_process(true);
		} break;
		case NOTIFICATION_PROCESS: {
			if (invulnerability_timer > 0.0f) {
				invulnerability_timer -= get_process_delta_time();
				if (flash_on_damage) {
					// Flash effect in real-time Action RPG combat
					if (Math::fmod(invulnerability_timer, 0.1f) > 0.05f) {
						set_modulate(Color(1.0f, 0.4f, 0.4f, 0.6f));
					} else {
						set_modulate(Color(1.0f, 1.0f, 1.0f, 0.9f));
					}
				}
				if (invulnerability_timer <= 0.0f) {
					invulnerability_timer = 0.0f;
					set_modulate(Color(1.0f, 1.0f, 1.0f, 1.0f));
				}
			}
		} break;
	}
}

void ClockGearsBattler::set_level(int p_level) {
	level = CLAMP(p_level, 1, 99);
	recalculate_stats_for_level(level);
}

int ClockGearsBattler::get_exp_for_level(int p_level) const {
	if (p_level <= 1) {
		return 0;
	}
	double l = (double)(p_level - 1);
	return (int)(30.0 * l + 20.0 * Math::pow(l, 1.25));
}

int ClockGearsBattler::get_exp_to_next_level() const {
	if (level >= 99) {
		return 0;
	}
	int next_total = get_exp_for_level(level + 1);
	return MAX(0, next_total - current_exp);
}

void ClockGearsBattler::gain_exp(int p_amount) {
	if (p_amount <= 0 || level >= 99) {
		return;
	}
	current_exp += p_amount;
	while (level < 99 && current_exp >= get_exp_for_level(level + 1)) {
		level++;
		recalculate_stats_for_level(level);
		emit_signal("level_up", level);
	}
}

void ClockGearsBattler::recalculate_stats_for_level(int p_level) {
	float t = (float)(p_level - 1) / 98.0f;
	max_hp = min_hp + (int)(t * (max_curve_hp - min_hp));
	max_mp = min_mp + (int)(t * (max_curve_mp - min_mp));
	atk = min_atk + (int)(t * (max_curve_atk - min_atk));
	def = min_def + (int)(t * (max_curve_def - min_def));
	mat = min_mat + (int)(t * (max_curve_mat - min_mat));
	mdf = min_mdf + (int)(t * (max_curve_mdf - min_mdf));
	agi = min_agi + (int)(t * (max_curve_agi - min_agi));
	luk = min_luk + (int)(t * (max_curve_luk - min_luk));

	current_hp = CLAMP(current_hp, 1, max_hp);
	current_mp = CLAMP(current_mp, 0, max_mp);
}

void ClockGearsBattler::set_current_hp(int p_hp) {
	current_hp = CLAMP(p_hp, 0, max_hp);
	emit_signal("hp_changed", current_hp, max_hp);
}

void ClockGearsBattler::set_max_hp(int p_hp) {
	max_hp = MAX(1, p_hp);
	current_hp = CLAMP(current_hp, 0, max_hp);
	emit_signal("hp_changed", current_hp, max_hp);
}

void ClockGearsBattler::set_current_mp(int p_mp) {
	current_mp = CLAMP(p_mp, 0, max_mp);
	emit_signal("mp_changed", current_mp, max_mp);
}

void ClockGearsBattler::set_max_mp(int p_mp) {
	max_mp = MAX(0, p_mp);
	current_mp = CLAMP(current_mp, 0, max_mp);
	emit_signal("mp_changed", current_mp, max_mp);
}

void ClockGearsBattler::set_element_affinity(int p_element, int p_affinity) {
	element_affinities[p_element] = p_affinity;
}

int ClockGearsBattler::get_element_affinity(int p_element) const {
	if (element_affinities.has(p_element)) {
		return (int)element_affinities[p_element];
	}
	return AFFINITY_NEUTRAL;
}

float ClockGearsBattler::get_element_rate(int p_element) const {
	int aff = get_element_affinity(p_element);
	switch (aff) {
		case AFFINITY_IMMUNE: return 0.0f;
		case AFFINITY_RESISTANT: return 0.5f;
		case AFFINITY_NEUTRAL: return 1.0f;
		case AFFINITY_WEAK: return 1.5f;
		case AFFINITY_SUPER_WEAK: return 2.0f;
		case AFFINITY_ABSORB: return -1.0f;
		default: return 1.0f;
	}
}

int ClockGearsBattler::take_damage(int p_damage, bool p_critical, int p_element, Vector2 p_hit_pos) {
	if (invulnerability_timer > 0.0f) {
		return 0; // In Action RPG, i-frames prevent spam damage
	}

	if (p_damage < 0) {
		heal_hp(-p_damage);
		return p_damage;
	}

	current_hp = MAX(0, current_hp - p_damage);
	invulnerability_timer = invulnerability_duration;

	emit_signal("damaged", p_damage, p_critical, p_element, p_hit_pos);
	emit_signal("hp_changed", current_hp, max_hp);

	if (current_hp == 0) {
		emit_signal("defeated");
	}

	return p_damage;
}

void ClockGearsBattler::heal_hp(int p_amount) {
	if (p_amount <= 0 || current_hp <= 0) {
		return;
	}
	current_hp = MIN(max_hp, current_hp + p_amount);
	emit_signal("healed", p_amount);
	emit_signal("hp_changed", current_hp, max_hp);
}

void ClockGearsBattler::heal_mp(int p_amount) {
	if (p_amount <= 0) {
		return;
	}
	current_mp = MIN(max_mp, current_mp + p_amount);
	emit_signal("mp_changed", current_mp, max_mp);
}

bool ClockGearsBattler::consume_mp(int p_amount) {
	if (current_mp < p_amount) {
		return false;
	}
	current_mp -= p_amount;
	emit_signal("mp_changed", current_mp, max_mp);
	return true;
}

ClockGearsBattler::ClockGearsBattler() {
	// Default affinities: All neutral
	for (int i = 0; i < ELEMENT_COUNT; i++) {
		element_affinities[i] = (int)AFFINITY_NEUTRAL;
	}
}
