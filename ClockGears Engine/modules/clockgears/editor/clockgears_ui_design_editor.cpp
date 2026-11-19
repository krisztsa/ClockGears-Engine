#include "clockgears_ui_design_editor.h"

#include "../clockgears_database.h"
#include "../core/clockgears_data_manager.h"
#include "../icons/clockgears_icons.h"
#include "../runtime/clockgears_credits.h"
#include "../runtime/clockgears_game_menu.h"
#include "../runtime/clockgears_hud.h"
#include "../runtime/clockgears_title_screen.h"
#include "../runtime/clockgears_ui_design_layer.h"
#include "clockgears_theme.h"
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/os/keyboard.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/aspect_ratio_container.h"
#include "scene/gui/label.h"
#include "scene/gui/separator.h"
#include "scene/gui/text_edit.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/texture.h"

namespace {
class UiCanvasHudGridBackdrop : public Control {
	GDCLASS(UiCanvasHudGridBackdrop, Control);

protected:
	void _notification(int p_what) {
		if (p_what == NOTIFICATION_DRAW) {
			Size2 sz = get_size();
			draw_rect(Rect2(Vector2(), sz), Color(0.11f, 0.14f, 0.19f, 1.0f));
			float step = MAX(16.0f, 32.0f * (sz.x / 960.0f));
			Color grid_c(0.20f, 0.25f, 0.33f, 0.55f);
			for (float x = step; x < sz.x; x += step) {
				draw_line(Vector2(x, 0.0f), Vector2(x, sz.y), grid_c, 1.0f);
			}
			for (float y = step; y < sz.y; y += step) {
				draw_line(Vector2(0.0f, y), Vector2(sz.x, y), grid_c, 1.0f);
			}
			// Guías centrales sutiles
			Color mid_c(0.88f, 0.63f, 0.25f, 0.25f);
			draw_line(Vector2(sz.x * 0.5f, 0.0f), Vector2(sz.x * 0.5f, sz.y), mid_c, 1.0f);
			draw_line(Vector2(0.0f, sz.y * 0.5f), Vector2(sz.x, sz.y * 0.5f), mid_c, 1.0f);
		}
	}
};

static void s_reparent_ctrl(Control *p_ctrl, Control *p_new_parent) {
	if (!p_ctrl || !p_new_parent || p_ctrl->get_parent() == p_new_parent) {
		return;
	}
	if (p_ctrl->get_parent()) {
		p_ctrl->get_parent()->remove_child(p_ctrl);
	}
	p_new_parent->add_child(p_ctrl);
}

static void s_make_rounded_action_btn(Button *p_btn) {
	if (!p_btn) {
		return;
	}
	p_btn->set_clip_text(true);
	p_btn->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	p_btn->set_theme_type_variation("ClockGearsTabButton");
	Ref<StyleBoxFlat> sb = ClockGearsTheme::make_card_stylebox(EDSCALE);
	sb->set_corner_radius_all(int(6 * EDSCALE));
	sb->set_content_margin_all(int(6 * EDSCALE));
	sb->set_bg_color(Color(0.18f, 0.20f, 0.25f, 1.0f));
	sb->set_border_color(Color(0.32f, 0.36f, 0.44f, 0.9f));
	p_btn->add_theme_style_override("normal", sb);

	Ref<StyleBoxFlat> hov = sb->duplicate();
	hov->set_bg_color(Color(0.24f, 0.27f, 0.34f, 1.0f));
	hov->set_border_color(ClockGearsTheme::color_accent_amber());
	p_btn->add_theme_style_override("hover", hov);

	Ref<StyleBoxFlat> prs = sb->duplicate();
	prs->set_bg_color(Color(0.32f, 0.24f, 0.13f, 1.0f));
	prs->set_border_color(ClockGearsTheme::color_accent_amber());
	p_btn->add_theme_style_override("pressed", prs);
}

// Miniatura procedural 16:9 generada y cacheada para galeria de plantillas y asistente (4A/4B)
class ClockGearsTemplateThumbCtrl : public Control {
	GDCLASS(ClockGearsTemplateThumbCtrl, Control);

	Dictionary design_data;
	Dictionary comp_elem;
	bool is_comp = false;

protected:
	void _notification(int p_what) {
		if (p_what != NOTIFICATION_DRAW) {
			return;
		}
		Size2 sz = get_size();
		if (sz.x <= 4.0f || sz.y <= 4.0f) {
			return;
		}
		draw_rect(Rect2(Vector2(), sz), Color(0.08f, 0.10f, 0.15f, 1.0f));
		Dictionary bg = design_data.get("background", Dictionary());
		String bt = String(bg.get("type", "none")).to_lower();
		if (bt == "color" && bg.has("color") && Color::html_is_valid(String(bg["color"]))) {
			draw_rect(Rect2(Vector2(), sz), Color::html(String(bg["color"])));
		} else if (bt == "gradient") {
			Color gt = bg.has("gradient_top") && Color::html_is_valid(String(bg["gradient_top"])) ? Color::html(String(bg["gradient_top"])) : Color(0.14f, 0.17f, 0.28f, 1.0f);
			draw_rect(Rect2(Vector2(), sz), gt);
		}
		if (is_comp && !comp_elem.is_empty()) {
			Rect2 r(sz.x * 0.2f, sz.y * 0.2f, sz.x * 0.6f, sz.y * 0.6f);
			draw_rect(r, Color(0.22f, 0.26f, 0.36f, 1.0f));
			draw_rect(r, Color(0.95f, 0.72f, 0.30f, 1.0f), false, 1.5f);
			return;
		}
		float sx = sz.x / 960.0f;
		float sy = sz.y / 540.0f;
		Dictionary lay = design_data.get("layout", Dictionary());
		if (String(lay.get("mode", "")) == "radial") {
			Vector2 c(float(lay.get("radial_center_x", 480)) * sx, float(lay.get("radial_center_y", 270)) * sy);
			float r = float(lay.get("radial_radius", 145)) * MIN(sx, sy);
			float start_deg = float(lay.get("radial_start_angle_deg", -90.0));
			float sweep_deg = float(lay.get("radial_sweep_angle_deg", 360.0));
			bool cw = lay.has("radial_clockwise") ? bool(lay["radial_clockwise"]) : true;
			Array rslots = lay.get("radial_slots", Array());
			int n = MAX(4, rslots.size());
			draw_arc(c, r, Math::deg_to_rad(start_deg), Math::deg_to_rad(start_deg + (cw ? sweep_deg : -sweep_deg)), 24, Color(0.88f, 0.63f, 0.25f, 0.45f), 1.5f);
			for (int i = 0; i < n; i++) {
				float frac = (sweep_deg >= 359.0f) ? (float(i) / float(n)) : (n > 1 ? float(i) / float(n - 1) : 0.0f);
				float a = Math::deg_to_rad(start_deg + (cw ? 1.0f : -1.0f) * sweep_deg * frac);
				Vector2 sp = c + Vector2(Math::cos(a), Math::sin(a)) * r;
				draw_circle(sp, MAX(4.0f, r * 0.18f), (i == 0) ? Color(0.95f, 0.72f, 0.30f, 1.0f) : Color(0.24f, 0.28f, 0.38f, 0.95f));
			}
			draw_circle(c, MAX(6.0f, r * 0.30f), Color(0.14f, 0.16f, 0.22f, 0.95f));
		} else {
			Array els = design_data.get("elements", Array());
			for (int i = 0; i < els.size(); i++) {
				if (els[i].get_type() != Variant::DICTIONARY) continue;
				Dictionary el = els[i];
				Rect2 r = ClockGearsUiDesignLayer::compute_element_rect(el, Rect2(Vector2(), sz), MIN(sx, sy));
				String et = String(el.get("type", "text"));
				Color fc(0.22f, 0.26f, 0.36f, 0.85f);
				Color bc(0.88f, 0.63f, 0.25f, 0.75f);
				if (et == "button") {
					fc = Color(0.26f, 0.22f, 0.16f, 0.95f);
					bc = Color(0.95f, 0.72f, 0.30f, 0.95f);
				} else if (et == "text") {
					fc = Color(0.85f, 0.88f, 0.95f, 0.55f);
				} else if (et == "bar") {
					fc = Color(0.82f, 0.25f, 0.25f, 0.90f);
				}
				draw_rect(r, fc, true);
				draw_rect(r, bc, false, 1.0f);
			}
		}
		draw_rect(Rect2(Vector2(), sz), Color(0.45f, 0.50f, 0.60f, 0.75f), false, 1.0f);
	}

public:
	void setup_template(const Dictionary &p_tpl) {
		is_comp = bool(p_tpl.get("is_component", false));
		comp_elem = is_comp ? Dictionary(p_tpl.get("element", Dictionary())) : Dictionary();
		design_data = is_comp ? Dictionary() : Dictionary(p_tpl.get("design", Dictionary()));
		queue_redraw();
	}
};
} // namespace

// ============================================================================
// ClockGearsUiPaletteButton
// ============================================================================

void ClockGearsUiPaletteButton::setup(ClockGearsUiDesignEditor *p_owner, const String &p_type, const String &p_label) {
	owner_editor = p_owner;
	element_type = p_type;
	set_text(p_label);
	set_tooltip_text(String::utf8("Arrastra al lienzo o haz doble clic para añadir: ") + p_label);
	set_clip_text(true);
	set_custom_minimum_size(Size2(0, 30 * EDSCALE));
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s_make_rounded_action_btn(this);
}

Variant ClockGearsUiPaletteButton::get_drag_data(const Point2 &p_at_position) {
	PanelContainer *prev = memnew(PanelContainer);
	prev->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	Label *lbl = memnew(Label(get_text()));
	lbl->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	prev->add_child(lbl);
	set_drag_preview(prev);

	Dictionary payload;
	payload["type"] = String("clockgears_ui_palette");
	payload["element_type"] = element_type;
	return payload;
}

void ClockGearsUiPaletteButton::gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT && mb->is_double_click()) {
		if (owner_editor) {
			(void)owner_editor->add_element_at(element_type, Vector2i(-1, -1));
			accept_event();
			return;
		}
	}
	Button::gui_input(p_event);
}

// ============================================================================
// ClockGearsUiCanvasOverlay
// ============================================================================

void ClockGearsUiCanvasOverlay::set_grid_overlay_enabled(bool p_en) {
	show_grid_overlay = p_en;
	queue_redraw();
}

void ClockGearsUiCanvasOverlay::set_snap_enabled(bool p_en) {
	snap_to_grid = p_en;
	queue_redraw();
}

Rect2 ClockGearsUiCanvasOverlay::_get_handle_rect(const Rect2 &p_elem_rect, HandleType p_handle) const {
	float hs = 8.0f * MAX(1.0f, EDSCALE * 0.8f);
	float half = hs * 0.5f;
	Vector2 p = p_elem_rect.position;
	Vector2 s = p_elem_rect.size;
	Vector2 c;
	switch (p_handle) {
		case HANDLE_TL: c = p; break;
		case HANDLE_T: c = Vector2(p.x + s.x * 0.5f, p.y); break;
		case HANDLE_TR: c = Vector2(p.x + s.x, p.y); break;
		case HANDLE_L: c = Vector2(p.x, p.y + s.y * 0.5f); break;
		case HANDLE_R: c = Vector2(p.x + s.x, p.y + s.y * 0.5f); break;
		case HANDLE_BL: c = Vector2(p.x, p.y + s.y); break;
		case HANDLE_B: c = Vector2(p.x + s.x * 0.5f, p.y + s.y); break;
		case HANDLE_BR: c = Vector2(p.x + s.x, p.y + s.y); break;
		case HANDLE_RADIUS: {
			float rad_px = 14.0f;
			if (owner_editor) {
				ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
				Dictionary el = dm ? dm->get_ui_element(owner_editor->get_current_screen(), owner_editor->get_selected_element_id()) : Dictionary();
				if (el.has("shape") && el["shape"].get_type() == Variant::DICTIONARY) {
					float z = float(MAX(1, owner_editor->get_canvas_zoom()));
					rad_px = CLAMP(float(Dictionary(el["shape"]).get("corner_radius", 8)) * z, 8.0f, MIN(s.x, s.y) * 0.45f);
				}
			}
			c = p + Vector2(rad_px, 10.0f);
			break;
		}
		default: return Rect2();
	}
	return Rect2(c.x - half, c.y - half, hs, hs);
}

ClockGearsUiCanvasOverlay::HandleType ClockGearsUiCanvasOverlay::_hit_test_radial_handle(const Vector2 &p_pt) const {
	if (!owner_editor) {
		return HANDLE_NONE;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return HANDLE_NONE;
	}
	Dictionary design = dm->get_ui_screen_design(owner_editor->get_current_screen());
	Dictionary lay = design.get("layout", Dictionary());
	if (String(lay.get("mode", "free")) != "radial") {
		return HANDLE_NONE;
	}
	float z = float(MAX(1, owner_editor->get_canvas_zoom()));
	Vector2 c(float(lay.get("radial_center_x", 480)) * z, float(lay.get("radial_center_y", 270)) * z);
	float rad = float(lay.get("radial_radius", 145)) * z;
	float ang = Math::deg_to_rad(float(lay.get("radial_start_angle_deg", -90.0)));
	Vector2 ang_handle = c + Vector2(Math::cos(ang), Math::sin(ang)) * rad;
	if (p_pt.distance_to(ang_handle) <= 10.0f) {
		return HANDLE_RADIAL_ANGLE;
	}
	Vector2 rad_handle = c + Vector2(rad, 0.0f);
	if (p_pt.distance_to(rad_handle) <= 10.0f) {
		return HANDLE_RADIAL_RADIUS;
	}
	return HANDLE_NONE;
}

ClockGearsUiCanvasOverlay::HandleType ClockGearsUiCanvasOverlay::_hit_test_handle(const Rect2 &p_elem_rect, const Vector2 &p_pt) const {
	HandleType rad_h = _hit_test_radial_handle(p_pt);
	if (rad_h != HANDLE_NONE) {
		return rad_h;
	}
	if (!p_elem_rect.has_area()) {
		return HANDLE_NONE;
	}
	for (int h = HANDLE_TL; h <= HANDLE_RADIUS; h++) {
		Rect2 hr = _get_handle_rect(p_elem_rect, static_cast<HandleType>(h)).grow(4.0f);
		if (hr.has_point(p_pt)) {
			return static_cast<HandleType>(h);
		}
	}
	return HANDLE_NONE;
}

Vector2 ClockGearsUiCanvasOverlay::_apply_snap_and_guides(const Rect2 &p_base_rect, const String &p_ignore_id, Vector2 &r_delta) {
	guide_lines_x.clear();
	guide_lines_y.clear();
	Vector2 moved_pos = p_base_rect.position + r_delta;
	Vector2 moved_ctr = moved_pos + p_base_rect.size * 0.5f;

	if (snap_to_grid && grid_step > 1) {
		moved_pos.x = Math::round(moved_pos.x / float(grid_step)) * float(grid_step);
		moved_pos.y = Math::round(moved_pos.y / float(grid_step)) * float(grid_step);
		r_delta = moved_pos - p_base_rect.position;
		moved_ctr = moved_pos + p_base_rect.size * 0.5f;
	}

	if (show_smart_guides && owner_editor) {
		const float tol = 5.0f;
		// Centro del lienzo 960x540
		if (Math::abs(moved_ctr.x - 480.0f) <= tol) {
			r_delta.x += (480.0f - moved_ctr.x);
			guide_lines_x.push_back(480.0f);
		}
		if (Math::abs(moved_ctr.y - 270.0f) <= tol) {
			r_delta.y += (270.0f - moved_ctr.y);
			guide_lines_y.push_back(270.0f);
		}
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			Array elems = dm->get_ui_screen_elements(owner_editor->get_current_screen());
			Rect2 stage(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_H));
			Vector2 cur_pos = p_base_rect.position + r_delta;
			for (int i = 0; i < elems.size(); i++) {
				if (elems[i].get_type() != Variant::DICTIONARY) continue;
				Dictionary e = elems[i];
				if (String(e.get("id", "")) == p_ignore_id || !bool(e.get("visible", true))) continue;
				Rect2 er = ClockGearsUiDesignLayer::compute_element_rect(e, stage, 1.0f);
				if (guide_lines_x.is_empty()) {
					if (Math::abs(cur_pos.x - er.position.x) <= tol) {
						r_delta.x += (er.position.x - cur_pos.x);
						guide_lines_x.push_back(er.position.x);
					} else if (Math::abs((cur_pos.x + p_base_rect.size.x * 0.5f) - (er.position.x + er.size.x * 0.5f)) <= tol) {
						float cx = er.position.x + er.size.x * 0.5f;
						r_delta.x += (cx - (cur_pos.x + p_base_rect.size.x * 0.5f));
						guide_lines_x.push_back(cx);
					}
				}
				if (guide_lines_y.is_empty()) {
					if (Math::abs(cur_pos.y - er.position.y) <= tol) {
						r_delta.y += (er.position.y - cur_pos.y);
						guide_lines_y.push_back(er.position.y);
					} else if (Math::abs((cur_pos.y + p_base_rect.size.y * 0.5f) - (er.position.y + er.size.y * 0.5f)) <= tol) {
						float cy = er.position.y + er.size.y * 0.5f;
						r_delta.y += (cy - (cur_pos.y + p_base_rect.size.y * 0.5f));
						guide_lines_y.push_back(cy);
					}
				}
			}
		}
	}
	return r_delta;
}

String ClockGearsUiCanvasOverlay::_hit_test_element(const Vector2 &p_pt) const {
	if (!owner_editor) {
		return String();
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return String();
	}
	Array elems = dm->get_ui_screen_elements(owner_editor->get_current_screen());
	if (elems.is_empty()) {
		return String();
	}
	float scale = float(owner_editor->get_canvas_zoom());
	Rect2 stage(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W * scale, ClockGearsUiDesignEditor::BASE_H * scale));

	int best_score_layer = -999999;
	int best_idx = -1;
	String best_id;
	for (int i = 0; i < elems.size(); i++) {
		if (elems[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary e = elems[i];
		if (!bool(e.get("visible", true))) {
			continue;
		}
		Rect2 r = ClockGearsUiDesignLayer::compute_element_rect(e, stage, scale);
		if (r.has_point(p_pt)) {
			int lo = int(e.get("layer_order", 0));
			if (lo > best_score_layer || (lo == best_score_layer && i > best_idx)) {
				best_score_layer = lo;
				best_idx = i;
				best_id = String(e.get("id", ""));
			}
		}
	}
	return best_id;
}

Rect2 ClockGearsUiCanvasOverlay::get_selected_widget_rect() const {
	if (!owner_editor) {
		return Rect2();
	}
	String sel_id = owner_editor->get_selected_element_id();
	if (sel_id.is_empty()) {
		return Rect2();
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return Rect2();
	}
	Dictionary elem = dm->get_ui_element(owner_editor->get_current_screen(), sel_id);
	if (elem.is_empty()) {
		return Rect2();
	}
	float scale = float(owner_editor->get_canvas_zoom());
	Rect2 stage(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W * scale, ClockGearsUiDesignEditor::BASE_H * scale));
	return ClockGearsUiDesignLayer::compute_element_rect(elem, stage, scale);
}

Rect2 ClockGearsUiCanvasOverlay::get_handle_widget_rect(int p_handle) const {
	Rect2 sr = get_selected_widget_rect();
	if (!sr.has_area() || p_handle < HANDLE_TL || p_handle > HANDLE_BR) {
		return Rect2();
	}
	return _get_handle_rect(sr, static_cast<HandleType>(p_handle));
}

bool ClockGearsUiCanvasOverlay::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
	if (p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}
	Dictionary d = p_data;
	return String(d.get("type", "")) == "clockgears_ui_palette" && !String(d.get("element_type", "")).is_empty();
}

void ClockGearsUiCanvasOverlay::drop_data(const Point2 &p_point, const Variant &p_data) {
	if (!owner_editor || !can_drop_data(p_point, p_data)) {
		return;
	}
	Dictionary d = p_data;
	String etype = String(d.get("element_type", ""));
	float z = float(MAX(1, owner_editor->get_canvas_zoom()));
	Vector2i logical_pos(
			CLAMP(int(Math::round(p_point.x / z)), 0, ClockGearsUiDesignEditor::BASE_W),
			CLAMP(int(Math::round(p_point.y / z)), 0, ClockGearsUiDesignEditor::BASE_H));
	(void)owner_editor->add_element_at(etype, logical_pos);
}

void ClockGearsUiCanvasOverlay::gui_input(const Ref<InputEvent> &p_event) {
	if (!owner_editor) {
		return;
	}
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		// Regla: la rueda del ratón NO hace zoom en el lienzo
		if (mb->get_button_index() == MouseButton::WHEEL_UP ||
				mb->get_button_index() == MouseButton::WHEEL_DOWN ||
				mb->get_button_index() == MouseButton::WHEEL_LEFT ||
				mb->get_button_index() == MouseButton::WHEEL_RIGHT) {
			accept_event();
			return;
		}
		if (mb->get_button_index() == MouseButton::RIGHT && mb->is_pressed()) {
			Vector2 pt = mb->get_position();
			String hit_id = _hit_test_element(pt);
			if (!hit_id.is_empty() && hit_id != owner_editor->get_selected_element_id()) {
				owner_editor->select_element(hit_id);
			}
			owner_editor->show_canvas_context_menu(mb->get_global_position());
			accept_event();
			return;
		}
		if (mb->get_button_index() == MouseButton::LEFT) {
			Vector2 pt = mb->get_position();
			if (mb->is_pressed() && mb->is_double_click()) {
				String hit_id = _hit_test_element(pt);
				if (!hit_id.is_empty()) {
					owner_editor->select_element(hit_id);
					owner_editor->begin_inline_edit_for_selected();
					dragging = false;
					accept_event();
					return;
				}
			}
			if (mb->is_pressed()) {
				ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
				Rect2 sel_r = get_selected_widget_rect();
				HandleType hh = _hit_test_handle(sel_r, pt);
				if ((hh == HANDLE_RADIAL_RADIUS || hh == HANDLE_RADIAL_ANGLE) && dm) {
					drag_initial_screen_snapshot = dm->get_ui_screen_design(owner_editor->get_current_screen());
					drag_elem_id = String();
					drag_start_pos = pt;
					active_handle = hh;
					dragging = true;
					drag_changed = false;
					accept_event();
					return;
				}
				String hit_id = (hh != HANDLE_NONE) ? owner_editor->get_selected_element_id() : _hit_test_element(pt);
				if (hit_id.is_empty()) {
					owner_editor->select_element("");
					dragging = false;
					active_handle = HANDLE_NONE;
					guide_lines_x.clear();
					guide_lines_y.clear();
					queue_redraw();
					accept_event();
					return;
				}
				if (hit_id != owner_editor->get_selected_element_id()) {
					owner_editor->select_element(hit_id);
					hh = HANDLE_NONE;
				}
				if (dm) {
					drag_initial_elem = dm->get_ui_element(owner_editor->get_current_screen(), hit_id);
					drag_initial_screen_snapshot = dm->get_ui_screen_design(owner_editor->get_current_screen());
					drag_elem_id = hit_id;
					drag_start_pos = pt;
					active_handle = (hh != HANDLE_NONE) ? hh : HANDLE_MOVE;
					dragging = !drag_initial_elem.is_empty() && !bool(drag_initial_elem.get("locked", false));
					drag_changed = false;
				}
				accept_event();
				return;
			} else if (dragging) {
				ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
				if (dm && drag_changed && !drag_initial_screen_snapshot.is_empty()) {
					dm->push_ui_design_undo_state(owner_editor->get_current_screen(), drag_initial_screen_snapshot);
					owner_editor->call_deferred("refresh_from_data_manager");
				}
				dragging = false;
				active_handle = HANDLE_NONE;
				drag_changed = false;
				drag_initial_elem.clear();
				drag_initial_screen_snapshot.clear();
				guide_lines_x.clear();
				guide_lines_y.clear();
				queue_redraw();
				accept_event();
				return;
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		Vector2 pt = mm->get_position();
		if (dragging && (active_handle == HANDLE_RADIAL_RADIUS || active_handle == HANDLE_RADIAL_ANGLE)) {
			ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
			if (dm) {
				float z = float(MAX(1, owner_editor->get_canvas_zoom()));
				Dictionary design = dm->get_ui_screen_design(owner_editor->get_current_screen());
				Dictionary lay = design.get("layout", Dictionary());
				Vector2 c(float(lay.get("radial_center_x", 480)) * z, float(lay.get("radial_center_y", 270)) * z);
				Vector2 d = (pt - c) / z;
				int rad = int(lay.get("radial_radius", 145));
				float ang = float(lay.get("radial_start_angle_deg", -90.0));
				if (active_handle == HANDLE_RADIAL_RADIUS) {
					rad = CLAMP(int(Math::round(d.length())), 40, 260);
				} else {
					ang = Math::rad_to_deg(Math::atan2(d.y, d.x));
				}
				drag_changed = true;
				owner_editor->set_radial_layout_live(rad, ang);
			}
			accept_event();
			return;
		}
		if (dragging && !drag_elem_id.is_empty()) {
			ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
			if (dm) {
				float z = float(MAX(1, owner_editor->get_canvas_zoom()));
				int dx = int(Math::round((pt.x - drag_start_pos.x) / z));
				int dy = int(Math::round((pt.y - drag_start_pos.y) / z));

				String anchor = String(drag_initial_elem.get("anchor", "top_left"));
				Vector2 af = ClockGearsUiDesignLayer::compute_anchor_factors(anchor);
				int ox0 = int(drag_initial_elem.get("offset_x", 0));
				int oy0 = int(drag_initial_elem.get("offset_y", 0));
				int w0 = CLAMP(int(drag_initial_elem.get("width", 120)), 16, ClockGearsUiDesignEditor::BASE_W);
				int h0 = CLAMP(int(drag_initial_elem.get("height", 36)), 16, ClockGearsUiDesignEditor::BASE_H);
				int rad0 = int(drag_initial_elem.get("corner_radius", 8));

				Dictionary next_elem = drag_initial_elem.duplicate(true);
				if (active_handle == HANDLE_MOVE) {
					Rect2 stage(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_H));
					Rect2 base_r = ClockGearsUiDesignLayer::compute_element_rect(drag_initial_elem, stage, 1.0f);
					Vector2 delta{ float(dx), float(dy) };
					delta = _apply_snap_and_guides(base_r, drag_elem_id, delta);
					dx = int(Math::round(delta.x));
					dy = int(Math::round(delta.y));
					next_elem["offset_x"] = CLAMP(ox0 + dx, -ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_W);
					next_elem["offset_y"] = CLAMP(oy0 + dy, -ClockGearsUiDesignEditor::BASE_H, ClockGearsUiDesignEditor::BASE_H);
				} else if (active_handle == HANDLE_RADIUS) {
					int nrad = CLAMP(rad0 + dx, 0, 64);
					next_elem["corner_radius"] = nrad;
					String shp = String(next_elem.get("shape", "rect"));
					if (shp == "rect" && nrad > 0) {
						next_elem["shape"] = "rounded";
					}
				} else {
					guide_lines_x.clear();
					guide_lines_y.clear();
					int x0 = int(Math::round(af.x * float(ClockGearsUiDesignEditor::BASE_W - w0))) + ox0;
					int y0 = int(Math::round(af.y * float(ClockGearsUiDesignEditor::BASE_H - h0))) + oy0;
					int r0 = x0 + w0;
					int b0 = y0 + h0;

					int nx = x0;
					int ny = y0;
					int nr = r0;
					int nb = b0;

					if (active_handle == HANDLE_TL || active_handle == HANDLE_L || active_handle == HANDLE_BL) {
						nx = MIN(r0 - 16, x0 + dx);
					}
					if (active_handle == HANDLE_TR || active_handle == HANDLE_R || active_handle == HANDLE_BR) {
						nr = MAX(x0 + 16, r0 + dx);
					}
					if (active_handle == HANDLE_TL || active_handle == HANDLE_T || active_handle == HANDLE_TR) {
						ny = MIN(b0 - 16, y0 + dy);
					}
					if (active_handle == HANDLE_BL || active_handle == HANDLE_B || active_handle == HANDLE_BR) {
						nb = MAX(y0 + 16, b0 + dy);
					}

					int nw = CLAMP(nr - nx, 16, ClockGearsUiDesignEditor::BASE_W);
					int nh = CLAMP(nb - ny, 16, ClockGearsUiDesignEditor::BASE_H);
					if (active_handle == HANDLE_TL || active_handle == HANDLE_L || active_handle == HANDLE_BL) {
						nx = nr - nw;
					}
					if (active_handle == HANDLE_TL || active_handle == HANDLE_T || active_handle == HANDLE_TR) {
						ny = nb - nh;
					}
					int nox = nx - int(Math::round(af.x * float(ClockGearsUiDesignEditor::BASE_W - nw)));
					int noy = ny - int(Math::round(af.y * float(ClockGearsUiDesignEditor::BASE_H - nh)));

					next_elem["width"] = nw;
					next_elem["height"] = nh;
					next_elem["offset_x"] = CLAMP(nox, -ClockGearsUiDesignEditor::BASE_W, ClockGearsUiDesignEditor::BASE_W);
					next_elem["offset_y"] = CLAMP(noy, -ClockGearsUiDesignEditor::BASE_H, ClockGearsUiDesignEditor::BASE_H);
				}

				if (int(next_elem["offset_x"]) != ox0 || int(next_elem["offset_y"]) != oy0 ||
						int(next_elem["width"]) != w0 || int(next_elem["height"]) != h0 ||
						int(next_elem.get("corner_radius", 8)) != rad0) {
					drag_changed = true;
				}
				owner_editor->update_dragged_element_fast(drag_elem_id, next_elem);
			}
			accept_event();
			return;
		} else {
			Rect2 sel_r = get_selected_widget_rect();
			HandleType new_hh = _hit_test_handle(sel_r, pt);
			String new_hov = (new_hh != HANDLE_NONE) ? owner_editor->get_selected_element_id() : _hit_test_element(pt);
			if (new_hh != hover_handle || new_hov != hover_elem_id) {
				hover_handle = new_hh;
				hover_elem_id = new_hov;
				queue_redraw();
			}
		}
	}
}

