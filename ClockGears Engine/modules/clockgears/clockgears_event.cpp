#include "clockgears_event.h"
#include "clockgears_animation.h"
#include "clockgears_debug_panel.h"
#include "clockgears_map.h"
#include "core/clockgears_data_manager.h"
#include "core/clockgears_system_catalog.h"
#include "core/config/engine.h"
#include "core/io/image.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "runtime/clockgears_entity_ai.h"
#include "runtime/clockgears_runtime.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/texture.h"
#include "scene/theme/theme_db.h"

void ClockGearsEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_event_id", "id"), &ClockGearsEvent::set_event_id);
	ClassDB::bind_method(D_METHOD("get_event_id"), &ClockGearsEvent::get_event_id);

	ClassDB::bind_method(D_METHOD("set_event_name", "name"), &ClockGearsEvent::set_event_name);
	ClassDB::bind_method(D_METHOD("get_event_name"), &ClockGearsEvent::get_event_name);

	ClassDB::bind_method(D_METHOD("set_trigger_type", "trigger"), &ClockGearsEvent::set_trigger_type);
	ClassDB::bind_method(D_METHOD("get_trigger_type"), &ClockGearsEvent::get_trigger_type);

	ClassDB::bind_method(D_METHOD("set_move_type", "move_type"), &ClockGearsEvent::set_move_type);
	ClassDB::bind_method(D_METHOD("get_move_type"), &ClockGearsEvent::get_move_type);

	ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &ClockGearsEvent::set_move_speed);
	ClassDB::bind_method(D_METHOD("get_move_speed"), &ClockGearsEvent::get_move_speed);

	ClassDB::bind_method(D_METHOD("set_move_frequency", "freq"), &ClockGearsEvent::set_move_frequency);
	ClassDB::bind_method(D_METHOD("get_move_frequency"), &ClockGearsEvent::get_move_frequency);

	ClassDB::bind_method(D_METHOD("set_through", "through"), &ClockGearsEvent::set_through);
	ClassDB::bind_method(D_METHOD("is_through"), &ClockGearsEvent::is_through);

	ClassDB::bind_method(D_METHOD("set_direction_fix", "fix"), &ClockGearsEvent::set_direction_fix);
	ClassDB::bind_method(D_METHOD("is_direction_fix"), &ClockGearsEvent::is_direction_fix);

	ClassDB::bind_method(D_METHOD("set_direction", "dir"), &ClockGearsEvent::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &ClockGearsEvent::get_direction);

	ClassDB::bind_method(D_METHOD("set_graphic_path", "path"), &ClockGearsEvent::set_graphic_path);
	ClassDB::bind_method(D_METHOD("get_graphic_path"), &ClockGearsEvent::get_graphic_path);

	ClassDB::bind_method(D_METHOD("set_sprite_hframes", "hframes"), &ClockGearsEvent::set_sprite_hframes);
	ClassDB::bind_method(D_METHOD("get_sprite_hframes"), &ClockGearsEvent::get_sprite_hframes);
	ClassDB::bind_method(D_METHOD("set_sprite_vframes", "vframes"), &ClockGearsEvent::set_sprite_vframes);
	ClassDB::bind_method(D_METHOD("get_sprite_vframes"), &ClockGearsEvent::get_sprite_vframes);
	ClassDB::bind_method(D_METHOD("set_sprite_frame", "frame"), &ClockGearsEvent::set_sprite_frame);
	ClassDB::bind_method(D_METHOD("get_sprite_frame"), &ClockGearsEvent::get_sprite_frame);

	ClassDB::bind_method(D_METHOD("set_condition_switch_1_enabled", "enabled"), &ClockGearsEvent::set_condition_switch_1_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_switch_1_enabled"), &ClockGearsEvent::is_condition_switch_1_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_switch_1_name", "name"), &ClockGearsEvent::set_condition_switch_1_name);
	ClassDB::bind_method(D_METHOD("get_condition_switch_1_name"), &ClockGearsEvent::get_condition_switch_1_name);

	ClassDB::bind_method(D_METHOD("set_condition_switch_2_enabled", "enabled"), &ClockGearsEvent::set_condition_switch_2_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_switch_2_enabled"), &ClockGearsEvent::is_condition_switch_2_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_switch_2_name", "name"), &ClockGearsEvent::set_condition_switch_2_name);
	ClassDB::bind_method(D_METHOD("get_condition_switch_2_name"), &ClockGearsEvent::get_condition_switch_2_name);

	ClassDB::bind_method(D_METHOD("set_condition_self_switch_enabled", "enabled"), &ClockGearsEvent::set_condition_self_switch_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_self_switch_enabled"), &ClockGearsEvent::is_condition_self_switch_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_self_switch_letter", "letter"), &ClockGearsEvent::set_condition_self_switch_letter);
	ClassDB::bind_method(D_METHOD("get_condition_self_switch_letter"), &ClockGearsEvent::get_condition_self_switch_letter);

	ClassDB::bind_method(D_METHOD("set_self_switch", "letter", "val"), &ClockGearsEvent::set_self_switch);
	ClassDB::bind_method(D_METHOD("get_self_switch", "letter"), &ClockGearsEvent::get_self_switch);
	ClassDB::bind_method(D_METHOD("get_self_switches"), &ClockGearsEvent::get_self_switches);
	ClassDB::bind_method(D_METHOD("set_self_switches", "switches"), &ClockGearsEvent::set_self_switches);

	ClassDB::bind_method(D_METHOD("get_active_page_index"), &ClockGearsEvent::get_active_page_index);
	ClassDB::bind_method(D_METHOD("update_active_page"), &ClockGearsEvent::update_active_page);

	ClassDB::bind_method(D_METHOD("set_condition_var_enabled", "enabled"), &ClockGearsEvent::set_condition_var_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_var_enabled"), &ClockGearsEvent::is_condition_var_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_var_name", "name"), &ClockGearsEvent::set_condition_var_name);
	ClassDB::bind_method(D_METHOD("get_condition_var_name"), &ClockGearsEvent::get_condition_var_name);
	ClassDB::bind_method(D_METHOD("set_condition_var_value", "value"), &ClockGearsEvent::set_condition_var_value);
	ClassDB::bind_method(D_METHOD("get_condition_var_value"), &ClockGearsEvent::get_condition_var_value);

	ClassDB::bind_method(D_METHOD("set_condition_item_enabled", "enabled"), &ClockGearsEvent::set_condition_item_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_item_enabled"), &ClockGearsEvent::is_condition_item_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_item_name", "name"), &ClockGearsEvent::set_condition_item_name);
	ClassDB::bind_method(D_METHOD("get_condition_item_name"), &ClockGearsEvent::get_condition_item_name);

	ClassDB::bind_method(D_METHOD("set_condition_actor_enabled", "enabled"), &ClockGearsEvent::set_condition_actor_enabled);
	ClassDB::bind_method(D_METHOD("is_condition_actor_enabled"), &ClockGearsEvent::is_condition_actor_enabled);
	ClassDB::bind_method(D_METHOD("set_condition_actor_name", "name"), &ClockGearsEvent::set_condition_actor_name);
	ClassDB::bind_method(D_METHOD("get_condition_actor_name"), &ClockGearsEvent::get_condition_actor_name);

	ClassDB::bind_method(D_METHOD("set_priority_type", "priority"), &ClockGearsEvent::set_priority_type);
	ClassDB::bind_method(D_METHOD("get_priority_type"), &ClockGearsEvent::get_priority_type);

	ClassDB::bind_method(D_METHOD("set_commands", "commands"), &ClockGearsEvent::set_commands);
	ClassDB::bind_method(D_METHOD("get_commands"), &ClockGearsEvent::get_commands);
	ClassDB::bind_method(D_METHOD("get_active_commands"), &ClockGearsEvent::get_commands);
	ClassDB::bind_method(D_METHOD("set_behavior_type", "type"), &ClockGearsEvent::set_behavior_type);
	ClassDB::bind_method(D_METHOD("get_behavior_type"), &ClockGearsEvent::get_behavior_type);
	ClassDB::bind_method(D_METHOD("set_behavior_ref_id", "id"), &ClockGearsEvent::set_behavior_ref_id);
	ClassDB::bind_method(D_METHOD("get_behavior_ref_id"), &ClockGearsEvent::get_behavior_ref_id);
	ClassDB::bind_method(D_METHOD("set_body_shape", "shape"), &ClockGearsEvent::set_body_shape);
	ClassDB::bind_method(D_METHOD("get_body_shape"), &ClockGearsEvent::get_body_shape);
	ClassDB::bind_method(D_METHOD("set_body_through", "through"), &ClockGearsEvent::set_body_through);
	ClassDB::bind_method(D_METHOD("is_body_through"), &ClockGearsEvent::is_body_through);
	ClassDB::bind_method(D_METHOD("set_avoid_obstacles", "avoid"), &ClockGearsEvent::set_avoid_obstacles);
	ClassDB::bind_method(D_METHOD("is_avoid_obstacles"), &ClockGearsEvent::is_avoid_obstacles);
	ClassDB::bind_method(D_METHOD("get_detection_zones"), &ClockGearsEvent::get_detection_zones);
	ClassDB::bind_method(D_METHOD("get_zone_count"), &ClockGearsEvent::get_zone_count);
	ClassDB::bind_method(D_METHOD("get_zone_node", "index"), &ClockGearsEvent::get_zone_node);
	ClassDB::bind_method(D_METHOD("get_entity_body"), &ClockGearsEvent::get_entity_body);
	ClassDB::bind_method(D_METHOD("reset_spawn_origin"), &ClockGearsEvent::reset_spawn_origin);
	ClassDB::bind_method(D_METHOD("get_spawn_origin"), &ClockGearsEvent::get_spawn_origin);
	ClassDB::bind_method(D_METHOD("get_ai_state"), &ClockGearsEvent::get_ai_state);
	ClassDB::bind_method(D_METHOD("get_ai_state_name"), &ClockGearsEvent::get_ai_state_name);
	ClassDB::bind_method(D_METHOD("get_action_state"), &ClockGearsEvent::get_action_state);
	ClassDB::bind_method(D_METHOD("get_active_visual_slot"), &ClockGearsEvent::get_active_visual_slot);
	ClassDB::bind_method(D_METHOD("is_paused_by_event"), &ClockGearsEvent::is_paused_by_event);
	ClassDB::bind_method(D_METHOD("get_current_astar_path"), &ClockGearsEvent::get_current_astar_path);
	ClassDB::bind_method(D_METHOD("has_astar_no_path"), &ClockGearsEvent::has_astar_no_path);
	ClassDB::bind_method(D_METHOD("execute_zone_actions", "actions", "target"), &ClockGearsEvent::execute_zone_actions, DEFVAL(Variant()));

	ClassDB::bind_method(D_METHOD("get_arpg_hp"), &ClockGearsEvent::get_arpg_hp);
	ClassDB::bind_method(D_METHOD("get_arpg_max_hp"), &ClockGearsEvent::get_arpg_max_hp);
	ClassDB::bind_method(D_METHOD("get_arpg_atk"), &ClockGearsEvent::get_arpg_atk);
	ClassDB::bind_method(D_METHOD("get_arpg_def"), &ClockGearsEvent::get_arpg_def);
	ClassDB::bind_method(D_METHOD("get_arpg_knockback_resist"), &ClockGearsEvent::get_arpg_knockback_resist);
	ClassDB::bind_method(D_METHOD("set_knockback_duration", "dur"), &ClockGearsEvent::set_knockback_duration);
	ClassDB::bind_method(D_METHOD("get_knockback_duration"), &ClockGearsEvent::get_knockback_duration);
	ClassDB::bind_method(D_METHOD("is_arpg_dying"), &ClockGearsEvent::is_arpg_dying);
	ClassDB::bind_method(D_METHOD("is_arpg_dead"), &ClockGearsEvent::is_arpg_dead);
	ClassDB::bind_method(D_METHOD("is_arpg_alive"), &ClockGearsEvent::is_arpg_alive);
	ClassDB::bind_method(D_METHOD("is_arpg_hp_bar_visible"), &ClockGearsEvent::is_arpg_hp_bar_visible);
	ClassDB::bind_method(D_METHOD("get_arpg_hp_bar_remaining"), &ClockGearsEvent::get_arpg_hp_bar_remaining);
	ClassDB::bind_method(D_METHOD("get_arpg_hitstop_remaining"), &ClockGearsEvent::get_arpg_hitstop_remaining);
	ClassDB::bind_method(D_METHOD("get_arpg_knockback_remaining"), &ClockGearsEvent::get_arpg_knockback_remaining);
	ClassDB::bind_method(D_METHOD("get_arpg_world_center"), &ClockGearsEvent::get_arpg_world_center);
	ClassDB::bind_method(D_METHOD("get_arpg_effective_hurtbox"), &ClockGearsEvent::get_arpg_effective_hurtbox);
	ClassDB::bind_method(D_METHOD("get_arpg_enemy_record"), &ClockGearsEvent::get_arpg_enemy_record);
	ClassDB::bind_method(D_METHOD("get_arpg_anim_frame"), &ClockGearsEvent::get_arpg_anim_frame);
	ClassDB::bind_method(D_METHOD("take_arpg_hit", "damage", "knockback_force", "hit_dir", "hitstop_ms", "is_crit"), &ClockGearsEvent::take_arpg_hit, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("start_arpg_attack", "target"), &ClockGearsEvent::start_arpg_attack, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("sync_arpg_defeated_state"), &ClockGearsEvent::sync_arpg_defeated_state);
	ClassDB::bind_method(D_METHOD("respawn_arpg_enemy"), &ClockGearsEvent::respawn_arpg_enemy);

	ClassDB::bind_method(D_METHOD("set_pages", "pages"), &ClockGearsEvent::set_pages);
	ClassDB::bind_method(D_METHOD("get_pages"), &ClockGearsEvent::get_pages);

	ClassDB::bind_method(D_METHOD("add_command", "command"), &ClockGearsEvent::add_command);
	ClassDB::bind_method(D_METHOD("clear_commands"), &ClockGearsEvent::clear_commands);
	ClassDB::bind_method(D_METHOD("serialize_to_meta"), &ClockGearsEvent::serialize_to_meta);
	ClassDB::bind_method(D_METHOD("deserialize_from_meta"), &ClockGearsEvent::deserialize_from_meta);
	ClassDB::bind_method(D_METHOD("execute"), &ClockGearsEvent::execute);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "event_id"), "set_event_id", "get_event_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "event_name"), "set_event_name", "get_event_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "trigger_type"), "set_trigger_type", "get_trigger_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_type"), "set_move_type", "get_move_type");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_speed"), "set_move_speed", "get_move_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_frequency"), "set_move_frequency", "get_move_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "through"), "set_through", "is_through");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "direction_fix"), "set_direction_fix", "is_direction_fix");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "direction"), "set_direction", "get_direction");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority_type"), "set_priority_type", "get_priority_type");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "graphic_path"), "set_graphic_path", "get_graphic_path");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sprite_hframes"), "set_sprite_hframes", "get_sprite_hframes");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sprite_vframes"), "set_sprite_vframes", "get_sprite_vframes");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sprite_frame"), "set_sprite_frame", "get_sprite_frame");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "commands"), "set_commands", "get_commands");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "pages"), "set_pages", "get_pages");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "behavior_type"), "set_behavior_type", "get_behavior_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "behavior_ref_id"), "set_behavior_ref_id", "get_behavior_ref_id");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_switch_1_enabled"), "set_condition_switch_1_enabled", "is_condition_switch_1_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_switch_1_name"), "set_condition_switch_1_name", "get_condition_switch_1_name");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_switch_2_enabled"), "set_condition_switch_2_enabled", "is_condition_switch_2_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_switch_2_name"), "set_condition_switch_2_name", "get_condition_switch_2_name");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_self_switch_enabled"), "set_condition_self_switch_enabled", "is_condition_self_switch_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_self_switch_letter"), "set_condition_self_switch_letter", "get_condition_self_switch_letter");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "self_switches"), "set_self_switches", "get_self_switches");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "active_page_index"), "", "get_active_page_index");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_var_enabled"), "set_condition_var_enabled", "is_condition_var_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_var_name"), "set_condition_var_name", "get_condition_var_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "condition_var_value"), "set_condition_var_value", "get_condition_var_value");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_item_enabled"), "set_condition_item_enabled", "is_condition_item_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_item_name"), "set_condition_item_name", "get_condition_item_name");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "condition_actor_enabled"), "set_condition_actor_enabled", "is_condition_actor_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "condition_actor_name"), "set_condition_actor_name", "get_condition_actor_name");

	ADD_SIGNAL(MethodInfo("event_triggered", PropertyInfo(Variant::OBJECT, "event")));
	ADD_SIGNAL(MethodInfo("arpg_damaged", PropertyInfo(Variant::INT, "damage"), PropertyInfo(Variant::BOOL, "is_crit")));
	ADD_SIGNAL(MethodInfo("arpg_defeated"));
}

