#pragma once

#include "clockgears_types.h"
#include "scene/gui/control.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/label.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

class ClockGearsUIEditorPanel : public PanelContainer {
	GDCLASS(ClockGearsUIEditorPanel, PanelContainer);

private:
	ItemList *widgets_palette = nullptr;
	Control *wysiwyg_canvas = nullptr;
	VBoxContainer *inspector_vb = nullptr;

	SpinBox *pos_x_spin = nullptr;
	SpinBox *pos_y_spin = nullptr;
	SpinBox *size_w_spin = nullptr;
	SpinBox *size_h_spin = nullptr;
	ColorPickerButton *color_btn = nullptr;

	Array placed_widgets;
	int selected_widget_idx = -1;

	void _on_widget_palette_activated(int p_idx);
	void _on_add_button_pressed();
	void _on_spin_value_changed(double p_value);
	void _on_color_changed(const Color &p_color);
	void _refresh_canvas();
	void _select_widget(int p_idx);
	void _update_widget_properties();
	void _compile_hud_to_scene();

protected:
	static void _bind_methods();

public:
	ClockGearsUIEditorPanel();
};
