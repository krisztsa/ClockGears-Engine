#include "clockgears_theme.h"
#include "scene/resources/style_box_line.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#define CG_SCALE EDSCALE
#else
#define CG_SCALE 1.0f
#endif

ClockGearsTheme *ClockGearsTheme::singleton = nullptr;

Ref<StyleBoxFlat> ClockGearsTheme::make_card_stylebox(float p_scale) {
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_anti_aliased(false);
	sb->set_bg_color(color_panel());
	sb->set_corner_radius_all(int(4 * p_scale));
	sb->set_border_width_all(1);
	sb->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.08f));
	sb->set_content_margin_all(int(8 * p_scale));
	return sb;
}

Ref<StyleBoxFlat> ClockGearsTheme::make_input_stylebox(bool p_has_error, bool p_focused, float p_scale) {
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_anti_aliased(false);
	if (p_has_error) {
		sb->set_bg_color(Color(0.28f, 0.12f, 0.12f, 1.0f));
		sb->set_border_width_all(int(MAX(1.0f, Math::round(p_scale))));
		sb->set_border_color(color_error());
	} else {
		sb->set_bg_color(Color::html("#16181d"));
		sb->set_border_width_all(1);
		sb->set_border_color(p_focused ? color_accent_amber() : Color(1.0f, 1.0f, 1.0f, 0.14f));
	}
	sb->set_corner_radius_all(int(4 * p_scale));
	sb->set_content_margin_individual(int(8 * p_scale), int(4 * p_scale), int(8 * p_scale), int(4 * p_scale));
	return sb;
}

