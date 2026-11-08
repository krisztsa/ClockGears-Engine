#pragma once

#include "../clockgears_event.h"
#include "../clockgears_map.h"
#include "../clockgears_types.h"
#include "../core/clockgears_pixel_art.h"
#include "clockgears_message_window.h"
#include "clockgears_picture_manager.h"
#include "clockgears_cutscene.h"
#include "clockgears_audio_manager.h"
#include "scene/2d/physics/character_body_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/canvas_layer.h"

class ClockGearsFollowerManager;
class ClockGearsEventInterpreter;

// ============================================================================
// ClockGearsPlayer — Controlador de Jugador en Rejilla 32x32 (Fase 6 / Sección 7.1)
// ============================================================================
class ClockGearsPlayer : public CharacterBody2D {
	GDCLASS(ClockGearsPlayer, CharacterBody2D);

private:
	int actor_id = 0;
	String actor_name;
	String graphic_path;
	int tile_size = 32;
	Vector2i grid_cell = Vector2i(0, 0);
	int direction = 2; // 2=Abajo, 4=Izquierda, 6=Derecha, 8=Arriba

	bool allow_diagonal = false;
	bool through_enabled = false;
	float walk_speed = 128.0f; // 4 tiles/s (32px * 4)
	float dash_speed = 256.0f; // 8 tiles/s (32px * 8)
	bool dashing = false;

	bool moving_step = false;
	Vector2 step_start_px;
	Vector2 step_target_px;
	Vector2i step_target_cell;
	float step_progress = 0.0f;
	float current_step_speed = 128.0f;

	float anim_timer = 0.0f;
	int anim_pattern_idx = 1; // Ciclo 0-1-2-1
	int sprite_cols = 3;
	int sprite_rows = 4;
	bool is_spritesheet = false;

	bool action_key_was_pressed = false;
	bool action_key_released = true;
	float action_cooldown = 0.0f;
	bool diag_key_blocked_printed = false;

	Sprite2D *sprite = nullptr;
	ClockGearsMap *current_map = nullptr;
	ClockGearsFollowerManager *follower_manager = nullptr;
	ClockGearsEventInterpreter *event_interpreter = nullptr;

	Ref<Texture2D> _create_fallback_actor_texture(const Color &p_primary, const Color &p_accent) const;
	void _update_sprite_frame();
	int _direction_to_row(int p_dir) const;
	bool _is_tile_counter(const Vector2i &p_cell) const;
	int _get_tile_passage_mode(const Vector2i &p_cell, int &r_dir4_mask) const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void setup_on_map(ClockGearsMap *p_map, ClockGearsFollowerManager *p_followers = nullptr, ClockGearsEventInterpreter *p_interpreter = nullptr);
	void load_actor_data(int p_actor_id);

	void set_grid_cell_immediate(const Vector2i &p_cell, int p_dir = 2);
	Vector2i get_grid_cell() const { return grid_cell; }

	void set_direction(int p_dir);
	int get_direction() const { return direction; }

	void set_allow_diagonal(bool p_allow) { allow_diagonal = p_allow; }
	bool is_allow_diagonal() const { return allow_diagonal; }

	void set_through_enabled(bool p_through) { through_enabled = p_through; }
	bool is_through_enabled() const { return through_enabled; }

	void set_walk_speed(float p_speed) { walk_speed = MAX(16.0f, p_speed); }
	float get_walk_speed() const { return walk_speed; }

	bool is_moving_step() const { return moving_step; }

	bool can_pass_cell(const Vector2i &p_from, const Vector2i &p_to, int p_dir, ClockGearsEvent **r_blocking_event = nullptr) const;
	bool can_pass(const Vector2i &p_from, const Vector2i &p_to, int p_dir) const { return can_pass_cell(p_from, p_to, p_dir, nullptr); }
	bool try_step(int p_dir, bool p_is_dash = false);
	bool try_activate_front_event();
	void lock_action_key();
	void unlock_action_key();
	bool is_action_key_locked() const;
	bool check_touch_events_at(const Vector2i &p_cell);

