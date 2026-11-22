#ifndef CLOCKGEARS_IMAGE_ADJUST_DIALOG_H
#define CLOCKGEARS_IMAGE_ADJUST_DIALOG_H

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/spin_box.h"

class ClockGearsImageAdjustDialog;

class ClockGearsImageAdjustCanvas : public Control {
	GDCLASS(ClockGearsImageAdjustCanvas, Control);

	friend class ClockGearsImageAdjustDialog;

	ClockGearsImageAdjustDialog *owner = nullptr;
	int drag_mode = 0; // 0=none, 1=move, 2..5=corner handle 0..3
	Vector2 drag_start_mouse;
	Dictionary drag_start_vals;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void set_owner_dialog(ClockGearsImageAdjustDialog *p_owner) { owner = p_owner; }
	ClockGearsImageAdjustCanvas();
};

class ClockGearsImageAdjustDialog : public AcceptDialog {
	GDCLASS(ClockGearsImageAdjustDialog, AcceptDialog);

	friend class ClockGearsImageAdjustCanvas;

public:
	struct StateImageSlot {
		String label;
		String path;
		Ref<Texture2D> texture;
	};

private:
	PanelContainer *bg_root = nullptr;
	ClockGearsImageAdjustCanvas *canvas = nullptr;

	HFlowContainer *state_bar = nullptr;
	Vector<Button *> state_btns;
	CheckBox *keep_aspect_chk = nullptr;
	CheckBox *integer_scale_chk = nullptr;
	Button *center_btn = nullptr;
	Button *fit_frame_btn = nullptr;
	Button *reset_btn = nullptr;

	SpinBox *ox_spin = nullptr;
	SpinBox *oy_spin = nullptr;
	SpinBox *sx_spin = nullptr;
	SpinBox *sy_spin = nullptr;
	Label *sy_lbl = nullptr;
	Label *info_lbl = nullptr;

	Vector2 logical_frame_size = Vector2(320, 180);
	Dictionary shape_cfg;
	Dictionary initial_vals;
	Dictionary edit_vals; // manual_offset_x_pct, manual_offset_y_pct, manual_scale_x_pct, manual_scale_y_pct, manual_keep_aspect, manual_integer_scale
	Vector<StateImageSlot> state_slots;
	int active_state_idx = 0;
	Callable on_confirm_cb;
	bool updating_controls = false;

	void _build_ui();
	void _refresh_theme();
	void _on_theme_changed();
	void _sync_controls_from_vals();
	void _redraw_canvas();
	Rect2 _compute_frame_preview_rect() const;
	Rect2 _compute_image_preview_rect(const Rect2 &p_frame_rect, const Dictionary &p_vals) const;
	double _snap_scale_if_integer(double p_scale_pct, const Ref<Texture2D> &p_tex) const;

	void _on_state_btn_pressed(int p_idx);
	void _on_keep_aspect_toggled(bool p_pressed);
	void _on_integer_scale_toggled(bool p_pressed);
	void _on_center_pressed();
	void _on_fit_frame_pressed();
	void _on_reset_pressed();
	void _on_spin_changed(double p_val);
	void _on_confirmed();
	void _on_window_input(const Ref<InputEvent> &p_ev);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	static Vector<Point2> get_corner_handles(const Rect2 &p_rect);
	static int hit_test_corner_handle(const Point2 &p_mouse_pos, const Vector<Point2> &p_handles, float p_radius = 14.0f);

	void open_adjust(
			const String &p_title,
			const Vector2 &p_logical_frame_sz,
			const Dictionary &p_shape_cfg,
			const Dictionary &p_current_cfg,
			const Vector<Pair<String, String>> &p_state_images,
			bool p_default_pixel_art,
			const Callable &p_on_confirm);

	Dictionary get_values() const { return edit_vals.duplicate(true); }
	void set_values(const Dictionary &p_vals);
	void confirm_adjust();
	Dictionary get_ui_handles() const;

	ClockGearsImageAdjustDialog();
};

#endif // CLOCKGEARS_IMAGE_ADJUST_DIALOG_H
