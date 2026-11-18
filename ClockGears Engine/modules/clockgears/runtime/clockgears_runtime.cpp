#include "clockgears_runtime.h"
#include "clockgears_credits.h"
#include "clockgears_game_over.h"
#include "clockgears_entity_ai.h"
#include "clockgears_game_menu.h"
#include "clockgears_hud.h"
#include "clockgears_title_screen.h"
#include "../clockgears_battle_window.h"
#include "../clockgears_debug_panel.h"
#include "../core/clockgears_data_manager.h"
#include "../core/clockgears_formulas.h"
#include "../core/clockgears_system_catalog.h"
#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/style_box_flat.h"

#define CG_RT_TR(m_str) String::utf8(m_str)

static void _cg_collect_events_recursive(Node *p_node, Vector<ClockGearsEvent *> &r_events) {
	if (!p_node) {
		return;
	}
	ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(p_node);
	if (ev) {
		r_events.push_back(ev);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_cg_collect_events_recursive(p_node->get_child(i), r_events);
	}
}

static Vector<ClockGearsEvent *> _cg_get_map_events(ClockGearsMap *p_map) {
	Vector<ClockGearsEvent *> list;
	if (!p_map) {
		return list;
	}
	_cg_collect_events_recursive(p_map, list);
	return list;
}

static Ref<Texture2D> _cg_load_runtime_texture(const String &p_path) {
	if (p_path.is_empty()) {
		return Ref<Texture2D>();
	}
	if (ResourceLoader::exists(p_path)) {
		Ref<Texture2D> res = ResourceLoader::load(p_path);
		if (res.is_valid()) {
			return res;
		}
	}
	Ref<Image> img = Image::load_from_file(p_path);
	if (img.is_valid() && !img->is_empty()) {
		return ImageTexture::create_from_image(img);
	}
	return Ref<Texture2D>();
}

static Ref<Texture2D> _cg_make_pixel_char_sheet(const Color &p_body, const Color &p_hair) {
	// Genera una hoja 3x4 (96x128, celdas de 32x32) pixel-perfect cuando el actor aún no tiene PNG asignado
	const int w = 96;
	const int h = 128;
	Ref<Image> img = Image::create_empty(w, h, false, Image::FORMAT_RGBA8);
	img->fill(Color(0, 0, 0, 0));

	Color outline(0.08f, 0.08f, 0.10f, 1.0f);
	Color skin(0.96f, 0.82f, 0.68f, 1.0f);
	Color eye(0.12f, 0.14f, 0.20f, 1.0f);

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 3; col++) {
			int ox = col * 32;
			int oy = row * 32;
			int step_shift = (col == 0) ? -1 : ((col == 2) ? 1 : 0);

			// Sombra bajo los pies
			for (int x = 9; x <= 22; x++) {
				for (int y = 27; y <= 30; y++) {
					img->set_pixel(ox + x, oy + y, Color(0, 0, 0, 0.35f));
				}
			}
			// Piernas / botas (con paso animado)
			int left_leg_y = 22 + (step_shift < 0 ? -1 : 0);
			int right_leg_y = 22 + (step_shift > 0 ? -1 : 0);
			for (int x = 11; x <= 14; x++) {
				for (int y = left_leg_y; y <= 27; y++) {
					img->set_pixel(ox + x, oy + y, outline);
				}
			}
			for (int x = 17; x <= 20; x++) {
				for (int y = right_leg_y; y <= 27; y++) {
					img->set_pixel(ox + x, oy + y, outline);
				}
			}
			// Torso / túnica
			for (int x = 9; x <= 22; x++) {
				for (int y = 14; y <= 22; y++) {
					bool border = (x == 9 || x == 22 || y == 14 || y == 22);
					img->set_pixel(ox + x, oy + y, border ? outline : p_body);
				}
			}
			// Cabeza
			for (int x = 9; x <= 22; x++) {
				for (int y = 4; y <= 14; y++) {
					bool border = (x == 9 || x == 22 || y == 4 || y == 14);
					Color c = skin;
					if (border) {
						c = outline;
					} else if (y <= 8 || (row == 3 && y <= 12)) {
						c = p_hair;
					}
					img->set_pixel(ox + x, oy + y, c);
				}
			}
			// Ojos según dirección (row 0=Abajo, 1=Izquierda, 2=Derecha, 3=Arriba)
			if (row == 0) {
				img->set_pixel(ox + 13, oy + 10, eye);
				img->set_pixel(ox + 13, oy + 11, eye);
				img->set_pixel(ox + 18, oy + 10, eye);
				img->set_pixel(ox + 18, oy + 11, eye);
			} else if (row == 1) {
				img->set_pixel(ox + 11, oy + 10, eye);
				img->set_pixel(ox + 11, oy + 11, eye);
			} else if (row == 2) {
				img->set_pixel(ox + 20, oy + 10, eye);
				img->set_pixel(ox + 20, oy + 11, eye);
			}
		}
	}
	return ImageTexture::create_from_image(img);
}

// ============================================================================
// ClockGearsPlayer
// ============================================================================

void ClockGearsPlayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_grid_cell_immediate", "cell", "dir"), &ClockGearsPlayer::set_grid_cell_immediate, DEFVAL(2));
	ClassDB::bind_method(D_METHOD("set_world_position_immediate", "pos", "dir"), &ClockGearsPlayer::set_world_position_immediate, DEFVAL(2));
	ClassDB::bind_method(D_METHOD("get_grid_cell"), &ClockGearsPlayer::get_grid_cell);
	ClassDB::bind_method(D_METHOD("set_direction", "dir"), &ClockGearsPlayer::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &ClockGearsPlayer::get_direction);
	ClassDB::bind_method(D_METHOD("set_allow_diagonal", "allow"), &ClockGearsPlayer::set_allow_diagonal);
	ClassDB::bind_method(D_METHOD("is_allow_diagonal"), &ClockGearsPlayer::is_allow_diagonal);
	ClassDB::bind_method(D_METHOD("set_through_enabled", "through"), &ClockGearsPlayer::set_through_enabled);
	ClassDB::bind_method(D_METHOD("is_through_enabled"), &ClockGearsPlayer::is_through_enabled);
	ClassDB::bind_method(D_METHOD("set_walk_speed", "speed"), &ClockGearsPlayer::set_walk_speed);
	ClassDB::bind_method(D_METHOD("get_walk_speed"), &ClockGearsPlayer::get_walk_speed);
	ClassDB::bind_method(D_METHOD("get_dash_speed"), &ClockGearsPlayer::get_dash_speed);
	ClassDB::bind_method(D_METHOD("get_walk_speed_pct"), &ClockGearsPlayer::get_walk_speed_pct);
	ClassDB::bind_method(D_METHOD("get_run_speed_pct"), &ClockGearsPlayer::get_run_speed_pct);
	ClassDB::bind_method(D_METHOD("get_attack_speed_pct"), &ClockGearsPlayer::get_attack_speed_pct);
	ClassDB::bind_method(D_METHOD("is_moving_step"), &ClockGearsPlayer::is_moving_step);
	ClassDB::bind_method(D_METHOD("try_step", "dir", "is_dash"), &ClockGearsPlayer::try_step, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_front_cell", "candidate_idx"), &ClockGearsPlayer::get_front_cell, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_front_cell_count"), &ClockGearsPlayer::get_front_cell_count);
	ClassDB::bind_method(D_METHOD("try_activate_front_event"), &ClockGearsPlayer::try_activate_front_event);
	ClassDB::bind_method(D_METHOD("lock_action_key"), &ClockGearsPlayer::lock_action_key);
	ClassDB::bind_method(D_METHOD("unlock_action_key"), &ClockGearsPlayer::unlock_action_key);
	ClassDB::bind_method(D_METHOD("is_action_key_locked"), &ClockGearsPlayer::is_action_key_locked);
	ClassDB::bind_method(D_METHOD("check_touch_events_at", "cell"), &ClockGearsPlayer::check_touch_events_at);
	ClassDB::bind_method(D_METHOD("set_action_state", "state_id"), &ClockGearsPlayer::set_action_state);
	ClassDB::bind_method(D_METHOD("get_action_state"), &ClockGearsPlayer::get_action_state);
	ClassDB::bind_method(D_METHOD("is_action_state_blocking_movement"), &ClockGearsPlayer::is_action_state_blocking_movement);
	ClassDB::bind_method(D_METHOD("get_active_visual_slot"), &ClockGearsPlayer::get_active_visual_slot);
	ClassDB::bind_method(D_METHOD("is_using_freeze_frame0"), &ClockGearsPlayer::is_using_freeze_frame0);
	ClassDB::bind_method(D_METHOD("get_active_slot_frame_index"), &ClockGearsPlayer::get_active_slot_frame_index);
	ClassDB::bind_method(D_METHOD("refresh_arpg_equip_layers"), &ClockGearsPlayer::refresh_arpg_equip_layers);
	ClassDB::bind_method(D_METHOD("get_active_arpg_equip_layer_count"), &ClockGearsPlayer::get_active_arpg_equip_layer_count);
	ClassDB::bind_method(D_METHOD("get_arpg_equip_layer_info"), &ClockGearsPlayer::get_arpg_equip_layer_info);
	ClassDB::bind_method(D_METHOD("debug_advance_animation", "delta"), &ClockGearsPlayer::debug_advance_animation);
	ClassDB::bind_method(D_METHOD("load_actor_data", "actor_id"), &ClockGearsPlayer::load_actor_data);
	ClassDB::bind_method(D_METHOD("get_actor_id"), &ClockGearsPlayer::get_actor_id);
	ClassDB::bind_method(D_METHOD("get_arpg_hp"), &ClockGearsPlayer::get_arpg_hp);
	ClassDB::bind_method(D_METHOD("get_arpg_max_hp"), &ClockGearsPlayer::get_arpg_max_hp);
	ClassDB::bind_method(D_METHOD("get_arpg_level"), &ClockGearsPlayer::get_arpg_level);
	ClassDB::bind_method(D_METHOD("get_arpg_exp"), &ClockGearsPlayer::get_arpg_exp);
	ClassDB::bind_method(D_METHOD("get_arpg_hitstop_remaining"), &ClockGearsPlayer::get_arpg_hitstop_remaining);
	ClassDB::bind_method(D_METHOD("get_arpg_world_center"), &ClockGearsPlayer::get_arpg_world_center);
	ClassDB::bind_method(D_METHOD("get_arpg_effective_hurtbox"), &ClockGearsPlayer::get_arpg_effective_hurtbox);
	ClassDB::bind_method(D_METHOD("is_arpg_invulnerable"), &ClockGearsPlayer::is_arpg_invulnerable);
	ClassDB::bind_method(D_METHOD("get_arpg_invuln_remaining"), &ClockGearsPlayer::get_arpg_invuln_remaining);
	ClassDB::bind_method(D_METHOD("get_arpg_invuln_blink_alpha"), &ClockGearsPlayer::get_arpg_invuln_blink_alpha);
	ClassDB::bind_method(D_METHOD("is_arpg_blocking"), &ClockGearsPlayer::is_arpg_blocking);
	ClassDB::bind_method(D_METHOD("is_arpg_dodging"), &ClockGearsPlayer::is_arpg_dodging);
	ClassDB::bind_method(D_METHOD("is_arpg_dodge_invulnerable"), &ClockGearsPlayer::is_arpg_dodge_invulnerable);
	ClassDB::bind_method(D_METHOD("get_arpg_dodge_cooldown_remaining"), &ClockGearsPlayer::get_arpg_dodge_cooldown_remaining);
	ClassDB::bind_method(D_METHOD("is_arpg_dying"), &ClockGearsPlayer::is_arpg_dying);
	ClassDB::bind_method(D_METHOD("is_arpg_dead"), &ClockGearsPlayer::is_arpg_dead);
	ClassDB::bind_method(D_METHOD("is_arpg_alive"), &ClockGearsPlayer::is_arpg_alive);
	ClassDB::bind_method(D_METHOD("try_trigger_dodge", "dir"), &ClockGearsPlayer::try_trigger_dodge, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("take_arpg_hit", "damage", "knockback_force", "hit_dir", "hitstop_ms", "is_crit", "attacker_name"), &ClockGearsPlayer::take_arpg_hit, DEFVAL(0.0f), DEFVAL(Vector2()), DEFVAL(0), DEFVAL(false), DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("reset_after_arpg_death", "cell", "dir"), &ClockGearsPlayer::reset_after_arpg_death);
	ClassDB::bind_method(D_METHOD("reload_actor_arpg_data"), &ClockGearsPlayer::reload_actor_arpg_data);
	ClassDB::bind_method(D_METHOD("is_arpg_active_here"), &ClockGearsPlayer::is_arpg_active_here);
	ClassDB::bind_method(D_METHOD("setup_on_map", "map", "followers", "interpreter"), &ClockGearsPlayer::setup_on_map, DEFVAL(Variant()), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("set_body_shape", "shape"), &ClockGearsPlayer::set_body_shape);
	ClassDB::bind_method(D_METHOD("get_body_shape"), &ClockGearsPlayer::get_body_shape);
	ClassDB::bind_method(D_METHOD("get_body_local_origin"), &ClockGearsPlayer::get_body_local_origin);
	ClassDB::bind_method(D_METHOD("get_body_world_origin"), &ClockGearsPlayer::get_body_world_origin);
	ClassDB::bind_method(D_METHOD("set_body_through", "through"), &ClockGearsPlayer::set_body_through);
	ClassDB::bind_method(D_METHOD("is_body_through"), &ClockGearsPlayer::is_body_through);
	ClassDB::bind_method(D_METHOD("move_with_collision", "delta"), &ClockGearsPlayer::move_with_collision);
	ClassDB::bind_method(D_METHOD("can_pass", "from", "to", "dir"), &ClockGearsPlayer::can_pass);
	ClassDB::bind_static_method("ClockGearsPlayer", D_METHOD("ensure_default_arpg_input_actions"), &ClockGearsPlayer::ensure_default_arpg_input_actions);
	ClassDB::bind_static_method("ClockGearsPlayer", D_METHOD("is_arpg_combat_mode"), &ClockGearsPlayer::is_arpg_combat_mode);
	ClassDB::bind_method(D_METHOD("is_arpg_action_allowed"), &ClockGearsPlayer::is_arpg_action_allowed);

	ADD_SIGNAL(MethodInfo("step_started", PropertyInfo(Variant::VECTOR2I, "from_cell"), PropertyInfo(Variant::VECTOR2I, "to_cell"), PropertyInfo(Variant::INT, "direction")));
	ADD_SIGNAL(MethodInfo("step_finished", PropertyInfo(Variant::VECTOR2I, "cell")));
	ADD_SIGNAL(MethodInfo("jugador_muerto"));
}

Vector2i ClockGearsPlayer::direction_to_vector(int p_dir) {
	switch (p_dir) {
		case 2: return Vector2i(0, 1);   // Abajo
		case 4: return Vector2i(-1, 0);  // Izquierda
		case 6: return Vector2i(1, 0);   // Derecha
		case 8: return Vector2i(0, -1);  // Arriba
		case 1: return Vector2i(-1, 1);  // Diagonal Abajo-Izq
		case 3: return Vector2i(1, 1);   // Diagonal Abajo-Der
		case 7: return Vector2i(-1, -1); // Diagonal Arriba-Izq
		case 9: return Vector2i(1, -1);  // Diagonal Arriba-Der
		default: return Vector2i(0, 1);
	}
}

int ClockGearsPlayer::opposite_direction(int p_dir) {
	switch (p_dir) {
		case 2: return 8;
		case 8: return 2;
		case 4: return 6;
		case 6: return 4;
		case 1: return 9;
		case 9: return 1;
		case 3: return 7;
		case 7: return 3;
		default: return 2;
	}
}

int ClockGearsPlayer::_direction_to_row(int p_dir) const {
	return ClockGearsCharacterStateRegistry::direction_to_sheet_row(p_dir, sprite_rows);
}

Ref<Texture2D> ClockGearsPlayer::_get_cached_slot_texture(const String &p_path) {
	if (p_path.is_empty()) {
		return Ref<Texture2D>();
	}
	if (cached_slot_textures.has(p_path)) {
		return cached_slot_textures[p_path];
	}
	Ref<Texture2D> tex = _cg_load_runtime_texture(p_path);
	cached_slot_textures[p_path] = tex;
	return tex;
}

void ClockGearsPlayer::_begin_attack_swing() {
	if (swing_tracker.is_null()) {
		swing_tracker.instantiate();
	}
	if (current_swing_id > 0) {
		swing_tracker->end_swing(current_swing_id);
	}
	current_swing_id = swing_tracker->start_new_swing();
}

void ClockGearsPlayer::_end_attack_swing() {
	if (swing_tracker.is_valid() && current_swing_id > 0) {
		swing_tracker->end_swing(current_swing_id);
	}
	current_swing_id = 0;
}

void ClockGearsPlayer::set_action_state(const String &p_state_id) {
	String clean = p_state_id.strip_edges().to_lower();
	if (clean.is_empty()) {
		clean = "quieto";
	}
	if ((arpg_dying || arpg_dead) && clean != "morir") {
		return;
	}
	if (clean == "atacar" && (action_state_id == "bloquear" || arpg_block_held || action_state_id == "esquivar")) {
		return;
	}
	if (clean == "bloquear" && (action_state_id == "atacar" || action_state_id == "esquivar")) {
		return;
	}
	if (clean == "esquivar" && action_state_id == "atacar") {
		return;
	}
	if (action_state_id != clean) {
		if (action_state_id == "atacar") {
			_end_attack_swing();
		}
		if (clean != "esquivar" && arpg_dodge_timer > 0.0f) {
			arpg_dodge_timer = 0.0f;
			arpg_dodge_invuln = false;
			arpg_dodge_velocity = Vector2();
		}
		action_state_id = clean;
		slot_anim_timer = 0.0f;
		slot_anim_frame_idx = 0;
		if (clean == "atacar") {
			_begin_attack_swing();
		}
		_update_sprite_frame();
		if (clean == "atacar") {
			_process_active_attack_hits();
		}
	}
}

bool ClockGearsPlayer::is_action_state_blocking_movement() const {
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (!reg) {
		return false;
	}
	return reg->does_state_block_movement(action_state_id);
}

void ClockGearsPlayer::_sync_locomotion_state() {
	if (action_state_id != "quieto" && action_state_id != "caminar" && action_state_id != "correr") {
		return; // Respetar estados de acción personalizados o de combate (atacar / bloquear)
	}
	String next_state = "quieto";
	if (moving_step) {
		next_state = dashing ? "correr" : "caminar";
	}
	if (action_state_id != next_state) {
		action_state_id = next_state;
		slot_anim_timer = 0.0f;
		slot_anim_frame_idx = 0;
	}
}

void ClockGearsPlayer::_end_temporary_action_to_locomotion() {
	_end_attack_swing();
	action_state_id = moving_step ? (dashing ? "correr" : "caminar") : "quieto";
	slot_anim_timer = 0.0f;
	slot_anim_frame_idx = 0;
	_update_sprite_frame();
}

void ClockGearsPlayer::_advance_slot_animation(float p_delta) {
	if (hitstop_timer > 0.0f) {
		if (p_delta <= hitstop_timer) {
			hitstop_timer -= p_delta;
			return;
		}
		p_delta -= hitstop_timer;
		hitstop_timer = 0.0f;
	}
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (!reg) {
		if (arpg_dying && action_state_id == "morir") {
			arpg_death_fallback_timer -= p_delta;
			if (arpg_death_fallback_timer <= 0.0f) {
				_finish_arpg_player_death();
			}
		}
		return;
	}
	ClockGearsSpriteAnimation anim;
	bool freeze_f0 = false;
	String res_slot;
	if (!reg->resolve_actor_animation(actor_sprite_data, directional_idle, action_state_id, direction, anim, freeze_f0, res_slot)) {
		if (action_state_id == "atacar") {
			_end_temporary_action_to_locomotion();
		} else if (arpg_dying && action_state_id == "morir") {
			arpg_death_fallback_timer -= p_delta;
			if (arpg_death_fallback_timer <= 0.0f) {
				_finish_arpg_player_death();
			}
		}
		return;
	}
	int total = anim.get_frame_count();
	if (total <= 0) {
		if (action_state_id == "atacar") {
			_end_temporary_action_to_locomotion();
		} else if (arpg_dying && action_state_id == "morir") {
			arpg_death_fallback_timer -= p_delta;
			if (arpg_death_fallback_timer <= 0.0f) {
				_finish_arpg_player_death();
			}
		}
		return;
	}
	const bool is_attack = (action_state_id == "atacar");
	const bool is_die = (action_state_id == "morir" && arpg_dying);
	if (freeze_f0 || (total <= 1 && !is_attack && !is_die)) {
		if (is_die) {
			arpg_death_fallback_timer -= p_delta;
			if (arpg_death_fallback_timer <= 0.0f) {
				_finish_arpg_player_death();
			}
		}
		return;
	}
	int cur_idx = CLAMP(slot_anim_frame_idx, 0, total - 1);
	float target_dur = anim.get_frame_duration(cur_idx);
	float effective_delta = p_delta;
	if (is_attack && is_arpg_active_here()) {
		effective_delta *= (MAX(1.0f, attack_speed_pct) / 100.0f);
	}
	slot_anim_timer += effective_delta;
	bool changed = false;
	const bool allow_loop = (is_attack || is_die) ? false : anim.loop;
	while (slot_anim_timer >= target_dur && target_dur > 0.0f) {
		slot_anim_timer -= target_dur;
		if (slot_anim_frame_idx + 1 < total) {
			slot_anim_frame_idx++;
			if (is_attack) {
				_update_sprite_frame();
				_process_active_attack_hits();
			}
		} else if (allow_loop) {
			slot_anim_frame_idx = 0;
		} else if (is_die) {
			_finish_arpg_player_death();
			return;
		} else if (is_attack) {
			_end_temporary_action_to_locomotion();
			return;
		} else {
			slot_anim_frame_idx = total - 1;
			slot_anim_timer = 0.0f;
			break;
		}
		changed = true;
		target_dur = anim.get_frame_duration(slot_anim_frame_idx);
	}
	if (changed) {
		_update_sprite_frame();
	}
}