void ClockGearsEvent::serialize_to_meta() {
	set_meta("clockgears_event", true);
	set_meta("event_id", event_id);
	set_meta("event_name", event_name);
	set_meta("trigger_type", trigger_type);
	set_meta("move_type", move_type);
	set_meta("move_speed", move_speed);
	set_meta("move_frequency", move_frequency);
	set_meta("through", through);
	set_meta("direction_fix", direction_fix);
	set_meta("direction", direction);
	set_meta("priority_type", priority_type);
	set_meta("graphic_path", graphic_path);
	set_meta("sprite_hframes", sprite_hframes);
	set_meta("sprite_vframes", sprite_vframes);
	set_meta("sprite_frame", sprite_frame);
	set_meta("commands", commands.duplicate(true));
	set_meta("pages", pages.duplicate(true));
	set_meta("behavior_type", behavior_type);
	set_meta("behavior_ref_id", behavior_ref_id);
	set_meta("condition_switch_1_enabled", condition_switch_1_enabled);
	set_meta("condition_switch_1_name", condition_switch_1_name);
	set_meta("condition_switch_2_enabled", condition_switch_2_enabled);
	set_meta("condition_switch_2_name", condition_switch_2_name);
	set_meta("condition_self_switch_enabled", condition_self_switch_enabled);
	set_meta("condition_self_switch_letter", condition_self_switch_letter);
	set_meta("self_switches", self_switches.duplicate(true));
	set_meta("condition_var_enabled", condition_var_enabled);
	set_meta("condition_var_name", condition_var_name);
	set_meta("condition_var_value", condition_var_value);
	set_meta("condition_item_enabled", condition_item_enabled);
	set_meta("condition_item_name", condition_item_name);
	set_meta("condition_actor_enabled", condition_actor_enabled);
	set_meta("condition_actor_name", condition_actor_name);
}

void ClockGearsEvent::deserialize_from_meta() {
	if (has_meta("event_id")) event_id = get_meta("event_id");
	if (has_meta("event_name")) event_name = get_meta("event_name");
	if (has_meta("trigger_type")) trigger_type = get_meta("trigger_type");
	if (has_meta("move_type")) move_type = get_meta("move_type");
	if (has_meta("move_speed")) move_speed = get_meta("move_speed");
	if (has_meta("move_frequency")) move_frequency = get_meta("move_frequency");
	if (has_meta("through")) through = get_meta("through");
	if (has_meta("direction_fix")) direction_fix = get_meta("direction_fix");
	if (has_meta("direction")) direction = get_meta("direction");
	if (has_meta("priority_type")) priority_type = get_meta("priority_type");
	if (has_meta("sprite_hframes")) sprite_hframes = MAX(1, int(get_meta("sprite_hframes")));
	if (has_meta("sprite_vframes")) sprite_vframes = MAX(1, int(get_meta("sprite_vframes")));
	if (has_meta("sprite_frame")) sprite_frame = MAX(0, int(get_meta("sprite_frame")));
	if (has_meta("graphic_path")) set_graphic_path(get_meta("graphic_path"));
	if (has_meta("commands")) commands = get_meta("commands");
	if (has_meta("pages")) pages = get_meta("pages");
	if (has_meta("behavior_type")) behavior_type = int(get_meta("behavior_type"));
	if (has_meta("behavior_ref_id")) behavior_ref_id = int(get_meta("behavior_ref_id"));
	if (has_meta("condition_switch_1_enabled")) condition_switch_1_enabled = get_meta("condition_switch_1_enabled");
	if (has_meta("condition_switch_1_name")) condition_switch_1_name = get_meta("condition_switch_1_name");
	if (has_meta("condition_switch_2_enabled")) condition_switch_2_enabled = get_meta("condition_switch_2_enabled");
	if (has_meta("condition_switch_2_name")) condition_switch_2_name = get_meta("condition_switch_2_name");
	if (has_meta("condition_self_switch_enabled")) condition_self_switch_enabled = get_meta("condition_self_switch_enabled");
	if (has_meta("condition_self_switch_letter")) condition_self_switch_letter = get_meta("condition_self_switch_letter");
	if (has_meta("self_switches")) self_switches = get_meta("self_switches");
	if (has_meta("condition_var_enabled")) condition_var_enabled = get_meta("condition_var_enabled");
	if (has_meta("condition_var_name")) condition_var_name = get_meta("condition_var_name");
	if (has_meta("condition_var_value")) condition_var_value = get_meta("condition_var_value");
	if (has_meta("condition_item_enabled")) condition_item_enabled = get_meta("condition_item_enabled");
	if (has_meta("condition_item_name")) condition_item_name = get_meta("condition_item_name");
	if (has_meta("condition_actor_enabled")) condition_actor_enabled = get_meta("condition_actor_enabled");
	if (has_meta("condition_actor_name")) condition_actor_name = get_meta("condition_actor_name");
}

