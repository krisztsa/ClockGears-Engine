#pragma once

#include "clockgears_types.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/option_button.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"

class ClockGearsMapPropertiesDialog : public AcceptDialog {
	GDCLASS(ClockGearsMapPropertiesDialog, AcceptDialog);

private:
	LineEdit *name_edit = nullptr;
	LineEdit *display_name_edit = nullptr;
	OptionButton *tileset_option = nullptr;
	OptionButton *bgm_option = nullptr;
	SpinBox *width_spin = nullptr;
	SpinBox *height_spin = nullptr;
	OptionButton *scroll_option = nullptr;
	OptionButton *combat_mode_option = nullptr;
	OptionButton *respawn_mode_option = nullptr;
	SpinBox *respawn_minutes_spin = nullptr;

	int editing_map_id = 1;
	bool is_new_map = false;
	int initial_combat_mode = -1;
	int initial_respawn_mode = 0;
	double initial_respawn_minutes = 5.0;

	void _refresh_tileset_options(int p_selected_id);
	void _on_respawn_mode_selected(int p_idx);
	void _on_confirmed();

protected:
	static void _bind_methods();

public:
	void set_arpg_map_options(int p_combat_mode, int p_respawn_mode, double p_respawn_minutes);
	int get_selected_combat_mode() const;
	int get_selected_respawn_mode() const;
	double get_selected_respawn_minutes() const;
	bool is_respawn_minutes_active() const;
	bool check_dialog_fits_size(const Vector2i &p_window_size) const;

	void popup_for_new_map(int p_next_id, int p_default_tileset_id = 0);
	void popup_for_edit_map(int p_id, const String &p_name, const String &p_display_name, const String &p_bgm, int p_width, int p_height, int p_tileset_id = 0);

	ClockGearsMapPropertiesDialog();
};
