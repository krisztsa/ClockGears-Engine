#include "clockgears_animation.h"
#include "core/clockgears_data_manager.h"
#include "core/io/image.h"
#include "core/object/message_queue.h"
#include "scene/resources/image_texture.h"

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
	BIND_CONSTANT(ANIM_TRIGGER_IDLE);
	BIND_CONSTANT(ANIM_TRIGGER_DEFEND);
}

int ClockGearsAnimationTriggerTable::string_to_trigger(const String &p_name) {
	String n = p_name.to_lower().strip_edges();
	if (n == "attack" || n == "atacar" || n == "ataque") return ANIM_TRIGGER_ATTACK;
	if (n == "skill" || n == "habilidad") return ANIM_TRIGGER_SKILL;
	if (n == "item" || n == "objeto") return ANIM_TRIGGER_ITEM;
	if (n == "damage" || n == "dano" || n == "daño" || n == "daño recibido" || n == "dano recibido" || n == "hurt") return ANIM_TRIGGER_DAMAGE;
	if (n == "state_add" || n == "estado") return ANIM_TRIGGER_STATE_ADD;
	if (n == "death" || n == "morir" || n == "muerte") return ANIM_TRIGGER_DEATH;
	if (n == "turn_start" || n == "turno") return ANIM_TRIGGER_TURN_START;
	if (n == "victory" || n == "victoria") return ANIM_TRIGGER_VICTORY;
	if (n == "map_walk" || n == "caminar") return ANIM_TRIGGER_MAP_WALK;
	if (n == "map_talk" || n == "hablar") return ANIM_TRIGGER_MAP_TALK;
	if (n == "idle" || n == "reposo") return ANIM_TRIGGER_IDLE;
	if (n == "defend" || n == "defender" || n == "defensa") return ANIM_TRIGGER_DEFEND;
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
		case ANIM_TRIGGER_IDLE: return "idle";
		case ANIM_TRIGGER_DEFEND: return "defend";
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

// ============================================================================
// ClockGearsCharacterStateRegistry (Tanda 1A - Secciones 4, 5, 6)
// ============================================================================

ClockGearsCharacterStateRegistry *ClockGearsCharacterStateRegistry::singleton = nullptr;

void ClockGearsCharacterStateRegistry::_bind_methods() {
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("is_valid_direction", "dir"), &ClockGearsCharacterStateRegistry::is_valid_direction);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("is_diagonal_direction", "dir"), &ClockGearsCharacterStateRegistry::is_diagonal_direction);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("direction_to_suffix", "dir"), &ClockGearsCharacterStateRegistry::direction_to_suffix);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("direction_to_nearest_horizontal", "dir"), &ClockGearsCharacterStateRegistry::direction_to_nearest_horizontal);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("direction_to_sheet_row", "dir", "rows"), &ClockGearsCharacterStateRegistry::direction_to_sheet_row);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("vector_to_direction", "vec"), &ClockGearsCharacterStateRegistry::vector_to_direction);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("get_actor_slot_ids", "movement_directions", "directional_idle"), &ClockGearsCharacterStateRegistry::get_actor_slot_ids);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("get_category_slot_ids", "category", "movement_directions", "directional_idle", "combat_mode"), &ClockGearsCharacterStateRegistry::get_category_slot_ids);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("get_category_slot_ids_by_key", "category_key", "movement_directions", "directional_idle", "combat_mode"), &ClockGearsCharacterStateRegistry::get_category_slot_ids_by_key);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("is_actor_fixed_directional_slot", "slot_id"), &ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot);
	ClassDB::bind_static_method("ClockGearsCharacterStateRegistry", D_METHOD("get_missing_question_marker_texture"), &ClockGearsCharacterStateRegistry::get_missing_question_marker_texture);

	ClassDB::bind_method(D_METHOD("register_state", "id", "slot_prefix", "blocks_movement", "fallback_state_id", "fallback_freeze_frame0", "default_loop"),
			&ClockGearsCharacterStateRegistry::register_state, DEFVAL(false), DEFVAL(""), DEFVAL(false), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("has_state", "id"), &ClockGearsCharacterStateRegistry::has_state);
	ClassDB::bind_method(D_METHOD("get_state_info", "id"), &ClockGearsCharacterStateRegistry::get_state_info);
	ClassDB::bind_method(D_METHOD("does_state_block_movement", "id"), &ClockGearsCharacterStateRegistry::does_state_block_movement);
	ClassDB::bind_method(D_METHOD("get_registered_state_ids"), &ClockGearsCharacterStateRegistry::get_registered_state_ids);
	ClassDB::bind_method(D_METHOD("resolve_slot_for_state", "state_id", "direction"), &ClockGearsCharacterStateRegistry::resolve_slot_for_state);
}