void ClockGearsEvent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_READY: {
			if (has_meta("clockgears_event")) {
				deserialize_from_meta();
			}
			if (behavior_type != 0 || !pages.is_empty()) {
				update_active_page();
			} else if (!graphic_path.is_empty()) {
				set_graphic_path(graphic_path);
			}
			if (p_what == NOTIFICATION_READY && !Engine::get_singleton()->is_editor_hint() && behavior_type != 0) {
				if (!spawn_origin_set) {
					reset_spawn_origin();
				}
				astar_timer = float((event_id * 37) % MAX(1, path_recalc_ms)) / 1000.0f;
				wander_rng.seed(uint64_t(event_id) * 2654435761ULL ^ uint64_t(behavior_ref_id));
				set_physics_process(true);
			}
			queue_redraw();
		} break;
		case NOTIFICATION_PHYSICS_PROCESS: {
			if (!Engine::get_singleton()->is_editor_hint() && behavior_type != 0) {
				_physics_tick_behavior(float(get_physics_process_delta_time()));
			}
		} break;
		case NOTIFICATION_DRAW: {
			// Marcadores de evento (EV_0): No se dibujan en el juego por defecto (Punto 1).
			// Solo se dibuja el sprite si lo tiene. La visualización de depuración se activa con F3.
			if (!Engine::get_singleton()->is_editor_hint() && is_arpg_hp_bar_visible()) {
				if (sprite) {
					sprite->set_draw_behind_parent(true);
				}
				float frac = CLAMP(float(arpg_hp) / float(MAX(1, arpg_max_hp)), 0.0f, 1.0f);
				float cx = (get_arpg_world_center() - get_position()).x;
				float top_y = sprite ? MIN(-6.0f, sprite->get_position().y - 7.0f) : -8.0f;
				float bw = 30.0f;
				float bh = 4.0f;
				draw_rect(Rect2(cx - bw * 0.5f - 1.0f, top_y - 1.0f, bw + 2.0f, bh + 2.0f), Color(0.05f, 0.05f, 0.08f, 0.88f), true);
				Color hp_col = (frac > 0.5f) ? Color(0.28f, 0.88f, 0.36f, 0.96f) : ((frac > 0.25f) ? Color(0.95f, 0.72f, 0.20f, 0.96f) : Color(0.95f, 0.25f, 0.25f, 0.96f));
				draw_rect(Rect2(cx - bw * 0.5f, top_y, bw * frac, bh), hp_col, true);
			}
		} break;
	}
}

void ClockGearsEvent::_update_sprite_region() {
	if (!sprite) {
		return;
	}
	int hf = MAX(1, sprite_hframes);
	int vf = MAX(1, sprite_vframes);
	sprite->set_hframes(hf);
	sprite->set_vframes(vf);
	sprite->set_frame(CLAMP(sprite_frame, 0, hf * vf - 1));
	if (sprite->get_texture().is_valid()) {
		int frame_h = sprite->get_texture()->get_height() / vf;
		if (frame_h > 32) {
			sprite->set_position(Vector2(0, 32 - frame_h));
		} else {
			sprite->set_position(Vector2(0, 0));
		}
	}
}

void ClockGearsEvent::set_graphic_path(const String &p_path) {
	graphic_path = p_path;
	if (!sprite) {
		return;
	}
	sprite->set_region_enabled(false);
	if (p_path.is_empty()) {
		sprite->set_texture(Ref<Texture2D>());
		return;
	}
	Ref<Texture2D> tex;
	if (ResourceLoader::exists(p_path)) {
		tex = ResourceLoader::load(p_path);
	}
	if (tex.is_null()) {
		Ref<Image> img = Image::load_from_file(p_path);
		if (img.is_valid() && !img->is_empty()) {
			tex = ImageTexture::create_from_image(img);
		}
	}
	if (tex.is_valid()) {
		sprite->set_texture(tex);
		_update_sprite_region();
	}
}

void ClockGearsEvent::set_sprite_hframes(int p_hframes) {
	sprite_hframes = MAX(1, p_hframes);
	_update_sprite_region();
}

void ClockGearsEvent::set_sprite_vframes(int p_vframes) {
	sprite_vframes = MAX(1, p_vframes);
	_update_sprite_region();
}

void ClockGearsEvent::set_sprite_frame(int p_frame) {
	sprite_frame = MAX(0, p_frame);
	_update_sprite_region();
}

void ClockGearsEvent::set_direction(int p_dir) {
	if (!ClockGearsCharacterStateRegistry::is_valid_direction(p_dir)) {
		return;
	}
	direction = p_dir;
	if (behavior_type != 0) {
		if (db_record_loaded) {
			_update_behavior_visual_frame();
		} else {
			update_active_page();
		}
		return;
	}
	if (sprite_vframes == 4 || sprite_vframes >= 8) {
		int hf = MAX(1, sprite_hframes);
		int col = sprite_frame % hf;
		int row = ClockGearsCharacterStateRegistry::direction_to_sheet_row(direction, sprite_vframes);
		row = CLAMP(row, 0, sprite_vframes - 1);
		sprite_frame = row * hf + col;
		_update_sprite_region();
	}
}

int ClockGearsEvent::behavior_type_to_category(int p_type) {
	switch (p_type) {
		case 1: return CAT_NPCS;
		case 2: return CAT_ARPG_NPCS;
		case 3: return CAT_ARPG_ENEMIES;
		default: return -1;
	}
}

void ClockGearsEvent::set_behavior_type(int p_type) {
	behavior_type = CLAMP(p_type, 0, 3);
	update_active_page();
}

void ClockGearsEvent::set_behavior_ref_id(int p_id) {
	behavior_ref_id = MAX(0, p_id);
	if (behavior_type != 0) {
		update_active_page();
	}
}

void ClockGearsEvent::_update_behavior_sprite(const Dictionary &p_rec, bool p_found) {
	if (!sprite) {
		return;
	}
	auto apply_missing_marker = [&]() {
		sprite->set_region_enabled(false);
		sprite->set_texture(ClockGearsCharacterStateRegistry::get_missing_question_marker_texture());
		sprite_hframes = 1;
		sprite_vframes = 1;
		sprite_frame = 0;
		sprite->set_hframes(1);
		sprite->set_vframes(1);
		sprite->set_frame(0);
		sprite->set_position(Vector2(0, 0));
	};

	if (!p_found) {
		apply_missing_marker();
		return;
	}

	Dictionary sp_dict = p_rec.get("sprite_data", Dictionary());
	if (!sp_dict.is_empty()) {
		ClockGearsSpriteSheetData sheet;
		sheet.from_dict(sp_dict);
		bool dir_idle = bool(p_rec.get("directional_idle", false));
		ClockGearsSpriteAnimation resolved_anim;
		bool freeze_f0 = false;
		String res_slot;
		ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
		if (reg && reg->resolve_actor_animation(sheet, dir_idle, "quieto", direction, resolved_anim, freeze_f0, res_slot) && resolved_anim.get_frame_count() > 0) {
			ClockGearsAnimationFrame raw_f = resolved_anim.get_frame(0, 0, 0);
			String spath = raw_f.sheet_path.is_empty() ? resolved_anim.spritesheet_path : raw_f.sheet_path;
			if (spath.is_empty()) {
				spath = String(p_rec.get("graphic_path", ""));
			}
			Ref<Texture2D> tex;
			if (!spath.is_empty()) {
				if (ResourceLoader::exists(spath)) {
					tex = ResourceLoader::load(spath);
				}
				if (tex.is_null()) {
					Ref<Image> img = Image::load_from_file(spath);
					if (img.is_valid() && !img->is_empty()) {
						tex = ImageTexture::create_from_image(img);
					}
				}
			}
			if (tex.is_valid()) {
				int tw = tex->get_width();
				int th = tex->get_height();
				ClockGearsAnimationFrame f = resolved_anim.get_frame(0, tw, th);
				Rect2 reg_rect(f.region);
				if (reg_rect.size.x <= 0 || reg_rect.size.y <= 0 || reg_rect.position.x + reg_rect.size.x > tw || reg_rect.position.y + reg_rect.size.y > th) {
					reg_rect = Rect2(0, 0, tw, th);
				}
				sprite->set_texture(tex);
				sprite->set_region_enabled(true);
				sprite->set_region_rect(reg_rect);
				sprite_hframes = 1;
				sprite_vframes = 1;
				sprite_frame = 0;
				sprite->set_hframes(1);
				sprite->set_vframes(1);
				sprite->set_frame(0);
				sprite->set_position(Vector2((32 - reg_rect.size.x) / 2, 32 - reg_rect.size.y));
				return;
			}
		}
	}

	String gp = String(p_rec.get("graphic_path", "")).strip_edges();
	if (!gp.is_empty()) {
		sprite_hframes = 1;
		sprite_vframes = 1;
		sprite_frame = 0;
		set_graphic_path(gp);
		if (sprite->get_texture().is_valid()) {
			return;
		}
	}

	apply_missing_marker();
}

void ClockGearsEvent::add_command(const Dictionary &p_cmd) {
	commands.push_back(p_cmd);
}

void ClockGearsEvent::set_self_switch(const String &p_letter, bool p_val) {
	self_switches[p_letter] = p_val;
	update_active_page();
}

bool ClockGearsEvent::get_self_switch(const String &p_letter) const {
	if (self_switches.has(p_letter)) {
		return bool(self_switches[p_letter]);
	}
	return false;
}

void ClockGearsEvent::set_self_switches(const Dictionary &p_sw) {
	self_switches = p_sw.duplicate(true);
	update_active_page();
}

