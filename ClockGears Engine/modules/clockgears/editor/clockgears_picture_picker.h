#ifndef CLOCKGEARS_PICTURE_PICKER_H
#define CLOCKGEARS_PICTURE_PICKER_H

#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/texture.h"

class ClockGearsPictureVisualPicker;

// ============================================================================
// ClockGearsPicturePreviewCanvas — Lienzo de Vista Previa Interactiva 16:9
// Permite arrastrar para mover, tirar de las esquinas para escalar con snap
// entero a pixel art o Shift libre, y dibuja la pantalla del juego (960x540).
// ============================================================================
class ClockGearsPicturePreviewCanvas : public Control {
	GDCLASS(ClockGearsPicturePreviewCanvas, Control);

	ClockGearsPictureVisualPicker *picker = nullptr;

	enum DragMode {
		DRAG_NONE = 0,
		DRAG_MOVE,
		DRAG_HANDLE_TL,
		DRAG_HANDLE_TR,
		DRAG_HANDLE_BL,
		DRAG_HANDLE_BR,
		DRAG_HANDLE_T,
		DRAG_HANDLE_B,
		DRAG_HANDLE_L,
		DRAG_HANDLE_R
	};

	DragMode current_drag = DRAG_NONE;
	Vector2 drag_start_mouse;
	Vector2 drag_start_pos;
	Vector2 drag_start_scale;

	bool _get_canvas_transforms(Rect2 &r_screen_rect, float &r_scale, Vector2 &r_box_pos) const;
	bool _get_image_screen_rect(Rect2 &r_img_rect, Rect2 &r_h_tl, Rect2 &r_h_tr, Rect2 &r_h_bl, Rect2 &r_h_br, Rect2 &r_h_t, Rect2 &r_h_b, Rect2 &r_h_l, Rect2 &r_h_r, Vector2 &r_anchor_pt) const;

protected:
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	static void _bind_methods();

public:
	virtual Size2 get_minimum_size() const override;
	ClockGearsPicturePreviewCanvas(ClockGearsPictureVisualPicker *p_picker = nullptr);
};

// ============================================================================
// ClockGearsPictureVisualPicker — Selector Visual Interactivo (Fase 7B)
// Soporta anclajes rápidos (Centro, Arriba, Abajo, Izquierda, Derecha),
// sincronización bidireccional con campos numéricos, snap entero de pixel art.
// ============================================================================
class ClockGearsPictureVisualPicker : public VBoxContainer {
	GDCLASS(ClockGearsPictureVisualPicker, VBoxContainer);

	friend class ClockGearsPicturePreviewCanvas;

public:
	enum OriginMode {
		ORIGIN_TOP_LEFT = 0,
		ORIGIN_CENTER = 1
	};

private:
	ClockGearsPicturePreviewCanvas *canvas = nullptr;

	Button *btn_anchor_center = nullptr;
	Button *btn_anchor_top = nullptr;
	Button *btn_anchor_bottom = nullptr;
	Button *btn_anchor_left = nullptr;
	Button *btn_anchor_right = nullptr;
	CheckBox *chk_integer_snap = nullptr;

	String image_path;
	Ref<Texture2D> texture;
	OriginMode origin_mode = ORIGIN_TOP_LEFT;
	int pos_x = 0;
	int pos_y = 0;
	int scale_x = 100;
	int scale_y = 100;
	bool integer_snap_enabled = false;

	SpinBox *bound_x_spin = nullptr;
	SpinBox *bound_y_spin = nullptr;
	SpinBox *bound_scale_x_spin = nullptr;
	SpinBox *bound_scale_y_spin = nullptr;
	bool updating_from_external = false;

	void _on_snap_toggled(bool p_toggled);
	void _notify_values_changed();

protected:
	static void _bind_methods();

public:
	void _on_spinbox_changed(double p_val);

	void quick_anchor_center();
	void quick_anchor_top();
	void quick_anchor_bottom();
	void quick_anchor_left();
	void quick_anchor_right();

	void set_image_path(const String &p_path);
	String get_image_path() const { return image_path; }

	void set_origin_mode(int p_origin);
	int get_origin_mode() const { return (int)origin_mode; }

	void set_values(int p_x, int p_y, int p_sx, int p_sy);
	void set_visual_values(int p_x, int p_y, int p_sx, int p_sy);
	int get_pos_x() const { return pos_x; }
	int get_pos_y() const { return pos_y; }
	int get_scale_x() const { return scale_x; }
	int get_scale_y() const { return scale_y; }

	void set_integer_snap(bool p_snap);
	bool is_integer_snap() const { return integer_snap_enabled; }

	void bind_spinboxes(SpinBox *p_x, SpinBox *p_y, SpinBox *p_sx, SpinBox *p_sy);

	ClockGearsPictureVisualPicker();
};

#endif // CLOCKGEARS_PICTURE_PICKER_H
