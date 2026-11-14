#ifndef CLOCKGEARS_CREDITS_H
#define CLOCKGEARS_CREDITS_H

#include "core/os/keyboard.h"
#include "core/variant/dictionary.h"
#include "scene/gui/box_container.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/texture_rect.h"

class ClockGearsCreditsPlayer : public Control {
	GDCLASS(ClockGearsCreditsPlayer, Control);

private:
	ColorRect *bg_color_rect = nullptr;
	TextureRect *bg_texture_rect = nullptr;
	Control *stage_clip = nullptr;
	VBoxContainer *scroll_vbox = nullptr;

	Dictionary active_config;
	bool playing = false;
	bool completed_naturally = false;
	bool cancelled_exit = false;
	bool enter_held = false;
	bool mouse_held = false;
	bool script_accelerated = false;
	bool audio_saved_prev_bgm = false;
	bool audio_played_credits_bgm = false;

	float scroll_y = 540.0f;
	float scroll_speed = 60.0f;
	float accel_multiplier = 4.0f;
	float content_height = 0.0f;
	int item_count_rendered = 0;

	void _ensure_ui_nodes();
	void _clear_scroll_items();
	void _rebuild_from_config(const Dictionary &p_cfg);
	void _update_responsive_stage(const Vector2 &p_override_vp_size = Vector2());
	void _finish(bool p_cancelled);

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void input(const Ref<InputEvent> &p_event) override;
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	static constexpr float LOGICAL_WIDTH = 960.0f;
	static constexpr float LOGICAL_HEIGHT = 540.0f;

	void start_credits(const Dictionary &p_cfg_override = Dictionary());
	void stop_credits(bool p_cancelled = true);
	void step_credits(float p_delta);

	bool is_playing() const { return playing; }
	bool was_cancelled() const { return cancelled_exit; }
	bool was_completed_naturally() const { return completed_naturally; }

	void set_accelerated(bool p_accel) { script_accelerated = p_accel; }
	bool is_accelerated() const { return enter_held || mouse_held || script_accelerated; }
	bool handle_credits_key(Key p_keycode, bool p_pressed = true);

	Dictionary get_scroll_state() const;
	Dictionary get_layout_bounds_info(const Vector2 &p_viewport_size = Vector2()) const;

	ClockGearsCreditsPlayer();
};

#endif // CLOCKGEARS_CREDITS_H