void ClockGearsEvent::update_active_page() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();

	Array effective_pages = pages;
	bool using_db_behavior = (behavior_type != 0);
	Dictionary db_rec;
	bool db_found = false;

	if (using_db_behavior) {
		int cat = behavior_type_to_category(behavior_type);
		if (dm && cat >= 0 && behavior_ref_id > 0) {
			db_rec = dm->get_record_by_id(cat, behavior_ref_id);
			if (int(db_rec.get("id", 0)) > 0) {
				db_found = true;
				Variant vp = db_rec.get("event_pages", Array());
				effective_pages = (vp.get_type() == Variant::ARRAY) ? Array(vp) : Array();
			}
		}
		db_record_loaded = db_found;
		cached_db_rec = db_found ? db_rec.duplicate(true) : Dictionary();
		if (db_found) {
			_apply_behavior_record_config(db_rec, 120.0f);
		}
		_update_behavior_sprite(db_rec, db_found);
		_sync_runtime_entity_components();
		if (!Engine::get_singleton()->is_editor_hint()) {
			if (!spawn_origin_set) {
				reset_spawn_origin();
			}
			set_physics_process(true);
		}
		if (!db_found || effective_pages.is_empty()) {
			active_page_index = 0;
			set_commands(Array());
			queue_redraw();
			return;
		}
	} else {
		db_record_loaded = false;
		cached_db_rec.clear();
		_clear_runtime_entity_components();
		set_physics_process(false);
		if (pages.is_empty()) {
			return;
		}
	}

	int matched_page = -1;
	for (int i = effective_pages.size() - 1; i >= 0; i--) {
		Dictionary p = effective_pages[i];
		bool ok = true;

		// Check Switch 1
		if (bool(p.get("sw1_en", false))) {
			String sw_name = p.get("sw1_name", "");
			if (!dm || !dm->get_switch(sw_name)) {
				ok = false;
			}
		}
		// Check Switch 2
		if (ok && bool(p.get("sw2_en", false))) {
			String sw_name = p.get("sw2_name", "");
			if (!dm || !dm->get_switch(sw_name)) {
				ok = false;
			}
		}
		// Check Self Switch (A, B, C, D)
		if (ok && bool(p.get("self_sw_en", false))) {
			String s_letter = p.get("self_sw_letter", "A");
			if (!get_self_switch(s_letter)) {
				ok = false;
			}
		}
		// Check Variable (>= val)
		if (ok && bool(p.get("var_en", false))) {
			String var_name = p.get("var_name", "");
			int target_val = int(p.get("var_val", 0));
			int cur_val = dm ? dm->get_variable(var_name) : 0;
			if (cur_val < target_val) {
				ok = false;
			}
		}
		// Check Item
		if (ok && bool(p.get("item_en", false))) {
			String item_name = p.get("item_name", "");
			if (!dm || !dm->has_item(item_name)) {
				ok = false;
			}
		}

		if (ok) {
			matched_page = i;
			break;
		}
	}

	if (matched_page < 0) {
		matched_page = 0; // Default to page 0
	}

	active_page_index = matched_page;
	Dictionary ap = effective_pages[matched_page];

	// Apply properties from active page
	set_trigger_type(int(ap.get("trigger", 0)));
	set_priority_type(int(ap.get("priority", 1)));
	set_through(ap.get("through", false));
	set_direction_fix(ap.get("dir_fix", false));

	if (!using_db_behavior) {
		set_sprite_hframes(int(ap.get("hframes", 1)));
		set_sprite_vframes(int(ap.get("vframes", 1)));
		set_sprite_frame(int(ap.get("frame", 0)));
		set_graphic_path(ap.get("graphic_path", ""));

		set_move_type(int(ap.get("move_type", 0)));
		set_move_speed(int(ap.get("speed", 3)) * 40.0f);
		set_move_frequency(int(ap.get("freq", 3)));
	} else {
		float base_ev_speed = float(int(ap.get("speed", 3)) * 40.0f);
		float walk_pct = MAX(1.0f, float(db_rec.get("walk_speed_pct", 100)));
		set_move_speed(base_ev_speed * (walk_pct / 100.0f));
		if (db_found) {
			_apply_behavior_record_config(db_rec, base_ev_speed);
		}
	}
	set_commands(ap.get("commands", Array()));
	if (using_db_behavior && behavior_type == 3) {
		sync_arpg_defeated_state();
	}
	queue_redraw();
}

void ClockGearsEvent::execute() {
	emit_signal("event_triggered", this);
	print_line(vformat(String::utf8("[ClockGears Event] Ejecutando Evento '%s' (ID: %d) [Página activa: %d] con %d comandos."),
			event_name, event_id, active_page_index + 1, commands.size()));

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();

	for (int i = 0; i < commands.size(); i++) {
		Dictionary cmd = commands[i];
		String type = cmd.get("type", "");

		if (type == "message") {
			String raw_txt = cmd.get("text", "");
			String formatted = dm ? dm->format_text(raw_txt) : raw_txt;
			String face = cmd.get("face", "");
			print_line(vformat("  [Mensaje] %s%s", face.is_empty() ? "" : vformat("(Cara: %s) ", face.get_file()), formatted));
		} else if (type == "switch") {
			String sw_name = cmd.get("name", "");
			bool val = cmd.get("val", true);
			if (dm) {
				dm->set_switch(sw_name, val);
			}
			print_line(vformat("  [Interruptor] %s = %s", sw_name, val ? "ON" : "OFF"));
		} else if (type == "self_switch") {
			String letter = cmd.get("letter", "A");
			bool val = cmd.get("val", true);
			set_self_switch(letter, val);
			print_line(vformat("  [Interruptor Local] %s = %s", letter, val ? "ON" : "OFF"));
		} else if (type == "variable") {
			String var_name = cmd.get("name", "");
			String op = cmd.get("op", "=");
			int val = int(cmd.get("val", 0));
			if (dm) {
				int cur = dm->get_variable(var_name);
				if (op == "+") cur += val;
				else if (op == "-") cur -= val;
				else if (op == "*") cur *= val;
				else if (op == "/" && val != 0) cur /= val;
				else if (op == "%" && val != 0) cur %= val;
				else cur = val;
				dm->set_variable(var_name, cur);
				print_line(vformat("  [Variable] %s %s %d (Nuevo valor: %d)", var_name, op, val, cur));
			}
		} else if (type == "wait") {
			int frames = int(cmd.get("frames", 60));
			print_line(vformat("  [Esperar] %d fotogramas", frames));
		} else if (type == "teleport") {
			String map_dest = cmd.get("map", "");
			int tx = int(cmd.get("x", 0));
			int ty = int(cmd.get("y", 0));
			String dir = cmd.get("dir", "");
			print_line(vformat("  [Transferir] Mapa '%s' a (%d, %d) mirando %s", map_dest, tx, ty, dir));
		} else if (type == "move_route") {
			String target = cmd.get("target", "Este Evento");
			String action = cmd.get("action", "");
			print_line(vformat("  [Fijar Ruta] Objetivo: %s -> %s", target, action));
		} else if (type == "item") {
			String iname = cmd.get("name", "");
			String op = cmd.get("op", "+");
			int qty = int(cmd.get("qty", 1));
			if (dm) {
				dm->change_item(iname, (op == "-") ? -qty : qty);
			}
			print_line(vformat("  [Objeto] %s%d %s", op, qty, iname));
		} else if (type == "gold") {
			String op = cmd.get("op", "+");
			int amount = int(cmd.get("amount", 0));
			if (dm) {
				dm->change_gold((op == "-") ? -amount : amount);
			}
			print_line(vformat("  [Dinero] %s%d Oro", op, amount));
		} else if (type == "hp_mp") {
			String stat = cmd.get("stat", "HP");
			String op = cmd.get("op", "+");
			int amt = int(cmd.get("amount", 50));
			print_line(vformat("  [Cambiar %s] %s%d", stat, op, amt));
		} else if (type == "audio" || type == "bgm" || type == "se") {
			String afile = cmd.get("file", "");
			print_line(vformat("  [Audio] Reproduciendo '%s'", afile));
		} else if (type == "flash") {
			print_line(vformat("  [Flash] Color %s (%d f)", cmd.get("color", ""), int(cmd.get("duration", 30))));
		} else if (type == "tint") {
			print_line(vformat(String::utf8("  [Teñir] %s (%d f)"), cmd.get("preset", ""), int(cmd.get("duration", 60))));
		} else if (type == "fade") {
			print_line(vformat("  [Fundido] %s (%d f)", cmd.get("mode", "Oscurecer"), int(cmd.get("duration", 30))));
		} else if (type == "battle") {
			print_line(vformat("  [Batalla] Iniciando combate contra tropa '%s'", cmd.get("troop", "")));
		}
	}
}

ClockGearsEvent::ClockGearsEvent() {
	sprite = memnew(Sprite2D);
	sprite->set_centered(false);
	sprite->set_texture_filter(CanvasItem::TEXTURE_FILTER_NEAREST);
	add_child(sprite);
}

void ClockGearsEvent::set_body_shape(const Dictionary &p_shape) {
	body_shape.from_dict(p_shape);
	_sync_runtime_entity_components();
}

void ClockGearsEvent::set_body_through(bool p_through) {
	body_through = p_through;
}

ClockGearsDetectionZone2D *ClockGearsEvent::get_zone_node(int p_idx) const {
	if (p_idx < 0 || p_idx >= zone_nodes.size()) {
		return nullptr;
	}
	return zone_nodes[p_idx];
}

void ClockGearsEvent::reset_spawn_origin() {
	spawn_origin_px = get_position();
	spawn_direction = direction;
	spawn_origin_set = true;
}

String ClockGearsEvent::get_ai_state_name() const {
	switch (ai_state) {
		case CG_AI_STATE_CHASE: return "perseguir";
		case CG_AI_STATE_FLEE: return "huir";
		case CG_AI_STATE_RETURN_HOME: return "volver_puesto";
		case CG_AI_STATE_STOPPED: return "detenerse";
		default: return (behavior_type == 3) ? ai_type_str : npc_move_type_str;
	}
}

Array ClockGearsEvent::get_current_astar_path() const {
	Array out;
	for (int i = 0; i < astar_path.size(); i++) {
		out.push_back(astar_path[i]);
	}
	return out;
}