void ClockGearsPlayer::_collect_active_attack_hitboxes(Vector<ClockGearsFrameHitboxData> &r_hitboxes) const {
	r_hitboxes.clear();
	if (action_state_id != "atacar") {
		return;
	}
	int cur_fi = active_freeze_frame0 ? 0 : MAX(0, slot_anim_frame_idx);
	bool any_slot_has_authored_hitbox = false;

	// 1) Hitbox en el propio slot del actor
	if (ClockGearsSpriteAnimation::is_attack_slot_name(active_resolved_slot) && actor_sprite_data.animations.has(active_resolved_slot)) {
		if (actor_sprite_data.has_authored_hitboxes_in_slot(active_resolved_slot)) {
			any_slot_has_authored_hitbox = true;
		}
		const ClockGearsSpriteAnimation &a = actor_sprite_data.animations[active_resolved_slot];
		if (cur_fi >= 0 && cur_fi < a.frames_data.size() && !a.frames_data[cur_fi].hitbox.is_empty()) {
			r_hitboxes.push_back(a.frames_data[cur_fi].hitbox);
		}
	}

	// 2) Hitboxes en el arma ARPG equipada (si define animación de ataque con formas)
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (dm && reg && actor_id > 0) {
		Array slots = dm->get_actor_arpg_equip_slots(actor_id);
		for (int s = 0; s < slots.size(); s++) {
			if (slots[s].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary sd = slots[s];
			int pid = int(sd.get("piece_id", 0));
			if (pid <= 0) {
				continue;
			}
			String slot_id = String(sd.get("slot", "")).strip_edges().to_lower();
			int cat = ClockGearsDataManager::get_category_from_key(String(sd.get("category", "")));
			if (slot_id != "arma" && cat != CAT_ARPG_WEAPONS && cat != CAT_WEAPONS) {
				continue;
			}
			Dictionary piece = dm->get_record_by_id(cat, pid);
			if (piece.is_empty()) {
				continue;
			}
			Dictionary sp_dict;
			if (piece.has("sprite_data") && piece["sprite_data"].get_type() == Variant::DICTIONARY) {
				sp_dict = Dictionary(piece["sprite_data"]);
			} else if (piece.has("animations") && piece["animations"].get_type() == Variant::DICTIONARY) {
				sp_dict["animations"] = Dictionary(piece["animations"]);
			}
			if (sp_dict.is_empty()) {
				continue;
			}
			ClockGearsSpriteSheetData p_sheet;
			p_sheet.from_dict(sp_dict);
			ClockGearsSpriteAnimation p_anim;
			bool p_freeze = false;
			String p_slot;
			if (reg->resolve_actor_animation(p_sheet, false, "atacar", direction, p_anim, p_freeze, p_slot) && !p_freeze && p_anim.get_frame_count() > 0) {
				if (p_sheet.has_authored_hitboxes_in_slot(p_slot)) {
					any_slot_has_authored_hitbox = true;
				}
				int wfi = CLAMP(cur_fi, 0, p_anim.get_frame_count() - 1);
				if (wfi < p_anim.frames_data.size() && !p_anim.frames_data[wfi].hitbox.is_empty()) {
					r_hitboxes.push_back(p_anim.frames_data[wfi].hitbox);
				}
			}
		}
	}

	// 3) Si el slot de ataque no tiene ninguna hitbox dibujada en ningún frame, usar hitbox por defecto frente al personaje.
	if (!any_slot_has_authored_hitbox && r_hitboxes.is_empty()) {
		r_hitboxes.push_back(ClockGearsShapeGeometry::make_default_attack_hitbox_data(direction, tile_size));
	}
}

void ClockGearsPlayer::_process_active_attack_hits() {
	if (action_state_id != "atacar" || !current_map || !is_arpg_active_here()) {
		return;
	}
	if (swing_tracker.is_null() || current_swing_id <= 0) {
		_begin_attack_swing();
	}
	Vector<ClockGearsFrameHitboxData> hitboxes;
	_collect_active_attack_hitboxes(hitboxes);
	if (hitboxes.is_empty()) {
		return;
	}

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	ClockGearsFormulas *fm = ClockGearsFormulas::get_singleton();

	Dictionary eff_stats = (dm && actor_id > 0) ? dm->get_actor_effective_stats(actor_id) : Dictionary();
	Dictionary attacker_dict;
	attacker_dict["atk"] = eff_stats.is_empty() ? 10 : MAX(0, int(eff_stats.get("atk", 10)));
	attacker_dict["def"] = eff_stats.is_empty() ? 0 : MAX(0, int(eff_stats.get("def", 0)));
	attacker_dict["mat"] = eff_stats.is_empty() ? 10 : MAX(0, int(eff_stats.get("mat", 10)));
	attacker_dict["mdf"] = eff_stats.is_empty() ? 10 : MAX(0, int(eff_stats.get("mdf", 10)));
	attacker_dict["luk"] = eff_stats.is_empty() ? 10 : MAX(0, int(eff_stats.get("luk", 10)));

	String atk_elem_key = "fisico";
	int atk_elem_id = (dm && actor_id > 0) ? dm->get_actor_arpg_attack_element_id(actor_id, atk_elem_key) : 0;

	Vector2 atk_origin = get_arpg_world_center();
	Vector<ClockGearsEvent *> events = _cg_get_map_events(current_map);

	for (int ei = 0; ei < events.size(); ei++) {
		ClockGearsEvent *ev = events[ei];
		if (!ev || !ev->is_arpg_alive()) {
			continue;
		}
		int64_t tid = int64_t(ev->get_instance_id());
		if (swing_tracker->has_hit_target(current_swing_id, tid)) {
			continue;
		}

		Dictionary hurt_dict = ev->get_arpg_effective_hurtbox();
		Vector2 target_origin = ev->get_arpg_world_center();

		for (int hi = 0; hi < hitboxes.size(); hi++) {
			const ClockGearsFrameHitboxData &hb = hitboxes[hi];
			Array shapes_arr;
			shapes_arr.resize(hb.shapes.size());
			for (int si = 0; si < hb.shapes.size(); si++) {
				shapes_arr[si] = hb.shapes[si].to_dict();
			}
			if (!ClockGearsShapeGeometry::check_attack_hits_hurtbox(shapes_arr, atk_origin, direction, hurt_dict, target_origin, ev->get_direction(), 0, 0)) {
				continue;
			}

			swing_tracker->register_hit(current_swing_id, tid);

			bool is_crit = false;
			int64_t base_dmg = 1;
			Dictionary defender_dict;
			defender_dict["def"] = ev->get_arpg_def();
			defender_dict["atk"] = ev->get_arpg_atk();

			if (fm) {
				double crit_chance = fm->calculate_critical_chance(double(int(attacker_dict["luk"])) * 0.005, 0.0);
				is_crit = (crit_chance > 0.0) ? (fm->rand_float() <= crit_chance) : false;
				Dictionary ctx;
				ctx["is_critical"] = is_crit;
				ctx["critical_multiplier"] = 2.0;
				ctx["variance"] = 20.0;
				base_dmg = fm->calculate_damage("max(a.atk * 4 - b.def * 2, 0)", attacker_dict, defender_dict, ctx);
			} else {
				base_dmg = MAX(int64_t(0), int64_t(attacker_dict["atk"]) * 4 - int64_t(ev->get_arpg_def()) * 2);
			}
			if (base_dmg <= 0) {
				base_dmg = 1;
			}

			float elem_mult = 1.0f;
			Dictionary en_rec = ev->get_arpg_enemy_record();
			Variant tr_v = en_rec.get("traits", Array());
			if (tr_v.get_type() == Variant::ARRAY) {
				elem_mult = ClockGearsDataManager::evaluate_traits_element_rate(Array(tr_v), atk_elem_id, atk_elem_key);
			}

			double hurt_mult_pct = MAX(0.0, double(hurt_dict.get("damage_mult_pct", 100.0)));
			double hb_mult_pct = MAX(0.0, double(hb.damage_mult_pct));
			double total_dmg = double(base_dmg) * double(elem_mult) * (hb_mult_pct / 100.0) * (hurt_mult_pct / 100.0);
			int final_dmg = (elem_mult <= 0.0f || hb_mult_pct <= 0.0 || hurt_mult_pct <= 0.0) ? 0 : MAX(1, int(Math::round(total_dmg)));

			Vector2 hit_dir = target_origin - atk_origin;
			if (hit_dir.length_squared() < 1.0f) {
				hit_dir = Vector2(direction_to_vector(direction));
			} else {
				hit_dir = hit_dir.normalized();
			}

			ev->take_arpg_hit(final_dmg, hb.knockback, hit_dir, hb.hitstop_ms, is_crit);
			if (hb.hitstop_ms > 0) {
				hitstop_timer = MAX(hitstop_timer, float(hb.hitstop_ms) / 1000.0f);
			}

			String hit_log = vformat("[ARPG] %s -> %s: %d dmg | Elem: %s (x%.2f)%s | HP: %d/%d",
					actor_name.is_empty() ? String("Jugador") : actor_name,
					ev->get_event_name(),
					final_dmg,
					atk_elem_key,
					elem_mult,
					is_crit ? String(" [CRIT]") : String(),
					ev->get_arpg_hp(),
					ev->get_arpg_max_hp());
			print_line(hit_log);
			ClockGearsDebugOverlay::log_battle(hit_log);
			break;
		}
	}
}

void ClockGearsPlayer::_update_sprite_frame() {
	_update_body_sprite_frame();
	_sync_body_collision_shapes();
	refresh_arpg_equip_layers();
}

void ClockGearsPlayer::_update_body_sprite_frame() {
	if (!sprite) {
		return;
	}

	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	ClockGearsSpriteAnimation resolved_anim;
	bool freeze_f0 = false;
	String resolved_slot;
	bool has_slot = reg && reg->resolve_actor_animation(actor_sprite_data, directional_idle, action_state_id, direction, resolved_anim, freeze_f0, resolved_slot);

	if (active_resolved_slot != resolved_slot || active_freeze_frame0 != freeze_f0) {
		active_resolved_slot = resolved_slot;
		active_freeze_frame0 = freeze_f0;
		slot_anim_timer = 0.0f;
		slot_anim_frame_idx = 0;
	}

	if (has_slot && resolved_anim.get_frame_count() > 0) {
		int total = resolved_anim.get_frame_count();
		int fi = active_freeze_frame0 ? 0 : CLAMP(slot_anim_frame_idx, 0, total - 1);
		ClockGearsAnimationFrame raw_f = resolved_anim.get_frame(fi, 0, 0);
		String spath = raw_f.sheet_path;
		if (spath.is_empty()) {
			spath = resolved_anim.spritesheet_path;
		}
		if (spath.is_empty()) {
			spath = graphic_path;
		}
		Ref<Texture2D> tex = _get_cached_slot_texture(spath);
		if (tex.is_valid()) {
			int tw = tex->get_width();
			int th = tex->get_height();
			ClockGearsAnimationFrame f = resolved_anim.get_frame(fi, tw, th);
			Rect2 reg_rect(f.region);
			if (reg_rect.size.x <= 0 || reg_rect.size.y <= 0 || reg_rect.position.x + reg_rect.size.x > tw || reg_rect.position.y + reg_rect.size.y > th) {
				reg_rect = Rect2(0, 0, tw, th);
			}
			sprite->set_texture(tex);
			sprite->set_region_enabled(true);
			sprite->set_region_rect(reg_rect);
			sprite->set_hframes(1);
			sprite->set_vframes(1);
			sprite->set_frame(0);
			sprite->set_position(Vector2((tile_size - reg_rect.size.x) / 2, tile_size - reg_rect.size.y));
			return;
		}
	}

	sprite->set_region_enabled(false);
	Ref<Texture2D> std_tex = fallback_std_texture;
	if (std_tex.is_null()) {
		std_tex = sprite->get_texture();
	}
	if (std_tex.is_null()) {
		std_tex = ClockGearsCharacterStateRegistry::get_missing_question_marker_texture();
		sprite->set_texture(std_tex);
		sprite->set_hframes(1);
		sprite->set_vframes(1);
		sprite->set_frame(0);
		sprite->set_position(Vector2(0, 0));
		return;
	}
	if (sprite->get_texture() != std_tex) {
		sprite->set_texture(std_tex);
	}
	if (!is_spritesheet) {
		sprite->set_hframes(1);
		sprite->set_vframes(1);
		sprite->set_frame(0);
		int fw = std_tex->get_width();
		int fh = std_tex->get_height();
		sprite->set_position(Vector2((tile_size - fw) / 2, tile_size - fh));
		return;
	}
	sprite->set_hframes(sprite_cols);
	sprite->set_vframes(sprite_rows);
	const int walk_cycle[4] = { 0, 1, 2, 1 };
	int idle_col = (sprite_cols >= 3) ? 1 : 0;
	int col = moving_step ? (walk_cycle[anim_pattern_idx % 4] % sprite_cols) : idle_col;
	int row = CLAMP(_direction_to_row(direction), 0, sprite_rows - 1);
	sprite->set_frame(row * sprite_cols + col);

	int frame_h = std_tex->get_height() / sprite_rows;
	int frame_w = std_tex->get_width() / sprite_cols;
	sprite->set_position(Vector2((tile_size - frame_w) / 2, tile_size - frame_h));
}

void ClockGearsPlayer::load_actor_data(int p_actor_id) {
	actor_id = p_actor_id;
	directional_idle = false;
	actor_sprite_data = ClockGearsSpriteSheetData();
	cached_slot_textures.clear();
	active_resolved_slot = "";
	active_freeze_frame0 = false;
	slot_anim_timer = 0.0f;
	slot_anim_frame_idx = 0;
	graphic_path = "";

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && actor_id > 0) {
		Dictionary rec = dm->get_record_by_id(CAT_ACTORS, actor_id);
		if (!rec.is_empty()) {
			actor_name = rec.get("name", "");
			graphic_path = rec.get("graphic_path", "");
			directional_idle = bool(rec.get("directional_idle", false));
			Dictionary bsd = rec.get("body_shape", Dictionary());
			if (!bsd.is_empty()) {
				body_shape.from_dict(bsd);
			} else {
				body_shape = ClockGearsHitShape::make_default_body();
			}
			body_through = bool(rec.get("body_through", false));
			Dictionary sp_dict = rec.get("sprite_data", Dictionary());
			if (!sp_dict.is_empty()) {
				actor_sprite_data.from_dict(sp_dict);
			} else if (rec.has("animations") && rec["animations"].get_type() == Variant::DICTIONARY) {
				Dictionary tmp;
				tmp["animations"] = rec["animations"];
				actor_sprite_data.from_dict(tmp);
			}
		}
	}

	Ref<Texture2D> tex = _cg_load_runtime_texture(graphic_path);
	has_actor_graphic = tex.is_valid();
	if (tex.is_valid()) {
		int tw = tex->get_width();
		int th = tex->get_height();
		bool detected_8 = false;
		if (tw % 3 == 0 && (tw / 3) >= 16 && (tw / 3) * 8 == th) {
			detected_8 = true;
			is_spritesheet = true;
			sprite_rows = 8;
			sprite_cols = 3;
		} else if (tw % 4 == 0 && (tw / 4) >= 16 && (tw / 4) * 8 == th) {
			detected_8 = true;
			is_spritesheet = true;
			sprite_rows = 8;
			sprite_cols = 4;
		}
		if (!detected_8) {
			if (th >= 64 && th % 4 == 0 && (tw % 3 == 0 || tw % 4 == 0)) {
				is_spritesheet = true;
				sprite_rows = 4;
				sprite_cols = (tw % 3 == 0) ? 3 : 4;
			} else {
				is_spritesheet = false;
				sprite_rows = 1;
				sprite_cols = 1;
			}
		}
		fallback_std_texture = tex;
		sprite->set_texture(tex);
	} else {
		is_spritesheet = false;
		sprite_cols = 1;
		sprite_rows = 1;
		fallback_std_texture = ClockGearsCharacterStateRegistry::get_missing_question_marker_texture();
		sprite->set_texture(fallback_std_texture);
	}
	_sync_locomotion_state();
	_refresh_actor_speeds_from_dm();
	_refresh_dodge_input_action();
	_sync_body_collision_shapes();
	_update_sprite_frame();
}

void ClockGearsPlayer::setup_on_map(ClockGearsMap *p_map, ClockGearsFollowerManager *p_followers, ClockGearsEventInterpreter *p_interpreter) {
	current_map = p_map;
	follower_manager = p_followers;
	event_interpreter = p_interpreter;
	if (current_map) {
		tile_size = MAX(16, current_map->get_grid_resolution());
	}
	_refresh_dodge_input_action();
	_sync_body_collision_shapes();
	_update_sprite_frame();
}

void ClockGearsPlayer::_sync_body_collision_shapes() {
	if (body_shape_owner == UINT32_MAX) {
		return;
	}
	ClockGearsShapeGeometry::populate_collision_object_shapes(this, body_shape_owner, body_shape, get_body_local_origin());
}

void ClockGearsPlayer::set_body_shape(const Dictionary &p_shape) {
	body_shape.from_dict(p_shape);
	_sync_body_collision_shapes();
}

Vector2 ClockGearsPlayer::move_with_collision(const Vector2 &p_delta) {
	Vector2 prev_pos = get_position();
	Vector2 next_pos = ClockGearsEntityNav::move_entity_with_slide(current_map, body_shape, direction, prev_pos, p_delta, through_enabled || body_through, this, tile_size);
	set_position(next_pos);
	step_exact_px = next_pos;
	Vector2 center = next_pos + Vector2(tile_size * 0.5f, tile_size * 0.5f);
	grid_cell = Vector2i(int(Math::floor(center.x / float(tile_size))), int(Math::floor(center.y / float(tile_size))));
	return next_pos;
}

void ClockGearsPlayer::debug_advance_animation(float p_delta) {
	if (action_state_id == "atacar") {
		_process_active_attack_hits();
	}
	_advance_slot_animation(p_delta);
	refresh_arpg_equip_layers();
}

int ClockGearsPlayer::get_arpg_hp() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	return (dm && actor_id > 0) ? dm->get_actor_runtime_hp(actor_id) : 0;
}

int ClockGearsPlayer::get_arpg_max_hp() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	return (dm && actor_id > 0) ? dm->get_actor_runtime_max_hp(actor_id) : 1;
}

int ClockGearsPlayer::get_arpg_level() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	return (dm && actor_id > 0) ? dm->get_actor_runtime_level(actor_id) : 1;
}

int64_t ClockGearsPlayer::get_arpg_exp() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	return (dm && actor_id > 0) ? dm->get_actor_runtime_exp(actor_id) : 0;
}

Vector2 ClockGearsPlayer::get_body_local_origin() const {
	if (sprite && sprite->is_region_enabled()) {
		Size2 rsz = sprite->get_region_rect().size;
		if (rsz.x > 0.0f && rsz.y > 0.0f) {
			return sprite->get_position() + rsz * 0.5f;
		}
	}
	if (sprite && sprite->get_texture().is_valid()) {
		int hf = MAX(1, sprite->get_hframes());
		int vf = MAX(1, sprite->get_vframes());
		Size2 fsz(float(sprite->get_texture()->get_width()) / float(hf), float(sprite->get_texture()->get_height()) / float(vf));
		if (fsz.x > 0.0f && fsz.y > 0.0f) {
			return sprite->get_position() + fsz * 0.5f;
		}
	}
	return Vector2(tile_size * 0.5f, tile_size * 0.5f);
}