bool ClockGearsCharacterStateRegistry::is_valid_direction(int p_dir) {
	return p_dir == 1 || p_dir == 2 || p_dir == 3 || p_dir == 4 || p_dir == 6 || p_dir == 7 || p_dir == 8 || p_dir == 9;
}

bool ClockGearsCharacterStateRegistry::is_diagonal_direction(int p_dir) {
	return p_dir == 1 || p_dir == 3 || p_dir == 7 || p_dir == 9;
}

String ClockGearsCharacterStateRegistry::direction_to_suffix(int p_dir) {
	switch (p_dir) {
		case 1: return "abajo_izquierda";
		case 2: return "abajo";
		case 3: return "abajo_derecha";
		case 4: return "izquierda";
		case 6: return "derecha";
		case 7: return "arriba_izquierda";
		case 8: return "arriba";
		case 9: return "arriba_derecha";
		default: return "abajo";
	}
}

int ClockGearsCharacterStateRegistry::direction_to_nearest_horizontal(int p_dir) {
	switch (p_dir) {
		case 1:
		case 4:
		case 7:
			return 4; // Izquierda
		case 3:
		case 6:
		case 9:
			return 6; // Derecha
		default:
			return 0;
	}
}

int ClockGearsCharacterStateRegistry::direction_to_sheet_row(int p_dir, int p_rows) {
	if (p_rows >= 8) {
		// Hoja opcional de 8 filas: 0..3 estándar + 4..7 diagonales
		switch (p_dir) {
			case 2: return 0; // Abajo
			case 4: return 1; // Izquierda
			case 6: return 2; // Derecha
			case 8: return 3; // Arriba
			case 1: return 4; // Abajo-Izquierda
			case 3: return 5; // Abajo-Derecha
			case 7: return 6; // Arriba-Izquierda
			case 9: return 7; // Arriba-Derecha
			default: return 0;
		}
	}
	// Hoja estándar de 4 filas: 0=Abajo, 1=Izquierda, 2=Derecha, 3=Arriba.
	// Diagonales usan la pose horizontal más cercana (1,7->Izq; 3,9->Der).
	switch (p_dir) {
		case 2: return 0;
		case 4:
		case 1:
		case 7:
			return 1;
		case 6:
		case 3:
		case 9:
			return 2;
		case 8: return 3;
		default: return 0;
	}
}

int ClockGearsCharacterStateRegistry::vector_to_direction(const Vector2i &p_vec) {
	int dx = (p_vec.x > 0) ? 1 : ((p_vec.x < 0) ? -1 : 0);
	int dy = (p_vec.y > 0) ? 1 : ((p_vec.y < 0) ? -1 : 0);
	if (dx < 0 && dy > 0) return 1;
	if (dx == 0 && dy > 0) return 2;
	if (dx > 0 && dy > 0) return 3;
	if (dx < 0 && dy == 0) return 4;
	if (dx > 0 && dy == 0) return 6;
	if (dx < 0 && dy < 0) return 7;
	if (dx == 0 && dy < 0) return 8;
	if (dx > 0 && dy < 0) return 9;
	return 2;
}

static void s_append_directional_slots(PackedStringArray &r_out, const String &p_prefix, bool p_allow8) {
	static const char *card_suffixes[4] = { "abajo", "izquierda", "derecha", "arriba" };
	static const char *diag_suffixes[4] = { "abajo_izquierda", "abajo_derecha", "arriba_izquierda", "arriba_derecha" };
	for (int i = 0; i < 4; i++) {
		r_out.push_back(p_prefix + "_" + card_suffixes[i]);
	}
	if (p_allow8) {
		for (int i = 0; i < 4; i++) {
			r_out.push_back(p_prefix + "_" + diag_suffixes[i]);
		}
	}
}

PackedStringArray ClockGearsCharacterStateRegistry::get_actor_slot_ids(int p_movement_directions, bool p_directional_idle) {
	return get_category_slot_ids(CAT_ACTORS, p_movement_directions, p_directional_idle, COMBAT_MODE_TURN_BASED);
}