void ClockGearsEvent::_apply_behavior_record_config(const Dictionary &p_rec, float p_base_ev_speed) {
	Dictionary bsd = p_rec.get("body_shape", Dictionary());
	if (!bsd.is_empty()) {
		body_shape.from_dict(bsd);
	} else {
		body_shape = ClockGearsHitShape::make_circle(12.0f);
	}
	body_through = bool(p_rec.get("body_through", false));
	avoid_obstacles = p_rec.has("avoid_obstacles") ? bool(p_rec["avoid_obstacles"]) : true;
	path_recalc_ms = MAX(50, int(p_rec.get("path_recalc_ms", 250)));
	path_max_nodes = MAX(16, int(p_rec.get("path_max_nodes", 256)));

	float walk_pct = MAX(1.0f, float(p_rec.get("walk_speed_pct", 100)));
	float run_pct = MAX(1.0f, float(p_rec.get("run_speed_pct", 100)));
	attack_speed_pct = MAX(1.0f, float(p_rec.get("attack_speed_pct", 100)));
	walk_move_speed = MAX(8.0f, p_base_ev_speed * (walk_pct / 100.0f));
	run_move_speed = MAX(8.0f, p_base_ev_speed * 1.5f * (run_pct / 100.0f));

	wander_radius = MAX(0.0f, float(p_rec.get("wander_radius", 3.0)));
	patrol_loop = bool(p_rec.get("patrol_loop", false));
	patrol_points.clear();
	if (p_rec.has("patrol_points") && p_rec["patrol_points"].get_type() == Variant::ARRAY) {
		Array pts = p_rec["patrol_points"];
		for (int i = 0; i < pts.size(); i++) {
			if (pts[i].get_type() == Variant::DICTIONARY) {
				Dictionary pd = pts[i];
				patrol_points.push_back(Vector2(float(pd.get("x", 0.0)), float(pd.get("y", 0.0))));
			} else if (pts[i].get_type() == Variant::VECTOR2 || pts[i].get_type() == Variant::VECTOR2I) {
				patrol_points.push_back(Vector2(pts[i]));
			}
		}
	}

	lose_sight_sec = MAX(0.0f, float(p_rec.get("lose_sight_sec", 4.0)));
	min_distance = MAX(0.0f, float(p_rec.get("min_distance", 3.0)));
	max_distance = MAX(min_distance, float(p_rec.get("max_distance", 5.0)));
	ai_type_str = String(p_rec.get("ai_type", "persigue_y_ataca")).strip_edges().to_lower();
	npc_move_type_str = String(p_rec.get("npc_move_type", "quieto")).strip_edges().to_lower();
	follow_distance = MAX(0.5f, float(p_rec.get("follow_distance", 2.0)));
	face_player_on_event = p_rec.has("face_player_on_event") ? bool(p_rec["face_player_on_event"]) : true;

	Variant dz = p_rec.get("detection_zones", Array());
	detection_zones_cfg = (dz.get_type() == Variant::ARRAY) ? Array(dz).duplicate(true) : Array();

	Dictionary sp_dict = p_rec.get("sprite_data", Dictionary());
	if (!sp_dict.is_empty()) {
		cached_sprite_data.from_dict(sp_dict);
	} else {
		cached_sprite_data = ClockGearsSpriteSheetData();
	}
	cached_directional_idle = bool(p_rec.get("directional_idle", false));
	cached_slot_textures.clear();

	if (behavior_type == 3) {
		arpg_max_hp = MAX(1, int(p_rec.get("hp", 50)));
		arpg_atk = MAX(0, int(p_rec.get("atk", 10)));
		arpg_def = MAX(0, int(p_rec.get("def", 0)));
		arpg_exp_reward = MAX(0, int(p_rec.get("exp", 0)));
		arpg_gold_reward = MAX(0, int(p_rec.get("gold", 0)));
		arpg_knockback_resist = MAX(0.0f, float(p_rec.get("knockback_resist", 0.0)));
		if (!arpg_stats_initialized || cached_behavior_ref_id != behavior_ref_id) {
			arpg_hp = arpg_max_hp;
			arpg_stats_initialized = true;
			cached_behavior_ref_id = behavior_ref_id;
		} else {
			arpg_hp = CLAMP(arpg_hp, 0, arpg_max_hp);
		}
	}
}

void ClockGearsEvent::_clear_runtime_entity_components() {
	if (entity_body) {
		entity_body->queue_free();
		entity_body = nullptr;
		entity_body_shape_owner = 0;
	}
	if (zones_root) {
		zones_root->queue_free();
		zones_root = nullptr;
	}
	zone_nodes.clear();
	cached_body_sig = "";
	cached_zones_sig = "";
}

void ClockGearsEvent::_sync_runtime_entity_components() {
	if (Engine::get_singleton()->is_editor_hint() || behavior_type == 0) {
		_clear_runtime_entity_components();
		return;
	}
	ClockGearsMap *map = ClockGearsEntityNav::find_parent_map(this);
	int ts = map ? MAX(16, map->get_grid_resolution()) : 32;
	Vector2 half_tile(ts * 0.5f, ts * 0.5f);

	String body_sig = JSON::stringify(body_shape.to_dict()) + ":" + String::num_int64(behavior_type) + ":" + String::num_int64(ts);
	if (!entity_body || body_sig != cached_body_sig) {
		if (!entity_body) {
			entity_body = memnew(CharacterBody2D);
			entity_body->set_name("RuntimeEntityBody");
			entity_body->set_collision_mask(0);
			entity_body->set_meta("cg_event_owner", uint64_t(get_instance_id()));
			entity_body_shape_owner = entity_body->create_shape_owner(entity_body);
			add_child(entity_body);
		}
		entity_body->set_position(half_tile);
		_apply_entity_body_collision_state();
		ClockGearsShapeGeometry::populate_collision_object_shapes(entity_body, entity_body_shape_owner, body_shape, Vector2());
		cached_body_sig = body_sig;
	} else {
		_apply_entity_body_collision_state();
	}

	String zones_sig = JSON::stringify(detection_zones_cfg) + ":" + String::num_int64(ts);
	if (!zones_root || zones_sig != cached_zones_sig) {
		if (!zones_root) {
			zones_root = memnew(Node2D);
			zones_root->set_name("RuntimeDetectionZones");
			add_child(zones_root);
		}
		zones_root->set_position(half_tile);
		for (int i = 0; i < zone_nodes.size(); i++) {
			if (zone_nodes[i]) {
				zone_nodes[i]->queue_free();
			}
		}
		zone_nodes.clear();
		for (int i = 0; i < detection_zones_cfg.size(); i++) {
			if (detection_zones_cfg[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			ClockGearsDetectionZone2D *z = memnew(ClockGearsDetectionZone2D);
			zones_root->add_child(z);
			z->configure(this, i, Dictionary(detection_zones_cfg[i]), ts);
			zone_nodes.push_back(z);
		}
		cached_zones_sig = zones_sig;
	}
}

void ClockGearsEvent::_update_behavior_visual_frame() {
	if (!sprite || !db_record_loaded) {
		return;
	}
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (!reg) {
		return;
	}
	ClockGearsSpriteAnimation resolved_anim;
	bool freeze_f0 = false;
	String res_slot;
	if (!reg->resolve_actor_animation(cached_sprite_data, cached_directional_idle, ev_action_state, direction, resolved_anim, freeze_f0, res_slot)) {
		return;
	}
	ev_resolved_slot = res_slot;
	int fcount = resolved_anim.get_frame_count();
	if (fcount <= 0) {
		return;
	}
	int fi = freeze_f0 ? 0 : (ev_anim_frame % fcount);
	ClockGearsAnimationFrame raw_f = resolved_anim.get_frame(fi, 0, 0);
	String spath = raw_f.sheet_path.is_empty() ? resolved_anim.spritesheet_path : raw_f.sheet_path;
	if (spath.is_empty()) {
		spath = String(cached_db_rec.get("graphic_path", ""));
	}
	if (spath.is_empty()) {
		return;
	}
	Ref<Texture2D> tex;
	if (cached_slot_textures.has(spath)) {
		tex = cached_slot_textures[spath];
	} else {
		if (ResourceLoader::exists(spath)) {
			tex = ResourceLoader::load(spath);
		}
		if (tex.is_null()) {
			Ref<Image> img = Image::load_from_file(spath);
			if (img.is_valid() && !img->is_empty()) {
				tex = ImageTexture::create_from_image(img);
			}
		}
		if (tex.is_valid()) {
			cached_slot_textures[spath] = tex;
		}
	}
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
		sprite_hframes = 1;
		sprite_vframes = 1;
		sprite_frame = 0;
		sprite->set_hframes(1);
		sprite->set_vframes(1);
		sprite->set_frame(0);
		sprite->set_position(Vector2((32 - reg_rect.size.x) / 2, 32 - reg_rect.size.y));
	}
}

void ClockGearsEvent::_advance_behavior_animation(float p_delta) {
	if (!db_record_loaded) {
		return;
	}
	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (!reg) {
		return;
	}
	ClockGearsSpriteAnimation resolved_anim;
	bool freeze_f0 = false;
	String res_slot;
	if (!reg->resolve_actor_animation(cached_sprite_data, cached_directional_idle, ev_action_state, direction, resolved_anim, freeze_f0, res_slot)) {
		if (arpg_dying && ev_action_state == "morir") {
			arpg_dying = false;
			arpg_dead = true;
			set_visible(false);
			_apply_entity_body_collision_state();
		}
		return;
	}
	ev_resolved_slot = res_slot;
	int fcount = resolved_anim.get_frame_count();
	if (fcount <= 0) {
		if (arpg_dying && ev_action_state == "morir") {
			arpg_dying = false;
			arpg_dead = true;
			set_visible(false);
			_apply_entity_body_collision_state();
		}
		return;
	}
	if (freeze_f0) {
		ev_anim_frame = 0;
		ev_anim_timer = 0.0f;
		_update_behavior_visual_frame();
		if (arpg_dying && ev_action_state == "morir") {
			arpg_dying = false;
			arpg_dead = true;
			set_visible(false);
			_apply_entity_body_collision_state();
		}
		return;
	}
	if (ev_anim_frame >= fcount) {
		ev_anim_frame = 0;
	}
	float dur = MAX(0.01f, resolved_anim.get_frame_duration(ev_anim_frame));
	float spd_mult = 1.0f;
	if (ev_action_state == "atacar") {
		spd_mult = MAX(0.1f, attack_speed_pct / 100.0f);
	}
	ev_anim_timer += p_delta * spd_mult;
	if (ev_anim_timer >= dur) {
		ev_anim_timer -= dur;
		if (ev_anim_frame + 1 < fcount) {
			ev_anim_frame++;
			_update_behavior_visual_frame();
			if (ev_action_state == "atacar") {
				_process_enemy_attack_hits(ClockGearsEntityNav::find_parent_map(this));
			}
			return;
		} else if (ev_action_state == "morir" && arpg_dying) {
			arpg_dying = false;
			arpg_dead = true;
			ev_anim_frame = 0;
			ev_anim_timer = 0.0f;
			set_visible(false);
			_apply_entity_body_collision_state();
			return;
		} else if (resolved_anim.loop && ev_action_state != "atacar" && ev_action_state != "morir") {
			ev_anim_frame = 0;
		} else if (ev_action_state == "atacar") {
			_end_enemy_attack_swing();
			ev_action_state = "quieto";
			ev_anim_frame = 0;
			ev_anim_timer = 0.0f;
		}
	}
	_update_behavior_visual_frame();
}

Node *ClockGearsEvent::_resolve_target_node(ClockGearsMap *p_map) const {
	if (target_object_id.is_valid()) {
		Node *n = Object::cast_to<Node>(ObjectDB::get_instance(target_object_id));
		if (n && n->is_inside_tree()) {
			return n;
		}
	}
	for (int i = 0; i < zone_nodes.size(); i++) {
		if (zone_nodes[i] && zone_nodes[i]->has_any_inside()) {
			Node *n = zone_nodes[i]->get_first_inside_entity();
			if (n) {
				return n;
			}
		}
	}
	return p_map ? static_cast<Node *>(p_map->get_runtime_player()) : nullptr;
}

bool ClockGearsEvent::_has_any_target_in_zones() const {
	for (int i = 0; i < zone_nodes.size(); i++) {
		if (zone_nodes[i] && zone_nodes[i]->has_any_inside()) {
			return true;
		}
	}
	return false;
}

void ClockGearsEvent::_face_toward_world_pos(const Vector2 &p_target_pos) {
	if (direction_fix) {
		return;
	}
	Vector2 diff = p_target_pos - get_position();
	if (diff.length_squared() < 0.01f) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	bool diag = dm && int(dm->get_system_config().get("movement_directions", 4)) == 8;
	int ix = (Math::abs(diff.x) > 4.0f) ? (diff.x > 0.0f ? 1 : -1) : 0;
	int iy = (Math::abs(diff.y) > 4.0f) ? (diff.y > 0.0f ? 1 : -1) : 0;
	if (!diag && ix != 0 && iy != 0) {
		if (Math::abs(diff.x) >= Math::abs(diff.y)) {
			iy = 0;
		} else {
			ix = 0;
		}
	}
	if (ix != 0 || iy != 0) {
		int d = ClockGearsCharacterStateRegistry::vector_to_direction(Vector2i(ix, iy));
		if (d != direction) {
			direction = d;
			_update_behavior_visual_frame();
		}
	}
}

void ClockGearsEvent::notify_zone_sight(bool p_in, Node *p_target) {
	if (p_in) {
		time_without_sight = 0.0f;
		if (p_target) {
			target_object_id = p_target->get_instance_id();
		}
	}
}

void ClockGearsEvent::execute_zone_actions(const Array &p_actions, Node *p_target) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	ClockGearsMap *map = ClockGearsEntityNav::find_parent_map(this);

	for (int i = 0; i < p_actions.size(); i++) {
		String act_type;
		Dictionary act_dict;
		if (p_actions[i].get_type() == Variant::DICTIONARY) {
			act_dict = p_actions[i];
			act_type = String(act_dict.get("type", "")).strip_edges().to_lower();
		} else {
			act_type = String(p_actions[i]).strip_edges().to_lower();
		}

		if (act_type == "perseguir") {
			ai_state = CG_AI_STATE_CHASE;
			if (p_target) {
				target_object_id = p_target->get_instance_id();
			}
			time_without_sight = 0.0f;
			astar_timer = 0.0f;
			astar_no_path = false;
		} else if (act_type == "huir") {
			ai_state = CG_AI_STATE_FLEE;
			if (p_target) {
				target_object_id = p_target->get_instance_id();
			}
			time_without_sight = 0.0f;
			astar_timer = 0.0f;
		} else if (act_type == "volver_puesto") {
			ai_state = CG_AI_STATE_RETURN_HOME;
			astar_timer = 0.0f;
			astar_no_path = false;
		} else if (act_type == "mirar_jugador") {
			Node *look_t = p_target ? p_target : (map ? static_cast<Node *>(map->get_runtime_player()) : nullptr);
			Node2D *n2 = Object::cast_to<Node2D>(look_t);
			if (n2) {
				_face_toward_world_pos(n2->get_position());
			}
		} else if (act_type == "detenerse") {
			ai_state = CG_AI_STATE_STOPPED;
			ev_action_state = "quieto";
			astar_path.clear();
		} else if (act_type == "ejecutar_evento") {
			update_active_page();
			ClockGearsEventInterpreter *interp = map ? map->get_runtime_interpreter() : nullptr;
			if (interp && !interp->is_running()) {
				if (!commands.is_empty()) {
					interp->start_event(this);
				} else {
					execute();
				}
			} else if (!interp) {
				execute();
			}
		} else if (act_type == "activar_interruptor") {
			if (dm) {
				String sname = String(act_dict.get("switch_name", ""));
				bool sval = act_dict.has("value") ? bool(act_dict["value"]) : true;
				if (!sname.is_empty()) {
					dm->set_switch(sname, sval);
				}
			}
		} else if (act_type == "poner_variable") {
			if (dm) {
				String vname = String(act_dict.get("var_name", ""));
				int vval = int(act_dict.get("value", 0));
				String vop = String(act_dict.get("op", "="));
				if (!vname.is_empty()) {
					int cur = dm->get_variable(vname);
					if (vop == "+") cur += vval;
					else if (vop == "-") cur -= vval;
					else cur = vval;
					dm->set_variable(vname, cur);
				}
			}
		} else if (act_type == "atacar") {
			if (ev_action_state != "atacar") {
				start_arpg_attack(p_target);
			}
		}
	}
}

