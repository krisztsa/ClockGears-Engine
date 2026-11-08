#include "clockgears_fx_manager.h"
#include "core/string/print_string.h"

void ClockGearsFXInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup_hit", "damage", "critical", "element", "immune", "absorbed"), &ClockGearsFXInstance::setup_hit, DEFVAL(false), DEFVAL(false));
}

void ClockGearsFXInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			set_process(true);
		} break;
		case NOTIFICATION_PROCESS: {
			float dt = get_process_delta_time();
			elapsed += dt;
			if (damage_label) {
				damage_label->set_position(damage_label->get_position() + float_velocity * dt);
			}
			if (elapsed > 0.35f) {
				float a = CLAMP(1.0f - (elapsed - 0.35f) / 0.3f, 0.0f, 1.0f);
				set_modulate(Color(1.0f, 1.0f, 1.0f, a));
			}
			if (elapsed >= duration) {
				set_process(false);
				queue_free();
			}
		} break;
	}
}

void ClockGearsFXInstance::setup_hit(int p_damage, bool p_critical, int p_element, bool p_immune, bool p_absorbed) {
	Color elem_color = ClockGearsDamageEngine::get_element_color(p_element);

	if (particles) {
		particles->set_emitting(true);
		particles->set_amount(p_critical ? 45 : 24);
		particles->set_lifetime(0.45);
		particles->set_one_shot(true);
		particles->set_explosiveness_ratio(0.85f);
		particles->set_spread(180.0f);
		particles->set_direction(Vector2(0, -1));
		particles->set_param_min(CPUParticles2D::PARAM_INITIAL_LINEAR_VELOCITY, p_critical ? 80.0f : 50.0f);
		particles->set_param_max(CPUParticles2D::PARAM_INITIAL_LINEAR_VELOCITY, p_critical ? 180.0f : 120.0f);
		particles->set_param_min(CPUParticles2D::PARAM_SCALE, 3.0f);
		particles->set_param_max(CPUParticles2D::PARAM_SCALE, p_critical ? 6.0f : 4.0f);
		particles->set_color(elem_color);
	}

	if (damage_label) {
		if (p_immune) {
			damage_label->set_text("INMUNE");
			damage_label->add_theme_color_override("font_color", Color(0.7f, 0.7f, 0.7f));
		} else if (p_absorbed) {
			damage_label->set_text(vformat("+%d", Math::abs(p_damage)));
			damage_label->add_theme_color_override("font_color", Color(0.2f, 1.0f, 0.4f));
		} else if (p_critical) {
			damage_label->set_text(vformat("CRITICO!\n%d", p_damage));
			damage_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.1f));
		} else {
			damage_label->set_text(String::num_int64(p_damage));
			damage_label->add_theme_color_override("font_color", elem_color);
		}
	}
}

ClockGearsFXInstance::ClockGearsFXInstance() {
	particles = memnew(CPUParticles2D);
	add_child(particles);

	damage_label = memnew(Label);
	damage_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	damage_label->set_position(Vector2(-50, -25));
	damage_label->set_size(Size2(100, 40));
	add_child(damage_label);
}

void ClockGearsFXManager::_bind_methods() {
	ClassDB::bind_static_method("ClockGearsFXManager", D_METHOD("spawn_impact_fx", "parent", "pos", "damage", "critical", "element", "immune", "absorbed"), &ClockGearsFXManager::spawn_impact_fx, DEFVAL(false), DEFVAL(0), DEFVAL(false), DEFVAL(false));
}

Node2D *ClockGearsFXManager::spawn_impact_fx(
		Node *p_parent,
		Vector2 p_pos,
		int p_damage,
		bool p_critical,
		int p_element,
		bool p_immune,
		bool p_absorbed) {
	ERR_FAIL_NULL_V(p_parent, nullptr);

	ClockGearsFXInstance *fx = memnew(ClockGearsFXInstance);
	fx->set_position(p_pos);
	fx->setup_hit(p_damage, p_critical, p_element, p_immune, p_absorbed);
	p_parent->add_child(fx);
	return fx;
}

ClockGearsFXManager::ClockGearsFXManager() {}
