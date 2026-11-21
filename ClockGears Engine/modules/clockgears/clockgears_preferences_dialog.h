#pragma once

#include "clockgears_types.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/box_container.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/button.h"

class ClockGearsPreferencesDialog : public AcceptDialog {
	GDCLASS(ClockGearsPreferencesDialog, AcceptDialog);

private:
	TabContainer *tabs = nullptr;

	// Tab 1: Resolución y editor de mapa
	OptionButton *preset_opt = nullptr;
	SpinBox *width_spin = nullptr;
	SpinBox *height_spin = nullptr;
	OptionButton *win_preset_opt = nullptr;
	SpinBox *win_width_spin = nullptr;
	SpinBox *win_height_spin = nullptr;
	CheckBox *integer_scale_check = nullptr;
	CheckBox *fullscreen_check = nullptr;
	OptionButton *stretch_mode_opt = nullptr;
	OptionButton *stretch_aspect_opt = nullptr;
	CheckBox *dim_inactive_layers_check = nullptr;
	SpinBox *inactive_layer_opacity_spin = nullptr;
	OptionButton *editor_theme_opt = nullptr;
	OptionButton *glow_effects_opt = nullptr;

	// Tab 2: Exportación
	LineEdit *windows_path_edit = nullptr;
	LineEdit *linux_path_edit = nullptr;
	LineEdit *android_path_edit = nullptr;
	OptionButton *build_type_opt = nullptr;
	CheckBox *pck_check = nullptr;
	Button *export_now_btn = nullptr;

	void _on_preset_selected(int p_idx);
	void _on_win_preset_selected(int p_idx);
	void _on_dim_inactive_toggled(bool p_pressed);
	void _on_editor_theme_selected(int p_idx);
	void _on_glow_effects_selected(int p_idx);
	void _on_export_now_pressed();
	void _on_dialog_confirmed();
	void _load_settings();
	void _save_settings();
	void _refresh_pref_theme_accents();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void open_preferences();
	bool is_dim_inactive_layers_checked() const;
	void set_dim_inactive_layers_checked(bool p_enabled);
	int get_inactive_layer_opacity_value() const;
	void set_inactive_layer_opacity_value(int p_pct);
	String get_selected_editor_theme_id() const;
	bool select_editor_theme_by_id(const String &p_theme_id);
	OptionButton *get_editor_theme_option_button() const { return editor_theme_opt; }
	int get_glow_effects_level_selected() const;
	bool select_glow_effects_level(int p_level);
	OptionButton *get_glow_effects_option_button() const { return glow_effects_opt; }
	bool is_glow_effects_checked() const;
	void set_glow_effects_checked(bool p_enabled);
	void apply_and_save_preferences();

	ClockGearsPreferencesDialog();
};
