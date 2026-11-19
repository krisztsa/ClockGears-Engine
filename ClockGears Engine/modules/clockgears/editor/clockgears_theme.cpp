#include "clockgears_theme.h"
#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/object/callable_mp.h"
#include "scene/animation/tween.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_bar.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/style_box_line.h"
#include "servers/rendering/rendering_server.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#define CG_SCALE EDSCALE
#else
#define CG_SCALE 1.0f
#endif

void ClockGearsBrassStyleBox::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_gradient_mode", "mode"), &ClockGearsBrassStyleBox::set_gradient_mode);
	ClassDB::bind_method(D_METHOD("get_gradient_mode"), &ClockGearsBrassStyleBox::get_gradient_mode);
	ClassDB::bind_method(D_METHOD("set_sunken_style", "sunken"), &ClockGearsBrassStyleBox::set_sunken_style);
	ClassDB::bind_method(D_METHOD("is_sunken_style"), &ClockGearsBrassStyleBox::is_sunken_style);
	ClassDB::bind_method(D_METHOD("set_gear_separator_style", "sep"), &ClockGearsBrassStyleBox::set_gear_separator_style);
	ClassDB::bind_method(D_METHOD("is_gear_separator_style"), &ClockGearsBrassStyleBox::is_gear_separator_style);
	ClassDB::bind_method(D_METHOD("set_top_highlight_alpha", "alpha"), &ClockGearsBrassStyleBox::set_top_highlight_alpha);
	ClassDB::bind_method(D_METHOD("get_top_highlight_alpha"), &ClockGearsBrassStyleBox::get_top_highlight_alpha);
}