Vector2 ClockGearsPlayer::get_arpg_world_center() const {
	return get_body_world_origin();
}

Dictionary ClockGearsPlayer::get_arpg_effective_hurtbox() const {
	if (!is_arpg_alive()) {
		ClockGearsHurtboxData off;
		off.enabled = false;
		return off.to_dict();
	}
	int cur_fi = active_freeze_frame0 ? 0 : MAX(0, slot_anim_frame_idx);
	ClockGearsHurtboxData hb = actor_sprite_data.get_effective_hurtbox(active_resolved_slot, cur_fi);
	return hb.to_dict();
}

void ClockGearsPlayer::_sync_grid_cell_from_world_pos(const Vector2 &p_world_pos) {
	int ts = MAX(16, tile_size);
	int cx = int(Math::floor((p_world_pos.x + ts * 0.5f) / float(ts)));
	int cy = int(Math::floor((p_world_pos.y + ts * 0.5f) / float(ts)));
	if (current_map) {
		Vector2i msz = current_map->get_map_size();
		cx = CLAMP(cx, 0, MAX(0, msz.x - 1));
		cy = CLAMP(cy, 0, MAX(0, msz.y - 1));
	}
	grid_cell = Vector2i(cx, cy);
	step_start_px = p_world_pos;
	step_target_px = p_world_pos;
	step_exact_px = p_world_pos;
}

void ClockGearsPlayer::_begin_arpg_player_death() {
	if (arpg_dying || arpg_dead) {
		return;
	}
	arpg_dying = true;
	arpg_block_held = false;
	moving_step = false;
	dashing = false;
	arpg_dodge_timer = 0.0f;
	arpg_dodge_invuln = false;
	arpg_dodge_velocity = Vector2();
	arpg_death_fallback_timer = 0.25f;
	_end_attack_swing();
	action_state_id = "morir";
	slot_anim_frame_idx = 0;
	slot_anim_timer = 0.0f;
	_update_sprite_frame();
	emit_signal("jugador_muerto");
}

void ClockGearsPlayer::_finish_arpg_player_death() {
	if (!arpg_dying && arpg_dead) {
		return;
	}
	arpg_dying = false;
	arpg_dead = true;
	arpg_death_fallback_timer = 0.0f;
	if (current_map && current_map->get_runtime_interpreter()) {
		// Al terminar la animación "morir", se muestra la pantalla de Game Over en lugar del reinicio directo.
		current_map->get_runtime_interpreter()->show_game_over();
	} else if (current_map) {
		current_map->restart_map_after_player_death();
	} else {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (dm && actor_id > 0) {
			dm->set_actor_runtime_hp(actor_id, dm->get_actor_runtime_max_hp(actor_id));
		}
		reset_after_arpg_death(grid_cell, direction);
	}
}

void ClockGearsPlayer::_update_arpg_invuln_blink(float p_dt) {
	if (is_arpg_invulnerable() && is_arpg_alive()) {
		arpg_invuln_blink_timer += MAX(0.0f, p_dt);
		int phase = int(Math::floor(arpg_invuln_blink_timer / 0.06f));
		arpg_invuln_blink_alpha = ((phase % 2) == 0) ? 0.28f : 1.0f;
	} else {
		arpg_invuln_blink_timer = 0.0f;
		arpg_invuln_blink_alpha = 1.0f;
	}
	if (sprite) {
		Color m = sprite->get_modulate();
		m.a = arpg_invuln_blink_alpha;
		sprite->set_modulate(m);
	}
	for (KeyValue<String, Sprite2D *> &E : arpg_layer_sprites) {
		if (E.value) {
			Color lm = E.value->get_modulate();
			lm.a = arpg_invuln_blink_alpha;
			E.value->set_modulate(lm);
		}
	}
}

void ClockGearsPlayer::reset_after_arpg_death(const Vector2i &p_cell, int p_dir) {
	arpg_dying = false;
	arpg_dead = false;
	arpg_death_fallback_timer = 0.0f;
	arpg_invuln_timer = 0.0f;
	arpg_invuln_blink_timer = 0.0f;
	arpg_invuln_blink_alpha = 1.0f;
	arpg_knockback_timer = 0.0f;
	arpg_knockback_velocity = Vector2();
	hitstop_timer = 0.0f;
	arpg_dodge_timer = 0.0f;
	arpg_dodge_cooldown_timer = 0.0f;
	arpg_dodge_velocity = Vector2();
	arpg_dodge_invuln = false;
	arpg_block_held = false;
	_end_attack_swing();
	action_state_id = "quieto";
	slot_anim_frame_idx = 0;
	slot_anim_timer = 0.0f;
	_update_arpg_invuln_blink(0.0f);
	set_grid_cell_immediate(p_cell, p_dir);
}

bool ClockGearsPlayer::try_trigger_dodge(int p_dir) {
	if (!is_arpg_action_allowed()) {
		return false;
	}
	if (arpg_dying || arpg_dead || arpg_knockback_timer > 0.0f || arpg_dodge_timer > 0.0f || arpg_dodge_cooldown_timer > 0.0f) {
		return false;
	}
	// No se puede esquivar a mitad de un ataque
	if (action_state_id == "atacar") {
		return false;
	}

	int dodge_dir = p_dir;
	if (!ClockGearsCharacterStateRegistry::is_valid_direction(dodge_dir)) {
		int held = _get_held_movement_direction();
		dodge_dir = (held != 0) ? held : direction;
	}
	if (!ClockGearsCharacterStateRegistry::is_valid_direction(dodge_dir)) {
		dodge_dir = 2;
	}

	// Soltar bloqueo si estaba bloqueando antes de esquivar
	arpg_block_held = false;
	direction = dodge_dir;
	moving_step = false;
	dashing = false;

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary sys = dm ? dm->get_system_config() : Dictionary();
	String dtype = String(sys.get("arpg_dodge_type", "voltereta")).strip_edges().to_lower();
	float ddur = CLAMP(float(sys.get("arpg_dodge_duration", 0.25)), 0.05f, 5.0f);
	float ddist = CLAMP(float(sys.get("arpg_dodge_distance", 96.0)), 0.0f, 2000.0f);

	Vector2 dv = Vector2(direction_to_vector(dodge_dir));
	if (dv.length_squared() < 0.01f) {
		dv = Vector2(0.0f, 1.0f);
	} else {
		dv = dv.normalized();
	}

	arpg_dodge_timer = ddur;
	arpg_dodge_invuln = (dtype != "dash");
	arpg_dodge_velocity = (ddist > 0.0f) ? (dv * (ddist / ddur)) : Vector2();
	action_state_id = "esquivar";
	slot_anim_frame_idx = 0;
	slot_anim_timer = 0.0f;
	_update_sprite_frame();
	return true;
}

bool ClockGearsPlayer::take_arpg_hit(int p_damage, float p_knockback_force, const Vector2 &p_hit_dir, int p_hitstop_ms, bool p_is_crit, const String &p_attacker_name) {
	if (!is_arpg_alive() || is_arpg_invulnerable()) {
		return false;
	}

	Vector2 hit_dir_n = p_hit_dir;
	if (hit_dir_n.length_squared() < 0.0001f) {
		hit_dir_n = Vector2(direction_to_vector(opposite_direction(direction))).normalized();
	} else {
		hit_dir_n = hit_dir_n.normalized();
	}

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary sys = dm ? dm->get_system_config() : Dictionary();

	bool blocked_frontal = false;
	if (is_arpg_blocking()) {
		Vector2 facing = Vector2(direction_to_vector(direction)).normalized();
		float front_dot = facing.dot(-hit_dir_n);
		if (front_dot > 0.0f) {
			blocked_frontal = true;
		}
	}

	int dmg = MAX(0, p_damage);
	if (blocked_frontal) {
		float red_pct = CLAMP(float(sys.get("arpg_block_reduction_pct", 50.0)), 0.0f, 100.0f);
		dmg = MAX(0, int(Math::round(double(dmg) * (1.0 - double(red_pct) / 100.0))));
	} else if (dmg > 0) {
		dmg = MAX(1, dmg);
	}

	int cur_hp = get_arpg_hp();
	int new_hp = MAX(0, cur_hp - dmg);
	if (dm && actor_id > 0) {
		dm->set_actor_runtime_hp(actor_id, new_hp);
	}

	if (ClockGearsDebugOverlay *dbg = ClockGearsDebugOverlay::get_singleton()) {
		dbg->push_damage_number(get_arpg_world_center() + Vector2(0.0f, -16.0f), dmg, p_is_crit && !blocked_frontal);
	}

	String hit_log = vformat("[ARPG] %s -> %s: %d dmg%s%s | HP: %d/%d",
			p_attacker_name.is_empty() ? String("Enemigo") : p_attacker_name,
			actor_name.is_empty() ? String("Jugador") : actor_name,
			dmg,
			blocked_frontal ? String(" [BLOCK]") : String(),
			(p_is_crit && !blocked_frontal) ? String(" [CRIT]") : String(),
			new_hp,
			get_arpg_max_hp());
	print_line(hit_log);
	ClockGearsDebugOverlay::log_battle(hit_log);

	float inv_sec = sys.has("arpg_invuln_seconds") ? MAX(0.0f, float(sys["arpg_invuln_seconds"])) : 0.5f;
	if (inv_sec > 0.0f) {
		arpg_invuln_timer = inv_sec;
		arpg_invuln_blink_timer = 0.0f;
		_update_arpg_invuln_blink(0.0f);
	}

	if (p_hitstop_ms > 0) {
		hitstop_timer = MAX(hitstop_timer, float(p_hitstop_ms) / 1000.0f);
	}

	float eff_kb = MAX(0.0f, p_knockback_force);
	if (eff_kb > 0.0f) {
		float dur = MAX(0.02f, arpg_knockback_duration);
		moving_step = false;
		dashing = false;
		arpg_dodge_timer = 0.0f;
		arpg_dodge_invuln = false;
		arpg_dodge_velocity = Vector2();
		arpg_knockback_timer = dur;
		arpg_knockback_velocity = hit_dir_n * (eff_kb / dur);
		if (new_hp > 0 && !blocked_frontal) {
			_end_attack_swing();
			action_state_id = "golpeado";
			slot_anim_frame_idx = 0;
			slot_anim_timer = 0.0f;
			_update_sprite_frame();
		}
	}

	if (new_hp <= 0) {
		_begin_arpg_player_death();
	}
	return true;
}

void ClockGearsPlayer::_refresh_actor_speeds_from_dm() {
	walk_speed_pct = 100.0f;
	run_speed_pct = 100.0f;
	attack_speed_pct = 100.0f;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && actor_id > 0) {
		Dictionary rec = dm->get_record_by_id(CAT_ACTORS, actor_id);
		if (!rec.is_empty()) {
			walk_speed_pct = MAX(1.0f, float(rec.get("walk_speed_pct", 100)));
			run_speed_pct = MAX(1.0f, float(rec.get("run_speed_pct", 100)));
			attack_speed_pct = MAX(1.0f, float(rec.get("attack_speed_pct", 100)));
		}
	}
	walk_speed = MAX(8.0f, base_walk_speed * (walk_speed_pct / 100.0f));
	dash_speed = MAX(8.0f, base_dash_speed * (run_speed_pct / 100.0f));
}

void ClockGearsPlayer::_on_dm_data_changed(const String &p_category, int p_id) {
	if (p_category == "actors" && (p_id < 0 || p_id == actor_id)) {
		_refresh_actor_speeds_from_dm();
	}
	if (p_category == "system") {
		_refresh_dodge_input_action();
	}
	if (p_category == "arpg_equip" || p_category == "system" || p_category.begins_with("arpg_")) {
		cached_slot_textures.clear();
		refresh_arpg_equip_layers();
	}
}