Ref<Theme> ClockGearsTheme::_build_theme(bool p_light, float p_scale, bool *r_has_editor_base) const {
	Ref<Theme> th;
	bool has_base = false;
#ifdef TOOLS_ENABLED
	if (EditorNode::get_singleton() && EditorNode::get_singleton()->get_editor_theme().is_valid()) {
		// Partir SIEMPRE de una copia del tema del editor para conservar todos los EditorIcons
		// (Error, Warning, StatusError, StatusWarning, Folder, Search, Clear, etc.) y EditorFonts.
		th = EditorNode::get_singleton()->get_editor_theme()->duplicate();
		has_base = true;
	}
#endif
	if (th.is_null()) {
		th.instantiate();
	}
	if (r_has_editor_base) {
		*r_has_editor_base = has_base;
	}

	const Color bg = p_light ? Color::html("#eef0f4") : color_bg_dark();
	const Color panel_col = p_light ? Color::html("#ffffff") : color_panel();
	const Color accent = color_accent_amber();
	const Color txt = p_light ? Color::html("#1e2026") : color_text_primary();
	const Color txt_sec = p_light ? Color::html("#5c6270") : color_text_secondary();
	const int r4 = int(4 * p_scale);
	const int r6 = int(6 * p_scale);
	const int m4 = int(4 * p_scale);
	const int m8 = int(8 * p_scale);

	// 1. Panel & PanelContainer (A4.3: anti_aliased = false para bordes nítidos alineados a píxel)
	Ref<StyleBoxFlat> sb_panel;
	sb_panel.instantiate();
	sb_panel->set_anti_aliased(false);
	sb_panel->set_bg_color(panel_col);
	sb_panel->set_corner_radius_all(r6);
	sb_panel->set_border_width_all(1);
	sb_panel->set_border_color(p_light ? Color(0, 0, 0, 0.12f) : Color(1, 1, 1, 0.08f));
	sb_panel->set_content_margin_all(m8);
	th->set_stylebox("panel", "Panel", sb_panel);
	th->set_stylebox("panel", "PanelContainer", sb_panel);

	// 2. AcceptDialog / Window
	Ref<StyleBoxFlat> sb_win;
	sb_win.instantiate();
	sb_win->set_anti_aliased(false);
	sb_win->set_bg_color(bg);
	sb_win->set_corner_radius_all(r6);
	sb_win->set_border_width_all(1);
	sb_win->set_border_color(Color(accent.r, accent.g, accent.b, 0.40f));
	sb_win->set_content_margin_all(m8);
	th->set_stylebox("panel", "AcceptDialog", sb_win);
	th->set_stylebox("embedded_border", "Window", sb_win);
	th->set_color("title_color", "Window", txt);

	// 3. Button (normal, hover, pressed, focus, disabled)
	Ref<StyleBoxFlat> btn_normal;
	btn_normal.instantiate();
	btn_normal->set_anti_aliased(false);
	btn_normal->set_bg_color(p_light ? Color::html("#e2e5ec") : Color::html("#2a2e38"));
	btn_normal->set_corner_radius_all(r4);
	btn_normal->set_border_width_all(1);
	btn_normal->set_border_color(p_light ? Color(0, 0, 0, 0.15f) : Color(1, 1, 1, 0.12f));
	btn_normal->set_content_margin_individual(m8, m4, m8, m4);

	Ref<StyleBoxFlat> btn_hover = btn_normal->duplicate();
	btn_hover->set_bg_color(p_light ? Color::html("#d5d9e2") : Color::html("#343946"));
	btn_hover->set_border_color(Color(accent.r, accent.g, accent.b, 0.65f));

	Ref<StyleBoxFlat> btn_pressed = btn_normal->duplicate();
	btn_pressed->set_bg_color(Color(accent.r, accent.g, accent.b, 0.24f));
	btn_pressed->set_border_color(accent);

	Ref<StyleBoxFlat> btn_focus = btn_normal->duplicate();
	btn_focus->set_draw_center(false);
	btn_focus->set_border_width_all(int(MAX(1.0f, Math::round(p_scale))));
	btn_focus->set_border_color(accent);

	Ref<StyleBoxFlat> btn_disabled = btn_normal->duplicate();
	btn_disabled->set_bg_color(p_light ? Color::html("#ebedf2") : Color::html("#1e2027"));
	btn_disabled->set_border_color(Color(1, 1, 1, 0.05f));

	th->set_stylebox("normal", "Button", btn_normal);
	th->set_stylebox("hover", "Button", btn_hover);
	th->set_stylebox("pressed", "Button", btn_pressed);
	th->set_stylebox("focus", "Button", btn_focus);
	th->set_stylebox("disabled", "Button", btn_disabled);
	th->set_color("font_color", "Button", txt);
	th->set_color("font_hover_color", "Button", p_light ? txt : Color::html("#ffffff"));
	th->set_color("font_pressed_color", "Button", accent);
	th->set_color("font_disabled_color", "Button", txt_sec);

	// 4. LineEdit & TextEdit
	Ref<StyleBoxFlat> le_normal = make_input_stylebox(false, false, p_scale);
	if (p_light) {
		le_normal->set_bg_color(Color::html("#ffffff"));
	}
	Ref<StyleBoxFlat> le_focus = make_input_stylebox(false, true, p_scale);
	if (p_light) {
		le_focus->set_bg_color(Color::html("#ffffff"));
	}
	th->set_stylebox("normal", "LineEdit", le_normal);
	th->set_stylebox("focus", "LineEdit", le_focus);
	th->set_color("font_color", "LineEdit", txt);
	th->set_color("font_placeholder_color", "LineEdit", txt_sec);
	th->set_color("caret_color", "LineEdit", accent);
	th->set_color("selection_color", "LineEdit", Color(accent.r, accent.g, accent.b, 0.32f));

	th->set_stylebox("normal", "TextEdit", le_normal);
	th->set_stylebox("focus", "TextEdit", le_focus);
	th->set_color("font_color", "TextEdit", txt);
	th->set_color("caret_color", "TextEdit", accent);

	// 5. ItemList & Tree (filas alternas y selección ámbar)
	Ref<StyleBoxFlat> list_bg;
	list_bg.instantiate();
	list_bg->set_anti_aliased(false);
	list_bg->set_bg_color(p_light ? Color::html("#ffffff") : Color::html("#17191f"));
	list_bg->set_corner_radius_all(r4);
	list_bg->set_border_width_all(1);
	list_bg->set_border_color(p_light ? Color(0, 0, 0, 0.12f) : Color(1, 1, 1, 0.10f));
	list_bg->set_content_margin_all(m4);

	Ref<StyleBoxFlat> item_sel;
	item_sel.instantiate();
	item_sel->set_anti_aliased(false);
	item_sel->set_bg_color(Color(accent.r, accent.g, accent.b, 0.25f));
	item_sel->set_corner_radius_all(r4);
	item_sel->set_border_width_all(1);
	item_sel->set_border_color(accent);

	th->set_stylebox("panel", "ItemList", list_bg);
	th->set_stylebox("selected", "ItemList", item_sel);
	th->set_stylebox("selected_focus", "ItemList", item_sel);
	th->set_color("font_color", "ItemList", txt);
	th->set_color("font_selected_color", "ItemList", p_light ? txt : Color::html("#ffffff"));

	th->set_stylebox("panel", "Tree", list_bg);
	th->set_stylebox("selected", "Tree", item_sel);
	th->set_stylebox("selected_focus", "Tree", item_sel);
	th->set_color("font_color", "Tree", txt);
	th->set_color("font_selected_color", "Tree", p_light ? txt : Color::html("#ffffff"));
	th->set_color("guide_color", "Tree", Color(1, 1, 1, 0.05f));

	// 6. TabContainer & TabBar
	Ref<StyleBoxFlat> tab_sel;
	tab_sel.instantiate();
	tab_sel->set_anti_aliased(false);
	tab_sel->set_bg_color(panel_col);
	tab_sel->set_corner_radius(CORNER_TOP_LEFT, r4);
	tab_sel->set_corner_radius(CORNER_TOP_RIGHT, r4);
	tab_sel->set_border_width(SIDE_TOP, int(MAX(2.0f, 2.0f * p_scale)));
	tab_sel->set_border_color(accent);
	tab_sel->set_content_margin_individual(m8, m4, m8, m4);

	Ref<StyleBoxFlat> tab_unsel;
	tab_unsel.instantiate();
	tab_unsel->set_anti_aliased(false);
	tab_unsel->set_bg_color(bg);
	tab_unsel->set_corner_radius(CORNER_TOP_LEFT, r4);
	tab_unsel->set_corner_radius(CORNER_TOP_RIGHT, r4);
	tab_unsel->set_content_margin_individual(m8, m4, m8, m4);

	th->set_stylebox("tab_selected", "TabBar", tab_sel);
	th->set_stylebox("tab_unselected", "TabBar", tab_unsel);
	th->set_stylebox("tab_hovered", "TabBar", tab_sel);
	th->set_color("font_selected_color", "TabBar", accent);
	th->set_color("font_unselected_color", "TabBar", txt_sec);

	th->set_stylebox("tab_selected", "TabContainer", tab_sel);
	th->set_stylebox("tab_unselected", "TabContainer", tab_unsel);
	th->set_stylebox("panel", "TabContainer", sb_panel);
	th->set_color("font_selected_color", "TabContainer", accent);
	th->set_color("font_unselected_color", "TabContainer", txt_sec);

	// 7. Labels & Separators
	th->set_color("font_color", "Label", txt);
	Ref<StyleBoxLine> sep_line;
	sep_line.instantiate();
	sep_line->set_color(p_light ? Color(0, 0, 0, 0.12f) : Color(1, 1, 1, 0.10f));
	sep_line->set_thickness(1);
	th->set_stylebox("separator", "HSeparator", sep_line);

	Ref<StyleBoxLine> vsep_line = sep_line->duplicate();
	vsep_line->set_vertical(true);
	th->set_stylebox("separator", "VSeparator", vsep_line);

	// 8. Espaciado estándar en múltiplos de 4/8
	th->set_constant("separation", "HBoxContainer", m8);
	th->set_constant("separation", "VBoxContainer", m8);
	th->set_constant("h_separation", "GridContainer", m8);
	th->set_constant("v_separation", "GridContainer", m8);

	return th;
}