void ClockGearsBrassStyleBox::draw(RID p_canvas_item, const Rect2 &p_rect) const {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) {
		return;
	}
	if (is_gear_separator) {
		const float cy = Math::round(p_rect.position.y + p_rect.size.y * 0.5f);
		const float cx = Math::round(p_rect.position.x + p_rect.size.x * 0.5f);
		const Color line_col(0.86f, 0.63f, 0.25f, 0.36f);
		const Color glow_col(0.96f, 0.76f, 0.36f, 0.55f);
		if (p_rect.size.x >= 36.0f && ClockGearsTheme::get_singleton()) {
			rs->canvas_item_add_line(p_canvas_item, Point2(p_rect.position.x, cy), Point2(cx - 8.0f, cy), line_col, 1.0f);
			rs->canvas_item_add_line(p_canvas_item, Point2(cx + 8.0f, cy), Point2(p_rect.position.x + p_rect.size.x, cy), line_col, 1.0f);
			rs->canvas_item_add_line(p_canvas_item, Point2(MAX(p_rect.position.x, cx - 18.0f), cy), Point2(cx - 8.0f, cy), glow_col, 1.0f);
			rs->canvas_item_add_line(p_canvas_item, Point2(cx + 8.0f, cy), Point2(MIN(p_rect.position.x + p_rect.size.x, cx + 18.0f), cy), glow_col, 1.0f);
			Ref<ImageTexture> gtex = ClockGearsTheme::get_singleton()->get_cached_brass_gear_texture();
			if (gtex.is_valid()) {
				rs->canvas_item_add_texture_rect(p_canvas_item, Rect2(cx - 5.0f, cy - 5.0f, 11.0f, 11.0f), gtex->get_rid(), false, Color(1, 1, 1, 1));
			}
		} else {
			rs->canvas_item_add_line(p_canvas_item, Point2(p_rect.position.x, cy), Point2(p_rect.position.x + p_rect.size.x, cy), line_col, 1.0f);
		}
		return;
	}

	StyleBoxFlat::draw(p_canvas_item, p_rect);

	if (p_rect.size.x <= 4.0f || p_rect.size.y <= 4.0f) {
		return;
	}

	if (gradient_mode != GRADIENT_NONE && ClockGearsTheme::get_singleton()) {
		Ref<ImageTexture> grad_tex = ClockGearsTheme::get_singleton()->get_cached_brass_gradient_texture(gradient_mode);
		if (grad_tex.is_valid()) {
			const float inset = float(MAX(1, get_border_width_min()) + 1);
			Rect2 inner = p_rect.grow(-inset);
			if (inner.size.x > 1.0f && inner.size.y > 1.0f) {
				rs->canvas_item_add_texture_rect(p_canvas_item, inner, grad_tex->get_rid(), false, Color(1, 1, 1, 1));
			}
		}
	}

	const float r_tl = float(MAX(2, get_corner_radius(CORNER_TOP_LEFT)));
	const float r_tr = float(MAX(2, get_corner_radius(CORNER_TOP_RIGHT)));
	const float r_bl = float(MAX(2, get_corner_radius(CORNER_BOTTOM_LEFT)));
	const float r_br = float(MAX(2, get_corner_radius(CORNER_BOTTOM_RIGHT)));

	if (top_highlight_alpha > 0.001f && !is_sunken && p_rect.size.x > (r_tl + r_tr + 2.0f)) {
		const float y_top = p_rect.position.y + float(MAX(1, get_border_width(SIDE_TOP)));
		rs->canvas_item_add_line(p_canvas_item, Point2(p_rect.position.x + r_tl, y_top), Point2(p_rect.position.x + p_rect.size.x - r_tr, y_top), Color(1.0f, 0.87f, 0.52f, top_highlight_alpha), 1.0f);
	}

	if (is_sunken && p_rect.size.x > (r_tl + r_tr + 2.0f)) {
		const float y_top = p_rect.position.y + float(MAX(1, get_border_width(SIDE_TOP)));
		const float y_bot = p_rect.position.y + p_rect.size.y - float(MAX(1, get_border_width(SIDE_BOTTOM))) - 1.0f;
		rs->canvas_item_add_line(p_canvas_item, Point2(p_rect.position.x + r_tl, y_top), Point2(p_rect.position.x + p_rect.size.x - r_tr, y_top), Color(0.0f, 0.0f, 0.0f, 0.38f), 2.0f);
		if (p_rect.size.x > (r_bl + r_br + 2.0f)) {
			rs->canvas_item_add_line(p_canvas_item, Point2(p_rect.position.x + r_bl, y_bot), Point2(p_rect.position.x + p_rect.size.x - r_br, y_bot), Color(0.95f, 0.74f, 0.32f, 0.24f), 1.0f);
		}
	}
}

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
	const bool is_brass = (singleton && singleton->get_active_theme_id() == "brass");
	Ref<StyleBoxFlat> sb;
	sb.instantiate();
	sb->set_anti_aliased(true);
	if (p_has_error) {
		sb->set_bg_color(is_brass ? Color(0.19f, 0.07f, 0.07f, 1.0f) : Color(0.28f, 0.12f, 0.12f, 1.0f));
		sb->set_border_width_all(1);
		sb->set_border_color(is_brass ? Color::html("#ff8278") : color_error());
	} else {
		sb->set_bg_color(is_brass ? color_brass_input() : Color::html("#16181d"));
		sb->set_border_width_all(1);
		sb->set_border_color(p_focused ? (is_brass ? color_brass_accent() : color_accent_amber()) : (is_brass ? Color(color_brass_border().r, color_brass_border().g, color_brass_border().b, 0.45f) : Color(1.0f, 1.0f, 1.0f, 0.14f)));
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
		// Partir SIEMPRE de una copia de items del tema del editor (sin duplicate() sobre EditorTheme no expuesto)
		// para conservar EditorIcons (Error, Warning, Folder, Search, Clear, etc.) y EditorFonts.
		th.instantiate();
		th->merge_with(EditorNode::get_singleton()->get_editor_theme());
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

Ref<Theme> ClockGearsTheme::_build_brass_theme(float p_scale, bool *r_has_editor_base) const {
	if (simulate_brass_load_failure) {
		return Ref<Theme>();
	}
	Ref<Theme> th;
	bool has_base = false;
#ifdef TOOLS_ENABLED
	if (EditorNode::get_singleton() && EditorNode::get_singleton()->get_editor_theme().is_valid()) {
		th.instantiate();
		th->merge_with(EditorNode::get_singleton()->get_editor_theme());
		has_base = true;
	}
#endif
	if (th.is_null()) {
		th.instantiate();
	}
	if (r_has_editor_base) {
		*r_has_editor_base = has_base;
	}

	// Pre-generar y cachear las texturas de degradado y el motivo de engranaje (una sola vez)
	for (int gm = ClockGearsBrassStyleBox::GRADIENT_PANEL; gm <= ClockGearsBrassStyleBox::GRADIENT_BUTTON_HOVER; gm++) {
		get_cached_brass_gradient_texture(gm);
	}
	get_cached_brass_gear_texture();

	const Color bg = color_brass_bg();
	const Color panel_col = color_brass_panel();
	const Color header_col = color_brass_header();
	const Color input_col = color_brass_input();
	const Color border_brass = color_brass_border();
	const Color accent = color_brass_accent();
	const Color txt = color_brass_text_primary();
	const Color txt_bright = Color::html("#fff8ea");
	const Color txt_sec = color_brass_text_secondary();
	const Color txt_dis = color_brass_text_disabled();

	const int r6 = int(6 * p_scale);
	const int m4 = int(4 * p_scale);
	const int m6 = int(6 * p_scale);
	const int m8 = int(8 * p_scale);
	const int m12 = int(12 * p_scale);

	// 1. Panel & PanelContainer (degradado suave, borde latón con brillo superior, sombra suave)
	Ref<ClockGearsBrassStyleBox> sb_panel;
	sb_panel.instantiate();
	sb_panel->set_anti_aliased(true);
	sb_panel->set_bg_color(panel_col);
	sb_panel->set_corner_radius_all(r6);
	sb_panel->set_border_width_all(1);
	sb_panel->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.56f));
	sb_panel->set_content_margin_all(m12);
	sb_panel->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_PANEL);
	sb_panel->set_top_highlight_alpha(0.28f);
	sb_panel->set_shadow_color(Color(0.02f, 0.01f, 0.0f, 0.36f));
	sb_panel->set_shadow_size(int(4 * p_scale));
	sb_panel->set_shadow_offset(Vector2(0, 2 * p_scale));
	th->set_stylebox("panel", "Panel", sb_panel);
	th->set_stylebox("panel", "PanelContainer", sb_panel);

	// 1b. MenuBar / Cabecera de barra
	Ref<ClockGearsBrassStyleBox> sb_menubar;
	sb_menubar.instantiate();
	sb_menubar->set_anti_aliased(true);
	sb_menubar->set_bg_color(header_col);
	sb_menubar->set_corner_radius_all(r6);
	sb_menubar->set_border_width_all(1);
	sb_menubar->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.65f));
	sb_menubar->set_content_margin_individual(m8, m4, m8, m4);
	sb_menubar->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_HEADER);
	sb_menubar->set_top_highlight_alpha(0.32f);

	Ref<ClockGearsBrassStyleBox> sb_menubar_hov = sb_menubar->duplicate();
	sb_menubar_hov->set_bg_color(Color::html("#332a1e"));
	sb_menubar_hov->set_border_color(accent);
	sb_menubar_hov->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.28f));
	sb_menubar_hov->set_shadow_size(int(4 * p_scale));

	Ref<ClockGearsBrassStyleBox> sb_menubar_prs = sb_menubar->duplicate();
	sb_menubar_prs->set_bg_color(Color::html("#1d1610"));
	sb_menubar_prs->set_border_color(accent);
	sb_menubar_prs->set_sunken_style(true);

	th->set_stylebox("normal", "MenuBar", sb_menubar);
	th->set_stylebox("hover", "MenuBar", sb_menubar_hov);
	th->set_stylebox("pressed", "MenuBar", sb_menubar_prs);
	th->set_stylebox("hover_pressed", "MenuBar", sb_menubar_prs);
	th->set_stylebox("disabled", "MenuBar", sb_menubar);
	th->set_color("font_color", "MenuBar", txt);
	th->set_color("font_hover_color", "MenuBar", txt_bright);
	th->set_color("font_pressed_color", "MenuBar", accent);
	th->set_color("font_hover_pressed_color", "MenuBar", txt_bright);
	th->set_color("font_disabled_color", "MenuBar", txt_dis);
	th->set_constant("h_separation", "MenuBar", int(Math::round(6 * p_scale)));

	th->set_type_variation("MainMenuBar", "MenuBar");
	th->set_stylebox("normal", "MainMenuBar", sb_menubar);
	th->set_stylebox("hover", "MainMenuBar", sb_menubar_hov);
	th->set_stylebox("pressed", "MainMenuBar", sb_menubar_prs);
	th->set_stylebox("hover_pressed", "MainMenuBar", sb_menubar_prs);
	th->set_stylebox("disabled", "MainMenuBar", sb_menubar);
	th->set_color("font_color", "MainMenuBar", txt);
	th->set_color("font_hover_color", "MainMenuBar", txt_bright);
	th->set_color("font_pressed_color", "MainMenuBar", accent);
	th->set_color("font_hover_pressed_color", "MainMenuBar", txt_bright);
	th->set_color("font_disabled_color", "MainMenuBar", txt_dis);
	th->set_constant("h_separation", "MainMenuBar", int(Math::round(6 * p_scale)));
	th->set_meta("_cg_is_brass", true);

	// 2. AcceptDialog / ConfirmationDialog / FileDialog / Window (sombra suave bajo diálogos y borde latón)
	Ref<ClockGearsBrassStyleBox> sb_win;
	sb_win.instantiate();
	sb_win->set_anti_aliased(true);
	sb_win->set_bg_color(bg);
	sb_win->set_corner_radius_all(r6);
	sb_win->set_border_width_all(1);
	sb_win->set_border_color(Color(accent.r, accent.g, accent.b, 0.78f));
	sb_win->set_content_margin_all(m12);
	sb_win->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_PANEL);
	sb_win->set_top_highlight_alpha(0.34f);
	sb_win->set_shadow_color(Color(0.01f, 0.01f, 0.0f, 0.50f));
	sb_win->set_shadow_size(int(7 * p_scale));
	sb_win->set_shadow_offset(Vector2(0, 3 * p_scale));
	th->set_stylebox("panel", "AcceptDialog", sb_win);
	th->set_stylebox("panel", "ConfirmationDialog", sb_win);
	th->set_stylebox("panel", "FileDialog", sb_win);
	th->set_stylebox("embedded_border", "Window", sb_win);
	th->set_constant("buttons_separation", "AcceptDialog", m8);
	th->set_color("title_color", "Window", txt);

	// 3. Button, OptionButton & MenuButton (redondeados, resplandor suave en hover/focus, ligero hundimiento al pulsar)
	Ref<ClockGearsBrassStyleBox> btn_normal;
	btn_normal.instantiate();
	btn_normal->set_anti_aliased(true);
	btn_normal->set_bg_color(Color::html("#2a231a"));
	btn_normal->set_corner_radius_all(r6);
	btn_normal->set_border_width_all(1);
	btn_normal->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.58f));
	btn_normal->set_content_margin_all(m8);
	btn_normal->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_BUTTON);
	btn_normal->set_top_highlight_alpha(0.26f);

	Ref<ClockGearsBrassStyleBox> btn_hover;
	btn_hover.instantiate();
	btn_hover->set_anti_aliased(true);
	btn_hover->set_bg_color(Color::html("#332a1e"));
	btn_hover->set_corner_radius_all(r6);
	btn_hover->set_border_width_all(1);
	btn_hover->set_border_color(accent);
	btn_hover->set_content_margin_all(m8);
	btn_hover->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_BUTTON_HOVER);
	btn_hover->set_top_highlight_alpha(0.38f);
	btn_hover->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.30f));
	btn_hover->set_shadow_size(int(4 * p_scale));
	btn_hover->set_shadow_offset(Vector2(0, 0));

	Ref<ClockGearsBrassStyleBox> btn_pressed;
	btn_pressed.instantiate();
	btn_pressed->set_anti_aliased(true);
	btn_pressed->set_bg_color(Color::html("#1d1610"));
	btn_pressed->set_corner_radius_all(r6);
	btn_pressed->set_border_width_all(1);
	btn_pressed->set_border_color(accent);
	// Ligero hundimiento: desplazamos 1px hacia abajo manteniendo top + bottom == 2 * m8
	btn_pressed->set_content_margin_individual(m8, m8 + 1, m8, MAX(0, m8 - 1));
	btn_pressed->set_sunken_style(true);

	Ref<ClockGearsBrassStyleBox> btn_hover_pressed;
	btn_hover_pressed.instantiate();
	btn_hover_pressed->set_anti_aliased(true);
	btn_hover_pressed->set_bg_color(Color::html("#261d14"));
	btn_hover_pressed->set_corner_radius_all(r6);
	btn_hover_pressed->set_border_width_all(1);
	btn_hover_pressed->set_border_color(accent);
	btn_hover_pressed->set_content_margin_individual(m8, m8 + 1, m8, MAX(0, m8 - 1));
	btn_hover_pressed->set_sunken_style(true);
	btn_hover_pressed->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.26f));
	btn_hover_pressed->set_shadow_size(int(3 * p_scale));

	Ref<ClockGearsBrassStyleBox> btn_focus;
	btn_focus.instantiate();
	btn_focus->set_anti_aliased(true);
	btn_focus->set_draw_center(false);
	btn_focus->set_corner_radius_all(r6);
	btn_focus->set_border_width_all(1);
	btn_focus->set_border_color(accent);
	btn_focus->set_content_margin_all(m8);
	btn_focus->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.38f));
	btn_focus->set_shadow_size(int(5 * p_scale));
	btn_focus->set_shadow_offset(Vector2(0, 0));

	Ref<ClockGearsBrassStyleBox> btn_disabled;
	btn_disabled.instantiate();
	btn_disabled->set_anti_aliased(true);
	btn_disabled->set_bg_color(Color::html("#181511"));
	btn_disabled->set_corner_radius_all(r6);
	btn_disabled->set_border_width_all(1);
	btn_disabled->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.20f));
	btn_disabled->set_content_margin_all(m8);

	const char *button_types[] = { "Button", "OptionButton", "MenuButton", nullptr };
	for (int bi = 0; button_types[bi] != nullptr; bi++) {
		const String btype = button_types[bi];
		th->set_stylebox("normal", btype, btn_normal);
		th->set_stylebox("hover", btype, btn_hover);
		th->set_stylebox("pressed", btype, btn_pressed);
		th->set_stylebox("hover_pressed", btype, btn_hover_pressed);
		th->set_stylebox("focus", btype, btn_focus);
		th->set_stylebox("disabled", btype, btn_disabled);
		th->set_color("font_color", btype, txt);
		th->set_color("font_hover_color", btype, txt_bright);
		th->set_color("font_pressed_color", btype, accent);
		th->set_color("font_hover_pressed_color", btype, txt_bright);
		th->set_color("font_focus_color", btype, txt_bright);
		th->set_color("font_disabled_color", btype, txt_dis);
		th->set_color("icon_normal_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_hover_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_pressed_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_hover_pressed_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_focus_color", btype, Color(1, 1, 1, 1));
		th->set_color("icon_disabled_color", btype, Color(1, 1, 1, 0.5f));
		th->set_constant("h_separation", btype, m6);
		th->set_constant("hover_transition_ms", btype, BRASS_TRANSITION_MS);
	}

	// 3b. CheckBox & CheckButton
	Ref<ClockGearsBrassStyleBox> chk_normal;
	chk_normal.instantiate();
	chk_normal->set_anti_aliased(true);
	chk_normal->set_bg_color(Color(0, 0, 0, 0));
	chk_normal->set_corner_radius_all(r6);
	chk_normal->set_border_width_all(1);
	chk_normal->set_border_color(Color(1, 1, 1, 0.0f));
	chk_normal->set_content_margin_individual(m8, m4, m8, m4);

	Ref<ClockGearsBrassStyleBox> chk_hover = chk_normal->duplicate();
	chk_hover->set_bg_color(Color(accent.r, accent.g, accent.b, 0.10f));
	chk_hover->set_border_color(Color(accent.r, accent.g, accent.b, 0.60f));
	chk_hover->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.24f));
	chk_hover->set_shadow_size(int(3 * p_scale));

	Ref<ClockGearsBrassStyleBox> chk_pressed = chk_normal->duplicate();
	chk_pressed->set_bg_color(Color(accent.r, accent.g, accent.b, 0.18f));
	chk_pressed->set_border_color(accent);

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
		th->set_color("font_hover_color", ctype, txt_bright);
		th->set_color("font_pressed_color", ctype, accent);
		th->set_color("font_hover_pressed_color", ctype, accent);
		th->set_color("font_disabled_color", ctype, txt_dis);
		th->set_constant("h_separation", ctype, m8);
		th->set_constant("hover_transition_ms", ctype, BRASS_TRANSITION_MS);
	}

	// 3c. PopupMenu (menús desplegables con sombra suave, borde latón y selección ámbar)
	Ref<ClockGearsBrassStyleBox> pop_panel;
	pop_panel.instantiate();
	pop_panel->set_anti_aliased(true);
	pop_panel->set_bg_color(Color::html("#1b1713"));
	pop_panel->set_corner_radius_all(r6);
	pop_panel->set_border_width_all(1);
	pop_panel->set_border_color(Color(accent.r, accent.g, accent.b, 0.72f));
	pop_panel->set_content_margin_all(m6);
	pop_panel->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_PANEL);
	pop_panel->set_top_highlight_alpha(0.30f);
	pop_panel->set_shadow_color(Color(0.01f, 0.01f, 0.0f, 0.48f));
	pop_panel->set_shadow_size(int(6 * p_scale));
	pop_panel->set_shadow_offset(Vector2(0, 3 * p_scale));

	Ref<ClockGearsBrassStyleBox> pop_hover;
	pop_hover.instantiate();
	pop_hover->set_anti_aliased(true);
	pop_hover->set_bg_color(Color(accent.r, accent.g, accent.b, 0.24f));
	pop_hover->set_corner_radius_all(r6);
	pop_hover->set_border_width_all(1);
	pop_hover->set_border_color(accent);
	pop_hover->set_content_margin_individual(m8, m4, m8, m4);
	pop_hover->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.28f));
	pop_hover->set_shadow_size(int(3 * p_scale));

	th->set_stylebox("panel", "PopupMenu", pop_panel);
	th->set_stylebox("hover", "PopupMenu", pop_hover);
	th->set_color("font_color", "PopupMenu", txt);
	th->set_color("font_hover_color", "PopupMenu", txt_bright);
	th->set_color("font_disabled_color", "PopupMenu", txt_dis);
	th->set_color("font_accelerator_color", "PopupMenu", txt_sec);

	// 4. LineEdit, TextEdit & SpinBox (campos redondeados, hundimiento suave y resplandor al enfocar)
	Ref<ClockGearsBrassStyleBox> le_normal;
	le_normal.instantiate();
	le_normal->set_anti_aliased(true);
	le_normal->set_bg_color(input_col);
	le_normal->set_corner_radius_all(r6);
	le_normal->set_border_width_all(1);
	le_normal->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.45f));
	le_normal->set_content_margin_individual(m8, m6, m8, m6);
	le_normal->set_sunken_style(true);

	Ref<ClockGearsBrassStyleBox> le_focus = le_normal->duplicate();
	le_focus->set_border_color(accent);
	le_focus->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.36f));
	le_focus->set_shadow_size(int(4 * p_scale));

	Ref<ClockGearsBrassStyleBox> le_ro = le_normal->duplicate();
	le_ro->set_bg_color(Color::html("#110f0c"));
	le_ro->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.22f));

	th->set_stylebox("normal", "LineEdit", le_normal);
	th->set_stylebox("focus", "LineEdit", le_focus);
	th->set_stylebox("read_only", "LineEdit", le_ro);
	th->set_color("font_color", "LineEdit", txt);
	th->set_color("font_uneditable_color", "LineEdit", txt_sec);
	th->set_color("font_placeholder_color", "LineEdit", txt_sec);
	th->set_color("caret_color", "LineEdit", accent);
	th->set_color("selection_color", "LineEdit", Color(accent.r, accent.g, accent.b, 0.36f));

	th->set_stylebox("normal", "TextEdit", le_normal);
	th->set_stylebox("focus", "TextEdit", le_focus);
	th->set_stylebox("read_only", "TextEdit", le_ro);
	th->set_color("font_color", "TextEdit", txt);
	th->set_color("font_readonly_color", "TextEdit", txt_sec);
	th->set_color("caret_color", "TextEdit", accent);
	th->set_color("selection_color", "TextEdit", Color(accent.r, accent.g, accent.b, 0.36f));

	Ref<ClockGearsBrassStyleBox> sb_up_norm = btn_normal->duplicate();
	sb_up_norm->set_corner_radius_all(0);
	sb_up_norm->set_corner_radius(CORNER_TOP_RIGHT, r6);
	sb_up_norm->set_content_margin_all(m4);
	Ref<ClockGearsBrassStyleBox> sb_up_hov = sb_up_norm->duplicate();
	sb_up_hov->set_bg_color(btn_hover->get_bg_color());
	sb_up_hov->set_border_color(accent);
	Ref<ClockGearsBrassStyleBox> sb_up_prs = sb_up_norm->duplicate();
	sb_up_prs->set_bg_color(btn_pressed->get_bg_color());
	sb_up_prs->set_border_color(accent);
	sb_up_prs->set_sunken_style(true);
	Ref<ClockGearsBrassStyleBox> sb_up_dis = sb_up_norm->duplicate();
	sb_up_dis->set_bg_color(btn_disabled->get_bg_color());
	sb_up_dis->set_border_color(btn_disabled->get_border_color());

	Ref<ClockGearsBrassStyleBox> sb_dn_norm = btn_normal->duplicate();
	sb_dn_norm->set_corner_radius_all(0);
	sb_dn_norm->set_corner_radius(CORNER_BOTTOM_RIGHT, r6);
	sb_dn_norm->set_content_margin_all(m4);
	Ref<ClockGearsBrassStyleBox> sb_dn_hov = sb_dn_norm->duplicate();
	sb_dn_hov->set_bg_color(btn_hover->get_bg_color());
	sb_dn_hov->set_border_color(accent);
	Ref<ClockGearsBrassStyleBox> sb_dn_prs = sb_dn_norm->duplicate();
	sb_dn_prs->set_bg_color(btn_pressed->get_bg_color());
	sb_dn_prs->set_border_color(accent);
	sb_dn_prs->set_sunken_style(true);
	Ref<ClockGearsBrassStyleBox> sb_dn_dis = sb_dn_norm->duplicate();
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

	// 5. ItemList & Tree (selección con acento ámbar y resplandor en hover)
	Ref<ClockGearsBrassStyleBox> list_bg;
	list_bg.instantiate();
	list_bg->set_anti_aliased(true);
	list_bg->set_bg_color(Color::html("#15120f"));
	list_bg->set_corner_radius_all(r6);
	list_bg->set_border_width_all(1);
	list_bg->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.45f));
	list_bg->set_content_margin_all(m8);
	list_bg->set_sunken_style(true);

	Ref<ClockGearsBrassStyleBox> item_sel;
	item_sel.instantiate();
	item_sel->set_anti_aliased(true);
	item_sel->set_bg_color(Color(accent.r, accent.g, accent.b, 0.25f));
	item_sel->set_corner_radius_all(r6);
	item_sel->set_border_width_all(1);
	item_sel->set_border_color(accent);
	item_sel->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_BUTTON_HOVER);
	item_sel->set_top_highlight_alpha(0.32f);

	Ref<ClockGearsBrassStyleBox> item_hov;
	item_hov.instantiate();
	item_hov->set_anti_aliased(true);
	item_hov->set_bg_color(Color::html("#2a2219"));
	item_hov->set_corner_radius_all(r6);
	item_hov->set_border_width_all(1);
	item_hov->set_border_color(Color(accent.r, accent.g, accent.b, 0.55f));
	item_hov->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.22f));
	item_hov->set_shadow_size(int(3 * p_scale));

	Ref<ClockGearsBrassStyleBox> item_hov_sel = item_sel->duplicate();
	item_hov_sel->set_bg_color(Color(accent.r, accent.g, accent.b, 0.34f));

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
	th->set_color("font_hovered_color", "ItemList", txt_bright);
	th->set_color("font_selected_color", "ItemList", txt_bright);

	th->set_stylebox("panel", "Tree", list_bg);
	th->set_stylebox("focus", "Tree", btn_focus);
	th->set_stylebox("hovered", "Tree", item_hov);
	th->set_stylebox("selected", "Tree", item_sel);
	th->set_stylebox("selected_focus", "Tree", item_sel);
	th->set_color("font_color", "Tree", txt);
	th->set_color("font_selected_color", "Tree", txt_bright);
	th->set_color("guide_color", "Tree", Color(accent.r, accent.g, accent.b, 0.12f));

	// 6. TabContainer, TabBar & ClockGearsTabButton (degradado en pestañas, borde ámbar y transiciones <= 150 ms)
	Ref<ClockGearsBrassStyleBox> tab_sel;
	tab_sel.instantiate();
	tab_sel->set_anti_aliased(true);
	tab_sel->set_bg_color(Color::html("#2b2116"));
	tab_sel->set_corner_radius(CORNER_TOP_LEFT, r6);
	tab_sel->set_corner_radius(CORNER_TOP_RIGHT, r6);
	tab_sel->set_border_width_all(1);
	tab_sel->set_border_color(accent);
	tab_sel->set_content_margin_individual(m12, m8, m12, m8);
	tab_sel->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_HEADER);
	tab_sel->set_top_highlight_alpha(0.38f);
	tab_sel->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.26f));
	tab_sel->set_shadow_size(int(4 * p_scale));

	Ref<ClockGearsBrassStyleBox> tab_unsel;
	tab_unsel.instantiate();
	tab_unsel->set_anti_aliased(true);
	tab_unsel->set_bg_color(Color::html("#181511"));
	tab_unsel->set_corner_radius(CORNER_TOP_LEFT, r6);
	tab_unsel->set_corner_radius(CORNER_TOP_RIGHT, r6);
	tab_unsel->set_border_width_all(1);
	tab_unsel->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.35f));
	tab_unsel->set_content_margin_individual(m12, m8, m12, m8);
	tab_unsel->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_PANEL);

	Ref<ClockGearsBrassStyleBox> tab_hov = tab_unsel->duplicate();
	tab_hov->set_bg_color(Color::html("#282118"));
	tab_hov->set_border_color(Color(accent.r, accent.g, accent.b, 0.72f));
	tab_hov->set_gradient_mode(ClockGearsBrassStyleBox::GRADIENT_BUTTON_HOVER);
	tab_hov->set_top_highlight_alpha(0.30f);
	tab_hov->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.24f));
	tab_hov->set_shadow_size(int(3 * p_scale));

	Ref<ClockGearsBrassStyleBox> tab_hov_sel = tab_sel->duplicate();
	tab_hov_sel->set_bg_color(Color::html("#33271a"));

	Ref<ClockGearsBrassStyleBox> tab_dis = tab_unsel->duplicate();
	tab_dis->set_bg_color(Color::html("#13110e"));
	tab_dis->set_border_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.16f));

	Ref<ClockGearsBrassStyleBox> tab_foc = tab_unsel->duplicate();
	tab_foc->set_draw_center(false);
	tab_foc->set_border_color(accent);
	tab_foc->set_shadow_color(Color(accent.r, accent.g, accent.b, 0.36f));
	tab_foc->set_shadow_size(int(4 * p_scale));

	th->set_stylebox("tab_selected", "TabBar", tab_sel);
	th->set_stylebox("tab_unselected", "TabBar", tab_unsel);
	th->set_stylebox("tab_hovered", "TabBar", tab_hov);
	th->set_stylebox("tab_disabled", "TabBar", tab_dis);
	th->set_stylebox("tab_focus", "TabBar", tab_foc);
	th->set_color("font_selected_color", "TabBar", accent);
	th->set_color("font_hovered_color", "TabBar", txt_bright);
	th->set_color("font_unselected_color", "TabBar", txt_sec);
	th->set_color("font_disabled_color", "TabBar", txt_dis);
	th->set_constant("tab_transition_ms", "TabBar", BRASS_TRANSITION_MS);

	th->set_stylebox("tab_selected", "TabContainer", tab_sel);
	th->set_stylebox("tab_unselected", "TabContainer", tab_unsel);
	th->set_stylebox("tab_hovered", "TabContainer", tab_hov);
	th->set_stylebox("tab_disabled", "TabContainer", tab_dis);
	th->set_stylebox("tab_focus", "TabContainer", tab_foc);
	th->set_stylebox("panel", "TabContainer", sb_panel);
	th->set_color("font_selected_color", "TabContainer", accent);
	th->set_color("font_hovered_color", "TabContainer", txt_bright);
	th->set_color("font_unselected_color", "TabContainer", txt_sec);
	th->set_color("font_disabled_color", "TabContainer", txt_dis);
	th->set_constant("tab_transition_ms", "TabContainer", BRASS_TRANSITION_MS);

	th->set_type_variation("ClockGearsTabButton", "Button");
	th->set_stylebox("normal", "ClockGearsTabButton", tab_unsel);
	th->set_stylebox("hover", "ClockGearsTabButton", tab_hov);
	th->set_stylebox("pressed", "ClockGearsTabButton", tab_sel);
	th->set_stylebox("hover_pressed", "ClockGearsTabButton", tab_hov_sel);
	th->set_stylebox("focus", "ClockGearsTabButton", tab_foc);
	th->set_stylebox("disabled", "ClockGearsTabButton", tab_dis);
	th->set_color("font_color", "ClockGearsTabButton", txt_sec);
	th->set_color("font_hover_color", "ClockGearsTabButton", txt_bright);
	th->set_color("font_pressed_color", "ClockGearsTabButton", accent);
	th->set_color("font_hover_pressed_color", "ClockGearsTabButton", txt_bright);
	th->set_color("font_focus_color", "ClockGearsTabButton", txt_bright);
	th->set_color("font_disabled_color", "ClockGearsTabButton", txt_dis);
	th->set_color("icon_normal_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_hover_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_pressed_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_hover_pressed_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_focus_color", "ClockGearsTabButton", Color(1, 1, 1, 1));
	th->set_color("icon_disabled_color", "ClockGearsTabButton", Color(1, 1, 1, 0.5f));
	th->set_constant("h_separation", "ClockGearsTabButton", m8);
	th->set_constant("hover_transition_ms", "ClockGearsTabButton", BRASS_TRANSITION_MS);

	// 7. Labels & Separators (separadores finos con detalle sutil de engranaje)
	th->set_color("font_color", "Label", txt);
	th->set_color("default_color", "RichTextLabel", txt);

	Ref<ClockGearsBrassStyleBox> sep_gear;
	sep_gear.instantiate();
	sep_gear->set_gear_separator_style(true);
	sep_gear->set_content_margin_individual(0, 2, 0, 2);
	th->set_stylebox("separator", "HSeparator", sep_gear);

	Ref<StyleBoxLine> pop_sep_line;
	pop_sep_line.instantiate();
	pop_sep_line->set_color(Color(border_brass.r, border_brass.g, border_brass.b, 0.34f));
	pop_sep_line->set_thickness(1);
	th->set_stylebox("separator", "PopupMenu", pop_sep_line);

	Ref<StyleBoxLine> vsep_line = pop_sep_line->duplicate();
	vsep_line->set_vertical(true);
	th->set_stylebox("separator", "VSeparator", vsep_line);

	// 8. Espaciado estándar (con aire, manteniendo las métricas exactas 16:9)
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