void ClockGearsPlayer::refresh_arpg_equip_layers() {
	auto hide_all_layers = [&]() {
		for (KeyValue<String, Sprite2D *> &E : arpg_layer_sprites) {
			if (E.value) {
				E.value->set_visible(false);
			}
		}
	};

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (!dm || !reg || !is_arpg_active_here() || actor_id <= 0) {
		hide_all_layers();
		return;
	}

	if (sprite) {
		sprite->set_z_index(0);
	}

	Dictionary sys_cfg = dm->get_system_config();
	int move_dirs = (int(sys_cfg.get("movement_directions", 4)) == 8) ? 8 : 4;
	int cur_body_fi = active_freeze_frame0 ? 0 : MAX(0, slot_anim_frame_idx);

	Array slots = dm->get_actor_arpg_equip_slots(actor_id);
	HashMap<String, bool> active_slot_keys;
	for (int s = 0; s < slots.size(); s++) {
		if (slots[s].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary sd = slots[s];
		String slot_id = String(sd.get("slot", "")).strip_edges().to_lower();
		if (slot_id.is_empty()) {
			continue;
		}
		active_slot_keys[slot_id] = true;

		Sprite2D *lsp = arpg_layer_sprites.has(slot_id) ? arpg_layer_sprites[slot_id] : nullptr;
		if (!lsp) {
			lsp = memnew(Sprite2D);
			lsp->set_name("ARPG_Layer_" + slot_id);
			lsp->set_centered(false);
			lsp->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
			lsp->set_visible(false);
			add_child(lsp);
			arpg_layer_sprites[slot_id] = lsp;
		}

		int pid = int(sd.get("piece_id", 0));
		int pcat = ClockGearsDataManager::get_category_from_key(String(sd.get("category", "")));
		if (pid <= 0 || pcat < 0) {
			lsp->set_visible(false);
			continue;
		}

		Dictionary piece = dm->get_record_by_id(pcat, pid);
		ClockGearsResolvedEquipLayer rl;
		if (!reg->resolve_arpg_piece_layer(piece, pcat, action_state_id, direction, cur_body_fi, directional_idle, move_dirs, rl) || !rl.visible) {
			lsp->set_visible(false);
			continue;
		}

		Ref<Texture2D> tex = _get_cached_slot_texture(rl.texture_path);
		if (tex.is_null()) {
			lsp->set_visible(false);
			continue;
		}

		int tw = tex->get_width();
		int th = tex->get_height();
		int fw = tw;
		int fh = th;
		lsp->set_texture(tex);
		lsp->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
		lsp->set_scale(Vector2(1, 1));
		lsp->set_hframes(1);
		lsp->set_vframes(1);
		lsp->set_frame(0);

		if (rl.is_completo && rl.region.size.x > 0 && rl.region.size.y > 0) {
			Rect2 reg_r(rl.region);
			if (reg_r.position.x + reg_r.size.x > tw || reg_r.position.y + reg_r.size.y > th) {
				reg_r = Rect2(0, 0, tw, th);
			}
			lsp->set_region_enabled(true);
			lsp->set_region_rect(reg_r);
			fw = int(reg_r.size.x);
			fh = int(reg_r.size.y);
		} else {
			lsp->set_region_enabled(false);
		}

		lsp->set_position(ClockGearsCharacterStateRegistry::compute_layer_sprite_position(rl.is_completo, tile_size, fw, fh, rl.offset_x, rl.offset_y));
		lsp->set_z_index(ClockGearsCharacterStateRegistry::compute_layer_z_index(rl.behind_body, rl.layer_order, s));
		lsp->set_visible(true);
	}

	for (KeyValue<String, Sprite2D *> &E : arpg_layer_sprites) {
		if (!active_slot_keys.has(E.key) && E.value) {
			E.value->set_visible(false);
		}
	}
}

int ClockGearsPlayer::get_active_arpg_equip_layer_count() const {
	int cnt = 0;
	for (const KeyValue<String, Sprite2D *> &E : arpg_layer_sprites) {
		if (E.value && E.value->is_visible()) {
			cnt++;
		}
	}
	return cnt;
}

Array ClockGearsPlayer::get_arpg_equip_layer_info() const {
	Array out;
	for (const KeyValue<String, Sprite2D *> &E : arpg_layer_sprites) {
		if (!E.value) {
			continue;
		}
		Dictionary d;
		d["slot"] = E.key;
		d["visible"] = E.value->is_visible();
		d["z_index"] = E.value->get_z_index();
		d["position"] = E.value->get_position();
		d["region"] = E.value->is_region_enabled() ? E.value->get_region_rect() : Rect2();
		out.push_back(d);
	}
	return out;
}

void ClockGearsPlayer::set_grid_cell_immediate(const Vector2i &p_cell, int p_dir) {
	grid_cell = p_cell;
	direction = ClockGearsCharacterStateRegistry::is_valid_direction(p_dir) ? p_dir : 2;
	moving_step = false;
	dashing = false;
	step_start_px = Vector2(grid_cell.x * tile_size, grid_cell.y * tile_size);
	step_target_px = step_start_px;
	step_exact_px = step_start_px;
	set_position(step_start_px);
	anim_pattern_idx = 1;
	_sync_locomotion_state();
	_update_sprite_frame();
}

void ClockGearsPlayer::set_world_position_immediate(const Vector2 &p_pos, int p_dir) {
	moving_step = false;
	dashing = false;
	direction = ClockGearsCharacterStateRegistry::is_valid_direction(p_dir) ? p_dir : 2;
	step_start_px = p_pos;
	step_target_px = p_pos;
	step_exact_px = p_pos;
	set_position(p_pos);
	_sync_grid_cell_from_world_pos(p_pos);
	anim_pattern_idx = 1;
	_sync_locomotion_state();
	_update_sprite_frame();
}

void ClockGearsPlayer::set_direction(int p_dir) {
	if (action_state_id == "bloquear" || arpg_block_held || arpg_dying || arpg_dead) {
		return; // Mirada fija mientras bloquea o durante muerte
	}
	if (!ClockGearsCharacterStateRegistry::is_valid_direction(p_dir)) {
		return;
	}
	direction = p_dir;
	_update_sprite_frame();
}

int ClockGearsPlayer::_get_tile_passage_mode(const Vector2i &p_cell, int &r_dir4_mask) const {
	r_dir4_mask = 15;
	if (!current_map) {
		return 0;
	}
	return current_map->get_tile_passage_mode(p_cell, r_dir4_mask);
}

bool ClockGearsPlayer::_is_tile_counter(const Vector2i &p_cell) const {
	if (!current_map) {
		return false;
	}
	return current_map->is_tile_counter(p_cell);
}

bool ClockGearsPlayer::_can_pass_cardinal_step(const Vector2i &p_from, const Vector2i &p_to, int p_dir, ClockGearsEvent **r_blocking_event) const {
	if (r_blocking_event) {
		*r_blocking_event = nullptr;
	}
	if (!current_map) {
		return true;
	}
	Vector2i msize = current_map->get_map_size();
	if (p_to.x < 0 || p_to.y < 0 || p_to.x >= msize.x || p_to.y >= msize.y) {
		return false;
	}
	if (through_enabled || body_through) {
		return true;
	}

	if (!current_map->can_pass_tile_step(p_from, p_to, p_dir)) {
		return false;
	}

	// Comprobar colisión con Eventos en la celda destino (Prioridad 1 = Mismo nivel que el jugador, sin Through/atravesable)
	Vector<ClockGearsEvent *> ev_nodes = _cg_get_map_events(current_map);
	Vector2 to_center = Vector2(p_to) * float(tile_size) + get_body_local_origin();
	for (int i = 0; i < ev_nodes.size(); i++) {
		ClockGearsEvent *ev = ev_nodes[i];
		if (!ev || !ev->is_visible() || ev->is_through()) {
			continue;
		}
		if (ev->get_behavior_type() != 0) {
			if (ev->is_body_through()) {
				continue;
			}
			Vector2 ev_center = ev->get_body_world_origin();
			if (ClockGearsShapeGeometry::shapes_overlap_native(body_shape, to_center, direction, ev->get_body_shape_struct(), ev_center, ev->get_direction())) {
				if (r_blocking_event) {
					*r_blocking_event = ev;
				}
				return false;
			}
		} else {
			int ecx = int(Math::floor(ev->get_position().x / float(tile_size)));
			int ecy = int(Math::floor(ev->get_position().y / float(tile_size)));
			if (ecx == p_to.x && ecy == p_to.y && ev->get_priority_type() == 1) {
				if (r_blocking_event) {
					*r_blocking_event = ev;
				}
				return false;
			}
		}
	}
	return true;
}

bool ClockGearsPlayer::can_pass_cell(const Vector2i &p_from, const Vector2i &p_to, int p_dir, ClockGearsEvent **r_blocking_event) const {
	if (r_blocking_event) {
		*r_blocking_event = nullptr;
	}
	if (!current_map) {
		return true;
	}
	Vector2i msize = current_map->get_map_size();
	if (p_to.x < 0 || p_to.y < 0 || p_to.x >= msize.x || p_to.y >= msize.y) {
		return false;
	}
	if (through_enabled || body_through) {
		return true;
	}

	Vector2i delta = p_to - p_from;
	if (Math::abs(delta.x) == 1 && Math::abs(delta.y) == 1) {
		// Movimiento diagonal: requiere casilla destino Y ambas cardinales contiguas pasables (sin rozar esquinas)
		int horiz_dir = (delta.x < 0) ? 4 : 6;
		int vert_dir = (delta.y < 0) ? 8 : 2;
		Vector2i horiz_cell(p_from.x + delta.x, p_from.y);
		Vector2i vert_cell(p_from.x, p_from.y + delta.y);

		if (!_can_pass_cardinal_step(p_from, horiz_cell, horiz_dir, nullptr)) {
			return false;
		}
		if (!_can_pass_cardinal_step(p_from, vert_cell, vert_dir, nullptr)) {
			return false;
		}
		if (!_can_pass_cardinal_step(horiz_cell, p_to, vert_dir, r_blocking_event)) {
			return false;
		}
		if (!_can_pass_cardinal_step(vert_cell, p_to, horiz_dir, r_blocking_event)) {
			return false;
		}
		return true;
	}

	return _can_pass_cardinal_step(p_from, p_to, p_dir, r_blocking_event);
}

bool ClockGearsPlayer::try_step(int p_dir, bool p_is_dash) {
	if (arpg_dying || arpg_dead || arpg_knockback_timer > 0.0f || arpg_dodge_timer > 0.0f) {
		return false;
	}
	if (moving_step) {
		return false;
	}
	if (event_interpreter && event_interpreter->is_blocking_player()) {
		return false;
	}
	float act_spd_mult = 1.0f;
	if (!_can_move_during_current_action(act_spd_mult)) {
		return false;
	}

	if (action_state_id != "atacar" && action_state_id != "bloquear") {
		set_direction(p_dir);
	}
	Vector2i delta = direction_to_vector(p_dir);
	Vector2i target_cell = grid_cell + delta;

	ClockGearsEvent *blocking_ev = nullptr;
	if (!can_pass_cell(grid_cell, target_cell, p_dir, &blocking_ev)) {
		// Si chocamos con un evento que tiene disparador de Contacto (Jugador o Evento), lo activamos
		if (blocking_ev && event_interpreter) {
			int trig = blocking_ev->get_trigger_type();
			if (trig == TRIGGER_PLAYER_TOUCH || trig == TRIGGER_EVENT_TOUCH) {
				if (!blocking_ev->is_direction_fix()) {
					blocking_ev->set_direction(opposite_direction(direction));
				}
				event_interpreter->start_event(blocking_ev);
			}
		}
		return false;
	}

	Vector2i prev_cell = grid_cell;
	grid_cell = target_cell;
	step_start_px = get_position();
	step_target_px = Vector2(grid_cell.x * tile_size, grid_cell.y * tile_size);
	step_exact_px = step_start_px;
	step_target_cell = target_cell;
	dashing = p_is_dash;
	current_step_speed = p_is_dash ? dash_speed : walk_speed;
	moving_step = true;
	_sync_locomotion_state();
	_update_sprite_frame();

	if (follower_manager) {
		follower_manager->record_leader_step(prev_cell, p_dir, current_step_speed);
	}
	emit_signal("step_started", prev_cell, target_cell, p_dir);
	return true;
}

Vector2i ClockGearsPlayer::get_front_cell(int p_candidate_idx) const {
	Vector2i delta = direction_to_vector(direction);
	if (p_candidate_idx == 1 && delta.x != 0 && delta.y != 0) {
		return grid_cell + Vector2i(delta.x, 0); // Cardinal contigua horizontal primero
	}
	if (p_candidate_idx == 2 && delta.x != 0 && delta.y != 0) {
		return grid_cell + Vector2i(0, delta.y); // Cardinal contigua vertical segundo
	}
	return grid_cell + delta; // En mirada diagonal, p_candidate_idx==0 devuelve la casilla diagonal
}

int ClockGearsPlayer::get_front_cell_count() const {
	return ClockGearsCharacterStateRegistry::is_diagonal_direction(direction) ? 3 : 1;
}

bool ClockGearsPlayer::try_activate_front_event() {
	if (moving_step || !current_map || !event_interpreter || event_interpreter->is_blocking_player() || action_cooldown > 0.0f) {
		return false;
	}

	Vector<ClockGearsEvent *> ev_nodes = _cg_get_map_events(current_map);

	auto find_event_at = [&](const Vector2i &p_c) -> ClockGearsEvent * {
		for (int i = 0; i < ev_nodes.size(); i++) {
			ClockGearsEvent *ev = ev_nodes[i];
			if (!ev || !ev->is_visible()) {
				continue;
			}
			int ecx = int(Math::floor(ev->get_position().x / float(tile_size)));
			int ecy = int(Math::floor(ev->get_position().y / float(tile_size)));
			if (ecx == p_c.x && ecy == p_c.y && ev->get_trigger_type() == TRIGGER_ACTION_BUTTON) {
				return ev;
			}
		}
		return nullptr;
	};

	// 1. Misma celda (Prioridad 0 = Debajo del jugador o cualquier evento con Botón de Acción bajo los pies)
	ClockGearsEvent *same_ev = find_event_at(grid_cell);
	if (same_ev) {
		event_interpreter->start_event(same_ev);
		return true;
	}

	// 2. Casilla(s) de enfrente: en cardinal la frontal; en diagonal: diagonal -> horizontal -> vertical
	int cand_count = get_front_cell_count();
	for (int ci = 0; ci < cand_count; ci++) {
		Vector2i front_cell = get_front_cell(ci);
		ClockGearsEvent *front_ev = find_event_at(front_cell);
		if (front_ev) {
			if (!front_ev->is_direction_fix()) {
				front_ev->set_direction(ClockGearsCharacterStateRegistry::vector_to_direction(grid_cell - front_cell));
			}
			event_interpreter->start_event(front_ev);
			return true;
		}
		// 3. Si la casilla frontal tiene flag Mostrador (♦), buscar 1 celda más allá
		if (_is_tile_counter(front_cell)) {
			Vector2i step_vec = front_cell - grid_cell;
			Vector2i counter_target = front_cell + step_vec;
			ClockGearsEvent *counter_ev = find_event_at(counter_target);
			if (counter_ev) {
				if (!counter_ev->is_direction_fix()) {
					counter_ev->set_direction(ClockGearsCharacterStateRegistry::vector_to_direction(grid_cell - counter_target));
				}
				event_interpreter->start_event(counter_ev);
				return true;
			}
		}
	}

	return false;
}

void ClockGearsPlayer::lock_action_key() {
	action_key_was_pressed = true;
	action_key_released = false;
	action_cooldown = 0.20f;
	diag_key_blocked_printed = false;
}

void ClockGearsPlayer::unlock_action_key() {
	action_key_released = true;
	action_key_was_pressed = false;
	action_cooldown = 0.0f;
	diag_key_blocked_printed = false;
}

bool ClockGearsPlayer::is_action_key_locked() const {
	return !action_key_released || action_cooldown > 0.0f;
}

bool ClockGearsPlayer::check_touch_events_at(const Vector2i &p_cell) {
	if (!current_map || !event_interpreter || event_interpreter->is_blocking_player()) {
		return false;
	}
	Vector<ClockGearsEvent *> ev_nodes = _cg_get_map_events(current_map);
	for (int i = 0; i < ev_nodes.size(); i++) {
		ClockGearsEvent *ev = ev_nodes[i];
		if (!ev || !ev->is_visible()) {
			continue;
		}
		int ecx = int(Math::floor(ev->get_position().x / float(tile_size)));
		int ecy = int(Math::floor(ev->get_position().y / float(tile_size)));
		if (ecx == p_cell.x && ecy == p_cell.y) {
			int trig = ev->get_trigger_type();
			if (trig == TRIGGER_PLAYER_TOUCH || trig == TRIGGER_EVENT_TOUCH) {
				event_interpreter->start_event(ev);
				return true;
			}
		}
	}
	return false;
}

void ClockGearsPlayer::_notification(int p_what) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
				Callable cb = callable_mp(this, &ClockGearsPlayer::_on_dm_data_changed);
				if (!dm->is_connected("data_changed", cb)) {
					dm->connect("data_changed", cb);
				}
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
				Callable cb = callable_mp(this, &ClockGearsPlayer::_on_dm_data_changed);
				if (dm->is_connected("data_changed", cb)) {
					dm->disconnect("data_changed", cb);
				}
			}
		} break;

		case NOTIFICATION_READY: {
			set_physics_process(true);
			set_process_unhandled_input(true);
		} break;

		case NOTIFICATION_WM_WINDOW_FOCUS_OUT:
		case NOTIFICATION_APPLICATION_FOCUS_OUT: {
			if (arpg_block_held || action_state_id == "bloquear") {
				arpg_block_held = false;
				if (action_state_id == "bloquear") {
					_end_temporary_action_to_locomotion();
				}
			}
		} break;

		case NOTIFICATION_PHYSICS_PROCESS: {
			float dt = get_physics_process_delta_time();
			Input *inp = Input::get_singleton();

			if (action_cooldown > 0.0f) {
				action_cooldown -= dt;
			}
			if (arpg_invuln_timer > 0.0f) {
				arpg_invuln_timer = MAX(0.0f, arpg_invuln_timer - dt);
			}
			_update_arpg_invuln_blink(dt);
			if (arpg_dodge_cooldown_timer > 0.0f) {
				arpg_dodge_cooldown_timer = MAX(0.0f, arpg_dodge_cooldown_timer - dt);
			}

			if (hitstop_timer > 0.0f) {
				if (dt <= hitstop_timer) {
					hitstop_timer -= dt;
					return;
				}
				dt -= hitstop_timer;
				hitstop_timer = 0.0f;
			}

			if (arpg_dying || arpg_dead) {
				arpg_block_held = false;
				if (arpg_knockback_timer > 0.0f && current_map) {
					float step_dt = MIN(dt, arpg_knockback_timer);
					Vector2 next_pos = ClockGearsEntityNav::move_entity_with_slide(current_map, body_shape, direction, get_position(), arpg_knockback_velocity * step_dt, through_enabled || body_through, this, tile_size);
					set_position(next_pos);
					_sync_grid_cell_from_world_pos(next_pos);
					arpg_knockback_timer = MAX(0.0f, arpg_knockback_timer - dt);
					if (arpg_knockback_timer <= 0.0f) {
						arpg_knockback_velocity = Vector2();
					}
				}
				_advance_slot_animation(dt);
				return;
			}

			if (arpg_knockback_timer > 0.0f) {
				float step_dt = MIN(dt, arpg_knockback_timer);
				if (current_map) {
					Vector2 next_pos = ClockGearsEntityNav::move_entity_with_slide(current_map, body_shape, direction, get_position(), arpg_knockback_velocity * step_dt, through_enabled || body_through, this, tile_size);
					set_position(next_pos);
					_sync_grid_cell_from_world_pos(next_pos);
				}
				arpg_knockback_timer -= dt;
				if (arpg_knockback_timer <= 0.0f) {
					arpg_knockback_timer = 0.0f;
					arpg_knockback_velocity = Vector2();
					if (action_state_id == "golpeado") {
						_end_temporary_action_to_locomotion();
					}
				}
				_advance_slot_animation(dt);
				return;
			}

			if (arpg_dodge_timer > 0.0f) {
				float step_dt = MIN(dt, arpg_dodge_timer);
				if (current_map) {
					Vector2 next_pos = ClockGearsEntityNav::move_entity_with_slide(current_map, body_shape, direction, get_position(), arpg_dodge_velocity * step_dt, through_enabled || body_through, this, tile_size);
					set_position(next_pos);
					_sync_grid_cell_from_world_pos(next_pos);
				}
				arpg_dodge_timer -= dt;
				_advance_slot_animation(dt);
				if (arpg_dodge_timer <= 0.0f) {
					arpg_dodge_timer = 0.0f;
					arpg_dodge_invuln = false;
					arpg_dodge_velocity = Vector2();
					ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
					float cd = dm ? float(dm->get_system_config().get("arpg_dodge_cooldown", 0.4)) : 0.4f;
					arpg_dodge_cooldown_timer = CLAMP(cd, 0.0f, 30.0f);
					_end_temporary_action_to_locomotion();
				}
				return;
			}

			if (action_state_id == "bloquear") {
				if (!arpg_block_held || !is_arpg_action_allowed()) {
					arpg_block_held = false;
					_end_temporary_action_to_locomotion();
				}
			} else if (!is_arpg_action_allowed()) {
				arpg_block_held = false;
			}

			if (moving_step) {
				float act_spd_mult = 1.0f;
				bool can_move_now = _can_move_during_current_action(act_spd_mult);
				_advance_slot_animation(dt);
				if (action_state_id == "atacar") {
					_process_active_attack_hits();
				}
				if (can_move_now) {
					Vector2 diff = step_target_px - step_exact_px;
					float dist = diff.length();
					float eff_speed = current_step_speed * act_spd_mult;
					float move_dist = eff_speed * dt;

					anim_timer += dt * (eff_speed / MAX(1.0f, base_walk_speed));
					if (anim_timer >= 0.14f) {
						anim_timer = 0.0f;
						anim_pattern_idx = (anim_pattern_idx + 1) % 4;
						_update_sprite_frame();
					}

					if (dist <= move_dist) {
						step_exact_px = step_target_px;
						set_position(step_target_px.round());
						moving_step = false;
						dashing = false;
						_sync_locomotion_state();
						_update_sprite_frame();
						emit_signal("step_finished", grid_cell);
						check_touch_events_at(grid_cell);
					} else {
						step_exact_px += diff.normalized() * move_dist;
						// A4.1: Redondear posición visual a píxel entero
						set_position(step_exact_px.round());
					}
				}
			} else {
				_sync_locomotion_state();
				_advance_slot_animation(dt);
				if (action_state_id == "atacar") {
					_process_active_attack_hits();
				}
			}

			if (!moving_step && inp && (!event_interpreter || !event_interpreter->is_blocking_player())) {
				bool allow_space_interact = !(is_arpg_active_here() && cached_dodge_key_str.to_lower() == "space");
				bool act_now = inp->is_key_pressed(Key::ENTER) || inp->is_key_pressed(Key::KP_ENTER) ||
						(allow_space_interact && inp->is_key_pressed(Key::SPACE)) || inp->is_key_pressed(Key::Z);

				if (!act_now) {
					action_key_released = true;
					action_key_was_pressed = false;
					action_cooldown = 0.0f;
					diag_key_blocked_printed = false;
				} else if (!action_key_released) {
					// Jugador mantiene pulsada la tecla de cierre; esperar a que la suelte.
				} else if (act_now && !action_key_was_pressed && action_key_released && action_cooldown <= 0.0f) {
					action_key_was_pressed = true;
					action_key_released = false;
					diag_key_blocked_printed = false;
					if (try_activate_front_event()) {
						return;
					}
				}

				bool up = inp->is_key_pressed(Key::UP) || inp->is_key_pressed(Key::W);
				bool down = inp->is_key_pressed(Key::DOWN) || inp->is_key_pressed(Key::S);
				bool left = inp->is_key_pressed(Key::LEFT) || inp->is_key_pressed(Key::A);
				bool right = inp->is_key_pressed(Key::RIGHT) || inp->is_key_pressed(Key::D);
				bool dash = inp->is_key_pressed(Key::SHIFT);

				int desired_dir = 0;
				if (allow_diagonal) {
					int ix = (right ? 1 : 0) - (left ? 1 : 0);
					int iy = (down ? 1 : 0) - (up ? 1 : 0);
					if (ix != 0 || iy != 0) {
						desired_dir = ClockGearsCharacterStateRegistry::vector_to_direction(Vector2i(ix, iy));
					}
				} else {
					if (down) desired_dir = 2;
					else if (up) desired_dir = 8;
					else if (left) desired_dir = 4;
					else if (right) desired_dir = 6;
				}

				if (desired_dir != 0) {
					try_step(desired_dir, dash);
				} else if (anim_pattern_idx != 1) {
					anim_pattern_idx = 1;
					_update_sprite_frame();
				}
			} else if (inp) {
				bool act_now = inp->is_key_pressed(Key::ENTER) || inp->is_key_pressed(Key::KP_ENTER) ||
						inp->is_key_pressed(Key::SPACE) || inp->is_key_pressed(Key::Z);
				if (!act_now) {
					action_key_released = true;
					action_key_was_pressed = false;
				} else {
					action_key_was_pressed = true;
					action_key_released = false;
				}
			}
		} break;
	}
}

void ClockGearsPlayer::ensure_default_arpg_input_actions() {
	InputMap *im = InputMap::get_singleton();
	if (!im) {
		return;
	}
	auto ensure_mouse_action = [&](const StringName &p_action, MouseButton p_btn) {
		if (!im->has_action(p_action)) {
			im->add_action(p_action);
			Ref<InputEventMouseButton> ev;
			ev.instantiate();
			ev->set_button_index(p_btn);
			ev->set_pressed(true);
			im->action_add_event(p_action, ev);
		}
	};
	ensure_mouse_action("atacar", MouseButton::LEFT);
	ensure_mouse_action("bloquear", MouseButton::RIGHT);
	if (!im->has_action("esquivar")) {
		im->add_action("esquivar");
		Ref<InputEventKey> kev;
		kev.instantiate();
		kev->set_keycode(Key::SPACE);
		kev->set_pressed(true);
		im->action_add_event("esquivar", kev);
	}
}

void ClockGearsPlayer::_refresh_dodge_input_action() {
	InputMap *im = InputMap::get_singleton();
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String key_str = "Space";
	if (dm) {
		String cfg_key = String(dm->get_system_config().get("arpg_dodge_key", "Space")).strip_edges();
		if (!cfg_key.is_empty()) {
			key_str = cfg_key;
		}
	}
	if (key_str == cached_dodge_key_str && im && im->has_action("esquivar")) {
		return;
	}
	cached_dodge_key_str = key_str;
	if (!im) {
		return;
	}
	if (!im->has_action("esquivar")) {
		im->add_action("esquivar");
	}
	Key kc = find_keycode(key_str);
	if (kc == Key::NONE) {
		kc = Key::SPACE;
	}
	im->action_erase_events("esquivar");
	Ref<InputEventKey> kev;
	kev.instantiate();
	kev->set_keycode(kc);
	kev->set_pressed(true);
	im->action_add_event("esquivar", kev);
}

bool ClockGearsPlayer::is_arpg_combat_mode() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	return int(dm->get_system_config().get("combat_mode", COMBAT_MODE_TURN_BASED)) == COMBAT_MODE_ACTION_RPG;
}

bool ClockGearsPlayer::is_arpg_active_here() const {
	if (current_map) {
		return current_map->is_arpg_effective();
	}
	return is_arpg_combat_mode();
}

bool ClockGearsPlayer::is_arpg_action_allowed() const {
	if (arpg_dying || arpg_dead || arpg_knockback_timer > 0.0f) {
		return false;
	}
	if (!is_arpg_active_here()) {
		return false;
	}
	if (event_interpreter && event_interpreter->is_blocking_player()) {
		return false;
	}
	ClockGearsMessageWindow *mw = ClockGearsMessageWindow::get_singleton();
	if (mw && (mw->is_busy() || mw->is_history_open())) {
		return false;
	}
	ClockGearsDebugOverlay *dbg = ClockGearsDebugOverlay::get_singleton();
	if (dbg && dbg->is_panel_open()) {
		return false;
	}
	if (is_inside_tree()) {
		SceneTree *st = get_tree();
		if (st && st->is_paused()) {
			return false;
		}
		Window *root = st ? st->get_root() : nullptr;
		if (root) {
			for (int i = 0; i < root->get_child_count(); i++) {
				Node *ch = root->get_child(i);
				Window *sub_w = Object::cast_to<Window>(ch);
				if (sub_w && sub_w->is_visible() && sub_w->is_exclusive()) {
					return false;
				}
				ClockGearsBattleScene *bs = Object::cast_to<ClockGearsBattleScene>(ch);
				if (bs && bs->is_visible()) {
					return false;
				}
			}
			Control *focused = root->gui_get_focus_owner();
			if (focused && focused->is_visible_in_tree()) {
				return false;
			}
		}
	}
	return true;
}

