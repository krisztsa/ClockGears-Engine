#pragma once

#include "core/math/color.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "scene/gui/control.h"
#include "scene/main/window.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/theme.h"

class ClockGearsTheme : public Object {
	GDCLASS(ClockGearsTheme, Object);

public:
	// Paleta oficial ClockGears Dark (Sección 11.2)
	static Color color_bg_dark() { return Color::html("#1b1d23"); }
	static Color color_panel() { return Color::html("#23262e"); }
	static Color color_accent_amber() { return Color::html("#e0a040"); }
	static Color color_text_primary() { return Color::html("#e6e6e6"); }
	static Color color_text_secondary() { return Color::html("#9aa0ab"); }
	static Color color_error() { return Color::html("#e5534b"); }
	static Color color_success() { return Color::html("#4cc38a"); }

private:
	static ClockGearsTheme *singleton;
	Ref<Theme> dark_theme;
	Ref<Theme> light_theme;
	bool dark_has_editor_base = false;
	bool light_has_editor_base = false;
	bool use_light_mode = false;

	Ref<Theme> _build_theme(bool p_light, float p_scale, bool *r_has_editor_base = nullptr) const;

protected:
	static void _bind_methods();

public:
	static ClockGearsTheme *get_singleton() { return singleton; }

	Ref<Theme> get_theme(bool p_light_mode = false);
	Ref<Theme> get_game_pixel_theme(int p_integer_scale = 1);
	void set_light_mode(bool p_light);
	bool is_light_mode() const { return use_light_mode; }

	void apply_to_control(Control *p_control);
	void apply_to_window(Window *p_window);

	static Ref<StyleBoxFlat> make_card_stylebox(float p_scale = 1.0f);
	static Ref<StyleBoxFlat> make_input_stylebox(bool p_has_error, bool p_focused, float p_scale = 1.0f);

	ClockGearsTheme();
	~ClockGearsTheme();
};