Ref<ImageTexture> ClockGearsTheme::get_cached_brass_gradient_texture(int p_mode) const {
	int idx = CLAMP(p_mode, 0, 4);
	if (idx == 0) {
		return Ref<ImageTexture>();
	}
	if (cached_gradient_textures[idx].is_valid()) {
		return cached_gradient_textures[idx];
	}
	Ref<Image> img = Image::create_empty(2, 32, false, Image::FORMAT_RGBA8);
	Color top_c(0.95f, 0.72f, 0.30f, 0.14f);
	Color mid_c(0.60f, 0.42f, 0.18f, 0.04f);
	Color bot_c(0.03f, 0.02f, 0.01f, 0.18f);
	if (idx == ClockGearsBrassStyleBox::GRADIENT_HEADER) {
		top_c = Color(0.98f, 0.76f, 0.34f, 0.22f);
		mid_c = Color(0.65f, 0.45f, 0.18f, 0.06f);
		bot_c = Color(0.05f, 0.03f, 0.01f, 0.24f);
	} else if (idx == ClockGearsBrassStyleBox::GRADIENT_BUTTON) {
		top_c = Color(0.95f, 0.72f, 0.30f, 0.17f);
		mid_c = Color(0.55f, 0.38f, 0.15f, 0.04f);
		bot_c = Color(0.04f, 0.02f, 0.01f, 0.22f);
	} else if (idx == ClockGearsBrassStyleBox::GRADIENT_BUTTON_HOVER) {
		top_c = Color(1.0f, 0.80f, 0.38f, 0.26f);
		mid_c = Color(0.75f, 0.52f, 0.20f, 0.09f);
		bot_c = Color(0.06f, 0.04f, 0.01f, 0.18f);
	}
	for (int y = 0; y < 32; y++) {
		float t = float(y) / 31.0f;
		Color c = (t < 0.5f) ? top_c.lerp(mid_c, t * 2.0f) : mid_c.lerp(bot_c, (t - 0.5f) * 2.0f);
		img->set_pixel(0, y, c);
		img->set_pixel(1, y, c);
	}
	cached_gradient_textures[idx] = ImageTexture::create_from_image(img);
	gradient_generation_count++;
	return cached_gradient_textures[idx];
}