PackedStringArray ClockGearsCharacterStateRegistry::get_category_slot_ids(int p_category, int p_movement_directions, bool p_directional_idle, int p_combat_mode) {
	PackedStringArray out;
	const bool allow8 = (p_movement_directions == 8);
	const bool is_arpg = (p_combat_mode == COMBAT_MODE_ACTION_RPG);

	bool has_move = false;
	bool has_run = false;
	bool has_attack = false;
	bool has_block = false;

	switch (p_category) {
		case CAT_ACTORS:
			has_move = true;
			has_run = true;
			has_attack = is_arpg;
			has_block = is_arpg;
			break;
		case CAT_NPCS:
			has_move = true;
			break;
		case CAT_ARPG_ENEMIES:
		case CAT_ARPG_NPCS:
			has_move = true;
			has_attack = is_arpg;
			break;
		case CAT_ARPG_WEAPONS:
		case CAT_ARPG_ARMORS:
		case CAT_ARPG_SHIELDS:
		case CAT_ARPG_ACCESSORIES:
			has_move = true;
			has_attack = is_arpg;
			has_block = is_arpg;
			break;
		default:
			return out;
	}

	if (has_move) {
		s_append_directional_slots(out, "mover", allow8);
	}
	if (has_run) {
		s_append_directional_slots(out, "correr", allow8);
	}
	if (p_directional_idle) {
		s_append_directional_slots(out, "quieto", allow8);
	}
	if (has_attack) {
		s_append_directional_slots(out, "atacar", allow8);
	}
	if (has_block) {
		s_append_directional_slots(out, "bloquear", allow8);
	}
	return out;
}

PackedStringArray ClockGearsCharacterStateRegistry::get_category_slot_ids_by_key(const String &p_category_key, int p_movement_directions, bool p_directional_idle, int p_combat_mode) {
	int cat = ClockGearsDataManager::get_category_from_key(p_category_key);
	if (cat < 0) {
		return PackedStringArray();
	}
	return get_category_slot_ids(cat, p_movement_directions, p_directional_idle, p_combat_mode);
}

bool ClockGearsCharacterStateRegistry::is_actor_fixed_directional_slot(const String &p_slot_id) {
	String low = p_slot_id.strip_edges().to_lower();
	PackedStringArray all_slots;
	s_append_directional_slots(all_slots, "mover", true);
	s_append_directional_slots(all_slots, "correr", true);
	s_append_directional_slots(all_slots, "quieto", true);
	s_append_directional_slots(all_slots, "atacar", true);
	s_append_directional_slots(all_slots, "bloquear", true);
	for (int i = 0; i < all_slots.size(); i++) {
		if (all_slots[i] == low) {
			return true;
		}
	}
	return false;
}

static Ref<ImageTexture> s_missing_question_marker_tex;

Ref<Texture2D> ClockGearsCharacterStateRegistry::get_missing_question_marker_texture() {
	if (s_missing_question_marker_tex.is_valid()) {
		return s_missing_question_marker_tex;
	}
	Ref<Image> img = Image::create_empty(32, 32, false, Image::FORMAT_RGBA8);
	Color bg(0.14f, 0.16f, 0.21f, 1.0f);
	Color border(0.45f, 0.50f, 0.62f, 1.0f);
	Color glyph(0.92f, 0.80f, 0.35f, 1.0f);
	img->fill(bg);
	for (int i = 0; i < 32; i++) {
		img->set_pixel(i, 0, border);
		img->set_pixel(i, 31, border);
		img->set_pixel(0, i, border);
		img->set_pixel(31, i, border);
	}
	// Glifo pixel-art '?' 5x7 escalado a 2x (10x14) centrado en (11, 9)
	static const char *q_bmp[7] = {
		" ### ",
		"#   #",
		"   # ",
		"  #  ",
		"  #  ",
		"     ",
		"  #  "
	};
	for (int gy = 0; gy < 7; gy++) {
		for (int gx = 0; gx < 5; gx++) {
			if (q_bmp[gy][gx] == '#') {
				for (int dy = 0; dy < 2; dy++) {
					for (int dx = 0; dx < 2; dx++) {
						img->set_pixel(11 + gx * 2 + dx, 9 + gy * 2 + dy, glyph);
					}
				}
			}
		}
	}
	s_missing_question_marker_tex = ImageTexture::create_from_image(img);
	return s_missing_question_marker_tex;
}