	static Vector2i direction_to_vector(int p_dir);
	static int opposite_direction(int p_dir);

	ClockGearsPlayer();
};

// ============================================================================
// ClockGearsFollower — Seguidor Individual del Grupo (Sección 7.4)
// ============================================================================
class ClockGearsFollower : public Node2D {
	GDCLASS(ClockGearsFollower, Node2D);

private:
	int actor_id = 0;
	int member_index = 1;
	int tile_size = 32;
	Vector2i grid_cell = Vector2i(0, 0);
	int direction = 2;
	bool enabled = true;

	bool moving_step = false;
	Vector2 step_start_px;
	Vector2 step_target_px;
	float current_step_speed = 128.0f;
	int stuck_frames = 0;

	float anim_timer = 0.0f;
	int anim_pattern_idx = 1;
	int sprite_cols = 3;
	int sprite_rows = 4;
	bool is_spritesheet = false;

	Sprite2D *sprite = nullptr;

	void _update_sprite_frame();

protected:
	static void _bind_methods();

public:
	void setup_follower(int p_member_index, int p_actor_id, int p_tile_size);
	void set_cell_immediate(const Vector2i &p_cell, int p_dir = 2);
	Vector2i get_grid_cell() const { return grid_cell; }
	int get_direction() const { return direction; }

	void move_toward_cell(const Vector2i &p_target_cell, int p_dir, float p_speed);
	void tick_follower(float p_delta, const Vector2i &p_leader_cell);

	void set_follower_enabled(bool p_enabled);
	bool is_follower_enabled() const { return enabled; }
	bool is_moving_step() const { return moving_step; }

	ClockGearsFollower();
};

// ============================================================================
// ClockGearsFollowerManager — Gestor de Seguidores en Cola por Pasos (Sección 7.4)
// ============================================================================
class ClockGearsFollowerManager : public Node2D {
	GDCLASS(ClockGearsFollowerManager, Node2D);

public:
	struct StepHistoryEntry {
		Vector2i cell;
		int direction = 2;
		float speed = 128.0f;
	};

private:
	int max_followers = 8;
	int tile_size = 32;
	bool followers_visible = true;
	Vector<ClockGearsFollower *> followers;
	Vector<StepHistoryEntry> step_history;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void setup_party_followers(const Array &p_party_actor_ids, const Vector2i &p_start_cell, int p_start_dir = 2, int p_tile_size = 32);
	void record_leader_step(const Vector2i &p_previous_leader_cell, int p_dir, float p_speed);
	void gather_followers(const Vector2i &p_cell, int p_dir = 2);
	void set_followers_visible_flag(bool p_visible);
	bool are_followers_visible() const { return followers_visible; }
	void set_follower_member_enabled(int p_member_idx, bool p_enabled);
	int get_follower_count() const { return followers.size(); }
	Vector2i get_follower_cell(int p_idx) const;

	ClockGearsFollowerManager();
};

// ============================================================================
// ClockGearsEventInterpreter — Intérprete de Eventos en Runtime (Sección 7.3)
// ============================================================================
class ClockGearsEventInterpreter : public Node {
	GDCLASS(ClockGearsEventInterpreter, Node);

public:
	enum InterpreterState {
		STATE_IDLE = 0,
		STATE_RUNNING,
		STATE_WAITING_FRAMES,
		STATE_WAITING_MESSAGE,
		STATE_WAITING_CHOICE,
		STATE_WAITING_NUMBER,
		STATE_WAITING_TRANSFER,
		STATE_WAITING_BATTLE,
		STATE_GAME_OVER
	};

	static constexpr int MAX_COMMANDS_PER_FRAME = 200;

private:
	InterpreterState state = STATE_IDLE;
	ClockGearsMap *current_map = nullptr;
	ClockGearsPlayer *player = nullptr;
	ClockGearsFollowerManager *follower_manager = nullptr;

