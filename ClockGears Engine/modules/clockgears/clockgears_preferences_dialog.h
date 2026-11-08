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

	// Tab 1: Resolución
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

	// Tab 2: Exportación
	LineEdit *windows_path_edit = nullptr;
	LineEdit *linux_path_edit = nullptr;
	LineEdit *android_path_edit = nullptr;
	OptionButton *build_type_opt = nullptr;
	CheckBox *pck_check = nullptr;
	Button *export_now_btn = nullptr;

	void _on_preset_selected(int p_idx);
	void _on_win_preset_selected(int p_idx);
	void _on_export_now_pressed();
	void _on_dialog_confirmed();
	void _load_settings();
	void _save_settings();

protected:
	static void _bind_methods();

public:
	void open_preferences();

	ClockGearsPreferencesDialog();
};
