#pragma once

#include "clockgears_types.h"
#include "clockgears_battler.h"
#include "clockgears_damage_engine.h"
#include "clockgears_fx_manager.h"
#include "scene/2d/physics/area_2d.h"

class ClockGearsHitbox2D;

class ClockGearsHurtbox2D : public Area2D {
	GDCLASS(ClockGearsHurtbox2D, Area2D);

private:
	ClockGearsBattler *battler = nullptr;
	NodePath battler_path;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_battler_path(const NodePath &p_path) { battler_path = p_path; }
	NodePath get_battler_path() const { return battler_path; }

	void set_battler(ClockGearsBattler *p_battler) { battler = p_battler; }
	ClockGearsBattler *get_battler() const { return battler; }

	Dictionary take_hit(ClockGearsHitbox2D *p_hitbox);

	ClockGearsHurtbox2D();
};

class ClockGearsHitbox2D : public Area2D {
	GDCLASS(ClockGearsHitbox2D, Area2D);

private:
	ClockGearsBattler *attacker = nullptr;
	NodePath attacker_path;
	String damage_formula = "a.atk * 4 - b.def * 2";
	int element = ELEMENT_PHYSICAL;
	float variance_pct = 20.0f;
	float crit_chance = 0.05f;
	float crit_mult = 2.0f;
	float knockback_force = 150.0f;

	void _on_area_entered(Area2D *p_other);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_attacker_path(const NodePath &p_path) { attacker_path = p_path; }
	NodePath get_attacker_path() const { return attacker_path; }

	void set_attacker(ClockGearsBattler *p_attacker) { attacker = p_attacker; }
	ClockGearsBattler *get_attacker() const { return attacker; }

	void set_damage_formula(const String &p_formula) { damage_formula = p_formula; }
	String get_damage_formula() const { return damage_formula; }

	void set_element(int p_element) { element = p_element; }
	int get_element() const { return element; }

	void set_variance_pct(float p_var) { variance_pct = p_var; }
	float get_variance_pct() const { return variance_pct; }

	void set_crit_chance(float p_crit) { crit_chance = p_crit; }
	float get_crit_chance() const { return crit_chance; }

	void set_crit_mult(float p_mult) { crit_mult = p_mult; }
	float get_crit_mult() const { return crit_mult; }

	void set_knockback_force(float p_force) { knockback_force = p_force; }
	float get_knockback_force() const { return knockback_force; }

	ClockGearsHitbox2D();
};