void ClockGearsUiCanvasOverlay::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW && owner_editor) {
		ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
		if (!dm) {
			return;
		}
		float scale = float(MAX(1, owner_editor->get_canvas_zoom()));
		Rect2 stage(Vector2(), Vector2(ClockGearsUiDesignEditor::BASE_W * scale, ClockGearsUiDesignEditor::BASE_H * scale));

		// Rejilla opcional con imán (4D)
		if (show_grid_overlay && grid_step >= 4) {
			Color gc(0.75f, 0.72f, 0.62f, 0.14f);
			for (int gx = grid_step; gx < ClockGearsUiDesignEditor::BASE_W; gx += grid_step) {
				float px = float(gx) * scale;
				draw_line(Vector2(px, 0.0f), Vector2(px, stage.size.y), gc, 1.0f);
			}
			for (int gy = grid_step; gy < ClockGearsUiDesignEditor::BASE_H; gy += grid_step) {
				float py = float(gy) * scale;
				draw_line(Vector2(0.0f, py), Vector2(stage.size.x, py), gc, 1.0f);
			}
		}

		// Marco sutil del área base 960x540
		draw_rect(stage, Color(0.95f, 0.72f, 0.30f, 0.35f), false, 1.0f);

		// Asas interactivas radiales si la pantalla usa layout radial (4D)
		Dictionary design = dm->get_ui_screen_design(owner_editor->get_current_screen());
		Dictionary lay = design.get("layout", Dictionary());
		if (String(lay.get("mode", "free")) == "radial") {
			Vector2 c(float(lay.get("radial_center_x", 480)) * scale, float(lay.get("radial_center_y", 270)) * scale);
			float rad = float(lay.get("radial_radius", 145)) * scale;
			float ang = Math::deg_to_rad(float(lay.get("radial_start_angle_deg", -90.0)));
			draw_arc(c, rad, 0.0f, Math::TAU, 48, Color(0.96f, 0.78f, 0.34f, 0.45f), 1.5f);
			Vector2 rad_pt = c + Vector2(rad, 0.0f);
			Vector2 ang_pt = c + Vector2(Math::cos(ang), Math::sin(ang)) * rad;
			draw_line(c, ang_pt, Color(0.55f, 0.88f, 1.0f, 0.5f), 1.0f);
			draw_circle(rad_pt, 6.0f, (hover_handle == HANDLE_RADIAL_RADIUS) ? Color(1.0f, 0.94f, 0.65f) : Color(0.92f, 0.65f, 0.20f));
			draw_circle(ang_pt, 6.0f, (hover_handle == HANDLE_RADIAL_ANGLE) ? Color(0.85f, 1.0f, 1.0f) : Color(0.35f, 0.80f, 0.95f));
		}

		Array elems = dm->get_ui_screen_elements(owner_editor->get_current_screen());
		String sel_id = owner_editor->get_selected_element_id();
		for (int i = 0; i < elems.size(); i++) {
			if (elems[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary e = elems[i];
			String eid = String(e.get("id", ""));
			if (eid == sel_id) {
				continue;
			}
			Rect2 r = ClockGearsUiDesignLayer::compute_element_rect(e, stage, scale);
			Color c = (eid == hover_elem_id) ? Color(0.45f, 0.78f, 1.0f, 0.85f) : Color(0.55f, 0.62f, 0.74f, 0.25f);
			draw_rect(r, c, false, (eid == hover_elem_id) ? 1.5f : 1.0f);
		}

		if (!sel_id.is_empty()) {
			Rect2 sel_r = get_selected_widget_rect();
			if (sel_r.has_area()) {
				Color hi_c = ClockGearsTheme::color_accent_amber();
				draw_rect(sel_r, hi_c, false, 2.0f);
				draw_rect(sel_r.grow(1.0f), Color(0.08f, 0.09f, 0.12f, 0.85f), false, 1.0f);

				for (int h = HANDLE_TL; h <= HANDLE_BR; h++) {
					Rect2 hr = _get_handle_rect(sel_r, static_cast<HandleType>(h));
					draw_rect(hr, (hover_handle == h || (dragging && active_handle == h)) ? Color(1.0f, 0.94f, 0.68f, 1.0f) : hi_c, true);
					draw_rect(hr, Color(0.09f, 0.10f, 0.14f, 1.0f), false, 1.0f);
				}
				// Asa circular para radio de esquina (4D)
				Rect2 rhr = _get_handle_rect(sel_r, HANDLE_RADIUS);
				Vector2 rctr = rhr.position + rhr.size * 0.5f;
				draw_circle(rctr, rhr.size.x * 0.48f, (hover_handle == HANDLE_RADIUS || (dragging && active_handle == HANDLE_RADIUS)) ? Color(1.0f, 0.85f, 0.40f) : Color(0.95f, 0.55f, 0.18f));
			}
		}

		// Guías inteligentes mientras se arrastra (4D)
		Color g_col(1.0f, 0.42f, 0.78f, 0.88f);
		for (int i = 0; i < guide_lines_x.size(); i++) {
			float gx = guide_lines_x[i] * scale;
			draw_line(Vector2(gx, 0.0f), Vector2(gx, stage.size.y), g_col, 1.5f);
		}
		for (int i = 0; i < guide_lines_y.size(); i++) {
			float gy = guide_lines_y[i] * scale;
			draw_line(Vector2(0.0f, gy), Vector2(stage.size.x, gy), g_col, 1.5f);
		}
	}
}

// ============================================================================
// ClockGearsLayersCanvas (5C: dibujo propio para listas largas > 32 capas)
// ============================================================================

void ClockGearsLayersCanvas::set_rows(const Vector<Row> &p_rows, float p_row_height) {
	rows = p_rows;
	row_height = MAX(22.0f, p_row_height);
	float sep = 4.0f * EDSCALE;
	float total_h = rows.is_empty() ? 0.0f : (float(rows.size()) * (row_height + sep) - sep);
	set_custom_minimum_size(Size2(0, total_h));
	queue_redraw();
}

void ClockGearsLayersCanvas::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		Ref<Font> font = get_theme_font("font", "Button");
		int fsz = MAX(10, get_theme_font_size("font_size", "Button"));
		Size2 sz = get_size();
		float sep = 4.0f * EDSCALE;
		float step = row_height + sep;
		float eye_w = 34.0f * EDSCALE;

		// Dibujar únicamente las filas visibles en el viewport del ScrollContainer padre
		int i0 = 0;
		int i1 = rows.size();
		if (ScrollContainer *sc = Object::cast_to<ScrollContainer>(get_parent() ? get_parent()->get_parent() : nullptr)) {
			float sy = float(sc->get_v_scroll());
			float vh = MAX(row_height, sc->get_size().y);
			i0 = MAX(0, int(Math::floor(sy / step)) - 1);
			i1 = MIN(rows.size(), int(Math::ceil((sy + vh) / step)) + 1);
		}

		for (int i = i0; i < i1; i++) {
			const Row &r = rows[i];
			float y = float(i) * step;
			Rect2 eye_rc(0.0f, y, eye_w, row_height);
			Rect2 row_rc(eye_w + 4.0f * EDSCALE, y, MAX(20.0f, sz.x - eye_w - 4.0f * EDSCALE), row_height);

			Color eye_bg = r.visible ? Color(0.18f, 0.20f, 0.25f, 1.0f) : Color(0.12f, 0.13f, 0.16f, 0.9f);
			draw_rect(eye_rc, eye_bg, true);
			draw_rect(eye_rc, Color(0.32f, 0.36f, 0.44f, 0.9f), false, 1.0f);

			Color row_bg = r.selected ? Color(0.32f, 0.24f, 0.13f, 1.0f) : Color(0.18f, 0.20f, 0.25f, 1.0f);
			Color row_bd = r.selected ? ClockGearsTheme::color_accent_amber() : Color(0.32f, 0.36f, 0.44f, 0.9f);
			draw_rect(row_rc, row_bg, true);
			draw_rect(row_rc, row_bd, false, 1.0f);

			if (font.is_valid()) {
				float ty = y + row_height * 0.5f + float(fsz) * 0.35f;
				draw_string(font, Vector2(eye_rc.position.x + 5.0f * EDSCALE, ty), r.visible ? "Ver" : "---",
						HORIZONTAL_ALIGNMENT_LEFT, eye_w - 8.0f * EDSCALE, fsz, Color(0.92f, 0.92f, 0.94f));
				draw_string(font, Vector2(row_rc.position.x + 6.0f * EDSCALE, ty), r.label,
						HORIZONTAL_ALIGNMENT_LEFT, MAX(10.0f, row_rc.size.x - 12.0f * EDSCALE), fsz, Color(0.95f, 0.95f, 0.96f));
			}
		}
	}
}

void ClockGearsLayersCanvas::gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
		float sep = 4.0f * EDSCALE;
		float step = row_height + sep;
		int idx = int(Math::floor(mb->get_position().y / step));
		if (idx >= 0 && idx < rows.size() && owner_editor) {
			float eye_w = 34.0f * EDSCALE;
			if (mb->get_position().x <= eye_w) {
				owner_editor->toggle_element_visibility(rows[idx].elem_id);
			} else {
				owner_editor->select_element(rows[idx].elem_id);
			}
			accept_event();
		}
	}
}

// ============================================================================
// ClockGearsUiDesignEditor
// ============================================================================

ClockGearsUiDesignEditor::ClockGearsUiDesignEditor() {
	set_name("UiDesignEditorPanel");
	set_h_size_flags(Control::SIZE_EXPAND_FILL);
	set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_theme_constant_override("separation", int(10 * EDSCALE));
	set_process_input(true);
	set_process_shortcut_input(true);
	_build_ui();
}

ClockGearsUiDesignEditor::~ClockGearsUiDesignEditor() {
	thumb_tex_cache.clear();
	cached_action_sb_normal.unref();
	cached_action_sb_hover.unref();
	cached_action_sb_pressed.unref();
	if (test_window) {
		test_window->queue_free();
		test_window = nullptr;
	}
}

void ClockGearsUiDesignEditor::_apply_rounded_btn_style(Button *p_btn) {
	if (!p_btn) {
		return;
	}
	p_btn->set_clip_text(true);
	p_btn->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	p_btn->set_theme_type_variation("ClockGearsTabButton");
	if (cached_action_sb_normal.is_null()) {
		cached_action_sb_normal = ClockGearsTheme::make_card_stylebox(EDSCALE);
		cached_action_sb_normal->set_corner_radius_all(int(6 * EDSCALE));
		cached_action_sb_normal->set_content_margin_all(int(6 * EDSCALE));
		cached_action_sb_normal->set_bg_color(Color(0.18f, 0.20f, 0.25f, 1.0f));
		cached_action_sb_normal->set_border_color(Color(0.32f, 0.36f, 0.44f, 0.9f));

		cached_action_sb_hover = cached_action_sb_normal->duplicate();
		cached_action_sb_hover->set_bg_color(Color(0.24f, 0.27f, 0.34f, 1.0f));
		cached_action_sb_hover->set_border_color(ClockGearsTheme::color_accent_amber());

		cached_action_sb_pressed = cached_action_sb_normal->duplicate();
		cached_action_sb_pressed->set_bg_color(Color(0.32f, 0.24f, 0.13f, 1.0f));
		cached_action_sb_pressed->set_border_color(ClockGearsTheme::color_accent_amber());
	}
	p_btn->add_theme_style_override("normal", cached_action_sb_normal);
	p_btn->add_theme_style_override("hover", cached_action_sb_hover);
	p_btn->add_theme_style_override("pressed", cached_action_sb_pressed);
}

void ClockGearsUiDesignEditor::_bind_methods() {
	ADD_SIGNAL(MethodInfo("design_modified"));

	ClassDB::bind_method(D_METHOD("refresh_from_data_manager"), &ClockGearsUiDesignEditor::refresh_from_data_manager);
	ClassDB::bind_method(D_METHOD("adapt_to_width", "window_width"), &ClockGearsUiDesignEditor::adapt_to_width);
	ClassDB::bind_method(D_METHOD("select_screen", "screen_id"), &ClockGearsUiDesignEditor::select_screen);
	ClassDB::bind_method(D_METHOD("get_current_screen"), &ClockGearsUiDesignEditor::get_current_screen);
	ClassDB::bind_method(D_METHOD("set_props_advanced_mode", "advanced"), &ClockGearsUiDesignEditor::set_props_advanced_mode);
	ClassDB::bind_method(D_METHOD("is_props_advanced_mode"), &ClockGearsUiDesignEditor::is_props_advanced_mode);
	ClassDB::bind_method(D_METHOD("set_props_search_filter", "query"), &ClockGearsUiDesignEditor::set_props_search_filter);
	ClassDB::bind_method(D_METHOD("get_visible_prop_row_count"), &ClockGearsUiDesignEditor::get_visible_prop_row_count);
	ClassDB::bind_method(D_METHOD("create_menu_via_wizard", "opts"), &ClockGearsUiDesignEditor::create_menu_via_wizard);
	ClassDB::bind_method(D_METHOD("apply_template_by_id", "tpl_id", "preserve_buttons", "record_undo"), &ClockGearsUiDesignEditor::apply_template_by_id, DEFVAL(true), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("save_current_as_user_template", "name"), &ClockGearsUiDesignEditor::save_current_as_user_template);
	ClassDB::bind_method(D_METHOD("begin_inline_edit_for_selected"), &ClockGearsUiDesignEditor::begin_inline_edit_for_selected);
	ClassDB::bind_method(D_METHOD("show_canvas_context_menu", "screen_pos"), &ClockGearsUiDesignEditor::show_canvas_context_menu);
	ClassDB::bind_method(D_METHOD("align_selected_elements", "mode"), &ClockGearsUiDesignEditor::align_selected_elements);
	ClassDB::bind_method(D_METHOD("duplicate_selected_element"), &ClockGearsUiDesignEditor::duplicate_selected_element);
	ClassDB::bind_method(D_METHOD("copy_selected_to_clipboard"), &ClockGearsUiDesignEditor::copy_selected_to_clipboard);
	ClassDB::bind_method(D_METHOD("paste_from_clipboard"), &ClockGearsUiDesignEditor::paste_from_clipboard);
	ClassDB::bind_method(D_METHOD("set_radial_layout_live", "radius", "start_deg"), &ClockGearsUiDesignEditor::set_radial_layout_live);
	ClassDB::bind_method(D_METHOD("add_radial_slot_from_editor"), &ClockGearsUiDesignEditor::add_radial_slot_from_editor);
	ClassDB::bind_method(D_METHOD("update_dragged_element_fast", "elem_id", "elem"), &ClockGearsUiDesignEditor::update_dragged_element_fast);

	ClassDB::bind_method(D_METHOD("set_canvas_zoom", "zoom"), &ClockGearsUiDesignEditor::set_canvas_zoom);
	ClassDB::bind_method(D_METHOD("get_canvas_zoom"), &ClockGearsUiDesignEditor::get_canvas_zoom);
	ClassDB::bind_method(D_METHOD("set_preview_resolution", "width", "height", "sync_owner_db"), &ClockGearsUiDesignEditor::set_preview_resolution, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("get_preview_resolution"), &ClockGearsUiDesignEditor::get_preview_resolution);
	ClassDB::bind_method(D_METHOD("add_element_at", "type", "canvas_pos"), &ClockGearsUiDesignEditor::add_element_at, DEFVAL(Vector2i(-1, -1)));
	ClassDB::bind_method(D_METHOD("select_element", "elem_id"), &ClockGearsUiDesignEditor::select_element);
	ClassDB::bind_method(D_METHOD("get_selected_element_id"), &ClockGearsUiDesignEditor::get_selected_element_id);
	ClassDB::bind_method(D_METHOD("delete_selected_element"), &ClockGearsUiDesignEditor::delete_selected_element);
	ClassDB::bind_method(D_METHOD("toggle_element_visibility", "elem_id"), &ClockGearsUiDesignEditor::toggle_element_visibility);
	ClassDB::bind_method(D_METHOD("move_selected_layer", "delta"), &ClockGearsUiDesignEditor::move_selected_layer);
	ClassDB::bind_method(D_METHOD("apply_undo"), &ClockGearsUiDesignEditor::apply_undo);
	ClassDB::bind_method(D_METHOD("apply_redo"), &ClockGearsUiDesignEditor::apply_redo);
	ClassDB::bind_method(D_METHOD("request_reset_to_default", "confirmed_directly"), &ClockGearsUiDesignEditor::request_reset_to_default, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("open_test_popup"), &ClockGearsUiDesignEditor::open_test_popup);
	ClassDB::bind_method(D_METHOD("close_test_popup"), &ClockGearsUiDesignEditor::close_test_popup);
	ClassDB::bind_method(D_METHOD("get_canvas_design_layer"), &ClockGearsUiDesignEditor::get_canvas_design_layer);
	ClassDB::bind_method(D_METHOD("get_canvas_stage"), &ClockGearsUiDesignEditor::get_canvas_stage);
	ClassDB::bind_method(D_METHOD("get_canvas_overlay"), &ClockGearsUiDesignEditor::get_canvas_overlay);
	ClassDB::bind_method(D_METHOD("get_ui_handles"), &ClockGearsUiDesignEditor::get_ui_handles);
	ClassDB::bind_method(D_METHOD("get_test_popup_info"), &ClockGearsUiDesignEditor::get_test_popup_info);
}

void ClockGearsUiDesignEditor::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY || p_what == NOTIFICATION_VISIBILITY_CHANGED) {
		if (is_visible()) {
			refresh_from_data_manager();
			callable_mp(this, &ClockGearsUiDesignEditor::_update_canvas_fit_scale).call_deferred();
		}
	} else if (p_what == NOTIFICATION_RESIZED) {
		int w = int(get_size().x);
		if (get_window() && get_window()->get_size().x > 0) {
			w = int(get_window()->get_size().x);
		}
		if (w > 200) {
			_rebalance_responsive_layout(w);
		}
		_update_canvas_fit_scale();
	}
}

static Dictionary s_input_event_to_trigger(const Ref<InputEvent> &p_ev) {
	Dictionary tr;
	Ref<InputEventKey> k = p_ev;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		tr["kind"] = "key";
		tr["keycode"] = int(k->get_keycode() != Key::NONE ? k->get_keycode() : k->get_physical_keycode());
		return tr;
	}
	Ref<InputEventJoypadButton> jb = p_ev;
	if (jb.is_valid() && jb->is_pressed()) {
		tr["kind"] = "joypad";
		tr["button_index"] = int(jb->get_button_index());
		return tr;
	}
	return tr;
}

void ClockGearsUiDesignEditor::input(const Ref<InputEvent> &p_event) {
	if (!capturing_key_for_screen && !capturing_key_for_wizard) {
		return;
	}
	if (!is_visible_in_tree()) {
		capturing_key_for_screen = false;
		capturing_key_for_wizard = false;
		return;
	}
	Dictionary tr = s_input_event_to_trigger(p_event);
	if (tr.is_empty()) {
		return;
	}
	Ref<InputEventKey> ke = p_event;
	if (ke.is_valid() && ke->get_keycode() == Key::ESCAPE && capturing_key_for_wizard && wizard_triggers.size() > 0) {
		capturing_key_for_wizard = false;
		_update_wizard_step_ui();
		if (get_viewport()) get_viewport()->set_input_as_handled();
		return;
	}
	if (capturing_key_for_wizard) {
		capturing_key_for_wizard = false;
		wizard_triggers.clear();
		wizard_triggers.push_back(tr);
		_update_wizard_step_ui();
		_refresh_wizard_conflict_label();
	} else if (capturing_key_for_screen) {
		capturing_key_for_screen = false;
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			Dictionary d = dm->get_ui_screen_design(current_screen_id).duplicate(true);
			Array trs = d.get("triggers", Array());
			if (!trs.has(tr)) {
				trs.push_back(tr);
			}
			d["triggers"] = trs;
			dm->set_ui_screen_design(current_screen_id, d, true);
			_notify_data_mutated(true);
			_refresh_menu_global_properties();
		}
	}
	if (get_viewport()) {
		get_viewport()->set_input_as_handled();
	}
}

void ClockGearsUiDesignEditor::shortcut_input(const Ref<InputEvent> &p_event) {
	if (!is_visible_in_tree() || capturing_key_for_screen || capturing_key_for_wizard) {
		return;
	}
	if (inline_text_edit && inline_text_edit->is_visible()) {
		return;
	}
	// No interceptar atajos mientras se escribe en un campo de texto o número
	Control *focus = get_viewport() ? get_viewport()->gui_get_focus_owner() : nullptr;
	if (Object::cast_to<LineEdit>(focus) || Object::cast_to<TextEdit>(focus)) {
		return;
	}
	Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed() || k->is_echo()) {
		return;
	}
	Key kc = k->get_keycode();
	bool ctrl = k->is_ctrl_pressed() || k->is_command_or_control_pressed();
	if (ctrl && kc == Key::D) {
		if (duplicate_selected_element()) {
			if (get_viewport()) get_viewport()->set_input_as_handled();
		}
	} else if (ctrl && kc == Key::C) {
		copy_selected_to_clipboard();
		if (get_viewport()) get_viewport()->set_input_as_handled();
	} else if (ctrl && kc == Key::V) {
		paste_from_clipboard();
		if (get_viewport()) get_viewport()->set_input_as_handled();
	} else if (ctrl && kc == Key::Z && !k->is_shift_pressed()) {
		apply_undo();
		if (get_viewport()) get_viewport()->set_input_as_handled();
	} else if ((ctrl && kc == Key::Y) || (ctrl && k->is_shift_pressed() && kc == Key::Z)) {
		apply_redo();
		if (get_viewport()) get_viewport()->set_input_as_handled();
	} else if ((kc == Key::KEY_DELETE || kc == Key::BACKSPACE) && !selected_elem_id.is_empty()) {
		delete_selected_element();
		if (get_viewport()) get_viewport()->set_input_as_handled();
	} else if (!selected_elem_id.is_empty() && (kc == Key::LEFT || kc == Key::RIGHT || kc == Key::UP || kc == Key::DOWN)) {
		int step = k->is_shift_pressed() ? 8 : 1;
		int dx = (kc == Key::LEFT ? -step : (kc == Key::RIGHT ? step : 0));
		int dy = (kc == Key::UP ? -step : (kc == Key::DOWN ? step : 0));
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			Dictionary el = dm->get_ui_element(current_screen_id, selected_elem_id);
			if (!el.is_empty() && !bool(el.get("locked", false))) {
				el["offset_x"] = CLAMP(int(el.get("offset_x", 0)) + dx, -BASE_W, BASE_W);
				el["offset_y"] = CLAMP(int(el.get("offset_y", 0)) + dy, -BASE_H, BASE_H);
				dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
				refresh_geometry_fast(true);
				_notify_data_mutated(true);
				if (get_viewport()) get_viewport()->set_input_as_handled();
			}
		}
	}
}

Button *ClockGearsUiDesignEditor::_attach_collapsible_header(VBoxContainer *p_card_vb, const String &p_title, Control *p_body, bool p_expanded) {
	Button *hdr = memnew(Button(String::utf8(p_expanded ? "▾ " : "▸ ") + p_title));
	hdr->set_toggle_mode(true);
	hdr->set_pressed(p_expanded);
	hdr->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hdr->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	hdr->set_clip_text(true);
	hdr->set_custom_minimum_size(Size2(0, 28 * EDSCALE));
	hdr->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	hdr->add_theme_color_override("font_pressed_color", ClockGearsTheme::color_accent_amber());
	hdr->add_theme_color_override("font_hover_color", Color(1.0f, 0.85f, 0.45f));
	s_make_rounded_action_btn(hdr);
	p_card_vb->add_child(hdr);
	if (p_body) {
		p_body->set_visible(p_expanded);
		hdr->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_collapsible_section_toggled).bind(p_body, hdr, p_title));
	}
	return hdr;
}

void ClockGearsUiDesignEditor::_on_collapsible_section_toggled(bool p_expanded, Control *p_body, Button *p_btn, const String &p_title) {
	if (p_body) {
		p_body->set_visible(p_expanded);
	}
	if (p_btn) {
		p_btn->set_text(String::utf8(p_expanded ? "▾ " : "▸ ") + p_title);
	}
}

void ClockGearsUiDesignEditor::_on_preview_res_selected(int p_idx) {
	if (updating_ui || !preview_res_opt || p_idx < 0) {
		return;
	}
	Vector2i res = preview_res_opt->get_item_metadata(p_idx);
	if (res.x >= 320 && res.y >= 180) {
		set_preview_resolution(res.x, res.y, true);
	}
}

void ClockGearsUiDesignEditor::_update_canvas_fit_scale() {
	if (!canvas_stage || !canvas_fit_holder) {
		return;
	}
	Size2 stage_sz(float(preview_resolution.x * canvas_zoom), float(preview_resolution.y * canvas_zoom));
	if (canvas_zoom <= 1 && canvas_aspect && canvas_scroll) {
		float avail_w = canvas_scroll->get_size().x;
		if (avail_w >= 32.0f) {
			float max_h = 440.0f * EDSCALE;
			float target_w = MAX(160.0f * EDSCALE, MIN(avail_w, max_h * (16.0f / 9.0f)));
			float target_h = Math::round(target_w * (9.0f / 16.0f));
			Size2 target_sz(Math::round(target_w), target_h);
			if (canvas_aspect->get_custom_minimum_size().distance_to(target_sz) > 1.5f) {
				canvas_aspect->set_custom_minimum_size(target_sz);
				canvas_aspect->set_size(target_sz);
			}
		}
	}
	Size2 holder_sz = canvas_fit_holder->get_size();
	if (holder_sz.x < 16.0f || holder_sz.y < 16.0f) {
		return;
	}
	if (canvas_zoom <= 1) {
		float s = MIN(holder_sz.x / MAX(1.0f, stage_sz.x), holder_sz.y / MAX(1.0f, stage_sz.y));
		canvas_stage->set_scale(Vector2(s, s));
		canvas_stage->set_position((holder_sz - stage_sz * s) * 0.5f);
	} else {
		canvas_stage->set_scale(Vector2(1.0f, 1.0f));
		canvas_stage->set_position(Vector2(0, 0));
	}
}