Ref<ImageTexture> ClockGearsTheme::get_cached_brass_gear_texture() const {
	if (cached_gear_texture.is_valid()) {
		return cached_gear_texture;
	}
	Ref<Image> img = Image::create_empty(11, 11, false, Image::FORMAT_RGBA8);
	img->fill(Color(0, 0, 0, 0));
	const Color ring_col(0.95f, 0.73f, 0.33f, 0.62f);
	const Color tooth_col(0.92f, 0.68f, 0.28f, 0.52f);
	const Color hi_col(1.0f, 0.86f, 0.50f, 0.75f);
	for (int y = 0; y < 11; y++) {
		for (int x = 0; x < 11; x++) {
			int dx = x - 5;
			int dy = y - 5;
			int d2 = dx * dx + dy * dy;
			if (d2 >= 5 && d2 <= 13) {
				img->set_pixel(x, y, (dx <= 0 && dy <= 0) ? hi_col : ring_col);
			} else if (d2 >= 14 && d2 <= 25 && (dx == 0 || dy == 0 || Math::abs(dx) == Math::abs(dy))) {
				img->set_pixel(x, y, tooth_col);
			}
		}
	}
	cached_gear_texture = ImageTexture::create_from_image(img);
	gear_generation_count++;
	return cached_gear_texture;
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

Ref<Theme> ClockGearsTheme::get_brass_theme() {
	if (simulate_brass_load_failure) {
		return Ref<Theme>();
	}
	bool editor_theme_available = false;
#ifdef TOOLS_ENABLED
	editor_theme_available = (EditorNode::get_singleton() && EditorNode::get_singleton()->get_editor_theme().is_valid());
#endif
	if (brass_theme.is_null() || (editor_theme_available && !brass_has_editor_base)) {
		brass_theme = _build_brass_theme(CG_SCALE, &brass_has_editor_base);
	}
	return brass_theme;
}

Ref<Theme> ClockGearsTheme::get_theme_by_id(const String &p_theme_id) {
	String id = p_theme_id.strip_edges().to_lower();
	if (id == "brass" || id == "laton" || id == String::utf8("latón")) {
		Ref<Theme> b = get_brass_theme();
		if (b.is_valid()) {
			return b;
		}
		return get_theme(false);
	}
	if (id == "light" || id == "claro") {
		return get_theme(true);
	}
	return get_theme(false);
}

Ref<Theme> ClockGearsTheme::get_active_theme() {
	return get_theme_by_id(active_theme_id);
}

PackedStringArray ClockGearsTheme::get_available_theme_ids() const {
	PackedStringArray arr;
	arr.push_back("dark");
	arr.push_back("light");
	arr.push_back("brass");
	return arr;
}

String ClockGearsTheme::get_theme_display_name(const String &p_theme_id) const {
	String id = p_theme_id.strip_edges().to_lower();
	if (id == "brass" || id == "laton" || id == String::utf8("latón")) {
		return String::utf8("ClockGears Latón");
	}
	if (id == "light" || id == "claro") {
		return String::utf8("ClockGears Claro");
	}
	return String::utf8("ClockGears Oscuro (Por defecto)");
}

void ClockGearsTheme::set_simulate_brass_load_failure(bool p_fail) {
	simulate_brass_load_failure = p_fail;
	if (p_fail) {
		brass_theme.unref();
		brass_has_editor_base = false;
	}
}

bool ClockGearsTheme::set_active_theme_id(const String &p_theme_id) {
	String id = p_theme_id.strip_edges().to_lower();
	String prev_valid = (active_theme_id == "light" || active_theme_id == "brass") ? active_theme_id : String("dark");
	String fallback_non_brass = use_light_mode ? String("light") : String("dark");
	if (id == "laton" || id == String::utf8("latón")) {
		id = "brass";
	} else if (id == "claro") {
		id = "light";
	} else if (id == "oscuro") {
		id = "dark";
	}

	if (id != "dark" && id != "light" && id != "brass") {
		// Tema desconocido o inválido -> volver al tema actual por defecto sin error
		active_theme_id = prev_valid;
		use_light_mode = (active_theme_id == "light");
		refresh_all_themed_roots();
		emit_signal("theme_changed", active_theme_id);
		return false;
	}

	if (id == "brass") {
		Ref<Theme> b = get_brass_theme();
		if (b.is_null()) {
			// Si el tema nuevo falla al cargar, vuelve al actual sin error
			active_theme_id = (prev_valid == "brass") ? fallback_non_brass : prev_valid;
			use_light_mode = (active_theme_id == "light");
			refresh_all_themed_roots();
			emit_signal("theme_changed", active_theme_id);
			return false;
		}
	}

	active_theme_id = id;
	use_light_mode = (active_theme_id == "light");
	if (ProjectSettings::get_singleton()) {
		ProjectSettings::get_singleton()->set_setting("clockgears/editor/theme_id", active_theme_id);
	}
	refresh_all_themed_roots();
	emit_signal("theme_changed", active_theme_id);
	return true;
}

void ClockGearsTheme::sync_from_project_settings() {
	if (!ProjectSettings::get_singleton()) {
		return;
	}
	String saved_id = "dark";
	if (ProjectSettings::get_singleton()->has_setting("clockgears/editor/theme_id")) {
		saved_id = String(ProjectSettings::get_singleton()->get_setting("clockgears/editor/theme_id")).strip_edges().to_lower();
		if (saved_id.is_empty()) {
			saved_id = "dark";
		}
	}
	if (saved_id != active_theme_id) {
		set_active_theme_id(saved_id);
	}
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
	active_theme_id = p_light ? "light" : "dark";
	refresh_all_themed_roots();
}

void ClockGearsTheme::register_themed_root(Node *p_root) {
	if (!p_root) {
		return;
	}
	ObjectID oid = p_root->get_instance_id();
	if (!registered_roots.has(oid)) {
		registered_roots.push_back(oid);
	}
}

static bool _cg_is_in_game_preview_node(Node *p_node) {
	if (!p_node) {
		return false;
	}
	String cname = p_node->get_class();
	if (cname == "ClockGearsTitleScreen" || cname == "ClockGearsGameMenu" || cname == "ClockGearsCreditsPlayer" ||
			cname == "ClockGearsHudOverlay" || cname == "ClockGearsUiDesignLayer" || cname == "ClockGearsWindowStyleBox" ||
			cname == "ClockGearsNineSlicePreview" || cname == "ClockGearsUiCanvasOverlay") {
		return true;
	}
	return false;
}

static Ref<ClockGearsBrassStyleBox> _cg_clone_flat_to_brass(const Ref<StyleBoxFlat> &p_src, const Color &p_bg, const Color &p_border, int p_grad_mode, float p_hi_alpha, bool p_sunken = false, int p_shadow_sz = 0, const Color &p_shadow_col = Color(0, 0, 0, 0)) {
	Ref<ClockGearsBrassStyleBox> b;
	b.instantiate();
	b->set_anti_aliased(true);
	b->set_bg_color(p_bg);
	b->set_border_color(p_border);
	if (p_src.is_valid()) {
		b->set_draw_center(p_src->is_draw_center_enabled());
		for (int s = 0; s < 4; s++) {
			Side side = (Side)s;
			b->set_border_width(side, MAX(1, p_src->get_border_width(side)));
			b->set_content_margin(side, p_src->get_content_margin(side));
		}
		for (int c = 0; c < 4; c++) {
			Corner corner = (Corner)c;
			b->set_corner_radius(corner, p_src->get_corner_radius(corner));
		}
	} else {
		b->set_border_width_all(1);
		b->set_corner_radius_all(6);
	}
	b->set_gradient_mode(p_grad_mode);
	b->set_top_highlight_alpha(p_hi_alpha);
	b->set_sunken_style(p_sunken);
	if (p_shadow_sz > 0) {
		b->set_shadow_size(p_shadow_sz);
		b->set_shadow_color(p_shadow_col);
	}
	return b;
}

void ClockGearsTheme::_on_brass_control_hover_entered(ObjectID p_id) {
	if (active_theme_id != "brass") {
		return;
	}
	Control *c = Object::cast_to<Control>(ObjectDB::get_instance(p_id));
	if (!c || !c->is_inside_tree() || !c->is_visible_in_tree()) {
		return;
	}
	const float dur = float(BRASS_TRANSITION_MS) / 1000.0f;
	c->set_self_modulate(Color(1.08f, 1.05f, 0.96f, 1.0f));
	Ref<Tween> tw = c->create_tween();
	if (tw.is_valid()) {
		tw->set_trans(Tween::TRANS_QUAD);
		tw->set_ease(Tween::EASE_OUT);
		tw->tween_property(c, NodePath("self_modulate"), Color(1.0f, 1.0f, 1.0f, 1.0f), dur);
	}
}

void ClockGearsTheme::_on_brass_tab_changed(int p_tab, ObjectID p_id) {
	if (active_theme_id != "brass") {
		return;
	}
	Control *c = Object::cast_to<Control>(ObjectDB::get_instance(p_id));
	if (!c || !c->is_inside_tree() || !c->is_visible_in_tree()) {
		return;
	}
	Control *target = c;
	if (TabContainer *tc = Object::cast_to<TabContainer>(c)) {
		if (Control *tab_c = tc->get_current_tab_control()) {
			target = tab_c;
		}
	}
	const float dur = float(BRASS_TRANSITION_MS) / 1000.0f;
	target->set_self_modulate(Color(1.06f, 1.03f, 0.94f, 1.0f));
	Ref<Tween> tw = target->create_tween();
	if (tw.is_valid()) {
		tw->set_trans(Tween::TRANS_QUAD);
		tw->set_ease(Tween::EASE_OUT);
		tw->tween_property(target, NodePath("self_modulate"), Color(1.0f, 1.0f, 1.0f, 1.0f), dur);
	}
}

void ClockGearsTheme::apply_theme_to_node_tree(Node *p_root) {
	if (!p_root) {
		return;
	}
	register_themed_root(p_root);

	const bool is_brass = (active_theme_id == "brass");
	Ref<Theme> target_theme = get_active_theme();

	// 1. Aplicar o restaurar el Theme del nodo raíz (Window o Control)
	if (Window *w = Object::cast_to<Window>(p_root)) {
		if (is_brass) {
			if (!w->has_meta("_cg_has_pre_brass_theme")) {
				w->set_meta("_cg_pre_brass_theme", w->get_theme());
				w->set_meta("_cg_has_pre_brass_theme", true);
			}
			w->set_theme(target_theme);
		} else {
			if (w->has_meta("_cg_has_pre_brass_theme")) {
				Ref<Theme> orig_th = w->get_meta("_cg_pre_brass_theme");
				w->remove_meta("_cg_has_pre_brass_theme");
				w->remove_meta("_cg_pre_brass_theme");
				w->set_theme(orig_th.is_valid() ? target_theme : Ref<Theme>());
			} else {
				w->set_theme(target_theme);
			}
		}
	} else if (Control *rc = Object::cast_to<Control>(p_root)) {
		if (is_brass) {
			if (!rc->has_meta("_cg_has_pre_brass_theme")) {
				rc->set_meta("_cg_pre_brass_theme", rc->get_theme());
				rc->set_meta("_cg_has_pre_brass_theme", true);
			}
			rc->set_theme(target_theme);
		} else {
			if (rc->has_meta("_cg_has_pre_brass_theme")) {
				Ref<Theme> orig_th = rc->get_meta("_cg_pre_brass_theme");
				rc->remove_meta("_cg_has_pre_brass_theme");
				rc->remove_meta("_cg_pre_brass_theme");
				rc->set_theme(orig_th.is_valid() ? target_theme : Ref<Theme>());
			} else if (rc->get_theme().is_valid()) {
				rc->set_theme(target_theme);
			}
		}
	}

	// Si el nodo aún no fue cambiado a Latón y estamos en el tema por defecto ("dark" / "light"),
	// no hace falta recorrer todos sus hijos salvo que ya tuviera overrides de Latón aplicados antes.
	if (!is_brass && !p_root->has_meta("_cg_brass_tree_applied")) {
		return;
	}
	if (is_brass) {
		p_root->set_meta("_cg_brass_tree_applied", true);
	} else {
		p_root->remove_meta("_cg_brass_tree_applied");
	}

	Vector<Node *> stack;
	stack.push_back(p_root);

	while (!stack.is_empty()) {
		Node *n = stack[stack.size() - 1];
		stack.remove_at(stack.size() - 1);

		if (_cg_is_in_game_preview_node(n)) {
			continue;
		}

		for (int i = 0; i < n->get_child_count(); i++) {
			stack.push_back(n->get_child(i));
		}

		if (n != p_root) {
			if (Window *cw = Object::cast_to<Window>(n)) {
				if (is_brass) {
					if (!cw->has_meta("_cg_has_pre_brass_theme")) {
						cw->set_meta("_cg_pre_brass_theme", cw->get_theme());
						cw->set_meta("_cg_has_pre_brass_theme", true);
					}
					cw->set_theme(target_theme);
				} else if (cw->has_meta("_cg_has_pre_brass_theme")) {
					Ref<Theme> orig_th = cw->get_meta("_cg_pre_brass_theme");
					cw->remove_meta("_cg_has_pre_brass_theme");
					cw->remove_meta("_cg_pre_brass_theme");
					cw->set_theme(orig_th.is_valid() ? target_theme : Ref<Theme>());
				}
			} else if (MenuBar *mb = Object::cast_to<MenuBar>(n)) {
				if (is_brass) {
					if (!mb->has_meta("_cg_has_pre_brass_theme")) {
						mb->set_meta("_cg_pre_brass_theme", mb->get_theme());
						mb->set_meta("_cg_has_pre_brass_theme", true);
					}
					mb->set_theme(target_theme);
				} else if (mb->has_meta("_cg_has_pre_brass_theme")) {
					Ref<Theme> orig_th = mb->get_meta("_cg_pre_brass_theme");
					mb->remove_meta("_cg_has_pre_brass_theme");
					mb->remove_meta("_cg_pre_brass_theme");
					mb->set_theme(orig_th.is_valid() ? orig_th : Ref<Theme>());
				}
#ifdef TOOLS_ENABLED
				if (mb->get_theme_type_variation() == StringName("MainMenuBar") || mb->get_name() == "MainMenuBar") {
					EditorNode::style_clockgears_main_menu_bar(mb);
				}
#endif
			}
		}

		Control *c = Object::cast_to<Control>(n);
		if (!c) {
			continue;
		}

		if (is_brass) {
			// Conectar transiciones cortas (120 ms <= 150 ms) por evento en botones y pestañas
			if (Object::cast_to<Button>(c) && !c->has_meta("_cg_brass_hov_conn")) {
				c->set_meta("_cg_brass_hov_conn", true);
				c->connect(SceneStringName(mouse_entered), callable_mp(this, &ClockGearsTheme::_on_brass_control_hover_entered).bind(c->get_instance_id()));
			}
			if ((Object::cast_to<TabContainer>(c) || Object::cast_to<TabBar>(c)) && !c->has_meta("_cg_brass_tab_conn")) {
				c->set_meta("_cg_brass_tab_conn", true);
				c->connect("tab_changed", callable_mp(this, &ClockGearsTheme::_on_brass_tab_changed).bind(c->get_instance_id()));
			}

			// Adaptar overrides locales de tarjetas/cabeceras oscuras a ClockGearsBrassStyleBox
			if (Object::cast_to<PanelContainer>(c) || Object::cast_to<Panel>(c)) {
				if (c->has_theme_stylebox_override(SceneStringName(panel))) {
					Ref<StyleBox> cur_sb = c->get_theme_stylebox(SceneStringName(panel));
					if (Object::cast_to<ClockGearsBrassStyleBox>(cur_sb.ptr()) == nullptr) {
						Ref<StyleBoxFlat> sbf = cur_sb;
						if (sbf.is_valid()) {
							Color bgc = sbf->get_bg_color();
							bool is_neutral_dark = (bgc.a > 0.5f && bgc.r < 0.26f && bgc.g < 0.27f && bgc.b < 0.34f && Math::abs(bgc.r - bgc.b) < 0.14f);
							if (is_neutral_dark) {
								if (!c->has_meta("_cg_orig_sb_panel")) {
									c->set_meta("_cg_orig_sb_panel", cur_sb);
								}
								bool is_bar_or_hdr = (c->get_class() == "ClockGearsTopBar" || (c->get_size().y > 0 && c->get_size().y <= 72));
								Color new_bg = (bgc.get_v() < 0.13f) ? color_brass_bg() : (is_bar_or_hdr ? color_brass_header() : color_brass_panel());
								Color new_bd = Color(color_brass_border().r, color_brass_border().g, color_brass_border().b, is_bar_or_hdr ? 0.68f : 0.55f);
								int gmode = is_bar_or_hdr ? ClockGearsBrassStyleBox::GRADIENT_HEADER : ClockGearsBrassStyleBox::GRADIENT_PANEL;
								Ref<ClockGearsBrassStyleBox> bsb = _cg_clone_flat_to_brass(sbf, new_bg, new_bd, gmode, 0.28f, false, 3, Color(0.02f, 0.01f, 0.0f, 0.34f));
								c->add_theme_style_override(SceneStringName(panel), bsb);
							}
						}
					}
				}
			}

			if (Object::cast_to<TabContainer>(c) || Object::cast_to<TabBar>(c)) {
				const char *tab_sbs[4] = { "tab_selected", "tab_hovered", "tab_unselected", "panel" };
				for (int ti = 0; ti < 4; ti++) {
					StringName sname(tab_sbs[ti]);
					if (c->has_theme_stylebox_override(sname)) {
						Ref<StyleBox> cur_sb = c->get_theme_stylebox(sname);
						if (Object::cast_to<ClockGearsBrassStyleBox>(cur_sb.ptr()) == nullptr) {
							Ref<StyleBoxFlat> sbf = cur_sb;
							if (sbf.is_valid()) {
								String mkey = String("_cg_orig_sb_") + String(tab_sbs[ti]);
								if (!c->has_meta(mkey)) {
									c->set_meta(mkey, cur_sb);
								}
								Color new_bg = (ti == 0 || ti == 1) ? Color::html("#2b2116") : (ti == 2 ? Color::html("#181511") : color_brass_panel());
								Color new_bd = (ti == 0 || ti == 1) ? color_brass_accent() : Color(color_brass_border().r, color_brass_border().g, color_brass_border().b, 0.45f);
								int gmode = (ti <= 1) ? ClockGearsBrassStyleBox::GRADIENT_HEADER : ClockGearsBrassStyleBox::GRADIENT_PANEL;
								Ref<ClockGearsBrassStyleBox> bsb = _cg_clone_flat_to_brass(sbf, new_bg, new_bd, gmode, 0.32f, false, (ti == 0) ? 3 : 0, Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.24f));
								c->add_theme_style_override(sname, bsb);
							}
						}
					}
				}
			}

			if (Object::cast_to<Button>(c) && c->get_name() == "ClockGearsActiveLayerIndicator") {
				const char *btn_sbs[3] = { "normal", "hover", "pressed" };
				for (int bi = 0; bi < 3; bi++) {
					StringName sname(btn_sbs[bi]);
					if (c->has_theme_stylebox_override(sname)) {
						Ref<StyleBox> cur_sb = c->get_theme_stylebox(sname);
						if (Object::cast_to<ClockGearsBrassStyleBox>(cur_sb.ptr()) == nullptr) {
							Ref<StyleBoxFlat> sbf = cur_sb;
							if (sbf.is_valid()) {
								String mkey = String("_cg_orig_sb_") + String(btn_sbs[bi]);
								if (!c->has_meta(mkey)) {
									c->set_meta(mkey, cur_sb);
								}
								Color new_bg = (bi == 0) ? color_brass_header() : (bi == 1 ? Color::html("#332a1e") : Color::html("#1d1610"));
								Color new_bd = (bi == 0) ? Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.72f) : color_brass_accent();
								int gmode = (bi == 1) ? ClockGearsBrassStyleBox::GRADIENT_BUTTON_HOVER : ClockGearsBrassStyleBox::GRADIENT_HEADER;
								Ref<ClockGearsBrassStyleBox> bsb = _cg_clone_flat_to_brass(sbf, new_bg, new_bd, gmode, 0.30f, (bi == 2), (bi == 1) ? 3 : 0, Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.28f));
								c->add_theme_style_override(sname, bsb);
							}
						}
					}
				}
			} else if (Object::cast_to<Button>(c) && !Object::cast_to<CheckBox>(c) && !Object::cast_to<CheckButton>(c)) {
				const bool in_topbar = (c->get_parent() && c->get_parent()->get_parent() && c->get_parent()->get_parent()->get_class() == "ClockGearsTopBar");
				const char *btn_sbs[4] = { "normal", "hover", "pressed", "hover_pressed" };
				for (int bi = 0; bi < 4; bi++) {
					StringName sname(btn_sbs[bi]);
					if (c->has_theme_stylebox_override(sname)) {
						Ref<StyleBox> cur_sb = c->get_theme_stylebox(sname);
						if (Object::cast_to<ClockGearsBrassStyleBox>(cur_sb.ptr()) == nullptr) {
							Ref<StyleBoxFlat> sbf = cur_sb;
							if (sbf.is_valid()) {
								Color bgc = sbf->get_bg_color();
								const bool is_semantic_red = (bgc.r > 0.32f && bgc.g < 0.22f && bgc.a > 0.25f);
								const bool is_semantic_green = (bgc.g > 0.42f && bgc.r < 0.32f && bgc.a > 0.25f);
								const bool is_neutral = (!is_semantic_red && !is_semantic_green) && ((bgc.a < 0.25f) || (bgc.r < 0.28f && bgc.g < 0.28f && bgc.b < 0.36f && Math::abs(bgc.r - bgc.b) < 0.14f) || (in_topbar && (bi >= 2)));
								if (is_neutral) {
									String mkey = String("_cg_orig_sb_") + String(btn_sbs[bi]);
									if (!c->has_meta(mkey)) {
										c->set_meta(mkey, cur_sb);
									}
									Color new_bg = bgc;
									Color new_bd = color_brass_border();
									int gmode = ClockGearsBrassStyleBox::GRADIENT_NONE;
									float hi = 0.0f;
									bool sunken = (bi == 2 || bi == 3);
									int sh_sz = 0;
									Color sh_col(0, 0, 0, 0);
									if (bi == 0) {
										new_bg = (bgc.a < 0.05f) ? Color(0, 0, 0, 0) : Color::html("#262018");
										new_bd = (bgc.a < 0.05f && sbf->get_border_color().a < 0.05f) ? Color(0, 0, 0, 0) : Color(color_brass_border().r, color_brass_border().g, color_brass_border().b, 0.55f);
										gmode = (bgc.a >= 0.05f) ? ClockGearsBrassStyleBox::GRADIENT_BUTTON : ClockGearsBrassStyleBox::GRADIENT_NONE;
										hi = (bgc.a >= 0.05f) ? 0.24f : 0.0f;
									} else if (bi == 1) {
										new_bg = in_topbar ? Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.18f) : Color::html("#332a1e");
										new_bd = color_brass_accent();
										gmode = ClockGearsBrassStyleBox::GRADIENT_BUTTON_HOVER;
										hi = 0.34f;
										sh_sz = 3;
										sh_col = Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.28f);
									} else if (bi == 2) {
										new_bg = in_topbar ? Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.30f) : Color::html("#1d1610");
										new_bd = color_brass_accent();
									} else {
										new_bg = in_topbar ? Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.40f) : Color::html("#261d14");
										new_bd = color_brass_accent();
										sh_sz = 3;
										sh_col = Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.26f);
									}
									Ref<ClockGearsBrassStyleBox> bsb = _cg_clone_flat_to_brass(sbf, new_bg, new_bd, gmode, hi, sunken, sh_sz, sh_col);
									if (sunken) {
										bsb->set_content_margin(SIDE_TOP, sbf->get_content_margin(SIDE_TOP) + 1.0f);
										bsb->set_content_margin(SIDE_BOTTOM, MAX(0.0f, sbf->get_content_margin(SIDE_BOTTOM) - 1.0f));
									}
									c->add_theme_style_override(sname, bsb);
								}
							}
						}
					}
				}
			}

			// Adaptar inputs con overrides locales (ej. ClockGearsFormulaLineEdit)
			if (Object::cast_to<LineEdit>(c) || Object::cast_to<TextEdit>(c)) {
				const char *in_sbs[2] = { "normal", "focus" };
				for (int ii = 0; ii < 2; ii++) {
					StringName sname(in_sbs[ii]);
					if (c->has_theme_stylebox_override(sname)) {
						Ref<StyleBox> cur_sb = c->get_theme_stylebox(sname);
						if (Object::cast_to<ClockGearsBrassStyleBox>(cur_sb.ptr()) == nullptr) {
							Ref<StyleBoxFlat> sbf = cur_sb;
							if (sbf.is_valid()) {
								Color bgc = sbf->get_bg_color();
								const bool is_err_bg = (bgc.r > 0.20f && bgc.g < 0.16f && bgc.b < 0.16f && bgc.a > 0.5f);
								const bool is_dark_in = (bgc.a > 0.5f && bgc.r < 0.24f && bgc.g < 0.24f && bgc.b < 0.30f);
								if (is_err_bg || is_dark_in) {
									String mkey = String("_cg_orig_sb_") + String(in_sbs[ii]);
									if (!c->has_meta(mkey)) {
										c->set_meta(mkey, cur_sb);
									}
									Color new_bg = is_err_bg ? Color(0.19f, 0.07f, 0.07f, 1.0f) : color_brass_input();
									Color new_bd = is_err_bg ? Color::html("#ff8278") : ((ii == 1) ? color_brass_accent() : Color(color_brass_border().r, color_brass_border().g, color_brass_border().b, 0.45f));
									Ref<ClockGearsBrassStyleBox> bsb = _cg_clone_flat_to_brass(sbf, new_bg, new_bd, ClockGearsBrassStyleBox::GRADIENT_NONE, 0.0f, true, (ii == 1) ? 4 : 0, (ii == 1) ? Color(color_brass_accent().r, color_brass_accent().g, color_brass_accent().b, 0.36f) : Color(0, 0, 0, 0));
									c->add_theme_style_override(sname, bsb);
								}
							}
						}
					}
				}
			}

			// Asegurar contraste >= 4.5:1 en cualquier override local de font_color contra su fondo real
			if (c->has_theme_color_override(SceneStringName(font_color))) {
				Color fc = c->get_theme_color(SceneStringName(font_color));
				Color bg_for_fc = color_brass_panel();
				if (Object::cast_to<LineEdit>(c) || Object::cast_to<TextEdit>(c) || Object::cast_to<Button>(c)) {
					Ref<StyleBoxFlat> sbf = c->get_theme_stylebox(StringName("normal"));
					if (sbf.is_valid() && sbf->get_bg_color().a > 0.5f) {
						bg_for_fc = sbf->get_bg_color();
					}
				}
				if (compute_contrast_ratio(fc, bg_for_fc) < 4.5f) {
					if (!c->has_meta("_cg_orig_col_font_color")) {
						c->set_meta("_cg_orig_col_font_color", fc);
					}
					Color replacement_fc = (fc.r > fc.g * 1.4f && fc.r > 0.45f) ? Color::html("#ff9e96") : color_brass_text_primary();
					c->add_theme_color_override(SceneStringName(font_color), replacement_fc);
				}
			}
		} else {
			// Restaurar estado exacto previo a Latón ("dark" / "light" 100% idéntico a antes)
			c->set_self_modulate(Color(1, 1, 1, 1));
			const char *restore_sbs[11] = { "panel", "tab_selected", "tab_hovered", "tab_unselected", "normal", "hover", "pressed", "hover_pressed", "focus", "separator", nullptr };
			for (int ri = 0; restore_sbs[ri] != nullptr; ri++) {
				String mkey = String("_cg_orig_sb_") + String(restore_sbs[ri]);
				if (c->has_meta(mkey)) {
					Ref<StyleBox> orig_sb = c->get_meta(mkey);
					c->remove_meta(mkey);
					if (orig_sb.is_valid()) {
						c->add_theme_style_override(StringName(restore_sbs[ri]), orig_sb);
					}
				}
			}
			if (c->has_meta("_cg_orig_col_font_color")) {
				Color orig_c = c->get_meta("_cg_orig_col_font_color");
				c->remove_meta("_cg_orig_col_font_color");
				c->add_theme_color_override(SceneStringName(font_color), orig_c);
			}
		}
	}
}