Ref<Theme> ClockGearsTheme::get_theme(bool p_light_mode) {
	bool editor_theme_available = false;
#ifdef TOOLS_ENABLED
	editor_theme_available = (EditorNode::get_singleton() && EditorNode::get_singleton()->get_editor_theme().is_valid());
#endif
	if (p_light_mode) {
		if (light_theme.is_null() || (editor_theme_available && !light_has_editor_base)) {
			light_theme = _build_theme(true, CG_SCALE, &light_has_editor_base);
		}
		return light_theme;
	}
	if (dark_theme.is_null() || (editor_theme_available && !dark_has_editor_base)) {
		dark_theme = _build_theme(false, CG_SCALE, &dark_has_editor_base);
	}
	return dark_theme;
}

Ref<Theme> ClockGearsTheme::get_game_pixel_theme(int p_integer_scale) {
	// A4.3: Tema de UI in-game en Pixel Art (escala entera, esquinas rectas 0px, sin antialiasing)
	int sc = MAX(1, p_integer_scale);
	Ref<Theme> gth;
	gth.instantiate();

	Ref<StyleBoxFlat> dlg_box;
	dlg_box.instantiate();
	dlg_box->set_anti_aliased(false);
	dlg_box->set_bg_color(Color::html("#12141a"));
	dlg_box->set_corner_radius_all(0);
	dlg_box->set_border_width_all(2 * sc);
	dlg_box->set_border_color(color_text_primary());
	dlg_box->set_content_margin_all(8 * sc);

	gth->set_stylebox("panel", "Panel", dlg_box);
	gth->set_stylebox("panel", "PanelContainer", dlg_box);
	gth->set_color("font_color", "Label", color_text_primary());
	gth->set_color("font_color", "RichTextLabel", color_text_primary());
	return gth;
}

void ClockGearsTheme::set_light_mode(bool p_light) {
	use_light_mode = p_light;
}

void ClockGearsTheme::apply_to_control(Control *p_control) {
	ERR_FAIL_NULL(p_control);
	p_control->set_theme(get_theme(use_light_mode));
}

void ClockGearsTheme::apply_to_window(Window *p_window) {
	ERR_FAIL_NULL(p_window);
	p_window->set_theme(get_theme(use_light_mode));
}

void ClockGearsTheme::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_theme", "light_mode"), &ClockGearsTheme::get_theme, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_game_pixel_theme", "integer_scale"), &ClockGearsTheme::get_game_pixel_theme, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("set_light_mode", "light"), &ClockGearsTheme::set_light_mode);
	ClassDB::bind_method(D_METHOD("is_light_mode"), &ClockGearsTheme::is_light_mode);
	ClassDB::bind_method(D_METHOD("apply_to_control", "control"), &ClockGearsTheme::apply_to_control);
	ClassDB::bind_method(D_METHOD("apply_to_window", "window"), &ClockGearsTheme::apply_to_window);
}

ClockGearsTheme::ClockGearsTheme() {
	singleton = this;
}

ClockGearsTheme::~ClockGearsTheme() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