void ClockGearsUiDesignEditor::_register_prop_filter_row(Control *p_row, const String &p_keywords, bool p_advanced_only) {
	if (!p_row) {
		return;
	}
	PropRowFilterEntry ent;
	ent.row = p_row;
	ent.search_text = p_keywords.to_lower();
	ent.advanced_only = p_advanced_only;
	prop_filter_entries.push_back(ent);
}

void ClockGearsUiDesignEditor::_build_ui() {
	const float row_h = 30.0f * EDSCALE;

	// --- Barra superior redondeada ---
	PanelContainer *top_card = memnew(PanelContainer);
	top_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	add_child(top_card);

	HFlowContainer *top_flow = memnew(HFlowContainer);
	top_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	top_flow->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	top_flow->add_theme_constant_override("v_separation", int(6 * EDSCALE));
	top_card->add_child(top_flow);

	top_bar = memnew(HBoxContainer);
	top_bar->add_theme_constant_override("separation", int(6 * EDSCALE));
	top_flow->add_child(top_bar);

	top_bar->add_child(memnew(Label(String::utf8("Pantalla:"))));
	screen_opt = memnew(OptionButton);
	screen_opt->set_custom_minimum_size(Size2(150 * EDSCALE, row_h));
	screen_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_screen_selected));
	top_bar->add_child(screen_opt);

	new_menu_wizard_btn = memnew(Button(String::utf8("+ Nuevo menú")));
	new_menu_wizard_btn->set_custom_minimum_size(Size2(0, row_h));
	new_menu_wizard_btn->set_tooltip_text(String::utf8("Abre el asistente guiado de 3 pasos para crear un menú nuevo."));
	s_make_rounded_action_btn(new_menu_wizard_btn);
	new_menu_wizard_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_open_menu_wizard));
	top_bar->add_child(new_menu_wizard_btn);

	templates_btn = memnew(Button(String::utf8("Plantillas…")));
	templates_btn->set_custom_minimum_size(Size2(0, row_h));
	templates_btn->set_tooltip_text(String::utf8("Explora plantillas de pantallas, componentes o tus diseños guardados."));
	s_make_rounded_action_btn(templates_btn);
	templates_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_templates_btn_pressed));
	top_bar->add_child(templates_btn);

	HBoxContainer *res_hb = memnew(HBoxContainer);
	res_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	top_flow->add_child(res_hb);
	res_hb->add_child(memnew(Label(String::utf8("Vista previa:"))));
	preview_res_opt = memnew(OptionButton);
	preview_res_opt->set_custom_minimum_size(Size2(140 * EDSCALE, row_h));
	preview_res_opt->add_item("960 x 540 (Base)", 0);
	preview_res_opt->set_item_metadata(0, Vector2i(960, 540));
	preview_res_opt->add_item("1366 x 768 (HD)", 1);
	preview_res_opt->set_item_metadata(1, Vector2i(1366, 768));
	preview_res_opt->add_item("1920 x 1080 (Full HD)", 2);
	preview_res_opt->set_item_metadata(2, Vector2i(1920, 1080));
	preview_res_opt->add_item("2560 x 1440 (QHD)", 3);
	preview_res_opt->set_item_metadata(3, Vector2i(2560, 1440));
	preview_res_opt->select(0);
	preview_res_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_preview_res_selected));
	res_hb->add_child(preview_res_opt);

	HBoxContainer *zoom_hb = memnew(HBoxContainer);
	zoom_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	top_flow->add_child(zoom_hb);
	zoom_hb->add_child(memnew(Label("Zoom:")));
	for (int z = 1; z <= 3; z++) {
		Button *zb = memnew(Button(itos(z) + "x"));
		zb->set_toggle_mode(true);
		zb->set_pressed(z == canvas_zoom);
		zb->set_custom_minimum_size(Size2(40 * EDSCALE, row_h));
		s_make_rounded_action_btn(zb);
		zb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_zoom_pressed).bind(z));
		zoom_hb->add_child(zb);
		zoom_btns[z - 1] = zb;
	}
	zoom_info_lbl = memnew(Label("960x540"));
	zoom_info_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	zoom_hb->add_child(zoom_info_lbl);

	HBoxContainer *act_hb = memnew(HBoxContainer);
	act_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	top_flow->add_child(act_hb);

	undo_btn = memnew(Button("Deshacer"));
	undo_btn->set_custom_minimum_size(Size2(0, row_h));
	s_make_rounded_action_btn(undo_btn);
	undo_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_undo_pressed));
	act_hb->add_child(undo_btn);

	redo_btn = memnew(Button("Rehacer"));
	redo_btn->set_custom_minimum_size(Size2(0, row_h));
	s_make_rounded_action_btn(redo_btn);
	redo_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_redo_pressed));
	act_hb->add_child(redo_btn);

	reset_btn = memnew(Button(String::utf8("Restaurar plantilla original")));
	reset_btn->set_custom_minimum_size(Size2(0, row_h));
	s_make_rounded_action_btn(reset_btn);
	reset_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_reset_pressed));
	act_hb->add_child(reset_btn);

	test_btn = memnew(Button("Probar"));
	test_btn->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
	test_btn->set_tooltip_text(String::utf8("Prueba el menú actual en tiempo real con datos de ejemplo."));
	s_make_rounded_action_btn(test_btn);
	test_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_pressed));
	act_hb->add_child(test_btn);

	screen_status_lbl = memnew(Label);
	screen_status_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	top_flow->add_child(screen_status_lbl);

	// --- Contenedores de distribución responsiva ---
	layout_root = memnew(VBoxContainer);
	layout_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	layout_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	layout_root->add_theme_constant_override("separation", int(10 * EDSCALE));
	add_child(layout_root);

	wide_hbox = memnew(HBoxContainer);
	wide_hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wide_hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_hbox->add_theme_constant_override("separation", int(10 * EDSCALE));
	layout_root->add_child(wide_hbox);

	wide_left_col = memnew(VBoxContainer);
	wide_left_col->set_custom_minimum_size(Size2(198 * EDSCALE, 0));
	wide_left_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_left_col->add_theme_constant_override("separation", int(8 * EDSCALE));
	wide_hbox->add_child(wide_left_col);

	wide_center_col = memnew(VBoxContainer);
	wide_center_col->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wide_center_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_center_col->set_stretch_ratio(1.5f);
	wide_hbox->add_child(wide_center_col);

	wide_right_col = memnew(VBoxContainer);
	wide_right_col->set_custom_minimum_size(Size2(284 * EDSCALE, 0));
	wide_right_col->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wide_right_col->add_theme_constant_override("separation", int(8 * EDSCALE));
	wide_hbox->add_child(wide_right_col);

	compact_vbox = memnew(VBoxContainer);
	compact_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	compact_vbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	compact_vbox->add_theme_constant_override("separation", int(8 * EDSCALE));
	compact_vbox->hide();
	layout_root->add_child(compact_vbox);

	// --- Tarjeta 0 (4A): Panel de Menús (sistema + personalizados) ---
	menus_card = memnew(PanelContainer);
	menus_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	wide_left_col->add_child(menus_card);

	VBoxContainer *menus_vb = memnew(VBoxContainer);
	menus_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_vb->add_theme_constant_override("separation", int(5 * EDSCALE));
	menus_card->add_child(menus_vb);

	VBoxContainer *menus_body = memnew(VBoxContainer);
	menus_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_body->add_theme_constant_override("separation", int(5 * EDSCALE));
	_attach_collapsible_header(menus_vb, String::utf8("Menús"), menus_body, true);
	menus_vb->add_child(menus_body);

	HFlowContainer *menu_ops = memnew(HFlowContainer);
	menu_ops->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_ops->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	menu_ops->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	menus_body->add_child(menu_ops);

	menu_dup_btn = memnew(Button("Duplicar"));
	s_make_rounded_action_btn(menu_dup_btn);
	menu_dup_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_list_dup_pressed));
	menu_ops->add_child(menu_dup_btn);

	menu_up_btn = memnew(Button("Subir"));
	s_make_rounded_action_btn(menu_up_btn);
	menu_up_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_list_move_pressed).bind(-1));
	menu_ops->add_child(menu_up_btn);

	menu_down_btn = memnew(Button("Bajar"));
	s_make_rounded_action_btn(menu_down_btn);
	menu_down_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_list_move_pressed).bind(1));
	menu_ops->add_child(menu_down_btn);

	menu_del_btn = memnew(Button("Borrar"));
	s_make_rounded_action_btn(menu_del_btn);
	menu_del_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_list_del_pressed));
	menu_ops->add_child(menu_del_btn);

	menu_restore_btn = memnew(Button("Restaurar"));
	menu_restore_btn->set_tooltip_text(String::utf8("Restaura la plantilla original en pantallas de sistema."));
	s_make_rounded_action_btn(menu_restore_btn);
	menu_restore_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_restore_pressed));
	menu_ops->add_child(menu_restore_btn);

	ScrollContainer *menus_scroll = memnew(ScrollContainer);
	menus_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_scroll->set_custom_minimum_size(Size2(0, 128 * EDSCALE));
	menus_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	menus_body->add_child(menus_scroll);

	menus_list_vbox = memnew(VBoxContainer);
	menus_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menus_list_vbox->add_theme_constant_override("separation", int(3 * EDSCALE));
	menus_scroll->add_child(menus_list_vbox);

	// --- Tarjeta 1: Paleta lateral (plegable) ---
	palette_card = memnew(PanelContainer);
	palette_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	palette_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	wide_left_col->add_child(palette_card);

	VBoxContainer *pal_vb = memnew(VBoxContainer);
	pal_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	pal_vb->add_theme_constant_override("separation", int(6 * EDSCALE));
	palette_card->add_child(pal_vb);

	VBoxContainer *pal_body = memnew(VBoxContainer);
	pal_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	_attach_collapsible_header(pal_vb, "Elementos (arrastrar)", pal_body, true);
	pal_vb->add_child(pal_body);

	palette_grid = memnew(GridContainer);
	palette_grid->set_columns(1);
	palette_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	palette_grid->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	palette_grid->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	pal_body->add_child(palette_grid);

	struct PaletteDef {
		const char *type;
		const char *label_utf8;
	};
	static const PaletteDef k_palette[9] = {
		{ "image", "Imagen" },
		{ "text", "Texto" },
		{ "button", "Bot\xC3\xB3n" },
		{ "frame", "Marco" },
		{ "bar", "Barra" },
		{ "hearts", "Corazones" },
		{ "currency", "Monedas" },
		{ "item", "Contador de objetos" },
		{ "variable", "Variable" }
	};
	for (int i = 0; i < 9; i++) {
		ClockGearsUiPaletteButton *pb = memnew(ClockGearsUiPaletteButton);
		pb->setup(this, String(k_palette[i].type), String::utf8(k_palette[i].label_utf8));
		palette_grid->add_child(pb);
		palette_buttons.push_back(pb);
	}

	// --- Tarjeta 2: Lienzo 960x540 usando ClockGearsUiDesignLayer ---
	canvas_card = memnew(PanelContainer);
	canvas_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	wide_center_col->add_child(canvas_card);

	VBoxContainer *can_vb = memnew(VBoxContainer);
	can_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	can_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	can_vb->add_theme_constant_override("separation", int(6 * EDSCALE));
	canvas_card->add_child(can_vb);

	HFlowContainer *can_hdr_flow = memnew(HFlowContainer);
	can_hdr_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	can_hdr_flow->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	can_hdr_flow->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	can_vb->add_child(can_hdr_flow);

	Label *can_hdr = memnew(Label(String::utf8("Lienzo 16:9 escalado")));
	can_hdr->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	can_hdr_flow->add_child(can_hdr);

	grid_chk = memnew(CheckBox("Rejilla"));
	grid_chk->set_pressed(false);
	can_hdr_flow->add_child(grid_chk);

	snap_chk = memnew(CheckBox(String::utf8("Imán")));
	snap_chk->set_pressed(false);
	can_hdr_flow->add_child(snap_chk);

	guides_chk = memnew(CheckBox(String::utf8("Guías")));
	guides_chk->set_pressed(true);
	can_hdr_flow->add_child(guides_chk);

	align_center_h_btn = memnew(Button("Centrar H"));
	s_make_rounded_action_btn(align_center_h_btn);
	align_center_h_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_align_center_pressed).bind(false));
	can_hdr_flow->add_child(align_center_h_btn);

	align_center_v_btn = memnew(Button("Centrar V"));
	s_make_rounded_action_btn(align_center_v_btn);
	align_center_v_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_align_center_pressed).bind(true));
	can_hdr_flow->add_child(align_center_v_btn);

	dist_v_btn = memnew(Button("Distribuir V"));
	s_make_rounded_action_btn(dist_v_btn);
	dist_v_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_distribute_vertical_pressed));
	can_hdr_flow->add_child(dist_v_btn);

	dup_elem_btn = memnew(Button("Duplicar"));
	s_make_rounded_action_btn(dup_elem_btn);
	dup_elem_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_duplicate_elem_pressed));
	can_hdr_flow->add_child(dup_elem_btn);

	copy_elem_btn = memnew(Button("Copiar"));
	s_make_rounded_action_btn(copy_elem_btn);
	copy_elem_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_copy_elem_pressed));
	can_hdr_flow->add_child(copy_elem_btn);

	paste_elem_btn = memnew(Button("Pegar"));
	s_make_rounded_action_btn(paste_elem_btn);
	paste_elem_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_paste_elem_pressed));
	can_hdr_flow->add_child(paste_elem_btn);

	canvas_scroll = memnew(ScrollContainer);
	canvas_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	canvas_scroll->set_custom_minimum_size(Size2(160 * EDSCALE, 90 * EDSCALE));
	canvas_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	canvas_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	can_vb->add_child(canvas_scroll);

	canvas_aspect = memnew(AspectRatioContainer);
	canvas_aspect->set_ratio(16.0f / 9.0f);
	canvas_aspect->set_stretch_mode(AspectRatioContainer::STRETCH_FIT);
	canvas_aspect->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	canvas_aspect->set_custom_minimum_size(Size2(320 * EDSCALE, 180 * EDSCALE));
	canvas_scroll->add_child(canvas_aspect);
	canvas_scroll->connect("resized", callable_mp(this, &ClockGearsUiDesignEditor::_update_canvas_fit_scale));

	canvas_fit_holder = memnew(Control);
	canvas_fit_holder->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	canvas_fit_holder->set_clip_contents(true);
	canvas_fit_holder->connect("resized", callable_mp(this, &ClockGearsUiDesignEditor::_update_canvas_fit_scale));
	canvas_aspect->add_child(canvas_fit_holder);

	canvas_stage = memnew(Control);
	canvas_stage->set_name("UiDesignerCanvasStage");
	canvas_stage->set_custom_minimum_size(Size2(BASE_W, BASE_H));
	canvas_stage->set_size(Size2(BASE_W, BASE_H));
	canvas_stage->set_clip_contents(true);
	canvas_fit_holder->add_child(canvas_stage);

	canvas_bg_color = memnew(ColorRect);
	canvas_bg_color->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	canvas_bg_color->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	canvas_bg_color->set_color(Color::html("#12151e"));
	canvas_stage->add_child(canvas_bg_color);

	canvas_bg_tex = memnew(TextureRect);
	canvas_bg_tex->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	canvas_bg_tex->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	canvas_bg_tex->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	canvas_bg_tex->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	canvas_stage->add_child(canvas_bg_tex);

	canvas_hud_grid_bg = memnew(UiCanvasHudGridBackdrop);
	canvas_hud_grid_bg->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	canvas_hud_grid_bg->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	canvas_hud_grid_bg->hide();
	canvas_stage->add_child(canvas_hud_grid_bg);

	design_layer = memnew(ClockGearsUiDesignLayer);
	design_layer->set_preview_mode(true);
	design_layer->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	design_layer->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	design_layer->set_viewport_size_override(Vector2(BASE_W, BASE_H));
	canvas_stage->add_child(design_layer);

	canvas_overlay = memnew(ClockGearsUiCanvasOverlay);
	canvas_overlay->set_name("UiDesignerCanvasOverlay");
	canvas_overlay->set_owner_editor(this);
	canvas_overlay->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	canvas_overlay->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	canvas_stage->add_child(canvas_overlay);

	grid_chk->connect("toggled", callable_mp(canvas_overlay, &ClockGearsUiCanvasOverlay::set_grid_overlay_enabled));
	snap_chk->connect("toggled", callable_mp(canvas_overlay, &ClockGearsUiCanvasOverlay::set_snap_enabled));
	guides_chk->connect("toggled", callable_mp(canvas_overlay, &ClockGearsUiCanvasOverlay::set_smart_guides_enabled));

	inline_text_edit = memnew(LineEdit);
	inline_text_edit->hide();
	inline_text_edit->connect("text_submitted", callable_mp(this, &ClockGearsUiDesignEditor::_commit_inline_text_edit).unbind(1));
	inline_text_edit->connect("focus_exited", callable_mp(this, &ClockGearsUiDesignEditor::_commit_inline_text_edit));
	canvas_stage->add_child(inline_text_edit);

	canvas_context_menu = memnew(PopupMenu);
	canvas_context_menu->add_item("Duplicar (Ctrl+D)", 1);
	canvas_context_menu->add_item("Copiar (Ctrl+C)", 2);
	canvas_context_menu->add_item("Pegar (Ctrl+V)", 3);
	canvas_context_menu->add_separator();
	canvas_context_menu->add_item("Subir capa", 4);
	canvas_context_menu->add_item("Bajar capa", 5);
	canvas_context_menu->add_item("Centrar en pantalla", 6);
	canvas_context_menu->add_item("Guardar como mi plantilla de componente", 7);
	canvas_context_menu->add_separator();
	canvas_context_menu->add_item("Borrar (Supr)", 8);
	canvas_context_menu->connect("id_pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_canvas_context_menu_id));
	add_child(canvas_context_menu);

	// --- Tarjeta 3: Lista de capas con ojo y orden (plegable) ---
	layers_card = memnew(PanelContainer);
	layers_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	layers_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	layers_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	wide_left_col->add_child(layers_card);

	VBoxContainer *lay_vb = memnew(VBoxContainer);
	lay_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lay_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	lay_vb->add_theme_constant_override("separation", int(6 * EDSCALE));
	layers_card->add_child(lay_vb);

	VBoxContainer *lay_body = memnew(VBoxContainer);
	lay_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lay_body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	lay_body->add_theme_constant_override("separation", int(6 * EDSCALE));
	_attach_collapsible_header(lay_vb, "Capas", lay_body, true);
	lay_vb->add_child(lay_body);

	HFlowContainer *lay_btns = memnew(HFlowContainer);
	lay_btns->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lay_btns->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	lay_btns->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	lay_body->add_child(lay_btns);

	layer_up_btn = memnew(Button(String::utf8("Subir")));
	s_make_rounded_action_btn(layer_up_btn);
	layer_up_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_move_pressed).bind(1));
	lay_btns->add_child(layer_up_btn);

	layer_down_btn = memnew(Button(String::utf8("Bajar")));
	s_make_rounded_action_btn(layer_down_btn);
	layer_down_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_move_pressed).bind(-1));
	lay_btns->add_child(layer_down_btn);

	layer_del_btn = memnew(Button("Borrar"));
	s_make_rounded_action_btn(layer_del_btn);
	layer_del_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_layer_delete_pressed));
	lay_btns->add_child(layer_del_btn);

	ScrollContainer *lay_scroll = memnew(ScrollContainer);
	lay_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	lay_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	lay_scroll->set_custom_minimum_size(Size2(0, 120 * EDSCALE));
	lay_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	lay_body->add_child(lay_scroll);

	layers_list_vbox = memnew(VBoxContainer);
	layers_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	layers_list_vbox->add_theme_constant_override("separation", int(4 * EDSCALE));
	lay_scroll->add_child(layers_list_vbox);

	layers_long_canvas = memnew(ClockGearsLayersCanvas);
	layers_long_canvas->set_owner_editor(this);
	layers_long_canvas->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	layers_long_canvas->hide();
	lay_body->add_child(layers_long_canvas);
	if (lay_scroll->get_v_scroll_bar()) {
		lay_scroll->get_v_scroll_bar()->connect("value_changed", callable_mp(static_cast<CanvasItem *>(layers_long_canvas), &CanvasItem::queue_redraw).unbind(1));
	}

	// --- Tarjeta 4: Panel de propiedades (4C: Simple/Avanzado, Buscador, Secciones) ---
	props_card = memnew(PanelContainer);
	props_card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_card->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	props_card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
	wide_right_col->add_child(props_card);

	VBoxContainer *props_outer = memnew(VBoxContainer);
	props_outer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_outer->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	props_outer->add_theme_constant_override("separation", int(6 * EDSCALE));
	props_card->add_child(props_outer);

	VBoxContainer *props_body = memnew(VBoxContainer);
	props_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	props_body->add_theme_constant_override("separation", int(6 * EDSCALE));
	_attach_collapsible_header(props_outer, "Propiedades", props_body, true);
	props_outer->add_child(props_body);

	HBoxContainer *mode_hb = memnew(HBoxContainer);
	mode_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	props_body->add_child(mode_hb);

	props_mode_simple_btn = memnew(Button("Simple"));
	props_mode_simple_btn->set_toggle_mode(true);
	props_mode_simple_btn->set_pressed(true);
	props_mode_simple_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s_make_rounded_action_btn(props_mode_simple_btn);
	props_mode_simple_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_props_mode_changed).bind(false));
	mode_hb->add_child(props_mode_simple_btn);

	props_mode_adv_btn = memnew(Button("Avanzado"));
	props_mode_adv_btn->set_toggle_mode(true);
	props_mode_adv_btn->set_pressed(false);
	props_mode_adv_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	s_make_rounded_action_btn(props_mode_adv_btn);
	props_mode_adv_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_props_mode_changed).bind(true));
	mode_hb->add_child(props_mode_adv_btn);

	props_search_edit = memnew(LineEdit);
	props_search_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_search_edit->set_placeholder("Buscar propiedad...");
	props_search_edit->set_clear_button_enabled(true);
	props_search_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_props_search_changed));
	props_body->add_child(props_search_edit);

	props_sel_lbl = memnew(Label(String::utf8("Selecciona un elemento o ajusta el menú abajo.")));
	props_sel_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	props_sel_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_text_secondary());
	props_body->add_child(props_sel_lbl);

	ScrollContainer *props_scroll = memnew(ScrollContainer);
	props_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	props_scroll->set_custom_minimum_size(Size2(160 * EDSCALE, 220 * EDSCALE));
	props_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	props_body->add_child(props_scroll);

	VBoxContainer *props_scroll_vb = memnew(VBoxContainer);
	props_scroll_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_scroll_vb->add_theme_constant_override("separation", int(8 * EDSCALE));
	props_scroll->add_child(props_scroll_vb);

	props_fields_box = memnew(VBoxContainer);
	props_fields_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	props_fields_box->add_theme_constant_override("separation", int(6 * EDSCALE));
	props_fields_box->hide();
	props_scroll_vb->add_child(props_fields_box);

	auto make_prop_row = [&](VBoxContainer *p_parent, const String &p_lbl, const String &p_tip, const String &p_kw, bool p_adv, Control *p_ctrl) {
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hb->add_theme_constant_override("separation", int(6 * EDSCALE));
		Label *l = memnew(Label(p_lbl));
		l->set_custom_minimum_size(Size2(86 * EDSCALE, 0));
		l->set_tooltip_text(p_tip);
		hb->add_child(l);
		p_ctrl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		p_ctrl->set_tooltip_text(p_tip);
		hb->add_child(p_ctrl);
		p_parent->add_child(hb);
		_register_prop_filter_row(hb, p_lbl + " " + p_kw, p_adv);
	};

	prop_name_edit = memnew(LineEdit);
	prop_name_edit->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	prop_name_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(props_fields_box, "Nombre:", String::utf8("Nombre visible del elemento en capas y diseño."), "nombre texto", false, prop_name_edit);

	HBoxContainer *anc_row = memnew(HBoxContainer);
	anc_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	Label *anc_lbl = memnew(Label("Anclaje:"));
	anc_lbl->set_custom_minimum_size(Size2(86 * EDSCALE, 0));
	anc_row->add_child(anc_lbl);
	GridContainer *anc_grid = memnew(GridContainer);
	anc_grid->set_columns(3);
	anc_grid->add_theme_constant_override("h_separation", int(3 * EDSCALE));
	anc_grid->add_theme_constant_override("v_separation", int(3 * EDSCALE));
	anc_row->add_child(anc_grid);
	props_fields_box->add_child(anc_row);
	_register_prop_filter_row(anc_row, "anclaje posicion esquina centro", false);

	static const char *k_anc_glyphs[9] = { "NO", "N", "NE", "O", "C", "E", "SO", "S", "SE" };
	for (int i = 0; i < 9; i++) {
		Button *ab = memnew(Button(k_anc_glyphs[i]));
		ab->set_toggle_mode(true);
		ab->set_custom_minimum_size(Size2(32 * EDSCALE, 24 * EDSCALE));
		s_make_rounded_action_btn(ab);
		ab->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_anchor_btn_pressed).bind(i));
		anc_grid->add_child(ab);
		anchor_btns[i] = ab;
	}

	auto make_spin = [&](VBoxContainer *p_parent, const String &p_lbl, const String &p_tip, const String &p_kw, bool p_adv, int p_min, int p_max, SpinBox *&r_sp) {
		r_sp = memnew(SpinBox);
		r_sp->set_min(p_min);
		r_sp->set_max(p_max);
		r_sp->set_step(1);
		r_sp->set_custom_minimum_size(Size2(80 * EDSCALE, row_h));
		r_sp->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
		make_prop_row(p_parent, p_lbl, p_tip, p_kw, p_adv, r_sp);
	};

	make_spin(props_fields_box, "Pos X:", String::utf8("Desplazamiento horizontal desde su anclaje."), "x posicion mover", false, -960, 960, prop_ox_spin);
	make_spin(props_fields_box, "Pos Y:", String::utf8("Desplazamiento vertical desde su anclaje."), "y posicion mover", false, -540, 540, prop_oy_spin);
	make_spin(props_fields_box, "Ancho:", String::utf8("Anchura en píxeles base 960x540."), "ancho tamano width", false, 16, 960, prop_w_spin);
	make_spin(props_fields_box, "Alto:", String::utf8("Altura en píxeles base 960x540."), "alto tamano height", false, 16, 540, prop_h_spin);
	make_spin(props_fields_box, "Capa:", String::utf8("Orden de dibujo (mayor aparece encima)."), "capa orden profundidad", true, -100, 1000, prop_layer_spin);

	prop_visible_chk = memnew(CheckBox("Mostrar en juego"));
	prop_visible_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(props_fields_box, "Visible:", String::utf8("Muestra u oculta el elemento."), "visible ocultar mostrar", false, prop_visible_chk);

	prop_show_bg_chk = memnew(CheckBox("Fondo"));
	prop_show_bg_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(props_fields_box, "Mostrar fondo:", String::utf8("Activa el fondo o caja del componente."), "fondo panel", false, prop_show_bg_chk);

	prop_style_opt = memnew(OptionButton);
	prop_style_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	prop_style_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(props_fields_box, "Estilo:", String::utf8("Estilo de ventana o marco del tema."), "estilo ventana marco", false, prop_style_opt);

	// Sección Forma, Color y Efectos (3F / 4C)
	VBoxContainer *shape_body = memnew(VBoxContainer);
	shape_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	shape_body->add_theme_constant_override("separation", int(5 * EDSCALE));
	_attach_collapsible_header(props_fields_box, "Forma, color y borde", shape_body, true);
	props_fields_box->add_child(shape_body);

	prop_shape_opt = memnew(OptionButton);
	prop_shape_opt->add_item(String::utf8("Rectángulo"), 0); prop_shape_opt->set_item_metadata(0, "rect");
	prop_shape_opt->add_item("Redondeado", 1); prop_shape_opt->set_item_metadata(1, "rounded");
	prop_shape_opt->add_item(String::utf8("Píldora"), 2); prop_shape_opt->set_item_metadata(2, "pill");
	prop_shape_opt->add_item(String::utf8("Círculo"), 3); prop_shape_opt->set_item_metadata(3, "circle");
	prop_shape_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Forma:", String::utf8("Silueta de la caja o botón."), "forma circulo pildora esquina", false, prop_shape_opt);

	HBoxContainer *rad_hb = memnew(HBoxContainer);
	prop_radius_slider = memnew(HSlider);
	prop_radius_slider->set_min(0);
	prop_radius_slider->set_max(64);
	prop_radius_slider->set_step(1);
	prop_radius_slider->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_radius_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	rad_hb->add_child(prop_radius_slider);
	prop_radius_val_lbl = memnew(Label("8px"));
	prop_radius_val_lbl->set_custom_minimum_size(Size2(34 * EDSCALE, 0));
	rad_hb->add_child(prop_radius_val_lbl);
	make_prop_row(shape_body, "Radio:", String::utf8("Redondeo instantáneo de esquinas."), "radio esquina redondeo", false, rad_hb);

	prop_fill_color_btn = memnew(ColorPickerButton);
	prop_fill_color_btn->set_custom_minimum_size(Size2(64 * EDSCALE, row_h));
	prop_fill_color_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Color relleno:", String::utf8("Color principal del fondo del elemento."), "color relleno fondo", false, prop_fill_color_btn);

	prop_recent_colors_box = memnew(HBoxContainer);
	prop_recent_colors_box->add_theme_constant_override("separation", int(3 * EDSCALE));
	make_prop_row(shape_body, "Recientes:", String::utf8("Últimos colores usados en el proyecto."), "paleta colores recientes", false, prop_recent_colors_box);

	prop_grad_chk = memnew(CheckBox("Degradado vertical"));
	prop_grad_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Degradado:", String::utf8("Activa degradado superior/inferior."), "degradado gradiente", true, prop_grad_chk);

	HBoxContainer *grad_cols = memnew(HBoxContainer);
	grad_cols->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_grad_top_btn = memnew(ColorPickerButton);
	prop_grad_top_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_grad_top_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	grad_cols->add_child(prop_grad_top_btn);
	prop_grad_bot_btn = memnew(ColorPickerButton);
	prop_grad_bot_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_grad_bot_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	grad_cols->add_child(prop_grad_bot_btn);
	make_prop_row(shape_body, "Colores degrad.:", String::utf8("Colores superior e inferior del degradado."), "degradado colores", true, grad_cols);

	prop_border_w_slider = memnew(HSlider);
	prop_border_w_slider->set_min(0);
	prop_border_w_slider->set_max(12);
	prop_border_w_slider->set_step(1);
	prop_border_w_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Grosor borde:", String::utf8("Anchura del borde en píxeles."), "borde grosor", true, prop_border_w_slider);

	prop_border_col_btn = memnew(ColorPickerButton);
	prop_border_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Color borde:", String::utf8("Color del borde del elemento."), "color borde", true, prop_border_col_btn);

	HBoxContainer *fx_hb = memnew(HBoxContainer);
	prop_shadow_chk = memnew(CheckBox("Sombra"));
	prop_shadow_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	fx_hb->add_child(prop_shadow_chk);
	prop_glow_chk = memnew(CheckBox("Brillo"));
	prop_glow_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	fx_hb->add_child(prop_glow_chk);
	prop_glow_col_btn = memnew(ColorPickerButton);
	prop_glow_col_btn->set_custom_minimum_size(Size2(34 * EDSCALE, 24 * EDSCALE));
	prop_glow_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	fx_hb->add_child(prop_glow_col_btn);
	make_prop_row(shape_body, "Efectos:", String::utf8("Sombra suave y brillo exterior."), "sombra brillo glow", true, fx_hb);

	prop_clip_chk = memnew(CheckBox("Recortar contenido"));
	prop_clip_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Recorte:", String::utf8("Recorta hijos o contenido a la silueta."), "recorte mascara clip", true, prop_clip_chk);

	prop_pixel_art_chk = memnew(CheckBox("Pixelado (Nearest)"));
	prop_pixel_art_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(shape_body, "Filtro imagen:", String::utf8("Pixelado nítido o suave interpolado."), "pixel art filtro imagen", true, prop_pixel_art_chk);

	// Texto UTF-8 y Fuente (.ttf/.otf)
	prop_text_row = memnew(VBoxContainer);
	prop_text_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	props_fields_box->add_child(prop_text_row);
	prop_text_row->add_child(memnew(Label("Texto / plantilla ({value}):")));
	prop_text_edit = memnew(LineEdit);
	prop_text_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_text_edit->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	prop_text_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	prop_text_row->add_child(prop_text_edit);
	_register_prop_filter_row(prop_text_row, "texto etiqueta titulo fuente", false);

	HBoxContainer *fnt_hb = memnew(HBoxContainer);
	fnt_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_font_opt = memnew(OptionButton);
	prop_font_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_font_opt->add_item("(Fuente por defecto)", 0);
	prop_font_opt->set_item_metadata(0, "");
	prop_font_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	fnt_hb->add_child(prop_font_opt);
	prop_font_import_btn = memnew(Button("+ TTF"));
	s_make_rounded_action_btn(prop_font_import_btn);
	prop_font_import_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_font_import_pressed));
	fnt_hb->add_child(prop_font_import_btn);
	make_prop_row(prop_text_row, "Fuente:", String::utf8("Fuente importada (.ttf/.otf) para el texto."), "fuente ttf otf tipografia", true, fnt_hb);

	prop_font_size_slider = memnew(HSlider);
	prop_font_size_slider->set_min(10);
	prop_font_size_slider->set_max(64);
	prop_font_size_slider->set_step(1);
	prop_font_size_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(prop_text_row, String::utf8("Tamaño fuente:"), String::utf8("Tamaño de letra en píxeles."), "tamano fuente letra", true, prop_font_size_slider);

	prop_font_color_btn = memnew(ColorPickerButton);
	prop_font_color_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	make_prop_row(prop_text_row, "Color texto:", String::utf8("Color del texto."), "color texto fuente", true, prop_font_color_btn);

	// Imagen (importar + galería rápida)
	prop_image_row = memnew(VBoxContainer);
	prop_image_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	props_fields_box->add_child(prop_image_row);
	prop_image_row->add_child(memnew(Label("Imagen:")));
	HBoxContainer *im_hb = memnew(HBoxContainer);
	im_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_image_row->add_child(im_hb);
	prop_image_edit = memnew(LineEdit);
	prop_image_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_image_edit->set_placeholder("res://img/ui/...");
	prop_image_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	im_hb->add_child(prop_image_edit);
	prop_image_import_btn = memnew(Button("Importar"));
	s_make_rounded_action_btn(prop_image_import_btn);
	prop_image_import_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_import_image_pressed));
	im_hb->add_child(prop_image_import_btn);
	prop_image_clear_btn = memnew(Button("Quitar"));
	s_make_rounded_action_btn(prop_image_clear_btn);
	prop_image_clear_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_clear_image_pressed));
	im_hb->add_child(prop_image_clear_btn);

	prop_img_thumbs_flow = memnew(HFlowContainer);
	prop_img_thumbs_flow->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_img_thumbs_flow->add_theme_constant_override("h_separation", int(4 * EDSCALE));
	prop_img_thumbs_flow->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	prop_image_row->add_child(prop_img_thumbs_flow);
	_register_prop_filter_row(prop_image_row, "imagen textura sprite icono", false);

	// Enlace de datos (desde catálogo)
	prop_binding_row = memnew(VBoxContainer);
	prop_binding_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	props_fields_box->add_child(prop_binding_row);
	prop_binding_row->add_child(memnew(Label("Enlace de datos:")));
	prop_binding_opt = memnew(OptionButton);
	prop_binding_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_binding_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	prop_binding_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_binding_changed));
	prop_binding_row->add_child(prop_binding_opt);

	prop_var_row = memnew(HBoxContainer);
	prop_var_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_binding_row->add_child(prop_var_row);
	prop_var_opt = memnew(OptionButton);
	prop_var_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_var_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_var_opt_selected));
	prop_var_row->add_child(prop_var_opt);
	prop_var_edit = memnew(LineEdit);
	prop_var_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_var_edit->set_placeholder("variable");
	prop_var_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	prop_var_row->add_child(prop_var_edit);

	prop_item_row = memnew(HBoxContainer);
	prop_item_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_binding_row->add_child(prop_item_row);
	prop_item_row->add_child(memnew(Label("Objeto:")));
	prop_item_opt = memnew(OptionButton);
	prop_item_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_item_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_item_opt_selected));
	prop_item_row->add_child(prop_item_opt);
	_register_prop_filter_row(prop_binding_row, "enlace datos vida mana oro nivel variable objeto", false);

	// Acción de botón + secuencia multi-acción + foco + condiciones por interruptor (3D)
	prop_action_row = memnew(VBoxContainer);
	prop_action_row->add_theme_constant_override("separation", int(4 * EDSCALE));
	props_fields_box->add_child(prop_action_row);
	prop_action_row->add_child(memnew(Label(String::utf8("Acción principal del botón:"))));
	prop_action_opt = memnew(OptionButton);
	prop_action_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_action_opt->set_custom_minimum_size(Size2(90 * EDSCALE, row_h));
	prop_action_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_action_changed));
	prop_action_row->add_child(prop_action_opt);

	HBoxContainer *fo_hb = memnew(HBoxContainer);
	fo_hb->add_theme_constant_override("separation", int(6 * EDSCALE));
	prop_action_row->add_child(fo_hb);
	fo_hb->add_child(memnew(Label("Orden foco:")));
	prop_focus_spin = memnew(SpinBox);
	prop_focus_spin->set_min(0);
	prop_focus_spin->set_max(9999);
	prop_focus_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_focus_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	fo_hb->add_child(prop_focus_spin);

	prop_multi_actions_vbox = memnew(VBoxContainer);
	prop_multi_actions_vbox->add_theme_constant_override("separation", int(3 * EDSCALE));
	prop_action_row->add_child(prop_multi_actions_vbox);

	HBoxContainer *add_act_hb = memnew(HBoxContainer);
	add_act_hb->add_theme_constant_override("separation", int(4 * EDSCALE));
	prop_action_row->add_child(add_act_hb);
	prop_add_action_opt = memnew(OptionButton);
	prop_add_action_opt->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_act_hb->add_child(prop_add_action_opt);
	prop_add_action_btn = memnew(Button(String::utf8("+ Acción")));
	s_make_rounded_action_btn(prop_add_action_btn);
	prop_add_action_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_add_multi_action_pressed));
	add_act_hb->add_child(prop_add_action_btn);

	HBoxContainer *vis_sw_hb = memnew(HBoxContainer);
	prop_vis_mode_opt = memnew(OptionButton);
	prop_vis_mode_opt->add_item("Siempre", 0); prop_vis_mode_opt->set_item_metadata(0, "always");
	prop_vis_mode_opt->add_item("Si ON", 1); prop_vis_mode_opt->set_item_metadata(1, "switch_on");
	prop_vis_mode_opt->add_item("Si OFF", 2); prop_vis_mode_opt->set_item_metadata(2, "switch_off");
	prop_vis_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	vis_sw_hb->add_child(prop_vis_mode_opt);
	prop_vis_sw_edit = memnew(LineEdit);
	prop_vis_sw_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_vis_sw_edit->set_placeholder("interruptor");
	prop_vis_sw_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	vis_sw_hb->add_child(prop_vis_sw_edit);
	make_prop_row(prop_action_row, "Cond. visible:", String::utf8("Visibilidad según interruptor del juego."), "condicion interruptor visible", true, vis_sw_hb);

	HBoxContainer *en_sw_hb = memnew(HBoxContainer);
	prop_en_mode_opt = memnew(OptionButton);
	prop_en_mode_opt->add_item("Siempre", 0); prop_en_mode_opt->set_item_metadata(0, "always");
	prop_en_mode_opt->add_item("Si ON", 1); prop_en_mode_opt->set_item_metadata(1, "switch_on");
	prop_en_mode_opt->add_item("Si OFF", 2); prop_en_mode_opt->set_item_metadata(2, "switch_off");
	prop_en_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	en_sw_hb->add_child(prop_en_mode_opt);
	prop_en_sw_edit = memnew(LineEdit);
	prop_en_sw_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	prop_en_sw_edit->set_placeholder("interruptor");
	prop_en_sw_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed).unbind(1));
	en_sw_hb->add_child(prop_en_sw_edit);
	make_prop_row(prop_action_row, "Cond. activo:", String::utf8("Habilita o desactiva el botón según interruptor."), "condicion interruptor activo", true, en_sw_hb);
	_register_prop_filter_row(prop_action_row, "accion boton foco secuencia evento", false);

	// --- Sección Global del Menú actual (4F / 3B / 3E - siempre disponible) ---
	menu_props_box = memnew(VBoxContainer);
	menu_props_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_props_box->add_theme_constant_override("separation", int(5 * EDSCALE));
	props_scroll_vb->add_child(menu_props_box);

	VBoxContainer *bg_body = memnew(VBoxContainer);
	bg_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	bg_body->add_theme_constant_override("separation", int(4 * EDSCALE));
	_attach_collapsible_header(menu_props_box, String::utf8("Fondo y transición del menú"), bg_body, false);
	menu_props_box->add_child(bg_body);

	menu_bg_type_opt = memnew(OptionButton);
	menu_bg_type_opt->add_item("Por defecto", 0); menu_bg_type_opt->set_item_metadata(0, "default");
	menu_bg_type_opt->add_item(String::utf8("Color sólido"), 1); menu_bg_type_opt->set_item_metadata(1, "color");
	menu_bg_type_opt->add_item("Degradado", 2); menu_bg_type_opt->set_item_metadata(2, "gradient");
	menu_bg_type_opt->add_item("Imagen", 3); menu_bg_type_opt->set_item_metadata(3, "image");
	menu_bg_type_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, "Tipo fondo:", String::utf8("Fondo del menú (color, degradado o imagen)."), "fondo menu tipo", false, menu_bg_type_opt);

	HBoxContainer *mbg_cols = memnew(HBoxContainer);
	menu_bg_col_btn = memnew(ColorPickerButton);
	menu_bg_col_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_col_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	mbg_cols->add_child(menu_bg_col_btn);
	menu_bg_grad_bot_btn = memnew(ColorPickerButton);
	menu_bg_grad_bot_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_grad_bot_btn->connect("color_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	mbg_cols->add_child(menu_bg_grad_bot_btn);
	make_prop_row(bg_body, "Colores fondo:", String::utf8("Color base / inferior del fondo del menú."), "fondo color degradado", false, mbg_cols);

	HBoxContainer *mbg_im = memnew(HBoxContainer);
	menu_bg_img_edit = memnew(LineEdit);
	menu_bg_img_edit->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_img_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	mbg_im->add_child(menu_bg_img_edit);
	menu_bg_img_btn = memnew(Button("..."));
	s_make_rounded_action_btn(menu_bg_img_btn);
	menu_bg_img_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_bg_import_pressed));
	mbg_im->add_child(menu_bg_img_btn);
	make_prop_row(bg_body, "Imagen fondo:", String::utf8("Imagen de fondo del menú."), "fondo imagen scroll", false, mbg_im);

	HBoxContainer *scr_hb = memnew(HBoxContainer);
	menu_bg_scroll_x_spin = memnew(SpinBox);
	menu_bg_scroll_x_spin->set_min(-200); menu_bg_scroll_x_spin->set_max(200);
	menu_bg_scroll_x_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_scroll_x_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	scr_hb->add_child(menu_bg_scroll_x_spin);
	menu_bg_scroll_y_spin = memnew(SpinBox);
	menu_bg_scroll_y_spin->set_min(-200); menu_bg_scroll_y_spin->set_max(200);
	menu_bg_scroll_y_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_bg_scroll_y_spin->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	scr_hb->add_child(menu_bg_scroll_y_spin);
	make_prop_row(bg_body, "Scroll fondo X/Y:", String::utf8("Velocidad en px/s de desplazamiento infinito."), "scroll movimiento fondo", true, scr_hb);

	menu_bg_veil_chk = memnew(CheckBox("Velo oscuro sobre el mapa"));
	menu_bg_veil_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, "Velo oscuro:", String::utf8("Oscurece suavemente el juego detrás del menú."), "velo oscuro sombra mapa", false, menu_bg_veil_chk);

	menu_trans_in_opt = memnew(OptionButton);
	const char *k_tr_labels[8] = { "Sin animación", "Fundido", "Desde izquierda", "Desde derecha", "Desde arriba", "Desde abajo", "Escala", "Giro radial" };
	const char *k_tr_ids[8] = { "none", "fade", "slide_left", "slide_right", "slide_top", "slide_bottom", "scale", "radial_spin" };
	menu_trans_out_opt = memnew(OptionButton);
	for (int i = 0; i < 8; i++) {
		menu_trans_in_opt->add_item(String::utf8(k_tr_labels[i]), i);
		menu_trans_in_opt->set_item_metadata(i, String(k_tr_ids[i]));
		menu_trans_out_opt->add_item(String::utf8(k_tr_labels[i]), i);
		menu_trans_out_opt->set_item_metadata(i, String(k_tr_ids[i]));
	}
	menu_trans_in_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	menu_trans_out_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, "Anim. entrada:", String::utf8("Animación al abrir este menú."), "animacion entrada abrir transicion", false, menu_trans_in_opt);
	make_prop_row(bg_body, "Anim. salida:", String::utf8("Animación al cerrar este menú."), "animacion salida cerrar transicion", true, menu_trans_out_opt);

	menu_trans_dur_slider = memnew(HSlider);
	menu_trans_dur_slider->set_min(0); menu_trans_dur_slider->set_max(800); menu_trans_dur_slider->set_step(10);
	menu_trans_dur_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, String::utf8("Duración (ms):"), String::utf8("Duración de la transición en milisegundos."), "duracion ms animacion", true, menu_trans_dur_slider);

	menu_trans_curve_opt = memnew(OptionButton);
	menu_trans_curve_opt->add_item("Suave", 0); menu_trans_curve_opt->set_item_metadata(0, "ease_out");
	menu_trans_curve_opt->add_item("Rebote", 1); menu_trans_curve_opt->set_item_metadata(1, "back");
	menu_trans_curve_opt->add_item("Lineal", 2); menu_trans_curve_opt->set_item_metadata(2, "linear");
	menu_trans_curve_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(bg_body, "Curva:", String::utf8("Curva de interpolación."), "curva easing", true, menu_trans_curve_opt);

	// Sección Entradas / Apertura (3B) y Layout Radial (3E)
	VBoxContainer *open_body = memnew(VBoxContainer);
	open_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	open_body->add_theme_constant_override("separation", int(4 * EDSCALE));
	_attach_collapsible_header(menu_props_box, String::utf8("Apertura y distribución (3B/3E)"), open_body, false);
	menu_props_box->add_child(open_body);

	menu_ctx_opt = memnew(OptionButton);
	menu_ctx_opt->add_item("Solo en el mapa", 0); menu_ctx_opt->set_item_metadata(0, "map_only");
	menu_ctx_opt->add_item("Mapa y combate", 1); menu_ctx_opt->set_item_metadata(1, "map_and_combat");
	menu_ctx_opt->add_item("Cualquier pantalla", 2); menu_ctx_opt->set_item_metadata(2, "any");
	menu_ctx_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(open_body, "Contexto:", String::utf8("Dónde puede abrirse con tecla o mando."), "contexto apertura mapa combate", false, menu_ctx_opt);

	menu_pause_chk = memnew(CheckBox("Pausar juego al abrir"));
	menu_pause_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(open_body, "Pausa:", String::utf8("Congela mapa y eventos mientras está abierto."), "pausar juego abrir", false, menu_pause_chk);

	HBoxContainer *trig_hb = memnew(HBoxContainer);
	menu_capture_key_btn = memnew(Button("+ Tecla / Mando"));
	s_make_rounded_action_btn(menu_capture_key_btn);
	menu_capture_key_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_capture_key_pressed));
	trig_hb->add_child(menu_capture_key_btn);
	menu_triggers_lbl = memnew(Label("-"));
	menu_triggers_lbl->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	menu_triggers_lbl->set_clip_text(true);
	trig_hb->add_child(menu_triggers_lbl);
	make_prop_row(open_body, "Entradas:", String::utf8("Teclas o botones de mando que abren este menú."), "tecla entrada mando atajo", false, trig_hb);

	menu_conflict_lbl = memnew(Label);
	menu_conflict_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	menu_conflict_lbl->add_theme_color_override("font_color", Color(1.0f, 0.48f, 0.35f));
	menu_conflict_lbl->hide();
	open_body->add_child(menu_conflict_lbl);

	menu_layout_mode_opt = memnew(OptionButton);
	menu_layout_mode_opt->add_item("Libre (manual)", 0); menu_layout_mode_opt->set_item_metadata(0, "free");
	menu_layout_mode_opt->add_item("Lista vertical", 1); menu_layout_mode_opt->set_item_metadata(1, "list_v");
	menu_layout_mode_opt->add_item("Lista horizontal", 2); menu_layout_mode_opt->set_item_metadata(2, "list_h");
	menu_layout_mode_opt->add_item("Rejilla", 3); menu_layout_mode_opt->set_item_metadata(3, "grid");
	menu_layout_mode_opt->add_item("Anillo radial", 4); menu_layout_mode_opt->set_item_metadata(4, "radial");
	menu_layout_mode_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(open_body, String::utf8("Distribución:"), String::utf8("Organización automática de botones/elementos."), "distribucion radial lista rejilla", false, menu_layout_mode_opt);

	menu_radial_radius_slider = memnew(HSlider);
	menu_radial_radius_slider->set_min(40); menu_radial_radius_slider->set_max(260); menu_radial_radius_slider->set_step(1);
	menu_radial_radius_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(open_body, "Radio radial:", String::utf8("Radio del anillo en distribución radial."), "radial radio circulo", false, menu_radial_radius_slider);

	menu_radial_start_slider = memnew(HSlider);
	menu_radial_start_slider->set_min(-180); menu_radial_start_slider->set_max(180); menu_radial_start_slider->set_step(1);
	menu_radial_start_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(open_body, String::utf8("Ángulo inicial:"), String::utf8("Ángulo inicial del primer hueco radial."), "radial angulo inicio", true, menu_radial_start_slider);

	menu_radial_sweep_slider = memnew(HSlider);
	menu_radial_sweep_slider->set_min(45); menu_radial_sweep_slider->set_max(360); menu_radial_sweep_slider->set_step(5);
	menu_radial_sweep_slider->connect("value_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(open_body, "Amplitud:", String::utf8("Arco total cubierto por las ranuras radiales."), "radial amplitud arco", true, menu_radial_sweep_slider);

	HBoxContainer *rad_op_hb = memnew(HBoxContainer);
	menu_radial_cw_chk = memnew(CheckBox("Horario"));
	menu_radial_cw_chk->connect("toggled", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	rad_op_hb->add_child(menu_radial_cw_chk);
	menu_radial_add_slot_btn = memnew(Button(String::utf8("+ Añadir ranura")));
	s_make_rounded_action_btn(menu_radial_add_slot_btn);
	menu_radial_add_slot_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_radial_add_slot_pressed));
	rad_op_hb->add_child(menu_radial_add_slot_btn);
	make_prop_row(open_body, "Ranuras:", String::utf8("Añade otra ranura al anillo radial."), "radial anadir ranura boton", false, rad_op_hb);

	// Sección Sonidos (4F)
	VBoxContainer *snd_body = memnew(VBoxContainer);
	snd_body->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	snd_body->add_theme_constant_override("separation", int(4 * EDSCALE));
	_attach_collapsible_header(menu_props_box, String::utf8("Sonidos del menú"), snd_body, false);
	menu_props_box->add_child(snd_body);

	auto make_snd = [&](const String &p_l, LineEdit *&r_ed) {
		r_ed = memnew(LineEdit);
		r_ed->set_placeholder("res://audio/...");
		r_ed->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
		make_prop_row(snd_body, p_l, String::utf8("Archivo de sonido opcional para el menú."), "sonido audio sfx " + p_l, true, r_ed);
	};
	make_snd("Abrir:", menu_snd_open_edit);
	make_snd("Cerrar:", menu_snd_close_edit);
	make_snd("Mover:", menu_snd_move_edit);
	make_snd("Aceptar:", menu_snd_accept_edit);
	make_snd("Cancelar:", menu_snd_cancel_edit);
	menu_bgm_edit = memnew(LineEdit);
	menu_bgm_edit->set_placeholder("res://audio/bgm/...");
	menu_bgm_edit->connect("text_changed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_global_changed).unbind(1));
	make_prop_row(snd_body, String::utf8("Música (BGM):"), String::utf8("Música de fondo opcional al abrir esta pantalla."), "bgm musica audio titulo", true, menu_bgm_edit);

	// --- Diálogos auxiliares, Asistente 3 pasos (4A) y Galería de Plantillas (4B) ---
	import_image_dialog = memnew(FileDialog);
	import_image_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	import_image_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	import_image_dialog->add_filter("*.png, *.webp, *.jpg, *.jpeg ; Imagen");
	import_image_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_image_file_selected));
	add_child(import_image_dialog);

	import_font_dialog = memnew(FileDialog);
	import_font_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	import_font_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	import_font_dialog->add_filter("*.ttf, *.otf ; Fuente");
	import_font_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_font_file_selected));
	add_child(import_font_dialog);

	export_pkg_dialog = memnew(FileDialog);
	export_pkg_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
	export_pkg_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	export_pkg_dialog->add_filter("*.cgmenu, *.json ; Paquete ClockGears UI");
	export_pkg_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_export_pkg_file_selected));
	add_child(export_pkg_dialog);

	import_pkg_dialog = memnew(FileDialog);
	import_pkg_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	import_pkg_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
	import_pkg_dialog->add_filter("*.cgmenu, *.json ; Paquete ClockGears UI");
	import_pkg_dialog->connect("file_selected", callable_mp(this, &ClockGearsUiDesignEditor::_on_import_pkg_file_selected));
	add_child(import_pkg_dialog);

	reset_confirm_dialog = memnew(ConfirmationDialog);
	reset_confirm_dialog->set_title(String::utf8("Restaurar diseño"));
	reset_confirm_dialog->set_text(String::utf8("¿Restablecer esta pantalla a su diseño original?"));
	reset_confirm_dialog->connect("confirmed", callable_mp(this, &ClockGearsUiDesignEditor::_on_reset_confirmed));
	add_child(reset_confirm_dialog);

	delete_menu_confirm_dialog = memnew(ConfirmationDialog);
	delete_menu_confirm_dialog->set_title(String::utf8("Eliminar menú"));
	delete_menu_confirm_dialog->connect("confirmed", callable_mp(this, &ClockGearsUiDesignEditor::_on_menu_del_confirmed));
	add_child(delete_menu_confirm_dialog);

	template_apply_confirm_dialog = memnew(ConfirmationDialog);
	template_apply_confirm_dialog->set_title("Aplicar plantilla");
	template_apply_confirm_dialog->connect("confirmed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_apply_confirmed));
	add_child(template_apply_confirm_dialog);

	save_tpl_name_dialog = memnew(ConfirmationDialog);
	save_tpl_name_dialog->set_title("Guardar como mi plantilla");
	VBoxContainer *st_vb = memnew(VBoxContainer);
	st_vb->add_theme_constant_override("separation", int(6 * EDSCALE));
	st_vb->add_child(memnew(Label("Nombre de la plantilla:")));
	save_tpl_name_edit = memnew(LineEdit);
	save_tpl_name_edit->set_custom_minimum_size(Size2(260 * EDSCALE, row_h));
	st_vb->add_child(save_tpl_name_edit);
	save_tpl_name_dialog->add_child(st_vb);
	save_tpl_name_dialog->connect("confirmed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_save_name_confirmed));
	add_child(save_tpl_name_dialog);

	// --- Asistente 3 pasos (4A) ---
	menu_wizard_dialog = memnew(AcceptDialog);
	menu_wizard_dialog->set_title(String::utf8("Asistente: Nuevo menú"));
	menu_wizard_dialog->set_min_size(Size2i(int(720 * EDSCALE), int(500 * EDSCALE)));
	menu_wizard_dialog->get_ok_button()->hide();
	add_child(menu_wizard_dialog);

	VBoxContainer *wiz_root = memnew(VBoxContainer);
	wiz_root->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wiz_root->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wiz_root->add_theme_constant_override("separation", int(8 * EDSCALE));
	menu_wizard_dialog->add_child(wiz_root);

	wizard_step_lbl = memnew(Label(String::utf8("Paso 1 de 3: Elige una plantilla")));
	wizard_step_lbl->add_theme_color_override("font_color", ClockGearsTheme::color_accent_amber());
	wiz_root->add_child(wizard_step_lbl);

	// Paso 1: Plantillas con miniaturas
	ScrollContainer *wiz_sc1 = memnew(ScrollContainer);
	wiz_sc1->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wiz_sc1->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	wizard_step_panels[0] = memnew(VBoxContainer);
	wizard_step_panels[0]->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_step_panels[0]->add_child(wiz_sc1);
	wizard_tpl_grid = memnew(GridContainer);
	wizard_tpl_grid->set_columns(3);
	wizard_tpl_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_tpl_grid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
	wizard_tpl_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
	wiz_sc1->add_child(wizard_tpl_grid);
	wiz_root->add_child(wizard_step_panels[0]);

	// Paso 2: Nombre y botones
	wizard_step_panels[1] = memnew(VBoxContainer);
	wizard_step_panels[1]->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_step_panels[1]->add_theme_constant_override("separation", int(6 * EDSCALE));
	wizard_step_panels[1]->add_child(memnew(Label(String::utf8("Nombre del menú:"))));
	wizard_name_edit = memnew(LineEdit);
	wizard_name_edit->set_text(String::utf8("Mi Menú"));
	wizard_step_panels[1]->add_child(wizard_name_edit);
	wizard_step_panels[1]->add_child(memnew(Label("Botones iniciales:")));
	GridContainer *wiz_chk_grid = memnew(GridContainer);
	wiz_chk_grid->set_columns(3);
	wizard_step_panels[1]->add_child(wiz_chk_grid);
	struct WizBtnDef { const char *label; const char *act; bool def_on; };
	static const WizBtnDef k_wbtns[9] = {
		{ "Inventario", "open_items", true },
		{ "Estado", "open_status", true },
		{ "Equipo", "open_equip", false },
		{ "Guardar", "open_save", true },
		{ "Opciones", "open_options", true },
		{ "Cerrar", "close_menu", true },
		{ "Continuar", "continue_game", false },
		{ "Nueva partida", "new_game", false },
		{ "Salir al t\xC3\xADtulo", "return_title", false }
	};
	for (int i = 0; i < 9; i++) {
		CheckBox *cb = memnew(CheckBox(String::utf8(k_wbtns[i].label)));
		cb->set_pressed(k_wbtns[i].def_on);
		cb->set_meta("action", String(k_wbtns[i].act));
		cb->set_meta("label", String::utf8(k_wbtns[i].label));
		wiz_chk_grid->add_child(cb);
		wizard_btn_checks.push_back(cb);
	}
	wizard_step_panels[1]->hide();
	wiz_root->add_child(wizard_step_panels[1]);

	// Paso 3: Cómo se abre y pausa
	wizard_step_panels[2] = memnew(VBoxContainer);
	wizard_step_panels[2]->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	wizard_step_panels[2]->add_theme_constant_override("separation", int(8 * EDSCALE));
	HBoxContainer *wctx_hb = memnew(HBoxContainer);
	wctx_hb->add_child(memnew(Label("Contexto:")));
	wizard_ctx_opt = memnew(OptionButton);
	wizard_ctx_opt->add_item("Solo en el mapa", 0); wizard_ctx_opt->set_item_metadata(0, "map_only");
	wizard_ctx_opt->add_item("Mapa y combate", 1); wizard_ctx_opt->set_item_metadata(1, "map_and_combat");
	wizard_ctx_opt->add_item("Cualquier pantalla", 2); wizard_ctx_opt->set_item_metadata(2, "any");
	wizard_ctx_opt->connect("item_selected", callable_mp(this, &ClockGearsUiDesignEditor::_refresh_wizard_conflict_label).unbind(1));
	wctx_hb->add_child(wizard_ctx_opt);
	wizard_step_panels[2]->add_child(wctx_hb);

	wizard_pause_chk = memnew(CheckBox(String::utf8("Pausar el juego al abrir este menú")));
	wizard_pause_chk->set_pressed(true);
	wizard_step_panels[2]->add_child(wizard_pause_chk);

	HBoxContainer *wkey_hb = memnew(HBoxContainer);
	wizard_capture_key_btn = memnew(Button("Capturar tecla / mando"));
	s_make_rounded_action_btn(wizard_capture_key_btn);
	wizard_capture_key_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_wizard_capture_key_pressed));
	wkey_hb->add_child(wizard_capture_key_btn);
	wizard_triggers_lbl = memnew(Label("Sin tecla asignada (opcional)"));
	wkey_hb->add_child(wizard_triggers_lbl);
	wizard_step_panels[2]->add_child(wkey_hb);

	wizard_conflict_lbl = memnew(Label);
	wizard_conflict_lbl->add_theme_color_override("font_color", Color(1.0f, 0.48f, 0.35f));
	wizard_conflict_lbl->hide();
	wizard_step_panels[2]->add_child(wizard_conflict_lbl);

	wizard_step_panels[2]->hide();
	wiz_root->add_child(wizard_step_panels[2]);

	HBoxContainer *wiz_nav = memnew(HBoxContainer);
	wiz_nav->set_alignment(BoxContainer::ALIGNMENT_END);
	wiz_nav->add_theme_constant_override("separation", int(8 * EDSCALE));
	wiz_root->add_child(wiz_nav);
	wizard_prev_btn = memnew(Button("Anterior"));
	s_make_rounded_action_btn(wizard_prev_btn);
	wizard_prev_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_wizard_prev_pressed));
	wiz_nav->add_child(wizard_prev_btn);
	wizard_next_btn = memnew(Button("Siguiente"));
	s_make_rounded_action_btn(wizard_next_btn);
	wizard_next_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_wizard_next_pressed));
	wiz_nav->add_child(wizard_next_btn);
	wizard_finish_btn = memnew(Button(String::utf8("Crear menú")));
	s_make_rounded_action_btn(wizard_finish_btn);
	wizard_finish_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_wizard_finish_pressed));
	wiz_nav->add_child(wizard_finish_btn);

	// --- Galería de Plantillas (4B) ---
	template_gallery_dialog = memnew(AcceptDialog);
	template_gallery_dialog->set_title("Biblioteca de plantillas");
	template_gallery_dialog->set_min_size(Size2i(int(760 * EDSCALE), int(520 * EDSCALE)));
	template_gallery_dialog->get_ok_button()->hide();
	add_child(template_gallery_dialog);

	VBoxContainer *gal_vb = memnew(VBoxContainer);
	gal_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	gal_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	gal_vb->add_theme_constant_override("separation", int(8 * EDSCALE));
	template_gallery_dialog->add_child(gal_vb);

	template_tabs = memnew(TabContainer);
	template_tabs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	template_tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	gal_vb->add_child(template_tabs);

	auto make_tpl_tab = [&](const String &p_tab_name, GridContainer *&r_grid) {
		ScrollContainer *sc = memnew(ScrollContainer);
		sc->set_name(p_tab_name);
		sc->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		r_grid = memnew(GridContainer);
		r_grid->set_columns(3);
		r_grid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		r_grid->add_theme_constant_override("h_separation", int(8 * EDSCALE));
		r_grid->add_theme_constant_override("v_separation", int(8 * EDSCALE));
		sc->add_child(r_grid);
		template_tabs->add_child(sc);
	};
	make_tpl_tab("Pantallas completas", tpl_screen_grid);
	make_tpl_tab("Componentes", tpl_comp_grid);
	make_tpl_tab("Mis plantillas", tpl_user_grid);

	tpl_selected_info_lbl = memnew(Label(String::utf8("Selecciona una plantilla para previsualizarla y aplicarla.")));
	tpl_selected_info_lbl->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	gal_vb->add_child(tpl_selected_info_lbl);

	HFlowContainer *gal_acts = memnew(HFlowContainer);
	gal_acts->add_theme_constant_override("h_separation", int(6 * EDSCALE));
	gal_acts->add_theme_constant_override("v_separation", int(4 * EDSCALE));
	gal_vb->add_child(gal_acts);

	tpl_preserve_btns_chk = memnew(CheckBox("Conservar acciones de botones"));
	tpl_preserve_btns_chk->set_pressed(true);
	gal_acts->add_child(tpl_preserve_btns_chk);

	tpl_apply_btn = memnew(Button(String::utf8("Aplicar al menú")));
	s_make_rounded_action_btn(tpl_apply_btn);
	tpl_apply_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_apply_pressed));
	gal_acts->add_child(tpl_apply_btn);

	Button *tpl_save_btn = memnew(Button(String::utf8("Guardar menú como mi plantilla")));
	s_make_rounded_action_btn(tpl_save_btn);
	tpl_save_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_save_current_pressed));
	gal_acts->add_child(tpl_save_btn);

	tpl_export_btn = memnew(Button(String::utf8("Exportar paquete (.cgmenu)…")));
	s_make_rounded_action_btn(tpl_export_btn);
	tpl_export_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_export_pressed));
	gal_acts->add_child(tpl_export_btn);

	Button *tpl_imp_btn = memnew(Button(String::utf8("Importar paquete…")));
	s_make_rounded_action_btn(tpl_imp_btn);
	tpl_imp_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_import_pressed));
	gal_acts->add_child(tpl_imp_btn);

	tpl_delete_user_btn = memnew(Button(String::utf8("Borrar mía")));
	s_make_rounded_action_btn(tpl_delete_user_btn);
	tpl_delete_user_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_delete_user_pressed));
	gal_acts->add_child(tpl_delete_user_btn);
}