void ClockGearsTheme::refresh_all_themed_roots() {
#ifdef TOOLS_ENABLED
	if (EditorNode::get_singleton()) {
		if (VBoxContainer *wroot = EditorNode::get_singleton()->get_clockgears_workspace_root()) {
			register_themed_root(wroot);
			if (wroot->get_child_count() > 0) {
				register_themed_root(wroot->get_child(0)); // title_bar
			}
		}
		if (MenuBar *mbar = EditorNode::get_singleton()->get_clockgears_main_menu_bar()) {
			register_themed_root(mbar);
		}
	}
	if (Button *ind = EditorNode::get_active_clockgears_layer_indicator()) {
		if (ind->get_parent() && ind->get_parent()->get_parent()) {
			register_themed_root(ind->get_parent()->get_parent());
		} else {
			register_themed_root(ind);
		}
	}
#endif
	// Auto-descubrir paneles, barras, diálogos y F3 activos en el SceneTree
	if (SceneTree::get_singleton() && SceneTree::get_singleton()->get_root()) {
		Vector<Node *> scan;
		scan.push_back(SceneTree::get_singleton()->get_root());
		while (!scan.is_empty()) {
			Node *cur = scan[scan.size() - 1];
			scan.remove_at(scan.size() - 1);
			String cn = cur->get_class();
			if (cn == "ClockGearsTopBar" || cn == "ClockGearsMapPanel" || cn == "ClockGearsDatabaseDialog" ||
					cn == "ClockGearsEventEditorDialog" || cn == "ClockGearsPreferencesDialog" || cn == "ClockGearsLayersDialog" ||
					cn == "ClockGearsCameraDialog" || cn == "ClockGearsSimplifiedExportDialog" || cn == "ClockGearsNewProjectDialog" ||
					cn == "ClockGearsResourceManagerDialog" || cn == "ClockGearsGenerateEntityDialog" || cn == "ClockGearsCommandSelector" ||
					cn == "ClockGearsTilePickerDialog" || cn == "ClockGearsMapPropertiesDialog" || cn == "ClockGearsDebugOverlay" ||
					cur->get_name() == "ClockGearsTitleBarRow") {
				register_themed_root(cur);
			}
			for (int i = 0; i < cur->get_child_count(); i++) {
				scan.push_back(cur->get_child(i));
			}
		}
	}

	Vector<ObjectID> alive_ids;
	for (int i = 0; i < registered_roots.size(); i++) {
		Node *n = Object::cast_to<Node>(ObjectDB::get_instance(registered_roots[i]));
		if (n) {
			alive_ids.push_back(registered_roots[i]);
			apply_theme_to_node_tree(n);
		}
	}
	registered_roots = alive_ids;
}

