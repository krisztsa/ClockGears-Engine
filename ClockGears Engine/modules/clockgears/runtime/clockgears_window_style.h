#ifndef CLOCKGEARS_WINDOW_STYLE_H
#define CLOCKGEARS_WINDOW_STYLE_H

#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/resources/style_box.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/texture.h"

class ClockGearsWindowStyleBox : public StyleBox {
	GDCLASS(ClockGearsWindowStyleBox, StyleBox);

private:
	String style_id = "rounded";
	String window_type_id = "";
	String border_type = "rounded";
	String fill_type = "color";
	String center_mode = "stretch";
	bool pixel_art = true;
	bool fallback_used = false;
	float scale_factor = 1.0f;

	int base_border_width = 2;
	int base_corner_radius = 8;
	int scaled_border_width = 2;
	int scaled_corner_radius = 8;

	Color border_color = Color(0.88f, 0.63f, 0.25f, 1.0f);
	Color fill_color = Color(0.08f, 0.09f, 0.12f, 0.94f);
	Color font_color = Color(0.93f, 0.95f, 0.98f, 1.0f);

	int base_padding[4] = { 12, 10, 12, 10 };
	int base_nine_slice[4] = { 8, 8, 8, 8 };
	float scaled_nine_slice[4] = { 8.0f, 8.0f, 8.0f, 8.0f };

	String image_path;
	String fill_image_path;
	Ref<Texture2D> frame_texture;
	Ref<Texture2D> fill_texture;
	Ref<StyleBoxFlat> flat_box;

	// Propiedades extendidas de forma (Etapa 3F)
	String shape_type;
	bool gradient_enabled = false;
	Color gradient_top_color = Color(0.16f, 0.13f, 0.10f, 1.0f);
	Color gradient_bottom_color = Color(0.08f, 0.07f, 0.05f, 1.0f);
	String gradient_angle = "vertical";
	bool shadow_enabled = false;
	Color shadow_color = Color(0.0f, 0.0f, 0.0f, 0.45f);
	int shadow_size = 4;
	int shadow_offset_x = 0;
	int shadow_offset_y = 2;
	bool glow_enabled = false;
	Color glow_color = Color(0.96f, 0.74f, 0.33f, 0.40f);
	int glow_size = 4;

	void _rebuild_internal();

protected:
	static void _bind_methods();
	virtual float get_style_margin(Side p_side) const override;

public:
	static Vector<Vector2> build_shape_polygon(const Rect2 &p_rect, const String &p_shape_type, float p_corner_radius, int p_segments = 24);
	String get_effective_shape_type() const;

	bool configure_from_surface(const Dictionary &p_surface, float p_vp_scale, const String &p_style_id = "", const String &p_window_type_id = "");

	virtual void draw(RID p_canvas_item, const Rect2 &p_rect) const override;

	String get_style_id() const { return style_id; }
	String get_window_type_id() const { return window_type_id; }
	String get_border_type() const { return border_type; }
	String get_fill_type() const { return fill_type; }
	String get_center_mode() const { return center_mode; }
	bool is_pixel_art() const { return pixel_art; }
	bool was_fallback_used() const { return fallback_used; }
	void set_fallback_used(bool p_used) { fallback_used = p_used; }
	float get_scale_factor() const { return scale_factor; }

	int get_scaled_border_width() const { return scaled_border_width; }
	int get_scaled_corner_radius() const { return scaled_corner_radius; }
	Color get_border_color() const { return border_color; }
	Color get_fill_color() const { return fill_color; }
	Color get_font_color() const { return font_color; }
	String get_image_path() const { return image_path; }
	String get_fill_image_path() const { return fill_image_path; }
	Dictionary get_scaled_padding() const;
	Dictionary get_scaled_nine_slice() const;

	ClockGearsWindowStyleBox();
};

class ClockGearsWindowStyleManager : public Object {
	GDCLASS(ClockGearsWindowStyleManager, Object);

	static ClockGearsWindowStyleManager *singleton;

protected:
	static void _bind_methods();

public:
	static ClockGearsWindowStyleManager *get_singleton() { return singleton; }

	static float compute_viewport_scale(const Control *p_control, bool p_pixel_art = true);
	static Ref<Texture2D> load_texture_safe(const String &p_path);

	static Ref<ClockGearsWindowStyleBox> build_window_style_box(const String &p_window_type_id, float p_vp_scale = 1.0f, const Dictionary &p_overrides = Dictionary());
	static Ref<ClockGearsWindowStyleBox> build_button_style_box(const String &p_window_type_id, const String &p_state = "normal", float p_vp_scale = 1.0f, const Dictionary &p_overrides = Dictionary());

	static void apply_window_style(Control *p_control, const String &p_window_type_id, const Dictionary &p_overrides = Dictionary());
	static void apply_button_style(Button *p_button, const String &p_window_type_id, const Dictionary &p_overrides = Dictionary());
	static void apply_buttons_in_container(Node *p_container, const String &p_window_type_id, const Dictionary &p_overrides = Dictionary());

	static Dictionary inspect_control_style(const Control *p_control);
	static void clear_caches();

	ClockGearsWindowStyleManager();
	~ClockGearsWindowStyleManager();
};

#endif // CLOCKGEARS_WINDOW_STYLE_H