int ClockGearsPlayer::_get_held_movement_direction() const {
	Input *inp = Input::get_singleton();
	if (!inp) {
		return 0;
	}
	bool up = inp->is_key_pressed(Key::UP) || inp->is_key_pressed(Key::W);
	bool down = inp->is_key_pressed(Key::DOWN) || inp->is_key_pressed(Key::S);
	bool left = inp->is_key_pressed(Key::LEFT) || inp->is_key_pressed(Key::A);
	bool right = inp->is_key_pressed(Key::RIGHT) || inp->is_key_pressed(Key::D);
	if (allow_diagonal) {
		int ix = (right ? 1 : 0) - (left ? 1 : 0);
		int iy = (down ? 1 : 0) - (up ? 1 : 0);
		if (ix != 0 || iy != 0) {
			return ClockGearsCharacterStateRegistry::vector_to_direction(Vector2i(ix, iy));
		}
		return 0;
	}
	if (down) return 2;
	if (up) return 8;
	if (left) return 4;
	if (right) return 6;
	return 0;
}

bool ClockGearsPlayer::_has_usable_action_slot(const String &p_state_id, int p_dir) const {
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (!reg) {
		return false;
	}
	ClockGearsSpriteAnimation anim;
	bool freeze_f0 = false;
	String res_slot;
	if (!reg->resolve_actor_animation(actor_sprite_data, directional_idle, p_state_id, p_dir, anim, freeze_f0, res_slot)) {
		return false;
	}
	return !freeze_f0 && anim.get_frame_count() > 0;
}

bool ClockGearsPlayer::_can_move_during_current_action(float &r_speed_mult) const {
	r_speed_mult = 1.0f;
	if (is_action_state_blocking_movement()) {
		r_speed_mult = 0.0f;
		return false;
	}
	if (action_state_id == "atacar" || action_state_id == "bloquear") {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		Dictionary sys = dm ? dm->get_system_config() : Dictionary();
		if (action_state_id == "atacar") {
			bool allow_move = sys.has("arpg_move_while_attacking") ? bool(sys["arpg_move_while_attacking"]) : true;
			int pct = CLAMP(int(sys.get("arpg_attack_move_speed_pct", 100)), 0, 200);
			if (!allow_move || pct <= 0) {
				r_speed_mult = 0.0f;
				return false;
			}
			r_speed_mult = float(pct) / 100.0f;
			return true;
		}
		bool allow_move = sys.has("arpg_move_while_blocking") ? bool(sys["arpg_move_while_blocking"]) : false;
		int pct = CLAMP(int(sys.get("arpg_block_move_speed_pct", 50)), 0, 200);
		if (!allow_move || pct <= 0) {
			r_speed_mult = 0.0f;
			return false;
		}
		r_speed_mult = float(pct) / 100.0f;
		return true;
	}
	return true;
}

void ClockGearsPlayer::unhandled_input(const Ref<InputEvent> &p_event) {
	if (p_event.is_null()) {
		return;
	}
	if (p_event->is_action_released("bloquear")) {
		arpg_block_held = false;
		if (action_state_id == "bloquear") {
			_end_temporary_action_to_locomotion();
			if (Viewport *vp = get_viewport()) {
				vp->set_input_as_handled();
			}
		}
		return;
	}

	String action;
	if (!should_handle_arpg_input(p_event, action)) {
		return;
	}

	// Respetar paneles de UI que puedan estar bajo el cursor
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && is_inside_tree()) {
		Window *root = get_tree() ? get_tree()->get_root() : nullptr;
		Control *hovered = root ? root->gui_get_hovered_control() : nullptr;
		if (hovered && hovered->is_visible_in_tree() && hovered->get_mouse_filter() == Control::MOUSE_FILTER_STOP) {
			return;
		}
	}

	if (action == "esquivar") {
		if (try_trigger_dodge(0)) {
			if (Viewport *vp = get_viewport()) {
				vp->set_input_as_handled();
			}
		}
		return;
	}

	if (action == "atacar") {
		if (action_state_id == "atacar" || action_state_id == "bloquear" || action_state_id == "esquivar" || arpg_block_held || arpg_dodge_timer > 0.0f || arpg_dying || arpg_dead) {
			return;
		}
		int held_dir = _get_held_movement_direction();
		int atk_dir = (held_dir != 0) ? held_dir : direction;
		if (!_has_usable_action_slot("atacar", atk_dir)) {
			return;
		}
		direction = atk_dir;
		set_action_state("atacar");
		if (Viewport *vp = get_viewport()) {
			vp->set_input_as_handled();
		}
		return;
	}

	if (action == "bloquear") {
		if (action_state_id == "atacar" || action_state_id == "esquivar" || arpg_dodge_timer > 0.0f || arpg_dying || arpg_dead) {
			return;
		}
		arpg_block_held = true;
		if (action_state_id != "bloquear") {
			set_action_state("bloquear");
		}
		if (Viewport *vp = get_viewport()) {
			vp->set_input_as_handled();
		}
	}
}

bool ClockGearsPlayer::should_handle_arpg_input(const Ref<InputEvent> &p_event, String &r_action_out) const {
	r_action_out = "";
	if (p_event.is_null() || !is_arpg_action_allowed()) {
		return false;
	}
	if (p_event->is_action_pressed("esquivar", false, true)) {
		r_action_out = "esquivar";
		return true;
	}
	if (p_event->is_action_pressed("atacar", false, true)) {
		r_action_out = "atacar";
		return true;
	}
	if (p_event->is_action_pressed("bloquear", false, true)) {
		r_action_out = "bloquear";
		return true;
	}
	return false;
}

ClockGearsPlayer::ClockGearsPlayer() {
	ensure_default_arpg_input_actions();
	_refresh_dodge_input_action();
	set_process_unhandled_input(true);
	set_z_index(3);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	set_collision_layer(CG_PHYSICS_LAYER_PLAYER);
	set_collision_mask(0);
	body_shape_owner = create_shape_owner(this);
	sprite = memnew(Sprite2D);
	sprite->set_name("Sprite");
	sprite->set_z_index(0);
	sprite->set_centered(false);
	sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	add_child(sprite);
	_sync_body_collision_shapes();
}

// ============================================================================
// ClockGearsFollower
// ============================================================================

void ClockGearsFollower::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_grid_cell"), &ClockGearsFollower::get_grid_cell);
	ClassDB::bind_method(D_METHOD("get_direction"), &ClockGearsFollower::get_direction);
	ClassDB::bind_method(D_METHOD("set_follower_enabled", "enabled"), &ClockGearsFollower::set_follower_enabled);
	ClassDB::bind_method(D_METHOD("is_follower_enabled"), &ClockGearsFollower::is_follower_enabled);
}

void ClockGearsFollower::_update_sprite_frame() {
	if (!sprite || sprite->get_texture().is_null()) {
		return;
	}
	if (!is_spritesheet) {
		sprite->set_hframes(1);
		sprite->set_vframes(1);
		sprite->set_frame(0);
		sprite->set_position(Vector2(0, 0));
		return;
	}
	sprite->set_hframes(sprite_cols);
	sprite->set_vframes(sprite_rows);
	const int walk_cycle[4] = { 0, 1, 2, 1 };
	int col = moving_step ? (walk_cycle[anim_pattern_idx % 4] % sprite_cols) : (1 % sprite_cols);
	int row = ClockGearsCharacterStateRegistry::direction_to_sheet_row(direction, sprite_rows);
	sprite->set_frame(CLAMP(row, 0, sprite_rows - 1) * sprite_cols + col);

	int frame_h = sprite->get_texture()->get_height() / sprite_rows;
	int frame_w = sprite->get_texture()->get_width() / sprite_cols;
	sprite->set_position(Vector2((tile_size - frame_w) / 2, tile_size - frame_h));
}

void ClockGearsFollower::setup_follower(int p_member_index, int p_actor_id, int p_tile_size) {
	member_index = p_member_index;
	actor_id = p_actor_id;
	tile_size = MAX(16, p_tile_size);

	String gpath;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && actor_id > 0) {
		Dictionary rec = dm->get_record_by_id(CAT_ACTORS, actor_id);
		gpath = rec.get("graphic_path", "");
	}

	Ref<Texture2D> tex = _cg_load_runtime_texture(gpath);
	if (tex.is_valid()) {
		int tw = tex->get_width();
		int th = tex->get_height();
		if (th >= 64 && th % 4 == 0 && (tw % 3 == 0 || tw % 4 == 0)) {
			is_spritesheet = true;
			sprite_rows = 4;
			sprite_cols = (tw % 3 == 0) ? 3 : 4;
		} else {
			is_spritesheet = false;
			sprite_rows = 1;
			sprite_cols = 1;
		}
		sprite->set_texture(tex);
	} else {
		is_spritesheet = true;
		sprite_cols = 3;
		sprite_rows = 4;
		const Color palette[4] = {
			Color(0.28f, 0.72f, 0.42f, 1.0f),
			Color(0.82f, 0.32f, 0.38f, 1.0f),
			Color(0.68f, 0.38f, 0.85f, 1.0f),
			Color(0.85f, 0.62f, 0.22f, 1.0f)
		};
		sprite->set_texture(_cg_make_pixel_char_sheet(palette[(member_index - 1) % 4], Color(0.35f, 0.25f, 0.18f, 1.0f)));
	}
	_update_sprite_frame();
}

void ClockGearsFollower::set_cell_immediate(const Vector2i &p_cell, int p_dir) {
	grid_cell = p_cell;
	direction = ClockGearsCharacterStateRegistry::is_valid_direction(p_dir) ? p_dir : 2;
	moving_step = false;
	stuck_frames = 0;
	step_start_px = Vector2(grid_cell.x * tile_size, grid_cell.y * tile_size);
	step_target_px = step_start_px;
	step_exact_px = step_start_px;
	set_position(step_start_px);
	anim_pattern_idx = 1;
	_update_sprite_frame();
}

void ClockGearsFollower::move_toward_cell(const Vector2i &p_target_cell, int p_dir, float p_speed) {
	if (grid_cell == p_target_cell && !moving_step) {
		direction = p_dir;
		_update_sprite_frame();
		return;
	}
	Vector2i diff = p_target_cell - grid_cell;
	if (diff != Vector2i(0, 0)) {
		direction = ClockGearsCharacterStateRegistry::vector_to_direction(diff);
	} else {
		direction = p_dir;
	}

	grid_cell = p_target_cell;
	step_start_px = get_position();
	step_target_px = Vector2(grid_cell.x * tile_size, grid_cell.y * tile_size);
	step_exact_px = step_start_px;
	current_step_speed = MAX(32.0f, p_speed);
	moving_step = true;
	_update_sprite_frame();
}

void ClockGearsFollower::tick_follower(float p_delta, const Vector2i &p_leader_cell) {
	if (!moving_step) {
		stuck_frames = 0;
		return;
	}
	Vector2 diff = step_target_px - step_exact_px;
	float dist = diff.length();
	float move_dist = current_step_speed * p_delta;

	anim_timer += p_delta;
	if (anim_timer >= 0.14f) {
		anim_timer = 0.0f;
		anim_pattern_idx = (anim_pattern_idx + 1) % 4;
		_update_sprite_frame();
	}

	if (dist > float(tile_size * 4)) {
		stuck_frames++;
		if (stuck_frames > 120) {
			// Salvaguarda anti-atasco (> 120 frames lejos): teletransportar suavemente a su celda objetivo
			step_exact_px = step_target_px;
			set_position(step_target_px.round());
			moving_step = false;
			stuck_frames = 0;
			anim_pattern_idx = 1;
			_update_sprite_frame();
			return;
		}
	} else {
		stuck_frames = 0;
	}

	if (dist <= move_dist) {
		step_exact_px = step_target_px;
		set_position(step_target_px.round());
		moving_step = false;
		anim_pattern_idx = 1;
		_update_sprite_frame();
	} else {
		step_exact_px += diff.normalized() * move_dist;
		set_position(step_exact_px.round());
	}
}

void ClockGearsFollower::set_follower_enabled(bool p_enabled) {
	enabled = p_enabled;
	set_visible(enabled);
}

ClockGearsFollower::ClockGearsFollower() {
	set_z_index(3);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	sprite = memnew(Sprite2D);
	sprite->set_centered(false);
	sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	add_child(sprite);
}

// ============================================================================
// ClockGearsFollowerManager
// ============================================================================

void ClockGearsFollowerManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup_party_followers", "party_actor_ids", "start_cell", "start_dir", "tile_size"), &ClockGearsFollowerManager::setup_party_followers, DEFVAL(2), DEFVAL(32));
	ClassDB::bind_method(D_METHOD("record_leader_step", "previous_leader_cell", "dir", "speed"), &ClockGearsFollowerManager::record_leader_step);
	ClassDB::bind_method(D_METHOD("gather_followers", "cell", "dir"), &ClockGearsFollowerManager::gather_followers, DEFVAL(2));
	ClassDB::bind_method(D_METHOD("set_followers_visible_flag", "visible"), &ClockGearsFollowerManager::set_followers_visible_flag);
	ClassDB::bind_method(D_METHOD("are_followers_visible"), &ClockGearsFollowerManager::are_followers_visible);
	ClassDB::bind_method(D_METHOD("set_follower_member_enabled", "member_idx", "enabled"), &ClockGearsFollowerManager::set_follower_member_enabled);
	ClassDB::bind_method(D_METHOD("get_follower_count"), &ClockGearsFollowerManager::get_follower_count);
	ClassDB::bind_method(D_METHOD("get_follower_cell", "idx"), &ClockGearsFollowerManager::get_follower_cell);
}

void ClockGearsFollowerManager::setup_party_followers(const Array &p_party_actor_ids, const Vector2i &p_start_cell, int p_start_dir, int p_tile_size) {
	tile_size = MAX(16, p_tile_size);
	for (int i = 0; i < followers.size(); i++) {
		if (followers[i]) {
			followers[i]->queue_free();
		}
	}
	followers.clear();
	step_history.clear();

	// El índice 0 del grupo es el Líder (ClockGearsPlayer); los índices 1..8 son los Seguidores
	int count = MIN(p_party_actor_ids.size() - 1, max_followers);
	for (int i = 0; i < count; i++) {
		int actor_id = int(p_party_actor_ids[i + 1]);
		ClockGearsFollower *f = memnew(ClockGearsFollower);
		f->set_name(vformat("Follower_%d", i + 1));
		add_child(f);
		f->setup_follower(i + 1, actor_id, tile_size);
		f->set_cell_immediate(p_start_cell, p_start_dir);
		f->set_visible(followers_visible);
		followers.push_back(f);
	}
}

void ClockGearsFollowerManager::record_leader_step(const Vector2i &p_previous_leader_cell, int p_dir, float p_speed) {
	StepHistoryEntry entry;
	entry.cell = p_previous_leader_cell;
	entry.direction = p_dir;
	entry.speed = p_speed;
	step_history.insert(0, entry);

	int max_hist = MAX(16, followers.size() + 2);
	while (step_history.size() > max_hist) {
		step_history.remove_at(step_history.size() - 1);
	}

	for (int i = 0; i < followers.size(); i++) {
		if (i < step_history.size() && followers[i]) {
			const StepHistoryEntry &st = step_history[i];
			followers[i]->move_toward_cell(st.cell, st.direction, st.speed);
		}
	}
}

void ClockGearsFollowerManager::gather_followers(const Vector2i &p_cell, int p_dir) {
	step_history.clear();
	for (int i = 0; i < followers.size(); i++) {
		if (followers[i]) {
			followers[i]->set_cell_immediate(p_cell, p_dir);
		}
	}
}

void ClockGearsFollowerManager::set_followers_visible_flag(bool p_visible) {
	followers_visible = p_visible;
	for (int i = 0; i < followers.size(); i++) {
		if (followers[i]) {
			followers[i]->set_visible(followers_visible && followers[i]->is_follower_enabled());
		}
	}
}

void ClockGearsFollowerManager::set_follower_member_enabled(int p_member_idx, bool p_enabled) {
	int idx = p_member_idx - 1;
	if (idx >= 0 && idx < followers.size() && followers[idx]) {
		followers[idx]->set_follower_enabled(p_enabled);
		followers[idx]->set_visible(followers_visible && p_enabled);
	}
}

Vector2i ClockGearsFollowerManager::get_follower_cell(int p_idx) const {
	if (p_idx >= 0 && p_idx < followers.size() && followers[p_idx]) {
		return followers[p_idx]->get_grid_cell();
	}
	return Vector2i(0, 0);
}

void ClockGearsFollowerManager::_notification(int p_what) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (p_what == NOTIFICATION_READY) {
		set_physics_process(true);
	} else if (p_what == NOTIFICATION_PHYSICS_PROCESS) {
		float dt = get_physics_process_delta_time();
		Vector2i leader_cell = step_history.is_empty() ? Vector2i(0, 0) : step_history[0].cell;
		for (int i = 0; i < followers.size(); i++) {
			if (followers[i]) {
				followers[i]->tick_follower(dt, leader_cell);
			}
		}
	}
}

ClockGearsFollowerManager::ClockGearsFollowerManager() {
	set_z_index(3);
	set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
}

// ============================================================================
// ClockGearsEventInterpreter
// ============================================================================

void ClockGearsEventInterpreter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_running"), &ClockGearsEventInterpreter::is_running);
	ClassDB::bind_method(D_METHOD("setup_interpreter", "map", "player", "followers"), &ClockGearsEventInterpreter::setup_interpreter, DEFVAL(Variant()), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("is_blocking_player"), &ClockGearsEventInterpreter::is_blocking_player);
	ClassDB::bind_method(D_METHOD("get_active_event"), &ClockGearsEventInterpreter::get_active_event);
	ClassDB::bind_method(D_METHOD("get_state"), &ClockGearsEventInterpreter::get_state);
	ClassDB::bind_method(D_METHOD("get_last_choice_index"), &ClockGearsEventInterpreter::get_last_choice_index);
	ClassDB::bind_method(D_METHOD("start_event", "event"), &ClockGearsEventInterpreter::start_event);
	ClassDB::bind_method(D_METHOD("start_command_list", "commands", "source_event"), &ClockGearsEventInterpreter::start_command_list, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("advance_message_or_choice", "choice_idx"), &ClockGearsEventInterpreter::advance_message_or_choice, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("step_frame"), &ClockGearsEventInterpreter::step_frame);
	ClassDB::bind_method(D_METHOD("get_picture_manager"), &ClockGearsEventInterpreter::get_picture_manager);
	ClassDB::bind_method(D_METHOD("get_cutscene_player"), &ClockGearsEventInterpreter::get_cutscene_player);
	ClassDB::bind_method(D_METHOD("get_game_menu"), &ClockGearsEventInterpreter::get_game_menu);
	ClassDB::bind_method(D_METHOD("get_credits_player"), &ClockGearsEventInterpreter::get_credits_player);
	ClassDB::bind_method(D_METHOD("get_game_over_screen"), &ClockGearsEventInterpreter::get_game_over_screen);
	ClassDB::bind_method(D_METHOD("show_game_over", "cfg_override"), &ClockGearsEventInterpreter::show_game_over, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_fade_state"), &ClockGearsEventInterpreter::get_fade_state);
	ClassDB::bind_method(D_METHOD("get_hud_layer"), &ClockGearsEventInterpreter::get_hud_layer);
	ClassDB::bind_method(D_METHOD("get_hud_overlay"), &ClockGearsEventInterpreter::get_hud_overlay);
	ClassDB::bind_method(D_METHOD("has_active_battle"), &ClockGearsEventInterpreter::has_active_battle);
	ClassDB::bind_method(D_METHOD("has_active_credits"), &ClockGearsEventInterpreter::has_active_credits);
	ClassDB::bind_method(D_METHOD("was_return_to_title_requested"), &ClockGearsEventInterpreter::was_return_to_title_requested);
	ClassDB::bind_method(D_METHOD("get_screen_tint_color"), &ClockGearsEventInterpreter::get_screen_tint_color);
	ClassDB::bind_method(D_METHOD("get_screen_tint_rect"), &ClockGearsEventInterpreter::get_screen_tint_rect);
	ClassDB::bind_method(D_METHOD("get_screen_tint_layer"), &ClockGearsEventInterpreter::get_screen_tint_layer);
	ClassDB::bind_method(D_METHOD("get_ui_layer"), &ClockGearsEventInterpreter::get_ui_layer);
	ClassDB::bind_method(D_METHOD("get_message_window"), &ClockGearsEventInterpreter::get_message_window);
	ClassDB::bind_method(D_METHOD("start_screen_shake", "power", "speed", "duration", "wait"), &ClockGearsEventInterpreter::start_screen_shake, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("stop_screen_shake"), &ClockGearsEventInterpreter::stop_screen_shake);
	ClassDB::bind_method(D_METHOD("get_current_shake_offset"), &ClockGearsEventInterpreter::get_current_shake_offset);
	ClassDB::bind_method(D_METHOD("is_screen_shaking"), &ClockGearsEventInterpreter::is_screen_shaking);
	ClassDB::bind_method(D_METHOD("_on_battle_scene_finished", "result"), &ClockGearsEventInterpreter::_on_battle_scene_finished);

	BIND_CONSTANT(STATE_IDLE);
	BIND_CONSTANT(STATE_RUNNING);
	BIND_CONSTANT(STATE_WAITING_FRAMES);
	BIND_CONSTANT(STATE_WAITING_MESSAGE);
	BIND_CONSTANT(STATE_WAITING_CHOICE);
	BIND_CONSTANT(STATE_WAITING_NUMBER);
	BIND_CONSTANT(STATE_WAITING_TRANSFER);
	BIND_CONSTANT(STATE_WAITING_BATTLE);
	BIND_CONSTANT(STATE_WAITING_CREDITS);
	BIND_CONSTANT(STATE_WAITING_MENU);
	BIND_CONSTANT(STATE_GAME_OVER);
	ClassDB::bind_method(D_METHOD("start_camera_zoom_transition", "target_zoom", "duration_frames"), &ClockGearsEventInterpreter::start_camera_zoom_transition, DEFVAL(0));

	ADD_SIGNAL(MethodInfo("event_started", PropertyInfo(Variant::OBJECT, "event")));
	ADD_SIGNAL(MethodInfo("event_finished", PropertyInfo(Variant::OBJECT, "event")));
	ADD_SIGNAL(MethodInfo("message_shown", PropertyInfo(Variant::STRING, "text")));
	ADD_SIGNAL(MethodInfo("return_to_title_requested"));
	ADD_SIGNAL(MethodInfo("window_style_changed", PropertyInfo(Variant::STRING, "window_type_id"), PropertyInfo(Variant::STRING, "style_id")));
	ADD_SIGNAL(MethodInfo("hud_visibility_changed", PropertyInfo(Variant::STRING, "scope"), PropertyInfo(Variant::STRING, "widget_name"), PropertyInfo(Variant::BOOL, "visible")));
}