Vector2 ClockGearsEvent::_compute_move_toward(ClockGearsMap *p_map, const Vector2 &p_target_px, float p_delta, int p_tile_size) {
	Vector2 cur_pos = get_position();
	Vector2 direct = p_target_px - cur_pos;
	if (direct.length_squared() < 4.0f) {
		return Vector2();
	}

	if (!avoid_obstacles || !p_map) {
		astar_no_path = false;
		astar_path.clear();
		return direct.normalized();
	}

	Vector2 half(p_tile_size * 0.5f, p_tile_size * 0.5f);
	Vector2i start_cell(
			int(Math::floor((cur_pos.x + half.x) / float(p_tile_size))),
			int(Math::floor((cur_pos.y + half.y) / float(p_tile_size))));
	Vector2i goal_cell(
			int(Math::floor((p_target_px.x + half.x) / float(p_tile_size))),
			int(Math::floor((p_target_px.y + half.y) / float(p_tile_size))));

	if (start_cell == goal_cell) {
		astar_no_path = false;
		astar_path.clear();
		return direct.normalized();
	}

	astar_timer -= p_delta;
	if (astar_timer <= 0.0f || astar_path.is_empty() || astar_path_idx >= astar_path.size()) {
		astar_timer = float(path_recalc_ms) / 1000.0f;
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		bool allow_diag = dm && int(dm->get_system_config().get("movement_directions", 4)) == 8;
		bool ok = ClockGearsEntityNav::compute_grid_astar_path(p_map, start_cell, goal_cell, allow_diag, path_max_nodes, astar_path);
		astar_path_idx = 0;
		astar_no_path = !ok;
	}

	if (astar_no_path || astar_path.is_empty()) {
		return Vector2();
	}

	while (astar_path_idx < astar_path.size()) {
		Vector2 wp_px = Vector2(astar_path[astar_path_idx]) * float(p_tile_size);
		Vector2 diff = wp_px - cur_pos;
		if (diff.length() <= 5.0f && astar_path_idx + 1 < astar_path.size()) {
			astar_path_idx++;
			continue;
		}
		if (diff.length_squared() < 1.0f) {
			astar_path_idx++;
			continue;
		}
		return diff.normalized();
	}
	return direct.normalized();
}

void ClockGearsEvent::_apply_entity_body_collision_state() {
	bool disable_col = arpg_dying || arpg_dead || !is_visible();
	if (entity_body) {
		entity_body->set_collision_layer(disable_col ? 0 : ((behavior_type == 3) ? CG_PHYSICS_LAYER_ENEMY : CG_PHYSICS_LAYER_NPC));
		if (entity_body_shape_owner != 0) {
			entity_body->shape_owner_set_disabled(entity_body_shape_owner, disable_col);
		}
	}
	for (int i = 0; i < zone_nodes.size(); i++) {
		if (zone_nodes[i]) {
			zone_nodes[i]->set_monitoring(!disable_col);
		}
	}
}

Vector2 ClockGearsEvent::get_arpg_world_center() const {
	ClockGearsMap *map = ClockGearsEntityNav::find_parent_map(this);
	int ts = map ? MAX(16, map->get_grid_resolution()) : 32;
	if (sprite && sprite->is_region_enabled()) {
		Size2 rsz = sprite->get_region_rect().size;
		if (rsz.x > 0.0f && rsz.y > 0.0f) {
			return get_position() + sprite->get_position() + rsz * 0.5f;
		}
	}
	return get_position() + Vector2(ts * 0.5f, ts * 0.5f);
}

Dictionary ClockGearsEvent::get_arpg_effective_hurtbox() const {
	if (!is_arpg_alive()) {
		ClockGearsHurtboxData off;
		off.enabled = false;
		return off.to_dict();
	}
	ClockGearsHurtboxData hb = cached_sprite_data.get_effective_hurtbox(ev_resolved_slot, ev_anim_frame);
	return hb.to_dict();
}

void ClockGearsEvent::_grant_arpg_kill_rewards() {
	if (arpg_rewards_granted) {
		return;
	}
	arpg_rewards_granted = true;
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	ClockGearsMap *map = ClockGearsEntityNav::find_parent_map(this);
	if (dm) {
		if (map && event_id > 0) {
			dm->mark_arpg_enemy_defeated(map->get_respawn_state_key(), event_id);
		}
		if (arpg_gold_reward > 0) {
			dm->change_gold(arpg_gold_reward);
		}
		if (arpg_exp_reward > 0) {
			dm->grant_party_exp(arpg_exp_reward);
		}
		dm->resolve_arpg_enemy_drops(cached_db_rec);
	}
	emit_signal("arpg_defeated");
}

void ClockGearsEvent::_begin_enemy_attack_swing() {
	if (ev_swing_tracker.is_null()) {
		ev_swing_tracker.instantiate();
	}
	ev_attack_swing_id = ev_swing_tracker->start_new_swing();
}

void ClockGearsEvent::_end_enemy_attack_swing() {
	if (ev_swing_tracker.is_valid() && ev_attack_swing_id > 0) {
		ev_swing_tracker->end_swing(ev_attack_swing_id);
	}
	ev_attack_swing_id = 0;
}

void ClockGearsEvent::start_arpg_attack(Node *p_target) {
	if (arpg_dead || arpg_dying) {
		return;
	}
	ClockGearsMap *map = ClockGearsEntityNav::find_parent_map(this);
	Node2D *n2 = Object::cast_to<Node2D>(p_target);
	if (!n2 && map) {
		n2 = map->get_runtime_player();
	}
	if (n2 && !direction_fix) {
		_face_toward_world_pos(n2->get_position());
	}
	ev_action_state = "atacar";
	ev_anim_frame = 0;
	ev_anim_timer = 0.0f;
	ev_attack_timer = 0.45f * (100.0f / MAX(1.0f, attack_speed_pct));
	_begin_enemy_attack_swing();
	_update_behavior_visual_frame();
	_process_enemy_attack_hits(map);
}

