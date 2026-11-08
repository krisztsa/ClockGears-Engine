#pragma once

#include "clockgears_types.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"

class ClockGearsDatabaseDialog;
class ClockGearsEventEditorDialog;
class ClockGearsSimplifiedExportDialog;
class ButtonGroup;

// Icon-only ClockGears command bar. Layout: file/edit history on the left,
// the large DATABASE gear in the exact center, map tools on the right.
class ClockGearsTopBar : public PanelContainer {
	GDCLASS(ClockGearsTopBar, PanelContainer);

	Button *database_btn = nullptr;
	Button *map_mode_btn = nullptr;
	Button *event_mode_btn = nullptr;
	Button *layer_buttons[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	Button *tool_buttons[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	Button *playtest_btn = nullptr;
	Button *undo_btn = nullptr;
	Button *redo_btn = nullptr;
	MenuButton *folders_btn = nullptr;
	PopupMenu *folders_popup = nullptr;

	int current_combat_mode = COMBAT_MODE_TURN_BASED;

	ClockGearsDatabaseDialog *database_dialog = nullptr;
	ClockGearsEventEditorDialog *event_editor_dialog = nullptr;
	ClockGearsSimplifiedExportDialog *export_dialog = nullptr;

	Ref<Texture2D> _make_svg_icon(const String &p_svg) const;
	Button *_make_icon_button(HBoxContainer *p_parent, const Ref<Texture2D> &p_icon, const String &p_tooltip, const Callable &p_action);
	Button *_make_toggle_button(HBoxContainer *p_parent, const Ref<Texture2D> &p_icon, const String &p_tooltip, const Ref<ButtonGroup> &p_group);
	Button *_make_layer_button(HBoxContainer *p_parent, const String &p_label, const String &p_tooltip, const Ref<ButtonGroup> &p_group);
	void _apply_toolbar_button_style(Button *p_button) const;

	void _on_undo_pressed();
	void _on_redo_pressed();
	void _on_cut_pressed();
	void _on_copy_pressed();
	void _on_paste_pressed();
	void _on_folder_menu_id_pressed(int p_id);
	void _on_database_pressed();
	void _on_map_mode_pressed();
	void _on_event_mode_pressed();
	void _on_layer_pressed(int p_layer);
	void _on_tool_pressed(int p_tool);
	void _on_zoom_in_pressed();
	void _on_zoom_out_pressed();
	void _on_playtest_pressed();

protected:
	static void _bind_methods();
	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

	static ClockGearsTopBar *singleton;

public:
	static ClockGearsTopBar *get_singleton() { return singleton; }
	void set_map_mode();
	void set_dialogs(ClockGearsDatabaseDialog *p_db, ClockGearsEventEditorDialog *p_ev, ClockGearsSimplifiedExportDialog *p_exp);

	ClockGearsTopBar();
	~ClockGearsTopBar();
};