void ClockGearsCharacterStateRegistry::register_state(
		const String &p_id,
		const String &p_slot_prefix,
		bool p_blocks_movement,
		const String &p_fallback_state_id,
		bool p_fallback_freeze_frame0,
		bool p_default_loop) {
	String clean_id = p_id.strip_edges().to_lower();
	if (clean_id.is_empty()) {
		return;
	}
	ClockGearsCharacterStateDef def;
	def.id = clean_id;
	def.slot_prefix = p_slot_prefix.strip_edges().to_lower();
	def.blocks_movement = p_blocks_movement;
	def.fallback_state_id = p_fallback_state_id.strip_edges().to_lower();
	def.fallback_freeze_frame0 = p_fallback_freeze_frame0;
	def.default_loop = p_default_loop;

	if (!states.has(clean_id)) {
		state_order.push_back(clean_id);
	}
	states[clean_id] = def;
}

bool ClockGearsCharacterStateRegistry::has_state(const String &p_id) const {
	return states.has(p_id.strip_edges().to_lower());
}

Dictionary ClockGearsCharacterStateRegistry::get_state_info(const String &p_id) const {
	String k = p_id.strip_edges().to_lower();
	if (!states.has(k)) {
		return Dictionary();
	}
	return states[k].to_dict();
}

bool ClockGearsCharacterStateRegistry::does_state_block_movement(const String &p_id) const {
	String k = p_id.strip_edges().to_lower();
	if (!states.has(k)) {
		return false;
	}
	return states[k].blocks_movement;
}

PackedStringArray ClockGearsCharacterStateRegistry::get_registered_state_ids() const {
	PackedStringArray out;
	for (int i = 0; i < state_order.size(); i++) {
		out.push_back(state_order[i]);
	}
	return out;
}

String ClockGearsCharacterStateRegistry::resolve_slot_for_state(const String &p_state_id, int p_direction) const {
	String k = p_state_id.strip_edges().to_lower();
	if (!states.has(k)) {
		return "";
	}
	return states[k].slot_prefix + "_" + direction_to_suffix(p_direction);
}

bool ClockGearsCharacterStateRegistry::resolve_actor_animation(
		const ClockGearsSpriteSheetData &p_sheet,
		bool p_directional_idle,
		const String &p_state_id,
		int p_direction,
		ClockGearsSpriteAnimation &r_anim,
		bool &r_freeze_frame0,
		String &r_resolved_slot) const {
	r_freeze_frame0 = false;
	r_resolved_slot = "";
	String cur_state = p_state_id.strip_edges().to_lower();
	if (cur_state.is_empty()) {
		cur_state = "quieto";
	}

	int depth = 0;
	while (!cur_state.is_empty() && depth < 8) {
		depth++;
		if (!states.has(cur_state)) {
			break;
		}
		const ClockGearsCharacterStateDef &def = states[cur_state];

		// Si "Idle direccional" está desactivado, quieto ignora las ranuras quieto_* y cae al frame 0 de caminar
		bool skip_own_slots = (cur_state == "quieto" && !p_directional_idle);
		if (!skip_own_slots && !def.slot_prefix.is_empty()) {
			String exact_key = def.slot_prefix + "_" + direction_to_suffix(p_direction);
			if (p_sheet.animations.has(exact_key) && p_sheet.animations[exact_key].get_frame_count() > 0) {
				r_anim = p_sheet.animations[exact_key];
				r_resolved_slot = exact_key;
				return true;
			}
			if (is_diagonal_direction(p_direction)) {
				int horiz_dir = direction_to_nearest_horizontal(p_direction);
				if (horiz_dir != 0) {
					String horiz_key = def.slot_prefix + "_" + direction_to_suffix(horiz_dir);
					if (p_sheet.animations.has(horiz_key) && p_sheet.animations[horiz_key].get_frame_count() > 0) {
						r_anim = p_sheet.animations[horiz_key];
						r_resolved_slot = horiz_key;
						return true;
					}
				}
			}
		}

		if (def.fallback_freeze_frame0) {
			r_freeze_frame0 = true;
		}
		cur_state = def.fallback_state_id;
	}
	return false;
}

ClockGearsCharacterStateRegistry::ClockGearsCharacterStateRegistry() {
	singleton = this;
	// Registrar exclusivamente estados del motor y ranuras ARPG por defecto
	register_state("quieto", "quieto", false, "caminar", true, true);
	register_state("caminar", "mover", false, "", false, true);
	register_state("correr", "correr", false, "caminar", false, true);
	register_state("atacar", "atacar", false, "", false, false);
	register_state("bloquear", "bloquear", false, "", false, true);
}

ClockGearsCharacterStateRegistry::~ClockGearsCharacterStateRegistry() {
	s_missing_question_marker_tex.unref();
	if (singleton == this) {
		singleton = nullptr;
	}
}
