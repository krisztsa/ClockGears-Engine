#pragma once

#include "clockgears_types.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/box_container.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/slider.h"
#include "scene/gui/option_button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"

class ClockGearsCameraPreviewCanvas : public Control {
	GDCLASS(ClockGearsCameraPreviewCanvas, Control);

private:
	Vector2 map_size_px = Vector2(960, 640);
	Vector2 player_start_px = Vector2(160, 160);
	Rect2 camera_visible_rect = Rect2(0, 0, 480, 270);
	float camera_zoom = 2.0f;
	int grid_res = 32;

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_preview_data(const Vector2 &p_map_size_px, const Vector2 &p_start_px, const Rect2 &p_cam_rect, float p_zoom, int p_grid);

	ClockGearsCameraPreviewCanvas();
};

class ClockGearsCameraDialog : public ConfirmationDialog {
	GDCLASS(ClockGearsCameraDialog, ConfirmationDialog);

private:
	// Vista Previa Integrada
	ClockGearsCameraPreviewCanvas *preview_canvas = nullptr;
	Label *lbl_preview_info = nullptr;

	// Resolución Interna
	OptionButton *opt_res_preset = nullptr;
	SpinBox *spin_width = nullptr;
	SpinBox *spin_height = nullptr;

	// Zoom Decimal y Ajuste
	HSlider *slider_zoom = nullptr;
	SpinBox *spin_zoom = nullptr;
	CheckBox *chk_integer_zoom = nullptr;

	// Comportamiento de Cámara
	CheckBox *chk_follow_player = nullptr;
	CheckBox *chk_smooth = nullptr;
	SpinBox *spin_smooth_speed = nullptr;
	CheckBox *chk_clamp_map = nullptr;

	// Modo de Pantalla
	OptionButton *opt_screen_mode = nullptr;

	bool updating_ui = false;

	void _load_settings();
	void _save_settings();
	void _update_preview();

	void _on_preset_selected(int p_idx);
	void _on_dimension_changed(double p_val);
	void _on_zoom_slider_changed(double p_val);
	void _on_zoom_spin_changed(double p_val);
	void _on_integer_snap_toggled(bool p_toggled);
	void _on_camera_param_changed();
	void _on_screen_mode_selected(int p_idx);
	void _on_dialog_confirmed();
	void _on_dialog_canceled();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void open_camera_dialog();

	ClockGearsCameraDialog();
	~ClockGearsCameraDialog();
};