void ClockGearsEventInterpreter::_build_runtime_ui() {
	Size2 init_vsz = Size2(1920, 1080);
	if (is_inside_tree() && get_viewport()) {
		init_vsz = get_viewport()->get_visible_rect().size;
	}

	// 0. Capa de Tinte de Pantalla dedicada (Capa 10: Entre mapa/personajes a capa 0 y ventana de mensajes a capa 100)
	if (!screen_tint_layer) {
		screen_tint_layer = memnew(CanvasLayer);
		screen_tint_layer->set_name("ClockGearsScreenTintLayer");
		screen_tint_layer->set_layer(10);
		add_child(screen_tint_layer);

		screen_tint_rect = memnew(ColorRect);
		screen_tint_rect->set_name("ScreenTintRect");
		screen_tint_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		screen_tint_rect->set_color(current_tint_color);
		screen_tint_rect->set_position(Vector2(0, 0));
		screen_tint_rect->set_size(init_vsz);
		screen_tint_layer->add_child(screen_tint_rect);
	}

	if (!hud_layer) {
		hud_layer = memnew(CanvasLayer);
		hud_layer->set_name("ClockGearsHudLayer");
		hud_layer->set_layer(50);
		add_child(hud_layer);
	}
	if (!hud_overlay) {
		hud_overlay = memnew(ClockGearsHudOverlay);
		hud_layer->add_child(hud_overlay);
	}

	if (ui_layer) {
		return;
	}
	ui_layer = memnew(CanvasLayer);
	ui_layer->set_name("ClockGearsRuntimeUI");
	ui_layer->set_layer(100);
	add_child(ui_layer);

	// 1. Capa de fundido de pantalla
	screen_fade_rect = memnew(ColorRect);
	screen_fade_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	screen_fade_rect->set_color(Color(0, 0, 0, 0));
	screen_fade_rect->set_position(Vector2(0, 0));
	screen_fade_rect->set_size(init_vsz);
	ui_layer->add_child(screen_fade_rect);

	// 2. Capa de flash de pantalla
	screen_flash_rect = memnew(ColorRect);
	screen_flash_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	screen_flash_rect->set_color(Color(1, 1, 1, 0));
	screen_flash_rect->set_position(Vector2(0, 0));
	screen_flash_rect->set_size(init_vsz);
	ui_layer->add_child(screen_flash_rect);

	// 4. Gestor de Imágenes de Pantalla (Anexo A1.3: hasta 100 imágenes simultáneas)
	if (!picture_manager) {
		picture_manager = memnew(ClockGearsPictureManager);
		picture_manager->set_name("ClockGearsPictureManager");
		ui_layer->add_child(picture_manager);
	}
}

ClockGearsMessageWindow *ClockGearsEventInterpreter::_ensure_message_window() {
	if (!ui_layer) {
		_build_runtime_ui();
	}
	if (!message_window && ui_layer) {
		message_window = memnew(ClockGearsMessageWindow);
		message_window->connect("message_completed", callable_mp(this, &ClockGearsEventInterpreter::_on_message_window_completed));
		message_window->connect("choice_selected", callable_mp(this, &ClockGearsEventInterpreter::_on_message_window_choice_selected));
		message_window->connect("number_entered", callable_mp(this, &ClockGearsEventInterpreter::_on_message_window_number_entered));
		ui_layer->add_child(message_window);
	}
	return message_window;
}

ClockGearsCutscenePlayer *ClockGearsEventInterpreter::_ensure_cutscene_player() {
	if (!ui_layer) {
		_build_runtime_ui();
	}
	if (!cutscene_player && ui_layer) {
		cutscene_player = memnew(ClockGearsCutscenePlayer);
		cutscene_player->set_name("ClockGearsCutscenePlayer");
		ui_layer->add_child(cutscene_player);
	}
	return cutscene_player;
}

ClockGearsGameMenu *ClockGearsEventInterpreter::_ensure_game_menu() {
	if (!ui_layer) {
		_build_runtime_ui();
	}
	if (!game_menu) {
		game_menu = memnew(ClockGearsGameMenu);
		game_menu->connect("menu_opened", callable_mp(this, &ClockGearsEventInterpreter::_on_game_menu_opened));
		game_menu->connect("menu_closed", callable_mp(this, &ClockGearsEventInterpreter::_on_game_menu_closed));
		add_child(game_menu);
		game_menu->setup_game_menu(this, current_map, player);
	}
	return game_menu;
}

ClockGearsCreditsPlayer *ClockGearsEventInterpreter::_ensure_credits_player() {
	if (!ui_layer) {
		_build_runtime_ui();
	}
	if (!credits_player && ui_layer) {
		credits_player = memnew(ClockGearsCreditsPlayer);
		credits_player->set_name("ClockGearsCreditsPlayer");
		credits_player->set_visible(false);
		credits_player->connect("credits_finished", callable_mp(this, &ClockGearsEventInterpreter::_on_credits_player_finished));
		ui_layer->add_child(credits_player);
	}
	return credits_player;
}

ClockGearsGameOverScreen *ClockGearsEventInterpreter::_ensure_game_over_screen() {
	if (!ui_layer) {
		_build_runtime_ui();
	}
	if (!game_over_screen && ui_layer) {
		game_over_screen = memnew(ClockGearsGameOverScreen);
		game_over_screen->set_name("ClockGearsGameOverScreen");
		game_over_screen->set_visible(false);
		game_over_screen->connect("retry_requested", callable_mp(this, &ClockGearsEventInterpreter::_on_game_over_retry_requested));
		game_over_screen->connect("return_to_title_requested", callable_mp(this, &ClockGearsEventInterpreter::_on_game_over_return_to_title_requested));
		ui_layer->add_child(game_over_screen);
	}
	return game_over_screen;
}

bool ClockGearsEventInterpreter::is_blocking_player() const {
	return state != STATE_IDLE || (game_menu && game_menu->is_menu_open()) || (game_over_screen && game_over_screen->is_open());
}

void ClockGearsEventInterpreter::setup_interpreter(ClockGearsMap *p_map, ClockGearsPlayer *p_player, ClockGearsFollowerManager *p_followers) {
	current_map = p_map;
	player = p_player;
	follower_manager = p_followers;
	_build_runtime_ui();
	if (game_menu) {
		game_menu->setup_game_menu(this, current_map, player);
	}
	if (hud_overlay && player && player->get_actor_id() > 0) {
		hud_overlay->set_leader_actor_id(player->get_actor_id());
	}
	_sync_hud_context_visibility();
	_refresh_all_map_events();
	_get_fade_rect();
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		if (dm->has_pending_post_transfer_commands()) {
			callable_mp(this, &ClockGearsEventInterpreter::_resume_post_transfer_commands).call_deferred();
		}
	}
}

void ClockGearsEventInterpreter::_refresh_all_map_events() {
	if (!current_map) {
		return;
	}
	Node2D *ev_root = current_map->get_events_container();
	Node *search_root = ev_root ? static_cast<Node *>(ev_root) : static_cast<Node *>(current_map);
	TypedArray<Node> ev_nodes = search_root->find_children("*", "ClockGearsEvent", true, false);
	for (int i = 0; i < ev_nodes.size(); i++) {
		ClockGearsEvent *ev = Object::cast_to<ClockGearsEvent>(ev_nodes[i]);
		if (ev) {
			ev->update_active_page();
		}
	}
}

void ClockGearsEventInterpreter::start_event(ClockGearsEvent *p_event) {
	if (!p_event) {
		return;
	}
	p_event->update_active_page();
	Array cmds = p_event->get_commands();
	if (cmds.is_empty()) {
		return;
	}
	start_command_list(cmds, p_event);
}

void ClockGearsEventInterpreter::start_command_list(const Array &p_commands, ClockGearsEvent *p_source_event) {
	if (p_commands.is_empty()) {
		return;
	}
	active_event = p_source_event;
	active_commands = p_commands.duplicate(true);
	command_index = 0;
	last_message_cmd_idx = 0;
	last_choice_index = -999;
	choice_stack.clear();
	state = STATE_RUNNING;
	key_advance_was_pressed = true; // Evitar que la misma pulsación de Enter cierre el primer mensaje
	if (active_event) {
		emit_signal("event_started", active_event);
	}
	_execute_next_commands();
}

void ClockGearsEventInterpreter::_hide_message_ui() {
	if (message_window) {
		message_window->close_window();
	}
}

void ClockGearsEventInterpreter::_on_message_window_completed() {
	if (picture_manager) {
		picture_manager->erase_pictures_on_message_close();
	}
	if (state == STATE_WAITING_MESSAGE) {
		command_index++;
		state = STATE_RUNNING;
		if (player) {
			player->lock_action_key();
		}
		_execute_next_commands();
	}
}

void ClockGearsEventInterpreter::_on_message_window_choice_selected(int p_idx) {
	if (state == STATE_WAITING_CHOICE) {
		last_choice_index = p_idx;
		choice_stack.push_back(p_idx);

		// Si el comando de choices tenía ramas inline (cmd["branches"]):
		if (command_index < active_commands.size()) {
			Variant cv = active_commands[command_index];
			if (cv.get_type() == Variant::DICTIONARY) {
				Dictionary cur_cmd = cv;
				if (cur_cmd.has("branches")) {
					Array branches = cur_cmd.get("branches", Array());
					if (p_idx >= 0 && p_idx < branches.size()) {
						Array b_cmds = branches[p_idx];
						for (int j = b_cmds.size() - 1; j >= 0; j--) {
							active_commands.insert(command_index + 1, b_cmds[j]);
						}
					}
				}
			}
		}

		command_index++;
		state = STATE_RUNNING;
		if (player) {
			player->lock_action_key();
		}
		_execute_next_commands();
	}
}

void ClockGearsEventInterpreter::_on_message_window_number_entered(int p_val) {
	if (state == STATE_WAITING_NUMBER) {
		if (command_index < active_commands.size()) {
			Variant cv = active_commands[command_index];
			if (cv.get_type() == Variant::DICTIONARY) {
				Dictionary cmd = cv;
				int var_id = int(cmd.get("variable", cmd.get("var_id", 0)));
				if (var_id > 0) {
					ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
					if (dm) {
						dm->set_variable(itos(var_id), p_val);
					}
				}
			}
		}
		command_index++;
		state = STATE_RUNNING;
		if (player) {
			player->lock_action_key();
		}
		_execute_next_commands();
	}
}

void ClockGearsEventInterpreter::_on_credits_player_finished(bool p_cancelled) {
	if (state == STATE_WAITING_CREDITS) {
		command_index++;
		state = STATE_RUNNING;
		if (player) {
			player->lock_action_key();
		}
		_execute_next_commands();
	}
	_sync_hud_context_visibility();
}

void ClockGearsEventInterpreter::_on_game_menu_opened() {
	_sync_hud_context_visibility();
}

void ClockGearsEventInterpreter::_on_game_menu_closed() {
	if (state == STATE_WAITING_MENU) {
		command_index++;
		state = STATE_RUNNING;
		if (player) {
			player->lock_action_key();
		}
		_execute_next_commands();
	}
	_sync_hud_context_visibility();
}

void ClockGearsEventInterpreter::show_game_over(const Dictionary &p_cfg_override) {
	_build_runtime_ui();
	if (game_menu && game_menu->is_menu_open()) {
		game_menu->close_menu();
	}
	if (credits_player && credits_player->is_playing()) {
		credits_player->stop_credits(true);
	}
	if (ClockGearsGameOverScreen *go = _ensure_game_over_screen()) {
		go->show_game_over(p_cfg_override);
	}
	state = STATE_GAME_OVER;
	_sync_hud_context_visibility();
}

void ClockGearsEventInterpreter::_on_game_over_retry_requested(const String &p_mode, int p_slot) {
	state = STATE_IDLE;
	active_event = nullptr;
	active_commands.clear();
	command_index = 0;
	if (p_mode == "restart_initial_map" && current_map) {
		// Fallback cuando no hay start_map guardado como archivo .tscn (ej. mapa en memoria).
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		Dictionary sys = dm ? dm->get_system_config() : Dictionary();
		String start_map = String(sys.get("start_map", "")).strip_edges();
		if (start_map.is_empty() || !ResourceLoader::exists(start_map)) {
			current_map->restart_map_after_player_death();
		}
	}
	_sync_hud_context_visibility();
}

void ClockGearsEventInterpreter::_on_game_over_return_to_title_requested() {
	state = STATE_IDLE;
	return_to_title_requested_flag = true;
	emit_signal("return_to_title_requested");
	_sync_hud_context_visibility();
}

void ClockGearsEventInterpreter::_sync_hud_context_visibility() {
	if (!hud_overlay) {
		return;
	}
	bool blocked = (state == STATE_WAITING_MESSAGE || state == STATE_WAITING_CHOICE ||
			state == STATE_WAITING_NUMBER || state == STATE_WAITING_BATTLE ||
			state == STATE_WAITING_CREDITS || state == STATE_WAITING_MENU ||
			state == STATE_GAME_OVER ||
			(game_menu && game_menu->is_menu_open()) ||
			(credits_player && credits_player->is_playing()) ||
			(game_over_screen && game_over_screen->is_open()) ||
			(message_window && message_window->is_busy()));
	hud_overlay->set_context_blocked(blocked);
}

void ClockGearsEventInterpreter::_on_battle_scene_finished(int p_result) {
	if (active_battle_scene) {
		active_battle_scene->queue_free();
		active_battle_scene = nullptr;
	}

	if (p_result == 4 /* RESULT_GAME_OVER */ || (p_result == 2 /* RESULT_DEFEAT */ && !battle_can_lose_flag)) {
		show_game_over();
		print_line("[ClockGears Event] Batalla terminada en GAME OVER.");
		return;
	}

	command_index++;
	state = STATE_RUNNING;
	if (player) {
		player->lock_action_key();
	}
	_execute_next_commands();
	_sync_hud_context_visibility();
}

void ClockGearsEventInterpreter::advance_message_or_choice(int p_choice_idx) {
	if (state == STATE_WAITING_CHOICE) {
		_on_message_window_choice_selected(p_choice_idx);
	} else if (state == STATE_WAITING_MESSAGE) {
		if (message_window) {
			message_window->advance_or_close();
		}
	} else if (state == STATE_WAITING_NUMBER) {
		_on_message_window_number_entered(p_choice_idx);
	}
}

void ClockGearsEventInterpreter::_execute_next_commands() {
	int budget = 0;
	while (state == STATE_RUNNING && command_index < active_commands.size()) {
		if (++budget > MAX_COMMANDS_PER_FRAME) {
			// Ceder el frame si un evento supera el presupuesto de 200 comandos por fotograma (Bug #18/#19)
			break;
		}
		Variant vcmd = active_commands[command_index];
		if (vcmd.get_type() != Variant::DICTIONARY) {
			command_index++;
			continue;
		}
		Dictionary cmd = vcmd;
		if (!_execute_single_command(cmd)) {
			// El comando pausó el intérprete (ej. Mostrar Mensaje, Mostrar Opciones, Esperar, Transferir)
			// Solo al llegar a CLOSED se hace command_index++ (una única vez) y sigue.
			_sync_hud_context_visibility();
			return;
		}
		command_index++;
	}

	if (state == STATE_RUNNING && command_index >= active_commands.size()) {
		ClockGearsEvent *finished_ev = active_event;
		active_event = nullptr;
		active_commands.clear();
		command_index = 0;
		state = STATE_IDLE;
		_hide_message_ui();
		_refresh_all_map_events();
		if (player) {
			player->lock_action_key();
		}
		if (finished_ev) {
			emit_signal("event_finished", finished_ev);
		}
	}
	_sync_hud_context_visibility();
}