String ClockGearsUiDesignEditor::_anchor_key_by_index(int p_idx) {
	static const char *k_anchors[9] = {
		"top_left", "top_center", "top_right",
		"middle_left", "center", "middle_right",
		"bottom_left", "bottom_center", "bottom_right"
	};
	return String(k_anchors[CLAMP(p_idx, 0, 8)]);
}

int ClockGearsUiDesignEditor::_anchor_index_by_key(const String &p_key) {
	for (int i = 0; i < 9; i++) {
		if (_anchor_key_by_index(i) == p_key) {
			return i;
		}
	}
	return 0;
}

void ClockGearsUiDesignEditor::_rebalance_responsive_layout(int p_avail_w) {
	last_adapt_width = p_avail_w;
	bool want_compact = (p_avail_w > 0 && p_avail_w < int(1400 * EDSCALE));
	compact_two_row_layout = want_compact;

	if (want_compact) {
		wide_hbox->hide();
		compact_vbox->show();
		s_reparent_ctrl(canvas_card, compact_vbox);
		s_reparent_ctrl(menus_card, compact_vbox);
		s_reparent_ctrl(palette_card, compact_vbox);
		s_reparent_ctrl(layers_card, compact_vbox);
		s_reparent_ctrl(props_card, compact_vbox);
		menus_card->set_custom_minimum_size(Size2(0, 0));
		palette_card->set_custom_minimum_size(Size2(0, 0));
		layers_card->set_custom_minimum_size(Size2(0, 0));
		props_card->set_custom_minimum_size(Size2(0, 0));
		if (palette_grid) {
			palette_grid->set_columns(3);
		}
	} else {
		compact_vbox->hide();
		wide_hbox->show();
		s_reparent_ctrl(menus_card, wide_left_col);
		s_reparent_ctrl(palette_card, wide_left_col);
		s_reparent_ctrl(layers_card, wide_left_col);
		s_reparent_ctrl(canvas_card, wide_center_col);
		s_reparent_ctrl(props_card, wide_right_col);
		menus_card->set_custom_minimum_size(Size2(0, 0));
		palette_card->set_custom_minimum_size(Size2(0, 0));
		layers_card->set_custom_minimum_size(Size2(0, 0));
		props_card->set_custom_minimum_size(Size2(0, 0));
		if (palette_grid) {
			palette_grid->set_columns(1);
		}
	}
	callable_mp(this, &ClockGearsUiDesignEditor::_update_canvas_fit_scale).call_deferred();
}

