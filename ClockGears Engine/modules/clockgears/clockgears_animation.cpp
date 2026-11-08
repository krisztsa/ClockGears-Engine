#include "clockgears_animation.h"
#include "core/object/message_queue.h"

// ============================================================================
// ClockGearsAnimationTriggerTable
// ============================================================================

void ClockGearsAnimationTriggerTable::_bind_methods() {
	ClassDB::bind_static_method("ClockGearsAnimationTriggerTable", D_METHOD("string_to_trigger", "name"), &ClockGearsAnimationTriggerTable::string_to_trigger);
	ClassDB::bind_static_method("ClockGearsAnimationTriggerTable", D_METHOD("trigger_to_string", "trigger"), &ClockGearsAnimationTriggerTable::trigger_to_string);

	ClassDB::bind_method(D_METHOD("bind_trigger", "trigger", "anim_name", "sub_id"), &ClockGearsAnimationTriggerTable::bind_trigger, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("bind_trigger_named", "trigger_name", "anim_name", "sub_id"), &ClockGearsAnimationTriggerTable::bind_trigger_named, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_animation", "trigger", "sub_id"), &ClockGearsAnimationTriggerTable::get_animation, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_animation_named", "trigger_name", "sub_id"), &ClockGearsAnimationTriggerTable::get_animation_named, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("has_trigger", "trigger", "sub_id"), &ClockGearsAnimationTriggerTable::has_trigger, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("has_trigger_named", "trigger_name", "sub_id"), &ClockGearsAnimationTriggerTable::has_trigger_named, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("unbind_trigger", "trigger", "sub_id"), &ClockGearsAnimationTriggerTable::unbind_trigger, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("clear"), &ClockGearsAnimationTriggerTable::clear);
	ClassDB::bind_method(D_METHOD("serialize"), &ClockGearsAnimationTriggerTable::serialize);
	ClassDB::bind_method(D_METHOD("deserialize", "data"), &ClockGearsAnimationTriggerTable::deserialize);

	BIND_CONSTANT(ANIM_TRIGGER_NONE);
	BIND_CONSTANT(ANIM_TRIGGER_ATTACK);
	BIND_CONSTANT(ANIM_TRIGGER_SKILL);
	BIND_CONSTANT(ANIM_TRIGGER_ITEM);
	BIND_CONSTANT(ANIM_TRIGGER_DAMAGE);
	BIND_CONSTANT(ANIM_TRIGGER_STATE_ADD);
	BIND_CONSTANT(ANIM_TRIGGER_DEATH);
	BIND_CONSTANT(ANIM_TRIGGER_TURN_START);
	BIND_CONSTANT(ANIM_TRIGGER_VICTORY);
	BIND_CONSTANT(ANIM_TRIGGER_MAP_WALK);
	BIND_CONSTANT(ANIM_TRIGGER_MAP_TALK);
	BIND_CONSTANT(ANIM_TRIGGER_CUSTOM);
}

int ClockGearsAnimationTriggerTable::string_to_trigger(const String &p_name) {
	String n = p_name.to_lower().strip_edges();
	if (n == "attack" || n == "atacar") return ANIM_TRIGGER_ATTACK;
	if (n == "skill" || n == "habilidad") return ANIM_TRIGGER_SKILL;
	if (n == "item" || n == "objeto") return ANIM_TRIGGER_ITEM;
	if (n == "damage" || n == "dano" || n == "daño") return ANIM_TRIGGER_DAMAGE;
	if (n == "state_add" || n == "estado") return ANIM_TRIGGER_STATE_ADD;
	if (n == "death" || n == "morir" || n == "muerte") return ANIM_TRIGGER_DEATH;
	if (n == "turn_start" || n == "turno") return ANIM_TRIGGER_TURN_START;
	if (n == "victory" || n == "victoria") return ANIM_TRIGGER_VICTORY;
	if (n == "map_walk" || n == "caminar") return ANIM_TRIGGER_MAP_WALK;
	if (n == "map_talk" || n == "hablar") return ANIM_TRIGGER_MAP_TALK;
	if (n == "custom") return ANIM_TRIGGER_CUSTOM;
	return ANIM_TRIGGER_NONE;
}

