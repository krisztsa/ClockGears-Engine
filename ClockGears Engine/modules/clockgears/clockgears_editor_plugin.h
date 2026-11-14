#pragma once

#include "editor/export/editor_export_plugin.h"
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
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/texture_rect.h"

class ClockGearsGenerateEntityDialog : public ConfirmationDialog {
	GDCLASS(ClockGearsGenerateEntityDialog, ConfirmationDialog);

public:
	enum EntityTab {
		TAB_NPC = 0,
		TAB_ARPG_NPC = 1,
		TAB_ENEMIGO = 2,
		TAB_ARPG_ENEMIGO = 3,
		TAB_FLORA = 4,
		TAB_FAUNA = 5,
		TAB_MAX = 6,
	};

private:
	TabBar *entity_tabs = nullptr;
	Label *list_header_label = nullptr;
	ItemList *entity_list = nullptr;
	Label *empty_list_label = nullptr;
	PanelContainer *preview_frame_panel = nullptr;
	TextureRect *preview_texture_rect = nullptr;
	Label *preview_name_label = nullptr;
	Label *preview_detail_label = nullptr;
	Point2i target_cell = Point2i(-1, -1);
	Vector<Dictionary> current_records;
	int current_tab_idx = TAB_NPC;

	static int _tab_to_category(int p_tab);
	void _populate_list_for_tab(int p_tab);
	void _update_preview_for_index(int p_list_idx);
	void _resolve_preview_sprite(int p_tab, const Dictionary &p_rec, Ref<Texture2D> &r_tex, String &r_detail) const;

	void _on_tab_changed(int p_tab);
	void _on_entity_selected(int p_idx);
	void _on_entity_activated(int p_idx);
	void _on_confirmed_generate();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void open_for_cell(int p_x, int p_y);
	void select_tab(int p_tab);
	int get_selected_tab() const { return current_tab_idx; }
	int get_listed_count() const;
	void select_list_index(int p_idx);
	Dictionary check_layout_fits_window(const Vector2i &p_window_size = Vector2i(1366, 768)) const;
	Point2i get_target_cell() const { return target_cell; }

	static void populate_event_context_menu(Node *p_menu, Node *p_quick_menu, bool p_has_event, bool p_clipboard_empty);
	static Node *create_entity_event_on_map(Node *p_scene, int p_tab, int p_record_id, int p_x, int p_y, int p_grid_size = 32, bool p_use_undo_redo = true, Object *p_undo_redo = nullptr);

	ClockGearsGenerateEntityDialog();
};

class ClockGearsExportPlugin : public EditorExportPlugin {
	GDCLASS(ClockGearsExportPlugin, EditorExportPlugin);

protected:
	virtual void _export_file(const String &p_path, const String &p_type, const HashSet<String> &p_features) override;

public:
	virtual String get_name() const override { return "ClockGearsSystemExportFilter"; }
};

class ClockGearsEditorPlugin : public EditorPlugin {
	GDCLASS(ClockGearsEditorPlugin, EditorPlugin);

private:
	Ref<ClockGearsExportPlugin> cg_export_plugin;
	ClockGearsTopBar *top_bar = nullptr;
	ClockGearsDatabaseDialog *database_dialog = nullptr;
	ClockGearsEventEditorDialog *event_editor_dialog = nullptr;
	ClockGearsSimplifiedExportDialog *export_dialog = nullptr;
	ClockGearsPreferencesDialog *preferences_dialog = nullptr;
	ClockGearsCameraDialog *camera_dialog = nullptr;
	ClockGearsNewProjectDialog *new_project_dialog = nullptr;
	ClockGearsResourceManagerDialog *resource_manager_dialog = nullptr;
	ClockGearsGenerateEntityDialog *generate_entity_dialog = nullptr;
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
		ACTION_GENERATE_ENTITY = 5,
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
	void _on_generate_entity_chosen(int p_tab, int p_record_id, const String &p_record_name, int p_cell_x, int p_cell_y);
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
	void open_generate_entity_dialog_at_cell(int p_x, int p_y);
	ClockGearsEvent *generate_entity_at_cell(int p_tab, int p_record_id, int p_x, int p_y, bool p_use_undo_redo = true);
	void show_event_context_menu(int p_x, int p_y, const Vector2 &p_screen_pos);
	PopupMenu *get_event_context_menu() const { return event_context_menu; }
	ClockGearsGenerateEntityDialog *get_generate_entity_dialog() const { return generate_entity_dialog; }
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