void ClockGearsUiDesignEditor::adapt_to_width(int p_window_width) {
	_rebalance_responsive_layout(p_window_width);
}

void ClockGearsUiDesignEditor::set_preview_resolution(int p_width, int p_height, bool p_sync_owner_db) {
	preview_resolution = Vector2i(MAX(320, p_width), MAX(180, p_height));
	if (preview_res_opt) {
		bool prev = updating_ui;
		updating_ui = true;
		for (int i = 0; i < preview_res_opt->get_item_count(); i++) {
			Vector2i v = preview_res_opt->get_item_metadata(i);
			if (v == preview_resolution) {
				preview_res_opt->select(i);
				break;
			}
		}
		updating_ui = prev;
	}
	_refresh_canvas_stage();
	if (p_sync_owner_db && owner_db && owner_db->get_menus_preview_resolution() != preview_resolution) {
		owner_db->set_menus_preview_resolution(preview_resolution.x, preview_resolution.y);
	}
}

void ClockGearsUiDesignEditor::refresh_from_data_manager() {
	_refresh_screen_dropdown();
	_refresh_menus_list();
	_refresh_style_binding_action_options();
	_refresh_canvas_stage();
	_refresh_layers_list();
	_refresh_properties_panel();
	_refresh_menu_global_properties();
	_refresh_undo_redo_buttons();
}

void ClockGearsUiDesignEditor::refresh_geometry_fast(bool p_refresh_props) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !design_layer) {
		return;
	}
	bool has_custom = dm->has_custom_ui_design(current_screen_id) || !ClockGearsDataManager::is_system_menu_id(current_screen_id);
	if (current_screen_id == "hud" || has_custom) {
		Dictionary design = dm->get_ui_screen_design(current_screen_id);
		design_layer->apply_prebuilt_screen_design(current_screen_id, design);
		if (!design_layer->is_visible()) {
			design_layer->show();
		}
	}
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
	if (p_refresh_props) {
		_refresh_properties_panel();
	}
}

void ClockGearsUiDesignEditor::update_dragged_element_fast(const String &p_elem_id, const Dictionary &p_elem) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || p_elem_id.is_empty()) {
		return;
	}
	dm->update_ui_element(current_screen_id, p_elem_id, p_elem, false);
	if (design_layer) {
		if (!design_layer->is_visible()) {
			Dictionary design = dm->get_ui_screen_design(current_screen_id);
			design_layer->apply_screen_design(current_screen_id, design);
			design_layer->show();
		} else {
			design_layer->update_element_geometry_fast(p_elem_id, p_elem);
		}
	}
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
	// Sincroniza spinners sin reconstruir todo el árbol de propiedades
	bool prev_upd = updating_ui;
	updating_ui = true;
	if (prop_ox_spin) prop_ox_spin->set_value(int(p_elem.get("offset_x", 0)));
	if (prop_oy_spin) prop_oy_spin->set_value(int(p_elem.get("offset_y", 0)));
	if (prop_w_spin) prop_w_spin->set_value(int(p_elem.get("width", 120)));
	if (prop_h_spin) prop_h_spin->set_value(int(p_elem.get("height", 36)));
	if (prop_radius_slider) prop_radius_slider->set_value(int(p_elem.get("corner_radius", 8)));
	if (prop_radius_val_lbl) prop_radius_val_lbl->set_text(itos(int(p_elem.get("corner_radius", 8))) + "px");
	updating_ui = prev_upd;
}

ClockGearsTitleScreen *ClockGearsUiDesignEditor::_ensure_default_title_preview() {
	if (!default_title_preview && canvas_stage) {
		default_title_preview = memnew(ClockGearsTitleScreen);
		default_title_preview->set_preview_mode(true);
		default_title_preview->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		default_title_preview->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		default_title_preview->hide();
		canvas_stage->add_child(default_title_preview);
		if (design_layer) {
			canvas_stage->move_child(default_title_preview, design_layer->get_index());
		}
	}
	return default_title_preview;
}

ClockGearsGameMenu *ClockGearsUiDesignEditor::_ensure_default_gmenu_preview() {
	if (!default_gmenu_vp && canvas_stage) {
		default_gmenu_vp = memnew(SubViewport);
		default_gmenu_vp->set_size(Size2i(BASE_W, BASE_H));
		default_gmenu_vp->set_disable_3d(true);
		default_gmenu_vp->set_transparent_background(true);
		default_gmenu_vp->set_update_mode(SubViewport::UPDATE_WHEN_PARENT_VISIBLE);
		canvas_stage->add_child(default_gmenu_vp);

		default_gmenu_disp = memnew(TextureRect);
		default_gmenu_disp->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		default_gmenu_disp->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		default_gmenu_disp->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		default_gmenu_disp->set_stretch_mode(TextureRect::STRETCH_SCALE);
		default_gmenu_disp->set_texture(default_gmenu_vp->get_texture());
		default_gmenu_disp->hide();
		canvas_stage->add_child(default_gmenu_disp);
		if (design_layer) {
			canvas_stage->move_child(default_gmenu_disp, design_layer->get_index());
		}
	}
	if (!default_gmenu_preview && default_gmenu_vp) {
		default_gmenu_preview = memnew(ClockGearsGameMenu);
		default_gmenu_preview->set_preview_mode(true);
		default_gmenu_preview->set_editor_test_popup_mode(true);
		default_gmenu_preview->hide();
		default_gmenu_vp->add_child(default_gmenu_preview);
	}
	return default_gmenu_preview;
}

ClockGearsCreditsPlayer *ClockGearsUiDesignEditor::_ensure_default_credits_preview() {
	if (!default_credits_preview && canvas_stage) {
		default_credits_preview = memnew(ClockGearsCreditsPlayer);
		default_credits_preview->set_preview_mode(true);
		default_credits_preview->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		default_credits_preview->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
		default_credits_preview->hide();
		canvas_stage->add_child(default_credits_preview);
		if (design_layer) {
			canvas_stage->move_child(default_credits_preview, design_layer->get_index());
		}
	}
	return default_credits_preview;
}

void ClockGearsUiDesignEditor::_refresh_background_and_default_preview(bool p_has_custom) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	bool is_hud = (current_screen_id == "hud");
	canvas_hud_grid_bg->set_visible(is_hud);
	canvas_bg_color->set_visible(!is_hud);
	canvas_bg_tex->set_visible(!is_hud);

	if (!is_hud) {
		Dictionary bg;
		if (current_screen_id == "title") {
			bg = dm->get_title_menu_config().get("background", Dictionary());
		} else if (current_screen_id == "credits") {
			bg = dm->get_credits_config().get("background", Dictionary());
		} else if (current_screen_id.begins_with("game_menu_")) {
			String sub = current_screen_id.trim_prefix("game_menu_");
			Dictionary bgs = dm->get_game_menu_config().get("backgrounds", Dictionary());
			if (bgs.has(sub)) {
				bg = bgs[sub];
			}
		}
		ClockGearsTitleScreen::apply_screen_background(canvas_bg_color, canvas_bg_tex, bg, "#12151e");
	}

	Size2 stage_sz(float(preview_resolution.x * canvas_zoom), float(preview_resolution.y * canvas_zoom));
	if (default_title_preview) {
		default_title_preview->hide();
	}
	if (default_gmenu_disp) {
		default_gmenu_disp->hide();
	}
	if (default_gmenu_preview) {
		default_gmenu_preview->hide();
	}
	if (default_credits_preview) {
		default_credits_preview->hide();
	}

	if (!p_has_custom && !is_hud) {
		if (current_screen_id == "title") {
			if (ClockGearsTitleScreen *tp = _ensure_default_title_preview()) {
				tp->set_size(stage_sz);
				tp->apply_title_config_preview(dm->get_title_menu_config(), dm->get_system_config());
				tp->show();
			}
		} else if (current_screen_id.begins_with("game_menu_")) {
			String sub = current_screen_id.trim_prefix("game_menu_");
			if (ClockGearsGameMenu *gp = _ensure_default_gmenu_preview()) {
				if (default_gmenu_vp) {
					default_gmenu_vp->set_size(Size2i(stage_sz));
				}
				if (default_gmenu_disp) {
					default_gmenu_disp->show();
				}
				gp->open_menu();
				gp->open_subscreen(sub);
				gp->show();
			}
		} else if (current_screen_id == "credits") {
			if (ClockGearsCreditsPlayer *cp = _ensure_default_credits_preview()) {
				cp->set_size(stage_sz);
				cp->apply_credits_config_preview(dm->get_credits_config());
				cp->show();
			}
		}
	}
}

static void s_select_option_meta(OptionButton *p_opt, const String &p_meta) {
	if (!p_opt) {
		return;
	}
	String t = p_meta.strip_edges().to_lower();
	for (int i = 0; i < p_opt->get_item_count(); i++) {
		if (String(p_opt->get_item_metadata(i)).strip_edges().to_lower() == t) {
			p_opt->select(i);
			return;
		}
	}
	if (p_opt->get_item_count() > 0) {
		p_opt->select(0);
	}
}

static String s_format_trigger_text(const Dictionary &p_tr) {
	String kind = String(p_tr.get("kind", "key"));
	if (kind == "joypad") {
		return "Mando #" + itos(int(p_tr.get("button_index", 0)));
	}
	if (kind == "mouse") {
		return String::utf8("Ratón #") + itos(int(p_tr.get("mouse_button", 1)));
	}
	Key kc = static_cast<Key>(int(p_tr.get("keycode", 0)));
	if (kc != Key::NONE) {
		return keycode_get_string(kc);
	}
	return String(p_tr.get("action", "Tecla"));
}

static void s_clear_container_children(Container *p_box) {
	if (!p_box) {
		return;
	}
	while (p_box->get_child_count() > 0) {
		Node *ch = p_box->get_child(0);
		p_box->remove_child(ch);
		memdelete(ch);
	}
}

void ClockGearsUiDesignEditor::_refresh_canvas_stage() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !design_layer || !canvas_stage) {
		return;
	}
	Size2 stage_sz(float(preview_resolution.x * canvas_zoom), float(preview_resolution.y * canvas_zoom));
	canvas_stage->set_custom_minimum_size(stage_sz);
	canvas_stage->set_size(stage_sz);
	if (canvas_scroll) {
		if (canvas_zoom <= 1) {
			canvas_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			canvas_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
			if (canvas_aspect) {
				canvas_aspect->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
			}
			if (canvas_fit_holder) {
				canvas_fit_holder->set_custom_minimum_size(Size2(0, 0));
			}
		} else {
			canvas_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
			canvas_scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_AUTO);
			if (canvas_fit_holder) {
				canvas_fit_holder->set_custom_minimum_size(stage_sz);
			}
		}
	}
	if (zoom_info_lbl) {
		zoom_info_lbl->set_text(itos(int(stage_sz.x)) + "x" + itos(int(stage_sz.y)));
	}
	callable_mp(this, &ClockGearsUiDesignEditor::_update_canvas_fit_scale).call_deferred();

	Dictionary design = dm->get_ui_screen_design(current_screen_id);
	bool is_custom_menu = dm->has_menu(current_screen_id) && !ClockGearsDataManager::is_system_menu_id(current_screen_id);
	bool has_custom = dm->has_custom_ui_design(current_screen_id) || is_custom_menu;
	_refresh_background_and_default_preview(has_custom);

	if (screen_status_lbl) {
		if (current_screen_id == "hud") {
			Array elems = design.get("elements", Array());
			screen_status_lbl->set_text(String::utf8("HUD activo (") + itos(elems.size()) + ")");
		} else if (is_custom_menu) {
			Array elems = design.get("elements", Array());
			screen_status_lbl->set_text(String::utf8("Menú propio (") + itos(elems.size()) + ")");
		} else if (has_custom) {
			screen_status_lbl->set_text(String::utf8("Diseño personalizado activo"));
		} else {
			screen_status_lbl->set_text(String::utf8("Por defecto (arrastra elementos para personalizar)"));
		}
	}

	design_layer->set_viewport_size_override(stage_sz);
	design_layer->set_size(stage_sz);
	if (current_screen_id == "hud" || has_custom) {
		design_layer->apply_prebuilt_screen_design(current_screen_id, design);
		design_layer->show();
	} else {
		design_layer->clear_elements();
		design_layer->hide();
	}
	if (canvas_overlay) {
		canvas_overlay->set_size(stage_sz);
		canvas_overlay->queue_redraw();
	}
}

void ClockGearsUiDesignEditor::_refresh_screen_dropdown() {
	if (!screen_opt) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array menus = dm ? dm->get_all_menus_for_selector() : ClockGearsDataManager::get_ui_design_screens();
	bool prev = updating_ui;
	updating_ui = true;
	screen_opt->clear();
	int sel_idx = 0;
	for (int i = 0; i < menus.size(); i++) {
		Dictionary sd = menus[i];
		String id = String(sd.get("id", ""));
		String lbl = String(sd.get("label", sd.get("name", id)));
		screen_opt->add_item(lbl, i);
		screen_opt->set_item_metadata(i, id);
		if (id == current_screen_id) {
			sel_idx = i;
		}
	}
	if (screen_opt->get_item_count() > 0) {
		screen_opt->select(sel_idx);
	}
	updating_ui = prev;
}

void ClockGearsUiDesignEditor::_on_pooled_menu_btn_pressed(Button *p_btn) {
	if (!p_btn || !p_btn->has_meta("cg_menu_id")) {
		return;
	}
	_on_menu_list_select_pressed(String(p_btn->get_meta("cg_menu_id")));
}

void ClockGearsUiDesignEditor::_on_pooled_layer_eye_toggled(Button *p_btn) {
	if (!p_btn || !p_btn->has_meta("cg_elem_id")) {
		return;
	}
	_on_layer_eye_toggled(String(p_btn->get_meta("cg_elem_id")));
}

void ClockGearsUiDesignEditor::_on_pooled_layer_row_pressed(Button *p_btn) {
	if (!p_btn || !p_btn->has_meta("cg_elem_id")) {
		return;
	}
	_on_layer_row_pressed(String(p_btn->get_meta("cg_elem_id")));
}