bool ClockGearsEventInterpreter::_execute_single_command(const Dictionary &p_cmd) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String type = p_cmd.get("type", "");
	int code = int(p_cmd.get("code", 0));

	if (type == "message" || code == CG_CMD_SHOW_TEXT) {
		last_message_cmd_idx = command_index - 1;
		String raw_txt = p_cmd.get("text", "");
		String speaker = p_cmd.get("speaker", "");
		String face = p_cmd.get("face", "");
		int pos = int(p_cmd.get("position", p_cmd.get("pos", 0)));
		int bg = int(p_cmd.get("frame", p_cmd.get("bg", 0)));
		_build_runtime_ui();
		if (ClockGearsMessageWindow *mw = _ensure_message_window()) {
			mw->show_message(raw_txt, speaker, face, pos, bg);
		}
		emit_signal("message_shown", raw_txt);
		state = STATE_WAITING_MESSAGE;
		return false;
	}

	if (type == "choices" || code == CG_CMD_SHOW_CHOICES) {
		PackedStringArray clean_opts;
		if (p_cmd.has("c1")) {
			for (int i = 1; i <= 6; i++) {
				String key = "c" + itos(i);
				if (p_cmd.has(key) && !String(p_cmd.get(key, "")).is_empty()) {
					clean_opts.push_back(p_cmd.get(key, ""));
				}
			}
		} else {
			String opts_str = p_cmd.get("options", CG_RT_TR("Sí, No"));
			PackedStringArray parts = opts_str.split(",");
			for (int i = 0; i < parts.size(); i++) {
				String s = parts[i].strip_edges();
				if (!s.is_empty()) {
					clean_opts.push_back(s);
				}
			}
		}
		if (clean_opts.is_empty()) {
			clean_opts.push_back(CG_RT_TR("Sí"));
			clean_opts.push_back(CG_RT_TR("No"));
		}
		int cancel_opt = 0;
		if (p_cmd.has("cancel_opt")) {
			cancel_opt = int(p_cmd.get("cancel_opt", 0));
		} else if (p_cmd.has("cancel_type")) {
			cancel_opt = int(p_cmd.get("cancel_type", 0));
		} else if (p_cmd.has("cancel")) {
			cancel_opt = int(p_cmd.get("cancel", 0));
		}
		_build_runtime_ui();
		if (ClockGearsMessageWindow *mw = _ensure_message_window()) {
			mw->show_choices(clean_opts, cancel_opt, 0);
		}
		state = STATE_WAITING_CHOICE;
		return false;
	}

	if (type == "number_input" || code == CG_CMD_INPUT_NUMBER) {
		int var_id = int(p_cmd.get("variable", p_cmd.get("var_id", 1)));
		int digits = int(p_cmd.get("digits", 3));
		_build_runtime_ui();
		if (ClockGearsMessageWindow *mw = _ensure_message_window()) {
			mw->show_number_input(var_id, digits);
		}
		state = STATE_WAITING_NUMBER;
		return false;
	}

	if (type == "when_choice" || code == CG_CMD_WHEN_CHOICE) {
		int target_choice = int(p_cmd.get("choice", p_cmd.get("index", 0)));
		if (last_choice_index == target_choice) {
			// Es la rama elegida por el jugador: ejecutar su contenido
			return true;
		}

		// Rama distinta o ya ejecutada: saltar comandos hasta la siguiente rama o BRANCH_END_CHOICE
		int depth = 0;
		int jump_to = -1;
		for (int i = command_index + 1; i < active_commands.size(); i++) {
			Variant vc = active_commands[i];
			if (vc.get_type() != Variant::DICTIONARY) continue;
			Dictionary c = vc;
			int c_code = int(c.get("code", 0));
			String c_type = c.get("type", "");

			if (c_type == "choices" || c_code == CG_CMD_SHOW_CHOICES) {
				depth++;
			} else if (depth > 0 && (c_type == "branch_end_choice" || c_code == CG_CMD_BRANCH_END_CHOICE)) {
				depth--;
			} else if (depth == 0) {
				if (c_type == "when_choice" || c_code == CG_CMD_WHEN_CHOICE) {
					int next_choice = int(c.get("choice", c.get("index", 0)));
					if (next_choice == last_choice_index) {
						jump_to = i;
						break;
					}
				} else if (c_type == "when_cancel" || c_code == CG_CMD_WHEN_CANCEL) {
					if (last_choice_index == -1) {
						jump_to = i;
						break;
					}
				} else if (c_type == "branch_end_choice" || c_code == CG_CMD_BRANCH_END_CHOICE) {
					jump_to = i;
					break;
				}
			}
		}

		if (jump_to != -1) {
			command_index = jump_to - 1;
		} else {
			command_index = active_commands.size();
		}
		return true;
	}

	if (type == "when_cancel" || code == CG_CMD_WHEN_CANCEL) {
		if (last_choice_index == -1) {
			return true;
		}
		// Rama cancelada no activa: saltar hasta BRANCH_END_CHOICE
		int depth = 0;
		int jump_to = -1;
		for (int i = command_index + 1; i < active_commands.size(); i++) {
			Variant vc = active_commands[i];
			if (vc.get_type() != Variant::DICTIONARY) continue;
			Dictionary c = vc;
			int c_code = int(c.get("code", 0));
			String c_type = c.get("type", "");
			if (c_type == "choices" || c_code == CG_CMD_SHOW_CHOICES) {
				depth++;
			} else if (depth > 0 && (c_type == "branch_end_choice" || c_code == CG_CMD_BRANCH_END_CHOICE)) {
				depth--;
			} else if (depth == 0 && (c_type == "branch_end_choice" || c_code == CG_CMD_BRANCH_END_CHOICE)) {
				jump_to = i;
				break;
			}
		}
		if (jump_to != -1) {
			command_index = jump_to - 1;
		} else {
			command_index = active_commands.size();
		}
		return true;
	}

	if (type == "branch_end_choice" || code == CG_CMD_BRANCH_END_CHOICE) {
		if (!choice_stack.is_empty()) {
			choice_stack.remove_at(choice_stack.size() - 1);
			last_choice_index = choice_stack.is_empty() ? -999 : choice_stack[choice_stack.size() - 1];
		} else {
			last_choice_index = -999;
		}
		return true;
	}

	if (type == "wait" || code == CG_CMD_WAIT) {
		wait_frames_remaining = MAX(1, int(p_cmd.get("frames", 60)));
		state = STATE_WAITING_FRAMES;
		return false;
	}

	if (type == "switch" || code == CG_CMD_CONTROL_SWITCHES) {
		String sw_name = p_cmd.get("name", "");
		bool val = bool(p_cmd.get("val", true));
		if (dm && !sw_name.is_empty()) {
			dm->set_switch(sw_name, val);
		}
		_refresh_all_map_events();
		return true;
	}

	if (type == "self_switch" || code == CG_CMD_CONTROL_SELF_SWITCH) {
		String letter = p_cmd.get("letter", "A");
		bool val = bool(p_cmd.get("val", true));
		if (active_event) {
			active_event->set_self_switch(letter, val);
		}
		return true;
	}

	if (type == "variable" || code == CG_CMD_CONTROL_VARIABLES) {
		String var_name = p_cmd.get("name", "");
		if (var_name.is_empty() && p_cmd.has("var_id")) {
			var_name = itos(int(p_cmd["var_id"]));
		}
		String op = p_cmd.get("op", "=");
		int val = int(p_cmd.get("val", p_cmd.get("value", 0)));
		if (dm && !var_name.is_empty()) {
			int cur = dm->get_variable(var_name);
			if (op == "+") cur += val;
			else if (op == "-") cur -= val;
			else if (op == "*") cur *= val;
			else if (op == "/" && val != 0) cur /= val;
			else if (op == "%" && val != 0) cur %= val;
			else cur = val;
			dm->set_variable(var_name, cur);
		}
		_refresh_all_map_events();
		return true;
	}

	if (type == "item" || code == CG_CMD_CHANGE_ITEMS) {
		String iname = p_cmd.get("name", "");
		String op = p_cmd.get("op", "+");
		int qty = int(p_cmd.get("qty", 1));
		if (dm && !iname.is_empty()) {
			dm->change_item(iname, (op == "-") ? -qty : qty);
		}
		_refresh_all_map_events();
		return true;
	}

	if (type == "gold" || code == CG_CMD_CHANGE_GOLD) {
		String op = p_cmd.get("op", "+");
		int amt = int(p_cmd.get("amount", 0));
		if (dm) {
			dm->change_gold((op == "-") ? -amt : amt);
		}
		return true;
	}

	if (type == "change_arpg_equip" || code == CMD_CHANGE_ARPG_EQUIP || code == CG_CMD_CHANGE_ARPG_EQUIP) {
		int aid = int(p_cmd.get("actor_id", 0));
		String slot_id = String(p_cmd.get("slot", "")).strip_edges().to_lower();
		int pid = int(p_cmd.get("piece_id", 0));
		if (dm && aid > 0 && !slot_id.is_empty()) {
			dm->set_actor_arpg_equip(aid, slot_id, pid);
			if (player) {
				player->refresh_arpg_equip_layers();
			}
		}
		return true;
	}

	if (type == "teleport" || code == CG_CMD_TRANSFER_PLAYER) {
		String dest_map = p_cmd.get("map", "");
		int tx = int(p_cmd.get("x", 0));
		int ty = int(p_cmd.get("y", 0));
		String dir_str = p_cmd.get("dir", "");
		int tdir = player ? player->get_direction() : 2;
		if (dir_str.begins_with("Abajo")) tdir = 2;
		else if (dir_str.begins_with("Izquierda")) tdir = 4;
		else if (dir_str.begins_with("Derecha")) tdir = 6;
		else if (dir_str.begins_with("Arriba")) tdir = 8;
		_perform_map_transfer(dest_map, tx, ty, tdir);
		return true;
	}

	if (type == "move_route" || code == CG_CMD_SET_MOVEMENT_ROUTE) {
		String target = p_cmd.get("target", "");
		String action = p_cmd.get("action", "");
		if (target.begins_with("Jugador") && player) {
			if (action.find("Arriba") >= 0) player->try_step(8);
			else if (action.find("Abajo") >= 0) player->try_step(2);
			else if (action.find("Izquierda") >= 0) player->try_step(4);
			else if (action.find("Derecha") >= 0) player->try_step(6);
		}
		return true;
	}

	if (code == CG_CMD_GATHER_FOLLOWERS) {
		if (follower_manager && player) {
			follower_manager->gather_followers(player->get_grid_cell(), player->get_direction());
		}
		return true;
	}

	if (code == CG_CMD_CHANGE_FOLLOWERS) {
		if (follower_manager) {
			follower_manager->set_followers_visible_flag(bool(p_cmd.get("visible", true)));
		}
		return true;
	}

	if (type == "fade" || code == CG_CMD_FADEOUT_SCREEN || code == CG_CMD_FADEIN_SCREEN) {
		_build_runtime_ui();
		String mode = String(p_cmd.get("mode", (code == CG_CMD_FADEIN_SCREEN) ? "Aclarar" : "Oscurecer")).strip_edges();
		int dur_frames = MAX(1, int(p_cmd.get("duration", p_cmd.get("frames", 30))));
		bool is_out = mode.begins_with("Oscurecer") || mode.to_lower() == "oscurecer" || mode.to_lower().find("out") >= 0;
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		_get_fade_rect();
		fade_start_alpha = dm ? dm->get_runtime_fade_alpha() : (screen_fade_rect ? screen_fade_rect->get_color().a : 0.0f);
		fade_target_alpha = is_out ? 1.0f : 0.0f;
		fade_frames_elapsed = 0;
		fade_frames_total = dur_frames;
		fade_active = true;
		_apply_fade_alpha(fade_start_alpha);
		if (bool(p_cmd.get("wait", true))) {
			wait_frames_remaining = dur_frames;
			state = STATE_WAITING_FRAMES;
			return false;
		}
		return true;
	}

	if (type == "flash" || code == CG_CMD_FLASH_SCREEN) {
		int dur_frames = MAX(1, int(p_cmd.get("duration", 30)));
		flash_duration = float(dur_frames) / 60.0f;
		flash_timer = flash_duration;
		flash_base_color = Color(1, 1, 1, 0.85f);
		if (screen_flash_rect) {
			screen_flash_rect->set_color(flash_base_color);
		}
		return true;
	}

	if (type == "tint" || code == CG_CMD_TINT_SCREEN) {
		_build_runtime_ui();
		String preset = p_cmd.get("preset", "Normal");
		int dur_frames = MAX(1, int(p_cmd.get("duration", p_cmd.get("frames", 30))));
		tint_duration = float(dur_frames) / 60.0f;
		tint_timer = 0.0f;
		if (screen_tint_rect) {
			tint_start_color = current_tint_color;
			if (preset.find("Noche") >= 0 || preset.find("Oscuro") >= 0 || preset == "Night") {
				// Tono azul oscuro nocturno intenso (A1.3): R=0.04, G=0.08, B=0.35, Alpha=0.65
				tint_target_color = Color(0.04f, 0.08f, 0.35f, 0.65f);
			} else if (preset.find("Atardecer") >= 0 || preset == "Sunset") {
				tint_target_color = Color(0.38f, 0.16f, 0.05f, 0.45f);
			} else if (preset.find("Sepia") >= 0) {
				tint_target_color = Color(0.32f, 0.22f, 0.10f, 0.40f);
			} else if (p_cmd.has("color")) {
				tint_target_color = p_cmd.get("color", Color(0, 0, 0, 0));
			} else {
				// Normal / Sin tinte
				tint_target_color = Color(0, 0, 0, 0);
			}
			print_line(vformat("[ClockGears Tint] Tinte '%s' aplicado: %s en nodo '%s' (capa %d)", preset, tint_target_color.to_html(), screen_tint_rect->get_name(), screen_tint_layer ? screen_tint_layer->get_layer() : 0));
		}
		return true;
	}

	// --- Imágenes (Anexo A1.3) ---
	if (type == "picture" || type == "show_picture" || code == CG_CMD_SHOW_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			int origin = int(p_cmd.get("origin", 0));
			Vector2 pos = p_cmd.get("pos", Vector2(float(p_cmd.get("x", 0)), float(p_cmd.get("y", 0))));
			Vector2 scale = p_cmd.get("scale", Vector2(float(p_cmd.get("scale_x", 100)), float(p_cmd.get("scale_y", 100))));
			float opacity = float(p_cmd.get("opacity", 1.0f));
			int blend = int(p_cmd.get("blend", 0));
			int dur_mode = int(p_cmd.get("duration_mode", 0));
			float dur_sec = float(p_cmd.get("duration_sec", p_cmd.get("duration", 0.0f)));
			bool auto_fade = bool(p_cmd.get("auto_fadeout", false));
			bool wait = bool(p_cmd.get("wait", false));

			picture_manager->show_picture(id, path, origin, pos, scale, opacity, blend, dur_mode, dur_sec, auto_fade);

			if (wait && dur_mode == 1 && dur_sec > 0.0f) {
				wait_frames_remaining = int(dur_sec * 60.0f);
				state = STATE_WAITING_FRAMES;
				return false;
			}
		}
		return true;
	}

	if (type == "move_picture" || code == CG_CMD_MOVE_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			int origin = int(p_cmd.get("origin", 0));
			Vector2 pos = p_cmd.get("pos", Vector2(float(p_cmd.get("x", 0)), float(p_cmd.get("y", 0))));
			Vector2 scale = p_cmd.get("scale", Vector2(float(p_cmd.get("scale_x", 100)), float(p_cmd.get("scale_y", 100))));
			float opacity = float(p_cmd.get("opacity", 1.0f));
			int dur_frames = int(p_cmd.get("duration", p_cmd.get("frames", 60)));
			float dur_sec = float(dur_frames) / 60.0f;
			int easing = int(p_cmd.get("easing", 0));
			bool wait = bool(p_cmd.get("wait", false));
			picture_manager->move_picture(id, origin, pos, scale, opacity, dur_sec, easing);
			if (wait && dur_frames > 0) {
				wait_frames_remaining = dur_frames;
				state = STATE_WAITING_FRAMES;
				return false;
			}
		}
		return true;
	}

	if (type == "rotate_picture" || code == CG_CMD_ROTATE_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			float speed = float(p_cmd.get("speed", 0.0f));
			picture_manager->rotate_picture(id, speed);
		}
		return true;
	}

	if (type == "tint_picture" || code == CG_CMD_TINT_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			Color col = p_cmd.get("color", Color(1, 1, 1, 1));
			int dur_frames = int(p_cmd.get("duration", p_cmd.get("frames", 60)));
			float dur_sec = float(dur_frames) / 60.0f;
			bool wait = bool(p_cmd.get("wait", false));
			picture_manager->tint_picture(id, col, dur_sec);
			if (wait && dur_frames > 0) {
				wait_frames_remaining = dur_frames;
				state = STATE_WAITING_FRAMES;
				return false;
			}
		}
		return true;
	}

	if (type == "erase_picture" || code == CG_CMD_ERASE_PICTURE) {
		_build_runtime_ui();
		if (picture_manager) {
			int id = int(p_cmd.get("id", p_cmd.get("index", 1)));
			picture_manager->erase_picture(id);
		}
		return true;
	}

	// --- Audio (Anexo A1.4) ---
	if (type == "play_bgm" || type == "bgm" || code == CG_CMD_PLAY_BGM) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			float vol = float(p_cmd.get("volume", p_cmd.get("vol", 1.0f)));
			float pitch = float(p_cmd.get("pitch", 1.0f));
			float crossfade = float(p_cmd.get("crossfade", 0.5f));
			float pos = float(p_cmd.get("pos", 0.0f));
			am->play_bgm(path, vol, pitch, crossfade, pos);
		}
		return true;
	}

	if (type == "fadeout_bgm" || type == "stop_fade_bgm" || code == CG_CMD_FADEOUT_BGM) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String action = p_cmd.get("action", "fade");
			if (action == "stop" || action.find("Parar") >= 0) {
				am->stop_bgm(0.0f);
			} else {
				float fade = float(p_cmd.get("duration", p_cmd.get("fade", 1.0f)));
				am->stop_bgm(fade);
			}
		}
		return true;
	}

	if (type == "save_bgm" || code == CG_CMD_SAVE_BGM) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			am->save_bgm();
		}
		return true;
	}

	if (type == "resume_bgm" || type == "restore_bgm" || code == CG_CMD_RESUME_BGM) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			float fade = float(p_cmd.get("crossfade", p_cmd.get("fade", 0.5f)));
			am->restore_bgm(fade);
		}
		return true;
	}

	if (type == "play_bgs" || type == "bgs" || code == CG_CMD_PLAY_BGS) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			float vol = float(p_cmd.get("volume", 1.0f));
			float pitch = float(p_cmd.get("pitch", 1.0f));
			float fade = float(p_cmd.get("fade", 0.0f));
			am->play_bgs(path, vol, pitch, fade);
		}
		return true;
	}

	if (type == "fadeout_bgs" || code == CG_CMD_FADEOUT_BGS) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			float fade = float(p_cmd.get("duration", 1.0f));
			am->stop_bgs(fade);
		}
		return true;
	}

	if (type == "play_me" || type == "me" || code == CG_CMD_PLAY_ME) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			float vol = float(p_cmd.get("volume", 1.0f));
			float pitch = float(p_cmd.get("pitch", 1.0f));
			am->play_me(path, vol, pitch);
		}
		return true;
	}

	if (type == "play_se" || type == "se" || code == CG_CMD_PLAY_SE) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			String path = p_cmd.get("path", p_cmd.get("file", ""));
			float vol = float(p_cmd.get("volume", 1.0f));
			float pitch = float(p_cmd.get("pitch", 1.0f));
			am->play_se(path, vol, pitch);
		}
		return true;
	}

	if (type == "stop_se" || code == CG_CMD_STOP_SE) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			am->stop_all_se();
		}
		return true;
	}

	if (type == "audio_stems" || type == "stems" || code == CG_CMD_AUDIO_STEMS) {
		ClockGearsAudioManager *am = ClockGearsAudioManager::get_singleton();
		if (am) {
			int stem_idx = int(p_cmd.get("stem", 0));
			float vol = float(p_cmd.get("volume", 1.0f));
			float fade = float(p_cmd.get("fade", 0.3f));
			if (p_cmd.has("path") && !String(p_cmd.get("path", "")).is_empty()) {
				am->play_stem(stem_idx, p_cmd.get("path", ""), vol);
			} else {
				am->set_stem_volume(stem_idx, vol, fade);
			}
		}
		return true;
	}

	// --- Efectos de pantalla (Shake & Weather) ---
	if (type == "shake" || type == "shake_screen" || code == CG_CMD_SHAKE_SCREEN) {
		_build_runtime_ui();
		float power = float(p_cmd.get("power", 5.0f));
		float speed = float(p_cmd.get("speed", 5.0f));
		int dur_frames = MAX(1, int(p_cmd.get("duration", p_cmd.get("frames", 60))));
		float dur_sec = float(dur_frames) / 60.0f;
		bool wait = bool(p_cmd.get("wait", false));

		start_screen_shake(power, speed, dur_sec, wait);
		if (wait && dur_frames > 0) {
			return false;
		}
		return true;
	}

	if (type == "change_camera_zoom" || code == CG_CMD_CHANGE_CAMERA_ZOOM) {
		float target_zoom = float(p_cmd.get("zoom", 2.0f));
		if (target_zoom <= 0.05f) {
			target_zoom = 2.0f;
		}
		int dur_frames = int(p_cmd.get("duration", 0));
		bool wait = bool(p_cmd.get("wait", false));

		start_camera_zoom_transition(target_zoom, dur_frames);
		if (wait && dur_frames > 0) {
			wait_frames_remaining = dur_frames;
			state = STATE_WAITING_FRAMES;
			return false;
		}
		return true;
	}

	if (type == "weather" || code == CG_CMD_SET_WEATHER) {
		_build_runtime_ui();
		if (picture_manager) {
			String wtype = p_cmd.get("weather", p_cmd.get("type_name", "none"));
			float power = float(p_cmd.get("power", 5.0f));
			picture_manager->set_weather(wtype, power);
		}
		return true;
	}

	// --- Cinemáticas (Anexo A1.2) ---
	if (type == "cutscene" || type == "play_cutscene") {
		_build_runtime_ui();
		if (ClockGearsCutscenePlayer *cp = _ensure_cutscene_player()) {
			Ref<ClockGearsCutscene> cs;
			if (p_cmd.has("cutscene_dict")) {
				cs.instantiate();
				cs->from_dict(p_cmd.get("cutscene_dict", Dictionary()));
			} else if (p_cmd.has("path")) {
				String path = p_cmd.get("path", "");
				if (ResourceLoader::exists(path)) {
					cs = ResourceLoader::load(path);
				}
			}
			if (cs.is_valid()) {
				cp->play(cs);
			}
		}
		return true;
	}

	// --- Batalla por turnos (Fase 8A) ---
	if (type == "battle" || code == CMD_BOSS_BATTLE) {
		_build_runtime_ui();
		Variant tr_var = p_cmd.get("troop_id", p_cmd.get("troop", 1));
		int troop = 1;
		if (tr_var.get_type() == Variant::INT || tr_var.get_type() == Variant::FLOAT) {
			troop = int(tr_var);
		} else {
			String tr_str = tr_var;
			int idx_colon = tr_str.find(":");
			if (idx_colon != -1) {
				troop = tr_str.substr(0, idx_colon).to_int();
			} else {
				troop = tr_str.to_int();
			}
		}
		if (troop <= 0) troop = 1;
		bool can_escape = bool(p_cmd.get("can_escape", true));
		battle_can_lose_flag = bool(p_cmd.get("can_lose", false));
		String bgm = p_cmd.get("bgm", "");

		ClockGearsBattleScene *bs = memnew(ClockGearsBattleScene);
		active_battle_scene = bs;
		bs->setup_battle(troop, can_escape, battle_can_lose_flag, bgm, _ensure_message_window());
		bs->connect("battle_finished", callable_mp(this, &ClockGearsEventInterpreter::_on_battle_scene_finished));
		add_child(bs);

		if (game_menu && game_menu->is_menu_open()) {
			game_menu->close_menu();
		}
		state = STATE_WAITING_BATTLE;
		return false;
	}

	// --- Mostrar Créditos (9B Tanda 4) ---
	if (type == "show_credits" || code == CG_CMD_SHOW_CREDITS) {
		_build_runtime_ui();
		if (game_menu && game_menu->is_menu_open()) {
			game_menu->close_menu();
		}
		if (ClockGearsCreditsPlayer *cp = _ensure_credits_player()) {
			Dictionary cfg_ov = p_cmd.has("credits_config") && p_cmd["credits_config"].get_type() == Variant::DICTIONARY
					? Dictionary(p_cmd["credits_config"])
					: Dictionary();
			cp->start_credits(cfg_ov);
			if (cp->is_playing()) {
				state = STATE_WAITING_CREDITS;
				return false;
			}
		}
		return true;
	}

	// --- Mostrar Game Over (9B Punto 8) ---
	if (type == "show_game_over" || type == "game_over" || code == CG_CMD_SHOW_GAME_OVER || code == CG_CMD_GAME_OVER) {
		Dictionary cfg_ov = p_cmd.has("game_over_config") && p_cmd["game_over_config"].get_type() == Variant::DICTIONARY
				? Dictionary(p_cmd["game_over_config"])
				: Dictionary();
		show_game_over(cfg_ov);
		return false;
	}

	// --- Abrir Menú del Juego (9B Tanda 4) ---
	if (type == "open_menu" || code == CG_CMD_OPEN_MENU) {
		_build_runtime_ui();
		if (ClockGearsGameMenu *gm = _ensure_game_menu()) {
			gm->open_menu_from_event("main", false);
			if (gm->is_menu_open()) {
				state = STATE_WAITING_MENU;
				return false;
			}
		}
		return true;
	}

	// --- Abrir Pantalla de Guardado (9B Tanda 4) ---
	if (type == "open_save" || code == CG_CMD_OPEN_SAVE) {
		_build_runtime_ui();
		if (ClockGearsGameMenu *gm = _ensure_game_menu()) {
			gm->open_menu_from_event("save", true);
			if (gm->is_menu_open()) {
				state = STATE_WAITING_MENU;
				return false;
			}
		}
		return true;
	}

	// --- Bloquear / Permitir Menú del Juego (9B Tanda 4) ---
	if (type == "change_menu_access" || type == "set_menu_access" || type == "menu_access" || code == CG_CMD_CHANGE_MENU_ACCESS) {
		if (dm) {
			bool allow_menu = true;
			if (p_cmd.has("enabled")) {
				allow_menu = bool(p_cmd["enabled"]);
			} else if (p_cmd.has("allowed")) {
				allow_menu = bool(p_cmd["allowed"]);
			} else if (p_cmd.has("disabled")) {
				allow_menu = !bool(p_cmd["disabled"]);
			} else if (p_cmd.has("locked")) {
				allow_menu = !bool(p_cmd["locked"]);
			}
			dm->set_game_menu_disabled(!allow_menu);
		}
		return true;
	}

	// --- Cambiar Estilo de Ventana (9B Tanda 4) ---
	if (type == "change_window_style" || code == CG_CMD_CHANGE_WINDOW_STYLE) {
		String wtype = String(p_cmd.get("window_type_id", p_cmd.get("window_type", ""))).strip_edges().to_lower();
		String sid = String(p_cmd.get("style_id", p_cmd.get("style", ""))).strip_edges().to_lower();
		if (dm && !wtype.is_empty()) {
			dm->set_runtime_window_style_override(wtype, sid);
		}
		if (message_window) {
			message_window->refresh_window_styles();
		}
		if (game_menu) {
			game_menu->refresh_window_styles();
		}
		if (hud_overlay) {
			hud_overlay->refresh_window_styles();
		}
		emit_signal("window_style_changed", wtype, sid);
		return true;
	}

	// --- Mostrar / Ocultar HUD o Widget (9B Tanda 5) ---
	if (type == "change_hud_visibility" || type == "hud_visibility" || type == "show_hide_hud" || code == CG_CMD_CHANGE_HUD_VISIBILITY) {
		String scope = String(p_cmd.get("scope", "")).strip_edges().to_lower();
		String wtarget = String(p_cmd.get("widget_name", p_cmd.get("widget_id", p_cmd.get("widget", p_cmd.get("target", ""))))).strip_edges();
		if (scope.is_empty()) {
			scope = wtarget.is_empty() ? String("all") : String("widget");
		}
		bool vis = true;
		if (p_cmd.has("visible")) {
			vis = bool(p_cmd["visible"]);
		} else if (p_cmd.has("show")) {
			vis = bool(p_cmd["show"]);
		} else if (p_cmd.has("hidden")) {
			vis = !bool(p_cmd["hidden"]);
		} else if (p_cmd.has("action")) {
			String act = String(p_cmd["action"]).strip_edges().to_lower();
			vis = !(act == "hide" || act == "ocultar" || act == "off" || act == "false");
		}
		if (dm) {
			if (scope == "widget" && !wtarget.is_empty()) {
				dm->set_hud_widget_runtime_visible(wtarget, vis);
			} else {
				dm->set_hud_runtime_visible(vis);
			}
		}
		emit_signal("hud_visibility_changed", scope, wtarget, vis);
		return true;
	}

	// --- Volver al Título (9B Tanda 4: con confirmación opcional) ---
	if (type == "return_to_title" || code == CG_CMD_RETURN_TO_TITLE) {
		bool ask_confirm = bool(p_cmd.get("confirm", false));
		if (ask_confirm) {
			_build_runtime_ui();
			if (ClockGearsGameMenu *gm = _ensure_game_menu()) {
				gm->open_menu_from_event("to_title", true);
				if (gm->is_menu_open()) {
					state = STATE_WAITING_MENU;
					return false;
				}
			}
		}
		return_to_title_requested_flag = true;
		emit_signal("return_to_title_requested");
		if (is_inside_tree() && get_tree()) {
			ClockGearsTitleScreen::ensure_title_screen_scene("res://title_screen.tscn");
			if (ResourceLoader::exists("res://title_screen.tscn")) {
				get_tree()->change_scene_to_file("res://title_screen.tscn");
			}
		}
		return true;
	}

	return true;
}