String ClockGearsAnimationTriggerTable::trigger_to_string(int p_trigger) {
	switch (p_trigger) {
		case ANIM_TRIGGER_ATTACK: return "attack";
		case ANIM_TRIGGER_SKILL: return "skill";
		case ANIM_TRIGGER_ITEM: return "item";
		case ANIM_TRIGGER_DAMAGE: return "damage";
		case ANIM_TRIGGER_STATE_ADD: return "state_add";
		case ANIM_TRIGGER_DEATH: return "death";
		case ANIM_TRIGGER_TURN_START: return "turn_start";
		case ANIM_TRIGGER_VICTORY: return "victory";
		case ANIM_TRIGGER_MAP_WALK: return "map_walk";
		case ANIM_TRIGGER_MAP_TALK: return "map_talk";
		case ANIM_TRIGGER_CUSTOM: return "custom";
		default: return "none";
	}
}

void ClockGearsAnimationTriggerTable::bind_trigger(int p_trigger, const String &p_anim_name, int p_sub_id) {
	if (p_trigger <= ANIM_TRIGGER_NONE) return;
	int64_t k = _make_key(p_trigger, p_sub_id);
	if (p_anim_name.is_empty()) {
		trigger_bindings.erase(k);
	} else {
		trigger_bindings[k] = p_anim_name;
	}
}

void ClockGearsAnimationTriggerTable::bind_trigger_named(const String &p_trigger_name, const String &p_anim_name, int p_sub_id) {
	int trig = string_to_trigger(p_trigger_name);
	bind_trigger(trig, p_anim_name, p_sub_id);
}

String ClockGearsAnimationTriggerTable::get_animation(int p_trigger, int p_sub_id) const {
	int64_t k_sub = _make_key(p_trigger, p_sub_id);
	if (trigger_bindings.has(k_sub)) {
		return trigger_bindings[k_sub];
	}
	if (p_sub_id != 0) {
		int64_t k_default = _make_key(p_trigger, 0);
		if (trigger_bindings.has(k_default)) {
			return trigger_bindings[k_default];
		}
	}
	return "";
}

String ClockGearsAnimationTriggerTable::get_animation_named(const String &p_trigger_name, int p_sub_id) const {
	int trig = string_to_trigger(p_trigger_name);
	return get_animation(trig, p_sub_id);
}

bool ClockGearsAnimationTriggerTable::has_trigger(int p_trigger, int p_sub_id) const {
	return !get_animation(p_trigger, p_sub_id).is_empty();
}

bool ClockGearsAnimationTriggerTable::has_trigger_named(const String &p_trigger_name, int p_sub_id) const {
	return !get_animation_named(p_trigger_name, p_sub_id).is_empty();
}

void ClockGearsAnimationTriggerTable::unbind_trigger(int p_trigger, int p_sub_id) {
	int64_t k = _make_key(p_trigger, p_sub_id);
	trigger_bindings.erase(k);
}

void ClockGearsAnimationTriggerTable::clear() {
	trigger_bindings.clear();
}

Dictionary ClockGearsAnimationTriggerTable::serialize() const {
	Dictionary d;
	for (const KeyValue<int64_t, String> &E : trigger_bindings) {
		int trig = int(E.key >> 32);
		int sub = int(E.key & 0xFFFFFFFF);
		String key_str = vformat("%d:%d", trig, sub);
		d[key_str] = E.value;
	}
	return d;
}

void ClockGearsAnimationTriggerTable::deserialize(const Dictionary &p_data) {
	clear();
	Array keys = p_data.keys();
	for (int i = 0; i < keys.size(); i++) {
		String k_str = keys[i];
		PackedStringArray parts = k_str.split(":");
		if (parts.size() == 2) {
			int trig = parts[0].to_int();
			int sub = parts[1].to_int();
			bind_trigger(trig, p_data[k_str], sub);
		}
	}
}

ClockGearsAnimationTriggerTable::ClockGearsAnimationTriggerTable() {
}

// ============================================================================
// ClockGearsAnimationManager
// ============================================================================

