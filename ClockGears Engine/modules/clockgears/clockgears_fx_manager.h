#pragma once

#include "clockgears_types.h"
#include "clockgears_damage_engine.h"
#include "scene/2d/node_2d.h"
#include "scene/2d/cpu_particles_2d.h"
#include "scene/gui/label.h"

class ClockGearsFXInstance : public Node2D {
	GDCLASS(ClockGearsFXInstance, Node2D);

private:
	CPUParticles2D *particles = nullptr;
	Label *damage_label = nullptr;
	float duration = 0.65f;
	float elapsed = 0.0f;
	Vector2 float_velocity = Vector2(0, -45.0f);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void setup_hit(int p_damage, bool p_critical, int p_element, bool p_immune = false, bool p_absorbed = false);
	ClockGearsFXInstance();
};

class ClockGearsFXManager : public RefCounted {
	GDCLASS(ClockGearsFXManager, RefCounted);

protected:
	static void _bind_methods();

public:
	static Node2D *spawn_impact_fx(
			Node *p_parent,
			Vector2 p_pos,
			int p_damage,
			bool p_critical,
			int p_element,
			bool p_immune = false,
			bool p_absorbed = false);

	ClockGearsFXManager();
};
