#pragma once

#include "clockgears_types.h"
#include "scene/gui/box_container.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "scene/gui/popup_menu.h"
#include "scene/resources/texture.h"

class ClockGearsMapPropertiesDialog;
class ClockGearsTilePickerDialog;

// Interactive tileset viewer: draws the loaded sheet on a checker grid and
// lets the user pick a tile cell or rectangular block. Used by the A/B/C/D/E/R palette tabs.
class ClockGearsTilePalette : public Control {
	GDCLASS(ClockGearsTilePalette, Control);

	Ref<Texture2D> tileset_texture;
	int tile_size = 32;
	int selected_tile = -1;
	int hovered_tile = -1;
	bool is_region_palette = false;

	Vector<Vector2i> selected_coords_list;
	Vector2i selection_block_size = Vector2i(1, 1);
	bool is_dragging_selection = false;
	Vector2i drag_start_coords = Vector2i(0, 0);

	bool is_panning = false;
	Vector2 pan_last_mouse;
	Vector2 pan_offset;
	float user_zoom = 1.0f;

	int columns = 0;
	int rows = 0;
	real_t draw_scale = 1.0;
	Vector2 texture_origin;
	String empty_message;

	void _update_metrics();
	int _tile_at(const Point2 &p_position) const;
	Rect2 _tile_rect(int p_index) const;

protected:
	static void _bind_methods();
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	void _draw_palette();
	void _notification(int p_what);

public:
	void set_tileset(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_tileset() const { return tileset_texture; }

	void set_tile_size(int p_size);
	int get_tile_size() const { return tile_size; }
	int get_columns() const { return columns; }

	void set_selected_tile(int p_tile);
	int get_selected_tile() const { return selected_tile; }

	void set_empty_message(const String &p_msg) { empty_message = p_msg; queue_redraw(); }
	String get_empty_message() const { return empty_message; }

	void set_region_palette(bool p_is_region);
	bool is_region_palette_mode() const { return is_region_palette; }

	Vector<Vector2i> get_selected_coords_list() const { return selected_coords_list; }
	Vector2i get_selection_block_size() const { return selection_block_size; }

	virtual Size2 get_minimum_size() const override;

	ClockGearsTilePalette();
};

// Left half of the editor shell (Three-Pane Layout): tileset palette on top, map hierarchy below.
class ClockGearsMapPanel : public VBoxContainer {
	GDCLASS(ClockGearsMapPanel, VBoxContainer);

	static ClockGearsMapPanel *singleton;
	static constexpr int PALETTE_TABS = 6; // A, B, C, D, E, R

	TabContainer *tileset_tabs = nullptr;
	ClockGearsTilePalette *palettes[PALETTE_TABS] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
	FileDialog *tileset_file_dialog = nullptr;
	AcceptDialog *tileset_error_dialog = nullptr;
	ClockGearsTilePickerDialog *advanced_tile_picker_dialog = nullptr;

	Tree *map_tree = nullptr;
	PopupMenu *map_context_menu = nullptr;
	ClockGearsMapPropertiesDialog *map_properties_dialog = nullptr;
	ConfirmationDialog *tileset_change_dialog = nullptr;
	int map_counter = 0;

	int pending_save_id = 0;
	String pending_save_name;
	String pending_save_display_name;
	String pending_save_bgm;
	int pending_save_width = 0;
	int pending_save_height = 0;
	int pending_save_tileset_id = 0;
	bool pending_save_is_new = false;

	void _on_tileset_change_confirmed();
	void _on_tileset_change_custom_action(const StringName &p_action);
	void _on_tileset_change_canceled();
	void _apply_map_properties(bool p_clear_tiles);

	void _on_tileset_file_selected(const String &p_path);
	void _on_load_tileset_pressed();
	void _on_open_advanced_picker_pressed();
	void _on_advanced_picker_confirmed(const Array &p_sel, const Vector2i &p_block);
	void _on_palette_tab_changed(int p_tab);
	void _on_tile_selected(int p_tile, int p_tab);

	void _on_add_map();
	void _on_add_child_map();
	void _on_edit_map_properties();
	void _on_delete_map();
	void _on_map_selected();
	void _on_map_edited();
	void _on_map_activated();
	void _on_map_rmb_clicked(const Vector2 &p_pos, MouseButton p_button);
	void _on_context_menu_id_pressed(int p_id);
	void _on_map_properties_saved(int p_id, const String &p_name, const String &p_display_name, const String &p_bgm, int p_width, int p_height, int p_tileset_id, bool p_is_new);

	int _count_maps(TreeItem *p_item) const;
	void _select_map_item(TreeItem *p_item);

	Control *_make_section_header(const String &p_title, bool p_with_tileset_action);

public:
	bool import_tileset_file(const String &p_path, int p_tab = -1);
	void load_persisted_tilesets(int p_tileset_id = 0);
	String get_palette_tileset_path(int p_tab) const;
	void refresh_maps_from_disk();

protected:
	static void _bind_methods();

public:
	static ClockGearsMapPanel *get_singleton() { return singleton; }
	ClockGearsMapPropertiesDialog *get_map_properties_dialog() const { return map_properties_dialog; }

	ClockGearsMapPanel();
	~ClockGearsMapPanel();
};