ClockGearsAnimationManager *ClockGearsAnimationManager::singleton = nullptr;

void ClockGearsAnimationManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_rng_seed", "seed"), &ClockGearsAnimationManager::set_rng_seed);
	ClassDB::bind_method(D_METHOD("get_rng_seed"), &ClockGearsAnimationManager::get_rng_seed);

	ClassDB::bind_method(D_METHOD("register_animation", "name", "data"), &ClockGearsAnimationManager::register_animation);
	ClassDB::bind_method(D_METHOD("has_animation", "name"), &ClockGearsAnimationManager::has_animation);
	ClassDB::bind_method(D_METHOD("get_animation", "name"), &ClockGearsAnimationManager::get_animation);

	ClassDB::bind_method(D_METHOD("register_sequence", "name", "steps", "loop"), &ClockGearsAnimationManager::register_sequence, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("has_sequence", "name"), &ClockGearsAnimationManager::has_sequence);

	ClassDB::bind_method(D_METHOD("register_random_variant", "name", "choices"), &ClockGearsAnimationManager::register_random_variant);
	ClassDB::bind_method(D_METHOD("has_random_variant", "name"), &ClockGearsAnimationManager::has_random_variant);

	ClassDB::bind_method(D_METHOD("resolve_animation_name", "name_or_variant"), &ClockGearsAnimationManager::resolve_animation_name);

	ClassDB::bind_method(D_METHOD("get_default_trigger_table"), &ClockGearsAnimationManager::get_default_trigger_table);
	ClassDB::bind_method(D_METHOD("set_default_trigger_table", "table"), &ClockGearsAnimationManager::set_default_trigger_table);

	ClassDB::bind_method(D_METHOD("trigger_animation", "target", "trigger", "sub_id", "custom_table"), &ClockGearsAnimationManager::trigger_animation, DEFVAL(0), DEFVAL(Ref<ClockGearsAnimationTriggerTable>()));
	ClassDB::bind_method(D_METHOD("trigger_animation_named", "target", "trigger_name", "sub_id", "custom_table"), &ClockGearsAnimationManager::trigger_animation_named, DEFVAL(0), DEFVAL(Ref<ClockGearsAnimationTriggerTable>()));
	ClassDB::bind_method(D_METHOD("play_animation", "target", "anim_name"), &ClockGearsAnimationManager::play_animation);

	ClassDB::bind_method(D_METHOD("step", "delta"), &ClockGearsAnimationManager::step);
	ClassDB::bind_method(D_METHOD("is_playing", "target"), &ClockGearsAnimationManager::is_playing);
	ClassDB::bind_method(D_METHOD("stop", "target"), &ClockGearsAnimationManager::stop);
	ClassDB::bind_method(D_METHOD("clear_all"), &ClockGearsAnimationManager::clear_all);

	ADD_SIGNAL(MethodInfo("animation_started", PropertyInfo(Variant::OBJECT, "target"), PropertyInfo(Variant::STRING, "anim_name")));
	ADD_SIGNAL(MethodInfo("animation_finished", PropertyInfo(Variant::OBJECT, "target"), PropertyInfo(Variant::STRING, "anim_name")));
}

void ClockGearsAnimationManager::set_rng_seed(uint64_t p_seed) {
	rng.seed(p_seed);
}

uint64_t ClockGearsAnimationManager::get_rng_seed() const {
	return rng.get_seed();
}

void ClockGearsAnimationManager::register_animation(const String &p_name, const Dictionary &p_data) {
	if (p_name.is_empty()) return;
	ClockGearsAnimDef def;
	def.from_dict(p_data);
	def.name = p_name;
	registered_animations[p_name] = def;
}

bool ClockGearsAnimationManager::has_animation(const String &p_name) const {
	return registered_animations.has(p_name);
}

Dictionary ClockGearsAnimationManager::get_animation(const String &p_name) const {
	if (registered_animations.has(p_name)) {
		return registered_animations[p_name].to_dict();
	}
	return Dictionary();
}

