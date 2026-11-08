#pragma once

#include "editor/plugins/editor_plugin.h"
#include "core/os/os.h"
#include "clockgears_top_bar.h"
#include "clockgears_database.h"
#include "clockgears_event_editor_dialog.h"
#include "clockgears_simplified_export_dialog.h"
#include "clockgears_automation_panel.h"
#include "clockgears_ui_editor.h"
#include "clockgears_workspace.h"
#include "clockgears_preferences_dialog.h"
#include "clockgears_camera_dialog.h"
#include "editor/clockgears_project_manager.h"
#include "scene/gui/popup_menu.h"

class ClockGearsEditorPlugin : public EditorPlugin {
	GDCLASS(ClockGearsEditorPlugin, EditorPlugin);

private:
	ClockGearsTopBar *top_bar = nullptr;
	ClockGearsDatabaseDialog *database_dialog = nullptr;
	ClockGearsEventEditorDialog *event_editor_dialog = nullptr;
	ClockGearsSimplifiedExportDialog *export_dialog = nullptr;
	ClockGearsPreferencesDialog *preferences_dialog = nullptr;
	ClockGearsCameraDialog *camera_dialog = nullptr;
	ClockGearsNewProjectDialog *new_project_dialog = nullptr;
	ClockGearsResourceManagerDialog *resource_manager_dialog = nullptr;
	ConfirmationDialog *missing_folders_dialog = nullptr;
	AcceptDialog *playtest_warning_dialog = nullptr;
	ClockGearsAutomationPanel *automation_panel = nullptr;
	ClockGearsUIEditorPanel *ui_editor_panel = nullptr;
	ClockGearsMapPanel *map_panel = nullptr;
	PopupMenu *event_context_menu = nullptr;
	PopupMenu *quick_events_menu = nullptr;
	Dictionary clipboard_event_data;
	Point2i context_menu_cell = Point2i(-1, -1);
	ProcessID playtest_pid = 0;
	bool workspace_built = false;

	enum ContextMenuAction {
		ACTION_EDIT_OR_CREATE = 0,
		ACTION_COPY = 1,
		ACTION_PASTE = 2,
		ACTION_DELETE = 3,
		ACTION_SET_START_POS = 4,
		ACTION_QUICK_EVENTS_SUBMENU = 100,
	};

	enum QuickEventType {
		QUICK_EV_TRANSFER = 10,
		QUICK_EV_DOOR = 11,
		QUICK_EV_CHEST = 12,
		QUICK_EV_INN = 13,
		QUICK_EV_SAVE = 14,
	};

	static ClockGearsEditorPlugin *singleton;

	void _on_event_context_menu_id_pressed(int p_id);
	void _on_quick_event_id_pressed(int p_id);
	void _on_create_missing_folders_confirmed();
	void _check_missing_project_folders();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void shortcut_input(const Ref<InputEvent> &p_event) override;

public:
	static ClockGearsEditorPlugin *get_singleton() { return singleton; }

	virtual String get_plugin_name() const override { return String::utf8("Automatización"); }
	virtual bool has_main_screen() const override { return true; }
	virtual void make_visible(bool p_visible) override;
	virtual void save_external_data() override;

	ClockGearsEvent *get_event_at_cell(int p_x, int p_y) const;
	ClockGearsMapPanel *get_map_panel() const { return map_panel; }
	Node *ensure_active_map();
	void open_event_editor(ClockGearsEvent *p_ev);
	void open_event_editor_at_cell(int p_x, int p_y);
	void show_event_context_menu(int p_x, int p_y, const Vector2 &p_screen_pos);
	void open_database();
	void open_resource_manager();
	void open_new_project();
	void open_export();
	void open_preferences();
	void open_camera_dialog();
	Dictionary validate_playtest_conditions();
	bool start_playtest();
	void apply_clockgears_workspace();

	ClockGearsEditorPlugin();
	~ClockGearsEditorPlugin();
};
