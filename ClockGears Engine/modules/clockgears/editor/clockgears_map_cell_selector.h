#pragma once

#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/viewport.h"

class ClockGearsMap;
class ClockGearsEvent;

class ClockGearsMapCellSelector : public VBoxContainer {
	GDCLASS(ClockGearsMapCellSelector, VBoxContainer);

private:
	OptionButton *map_opt = nullptr;
	OptionButton *dir_opt = nullptr;
	LineEdit *map_path_edit = nullptr;
	SpinBox *x_spin = nullptr;
	SpinBox *y_spin = nullptr;
	Label *coords_label = nullptr;

	ScrollContainer *scroll_container = nullptr;
	Control *canvas_wrapper = nullptr;
	SubViewportContainer *subvp_container = nullptr;
	SubViewport *subvp = nullptr;
	Control *overlay_control = nullptr;

	ClockGearsMap *loaded_map_instance = nullptr;
	String loaded_map_path;
	int map_tiles_w = 25;
	int map_tiles_h = 20;
	int selected_cell_x = 0;
	int selected_cell_y = 0;
	bool is_updating_coords = false;

	void _populate_maps_internal(bool p_load_preview);
	void _on_map_selected(int p_idx);
	void _on_dir_selected(int p_idx);
	void _on_spin_changed(double p_val);
	void _on_overlay_draw();
	void _on_overlay_gui_input(const Ref<InputEvent> &p_event);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void populate_maps();
	void set_target_map(const String &p_map_path);
	String get_target_map() const;
	void set_target_coords(int p_x, int p_y);
	Vector2i get_target_coords() const;
	void set_target_direction(const String &p_dir);
	String get_target_direction() const;

	void select_cell_at_pixel(const Vector2 &p_pixel_pos);

	static ClockGearsEvent *create_quick_door_event(Node *p_scene, const Vector2i &p_cell, const String &p_map, int p_x, int p_y, const String &p_dir, const String &p_sprite_path, int p_hframes);

	void load_map_preview(const String &p_map_path);

	// Getters
	OptionButton *get_map_option_button() const { return map_opt; }
	OptionButton *get_dir_option_button() const { return dir_opt; }
	SpinBox *get_x_spinbox() const { return x_spin; }
	SpinBox *get_y_spinbox() const { return y_spin; }
	ScrollContainer *get_scroll_container() const { return scroll_container; }
	Control *get_overlay_control() const { return overlay_control; }

	ClockGearsMapCellSelector();
	~ClockGearsMapCellSelector();
};