void ClockGearsUiDesignEditor::_refresh_menus_list() {
	if (!menus_list_vbox) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Array menus = dm ? dm->get_all_menus_for_selector() : ClockGearsDataManager::get_ui_design_screens();
	bool is_sys = ClockGearsDataManager::is_system_menu_id(current_screen_id);
	int count = menus.size();

	while (menus_list_vbox->get_child_count() > count) {
		Node *ch = menus_list_vbox->get_child(menus_list_vbox->get_child_count() - 1);
		menus_list_vbox->remove_child(ch);
		memdelete(ch);
	}
	for (int i = 0; i < count; i++) {
		Dictionary md = menus[i];
		String mid = String(md.get("id", ""));
		String mname = String(md.get("name", md.get("label", mid)));
		bool m_sys = ClockGearsDataManager::is_system_menu_id(mid);
		String prefix = m_sys ? "[Sis] " : "[M] ";

		Button *b = nullptr;
		if (i < menus_list_vbox->get_child_count()) {
			b = Object::cast_to<Button>(menus_list_vbox->get_child(i));
		}
		if (!b) {
			b = memnew(Button);
			b->set_toggle_mode(true);
			b->set_clip_text(true);
			b->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
			b->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
			b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			b->set_custom_minimum_size(Size2(0, 26 * EDSCALE));
			_apply_rounded_btn_style(b);
			b->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_pooled_menu_btn_pressed).bind(b));
			menus_list_vbox->add_child(b);
		}
		b->set_text(prefix + mname);
		b->set_pressed_no_signal(mid == current_screen_id);
		b->set_meta("cg_menu_id", mid);
	}
	if (menu_up_btn) menu_up_btn->set_disabled(is_sys);
	if (menu_down_btn) menu_down_btn->set_disabled(is_sys);
	if (menu_del_btn) menu_del_btn->set_disabled(is_sys);
	if (menu_restore_btn) menu_restore_btn->set_disabled(!is_sys);
}

void ClockGearsUiDesignEditor::_refresh_layers_list() {
	if (!layers_list_vbox) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Array elems = dm->get_ui_screen_elements(current_screen_id);
	Vector<Dictionary> sorted;
	for (int i = 0; i < elems.size(); i++) {
		if (elems[i].get_type() == Variant::DICTIONARY) {
			sorted.push_back(elems[i]);
		}
	}
	struct LayerSort {
		bool operator()(const Dictionary &a, const Dictionary &b) const {
			return int(a.get("layer_order", 0)) > int(b.get("layer_order", 0));
		}
	};
	sorted.sort_custom<LayerSort>();

	if (!selected_elem_id.is_empty()) {
		bool found_sel = false;
		String sel_low = selected_elem_id.strip_edges().to_lower();
		for (int i = 0; i < sorted.size(); i++) {
			if (String(sorted[i].get("id", "")).to_lower() == sel_low) {
				found_sel = true;
				break;
			}
		}
		if (!found_sel) {
			selected_elem_id = "";
		}
	}

	int count = sorted.size();
	const int kLongLayerThreshold = 32;
	if (layers_long_canvas && count > kLongLayerThreshold) {
		for (int c = layers_list_vbox->get_child_count() - 1; c >= 0; c--) {
			Node *ch = layers_list_vbox->get_child(c);
			if (ch != layers_long_canvas) {
				layers_list_vbox->remove_child(ch);
				memdelete(ch);
			}
		}
		s_reparent_ctrl(layers_long_canvas, layers_list_vbox);
		layers_long_canvas->show();
		Vector<ClockGearsLayersCanvas::Row> crows;
		crows.resize(count);
		for (int i = 0; i < count; i++) {
			const Dictionary &e = sorted[i];
			String eid = String(e.get("id", ""));
			String ename = String(e.get("name", eid));
			int lo = int(e.get("layer_order", 0));
			ClockGearsLayersCanvas::Row r;
			r.elem_id = eid;
			r.label = ename + " [C" + itos(lo) + "]";
			r.visible = bool(e.get("visible", true));
			r.selected = (eid == selected_elem_id);
			crows.write[i] = r;
		}
		layers_long_canvas->set_rows(crows, 28.0f * EDSCALE);
	} else {
		if (layers_long_canvas && layers_long_canvas->get_parent() == layers_list_vbox) {
			layers_long_canvas->hide();
			Control *holder = Object::cast_to<Control>(layers_list_vbox->get_parent() ? layers_list_vbox->get_parent()->get_parent() : nullptr);
			if (holder) {
				s_reparent_ctrl(layers_long_canvas, holder);
			} else {
				layers_list_vbox->remove_child(layers_long_canvas);
				add_child(layers_long_canvas);
			}
		}
		while (layers_list_vbox->get_child_count() > count) {
			Node *ch = layers_list_vbox->get_child(layers_list_vbox->get_child_count() - 1);
			layers_list_vbox->remove_child(ch);
			memdelete(ch);
		}

		for (int i = 0; i < count; i++) {
			Dictionary e = sorted[i];
			String eid = String(e.get("id", ""));
			String ename = String(e.get("name", eid));
			int lo = int(e.get("layer_order", 0));
			bool vis = bool(e.get("visible", true));
			bool is_sel = (eid == selected_elem_id);

			HBoxContainer *row = nullptr;
			Button *eye_btn = nullptr;
			Button *sel_btn = nullptr;
			if (i < layers_list_vbox->get_child_count()) {
				row = Object::cast_to<HBoxContainer>(layers_list_vbox->get_child(i));
				if (row && row->get_child_count() >= 2) {
					eye_btn = Object::cast_to<Button>(row->get_child(0));
					sel_btn = Object::cast_to<Button>(row->get_child(1));
				}
			}
			if (!row || !eye_btn || !sel_btn) {
				row = memnew(HBoxContainer);
				row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				row->add_theme_constant_override("separation", int(4 * EDSCALE));
				layers_list_vbox->add_child(row);

				eye_btn = memnew(Button);
				eye_btn->set_toggle_mode(true);
				eye_btn->set_custom_minimum_size(Size2(34 * EDSCALE, 28 * EDSCALE));
				_apply_rounded_btn_style(eye_btn);
				eye_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_pooled_layer_eye_toggled).bind(eye_btn));
				row->add_child(eye_btn);

				sel_btn = memnew(Button);
				sel_btn->set_toggle_mode(true);
				sel_btn->set_clip_text(true);
				sel_btn->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
				sel_btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
				sel_btn->set_custom_minimum_size(Size2(0, 28 * EDSCALE));
				sel_btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
				_apply_rounded_btn_style(sel_btn);
				sel_btn->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_pooled_layer_row_pressed).bind(sel_btn));
				row->add_child(sel_btn);
			}

			eye_btn->set_pressed_no_signal(vis);
			eye_btn->set_tooltip_text(vis ? String::utf8("Ocultar capa") : String::utf8("Mostrar capa"));
			eye_btn->set_text(vis ? "Ver" : "---");
			eye_btn->set_meta("cg_elem_id", eid);

			sel_btn->set_text(ename + " [C" + itos(lo) + "]");
			sel_btn->set_pressed_no_signal(is_sel);
			sel_btn->set_meta("cg_elem_id", eid);
		}
	}

	bool has_sel = !selected_elem_id.is_empty();
	if (layer_up_btn) layer_up_btn->set_disabled(!has_sel);
	if (layer_down_btn) layer_down_btn->set_disabled(!has_sel);
	if (layer_del_btn) layer_del_btn->set_disabled(!has_sel);
}

void ClockGearsUiDesignEditor::_refresh_style_binding_action_options() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	bool prev_upd = updating_ui;
	updating_ui = true;

	Array styles = dm->get_window_styles();
	if (prop_style_opt && (cached_opts_styles_cnt != styles.size() || prop_style_opt->get_item_count() == 0)) {
		prop_style_opt->clear();
		prop_style_opt->add_item("(Ninguno)", 0);
		prop_style_opt->set_item_metadata(0, String(""));
		for (int i = 0; i < styles.size(); i++) {
			if (styles[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary st = styles[i];
			int idx = prop_style_opt->get_item_count();
			prop_style_opt->add_item(String(st.get("name", st.get("id", ""))), idx);
			prop_style_opt->set_item_metadata(idx, String(st.get("id", "")));
		}
		cached_opts_styles_cnt = styles.size();
	}

	if (prop_binding_opt && prop_binding_opt->get_item_count() == 0) {
		prop_binding_opt->clear();
		prop_binding_opt->add_item("(Sin enlace)", 0);
		prop_binding_opt->set_item_metadata(0, String("none"));
		Array bindings = ClockGearsDataManager::get_ui_data_bindings();
		for (int i = 0; i < bindings.size(); i++) {
			Dictionary bd = bindings[i];
			int idx = prop_binding_opt->get_item_count();
			prop_binding_opt->add_item(String(bd.get("label", bd.get("id", ""))), idx);
			prop_binding_opt->set_item_metadata(idx, String(bd.get("id", "")));
		}
	}

	if (prop_action_opt && (cached_opts_screen_id != current_screen_id || prop_action_opt->get_item_count() == 0)) {
		prop_action_opt->clear();
		Array actions = ClockGearsDataManager::get_ui_button_actions(current_screen_id);
		for (int i = 0; i < actions.size(); i++) {
			Dictionary ad = actions[i];
			prop_action_opt->add_item(String(ad.get("label", ad.get("id", ""))), i);
			prop_action_opt->set_item_metadata(i, String(ad.get("id", "")));
		}
		cached_opts_screen_id = current_screen_id;
	}

	if (prop_add_action_opt && prop_add_action_opt->get_item_count() == 0) {
		prop_add_action_opt->clear();
		Array all_acts = ClockGearsDataManager::get_all_ui_button_action_types();
		for (int i = 0; i < all_acts.size(); i++) {
			Dictionary ad = all_acts[i];
			prop_add_action_opt->add_item(String(ad.get("label", ad.get("id", ""))), i);
			prop_add_action_opt->set_item_metadata(i, String(ad.get("id", "")));
		}
	}

	if (prop_font_opt && (!cached_fonts_scanned || prop_font_opt->get_item_count() == 0)) {
		prop_font_opt->clear();
		prop_font_opt->add_item("(Fuente por defecto)", 0);
		prop_font_opt->set_item_metadata(0, String(""));
		const char *k_dirs[2] = { "res://fonts/ui", "res://fonts" };
		for (int d = 0; d < 2; d++) {
			Ref<DirAccess> da = DirAccess::open(k_dirs[d]);
			if (da.is_valid()) {
				da->list_dir_begin();
				String fn = da->get_next();
				while (!fn.is_empty()) {
					if (!da->current_is_dir() && (fn.to_lower().ends_with(".ttf") || fn.to_lower().ends_with(".otf"))) {
						int idx = prop_font_opt->get_item_count();
						prop_font_opt->add_item(fn, idx);
						prop_font_opt->set_item_metadata(idx, String(k_dirs[d]).path_join(fn));
					}
					fn = da->get_next();
				}
			}
		}
		cached_fonts_scanned = true;
	}

	Dictionary vs = dm->get_variables_names();
	if (prop_var_opt && (cached_opts_vars_cnt != vs.size() || prop_var_opt->get_item_count() == 0)) {
		prop_var_opt->clear();
		prop_var_opt->add_item("(Elegir variable)", 0);
		prop_var_opt->set_item_metadata(0, String(""));
		Array keys = vs.keys();
		keys.sort();
		for (int i = 0; i < keys.size(); i++) {
			String k = String(keys[i]);
			String vn = String(vs[k]).strip_edges();
			if (!vn.is_empty()) {
				int idx = prop_var_opt->get_item_count();
				prop_var_opt->add_item(vformat("#%s: %s", k, vn), idx);
				prop_var_opt->set_item_metadata(idx, vn);
			}
		}
		cached_opts_vars_cnt = vs.size();
	}

	Array items = dm->get_category_array(CAT_ITEMS);
	if (prop_item_opt && (cached_opts_items_cnt != items.size() || prop_item_opt->get_item_count() == 0)) {
		prop_item_opt->clear();
		prop_item_opt->add_item("(Elegir objeto)", 0);
		prop_item_opt->set_item_metadata(0, String(""));
		for (int i = 0; i < items.size(); i++) {
			if (items[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary it = items[i];
			int iid = int(it.get("id", 0));
			String iname = String(it.get("name", "")).strip_edges();
			if (iname.is_empty()) {
				iname = "Objeto #" + itos(iid);
			}
			int idx = prop_item_opt->get_item_count();
			prop_item_opt->add_item(iname, idx);
			prop_item_opt->set_item_metadata(idx, dm->get_record_inventory_key(CAT_ITEMS, iid));
		}
		cached_opts_items_cnt = items.size();
	}

	updating_ui = prev_upd;
}

Dictionary ClockGearsUiDesignEditor::_get_selected_element_copy() const {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return Dictionary();
	}
	return dm->get_ui_element(current_screen_id, selected_elem_id);
}

void ClockGearsUiDesignEditor::_refresh_recent_colors_palette() {
	if (!prop_recent_colors_box) {
		return;
	}
	s_clear_container_children(prop_recent_colors_box);
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	PackedStringArray cols = dm->get_recent_ui_colors();
	int lim = MIN(6, cols.size());
	for (int i = 0; i < lim; i++) {
		String hex = cols[i];
		if (!Color::html_is_valid(hex)) {
			continue;
		}
		Button *cb = memnew(Button);
		cb->set_custom_minimum_size(Size2(22 * EDSCALE, 22 * EDSCALE));
		cb->set_tooltip_text(hex);
		Ref<StyleBoxFlat> sb = memnew(StyleBoxFlat);
		sb->set_bg_color(Color::html(hex));
		sb->set_corner_radius_all(int(4 * EDSCALE));
		sb->set_border_width_all(1);
		sb->set_border_color(Color(0.85f, 0.85f, 0.90f, 0.7f));
		cb->add_theme_style_override("normal", sb);
		cb->add_theme_style_override("hover", sb);
		cb->add_theme_style_override("pressed", sb);
		if (prop_fill_color_btn) {
			cb->connect("pressed", callable_mp(prop_fill_color_btn, &ColorPickerButton::set_pick_color).bind(Color::html(hex)));
			cb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed));
		}
		prop_recent_colors_box->add_child(cb);
	}
}

void ClockGearsUiDesignEditor::_refresh_image_thumbs_gallery() {
	if (!prop_img_thumbs_flow) {
		return;
	}
	if (cached_thumbs_scanned && prop_img_thumbs_flow->get_child_count() > 0) {
		return;
	}
	cached_thumbs_scanned = true;
	s_clear_container_children(prop_img_thumbs_flow);
	Ref<DirAccess> da = DirAccess::open("res://img/ui");
	if (da.is_null()) {
		return;
	}
	da->list_dir_begin();
	int count = 0;
	String fn = da->get_next();
	while (!fn.is_empty() && count < 8) {
		String low = fn.to_lower();
		if (!da->current_is_dir() && (low.ends_with(".png") || low.ends_with(".webp") || low.ends_with(".jpg"))) {
			String full_p = "res://img/ui/" + fn;
			Ref<Texture2D> tex;
			if (thumb_tex_cache.has(full_p)) {
				tex = thumb_tex_cache[full_p];
			} else {
				Ref<Image> img = memnew(Image);
				if (img->load(full_p) == OK && !img->is_empty()) {
					tex = ImageTexture::create_from_image(img);
					thumb_tex_cache[full_p] = tex;
				}
			}
			if (tex.is_valid()) {
				Button *tb = memnew(Button);
				tb->set_custom_minimum_size(Size2(34 * EDSCALE, 34 * EDSCALE));
				tb->set_tooltip_text(full_p);
				tb->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
				tb->set_expand_icon(true);
				tb->set_button_icon(tex);
				_apply_rounded_btn_style(tb);
				if (prop_image_edit) {
					tb->connect("pressed", callable_mp(prop_image_edit, &LineEdit::set_text).bind(full_p));
					tb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_field_changed));
				}
				prop_img_thumbs_flow->add_child(tb);
				count++;
			}
		}
		fn = da->get_next();
	}
}

void ClockGearsUiDesignEditor::_refresh_multi_actions_ui(const Dictionary &p_elem) {
	if (!prop_multi_actions_vbox) {
		return;
	}
	s_clear_container_children(prop_multi_actions_vbox);
	Array acts = p_elem.get("actions", Array());
	for (int i = 0; i < acts.size(); i++) {
		if (acts[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary a = acts[i];
		HBoxContainer *hb = memnew(HBoxContainer);
		hb->add_theme_constant_override("separation", int(4 * EDSCALE));
		Label *l = memnew(Label(itos(i + 1) + ". " + String(a.get("type", ""))));
		l->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		l->set_clip_text(true);
		hb->add_child(l);
		Button *rb = memnew(Button("x"));
		s_make_rounded_action_btn(rb);
		rb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_prop_remove_multi_action_pressed).bind(i));
		hb->add_child(rb);
		prop_multi_actions_vbox->add_child(hb);
	}
}

void ClockGearsUiDesignEditor::_apply_properties_filter() {
	String q = props_search_query.strip_edges().to_lower();
	for (int i = 0; i < prop_filter_entries.size(); i++) {
		const PropRowFilterEntry &ent = prop_filter_entries[i];
		if (!ent.row) {
			continue;
		}
		bool ok_mode = props_advanced_mode || !ent.advanced_only || !q.is_empty();
		bool ok_query = q.is_empty() || ent.search_text.contains(q);
		ent.row->set_visible(ok_mode && ok_query);
	}
}

void ClockGearsUiDesignEditor::set_props_advanced_mode(bool p_adv) {
	props_advanced_mode = p_adv;
	if (props_mode_simple_btn) props_mode_simple_btn->set_pressed(!p_adv);
	if (props_mode_adv_btn) props_mode_adv_btn->set_pressed(p_adv);
	_apply_properties_filter();
}

void ClockGearsUiDesignEditor::set_props_search_filter(const String &p_query) {
	props_search_query = p_query;
	if (props_search_edit && props_search_edit->get_text() != p_query) {
		props_search_edit->set_text(p_query);
	}
	_apply_properties_filter();
}

int ClockGearsUiDesignEditor::get_visible_prop_row_count() const {
	int cnt = 0;
	for (int i = 0; i < prop_filter_entries.size(); i++) {
		if (prop_filter_entries[i].row && prop_filter_entries[i].row->is_visible_in_tree()) {
			cnt++;
		}
	}
	return cnt;
}

void ClockGearsUiDesignEditor::_on_props_mode_changed(bool p_advanced) {
	set_props_advanced_mode(p_advanced);
}

void ClockGearsUiDesignEditor::_on_props_search_changed(const String &p_text) {
	props_search_query = p_text;
	_apply_properties_filter();
}

void ClockGearsUiDesignEditor::_refresh_properties_panel() {
	Dictionary el = _get_selected_element_copy();
	if (el.is_empty()) {
		if (props_sel_lbl) {
			props_sel_lbl->set_text(String::utf8("Selecciona un elemento o ajusta el menú abajo."));
		}
		if (props_fields_box) {
			props_fields_box->hide();
		}
		_apply_properties_filter();
		return;
	}

	bool prev_upd = updating_ui;
	updating_ui = true;

	String etype = String(el.get("type", "text"));
	if (props_sel_lbl) {
		props_sel_lbl->set_text("Tipo: " + etype + "  |  ID: " + String(el.get("id", "")));
	}
	if (props_fields_box) {
		props_fields_box->show();
	}

	prop_name_edit->set_text(String(el.get("name", "")));
	int anc_i = _anchor_index_by_key(String(el.get("anchor", "top_left")));
	for (int i = 0; i < 9; i++) {
		if (anchor_btns[i]) {
			anchor_btns[i]->set_pressed(i == anc_i);
		}
	}
	prop_ox_spin->set_value(int(el.get("offset_x", 0)));
	prop_oy_spin->set_value(int(el.get("offset_y", 0)));
	prop_w_spin->set_value(int(el.get("width", 120)));
	prop_h_spin->set_value(int(el.get("height", 36)));
	prop_layer_spin->set_value(int(el.get("layer_order", 0)));
	prop_visible_chk->set_pressed(bool(el.get("visible", true)));
	if (prop_show_bg_chk) {
		prop_show_bg_chk->set_pressed(bool(el.get("show_background", false)));
	}
	s_select_option_meta(prop_style_opt, String(el.get("window_style_id", "")));

	s_select_option_meta(prop_shape_opt, String(el.get("shape", "rect")));
	int crad = int(el.get("corner_radius", 8));
	if (prop_radius_slider) prop_radius_slider->set_value(crad);
	if (prop_radius_val_lbl) prop_radius_val_lbl->set_text(itos(crad) + "px");
	String fcol = String(el.get("fill_color", "#232a3a"));
	if (prop_fill_color_btn && Color::html_is_valid(fcol)) prop_fill_color_btn->set_pick_color(Color::html(fcol));
	if (prop_grad_chk) prop_grad_chk->set_pressed(bool(el.get("use_gradient", false)));
	String gt = String(el.get("gradient_top", "#2b3348"));
	String gb = String(el.get("gradient_bottom", "#181d2a"));
	if (prop_grad_top_btn && Color::html_is_valid(gt)) prop_grad_top_btn->set_pick_color(Color::html(gt));
	if (prop_grad_bot_btn && Color::html_is_valid(gb)) prop_grad_bot_btn->set_pick_color(Color::html(gb));
	if (prop_border_w_slider) prop_border_w_slider->set_value(int(el.get("border_width", 2)));
	String bcol = String(el.get("border_color", "#d8a040"));
	if (prop_border_col_btn && Color::html_is_valid(bcol)) prop_border_col_btn->set_pick_color(Color::html(bcol));
	if (prop_shadow_chk) prop_shadow_chk->set_pressed(bool(el.get("shadow", false)));
	if (prop_glow_chk) prop_glow_chk->set_pressed(bool(el.get("glow", false)));
	String gcol = String(el.get("glow_color", "#f0b44c"));
	if (prop_glow_col_btn && Color::html_is_valid(gcol)) prop_glow_col_btn->set_pick_color(Color::html(gcol));
	if (prop_clip_chk) prop_clip_chk->set_pressed(bool(el.get("clip_content", false)));
	if (prop_pixel_art_chk) prop_pixel_art_chk->set_pressed(bool(el.get("pixel_art", true)));
	_refresh_recent_colors_palette();

	bool has_text = (etype == "text" || etype == "button" || etype == "variable" || etype == "bar");
	prop_text_row->set_visible(has_text);
	if (has_text) {
		prop_text_edit->set_text(etype == "bar" ? String(el.get("label_text", "")) : String(el.get("text", "")));
		s_select_option_meta(prop_font_opt, String(el.get("font_path", "")));
		if (prop_font_size_slider) prop_font_size_slider->set_value(int(el.get("font_size", 16)));
		String tcol = String(el.get("font_color", "#f5f2ea"));
		if (prop_font_color_btn && Color::html_is_valid(tcol)) prop_font_color_btn->set_pick_color(Color::html(tcol));
	}

	bool has_img = (etype == "image" || etype == "button" || etype == "currency" || etype == "item");
	prop_image_row->set_visible(has_img);
	if (has_img) {
		String ikey = (etype == "currency" || etype == "item") ? "icon_path" : "image_path";
		prop_image_edit->set_text(String(el.get(ikey, "")));
		_refresh_image_thumbs_gallery();
	}

	bool has_binding = (etype == "text" || etype == "button" || etype == "variable");
	prop_binding_row->set_visible(has_binding || etype == "item" || etype == "bar");
	String db = String(el.get("data_binding", "none"));
	s_select_option_meta(prop_binding_opt, db);
	bool show_var = (db == "variable" || etype == "variable" || (etype == "bar" && String(el.get("value_source", "")) == "variable"));
	bool show_item = (db == "item_qty" || etype == "item");
	prop_var_row->set_visible(show_var);
	prop_item_row->set_visible(show_item);
	if (show_var) {
		prop_var_edit->set_text(String(el.get("variable_name", "")));
		s_select_option_meta(prop_var_opt, String(el.get("variable_name", "")));
	}
	if (show_item) {
		s_select_option_meta(prop_item_opt, String(el.get("item_key", "")));
	}

	bool is_btn = (etype == "button");
	prop_action_row->set_visible(is_btn);
	if (is_btn) {
		s_select_option_meta(prop_action_opt, String(el.get("action", "")));
		prop_focus_spin->set_value(int(el.get("focus_order", 0)));
		Dictionary vc = el.get("visible_condition", Dictionary());
		Dictionary ec = el.get("enabled_condition", Dictionary());
		s_select_option_meta(prop_vis_mode_opt, String(vc.get("mode", "always")));
		if (prop_vis_sw_edit) prop_vis_sw_edit->set_text(String(vc.get("switch_name", "")));
		s_select_option_meta(prop_en_mode_opt, String(ec.get("mode", "always")));
		if (prop_en_sw_edit) prop_en_sw_edit->set_text(String(ec.get("switch_name", "")));
		_refresh_multi_actions_ui(el);
	}

	updating_ui = prev_upd;
	_apply_properties_filter();
}

void ClockGearsUiDesignEditor::_refresh_menu_global_properties() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary d = dm->get_ui_screen_design(current_screen_id);
	Dictionary bg = d.get("background", Dictionary());
	Dictionary tin = d.get("transition_in", Dictionary());
	Dictionary tout = d.get("transition_out", Dictionary());
	Dictionary lay = d.get("layout", Dictionary());
	Dictionary snd = d.get("sounds", Dictionary());

	bool prev_upd = updating_ui;
	updating_ui = true;

	s_select_option_meta(menu_bg_type_opt, String(bg.get("type", "default")));
	String mc = String(bg.get("color", "#141824"));
	String mg = String(bg.get("gradient_bottom", "#090c14"));
	if (menu_bg_col_btn && Color::html_is_valid(mc)) menu_bg_col_btn->set_pick_color(Color::html(mc));
	if (menu_bg_grad_bot_btn && Color::html_is_valid(mg)) menu_bg_grad_bot_btn->set_pick_color(Color::html(mg));
	if (menu_bg_img_edit) menu_bg_img_edit->set_text(String(bg.get("image_path", "")));
	if (menu_bg_scroll_x_spin) menu_bg_scroll_x_spin->set_value(int(bg.get("scroll_speed_x", 0)));
	if (menu_bg_scroll_y_spin) menu_bg_scroll_y_spin->set_value(int(bg.get("scroll_speed_y", 0)));
	if (menu_bg_veil_chk) menu_bg_veil_chk->set_pressed(bool(bg.get("dark_veil", false)));

	s_select_option_meta(menu_trans_in_opt, String(tin.get("type", "none")));
	s_select_option_meta(menu_trans_out_opt, String(tout.get("type", "none")));
	if (menu_trans_dur_slider) menu_trans_dur_slider->set_value(int(tin.get("duration_ms", 220)));
	s_select_option_meta(menu_trans_curve_opt, String(tin.get("curve", "ease_out")));

	s_select_option_meta(menu_ctx_opt, String(d.get("trigger_context", "map_only")));
	if (menu_pause_chk) menu_pause_chk->set_pressed(d.has("pause_on_open") ? bool(d["pause_on_open"]) : true);

	Array trs = d.get("triggers", Array());
	if (menu_triggers_lbl) {
		if (trs.is_empty()) {
			menu_triggers_lbl->set_text("Ninguna");
		} else {
			PackedStringArray parts;
			for (int i = 0; i < trs.size(); i++) {
				if (trs[i].get_type() == Variant::DICTIONARY) {
					parts.push_back(s_format_trigger_text(trs[i]));
				}
			}
			menu_triggers_lbl->set_text(String(", ").join(parts));
		}
	}
	if (menu_conflict_lbl) {
		Array confs = dm->get_menu_trigger_conflicts();
		String msg;
		for (int i = 0; i < confs.size(); i++) {
			Dictionary c = confs[i];
			if (String(c.get("menu_a", "")) == current_screen_id || String(c.get("menu_b", "")) == current_screen_id) {
				msg = String::utf8("⚠ Conflicto de tecla con otro menú en el mismo contexto.");
				break;
			}
		}
		menu_conflict_lbl->set_text(msg);
		menu_conflict_lbl->set_visible(!msg.is_empty());
	}

	s_select_option_meta(menu_layout_mode_opt, String(lay.get("mode", "free")));
	if (menu_radial_radius_slider) menu_radial_radius_slider->set_value(int(lay.get("radial_radius", 145)));
	if (menu_radial_start_slider) menu_radial_start_slider->set_value(float(lay.get("radial_start_angle_deg", -90)));
	if (menu_radial_sweep_slider) menu_radial_sweep_slider->set_value(float(lay.get("radial_sweep_angle_deg", 360)));
	if (menu_radial_cw_chk) menu_radial_cw_chk->set_pressed(lay.has("radial_clockwise") ? bool(lay["radial_clockwise"]) : true);

	if (menu_snd_open_edit) menu_snd_open_edit->set_text(String(snd.get("open", "")));
	if (menu_snd_close_edit) menu_snd_close_edit->set_text(String(snd.get("close", "")));
	if (menu_snd_move_edit) menu_snd_move_edit->set_text(String(snd.get("move", "")));
	if (menu_snd_accept_edit) menu_snd_accept_edit->set_text(String(snd.get("accept", "")));
	if (menu_snd_cancel_edit) menu_snd_cancel_edit->set_text(String(snd.get("cancel", "")));
	if (menu_bgm_edit) menu_bgm_edit->set_text(String(d.get("bgm_path", "")));

	updating_ui = prev_upd;
}