void ClockGearsAnimationManager::register_sequence(const String &p_name, const Array &p_steps, bool p_loop) {
	if (p_name.is_empty()) return;
	ClockGearsAnimSequence seq;
	seq.from_array(p_name, p_steps, p_loop);
	registered_sequences[p_name] = seq;
}

bool ClockGearsAnimationManager::has_sequence(const String &p_name) const {
	return registered_sequences.has(p_name);
}

void ClockGearsAnimationManager::register_random_variant(const String &p_name, const Array &p_choices) {
	if (p_name.is_empty()) return;
	ClockGearsRandomVariant rv;
	rv.from_array(p_name, p_choices);
	registered_variants[p_name] = rv;
}

bool ClockGearsAnimationManager::has_random_variant(const String &p_name) const {
	return registered_variants.has(p_name);
}

String ClockGearsAnimationManager::resolve_animation_name(const String &p_name_or_variant) {
	String cur = p_name_or_variant;
	int guard = 0;
	while (registered_variants.has(cur) && guard < 10) {
		cur = registered_variants[cur].pick_choice(rng);
		guard++;
	}
	return cur;
}

Ref<ClockGearsAnimationTriggerTable> ClockGearsAnimationManager::get_default_trigger_table() {
	return default_trigger_table;
}

void ClockGearsAnimationManager::set_default_trigger_table(const Ref<ClockGearsAnimationTriggerTable> &p_table) {
	default_trigger_table = p_table;
}

void ClockGearsAnimationManager::_apply_visual_feedback(Node2D *p_target, int p_trigger, const String &p_anim_name) {
	if (!p_target) return;

	if (p_trigger == ANIM_TRIGGER_DAMAGE) {
		p_target->set_modulate(Color(1.0f, 0.35f, 0.35f, 1.0f));
	} else if (p_trigger == ANIM_TRIGGER_DEATH) {
		p_target->set_modulate(Color(0.5f, 0.5f, 0.5f, 0.3f));
	} else if (p_trigger == ANIM_TRIGGER_STATE_ADD) {
		p_target->set_modulate(Color(0.7f, 0.5f, 1.0f, 1.0f));
	}
}

bool ClockGearsAnimationManager::trigger_animation(Node2D *p_target, int p_trigger, int p_sub_id, const Ref<ClockGearsAnimationTriggerTable> &p_custom_table) {
	if (p_trigger <= ANIM_TRIGGER_NONE) {
		return false;
	}

	String anim_name;
	if (p_custom_table.is_valid()) {
		anim_name = p_custom_table->get_animation(p_trigger, p_sub_id);
	}
	if (anim_name.is_empty() && default_trigger_table.is_valid()) {
		anim_name = default_trigger_table->get_animation(p_trigger, p_sub_id);
	}

	// Regla Cero: Si no tiene animación asignada, se ignora limpiamente sin error
	if (anim_name.is_empty()) {
		return false;
	}

	String resolved = resolve_animation_name(anim_name);
	if (resolved.is_empty()) {
		return false;
	}

	_apply_visual_feedback(p_target, p_trigger, resolved);

	ActivePlayback pb;
	pb.target_id = p_target ? p_target->get_instance_id() : ObjectID();
	pb.anim_name = resolved;
	pb.is_sequence = registered_sequences.has(resolved);
	pb.seq_step_idx = 0;
	pb.seq_repeat_count = 0;
	pb.timer = 0.0f;
	pb.pause_timer = 0.0f;
	pb.frame_index = 0;
	pb.finished = false;

	if (p_target) {
		stop(p_target);
	}
	active_playbacks.push_back(pb);

	emit_signal("animation_started", p_target, resolved);
	return true;
}

bool ClockGearsAnimationManager::trigger_animation_named(Node2D *p_target, const String &p_trigger_name, int p_sub_id, const Ref<ClockGearsAnimationTriggerTable> &p_custom_table) {
	int trig = ClockGearsAnimationTriggerTable::string_to_trigger(p_trigger_name);
	return trigger_animation(p_target, trig, p_sub_id, p_custom_table);
}

