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
	sb->set_anti_aliased(true);
	sb->set_bg_color(color_panel());
	sb->set_corner_radius_all(int(6 * p_scale));
	sb->set_border_width_all(1);
	sb->set_border_color(Color(1.0f, 1.0f, 1.0f, 0.10f));
	sb->set_content_margin_all(int(12 * p_scale));
	return sb;
}

Ref<StyleBoxFlat> ClockGearsTheme::make_input_stylebox(bool p_has_error, bool p_focused, float p_scale) {
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_anti_aliased(true);
	if (p_has_error) {
		sb->set_bg_color(Color(0.28f, 0.12f, 0.12f, 1.0f));
		sb->set_border_width_all(1);
		sb->set_border_color(color_error());
	} else {
		sb->set_bg_color(Color::html("#16181d"));
		sb->set_border_width_all(1);
		sb->set_border_color(p_focused ? color_accent_amber() : Color(1.0f, 1.0f, 1.0f, 0.14f));
	}
	sb->set_corner_radius_all(int(6 * p_scale));
	sb->set_content_margin_individual(int(8 * p_scale), int(6 * p_scale), int(8 * p_scale), int(6 * p_scale));
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
	const Color txt_dis = p_light ? Color::html("#9aa0ab") : Color(txt_sec.r, txt_sec.g, txt_sec.b, 0.55f);
	const int r6 = int(6 * p_scale);
	const int m4 = int(4 * p_scale);
	const int m6 = int(6 * p_scale);
	const int m8 = int(8 * p_scale);
	const int m12 = int(12 * p_scale);
	const Color border_subtle = p_light ? Color(0, 0, 0, 0.14f) : Color(1, 1, 1, 0.12f);

	// 1. Panel & PanelContainer (esquinas redondeadas 6px, borde fino 1px, margen 12px)
	Ref<StyleBoxFlat> sb_panel;
	sb_panel.instantiate();
	sb_panel->set_anti_aliased(true);
	sb_panel->set_bg_color(panel_col);
	sb_panel->set_corner_radius_all(r6);
	sb_panel->set_border_width_all(1);
	sb_panel->set_border_color(border_subtle);
	sb_panel->set_content_margin_all(m12);
	th->set_stylebox("panel", "Panel", sb_panel);
	th->set_stylebox("panel", "PanelContainer", sb_panel);

	// 2. AcceptDialog / ConfirmationDialog / FileDialog / Window
	Ref<StyleBoxFlat> sb_win;
	sb_win.instantiate();
	sb_win->set_anti_aliased(true);
	sb_win->set_bg_color(bg);
	sb_win->set_corner_radius_all(r6);
	sb_win->set_border_width_all(1);
	sb_win->set_border_color(Color(accent.r, accent.g, accent.b, 0.45f));
	sb_win->set_content_margin_all(m12);
	th->set_stylebox("panel", "AcceptDialog", sb_win);
	th->set_stylebox("panel", "ConfirmationDialog", sb_win);
	th->set_stylebox("panel", "FileDialog", sb_win);
	th->set_stylebox("embedded_border", "Window", sb_win);
	th->set_constant("buttons_separation", "AcceptDialog", m8);
	th->set_color("title_color", "Window", txt);

	// 3. Button & OptionButton (6px, borde 1px, padding 8px, estados diferenciados)
	Ref<StyleBoxFlat> btn_normal;
	btn_normal.instantiate();
	btn_normal->set_anti_aliased(true);
	btn_normal->set_bg_color(p_light ? Color::html("#e2e5ec") : Color::html("#2a2e38"));
	btn_normal->set_corner_radius_all(r6);
	btn_normal->set_border_width_all(1);
	btn_normal->set_border_color(p_light ? Color(0, 0, 0, 0.16f) : Color(1, 1, 1, 0.14f));
	btn_normal->set_content_margin_all(m8);

	Ref<StyleBoxFlat> btn_hover = btn_normal->duplicate();
	btn_hover->set_bg_color(p_light ? Color::html("#d5d9e2") : Color::html("#353b48"));
	btn_hover->set_border_color(Color(accent.r, accent.g, accent.b, 0.65f));

	Ref<StyleBoxFlat> btn_pressed = btn_normal->duplicate();
	btn_pressed->set_bg_color(p_light ? Color(accent.r, accent.g, accent.b, 0.18f) : Color(0.24f, 0.20f, 0.14f, 1.0f));
	btn_pressed->set_border_color(accent);

	Ref<StyleBoxFlat> btn_hover_pressed = btn_normal->duplicate();
	btn_hover_pressed->set_bg_color(p_light ? Color(accent.r, accent.g, accent.b, 0.28f) : Color(0.29f, 0.24f, 0.16f, 1.0f));
	btn_hover_pressed->set_border_color(accent);

	Ref<StyleBoxFlat> btn_focus = btn_normal->duplicate();
	btn_focus->set_draw_center(false);
	btn_focus->set_border_width_all(1);
	btn_focus->set_border_color(accent);

	Ref<StyleBoxFlat> btn_disabled = btn_normal->duplicate();
	btn_disabled->set_bg_color(p_light ? Color::html("#ebedf2") : Color::html("#1d1f26"));
	btn_disabled->set_border_color(Color(1, 1, 1, 0.06f));

	const char *button_types[] = { "Button", "OptionButton", nullptr };
	for (int bi = 0; button_types[bi] != nullptr; bi++) {
		const String btype = button_types[bi];
		th->set_stylebox("normal", btype, btn_normal);
		th->set_stylebox("hover", btype, btn_hover);
		th->set_stylebox("pressed", btype, btn_pressed);
		th->set_stylebox("hover_pressed", btype, btn_hover_pressed);
		th->set_stylebox("focus", btype, btn_focus);
		th->set_stylebox("disabled", btype, btn_disabled);
		th->set_color("font_color", btype, txt);
		th->set_color("font_hover_color", btype, p_light ? txt : Color::html("#ffffff"));
		th->set_color("font_pressed_color", btype, accent);
		th->set_color("font_hover_pressed_color", btype, p_light ? txt : Color::html("#ffffff"));
		th->set_color("font_focus_color", btype, p_light ? txt : Color::html("#ffffff"));
		th->set_color("font_disabled_color", btype, txt_dis);
		th->set_color("icon_normal_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_hover_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_pressed_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_hover_pressed_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_focus_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_disabled_color", btype, Color(1, 1, 1, 0.5f));
		th->set_constant("h_separation", btype, m6);
	}

	// 3b. CheckBox & CheckButton (borde 1px al pasar/enfocar/marcar, esquinas 6px, padding 8px)
	Ref<StyleBoxFlat> chk_normal;
	chk_normal.instantiate();
	chk_normal->set_anti_aliased(true);
	chk_normal->set_bg_color(Color(0, 0, 0, 0));
	chk_normal->set_corner_radius_all(r6);
	chk_normal->set_border_width_all(1);
	chk_normal->set_border_color(Color(1, 1, 1, 0.0f));
	chk_normal->set_content_margin_individual(m8, m4, m8, m4);

	Ref<StyleBoxFlat> chk_hover = chk_normal->duplicate();
	chk_hover->set_bg_color(p_light ? Color(0, 0, 0, 0.05f) : Color(1, 1, 1, 0.05f));
	chk_hover->set_border_color(Color(accent.r, accent.g, accent.b, 0.45f));

	Ref<StyleBoxFlat> chk_pressed = chk_normal->duplicate();
	chk_pressed->set_bg_color(Color(accent.r, accent.g, accent.b, 0.14f));
	chk_pressed->set_border_color(Color(accent.r, accent.g, accent.b, 0.70f));

	const char *check_types[] = { "CheckBox", "CheckButton", nullptr };
	for (int ci = 0; check_types[ci] != nullptr; ci++) {
		const String ctype = check_types[ci];
		th->set_stylebox("normal", ctype, chk_normal);
		th->set_stylebox("hover", ctype, chk_hover);
		th->set_stylebox("pressed", ctype, chk_pressed);
		th->set_stylebox("hover_pressed", ctype, chk_pressed);
		th->set_stylebox("focus", ctype, btn_focus);
		th->set_stylebox("disabled", ctype, chk_normal);
		th->set_color("font_color", ctype, txt);
		th->set_color("font_hover_color", ctype, p_light ? txt : Color::html("#ffffff"));
		th->set_color("font_pressed_color", ctype, accent);
		th->set_color("font_hover_pressed_color", ctype, accent);
		th->set_color("font_disabled_color", ctype, txt_dis);
		th->set_constant("h_separation", ctype, m8);
	}

	// 3c. PopupMenu (desplegables y menús)
	Ref<StyleBoxFlat> pop_panel;
	pop_panel.instantiate();
	pop_panel->set_anti_aliased(true);
	pop_panel->set_bg_color(p_light ? Color::html("#ffffff") : Color::html("#1d2028"));
	pop_panel->set_corner_radius_all(r6);
	pop_panel->set_border_width_all(1);
	pop_panel->set_border_color(p_light ? Color(0, 0, 0, 0.18f) : Color(1, 1, 1, 0.14f));
	pop_panel->set_content_margin_all(m6);

	Ref<StyleBoxFlat> pop_hover;
	pop_hover.instantiate();
	pop_hover->set_anti_aliased(true);
	pop_hover->set_bg_color(Color(accent.r, accent.g, accent.b, 0.20f));
	pop_hover->set_corner_radius_all(r6);
	pop_hover->set_border_width_all(1);
	pop_hover->set_border_color(Color(accent.r, accent.g, accent.b, 0.70f));
	pop_hover->set_content_margin_individual(m8, m4, m8, m4);

	th->set_stylebox("panel", "PopupMenu", pop_panel);
	th->set_stylebox("hover", "PopupMenu", pop_hover);
	th->set_color("font_color", "PopupMenu", txt);
	th->set_color("font_hover_color", "PopupMenu", p_light ? txt : Color::html("#ffffff"));
	th->set_color("font_disabled_color", "PopupMenu", txt_dis);
	th->set_color("font_accelerator_color", "PopupMenu", txt_sec);

	// 4. LineEdit, TextEdit & SpinBox
	Ref<StyleBoxFlat> le_normal = make_input_stylebox(false, false, p_scale);
	if (p_light) {
		le_normal->set_bg_color(Color::html("#ffffff"));
		le_normal->set_border_color(Color(0, 0, 0, 0.18f));
	}
	Ref<StyleBoxFlat> le_focus = make_input_stylebox(false, true, p_scale);
	if (p_light) {
		le_focus->set_bg_color(Color::html("#ffffff"));
	}
	Ref<StyleBoxFlat> le_ro = le_normal->duplicate();
	le_ro->set_bg_color(p_light ? Color::html("#eef0f4") : Color::html("#13151a"));
	le_ro->set_border_color(Color(1, 1, 1, 0.08f));

	th->set_stylebox("normal", "LineEdit", le_normal);
	th->set_stylebox("focus", "LineEdit", le_focus);
	th->set_stylebox("read_only", "LineEdit", le_ro);
	th->set_color("font_color", "LineEdit", txt);
	th->set_color("font_uneditable_color", "LineEdit", txt_sec);
	th->set_color("font_placeholder_color", "LineEdit", txt_sec);
	th->set_color("caret_color", "LineEdit", accent);
	th->set_color("selection_color", "LineEdit", Color(accent.r, accent.g, accent.b, 0.32f));

	th->set_stylebox("normal", "TextEdit", le_normal);
	th->set_stylebox("focus", "TextEdit", le_focus);
	th->set_stylebox("read_only", "TextEdit", le_ro);
	th->set_color("font_color", "TextEdit", txt);
	th->set_color("font_readonly_color", "TextEdit", txt_sec);
	th->set_color("caret_color", "TextEdit", accent);

	Ref<StyleBoxFlat> sb_up_norm = btn_normal->duplicate();
	sb_up_norm->set_corner_radius_all(0);
	sb_up_norm->set_corner_radius(CORNER_TOP_RIGHT, r6);
	sb_up_norm->set_content_margin_all(m4);
	Ref<StyleBoxFlat> sb_up_hov = sb_up_norm->duplicate();
	sb_up_hov->set_bg_color(btn_hover->get_bg_color());
	sb_up_hov->set_border_color(Color(accent.r, accent.g, accent.b, 0.65f));
	Ref<StyleBoxFlat> sb_up_prs = sb_up_norm->duplicate();
	sb_up_prs->set_bg_color(btn_pressed->get_bg_color());
	sb_up_prs->set_border_color(accent);
	Ref<StyleBoxFlat> sb_up_dis = sb_up_norm->duplicate();
	sb_up_dis->set_bg_color(btn_disabled->get_bg_color());
	sb_up_dis->set_border_color(btn_disabled->get_border_color());

	Ref<StyleBoxFlat> sb_dn_norm = btn_normal->duplicate();
	sb_dn_norm->set_corner_radius_all(0);
	sb_dn_norm->set_corner_radius(CORNER_BOTTOM_RIGHT, r6);
	sb_dn_norm->set_content_margin_all(m4);
	Ref<StyleBoxFlat> sb_dn_hov = sb_dn_norm->duplicate();
	sb_dn_hov->set_bg_color(btn_hover->get_bg_color());
	sb_dn_hov->set_border_color(Color(accent.r, accent.g, accent.b, 0.65f));
	Ref<StyleBoxFlat> sb_dn_prs = sb_dn_norm->duplicate();
	sb_dn_prs->set_bg_color(btn_pressed->get_bg_color());
	sb_dn_prs->set_border_color(accent);
	Ref<StyleBoxFlat> sb_dn_dis = sb_dn_norm->duplicate();
	sb_dn_dis->set_bg_color(btn_disabled->get_bg_color());
	sb_dn_dis->set_border_color(btn_disabled->get_border_color());

	th->set_stylebox("up_background", "SpinBox", sb_up_norm);
	th->set_stylebox("up_background_hovered", "SpinBox", sb_up_hov);
	th->set_stylebox("up_background_pressed", "SpinBox", sb_up_prs);
	th->set_stylebox("up_background_disabled", "SpinBox", sb_up_dis);
	th->set_stylebox("down_background", "SpinBox", sb_dn_norm);
	th->set_stylebox("down_background_hovered", "SpinBox", sb_dn_hov);
	th->set_stylebox("down_background_pressed", "SpinBox", sb_dn_prs);
	th->set_stylebox("down_background_disabled", "SpinBox", sb_dn_dis);
	th->set_constant("buttons_vertical_separation", "SpinBox", 2);

	// 5. ItemList & Tree (6px, borde 1px, estados hovered/selected/focus)
	Ref<StyleBoxFlat> list_bg;
	list_bg.instantiate();
	list_bg->set_anti_aliased(true);
	list_bg->set_bg_color(p_light ? Color::html("#ffffff") : Color::html("#17191f"));
	list_bg->set_corner_radius_all(r6);
	list_bg->set_border_width_all(1);
	list_bg->set_border_color(border_subtle);
	list_bg->set_content_margin_all(m8);

	Ref<StyleBoxFlat> item_sel;
	item_sel.instantiate();
	item_sel->set_anti_aliased(true);
	item_sel->set_bg_color(Color(accent.r, accent.g, accent.b, 0.22f));
	item_sel->set_corner_radius_all(r6);
	item_sel->set_border_width_all(1);
	item_sel->set_border_color(accent);

	Ref<StyleBoxFlat> item_hov;
	item_hov.instantiate();
	item_hov->set_anti_aliased(true);
	item_hov->set_bg_color(p_light ? Color(0, 0, 0, 0.06f) : Color::html("#262a34"));
	item_hov->set_corner_radius_all(r6);
	item_hov->set_border_width_all(1);
	item_hov->set_border_color(Color(accent.r, accent.g, accent.b, 0.35f));

	Ref<StyleBoxFlat> item_hov_sel = item_sel->duplicate();
	item_hov_sel->set_bg_color(Color(accent.r, accent.g, accent.b, 0.32f));

	th->set_stylebox("panel", "ItemList", list_bg);
	th->set_stylebox("focus", "ItemList", btn_focus);
	th->set_stylebox("hovered", "ItemList", item_hov);
	th->set_stylebox("hovered_selected", "ItemList", item_hov_sel);
	th->set_stylebox("hovered_selected_focus", "ItemList", item_hov_sel);
	th->set_stylebox("selected", "ItemList", item_sel);
	th->set_stylebox("selected_focus", "ItemList", item_sel);
	th->set_constant("v_separation", "ItemList", m6);
	th->set_constant("h_separation", "ItemList", m8);
	th->set_color("font_color", "ItemList", txt);
	th->set_color("font_hovered_color", "ItemList", p_light ? txt : Color::html("#ffffff"));
	th->set_color("font_selected_color", "ItemList", p_light ? txt : Color::html("#ffffff"));

	th->set_stylebox("panel", "Tree", list_bg);
	th->set_stylebox("focus", "Tree", btn_focus);
	th->set_stylebox("hovered", "Tree", item_hov);
	th->set_stylebox("selected", "Tree", item_sel);
	th->set_stylebox("selected_focus", "Tree", item_sel);
	th->set_color("font_color", "Tree", txt);
	th->set_color("font_selected_color", "Tree", p_light ? txt : Color::html("#ffffff"));
	th->set_color("guide_color", "Tree", Color(1, 1, 1, 0.05f));

	// 6. TabContainer, TabBar & ClockGearsTabButton (activa con acento + redondeado arriba; inactivas apagadas)
	Ref<StyleBoxFlat> tab_sel;
	tab_sel.instantiate();
	tab_sel->set_anti_aliased(true);
	tab_sel->set_bg_color(p_light ? Color::html("#ffffff") : Color(0.20f, 0.18f, 0.14f, 1.0f));
	tab_sel->set_corner_radius(CORNER_TOP_LEFT, r6);
	tab_sel->set_corner_radius(CORNER_TOP_RIGHT, r6);
	tab_sel->set_border_width_all(1);
	tab_sel->set_border_color(accent);
	tab_sel->set_content_margin_individual(m12, m8, m12, m8);

	Ref<StyleBoxFlat> tab_unsel;
	tab_unsel.instantiate();
	tab_unsel->set_anti_aliased(true);
	tab_unsel->set_bg_color(p_light ? Color::html("#e3e6ee") : Color::html("#17191f"));
	tab_unsel->set_corner_radius(CORNER_TOP_LEFT, r6);
	tab_unsel->set_corner_radius(CORNER_TOP_RIGHT, r6);
	tab_unsel->set_border_width_all(1);
	tab_unsel->set_border_color(p_light ? Color(0, 0, 0, 0.10f) : Color(1, 1, 1, 0.08f));
	tab_unsel->set_content_margin_individual(m12, m8, m12, m8);

	Ref<StyleBoxFlat> tab_hov = tab_unsel->duplicate();
	tab_hov->set_bg_color(p_light ? Color::html("#edf0f7") : Color::html("#272b35"));
	tab_hov->set_border_color(Color(accent.r, accent.g, accent.b, 0.55f));

	Ref<StyleBoxFlat> tab_hov_sel = tab_sel->duplicate();
	tab_hov_sel->set_bg_color(p_light ? Color::html("#fff9ee") : Color(0.25f, 0.22f, 0.16f, 1.0f));

	Ref<StyleBoxFlat> tab_dis = tab_unsel->duplicate();
	tab_dis->set_bg_color(p_light ? Color::html("#eceef4") : Color::html("#13151a"));
	tab_dis->set_border_color(Color(1, 1, 1, 0.04f));

	Ref<StyleBoxFlat> tab_foc = tab_unsel->duplicate();
	tab_foc->set_draw_center(false);
	tab_foc->set_border_color(accent);

	th->set_stylebox("tab_selected", "TabBar", tab_sel);
	th->set_stylebox("tab_unselected", "TabBar", tab_unsel);
	th->set_stylebox("tab_hovered", "TabBar", tab_hov);
	th->set_stylebox("tab_disabled", "TabBar", tab_dis);
	th->set_stylebox("tab_focus", "TabBar", tab_foc);
	th->set_color("font_selected_color", "TabBar", accent);
	th->set_color("font_hovered_color", "TabBar", p_light ? txt : Color::html("#ffffff"));
	th->set_color("font_unselected_color", "TabBar", txt_sec);
	th->set_color("font_disabled_color", "TabBar", txt_dis);

	th->set_stylebox("tab_selected", "TabContainer", tab_sel);
	th->set_stylebox("tab_unselected", "TabContainer", tab_unsel);
	th->set_stylebox("tab_hovered", "TabContainer", tab_hov);
	th->set_stylebox("tab_disabled", "TabContainer", tab_dis);
	th->set_stylebox("tab_focus", "TabContainer", tab_foc);
	th->set_stylebox("panel", "TabContainer", sb_panel);
	th->set_color("font_selected_color", "TabContainer", accent);
	th->set_color("font_hovered_color", "TabContainer", p_light ? txt : Color::html("#ffffff"));
	th->set_color("font_unselected_color", "TabContainer", txt_sec);
	th->set_color("font_disabled_color", "TabContainer", txt_dis);

	// 6b. Variación "ClockGearsTabButton" para pestañas de capítulo y categoría
	th->set_type_variation("ClockGearsTabButton", "Button");
	th->set_stylebox("normal", "ClockGearsTabButton", tab_unsel);
	th->set_stylebox("hover", "ClockGearsTabButton", tab_hov);
	th->set_stylebox("pressed", "ClockGearsTabButton", tab_sel);
	th->set_stylebox("hover_pressed", "ClockGearsTabButton", tab_hov_sel);
	th->set_stylebox("focus", "ClockGearsTabButton", tab_foc);
	th->set_stylebox("disabled", "ClockGearsTabButton", tab_dis);
	th->set_color("font_color", "ClockGearsTabButton", txt_sec);
	th->set_color("font_hover_color", "ClockGearsTabButton", p_light ? txt : Color::html("#ffffff"));
	th->set_color("font_pressed_color", "ClockGearsTabButton", accent);
	th->set_color("font_hover_pressed_color", "ClockGearsTabButton", p_light ? txt : Color::html("#ffffff"));
	th->set_color("font_focus_color", "ClockGearsTabButton", p_light ? txt : Color::html("#ffffff"));
	th->set_color("font_disabled_color", "ClockGearsTabButton", txt_dis);
	th->set_color("icon_normal_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_hover_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_pressed_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_hover_pressed_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_focus_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_disabled_color", "ClockGearsTabButton", Color(1, 1, 1, 0.5f));
	th->set_constant("h_separation", "ClockGearsTabButton", m8);

	// 7. Labels & Separators
	th->set_color("font_color", "Label", txt);
	Ref<StyleBoxLine> sep_line;
	sep_line.instantiate();
	sep_line->set_color(p_light ? Color(0, 0, 0, 0.12f) : Color(1, 1, 1, 0.10f));
	sep_line->set_thickness(1);
	th->set_stylebox("separator", "HSeparator", sep_line);
	th->set_stylebox("separator", "PopupMenu", sep_line);

	Ref<StyleBoxLine> vsep_line = sep_line->duplicate();
	vsep_line->set_vertical(true);
	th->set_stylebox("separator", "VSeparator", vsep_line);

	// 8. Espaciado estándar
	th->set_constant("separation", "HBoxContainer", m8);
	th->set_constant("separation", "VBoxContainer", m8);
	th->set_constant("h_separation", "HFlowContainer", m8);
	th->set_constant("v_separation", "HFlowContainer", m8);
	th->set_constant("h_separation", "VFlowContainer", m8);
	th->set_constant("v_separation", "VFlowContainer", m8);
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