void ClockGearsTheme::apply_to_control(Control *p_control) {
	ERR_FAIL_NULL(p_control);
	register_themed_root(p_control);
	if (active_theme_id == "brass" || p_control->has_meta("_cg_brass_tree_applied")) {
		apply_theme_to_node_tree(p_control);
	} else {
		p_control->set_theme(get_theme(use_light_mode));
	}
}

void ClockGearsTheme::apply_to_window(Window *p_window) {
	ERR_FAIL_NULL(p_window);
	register_themed_root(p_window);
	if (active_theme_id == "brass" || p_window->has_meta("_cg_brass_tree_applied")) {
		apply_theme_to_node_tree(p_window);
	} else {
		p_window->set_theme(get_theme(use_light_mode));
	}
}

static float _cg_srgb_channel_to_linear(float p_c) {
	float c = CLAMP(p_c, 0.0f, 1.0f);
	return (c <= 0.04045f) ? (c / 12.92f) : Math::pow((c + 0.055f) / 1.055f, 2.4f);
}

static float _cg_relative_luminance(const Color &p_col) {
	return 0.2126f * _cg_srgb_channel_to_linear(p_col.r) +
			0.7152f * _cg_srgb_channel_to_linear(p_col.g) +
			0.0722f * _cg_srgb_channel_to_linear(p_col.b);
}