void ClockGearsEvent::_process_enemy_attack_hits(ClockGearsMap *p_map) {
	if (behavior_type != 3 || !is_arpg_alive() || ev_action_state != "atacar" || !p_map || !p_map->is_arpg_effective()) {
		return;
	}
	ClockGearsPlayer *pl = p_map->get_runtime_player();
	if (!pl || !pl->is_arpg_alive()) {
		return;
	}
	if (ev_swing_tracker.is_null() || ev_attack_swing_id <= 0) {
		_begin_enemy_attack_swing();
	}
	int64_t pid = int64_t(pl->get_instance_id());
	if (ev_swing_tracker->has_hit_target(ev_attack_swing_id, pid)) {
		return;
	}

	ClockGearsCharacterStateRegistry *reg = ClockGearsCharacterStateRegistry::get_singleton();
	if (!reg) {
		return;
	}
	ClockGearsSpriteAnimation atk_anim;
	bool freeze_f0 = false;
	String res_slot;
	if (!reg->resolve_actor_animation(cached_sprite_data, cached_directional_idle, "atacar", direction, atk_anim, freeze_f0, res_slot) || freeze_f0) {
		return;
	}
	int fcount = atk_anim.get_frame_count();
	if (fcount <= 0) {
		return;
	}
	int fi = CLAMP(ev_anim_frame, 0, fcount - 1);
	if (fi < 0 || fi >= atk_anim.frames_data.size() || atk_anim.frames_data[fi].hitbox.is_empty()) {
		return;
	}
	const ClockGearsFrameHitboxData &hb = atk_anim.frames_data[fi].hitbox;
	Array shapes_arr;
	shapes_arr.resize(hb.shapes.size());
	for (int si = 0; si < hb.shapes.size(); si++) {
		shapes_arr[si] = hb.shapes[si].to_dict();
	}
	Dictionary pl_hurt = pl->get_arpg_effective_hurtbox();
	Vector2 atk_origin = get_arpg_world_center();
	Vector2 pl_origin = pl->get_arpg_world_center();
	if (!ClockGearsShapeGeometry::check_attack_hits_hurtbox(shapes_arr, atk_origin, direction, pl_hurt, pl_origin, pl->get_direction(), 0, 0)) {
		return;
	}

	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary eff_stats = (dm && pl->get_actor_id() > 0) ? dm->get_actor_effective_stats(pl->get_actor_id()) : Dictionary();
	Dictionary attacker_dict;
	attacker_dict["atk"] = MAX(0, arpg_atk);
	attacker_dict["def"] = MAX(0, arpg_def);
	attacker_dict["mat"] = MAX(0, int(cached_db_rec.get("mat", arpg_atk)));
	attacker_dict["mdf"] = MAX(0, int(cached_db_rec.get("mdf", arpg_def)));
	attacker_dict["luk"] = MAX(0, int(cached_db_rec.get("luk", 0)));

	Dictionary defender_dict;
	defender_dict["def"] = eff_stats.is_empty() ? 0 : MAX(0, int(eff_stats.get("def", 0)));
	defender_dict["atk"] = eff_stats.is_empty() ? 10 : MAX(0, int(eff_stats.get("atk", 10)));

	String atk_elem_key = String(cached_db_rec.get("element_key", cached_db_rec.get("element", ""))).strip_edges().to_lower();
	int atk_elem_id = int(cached_db_rec.get("element_id", 0));
	if (atk_elem_id <= 0 && atk_elem_key.is_empty()) {
		Variant tr_v = cached_db_rec.get("traits", Array());
		if (tr_v.get_type() == Variant::ARRAY) {
			atk_elem_id = ClockGearsDataManager::resolve_traits_attack_element_id(Array(tr_v), atk_elem_key);
		}
	}
	if (atk_elem_key.is_empty()) {
		atk_elem_key = "fisico";
	}
	if (atk_elem_id <= 0 && ClockGearsSystemCatalog::get_singleton()) {
		atk_elem_id = ClockGearsSystemCatalog::get_singleton()->resolve_element_id(atk_elem_key);
	}

	float elem_mult = 1.0f;
	if (dm && pl->get_actor_id() > 0) {
		elem_mult = dm->get_actor_element_rate(pl->get_actor_id(), atk_elem_id, atk_elem_key);
	}
	double hurt_mult_pct = MAX(0.0, double(pl_hurt.get("damage_mult_pct", 100.0)));
	bool is_crit = false;
	int final_dmg = ClockGearsDataManager::compute_arpg_hit_damage(attacker_dict, defender_dict, elem_mult, hb.damage_mult_pct, hurt_mult_pct, is_crit);

	Vector2 hit_dir = pl_origin - atk_origin;
	if (hit_dir.length_squared() < 1.0f) {
		hit_dir = Vector2(ClockGearsPlayer::direction_to_vector(direction));
	} else {
		hit_dir = hit_dir.normalized();
	}

	if (pl->take_arpg_hit(final_dmg, hb.knockback, hit_dir, hb.hitstop_ms, is_crit, get_event_name())) {
		ev_swing_tracker->register_hit(ev_attack_swing_id, pid);
		if (hb.hitstop_ms > 0) {
			arpg_hitstop_timer = MAX(arpg_hitstop_timer, float(hb.hitstop_ms) / 1000.0f);
		}
	}
}

void ClockGearsEvent::_begin_arpg_death() {
	if (arpg_dying || arpg_dead) {
		return;
	}
	_end_enemy_attack_swing();
	arpg_dying = true;
	saved_body_through_before_death = body_through;
	body_through = true;
	ev_attack_timer = 0.0f;
	wander_has_target = false;
	astar_path.clear();
	ev_action_state = "morir";
	ev_anim_frame = 0;
	ev_anim_timer = 0.0f;
	_apply_entity_body_collision_state();
	_grant_arpg_kill_rewards();
	_update_behavior_visual_frame();
}

bool ClockGearsEvent::take_arpg_hit(int p_damage, float p_knockback_force, const Vector2 &p_hit_dir, int p_hitstop_ms, bool p_is_crit) {
	if (!is_arpg_alive()) {
		return false;
	}
	int dmg = MAX(1, p_damage);
	arpg_hp = MAX(0, arpg_hp - dmg);

	if (ClockGearsDebugOverlay *dbg = ClockGearsDebugOverlay::get_singleton()) {
		dbg->push_damage_number(get_arpg_world_center() + Vector2(0.0f, -16.0f), dmg, p_is_crit);
	}
	emit_signal("arpg_damaged", dmg, p_is_crit);

	if (p_hitstop_ms > 0) {
		arpg_hitstop_timer = float(p_hitstop_ms) / 1000.0f;
	}

	float eff_kb = MAX(0.0f, p_knockback_force - arpg_knockback_resist);
	if (eff_kb > 0.0f) {
		Vector2 dir_n = p_hit_dir;
		if (dir_n.length_squared() < 0.0001f) {
			dir_n = Vector2(ClockGearsPlayer::direction_to_vector(ClockGearsPlayer::opposite_direction(direction))).normalized();
		} else {
			dir_n = dir_n.normalized();
		}
		float dur = MAX(0.02f, knockback_duration);
		arpg_knockback_timer = dur;
		arpg_knockback_velocity = dir_n * (eff_kb / dur);
		ev_attack_timer = 0.0f;
		wander_has_target = false;
		astar_path.clear();
		if (arpg_hp > 0) {
			ev_action_state = "golpeado";
			ev_anim_frame = 0;
			ev_anim_timer = 0.0f;
			_update_behavior_visual_frame();
		}
	}

	if (arpg_hp <= 0) {
		arpg_hp_bar_timer = 0.0f;
		queue_redraw();
		_begin_arpg_death();
	} else {
		arpg_hp_bar_timer = 3.0f;
		queue_redraw();
	}
	return true;
}

void ClockGearsEvent::sync_arpg_defeated_state() {
	if (behavior_type != 3 || event_id <= 0) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	ClockGearsMap *map = ClockGearsEntityNav::find_parent_map(this);
	if (dm && map && dm->is_arpg_enemy_defeated(map->get_respawn_state_key(), event_id)) {
		if (!arpg_dead && !arpg_dying) {
			saved_body_through_before_death = body_through;
		}
		arpg_dead = true;
		arpg_dying = false;
		body_through = true;
		arpg_hp = 0;
		arpg_hp_bar_timer = 0.0f;
		arpg_knockback_timer = 0.0f;
		arpg_hitstop_timer = 0.0f;
		set_visible(false);
		queue_redraw();
		_apply_entity_body_collision_state();
	}
}

void ClockGearsEvent::respawn_arpg_enemy() {
	if (behavior_type != 3) {
		return;
	}
	arpg_dead = false;
	arpg_dying = false;
	arpg_rewards_granted = false;
	body_through = saved_body_through_before_death;
	arpg_hp_bar_timer = 0.0f;
	arpg_knockback_timer = 0.0f;
	arpg_hitstop_timer = 0.0f;
	arpg_hp = MAX(1, arpg_max_hp);
	if (spawn_origin_set) {
		set_position(spawn_origin_px);
		direction = spawn_direction;
	}
	ev_action_state = "quieto";
	ev_anim_frame = 0;
	ev_anim_timer = 0.0f;
	set_visible(true);
	queue_redraw();
	_apply_entity_body_collision_state();
	_update_behavior_visual_frame();
}