bool ClockGearsAnimationManager::play_animation(Node2D *p_target, const String &p_anim_name) {
	String resolved = resolve_animation_name(p_anim_name);
	if (resolved.is_empty()) return false;

	ActivePlayback pb;
	pb.target_id = p_target ? p_target->get_instance_id() : ObjectID();
	pb.anim_name = resolved;
	pb.is_sequence = registered_sequences.has(resolved);
	pb.seq_step_idx = 0;
	pb.seq_repeat_count = 0;
	pb.timer = 0.0f;
	pb.pause_timer = 0.0f;
	pb.frame_index = 0;
	pb.finished = false;

	if (p_target) {
		stop(p_target);
	}
	active_playbacks.push_back(pb);
	emit_signal("animation_started", p_target, resolved);
	return true;
}

void ClockGearsAnimationManager::step(float p_delta) {
	for (int i = active_playbacks.size() - 1; i >= 0; i--) {
		ActivePlayback &pb = active_playbacks.write[i];
		Node2D *tgt = pb.target_id.is_valid() ? Object::cast_to<Node2D>(ObjectDB::get_instance(pb.target_id)) : nullptr;

		if (pb.is_sequence) {
			if (!registered_sequences.has(pb.anim_name)) {
				pb.finished = true;
			} else {
				const ClockGearsAnimSequence &seq = registered_sequences[pb.anim_name];
				if (pb.seq_step_idx >= seq.steps.size()) {
					if (seq.loop && !seq.steps.is_empty()) {
						pb.seq_step_idx = 0;
						pb.seq_repeat_count = 0;
					} else {
						pb.finished = true;
					}
				} else {
					const ClockGearsAnimSequenceStep &st = seq.steps[pb.seq_step_idx];
					if (pb.pause_timer > 0.0f) {
						pb.pause_timer -= p_delta;
					} else {
						float dur = 0.1f;
						if (registered_animations.has(st.anim_name)) {
							const ClockGearsAnimDef &d = registered_animations[st.anim_name];
							dur = d.frame_count * d.frame_duration;
						}
						pb.timer += p_delta;
						if (pb.timer >= dur) {
							pb.timer = 0.0f;
							pb.seq_repeat_count++;
							if (pb.seq_repeat_count >= st.repeats) {
								pb.seq_repeat_count = 0;
								pb.seq_step_idx++;
								pb.pause_timer = st.delay_after;
							}
						}
					}
				}
			}
		} else {
			// Animación individual
			float dur = 0.3f;
			if (registered_animations.has(pb.anim_name)) {
				const ClockGearsAnimDef &d = registered_animations[pb.anim_name];
				dur = d.frame_count * d.frame_duration;
			}
			pb.timer += p_delta;
			if (pb.timer >= dur) {
				pb.finished = true;
				if (tgt) {
					// Restaurar modulación si no está muerto
					if (tgt->get_modulate().a >= 0.5f) {
						tgt->set_modulate(Color(1, 1, 1, 1));
					}
				}
			}
		}

		if (pb.finished) {
			emit_signal("animation_finished", tgt, pb.anim_name);
			active_playbacks.remove_at(i);
		}
	}
}

bool ClockGearsAnimationManager::is_playing(Node2D *p_target) const {
	if (!p_target) return false;
	ObjectID tid = p_target->get_instance_id();
	for (int i = 0; i < active_playbacks.size(); i++) {
		if (active_playbacks[i].target_id == tid) return true;
	}
	return false;
}

void ClockGearsAnimationManager::stop(Node2D *p_target) {
	if (!p_target) return;
	ObjectID tid = p_target->get_instance_id();
	for (int i = active_playbacks.size() - 1; i >= 0; i--) {
		if (active_playbacks[i].target_id == tid) {
			active_playbacks.remove_at(i);
		}
	}
}

void ClockGearsAnimationManager::clear_all() {
	active_playbacks.clear();
	registered_animations.clear();
	registered_sequences.clear();
	registered_variants.clear();
}

ClockGearsAnimationManager::ClockGearsAnimationManager() {
	singleton = this;
	rng.seed(1337);
	default_trigger_table.instantiate();
}

ClockGearsAnimationManager::~ClockGearsAnimationManager() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