void ClockGearsUiDesignEditor::_refresh_undo_redo_buttons() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	int uc = dm->get_ui_design_undo_count(current_screen_id);
	int rc = dm->get_ui_design_redo_count(current_screen_id);
	if (undo_btn) {
		undo_btn->set_disabled(uc <= 0);
		undo_btn->set_text("Deshacer (" + itos(uc) + ")");
	}
	if (redo_btn) {
		redo_btn->set_disabled(rc <= 0);
		redo_btn->set_text("Rehacer (" + itos(rc) + ")");
	}
}

void ClockGearsUiDesignEditor::_notify_data_mutated(bool p_mark_dirty) {
	refresh_from_data_manager();
	if (p_mark_dirty) {
		emit_signal("design_modified");
	}
}

void ClockGearsUiDesignEditor::select_screen(const String &p_screen_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	String norm_sid = ClockGearsDataManager::normalize_menu_id(p_screen_id);
	if (dm && !dm->has_menu(norm_sid)) {
		Array menus = dm->get_all_menus_for_selector();
		if (!menus.is_empty()) {
			norm_sid = String(Dictionary(menus[0]).get("id", "title"));
		}
	}
	current_screen_id = norm_sid;
	selected_elem_id = "";
	if (dm) {
		Array elems = dm->get_ui_screen_elements(current_screen_id);
		if (!elems.is_empty() && elems[0].get_type() == Variant::DICTIONARY) {
			selected_elem_id = String(Dictionary(elems[0]).get("id", ""));
		}
	}
	refresh_from_data_manager();
}

void ClockGearsUiDesignEditor::set_canvas_zoom(int p_zoom) {
	canvas_zoom = CLAMP(p_zoom, 1, 3);
	for (int i = 0; i < 3; i++) {
		if (zoom_btns[i]) {
			zoom_btns[i]->set_pressed((i + 1) == canvas_zoom);
		}
	}
	_refresh_canvas_stage();
}

String ClockGearsUiDesignEditor::add_element_at(const String &p_type, const Vector2i &p_canvas_pos) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return String();
	}
	Dictionary el = dm->make_default_ui_element(p_type, current_screen_id);
	el["anchor"] = "top_left";
	int w = int(el.get("width", 120));
	int h = int(el.get("height", 36));
	if (p_canvas_pos.x >= 0 && p_canvas_pos.y >= 0) {
		int cx = CLAMP(p_canvas_pos.x - w / 2, 0, BASE_W - w);
		int cy = CLAMP(p_canvas_pos.y - h / 2, 0, BASE_H - h);
		el["offset_x"] = cx;
		el["offset_y"] = cy;
	} else {
		Array existing = dm->get_ui_screen_elements(current_screen_id);
		el["offset_x"] = CLAMP(36 + existing.size() * 14, 0, BASE_W - w);
		el["offset_y"] = CLAMP(36 + existing.size() * 14, 0, BASE_H - h);
	}
	String new_id = dm->add_ui_element(current_screen_id, el, true);
	if (!new_id.is_empty()) {
		selected_elem_id = new_id;
		_notify_data_mutated(true);
	}
	return new_id;
}

bool ClockGearsUiDesignEditor::select_element(const String &p_elem_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	if (p_elem_id.is_empty()) {
		selected_elem_id = "";
		_refresh_layers_list();
		_refresh_properties_panel();
		if (canvas_overlay) canvas_overlay->queue_redraw();
		return true;
	}
	Dictionary el = dm->get_ui_element(current_screen_id, p_elem_id);
	if (el.is_empty()) {
		return false;
	}
	selected_elem_id = String(el.get("id", p_elem_id));
	_refresh_layers_list();
	_refresh_properties_panel();
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
	return true;
}

bool ClockGearsUiDesignEditor::delete_selected_element() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return false;
	}
	if (!dm->delete_ui_element(current_screen_id, selected_elem_id, true)) {
		return false;
	}
	selected_elem_id = "";
	Array rem = dm->get_ui_screen_elements(current_screen_id);
	if (!rem.is_empty() && rem[0].get_type() == Variant::DICTIONARY) {
		selected_elem_id = String(Dictionary(rem[0]).get("id", ""));
	}
	_notify_data_mutated(true);
	return true;
}

bool ClockGearsUiDesignEditor::toggle_element_visibility(const String &p_elem_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	Dictionary el = dm->get_ui_element(current_screen_id, p_elem_id);
	if (el.is_empty()) {
		return false;
	}
	el["visible"] = !bool(el.get("visible", true));
	if (dm->update_ui_element(current_screen_id, p_elem_id, el, true)) {
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

bool ClockGearsUiDesignEditor::move_selected_layer(int p_delta) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return false;
	}
	if (dm->move_ui_element_layer(current_screen_id, selected_elem_id, p_delta, true)) {
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

bool ClockGearsUiDesignEditor::apply_undo() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !dm->undo_ui_design(current_screen_id)) {
		return false;
	}
	_notify_data_mutated(true);
	return true;
}

bool ClockGearsUiDesignEditor::apply_redo() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !dm->redo_ui_design(current_screen_id)) {
		return false;
	}
	_notify_data_mutated(true);
	return true;
}

void ClockGearsUiDesignEditor::request_reset_to_default(bool p_confirmed_directly) {
	if (p_confirmed_directly) {
		_on_reset_confirmed();
		return;
	}
	if (reset_confirm_dialog) {
		reset_confirm_dialog->popup_centered();
	}
}

void ClockGearsUiDesignEditor::_begin_inline_text_edit(const String &p_elem_id) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || !inline_text_edit || !canvas_overlay || p_elem_id.is_empty()) {
		return;
	}
	Dictionary el = dm->get_ui_element(current_screen_id, p_elem_id);
	if (el.is_empty()) {
		return;
	}
	String et = String(el.get("type", "text"));
	if (et != "text" && et != "button" && et != "bar" && et != "variable") {
		return;
	}
	float scale = float(MAX(1, canvas_zoom));
	Rect2 stage(Vector2(), Vector2(BASE_W * scale, BASE_H * scale));
	Rect2 r = ClockGearsUiDesignLayer::compute_element_rect(el, stage, scale);
	inline_edit_elem_id = p_elem_id;
	inline_text_edit->set_position(r.position);
	inline_text_edit->set_size(Vector2(MAX(96.0f, r.size.x), MAX(28.0f, r.size.y)));
	inline_text_edit->set_text(et == "bar" ? String(el.get("label_text", "")) : String(el.get("text", "")));
	inline_text_edit->show();
	inline_text_edit->grab_focus();
	inline_text_edit->select_all();
}

void ClockGearsUiDesignEditor::_commit_inline_text_edit() {
	if (!inline_text_edit || !inline_text_edit->is_visible() || inline_edit_elem_id.is_empty()) {
		return;
	}
	String eid = inline_edit_elem_id;
	String txt = inline_text_edit->get_text();
	inline_edit_elem_id = "";
	inline_text_edit->hide();
	if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
		Dictionary el = dm->get_ui_element(current_screen_id, eid);
		if (!el.is_empty()) {
			String et = String(el.get("type", "text"));
			if (et == "bar") {
				el["label_text"] = txt;
			} else {
				el["text"] = txt;
			}
			dm->update_ui_element(current_screen_id, eid, el, true);
			_notify_data_mutated(true);
		}
	}
}

bool ClockGearsUiDesignEditor::begin_inline_edit_for_selected() {
	if (selected_elem_id.is_empty()) {
		return false;
	}
	_begin_inline_text_edit(selected_elem_id);
	return inline_text_edit && inline_text_edit->is_visible();
}

void ClockGearsUiDesignEditor::show_canvas_context_menu(const Vector2 &p_screen_pos) {
	if (!canvas_context_menu) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	bool has_sel = !selected_elem_id.is_empty();
	bool has_clip = dm && dm->get_ui_clipboard_count() > 0;
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(1), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(2), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(3), !has_clip);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(4), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(5), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(6), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(7), !has_sel);
	canvas_context_menu->set_item_disabled(canvas_context_menu->get_item_index(8), !has_sel);
	canvas_context_menu->set_position(Vector2i(p_screen_pos));
	canvas_context_menu->popup();
}

void ClockGearsUiDesignEditor::_on_canvas_context_menu_id(int p_id) {
	if (p_id == 1) {
		duplicate_selected_element();
	} else if (p_id == 2) {
		copy_selected_to_clipboard();
	} else if (p_id == 3) {
		paste_from_clipboard();
	} else if (p_id == 4) {
		move_selected_layer(1);
	} else if (p_id == 5) {
		move_selected_layer(-1);
	} else if (p_id == 6) {
		align_selected_elements("center_both");
	} else if (p_id == 7) {
		save_tpl_is_component = true;
		if (save_tpl_name_edit) save_tpl_name_edit->set_text("Mi componente");
		if (save_tpl_name_dialog) save_tpl_name_dialog->popup_centered();
	} else if (p_id == 8) {
		delete_selected_element();
	}
}

bool ClockGearsUiDesignEditor::align_selected_elements(const String &p_mode) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	if (p_mode == "distribute_v") {
		Array elems = dm->get_ui_screen_elements(current_screen_id);
		if (elems.size() < 2) {
			return false;
		}
		Dictionary prev_d = dm->get_ui_screen_design(current_screen_id);
		dm->push_ui_design_undo_state(current_screen_id, prev_d);
		int start_y = 90;
		int step = (BASE_H - 180) / MAX(1, elems.size());
		for (int i = 0; i < elems.size(); i++) {
			Dictionary el = Dictionary(elems[i]).duplicate(true);
			el["anchor"] = "top_left";
			el["offset_y"] = start_y + i * step;
			dm->update_ui_element(current_screen_id, String(el.get("id", "")), el, false);
		}
		_notify_data_mutated(true);
		return true;
	}
	if (selected_elem_id.is_empty()) {
		return false;
	}
	Dictionary el = _get_selected_element_copy();
	if (el.is_empty()) {
		return false;
	}
	int w = int(el.get("width", 120));
	int h = int(el.get("height", 36));
	el["anchor"] = "top_left";
	if (p_mode == "center_h" || p_mode == "center_both") {
		el["offset_x"] = (BASE_W - w) / 2;
	}
	if (p_mode == "center_v" || p_mode == "center_both") {
		el["offset_y"] = (BASE_H - h) / 2;
	}
	dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
	_notify_data_mutated(true);
	return true;
}

bool ClockGearsUiDesignEditor::duplicate_selected_element() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return false;
	}
	String nid = dm->duplicate_ui_element(current_screen_id, selected_elem_id, Vector2i(16, 16));
	if (!nid.is_empty()) {
		selected_elem_id = nid;
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

void ClockGearsUiDesignEditor::copy_selected_to_clipboard() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || selected_elem_id.is_empty()) {
		return;
	}
	PackedStringArray ids;
	ids.push_back(selected_elem_id);
	dm->copy_ui_elements_to_clipboard(current_screen_id, ids);
}

PackedStringArray ClockGearsUiDesignEditor::paste_from_clipboard() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return PackedStringArray();
	}
	PackedStringArray added = dm->paste_ui_elements_from_clipboard(current_screen_id);
	if (!added.is_empty()) {
		selected_elem_id = added[0];
		_notify_data_mutated(true);
	}
	return added;
}

void ClockGearsUiDesignEditor::set_radial_layout_live(int p_radius, float p_start_deg) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary d = dm->get_ui_screen_design(current_screen_id).duplicate(true);
	Dictionary lay = d.get("layout", Dictionary());
	lay["mode"] = "radial";
	lay["radial_radius"] = CLAMP(p_radius, 40, 260);
	lay["radial_start_angle_deg"] = CLAMP(p_start_deg, -360.0f, 360.0f);
	d["layout"] = lay;
	dm->set_ui_screen_design(current_screen_id, d, false);
	if (design_layer) {
		design_layer->apply_screen_design(current_screen_id, d);
	}
	if (canvas_overlay) {
		canvas_overlay->queue_redraw();
	}
}

bool ClockGearsUiDesignEditor::add_radial_slot_from_editor() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return false;
	}
	Dictionary d = dm->get_ui_screen_design(current_screen_id).duplicate(true);
	Dictionary lay = d.get("layout", Dictionary());
	lay["mode"] = "radial";
	Array rslots = lay.get("radial_slots", Array());
	int next_idx = rslots.size() + 1;
	Dictionary btn = dm->make_default_ui_element("button", current_screen_id);
	btn["name"] = "Ranura " + itos(next_idx);
	btn["text"] = "Ranura " + itos(next_idx);
	btn["width"] = 104;
	btn["height"] = 44;
	btn["shape"] = "pill";
	btn["corner_radius"] = 22;
	btn["focus_order"] = next_idx * 10;
	d["layout"] = lay;
	dm->set_ui_screen_design(current_screen_id, d, true);
	String bid = dm->add_ui_element(current_screen_id, btn, false);
	if (!bid.is_empty()) {
		Dictionary d2 = dm->get_ui_screen_design(current_screen_id).duplicate(true);
		Dictionary lay2 = d2.get("layout", Dictionary());
		Array rs2 = lay2.get("radial_slots", Array());
		rs2.push_back(bid);
		lay2["radial_slots"] = rs2;
		d2["layout"] = lay2;
		dm->set_ui_screen_design(current_screen_id, d2, false);
		selected_elem_id = bid;
		_notify_data_mutated(true);
		return true;
	}
	return false;
}

void ClockGearsUiDesignEditor::_on_align_center_pressed(bool p_vertical) {
	align_selected_elements(p_vertical ? "center_v" : "center_h");
}
void ClockGearsUiDesignEditor::_on_distribute_vertical_pressed() {
	align_selected_elements("distribute_v");
}
void ClockGearsUiDesignEditor::_on_duplicate_elem_pressed() {
	duplicate_selected_element();
}
void ClockGearsUiDesignEditor::_on_copy_elem_pressed() {
	copy_selected_to_clipboard();
}
void ClockGearsUiDesignEditor::_on_paste_elem_pressed() {
	paste_from_clipboard();
}

void ClockGearsUiDesignEditor::_update_wizard_step_ui() {
	for (int i = 0; i < 3; i++) {
		if (wizard_step_panels[i]) {
			wizard_step_panels[i]->set_visible(i == wizard_step);
		}
	}
	if (wizard_step_lbl) {
		if (wizard_step == 0) wizard_step_lbl->set_text(String::utf8("Paso 1 de 3: Elige una plantilla con vista previa"));
		else if (wizard_step == 1) wizard_step_lbl->set_text(String::utf8("Paso 2 de 3: Nombre y botones iniciales"));
		else wizard_step_lbl->set_text(String::utf8("Paso 3 de 3: Cómo se abre y pausa"));
	}
	if (wizard_prev_btn) wizard_prev_btn->set_disabled(wizard_step <= 0);
	if (wizard_next_btn) wizard_next_btn->set_visible(wizard_step < 2);
	if (wizard_finish_btn) wizard_finish_btn->set_visible(wizard_step == 2);
	if (wizard_triggers_lbl) {
		if (capturing_key_for_wizard) {
			wizard_triggers_lbl->set_text(String::utf8("Pulsa una tecla o botón de mando..."));
		} else if (wizard_triggers.is_empty()) {
			wizard_triggers_lbl->set_text("Sin tecla asignada (opcional)");
		} else {
			wizard_triggers_lbl->set_text("Entrada: " + s_format_trigger_text(wizard_triggers[0]));
		}
	}
}

void ClockGearsUiDesignEditor::_refresh_wizard_conflict_label() {
	if (!wizard_conflict_lbl) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || wizard_triggers.is_empty() || !wizard_ctx_opt) {
		wizard_conflict_lbl->hide();
		return;
	}
	String ctx = String(wizard_ctx_opt->get_item_metadata(wizard_ctx_opt->get_selected()));
	Dictionary wtr = wizard_triggers[0];
	Array menus = dm->get_all_menus_for_selector();
	for (int i = 0; i < menus.size(); i++) {
		String mid = String(Dictionary(menus[i]).get("id", ""));
		Dictionary md = dm->get_ui_screen_design(mid);
		String mctx = String(md.get("trigger_context", "map_only"));
		if (mctx != "any" && ctx != "any" && mctx != ctx) {
			continue;
		}
		Array mtrs = md.get("triggers", Array());
		for (int j = 0; j < mtrs.size(); j++) {
			Dictionary mt = mtrs[j];
			if (String(mt.get("kind", "")) == String(wtr.get("kind", "")) &&
					int(mt.get("keycode", 0)) == int(wtr.get("keycode", 0)) &&
					int(mt.get("button_index", 0)) == int(wtr.get("button_index", 0))) {
				wizard_conflict_lbl->set_text(String::utf8("⚠ Esa entrada ya abre: ") + String(Dictionary(menus[i]).get("name", mid)));
				wizard_conflict_lbl->show();
				return;
			}
		}
	}
	wizard_conflict_lbl->hide();
}

void ClockGearsUiDesignEditor::_open_menu_wizard() {
	if (!menu_wizard_dialog) {
		return;
	}
	wizard_step = 0;
	capturing_key_for_wizard = false;
	wizard_triggers.clear();
	if (wizard_tpl_grid) {
		s_clear_container_children(wizard_tpl_grid);
		Array tpls = ClockGearsDataManager::get_builtin_menu_templates();
		for (int i = 0; i < tpls.size(); i++) {
			Dictionary t = tpls[i];
			String tid = String(t.get("id", ""));
			PanelContainer *card = memnew(PanelContainer);
			card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
			card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			VBoxContainer *vb = memnew(VBoxContainer);
			vb->add_theme_constant_override("separation", int(4 * EDSCALE));
			card->add_child(vb);
			ClockGearsTemplateThumbCtrl *th = memnew(ClockGearsTemplateThumbCtrl);
			th->set_custom_minimum_size(Size2(180 * EDSCALE, 101 * EDSCALE));
			th->setup_template(t);
			vb->add_child(th);
			Button *sb = memnew(Button(String(t.get("name", tid))));
			sb->set_toggle_mode(true);
			sb->set_pressed(tid == wizard_selected_template_id);
			sb->set_clip_text(true);
			s_make_rounded_action_btn(sb);
			sb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_wizard_tpl_selected).bind(tid));
			vb->add_child(sb);
			wizard_tpl_grid->add_child(card);
		}
	}
	_update_wizard_step_ui();
	_refresh_wizard_conflict_label();
	menu_wizard_dialog->popup_centered();
}

void ClockGearsUiDesignEditor::_on_wizard_tpl_selected(const String &p_tpl_id) {
	wizard_selected_template_id = p_tpl_id;
	if (wizard_tpl_grid) {
		for (int i = 0; i < wizard_tpl_grid->get_child_count(); i++) {
			PanelContainer *card = Object::cast_to<PanelContainer>(wizard_tpl_grid->get_child(i));
			if (card && card->get_child_count() > 0 && card->get_child(0)->get_child_count() > 1) {
				Button *b = Object::cast_to<Button>(card->get_child(0)->get_child(1));
				Dictionary t = ClockGearsDataManager::get_builtin_template_by_id(p_tpl_id);
				if (b) b->set_pressed(b->get_text() == String(t.get("name", p_tpl_id)));
			}
		}
	}
}

void ClockGearsUiDesignEditor::_on_wizard_prev_pressed() {
	wizard_step = MAX(0, wizard_step - 1);
	capturing_key_for_wizard = false;
	_update_wizard_step_ui();
}
void ClockGearsUiDesignEditor::_on_wizard_next_pressed() {
	wizard_step = MIN(2, wizard_step + 1);
	_update_wizard_step_ui();
}
void ClockGearsUiDesignEditor::_on_wizard_capture_key_pressed() {
	capturing_key_for_wizard = true;
	_update_wizard_step_ui();
}

String ClockGearsUiDesignEditor::create_menu_via_wizard(const Dictionary &p_opts) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return String();
	}
	String nid = dm->create_menu_from_wizard(p_opts);
	if (!nid.is_empty()) {
		select_screen(nid);
		_notify_data_mutated(true);
	}
	return nid;
}

void ClockGearsUiDesignEditor::_on_wizard_finish_pressed() {
	Dictionary opts;
	opts["template_id"] = wizard_selected_template_id;
	opts["name"] = wizard_name_edit ? wizard_name_edit->get_text().strip_edges() : String::utf8("Mi Menú");
	Array btns;
	for (int i = 0; i < wizard_btn_checks.size(); i++) {
		CheckBox *cb = wizard_btn_checks[i];
		if (cb && cb->is_pressed()) {
			Dictionary bd;
			bd["label"] = cb->get_meta("label");
			bd["action"] = cb->get_meta("action");
			btns.push_back(bd);
		}
	}
	opts["buttons"] = btns;
	opts["triggers"] = wizard_triggers;
	opts["trigger_context"] = wizard_ctx_opt ? String(wizard_ctx_opt->get_item_metadata(wizard_ctx_opt->get_selected())) : String("map_only");
	opts["pause_on_open"] = wizard_pause_chk ? wizard_pause_chk->is_pressed() : true;
	String nid = create_menu_via_wizard(opts);
	if (menu_wizard_dialog && !nid.is_empty()) {
		menu_wizard_dialog->hide();
	}
}

void ClockGearsUiDesignEditor::_populate_template_gallery() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	auto fill_grid = [&](GridContainer *p_grid, const Array &p_list) {
		if (!p_grid) return;
		s_clear_container_children(p_grid);
		for (int i = 0; i < p_list.size(); i++) {
			Dictionary t = p_list[i];
			String tid = String(t.get("id", ""));
			PanelContainer *card = memnew(PanelContainer);
			card->add_theme_style_override("panel", ClockGearsTheme::make_card_stylebox(EDSCALE));
			card->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			VBoxContainer *vb = memnew(VBoxContainer);
			vb->add_theme_constant_override("separation", int(4 * EDSCALE));
			card->add_child(vb);
			ClockGearsTemplateThumbCtrl *th = memnew(ClockGearsTemplateThumbCtrl);
			th->set_custom_minimum_size(Size2(192 * EDSCALE, 108 * EDSCALE));
			th->setup_template(t);
			vb->add_child(th);
			Button *sb = memnew(Button(String(t.get("name", tid))));
			sb->set_toggle_mode(true);
			sb->set_pressed(tid == selected_gallery_template_id);
			sb->set_clip_text(true);
			s_make_rounded_action_btn(sb);
			sb->connect("pressed", callable_mp(this, &ClockGearsUiDesignEditor::_on_tpl_card_selected).bind(tid));
			vb->add_child(sb);
			p_grid->add_child(card);
		}
	};
	fill_grid(tpl_screen_grid, ClockGearsDataManager::get_builtin_menu_templates());
	fill_grid(tpl_comp_grid, ClockGearsDataManager::get_builtin_component_templates());
	fill_grid(tpl_user_grid, dm ? dm->get_user_menu_templates() : Array());
}

void ClockGearsUiDesignEditor::_on_templates_btn_pressed() {
	if (!template_gallery_dialog) {
		return;
	}
	_populate_template_gallery();
	template_gallery_dialog->popup_centered();
}

void ClockGearsUiDesignEditor::_on_tpl_card_selected(const String &p_tpl_id) {
	selected_gallery_template_id = p_tpl_id;
	Dictionary t = ClockGearsDataManager::get_builtin_template_by_id(p_tpl_id);
	if (t.is_empty() && ClockGearsDataManager::get_singleton()) {
		Array ut = ClockGearsDataManager::get_singleton()->get_user_menu_templates();
		for (int i = 0; i < ut.size(); i++) {
			if (String(Dictionary(ut[i]).get("id", "")) == p_tpl_id) {
				t = ut[i];
				break;
			}
		}
	}
	if (tpl_selected_info_lbl) {
		tpl_selected_info_lbl->set_text(String(t.get("name", p_tpl_id)) + String::utf8(" — ") + String(t.get("description", "")));
	}
}

bool ClockGearsUiDesignEditor::apply_template_by_id(const String &p_tpl_id, bool p_preserve_buttons, bool p_record_undo) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm || p_tpl_id.is_empty()) {
		return false;
	}
	Dictionary t = ClockGearsDataManager::get_builtin_template_by_id(p_tpl_id);
	bool is_comp = bool(t.get("is_component", false));
	if (!is_comp) {
		Array ut = dm->get_user_menu_templates();
		for (int i = 0; i < ut.size(); i++) {
			if (String(Dictionary(ut[i]).get("id", "")) == p_tpl_id) {
				is_comp = bool(Dictionary(ut[i]).get("is_component", false));
				break;
			}
		}
	}
	bool ok = false;
	if (is_comp) {
		String eid = dm->instantiate_component_template(current_screen_id, p_tpl_id);
		if (!eid.is_empty()) {
			selected_elem_id = eid;
			ok = true;
		}
	} else {
		ok = dm->apply_menu_template(current_screen_id, p_tpl_id, p_preserve_buttons, p_record_undo);
		if (ok) {
			selected_elem_id = "";
		}
	}
	if (ok) {
		_notify_data_mutated(true);
	}
	return ok;
}

String ClockGearsUiDesignEditor::save_current_as_user_template(const String &p_name) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return String();
	}
	String tid = dm->save_screen_as_user_template(current_screen_id, p_name);
	_populate_template_gallery();
	_notify_data_mutated(true);
	return tid;
}

void ClockGearsUiDesignEditor::_on_tpl_apply_pressed() {
	if (selected_gallery_template_id.is_empty()) {
		return;
	}
	pending_apply_template_id = selected_gallery_template_id;
	if (template_apply_confirm_dialog) {
		template_apply_confirm_dialog->set_text(String::utf8("¿Aplicar esta plantilla a '") + current_screen_id + String::utf8("'? (Puedes usar Deshacer después)"));
		template_apply_confirm_dialog->popup_centered();
	} else {
		_on_tpl_apply_confirmed();
	}
}

void ClockGearsUiDesignEditor::_on_tpl_apply_confirmed() {
	bool preserve = tpl_preserve_btns_chk ? tpl_preserve_btns_chk->is_pressed() : true;
	if (apply_template_by_id(pending_apply_template_id, preserve, true) && template_gallery_dialog) {
		template_gallery_dialog->hide();
	}
}

