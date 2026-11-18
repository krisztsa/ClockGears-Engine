#ifndef CLOCKGEARS_GAME_OVER_H
#define CLOCKGEARS_GAME_OVER_H

#include "core/variant/dictionary.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/texture_rect.h"

class ClockGearsGameOverScreen : public Control {
	GDCLASS(ClockGearsGameOverScreen, Control);

private:
	ColorRect *bg_color_rect = nullptr;
	TextureRect *bg_texture_rect = nullptr;
	Control *stage_root = nullptr;
	VBoxContainer *main_vbox = nullptr;
	Label *title_label = nullptr;
	PanelContainer *panel = nullptr;
	VBoxContainer *buttons_vbox = nullptr;
	Button *retry_btn = nullptr;
	Button *return_title_btn = nullptr;

	Dictionary active_config;
	bool open_flag = false;
	bool preview_mode = false;
	String last_action;

	void _ensure_ui_nodes();
	void _rebuild_from_config(const Dictionary &p_cfg);
	void _update_responsive_stage(const Vector2 &p_override_vp = Vector2());
	void _on_retry_pressed();
	void _on_return_title_pressed();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	static constexpr float LOGICAL_WIDTH = 960.0f;
	static constexpr float LOGICAL_HEIGHT = 540.0f;

	void show_game_over(const Dictionary &p_cfg_override = Dictionary());
	void hide_game_over();
	void set_preview_mode(bool p_preview);
	bool is_preview_mode() const { return preview_mode; }
	void apply_preview_config(const Dictionary &p_cfg);
	bool is_open() const { return open_flag; }

	bool trigger_retry();
	bool trigger_return_to_title();

	Dictionary get_state_snapshot() const;
	String get_last_action() const { return last_action; }

	ClockGearsGameOverScreen();
};

#endif // CLOCKGEARS_GAME_OVER_H
