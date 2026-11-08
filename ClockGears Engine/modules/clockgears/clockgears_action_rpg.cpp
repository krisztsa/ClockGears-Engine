#include "clockgears_action_rpg.h"
#include "scene/main/scene_tree.h"

void ClockGearsHurtbox2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_battler_path", "path"), &ClockGearsHurtbox2D::set_battler_path);
	ClassDB::bind_method(D_METHOD("get_battler_path"), &ClockGearsHurtbox2D::get_battler_path);

	ClassDB::bind_method(D_METHOD("set_battler", "battler"), &ClockGearsHurtbox2D::set_battler);
	ClassDB::bind_method(D_METHOD("get_battler"), &ClockGearsHurtbox2D::get_battler);

	ClassDB::bind_method(D_METHOD("take_hit", "hitbox"), &ClockGearsHurtbox2D::take_hit);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "battler_path"), "set_battler_path", "get_battler_path");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "battler", PROPERTY_HINT_NODE_TYPE, "ClockGearsBattler"), "set_battler", "get_battler");

	ADD_SIGNAL(MethodInfo("hit_received", PropertyInfo(Variant::INT, "damage"), PropertyInfo(Variant::BOOL, "is_critical"), PropertyInfo(Variant::INT, "element")));
}

void ClockGearsHurtbox2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (!battler && !battler_path.is_empty()) {
				battler = Object::cast_to<ClockGearsBattler>(get_node_or_null(battler_path));
			}
			if (!battler) {
				battler = Object::cast_to<ClockGearsBattler>(get_parent());
			}
		} break;
	}
}

Dictionary ClockGearsHurtbox2D::take_hit(ClockGearsHitbox2D *p_hitbox) {
	Dictionary result;
	ERR_FAIL_NULL_V(p_hitbox, result);

	if (!battler) {
		if (!battler_path.is_empty()) {
			battler = Object::cast_to<ClockGearsBattler>(get_node_or_null(battler_path));
		}
		if (!battler) {
			battler = Object::cast_to<ClockGearsBattler>(get_parent());
		}
	}

	ERR_FAIL_NULL_V(battler, result);

	if (battler->is_invulnerable() || !battler->is_alive()) {
		return result;
	}

	ClockGearsBattler *att = p_hitbox->get_attacker();
	// Prevent friendly fire in Action RPG
	if (att && att->get_is_enemy() == battler->get_is_enemy()) {
		return result;
	}

	result = ClockGearsDamageEngine::evaluate_damage(
			att,
			battler,
			p_hitbox->get_damage_formula(),
			p_hitbox->get_element(),
			p_hitbox->get_variance_pct(),
			p_hitbox->get_crit_chance(),
			p_hitbox->get_crit_mult());

	int dmg = (int)result["damage"];
	bool crit = (bool)result["is_critical"];
	int elem = (int)result["element"];
	bool imm = (bool)result["is_immune"];
	bool abs = (bool)result["is_absorbed"];

	battler->take_damage(dmg, crit, elem, get_global_position());

	SceneTree *st = is_inside_tree() ? get_tree() : nullptr;
	Node *scene_root = st ? st->get_current_scene() : nullptr;
	if (!scene_root) {
		scene_root = get_parent();
	}
	if (scene_root) {
		ClockGearsFXManager::spawn_impact_fx(scene_root, get_global_position(), dmg, crit, elem, imm, abs);
	}

	emit_signal("hit_received", dmg, crit, elem);
	return result;
}

ClockGearsHurtbox2D::ClockGearsHurtbox2D() {}

void ClockGearsHitbox2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_attacker_path", "path"), &ClockGearsHitbox2D::set_attacker_path);
	ClassDB::bind_method(D_METHOD("get_attacker_path"), &ClockGearsHitbox2D::get_attacker_path);

	ClassDB::bind_method(D_METHOD("set_attacker", "attacker"), &ClockGearsHitbox2D::set_attacker);
	ClassDB::bind_method(D_METHOD("get_attacker"), &ClockGearsHitbox2D::get_attacker);

	ClassDB::bind_method(D_METHOD("set_damage_formula", "formula"), &ClockGearsHitbox2D::set_damage_formula);
	ClassDB::bind_method(D_METHOD("get_damage_formula"), &ClockGearsHitbox2D::get_damage_formula);

	ClassDB::bind_method(D_METHOD("set_element", "element"), &ClockGearsHitbox2D::set_element);
	ClassDB::bind_method(D_METHOD("get_element"), &ClockGearsHitbox2D::get_element);

	ClassDB::bind_method(D_METHOD("set_variance_pct", "variance"), &ClockGearsHitbox2D::set_variance_pct);
	ClassDB::bind_method(D_METHOD("get_variance_pct"), &ClockGearsHitbox2D::get_variance_pct);

	ClassDB::bind_method(D_METHOD("set_crit_chance", "crit_chance"), &ClockGearsHitbox2D::set_crit_chance);
	ClassDB::bind_method(D_METHOD("get_crit_chance"), &ClockGearsHitbox2D::get_crit_chance);

	ClassDB::bind_method(D_METHOD("set_crit_mult", "crit_mult"), &ClockGearsHitbox2D::set_crit_mult);
	ClassDB::bind_method(D_METHOD("get_crit_mult"), &ClockGearsHitbox2D::get_crit_mult);

	ClassDB::bind_method(D_METHOD("set_knockback_force", "force"), &ClockGearsHitbox2D::set_knockback_force);
	ClassDB::bind_method(D_METHOD("get_knockback_force"), &ClockGearsHitbox2D::get_knockback_force);

	ClassDB::bind_method(D_METHOD("_on_area_entered", "area"), &ClockGearsHitbox2D::_on_area_entered);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "attacker_path"), "set_attacker_path", "get_attacker_path");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "attacker", PROPERTY_HINT_NODE_TYPE, "ClockGearsBattler"), "set_attacker", "get_attacker");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "damage_formula"), "set_damage_formula", "get_damage_formula");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "element", PROPERTY_HINT_ENUM, "Fisico:0,Fuego:1,Hielo:2,Rayo:3,Agua:4,Tierra:5,Viento:6,Luz:7,Oscuridad:8"), "set_element", "get_element");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "variance_pct"), "set_variance_pct", "get_variance_pct");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crit_chance"), "set_crit_chance", "get_crit_chance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crit_mult"), "set_crit_mult", "get_crit_mult");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "knockback_force"), "set_knockback_force", "get_knockback_force");

	ADD_SIGNAL(MethodInfo("hit_landed", PropertyInfo(Variant::OBJECT, "target_hurtbox"), PropertyInfo(Variant::INT, "damage")));
}

void ClockGearsHitbox2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (!attacker && !attacker_path.is_empty()) {
				attacker = Object::cast_to<ClockGearsBattler>(get_node_or_null(attacker_path));
			}
			if (!attacker) {
				attacker = Object::cast_to<ClockGearsBattler>(get_parent());
			}
		} break;
	}
}

void ClockGearsHitbox2D::_on_area_entered(Area2D *p_other) {
	ClockGearsHurtbox2D *hurtbox = Object::cast_to<ClockGearsHurtbox2D>(p_other);
	if (hurtbox) {
		Dictionary res = hurtbox->take_hit(this);
		if (!res.is_empty()) {
			emit_signal("hit_landed", hurtbox, (int)res["damage"]);
		}
	}
}

ClockGearsHitbox2D::ClockGearsHitbox2D() {
	connect("area_entered", callable_mp(this, &ClockGearsHitbox2D::_on_area_entered));
}