void ClockGearsUiDesignEditor::_on_tpl_save_current_pressed() {
	save_tpl_is_component = false;
	if (save_tpl_name_edit) {
		save_tpl_name_edit->set_text("Mi plantilla " + current_screen_id);
	}
	if (save_tpl_name_dialog) {
		save_tpl_name_dialog->popup_centered();
	}
}

void ClockGearsUiDesignEditor::_on_tpl_save_name_confirmed() {
	String nm = save_tpl_name_edit ? save_tpl_name_edit->get_text().strip_edges() : "Mi plantilla";
	if (save_tpl_is_component) {
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			String tid = dm->save_element_as_component_template(current_screen_id, selected_elem_id, nm);
			if (!tid.is_empty()) selected_gallery_template_id = tid;
			_notify_data_mutated(true);
		}
	} else {
		String tid = save_current_as_user_template(nm);
		if (!tid.is_empty()) selected_gallery_template_id = tid;
	}
}

void ClockGearsUiDesignEditor::_on_tpl_export_pressed() {
	if (export_pkg_dialog) {
		export_pkg_dialog->popup_centered_ratio(0.55f);
	}
}

void ClockGearsUiDesignEditor::_on_tpl_import_pressed() {
	if (import_pkg_dialog) {
		import_pkg_dialog->popup_centered_ratio(0.55f);
	}
}

void ClockGearsUiDesignEditor::_on_tpl_delete_user_pressed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && !selected_gallery_template_id.is_empty() && dm->delete_user_template(selected_gallery_template_id)) {
		selected_gallery_template_id = "";
		_populate_template_gallery();
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_export_pkg_file_selected(const String &p_path) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	if (!selected_gallery_template_id.is_empty()) {
		dm->export_template_package(selected_gallery_template_id, p_path, false);
	} else {
		dm->export_template_package(current_screen_id, p_path, true);
	}
}

void ClockGearsUiDesignEditor::_on_import_pkg_file_selected(const String &p_path) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	Dictionary res = dm->import_template_package(p_path, false);
	if (bool(res.get("ok", false))) {
		selected_gallery_template_id = String(res.get("template_id", ""));
		_populate_template_gallery();
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_menu_list_select_pressed(const String &p_menu_id) {
	select_screen(p_menu_id);
}

void ClockGearsUiDesignEditor::_on_menu_list_dup_pressed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) return;
	String nid = dm->duplicate_menu(current_screen_id);
	if (!nid.is_empty()) {
		select_screen(nid);
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_menu_list_move_pressed(int p_delta) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && dm->move_custom_menu(current_screen_id, p_delta)) {
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_menu_list_del_pressed() {
	if (ClockGearsDataManager::is_system_menu_id(current_screen_id)) {
		return;
	}
	pending_delete_menu_id = current_screen_id;
	if (delete_menu_confirm_dialog) {
		delete_menu_confirm_dialog->set_text(String::utf8("¿Eliminar el menú personalizado '") + current_screen_id + "'?");
		delete_menu_confirm_dialog->popup_centered();
	} else {
		_on_menu_del_confirmed();
	}
}

void ClockGearsUiDesignEditor::_on_menu_del_confirmed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (dm && !pending_delete_menu_id.is_empty() && dm->delete_custom_menu(pending_delete_menu_id)) {
		pending_delete_menu_id = "";
		select_screen("title");
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_menu_restore_pressed() {
	request_reset_to_default(false);
}

void ClockGearsUiDesignEditor::_on_menu_global_changed() {
	if (updating_ui) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	Dictionary d = dm->get_ui_screen_design(current_screen_id).duplicate(true);
	Dictionary bg = d.get("background", Dictionary());
	if (menu_bg_type_opt && menu_bg_type_opt->get_selected() >= 0) {
		bg["type"] = String(menu_bg_type_opt->get_item_metadata(menu_bg_type_opt->get_selected()));
	}
	if (menu_bg_col_btn) bg["color"] = "#" + menu_bg_col_btn->get_pick_color().to_html(false);
	if (menu_bg_grad_bot_btn) bg["gradient_bottom"] = "#" + menu_bg_grad_bot_btn->get_pick_color().to_html(false);
	if (menu_bg_img_edit) bg["image_path"] = menu_bg_img_edit->get_text().strip_edges();
	if (menu_bg_scroll_x_spin) bg["scroll_speed_x"] = menu_bg_scroll_x_spin->get_value();
	if (menu_bg_scroll_y_spin) bg["scroll_speed_y"] = menu_bg_scroll_y_spin->get_value();
	if (menu_bg_veil_chk) bg["dark_veil"] = menu_bg_veil_chk->is_pressed();
	d["background"] = ClockGearsDataManager::normalize_ui_background(bg);

	Dictionary tin = d.get("transition_in", Dictionary());
	Dictionary tout = d.get("transition_out", Dictionary());
	if (menu_trans_in_opt && menu_trans_in_opt->get_selected() >= 0) {
		tin["type"] = String(menu_trans_in_opt->get_item_metadata(menu_trans_in_opt->get_selected()));
	}
	if (menu_trans_out_opt && menu_trans_out_opt->get_selected() >= 0) {
		tout["type"] = String(menu_trans_out_opt->get_item_metadata(menu_trans_out_opt->get_selected()));
	}
	if (menu_trans_dur_slider) {
		int dur = int(Math::round(menu_trans_dur_slider->get_value()));
		tin["duration_ms"] = dur;
		tout["duration_ms"] = dur;
	}
	if (menu_trans_curve_opt && menu_trans_curve_opt->get_selected() >= 0) {
		String cv = String(menu_trans_curve_opt->get_item_metadata(menu_trans_curve_opt->get_selected()));
		tin["curve"] = cv;
		tout["curve"] = cv;
	}
	d["transition_in"] = ClockGearsDataManager::normalize_ui_transition(tin);
	d["transition_out"] = ClockGearsDataManager::normalize_ui_transition(tout);

	if (menu_ctx_opt && menu_ctx_opt->get_selected() >= 0) {
		d["trigger_context"] = String(menu_ctx_opt->get_item_metadata(menu_ctx_opt->get_selected()));
	}
	if (menu_pause_chk) {
		d["pause_on_open"] = menu_pause_chk->is_pressed();
	}

	Dictionary lay = d.get("layout", Dictionary());
	if (menu_layout_mode_opt && menu_layout_mode_opt->get_selected() >= 0) {
		lay["mode"] = String(menu_layout_mode_opt->get_item_metadata(menu_layout_mode_opt->get_selected()));
	}
	if (menu_radial_radius_slider) lay["radial_radius"] = int(Math::round(menu_radial_radius_slider->get_value()));
	if (menu_radial_start_slider) lay["radial_start_angle_deg"] = menu_radial_start_slider->get_value();
	if (menu_radial_sweep_slider) lay["radial_sweep_angle_deg"] = menu_radial_sweep_slider->get_value();
	if (menu_radial_cw_chk) lay["radial_clockwise"] = menu_radial_cw_chk->is_pressed();
	d["layout"] = ClockGearsDataManager::normalize_menu_layout(lay);

	Dictionary snd = d.get("sounds", Dictionary());
	if (menu_snd_open_edit) snd["open"] = menu_snd_open_edit->get_text().strip_edges();
	if (menu_snd_close_edit) snd["close"] = menu_snd_close_edit->get_text().strip_edges();
	if (menu_snd_move_edit) snd["move"] = menu_snd_move_edit->get_text().strip_edges();
	if (menu_snd_accept_edit) snd["accept"] = menu_snd_accept_edit->get_text().strip_edges();
	if (menu_snd_cancel_edit) snd["cancel"] = menu_snd_cancel_edit->get_text().strip_edges();
	d["sounds"] = snd;
	if (menu_bgm_edit) d["bgm_path"] = menu_bgm_edit->get_text().strip_edges();

	dm->set_ui_screen_design(current_screen_id, d, false);
	_refresh_canvas_stage();
	_refresh_menu_global_properties();
	emit_signal("design_modified");
}

void ClockGearsUiDesignEditor::_on_menu_capture_key_pressed() {
	capturing_key_for_screen = true;
	if (menu_triggers_lbl) {
		menu_triggers_lbl->set_text("Pulsa tecla/mando...");
	}
}

void ClockGearsUiDesignEditor::_on_menu_radial_add_slot_pressed() {
	add_radial_slot_from_editor();
}

void ClockGearsUiDesignEditor::_on_menu_bg_import_pressed() {
	image_import_target = 1;
	_on_prop_import_image_pressed();
}

void ClockGearsUiDesignEditor::_on_prop_font_import_pressed() {
	if (!import_font_dialog) return;
	String proj_dir = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path("res://") : "";
	if (!proj_dir.is_empty()) {
		import_font_dialog->set_current_dir(proj_dir);
	}
	import_font_dialog->popup_centered_ratio(0.55f);
}

void ClockGearsUiDesignEditor::_on_font_file_selected(const String &p_path) {
	String target_dir = "res://fonts/ui";
	DirAccess::make_dir_recursive_absolute(target_dir);
	String fn = p_path.get_file();
	String dst = target_dir.path_join(fn);
	if (!p_path.begins_with("res://") && FileAccess::exists(p_path)) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (da.is_valid()) {
			da->copy(p_path, ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path(dst) : dst);
		}
	} else if (p_path.begins_with("res://")) {
		dst = p_path;
	}
	cached_fonts_scanned = false;
	_refresh_style_binding_action_options();
	if (prop_font_opt) {
		s_select_option_meta(prop_font_opt, dst);
		_on_prop_field_changed();
	}
}

void ClockGearsUiDesignEditor::_on_prop_add_multi_action_pressed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty() || !prop_add_action_opt || prop_add_action_opt->get_selected() < 0) {
		return;
	}
	String atype = String(prop_add_action_opt->get_item_metadata(prop_add_action_opt->get_selected()));
	Array acts = el.get("actions", Array()).duplicate(true);
	Dictionary na;
	na["type"] = atype;
	acts.push_back(na);
	el["actions"] = acts;
	dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_remove_multi_action_pressed(int p_idx) {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	Array acts = el.get("actions", Array()).duplicate(true);
	if (p_idx >= 0 && p_idx < acts.size()) {
		acts.remove_at(p_idx);
		el["actions"] = acts;
		dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_screen_selected(int p_idx) {
	if (updating_ui || !screen_opt || p_idx < 0) {
		return;
	}
	select_screen(String(screen_opt->get_item_metadata(p_idx)));
}

void ClockGearsUiDesignEditor::_on_zoom_pressed(int p_zoom) {
	set_canvas_zoom(p_zoom);
}

void ClockGearsUiDesignEditor::_on_undo_pressed() {
	apply_undo();
}

void ClockGearsUiDesignEditor::_on_redo_pressed() {
	apply_redo();
}

void ClockGearsUiDesignEditor::_on_reset_pressed() {
	request_reset_to_default(false);
}

void ClockGearsUiDesignEditor::_on_reset_confirmed() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return;
	}
	dm->reset_ui_screen_design_to_default(current_screen_id, true);
	selected_elem_id = "";
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_layer_eye_toggled(const String &p_elem_id) {
	toggle_element_visibility(p_elem_id);
}

void ClockGearsUiDesignEditor::_on_layer_row_pressed(const String &p_elem_id) {
	select_element(p_elem_id);
}

void ClockGearsUiDesignEditor::_on_layer_move_pressed(int p_delta) {
	move_selected_layer(p_delta);
}

void ClockGearsUiDesignEditor::_on_layer_delete_pressed() {
	delete_selected_element();
}

void ClockGearsUiDesignEditor::_on_anchor_btn_pressed(int p_idx) {
	if (updating_ui || selected_elem_id.is_empty()) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	el["anchor"] = _anchor_key_by_index(p_idx);
	dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_field_changed() {
	if (updating_ui || selected_elem_id.is_empty()) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	String etype = String(el.get("type", "text"));
	String old_name = String(el.get("name", ""));
	int old_layer = int(el.get("layer_order", 0));
	bool old_vis = bool(el.get("visible", true));
	bool was_custom = dm->has_custom_ui_design(current_screen_id);
	bool only_geom = (old_name == prop_name_edit->get_text() &&
			old_layer == int(Math::round(prop_layer_spin->get_value())) &&
			old_vis == prop_visible_chk->is_pressed() &&
			was_custom &&
			(!prop_show_bg_chk || bool(el.get("show_background", true)) == prop_show_bg_chk->is_pressed()));
	el["name"] = prop_name_edit->get_text();
	el["offset_x"] = int(Math::round(prop_ox_spin->get_value()));
	el["offset_y"] = int(Math::round(prop_oy_spin->get_value()));
	el["width"] = int(Math::round(prop_w_spin->get_value()));
	el["height"] = int(Math::round(prop_h_spin->get_value()));
	el["layer_order"] = int(Math::round(prop_layer_spin->get_value()));
	el["visible"] = prop_visible_chk->is_pressed();
	if (prop_show_bg_chk) {
		el["show_background"] = prop_show_bg_chk->is_pressed();
	}
	if (prop_style_opt && prop_style_opt->get_selected() >= 0) {
		String next_ws = String(prop_style_opt->get_item_metadata(prop_style_opt->get_selected()));
		if (String(el.get("window_style_id", "")) != next_ws) {
			only_geom = false;
		}
		el["window_style_id"] = next_ws;
	}
	if (prop_shape_opt && prop_shape_opt->get_selected() >= 0) {
		String nsh = String(prop_shape_opt->get_item_metadata(prop_shape_opt->get_selected()));
		if (String(el.get("shape", "rect")) != nsh) only_geom = false;
		el["shape"] = nsh;
	}
	if (prop_radius_slider) {
		int nr = int(Math::round(prop_radius_slider->get_value()));
		if (int(el.get("corner_radius", 8)) != nr) only_geom = false;
		el["corner_radius"] = nr;
		if (prop_radius_val_lbl) prop_radius_val_lbl->set_text(itos(nr) + "px");
	}
	if (prop_fill_color_btn) {
		String fc = "#" + prop_fill_color_btn->get_pick_color().to_html(false);
		if (String(el.get("fill_color", "")) != fc) {
			only_geom = false;
			dm->record_recent_ui_color(fc);
		}
		el["fill_color"] = fc;
	}
	if (prop_grad_chk) {
		bool ng = prop_grad_chk->is_pressed();
		if (bool(el.get("use_gradient", false)) != ng) only_geom = false;
		el["use_gradient"] = ng;
	}
	if (prop_grad_top_btn) el["gradient_top"] = "#" + prop_grad_top_btn->get_pick_color().to_html(false);
	if (prop_grad_bot_btn) el["gradient_bottom"] = "#" + prop_grad_bot_btn->get_pick_color().to_html(false);
	if (prop_border_w_slider) el["border_width"] = int(Math::round(prop_border_w_slider->get_value()));
	if (prop_border_col_btn) el["border_color"] = "#" + prop_border_col_btn->get_pick_color().to_html(false);
	if (prop_shadow_chk) el["shadow"] = prop_shadow_chk->is_pressed();
	if (prop_glow_chk) el["glow"] = prop_glow_chk->is_pressed();
	if (prop_glow_col_btn) el["glow_color"] = "#" + prop_glow_col_btn->get_pick_color().to_html(false);
	if (prop_clip_chk) el["clip_content"] = prop_clip_chk->is_pressed();
	if (prop_pixel_art_chk) el["pixel_art"] = prop_pixel_art_chk->is_pressed();

	if (prop_text_row->is_visible()) {
		if (etype == "bar") {
			if (String(el.get("label_text", "")) != prop_text_edit->get_text()) only_geom = false;
			el["label_text"] = prop_text_edit->get_text();
		} else {
			if (String(el.get("text", "")) != prop_text_edit->get_text()) only_geom = false;
			el["text"] = prop_text_edit->get_text();
		}
		if (prop_font_opt && prop_font_opt->get_selected() >= 0) {
			el["font_path"] = String(prop_font_opt->get_item_metadata(prop_font_opt->get_selected()));
			only_geom = false;
		}
		if (prop_font_size_slider) {
			el["font_size"] = int(Math::round(prop_font_size_slider->get_value()));
			only_geom = false;
		}
		if (prop_font_color_btn) {
			el["font_color"] = "#" + prop_font_color_btn->get_pick_color().to_html(false);
			only_geom = false;
		}
	}
	if (prop_image_row->is_visible()) {
		String ikey = (etype == "currency" || etype == "item") ? "icon_path" : "image_path";
		String next_img = prop_image_edit->get_text().strip_edges();
		if (String(el.get(ikey, "")) != next_img) only_geom = false;
		el[ikey] = next_img;
	}
	if (prop_var_row->is_visible()) {
		String next_var = prop_var_edit->get_text().strip_edges();
		if (String(el.get("variable_name", "")) != next_var) only_geom = false;
		el["variable_name"] = next_var;
		if (etype == "text" && String(el.get("data_binding", "none")) == "variable") {
			el["text_mode"] = "variable";
		}
	}
	if (etype == "button" && prop_action_row->is_visible()) {
		int next_fo = int(Math::round(prop_focus_spin->get_value()));
		if (int(el.get("focus_order", 0)) != next_fo) only_geom = false;
		el["focus_order"] = next_fo;
		if (prop_vis_mode_opt && prop_vis_sw_edit) {
			Dictionary vc;
			vc["mode"] = String(prop_vis_mode_opt->get_item_metadata(prop_vis_mode_opt->get_selected()));
			vc["switch_name"] = prop_vis_sw_edit->get_text().strip_edges();
			el["visible_condition"] = vc;
		}
		if (prop_en_mode_opt && prop_en_sw_edit) {
			Dictionary ec;
			ec["mode"] = String(prop_en_mode_opt->get_item_metadata(prop_en_mode_opt->get_selected()));
			ec["switch_name"] = prop_en_sw_edit->get_text().strip_edges();
			el["enabled_condition"] = ec;
		}
	}
	dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
	if (only_geom) {
		refresh_geometry_fast(false);
		_refresh_undo_redo_buttons();
		emit_signal("design_modified");
	} else {
		_notify_data_mutated(true);
	}
}

void ClockGearsUiDesignEditor::_on_prop_binding_changed(int p_idx) {
	if (updating_ui || selected_elem_id.is_empty() || !prop_binding_opt || p_idx < 0) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	el["data_binding"] = String(prop_binding_opt->get_item_metadata(p_idx));
	dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_var_opt_selected(int p_idx) {
	if (updating_ui || !prop_var_opt || p_idx < 0) {
		return;
	}
	String vn = String(prop_var_opt->get_item_metadata(p_idx));
	if (!vn.is_empty() && prop_var_edit) {
		prop_var_edit->set_text(vn);
		_on_prop_field_changed();
	}
}

void ClockGearsUiDesignEditor::_on_prop_item_opt_selected(int p_idx) {
	if (updating_ui || selected_elem_id.is_empty() || !prop_item_opt || p_idx < 0) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	el["item_key"] = String(prop_item_opt->get_item_metadata(p_idx));
	dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_action_changed(int p_idx) {
	if (updating_ui || selected_elem_id.is_empty() || !prop_action_opt || p_idx < 0) {
		return;
	}
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	Dictionary el = _get_selected_element_copy();
	if (!dm || el.is_empty()) {
		return;
	}
	el["action"] = String(prop_action_opt->get_item_metadata(p_idx));
	dm->update_ui_element(current_screen_id, selected_elem_id, el, true);
	_notify_data_mutated(true);
}

void ClockGearsUiDesignEditor::_on_prop_import_image_pressed() {
	if (!import_image_dialog) {
		return;
	}
	String proj_dir = ProjectSettings::get_singleton() ? ProjectSettings::get_singleton()->globalize_path("res://") : "";
	if (!proj_dir.is_empty()) {
		import_image_dialog->set_current_dir(proj_dir);
	}
	import_image_dialog->popup_centered_ratio(0.55f);
}

void ClockGearsUiDesignEditor::_on_prop_clear_image_pressed() {
	if (prop_image_edit) {
		prop_image_edit->set_text("");
		_on_prop_field_changed();
	}
}

void ClockGearsUiDesignEditor::_on_image_file_selected(const String &p_path) {
	String final_path = p_path;
	if (owner_db) {
		final_path = owner_db->import_menu_asset_for_ui_designer(p_path, true);
	}
	if (!final_path.is_empty()) {
		cached_thumbs_scanned = false;
		_refresh_image_thumbs_gallery();
		if (image_import_target == 1 && menu_bg_img_edit) {
			menu_bg_img_edit->set_text(final_path);
			_on_menu_global_changed();
		} else if (prop_image_edit) {
			prop_image_edit->set_text(final_path);
			_on_prop_field_changed();
		}
	}
	image_import_target = 0;
}

void ClockGearsUiDesignEditor::_on_test_pressed() {
	open_test_popup();
}

void ClockGearsUiDesignEditor::_on_test_window_close_requested() {
	close_test_popup();
}

void ClockGearsUiDesignEditor::_on_test_window_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventKey> ek = p_event;
	if (ek.is_valid() && ek->is_pressed() && !ek->is_echo() && ek->get_keycode() == Key::ESCAPE) {
		close_test_popup();
	}
}

void ClockGearsUiDesignEditor::close_test_popup() {
	if (test_credits) {
		test_credits->stop_credits(true);
	}
	if (test_window) {
		test_window->hide();
	}
	if (test_injected_sample_state) {
		if (ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton()) {
			dm->import_runtime_state(test_saved_runtime_state);
		}
		test_injected_sample_state = false;
		test_saved_runtime_state.clear();
	}
}

Window *ClockGearsUiDesignEditor::open_test_popup() {
	ClockGearsDataManager *dm = ClockGearsDataManager::get_singleton();
	if (!dm) {
		return nullptr;
	}
	close_test_popup();
	if (test_window) {
		test_window->queue_free();
		test_window = nullptr;
	}
	test_title = nullptr;
	test_gmenu = nullptr;
	test_credits = nullptr;
	test_hud = nullptr;

	// Datos de ejemplo temporales si no hay partida activa (4E)
	if (dm->get_gold() == 0 && dm->get_inventory_entries().is_empty()) {
		test_saved_runtime_state = dm->export_runtime_state();
		test_injected_sample_state = true;
		dm->set_gold(2500);
		Array items = dm->get_category_array(CAT_ITEMS);
		if (!items.is_empty() && items[0].get_type() == Variant::DICTIONARY) {
			String ikey = dm->get_record_inventory_key(CAT_ITEMS, int(Dictionary(items[0]).get("id", 1)));
			dm->change_item(ikey, 3);
		}
	}

	Size2i test_sz(MAX(BASE_W, preview_resolution.x), MAX(BASE_H, preview_resolution.y));
	test_window = memnew(Window);
	test_window->set_name("UiDesignTestPopup");
	test_window->set_title(String::utf8("Probar interfaz (") + itos(test_sz.x) + "x" + itos(test_sz.y) + String::utf8(", Esc para cerrar)"));
	test_window->set_size(test_sz);
	test_window->set_content_scale_size(Size2i(BASE_W, BASE_H));
	test_window->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
	test_window->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_KEEP);
	test_window->set_transient(true);
	test_window->set_exclusive(false);
	test_window->connect("close_requested", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_window_close_requested));
	test_window->connect("window_input", callable_mp(this, &ClockGearsUiDesignEditor::_on_test_window_input));
	add_child(test_window);

	if (current_screen_id == "title") {
		test_title = memnew(ClockGearsTitleScreen);
		test_title->set_editor_test_popup_mode(true);
		test_title->set_auto_quit_tree(false);
		test_title->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		test_window->add_child(test_title);
		test_title->rebuild_title_screen();
	} else if (current_screen_id == "credits") {
		test_credits = memnew(ClockGearsCreditsPlayer);
		test_credits->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		test_window->add_child(test_credits);
		test_credits->start_credits(dm->get_credits_config());
	} else if (current_screen_id == "hud") {
		Control *bg = memnew(UiCanvasHudGridBackdrop);
		bg->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		test_window->add_child(bg);

		test_hud = memnew(ClockGearsHudOverlay);
		test_hud->set_preview_mode(false);
		test_hud->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		test_window->add_child(test_hud);
		test_hud->reload_from_data_manager();
	} else {
		test_gmenu = memnew(ClockGearsGameMenu);
		test_gmenu->set_editor_test_popup_mode(true);
		test_window->add_child(test_gmenu);
		test_gmenu->open_menu_by_id(current_screen_id, true, false, true);
	}

	test_window->popup_centered(Size2i(MIN(test_sz.x, 1280), MIN(test_sz.y, 720)));
	return test_window;
}

Dictionary ClockGearsUiDesignEditor::get_ui_handles() const {
	Dictionary out;
	out["panel"] = this;
	out["screen_opt"] = screen_opt;
	out["new_menu_wizard_btn"] = new_menu_wizard_btn;
	out["templates_btn"] = templates_btn;
	out["menus_list_vbox"] = menus_list_vbox;
	out["menu_wizard_dialog"] = menu_wizard_dialog;
	out["template_gallery_dialog"] = template_gallery_dialog;
	out["props_mode_simple_btn"] = props_mode_simple_btn;
	out["props_mode_adv_btn"] = props_mode_adv_btn;
	out["props_search_edit"] = props_search_edit;
	out["prop_shape_opt"] = prop_shape_opt;
	out["prop_radius_slider"] = prop_radius_slider;
	out["menu_props_box"] = menu_props_box;
	out["zoom_1_btn"] = zoom_btns[0];
	out["zoom_2_btn"] = zoom_btns[1];
	out["zoom_3_btn"] = zoom_btns[2];
	out["undo_btn"] = undo_btn;
	out["redo_btn"] = redo_btn;
	out["reset_btn"] = reset_btn;
	out["reset_confirm_dialog"] = reset_confirm_dialog;
	out["test_btn"] = test_btn;
	out["screen_status_lbl"] = screen_status_lbl;
	out["canvas_scroll"] = canvas_scroll;
	out["canvas_stage"] = canvas_stage;
	out["design_layer"] = design_layer;
	out["canvas_overlay"] = canvas_overlay;
	out["layers_list_vbox"] = layers_list_vbox;
	out["layer_up_btn"] = layer_up_btn;
	out["layer_down_btn"] = layer_down_btn;
	out["layer_del_btn"] = layer_del_btn;
	out["prop_name_edit"] = prop_name_edit;
	out["prop_ox_spin"] = prop_ox_spin;
	out["prop_oy_spin"] = prop_oy_spin;
	out["prop_w_spin"] = prop_w_spin;
	out["prop_h_spin"] = prop_h_spin;
	out["prop_layer_spin"] = prop_layer_spin;
	out["prop_visible_chk"] = prop_visible_chk;
	out["prop_style_opt"] = prop_style_opt;
	out["prop_text_edit"] = prop_text_edit;
	out["prop_image_edit"] = prop_image_edit;
	out["prop_binding_opt"] = prop_binding_opt;
	out["prop_var_edit"] = prop_var_edit;
	out["prop_item_opt"] = prop_item_opt;
	out["prop_action_opt"] = prop_action_opt;
	out["prop_focus_spin"] = prop_focus_spin;

	Array anc_arr;
	for (int i = 0; i < 9; i++) {
		anc_arr.push_back(anchor_btns[i]);
	}
	out["anchor_btns"] = anc_arr;

	Dictionary pal;
	for (int i = 0; i < palette_buttons.size(); i++) {
		pal[palette_buttons[i]->get_element_type()] = palette_buttons[i];
	}
	out["palette_btns"] = pal;
	out["preview_res_opt"] = preview_res_opt;
	out["compact_mode"] = compact_two_row_layout;
	return out;
}

Dictionary ClockGearsUiDesignEditor::get_test_popup_info() const {
	Dictionary out;
	out["window"] = test_window;
	out["open"] = (test_window != nullptr && test_window->is_visible());
	out["screen_id"] = current_screen_id;
	out["title"] = test_title;
	out["game_menu"] = test_gmenu;
	out["credits"] = test_credits;
	out["hud"] = test_hud;
	return out;
}