void ClockGearsEventInterpreter::_perform_map_transfer(const String &p_map_path, int p_x, int p_y, int p_dir) {
	Vector2i target_cell(MAX(0, p_x), MAX(0, p_y));
	if (player) {
		player->set_grid_cell_immediate(target_cell, p_dir);
	}
	if (follower_manager) {
		follower_manager->gather_followers(target_cell, p_dir);
	}

	bool changing_scene = false;
	if (!p_map_path.is_empty() && current_map && is_inside_tree()) {
		String path = p_map_path;
		if (!path.begins_with("res://")) {
			if (!path.ends_with(".tscn")) {
				path += ".tscn";
			}
			path = "res://maps/" + path.get_file();
		}
		if (ResourceLoader::exists(path) && path != current_map->get_scene_file_path()) {
			changing_scene = true;
			ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
			if (dm) {
				Dictionary sys = dm->get_system_config();
				sys["runtime_transfer_x"] = target_cell.x;
				sys["runtime_transfer_y"] = target_cell.y;
				sys["runtime_transfer_dir"] = p_dir;
				sys["runtime_has_transfer"] = true;
				dm->set_system_config(sys);

				// Preservar los comandos posteriores al Transferir (ej. Aclarar en preset Puerta) al cambiar de escena.
				if (command_index + 1 < active_commands.size()) {
					Array rem;
					for (int ci = command_index + 1; ci < active_commands.size(); ci++) {
						rem.push_back(active_commands[ci]);
					}
					dm->set_pending_post_transfer_commands(rem);
				}
			}
			get_tree()->change_scene_to_file(path);
		}
	}
	if (!changing_scene && current_map) {
		current_map->apply_enemy_respawns_on_enter();
	}
}

ColorRect *ClockGearsEventInterpreter::_get_fade_rect(CanvasLayer **r_layer) {
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		CanvasLayer *pl = nullptr;
		ColorRect *pr = dm->ensure_persistent_fade_rect(this, &pl);
		if (pr) {
			if (screen_fade_rect) {
				screen_fade_rect->set_visible(false);
			}
			if (r_layer) {
				*r_layer = pl;
			}
			return pr;
		}
	}
	if (r_layer) {
		*r_layer = ui_layer;
	}
	return screen_fade_rect;
}

void ClockGearsEventInterpreter::_apply_fade_alpha(float p_alpha) {
	float a = CLAMP(p_alpha, 0.0f, 1.0f);
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		dm->set_runtime_fade_alpha(a, this);
		if (dm->get_persistent_fade_layer()) {
			return;
		}
	}
	if (screen_fade_rect) {
		screen_fade_rect->set_color(Color(0.0f, 0.0f, 0.0f, a));
		screen_fade_rect->set_visible(a > 0.001f);
	}
}

void ClockGearsEventInterpreter::_resume_post_transfer_commands() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !dm->has_pending_post_transfer_commands()) {
		return;
	}
	Array cmds = dm->take_pending_post_transfer_commands();
	if (!cmds.is_empty() && state == STATE_IDLE) {
		start_command_list(cmds, nullptr);
	}
}

Dictionary ClockGearsEventInterpreter::get_fade_state() {
	Dictionary out;
	CanvasLayer *layer = nullptr;
	ColorRect *rect = _get_fade_rect(&layer);
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	out["layer"] = layer ? layer->get_layer() : -1;
	out["layer_name"] = layer ? String(layer->get_name()) : String();
	out["visible"] = rect ? rect->is_visible() : false;
	out["alpha"] = rect ? rect->get_color().a : (dm ? dm->get_runtime_fade_alpha() : 0.0f);
	out["rect"] = rect ? rect->get_rect() : Rect2();
	out["viewport_rect"] = (is_inside_tree() && get_viewport()) ? get_viewport()->get_visible_rect() : Rect2();
	out["fade_active"] = fade_active;
	return out;
}

void ClockGearsEventInterpreter::start_screen_shake(float p_power, float p_speed, float p_duration, bool p_wait) {
	shake_power = MAX(0.0f, p_power);
	shake_speed = MAX(0.1f, p_speed);
	shake_duration = MAX(0.01f, p_duration);
	shake_timer = shake_duration;
	current_shake_offset = Vector2();

	String target_name = "Ninguno";
	if (current_map && current_map->get_runtime_camera()) {
		target_name = CG_RT_TR("Cámara (Offset)");
	} else if (current_map) {
		target_name = CG_RT_TR("Mapa (Posición)");
	}

	print_line(vformat("[ClockGears Shake] Temblor iniciado: potencia=%.1f, vel=%.1f, dur=%.2fs aplicado a '%s'", shake_power, shake_speed, shake_duration, target_name));

	if (picture_manager) {
		picture_manager->start_screen_shake(shake_power, shake_speed, shake_duration);
	}

	if (p_wait && shake_duration > 0.0f) {
		wait_frames_remaining = int(shake_duration * 60.0f);
		state = STATE_WAITING_FRAMES;
	}
}

void ClockGearsEventInterpreter::stop_screen_shake() {
	shake_timer = 0.0f;
	shake_duration = 0.0f;
	current_shake_offset = Vector2();
	if (current_map) {
		ClockGearsPixelCamera2D *cam = current_map->get_runtime_camera();
		if (cam) {
			cam->set_offset(Vector2(0, 0));
		} else {
			current_map->set_position(Vector2(0, 0));
		}
	}
	if (picture_manager) {
		picture_manager->stop_screen_shake();
	}
}

void ClockGearsEventInterpreter::start_camera_zoom_transition(float p_target_zoom, int p_duration_frames) {
	cam_zoom_target = CLAMP(p_target_zoom, 0.25f, 4.0f);
	cam_zoom_start = 2.0f;
	if (current_map && current_map->get_runtime_camera()) {
		cam_zoom_start = current_map->get_runtime_camera()->get_zoom().x;
	}
	if (p_duration_frames <= 0) {
		cam_zoom_frames_left = 0;
		cam_zoom_frames_total = 0;
		if (current_map && current_map->get_runtime_camera()) {
			bool int_mode = false;
			if (ClockGearsDataManager::get_singleton()) {
				int_mode = bool(ClockGearsDataManager::get_singleton()->get_system_config().get("camera_integer_zoom", false));
			}
			if (int_mode) {
				current_map->get_runtime_camera()->set_allow_fractional_zoom(false);
				current_map->get_runtime_camera()->set_integer_zoom(MAX(1, int(Math::round(cam_zoom_target))));
			} else {
				current_map->get_runtime_camera()->set_allow_fractional_zoom(true);
				current_map->get_runtime_camera()->set_zoom(Vector2(cam_zoom_target, cam_zoom_target));
			}
		}
	} else {
		cam_zoom_frames_total = p_duration_frames;
		cam_zoom_frames_left = p_duration_frames;
		if (current_map && current_map->get_runtime_camera()) {
			current_map->get_runtime_camera()->set_allow_fractional_zoom(true);
		}
	}
}

void ClockGearsEventInterpreter::step_frame() {
	float dt = 1.0f / 60.0f;
	if (is_inside_tree()) {
		dt = get_physics_process_delta_time();
	}

	// Transición suave de zoom de cámara
	if (cam_zoom_frames_left > 0) {
		cam_zoom_frames_left--;
		float t = 1.0f - (float)cam_zoom_frames_left / (float)cam_zoom_frames_total;
		if (cam_zoom_frames_left <= 0) {
			if (current_map && current_map->get_runtime_camera()) {
				bool int_mode = false;
				if (ClockGearsDataManager::get_singleton()) {
					int_mode = bool(ClockGearsDataManager::get_singleton()->get_system_config().get("camera_integer_zoom", false));
				}
				if (int_mode) {
					current_map->get_runtime_camera()->set_allow_fractional_zoom(false);
					current_map->get_runtime_camera()->set_integer_zoom(MAX(1, int(Math::round(cam_zoom_target))));
				} else {
					current_map->get_runtime_camera()->set_allow_fractional_zoom(true);
					current_map->get_runtime_camera()->set_zoom(Vector2(cam_zoom_target, cam_zoom_target));
				}
			}
		} else {
			if (current_map && current_map->get_runtime_camera()) {
				current_map->get_runtime_camera()->set_allow_fractional_zoom(true);
				float cur_z = Math::lerp(cam_zoom_start, cam_zoom_target, t);
				current_map->get_runtime_camera()->set_zoom(Vector2(cur_z, cur_z));
			}
		}
	}

	// Sincronizar tamaño de capas de pantalla con el Viewport
	if (is_inside_tree() && get_viewport()) {
		Size2 vsz = get_viewport()->get_visible_rect().size;
		if (vsz.x > 0 && vsz.y > 0) {
			if (screen_tint_rect && screen_tint_rect->get_size() != vsz) screen_tint_rect->set_size(vsz);
			if (screen_fade_rect && screen_fade_rect->get_size() != vsz) screen_fade_rect->set_size(vsz);
			if (screen_flash_rect && screen_flash_rect->get_size() != vsz) screen_flash_rect->set_size(vsz);
		}
	}
	_get_fade_rect();

	// Actualizar efectos de pantalla (Flash, Fade, Tint y Shake)
	if (shake_timer > 0.0f) {
		shake_timer -= dt;
		float progress = CLAMP(shake_timer / shake_duration, 0.0f, 1.0f);
		float amp = shake_power * 2.5f * progress;
		Vector2 offset = Vector2(0, 0);
		if (amp >= 0.5f) {
			float sx = Math::sin(shake_timer * shake_speed * 12.0f) * amp;
			float sy = Math::cos(shake_timer * shake_speed * 15.0f) * (amp * 0.7f);
			offset = Vector2(Math::round(sx), Math::round(sy));
		}
		current_shake_offset = offset;

		if (current_map) {
			ClockGearsPixelCamera2D *cam = current_map->get_runtime_camera();
			if (cam) {
				cam->set_offset(offset);
			} else {
				current_map->set_position(offset);
			}
		}

		if (shake_timer <= 0.0f) {
			stop_screen_shake();
		}
	}
	if (tint_duration > 0.0f && screen_tint_rect) {
		tint_timer += dt;
		float t = CLAMP(tint_timer / tint_duration, 0.0f, 1.0f);
		current_tint_color = tint_start_color.lerp(tint_target_color, t);
		screen_tint_rect->set_color(current_tint_color);
		if (t >= 1.0f) {
			tint_duration = 0.0f; // Completado: se mantiene en tint_target_color permanentemente
			screen_tint_rect->set_color(tint_target_color);
		}
	}

	if (flash_timer > 0.0f && screen_flash_rect) {
		flash_timer = MAX(0.0f, flash_timer - dt);
		float ratio = (flash_duration > 0.001f) ? (flash_timer / flash_duration) : 0.0f;
		Color c = flash_base_color;
		c.a = flash_base_color.a * ratio;
		screen_flash_rect->set_color(c);
	}
	if (fade_active) {
		fade_frames_elapsed++;
		float t = CLAMP(float(fade_frames_elapsed) / float(MAX(1, fade_frames_total)), 0.0f, 1.0f);
		_apply_fade_alpha(Math::lerp(fade_start_alpha, fade_target_alpha, t));
		if (fade_frames_elapsed >= fade_frames_total) {
			fade_active = false;
			_apply_fade_alpha(fade_target_alpha);
		}
	}

	if (state == STATE_WAITING_FRAMES) {
		wait_frames_remaining--;
		if (wait_frames_remaining <= 0) {
			command_index++;
			state = STATE_RUNNING;
			_execute_next_commands();
		}
		return;
	}

	if (state == STATE_WAITING_MESSAGE || state == STATE_WAITING_CHOICE || state == STATE_WAITING_NUMBER || state == STATE_WAITING_BATTLE || state == STATE_WAITING_CREDITS || state == STATE_WAITING_MENU || state == STATE_GAME_OVER) {
		return;
	}

	if (state == STATE_RUNNING) {
		_execute_next_commands();
		return;
	}

	// Si está IDLE, evaluar eventos con disparador TRIGGER_AUTORUN en el mapa activo
	if (state == STATE_IDLE && current_map) {
		Vector<ClockGearsEvent *> ev_nodes = _cg_get_map_events(current_map);
		for (int i = 0; i < ev_nodes.size(); i++) {
			ClockGearsEvent *ev = ev_nodes[i];
			if (ev && ev->is_visible() && ev->get_trigger_type() == TRIGGER_AUTORUN && !ev->get_commands().is_empty()) {
				start_event(ev);
				break;
			}
		}
	}
}

void ClockGearsEventInterpreter::_notification(int p_what) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (p_what == NOTIFICATION_READY) {
		set_physics_process(true);
		set_process_unhandled_input(true);
	} else if (p_what == NOTIFICATION_PHYSICS_PROCESS) {
		step_frame();
	}
}

void ClockGearsEventInterpreter::unhandled_input(const Ref<InputEvent> &p_event) {
	if (game_menu || !p_event.is_valid() || (Engine::get_singleton() && Engine::get_singleton()->is_editor_hint())) {
		return;
	}
	bool want_menu = false;
	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		Key kc = k->get_keycode();
		if (kc == Key::ESCAPE || kc == Key::M) {
			want_menu = true;
		}
	}
	if (!want_menu) {
		InputMap *im = InputMap::get_singleton();
		if (im && ((im->has_action("ui_cancel") && p_event->is_action_pressed("ui_cancel")) ||
						   (im->has_action("game_menu") && p_event->is_action_pressed("game_menu")) ||
						   (im->has_action("ui_menu") && p_event->is_action_pressed("ui_menu")))) {
			want_menu = true;
		}
	}
	if (want_menu) {
		if (ClockGearsGameMenu *gm = _ensure_game_menu()) {
			if (gm->open_menu()) {
				if (Viewport *vp = get_viewport()) {
					vp->set_input_as_handled();
				}
			}
		}
	}
}

ClockGearsEventInterpreter::ClockGearsEventInterpreter() {
}