void ClockGearsEvent::_physics_tick_behavior(float p_delta) {
	if (p_delta <= 0.0f || arpg_dead || !is_visible()) {
		return;
	}
	if (arpg_hp_bar_timer > 0.0f) {
		arpg_hp_bar_timer = MAX(0.0f, arpg_hp_bar_timer - p_delta);
		queue_redraw();
	}
	if (!spawn_origin_set) {
		reset_spawn_origin();
	}

	ClockGearsMap *map = ClockGearsEntityNav::find_parent_map(this);
	int ts = map ? MAX(16, map->get_grid_resolution()) : 32;

	// Hitstop: solo pausa animación y movimiento si hitstop_ms > 0
	if (arpg_hitstop_timer > 0.0f) {
		arpg_hitstop_timer -= p_delta;
		if (arpg_hitstop_timer > 0.0f) {
			return;
		}
		arpg_hitstop_timer = 0.0f;
	}

	// Muerte ARPG: terminar animación morir antes de desaparecer
	if (arpg_dying) {
		if (arpg_knockback_timer > 0.0f) {
			float step_dt = MIN(p_delta, arpg_knockback_timer);
			Vector2 prev_pos = get_position();
			Vector2 next_pos = ClockGearsEntityNav::move_entity_with_slide(map, body_shape, direction, prev_pos, arpg_knockback_velocity * step_dt, through || body_through, this, ts);
			set_position(next_pos);
			arpg_knockback_timer = MAX(0.0f, arpg_knockback_timer - p_delta);
		}
		_advance_behavior_animation(p_delta);
		return;
	}

	// Empuje por retroceso (respeta colisiones e interrumpe movimiento de IA)
	if (arpg_knockback_timer > 0.0f) {
		float step_dt = MIN(p_delta, arpg_knockback_timer);
		Vector2 prev_pos = get_position();
		Vector2 next_pos = ClockGearsEntityNav::move_entity_with_slide(map, body_shape, direction, prev_pos, arpg_knockback_velocity * step_dt, through || body_through, this, ts);
		set_position(next_pos);
		arpg_knockback_timer -= p_delta;
		if (arpg_knockback_timer <= 0.0f) {
			arpg_knockback_timer = 0.0f;
			if (ev_action_state == "golpeado") {
				ev_action_state = "quieto";
				ev_anim_frame = 0;
				ev_anim_timer = 0.0f;
			}
		}
		_advance_behavior_animation(p_delta);
		return;
	}

	// 1. Pausar si el intérprete de eventos está ejecutando este evento
	ClockGearsEventInterpreter *interp = map ? map->get_runtime_interpreter() : nullptr;
	bool running_me = (interp && interp->is_running() && interp->get_active_event() == this);
	if (running_me && !was_interpreter_running_me) {
		if (face_player_on_event && !direction_fix && map && map->get_runtime_player()) {
			_face_toward_world_pos(map->get_runtime_player()->get_position());
		}
	}
	was_interpreter_running_me = running_me;
	paused_by_event = running_me;
	if (running_me) {
		if (ev_action_state != "quieto") {
			ev_action_state = "quieto";
			ev_anim_frame = 0;
			ev_anim_timer = 0.0f;
		}
		_advance_behavior_animation(p_delta);
		return;
	}

	// 2. Actualizar zonas mientras_dentro
	for (int i = 0; i < zone_nodes.size(); i++) {
		if (zone_nodes[i]) {
			zone_nodes[i]->tick_zone(p_delta);
		}
	}

	// 3. Animación de ataque (con hitboxes ARPG hacia el jugador)
	if (ev_action_state == "atacar") {
		ev_attack_timer -= p_delta;
		_advance_behavior_animation(p_delta);
		_process_enemy_attack_hits(map);
		if (ev_attack_timer <= 0.0f && ev_action_state == "atacar") {
			_end_enemy_attack_swing();
			ev_action_state = "quieto";
			ev_anim_frame = 0;
			ev_anim_timer = 0.0f;
			_update_behavior_visual_frame();
		}
		return;
	}

	// 4. Lógica de estado IA y velocidad deseada
	Vector2 move_dir;
	float desired_speed = walk_move_speed;
	String target_loco_state = "quieto";

	auto run_wander = [&]() {
		if (wander_radius <= 0.01f) {
			return;
		}
		if (wander_pause_timer > 0.0f) {
			wander_pause_timer -= p_delta;
			return;
		}
		if (!wander_has_target) {
			float rad_px = wander_radius * float(ts);
			if (get_position().distance_to(spawn_origin_px) > rad_px) {
				wander_target_px = spawn_origin_px;
			} else {
				float ang = wander_rng.randf() * float(Math::TAU);
				float min_r = MIN(0.3f * ts, rad_px * 0.5f);
				float dist = min_r + MAX(0.0f, rad_px - min_r) * wander_rng.randf();
				Vector2 cand = spawn_origin_px + Vector2(Math::cos(ang), Math::sin(ang)) * dist;
				if (map) {
					Vector2i msz = map->get_map_size();
					cand.x = CLAMP(cand.x, 0.0f, float(MAX(0, msz.x - 1) * ts));
					cand.y = CLAMP(cand.y, 0.0f, float(MAX(0, msz.y - 1) * ts));
				}
				wander_target_px = cand;
			}
			wander_has_target = true;
		}
		Vector2 diff = wander_target_px - get_position();
		if (diff.length() <= 4.0f) {
			wander_has_target = false;
			wander_pause_timer = 0.5f + wander_rng.randf();
			return;
		}
		move_dir = diff.normalized();
		desired_speed = walk_move_speed;
		target_loco_state = "mover";
	};

	if (ai_state == CG_AI_STATE_CHASE) {
		if (_has_any_target_in_zones()) {
			time_without_sight = 0.0f;
		} else {
			time_without_sight += p_delta;
			if (lose_sight_sec > 0.0f && time_without_sight >= lose_sight_sec) {
				ai_state = CG_AI_STATE_RETURN_HOME;
				astar_timer = 0.0f;
				astar_no_path = false;
			}
		}
	} else if (ai_state == CG_AI_STATE_FLEE) {
		if (_has_any_target_in_zones()) {
			time_without_sight = 0.0f;
		} else {
			time_without_sight += p_delta;
			if (lose_sight_sec > 0.0f && time_without_sight >= lose_sight_sec) {
				ai_state = CG_AI_STATE_RETURN_HOME;
				astar_timer = 0.0f;
			}
		}
	}

	if (ai_state == CG_AI_STATE_CHASE) {
		Node2D *t2d = Object::cast_to<Node2D>(_resolve_target_node(map));
		if (t2d) {
			Vector2 tpos = t2d->get_position();
			float dist = get_position().distance_to(tpos);
			if (behavior_type == 3 && ai_type_str == "a_distancia") {
				float min_px = min_distance * float(ts);
				float max_px = max_distance * float(ts);
				if (dist > max_px) {
					move_dir = _compute_move_toward(map, tpos, p_delta, ts);
					desired_speed = run_move_speed;
					target_loco_state = "correr";
				} else if (dist < min_px) {
					move_dir = (get_position() - tpos).normalized();
					desired_speed = run_move_speed;
					target_loco_state = "correr";
				} else {
					_face_toward_world_pos(tpos);
				}
			} else {
				float stop_dist = float(ts) * 0.7f;
				if (dist > stop_dist) {
					move_dir = _compute_move_toward(map, tpos, p_delta, ts);
					if (astar_no_path) {
						// Sin camino: fallback a quieto o vagar
						if (ai_type_str == "vagar") {
							run_wander();
						}
					} else if (move_dir.length_squared() > 0.001f) {
						desired_speed = run_move_speed;
						target_loco_state = "correr";
					}
				} else {
					_face_toward_world_pos(tpos);
				}
			}
		}
	} else if (ai_state == CG_AI_STATE_FLEE) {
		Node2D *t2d = Object::cast_to<Node2D>(_resolve_target_node(map));
		if (t2d) {
			Vector2 away = get_position() - t2d->get_position();
			if (away.length_squared() < 0.01f) {
				away = Vector2(1.0f, 0.0f);
			}
			move_dir = away.normalized();
			desired_speed = run_move_speed;
			target_loco_state = "correr";
		}
	} else if (ai_state == CG_AI_STATE_RETURN_HOME) {
		Vector2 diff = spawn_origin_px - get_position();
		if (diff.length() <= 4.0f) {
			if (ClockGearsEntityNav::can_body_occupy_position(map, body_shape, direction, spawn_origin_px, get_position(), through || body_through, this, ts)) {
				set_position(spawn_origin_px);
			}
			if (!direction_fix) {
				direction = spawn_direction;
			}
			ai_state = CG_AI_STATE_BASE;
			astar_path.clear();
		} else {
			move_dir = _compute_move_toward(map, spawn_origin_px, p_delta, ts);
			if (move_dir.length_squared() < 0.001f && !astar_no_path) {
				move_dir = diff.normalized();
			}
			desired_speed = walk_move_speed;
			target_loco_state = "mover";
		}
	} else if (ai_state == CG_AI_STATE_STOPPED) {
		move_dir = Vector2();
	} else {
		// Estado base
		bool is_enemy = (behavior_type == 3);
		String mode = is_enemy ? ai_type_str : npc_move_type_str;
		if (mode == "vagar") {
			run_wander();
		} else if (mode == "patrulla" && !patrol_points.is_empty()) {
			if (patrol_idx < 0 || patrol_idx >= patrol_points.size()) {
				patrol_idx = 0;
			}
			Vector2 wp = spawn_origin_px + patrol_points[patrol_idx] * float(ts);
			Vector2 diff = wp - get_position();
			if (diff.length() <= 4.0f) {
				int n = patrol_points.size();
				if (n > 1) {
					if (patrol_loop) {
						patrol_idx = (patrol_idx + 1) % n;
					} else {
						int next_i = patrol_idx + patrol_dir;
						if (next_i >= n || next_i < 0) {
							patrol_dir = -patrol_dir;
							next_i = CLAMP(patrol_idx + patrol_dir, 0, n - 1);
						}
						patrol_idx = next_i;
					}
				}
			} else {
				move_dir = _compute_move_toward(map, wp, p_delta, ts);
				desired_speed = walk_move_speed;
				target_loco_state = "mover";
			}
		} else if (!is_enemy && mode == "seguir") {
			ClockGearsPlayer *pl = map ? map->get_runtime_player() : nullptr;
			if (pl) {
				Vector2 ppos = pl->get_position();
				float dist = get_position().distance_to(ppos);
				if (dist > follow_distance * float(ts)) {
					move_dir = _compute_move_toward(map, ppos, p_delta, ts);
					desired_speed = walk_move_speed;
					target_loco_state = "mover";
				} else {
					_face_toward_world_pos(ppos);
				}
			}
		} else if (is_enemy && mode == "a_distancia") {
			ClockGearsPlayer *pl = map ? map->get_runtime_player() : nullptr;
			if (pl && detection_zones_cfg.is_empty()) {
				Vector2 ppos = pl->get_position();
				float dist = get_position().distance_to(ppos);
				if (dist > max_distance * float(ts)) {
					move_dir = _compute_move_toward(map, ppos, p_delta, ts);
					desired_speed = walk_move_speed;
					target_loco_state = "mover";
				} else if (dist < min_distance * float(ts)) {
					move_dir = (get_position() - ppos).normalized();
					desired_speed = walk_move_speed;
					target_loco_state = "mover";
				} else {
					_face_toward_world_pos(ppos);
				}
			}
		}
	}

	// 5. Aplicar desplazamiento con colisión y deslizamiento (sin empujar otros cuerpos)
	bool actually_moved = false;
	if (move_dir.length_squared() > 0.0001f) {
		Vector2 prev_pos = get_position();
		Vector2 delta = move_dir * desired_speed * p_delta;
		Vector2 next_pos = ClockGearsEntityNav::move_entity_with_slide(map, body_shape, direction, prev_pos, delta, through || body_through, this, ts);
		Vector2 moved_vec = next_pos - prev_pos;
		if (moved_vec.length_squared() > 0.0001f) {
			set_position(next_pos);
			_face_toward_world_pos(prev_pos + moved_vec * 32.0f);
			actually_moved = true;
		} else if (ai_state == CG_AI_STATE_BASE && (ai_type_str == "vagar" || npc_move_type_str == "vagar")) {
			// Si chocó en vagar contra un muro, elegir otro objetivo tras breve pausa
			wander_has_target = false;
			wander_pause_timer = 0.25f;
		}
	}

	String new_state = actually_moved ? target_loco_state : String("quieto");
	if (new_state != ev_action_state) {
		ev_action_state = new_state;
		ev_anim_frame = 0;
		ev_anim_timer = 0.0f;
	}
	_advance_behavior_animation(p_delta);
}