float ClockGearsTheme::compute_contrast_ratio(const Color &p_fg, const Color &p_bg) {
	float l1 = _cg_relative_luminance(p_fg);
	float l2 = _cg_relative_luminance(p_bg);
	float hi = MAX(l1, l2);
	float lo = MIN(l1, l2);
	return (hi + 0.05f) / (lo + 0.05f);
}

Dictionary ClockGearsTheme::inspect_theme_metrics(const String &p_theme_id) {
	String id = p_theme_id.is_empty() ? active_theme_id : p_theme_id.strip_edges().to_lower();
	Ref<Theme> th = get_theme_by_id(id);
	Dictionary out;
	out["theme_id"] = id;
	out["display_name"] = get_theme_display_name(id);
	out["is_valid"] = th.is_valid();
	if (th.is_null()) {
		return out;
	}

	auto get_sb_bg = [&th](const StringName &p_name, const StringName &p_type, const Color &p_fallback) -> Color {
		Ref<StyleBoxFlat> sbf = th->get_stylebox(p_name, p_type);
		return sbf.is_valid() ? sbf->get_bg_color() : p_fallback;
	};

	Color bg_panel = get_sb_bg("panel", "PanelContainer", color_panel());
	Color bg_win = get_sb_bg("panel", "AcceptDialog", color_bg_dark());
	Color bg_btn_n = get_sb_bg("normal", "Button", color_panel());
	Color bg_btn_h = get_sb_bg("hover", "Button", color_panel());
	Color bg_btn_p = get_sb_bg("pressed", "Button", color_panel());
	Color bg_btn_d = get_sb_bg("disabled", "Button", color_panel());
	Color bg_input = get_sb_bg("normal", "LineEdit", color_bg_dark());
	Color bg_list = get_sb_bg("panel", "ItemList", color_bg_dark());
	Color bg_tab_s = get_sb_bg("tab_selected", "TabContainer", color_panel());
	Color bg_tab_u = get_sb_bg("tab_unselected", "TabContainer", color_bg_dark());
	Color bg_pop = get_sb_bg("panel", "PopupMenu", color_bg_dark());

	Dictionary contrasts;
	contrasts["label_on_panel"] = compute_contrast_ratio(th->get_color("font_color", "Label"), bg_panel);
	contrasts["label_on_window"] = compute_contrast_ratio(th->get_color("title_color", "Window"), bg_win);
	contrasts["button_normal"] = compute_contrast_ratio(th->get_color("font_color", "Button"), bg_btn_n);
	contrasts["button_hover"] = compute_contrast_ratio(th->get_color("font_hover_color", "Button"), bg_btn_h);
	contrasts["button_pressed"] = compute_contrast_ratio(th->get_color("font_pressed_color", "Button"), bg_btn_p);
	contrasts["button_disabled"] = compute_contrast_ratio(th->get_color("font_disabled_color", "Button"), bg_btn_d);
	contrasts["line_edit_normal"] = compute_contrast_ratio(th->get_color("font_color", "LineEdit"), bg_input);
	contrasts["line_edit_placeholder"] = compute_contrast_ratio(th->get_color("font_placeholder_color", "LineEdit"), bg_input);
	contrasts["text_edit_normal"] = compute_contrast_ratio(th->get_color("font_color", "TextEdit"), bg_input);
	contrasts["item_list_normal"] = compute_contrast_ratio(th->get_color("font_color", "ItemList"), bg_list);
	contrasts["item_list_selected"] = compute_contrast_ratio(th->get_color("font_selected_color", "ItemList"), bg_list);
	contrasts["tree_normal"] = compute_contrast_ratio(th->get_color("font_color", "Tree"), bg_list);
	contrasts["tab_selected"] = compute_contrast_ratio(th->get_color("font_selected_color", "TabContainer"), bg_tab_s);
	contrasts["tab_unselected"] = compute_contrast_ratio(th->get_color("font_unselected_color", "TabContainer"), bg_tab_u);
	contrasts["popup_normal"] = compute_contrast_ratio(th->get_color("font_color", "PopupMenu"), bg_pop);
	contrasts["popup_accelerator"] = compute_contrast_ratio(th->get_color("font_accelerator_color", "PopupMenu"), bg_pop);

	float min_contrast = 100.0f;
	String min_key;
	Array ckeys = contrasts.keys();
	for (int i = 0; i < ckeys.size(); i++) {
		String k = ckeys[i];
		float v = float(contrasts[k]);
		if (v < min_contrast) {
			min_contrast = v;
			min_key = k;
		}
	}
	out["contrasts"] = contrasts;
	out["min_contrast"] = min_contrast;
	out["min_contrast_pair"] = min_key;
	out["all_contrasts_pass_4_5"] = (min_contrast >= 4.5f);

	Ref<ClockGearsBrassStyleBox> sb_p = th->get_stylebox("panel", "PanelContainer");
	Ref<ClockGearsBrassStyleBox> sb_bh = th->get_stylebox("hover", "Button");
	Ref<ClockGearsBrassStyleBox> sb_bp = th->get_stylebox("pressed", "Button");
	Ref<ClockGearsBrassStyleBox> sb_bn = th->get_stylebox("normal", "Button");
	Ref<ClockGearsBrassStyleBox> sb_sep = th->get_stylebox("separator", "HSeparator");
	out["has_brass_gradients"] = sb_p.is_valid() && sb_p->get_gradient_mode() != ClockGearsBrassStyleBox::GRADIENT_NONE;
	out["has_panel_shadow"] = sb_p.is_valid() && sb_p->get_shadow_size() > 0;
	out["has_hover_glow"] = sb_bh.is_valid() && sb_bh->get_shadow_size() > 0;
	out["has_pressed_sink"] = sb_bp.is_valid() && sb_bp->is_sunken_style() && sb_bn.is_valid() && (sb_bp->get_content_margin(SIDE_TOP) > sb_bn->get_content_margin(SIDE_TOP));
	out["has_gear_separator"] = sb_sep.is_valid() && sb_sep->is_gear_separator_style();
	out["hover_transition_ms"] = th->get_constant("hover_transition_ms", "Button");
	out["tab_transition_ms"] = th->get_constant("tab_transition_ms", "TabContainer");
	out["cached_gradient_generations"] = gradient_generation_count;
	out["cached_gear_generations"] = gear_generation_count;
	return out;
}