	ClockGearsEvent *active_event = nullptr;
	Array active_commands;
	int command_index = 0;
	int last_message_cmd_idx = 0;
	int wait_frames_remaining = 0;
	String last_choice_selected;
	int last_choice_index = -999;
	Vector<int> choice_stack;
	bool key_advance_was_pressed = false;

	// UI de Mensajes, Opciones y Efectos de Pantalla en Runtime (CanvasLayer)
	CanvasLayer *screen_tint_layer = nullptr;
	ColorRect *screen_tint_rect = nullptr;
	CanvasLayer *ui_layer = nullptr;
	ColorRect *screen_fade_rect = nullptr;
	ColorRect *screen_flash_rect = nullptr;
	ClockGearsMessageWindow *message_window = nullptr;
	ClockGearsPictureManager *picture_manager = nullptr;
	ClockGearsCutscenePlayer *cutscene_player = nullptr;
	Node *active_battle_scene = nullptr;
	bool battle_can_lose_flag = false;

	float tint_timer = 0.0f;
	float tint_duration = 0.0f;
	Color tint_start_color = Color(0, 0, 0, 0);
	Color tint_target_color = Color(0, 0, 0, 0);
	Color current_tint_color = Color(0, 0, 0, 0);

	float flash_timer = 0.0f;
	float flash_duration = 0.0f;
	Color flash_base_color = Color(1, 1, 1, 0);

	float fade_timer = 0.0f;
	float fade_duration = 0.0f;
	Color fade_start_color = Color(0, 0, 0, 0);
	Color fade_target_color = Color(0, 0, 0, 0);

	// Temblor de Pantalla (Screen Shake)
	float shake_timer = 0.0f;
	float shake_duration = 0.0f;
	float shake_power = 0.0f;
	float shake_speed = 0.0f;
	Vector2 current_shake_offset;

	// Zoom de Cámara (Camera Zoom)
	float cam_zoom_target = 2.0f;
	float cam_zoom_start = 2.0f;
	int cam_zoom_frames_total = 0;
	int cam_zoom_frames_left = 0;

	void _build_runtime_ui();
	void _hide_message_ui();
	void _on_message_window_completed();
	void _on_message_window_choice_selected(int p_idx);
	void _on_message_window_number_entered(int p_val);
	void _on_battle_scene_finished(int p_result);
	void _execute_next_commands();
	bool _execute_single_command(const Dictionary &p_cmd);
	void _refresh_all_map_events();
	void _perform_map_transfer(const String &p_map_path, int p_x, int p_y, int p_dir);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void setup_interpreter(ClockGearsMap *p_map, ClockGearsPlayer *p_player, ClockGearsFollowerManager *p_followers);

	bool is_running() const { return state != STATE_IDLE; }
	bool is_blocking_player() const { return state != STATE_IDLE; }
	int get_state() const { return int(state); }
	int get_last_choice_index() const { return last_choice_index; }
	ClockGearsPictureManager *get_picture_manager() const { return picture_manager; }
	ClockGearsCutscenePlayer *get_cutscene_player() const { return cutscene_player; }
	Color get_screen_tint_color() const { return current_tint_color; }
	ColorRect *get_screen_tint_rect() const { return screen_tint_rect; }
	CanvasLayer *get_screen_tint_layer() const { return screen_tint_layer; }
	CanvasLayer *get_ui_layer() const { return ui_layer; }
	ClockGearsMessageWindow *get_message_window() const { return message_window; }

	void start_screen_shake(float p_power, float p_speed, float p_duration, bool p_wait = false);
	void stop_screen_shake();
	Vector2 get_current_shake_offset() const { return current_shake_offset; }
	bool is_screen_shaking() const { return shake_timer > 0.0f; }

	void start_camera_zoom_transition(float p_target_zoom, int p_duration_frames);

	void start_event(ClockGearsEvent *p_event);
	void start_command_list(const Array &p_commands, ClockGearsEvent *p_source_event = nullptr);
	void advance_message_or_choice(int p_choice_idx = 0);
	void step_frame();

	ClockGearsEventInterpreter();
};
