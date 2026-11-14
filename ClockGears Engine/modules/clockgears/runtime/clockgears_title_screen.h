#ifndef CLOCKGEARS_TITLE_SCREEN_H
#define CLOCKGEARS_TITLE_SCREEN_H

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/slider.h"
#include "scene/gui/texture_rect.h"

// ============================================================================
// ClockGearsTitleScreen — Pantalla de Título Runtime (9B Tanda 1)
// Arranque principal para F5 y juego exportado. Configurable por title_menu.json
// e independiente de ClockGearsMap para no alterar tests que instancian mapas.
// ============================================================================
class ClockGearsTitleScreen : public Control {
	GDCLASS(ClockGearsTitleScreen, Control);

private:
	ColorRect *bg_color_rect = nullptr;
	TextureRect *bg_texture_rect = nullptr;
	Label *title_label = nullptr;
	TextureRect *title_texture_rect = nullptr;

	PanelContainer *main_menu_panel = nullptr;
	VBoxContainer *entries_box = nullptr;

	Control *continue_overlay = nullptr;
	PanelContainer *continue_panel = nullptr;
	VBoxContainer *slots_box = nullptr;
	Button *btn_continue_back = nullptr;

	Control *options_overlay = nullptr;
	PanelContainer *options_panel = nullptr;
	HSlider *music_slider = nullptr;
	Label *music_val_label = nullptr;
	HSlider *sfx_slider = nullptr;
	Label *sfx_val_label = nullptr;
	HSlider *brightness_slider = nullptr;
	Label *brightness_val_label = nullptr;
	CheckBox *fullscreen_check = nullptr;
	Button *btn_options_reset = nullptr;
	Button *btn_options_back = nullptr;
	bool syncing_options_ui = false;

	bool auto_quit_tree = true;
	bool preview_mode = false;
	bool editor_test_popup_mode = false;
	bool quit_requested_flag = false;
	String applied_bgm_path;
	String last_started_map_path;
	PackedStringArray visible_entry_ids;
	PackedStringArray visible_entry_texts;
	Dictionary preview_title_config;
	Dictionary preview_system_config;

	void _build_base_ui();
	void _apply_theme_styling();
	void _apply_title_config(const Dictionary &p_cfg, const Dictionary &p_sys_cfg, bool p_check_saves);
	Vector<Button *> _get_active_nav_buttons() const;
	bool _move_focus_by(int p_delta);
	bool _activate_focused_button();
	void _populate_save_slots();
	void _sync_options_controls_from_dm();
	void _commit_options_from_controls();
	void _on_music_slider_changed(double p_val);
	void _on_sfx_slider_changed(double p_val);
	void _on_brightness_slider_changed(double p_val);
	void _on_fullscreen_toggled(bool p_pressed);
	void _on_reset_options_pressed();
	void _on_entry_button_pressed(const String &p_id);
	void _on_save_slot_pressed(int p_slot);

	Ref<Texture2D> _load_texture(const String &p_path) const;

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	static bool ensure_title_screen_scene(const String &p_scene_path = "res://title_screen.tscn");

	void rebuild_title_screen();
	void set_preview_mode(bool p_enable);
	bool is_preview_mode() const { return preview_mode; }
	void set_editor_test_popup_mode(bool p_enable);
	bool is_editor_test_popup_mode() const { return editor_test_popup_mode; }
	void apply_title_config_preview(const Dictionary &p_title_cfg, const Dictionary &p_sys_cfg = Dictionary());
	bool handle_navigation_key(Key p_keycode);
	Dictionary get_layout_bounds_info(const Vector2 &p_viewport_size = Vector2()) const;
	PackedStringArray get_visible_entry_ids() const { return visible_entry_ids; }
	PackedStringArray get_visible_entry_texts() const { return visible_entry_texts; }

	bool trigger_entry(const String &p_id);
	bool trigger_new_game();
	void open_continue_menu();
	bool select_save_slot(int p_slot);
	PackedStringArray get_displayed_save_slot_texts() const;
	void open_options_menu();
	void close_submenus();
	bool is_continue_menu_open() const;
	bool is_options_menu_open() const;
	void apply_options_values(double p_music, double p_sfx, double p_brightness, bool p_fullscreen);
	void trigger_quit();

	void set_auto_quit_tree(bool p_enable) { auto_quit_tree = p_enable; }
	bool is_auto_quit_tree() const { return auto_quit_tree; }
	bool was_quit_requested() const { return quit_requested_flag; }

	String get_title_text_displayed() const;
	bool is_title_image_active() const;
	bool is_background_image_active() const;
	String get_background_color_applied() const;
	String get_background_scale_mode_applied() const;
	String get_background_filter_applied() const;
	String get_applied_bgm_path() const { return applied_bgm_path; }
	String get_last_started_map_path() const { return last_started_map_path; }

	ClockGearsTitleScreen();
};

#endif // CLOCKGEARS_TITLE_SCREEN_H