Dictionary ClockGearsTheme::check_tree_legibility_and_layout(Node *p_root, const Vector2i &p_viewport_size) {
	Dictionary res;
	res["viewport_size"] = p_viewport_size;
	if (!p_root) {
		res["ok"] = false;
		return res;
	}
	Color fallback_bg = (active_theme_id == "brass") ? color_brass_panel() : (active_theme_id == "light" ? Color::html("#ffffff") : color_panel());
	int total_controls = 0;
	int legible_controls = 0;
	int illegible_controls = 0;
	float worst_contrast = 100.0f;
	String worst_control_name;

	Vector<Node *> stack;
	stack.push_back(p_root);
	while (!stack.is_empty()) {
		Node *n = stack[stack.size() - 1];
		stack.remove_at(stack.size() - 1);
		if (_cg_is_in_game_preview_node(n)) {
			continue;
		}
		for (int i = 0; i < n->get_child_count(); i++) {
			stack.push_back(n->get_child(i));
		}
		Control *c = Object::cast_to<Control>(n);
		if (!c) {
			continue;
		}
		bool has_text = Object::cast_to<Label>(c) || Object::cast_to<Button>(c) || Object::cast_to<LineEdit>(c) ||
				Object::cast_to<TextEdit>(c) || Object::cast_to<ItemList>(c) || Object::cast_to<Tree>(c) ||
				Object::cast_to<TabBar>(c) || Object::cast_to<TabContainer>(c);
		if (!has_text) {
			continue;
		}
		total_controls++;
		float ratio = 21.0f;
		if (Object::cast_to<TabContainer>(c) || Object::cast_to<TabBar>(c)) {
			Color fg_sel = c->get_theme_color(StringName("font_selected_color"));
			Color fg_unsel = c->get_theme_color(StringName("font_unselected_color"));
			Color bg_sel = fallback_bg;
			Color bg_unsel = fallback_bg;
			Ref<StyleBoxFlat> sb_sel = c->get_theme_stylebox(StringName("tab_selected"));
			if (sb_sel.is_valid() && sb_sel->get_bg_color().a > 0.5f) {
				bg_sel = sb_sel->get_bg_color();
			}
			Ref<StyleBoxFlat> sb_unsel = c->get_theme_stylebox(StringName("tab_unselected"));
			if (sb_unsel.is_valid() && sb_unsel->get_bg_color().a > 0.5f) {
				bg_unsel = sb_unsel->get_bg_color();
			}
			ratio = MIN(compute_contrast_ratio(fg_sel, bg_sel), compute_contrast_ratio(fg_unsel, bg_unsel));
		} else {
			Color fg = c->get_theme_color(SceneStringName(font_color));
			Color bg = fallback_bg;
			if (Object::cast_to<Button>(c)) {
				Ref<StyleBoxFlat> sbf = c->get_theme_stylebox(StringName("normal"));
				if (sbf.is_valid() && sbf->get_bg_color().a > 0.5f) {
					bg = sbf->get_bg_color();
				}
			} else if (Object::cast_to<LineEdit>(c) || Object::cast_to<TextEdit>(c)) {
				Ref<StyleBoxFlat> sbf = c->get_theme_stylebox(StringName("normal"));
				if (sbf.is_valid() && sbf->get_bg_color().a > 0.5f) {
					bg = sbf->get_bg_color();
				}
			} else if (Object::cast_to<ItemList>(c) || Object::cast_to<Tree>(c)) {
				Ref<StyleBoxFlat> sbf = c->get_theme_stylebox(SceneStringName(panel));
				if (sbf.is_valid() && sbf->get_bg_color().a > 0.5f) {
					bg = sbf->get_bg_color();
				}
			}
			ratio = compute_contrast_ratio(fg, bg);
		}
		if (ratio < worst_contrast) {
			worst_contrast = ratio;
			worst_control_name = String(c->get_class()) + ":" + String(c->get_name());
		}
		if (ratio >= 4.5f) {
			legible_controls++;
		} else {
			illegible_controls++;
		}
	}

	bool fits_bounds = true;
	if (Window *w = Object::cast_to<Window>(p_root)) {
		Size2i wsz = w->get_size();
		fits_bounds = (wsz.x <= p_viewport_size.x && wsz.y <= p_viewport_size.y);
	} else if (Control *rc = Object::cast_to<Control>(p_root)) {
		Size2 rsz = rc->get_size();
		fits_bounds = (rsz.x <= float(p_viewport_size.x) + 1.0f && rsz.y <= float(p_viewport_size.y) + 1.0f);
	}

	res["total_text_controls"] = total_controls;
	res["legible_controls"] = legible_controls;
	res["illegible_controls"] = illegible_controls;
	res["worst_contrast"] = worst_contrast;
	res["worst_control"] = worst_control_name;
	res["fits_bounds"] = fits_bounds;
	res["ok"] = (illegible_controls == 0 && fits_bounds);
	return res;
}

void ClockGearsTheme::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_theme", "light_mode"), &ClockGearsTheme::get_theme, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_brass_theme"), &ClockGearsTheme::get_brass_theme);
	ClassDB::bind_method(D_METHOD("get_theme_by_id", "theme_id"), &ClockGearsTheme::get_theme_by_id);
	ClassDB::bind_method(D_METHOD("get_active_theme"), &ClockGearsTheme::get_active_theme);
	ClassDB::bind_method(D_METHOD("get_game_pixel_theme", "integer_scale"), &ClockGearsTheme::get_game_pixel_theme, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("set_light_mode", "light"), &ClockGearsTheme::set_light_mode);
	ClassDB::bind_method(D_METHOD("is_light_mode"), &ClockGearsTheme::is_light_mode);
	ClassDB::bind_method(D_METHOD("get_available_theme_ids"), &ClockGearsTheme::get_available_theme_ids);
	ClassDB::bind_method(D_METHOD("get_theme_display_name", "theme_id"), &ClockGearsTheme::get_theme_display_name);
	ClassDB::bind_method(D_METHOD("get_active_theme_id"), &ClockGearsTheme::get_active_theme_id);
	ClassDB::bind_method(D_METHOD("set_active_theme_id", "theme_id"), &ClockGearsTheme::set_active_theme_id);
	ClassDB::bind_method(D_METHOD("sync_from_project_settings"), &ClockGearsTheme::sync_from_project_settings);
	ClassDB::bind_method(D_METHOD("set_simulate_brass_load_failure", "fail"), &ClockGearsTheme::set_simulate_brass_load_failure);
	ClassDB::bind_method(D_METHOD("is_simulating_brass_load_failure"), &ClockGearsTheme::is_simulating_brass_load_failure);
	ClassDB::bind_method(D_METHOD("get_cached_brass_gradient_texture", "mode"), &ClockGearsTheme::get_cached_brass_gradient_texture);
	ClassDB::bind_method(D_METHOD("get_cached_brass_gear_texture"), &ClockGearsTheme::get_cached_brass_gear_texture);
	ClassDB::bind_method(D_METHOD("get_cached_gradient_generation_count"), &ClockGearsTheme::get_cached_gradient_generation_count);
	ClassDB::bind_method(D_METHOD("get_cached_gear_generation_count"), &ClockGearsTheme::get_cached_gear_generation_count);
	ClassDB::bind_method(D_METHOD("register_themed_root", "root"), &ClockGearsTheme::register_themed_root);
	ClassDB::bind_method(D_METHOD("apply_theme_to_node_tree", "root"), &ClockGearsTheme::apply_theme_to_node_tree);
	ClassDB::bind_method(D_METHOD("refresh_all_themed_roots"), &ClockGearsTheme::refresh_all_themed_roots);
	ClassDB::bind_method(D_METHOD("apply_to_control", "control"), &ClockGearsTheme::apply_to_control);
	ClassDB::bind_method(D_METHOD("apply_to_window", "window"), &ClockGearsTheme::apply_to_window);
	ClassDB::bind_static_method("ClockGearsTheme", D_METHOD("compute_contrast_ratio", "fg", "bg"), &ClockGearsTheme::compute_contrast_ratio);
	ClassDB::bind_method(D_METHOD("inspect_theme_metrics", "theme_id"), &ClockGearsTheme::inspect_theme_metrics, DEFVAL(""));
	ClassDB::bind_method(D_METHOD("check_tree_legibility_and_layout", "root", "viewport_size"), &ClockGearsTheme::check_tree_legibility_and_layout, DEFVAL(Vector2i(1366, 768)));

	ADD_SIGNAL(MethodInfo("theme_changed", PropertyInfo(Variant::STRING, "theme_id")));
}

ClockGearsTheme::ClockGearsTheme() {
	singleton = this;
}

ClockGearsTheme::~ClockGearsTheme() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
